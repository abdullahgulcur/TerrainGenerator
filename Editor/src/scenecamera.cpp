#include "pch.h"
#include "GL/glew.h"
#include "scenecamera.h"
#include "editorcontext.h"
#include "corecontext.h"

using namespace Core;

namespace Editor {

	SceneCamera::SceneCamera() {

		SceneCamera::load("database/scenecamera.xml");
		rotationSpeed *= generalSpeed;
		rotationSpeed *= generalSpeed;
		translationSpeed *= generalSpeed;
		translationSpeed *= generalSpeed;
		scrollSpeed *= generalSpeed;
		fov = 60;

		glm::vec3 direction(cos(verticalAngle) * sin(horizontalAngle), sin(verticalAngle), cos(verticalAngle) * cos(horizontalAngle));
		glm::vec3 right = glm::vec3(sin(horizontalAngle - 3.14f / 2.0f), 0, cos(horizontalAngle - 3.14f / 2.0f));
		glm::vec3 up = glm::cross(right, direction);
		ProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100000.0f);
		ViewMatrix = glm::lookAt(position, position + direction, up);
		projectionViewMatrix = ProjectionMatrix * ViewMatrix;
		SceneCamera::frustum(projectionViewMatrix);
	}

	SceneCamera::~SceneCamera() {

		SceneCamera::save("database/scenecamera.xml");
	}

	void SceneCamera::init(int width, int height) {

		SceneCamera::createFBO(width, height);
	}

	bool SceneCamera::load(std::string path) {

		std::ifstream file(path);

		if (file.fail()) {
			SceneCamera::save(path);
			return false;
		}

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* root_node = NULL;

		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		buffer.push_back('\0');

		doc.parse<0>(&buffer[0]);

		root_node = doc.first_node("SceneCamera");
		position.x = atof(root_node->first_node("Position")->first_attribute("X")->value());
		position.y = atof(root_node->first_node("Position")->first_attribute("Y")->value());
		position.z = atof(root_node->first_node("Position")->first_attribute("Z")->value());
		horizontalAngle = atof(root_node->first_node("Angle")->first_attribute("Horizontal")->value());
		verticalAngle = atof(root_node->first_node("Angle")->first_attribute("Vertical")->value());

		file.close();
		return true;
	}

	void SceneCamera::update(float dt) {

		SceneCamera::controlMouse(dt);
	}

	void SceneCamera::controlMouse(float dt) {

		GlfwContext* glfwContext = CoreContext::instance->glfwContext;

		bool firstCycle = (controlFlags >> SceneCameraFlags::FirstCycle) & 1U;
		bool allow = (controlFlags >> SceneCameraFlags::AllowMovement) & 1U;
		bool mouseTeleport = (controlFlags >> SceneCameraFlags::MouseTeleported) & 1U;

		float offset = 10;
		firstCycle = ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(2);
		ImVec2 pos = ImGui::GetMousePos();
		glm::vec2 mousePos(pos.x, pos.y);
		float deltaX = !mouseTeleport ? mousePos.x - lastX : 0;
		float deltaY = !mouseTeleport ? mousePos.y - lastY : 0;
		mouseTeleport = false;

		Menu* menu = EditorContext::instance->menu;
		float scenePosX = menu->scenePos.x;
		float scenePosY = menu->scenePos.y;
		float sceneRegionX = menu->sceneRect.x;
		float sceneRegionY = menu->sceneRect.y;

		bool insideSceneView = mousePos.x > scenePosX && mousePos.x < scenePosX + sceneRegionX && mousePos.y > scenePosY && mousePos.y < scenePosY + sceneRegionY;

		float mouseWheelDelta = glfwContext->verticalScrollOffset();
		glfwContext->resetVerticalScrollOffset();

		if (insideSceneView && !ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift) {

			//ImGuiIO& io = ImGui::GetIO();
			//mouseWheelDelta = io.MouseWheel;
			glm::vec3 direction = glm::vec3(cos(verticalAngle) * sin(horizontalAngle), sin(verticalAngle), cos(verticalAngle) * cos(horizontalAngle));
			position += direction * mouseWheelDelta * scrollSpeed;
		}

		if (firstCycle) {

			allow = insideSceneView;
			firstCycle = false;
		}

		if (allow) {

			if (ImGui::IsMouseDown(1)) {

				SceneCamera::teleportMouse(mousePos, scenePosX, scenePosY, sceneRegionX, sceneRegionY, offset, mouseTeleport);

				horizontalAngle -= rotationSpeed * deltaX;
				verticalAngle -= rotationSpeed * deltaY;
			}

			if (ImGui::IsMouseDown(2)) {

				SceneCamera::teleportMouse(mousePos, scenePosX, scenePosY, sceneRegionX, sceneRegionY, offset, mouseTeleport);

				glm::vec3 direction = glm::vec3(cos(verticalAngle) * sin(horizontalAngle), sin(verticalAngle), cos(verticalAngle) * cos(horizontalAngle));
				glm::vec3 right = glm::vec3(sin(horizontalAngle - 3.14f / 2.0f), 0, cos(horizontalAngle - 3.14f / 2.0f));
				glm::vec3 up = glm::cross(right, direction);
				position -= right * deltaX * dt * translationSpeed;
				position += up * deltaY * dt * translationSpeed;
			}

			ImGuiMouseButton btn = ImGuiPopupFlags_MouseButtonLeft || ImGuiPopupFlags_MouseButtonRight || ImGuiPopupFlags_MouseButtonMiddle;
			if (ImGui::IsMouseReleased(btn))
				allow = false;
		}

		if (allow || mouseWheelDelta != 0) {

			glm::vec3 direction = glm::vec3(cos(verticalAngle) * sin(horizontalAngle), sin(verticalAngle), cos(verticalAngle) * cos(horizontalAngle));
			glm::vec3 right = glm::vec3(sin(horizontalAngle - 3.14f / 2.0f), 0, cos(horizontalAngle - 3.14f / 2.0f));
			glm::vec3 up = glm::cross(right, direction);
			ViewMatrix = glm::lookAt(position, position + direction, up);
		}

		projectionViewMatrix = ProjectionMatrix * ViewMatrix;
		SceneCamera::frustum(projectionViewMatrix);

		lastX = mousePos.x;
		lastY = mousePos.y;

		controlFlags ^= (-firstCycle ^ controlFlags) & (1UL << SceneCameraFlags::FirstCycle);
		controlFlags ^= (-allow ^ controlFlags) & (1UL << SceneCameraFlags::AllowMovement);
		controlFlags ^= (-mouseTeleport ^ controlFlags) & (1UL << SceneCameraFlags::MouseTeleported);
	}

	void SceneCamera::teleportMouse(glm::vec2& mousePos, float& scenePosX, float& scenePosY, float& sceneRegionX, float& sceneRegionY, float& offset, bool& mouseTeleport) {

		//void (GlfwContext::*setCursorPoss)(float x, float y);
		//setCursorPoss = &GlfwContext::setCursorPos;
		GlfwContext* glfwContext = CoreContext::instance->glfwContext;

		if (mousePos.x < scenePosX + offset) {

			//(glfwContext->*setCursorPoss)(scenePosX + sceneRegionX - offset - 1, mousePos.y);
			glfwContext->setCursorPos(scenePosX + sceneRegionX - offset - 1, mousePos.y);
			mouseTeleport = true;
		}
		else if (mousePos.x > scenePosX + sceneRegionX - offset) {

			glfwContext->setCursorPos(scenePosX + offset + 1, mousePos.y);
			mouseTeleport = true;
		}

		if (mousePos.y < scenePosY + offset) {

			glfwContext->setCursorPos(mousePos.x, scenePosY + sceneRegionY - offset * 5);// BUGGG !!!!
			mouseTeleport = true;
		}
		else if (mousePos.y > scenePosY + sceneRegionY - offset) {

			glfwContext->setCursorPos(mousePos.x, scenePosY + offset + 1);
			mouseTeleport = true;
		}
	}

	void SceneCamera::createFBO(int sizeX, int sizeY) {

		CoreContext::instance->glewContext->createFrameBuffer(FBO, RBO, textureBuffer, 1920, 1080); // son iki parametre de degismeli !!!
	}

	void SceneCamera::recreateFBO(int sizeX, int sizeY) {

		CoreContext::instance->glewContext->createFrameBuffer(FBO, RBO, textureBuffer, sizeX, sizeY);
		SceneCamera::updateProjectionMatrix(sizeX, sizeY);
	}

	void SceneCamera::updateProjectionMatrix(int sizeX, int sizeY) {

		aspectRatio = EditorContext::instance->menu->sceneRect.x / EditorContext::instance->menu->sceneRect.y;
		ProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
	}

	// ref: https://arm-software.github.io/opengl-es-sdk-for-android/terrain.html
	void SceneCamera::frustum(glm::mat4& view_projection)
	{
		// Frustum planes are in world space.
		glm::mat4 inv = glm::inverse(view_projection);
		// Get world-space coordinates for clip-space bounds.
		glm::vec4 lbn = inv * glm::vec4(-1, -1, -1, 1);
		glm::vec4 ltn = inv * glm::vec4(-1, 1, -1, 1);
		glm::vec4 lbf = inv * glm::vec4(-1, -1, 1, 1);
		glm::vec4 rbn = inv * glm::vec4(1, -1, -1, 1);
		glm::vec4 rtn = inv * glm::vec4(1, 1, -1, 1);
		glm::vec4 rbf = inv * glm::vec4(1, -1, 1, 1);
		glm::vec4 rtf = inv * glm::vec4(1, 1, 1, 1);
		// Divide by w.
		glm::vec3 lbn_pos = glm::vec3(lbn / lbn.w);
		glm::vec3 ltn_pos = glm::vec3(ltn / ltn.w);
		glm::vec3 lbf_pos = glm::vec3(lbf / lbf.w);
		glm::vec3 rbn_pos = glm::vec3(rbn / rbn.w);
		glm::vec3 rtn_pos = glm::vec3(rtn / rtn.w);
		glm::vec3 rbf_pos = glm::vec3(rbf / rbf.w);
		glm::vec3 rtf_pos = glm::vec3(rtf / rtf.w);
		// Get plane normals for all sides of frustum.
		glm::vec3 left_normal = glm::normalize(glm::cross(lbf_pos - lbn_pos, ltn_pos - lbn_pos));
		glm::vec3 right_normal = glm::normalize(glm::cross(rtn_pos - rbn_pos, rbf_pos - rbn_pos));
		glm::vec3 top_normal = glm::normalize(glm::cross(ltn_pos - rtn_pos, rtf_pos - rtn_pos));
		glm::vec3 bottom_normal = glm::normalize(glm::cross(rbf_pos - rbn_pos, lbn_pos - rbn_pos));
		glm::vec3 near_normal = glm::normalize(glm::cross(ltn_pos - lbn_pos, rbn_pos - lbn_pos));
		glm::vec3 far_normal = glm::normalize(glm::cross(rtf_pos - rbf_pos, lbf_pos - rbf_pos));
		// Plane equations compactly represent a plane in 3D space.
		// We want a way to compute the distance to the plane while preserving the sign to know which side we're on.
		// Let:
		//    O: an arbitrary point on the plane
		//    N: the normal vector for the plane, pointing in the direction
		//       we want to be "positive".
		//    X: Position we want to check.
		//
		// Distance D to the plane can now be expressed as a simple operation:
		// D = dot((X - O), N) = dot(X, N) - dot(O, N)
		//
		// We can reduce this to one dot product by assuming that X is four-dimensional (4th component = 1.0).
		// The normal can be extended to four dimensions as well:
		// X' = vec4(X, 1.0)
		// N' = vec4(N, -dot(O, N))
		//
		// The expression now reduces to: D = dot(X', N')
		planes[0] = glm::vec4(near_normal, -glm::dot(near_normal, lbn_pos));   // Near
		planes[1] = glm::vec4(far_normal, -glm::dot(far_normal, lbf_pos));    // Far
		planes[2] = glm::vec4(left_normal, -glm::dot(left_normal, lbn_pos));   // Left
		planes[3] = glm::vec4(right_normal, -glm::dot(right_normal, rbn_pos));  // Right
		planes[4] = glm::vec4(top_normal, -glm::dot(top_normal, ltn_pos));    // Top
		planes[5] = glm::vec4(bottom_normal, -glm::dot(bottom_normal, lbn_pos)); // Bottom
	}

	// ref: https://arm-software.github.io/opengl-es-sdk-for-android/terrain.html
	bool SceneCamera::intersectsAABB(glm::vec3 start, glm::vec3 end)
	{
		// If all corners of an axis-aligned bounding box are on the "wrong side" (negative distance)
		// of at least one of the frustum planes, we can safely cull the mesh.
		glm::vec4 corners[8];

		corners[0] = glm::vec4(start.x, start.y, start.z, 1);
		corners[1] = glm::vec4(start.x, start.y, end.z, 1);
		corners[2] = glm::vec4(start.x, end.y, start.z, 1);
		corners[3] = glm::vec4(start.x, end.y, end.z, 1);
		corners[4] = glm::vec4(end.x, start.y, start.z, 1);
		corners[5] = glm::vec4(end.x, start.y, end.z, 1);
		corners[6] = glm::vec4(end.x, end.y, start.z, 1);
		corners[7] = glm::vec4(end.x, end.y, end.z, 1);

		//for (unsigned int c = 0; c < 8; c++)
		//{
		//    // Require 4-dimensional coordinates for plane equations.
		//    corners[c] = glm::vec4(aabb[c], 1.0f);
		//}
		for (unsigned int p = 0; p < 6; p++)
		{
			bool inside_plane = false;
			for (unsigned int c = 0; c < 8; c++)
			{
				// If dot product > 0, we're "inside" the frustum plane,
				// otherwise, outside.
				if (glm::dot(corners[c], planes[p]) > 0.0f)
				{
					inside_plane = true;
					break;
				}
			}
			if (!inside_plane)
				return false;
		}
		return true;
	}

	bool SceneCamera::save(std::string path) {

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		doc.append_node(decl);

		rapidxml::xml_node<>* cameraNode = doc.allocate_node(rapidxml::node_element, "SceneCamera");
		doc.append_node(cameraNode);

		rapidxml::xml_node<>* positionNode = doc.allocate_node(rapidxml::node_element, "Position");
		positionNode->append_attribute(doc.allocate_attribute("X", doc.allocate_string(std::to_string(position.x).c_str())));
		positionNode->append_attribute(doc.allocate_attribute("Y", doc.allocate_string(std::to_string(position.y).c_str())));
		positionNode->append_attribute(doc.allocate_attribute("Z", doc.allocate_string(std::to_string(position.z).c_str())));
		cameraNode->append_node(positionNode);

		rapidxml::xml_node<>* angleNode = doc.allocate_node(rapidxml::node_element, "Angle");
		angleNode->append_attribute(doc.allocate_attribute("Horizontal", doc.allocate_string(std::to_string(horizontalAngle).c_str())));
		angleNode->append_attribute(doc.allocate_attribute("Vertical", doc.allocate_string(std::to_string(verticalAngle).c_str())));
		cameraNode->append_node(angleNode);

		std::string xml_as_string;
		rapidxml::print(std::back_inserter(xml_as_string), doc);

		std::ofstream file_stored(path);
		file_stored << doc;
		file_stored.close();
		doc.clear();
		return true;
	}
}