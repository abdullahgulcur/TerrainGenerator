#pragma once

#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

namespace Core {

	// ToDo: channel check, fastest lightweight broad decoder

	class __declspec(dllexport) Texture {

	private:

		void loadDDS(const char* path);
		void loadPNG(const char* path);

	public:

		unsigned int textureId;

		Texture(std::filesystem::path entry);
		~Texture();
		
	};
}