#pragma once

#include "texture.h"

namespace Core {

	enum class __declspec(dllexport) ShaderType {
		PBR, PBR_ALPHA
	};

	class __declspec(dllexport) Material {

	private:

	public:

		ShaderType shaderType;
		std::vector<Texture*> textures;

		Material(ShaderType type, std::vector<Texture*> textures);
		~Material();

	};
}