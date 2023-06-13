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

		FileSystem::loadTexture("resources/textures/terrain/texturemaps/cliffgranite_a.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/groundforest_a.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/groundsandy_a.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/lichenedrock_a.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/soilmulch_a.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/grasslawn_a.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/grasswild_a.png");

		FileSystem::loadTexture("resources/textures/terrain/texturemaps/cliffgranite_n.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/groundforest_n.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/groundsandy_n.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/lichenedrock_n.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/soilmulch_n.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/snowfresh_n.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/snowpure_n.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/grasslawn_n.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/grasswild_n.png");

		FileSystem::loadTexture("resources/textures/terrain/texturemaps/cliffgranite_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/groundforest_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/groundsandy_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/lichenedrock_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/soilmulch_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/grasslawn_ao.png");
		FileSystem::loadTexture("resources/textures/terrain/texturemaps/grasswild_ao.png");

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
