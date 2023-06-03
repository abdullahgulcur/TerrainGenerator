#include "pch.h"
#include "menu.h"
#include "editorcontext.h"
#include "GLM/gtc/type_ptr.hpp"

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

		Menu::initDefaultIcons();

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


	void Menu::inputControl() {

		if (!ImGui::GetIO().KeyCtrl) {

			if (ImGui::IsKeyPressed('T'))
				optype = ImGuizmo::OPERATION::TRANSLATE;

			if (ImGui::IsKeyPressed('R'))
				optype = ImGuizmo::OPERATION::ROTATE;

			if (ImGui::IsKeyPressed('S'))
				optype = ImGuizmo::OPERATION::SCALE;
		}

		if (ImGui::IsMouseReleased(0)) {

			if (terrainHolded) {
				terrainSelected = true;
				terrainHolded = false;

				environmentSelected = false;
			}

			if (environmentHolded) {
				environmentSelected = true;
				environmentHolded = false;

				terrainSelected = false;
			}

		}

		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)) || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_KeyPadEnter))) {

			
		}

		if (ImGui::IsMouseDoubleClicked(0)) {

		
		}

		if (scenePanelClicked) {

			if (!ImGuizmo::IsUsing()) {

			}

			//terrainColored = false;
			//environmentColored = false;
			//environmentSelected = false;
			//environmentColored = false;

			scenePanelClicked = false;
		}

		if (ImGui::GetIO().KeyCtrl) {

			if (ImGui::IsKeyPressed('D')) {

			}

			if (ImGui::IsKeyPressed('S')) {

				CoreContext::instance->scene->saveScene(Scene::getActiveScenePath());
			}

		}

		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete))) {

		}
	}

	void Menu::update() {

		Menu::newFrameImGui();
		Menu::createPanels();
		Menu::renderImGui();

		Menu::inputControl();

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
		//Menu::createFilesPanel();
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
				ImGui::SetCursorPos(ImVec2(width - 90, pos.y));

				int frame_padding = 1;
				ImVec2 size = ImVec2(16.0f, 16.0f);
				ImVec2 uv0 = ImVec2(0.0f, 0.0f);
				ImVec2 uv1 = ImVec2(1.0f, 1.0f);
				ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				ImVec4 bg_col = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

				ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
				ImGui::SetNextItemWidth(80);
				static int sceneIndex = CoreContext::instance->scene->activeSceneIndex;
				std::vector<const char*> sceneNames;
				sceneNames.push_back("Scene0");
				sceneNames.push_back("Scene1");
				sceneNames.push_back("Scene2");
				sceneNames.push_back("Scene3");
				sceneNames.push_back("Scene4");

				if (ImGui::Combo("##0", &sceneIndex, &sceneNames[0], sceneNames.size())) {
					EditorContext::instance->camera->save();
					Scene::changeScene(sceneIndex);
					EditorContext::instance->camera->changeSceneCamera();
					Menu::resetVariables();
				}

				ImGui::PopStyleColor();

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

		Scene* scene = CoreContext::instance->scene;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 5));
		ImGui::Begin("Inspector");


		ImGui::Indent(6);

		if (environmentSelected)
			Menu::showEnvironmentMenu();

		if (terrainSelected) {
			Terrain* terrain = CoreContext::instance->scene->terrain;
			Menu::showTerrainMenu(terrain);
		}

		ImGui::End();
		ImGui::PopStyleVar();

	}

	void Menu::createStatisticsPanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
		ImGui::Begin("Statistics");

		ImGui::TextColored(DEFAULT_TEXT_COLOR, "%.1f FPS", ImGui::GetIO().Framerate);

		ImGui::End();
		ImGui::PopStyleVar();

	}


	void Menu::createHierarchyPanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
		ImGui::Begin("Hierarchy");
		ImGui::PopStyleVar();

		Menu::hiearchyCreateButton();

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.f);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

		ImVec2 size = ImGui::GetWindowSize();
		ImVec2 scrolling_child_size = ImVec2(size.x - 10, size.y - 54);
		ImGui::BeginChild("scrolling", scrolling_child_size, true, ImGuiWindowFlags_HorizontalScrollbar);

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {

				if (!anyEntityHovered && !popupItemClicked) {

					terrainSelected = false;
					terrainColored = false;
					environmentSelected = false;
					environmentColored = false;
				}
			}
		}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);

		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 5));

		ImGui::SetNextItemOpen(true);

		bool treeNodeOpen = ImGui::TreeNode("##0");


		ImVec2 imgsize = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);

		ImGui::SameLine(20);
		ImGui::TextColored(DEFAULT_TEXT_COLOR, "Scene");

		anyEntityHovered = false;
		popupItemClicked = false;
		if (treeNodeOpen)
		{
			// ------ Environment

			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			ImGui::TreeNodeEx("##environment", node_flags, "");

			if (ImGui::IsItemHovered())
				anyEntityHovered = true;

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {

				if (!ImGui::IsItemToggledOpen()) {

					environmentColored = true;
					environmentHolded = true;

					terrainColored = false;
				}
			}
			ImGui::SameLine();
			ImGui::TextColored(environmentColored ? TEXT_SELECTED_COLOR : DEFAULT_TEXT_COLOR, "Environment");

			// ------ Terrain

			Terrain* terrain = CoreContext::instance->scene->terrain;
			if (terrain) {
				ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				ImGui::TreeNodeEx((void*)(intptr_t)&terrain, node_flags, "");

				if (ImGui::IsItemHovered())
					anyEntityHovered = true;

				if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {

					if (!ImGui::IsItemToggledOpen()) {

						terrainColored = true;
						terrainHolded = true;

						environmentColored = false;
					}
				}
				ImGui::SameLine();
				ImGui::TextColored(terrainColored ? TEXT_SELECTED_COLOR : DEFAULT_TEXT_COLOR, "Terrain");
			}
			//---------	

			ImGui::TreePop();
		}

		ImGui::EndChild();
		ImGui::End();
	}

	void Menu::showTerrainMenu(Terrain* terrain) {

		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemOpen(true);

		char str[4] = { '#', '#', '0', '\0' };
		char indexChar = '0';
		str[2] = indexChar;
		bool treeNodeOpen = ImGui::TreeNode(str);

		int frame_padding = 1;
		ImVec2 size = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
		ImVec4 bg_col = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

		ImGui::SameLine(25);
		ImGui::Image((ImTextureID)meshRendererTextureId, size, uv0, uv1, tint_col, border_col);
		ImGui::SameLine();
		ImGui::TextColored(DEFAULT_TEXT_COLOR, "  Terrain");

		if (treeNodeOpen) {

			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 3));
			float itemWidth = (width - 180) * 0.33f;

			ImGuiColorEditFlags misc_flags = ImGuiColorEditFlags_AlphaPreview;

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Ambient Amount"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##ambientAmount", &terrain->ambientAmount, 0.01f, 0.0f, 3.0f, "%.2f");

			ImGui::Separator();

			itemWidth = (width - 190) * 0.5f;

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "SPECULAR  Power"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##specularPower", &terrain->specularPower, 0.01f, 0.0f, 32.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Amount"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##specularAmount", &terrain->specularAmount, 0.01f, 0.0f, 3.0f, "%.2f");

			ImGui::Separator();

			itemWidth = (width - 190) * 0.5f;

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "BLEND  Distance"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##blendDistance", &terrain->blendDistance, 0.1f, 0.0f, 2000.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Amount"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##blendAmount", &terrain->blendAmount, 0.1f, 0.0f, 1000.0f, "%.2f");

			ImGui::Separator();

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "SCALE (N:Near F:Far)");

			itemWidth = (width - 120) * 0.5f;
			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Color 0  N"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color0_dist0", &terrain->scale_color0_dist0, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "F"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color0_dist1", &terrain->scale_color0_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Color 1  N"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color1_dist0", &terrain->scale_color1_dist0, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "F"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color1_dist1", &terrain->scale_color1_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Color 2  N"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color2_dist0", &terrain->scale_color2_dist0, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "F"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color2_dist1", &terrain->scale_color2_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Color 3  N"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color3_dist0", &terrain->scale_color3_dist0, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "F"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color3_dist1", &terrain->scale_color3_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Color 4  N"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color4_dist0", &terrain->scale_color4_dist0, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "F"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color4_dist1", &terrain->scale_color4_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Color 5  N"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color5_dist0", &terrain->scale_color5_dist0, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "F"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color5_dist1", &terrain->scale_color5_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Color 6  N"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color6_dist0", &terrain->scale_color6_dist0, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "F"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color6_dist1", &terrain->scale_color6_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Color 7  N"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color7_dist0", &terrain->scale_color7_dist0, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "F"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##scale_color7_dist1", &terrain->scale_color7_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::Separator();

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "FLAT COLOR  0"); ImGui::SameLine();

			ImVec4 color0 = ImVec4(terrain->color0.r, terrain->color0.g, terrain->color0.b, 1);
			ImGui::ColorEdit4("MyColor##31", (float*)&color0, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | misc_flags);
			terrain->color0.r = color0.x;
			terrain->color0.g = color0.y;
			terrain->color0.b = color0.z;

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "1"); ImGui::SameLine();

			ImVec4 color1 = ImVec4(terrain->color1.r, terrain->color1.g, terrain->color1.b, 1);
			ImGui::ColorEdit4("MyColor##32", (float*)&color1, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | misc_flags);
			terrain->color1.r = color1.x;
			terrain->color1.g = color1.y;
			terrain->color1.b = color1.z;

			ImGui::Separator();

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "MACRO");

			itemWidth = (width - 120) * 0.33f;
			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Scale  0"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##macroScale_0", &terrain->macroScale_0, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "1"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##macroScale_1", &terrain->macroScale_1, 0.001f, 0.0f, 1.0f, "%.3f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "2"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##macroScale_2", &terrain->macroScale_2, 0.0001f, 0.0f, 1.0f, "%.4f");

			itemWidth = (width - 180) * 0.33f;

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Amount"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##macroAmount", &terrain->macroAmount, 0.01f, 0.0f, 10.0f, "%.2f");

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Power"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##macroPower", &terrain->macroPower, 0.01f, 0.0f, 10.0f, "%.2f");

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Opacity"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##macroOpacity", &terrain->macroOpacity, 0.01f, 0.0f, 10.0f, "%.2f");

			ImGui::Separator();

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "OVERLAY BLEND");

			itemWidth = (width - 160) * 0.25f;

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Group 0  S"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##overlayBlendScale0", &terrain->overlayBlendScale0, 0.001f, 0.0f, 1.0f, "%.3f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "A"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##overlayBlendAmount0", &terrain->overlayBlendAmount0, 0.01f, 0.0f, 32.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "P"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##overlayBlendPower0", &terrain->overlayBlendPower0, 0.01f, 0.0f, 32.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "O"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##overlayBlendOpacity0", &terrain->overlayBlendOpacity0, 0.01f, 0.0f, 32.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Group 1  S"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##overlayBlendScale1", &terrain->overlayBlendScale1, 0.001f, 0.0f, 1.0f, "%.3f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "A"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##overlayBlendAmount1", &terrain->overlayBlendAmount1, 0.01f, 0.0f, 32.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "P"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##overlayBlendPower1", &terrain->overlayBlendPower1, 0.01f, 0.0f, 32.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "O"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##overlayBlendOpacity1", &terrain->overlayBlendOpacity1, 0.01f, 0.0f, 32.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Group 0  S"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##overlayBlendScale2", &terrain->overlayBlendScale2, 0.001f, 0.0f, 1.0f, "%.3f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "A"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##overlayBlendAmount2", &terrain->overlayBlendAmount2, 0.01f, 0.0f, 32.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "P"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##overlayBlendPower2", &terrain->overlayBlendPower2, 0.01f, 0.0f, 32.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "O"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##overlayBlendOpacity2", &terrain->overlayBlendOpacity2, 0.01f, 0.0f, 32.0f, "%.2f");

			ImGui::Separator();

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "SLOPE");

			itemWidth = (width - 200) * 0.5f;

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Transition 0  Bias"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##slopeBias0", &terrain->slopeBias0, 0.001f, 0.0f, 1.0f, "%.3f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Soft"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##slopeSharpness0", &terrain->slopeSharpness0, 0.001f, 0.0f, 1.0f, "%.3f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Transition 1  Bias"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##slopeBias1", &terrain->slopeBias1, 0.001f, 0.0f, 1.0f, "%.3f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Soft"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##slopeSharpness1", &terrain->slopeSharpness1, 0.001f, 0.0f, 1.0f, "%.3f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "HEIGHT");

			itemWidth = (width - 200) * 0.5f;

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Transition 0  Bias"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##heightBias0", &terrain->heightBias0, 0.01f, 0.0f, 300.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Soft"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##heightSharpness0", &terrain->heightSharpness0, 0.01f, 0.0f, 100.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Transition 1  Bias"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##heightBias1", &terrain->heightBias1, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Soft"); ImGui::PushItemWidth(itemWidth);
			ImGui::SameLine(); ImGui::DragFloat("##heightSharpness1", &terrain->heightSharpness1, 0.01f, 0.0f, 100.0f, "%.2f");

			ImGui::Separator();

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Show Bounds"); ImGui::SameLine();
			ImGui::Checkbox("##showBounds", &terrain->showBounds);

			glm::vec3 camPos = CoreContext::instance->scene->cameraInfo.camPos;
			std::string camPosStr = "Camera Pos X: " + std::to_string(camPos.x) + " Y: " + std::to_string(camPos.y) + " Z: " + std::to_string(camPos.z);
			ImGui::TextColored(DEFAULT_TEXT_COLOR, &camPosStr[0]);
			std::string terrainRenderDurationStr = "Terrain render time (microseconds): " + std::to_string(CoreContext::instance->renderer->terrainRenderDuration);
			ImGui::TextColored(DEFAULT_TEXT_COLOR, &terrainRenderDurationStr[0]);

			ImGui::TreePop();
		}

		ImGui::Separator();
	}

	void Menu::showEnvironmentMenu() {

		Terrain* terrain = CoreContext::instance->scene->terrain;

		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemOpen(true);

		char str[4] = { '#', '#', '0', '\0' };
		char indexChar = '0';
		str[2] = indexChar;
		bool treeNodeOpen = ImGui::TreeNode(str);

		int frame_padding = 1;
		ImVec2 size = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
		ImVec4 bg_col = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

		ImGui::SameLine(25);
		ImGui::Image((ImTextureID)meshRendererTextureId, size, uv0, uv1, tint_col, border_col);
		ImGui::SameLine();
		ImGui::TextColored(DEFAULT_TEXT_COLOR, "  Environment");

		if (treeNodeOpen) {

			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 3));
			float itemWidth = (width - 180) * 0.33f;

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Sun Direction  X");
			ImGui::SameLine();
			ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##1", &terrain->lightDir.x, 0.01f, -1.0f, 1.0f, "%.2f");
			ImGui::SameLine();
			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Y");
			ImGui::SameLine();
			ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##2", &terrain->lightDir.y, 0.01f, -1.0f, 1.0f, "%.2f");
			ImGui::SameLine();
			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Z");
			ImGui::SameLine();
			ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##3", &terrain->lightDir.z, 0.01f, -1.0f, 1.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Light Power"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##111110", &terrain->lightPow, 0.01f, 0.0f, 32.0f, "%.2f");

			ImGui::Separator();

			ImVec4 fog_color = ImVec4(terrain->fogColor.r, terrain->fogColor.g, terrain->fogColor.b, 1);
			
			ImGuiColorEditFlags misc_flags = ImGuiColorEditFlags_AlphaPreview;

			itemWidth = (width - 270) * 0.34f;

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "FOG  Distance"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##distanceNear", &terrain->distanceNear, 1.f, 0.0f, 3000.0f, "%.0f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Blend"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##fogBlendDistance", &terrain->fogBlendDistance, 1.f, 0.0f, 3000.0f, "%.0f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Max"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##maxFog", &terrain->maxFog, 0.01f, 0.0f, 1.0f, "%.2f");
			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Color"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);
			ImGui::ColorEdit4("##fog_color", (float*)&fog_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | misc_flags);
			terrain->fogColor.r = fog_color.x;
			terrain->fogColor.g = fog_color.y;
			terrain->fogColor.b = fog_color.z;

			ImGui::TreePop();
		}

		ImGui::Separator();
	}

	void Menu::hiearchyCreateButton() {

		if (ImGui::Button("Create", ImVec2(60, 20)))
			ImGui::OpenPopup("context_menu_scene_hierarchy_popup");

		ImGui::SetNextWindowSize(ImVec2(210, 100));

		if (ImGui::BeginPopup("context_menu_scene_hierarchy_popup"))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 2.0f));

			ImVec2 p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
			ImGui::Dummy(ImVec2(0, 1));

			p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
			ImGui::Dummy(ImVec2(0, 1));

			if (ImGui::Selectable("   Terrain")) {

				if (!CoreContext::instance->scene->terrain) {

					CoreContext::instance->scene->terrain = new Terrain;
					CoreContext::instance->scene->terrain->start();
				}
				
				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				return;
			}

			ImGui::PopStyleColor();
			ImGui::EndPopup();
		}

	}

	void Menu::createScenePanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Scene");

		scenePos = ImGui::GetCursorScreenPos();
		ImVec2 content = ImGui::GetContentRegionAvail();

		SceneCamera* camera = EditorContext::instance->camera;
		Scene* scene = CoreContext::instance->scene;

		ImGui::Image((ImTextureID)scene->textureBuffer, content, ImVec2(0, 1), ImVec2(1, 0));
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			scenePanelClicked = true;

		if (content.x != sceneRect.x || content.y != sceneRect.y) {

			scene->setSize((int)content.x, (int)content.y);

			camera->aspectRatio = content.x / content.y;
			sceneRect = content;
			camera->updateProjectionMatrix((int)content.x, (int)content.y);
		}

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

	void Menu::initDefaultIcons() {

		folder64TextureId = Texture::loadPNG_RGBA8("resources/editor/icons/folder_64.png");
		folderClosed16TextureId = Texture::loadPNG_RGBA8("resources/editor/icons/folder_closed_16.png");
		greaterTextureId = Texture::loadPNG_RGBA8("resources/editor/icons/greater_16.png");
		pauseTextureId = Texture::loadPNG_RGBA8("resources/editor/icons/pause_16.png");
		stopTextureId = Texture::loadPNG_RGBA8("resources/editor/icons/stop_16.png");
		startTextureId = Texture::loadPNG_RGBA8("resources/editor/icons/start_16.png");
		sceneFileTextureId = Texture::loadPNG_RGBA8("resources/editor/icons/scene.png");
		meshRendererTextureId = Texture::loadPNG_RGBA8("resources/editor/icons/mesh_renderer.png");
		meshColliderTextureId = Texture::loadPNG_RGBA8("resources/editor/icons/mesh_collider.png");
		contextMenuTextureId = Texture::loadPNG_RGBA8("resources/editor/icons/context_menu.png");
		transformTextureId = Texture::loadPNG_RGBA8("resources/editor/icons/transform_16.png");
		cameraTextureId = Texture::loadPNG_RGBA8("resources/editor/icons/camera.png");
		particleSystemTextureId = Texture::loadPNG_RGBA8("resources/editor/icons/particlesystem.png");
		folderOpened16TextureId = Texture::loadPNG_RGBA8("resources/editor/icons/folder_opened_16.png");
	}

	void Menu::resetVariables() {

		anyEntityHovered = false;
		popupItemClicked = false;

		ImGuizmo::OPERATION optype = ImGuizmo::OPERATION::TRANSLATE;

		scenePanelClicked = false;

		terrainSelected = false;
		terrainHolded = false;
		terrainColored = false;
		environmentSelected = false;
		environmentHolded = false;
		environmentColored = false;
	}

}