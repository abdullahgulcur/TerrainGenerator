// Copyright (c) Abdullah Gulcur 2022-2023
// 
// This project is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

// Terrain Class
// Constraints:

// REFERENCES
// Virtual texturing for heightmaps. Reference: https://notkyon.moe/vt/Clipmap.pdf
// Clipmap rendering using nested grids. Reference : https://developer.nvidia.com/gpugems/gpugems2/part-i-geometric-complexity/chapter-2-terrain-rendering-using-gpu-based-geometry
// Procedural shader splatting. Reference : https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/chapter5-andersson-terrain-rendering-in-frostbite.pdf

#include "pch.h"
#include "terrain.h"
#include "corecontext.h"
#include "gl/glew.h"
#include "lodepng/lodepng.h"

using namespace std::chrono;

namespace Core {

	Terrain::Terrain() {

		lightDir = glm::vec3(-0.5f, -1.f, -0.5f);
		fogColor = glm::vec3(190.f / 255, 220.f / 255, 1.f);
		color0 = glm::vec3(0.95f, 0.95f, 0.95f);
		color1 = glm::vec3(0.85f, 0.85f, 0.85f);

		std::srand((unsigned)time(0)); //?
	}

	Terrain::~Terrain() {

		glDeleteTextures(1, &elevationMapTexture);
		glDeleteVertexArrays(1, &blockVAO);
		glDeleteVertexArrays(1, &ringFixUpVerticalVAO);
		glDeleteVertexArrays(1, &ringFixUpHorizontalVAO);
		glDeleteVertexArrays(1, &smallSquareVAO);
		glDeleteVertexArrays(1, &outerDegenerateVAO);
		glDeleteVertexArrays(1, &interiorTrimVAO);

		delete[] blockPositions;
		delete[] ringFixUpPositions;
		delete[] ringFixUpVerticalPositions;
		delete[] interiorTrimPositions;
		delete[] outerDegeneratePositions;
		delete[] rotAmounts;

		delete[] clipmapStartIndices;

		for (int i = 0; i < CLIPMAP_LEVEL; i++)
			delete[] heightmapStack[i];
		delete[] heightmapStack;
	}

	void Terrain::start() {

		std::string path = "resources/textures/terrain/terrain.png";
		std::vector<unsigned char> out;
		unsigned int w, h;
		lodepng::decode(out, w, h, path, LodePNGColorType::LCT_GREY, 16);
		unsigned char* data = new unsigned char[w * w * TERRAIN_STACK_NUM_CHANNELS];
		for (int i = 0; i < out.size(); i++)
			data[i] = out[i];

		unsigned char* heightmap = Terrain::resizeHeightmap(data, w);
		unsigned char** heightMapList = Terrain::createMipmaps(heightmap, w * MEM_TILE_ONE_SIDE, CLIPMAP_LEVEL);
		Terrain::createHeightmapStack(heightMapList, w * MEM_TILE_ONE_SIDE, CLIPMAP_LEVEL);

		blockPositions = new glm::vec2[12 * CLIPMAP_LEVEL + 4];
		ringFixUpPositions = new glm::vec2[2 * CLIPMAP_LEVEL + 2];
		ringFixUpVerticalPositions = new glm::vec2[2 * CLIPMAP_LEVEL + 2];
		interiorTrimPositions = new glm::vec2[CLIPMAP_LEVEL];
		outerDegeneratePositions = new glm::vec2[4 * CLIPMAP_LEVEL];
		rotAmounts = new float[CLIPMAP_LEVEL];

		Terrain::generateTerrainClipmapsVertexArrays();
		Terrain::initShaders("resources/shaders/terrain/terrain.vert", "resources/shaders/terrain/terrain.frag");
		cameraPosition = glm::clamp(CoreContext::instance->scene->cameraInfo.camPos, glm::vec3(8193, 0, 8193), glm::vec3(12287, 0, 12287));
		Terrain::loadTerrainHeightmapOnInit(cameraPosition, CLIPMAP_LEVEL);
		Terrain::calculateBlockPositions(cameraPosition);
		Terrain::loadTextures();
	}

	void Terrain::initShaders(const char* vertexShader, const char* fragShader) {

		programID = Shader::loadShaders(vertexShader, fragShader);
		glUseProgram(programID);
		glUniform1i(glGetUniformLocation(programID, "heightmapArray"), 0);
		glUniform1i(glGetUniformLocation(programID, "irradianceMap"), 1);
		glUniform1i(glGetUniformLocation(programID, "macroTexture"), 2);
		glUniform1i(glGetUniformLocation(programID, "noiseTexture"), 3);
		glUniform1i(glGetUniformLocation(programID, "albedoT0"), 4);
		glUniform1i(glGetUniformLocation(programID, "albedoT1"), 5);
		glUniform1i(glGetUniformLocation(programID, "albedoT2"), 6);
		glUniform1i(glGetUniformLocation(programID, "albedoT3"), 7);
		glUniform1i(glGetUniformLocation(programID, "albedoT5"), 8);
		glUniform1i(glGetUniformLocation(programID, "albedoT6"), 9);
		glUniform1i(glGetUniformLocation(programID, "normalT0"), 10);
		glUniform1i(glGetUniformLocation(programID, "normalT1"), 11);
		glUniform1i(glGetUniformLocation(programID, "normalT2"), 12);
		glUniform1i(glGetUniformLocation(programID, "normalT3"), 13);
		glUniform1i(glGetUniformLocation(programID, "normalT4"), 14);
		glUniform1i(glGetUniformLocation(programID, "normalT5"), 15);
		glUniform1i(glGetUniformLocation(programID, "normalT6"), 16);
		glUniform1i(glGetUniformLocation(programID, "normalT7"), 17);
	}

