#pragma once

#include "texture.h"
#include "cubemap.h"
#include "mesh.h"
#include "shader.h"

namespace Core {


	class __declspec(dllexport) FileSystem {

	private:


	public:

		std::map<std::string, Texture*> textures;
		std::map<std::string, Cubemap*> cubemaps;
		std::map<std::string, Shader*> shaders;
		std::map<std::string, Mesh*> meshes;

		FileSystem();
		void init();
		Texture* loadTexture(std::filesystem::path entry);
		Mesh* loadMesh(std::filesystem::path entry);
		Shader* loadShader(std::filesystem::path entry);
		Cubemap* loadCubemap(std::filesystem::path entry);

	};
}