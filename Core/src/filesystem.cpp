#include "pch.h"
#include "filesystem.h"

namespace Core {

	FileSystem::FileSystem() {

	}

	void FileSystem::init() {

		FileSystem::loadCubemap("resources/cubemaps/sky.hdr");
		FileSystem::loadMesh("resources/meshes/sphere.obj");
		FileSystem::loadMesh("resources/meshes/cube.obj");
		FileSystem::loadMesh("resources/meshes/bush.obj");

		Texture* black = FileSystem::loadTexture("resources/textures/default/black.png");
		Texture* white = FileSystem::loadTexture("resources/textures/default/white.png");
		Texture* flatNormal = FileSystem::loadTexture("resources/textures/default/flat_normal.png");

		FileSystem::loadTexture("resources/textures/terrain/soil_a.DDS");
		FileSystem::loadTexture("resources/textures/terrain/mulch_a.DDS");
		FileSystem::loadTexture("resources/textures/terrain/granite_a.DDS");
		FileSystem::loadTexture("resources/textures/terrain/soil_rock_a.DDS");
		FileSystem::loadTexture("resources/textures/terrain/snow_a.DDS");
		FileSystem::loadTexture("resources/textures/terrain/sand_a.DDS");
		FileSystem::loadTexture("resources/textures/terrain/lichened_rock_a.DDS");

		FileSystem::loadTexture("resources/textures/terrain/soil_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/mulch_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/granite_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/soil_rock_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/snow_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/sand_n.DDS");
		FileSystem::loadTexture("resources/textures/terrain/lichened_rock_n.DDS");

		FileSystem::loadTexture("resources/textures/terrain/noise/gold_a.png");
		FileSystem::loadTexture("resources/textures/terrain/noise/noiseTexture.png");

		Texture* cottoneasterAlbedo = FileSystem::loadTexture("resources/textures/bushes/cottoneaster_a.png");
		Texture* cottoneasterNormal = FileSystem::loadTexture("resources/textures/bushes/cottoneaster_n.png");
		Texture* cottoneasterRoughness = FileSystem::loadTexture("resources/textures/bushes/cottoneaster_r.png");
		Texture* cottoneasterOpacity = FileSystem::loadTexture("resources/textures/bushes/cottoneaster_o.png");

		std::vector<Texture*> cottoneasterTextures;
		cottoneasterTextures.push_back(cottoneasterAlbedo);
		cottoneasterTextures.push_back(cottoneasterNormal);
		cottoneasterTextures.push_back(black);
		cottoneasterTextures.push_back(cottoneasterRoughness);
		cottoneasterTextures.push_back(white);
		cottoneasterTextures.push_back(cottoneasterOpacity);
		FileSystem::loadMaterial("bush_coconout", ShaderType::PBR_ALPHA, cottoneasterTextures);
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

	Material* FileSystem::loadMaterial(std::string name, ShaderType type, std::vector<Texture*> textures) {

		Material* material = new Material(type, textures);
		materials.insert({ name, material});

		return material;
	}
}
