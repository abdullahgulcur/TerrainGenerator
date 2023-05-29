#pragma once

#include "glfwcontext.h"
#include "glewcontext.h"
#include "filesystem.h"
#include "scene.h"
#include "renderer.h"

namespace Core {

	class __declspec(dllexport) CoreContext {

	private:

	public:

		static CoreContext* instance;
		
		GlfwContext* glfwContext = NULL;
		GlewContext* glewContext = NULL;
		FileSystem* fileSystem = NULL;
		Renderer* renderer = NULL;

		Scene* scene = NULL;

		CoreContext();
		~CoreContext();
		void init();
		void update(float dt);
		static CoreContext* getInstance();

	};
}