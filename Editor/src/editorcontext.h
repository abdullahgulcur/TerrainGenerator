#pragma once

#include "editorcontext.h"
#include "menu.h"
#include "scenecamera.h"

//using namespace Core;

namespace Editor {

	class EditorContext {

	private:


	public:

		Menu* menu = NULL;
		SceneCamera* camera = NULL;

		static EditorContext* instance;

		EditorContext();
		~EditorContext();
		void init();
		void update(float dt);
		static EditorContext* getInstance();
	};
}