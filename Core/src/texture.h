#pragma once

namespace Core {

	class __declspec(dllexport) Texture {

	private:

		void loadDDS(const char* path);
		void loadPNG(const char* path);

	public:

		unsigned int textureId;

		Texture(std::filesystem::path entry);
		~Texture();
		
		static unsigned int loadPNG_RGBA8(const char* path);

	};
}