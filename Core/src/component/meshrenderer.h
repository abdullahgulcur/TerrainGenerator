#pragma once

#include "component.h"
#include "material.h"
#include "mesh.h"

namespace Core {

	class Entity;

	class __declspec(dllexport) MeshRenderer : public Component {

	private:

	public:

		Mesh* mesh = NULL;
		Material* material = NULL;
		Entity* entity = NULL;

		MeshRenderer();
		~MeshRenderer();
		void start();
		void update(float dt);
		void setMesh(Mesh* mesh);
		void setMaterial(Material* material);
		
	};
}