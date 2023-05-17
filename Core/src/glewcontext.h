#pragma once
#include "GL/glew.h"
#include "glm/glm.hpp"

namespace Core {

	class __declspec(dllexport) GlewContext {

	private:

	public:

		GlewContext();
		void createFrameBuffer(unsigned int& FBO, unsigned int& RBO, unsigned int& textureBuffer, int sizeX, int sizeY);
		unsigned int loadShaders(const char* vertex_file_path, const char* fragment_file_path);
	};
}