#include "pch.h"
#include "corecontext.h"
#include "transform.h"

namespace Core {

	// root
	Transform::Transform(Entity* entity) {

		this->entity = entity;
		this->parent = NULL;

		localPosition = glm::vec3(0, 0, 0);
		localRotation = glm::vec3(0, 0, 0);
		localScale = glm::vec3(1, 1, 1);
		//globalPosition = localPosition;
		//globalRotation = localRotation;
		//globalScale = localScale;
		model = glm::mat4(1);
	}

	// new transform
	Transform::Transform(Entity* entity, Transform* parent) {

		this->entity = entity;
		this->parent = parent;
		(parent->children).push_back(this);

		localPosition = glm::vec3(0, 0, 0);
		localRotation = glm::vec3(0, 0, 0);
		localScale = glm::vec3(1, 1, 1);
		//globalPosition = parent->globalPosition;
		//globalRotation = parent->globalRotation;
		//globalScale = parent->globalScale;
		model = parent->model;
	}

	// copied transform
	Transform::Transform(Entity* entity, Transform* transform, Transform* parent) {

		this->entity = entity;
		this->parent = parent;
		(parent->children).push_back(this);

		localPosition = transform->localPosition;
		localRotation = transform->localRotation;
		localScale = transform->localScale;
		//globalPosition = transform->globalPosition;
		//globalRotation = transform->globalRotation;
		//globalScale = transform->globalScale;
		model = transform->model;
	}

	Transform::~Transform() {
	}

	void Transform::start() {

	}

	void Transform::update(float dt) {

	}

	// done with transform component panel
	void Transform::updateTransform() {

		model = parent->model * getLocalModelMatrix();

	//	if (GameCamera* cam = entity->getComponent<GameCamera>())
	//		cam->updateViewMatrix(this);

		for (auto& transform : children)
			transform->updateTransform();
	}

	// using gizmo 
	void Transform::updateTransformUsingGuizmo()
	{
		glm::mat4 localTransform = glm::inverse(parent->model) * model;
		glm::vec3 scale;
		glm::vec3 rotation;
		glm::vec3 position;
		Transform::decompose(localTransform, scale, rotation, position);

		localScale = scale;
		localRotation = rotation * (180.f / glm::pi<float>());
		localPosition = position;

		//globalPosition = parent->globalPosition * localPosition;
		//globalRotation = parent->globalRotation * localRotation;
		//globalScale = parent->globalScale * localScale;

	//	if (GameCamera* cam = entity->getComponent<GameCamera>())
	//		cam->updateViewMatrix(this);

		for (auto& transform : children)
			transform->updateModel();
	}

	glm::mat4 Transform::getLocalModelMatrix()
	{
		glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(localRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(localRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(localRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 rotationMatrix = rotX * rotY * rotZ;

		return glm::translate(glm::mat4(1.0f), localPosition) * rotationMatrix * glm::scale(glm::mat4(1.0f), localScale);
	}

	void Transform::updateLocals() {

		glm::mat4 localTransform = glm::inverse(parent->model) * model;
		glm::vec3 scale;
		glm::vec3 rotation;
		glm::vec3 position;
		Transform::decompose(localTransform, scale, rotation, position);

		localScale = scale;
		localRotation = rotation * (180.f / glm::pi<float>());
		localPosition = position;
	}

	void Transform::updateModel() {

		//globalPosition = parent->globalPosition * localPosition;
		//globalRotation = parent->globalRotation * localRotation;
		//globalScale = parent->globalScale * localScale;
		model = parent->model * getLocalModelMatrix();

		//if (GameCamera* cam = entity->getComponent<GameCamera>())
		//	cam->updateViewMatrix(this);

		for (auto& transform : children)
			transform->updateModel();
	}

	bool Transform::decompose(glm::mat4& ModelMatrix, glm::vec3& Scale, glm::vec3& Orientation, glm::vec3& Translation) {

		using namespace glm;
		using namespace detail;
		using T = float;

		mat4 LocalMatrix(ModelMatrix);

		// Normalize the matrix.
		if (epsilonEqual(LocalMatrix[3][3], static_cast<T>(0), epsilon<T>()))
			return false;

		// First, isolate perspective.  This is the messiest.
		if (
			epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
		{
			// Clear the perspective partition
			LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
			LocalMatrix[3][3] = static_cast<T>(1);
		}

		// Next take care of translation (easy).
		Translation = vec3(LocalMatrix[3]);
		LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

		vec3 Row[3], Pdum3;

		// Now get scale and shear.
		for (length_t i = 0; i < 3; ++i)
			for (length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		// Compute X scale factor and normalize first row.
		Scale.x = length(Row[0]);// v3Length(Row[0]);
		Row[0] = detail::scale(Row[0], static_cast<T>(1));
		Scale.y = length(Row[1]);
		Row[1] = detail::scale(Row[1], static_cast<T>(1));
		Scale.z = length(Row[2]);
		Row[2] = detail::scale(Row[2], static_cast<T>(1));

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				Scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
		}

		// Now, get the rotations out, as described in the gem.

		// FIXME - Add the ability to return either quaternions (which are
		// easier to recompose with) or Euler angles (rx, ry, rz), which
		// are easier for authors to deal with. The latter will only be useful
		// when we fix https://bugs.webkit.org/show_bug.cgi?id=23799, so I
		// will leave the Euler angle code here for now.

		Orientation.y = asin(-Row[0][2]);
		if (cos(Orientation.y) != 0) {
			Orientation.x = atan2(Row[1][2], Row[2][2]);
			Orientation.z = atan2(Row[0][1], Row[0][0]);
		}
		else {
			Orientation.x = atan2(-Row[2][0], Row[1][1]);
			Orientation.z = 0;
		}

		return true;
	}

	// fucker
	glm::vec3 Transform::getGlobalPosition() {

		glm::mat4 transform = model;
		glm::vec3 scale;
		glm::vec3 rotation;
		glm::vec3 position;
		Transform::decompose(transform, scale, rotation, position);
		return position;
	}

}