	void Terrain::loadTerrainHeightmapOnInit(glm::vec3 camPos, int clipmapLevel) {

		unsigned char** terrainStack = new unsigned char* [clipmapLevel];
		for (int i = 0; i < clipmapLevel; i++)
			terrainStack[i] = new unsigned char[MEM_TILE_ONE_SIDE * MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * TERRAIN_STACK_NUM_CHANNELS];

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

		for (int i = 0; i < MEM_TILE_ONE_SIDE; i++) {

			int startX = tileStart.x;

			for (int j = 0; j < MEM_TILE_ONE_SIDE; j++) {

				Terrain::writeHeightDataToGPUBuffer(border, glm::ivec2(startX, tileStart.y), MEM_TILE_ONE_SIDE * TILE_SIZE, heightData, level);
				border.x++;
				border.x %= MEM_TILE_ONE_SIDE;
				startX++;
			}

			border.y++;
			border.y %= MEM_TILE_ONE_SIDE;
			tileStart.y++;
		}

		// DEBUG
		//std::vector<unsigned char> out(TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2);
		//for (int i = 0; i < TILE_SIZE * MEM_TILE_ONE_SIDE * TILE_SIZE * MEM_TILE_ONE_SIDE * 2; i++)
		//	out[i] = heights[level][i];

		//unsigned int width = TILE_SIZE * MEM_TILE_ONE_SIDE;
		//unsigned int height = TILE_SIZE * MEM_TILE_ONE_SIDE;
		//std::string imagePath = "heights" + std::to_string(level) + ".png";
		//TextureFile::encodeTextureFile(width, height, out, &imagePath[0]);
	}

	/*
	* Any updates because of the camera movement. Nested grids positions, textures updates...
	*/
	void Terrain::update(float dt) {

		glm::vec3 camPosition = glm::clamp(CoreContext::instance->scene->cameraInfo.camPos, glm::vec3(8193, 0, 8193), glm::vec3(12287, 0, 12287));
		Terrain::calculateBlockPositions(camPosition);
		Terrain::streamTerrain(camPosition);
	}

