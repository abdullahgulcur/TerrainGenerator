#include "pch.h"
#include "filesystem.h"

namespace Core {

	FileSystem::FileSystem() {

	}

	FileSystem::~FileSystem() {

		for (auto it : textures)
			delete it.second;
		for (auto it : cubemaps)
			delete it.second;
		for (auto it : shaders)
			delete it.second;
		for (auto it : meshes)
			delete it.second;
	}

	void FileSystem::init() {

		FileSystem::loadCubemap("resources/cubemaps/hilly_terrain_01_puresky_4k.hdr");

		//FileSystem::loadTexture("resources/textures/terrain/cliffgranite_a.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/groundforest_a.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/groundsandy_a.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/lichenedrock_a.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/soilmulch_a.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/grasslawn_a.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/grasswild_a.DDS");

		//FileSystem::loadTexture("resources/textures/terrain/cliffgranite_n.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/groundforest_n.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/groundsandy_n.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/lichenedrock_n.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/soilmulch_n.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/snowfresh_n.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/snowpure_n.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/grasslawn_n.DDS");
		//FileSystem::loadTexture("resources/textures/terrain/grasswild_n.DDS");
		/*
		FileSystem::loadTexture("resources/textures/terrain/pngs/cliffgranite_a.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/groundforest_a.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/groundsandy_a.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/lichenedrock_a.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/soilmulch_a.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/grasslawn_a.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/grasswild_a.png");

		FileSystem::loadTexture("resources/textures/terrain/pngs/cliffgranite_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/groundforest_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/groundsandy_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/lichenedrock_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/soilmulch_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/snowfresh_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/snowpure_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/grasslawn_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/grasswild_n.png");

		FileSystem::loadTexture("resources/textures/terrain/pngs/cliffgranite_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/grasswild_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/grasswild_ao.png");*/

		FileSystem::loadTexture("resources/textures/terrain/pngs/cliffgranite_a.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/groundforest_a.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/groundsandy_a.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/lichenedrock_a.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/soilmulch_a.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/grasslawn_a.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/grasswild_a.png");

		FileSystem::loadTexture("resources/textures/terrain/pngs/cliffgranite_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/groundforest_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/groundsandy_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/lichenedrock_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/soilmulch_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/snowfresh_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/snowpure_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/grasslawn_n.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/grasswild_n.png");

		FileSystem::loadTexture("resources/textures/terrain/pngs/cliffgranite_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/groundforest_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/groundsandy_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/lichenedrock_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/soilmulch_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/grasslawn_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/pngs/grasswild_ao.png");

		FileSystem::loadTexture("resources/textures/terrain/gold_a.png");
		FileSystem::loadTexture("resources/textures/terrain/noiseTexture.png");
	}

	Texture* FileSystem::loadTexture(std::filesystem::path entry) {

		Texture* texture = new Texture(entry.string());
		textures.insert({ entry.stem().string(), texture });
		return texture;
	}

	Mesh* FileSystem::loadMesh(std::filesystem::path entry) {

		Mesh* mesh = new Mesh(entry.string());
		meshes.insert({ entry.stem().string(), mesh });
		return mesh;
	}

	Shader* FileSystem::loadShader(std::filesystem::path entry) {

		Shader* shader = new Shader(entry.string());
		shaders.insert({ entry.stem().string(), shader });
		return shader;
	}

	Cubemap* FileSystem::loadCubemap(std::filesystem::path entry) {

		Cubemap* cubemap = new Cubemap(entry.string());
		cubemaps.insert({ entry.stem().string(), cubemap });
		return cubemap;
	}

}
