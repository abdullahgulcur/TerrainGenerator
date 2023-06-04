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

namespace Core {

	Terrain::Terrain() {

		lightDir = glm::vec3(-0.5f, -1.f, -0.5f);
		fogColor = glm::vec3(190.f / 255, 220.f / 255, 1.f);
		color0 = glm::vec3(0.95f, 0.95f, 0.95f);
		color1 = glm::vec3(0.85f, 0.85f, 0.85f);
	}

	Terrain::~Terrain() {

		glDeleteTextures(1, &elevationMapTextureArray);
		glDeleteVertexArrays(1, &blockVAO);
		glDeleteVertexArrays(1, &ringFixUpVerticalVAO);
		glDeleteVertexArrays(1, &ringFixUpHorizontalVAO);
		glDeleteVertexArrays(1, &smallSquareVAO);
		glDeleteVertexArrays(1, &outerDegenerateVAO);
		glDeleteVertexArrays(1, &interiorTrimVAO);

		for (int i = 0; i < CLIPMAP_LEVEL; i++)
			delete[] heightmapStack[i];
		delete[] heightmapStack;

		for (int i = 0; i < CLIPMAP_LEVEL; i++)
			delete[] lowResolustionHeightmapStack[i];
		delete[] lowResolustionHeightmapStack;
	}

	void Terrain::start() {

		// limit camera position to the remapped terrain region
		cameraPosition = glm::clamp(CoreContext::instance->scene->cameraInfo.camPos, glm::vec3(MAP_SIZE * 2 + 1, 0, MAP_SIZE * 2 + 1), glm::vec3(MAP_SIZE * 3 - 1, 0, MAP_SIZE * 3 - 1));

		Terrain::initHeightmapStack("resources/textures/terrain/terrain.png");
		Terrain::createLowResolutionHeightmapStack();
		Terrain::generateTerrainClipmapsVertexArrays();
		Terrain::initShaders("resources/shaders/terrain/terrain.vert", "resources/shaders/terrain/terrain.frag");
		Terrain::loadTerrainHeightmapOnInit(cameraPosition, CLIPMAP_LEVEL);
		Terrain::calculateBlockPositions(cameraPosition);
		Terrain::initBlockAABBs();
		Terrain::loadTextures();
	}

	void Terrain::initShaders(const char* vertexShader, const char* fragShader) {

		terrainProgramID = Shader::loadShaders(vertexShader, fragShader);
		glUseProgram(terrainProgramID);
		glUniform1i(glGetUniformLocation(terrainProgramID, "heightmapArray"), 0);
		glUniform1i(glGetUniformLocation(terrainProgramID, "irradianceMap"), 1);
		glUniform1i(glGetUniformLocation(terrainProgramID, "macroTexture"), 2);
		glUniform1i(glGetUniformLocation(terrainProgramID, "noiseTexture"), 3);
		glUniform1i(glGetUniformLocation(terrainProgramID, "albedoT0"), 4);
		glUniform1i(glGetUniformLocation(terrainProgramID, "albedoT1"), 5);
		glUniform1i(glGetUniformLocation(terrainProgramID, "albedoT2"), 6);
		glUniform1i(glGetUniformLocation(terrainProgramID, "albedoT3"), 7);
		glUniform1i(glGetUniformLocation(terrainProgramID, "albedoT5"), 8);
		glUniform1i(glGetUniformLocation(terrainProgramID, "albedoT6"), 9);
		glUniform1i(glGetUniformLocation(terrainProgramID, "albedoT7"), 10);
		glUniform1i(glGetUniformLocation(terrainProgramID, "albedoT8"), 11);
		glUniform1i(glGetUniformLocation(terrainProgramID, "normalT0"), 12);
		glUniform1i(glGetUniformLocation(terrainProgramID, "normalT1"), 13);
		glUniform1i(glGetUniformLocation(terrainProgramID, "normalT2"), 14);
		glUniform1i(glGetUniformLocation(terrainProgramID, "normalT3"), 15);
		glUniform1i(glGetUniformLocation(terrainProgramID, "normalT4"), 16);
		glUniform1i(glGetUniformLocation(terrainProgramID, "normalT5"), 17);
		glUniform1i(glGetUniformLocation(terrainProgramID, "normalT6"), 18);
		glUniform1i(glGetUniformLocation(terrainProgramID, "normalT7"), 19);
		glUniform1i(glGetUniformLocation(terrainProgramID, "normalT8"), 20);
		glUniform1i(glGetUniformLocation(terrainProgramID, "normalT9"), 21);
	}

	void Terrain::initBlockAABBs() {

		for (int i = 0; i < CLIPMAP_LEVEL; i++)
			for (int j = 0; j < 12; j++)
				blockAABBs[12 * i + j] = Terrain::getBlockBoundingBox(i * 12 + j, i);
		for (int i = 0; i < 4; i++)
			blockAABBs[12 * CLIPMAP_LEVEL + i] = Terrain::getBlockBoundingBox(12 * CLIPMAP_LEVEL + i, 0);
	}

