#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "rapidXML/rapidxml.hpp"
#include "rapidXML/rapidxml_print.hpp"

#include "glfwcontext.h"

// todo: camera starts at 10k, 10k, limit movement

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

		float rotationSpeed = 0.02;
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



		float horizontalAngle = 0.f;
		float verticalAngle = 0.f;
		glm::vec3 position = glm::vec3(0, 0, -5);

		SceneCamera();
		~SceneCamera();
		void startMatrices();
		void changeSceneCamera();
		void update(float dt);
		void controlMouse(float dt);
		void teleportMouse(glm::vec2& mousePos, float& scenePosX, float& scenePosY, float& sceneRegionX,
			float& sceneRegionY, float& offset, bool& mouseTeleport);
		void frustum(glm::mat4& view_projection);
		void updateProjectionMatrix(int sizeX, int sizeY);
		bool intersectsAABB(glm::vec3 start, glm::vec3 end);
		bool load();
		bool save();
	};

}