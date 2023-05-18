#include "pch.h"
#include "terrain.h"
#include "corecontext.h"
#include "gl/glew.h"
#include "lodepng/lodepng.h"

using namespace std::chrono;

#define ASSERT(x) if(!(x)) __debugbreak;
#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x, __FILE__, __LINE__))

static void GLClearError() {

	while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* function, const char* file, int line) {

	while (GLenum error = glGetError()) {

		std::cout << "[OpenGL Error] (" << error << "): " << function << " " << file << ":" << line << std::endl;
		return false;
	}
	return true;
}

namespace Core {

	Terrain::Terrain() {

		std::srand((unsigned)time(0));
	}

	Terrain::~Terrain() {

		glDeleteTextures(1, &elevationMapTexture);
		glDeleteVertexArrays(1, &blockVAO);
		glDeleteVertexArrays(1, &ringFixUpVAO);
		glDeleteVertexArrays(1, &smallSquareVAO);
		glDeleteVertexArrays(1, &outerDegenerateVAO);
		glDeleteVertexArrays(1, &interiorTrimVAO);

		delete[] blockPositions;
		delete[] ringFixUpPositions;
		delete[] interiorTrimPositions;
		delete[] outerDegeneratePositions;
		delete[] rotAmounts;
		delete[] blockAABBs;

		Terrain::deleteHeightmapArray(heights);

		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		for (int i = 0; i < lodLevel; i++)
			delete[] mipStack[i];

		delete[] mipStack;
	}

	void Terrain::start() {

		lightDir = glm::vec3(-0.5f, -1.f, -0.5f);
		fogColor = glm::vec3(190.f / 255, 220.f / 255, 1.f);

		glm::vec3 camPos = CoreContext::instance->scene->cameraInfo.camPos;
		camPos = glm::clamp(camPos, glm::vec3(8193, 0, 8193), glm::vec3(12287, 0, 12287));

		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);

		//// create mipmaps
		//std::string path = "terrain.png";
		//std::vector<unsigned char> out;
		//unsigned int w, h;
		//lodepng::decode(out, w, h, path, LodePNGColorType::LCT_GREY, 16);
		//unsigned char* data = new unsigned char[w * w * 2];
		//for (int i = 0; i < out.size(); i++)
		//	data[i] = out[i];

		//unsigned char* heightmap = Terrain::remapHeightmap(data, w);
		//unsigned char** heightMapList = Terrain::createMipmaps(heightmap, w * 4, lodLevel);
		//Terrain::divideTerrainHeightmaps(heightMapList, w * 4, lodLevel);

		blockPositions = new glm::vec2[12 * lodLevel + 4];
		ringFixUpPositions = new glm::vec2[4 * lodLevel + 4];
		interiorTrimPositions = new glm::vec2[lodLevel];
		outerDegeneratePositions = new glm::vec2[4 * lodLevel];
		rotAmounts = new float[lodLevel];
		blockAABBs = new AABB_Box[12 * lodLevel + 4];

		heights = new unsigned char* [lodLevel];
		for (int i = 0; i < lodLevel; i++)
			heights[i] = new unsigned char[MEM_TILE_ONE_SIDE * MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * 2];

		mipStack = new unsigned char* [lodLevel];
		for (int i = 0; i < lodLevel; i++)
			mipStack[i] = new unsigned char[MIP_STACK_SIZE * MIP_STACK_SIZE];

		Terrain::generateTerrainClipmapsVertexArrays();

		programID = Shader::loadShaders("resources/shaders/terrain/terrain.vert", "resources/shaders/terrain/terrain.frag");
		glUseProgram(programID);
		glUniform1i(glGetUniformLocation(programID, "irradianceMap"), 0);
		glUniform1i(glGetUniformLocation(programID, "prefilterMap"), 1);
		glUniform1i(glGetUniformLocation(programID, "brdfLUT"), 2);
		glUniform1i(glGetUniformLocation(programID, "heightmapArray"), 3);
		glUniform1i(glGetUniformLocation(programID, "macroTexture"), 4);
		glUniform1i(glGetUniformLocation(programID, "noiseTexture"), 5);
					
		glUniform1i(glGetUniformLocation(programID, "albedoT0"), 7);
		glUniform1i(glGetUniformLocation(programID, "albedoT1"), 8);
		glUniform1i(glGetUniformLocation(programID, "albedoT2"), 9);
		glUniform1i(glGetUniformLocation(programID, "albedoT3"), 10);
		glUniform1i(glGetUniformLocation(programID, "albedoT4"), 11);
		glUniform1i(glGetUniformLocation(programID, "albedoT5"), 12);
		glUniform1i(glGetUniformLocation(programID, "albedoT6"), 13);
					
		glUniform1i(glGetUniformLocation(programID, "normalT0"), 14);
		glUniform1i(glGetUniformLocation(programID, "normalT1"), 15);
		glUniform1i(glGetUniformLocation(programID, "normalT2"), 16);
		glUniform1i(glGetUniformLocation(programID, "normalT3"), 17);
		glUniform1i(glGetUniformLocation(programID, "normalT4"), 18);
		glUniform1i(glGetUniformLocation(programID, "normalT5"), 19);
		glUniform1i(glGetUniformLocation(programID, "normalT6"), 20);


		cameraPosition = camPos;
		Terrain::loadTerrainHeightmapOnInit(cameraPosition, lodLevel);

		Terrain::calculateBlockPositions(cameraPosition, lodLevel);

		for (int i = 0; i < lodLevel; i++)
			for (int j = 0; j < 12; j++)
				blockAABBs[12 * i + j] = Terrain::getBoundingBoxOfClipmap(j, i);

		for (int i = 0; i < 4; i++)
			blockAABBs[12 * lodLevel + i] = Terrain::getBoundingBoxOfClipmap1(i, lodLevel);

