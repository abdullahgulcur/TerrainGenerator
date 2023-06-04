#pragma once

namespace Core {

	class __declspec(dllexport) Shader {

	private:

	public:

		Shader(std::string path);
		~Shader();
		static unsigned int loadShaders(std::string vertexPath, std::string fragmentPath);

	};
}