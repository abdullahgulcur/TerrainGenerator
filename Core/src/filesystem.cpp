#include "pch.h"
#include "filesystem.h"

namespace Core {

	FileSystem::FileSystem() {

	}

	void FileSystem::init() {

		// default assets.
		// env cubemap
		// cube, sphere, cone, cylinder, plane...
		// ...
	}

	Texture* FileSystem::loadTexture(std::filesystem::path entry) { // anisolevel vs, more parameters...

		Texture* texture = new Texture(entry.string());
		textures.insert({ entry.string(), texture });

		return texture;
	}

	Mesh* FileSystem::loadMesh(std::filesystem::path entry) { // three lod params

		Mesh* mesh = new Mesh(entry.string());
		meshes.insert({ entry.string(), mesh });

		return mesh;
	}

	Shader* FileSystem::loadShader(std::filesystem::path entry) {

		Shader* shader = new Shader(entry.string());
		shaders.insert({ entry.string(), shader });

		return shader;
	}

	Cubemap* FileSystem::loadCubemap(std::filesystem::path entry) {

		Cubemap* cubemap = new Cubemap(entry.string());
		cubemaps.insert({ entry.string(), cubemap });

		return cubemap;
	}
}
