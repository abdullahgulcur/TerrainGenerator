#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "rapidXML/rapidxml.hpp"
#include "rapidXML/rapidxml_print.hpp"

#include "glfwcontext.h"

namespace Core {

	class CoreContext;
}

namespace Editor {

	class Editor;

	enum SceneCameraFlags {

		FirstCycle = 0,
		AllowMovement = 1,
		MouseTeleported = 2
	};

	class SceneCamera {

	private:

		float rotationSpeed = 0.020;
		float translationSpeed = 50;
		float scrollSpeed = 20;
		float generalSpeed = 0.3f;

		//float rotationSpeed = 0.02;
		//float translationSpeed = 10;
		//float scrollSpeed = 5;
		//float generalSpeed = 0.3f;

		UINT16 controlFlags;

		float lastX;
		float lastY;

	public:

		float nearClip = 0.1;
		float farClip = 10000;
		int projectionType = 0;
		int fovAxis = 0;
		float fov;
		float aspectRatio = 1.77f;

		glm::mat4 ViewMatrix;
		glm::mat4 ProjectionMatrix;
		glm::mat4 projectionViewMatrix;

		glm::vec4 planes[6];

		unsigned int textureBuffer;
		unsigned int FBO;
		unsigned int RBO;

		float horizontalAngle = 0.f;
		float verticalAngle = 0.f;
		glm::vec3 position = glm::vec3(0, 0, -5);

		unsigned int width;
		unsigned int height;

		SceneCamera();
		~SceneCamera();
		void init(int width, int height);
		void update(float dt);
		void controlMouse(float dt);
		void teleportMouse(glm::vec2& mousePos, float& scenePosX, float& scenePosY, float& sceneRegionX,
			float& sceneRegionY, float& offset, bool& mouseTeleport);
		void frustum(glm::mat4& view_projection);
		void createFBO(int sizeX, int sizeY);
		void recreateFBO(int sizeX, int sizeY);
		void updateProjectionMatrix(int sizeX, int sizeY);
		bool intersectsAABB(glm::vec3 start, glm::vec3 end);
		bool load(std::string path);
		bool save(std::string path);
	};

}