#include "pch.h"
#include "menu.h"
#include "editorcontext.h"

#include "component/transform.h"
#include "entity.h"

#include "GLM/gtc/type_ptr.hpp"


//using namespace Core;

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

			if (holdedEntity) {
				selectedEntity = holdedEntity;
				holdedEntity = NULL;
			}
		}

		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)) || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_KeyPadEnter))) {

			
		}

		if (ImGui::IsMouseDoubleClicked(0)) {

		
		}

		if (scenePanelClicked) {

			if (!ImGuizmo::IsUsing()) {

				if (sceneClickEntity) {
					selectedEntity = sceneClickEntity;
					coloredEntity = selectedEntity;
				}
				else {
					selectedEntity = NULL;
					coloredEntity = NULL;
				}
			}

			sceneClickEntity = NULL;
			scenePanelClicked = false;
		}

		if (ImGui::GetIO().KeyCtrl) {

			if (ImGui::IsKeyPressed('D')) {

				if (selectedEntity != NULL)
					selectedEntity = CoreContext::instance->scene->duplicate(selectedEntity);
			}

			if (ImGui::IsKeyPressed('S')) {

				CoreContext::instance->scene->saveEntities("resources/scenes/samplescene.xml");
			}

		}

		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete))) {

			if (selectedEntity) {
				selectedEntity->destroy();
				selectedEntity = NULL;
			}
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


		ImGui::Indent(6);

		if (selectedEntity) {

			Menu::showEntityName();

			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 5));

			ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.23f, 0.23f, 0.23f, 1.f));

			int index = 2;
			Menu::showTransformComponent(index++);

			if (MeshRenderer* meshRendererComp = selectedEntity->getComponent<MeshRenderer>()) {

				//MaterialFile& material = *meshRendererComp->mat;
				Menu::showMeshRendererComponent(index++, meshRendererComp);
				//Menu::showMaterialProperties(index++, meshRendererComp->materialFile);
			}

			if (Terrain* terrainComp = selectedEntity->getComponent<Terrain>()) {

				Menu::showTerrainComponent(index++, terrainComp);
			}

			//if (GameCamera* gameCameraComp = selectedEntity->getComponent<GameCamera>())
			//	Menu::showGameCameraComponent(gameCameraComp, index++);

			//if (ParticleSystem* particleSystemComp = selectedEntity->getComponent<ParticleSystem>())
			//	Menu::showParticleSystemComponent(particleSystemComp, index++);

			ImGui::PopStyleColor(); // for the separator

			Menu::addComponentButton();
		}

		ImGui::End();
		ImGui::PopStyleVar();

	}

	void Menu::addComponentButton() {

		ImVec2 pos = ImGui::GetCursorPos();
		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 5));

		ImGui::Indent((width - 120) / 2);
		if (ImGui::Button("Add Component", ImVec2(120, 23))) {

			pos = ImVec2(ImGui::GetCursorScreenPos().x - 25, ImGui::GetCursorScreenPos().y);
			ImGui::SetNextWindowPos(pos);
			ImGui::OpenPopup("add_component_popup");

		}
		ImGui::Unindent((width - 120) / 2);

		ImGui::SetNextWindowSize(ImVec2(180, 280));

		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.18f, 0.18f, 0.18f, 1.f));
		if (ImGui::BeginPopupContextItem("add_component_popup"))
		{
			if (ImGui::Selectable("   Animation")) {

			}
			//ImGui::Separator();

			//if (ImGui::Selectable("   Animator")) { // bir gun buraya gelinirse o zaman buyuk bir is yapilmis demektir ;)

			//}

			ImGui::Separator();

			if (ImGui::Selectable("   Game Camera")) {

				//GameCamera* gameCamComp = selectedEntity->getComponent<GameCamera>();

				//if (!gameCamComp) {

				//	gameCamComp = selectedEntity->addComponent<GameCamera>();
				//	Core::instance->sceneManager->currentScene->primaryCamera = gameCamComp;
				//	gameCamComp->width = gameRegion.x;
				//	gameCamComp->height = gameRegion.y;
				//	gameCamComp->init(selectedEntity->transform);//, gameRegion.x, gameRegion.y
				//}

			}

			ImGui::Separator();

			if (ImGui::Selectable("   Collider (Box)")) {

				//BoxCollider* boxColliderComp = lastSelectedEntity->addComponent<BoxCollider>();
				//boxColliderComp->init(editor);
			}

			ImGui::Separator();

			if (ImGui::Selectable("   Collider (Capsule)")) {

				//CapsuleCollider* capsuleColliderComp = lastSelectedEntity->addComponent<CapsuleCollider>();
				//capsuleColliderComp->init(editor);
				// 
				//capsuleColliderComp->pmat = &editor->fileSystem.physicmaterials["Default"];

				//float max = lastSelectedEntity->transform->globalScale.x > lastSelectedEntity->transform->globalScale.y ?
				//	lastSelectedEntity->transform->globalScale.x : lastSelectedEntity->transform->globalScale.y;
				//max = max > lastSelectedEntity->transform->globalScale.z ? max : lastSelectedEntity->transform->globalScale.z;

				//float halfHeight = lastSelectedEntity->transform->globalScale * capsuleColliderComp->height / 2.f;
				//capsuleColliderComp->shape = editor->physics.gPhysics->createShape(PxCapsuleGeometry(halfHeight, ), *sphereColliderComp->pmat->pxmat);
				//sphereColliderComp->shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
				//glm::vec3 center = sphereColliderComp->transform->globalScale * sphereColliderComp->center;
				//sphereColliderComp->shape->setLocalPose(PxTransform(center.x, center.y, center.z));

				//if (Rigidbody* rb = lastSelectedEntity->getComponent<Rigidbody>())
				//	rb->body->attachShape(*sphereColliderComp->shape);
			}

			ImGui::Separator();

			if (ImGui::Selectable("   Collider (Mesh)")) {

				//if (MeshRenderer* mr = lastSelectedEntity->getComponent<MeshRenderer>()) {

				//	MeshCollider* meshColliderComp = lastSelectedEntity->addComponent<MeshCollider>();
				//	Rigidbody* rb = lastSelectedEntity->getComponent<Rigidbody>();
				//	meshColliderComp->init(editor, rb, mr, true);
				//}
			}

			ImGui::Separator();

			if (ImGui::Selectable("   Collider (Sphere)")) {

				//SphereCollider* sphereColliderComp = lastSelectedEntity->addComponent<SphereCollider>();
				//sphereColliderComp->init(editor);
			}

			ImGui::Separator();

			if (ImGui::Selectable("   Light")) {

				//if (Light* lightComp = lastSelectedEntity->addComponent<Light>()) {

				//	Transform* lighTransform = lastSelectedEntity->transform;
				//	editor->scene->pointLightTransforms.push_back(lighTransform);
				//	editor->scene->recompileAllMaterials();
				//}
				//else
				//	statusMessage = "There is existing component in the same type!";
			}

			ImGui::Separator();

			if (ImGui::Selectable("   Mesh Renderer")) {

				//// bunlari daha duzenli hale getirebilirsin

				if (MeshRenderer* meshRendererComp = selectedEntity->addComponent<MeshRenderer>()) {

					meshRendererComp->entity = selectedEntity; ////
				//	meshRendererComp->meshFile = NULL;
				//	meshRendererComp->materialFile = NULL;// Core::instance->fileSystem->pbrMaterialNoTexture;
				}
				////else
				////	statusMessage = "There is existing component in the same type!";
			}
			ImGui::Separator();

			if (ImGui::Selectable("   Particle System")) {

				//if (ParticleSystem* particleSystemComp = selectedEntity->addComponent<ParticleSystem>()) {
				//	particleSystemComp->start();
				//}
				//else
				//	statusMessage = "There is existing component in the same type!";
			}
			ImGui::Separator();

			if (ImGui::Selectable("   Rigidbody")) {

				//if (Rigidbody* rigidbodyComp = lastSelectedEntity->addComponent<Rigidbody>()) {

				//	glm::quat myquaternion = glm::quat(lastSelectedEntity->transform->globalRotation);
				//	PxTransform tm(PxVec3(lastSelectedEntity->transform->globalPosition.x, lastSelectedEntity->transform->globalPosition.y,
				//		lastSelectedEntity->transform->globalPosition.z),
				//		PxQuat(myquaternion.x, myquaternion.y, myquaternion.z, myquaternion.w));

				//	rigidbodyComp->body = editor->physics->gPhysics->createRigidDynamic(tm);
				//	rigidbodyComp->body->setMass(1.f);
				//	editor->physics->gScene->addActor(*rigidbodyComp->body);

				//	for (auto& comp : lastSelectedEntity->getComponents<Collider>())
				//		rigidbodyComp->body->attachShape(*comp->shape);
				//}
				//else
				//	statusMessage = "There is existing component in the same type!";

			}
			ImGui::Separator();

			if (ImGui::Selectable("   Terrain")) {

				//if (TerrainGenerator* terrainComp = lastSelectedEntity->addComponent<TerrainGenerator>())
				//	terrainComp->init(&editor->fileSystem->materials["Default"]);

				if (Terrain* terrainComp = selectedEntity->addComponent<Terrain>()) {
					terrainComp->start();
				}
			}

			//if (ImGui::Selectable("   Script")) {

			//}

			ImGui::EndPopup();
		}
		ImGui::PopStyleColor();
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
		ImGui::PopStyleVar();

		Menu::hiearchyCreateButton();

		//if (Core::instance->sceneManager->currentScene) {

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
						selectedEntity = NULL;
						coloredEntity = NULL;
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
			ImGui::TextColored(DEFAULT_TEXT_COLOR, "MainScene");

			anyEntityHovered = false;
			popupItemClicked = false;
			if (treeNodeOpen)
			{
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_TREENODE_ENTITY"))
					{
						IM_ASSERT(payload->DataSize == sizeof(Entity*));
						Entity* payload_n = *(Entity**)payload->Data;
						CoreContext::instance->scene->root->attachEntity(payload_n);
					}
					ImGui::EndDragDropTarget();
				}
				std::vector<Transform*>& children = CoreContext::instance->scene->root->transform->children;
				for (int i = 0; i < children.size(); i++)
					Menu::createSceneGraphRecursively(*children[i]);
				ImGui::TreePop();
			}

			ImGui::EndChild();
	//	}
		ImGui::End();
	}

	void Menu::createSceneGraphRecursively(Transform& transform) {

		ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

		bool hasChildren = transform.entity->hasAnyChild();
		if (!hasChildren)
			node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		else
			node_flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;

		if (toBeOpened == transform.entity) {

			ImGui::SetNextItemOpen(true);
			toBeOpened = NULL;
		}
		bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)&transform, node_flags, "");

		if (ImGui::IsItemHovered())
			anyEntityHovered = true;

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {

			if (!ImGui::IsItemToggledOpen()) {
				holdedEntity = transform.entity;
				coloredEntity = holdedEntity;
			}
		}

		ImGui::PushID(transform.entity);
		ImGui::SetNextWindowSize(ImVec2(210, 95));

		if (ImGui::BeginPopupContextItem("scene_graph_popup"))
		{
			selectedEntity = transform.entity;
			coloredEntity = transform.entity;

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 2.0f));

			if (ImGui::Selectable("   Create Empty")) {
				// entity classini kompile al ; ) transformu entity e ekle. do not betray fury !
				Entity* entity = CoreContext::instance->scene->newEntity("Entity", transform.entity);
				selectedEntity = entity;
				coloredEntity = entity;
				toBeOpened = entity->transform->parent->entity;
				popupItemClicked = true;

				ImGui::PopStyleColor();
				ImGui::EndPopup();
				ImGui::PopID();
				return;
			}

			ImVec2 p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
			ImGui::Dummy(ImVec2(0, 1));

			if (ImGui::Selectable("   Rename")) {

				renameEntity = transform.entity;

				ImGui::PopStyleColor();
				ImGui::EndPopup();
				ImGui::PopID();
				return;
			}

			if (ImGui::Selectable("   Duplicate")) {

				selectedEntity = CoreContext::instance->scene->duplicate(transform.entity);

				ImGui::PopStyleColor();
				ImGui::EndPopup();
				ImGui::PopID();
				return;
			}

			if (ImGui::Selectable("   Delete")) {

				delete selectedEntity;
				selectedEntity = NULL;

				ImGui::PopStyleColor();
				ImGui::EndPopup();
				ImGui::PopID();
				return;
			}

			ImGui::PopStyleColor();
			ImGui::EndPopup();
		}
		ImGui::PopID();

		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("_TREENODE_ENTITY", &transform.entity, sizeof(Entity*));
			ImGui::TextColored(DEFAULT_TEXT_COLOR, transform.entity->name.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_TREENODE_ENTITY"))
			{
				IM_ASSERT(payload->DataSize == sizeof(Entity*));
				Entity* payload_n = *(Entity**)payload->Data;
				if (transform.entity->attachEntity(payload_n)) {
					toBeOpened = transform.entity;
					return;
				}
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine();

		ImVec2 size = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);

		//ImGui::Image((ImTextureID)0, size, uv0, uv1, tint_col, border_col); //editorIcons.gameObjectTextureID

		//ImGui::SameLine();

		//if (fileSystemControlVars.lastSelectedItemID == transform->children[i]->id)
		//	editorColors.textColor = TEXT_SELECTED_COLOR;
		//else
		//	editorColors.textColor = editorColors.textUnselectedColor;

		ImVec2 pos = ImGui::GetCursorPos();

		if (renameEntity == transform.entity) {

			char name[32];
			strcpy(name, (char*)transform.entity->name.c_str());
			ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
			ImGui::SetKeyboardFocusHere(0);
			ImGui::SetCursorPos(ImVec2(pos.x - 5, pos.y - 2));
			int length = IM_ARRAYSIZE(name);
			if (ImGui::InputText("##0", name, length, input_text_flags)) {
				if (length != 0)
					transform.entity->rename(name);
				renameEntity = NULL;
			}
		}
		else {

			if (coloredEntity == transform.entity)
				ImGui::TextColored(TEXT_SELECTED_COLOR, transform.entity->name.c_str());
			else
				ImGui::TextColored(DEFAULT_TEXT_COLOR, transform.entity->name.c_str());
		}

		//for (Transform* transform : (&transform)->children) { // Bu sekilde neden hata veriyor???
		//
		//	ImGui::Indent(15);
		//	if(nodeOpen)
		//		Menu::createSceneGraphRecursively(*transform);
		//	ImGui::Unindent(15);
		//}

		for (int i = 0; i < transform.children.size(); i++) {
			ImGui::Indent(15);
			if (nodeOpen)
				Menu::createSceneGraphRecursively(*transform.children[i]);
			ImGui::Unindent(15);
		}
	}


	void Menu::showEntityName() {

		ImVec2 size = ImVec2(16.0f, 16.0f);
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);

		//ImGui::Image((ImTextureID)0, size, uv0, uv1, tint_col, border_col); //editorIcons.gameObjectTextureID
		//ImGui::SameLine();

		static bool active = true;
		ImGui::Checkbox("##0", &active);
		ImGui::SameLine();

		//int len = strlen(editor->scene->entities[lastSelectedEntityID].name);
		//char* str0 = new char[len + 1];
		//strcpy(str0, editor->scene->entities[lastSelectedEntityID].name);
		//str0[len] = '\0';
		//ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue;
		//ImVec2 textSize = ImGui::CalcTextSize(str0);

		//if (ImGui::InputText("##9", str0, IM_ARRAYSIZE(str0), input_text_flags)) {

		//	if (strlen(str0) != 0)
		//		editor->scene->renameEntity(editor->scene->entities[lastSelectedEntityID].transform->id, str0);
		//	renameEntityID = -1;
		//}
		//delete[] str0;
		//ImGui::Separator();

		char name[32];
		strcpy(name, selectedEntity->name.c_str());
		ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue;
		if (ImGui::InputText("##1", name, IM_ARRAYSIZE(name), input_text_flags)) {
			if (strlen(name) != 0)
				selectedEntity->rename(name);
			renameEntity = NULL;
		}

		ImGui::Separator();
	}

	void Menu::showTransformComponent(int index) {

		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemOpen(true);

		char str[4] = { '#', '#', '0', '\0' };
		char indexChar = '0' + index;
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
		ImGui::Image((ImTextureID)transformTextureId, size, uv0, uv1, tint_col, border_col);//transformTextureId
		ImGui::SameLine();
		ImGui::TextColored(DEFAULT_TEXT_COLOR, " Transform");

		ImGui::SameLine();
		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(width - 20, pos.y));

		ImGui::ImageButton((ImTextureID)contextMenuTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col);//contextMenuTextureId
		//if (ImGui::ImageButton((ImTextureID)0, size, uv0, uv1, frame_padding, bg_col, tint_col)) //editorIcons.contextMenuTextureID
			//ImGui::OpenPopup("context_menu_popup");

		//Menu::contextMenuPopup(ComponentType::Transform, 0);

		if (treeNodeOpen) {

			float itemWidth = (width - 150) * 0.33f;

			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 3));

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Position  X"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);

			if (ImGui::DragFloat("##0", &selectedEntity->transform->localPosition.x, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Y"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);

			if (ImGui::DragFloat("##1", &selectedEntity->transform->localPosition.y, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Z"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);

			if (ImGui::DragFloat("##2", &selectedEntity->transform->localPosition.z, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Rotation  X"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);

			if (ImGui::DragFloat("##3", &selectedEntity->transform->localRotation.x, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Y"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);

			if (ImGui::DragFloat("##4", &selectedEntity->transform->localRotation.y, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Z"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);

			if (ImGui::DragFloat("##5", &selectedEntity->transform->localRotation.z, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Scale     X"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);

			if (ImGui::DragFloat("##6", &selectedEntity->transform->localScale.x, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Y"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);

			if (ImGui::DragFloat("##7", &selectedEntity->transform->localScale.y, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::SameLine(); ImGui::TextColored(DEFAULT_TEXT_COLOR, "Z"); ImGui::SameLine(); ImGui::PushItemWidth(itemWidth);

			if (ImGui::DragFloat("##8", &selectedEntity->transform->localScale.z, 0.1f, 0.0f, 0.0f, "%.2f"))
				selectedEntity->transform->updateTransform();

			ImGui::TreePop();
		}

		ImGui::Separator();
	}

	void Menu::showMeshRendererComponent(int index, MeshRenderer* comp) {

		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemOpen(true);

		char str[4] = { '#', '#', '0', '\0' };
		char indexChar = '0' + index;
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
		ImGui::Image((ImTextureID)meshRendererTextureId, size, uv0, uv1, tint_col, border_col);//meshRendererTextureId
		ImGui::SameLine();
		ImGui::TextColored(DEFAULT_TEXT_COLOR, "  Mesh Renderer");

		ImGui::SameLine();
		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(width - 20, pos.y));

		if (ImGui::ImageButton((ImTextureID)contextMenuTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col))//contextMenuTextureId
			ImGui::OpenPopup("context_menu_popup");

		Menu::contextMenuPopup(comp, 0);

		if (treeNodeOpen) {

			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 3));

			std::map<std::string, Mesh*>& meshFiles = CoreContext::instance->fileSystem->meshes;
			std::map<std::string, Material*>& matFiles = CoreContext::instance->fileSystem->materials;
			int size_meshes = meshFiles.size() + 1;
			int size_mats = matFiles.size() + 1;

			const char** meshNames = new const char* [size_meshes];
			const char** matNames = new const char* [size_mats];

			std::vector<std::string> meshNameVec;
			std::vector<std::string> matNameVec;

			meshNameVec.push_back("Empty");
			matNameVec.push_back("Empty");

			/*meshNames[0] = "Empty";
			matNames[0] = "Empty";*/

			int meshIndex;
			int matIndex;

			if (comp->mesh == NULL)
				meshIndex = 0;

			if (comp->material == NULL)
				matIndex = 0;

			if (comp->mesh) {
				int i = 1;
				for (auto it : meshFiles) {
					if (it.second == comp->mesh)
						meshIndex = i;
					//meshNames[i] = it.first.c_str();
					meshNameVec.push_back(it.first);
					i++;
				}
			}
			else {
				int i = 1;
				for (auto it : meshFiles) {
					//meshNames[i] = it.first.c_str();
					meshNameVec.push_back(it.first);
					i++;
				}
			}

			if (comp->material) {
				int i = 1;
				for (auto it : matFiles) {
					if (it.second == comp->material)
						matIndex = i;
					//matNames[i] = it.first.c_str();
					matNameVec.push_back(it.first);
					i++;
				}
			}
			else {
				int i = 1;
				for (auto it : matFiles) {
					//matNames[i] = it.first.c_str();
					matNameVec.push_back(it.first);
					i++;
				}
			}

			for (int i = 0; i < size_meshes; i++)
				meshNames[i] = &meshNameVec[i][0];
			for (int i = 0; i < size_mats; i++)
				matNames[i] = &matNameVec[i][0];

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Mesh        ");
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
			ImGui::SetNextItemWidth(width - 115);

			if (ImGui::Combo("##0", &meshIndex, meshNames, size_meshes)) {

				if (meshIndex == 0)
					comp->setMesh(NULL);
				else {
					std::string meshName = meshNames[meshIndex];
					Mesh* mesh = CoreContext::instance->fileSystem->meshes.at(meshName);
					comp->setMesh(mesh);
				}
			}

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Material    ");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(width - 115);

			if (ImGui::Combo("##1", &matIndex, matNames, size_mats)) {

				if (matIndex == 0)
					comp->setMaterial(NULL);
				else {
					std::string matName = matNames[matIndex];
					Material* mat = CoreContext::instance->fileSystem->materials.at(matName);
					comp->setMaterial(mat);
				}
			}

			delete[] meshNames;
			delete[] matNames;

			ImGui::PopStyleColor();
			ImGui::TreePop();
		}

		ImGui::Separator();
	}

	void Menu::showTerrainComponent(int index, Terrain* comp) {

		float width = ImGui::GetContentRegionAvail().x;

		ImGui::SetNextItemOpen(true);

		char str[4] = { '#', '#', '0', '\0' };
		char indexChar = '0' + index;
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
		ImGui::Image((ImTextureID)meshRendererTextureId, size, uv0, uv1, tint_col, border_col);//meshRendererTextureId
		ImGui::SameLine();
		ImGui::TextColored(DEFAULT_TEXT_COLOR, "  Terrain");

		ImGui::SameLine();
		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(width - 20, pos.y));

		if (ImGui::ImageButton((ImTextureID)contextMenuTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col))//contextMenuTextureId
			ImGui::OpenPopup("context_menu_popup");

		Menu::contextMenuPopup(comp, 0);

		if (treeNodeOpen) {

			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 3));
			float itemWidth = (width - 180) * 0.33f;

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Sun Direction  X");
			ImGui::SameLine();
			ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##1", &comp->lightDir.x, 0.01f, -1.0f, 1.0f, "%.2f");
			ImGui::SameLine();
			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Y");
			ImGui::SameLine();
			ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##2", &comp->lightDir.y, 0.01f, -1.0f, 1.0f, "%.2f");
			ImGui::SameLine();
			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Z");
			ImGui::SameLine();
			ImGui::PushItemWidth(itemWidth);
			ImGui::DragFloat("##3", &comp->lightDir.z, 0.01f, -1.0f, 1.0f, "%.2f");

			ImVec4 color = ImVec4(comp->fogColor.r, comp->fogColor.g, comp->fogColor.b, 1);
			static bool alpha_preview = true;
			static bool alpha_half_preview = false;
			static bool drag_and_drop = true;
			static bool options_menu = true;
			static bool hdr = false;
			ImGuiColorEditFlags misc_flags = (hdr ? ImGuiColorEditFlags_HDR : 0) | (drag_and_drop ? 0 : ImGuiColorEditFlags_NoDragDrop) | (alpha_half_preview ? ImGuiColorEditFlags_AlphaPreviewHalf : (alpha_preview ? ImGuiColorEditFlags_AlphaPreview : 0)) | (options_menu ? 0 : ImGuiColorEditFlags_NoOptions);

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Fog Color"); ImGui::SameLine(width - 20);
			ImGui::ColorEdit4("MyColor##3", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | misc_flags);
			comp->fogColor.r = color.x;
			comp->fogColor.g = color.y;
			comp->fogColor.b = color.z;

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Ambient Amount"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##1112", &comp->ambientAmount, 0.01f, 0.0f, 3.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Specular Power"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##1110", &comp->specularPower, 0.01f, 0.0f, 32.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Specular Amount"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##1111", &comp->specularAmount, 0.01f, 0.0f, 3.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "distanceNear"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##11110", &comp->distanceNear, 1.f, 0.0f, 3000.0f, "%.0f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "fogBlendDistance"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##11111", &comp->fogBlendDistance, 1.f, 0.0f, 3000.0f, "%.0f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "maxFog"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##maxFog", &comp->maxFog, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "blendDistance0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##400", &comp->blendDistance0, 0.1f, 0.0f, 2000.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "blendAmount0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##401", &comp->blendAmount0, 0.1f, 0.0f, 1000.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "blendDistance1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##402", &comp->blendDistance1, 0.1f, 0.0f, 2000.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "blendAmount1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##403", &comp->blendAmount1, 0.1f, 0.0f, 1000.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color0_dist0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##6", &comp->scale_color0_dist0, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color0_dist1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##7", &comp->scale_color0_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color1_dist0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##9", &comp->scale_color1_dist0, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color1_dist1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##10", &comp->scale_color1_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color2_dist0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##110", &comp->scale_color2_dist0, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color2_dist1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##111", &comp->scale_color2_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color3_dist0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##113", &comp->scale_color3_dist0, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color3_dist1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##114", &comp->scale_color3_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color4_dist0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##116", &comp->scale_color4_dist0, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color4_dist1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##117", &comp->scale_color4_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color5_dist0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##119", &comp->scale_color5_dist0, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color5_dist1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##120", &comp->scale_color5_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color6_dist0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##122", &comp->scale_color6_dist0, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "scale_color6_dist1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##123", &comp->scale_color6_dist1, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Macro Scale 0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##12", &comp->macroScale_0, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Macro Scale 1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##13", &comp->macroScale_1, 0.001f, 0.0f, 1.0f, "%.3f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Macro Scale 2"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##14", &comp->macroScale_2, 0.0001f, 0.0f, 1.0f, "%.4f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Macro Amount"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##14111", &comp->macroAmount, 0.01f, 0.0f, 10.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Macro Power"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##14112", &comp->macroPower, 0.01f, 0.0f, 10.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Macro Opacity"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##14113", &comp->macroOpacity, 0.01f, 0.0f, 10.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Overlay Blend Scale 0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##15", &comp->overlayBlendScale0, 0.001f, 0.0f, 1.0f, "%.3f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Overlay Blend Amount 0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##16", &comp->overlayBlendAmount0, 0.01f, 0.0f, 10.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Overlay Blend Power 0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##17", &comp->overlayBlendPower0, 0.01f, 0.0f, 10.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Overlay Blend Opacity 0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##1117", &comp->overlayBlendOpacity0, 0.01f, 0.0f, 10.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Overlay Blend Scale 1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##111115", &comp->overlayBlendScale1, 0.001f, 0.0f, 10.0f, "%.3f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Overlay Blend Amount 1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##1118", &comp->overlayBlendAmount1, 0.01f, 0.0f, 10.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Overlay Blend Power 1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##1119", &comp->overlayBlendPower1, 0.01f, 0.0f, 20.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Overlay Blend Opacity 1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##1120", &comp->overlayBlendOpacity1, 0.01f, 0.0f, 10.0f, "%.2f");

			//--------

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Slope Bias 0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##21", &comp->slopeBias0, 0.001f, 0.0f, 1.0f, "%.3f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Slope Bias 1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##22", &comp->slopeBias1, 0.001f, 0.0f, 1.0f, "%.3f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Slope Bias 2"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##23", &comp->slopeBias2, 0.001f, 0.0f, 1.0f, "%.3f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Slope Sharpness 0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##24", &comp->slopeSharpness0, 0.001f, 0.0f, 1.0f, "%.3f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Slope Sharpness 1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##25", &comp->slopeSharpness1, 0.001f, 0.0f, 1.0f, "%.3f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Slope Sharpness 2"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##26", &comp->slopeSharpness2, 0.001f, 0.0f, 1.0f, "%.3f");

			//--------

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Height Bias 0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##240", &comp->heightBias0, 0.1f, 0.0f, 200.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Height Sharpness 0"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##250", &comp->heightSharpness0, 0.01f, 0.0f, 100.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Height Bias 1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##260", &comp->heightBias1, 0.1f, 0.0f, 200.0f, "%.2f");

			ImGui::TextColored(DEFAULT_TEXT_COLOR, "Height Sharpness 1"); ImGui::SameLine(width - 90); ImGui::PushItemWidth(90);
			ImGui::DragFloat("##270", &comp->heightSharpness1, 0.01f, 0.0f, 100.0f, "%.2f");

			ImGui::TreePop();
		}

		ImGui::Separator();
	}

	//void Menu::showGameCameraComponent(GameCamera* camComp, int index) {

	//	float width = ImGui::GetContentRegionAvail().x;

	//	ImGui::SetNextItemOpen(true);

	//	char str[4] = { '#', '#', '0', '\0' };
	//	char indexChar = '0' + index;
	//	str[2] = indexChar;
	//	bool treeNodeOpen = ImGui::TreeNode(str);

	//	int frame_padding = 1;
	//	ImVec2 size = ImVec2(16.0f, 16.0f);
	//	ImVec2 uv0 = ImVec2(0.0f, 0.0f);
	//	ImVec2 uv1 = ImVec2(1.0f, 1.0f);
	//	ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	//	ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
	//	ImVec4 bg_col = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

	//	ImGui::SameLine(25);
	//	ImGui::Image((ImTextureID)cameraTextureId, size, uv0, uv1, tint_col, border_col);
	//	ImGui::SameLine();
	//	ImGui::Text("  Game Camera");

	//	ImGui::SameLine();
	//	ImVec2 pos = ImGui::GetCursorPos();
	//	ImGui::SetCursorPos(ImVec2(width - 20, pos.y));

	//	if (ImGui::ImageButton((ImTextureID)contextMenuTextureId, size, uv0, uv1, frame_padding, bg_col, tint_col))
	//		ImGui::OpenPopup("context_menu_popup");

	//	Menu::contextMenuPopup(camComp, 0);

	//	//if (EditorGUI::contextMenuPopup(ComponentType::GameCamera, 0)) {

	//	//	ImGui::TreePop();
	//	//	return;
	//	//}

	//	if (treeNodeOpen) {

	//		ImVec2 pos = ImGui::GetCursorPos();
	//		ImGui::SetCursorPos(ImVec2(pos.x, pos.y + 3));

	//		const char** projectionTypes = new const char* [2];
	//		projectionTypes[0] = "Perspective";
	//		projectionTypes[1] = "Orthographic";

	//		const char** fovAxis = new const char* [2];
	//		fovAxis[0] = "Vertical";
	//		fovAxis[1] = "Horizontal";

	//		ImGui::Text("Projection    ");
	//		ImGui::SameLine();
	//		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
	//		ImGui::SetNextItemWidth(width - 130);

	//		ImGui::Combo("##0", &camComp->projectionType, projectionTypes, 2);
	//		/*if (ImGui::Combo("##0", &camComp->projectionType, projectionTypes, 2))
	//			camComp->createEditorGizmos(true);*/

	//		ImGui::Text("FOV Axis      ");
	//		ImGui::SameLine();
	//		ImGui::SetNextItemWidth(width - 130);
	//		int oldFovAxis = camComp->fovAxis;
	//		if (ImGui::Combo("##1", &camComp->fovAxis, fovAxis, 2))
	//			camComp->changeFovAxis(oldFovAxis);

	//		ImGui::Text("FOV           ");
	//		ImGui::SameLine();
	//		ImGui::SetNextItemWidth(width - 130);
	//		if (ImGui::DragFloat("##2", &camComp->fov, 1.f, 1.f, 179.f, "%.1f"))
	//			camComp->updateEditorGizmos();

	//		ImGui::Text("Near          ");
	//		ImGui::SameLine();
	//		ImGui::SetNextItemWidth(width - 130);
	//		if (ImGui::DragFloat("##3", &camComp->nearClip, 0.01f, 0.f, 10.f, "%.2f"))
	//			camComp->updateEditorGizmos();

	//		ImGui::Text("Far           ");
	//		ImGui::SameLine();
	//		ImGui::SetNextItemWidth(width - 130);
	//		if (ImGui::DragFloat("##4", &camComp->farClip, 1.f, 100.f, 10000.f, "%.2f"))
	//			camComp->updateEditorGizmos();

	//		delete[] projectionTypes;
	//		delete[] fovAxis;

	//		ImGui::PopStyleColor();
	//		ImGui::TreePop();
	//	}

	//	ImGui::Separator();
	//}


	void Menu::hiearchyCreateButton() {

		if (ImGui::Button("Create", ImVec2(60, 20)))
			ImGui::OpenPopup("context_menu_scene_hierarchy_popup");

		ImGui::SetNextWindowSize(ImVec2(210, 100));

		if (ImGui::BeginPopup("context_menu_scene_hierarchy_popup"))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 2.0f));

			if (ImGui::Selectable("   Entity")) {

				Entity* entity = CoreContext::instance->scene->newEntity("Entity", CoreContext::instance->scene->root);
				selectedEntity = entity;
				coloredEntity = entity;
				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				return;
			}

			ImVec2 p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
			ImGui::Dummy(ImVec2(0, 1));

			if (ImGui::Selectable("   Sun")) {

				//lastSelectedEntity = editor->scene->newLight(editor->scene->entities[0], "Sun", LightType::DirectionalLight);
				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				return;
			}

			if (ImGui::Selectable("   Point Light")) {

				//lastSelectedEntity = editor->scene->newLight(editor->scene->entities[0], "Light", LightType::PointLight);
				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				return;
			}

			p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
			ImGui::Dummy(ImVec2(0, 1));

			if (ImGui::Selectable("   Scene")) {

				//editor->addScene(Scene());

				// include all the necessary end codes...
				ImGui::PopStyleColor();
				ImGui::EndPopup();
				return;
			}

			ImGui::PopStyleColor();
			ImGui::EndPopup();
		}

	}

	//void Menu::createHierarchyPanel() {

	//	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
	//	ImGui::Begin("Hierarchy");


	//	ImGui::End();
	//	ImGui::PopStyleVar();
	//}

	void Menu::createScenePanel() {

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Scene");




		scenePos = ImGui::GetCursorScreenPos();
		ImVec2 content = ImGui::GetContentRegionAvail();

		SceneCamera* camera = EditorContext::instance->camera;

		ImGui::Image((ImTextureID)camera->textureBuffer, content, ImVec2(0, 1), ImVec2(1, 0));
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {

			scenePanelClicked = true;

			ImVec2 mousePos = ImGui::GetMousePos();
			float mX = mousePos.x < scenePos.x || mousePos.x > scenePos.x + sceneRect.x ? 0 : mousePos.x - scenePos.x;
			float mY = mousePos.y < scenePos.y || mousePos.y > scenePos.y + sceneRect.y ? 0 : mousePos.y - scenePos.y;

			if (mX != 0 && mY != 0) {
				//sceneClickEntity = CoreContext::instance->renderer->detectAndGetEntityId(mX, sceneRect.y - mY, camera->FBO, sceneRect.x, sceneRect.y,
				//	camera->projectionViewMatrix, camera->position, camera->planes);
			}
			//sceneClickEntity = Editor::instance->renderer->detectAndGetEntityId(mX, sceneRegion.y - mY);
		}

		if (content.x != sceneRect.x || content.y != sceneRect.y) {

			camera->width = content.x;
			camera->height = content.y;

			camera->aspectRatio = content.x / content.y;
			sceneRect = content;
			camera->recreateFBO((int)content.x, (int)content.y);
		}

		if (selectedEntity) {

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			float windowWidth = (float)ImGui::GetWindowWidth();
			float windowHeight = (float)ImGui::GetWindowHeight();

			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
			glm::mat4& model = selectedEntity->transform->model;

			ImGuizmo::Manipulate(glm::value_ptr(EditorContext::instance->camera->ViewMatrix), glm::value_ptr(EditorContext::instance->camera->ProjectionMatrix),
				optype, ImGuizmo::LOCAL, glm::value_ptr(model));

			if (ImGuizmo::IsUsing())
				selectedEntity->transform->updateTransformUsingGuizmo();
		}


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

	bool Menu::contextMenuPopup(Component* component, int index) {

		ImGui::SetNextWindowSize(ImVec2(180, 95));

		if (ImGui::BeginPopup("context_menu_popup"))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.20f, 0.20f, 2.0f));

			if (ImGui::Selectable("   Copy Values")) {

			}

			if (ImGui::Selectable("   Paste Values")) {

			}

			ImVec2 p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 192, p.y + 1), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
			ImGui::Dummy(ImVec2(0, 1));

			if (ImGui::Selectable("   Reset")) {

			}

			if (ImGui::Selectable("   Remove")) {

				//selectedEntity->removeComponent<Component>();
				//deleteBuffer = component;

				ImGui::PopStyleColor();
				ImGui::EndPopup();

				return true;
			}

			ImGui::PopStyleColor();
			ImGui::EndPopup();
		}

		return false;
	}


}