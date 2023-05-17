#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Core {

	class Entity;

	class __declspec(dllexport) Transform {

	private:

	public:

		Entity* entity;
		Transform* parent;
		std::vector<Transform*> children;

		glm::vec3 localPosition;
		glm::vec3 localRotation;
		glm::vec3 localScale;

		glm::mat4 model;

		Transform(Entity* entity);
		~Transform();
		void setLocalPosition(glm::vec3 position);
		void setLocalRotation(glm::vec3 rotation);
		void setLocalScale(glm::vec3 scale);
		void updateTransform();
		void updateTransformUsingGuizmo();
		glm::mat4 getLocalModelMatrix();
		void updateLocals();
		void updateModel();
		bool decompose(glm::mat4& ModelMatrix, glm::vec3& Scale, glm::vec3& Orientation, glm::vec3& Translation);
		glm::vec3 getGlobalPosition();

	};
}