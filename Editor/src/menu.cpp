#include "pch.h"
#include "corecontext.h"
#include "editorcontext.h"
#include "menu.h"

using namespace Core;

namespace Editor {

	Menu::Menu() {

		std::cout << "Menu started." << std::endl;
	}

	Menu::~Menu() {

		Menu::destroyImGui();
		std::cout << "Menu shutdown." << std::endl;
	}

	void Menu::init() {

		Menu::initImGui();
	}

	void Menu::initImGui() {

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			Menu::setTheme();

		ImGui_ImplGlfw_InitForOpenGL(CoreContext::instance->glfwContext->GLFW_window, true);
		ImGui_ImplOpenGL3_Init("#version 460");
	}

	void Menu::newFrameImGui() {

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void Menu::renderImGui() {

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		ImGuiIO& io = ImGui::GetIO(); (void)io;

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void Menu::destroyImGui() {

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void Menu::update() {

		Menu::newFrameImGui();
		Menu::createPanels();
		Menu::renderImGui();

	}

	void Menu::createPanels() {

		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking; //ImGuiWindowFlags_MenuBar

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		bool p_open = true;

		ImGui::Begin("EditorDockSpace", &p_open, window_flags);
		ImGui::PopStyleVar(1);

		ImGuiIO& io = ImGui::GetIO();

		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
		}

		Menu::mainMenuBar();
		Menu::secondaryMenuBar();
		Menu::createInspectorPanel();
		Menu::createStatisticsPanel();
		Menu::createHierarchyPanel();
		Menu::createFilesPanel();
		Menu::createScenePanel();
		
		ImGui::End();
	}

	void Menu::mainMenuBar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 4));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				//ShowExampleMenuFile();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("View"))
			{
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window"))
			{
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help"))
			{
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	void Menu::secondaryMenuBar()
	{
		ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
		float height = ImGui::GetFrameHeight();

		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.13f, 0.13f, 0.13f, 1.f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 6));
		if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height + 6, window_flags)) {
			if (ImGui::BeginMenuBar()) {

				float width = ImGui::GetWindowSize().x;
				ImVec2 pos = ImGui::GetCursorPos();
				ImGui::SetCursorPos(ImVec2(pos.x + width / 2 - 20, pos.y + 5));

				int frame_padding = 1;
				ImVec2 size = ImVec2(16.0f, 16.0f);
				ImVec2 uv0 = ImVec2(0.0f, 0.0f);
				ImVec2 uv1 = ImVec2(1.0f, 1.0f);
				ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				ImVec4 bg_col = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

				//if (!Editor::instance->gameStarted) {

				//	if (ImGui::ImageButton((ImTextureID)startTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {

				//		//Core::instance->sceneManager->currentScene->backup();
				//		auto file = Core::instance->fileSystem->sceneFileToFile.find(Core::instance->fileSystem->currentSceneFile);
				//		if (file != Core::instance->fileSystem->sceneFileToFile.end())
				//			Core::instance->fileSystem->currentSceneFile->saveEntities(file->second->path);

				//		Editor::instance->gameStarted = true;
				//		Core::instance->startGame();
				//	}
				//}
				//else {

				//	if (ImGui::ImageButton((ImTextureID)stopTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {

				//		int selectedEntityId = -1;
				//		if (selectedEntity)
				//			selectedEntityId = selectedEntity->id;

				//		Editor::instance->gameStarted = false;
				//		Core::instance->sceneManager->restartCurrentScene();

				//		if (selectedEntityId != -1) {
				//			selectedEntity = Core::instance->sceneManager->currentScene->entityIdToEntity[selectedEntityId];
				//			coloredEntity = selectedEntity;
				//		}
				//	}
				//}
				//ImGui::SameLine();

				//if (ImGui::ImageButton((ImTextureID)pauseTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {

				//}

				ImGui::EndMenuBar();
			}
			ImGui::End();
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();


		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 4));
		if (ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down, height + 4, window_flags)) {
			if (ImGui::BeginMenuBar()) {
				//ImGui::Text(statusMessage.c_str());
				ImGui::EndMenuBar();
			}
			ImGui::End();
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}

	void Menu::createInspectorPanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 5));
		ImGui::Begin("Inspector");


		ImGui::End();
		ImGui::PopStyleVar();

	}

	void Menu::createStatisticsPanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
		ImGui::Begin("Statistics");


		ImGui::End();
		ImGui::PopStyleVar();

	}

	void Menu::createHierarchyPanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
		ImGui::Begin("Hierarchy");


		ImGui::End();
		ImGui::PopStyleVar();
	}

	void Menu::createScenePanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Scene");




		scenePos = ImGui::GetCursorScreenPos();
		ImVec2 content = ImGui::GetContentRegionAvail();

		SceneCamera* camera = EditorContext::instance->camera;
		//SceneCamera* camera = (ImTextureID)EditorContext::instance->camera;

		ImGui::Image((ImTextureID)camera->textureBuffer, content, ImVec2(0, 1), ImVec2(1, 0));
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {

			ImVec2 mousePos = ImGui::GetMousePos();
			float mX = mousePos.x < scenePos.x || mousePos.x > scenePos.x + sceneRect.x ? 0 : mousePos.x - scenePos.x;
			float mY = mousePos.y < scenePos.y || mousePos.y > scenePos.y + sceneRect.y ? 0 : mousePos.y - scenePos.y;

			/*if (mX != 0 && mY != 0) {
				sceneClickEntity = Core::instance->renderer->detectAndGetEntityId(mX, sceneRegion.y - mY, camera->FBO, sceneRegion.x, sceneRegion.y,
					camera->projectionViewMatrix, camera->position, camera->planes);
			}*/
			//sceneClickEntity = Editor::instance->renderer->detectAndGetEntityId(mX, sceneRegion.y - mY);
		}

		if (content.x != sceneRect.x || content.y != sceneRect.y) {

			camera->width = content.x;
			camera->height = content.y;

			camera->aspectRatio = content.x / content.y;
			sceneRect = content;
			camera->recreateFBO((int)content.x, (int)content.y);
		}

	/*	if (selectedEntity) {

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			float windowWidth = (float)ImGui::GetWindowWidth();
			float windowHeight = (float)ImGui::GetWindowHeight();

			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
			glm::mat4& model = selectedEntity->transform->model;

			ImGuizmo::Manipulate(glm::value_ptr(Editor::instance->sceneCamera->ViewMatrix), glm::value_ptr(Editor::instance->sceneCamera->ProjectionMatrix),
				optype, ImGuizmo::LOCAL, glm::value_ptr(model));

			if (ImGuizmo::IsUsing())
				selectedEntity->transform->updateTransformUsingGuizmo();
		}*/


		ImGui::End();
		ImGui::PopStyleVar();

	}

	void Menu::createFilesPanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Files");


		ImGui::End();
		ImGui::PopStyleVar();

	}


	void Menu::setTheme()
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
		//colors[ImGuiCol_ChildBg] = ImVec4(0.9f, 0.00f, 0.00f,1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.8f, 0.8f, 0.8f, 1.f);
		colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.8f, 0.8f, 0.8f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.50f, 0.50f, 0.15f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
		colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 1.0f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(8.00f, 8.00f);
		style.FramePadding = ImVec2(5.00f, 2.00f);
		style.CellPadding = ImVec2(6.00f, 6.00f);
		style.ItemSpacing = ImVec2(6.00f, 6.00f);
		style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
		style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
		style.ScrollbarSize = 15;
		style.GrabMinSize = 10;
		style.WindowBorderSize = 0;
		style.ChildBorderSize = 1;
		style.PopupBorderSize = 1;
		style.FrameBorderSize = 1;
		style.TabBorderSize = 1;
		style.WindowRounding = 0;
		style.ChildRounding = 4;
		style.FrameRounding = 4;
		style.PopupRounding = 2;
		style.ScrollbarRounding = 9;
		style.GrabRounding = 3;
		style.LogSliderDeadzone = 4;
		style.TabRounding = 4;
		style.IndentSpacing = 20;
	}
}