	/*
	* Draw terrain
	*/
	void Terrain::onDraw() {

		int programID = this->programID;

		glm::vec3 camPos = CoreContext::instance->scene->cameraInfo.camPos;
		glm::mat4& PV = CoreContext::instance->scene->cameraInfo.VP;

		int blockVAO = this->blockVAO;
		int ringFixUpVerticalVAO = this->ringFixUpVerticalVAO;
		int ringFixUpHorizontalVAO = this->ringFixUpHorizontalVAO;
		int smallSquareVAO = this->smallSquareVAO;
		int interiorTrimVAO = this->interiorTrimVAO;
		int outerDegenerateVAO = this->outerDegenerateVAO;

		int blockIndiceCount = blockIndices.size();
		int ringFixUpIndiceCount = ringFixUpVerticalIndices.size();
		int ringFixUpVerticalIndiceCount = ringFixUpHorizontalIndices.size();
		int smallSquareIndiceCount = smallSquareIndices.size();
		int interiorTrimIndiceCount = interiorTrimIndices.size();
		int outerDegenerateIndiceCount = outerDegenerateIndices.size();

		Cubemap* cubemap = CoreContext::instance->scene->cubemap;

		glUseProgram(programID);
		glUniformMatrix4fv(glGetUniformLocation(programID, "PV"), 1, 0, &PV[0][0]);
		glUniform3fv(glGetUniformLocation(programID, "camPos"), 1, &camPos[0]);
		glUniform1f(glGetUniformLocation(programID, "texSize"), (float)TILE_SIZE * MEM_TILE_ONE_SIDE);

		// Terrain Material Parameters
		glUniform3f(glGetUniformLocation(programID, "lightDirection"), lightDir.x, lightDir.y, lightDir.z);
		glUniform1f(glGetUniformLocation(programID, "lightPow"), lightPow);

		glUniform1f(glGetUniformLocation(programID, "ambientAmount"), ambientAmount);
		glUniform1f(glGetUniformLocation(programID, "specularPower"), specularPower);
		glUniform1f(glGetUniformLocation(programID, "specularAmount"), specularAmount);
					
		glUniform1f(glGetUniformLocation(programID, "blendDistance"), blendDistance);
		glUniform1f(glGetUniformLocation(programID, "blendAmount"), blendAmount);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color0_dist0"), scale_color0_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color0_dist1"), scale_color0_dist1);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color1_dist0"), scale_color1_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color1_dist1"), scale_color1_dist1);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color2_dist0"), scale_color2_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color2_dist1"), scale_color2_dist1);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color3_dist0"), scale_color3_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color3_dist1"), scale_color3_dist1);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color4_dist0"), scale_color4_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color4_dist1"), scale_color4_dist1);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color5_dist0"), scale_color5_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color5_dist1"), scale_color5_dist1);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color6_dist0"), scale_color6_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color6_dist1"), scale_color6_dist1);
					
		glUniform1f(glGetUniformLocation(programID, "scale_color7_dist0"), scale_color7_dist0);
		glUniform1f(glGetUniformLocation(programID, "scale_color7_dist1"), scale_color7_dist1);

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

		glUniform1f(glGetUniformLocation(programID, "overlayBlendScale2"), overlayBlendScale2);
		glUniform1f(glGetUniformLocation(programID, "overlayBlendAmount2"), overlayBlendAmount2);
		glUniform1f(glGetUniformLocation(programID, "overlayBlendPower2"), overlayBlendPower2);
		glUniform1f(glGetUniformLocation(programID, "overlayBlendOpacity2"), overlayBlendOpacity2);

		glUniform3fv(glGetUniformLocation(programID, "color0"), 1, &color0[0]);
		glUniform3fv(glGetUniformLocation(programID, "color1"), 1, &color1[0]);
					
		glUniform1f(glGetUniformLocation(programID, "slopeSharpness0"), slopeSharpness0);
		glUniform1f(glGetUniformLocation(programID, "slopeSharpness1"), slopeSharpness1);
					
		glUniform1f(glGetUniformLocation(programID, "slopeBias0"), slopeBias0);
		glUniform1f(glGetUniformLocation(programID, "slopeBias1"), slopeBias1);
					
		glUniform1f(glGetUniformLocation(programID, "heightBias0"), heightBias0);
		glUniform1f(glGetUniformLocation(programID, "heightSharpness0"), heightSharpness0);
		glUniform1f(glGetUniformLocation(programID, "heightBias1"), heightBias1);
		glUniform1f(glGetUniformLocation(programID, "heightSharpness1"), heightSharpness1);
					
		glUniform1f(glGetUniformLocation(programID, "distanceNear"), distanceNear);
		glUniform1f(glGetUniformLocation(programID, "fogBlendDistance"), fogBlendDistance);
		glUniform1f(glGetUniformLocation(programID, "maxFog"), maxFog);
		glUniform3fv(glGetUniformLocation(programID, "fogColor"), 1, &fogColor[0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, elevationMapTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->irradianceMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, macroTexture);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, albedo0);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, albedo1);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, albedo2);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, albedo3);
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, albedo5);
		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_2D, albedo6);
		
		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D, normal0);
		glActiveTexture(GL_TEXTURE11);
		glBindTexture(GL_TEXTURE_2D, normal1);
		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D, normal2);
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_2D, normal3);
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_2D, normal4);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, normal5);
		glActiveTexture(GL_TEXTURE16);
		glBindTexture(GL_TEXTURE_2D, normal6);
		glActiveTexture(GL_TEXTURE17);
		glBindTexture(GL_TEXTURE_2D, normal7);

		glm::vec2* blockPositions = this->blockPositions;
		glm::vec2* ringFixUpPositions = this->ringFixUpPositions;
		glm::vec2* ringFixUpVerticalPositions = this->ringFixUpVerticalPositions;
		glm::vec2* interiorTrimPositions = this->interiorTrimPositions;
		glm::vec2* outerDegeneratePositions = this->outerDegeneratePositions;
		float* rotAmounts = this->rotAmounts;
		glm::vec2 smallSquarePosition = this->smallSquarePosition;

		std::vector<TerrainVertexAttribs> instanceArray;

		// BLOCKS

		for (int i = 0; i < CLIPMAP_LEVEL; i++) {

			for (int j = 0; j < 12; j++) {

				glm::vec4 startInWorldSpace;
				glm::vec4 endInWorldSpace;
				//AABB_Box aabb = blockAABBs[i * 12 + j];
				//startInWorldSpace = aabb.start;
				//endInWorldSpace = aabb.end;

				//if (camera->intersectsAABB(startInWorldSpace, endInWorldSpace)) {
				TerrainVertexAttribs attribs;
				attribs.level = i;
				attribs.model = glm::mat4(1);
				attribs.position = glm::vec2(blockPositions[i * 12 + j].x, blockPositions[i * 12 + j].y);
				attribs.color = BLOCK_COLOR;
				instanceArray.push_back(attribs);
				//}
			}
		}

		for (int i = 0; i < 4; i++) {

			glm::vec4 startInWorldSpace;
			glm::vec4 endInWorldSpace;
			//AABB_Box aabb = blockAABBs[CLIPMAP_LEVEL * 12 + i];
			//startInWorldSpace = aabb.start;
			//endInWorldSpace = aabb.end;

			//if (camera->intersectsAABB(startInWorldSpace, endInWorldSpace)) {
			TerrainVertexAttribs attribs;
			attribs.level = 0;
			attribs.model = glm::mat4(1);
			attribs.position = glm::vec2(blockPositions[CLIPMAP_LEVEL * 12 + i].x, blockPositions[CLIPMAP_LEVEL * 12 + i].y);
			attribs.color = BLOCK_COLOR;
			instanceArray.push_back(attribs);
			//}
		}

		if (instanceArray.size()) {
			Terrain::drawElementsInstanced(blockVAO, instanceArray, blockIndiceCount);
			instanceArray.clear();
		}

		// RING FIXUP VERTICAL
		for (int i = 0; i < CLIPMAP_LEVEL; i++) {

			TerrainVertexAttribs attribs;
			attribs.level = i;
			attribs.model = glm::mat4(1);
			attribs.color = FIXUP_VERTICAL_COLOR;

			attribs.position = glm::vec2(ringFixUpPositions[i * 2 + 0].x, ringFixUpPositions[i * 2 + 0].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(ringFixUpPositions[i * 2 + 1].x, ringFixUpPositions[i * 2 + 1].y);
			instanceArray.push_back(attribs);
		}

		{
			TerrainVertexAttribs attribs;
			attribs.level = 0;
			attribs.model = glm::mat4(1);
			attribs.color = FIXUP_VERTICAL_COLOR;

			attribs.position = glm::vec2(ringFixUpPositions[CLIPMAP_LEVEL * 2 + 0].x, ringFixUpPositions[CLIPMAP_LEVEL * 2 + 0].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(ringFixUpPositions[CLIPMAP_LEVEL * 2 + 1].x, ringFixUpPositions[CLIPMAP_LEVEL * 2 + 1].y);
			instanceArray.push_back(attribs);
		}

		Terrain::drawElementsInstanced(ringFixUpVerticalVAO, instanceArray, ringFixUpIndiceCount);
		instanceArray.clear();

		// RING FIXUP HORIZONTAL
		for (int i = 0; i < CLIPMAP_LEVEL; i++) {

			TerrainVertexAttribs attribs;
			attribs.level = i;
			attribs.model = glm::mat4(1);
			attribs.color = FIXUP_HORIZONTAL_COLOR;

			attribs.position = glm::vec2(ringFixUpVerticalPositions[i * 2 + 0].x, ringFixUpVerticalPositions[i * 2 + 0].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(ringFixUpVerticalPositions[i * 2 + 1].x, ringFixUpVerticalPositions[i * 2 + 1].y);
			instanceArray.push_back(attribs);
		}

		{
			TerrainVertexAttribs attribs;
			attribs.level = 0;
			attribs.model = glm::mat4(1);
			attribs.color = FIXUP_HORIZONTAL_COLOR;

			attribs.position = glm::vec2(ringFixUpVerticalPositions[CLIPMAP_LEVEL * 2 + 0].x, ringFixUpVerticalPositions[CLIPMAP_LEVEL * 2 + 0].y);
			instanceArray.push_back(attribs);
			attribs.position = glm::vec2(ringFixUpVerticalPositions[CLIPMAP_LEVEL * 2 + 1].x, ringFixUpVerticalPositions[CLIPMAP_LEVEL * 2 + 1].y);
			instanceArray.push_back(attribs);
		}

		Terrain::drawElementsInstanced(ringFixUpHorizontalVAO, instanceArray, ringFixUpVerticalIndiceCount);
		instanceArray.clear();

		// INTERIOR TRIM
		for (int i = 0; i < CLIPMAP_LEVEL - 1; i++) {

			glm::mat4 model = glm::rotate(glm::mat4(1), glm::radians(rotAmounts[i]), glm::vec3(0.0f, 1.0f, 0.0f));

			TerrainVertexAttribs attribs;
			attribs.level = i + 1;
			attribs.model = model;
			attribs.position = glm::vec2(interiorTrimPositions[i].x, interiorTrimPositions[i].y);
			attribs.color = INTERIOR_TRIM_COLOR;
			instanceArray.push_back(attribs);
		}
		Terrain::drawElementsInstanced(interiorTrimVAO, instanceArray, interiorTrimIndiceCount);
		instanceArray.clear();

		// OUTER DEGENERATE
		for (int i = 0; i < CLIPMAP_LEVEL - 1; i++) {

			glm::mat4 model = glm::mat4(1);

			TerrainVertexAttribs attribs;
			attribs.level = i;
			attribs.model = model;
			attribs.color = OUTER_DEGENERATE_COLOR;

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
		Terrain::drawElementsInstanced(outerDegenerateVAO, instanceArray, outerDegenerateIndiceCount);
		instanceArray.clear();

		// SMALL SQUARE
		TerrainVertexAttribs attribs;
		attribs.level = 0;
		attribs.model = glm::mat4(1);
		attribs.position = glm::vec2(smallSquarePosition.x, smallSquarePosition.y);
		attribs.color = SMALL_SQUARE_COLOR;
		instanceArray.push_back(attribs);
		Terrain::drawElementsInstanced(smallSquareVAO, instanceArray, smallSquareIndiceCount);
	}

	/*
	* Draw nested grids instanced.
	*/
	void Terrain::drawElementsInstanced(unsigned int VAO, std::vector<TerrainVertexAttribs>& instanceArray, unsigned int indiceCount) {

		unsigned int instanceBuffer;
		glGenBuffers(1, &instanceBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
		glBufferData(GL_ARRAY_BUFFER, instanceArray.size() * sizeof(TerrainVertexAttribs), &instanceArray[0], GL_STATIC_DRAW);

		glBindVertexArray(VAO);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)0);
		//glEnableVertexAttribArray(2);
		//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2)));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2)));
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2) + sizeof(float)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2) + sizeof(float) + sizeof(glm::vec4)));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2) + sizeof(float) + sizeof(glm::vec4) * 2));
		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2) + sizeof(float) + sizeof(glm::vec4) * 3));
		glEnableVertexAttribArray(8);
		glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2) + sizeof(float) + sizeof(glm::vec4) * 4));

		glVertexAttribDivisor(1, 1);
		glVertexAttribDivisor(2, 1);
		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);
		glVertexAttribDivisor(7, 1);
		glVertexAttribDivisor(8, 1);

		glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(indiceCount), GL_UNSIGNED_INT, 0, instanceArray.size());

		glBindVertexArray(0);
		glDeleteBuffers(1, &instanceBuffer);
	}

	/*
	* When camera moves, nested grids positions are also updated in this function.
	*/
	void Terrain::calculateBlockPositions(glm::vec3 camPosition) {

		float fake = 1000000;
		glm::vec3 fakeDisplacement = glm::vec3(fake, 0, fake);
		glm::vec3 camPos = camPosition + fakeDisplacement;
		int patchWidth = 2;

		for (int i = 0; i < CLIPMAP_LEVEL; i++) {

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

			float patchTranslation = (1 << i) * CLIPMAP_RESOLUTION;
			float patchOffset = 1 << i;

			// For outer trim rotation. This '2' at the end is constant. Because of binary movement style
			int rotX = (int)(posX / requiredCameraDisplacement) % 2;
			int rotZ = (int)(posZ / requiredCameraDisplacement) % 2;

			//// BLOCKS

			/*
			*  0 11 10  9
			*  1        8
			*  2        7
			*  3  4  5  6
			*/

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
			*    1
			*/

			// 0
			position = glm::vec2(-fake + posX, patchTranslation + patchOffset - fake + posZ);
			ringFixUpPositions[i * 2 + 0] = position;

			// 2
			position = glm::vec2(-fake + posX, (patchOffset - patchTranslation) * 2 - fake + posZ);
			ringFixUpPositions[i * 2 + 1] = position;

			// RING FIX-UP HORIZONTAL

			/*
			*  0   1
			*/

			// 0
			position = glm::vec2(-fake + posX + patchTranslation + patchOffset, - fake + posZ);
			ringFixUpVerticalPositions[i * 2 + 0] = position;

			// 1
			position = glm::vec2(-fake + posX - patchTranslation * 2 + patchOffset * 2, -fake + posZ);
			ringFixUpVerticalPositions[i * 2 + 1] = position;

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
		blockPositions[CLIPMAP_LEVEL * 12 + 0] = position;

		// 1
		position.y -= CLIPMAP_RESOLUTION + 1;
		blockPositions[CLIPMAP_LEVEL * 12 + 1] = position;

		// 2
		position.x -= CLIPMAP_RESOLUTION + 1;
		blockPositions[CLIPMAP_LEVEL * 12 + 2] = position;

		// 3
		position.y += CLIPMAP_RESOLUTION + 1;
		blockPositions[CLIPMAP_LEVEL * 12 + 3] = position;

		//
		//0
		position = glm::vec2(0 - fakeDisplacement.x + posX, 2 - fakeDisplacement.z + posZ);
		ringFixUpPositions[CLIPMAP_LEVEL * 2 + 0] = position;

		// 2
		position = glm::vec2(0 - fakeDisplacement.x + posX, 1 - CLIPMAP_RESOLUTION - fakeDisplacement.z + posZ);
		ringFixUpPositions[CLIPMAP_LEVEL * 2 + 1] = position;

		//
		//0
		position = glm::vec2(0 - fakeDisplacement.x + posX + 2, - fakeDisplacement.z + posZ);
		ringFixUpVerticalPositions[CLIPMAP_LEVEL * 2 + 0] = position;

		// 2
		position = glm::vec2(1 - fakeDisplacement.x + posX - CLIPMAP_RESOLUTION, -fakeDisplacement.z + posZ);
		ringFixUpVerticalPositions[CLIPMAP_LEVEL * 2 + 1] = position;

		//
		smallSquarePosition = glm::vec2(0 - fakeDisplacement.x + posX, 0 - fakeDisplacement.z + posZ);
	}

	void Terrain::streamTerrain(glm::vec3 newCamPos) {

		for (int level = 0; level < CLIPMAP_LEVEL; level++) {

			glm::ivec2 old_tileIndex = Terrain::getTileIndex(level, cameraPosition);
			glm::ivec2 old_tileStart = old_tileIndex - MEM_TILE_ONE_SIDE / 2;
			glm::ivec2 old_border = old_tileStart % MEM_TILE_ONE_SIDE;

			glm::ivec2 new_tileIndex = Terrain::getTileIndex(level, newCamPos);
			glm::ivec2 new_tileStart = new_tileIndex - MEM_TILE_ONE_SIDE / 2;
			glm::ivec2 new_border = new_tileStart % MEM_TILE_ONE_SIDE;

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

	/*
	* Controls any data update when camera movement is in x direction
	*/
	void Terrain::streamTerrainHorizontal(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level) {

		if (tileDelta.x > 0) {

			old_tileStart.x += MEM_TILE_ONE_SIDE;

			unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * TERRAIN_STACK_NUM_CHANNELS];

			for (int x = 0; x < tileDelta.x; x++) {

				int startY = old_tileStart.y;

				for (int z = 0; z < MEM_TILE_ONE_SIDE; z++) {

					Terrain::writeHeightDataToGPUBuffer(glm::ivec2(0, old_border.y), glm::ivec2(old_tileStart.x, startY), TILE_SIZE, heightData, level);

					old_border.y++;
					old_border.y %= MEM_TILE_ONE_SIDE;
					startY++;
				}

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

			unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * TERRAIN_STACK_NUM_CHANNELS];

			for (int x = tileDelta.x; x < 0; x++) {

				old_border.x--;
				old_border.x += 4;
				old_border.x %= MEM_TILE_ONE_SIDE;

				int startY = old_tileStart.y;

				for (int z = 0; z < MEM_TILE_ONE_SIDE; z++) {

					Terrain::writeHeightDataToGPUBuffer(glm::ivec2(0, old_border.y), glm::ivec2(old_tileStart.x, startY), TILE_SIZE, heightData, level);

					old_border.y++;
					old_border.y %= MEM_TILE_ONE_SIDE;
					startY++;
				}

				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE, TILE_SIZE * MEM_TILE_ONE_SIDE), glm::ivec2(old_border.x * TILE_SIZE, 0), heightData);

				old_tileStart.x--;
			}

			delete[] heightData;
		}
	}

	/*
	* Controls any data update when camera movement is in z direction
	*/
	void Terrain::streamTerrainVertical(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level) {

		if (tileDelta.y > 0) {

			old_tileStart.y += MEM_TILE_ONE_SIDE;

			unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * TERRAIN_STACK_NUM_CHANNELS];

			for (int z = 0; z < tileDelta.y; z++) {

				int startX = old_tileStart.x;

				for (int x = 0; x < MEM_TILE_ONE_SIDE; x++) {

					Terrain::writeHeightDataToGPUBuffer(glm::ivec2(old_border.x, 0), glm::ivec2(startX, old_tileStart.y), MEM_TILE_ONE_SIDE * TILE_SIZE, heightData, level);
					old_border.x++;
					old_border.x %= MEM_TILE_ONE_SIDE;
					startX++;
				}

				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE), glm::ivec2(0, old_border.y * TILE_SIZE), heightData);

				old_border.y++;
				old_border.y %= MEM_TILE_ONE_SIDE;
				old_tileStart.y++;
			}

			delete[] heightData;
		}
		else if (tileDelta.y < 0) {

			old_tileStart.y -= 1;

			unsigned char* heightData = new unsigned char[MEM_TILE_ONE_SIDE * TILE_SIZE * TILE_SIZE * TERRAIN_STACK_NUM_CHANNELS];

			for (int z = tileDelta.y; z < 0; z++) {

				old_border.y--;
				old_border.y += 4;
				old_border.y %= MEM_TILE_ONE_SIDE;

				int startX = old_tileStart.x;

				for (int x = 0; x < MEM_TILE_ONE_SIDE; x++) {

					Terrain::writeHeightDataToGPUBuffer(glm::ivec2(old_border.x, 0), glm::ivec2(startX, old_tileStart.y), MEM_TILE_ONE_SIDE * TILE_SIZE, heightData, level);

					old_border.x++;
					old_border.x %= MEM_TILE_ONE_SIDE;
					startX++;
				}

				Terrain::updateHeightMapTextureArrayPartial(level, glm::ivec2(TILE_SIZE * MEM_TILE_ONE_SIDE, TILE_SIZE), glm::ivec2(0, old_border.y * TILE_SIZE), heightData);

				old_tileStart.y--;
			}

			delete[] heightData;
		}
	}

	/*
	* Partially update heightmap texture in gpu memory
	*/
	void Terrain::updateHeightMapTextureArrayPartial(int level, glm::ivec2 size, glm::ivec2 position, unsigned char* heights) {

		glBindTexture(GL_TEXTURE_2D_ARRAY, elevationMapTexture);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, position.x, position.y, level, size.x, size.y, 1, GL_RG, GL_UNSIGNED_BYTE, &heights[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	void Terrain::deleteHeightmapArray(unsigned char** heightmapArray) {

		for (int i = 0; i < CLIPMAP_LEVEL; i++)
			delete[] heightmapArray[i];

		delete[] heightmapArray;
	}

	/*
	* The first time data sent to the gpu when map is loaded this function is called.
	* Or if you jump to far point of the map, all data is updated instead of toroidally.
	*/
	void Terrain::createElevationMapTextureArray(unsigned char** heightmapArray) {

		int size = TILE_SIZE * MEM_TILE_ONE_SIDE;

		if (elevationMapTexture)
			glDeleteTextures(1, &elevationMapTexture);

		glGenTextures(1, &elevationMapTexture);
		glBindTexture(GL_TEXTURE_2D_ARRAY, elevationMapTexture);

		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RG8, size, size, CLIPMAP_LEVEL);

		for (int i = 0; i < CLIPMAP_LEVEL; i++)
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, size, size, 1, GL_RG, GL_UNSIGNED_BYTE, &heightmapArray[i][0]);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	/*
	* Terrain textures. We procedurally merge them in the fragment shader.
	*/
	void Terrain::loadTextures() {

		std::map<std::string, Texture*>& textures = CoreContext::instance->fileSystem->textures;

		albedo0 = textures.at("soil_a")->textureId;
		albedo1 = textures.at("mulch_a")->textureId;
		albedo2 = textures.at("granite_a")->textureId;
		albedo3 = textures.at("soil_rock_a")->textureId;
		albedo5 = textures.at("sand_a")->textureId;
		albedo6 = textures.at("lichened_rock_a")->textureId;

		normal0 = textures.at("soil_n")->textureId;
		normal1 = textures.at("mulch_n")->textureId;
		normal2 = textures.at("granite_n")->textureId;
		normal3 = textures.at("soil_rock_n")->textureId;
		normal4 = textures.at("snow_fresh_n")->textureId;
		normal5 = textures.at("sand_n")->textureId;
		normal6 = textures.at("lichened_rock_n")->textureId;
		normal7 = textures.at("s1n")->textureId;

		macroTexture = textures.at("gold_a")->textureId;
		noiseTexture = textures.at("noiseTexture")->textureId;
	}

	/*
	* We toroidally update the height values of the texture in GPU.
	* In this case we only update small chunks of the memory instead of updating all data.
	*/
	void Terrain::writeHeightDataToGPUBuffer(glm::ivec2 index, glm::ivec2 tileStart, int texWidth, unsigned char* heightMap, int level) {

		int startX = index.x * TILE_SIZE;
		int startZ = index.y * TILE_SIZE;

		int coord_x = (tileStart.x - clipmapStartIndices[level].x) * TILE_SIZE;
		int coord_z = (tileStart.y - clipmapStartIndices[level].x) * TILE_SIZE;

		int stackSize = (clipmapStartIndices[level].y - clipmapStartIndices[level].x) * TILE_SIZE;

		for (int i = 0; i < TILE_SIZE; i++) {

			for (int j = 0; j < TILE_SIZE; j++) {

				int indexInChunk = ((i + coord_z) * stackSize + coord_x + j) * TERRAIN_STACK_NUM_CHANNELS;
				int indexInHeightmap = ((i + startZ) * texWidth + j + startX) * TERRAIN_STACK_NUM_CHANNELS;
		
				// Data that will be sent to GPU
				heightMap[indexInHeightmap] = heightmapStack[level][indexInChunk];
				heightMap[indexInHeightmap + 1] = heightmapStack[level][indexInChunk + 1];
			}
		}
	}

	/*
	* Creates all the vao for the meshes of the terrain
	*/
	void Terrain::generateTerrainClipmapsVertexArrays() {

		std::vector<glm::vec2> blockVerts;
		std::vector<glm::vec2> ringFixUpVerticalVerts;
		std::vector<glm::vec2> ringFixUpHorizontalVerts;
		std::vector<glm::vec2> smallSquareVerts;
		std::vector<glm::vec2> outerDegenerateVerts;
		std::vector<glm::vec2> interiorTrimVerts;

		// block
		for (int i = 0; i < CLIPMAP_RESOLUTION; i++)
			for (int j = 0; j < CLIPMAP_RESOLUTION; j++)
				blockVerts.push_back(glm::vec2(j, i));

		for (int i = 0; i < CLIPMAP_RESOLUTION - 1; i++) {

			for (int j = 0; j < CLIPMAP_RESOLUTION - 1; j++) {

				blockIndices.push_back(j + i * CLIPMAP_RESOLUTION);
				blockIndices.push_back(j + (i + 1) * CLIPMAP_RESOLUTION);
				blockIndices.push_back(j + i * CLIPMAP_RESOLUTION + 1);

				blockIndices.push_back(j + i * CLIPMAP_RESOLUTION + 1);
				blockIndices.push_back(j + (i + 1) * CLIPMAP_RESOLUTION);
				blockIndices.push_back(j + (i + 1) * CLIPMAP_RESOLUTION + 1);
			}
		}

		// small square
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				smallSquareVerts.push_back(glm::vec2(j, i));

		for (int i = 0; i < 3 - 1; i++) {

			for (int j = 0; j < 3 - 1; j++) {

				smallSquareIndices.push_back(j + i * 3);
				smallSquareIndices.push_back(j + (i + 1) * 3);
				smallSquareIndices.push_back(j + i * 3 + 1);

				smallSquareIndices.push_back(j + i * 3 + 1);
				smallSquareIndices.push_back(j + (i + 1) * 3);
				smallSquareIndices.push_back(j + (i + 1) * 3 + 1);
			}
		}

		// ring fixup vertical
		for (int i = 0; i < CLIPMAP_RESOLUTION; i++)
			for (int j = 0; j < 3; j++)
				ringFixUpVerticalVerts.push_back(glm::vec2(j, i));

		for (int i = 0; i < CLIPMAP_RESOLUTION - 1; i++) {

			for (int j = 0; j < 3 - 1; j++) {

				ringFixUpVerticalIndices.push_back(j + i * 3);
				ringFixUpVerticalIndices.push_back(j + (i + 1) * 3);
				ringFixUpVerticalIndices.push_back(j + i * 3 + 1);

				ringFixUpVerticalIndices.push_back(j + i * 3 + 1);
				ringFixUpVerticalIndices.push_back(j + (i + 1) * 3);
				ringFixUpVerticalIndices.push_back(j + (i + 1) * 3 + 1);
			}
		}

		// ring fixup horizontal
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < CLIPMAP_RESOLUTION; j++)
				ringFixUpHorizontalVerts.push_back(glm::vec2(j, i));

		for (int i = 0; i < 3 - 1; i++) {

			for (int j = 0; j < CLIPMAP_RESOLUTION - 1; j++) {

				ringFixUpHorizontalIndices.push_back(i * CLIPMAP_RESOLUTION + j);
				ringFixUpHorizontalIndices.push_back((i + 1) * CLIPMAP_RESOLUTION + j);
				ringFixUpHorizontalIndices.push_back(i * CLIPMAP_RESOLUTION + j + 1);

				ringFixUpHorizontalIndices.push_back(i * CLIPMAP_RESOLUTION + j + 1);
				ringFixUpHorizontalIndices.push_back((i + 1) * CLIPMAP_RESOLUTION + j);
				ringFixUpHorizontalIndices.push_back((i + 1) * CLIPMAP_RESOLUTION + j + 1);
			}
		}

		// outer degenerate
		for (int i = 0; i <= CLIPMAP_RESOLUTION * 4 - 2; i++)
			outerDegenerateVerts.push_back(glm::vec2(i, 0));

		for (int i = 0; i < CLIPMAP_RESOLUTION * 2 - 1; i++) {
			outerDegenerateIndices.push_back(i * 2);
			outerDegenerateIndices.push_back(i * 2 + 2);
			outerDegenerateIndices.push_back(i * 2 + 1);

			outerDegenerateIndices.push_back(i * 2);
			outerDegenerateIndices.push_back(i * 2 + 1);
			outerDegenerateIndices.push_back(i * 2 + 2);
		}

		// interior trim
		for (int i = -CLIPMAP_RESOLUTION; i <= CLIPMAP_RESOLUTION; i++)
			interiorTrimVerts.push_back(glm::vec2(i, CLIPMAP_RESOLUTION - 1));

		for (int i = -CLIPMAP_RESOLUTION; i <= CLIPMAP_RESOLUTION; i++)
			interiorTrimVerts.push_back(glm::vec2(i, CLIPMAP_RESOLUTION));

		for (int i = CLIPMAP_RESOLUTION - 2; i >= -CLIPMAP_RESOLUTION; i--)
			interiorTrimVerts.push_back(glm::vec2(CLIPMAP_RESOLUTION - 1, i));

		for (int i = CLIPMAP_RESOLUTION - 2; i >= -CLIPMAP_RESOLUTION; i--)
			interiorTrimVerts.push_back(glm::vec2(CLIPMAP_RESOLUTION, i));

		/*
		* INTERIOR TRIM INDICES
		*
		* 9   8   7   6   5
		* 4   3   2   1   0
		* 13 10
		* 14 11
		* 15 12
		*/

		int size = CLIPMAP_RESOLUTION * 2;
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

		unsigned int blockVBO;
		glGenBuffers(1, &blockVBO);
		glBindBuffer(GL_ARRAY_BUFFER, blockVBO);
		glBufferData(GL_ARRAY_BUFFER, blockVerts.size() * sizeof(glm::vec2), &blockVerts[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(glm::vec2), 0);

		unsigned int EBO;
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, blockIndices.size() * sizeof(unsigned int), &blockIndices[0], GL_STATIC_DRAW);

		// Ring Fix-up Vertical
		glGenVertexArrays(1, &ringFixUpVerticalVAO);
		glBindVertexArray(ringFixUpVerticalVAO);

		unsigned int ringFixUpVerticalVBO;
		glGenBuffers(1, &ringFixUpVerticalVBO);
		glBindBuffer(GL_ARRAY_BUFFER, ringFixUpVerticalVBO);
		glBufferData(GL_ARRAY_BUFFER, ringFixUpVerticalVerts.size() * sizeof(glm::vec2), &ringFixUpVerticalVerts[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(glm::vec2), 0);

		unsigned int ringFixUpVerticalEBO;
		glGenBuffers(1, &ringFixUpVerticalEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ringFixUpVerticalEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, ringFixUpVerticalIndices.size() * sizeof(unsigned int), &ringFixUpVerticalIndices[0], GL_STATIC_DRAW);

		// Ring Fix-up Horizontal
		glGenVertexArrays(1, &ringFixUpHorizontalVAO);
		glBindVertexArray(ringFixUpHorizontalVAO);

		unsigned int ringFixUpHorizontalVBO;
		glGenBuffers(1, &ringFixUpHorizontalVBO);
		glBindBuffer(GL_ARRAY_BUFFER, ringFixUpHorizontalVBO);
		glBufferData(GL_ARRAY_BUFFER, ringFixUpHorizontalVerts.size() * sizeof(glm::vec2), &ringFixUpHorizontalVerts[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(glm::vec2), 0);

		unsigned int ringFixUpHorizontalEBO;
		glGenBuffers(1, &ringFixUpHorizontalEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ringFixUpHorizontalEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, ringFixUpHorizontalIndices.size() * sizeof(unsigned int), &ringFixUpHorizontalIndices[0], GL_STATIC_DRAW);

		// Small Square
		glGenVertexArrays(1, &smallSquareVAO);
		glBindVertexArray(smallSquareVAO);

		unsigned int smallSquareVBO;
		glGenBuffers(1, &smallSquareVBO);
		glBindBuffer(GL_ARRAY_BUFFER, smallSquareVBO);
		glBufferData(GL_ARRAY_BUFFER, smallSquareVerts.size() * sizeof(glm::vec2), &smallSquareVerts[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(glm::vec2), 0);

		unsigned int smallSquareEBO;
		glGenBuffers(1, &smallSquareEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smallSquareEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, smallSquareIndices.size() * sizeof(unsigned int), &smallSquareIndices[0], GL_STATIC_DRAW);

		// Outer Degenerate
		glGenVertexArrays(1, &outerDegenerateVAO);
		glBindVertexArray(outerDegenerateVAO);

		unsigned int outerDegenerateVBO;
		glGenBuffers(1, &outerDegenerateVBO);
		glBindBuffer(GL_ARRAY_BUFFER, outerDegenerateVBO);
		glBufferData(GL_ARRAY_BUFFER, outerDegenerateVerts.size() * sizeof(glm::vec2), &outerDegenerateVerts[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(glm::vec2), 0);

		unsigned int outerDegenerateEBO;
		glGenBuffers(1, &outerDegenerateEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outerDegenerateEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, outerDegenerateIndices.size() * sizeof(unsigned int), &outerDegenerateIndices[0], GL_STATIC_DRAW);

		// Interior Trim
		glGenVertexArrays(1, &interiorTrimVAO);
		glBindVertexArray(interiorTrimVAO);

		unsigned int interiorTrimVBO;
		glGenBuffers(1, &interiorTrimVBO);
		glBindBuffer(GL_ARRAY_BUFFER, interiorTrimVBO);
		glBufferData(GL_ARRAY_BUFFER, interiorTrimVerts.size() * sizeof(glm::vec2), &interiorTrimVerts[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(glm::vec2), 0);

		unsigned int interiorTrimEBO;
		glGenBuffers(1, &interiorTrimEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, interiorTrimEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, interiorTrimIndices.size() * sizeof(unsigned int), &interiorTrimIndices[0], GL_STATIC_DRAW);

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

		int tileSizeInReal = TILE_SIZE * (1 << level);
		glm::ivec2 clipmapPos = Terrain::getClipmapPosition(level, camPos);
		return glm::ivec2(clipmapPos.x / tileSizeInReal, clipmapPos.y / tileSizeInReal);
	}

	/*
	* Heightmap is resized according to the structure below in the function (in the comments).
	* We have to do this in order to work with high level clipmaps.
	*/
	unsigned char* Terrain::resizeHeightmap(unsigned char* heightmap, int size) {

		// Resize Structure
		// 0 0 0 0
		// 0 0 1 0
		// 0 0 0 0
		// 0 0 0 0
		int totalSize = size * size * 4 * 4 * TERRAIN_STACK_NUM_CHANNELS;
		unsigned char* remappedHeightmap = new unsigned char[totalSize];

		for (int i = 0; i < totalSize; i++)
			remappedHeightmap[i] = 0;

		int start = size * 2;

		for (int i = 0; i < size; i++) {

			for (int j = 0; j < size; j++) {

				int indexInRemappedHeightmap = ((i + start) * (size * 4) + (j + start)) * TERRAIN_STACK_NUM_CHANNELS;
				int indexInHeightmap = (i * size + j) * TERRAIN_STACK_NUM_CHANNELS;
				remappedHeightmap[indexInRemappedHeightmap] = heightmap[indexInHeightmap];
				remappedHeightmap[indexInRemappedHeightmap + 1] = heightmap[indexInHeightmap + 1];
			}
		}
		return remappedHeightmap;
	}

	/*
	* Creates mipmaps for each clipmap level. It is good to use with clipmaps since coarser level requires less data.
	*/
	unsigned char** Terrain::createMipmaps(unsigned char* heights, int size, int totalLevel) {

		unsigned char** mipmaps = new unsigned char* [totalLevel];
		mipmaps[0] = new unsigned char[size * size * TERRAIN_STACK_NUM_CHANNELS];

		for (int i = 0; i < size; i++) {

			for (int j = 0; j < size; j++) {

				int indexInImportedHeightmap = (i * size + j) * TERRAIN_STACK_NUM_CHANNELS;
				int indexInTerrainMipmap = (i * size + j) * TERRAIN_STACK_NUM_CHANNELS;
				mipmaps[0][indexInTerrainMipmap] = heights[indexInImportedHeightmap];
				mipmaps[0][indexInTerrainMipmap + 1] = heights[indexInImportedHeightmap + 1];
			}
		}

		for (int level = 1; level < totalLevel; level++) {

			size /= 2;
			mipmaps[level] = new unsigned char[size * size * 2];

			for (int i = 0; i < size; i++) {
				for (int j = 0; j < size; j++) {

					int indexInFinerLevel = (i * 2 * size * 2 + j * 2) * TERRAIN_STACK_NUM_CHANNELS;
					int indexInCoarserLevel = (i * size + j) * TERRAIN_STACK_NUM_CHANNELS;
					mipmaps[level][indexInCoarserLevel] = mipmaps[level - 1][indexInFinerLevel];
					mipmaps[level][indexInCoarserLevel + 1] = mipmaps[level - 1][indexInFinerLevel + 1];
				}
			}
		}

		return mipmaps;
	}

	/*
	* Creates heightmap stack that is used by gpu. Rather than streaming from disk, we started to stream from memory.
	* Takes input from resized image.
	*/
	void Terrain::createHeightmapStack(unsigned char** heightMapList, int width, int totalLevel) {

		heightmapStack = new unsigned char* [CLIPMAP_LEVEL];
		clipmapStartIndices = new glm::ivec2[CLIPMAP_LEVEL];
		for (int i = 0; i < CLIPMAP_LEVEL; i++)
			clipmapStartIndices[i] = glm::ivec2(0, 0);

		int res = width;

		for (int level = 0; level < totalLevel; level++) {

			int numTiles = res / TILE_SIZE;
			int start = (numTiles >> 1) - 2;
			int end = ((numTiles * 3) >> 2) + 1;

			clipmapStartIndices[level] = glm::ivec2(start,end);
			int size = (end - start) * TILE_SIZE;
			heightmapStack[level] = new unsigned char[size * size * TERRAIN_STACK_NUM_CHANNELS];

			for (int i = start * TILE_SIZE; i < end * TILE_SIZE; i++) {
				for (int j = start * TILE_SIZE; j < end * TILE_SIZE; j++) {

					int coord_x = j - start * TILE_SIZE;
					int coord_z = i - start * TILE_SIZE;

					int baseCoord = (i * res + j) * TERRAIN_STACK_NUM_CHANNELS;
					int newCoord = (coord_z * size + coord_x) * TERRAIN_STACK_NUM_CHANNELS;

					heightmapStack[level][newCoord] = heightMapList[level][baseCoord];
					heightmapStack[level][newCoord + 1] = heightMapList[level][baseCoord + 1];
				}
			}

			//// DEBUG (SUCCESFUL)
			//std::vector<unsigned char> in(size * size * 2);
			//for (int i = 0; i < size * size * 2; i++)
			//	in[i] = heightmapStack[level][i];

			//unsigned int width = size;
			//unsigned int height = size;
			//std::string imagePath = "heightmapStack" + std::to_string(level) + ".png";
			//lodepng::encode(imagePath, in, width, height, LodePNGColorType::LCT_GREY, 16);

			res /= 2;
		}
	}
}