	void Terrain::initHeightmapStack(std::string path) {

		std::vector<unsigned char> out;
		unsigned int w, h;
		lodepng::decode(out, w, h, path, LodePNGColorType::LCT_GREY, 16);
		unsigned char* data = new unsigned char[w * w * TERRAIN_STACK_NUM_CHANNELS];
		for (int i = 0; i < out.size(); i++)
			data[i] = out[i];

		unsigned char* heightmap = Terrain::resizeHeightmap(data, w);
		unsigned char** heightMapList = Terrain::createMipmaps(heightmap, w * MEM_TILE_ONE_SIDE, CLIPMAP_LEVEL);
		Terrain::createHeightmapStack(heightMapList, w * MEM_TILE_ONE_SIDE);
	}

	/*
	* Loads whole heightmap from heightmap stack.
	*/
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
	 
	/*
	* Creates all the vao for the meshes of the terrain
	*/
	void Terrain::generateTerrainClipmapsVertexArrays() {

		std::vector<unsigned int> blockIndices;
		std::vector<unsigned int> ringFixUpVerticalIndices;
		std::vector<unsigned int> ringFixUpHorizontalIndices;
		std::vector<unsigned int> smallSquareIndices;
		std::vector<unsigned int> outerDegenerateIndices;
		std::vector<unsigned int> interiorTrimIndices;

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
		for (int i = 0; i < CLIPMAP_RESOLUTION * 4 - 2; i++)
			outerDegenerateVerts.push_back(glm::vec2(i, 0));

		for (int i = 0; i < CLIPMAP_RESOLUTION * 4 - 2; i++)
			outerDegenerateVerts.push_back(glm::vec2(CLIPMAP_RESOLUTION * 4 - 2, i));

		for (int i = 0; i < CLIPMAP_RESOLUTION * 4 - 2; i++)
			outerDegenerateVerts.push_back(glm::vec2(CLIPMAP_RESOLUTION * 4 - 2 - i, CLIPMAP_RESOLUTION * 4 - 2));

		for (int i = 0; i < CLIPMAP_RESOLUTION * 4 - 2; i++)
			outerDegenerateVerts.push_back(glm::vec2(0, CLIPMAP_RESOLUTION * 4 - 2 - i));

		int maxIndice = (CLIPMAP_RESOLUTION * 2 - 1) * 4 - 1;
		for (int i = 0; i < maxIndice; i++) {
			outerDegenerateIndices.push_back(i * 2);
			outerDegenerateIndices.push_back(i * 2 + 2);
			outerDegenerateIndices.push_back(i * 2 + 1);

			outerDegenerateIndices.push_back(i * 2);
			outerDegenerateIndices.push_back(i * 2 + 1);
			outerDegenerateIndices.push_back(i * 2 + 2);
		}

		{
			outerDegenerateIndices.push_back(maxIndice * 2);
			outerDegenerateIndices.push_back(0);
			outerDegenerateIndices.push_back(maxIndice * 2 + 1);

			outerDegenerateIndices.push_back(0);
			outerDegenerateIndices.push_back(maxIndice * 2);
			outerDegenerateIndices.push_back(maxIndice * 2 + 1);
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

		blockIndiceCount = blockIndices.size();
		ringFixUpVerticalIndiceCount = ringFixUpVerticalIndices.size();
		ringFixUpHorizontalIndiceCount = ringFixUpHorizontalIndices.size();
		smallSquareIndiceCount = smallSquareIndices.size();
		interiorTrimIndiceCount = interiorTrimIndices.size();
		outerDegenerateIndiceCount = outerDegenerateIndices.size();

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

	/*
	* The first time data sent to the gpu when map is loaded this function is called.
	* Or if you jump to far point of the map, all data is updated instead of toroidally.
	*/
	void Terrain::createElevationMapTextureArray(unsigned char** heightmapArray) {

		int size = TILE_SIZE * MEM_TILE_ONE_SIDE;

		if (elevationMapTextureArray)
			glDeleteTextures(1, &elevationMapTextureArray);

		glGenTextures(1, &elevationMapTextureArray);
		glBindTexture(GL_TEXTURE_2D_ARRAY, elevationMapTextureArray);

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

		albedo0 = textures.at("soilmulch_a")->textureId;
		albedo1 = textures.at("groundforest_a")->textureId;
		albedo2 = textures.at("cliffgranite_a")->textureId;
		albedo3 = textures.at("groundsandy_a")->textureId;
		albedo5 = textures.at("sandbeach_a")->textureId;
		albedo6 = textures.at("lichenedrock_a")->textureId;
		albedo7 = textures.at("grasslawn_a")->textureId;
		albedo8 = textures.at("grasswild_a")->textureId;

		normal0 = textures.at("soilmulch_n")->textureId;
		normal1 = textures.at("groundforest_n")->textureId;
		normal2 = textures.at("cliffgranite_n")->textureId;
		normal3 = textures.at("groundsandy_n")->textureId;
		normal4 = textures.at("snowfresh_n")->textureId;
		normal5 = textures.at("sandbeach_n")->textureId;
		normal6 = textures.at("lichenedrock_n")->textureId;
		normal7 = textures.at("snowpure_n")->textureId;
		normal8 = textures.at("grasslawn_n")->textureId;
		normal9 = textures.at("grasswild_n")->textureId;

		macroTexture = textures.at("gold_a")->textureId;
		noiseTexture = textures.at("noiseTexture")->textureId;
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
	void Terrain::createHeightmapStack(unsigned char** heightMapList, int width) {

		heightmapStack = new unsigned char* [CLIPMAP_LEVEL];

		for (int i = 0; i < CLIPMAP_LEVEL; i++)
			clipmapStartIndices[i] = glm::ivec2(0, 0);

		int res = width;

		for (int level = 0; level < CLIPMAP_LEVEL; level++) {

			int numTiles = res / TILE_SIZE;
			int start = (numTiles >> 1) - 2;
			int end = ((numTiles * 3) >> 2) + 1;

			clipmapStartIndices[level] = glm::ivec2(start, end);
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
			res /= 2;
		}
	}

	/*
	* Creates low resolution heightmap stack that will be used for frustum culling.
	* We do not need to know precise volume of terrain blocks for that purpose so that heightmap stack can be in low resolution.
	* In this case memory read will be lesser which will be good for runtime performance.
	*/
	void Terrain::createLowResolutionHeightmapStack() {

		lowResolustionHeightmapStack = new unsigned char* [CLIPMAP_LEVEL];

		for (int level = 0; level < CLIPMAP_LEVEL; level++) {

			int sizeInHeightmap = ((clipmapStartIndices[level].y - clipmapStartIndices[level].x) * TILE_SIZE);
			int sizeInLowResolutionHeightmap = sizeInHeightmap >> MIP_STACK_DIVISOR_POWER;
			lowResolustionHeightmapStack[level] = new unsigned char[sizeInLowResolutionHeightmap * sizeInLowResolutionHeightmap];

			unsigned char* first = new unsigned char[sizeInHeightmap * sizeInHeightmap];

			for (int i = 0; i < sizeInHeightmap; i++)
				for (int j = 0; j < sizeInHeightmap; j++)
					first[i * sizeInHeightmap + j] = heightmapStack[level][(i * sizeInHeightmap + j) * 2];

			int sizeIterator = sizeInHeightmap;
			for (int iter = 0; iter < MIP_STACK_DIVISOR_POWER; iter++) {

				sizeIterator >>= 1;
				unsigned char* second = new unsigned char[sizeIterator * sizeIterator];

				for (int i = 0; i < sizeIterator; i++) {
					for (int j = 0; j < sizeIterator; j++) {
						// c0 c1
						// c2 c3
						int c0 = first[(i * 2 * sizeIterator * 2 + j * 2)];
						int c1 = first[(i * 2 * sizeIterator * 2 + j * 2 + 1)];
						int c2 = first[((i * 2 + 1) * sizeIterator * 2 + j * 2)];
						int c3 = first[((i * 2 + 1) * sizeIterator * 2 + j * 2 + 1)];

						second[i * sizeIterator + j] = (c0 + c1 + c2 + c3) * 0.25f;
					}
				}
				delete[] first;
				first = second;
			}

			for (int i = 0; i < sizeIterator; i++)
				for (int j = 0; j < sizeIterator; j++)
					lowResolustionHeightmapStack[level][i * sizeIterator + j] = first[i * sizeIterator + j];

			delete[] first;
		}
	}

	/*
	* Any updates because of the camera movement. Nested grids positions, textures updates...
	*/
	void Terrain::update(float dt) {

		glm::vec3 camPosition = glm::clamp(CoreContext::instance->scene->cameraInfo.camPos, glm::vec3(MAP_SIZE * 2 + 1, 0, MAP_SIZE * 2 + 1), glm::vec3(MAP_SIZE * 3 - 1, 0, MAP_SIZE * 3 - 1));
		Terrain::calculateBlockPositions(camPosition);
		Terrain::calculateBoundingBoxes(camPosition);
		Terrain::streamTerrain(camPosition);
		cameraPosition = camPosition;
	}

	/*
	* Draw terrain
	*/
	void Terrain::onDraw() {

		glm::vec3 camPos = CoreContext::instance->scene->cameraInfo.camPos;
		glm::mat4& PV = CoreContext::instance->scene->cameraInfo.VP;
		Cubemap* cubemap = CoreContext::instance->scene->cubemap;

		glUseProgram(terrainProgramID);
		glUniformMatrix4fv(glGetUniformLocation(terrainProgramID, "PV"), 1, 0, &PV[0][0]);
		glUniform3fv(glGetUniformLocation(terrainProgramID, "camPos"), 1, &camPos[0]);
		glUniform1f(glGetUniformLocation(terrainProgramID, "texSize"), (float)TILE_SIZE * MEM_TILE_ONE_SIDE);

		// Terrain Material Parameters
		glUniform3f(glGetUniformLocation(terrainProgramID, "lightDirection"), lightDir.x, lightDir.y, lightDir.z);
		glUniform1f(glGetUniformLocation(terrainProgramID, "lightPow"), lightPow);

		glUniform1f(glGetUniformLocation(terrainProgramID, "ambientAmount"), ambientAmount);
		glUniform1f(glGetUniformLocation(terrainProgramID, "specularPower"), specularPower);
		glUniform1f(glGetUniformLocation(terrainProgramID, "specularAmount"), specularAmount);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "blendDistance"), blendDistance);
		glUniform1f(glGetUniformLocation(terrainProgramID, "blendAmount"), blendAmount);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color0_dist0"), scale_color0_dist0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color0_dist1"), scale_color0_dist1);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color1_dist0"), scale_color1_dist0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color1_dist1"), scale_color1_dist1);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color2_dist0"), scale_color2_dist0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color2_dist1"), scale_color2_dist1);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color3_dist0"), scale_color3_dist0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color3_dist1"), scale_color3_dist1);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color4_dist0"), scale_color4_dist0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color4_dist1"), scale_color4_dist1);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color5_dist0"), scale_color5_dist0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color5_dist1"), scale_color5_dist1);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color6_dist0"), scale_color6_dist0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color6_dist1"), scale_color6_dist1);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color7_dist0"), scale_color7_dist0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color7_dist1"), scale_color7_dist1);

		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color8_dist0"), scale_color8_dist0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color8_dist1"), scale_color8_dist1);

		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color9_dist0"), scale_color9_dist0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "scale_color9_dist1"), scale_color9_dist1);

		glUniform1f(glGetUniformLocation(terrainProgramID, "macroScale_0"), macroScale_0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "macroScale_1"), macroScale_1);
		glUniform1f(glGetUniformLocation(terrainProgramID, "macroScale_2"), macroScale_2);
		glUniform1f(glGetUniformLocation(terrainProgramID, "macroAmount"), macroAmount);
		glUniform1f(glGetUniformLocation(terrainProgramID, "macroPower"), macroPower);
		glUniform1f(glGetUniformLocation(terrainProgramID, "macroOpacity"), macroOpacity);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendScale0"), overlayBlendScale0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendAmount0"), overlayBlendAmount0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendPower0"), overlayBlendPower0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendOpacity0"), overlayBlendOpacity0);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendScale1"), overlayBlendScale1);
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendAmount1"), overlayBlendAmount1);
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendPower1"), overlayBlendPower1);
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendOpacity1"), overlayBlendOpacity1);

		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendScale2"), overlayBlendScale2);
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendAmount2"), overlayBlendAmount2);
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendPower2"), overlayBlendPower2);
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendOpacity2"), overlayBlendOpacity2);

		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendScale3"), overlayBlendScale3);
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendAmount3"), overlayBlendAmount3);
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendPower3"), overlayBlendPower3);
		glUniform1f(glGetUniformLocation(terrainProgramID, "overlayBlendOpacity3"), overlayBlendOpacity3);

		glUniform3fv(glGetUniformLocation(terrainProgramID, "color0"), 1, &color0[0]);
		glUniform3fv(glGetUniformLocation(terrainProgramID, "color1"), 1, &color1[0]);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "slopeSharpness0"), slopeSharpness0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "slopeSharpness1"), slopeSharpness1);
		glUniform1f(glGetUniformLocation(terrainProgramID, "slopeSharpness2"), slopeSharpness2);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "slopeBias0"), slopeBias0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "slopeBias1"), slopeBias1);
		glUniform1f(glGetUniformLocation(terrainProgramID, "slopeBias2"), slopeBias2);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "heightBias0"), heightBias0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "heightSharpness0"), heightSharpness0);
		glUniform1f(glGetUniformLocation(terrainProgramID, "heightBias1"), heightBias1);
		glUniform1f(glGetUniformLocation(terrainProgramID, "heightSharpness1"), heightSharpness1);
					
		glUniform1f(glGetUniformLocation(terrainProgramID, "distanceNear"), distanceNear);
		glUniform1f(glGetUniformLocation(terrainProgramID, "fogBlendDistance"), fogBlendDistance);
		glUniform1f(glGetUniformLocation(terrainProgramID, "maxFog"), maxFog);
		glUniform3fv(glGetUniformLocation(terrainProgramID, "fogColor"), 1, &fogColor[0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, elevationMapTextureArray);
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
		glBindTexture(GL_TEXTURE_2D, albedo7);
		glActiveTexture(GL_TEXTURE11);
		glBindTexture(GL_TEXTURE_2D, albedo8);

		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D, normal0);
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_2D, normal1);
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_2D, normal2);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, normal3);
		glActiveTexture(GL_TEXTURE16);
		glBindTexture(GL_TEXTURE_2D, normal4);
		glActiveTexture(GL_TEXTURE17);
		glBindTexture(GL_TEXTURE_2D, normal5);
		glActiveTexture(GL_TEXTURE18);
		glBindTexture(GL_TEXTURE_2D, normal6);
		glActiveTexture(GL_TEXTURE19);
		glBindTexture(GL_TEXTURE_2D, normal7);
		glActiveTexture(GL_TEXTURE20);
		glBindTexture(GL_TEXTURE_2D, normal8);
		glActiveTexture(GL_TEXTURE21);
		glBindTexture(GL_TEXTURE_2D, normal9);

		std::vector<TerrainVertexAttribs> instanceArray;

		// BLOCKS
		for (int i = 0; i < CLIPMAP_LEVEL; i++) {

			for (int j = 0; j < 12; j++) {

				glm::vec4 startInWorldSpace;
				glm::vec4 endInWorldSpace;
				AABB_Box aabb = blockAABBs[i * 12 + j];
				startInWorldSpace = aabb.start;
				endInWorldSpace = aabb.end;

				if (Terrain::intersectsAABB(startInWorldSpace, endInWorldSpace)) {
					TerrainVertexAttribs attribs;
					attribs.level = i;
					attribs.model = glm::mat4(1);
					attribs.position = blockPositions[i * 12 + j];
					attribs.color = BLOCK_COLOR;
					instanceArray.push_back(attribs);
				}
			}
		}

		for (int i = 0; i < 4; i++) {

			glm::vec4 startInWorldSpace;
			glm::vec4 endInWorldSpace;
			AABB_Box aabb = blockAABBs[CLIPMAP_LEVEL * 12 + i];
			startInWorldSpace = aabb.start;
			endInWorldSpace = aabb.end;

			//if (Terrain::intersectsAABB(startInWorldSpace, endInWorldSpace)) {
			TerrainVertexAttribs attribs;
			attribs.level = 0;
			attribs.model = glm::mat4(1);
			attribs.position = blockPositions[CLIPMAP_LEVEL * 12 + i];
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
			attribs.position = ringFixUpVerticalPositions[i * 2 + 0];
			instanceArray.push_back(attribs);
			attribs.position = ringFixUpVerticalPositions[i * 2 + 1];
			instanceArray.push_back(attribs);
		}

		{
			TerrainVertexAttribs attribs;
			attribs.level = 0;
			attribs.model = glm::mat4(1);
			attribs.color = FIXUP_VERTICAL_COLOR;
			attribs.position = ringFixUpVerticalPositions[CLIPMAP_LEVEL * 2 + 0];
			instanceArray.push_back(attribs);
			attribs.position = ringFixUpVerticalPositions[CLIPMAP_LEVEL * 2 + 1];
			instanceArray.push_back(attribs);
		}

		Terrain::drawElementsInstanced(ringFixUpVerticalVAO, instanceArray, ringFixUpVerticalIndiceCount);
		instanceArray.clear();

		// RING FIXUP HORIZONTAL
		for (int i = 0; i < CLIPMAP_LEVEL; i++) {

			TerrainVertexAttribs attribs;
			attribs.level = i;
			attribs.model = glm::mat4(1);
			attribs.color = FIXUP_HORIZONTAL_COLOR;
			attribs.position = ringFixUpHorizontalPositions[i * 2 + 0];
			instanceArray.push_back(attribs);
			attribs.position = ringFixUpHorizontalPositions[i * 2 + 1];
			instanceArray.push_back(attribs);
		}

		{
			TerrainVertexAttribs attribs;
			attribs.level = 0;
			attribs.model = glm::mat4(1);
			attribs.color = FIXUP_HORIZONTAL_COLOR;
			attribs.position = ringFixUpHorizontalPositions[CLIPMAP_LEVEL * 2 + 0];
			instanceArray.push_back(attribs);
			attribs.position = ringFixUpHorizontalPositions[CLIPMAP_LEVEL * 2 + 1];
			instanceArray.push_back(attribs);
		}

		Terrain::drawElementsInstanced(ringFixUpHorizontalVAO, instanceArray, ringFixUpHorizontalIndiceCount);
		instanceArray.clear();

		// INTERIOR TRIM
		for (int i = 0; i < CLIPMAP_LEVEL - 1; i++) {

			TerrainVertexAttribs attribs;
			attribs.level = i + 1;
			attribs.model = glm::rotate(glm::mat4(1), glm::radians(rotAmounts[i]), glm::vec3(0.0f, 1.0f, 0.0f));
			attribs.position = interiorTrimPositions[i];
			attribs.color = INTERIOR_TRIM_COLOR;
			instanceArray.push_back(attribs);
		}
		Terrain::drawElementsInstanced(interiorTrimVAO, instanceArray, interiorTrimIndiceCount);
		instanceArray.clear();

		// OUTER DEGENERATE
		for (int i = 0; i < CLIPMAP_LEVEL - 1; i++) {

			TerrainVertexAttribs attribs;
			attribs.level = i;
			attribs.model = glm::mat4(1);
			attribs.color = OUTER_DEGENERATE_COLOR;
			attribs.position = outerDegeneratePositions[i];
			instanceArray.push_back(attribs);
		}
		Terrain::drawElementsInstanced(outerDegenerateVAO, instanceArray, outerDegenerateIndiceCount);
		instanceArray.clear();

		// SMALL SQUARE
		TerrainVertexAttribs attribs;
		attribs.level = 0;
		attribs.model = glm::mat4(1);
		attribs.position = smallSquarePosition;
		attribs.color = SMALL_SQUARE_COLOR;
		instanceArray.push_back(attribs);
		Terrain::drawElementsInstanced(smallSquareVAO, instanceArray, smallSquareIndiceCount);

		// Draw bounding boxes
		if (showBounds) {
			for (int i = 0; i < CLIPMAP_LEVEL; i++) {

				for (int j = 0; j < 12; j++) {

					AABB_Box aabb = blockAABBs[i * 12 + j];
					glm::vec3 pos = (aabb.start + aabb.end) * 0.5f;
					glm::vec3 scale = aabb.end - aabb.start;
					glm::mat4 model = glm::translate(glm::mat4(1), pos) * glm::scale(glm::mat4(1), scale);
					glm::mat4 PVM = PV * model;
					glm::vec3 color = glm::vec3(1, 1, 1);
					CoreContext::instance->renderer->drawBoundingBoxVAO(PVM, color);
				}
			}

			for (int i = 0; i < 4; i++) {

				AABB_Box aabb = blockAABBs[CLIPMAP_LEVEL * 12 + i];
				glm::vec3 pos = (aabb.start + aabb.end) * 0.5f;
				glm::vec3 scale = aabb.end - aabb.start;
				glm::mat4 model = glm::translate(glm::mat4(1), pos) * glm::scale(glm::mat4(1), scale);
				glm::mat4 PVM = PV * model;
				glm::vec3 color = glm::vec3(1, 1, 1);
				CoreContext::instance->renderer->drawBoundingBoxVAO(PVM, color);
			}
		}
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
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2)));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2) + sizeof(float)));
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2) + sizeof(float) + sizeof(glm::vec4)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2) + sizeof(float) + sizeof(glm::vec4) * 2));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2) + sizeof(float) + sizeof(glm::vec4) * 3));
		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexAttribs), (void*)(sizeof(glm::vec2) + sizeof(float) + sizeof(glm::vec4) * 4));

		glVertexAttribDivisor(1, 1);
		glVertexAttribDivisor(2, 1);
		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);
		glVertexAttribDivisor(7, 1);

		glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(indiceCount), GL_UNSIGNED_INT, 0, instanceArray.size());

		glBindVertexArray(0);
		glDeleteBuffers(1, &instanceBuffer);
	}

	/*
	* When camera moves, nested grids positions are also updated in this function.
	*/
	void Terrain::calculateBlockPositions(glm::vec3 camPosition) {

		for (int i = 0; i < CLIPMAP_LEVEL; i++) {
			/*
			*      Z+
			*      ^
			*      |  This is our reference for numbers
			* x+ <-
			*/
			// Blocks move periodically according to camera's position.
			// For example:
			// Cam pos          : 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ...
			// Block at level 0 : 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10 ...
			// Block at level 1 : 0, 0, 0, 0, 4, 4, 4, 4, 8, 8, 8  ...

			float scale = 1 << i;
			int requiredCameraDistance = PATCH_WIDTH * scale;
			float pX = (int)(camPosition.x / requiredCameraDistance) * PATCH_WIDTH;
			float pZ = (int)(camPosition.z / requiredCameraDistance) * PATCH_WIDTH;
			int pI0 = PATCH_WIDTH - 2 * (CLIPMAP_RESOLUTION);
			int pI1 = PATCH_WIDTH - (CLIPMAP_RESOLUTION + 1);
			int pI2 = PATCH_WIDTH;
			int pI3 = PATCH_WIDTH + (CLIPMAP_RESOLUTION - 1);

			// For outer trim rotation. This '2' at the end is constant. Because of binary movement style
			int rotX = (int)((pX * scale) / requiredCameraDistance) % 2;
			int rotZ = (int)((pZ * scale) / requiredCameraDistance) % 2;

			/*   BLOCKS
			*  0 11 10  9
			*  1        8
			*  2        7
			*  3  4  5  6
			*/
			blockPositions[i * 12 + 0] = glm::vec2((pX + pI3) * scale, (pZ + pI3) * scale); // 0
			blockPositions[i * 12 + 1] = glm::vec2((pX + pI3) * scale, (pZ + pI2) * scale); // 1
			blockPositions[i * 12 + 2] = glm::vec2((pX + pI3) * scale, (pZ + pI1) * scale); // 2
			blockPositions[i * 12 + 3] = glm::vec2((pX + pI3) * scale, (pZ + pI0) * scale); // 3
			blockPositions[i * 12 + 4] = glm::vec2((pX + pI2) * scale, (pZ + pI0) * scale); // 4
			blockPositions[i * 12 + 5] = glm::vec2((pX + pI1) * scale, (pZ + pI0) * scale); // 5
			blockPositions[i * 12 + 6] = glm::vec2((pX + pI0) * scale, (pZ + pI0) * scale); // 6
			blockPositions[i * 12 + 7] = glm::vec2((pX + pI0) * scale, (pZ + pI1) * scale); // 7
			blockPositions[i * 12 + 8] = glm::vec2((pX + pI0) * scale, (pZ + pI2) * scale); // 8
			blockPositions[i * 12 + 9] = glm::vec2((pX + pI0) * scale, (pZ + pI3) * scale); // 9
			blockPositions[i * 12 + 10] = glm::vec2((pX + pI1) * scale, (pZ + pI3) * scale); // 10
			blockPositions[i * 12 + 11] = glm::vec2((pX + pI2) * scale, (pZ + pI3) * scale); // 11

			/* RING FIX-UP VERTICAL
			*  0
			*  1
			*/
			ringFixUpVerticalPositions[i * 2 + 0] = glm::vec2(pX * scale, (pZ + pI3) * scale); // 0
			ringFixUpVerticalPositions[i * 2 + 1] = glm::vec2(pX * scale, (pZ + pI0) * scale); // 1;

			/* RING FIX-UP HORIZONTAL
			*  0   1
			*/
			ringFixUpHorizontalPositions[i * 2 + 0] = glm::vec2((pX + pI3) * scale, pZ * scale); // 0
			ringFixUpHorizontalPositions[i * 2 + 1] = glm::vec2((pX + pI0) * scale, pZ * scale); // 1

			// INTERIOR TRIM
			interiorTrimPositions[i] = glm::vec2((pX + PATCH_WIDTH * (1 - rotX)) * scale, (pZ + PATCH_WIDTH * (1 - rotZ)) * scale);

			if (rotX == 0 && rotZ == 0)
				rotAmounts[i] = 0.f;
			if (rotX == 0 && rotZ == 1)
				rotAmounts[i] = 90.f;
			if (rotX == 1 && rotZ == 0)
				rotAmounts[i] = 270.f;
			if (rotX == 1 && rotZ == 1)
				rotAmounts[i] = 180.f;

			// OUTER DEGENERATE
			outerDegeneratePositions[i] = glm::vec2((pX + pI0) * scale, (pZ + pI0) * scale);
		}

		float pX = (int)(camPosition.x / PATCH_WIDTH) * PATCH_WIDTH;
		float pZ = (int)(camPosition.z / PATCH_WIDTH) * PATCH_WIDTH;

		int pI0 = PATCH_WIDTH - 2 * (CLIPMAP_RESOLUTION);
		int pI1 = PATCH_WIDTH - (CLIPMAP_RESOLUTION + 1);
		int pI2 = PATCH_WIDTH;
		int pI3 = PATCH_WIDTH + (CLIPMAP_RESOLUTION - 1);

		// 0 3
		// 1 2
		blockPositions[CLIPMAP_LEVEL * 12 + 0] = glm::vec2((pX + pI2), (pZ + pI2));
		blockPositions[CLIPMAP_LEVEL * 12 + 1] = glm::vec2((pX + pI2), (pZ + pI1));
		blockPositions[CLIPMAP_LEVEL * 12 + 2] = glm::vec2((pX + pI1), (pZ + pI1));
		blockPositions[CLIPMAP_LEVEL * 12 + 3] = glm::vec2((pX + pI1), (pZ + pI2));

		// 0
		// 1
		ringFixUpVerticalPositions[CLIPMAP_LEVEL * 2 + 0] = glm::vec2(pX, pZ + PATCH_WIDTH);
		ringFixUpVerticalPositions[CLIPMAP_LEVEL * 2 + 1] = glm::vec2(pX, pZ + 1 - CLIPMAP_RESOLUTION);

		// 0 1
		ringFixUpHorizontalPositions[CLIPMAP_LEVEL * 2 + 0] = glm::vec2(pX + PATCH_WIDTH, pZ);
		ringFixUpHorizontalPositions[CLIPMAP_LEVEL * 2 + 1] = glm::vec2(pX + 1 - CLIPMAP_RESOLUTION, pZ);

		smallSquarePosition = glm::vec2(pX, pZ);
	}

	/*
	* Streams terrain height information as camera moves.
	*/
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
	* Loads heightmap on a specific level from heightmap stack.
	*/
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
	}

	/*
	* Partially update heightmap texture in gpu memory
	*/
	void Terrain::updateHeightMapTextureArrayPartial(int level, glm::ivec2 size, glm::ivec2 position, unsigned char* heights) {

		glBindTexture(GL_TEXTURE_2D_ARRAY, elevationMapTextureArray);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, position.x, position.y, level, size.x, size.y, 1, GL_RG, GL_UNSIGNED_BYTE, &heights[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	/*
	* Since blocks are moving as camera moves, we have to calculate bounding box of block each time block position is changed.
	*/
	void Terrain::calculateBoundingBoxes(glm::vec3 camPos) {

		for (int level = 0; level < CLIPMAP_LEVEL; level++) {

			glm::ivec2 old_clipmapPos = Terrain::getClipmapPosition(level, cameraPosition);
			glm::ivec2 new_clipmapPos = Terrain::getClipmapPosition(level, camPos);

			if (old_clipmapPos.x != new_clipmapPos.x || old_clipmapPos.y != new_clipmapPos.y) {
				for (int i = 0; i < 12; i++)
					blockAABBs[level * 12 + i] = Terrain::getBlockBoundingBox(level * 12 + i, level);
			}
		}

		glm::ivec2 old_clipmapPos = Terrain::getClipmapPosition(0, cameraPosition);
		glm::ivec2 new_clipmapPos = Terrain::getClipmapPosition(0, camPos);

		if (old_clipmapPos.x != new_clipmapPos.x || old_clipmapPos.y != new_clipmapPos.y) {
			for (int i = 0; i < 4; i++)
				blockAABBs[CLIPMAP_LEVEL * 12 + i] = Terrain::getBlockBoundingBox(CLIPMAP_LEVEL * 12 + i, 0);
		}
	}

	/*
	* Each nested block has bounding box that will be used for frustum culling.
	*/
	AABB_Box Terrain::getBlockBoundingBox(int index, int level) {

		glm::ivec2 blockPositionInWorldSpace = blockPositions[index];

		int startPos = clipmapStartIndices[level].x;
		int endPos = clipmapStartIndices[level].y;
		int startPosWorldSpace = (startPos * TILE_SIZE) << level;

		int sizeInHeightmap = (endPos - startPos) * TILE_SIZE;
		int lowResolutionHeightmapTextureSize = sizeInHeightmap >> MIP_STACK_DIVISOR_POWER;

		glm::ivec2 startPosInHeightmapWorldSpace = glm::ivec2(startPosWorldSpace, startPosWorldSpace);
		glm::ivec2 offsetWorldSpace = blockPositionInWorldSpace - startPosInHeightmapWorldSpace;

		// this is where clipmap position is equivalent in low resolution heightmap
		glm::ivec2 offsetInLowResolutionHeightmap = (offsetWorldSpace >> level) >> MIP_STACK_DIVISOR_POWER;
		
		int blockSizeInLowResolutionHeightmapStack = CLIPMAP_RESOLUTION >> MIP_STACK_DIVISOR_POWER;
		int blockSizeInWorldSpace = (CLIPMAP_RESOLUTION - 1) << level;

		// size value of clipmap in low resolution heightmap
		glm::ivec2 sizeInLowResolutionHeightmapStack = glm::ivec2(blockSizeInLowResolutionHeightmapStack, blockSizeInLowResolutionHeightmapStack);

		unsigned char min = 255;
		unsigned char max = 0;

		int startX = offsetInLowResolutionHeightmap.x;
		int endX = startX + sizeInLowResolutionHeightmapStack.x;
		int startZ = offsetInLowResolutionHeightmap.y;
		int endZ = startZ + sizeInLowResolutionHeightmapStack.y;

		for (int i = startZ; i < endZ; i++) {
			for (int j = startX; j < endX; j++) {
				
				int index = i * lowResolutionHeightmapTextureSize + j;
				if (lowResolustionHeightmapStack[level][index] < min)
					min = lowResolustionHeightmapStack[level][index];

				if (lowResolustionHeightmapStack[level][index] > max)
					max = lowResolustionHeightmapStack[level][index];
			}
		}

		float margin = 1.f;
		glm::vec4 corner0(blockPositionInWorldSpace.x - margin, min * (MAX_HEIGHT / 255.f) - margin, blockPositionInWorldSpace.y - margin, 1);
		glm::vec4 corner1(blockPositionInWorldSpace.x + blockSizeInWorldSpace + margin, max * (MAX_HEIGHT / 255.f) + margin, blockPositionInWorldSpace.y + blockSizeInWorldSpace + margin, 1);

		AABB_Box boundingBox;
		boundingBox.start = corner0;
		boundingBox.end = corner1;
		return boundingBox;
	}

	// ref: https://arm-software.github.io/opengl-es-sdk-for-android/terrain.html
	bool Terrain::intersectsAABB(glm::vec4& start, glm::vec4& end) {
		// If all corners of an axis-aligned bounding box are on the "wrong side" (negative distance)
		// of at least one of the frustum planes, we can safely cull the mesh.
		glm::vec4 corners[8];

		corners[0] = glm::vec4(start.x, start.y, start.z, 1);
		corners[1] = glm::vec4(start.x, start.y, end.z, 1);
		corners[2] = glm::vec4(start.x, end.y, start.z, 1);
		corners[3] = glm::vec4(start.x, end.y, end.z, 1);
		corners[4] = glm::vec4(end.x, start.y, start.z, 1);
		corners[5] = glm::vec4(end.x, start.y, end.z, 1);
		corners[6] = glm::vec4(end.x, end.y, start.z, 1);
		corners[7] = glm::vec4(end.x, end.y, end.z, 1);

		for (unsigned int p = 0; p < 6; p++)
		{
			bool inside_plane = false;
			for (unsigned int c = 0; c < 8; c++)
			{
				// If dot product > 0, we're "inside" the frustum plane,
				// otherwise, outside.
				glm::vec4 plane = CoreContext::instance->scene->cameraInfo.planes[p];
				if (glm::dot(corners[c], plane) > 0.0f)
				{
					inside_plane = true;
					break;
				}
			}
			if (!inside_plane)
				return false;
		}
		return true;
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
}