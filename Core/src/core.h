#pragma once

namespace Core {

	class __declspec(dllexport) Core {

	private:

	public:

		static Core* instance;
		
		Core();
		void init();
		void update(float dt);
		static Core* getInstance();

	};
}