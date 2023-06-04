#pragma once

#include "glm/glm.hpp"

namespace Core {

	class __declspec(dllexport) Renderer {

	private:

		void createEnvironmentCubeVAO();
		void createBoundingBoxVAO();
	public:

		unsigned int backgroundShaderProgramId;
		unsigned int envCubeVAO;

		unsigned int defaultPbrShaderProgramId;
		unsigned int alphaBlendedPbrShaderProgramId;

		//debug
		unsigned int boundingBoxVAO;
		unsigned int lineShaderProgramId;

		unsigned int terrainRenderTotalTime = 0;
		unsigned int terrainRenderDuration = 0;
		unsigned int frameCounter = 0;

		void init();
		void update(float dt);
		void drawBoundingBoxVAO(glm::mat4& PVM, glm::vec3& color);

	};

}
