#pragma once

#include "component.h"

namespace Core {

	class __declspec(dllexport) Camera : public Component {

	private:

	public:

		Camera();
		~Camera();
		void start();
		void update(float dt);

	};
}