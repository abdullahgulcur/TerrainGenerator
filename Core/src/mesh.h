#pragma once

#include "glm/glm.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

namespace Core {

	struct __declspec(dllexport) Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;
	};

	struct __declspec(dllexport) AABB_Box {
		glm::vec4 start;
		glm::vec4 end;
	};

	class __declspec(dllexport) Mesh {

	private:

	public:

		unsigned int VAO;
		unsigned int indiceCount;
		AABB_Box aabbBox;

		Mesh(std::string path);
		~Mesh();
		void loadModel(std::string path);
		void processNode(aiNode* node, const aiScene* scene);
		void processMesh(aiMesh* mesh, const aiScene* scene);
		static void createVAO(unsigned int& VAO, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
	};
}