#pragma once

namespace Core {

	class __declspec(dllexport) Renderer {

	private:

		void createEnvironmentCubeVAO();

	public:

		unsigned int backgroundShaderProgramId;
		unsigned int envCubeVAO;

		void init();
		void update(float dt);
		
	};

}
