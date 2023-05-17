#include "pch.h"
#include "corecontext.h"
#include "entity.h"

namespace Core {

	Entity::Entity(std::string name) {
		
		this->name = name;
		transform = new Transform(this);
	}

	Entity::~Entity() {

		//for (Transform* transform : transform->children)
		//	delete transform->entity;

		//for (Component* component : components)
		//	delete component;

		//delete transform; // component destroy icinde olursa daha guzel gozukur ?
	}

	void Entity::rename(std::string name) {

		this->name = name;
	}

}