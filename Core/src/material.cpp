#include "pch.h"
#include "material.h"

namespace Core {

	Material::Material(ShaderType type, std::vector<Texture*> textures) {

		this->shaderType = type;
		this->textures = textures;
	}

	Material::~Material() {


	}

}
