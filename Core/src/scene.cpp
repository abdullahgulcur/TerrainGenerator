#include "pch.h"
#include "scene.h"
#include "entity.h"
#include "terrain.h"

namespace Core {

	Scene::Scene() {
	}

	Scene::~Scene() {

	}

	void Scene::start() {

		Terrain* terrain = entity->getComponent<Terrain>();
		terrain->start();
	}

	void Scene::update(float dt) {

		Terrain* terrain = entity->getComponent<Terrain>();
		terrain->update(cameraInfo.camPos, dt);
	}


}