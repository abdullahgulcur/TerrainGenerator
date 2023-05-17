#pragma once

#include "component.h"

namespace Core {

	class __declspec(dllexport) MeshRenderer : public Component {

	private:

	public:

		MeshRenderer();
		~MeshRenderer();
		void start();
		void update(float dt);
		
	};
}