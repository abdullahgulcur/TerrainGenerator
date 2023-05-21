#include "pch.h"
#include "corecontext.h"
#include "entity.h"

namespace Core {

	// for the root
	Entity::Entity(std::string name) {

		this->name = name;
		transform = new Transform(this);
	}

	// when clicked new entity
	Entity::Entity(std::string name, Entity* parent) {

		this->name = name;
		transform = new Transform(this, parent->transform);
	}

	// duplicate
	Entity::Entity(Entity* entity, Entity* parent) {

		this->name = entity->name;
		transform = new Transform(this, entity->transform, parent->transform);

		for (auto& it : entity->components) {

			if (MeshRenderer* comp = dynamic_cast<MeshRenderer*>(it))
			{
				MeshRenderer* meshRendererComp = Entity::addComponent<MeshRenderer>();
			}
		}
	}

	Entity::~Entity() {

		for (Transform* transform : transform->children)
			delete transform->entity;

		for (Component* component : components)
			delete component;

		delete transform; // component destroy icinde olursa daha guzel gozukur ?
	}

	void Entity::rename(std::string name) {

		this->name = name;
	}

	bool Entity::hasChild(Entity* entity) {

		Transform* iter = entity->transform;

		while (iter->parent != NULL) {

			if (iter->parent == transform)
				return true;

			iter = iter->parent;
		}
		return false;
	}

	bool Entity::hasAnyChild() {

		if (transform->children.size() != 0)
			return true;

		return false;
	}

	bool Entity::attachEntity(Entity* entity) {

		if (entity->hasChild(this) || entity->transform->parent == transform)
			return false;

		entity->releaseFromParent();
		entity->transform->parent = transform;
		(transform->children).push_back(entity->transform);
		entity->transform->updateLocals();
		return true;
	}

	void Entity::releaseFromParent() {

		if (!transform->parent)
			return;

		for (int i = 0; i < transform->parent->children.size(); i++) {

			if (transform->parent->children[i] == transform) {
				transform->parent->children.erase(transform->parent->children.begin() + i);
				break;
			}
		}
	}

	void Entity::destroy() {

		Entity::releaseFromParent();
		//Entity::destroyRecursively();
		delete this;
	}

}