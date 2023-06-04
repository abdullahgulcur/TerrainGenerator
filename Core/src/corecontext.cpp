#include "pch.h"
#include "corecontext.h"

namespace Core {

	CoreContext* CoreContext::instance;

	CoreContext::CoreContext() {

		std::cout << "Core module started." << std::endl;

		glfwContext = new GlfwContext();
		glewContext = new GlewContext();
		fileSystem = new FileSystem();
		renderer = new Renderer();
		scene = new Scene();
	}

	CoreContext::~CoreContext() {

		delete glfwContext;
		delete glewContext;
		delete fileSystem;
		delete renderer;
		delete scene;

		std::cout << "Core module shutdown." << std::endl;
	}

	void CoreContext::init() {

		renderer->init();
		fileSystem->init();

		scene->start();
	}

	void CoreContext::update(float dt) {

		scene->update(dt);
		renderer->update(dt);
	}


	CoreContext* CoreContext::getInstance() {

		CoreContext::instance = new CoreContext;
		return instance;
	}
}
