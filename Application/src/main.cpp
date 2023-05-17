#include "pch.h"
#include "corecontext.h"
#include "editorcontext.h"
#include "appcontext.h"

using namespace Application;
using namespace Core;
using namespace Editor;

EditorContext* EditorContext::instance;
CoreContext* CoreContext::instance;

int main() {

	std::cout << "Program started." << std::endl;

	CoreContext::instance = CoreContext::getInstance();
	CoreContext::instance->init();

	AppContext* app = new AppContext;
	app->start();

	EditorContext::instance = EditorContext::getInstance();
	EditorContext::instance->init();

	float time = 0;
	do {
		float currentTime = (float)glfwGetTime();
		float dt = currentTime - time;
		time = currentTime;

		app->update(dt);
		CoreContext::instance->update(dt);
		EditorContext::instance->update(dt);

	} while (CoreContext::instance->glfwContext->getOpen());

	delete EditorContext::instance;
	delete app;
	delete CoreContext::instance;

	return 0;
}