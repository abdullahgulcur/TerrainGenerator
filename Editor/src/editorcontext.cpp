#include "pch.h"
#include "editorcontext.h"
#include "scene.h"

using namespace Core;

namespace Editor {

	EditorContext::EditorContext() {

		std::cout << "Editor started." << std::endl;

		menu = new Menu();
		camera = new SceneCamera();
	}

	EditorContext::~EditorContext() {

		delete menu;
		delete camera;
		std::cout << "Editor shutdown." << std::endl;
	}

	void EditorContext::init() {

		menu->init();
		//camera->init(menu->sceneRect.x, menu->sceneRect.y);
	}

	void EditorContext::update(float dt) {

		menu->update();
		camera->update(dt);

		// scene update
		CameraInfo cameraInfo;
		cameraInfo.VP = camera->projectionViewMatrix;
		cameraInfo.projection = camera->ProjectionMatrix;
		cameraInfo.view = camera->ViewMatrix;
		cameraInfo.camPos = camera->position;
		for (int i = 0; i < 6; i++)
			cameraInfo.planes[i] = camera->planes[i];

		CoreContext::instance->scene->cameraInfo = cameraInfo;

		CoreContext::instance->glfwContext->end();
	}

	EditorContext* EditorContext::getInstance() {

		EditorContext::instance = new EditorContext;
		return instance;
	}
}