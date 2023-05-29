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

#pragma once
#include "mesh.h"
#include "texture.h"
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"

#define TILE_SIZE 256
#define MEM_TILE_ONE_SIDE 4
#define TERRAIN_STACK_NUM_CHANNELS 2
#define MIP_STACK_DIVISOR_RATIO 8
#define MIP_STACK_SIZE (TILE_SIZE / MIP_STACK_DIVISOR_RATIO) * MEM_TILE_ONE_SIDE 
#define TERRAIN_TEXTURE_SIZE 512
#define MAX_HEIGHT 180

#define CLIPMAP_RESOLUTION 120
#define CLIPMAP_LEVEL 4

#define BLOCK_COLOR glm::vec3(1,1,1)
#define FIXUP_VERTICAL_COLOR glm::vec3(1,1,0)
#define FIXUP_HORIZONTAL_COLOR glm::vec3(1,0,1)
#define INTERIOR_TRIM_COLOR glm::vec3(0,1,1)
#define OUTER_DEGENERATE_COLOR glm::vec3(1,0,0)
#define SMALL_SQUARE_COLOR glm::vec3(0,1,0)

namespace Core {

	/// <summary>
	///  TODO:
	///  orientation culling --> View frustum culling
	///  faster file read for heightmaps (robust library)
	///  less data
	/// </summary>

	class CoreContext;

	struct TerrainVertexAttribs {

		glm::vec2 position;
		float level;
		glm::mat4 model;
		glm::vec3 color;
	};

	class  __declspec(dllexport) Terrain {

	private:

		glm::vec3 cameraPosition;

	public:

		glm::vec2* blockPositions;
		glm::vec2* ringFixUpPositions;
		glm::vec2* ringFixUpVerticalPositions;
		glm::vec2* interiorTrimPositions;
		glm::vec2* outerDegeneratePositions;
		float* rotAmounts;
		glm::vec2 smallSquarePosition;

		unsigned int programID;
		unsigned int elevationMapTexture;

		unsigned int macroTexture;
		unsigned int noiseTexture;

		unsigned int albedo0;
		unsigned int albedo1;
		unsigned int albedo2;
		unsigned int albedo3;
		unsigned int albedo4;
		unsigned int albedo5;
		unsigned int albedo6;

		unsigned int normal0;
		unsigned int normal1;
		unsigned int normal2;
		unsigned int normal3;
		unsigned int normal4;
		unsigned int normal5;
		unsigned int normal6;
		unsigned int normal7;

		std::vector<unsigned int> blockIndices;
		unsigned int blockVAO;

		unsigned int ringFixUpVerticalVAO;
		std::vector<unsigned int> ringFixUpVerticalIndices;

		unsigned int ringFixUpHorizontalVAO;
		std::vector<unsigned int> ringFixUpHorizontalIndices;

		unsigned int smallSquareVAO;
		std::vector<unsigned int> smallSquareIndices;

		unsigned int outerDegenerateVAO;
		std::vector<unsigned int> outerDegenerateIndices;

		std::vector<unsigned int> interiorTrimIndices;
		unsigned int interiorTrimVAO;

		unsigned char** heightmapStack;
		glm::ivec2* clipmapStartIndices;

		// in ui 
		glm::vec3 lightDir;
		float lightPow= 5.0f;

		float maxFog = 0.37f;
		glm::vec3 fogColor;
		float distanceNear = 1400;
		float fogBlendDistance = 2200;

		float ambientAmount = 0.75f;
		float specularPower = 4.f;
		float specularAmount = 0.4f;

		float blendDistance = 50.f;
		float blendAmount = 90.f;

		float scale_color0_dist0 = 0.75f;
		float scale_color0_dist1 = 0.05f;

		float scale_color1_dist0 = 0.75f;
		float scale_color1_dist1 = 0.05f;

		float scale_color2_dist0 = 0.60f;
		float scale_color2_dist1 = 0.06f;

		float scale_color3_dist0 = 0.5f;
		float scale_color3_dist1 = 0.033f;

		float scale_color4_dist0 = 0.75f;
		float scale_color4_dist1 = 0.05f;

		float scale_color5_dist0 = 0.75f;
		float scale_color5_dist1 = 0.05f;

		float scale_color6_dist0 = 0.80f;
		float scale_color6_dist1 = 0.05f;

		float scale_color7_dist0 = 0.80f;
		float scale_color7_dist1 = 0.05f;

		glm::vec3 color0;
		glm::vec3 color1;

		float macroScale_0 = 0.3f;
		float macroScale_1 = 0.06f;
		float macroScale_2 = 0.005f;
		float macroPower = 6.0f;
		float macroAmount = 6.25f;
		float macroOpacity = 2.25f;

		float overlayBlendScale0 = 0.165f;
		float overlayBlendAmount0 = 0.63f;
		float overlayBlendPower0 = 6.64f;
		float overlayBlendOpacity0 = 0.92f;

		float overlayBlendScale1 = 0.55f;
		float overlayBlendAmount1 = 0.6f;
		float overlayBlendPower1 = 9.f;
		float overlayBlendOpacity1 = 0.9f;

		float overlayBlendScale2 = 0.55f;
		float overlayBlendAmount2 = 0.6f;
		float overlayBlendPower2 = 9.f;
		float overlayBlendOpacity2 = 0.9f;

		float slopeSharpness0 = 0.074f;
		float slopeSharpness1 = 0.08f;

		float slopeBias0 = 0.922f;
		float slopeBias1 = 0.904f;

		float heightBias0 = 15;
		float heightSharpness0 = 7;
		float heightBias1 = 135;
		float heightSharpness1 = 5;

		Terrain();
		~Terrain();
		void start();
		void update(float dt);
		void onDraw();
		void drawElementsInstanced(unsigned int VAO, std::vector<TerrainVertexAttribs>& instanceArray, unsigned int indiceCount);
		void calculateBlockPositions(glm::vec3 camPosition);
		void initShaders(const char* vertexShader, const char* fragShader);
		void generateTerrainClipmapsVertexArrays();
		void loadTerrainHeightmapOnInit(glm::vec3 camPos, int clipmapLevel);
		void loadHeightmapAtLevel(int level, glm::vec3 camPos, unsigned char* heightData);
		void streamTerrain(glm::vec3 newCamPos);
		void streamTerrainHorizontal(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level);
		void streamTerrainVertical(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level);
		void updateHeightMapTextureArrayPartial(int level, glm::ivec2 size, glm::ivec2 position, unsigned char* heights);
		void deleteHeightmapArray(unsigned char** heightmapArray);
		void createElevationMapTextureArray(unsigned char** heightmapArray);
		glm::ivec2 getClipmapPosition(int level, glm::vec3& camPos);
		glm::ivec2 getTileIndex(int level, glm::vec3& camPos);
		unsigned char** createMipmaps(unsigned char* heights, int size, int totalLevel);
		void loadTextures();
		unsigned char* resizeHeightmap(unsigned char* heightmap, int size);
		void createHeightmapStack(unsigned char** heightMapList, int width, int totalLevel);
		void writeHeightDataToGPUBuffer(glm::ivec2 index, glm::ivec2 tileStart, int texWidth, unsigned char* heightMap, int level);
	};
}