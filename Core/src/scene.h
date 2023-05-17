#pragma once

#include "GLM/glm.hpp"
#include "cubemap.h"

namespace Core {

	class Entity;

	struct __declspec(dllexport) DirectionalLight {
		glm::vec3 direction;
		glm::vec3 color;
		float power;
	};

	struct CameraInfo {
		unsigned int FBO;
		glm::mat4 VP;
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec3 camPos;
		unsigned int width;
		unsigned int height;
		glm::vec4 planes[6];
	};

	class __declspec(dllexport) Scene {

	private:

	public:

		// global volume
	    // light

		Cubemap* cubemap;

		DirectionalLight directionalLight;
		CameraInfo cameraInfo;

		Entity* entity;

		Scene();
		~Scene();
		void start();
		void update(float dt);

	};
}