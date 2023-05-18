#pragma once

#include "component.h"

namespace Core {

	class __declspec(dllexport) MeshRenderer : public Component {

	private:

	public:

		unsigned int shaderProgramId;
		unsigned int VAO_lod0;
		unsigned int VAO_lod1;
		unsigned int VAO_lod2;

		MeshRenderer();
		~MeshRenderer();
		void start();
		void update(float dt);
		
	};
}