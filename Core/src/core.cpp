#include "pch.h"
#include "core.h"

namespace Core {

	Core* Core::instance;

	Core::Core() {

		std::cout << "Core module started." << std::endl;
	}

	void Core::init() {

	}

	void Core::update(float dt) {

	}


	Core* Core::getInstance() {

		Core::instance = new Core;
		return instance;
	}
}
