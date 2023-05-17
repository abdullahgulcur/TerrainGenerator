#pragma once

namespace Core {

	class __declspec(dllexport) Cubemap {

	private:

		void createQuadVAO(unsigned int& quadVAO);

	public:

		unsigned int envCubemap;

		unsigned int irradianceMap = 0;
		unsigned int prefilterMap = 0;
		unsigned int brdfLUTTexture = 0;

		Cubemap(std::string path);
		void createCubemapTextures(std::string path);
		
	};

}
