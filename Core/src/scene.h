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

	// octree
	// node -> key: position val: (transform, type)

	class __declspec(dllexport) Scene {

	private:

	public:

		// no streaming
			// one octree 
			// custom structs for foliage
			// 
			// 
			// grass 0 transforms
			// grass 1 transforms
			// grass 2 transforms
			// 
			// bush 0 transforms
			// bush 1 transforms
			// bush 2 transforms
			//
			// tree 0 transforms
			// tree 1 transforms
			// tree 2 transforms

		// global volume
	    // light

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
		unsigned int framebufferProgramID;
		// ----------------

		Terrain* terrain = NULL; // void ptr

		Cubemap* cubemap;

		DirectionalLight directionalLight;
		CameraInfo cameraInfo;

		int activeSceneIndex; // EDITOR ONLY

		Scene();
		~Scene();
		void start();
		void update(float dt);
		static int getActiveSceneIndex();
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