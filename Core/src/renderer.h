#pragma once

namespace Core {

	// structs
	// standard mesh
	// alpha blended mesh
	// transparent mesh
	// ...

	class __declspec(dllexport) Renderer {

	private:

		void createEnvironmentCubeVAO();

	public:

		unsigned int backgroundShaderProgramId;
		unsigned int envCubeVAO;

		unsigned int defaultPbrShaderProgramId;
		unsigned int alphaBlendedPbrShaderProgramId;

		void init();
		void update(float dt);
		
	};

}
