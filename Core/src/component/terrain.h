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
#define MIP_STACK_DIVISOR_POWER 3
#define TERRAIN_TEXTURE_SIZE 1024
#define MAX_HEIGHT 180
#define MAP_SIZE 4096

#define CLIPMAP_RESOLUTION 120
#define CLIPMAP_LEVEL 4
#define PATCH_WIDTH 2

#define BLOCK_COLOR glm::vec3(1,1,1)
#define FIXUP_VERTICAL_COLOR glm::vec3(1,1,0)
#define FIXUP_HORIZONTAL_COLOR glm::vec3(1,0,1)
#define INTERIOR_TRIM_COLOR glm::vec3(0,1,1)
#define OUTER_DEGENERATE_COLOR glm::vec3(1,0,0)
#define SMALL_SQUARE_COLOR glm::vec3(0,1,0)

#define BLOCK_COUNT 12 * CLIPMAP_LEVEL + 4
#define RINGFIXUP_COUNT 2 * CLIPMAP_LEVEL + 2

namespace Core {

	/// <summary>
	///  TODO:
	///  less data
	///  bugs: getClipmap pos and grid poses are relatively not equal when camera goes negative direction. Importance: less
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

		AABB_Box blockAABBs[BLOCK_COUNT];
		glm::vec2 blockPositions[BLOCK_COUNT];
		glm::vec2 ringFixUpVerticalPositions[RINGFIXUP_COUNT];
		glm::vec2 ringFixUpHorizontalPositions[RINGFIXUP_COUNT];
		glm::vec2 interiorTrimPositions[CLIPMAP_LEVEL];
		glm::vec2 outerDegeneratePositions[CLIPMAP_LEVEL];
		glm::vec2 smallSquarePosition;
		float rotAmounts[CLIPMAP_LEVEL];

		unsigned int terrainProgramID;
		unsigned int elevationMapTextureArray;

		/* Solution textures to get rid of tiling effect of the terrain */
		unsigned int macroTexture;
		unsigned int noiseTexture;

		/* Terrain albedo textures */
		unsigned int albedo0;
		unsigned int albedo1;
		unsigned int albedo2;
		unsigned int albedo3;
		unsigned int albedo4;
		unsigned int albedo5;
		unsigned int albedo6;
		unsigned int albedo7;
		unsigned int albedo8;

		/* Terrain normal textures */
		unsigned int normal0;
		unsigned int normal1;
		unsigned int normal2;
		unsigned int normal3;
		unsigned int normal4;
		unsigned int normal5;
		unsigned int normal6;
		unsigned int normal7;
		unsigned int normal8;
		unsigned int normal9;

		/* For the geometry */
		unsigned int blockVAO;
		unsigned int blockIndiceCount;
		unsigned int ringFixUpVerticalVAO;
		unsigned int ringFixUpVerticalIndiceCount;
		unsigned int ringFixUpHorizontalVAO;
		unsigned int ringFixUpHorizontalIndiceCount;
		unsigned int smallSquareVAO;
		unsigned int smallSquareIndiceCount;
		unsigned int outerDegenerateVAO;
		unsigned int outerDegenerateIndiceCount;
		unsigned int interiorTrimVAO;
		unsigned int interiorTrimIndiceCount;

		/*
		* Heightmap stack is used by program while running to get 
		* terrain height values to give shape of the terrain
		*/
		unsigned char** heightmapStack;
		glm::ivec2 clipmapStartIndices[CLIPMAP_LEVEL];

		/*
		* Low resolution heightmap stack is for calculating bounding box
		* of each blocks that is used by frustum culling algorithm.
		*/
		unsigned char** lowResolustionHeightmapStack;

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

		float scale_color8_dist0 = 0.80f;
		float scale_color8_dist1 = 0.05f;

		float scale_color9_dist0 = 0.80f;
		float scale_color9_dist1 = 0.05f;

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

		float overlayBlendScale3 = 0.55f;
		float overlayBlendAmount3 = 0.6f;
		float overlayBlendPower3 = 9.f;
		float overlayBlendOpacity3 = 0.9f;

		float slopeSharpness0 = 0.074f;
		float slopeSharpness1 = 0.08f;
		float slopeSharpness2 = 0.08f;

		float slopeBias0 = 0.922f;
		float slopeBias1 = 0.904f;
		float slopeBias2 = 0.904f;

		float heightBias0 = 15;
		float heightSharpness0 = 7;
		float heightBias1 = 135;
		float heightSharpness1 = 5;

		bool showBounds = false;

		Terrain();
		~Terrain();
		void start();
		void initShaders(const char* vertexShader, const char* fragShader);
		void initBlockAABBs();
		void initHeightmapStack(std::string path);
		void loadTerrainHeightmapOnInit(glm::vec3 camPos, int clipmapLevel);
		void generateTerrainClipmapsVertexArrays();
		void createElevationMapTextureArray(unsigned char** heightmapArray);
		void loadTextures();
		unsigned char* resizeHeightmap(unsigned char* heightmap, int size);
		unsigned char** createMipmaps(unsigned char* heights, int size, int totalLevel);
		void createHeightmapStack(unsigned char** heightMapList, int width);
		void createLowResolutionHeightmapStack();
		void update(float dt);
		void onDraw();
		void drawElementsInstanced(unsigned int VAO, std::vector<TerrainVertexAttribs>& instanceArray, unsigned int indiceCount);
		void calculateBlockPositions(glm::vec3 camPosition);
		void streamTerrain(glm::vec3 newCamPos);
		void streamTerrainHorizontal(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level);
		void streamTerrainVertical(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level);
		void writeHeightDataToGPUBuffer(glm::ivec2 index, glm::ivec2 tileStart, int texWidth, unsigned char* heightMap, int level);
		void loadHeightmapAtLevel(int level, glm::vec3 camPos, unsigned char* heightData);
		void updateHeightMapTextureArrayPartial(int level, glm::ivec2 size, glm::ivec2 position, unsigned char* heights);
		void calculateBoundingBoxes(glm::vec3 camPos);
		AABB_Box getBlockBoundingBox(int index, int level);
		bool intersectsAABB(glm::vec4& start, glm::vec4& end);
		glm::ivec2 getClipmapPosition(int level, glm::vec3& camPos);
		glm::ivec2 getTileIndex(int level, glm::vec3& camPos);
	};
}