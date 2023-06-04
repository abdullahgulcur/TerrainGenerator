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

	int counter = 0;
	float deltaTotal = 0;

	float time = 0;
	do {
		float currentTime = (float)glfwGetTime();
		float dt = currentTime - time;
		time = currentTime;

		float coreStart = (float)glfwGetTime();
		CoreContext::instance->update(dt);
		float coreEnd = (float)glfwGetTime();
		float delta = coreEnd - coreStart;

		EditorContext::instance->update(dt);

		deltaTotal += delta;
		counter++;

	} while (CoreContext::instance->glfwContext->getOpen());

	delete EditorContext::instance;
	delete CoreContext::instance;

	std::cout << "Core average fps: " << counter / deltaTotal << std::endl;

	return 0;
}