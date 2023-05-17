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

	Texture* FileSystem::loadTexture(std::filesystem::path entry) {

		return 0;
	}

	Cubemap* FileSystem::loadCubemap(std::filesystem::path entry) {

		Cubemap* cubemap = new Cubemap(entry.string());
		cubemaps.insert({ entry.string(), cubemap });

		return cubemap;
	}
}