		//Terrain::createAlbedoMapTextureArray();
	}

	void Terrain::loadTerrainHeightmapOnInit(glm::vec3 camPos, int clipmapLevel) {

		unsigned char** terrainStack = new unsigned char* [clipmapLevel];
		for (int i = 0; i < clipmapLevel; i++)
			terrainStack[i] = new unsigned char[MEM_TILE_ONE_SIDE * MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * 4];

		for (int level = 0; level < clipmapLevel; level++)
			Terrain::loadHeightmapAtLevel(level, camPos, terrainStack[level]);

		Terrain::createElevationMapTextureArray(terrainStack);

		for (int i = 0; i < clipmapLevel; i++)
			delete[] terrainStack[i];
		delete[] terrainStack;
	}

	void Terrain::loadHeightmapAtLevel(int level, glm::vec3 camPos, unsigned char* heightData) {

		glm::ivec2 tileIndex = Terrain::getTileIndex(level, camPos);
		glm::ivec2 tileStart = tileIndex - MEM_TILE_ONE_SIDE / 2;
		glm::ivec2 border = tileStart % MEM_TILE_ONE_SIDE;

		std::vector<std::thread> pool;

		for (int i = 0; i < MEM_TILE_ONE_SIDE; i++) {

			int startX = tileStart.x;

			for (int j = 0; j < MEM_TILE_ONE_SIDE; j++) {

				glm::ivec2 tileCoordinates(startX, tileStart.y);

				std::thread th(&Terrain::loadFromDiscAndWriteGPUBufferAsync, this, level, TILE_SIZE * MEM_TILE_ONE_SIDE, glm::ivec2(startX, tileStart.y), border, heightData, border);
				pool.push_back(std::move(th));

				border.x++;
				border.x %= MEM_TILE_ONE_SIDE;
				startX++;
			}

			border.y++;
			border.y %= MEM_TILE_ONE_SIDE;
			tileStart.y++;
		}

		for (auto& it : pool)
			if (it.joinable())
				it.join();

		// DEBUG
		//std::vector<unsigned char> out(TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2);
		//for (int i = 0; i < TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2; i++)
		//	out[i] = heights[level][i];

		//unsigned int width = TILE_SIZE * MEM_TILE_ONE_SIDE;
		//unsigned int height = TILE_SIZE * MEM_TILE_ONE_SIDE;
		//std::string imagePath = "heights" + std::to_string(level) + ".png";
		//TextureFile::encodeTextureFile(width, height, out, &imagePath[0]);
	}

	void Terrain::update(glm::vec3 camPos, float dt) {

		int level = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		glm::vec3 camPosition = CoreContext::instance->scene->cameraInfo.camPos;
		camPosition = glm::clamp(camPosition, glm::vec3(8193, 0, 8193), glm::vec3(12287, 0, 12287));
		//camPosition = glm::clamp(camPosition, glm::vec3(4100, 0, 4100), glm::vec3(6100, 0, 6100));

		Terrain::calculateBlockPositions(camPosition, level);
		Terrain::streamTerrain(camPosition, level);
	}

	void Terrain::onDraw(glm::mat4& pv, glm::vec3& pos) {

		int level = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		int programID = this->programID;

		float* lightDirAddr = &glm::normalize(-glm::vec3(0.5, -1, 0.5))[0];

		glm::vec3 test = CoreContext::instance->scene->cameraInfo.camPos;

		int blockVAO = this->blockVAO;
		int ringFixUpVAO = this->ringFixUpVAO;
		int smallSquareVAO = this->smallSquareVAO;
		int interiorTrimVAO = this->interiorTrimVAO;
		int outerDegenerateVAO = this->outerDegenerateVAO;

		int blockIndiceCount = blockIndices.size();
		int ringFixUpIndiceCount = ringFixUpIndices.size();
		int smallSquareIndiceCount = smallSquareIndices.size();
		int interiorTrimIndiceCount = interiorTrimIndices.size();
		int outerDegenerateIndiceCount = outerDegenerateIndices.size();

		Cubemap* cubemap = CoreContext::instance->scene->cubemap;

		glUseProgram(programID);
		glUniformMatrix4fv(glGetUniformLocation(programID, "PV"), 1, 0, &pv[0][0]);
		glUniform3fv(glGetUniformLocation(programID, "camPos"), 1, &pos[0]);
		glUniform3fv(glGetUniformLocation(programID, "camPoss"), 1, &test[0]);
		glUniform1f(glGetUniformLocation(programID, "texSize"), (float)TILE_SIZE * MEM_TILE_ONE_SIDE);

		// Terrain Material Parameters
		glUniform1f(glGetUniformLocation(programID, "heightScale"), -displacementMapScale);
		glUniform3f(glGetUniformLocation(programID, "lightDirection"), lightDir.x, lightDir.y, lightDir.z);
					
		glUniform1f(glGetUniformLocation(programID, "ambientAmount"), ambientAmount);
		glUniform1f(glGetUniformLocation(programID, "specularPower"), specularPower);
		glUniform1f(glGetUniformLocation(programID, "specularAmount"), specularAmount);
					
		glUniform1f(glGetUniformLocation(programID, "blendDistance0"), blendDistance0);
		glUniform1f(glGetUniformLocation(programID, "blendAmount0"), blendAmount0);
		glUniform1f(glGetUniformLocation(programID, "blendDistance1"), blendDistance1);
		glUniform1f(glGetUniformLocation(programID, "blendAmount1"), blendAmount1);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color0_dist0"), scale_color0_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color0_dist1"), scale_color0_dist1);
		glUniform1f(glGetUniformLocation(programID, "scale_color0_dist2"), scale_color0_dist2);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color1_dist0"), scale_color1_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color1_dist1"), scale_color1_dist1);
		glUniform1f(glGetUniformLocation(programID, "scale_color1_dist2"), scale_color1_dist2);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color2_dist0"), scale_color2_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color2_dist1"), scale_color2_dist1);
		glUniform1f(glGetUniformLocation(programID, "scale_color2_dist2"), scale_color2_dist2);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color3_dist0"), scale_color3_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color3_dist1"), scale_color3_dist1);
		glUniform1f(glGetUniformLocation(programID, "scale_color3_dist2"), scale_color3_dist2);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color4_dist0"), scale_color4_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color4_dist1"), scale_color4_dist1);
		glUniform1f(glGetUniformLocation(programID, "scale_color4_dist2"), scale_color4_dist2);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color5_dist0"), scale_color5_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color5_dist1"), scale_color5_dist1);
		glUniform1f(glGetUniformLocation(programID, "scale_color5_dist2"), scale_color5_dist2);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color6_dist0"), scale_color6_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color6_dist1"), scale_color6_dist1);
		glUniform1f(glGetUniformLocation(programID, "scale_color6_dist2"), scale_color6_dist2);
					
		glUniform1f(glGetUniformLocation(programID, "macroScale_0"), macroScale_0);
		glUniform1f(glGetUniformLocation(programID, "macroScale_1"), macroScale_1);
		glUniform1f(glGetUniformLocation(programID, "macroScale_2"), macroScale_2);
		glUniform1f(glGetUniformLocation(programID, "macroAmount"), macroAmount);
		glUniform1f(glGetUniformLocation(programID, "macroPower"), macroPower);
		glUniform1f(glGetUniformLocation(programID, "macroOpacity"), macroOpacity);
					
		glUniform1f(glGetUniformLocation(programID, "overlayBlendScale0"), overlayBlendScale0);
		glUniform1f(glGetUniformLocation(programID, "overlayBlendAmount0"), overlayBlendAmount0);
		glUniform1f(glGetUniformLocation(programID, "overlayBlendPower0"), overlayBlendPower0);
		glUniform1f(glGetUniformLocation(programID, "overlayBlendOpacity0"), overlayBlendOpacity0);
					
		glUniform1f(glGetUniformLocation(programID, "overlayBlendScale1"), overlayBlendScale1);
		glUniform1f(glGetUniformLocation(programID, "overlayBlendAmount1"), overlayBlendAmount1);
		glUniform1f(glGetUniformLocation(programID, "overlayBlendPower1"), overlayBlendPower1);
		glUniform1f(glGetUniformLocation(programID, "overlayBlendOpacity1"), overlayBlendOpacity1);
					
		glUniform1f(glGetUniformLocation(programID, "slopeSharpness0"), slopeSharpness0);
		glUniform1f(glGetUniformLocation(programID, "slopeSharpness1"), slopeSharpness1);
		glUniform1f(glGetUniformLocation(programID, "slopeSharpness2"), slopeSharpness2);
					
		glUniform1f(glGetUniformLocation(programID, "slopeBias0"), slopeBias0);
		glUniform1f(glGetUniformLocation(programID, "slopeBias1"), slopeBias1);
		glUniform1f(glGetUniformLocation(programID, "slopeBias2"), slopeBias2);
					
		glUniform1f(glGetUniformLocation(programID, "heightBias0"), heightBias0);
		glUniform1f(glGetUniformLocation(programID, "heightSharpness0"), heightSharpness0);
		glUniform1f(glGetUniformLocation(programID, "heightBias1"), heightBias1);
		glUniform1f(glGetUniformLocation(programID, "heightSharpness1"), heightSharpness1);
					
		glUniform1f(glGetUniformLocation(programID, "distanceNear"), distanceNear);
		glUniform1f(glGetUniformLocation(programID, "fogBlendDistance"), fogBlendDistance);
		glUniform1f(glGetUniformLocation(programID, "maxFog"), maxFog);
		glUniform3fv(glGetUniformLocation(programID, "fogColor"), 1, &fogColor[0]);

		// bind pre-computed IBL data
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->irradianceMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->prefilterMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, cubemap->brdfLUTTexture);
		
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D_ARRAY, elevationMapTexture);
		
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, macroTexture);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, albedo0);
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, albedo1);
		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_2D, albedo2);
		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D, albedo3);
		glActiveTexture(GL_TEXTURE11);
		glBindTexture(GL_TEXTURE_2D, albedo4);
		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D, albedo5);
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_2D, albedo6);
		
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_2D, normal0);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, normal1);
		glActiveTexture(GL_TEXTURE16);
		glBindTexture(GL_TEXTURE_2D, normal2);
		glActiveTexture(GL_TEXTURE17);
		glBindTexture(GL_TEXTURE_2D, normal3);
		glActiveTexture(GL_TEXTURE18);
		glBindTexture(GL_TEXTURE_2D, normal4);
		glActiveTexture(GL_TEXTURE19);
		glBindTexture(GL_TEXTURE_2D, normal5);
		glActiveTexture(GL_TEXTURE20);
		glBindTexture(GL_TEXTURE_2D, normal6);

		glm::vec2* blockPositions = this->blockPositions;
		glm::vec2* ringFixUpPositions = this->ringFixUpPositions;
		glm::vec2* interiorTrimPositions = this->interiorTrimPositions;
		glm::vec2* outerDegeneratePositions = this->outerDegeneratePositions;
		float* rotAmounts = this->rotAmounts;
		glm::vec2 smallSquarePosition = this->smallSquarePosition;

		std::vector<TerrainVertexAttribs> instanceArray;

		// BLOCKS

		for (int i = 0; i < level; i++) {

			for (int j = 0; j < 12; j++) {

				glm::vec4 startInWorldSpace;
				glm::vec4 endInWorldSpace;
				AABB_Box aabb = blockAABBs[i * 12 + j];
				startInWorldSpace = aabb.start;
				endInWorldSpace = aabb.end;
				//if (camera->intersectsAABB(startInWorldSpace, endInWorldSpace)) {

				TerrainVertexAttribs attribs;
				attribs.level = i;
				attribs.model = glm::mat4(1);
				attribs.position = glm::vec2(blockPositions[i * 12 + j].x, blockPositions[i * 12 + j].y);
				glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(i, test);
				attribs.clipmapcenter = clipmapcenter;
				instanceArray.push_back(attribs);
				//}
			}
		}

		for (int i = 0; i < 4; i++) {

			glm::vec4 startInWorldSpace;
			glm::vec4 endInWorldSpace;
			AABB_Box aabb = blockAABBs[level * 12 + i];
			startInWorldSpace = aabb.start;
			endInWorldSpace = aabb.end;
			//if (camera->intersectsAABB(startInWorldSpace, endInWorldSpace)) {

			TerrainVertexAttribs attribs;
			attribs.level = 0;
			attribs.model = glm::mat4(1);
			attribs.position = glm::vec2(blockPositions[level * 12 + i].x, blockPositions[level * 12 + i].y);
			glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(0, test);
			attribs.clipmapcenter = clipmapcenter;
			instanceArray.push_back(attribs);
			//}
		}

		int size = sizeof(TerrainVertexAttribs);

		if (instanceArray.size()) {
			Terrain::drawElementsInstanced(size, blockVAO, instanceArray, blockIndiceCount);
			instanceArray.clear();
		}

		// RING FIXUP
		for (int i = 0; i < level; i++) {

			glm::mat4 model = glm::mat4(1);

			TerrainVertexAttribs attribs;
			attribs.level = i;
			attribs.model = model;
			glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(i, test);
			attribs.clipmapcenter = clipmapcenter;

			attribs.position = glm::vec2(ringFixUpPositions[i * 4 + 0].x, ringFixUpPositions[i * 4 + 0].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(ringFixUpPositions[i * 4 + 2].x, ringFixUpPositions[i * 4 + 2].y);
			instanceArray.push_back(attribs);

			model = glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
			attribs.model = model;

			attribs.position = glm::vec2(ringFixUpPositions[i * 4 + 1].x, ringFixUpPositions[i * 4 + 1].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(ringFixUpPositions[i * 4 + 3].x, ringFixUpPositions[i * 4 + 3].y);
			instanceArray.push_back(attribs);
		}

		{

			TerrainVertexAttribs attribs;
			attribs.level = 0;
			attribs.model = glm::mat4(1);
			glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(0, test);
			attribs.clipmapcenter = clipmapcenter;

			attribs.position = glm::vec2(ringFixUpPositions[level * 4 + 0].x, ringFixUpPositions[level * 4 + 0].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(ringFixUpPositions[level * 4 + 2].x, ringFixUpPositions[level * 4 + 2].y);
			instanceArray.push_back(attribs);

			glm::mat4 model = glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
			attribs.model = model;

			attribs.position = glm::vec2(ringFixUpPositions[level * 4 + 1].x, ringFixUpPositions[level * 4 + 1].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(ringFixUpPositions[level * 4 + 3].x, ringFixUpPositions[level * 4 + 3].y);
			instanceArray.push_back(attribs);
		}

		Terrain::drawElementsInstanced(size, ringFixUpVAO, instanceArray, ringFixUpIndiceCount);
		instanceArray.clear();

		// INTERIOR TRIM
		for (int i = 0; i < level - 1; i++) {

			glm::mat4 model = glm::rotate(glm::mat4(1), glm::radians(rotAmounts[i]), glm::vec3(0.0f, 1.0f, 0.0f));

			TerrainVertexAttribs attribs;
			glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(i + 1, test);
			attribs.clipmapcenter = clipmapcenter;
			attribs.level = i + 1;
			attribs.model = model;
			attribs.position = glm::vec2(interiorTrimPositions[i].x, interiorTrimPositions[i].y);
			instanceArray.push_back(attribs);
		}
		Terrain::drawElementsInstanced(size, interiorTrimVAO, instanceArray, interiorTrimIndiceCount);
		instanceArray.clear();

		// OUTER DEGENERATE
		for (int i = 0; i < level; i++) {

			glm::mat4 model = glm::mat4(1);

			TerrainVertexAttribs attribs;
			attribs.level = i;
			attribs.model = model;
			glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(i, test);
			attribs.clipmapcenter = clipmapcenter;

			attribs.position = glm::vec2(outerDegeneratePositions[i * 4 + 0].x, outerDegeneratePositions[i * 4 + 0].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(outerDegeneratePositions[i * 4 + 1].x, outerDegeneratePositions[i * 4 + 1].y);
			instanceArray.push_back(attribs);

			model = glm::rotate(glm::mat4(1), glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f));
			attribs.model = model;

			attribs.position = glm::vec2(outerDegeneratePositions[i * 4 + 2].x, outerDegeneratePositions[i * 4 + 2].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(outerDegeneratePositions[i * 4 + 3].x, outerDegeneratePositions[i * 4 + 3].y);
			instanceArray.push_back(attribs);
		}
		Terrain::drawElementsInstanced(size, outerDegenerateVAO, instanceArray, outerDegenerateIndiceCount);
		instanceArray.clear();

		// SMALL SQUARE
		TerrainVertexAttribs attribs;
		glm::ivec2 clipmapcenter = Terrain::getClipmapPosition(0, test);
		attribs.clipmapcenter = clipmapcenter;
		attribs.level = 0;
		attribs.model = glm::mat4(1);
		attribs.position = glm::vec2(smallSquarePosition.x, smallSquarePosition.y);
		instanceArray.push_back(attribs);
		Terrain::drawElementsInstanced(size, smallSquareVAO, instanceArray, smallSquareIndiceCount);
	}

	void Terrain::drawElementsInstanced(int size, unsigned int VAO, std::vector<TerrainVertexAttribs>& instanceArray, unsigned int indiceCount) {

		unsigned int instanceBuffer;
		glGenBuffers(1, &instanceBuffer);
		glBindBuffer(0x8892, instanceBuffer);
		glBufferData(0x8892, instanceArray.size() * sizeof(TerrainVertexAttribs), &instanceArray[0], 0x88E4);

		glBindVertexArray(VAO);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, size, (void*)0);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2)));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2));
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2 + sizeof(float)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2 + sizeof(float) + sizeof(glm::vec4)));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2 + sizeof(float) + sizeof(glm::vec4) * 2));
		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, size, (void*)(sizeof(glm::vec2) * 2 + sizeof(float) + sizeof(glm::vec4) * 3));

		glVertexAttribDivisor(1, 1);
		glVertexAttribDivisor(2, 1);
		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);

		glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(indiceCount), GL_UNSIGNED_INT, 0, instanceArray.size());

		glBindVertexArray(0);
		glDeleteBuffers(1, &instanceBuffer);
	}

	void Terrain::calculateBlockPositions(glm::vec3 camPosition, int level) {

		// Change this for debugging purposes--------
		//glm::vec3 camPos = camera->position;
		float fake = 1000000;
		glm::vec3 fakeDisplacement = glm::vec3(fake, 0, fake);
		glm::vec3 camPos = camPosition + fakeDisplacement;
		// '4' has to be constant, because every level has 4 block at each side.
		//int wholeClipmapRegionSize = clipmapResolution * 4 * (1 << level);
		// It has to be two. 
		int patchWidth = 2;
		int clipmapResolution = this->clipmapResolution;

		for (int i = 0; i < level; i++) {

			/*
			*         Z+
			*         ^
			*         |  This is our reference for numbers
			*         |
			* x+ <-----
			*/

			// Blocks move periodically according to camera's position.
			// For example:
			// Cam pos X       : 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ...
			// Block at level 0: 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10 ...
			// Block at level 1: 0, 0, 0, 0, 4, 4, 4, 4, 8, 8, 8  ...
			// Block at level 2: 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8  ...

			float requiredCameraDisplacement = patchWidth * (1 << i);
			float posX = (int)(camPos.x / requiredCameraDisplacement) * requiredCameraDisplacement;
			float posZ = (int)(camPos.z / requiredCameraDisplacement) * requiredCameraDisplacement;

			float patchTranslation = (1 << i) * clipmapResolution;
			float patchOffset = 1 << i;

			// For outer trim rotation. This '2' at the end is constant. Because of binary movement style
			int rotX = (int)(posX / requiredCameraDisplacement) % 2;
			int rotZ = (int)(posZ / requiredCameraDisplacement) % 2;

			// aciklama eklencek
			int parcelSizeInReal = TILE_SIZE * (1 << i);

			//// BLOCKS

			/*
			*  0 11 10  9
			*  1        8
			*  2        7
			*  3  4  5  6
			*/

			// 11. x, 1. z

			// 0
			glm::vec2 position(patchTranslation - fake + posX, patchTranslation - fake + posZ);
			position.x += patchOffset;
			position.y += patchOffset;
			blockPositions[i * 12 + 0] = position;

			// 1
			position.y += patchOffset - patchTranslation;
			blockPositions[i * 12 + 1] = position;
			position.y -= patchOffset;

			// 2
			position.y -= patchTranslation;
			blockPositions[i * 12 + 2] = position;
			position.y += patchOffset;

			// 3
			position.y -= patchTranslation;
			blockPositions[i * 12 + 3] = position;
			position.x += patchOffset;

			// 4
			position.x -= patchTranslation;
			blockPositions[i * 12 + 4] = position;

			// 5
			position.x -= patchTranslation + patchOffset;
			blockPositions[i * 12 + 5] = position;
			position.x += patchOffset;

			// 6
			position.x -= patchTranslation;
			blockPositions[i * 12 + 6] = position;
			position.y -= patchOffset;

			// 7
			position.y += patchTranslation;
			blockPositions[i * 12 + 7] = position;

			// 8
			position.y += patchTranslation + patchOffset;
			blockPositions[i * 12 + 8] = position;
			position.y -= patchOffset;

			// 9
			position.y += patchTranslation;
			blockPositions[i * 12 + 9] = position;

			// 10
			position.x += patchTranslation - patchOffset;
			blockPositions[i * 12 + 10] = position;
			position.x += patchOffset;

			// 11
			position.x += patchTranslation;
			blockPositions[i * 12 + 11] = position;

			// RING FIX-UP

			/*
			*    0
			*  1   3
			*    2
			*/

			// 0
			position = glm::vec2(-fake + posX, patchTranslation + patchOffset - fake + posZ);
			ringFixUpPositions[i * 4 + 0] = position;

			// 2
			position = glm::vec2(-fake + posX, (patchOffset - patchTranslation) * 2 - fake + posZ);
			ringFixUpPositions[i * 4 + 2] = position;

			// 1
			position = glm::vec2(patchTranslation + patchOffset - fake + posX, patchOffset * 2 - fake + posZ);
			ringFixUpPositions[i * 4 + 1] = position;

			// 3
			position = glm::vec2((patchOffset - patchTranslation) * 2 - fake + posX, patchOffset * 2 - fake + posZ);
			ringFixUpPositions[i * 4 + 3] = position;

			// INTERIOR TRIM

			position = glm::vec2(patchOffset * 2 * (1 - rotX) - fake + posX, patchOffset * 2 * (1 - rotZ) - fake + posZ);
			interiorTrimPositions[i] = position;

			if (rotX == 0 && rotZ == 0)
				rotAmounts[i] = 0.f;
			if (rotX == 0 && rotZ == 1)
				rotAmounts[i] = 90.f;
			if (rotX == 1 && rotZ == 0)
				rotAmounts[i] = 270.f;
			if (rotX == 1 && rotZ == 1)
				rotAmounts[i] = 180.f;

			// OUTER DEGENERATE

			// bottom (0) 
			position = glm::vec2((patchOffset - patchTranslation) * 2 - fake + posX, (patchOffset - patchTranslation) * 2 - fake + posZ);
			outerDegeneratePositions[i * 4 + 0] = position;

			// top (1)
			position = glm::vec2((patchOffset - patchTranslation) * 2 - fake + posX, (patchTranslation) * 2 - fake + posZ);
			outerDegeneratePositions[i * 4 + 1] = position;

			// right (2)
			position = glm::vec2((patchOffset - patchTranslation) * 2 - fake + posX, (patchOffset - patchTranslation) * 2 - fake + posZ);
			outerDegeneratePositions[i * 4 + 2] = position;

			// left (3)
			position = glm::vec2((patchTranslation) * 2 - fake + posX, (patchOffset - patchTranslation) * 2 - fake + posZ);
			outerDegeneratePositions[i * 4 + 3] = position;
		}

		float posX = (int)(camPos.x / 2) * 2;
		float posZ = (int)(camPos.z / 2) * 2;
		glm::vec2 position(2 - fakeDisplacement.x + posX, 2 - fakeDisplacement.z + posZ);

		// 0
		blockPositions[level * 12 + 0] = position;

		// 1
		position.y -= clipmapResolution + 1;
		blockPositions[level * 12 + 1] = position;

		// 2
		position.x -= clipmapResolution + 1;
		blockPositions[level * 12 + 2] = position;

		// 3
		position.y += clipmapResolution + 1;
		blockPositions[level * 12 + 3] = position;

		//
		//0
		position = glm::vec2(0 - fakeDisplacement.x + posX, 2 - fakeDisplacement.z + posZ);
		ringFixUpPositions[level * 4 + 0] = position;

		// 2
		position = glm::vec2(0 - fakeDisplacement.x + posX, 1 - clipmapResolution - fakeDisplacement.z + posZ);
		ringFixUpPositions[level * 4 + 2] = position;

		// 1
		position = glm::vec2(2 - fakeDisplacement.x + posX, 2 - fakeDisplacement.z + posZ);
		ringFixUpPositions[level * 4 + 1] = position;

		// 3
		position = glm::vec2(1 - clipmapResolution - fakeDisplacement.x + posX, 2 - fakeDisplacement.z + posZ);
		ringFixUpPositions[level * 4 + 3] = position;

		//
		smallSquarePosition = glm::vec2(0 - fakeDisplacement.x + posX, 0 - fakeDisplacement.z + posZ);
	}

	AABB_Box Terrain::getBoundingBoxOfClipmap(int clipmapIndex, int level) {

		glm::ivec2 blockPositionInWorldSpace = blockPositions[level * 12 + clipmapIndex];
		glm::ivec2 blockPositionInMipStack = (blockPositionInWorldSpace / MIP_STACK_DIVISOR_RATIO) % MIP_STACK_SIZE;
		int blockSizeInMipStack = clipmapResolution / MIP_STACK_DIVISOR_RATIO;
		int blockSizeInWorldSpace = clipmapResolution * (1 << level);

		unsigned char min = 255;
		unsigned char max = 0;

		int z = blockPositionInMipStack.y;
		int x = blockPositionInMipStack.x;

		for (int i = 0; i < blockSizeInMipStack; i++) {
			for (int j = 0; j < blockSizeInMipStack; j++) {

				if (mipStack[level][z * MIP_STACK_SIZE + x] < min)
					min = mipStack[level][z * MIP_STACK_SIZE + x];

				if (mipStack[level][z * MIP_STACK_SIZE + x] > max)
					max = mipStack[level][z * MIP_STACK_SIZE + x];

				z++;
				z %= MIP_STACK_SIZE;

				x++;
				x %= MIP_STACK_SIZE;
			}
		}

		glm::vec4 corner0(blockPositionInWorldSpace.x, min, blockPositionInWorldSpace.y, 1);
		glm::vec4 corner1(blockPositionInWorldSpace.x + blockSizeInWorldSpace, max, blockPositionInWorldSpace.y + blockSizeInWorldSpace, 1);
		AABB_Box boundingBox;
		boundingBox.start = corner0;
		boundingBox.end = corner1;
		return boundingBox;
	}

	AABB_Box Terrain::getBoundingBoxOfClipmap1(int clipmapIndex, int totalLevel) {

		glm::ivec2 blockPositionInWorldSpace = blockPositions[totalLevel * 12 + clipmapIndex];
		glm::ivec2 blockPositionInMipStack = (blockPositionInWorldSpace / MIP_STACK_DIVISOR_RATIO) % MIP_STACK_SIZE;
		int blockSizeInMipStack = clipmapResolution / MIP_STACK_DIVISOR_RATIO;
		int blockSizeInWorldSpace = clipmapResolution;

		unsigned char min = 255;
		unsigned char max = 0;

		int z = blockPositionInMipStack.y;
		int x = blockPositionInMipStack.x;

		for (int i = 0; i < blockSizeInMipStack; i++) {
			for (int j = 0; j < blockSizeInMipStack; j++) {

				if (mipStack[0][z * MIP_STACK_SIZE + x] < min)
					min = mipStack[0][z * MIP_STACK_SIZE + x];

				if (mipStack[0][z * MIP_STACK_SIZE + x] > max)
					max = mipStack[0][z * MIP_STACK_SIZE + x];

				z++;
				z %= MIP_STACK_SIZE;

				x++;
				x %= MIP_STACK_SIZE;
			}
		}

		glm::vec4 corner0(blockPositionInWorldSpace.x, min, blockPositionInWorldSpace.y, 1);
		glm::vec4 corner1(blockPositionInWorldSpace.x + blockSizeInWorldSpace, max, blockPositionInWorldSpace.y + blockSizeInWorldSpace, 1);
		AABB_Box boundingBox;
		boundingBox.start = corner0;
		boundingBox.end = corner1;
		return boundingBox;
	}

	void Terrain::streamTerrain(glm::vec3 newCamPos, int clipmapLevel) {

		for (int level = 0; level < clipmapLevel; level++) {

			glm::ivec2 old_tileIndex = Terrain::getTileIndex(level, cameraPosition);
			glm::ivec2 old_tileStart = old_tileIndex - MEM_TILE_ONE_SIDE / 2;
			glm::ivec2 old_border = old_tileStart % MEM_TILE_ONE_SIDE;
			glm::ivec2 old_clipmapPos = Terrain::getClipmapPosition(clipmapLevel, cameraPosition);

			glm::ivec2 new_tileIndex = Terrain::getTileIndex(level, newCamPos);
			glm::ivec2 new_tileStart = new_tileIndex - MEM_TILE_ONE_SIDE / 2;
			glm::ivec2 new_border = new_tileStart % MEM_TILE_ONE_SIDE;
			glm::ivec2 new_clipmapPos = Terrain::getClipmapPosition(clipmapLevel, newCamPos);

			if (old_clipmapPos != new_clipmapPos) {

				if (level == 0) {
					for (int i = 0; i < 4; i++)
						blockAABBs[clipmapLevel * 12 + i] = Terrain::getBoundingBoxOfClipmap1(i, clipmapLevel);
				}
				for (int i = 0; i < 12; i++)
					blockAABBs[level * 12 + i] = Terrain::getBoundingBoxOfClipmap(i, level);
			}

			glm::ivec2 tileDelta = new_tileIndex - old_tileIndex;

			if (tileDelta.x == 0 && tileDelta.y == 0)
				continue;

			if (tileDelta.x >= MEM_TILE_ONE_SIDE || tileDelta.y >= MEM_TILE_ONE_SIDE || tileDelta.x <= -MEM_TILE_ONE_SIDE || tileDelta.y <= -MEM_TILE_ONE_SIDE) {

				unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * 4];
				Terrain::loadHeightmapAtLevel(level, newCamPos, heightData);
				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE * MEM_TILE_ONE_SIDE), glm::ivec2(0, 0), heightData);
				delete[] heightData;
				continue;
			}

			Terrain::streamTerrainHorizontal(old_tileIndex, old_tileStart, old_border, new_tileIndex, new_tileStart, new_border, tileDelta, level);
			old_tileIndex.x = new_tileIndex.x;
			old_tileStart.x = new_tileStart.x;
			old_border.x = new_border.x;
			tileDelta.x = 0;
			Terrain::streamTerrainVertical(old_tileIndex, old_tileStart, old_border, new_tileIndex, new_tileStart, new_border, tileDelta, level);
		}

		cameraPosition = newCamPos;
	}

	void Terrain::streamTerrainHorizontal(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level) {

		int numChannels = 2;

		if (tileDelta.x > 0) {

			old_tileStart.x += MEM_TILE_ONE_SIDE;

			unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * numChannels];

			for (int x = 0; x < tileDelta.x; x++) {

				int startY = old_tileStart.y;

				std::vector<std::thread> pool;
				for (int z = 0; z < MEM_TILE_ONE_SIDE; z++) {

					std::thread th(&Terrain::loadFromDiscAndWriteGPUBufferAsync, this, level, TILE_SIZE, glm::ivec2(old_tileStart.x, startY), glm::ivec2(0, old_border.y), heightData, old_border);
					pool.push_back(std::move(th));

					old_border.y++;
					old_border.y %= MEM_TILE_ONE_SIDE;
					startY++;
				}
				for (auto& it : pool)
					if (it.joinable())
						it.join();

				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE, TILE_SIZE * MEM_TILE_ONE_SIDE), glm::ivec2(old_border.x * TILE_SIZE, 0), heightData);

				old_border.x++;
				old_border.x %= MEM_TILE_ONE_SIDE;
				old_tileStart.x++;
			}

			delete[] heightData;


			//// DEBUG
			//std::vector<unsigned char> out(TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2);
			//for (int i = 0; i < TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2; i++)
			//	out[i] = heights[level][i];

			//unsigned int width = TILE_SIZE * MEM_TILE_ONE_SIDE;
			//unsigned int height = TILE_SIZE * MEM_TILE_ONE_SIDE;
			//std::string imagePath = "heights" + std::to_string(level) + ".png";
			//TextureFile::encodeTextureFile(width, height, out, &imagePath[0]);
		}
		else if (tileDelta.x < 0) {

			old_tileStart.x -= 1;

			unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * numChannels];

			for (int x = tileDelta.x; x < 0; x++) {

				old_border.x--;
				old_border.x += 4;
				old_border.x %= MEM_TILE_ONE_SIDE;

				int startY = old_tileStart.y;

				std::vector<std::thread> pool;
				for (int z = 0; z < MEM_TILE_ONE_SIDE; z++) {

					std::thread th(&Terrain::loadFromDiscAndWriteGPUBufferAsync, this, level, TILE_SIZE, glm::ivec2(old_tileStart.x, startY), glm::ivec2(0, old_border.y), heightData, old_border);
					pool.push_back(std::move(th));

					old_border.y++;
					old_border.y %= MEM_TILE_ONE_SIDE;
					startY++;
				}
				for (auto& it : pool)
					if (it.joinable())
						it.join();

				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE, TILE_SIZE * MEM_TILE_ONE_SIDE), glm::ivec2(old_border.x * TILE_SIZE, 0), heightData);

				old_tileStart.x--;
			}

			delete[] heightData;
		}
	}

	void Terrain::streamTerrainVertical(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level) {

		int numChannels = 2;

		if (tileDelta.y > 0) {

			old_tileStart.y += MEM_TILE_ONE_SIDE;

			unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * numChannels];

			for (int z = 0; z < tileDelta.y; z++) {

				int startX = old_tileStart.x;

				std::vector<std::thread> pool;
				for (int x = 0; x < MEM_TILE_ONE_SIDE; x++) {

					std::thread th(&Terrain::loadFromDiscAndWriteGPUBufferAsync, this, level, MEM_TILE_ONE_SIDE * TILE_SIZE, glm::ivec2(startX, old_tileStart.y), glm::ivec2(old_border.x, 0), heightData, old_border);
					pool.push_back(std::move(th));

					old_border.x++;
					old_border.x %= MEM_TILE_ONE_SIDE;
					startX++;
				}
				for (auto& it : pool)
					if (it.joinable())
						it.join();

				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE), glm::ivec2(0, old_border.y * TILE_SIZE), heightData);

				old_border.y++;
				old_border.y %= MEM_TILE_ONE_SIDE;
				old_tileStart.y++;
			}

			delete[] heightData;

			//// DEBUG
			//std::vector<unsigned char> out(TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2);
			//for (int i = 0; i < TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2; i++)
			//	out[i] = heights[level][i];

			//unsigned int width = TILE_SIZE * MEM_TILE_ONE_SIDE;
			//unsigned int height = TILE_SIZE * MEM_TILE_ONE_SIDE;
			//std::string imagePath = "heights" + std::to_string(level) + ".png";
			//TextureFile::encodeTextureFile(width, height, out, &imagePath[0]);
		}
		else if (tileDelta.y < 0) {

			old_tileStart.y -= 1;

			unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * numChannels];

			for (int z = tileDelta.y; z < 0; z++) {

				old_border.y--;
				old_border.y += 4;
				old_border.y %= MEM_TILE_ONE_SIDE;

				int startX = old_tileStart.x;

				std::vector<std::thread> pool;
				for (int x = 0; x < MEM_TILE_ONE_SIDE; x++) {

					std::thread th(&Terrain::loadFromDiscAndWriteGPUBufferAsync, this, level, MEM_TILE_ONE_SIDE * TILE_SIZE, glm::ivec2(startX, old_tileStart.y), glm::ivec2(old_border.x, 0), heightData, old_border);
					pool.push_back(std::move(th));

					old_border.x++;
					old_border.x %= MEM_TILE_ONE_SIDE;
					startX++;
				}
				for (auto& it : pool)
					if (it.joinable())
						it.join();

				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE), glm::ivec2(0, old_border.y * TILE_SIZE), heightData);

				old_tileStart.y--;
			}

			delete[] heightData;
		}
	}

	void Terrain::loadFromDiscAndWriteGPUBufferAsync(int level, int texWidth, glm::ivec2 tileStart, glm::ivec2 border, unsigned char* heightData, glm::ivec2 toroidalUpdateBorder) {

		unsigned char* chunkHeightData = loadTerrainChunkFromDisc(level, tileStart);
		Terrain::writeHeightDataToGPUBuffer(border, texWidth, heightData, chunkHeightData, level, toroidalUpdateBorder);
		delete[] chunkHeightData;
	}

	void Terrain::updateHeightMapTextureArrayPartial(int level, glm::ivec2 size, glm::ivec2 position, unsigned char* heights) {

		glBindTexture(GL_TEXTURE_2D_ARRAY, elevationMapTexture);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, position.x, position.y, level, size.x, size.y, 1, GL_RG, GL_UNSIGNED_BYTE, &heights[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	void Terrain::deleteHeightmapArray(unsigned char** heightmapArray) {

		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		for (int i = 0; i < lodLevel; i++)
			delete[] heightmapArray[i];

		delete[] heightmapArray;
	}

	void Terrain::createElevationMapTextureArray(unsigned char** heightmapArray) {

		int lodLevel = Terrain::getMaxMipLevel(RESOLUTION, TILE_SIZE);
		int size = TILE_SIZE * MEM_TILE_ONE_SIDE;

		if (elevationMapTexture)
			glDeleteTextures(1, &elevationMapTexture);

		glGenTextures(1, &elevationMapTexture);
		glBindTexture(GL_TEXTURE_2D_ARRAY, elevationMapTexture);

		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RG8, size, size, lodLevel);

		for (int i = 0; i < lodLevel; i++)
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, size, size, 1, GL_RG, GL_UNSIGNED_BYTE, &heightmapArray[i][0]);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	void Terrain::loadTextures(std::vector<Texture*>& albedoTerrainTextureList, std::vector<Texture*>& normalTerrainTextureList, Texture* noise0, Texture* noise1) {

		albedo0 = albedoTerrainTextureList[0]->textureId;
		albedo1 = albedoTerrainTextureList[1]->textureId;
		albedo2 = albedoTerrainTextureList[2]->textureId;
		albedo3 = albedoTerrainTextureList[3]->textureId;
		albedo4 = albedoTerrainTextureList[4]->textureId;
		albedo5 = albedoTerrainTextureList[5]->textureId;
		albedo6 = albedoTerrainTextureList[6]->textureId;

		normal0 = normalTerrainTextureList[0]->textureId;
		normal1 = normalTerrainTextureList[1]->textureId;
		normal2 = normalTerrainTextureList[2]->textureId;
		normal3 = normalTerrainTextureList[3]->textureId;
		normal4 = normalTerrainTextureList[4]->textureId;
		normal5 = normalTerrainTextureList[5]->textureId;
		normal6 = normalTerrainTextureList[6]->textureId;

		macroTexture = noise0->textureId;
		noiseTexture = noise1->textureId;
	}

	void Terrain::createAlbedoMapTextureArray() {

		

		float maxAniso = 0.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
		float anisotropicFilteringValue = maxAniso;

		//-------------------- MACRO
		unsigned int width, height;
		std::vector<unsigned char> out;
		lodepng::decode(out, width, height, "resources/textures/terrain/noise/gold_a.png", LodePNGColorType::LCT_RGBA, 8);

		glGenTextures(1, &macroTexture);
		glBindTexture(GL_TEXTURE_2D, macroTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropicFilteringValue);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &out[0]);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		//--------------------

		//-------------------- NOISE
		out.clear();
		lodepng::decode(out, width, height, "resources/textures/terrain/noise/noiseTexture.png", LodePNGColorType::LCT_RGBA, 8);

		glGenTextures(1, &noiseTexture);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropicFilteringValue);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &out[0]);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		//--------------------

		//albedo0 = Terrain::loadDDS(&albedoTexturePaths[0][0]);
		//albedo1 = Terrain::loadDDS(&albedoTexturePaths[1][0]);
		//albedo2 = Terrain::loadDDS(&albedoTexturePaths[2][0]);
		//albedo3 = Terrain::loadDDS(&albedoTexturePaths[3][0]);
		//albedo4 = Terrain::loadDDS(&albedoTexturePaths[4][0]);
		//albedo5 = Terrain::loadDDS(&albedoTexturePaths[5][0]);
		//albedo6 = Terrain::loadDDS(&albedoTexturePaths[6][0]);

		//normal0 = Terrain::loadDDS(&normalTexturePaths[0][0]);
		//normal1 = Terrain::loadDDS(&normalTexturePaths[1][0]);
		//normal2 = Terrain::loadDDS(&normalTexturePaths[2][0]);
		//normal3 = Terrain::loadDDS(&normalTexturePaths[3][0]);
		//normal4 = Terrain::loadDDS(&normalTexturePaths[4][0]);
		//normal5 = Terrain::loadDDS(&normalTexturePaths[5][0]);
		//normal6 = Terrain::loadDDS(&normalTexturePaths[6][0]);
	}

	void Terrain::writeHeightDataToGPUBuffer(glm::ivec2 index, int texWidth, unsigned char* heightMap, unsigned char* chunk, int level, glm::ivec2 toroidalUpdateBorder) {

		int numChannels = 2;

		int startX = index.x * TILE_SIZE;
		int startZ = index.y * TILE_SIZE;

		int startXinHeights = toroidalUpdateBorder.x * TILE_SIZE;
		int startZinHeights = toroidalUpdateBorder.y * TILE_SIZE;

		for (int i = 0; i < TILE_SIZE; i++) {

			for (int j = 0; j < TILE_SIZE; j++) {

				int indexInChunk = (i * TILE_SIZE + j) * numChannels;
				int indexInHeightmap = ((i + startZ) * texWidth + j + startX) * numChannels;
				int indexInHeights = ((i + startZinHeights) * TILE_SIZE * MEM_TILE_ONE_SIDE + j + startXinHeights) * 2;

				// Data that will be sent to GPU
				heightMap[indexInHeightmap] = chunk[indexInChunk];
				heightMap[indexInHeightmap + 1] = chunk[indexInChunk + 1];
				//heightMap[indexInHeightmap + 2] = chunk[indexInChunk + 2];
				//heightMap[indexInHeightmap + 3] = chunk[indexInChunk + 3];

				// Data that will be used over the time, for example terrain collision detection (not tested but probably works)
				heights[level][indexInHeights] = chunk[indexInChunk];
				heights[level][indexInHeights + 1] = chunk[indexInChunk + 1];

				// For height mip stack that will be used for frustum culling
				if (i % MIP_STACK_DIVISOR_RATIO == 0 && j % MIP_STACK_DIVISOR_RATIO == 0) {

					int indexInMipStack = ((startZinHeights / MIP_STACK_DIVISOR_RATIO) + (i / MIP_STACK_DIVISOR_RATIO)) * MIP_STACK_SIZE + (startXinHeights / MIP_STACK_DIVISOR_RATIO) + (j / MIP_STACK_DIVISOR_RATIO);
					mipStack[level][indexInMipStack] = chunk[indexInChunk];
				}
			}
		}
	}

	// okurken yaz ;)
	unsigned char* Terrain::loadTerrainChunkFromDisc(int level, glm::ivec2 index) {

		if (index.x == 6 && index.y == 6 && level == 3) {
			int x = 5;
		}

		std::string path = "resources/textures/terrain/heightmaps/map_" + std::to_string(level) + '_' + std::to_string(index.y) + '_' + std::to_string(index.x) + "_.png";
		std::vector<unsigned char> out;
		unsigned int w, h;
		int numChannels = 2;

		lodepng::decode(out, w, h, path, LodePNGColorType::LCT_GREY, 16);

		unsigned char* data = new unsigned char[TILE_SIZE * TILE_SIZE * numChannels];

		if (out.size() == 0) {
			for (int i = 0; i < w * h * 2; i++)
				data[i] = 0;
		}
		else {
			for (int i = 0; i < out.size(); i++)
				data[i] = out[i];
		}

		return data;
	}

	void Terrain::updateHeightsWithTerrainChunk(unsigned char* heights, unsigned char* chunk, glm::ivec2 pos, glm::ivec2 chunkSize, glm::ivec2 heightsSize) {

		for (int i = 0; i < chunkSize.y; i++) {

			for (int j = 0; j < chunkSize.x; j++) {

				int indexInChunk = (i * chunkSize.x + j) * 2;
				int indexInHeights = ((pos.y + i) * heightsSize.x + pos.x + j) * 2;
				heights[indexInHeights] = chunk[indexInChunk];
				heights[indexInHeights + 1] = chunk[indexInChunk + 1];
			}
		}
	}

	void Terrain::generateTerrainClipmapsVertexArrays() {

		std::vector<glm::vec3> verts;
		std::vector<glm::vec3> ringFixUpVerts;
		std::vector<glm::vec3> smallSquareVerts;
		std::vector<glm::vec3> outerDegenerateVerts;
		std::vector<glm::vec3> interiorTrimVerts;

		for (int i = 0; i < clipmapResolution; i++)
			for (int j = 0; j < clipmapResolution; j++)
				verts.push_back(glm::vec3(j, 0, i));

		for (int i = 0; i < clipmapResolution - 1; i++) {

			for (int j = 0; j < clipmapResolution - 1; j++) {

				blockIndices.push_back(j + i * (clipmapResolution));
				blockIndices.push_back(j + (i + 1) * (clipmapResolution));
				blockIndices.push_back(j + i * (clipmapResolution)+1);

				blockIndices.push_back(j + i * (clipmapResolution)+1);
				blockIndices.push_back(j + (i + 1) * (clipmapResolution));
				blockIndices.push_back(j + (i + 1) * (clipmapResolution)+1);
			}
		}

		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				smallSquareVerts.push_back(glm::vec3(j, 0, i));

		for (int i = 0; i < 3 - 1; i++) {

			for (int j = 0; j < 3 - 1; j++) {

				smallSquareIndices.push_back(j + i * (3));
				smallSquareIndices.push_back(j + (i + 1) * (3));
				smallSquareIndices.push_back(j + i * (3) + 1);

				smallSquareIndices.push_back(j + i * (3) + 1);
				smallSquareIndices.push_back(j + (i + 1) * (3));
				smallSquareIndices.push_back(j + (i + 1) * (3) + 1);
			}
		}

		for (int i = 0; i < clipmapResolution; i++)
			for (int j = 0; j < 3; j++)
				ringFixUpVerts.push_back(glm::vec3(j, 0, i));

		for (int i = 0; i < clipmapResolution - 1; i++) {

			for (int j = 0; j < 3 - 1; j++) {

				ringFixUpIndices.push_back(j + i * (3));
				ringFixUpIndices.push_back(j + (i + 1) * (3));
				ringFixUpIndices.push_back(j + i * (3) + 1);

				ringFixUpIndices.push_back(j + i * (3) + 1);
				ringFixUpIndices.push_back(j + (i + 1) * (3));
				ringFixUpIndices.push_back(j + (i + 1) * (3) + 1);
			}
		}

		for (int i = 0; i <= clipmapResolution * 4 - 2; i++)
			outerDegenerateVerts.push_back(glm::vec3(i, 0, 0));

		for (int i = 0; i < clipmapResolution * 2 - 1; i++) {
			outerDegenerateIndices.push_back(i * 2);
			outerDegenerateIndices.push_back(i * 2 + 2);
			outerDegenerateIndices.push_back(i * 2 + 1);
		}

		for (int i = 0; i < clipmapResolution * 2 - 1; i++) {
			outerDegenerateIndices.push_back(i * 2);
			outerDegenerateIndices.push_back(i * 2 + 1);
			outerDegenerateIndices.push_back(i * 2 + 2);
		}

		int size = clipmapResolution * 2;

		for (int i = -size / 2; i <= size / 2; i++)
			interiorTrimVerts.push_back(glm::vec3(i, 0, size / 2 - 1));

		for (int i = -size / 2; i <= size / 2; i++)
			interiorTrimVerts.push_back(glm::vec3(i, 0, size / 2));

		for (int i = size / 2 - 2; i >= -size / 2; i--)
			interiorTrimVerts.push_back(glm::vec3(size / 2 - 1, 0, i));

		for (int i = size / 2 - 2; i >= -size / 2; i--)
			interiorTrimVerts.push_back(glm::vec3(size / 2, 0, i));

		/*
		* INTERIOR TRIM INDICES
		*
		* 9   8   7   6   5
		* 4   3   2   1   0
		* 13 10
		* 14 11
		* 15 12
		*/

		for (int i = 0; i < size; i++) {

			interiorTrimIndices.push_back(i);
			interiorTrimIndices.push_back(i + size + 1);
			interiorTrimIndices.push_back(i + 1);

			interiorTrimIndices.push_back(i + 1);
			interiorTrimIndices.push_back(i + size + 1);
			interiorTrimIndices.push_back(i + size + 2);
		}

		interiorTrimIndices.push_back(size - 1);
		interiorTrimIndices.push_back(size);
		interiorTrimIndices.push_back(size * 3 + 1);

		interiorTrimIndices.push_back(size - 1);
		interiorTrimIndices.push_back(size * 3 + 1);
		interiorTrimIndices.push_back(size * 2 + 2);

		for (int i = 0; i < size - 2; i++) {

			interiorTrimIndices.push_back(i + size * 2 + 2);
			interiorTrimIndices.push_back(i + size * 3 + 1);
			interiorTrimIndices.push_back(i + size * 3 + 2);

			interiorTrimIndices.push_back(i + size * 2 + 2);
			interiorTrimIndices.push_back(i + size * 3 + 2);
			interiorTrimIndices.push_back(i + size * 2 + 3);
		}

		// Block
		glGenVertexArrays(1, &blockVAO);
		glBindVertexArray(blockVAO);

		unsigned int VBO;
		glGenBuffers(1, &VBO);
		glBindBuffer(0x8892, VBO);
		glBufferData(0x8892, verts.size() * sizeof(glm::vec3), &verts[0], 0x88E4);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), 0);

		unsigned int EBO;
		glGenBuffers(1, &EBO);
		glBindBuffer(0x8893, EBO);
		glBufferData(0x8893, blockIndices.size() * sizeof(unsigned int), &blockIndices[0], 0x88E4);

		// Ring Fix-up
		glGenVertexArrays(1, &ringFixUpVAO);
		glBindVertexArray(ringFixUpVAO);

		unsigned int ringFixUpVBO;
		glGenBuffers(1, &ringFixUpVBO);
		glBindBuffer(0x8892, ringFixUpVBO);
		glBufferData(0x8892, ringFixUpVerts.size() * sizeof(glm::vec3), &ringFixUpVerts[0], 0x88E4);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), 0);

		unsigned int ringFixUpEBO;
		glGenBuffers(1, &ringFixUpEBO);
		glBindBuffer(0x8893, ringFixUpEBO);
		glBufferData(0x8893, ringFixUpIndices.size() * sizeof(unsigned int), &ringFixUpIndices[0], 0x88E4);

		// Small Square
		glGenVertexArrays(1, &smallSquareVAO);
		glBindVertexArray(smallSquareVAO);

		unsigned int smallSquareVBO;
		glGenBuffers(1, &smallSquareVBO);
		glBindBuffer(0x8892, smallSquareVBO);
		glBufferData(0x8892, smallSquareVerts.size() * sizeof(glm::vec3), &smallSquareVerts[0], 0x88E4);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), 0);

		unsigned int smallSquareEBO;
		glGenBuffers(1, &smallSquareEBO);
		glBindBuffer(0x8893, smallSquareEBO);
		glBufferData(0x8893, smallSquareIndices.size() * sizeof(unsigned int), &smallSquareIndices[0], 0x88E4);

		// Outer Degenerate
		glGenVertexArrays(1, &outerDegenerateVAO);
		glBindVertexArray(outerDegenerateVAO);

		unsigned int outerDegenerateVBO;
		glGenBuffers(1, &outerDegenerateVBO);
		glBindBuffer(0x8892, outerDegenerateVBO);
		glBufferData(0x8892, outerDegenerateVerts.size() * sizeof(glm::vec3), &outerDegenerateVerts[0], 0x88E4);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), 0);

		unsigned int outerDegenerateEBO;
		glGenBuffers(1, &outerDegenerateEBO);
		glBindBuffer(0x8893, outerDegenerateEBO);
		glBufferData(0x8893, outerDegenerateIndices.size() * sizeof(unsigned int), &outerDegenerateIndices[0], 0x88E4);

		// Interior Trim
		glGenVertexArrays(1, &interiorTrimVAO);
		glBindVertexArray(interiorTrimVAO);

		unsigned int interiorTrimVBO;
		glGenBuffers(1, &interiorTrimVBO);
		glBindBuffer(0x8892, interiorTrimVBO);
		glBufferData(0x8892, interiorTrimVerts.size() * sizeof(glm::vec3), &interiorTrimVerts[0], 0x88E4);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), 0);

		unsigned int interiorTrimEBO;
		glGenBuffers(1, &interiorTrimEBO);
		glBindBuffer(0x8893, interiorTrimEBO);
		glBufferData(0x8893, interiorTrimIndices.size() * sizeof(unsigned int), &interiorTrimIndices[0], 0x88E4);

		glBindVertexArray(0);
	}

	glm::ivec2 Terrain::getClipmapPosition(int level, glm::vec3& camPos) {

		int patchWidth = 2;
		float patchOffset = 1 << level;
		float requiredCameraDisplacement = patchWidth * patchOffset;
		float posX = (int)(camPos.x / requiredCameraDisplacement) * requiredCameraDisplacement;
		float posZ = (int)(camPos.z / requiredCameraDisplacement) * requiredCameraDisplacement;
		return glm::ivec2(posX + patchOffset, posZ + patchOffset);
	}

	glm::ivec2 Terrain::getTileIndex(int level, glm::vec3& camPos) {

		int parcelSizeInReal = TILE_SIZE * (1 << level);
		glm::ivec2 clipmapPos = Terrain::getClipmapPosition(level, camPos);
		return glm::ivec2(clipmapPos.x / parcelSizeInReal, clipmapPos.y / parcelSizeInReal);
	}

	int Terrain::getMaxMipLevel(int textureSize, int tileSize) {

		int lodLevel = 0;
		int num = tileSize;
		while (num <= textureSize) {
			num *= 2;
			lodLevel++;
		}
		return 4;
		//return lodLevel;
	}

	unsigned char* Terrain::remapHeightmap(unsigned char* heightmap, int size) {

		// Remap
		//
		// 0 0 0 0
		// 0 0 1 0
		// 0 0 0 0
		// 0 0 0 0
		int totalSize = size * size * 4 * 4 * 2;
		unsigned char* remappedHeightmap = new unsigned char[totalSize];

		for (int i = 0; i < totalSize; i++)
			remappedHeightmap[i] = 0;

		int start = size * 2;

		for (int i = 0; i < size; i++) {

			for (int j = 0; j < size; j++) {

				int indexInRemappedHeightmap = ((i + start) * (size * 4) + (j + start)) * 2;
				int indexInHeightmap = (i * size + j) * 2;
				remappedHeightmap[indexInRemappedHeightmap] = heightmap[indexInHeightmap];
				remappedHeightmap[indexInRemappedHeightmap + 1] = heightmap[indexInHeightmap + 1];
			}
		}
		return remappedHeightmap;
	}

	unsigned char** Terrain::createMipmaps(unsigned char* heights, int size, int totalLevel) {

		unsigned char** mipmaps = new unsigned char* [totalLevel];
		mipmaps[0] = new unsigned char[size * size * 2];

		//float minSlope = 1.f;
		//float maxSlope = 0.f;

		// [31, 16] height
		// [15, 12] base color
		// [11, 8] overlay color
		// [7, 5] scale
		// [4, 0] reserved

		// height pass, find max and min slope
		for (int i = 0; i < size; i++) {

			for (int j = 0; j < size; j++) {

				int indexInImportedHeightmap = (i * size + j) * 2;
				int indexInTerrainMipmap = (i * size + j) * 2;
				mipmaps[0][indexInTerrainMipmap] = heights[indexInImportedHeightmap];
				mipmaps[0][indexInTerrainMipmap + 1] = heights[indexInImportedHeightmap + 1];
				//mipmaps[0][indexInTerrainMipmap + 2] = 0;
				//mipmaps[0][indexInTerrainMipmap + 3] = 0;

				//glm::vec3 normal = Terrain::getNormal(j, i, 0, 2, size, heights);

				//if (normal.y < minSlope)
				//	minSlope = normal.y;

				//if (normal.y > maxSlope)
				//	maxSlope = normal.y;
			}
		}

		//// color pass
		//for (int i = 0; i < size; i++) {
		//	for (int j = 0; j < size; j++) {

		//		int randomScale = std::rand() % 4;

		//		int indexInColorMap = (i * size + j) * 4 + 2;
		//		glm::vec3 normal = Terrain::getNormal(j, i, 0, 2, size, heights);
		//		int baseIndex;
		//		int overlayIndex;
		//		int scale;
		//		Terrain::slopeToIndex(normal, minSlope, maxSlope, baseIndex, overlayIndex, scale);
		//		unsigned char colors = (baseIndex << 4u) + overlayIndex;
		//		unsigned char scaleVal = scale << 5u;
		//		unsigned char uvScale = randomScale << 3u;
		//		mipmaps[0][indexInColorMap] = colors;
		//		mipmaps[0][indexInColorMap + 1] = scaleVal + uvScale;
		//		
		//	}
		//}

		for (int level = 1; level < totalLevel; level++) {

			size /= 2;
			mipmaps[level] = new unsigned char[size * size * 2];

			//float minSlope = 1.f;
			//float maxSlope = 0.f;

			// height pass
			for (int i = 0; i < size; i++) {
				for (int j = 0; j < size; j++) {

					//(i * 2 * size * 2 + j * 2) * 2;
					//(i * 2 * size + j) * 4;
					int indexInFinerLevel = (i * size * 2 + j) * 4;
					int indexInCoarserLevel = (i * size + j) * 2;
					mipmaps[level][indexInCoarserLevel] = mipmaps[level - 1][indexInFinerLevel];
					mipmaps[level][indexInCoarserLevel + 1] = mipmaps[level - 1][indexInFinerLevel + 1];
					//mipmaps[level][indexInCoarserLevel + 2] = mipmaps[level - 1][indexInFinerLevel + 2];
					//mipmaps[level][indexInCoarserLevel + 3] = mipmaps[level - 1][indexInFinerLevel + 3];
				}
			}

			//// find max and min slope
			//for (int i = 0; i < size; i++) {
			//	for (int j = 0; j < size; j++) {

			//		glm::vec3 normal = Terrain::getNormal(j, i, level, 4, size, mipmaps[level]);

			//		if (normal.y < minSlope)
			//			minSlope = normal.y;

			//		if (normal.y > maxSlope)
			//			maxSlope = normal.y;
			//	}
			//}

			//// color pass
			//for (int i = 0; i < size; i++) {
			//	for (int j = 0; j < size; j++) {

			//		int indexInMipmap = (i * size + j) * 4 + 2;
			//		glm::vec3 normal = Terrain::getNormal(j, i, level, 4, size, mipmaps[level]);
			//		int baseIndex;
			//		int overlayIndex;
			//		int scale;
			//		Terrain::slopeToIndex(normal, minSlope, maxSlope, baseIndex, overlayIndex, scale);
			//		unsigned char colors = (baseIndex << 4u) + overlayIndex;
			//		unsigned char scaleVal = scale << 5u;
			//		mipmaps[0][indexInMipmap] = colors;
			//		mipmaps[0][indexInMipmap + 1] = scaleVal;
			//	}
			//}
		}

		return mipmaps;
	}

	void Terrain::createTextureImageFile(unsigned char* texture, int level, int channels, std::string name, int newTileSize, int baseTileSize, int ind_x, int ind_z) {

		std::vector<unsigned char> heightImage(newTileSize * newTileSize * channels);

		int coord_x = ind_x * newTileSize;
		int coord_z = ind_z * newTileSize;

		for (int z = 0; z < newTileSize; z++) {
			for (int x = 0; x < newTileSize; x++) {

				int baseIndex = ((z + coord_z) * baseTileSize + (x + coord_x)) * channels;
				int newIndex = (z * newTileSize + x) * channels;

				for (int c = 0; c < channels; c++)
					heightImage[newIndex + c] = texture[baseIndex + c];

				/*int baseCoord = ((z + coord_z) * baseTileSize + (x + coord_x)) * 2;
				int newCoord = (z * newTileSize + x) * 2;
				heightImage[newCoord] = texture[baseCoord];
				heightImage[newCoord + 1] = texture[baseCoord + 1];*/
			}
		}

		unsigned int width = newTileSize;
		unsigned int height = newTileSize;
		std::string imagePath = name + '_' + std::to_string(level) + '_' + std::to_string(ind_z) + '_' + std::to_string(ind_x) + "_.png";
		lodepng::encode(&imagePath[0], heightImage, width, height, LodePNGColorType::LCT_GREY, 16);

		//TextureFile::encodeTextureFile(width, height, heightImage, &imagePath[0]);

		//std::vector<unsigned char> heightImage(newTileSize * newTileSize);

		//int coord_x = ind_x * newTileSize;
		//int coord_z = ind_z * newTileSize;

		//for (int z = 0; z < newTileSize; z++) {
		//	for (int x = 0; x < newTileSize; x++) {

		//		int baseCoord = ((z + coord_z) * baseTileSize + (x + coord_x));
		//		int newCoord = (z * newTileSize + x);
		//		heightImage[newCoord] = heights[baseCoord];
		//	}
		//}

		//unsigned int width = newTileSize;
		//unsigned int height = newTileSize;
		//std::string imagePath = "heightmaps/map_" + std::to_string(level) + '_' + std::to_string(ind_z) + '_' + std::to_string(ind_x) + "_.png";
		//TextureFile::encodeTextureFile8Bits(width, height, heightImage, &imagePath[0]);
	}

	void Terrain::createHeightmapImageFile(unsigned char* heights, int level, int newTileSize, int baseTileSize, int ind_x, int ind_z) {

		int numChannels = 2;
		std::vector<unsigned char> heightImage(newTileSize * newTileSize * numChannels);

		int coord_x = ind_x * newTileSize;
		int coord_z = ind_z * newTileSize;

		for (int z = 0; z < newTileSize; z++) {
			for (int x = 0; x < newTileSize; x++) {

				int baseCoord = ((z + coord_z) * baseTileSize + (x + coord_x)) * numChannels;
				int newCoord = (z * newTileSize + x) * numChannels;
				heightImage[newCoord] = heights[baseCoord];
				heightImage[newCoord + 1] = heights[baseCoord + 1];
				//heightImage[newCoord + 2] = heights[baseCoord + 2];
				//heightImage[newCoord + 3] = heights[baseCoord + 3];
			}
		}

		unsigned int width = newTileSize;
		unsigned int height = newTileSize;
		std::string imagePath = "resources/textures/terrain/heightmaps/map_" + std::to_string(level) + '_' + std::to_string(ind_z) + '_' + std::to_string(ind_x) + "_.png";
		//TextureFile::encodeTextureFile(width, height, heightImage, &imagePath[0]);
		//lodepng::State state;
		lodepng::encode(imagePath, heightImage, width, height, LodePNGColorType::LCT_GREY, 16);

		//std::vector<unsigned char> heightImage(newTileSize * newTileSize);

		//int coord_x = ind_x * newTileSize;
		//int coord_z = ind_z * newTileSize;

		//for (int z = 0; z < newTileSize; z++) {
		//	for (int x = 0; x < newTileSize; x++) {

		//		int baseCoord = ((z + coord_z) * baseTileSize + (x + coord_x));
		//		int newCoord = (z * newTileSize + x);
		//		heightImage[newCoord] = heights[baseCoord];
		//	}
		//}

		//unsigned int width = newTileSize;
		//unsigned int height = newTileSize;
		//std::string imagePath = "heightmaps/map_" + std::to_string(level) + '_' + std::to_string(ind_z) + '_' + std::to_string(ind_x) + "_.png";
		//TextureFile::encodeTextureFile8Bits(width, height, heightImage, &imagePath[0]);
	}

	void Terrain::divideTextureStack(unsigned char** textureStack, int size, int patchSize, int channelCount, std::string name, int totalLevel) {

		//int mipCount = 0;
		//int sizeIterator = size;

		//while (sizeIterator >= patchSize) {
		//	sizeIterator /= 2;
		//	mipCount++;
		//}

		int res = size;

		for (int level = 0; level < totalLevel; level++) {

			int numTiles = res / patchSize;
			for (int i = 0; i < numTiles; i++)
				for (int j = 0; j < numTiles; j++)
					//make multi-threaded ?
					Terrain::createTextureImageFile(textureStack[level], level, channelCount, name, patchSize, res, j, i);

			res /= 2;
		}
	}

	void Terrain::divideTerrainHeightmaps(unsigned char** heightMapList, int width, int totalLevel) {

		//int mipCount = 0;
		//int sizeIterator = width;

		//while (sizeIterator >= TILE_SIZE) {
		//	sizeIterator /= 2;
		//	mipCount++;
		//}

		int res = width;

		for (int level = 0; level < totalLevel; level++) {

			int numTiles = res / TILE_SIZE;
			int start = (numTiles >> 1) - 2;
			int end = ((numTiles * 3) >> 2) + 1;
			for (int i = start; i < end; i++)
				for (int j = start; j < end; j++)
					//make multi-threaded ?
					Terrain::createHeightmapImageFile(heightMapList[level], level, TILE_SIZE, res, j, i);

			res /= 2;
		}
	}

}