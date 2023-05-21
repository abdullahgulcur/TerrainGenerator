#pragma once
#include "component.h"
#include "mesh.h"
#include "texture.h"
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"

#define RESOLUTION 8192
#define TILE_SIZE 256
#define MEM_TILE_ONE_SIDE 4
#define MIP_STACK_DIVISOR_RATIO 8
#define MIP_STACK_SIZE (TILE_SIZE / MIP_STACK_DIVISOR_RATIO) * MEM_TILE_ONE_SIDE 
#define TERRAIN_TEXTURE_SIZE 512
#define TERRAIN_TEXTURE_ARRAY_SIZE 7
#define TERRAIN_TEXTURE_MIPMAP_COUNT 9
#define MAX_HEIGHT 180

namespace Core {

	/// <summary>
	///  TODO:
	///  View frustum culling
	///  faster file read for heightmaps
	///  merge functions in a single func
	///  less data
	/// </summary>

	class CoreContext;

	struct TerrainVertexAttribs {

		glm::vec2 position;
		glm::vec2 clipmapcenter;
		float level;
		glm::mat4 model;
	};

	class  __declspec(dllexport) Terrain : public Component {

	private:

		glm::vec3 cameraPosition;

		// no streaming
		// one octree 
		// custom structs for foliage
		// 
		// grass 0 transforms
		// grass 1 transforms
		// grass 2 transforms
		// 
		// bush 0 transforms
		// bush 1 transforms
		// bush 2 transforms
		//
		// tree 0 transforms
		// tree 1 transforms
		// tree 2 transforms

	public:

		glm::vec2* blockPositions;
		glm::vec2* ringFixUpPositions;
		glm::vec2* interiorTrimPositions;
		glm::vec2* outerDegeneratePositions;
		float* rotAmounts;
		glm::vec2 smallSquarePosition;

		AABB_Box* blockAABBs;

		unsigned short clipmapResolution = 120;

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

		std::vector<unsigned int> blockIndices;
		unsigned int blockVAO;

		unsigned int ringFixUpVAO;
		std::vector<unsigned int> ringFixUpIndices;

		unsigned int smallSquareVAO;
		std::vector<unsigned int> smallSquareIndices;

		unsigned int outerDegenerateVAO;
		std::vector<unsigned int> outerDegenerateIndices;

		std::vector<unsigned int> interiorTrimIndices;
		unsigned int interiorTrimVAO;

		unsigned char** heights;
		unsigned char** mipStack;

		// in ui 
		glm::vec3 lightDir;

		float maxFog = 0.37f;
		glm::vec3 fogColor;
		float distanceNear = 1400;
		float fogBlendDistance = 2200;

		float ambientAmount = 0.75f;
		float specularPower = 4.f;
		float specularAmount = 0.4f;

		float blendDistance0 = 50.f;
		float blendAmount0 = 90.f;
		float blendDistance1 = 150.f;
		float blendAmount1 = 200.f;

		float scale_color0_dist0 = 0.75f;
		float scale_color0_dist1 = 0.05f;
		float scale_color0_dist2 = 0.02f;

		float scale_color1_dist0 = 0.75f;
		float scale_color1_dist1 = 0.05f;
		float scale_color1_dist2 = 0.02f;

		float scale_color2_dist0 = 0.60f;
		float scale_color2_dist1 = 0.06f;
		float scale_color2_dist2 = 0.02f;

		float scale_color3_dist0 = 0.5f;
		float scale_color3_dist1 = 0.033f;
		float scale_color3_dist2 = 0.02f;

		float scale_color4_dist0 = 0.75f;
		float scale_color4_dist1 = 0.05f;
		float scale_color4_dist2 = 0.02f;

		float scale_color5_dist0 = 0.75f;
		float scale_color5_dist1 = 0.05f;
		float scale_color5_dist2 = 0.02f;

		float scale_color6_dist0 = 0.80f;
		float scale_color6_dist1 = 0.05f;
		float scale_color6_dist2 = 0.02f;

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

		float slopeSharpness0 = 0.074f;
		float slopeSharpness1 = 0.08f;
		float slopeSharpness2 = 0.12f;

		float slopeBias0 = 0.922f;
		float slopeBias1 = 0.904f;
		float slopeBias2 = 0.88f;

		float heightBias0 = 15;
		float heightSharpness0 = 7;
		float heightBias1 = 135;
		float heightSharpness1 = 5;

		Terrain();
		~Terrain();
		void start();
		void update(glm::vec3 camPos, float dt);
		void onDraw(glm::mat4& pv, glm::vec3& pos);
		void drawElementsInstanced(int size, unsigned int VAO, std::vector<TerrainVertexAttribs>& instanceArray, unsigned int indiceCount);
		void calculateBlockPositions(glm::vec3 camPosition, int level);
		AABB_Box getBoundingBoxOfClipmap(int clipmapIndex, int level);
		void generateTerrainClipmapsVertexArrays();
		void loadTerrainHeightmapOnInit(glm::vec3 camPos, int clipmapLevel);
		void loadHeightmapAtLevel(int level, glm::vec3 camPos, unsigned char* heightData);
		void streamTerrain(glm::vec3 newCamPos, int clipmapLevel);
		void streamTerrainHorizontal(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level);
		void streamTerrainVertical(glm::ivec2 old_tileIndex, glm::ivec2 old_tileStart, glm::ivec2 old_border, glm::ivec2 new_tileIndex, glm::ivec2 new_tileStart, glm::ivec2 new_border, glm::ivec2 tileDelta, int level);
		void loadFromDiscAndWriteGPUBufferAsync(int level, int texWidth, glm::ivec2 tileStart, glm::ivec2 border, unsigned char* heightData, glm::ivec2 toroidalUpdateBorder);
		void updateHeightMapTextureArrayPartial(int level, glm::ivec2 size, glm::ivec2 position, unsigned char* heights);
		void deleteHeightmapArray(unsigned char** heightmapArray);
		void createElevationMapTextureArray(unsigned char** heightmapArray);
		void writeHeightDataToGPUBuffer(glm::ivec2 index, int texWidth, unsigned char* heightMap, unsigned char* chunk, int level, glm::ivec2 toroidalUpdateBorder);
		unsigned char* loadTerrainChunkFromDisc(int level, glm::ivec2 index);
		void updateHeightsWithTerrainChunk(unsigned char* heights, unsigned char* chunk, glm::ivec2 pos, glm::ivec2 chunkSize, glm::ivec2 heightsSize);
		glm::ivec2 getClipmapPosition(int level, glm::vec3& camPos);
		glm::ivec2 getTileIndex(int level, glm::vec3& camPos);
		int getMaxMipLevel(int textureSize, int tileSize);
		unsigned char** createMipmaps(unsigned char* heights, int size, int totalLevel);
		void createHeightmapImageFile(unsigned char* heights, int level, int newTileSize, int baseTileSize, int ind_x, int ind_z);
		void divideTerrainHeightmaps(unsigned char** heightMapList, int width, int totalLevel);
		void divideTextureStack(unsigned char** textureStack, int size, int patchSize, int channelCount, std::string name, int totalLevel);
		void createTextureImageFile(unsigned char* texture, int level, int channels, std::string name, int newTileSize, int baseTileSize, int ind_x, int ind_z);
		void loadTextures();
		unsigned char* remapHeightmap(unsigned char* heightmap, int size);
		AABB_Box getBoundingBoxOfClipmap1(int clipmapIndex, int totalLevel);
	};
}