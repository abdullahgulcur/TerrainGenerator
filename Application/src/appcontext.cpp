#include "pch.h"
#include "appcontext.h"
#include "corecontext.h"
#include "filesystem.h"
#include "entity.h"
#include "terrain.h"

using namespace Core;

namespace Application {

	void AppContext::start() {

		std::cout << "Application started." << std::endl;

		// file system load functions
		// textures, objects, materials, prefabs

		FileSystem* fileSystem = CoreContext::instance->fileSystem;

		Cubemap* cubemap = fileSystem->loadCubemap("resources/cubemaps/sky.hdr");
		Mesh* sphereMesh = fileSystem->loadMesh("resources/meshes/sphere.obj");
		Mesh* cubeMesh = fileSystem->loadMesh("resources/meshes/cube.obj");
		Texture* cliffTexture = fileSystem->loadTexture("resources/textures/terrain/cliff_1_a.DDS");

		std::string albedoTexturePaths[TERRAIN_TEXTURE_ARRAY_SIZE];
		std::string normalTexturePaths[TERRAIN_TEXTURE_ARRAY_SIZE];

		albedoTexturePaths[0] = "resources/textures/terrain/soil_a_a_PNG_DXT5_1.DDS";
		albedoTexturePaths[1] = "resources/textures/terrain/mulch_a.DDS";
		albedoTexturePaths[2] = "resources/textures/terrain/granite_a.DDS";
		albedoTexturePaths[3] = "resources/textures/terrain/soil_rock_a.DDS";
		albedoTexturePaths[4] = "resources/textures/terrain/snow_a.DDS";
		albedoTexturePaths[5] = "resources/textures/terrain/sand_a_a_PNG_DXT5_1.DDS";
		albedoTexturePaths[6] = "resources/textures/terrain/lichened_rock_a.DDS";

		normalTexturePaths[0] = "resources/textures/terrain/soil_n_a_PNG_DXT5_1.DDS";
		normalTexturePaths[1] = "resources/textures/terrain/mulch_n.DDS";
		normalTexturePaths[2] = "resources/textures/terrain/granite_n.DDS";
		normalTexturePaths[3] = "resources/textures/terrain/soil_rock_n.DDS";
		normalTexturePaths[4] = "resources/textures/terrain/snow_n.DDS";
		normalTexturePaths[5] = "resources/textures/terrain/sand_n_a_PNG_DXT5_1.DDS";
		normalTexturePaths[6] = "resources/textures/terrain/lichened_rock_n.DDS";

		std::vector<Texture*> albedoTerrainTextureList;
		std::vector<Texture*> normalTerrainTextureList;

		for (int i = 0; i < TERRAIN_TEXTURE_ARRAY_SIZE; i++) {
			Texture* albedoTex = fileSystem->loadTexture(albedoTexturePaths[i]);
			Texture* normalTex = fileSystem->loadTexture(normalTexturePaths[i]);

			albedoTerrainTextureList.push_back(albedoTex);
			normalTerrainTextureList.push_back(normalTex);
		}

		Texture* noise0 = fileSystem->loadTexture("resources/textures/terrain/noise/gold_a.png");
		Texture* noise1 = fileSystem->loadTexture("resources/textures/terrain/noise/noiseTexture.png");

		Scene* scene = CoreContext::instance->scene; // we need default cubemap...
		scene->cubemap = cubemap;

		Entity* terrainEnt = new Entity("Terrain");
		Terrain* terrainComp = terrainEnt->addComponent<Terrain>();
		terrainComp->loadTextures(albedoTerrainTextureList, normalTerrainTextureList, noise0, noise1);
		scene->entity = terrainEnt;

		scene->start(); // yeri degisebilir. to the core...
	}

	void AppContext::update(float dt) {

		Scene* scene = CoreContext::instance->scene;

		scene->update(dt); // yeri degisebilir. to the core...
	}

}
