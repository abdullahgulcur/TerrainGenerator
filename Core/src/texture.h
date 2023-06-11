#pragma once

namespace Core {

	class __declspec(dllexport) Texture {

	private:

		//void loadDDS(const char* path);
		//void loadPNG(const char* path);
		//void loadDataPNG(const char* path);

	public:

		unsigned char* data;
		unsigned int channels;
		unsigned int bitDepth;
		unsigned int width;
		unsigned int height;

		Texture();
		Texture(std::filesystem::path entry);
		~Texture();
		
		static unsigned int loadPNG_RGBA8(const char* path);
		void loadPNGFile(const char* path);
		static Texture* mergeTextures(Texture* tex0, Texture* tex1, int ch0, int ch1);
		static Texture* loadTexturePartial(Texture* tex, int ch0);
		unsigned int loadToGPU();

	};
}