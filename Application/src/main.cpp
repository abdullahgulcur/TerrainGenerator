#include "pch.h"
#include "corecontext.h"
#include "editorcontext.h"

using namespace Core;
using namespace Editor;

EditorContext* EditorContext::instance;
CoreContext* CoreContext::instance;

int main() {

	std::cout << "Program started." << std::endl;

	CoreContext::instance = CoreContext::getInstance();
	CoreContext::instance->init();

	EditorContext::instance = EditorContext::getInstance();
	EditorContext::instance->init();

	float time = 0;
	do {
		float currentTime = (float)glfwGetTime();
		float dt = currentTime - time;
		time = currentTime;

		CoreContext::instance->update(dt);
		EditorContext::instance->update(dt);

	} while (CoreContext::instance->glfwContext->getOpen());

	delete EditorContext::instance;
	delete CoreContext::instance;

	return 0;
}