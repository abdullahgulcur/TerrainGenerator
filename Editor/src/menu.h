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
	
	class Terrain;
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

		bool anyEntityHovered = false;
		bool popupItemClicked = false;

		ImGuizmo::OPERATION optype = ImGuizmo::OPERATION::TRANSLATE;

		bool scenePanelClicked = false;

		bool terrainSelected = false;
		bool terrainHolded = false;
		bool terrainColored = false;
		bool environmentSelected = false;
		bool environmentHolded = false;
		bool environmentColored = false;

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
		void createScenePanel();
		void createInspectorPanel();
		void setTheme();

		void createHierarchyPanel();
		void hiearchyCreateButton();

		// Inspector
		void showTerrainMenu(Terrain* terrain);
		void showEnvironmentMenu();

		void initDefaultIcons();
		void resetVariables();
	};
}