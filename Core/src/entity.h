#pragma once

#include "component/transform.h"
#include "component/component.h"
//#include "meshrenderer.h"
//#include "camera.h"

namespace Core {

	class __declspec(dllexport) Entity {

	private:

		//void updateItselfAndChildrenTransforms();
		void cloneRecursively(Entity* copied, Entity* base);
		void releaseFromParent();
		//void destroyEntity();
		//void destroyRecursively();

	public:

		unsigned int id;
		std::string name;

		Transform* transform;
		std::vector<Component*> components;

		Entity(std::string name);//, unsigned int id
		Entity(std::string name, Entity* parent);//, unsigned int id
		Entity(Entity* entity, Entity* parent);//, unsigned int id
		~Entity();
		bool hasAnyChild();
		bool hasChild(Entity* entity);
		void rename(std::string name);
		bool attachEntity(Entity* entity);
		//Entity* duplicate();
		void destroy();

		template <class T>
		T* addComponent() {

			T* comp = getComponent<T>();

			if (comp != nullptr) {

				//Collider* colliderComp = dynamic_cast<Collider*>(comp);

				//if (!colliderComp) {

				//	std::cout << "There is existing component in the same type!" << std::endl;
				//	return nullptr;
				//}
				return nullptr;
			}

			T* newcomp = new T;//this
			Component* compToAdd = dynamic_cast<Component*>(newcomp);
			//compToAdd->transform = transform;
			components.push_back(compToAdd);

			return newcomp;
		}

		template <class T>
		T* getComponent() {

			for (auto& it : components) {

				T* comp = dynamic_cast<T*>(it);

				if (comp != nullptr)
					return comp;
			}
			return nullptr;
		}

		template <class T>
		void removeComponent() {

			for (auto it = components.begin(); it < components.end(); it++) {

				T* comp = dynamic_cast<T*>(*it);

				if (comp != nullptr) {

					delete* it;
					components.erase(it);
					return;
				}
			}
		}

		template <class T>
		bool removeComponent(T* component) {

			for (auto it = components.begin(); it < components.end(); it++) {

				T* comp = dynamic_cast<T*>(*it);

				if (comp == component) {

					delete* it;
					components.erase(it);
					return true;
				}
			}
			return false;
		}

	};
}