#pragma once
#include "GL/glew.h"
#include "glm/glm.hpp"

namespace Core {

	class __declspec(dllexport) GlewContext {

	private:

	public:

		GlewContext();
		~GlewContext();
		void createFrameBuffer(unsigned int& FBO, unsigned int& RBO, unsigned int& textureBuffer, int sizeX, int sizeY);
	};
}