#include "pch.h"
#include "filesystem.h"

namespace Core {

	FileSystem::FileSystem() {

	}

	FileSystem::~FileSystem() {

	}

	void FileSystem::init() {

		FileSystem::loadCubemap("resources/cubemaps/hilly_terrain_01_puresky_4k.hdr");

		FileSystem::loadTexture("resources/textures/terrain/soil_a.DDS");
		FileSystem::loadTexture("resources/textures/terrain/mulch_a.DDS");
		FileSystem::loadTexture("resources/textures/terrain/granite_a.DDS");
		FileSystem::loadTexture("resources/textures/terrain/soil_rock_a.DDS");
		FileSystem::loadTexture("resources/textures/terrain/sand_a.DDS");
		FileSystem::loadTexture("resources/textures/terrain/lichened_rock_a.DDS");

		FileSystem::loadTexture("resources/textures/terrain/soil_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/mulch_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/granite_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/soil_rock_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/snow_fresh_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/sand_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/lichened_rock_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/s1n.DDS");

		FileSystem::loadTexture("resources/textures/terrain/gold_a.png");
		FileSystem::loadTexture("resources/textures/terrain/noiseTexture.png");
	}

	Texture* FileSystem::loadTexture(std::filesystem::path entry) { // anisolevel vs, more parameters...

		Texture* texture = new Texture(entry.string());
		textures.insert({ entry.stem().string(), texture });

		return texture;
	}

	Mesh* FileSystem::loadMesh(std::filesystem::path entry) { // three lod params

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
