#include "pch.h"
#include "core.h"

Core::Core* Core::Core::instance;

int main() {

	std::cout << "Application started." << std::endl;

	Core::Core::instance = Core::Core::getInstance();
	Core::Core::instance->init();


	return 0;
}