#pragma once

#include "texture.h"
#include "cubemap.h"
//#include "mesh.h"
//#include "material.h"

namespace Core {


	class __declspec(dllexport) FileSystem {

	private:


	public:

		std::map<std::string, Texture*> textures;
		std::map<std::string, Cubemap*> cubemaps;

		FileSystem();
		void init();
		Texture* loadTexture(std::filesystem::path entry);
		Cubemap* loadCubemap(std::filesystem::path entry);

	};
}