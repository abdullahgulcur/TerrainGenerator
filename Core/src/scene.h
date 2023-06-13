#pragma once

#include "rapidXML/rapidxml_print.hpp"
#include "rapidXML/rapidxml.hpp"
#include "GLM/glm.hpp"
#include "cubemap.h"
#include "component/terrain.h"

namespace Core {

	struct __declspec(dllexport) DirectionalLight {
		glm::vec3 direction;
		glm::vec3 color;
		float power;
	};

	struct CameraInfo {
		glm::mat4 VP;
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec3 camPos;
		glm::vec4 planes[6];
	};

	class __declspec(dllexport) Scene {

	private:

	public:

		// FRAMEBUFFER ----
		unsigned int width;
		unsigned int height;
		unsigned int screenQuadVAO;

		unsigned int filterTextureBuffer;
		unsigned int filterFramebufferProgramID;
		unsigned int filterFBO;
		unsigned int filterRBO;

		unsigned int textureBuffer;
		unsigned int FBO;
		unsigned int RBO;
		//unsigned int framebufferProgramID;

		Terrain* terrain = NULL;
		Cubemap* cubemap;
		DirectionalLight directionalLight;
		CameraInfo cameraInfo;

		int activeSceneIndex; // EDITOR

		Scene();
		~Scene();
		void start();
		void update(float dt);
		static int getActiveSceneIndex();
		void initFramebuffers();
		static std::string getActiveScenePath();
		static void setActiveSceneIndex(int index);
		static void changeScene(int index);
		void setSize(int width, int height);
		void destroyContent();
		bool saveScene(std::string filePath);
		void loadScene(std::string filePath);
		bool saveTerrain(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, Terrain* terrain);
		Terrain* loadTerrain(rapidxml::xml_node<>* entNode);
		void setFrameBuffer(int width, int height);
	};
}