#pragma once

#include "corecontext.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"

#include "imguizmo/imguizmo.h"

#define WHITE ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
#define DEFAULT_TEXT_COLOR ImVec4(0.8f, 0.8f, 0.8f, 1.0f)
#define TEXT_SELECTED_COLOR ImVec4(0.2f, 0.72f, 0.95f, 1.f)

namespace Editor {

	class Editor;

	class Menu {

	private:


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
		void createHierarchyPanel();
		void createFilesPanel();
		void createScenePanel();
		void createInspectorPanel();
		void setTheme();


	};
}