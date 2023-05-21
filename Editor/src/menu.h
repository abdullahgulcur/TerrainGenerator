#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"
#include "imguizmo/imguizmo.h"

#include "corecontext.h"



#define WHITE ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
#define DEFAULT_TEXT_COLOR ImVec4(0.8f, 0.8f, 0.8f, 1.0f)
#define TEXT_SELECTED_COLOR ImVec4(0.2f, 0.72f, 0.95f, 1.f)

namespace Core {
	
	class Entity;
	class Transform;
	class MeshRenderer;
	class Camera;
	class Terrain;
	class Component;
}

using namespace Core;

namespace Editor {

	class Editor;

	class Menu {

	private:

		unsigned int folder64TextureId;
		unsigned int folderClosed16TextureId;
		unsigned int folderOpened16TextureId;
		unsigned int greaterTextureId;
		unsigned int stopTextureId;
		unsigned int startTextureId;
		unsigned int pauseTextureId;
		unsigned int sceneFileTextureId;
		unsigned int contextMenuTextureId;
		unsigned int meshColliderTextureId;
		unsigned int meshRendererTextureId;
		unsigned int transformTextureId;
		unsigned int cameraTextureId;
		unsigned int particleSystemTextureId;

		Entity* holdedEntity = NULL;
		Entity* coloredEntity = NULL;
		Entity* selectedEntity = NULL;
		Entity* renameEntity = NULL;
		Entity* toBeOpened = NULL;
		Entity* sceneClickEntity = NULL;

		bool anyEntityHovered = false;
		bool popupItemClicked = false;

		ImGuizmo::OPERATION optype = ImGuizmo::OPERATION::TRANSLATE;

		bool scenePanelClicked = false;

		void inputControl();


	public:

		ImVec2 scenePos;
		ImVec2 sceneRect;

		Menu();
		~Menu();
		void init();
		void initImGui();
		void newFrameImGui();
		void renderImGui();
		void destroyImGui();
		void update();
		void createPanels();
		void mainMenuBar();
		void secondaryMenuBar();
		void createStatisticsPanel();
		//void createHierarchyPanel();
		void createFilesPanel();
		void createScenePanel();
		void createInspectorPanel();
		void setTheme();

		void createHierarchyPanel();
		void hiearchyCreateButton();
		void createSceneGraphRecursively(Transform& transform);

		// Inspector
		void showEntityName();
		void addComponentButton();
		void showTransformComponent(int index);
		void showMeshRendererComponent(int index, MeshRenderer* comp);
		void showTerrainComponent(int index, Terrain* comp);
		//void showGameCameraComponent(Core::Camera* camComp, int index);*/
		//void showParticleSystemComponent(ParticleSystem* particleSystemComp, int index);

		bool contextMenuPopup(Component* component, int index);
		void initDefaultIcons();

	};
}