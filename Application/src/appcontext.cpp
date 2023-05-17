#include "pch.h"
#include "appcontext.h"
#include "corecontext.h"
#include "filesystem.h"

using namespace Core;

namespace Application {

	void AppContext::start() {

		std::cout << "Application started." << std::endl;

		// file system load functions
		// textures, objects, materials, prefabs

		FileSystem* fileSystem = CoreContext::instance->fileSystem;

		Cubemap* cubemap = fileSystem->loadCubemap("resources/cubemaps/sky.hdr");

		Scene* scene = CoreContext::instance->scene; // we need default cubemap...
		scene->cubemap = cubemap;
	}

	void AppContext::update(float dt) {

		
	}

}
