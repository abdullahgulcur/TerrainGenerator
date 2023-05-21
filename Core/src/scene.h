#pragma once

#include "rapidXML/rapidxml_print.hpp"
#include "rapidXML/rapidxml.hpp"

#include "GLM/glm.hpp"
#include "cubemap.h"

#include "component/meshrenderer.h"
#include "component/terrain.h"
#include "component/camera.h"
#include "component/transform.h"


namespace Core {

	class Entity;

	struct __declspec(dllexport) DirectionalLight {
		glm::vec3 direction;
		glm::vec3 color;
		float power;
	};

	struct CameraInfo {
		unsigned int FBO;
		glm::mat4 VP;
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec3 camPos;
		unsigned int width;
		unsigned int height;
		glm::vec4 planes[6];
	};

	// octree
	// node -> key: position val: (transform, type)

	class __declspec(dllexport) Scene {

	private:

	public:

		// no streaming
			// one octree 
			// custom structs for foliage
			// 
			// 
			// grass 0 transforms
			// grass 1 transforms
			// grass 2 transforms
			// 
			// bush 0 transforms
			// bush 1 transforms
			// bush 2 transforms
			//
			// tree 0 transforms
			// tree 1 transforms
			// tree 2 transforms

		// global volume
	    // light

		Cubemap* cubemap;

		DirectionalLight directionalLight;
		CameraInfo cameraInfo;

		//Entity* entity;

		Entity* root;


		Scene();
		~Scene();
		void start();
		void update(float dt);

		bool saveEntities(std::string filePath);
		void saveEntitiesRecursively(Transform* parent, rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entityNode);
		void loadEntities(std::string filePath);
		//void renameFile(std::string path, std::string name);
		void loadEntitiesRecursively(rapidxml::xml_node<>* parentNode, Entity* parent);
		bool saveTransformComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, Transform* transform);
		bool loadTransformComponent(Entity* ent, rapidxml::xml_node<>* entNode);
		bool saveMeshRendererComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, MeshRenderer* meshRenderer);
		bool loadMeshRendererComponent(Entity* ent, rapidxml::xml_node<>* entNode);
		//bool saveGameCameraComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, Camera* camera);
		//bool loadGameCameraComponent(Entity* ent, rapidxml::xml_node<>* entNode);
		bool saveTerrainComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, Terrain* terrain);
		bool loadTerrainComponent(Entity* ent, rapidxml::xml_node<>* entNode);
		//bool saveParticleSystemComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, ParticleSystem* particleSystem);
		//bool loadParticleSystemComponent(Entity* ent, rapidxml::xml_node<>* entNode);

		Entity* newEntity(std::string name);
		Entity* newEntity(std::string name, Entity* parent);
		Entity* newEntity(Entity* entity, Entity* parent);
		Entity* duplicate(Entity* entity);
		void cloneRecursively(Entity* copied, Entity* parent);
	};
}