#pragma once

namespace Core {


	class __declspec(dllexport) Texture {

	private:


	public:

		Texture();
		static void loadDDS(const char* path);
		static void loadPNG(const char* path);

	};
}