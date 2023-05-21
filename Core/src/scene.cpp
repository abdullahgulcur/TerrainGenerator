#include "pch.h"
#include "scene.h"
#include "corecontext.h"
#include "entity.h"
#include "component/terrain.h"

namespace Core {

	Scene::Scene() {

	}

	Scene::~Scene() {

	}

	void Scene::start() {

		Scene::newEntity("Root");
		cubemap = CoreContext::instance->fileSystem->cubemaps.at("sky");
		Scene::loadEntities("resources/scenes/samplescene.xml");
	}

	void Scene::update(float dt) {

		std::stack<Entity*> entStack;
		entStack.push(root);

		while (!entStack.empty()) {

			Entity* popped = entStack.top();
			entStack.pop();

			Terrain* terrain = popped->getComponent<Terrain>();
			if (terrain != NULL)
				terrain->update(cameraInfo.camPos, dt);

			//ParticleSystem* particleSystem = popped->getComponent<ParticleSystem>();
			//if (particleSystem != NULL)
			//	particleSystem->update(dt);

			for (Transform*& child : popped->transform->children)
				entStack.push(child->entity);
		}

	}

	bool Scene::saveEntities(std::string filePath) {

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		doc.append_node(decl);

		rapidxml::xml_node<>* SceneNode = doc.allocate_node(rapidxml::node_element, "Scene");
		SceneNode->append_attribute(doc.allocate_attribute("Name", doc.allocate_string("MainScene")));
		doc.append_node(SceneNode);

		rapidxml::xml_node<>* entitiesNode = doc.allocate_node(rapidxml::node_element, "Entities");
		SceneNode->append_node(entitiesNode);

		Transform* root = CoreContext::instance->scene->root->transform;
		for (Transform* child : root->children)
			Scene::saveEntitiesRecursively(child, doc, entitiesNode);

		std::string xml_as_string;
		rapidxml::print(std::back_inserter(xml_as_string), doc);

		std::ofstream file_stored(filePath);
		file_stored << doc;
		file_stored.close();
		doc.clear();
		return true;
	}

	void Scene::saveEntitiesRecursively(Transform* transform, rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entityNode) {

		rapidxml::xml_node<>* entity = doc.allocate_node(rapidxml::node_element, "Entity");
		entity->append_attribute(doc.allocate_attribute("Name", doc.allocate_string(transform->entity->name.c_str())));
		//entity->append_attribute(doc.allocate_attribute("Id", doc.allocate_string(std::to_string(transform->entity->id).c_str())));

		Scene::saveTransformComponent(doc, entity, transform);

		//if (Light* lightComp = child->entity->getComponent<Light>())
		//	saveLightComponent(doc, entity, lightComp);

		if (MeshRenderer* meshRendererComp = transform->entity->getComponent<MeshRenderer>())
			saveMeshRendererComponent(doc, entity, meshRendererComp);

		if (Terrain* terrainComp = transform->entity->getComponent<Terrain>())
			Scene::saveTerrainComponent(doc, entity, terrainComp);

		//if (Rigidbody* rigidbodyComp = child->entity->getComponent<Rigidbody>())
		//	saveRigidbodyComponent(doc, entity, rigidbodyComp);

		//if (GameCamera* cameraComp = transform->entity->getComponent<GameCamera>())
		//	saveGameCameraComponent(doc, entity, cameraComp);

		//if (ParticleSystem* particleSystemComp = transform->entity->getComponent<ParticleSystem>())
		//	saveParticleSystemComponent(doc, entity, particleSystemComp);

		//for (auto& comp : child->entity->getComponents<BoxCollider>())
		//	saveBoxColliderComponent(doc, entity, comp);

		//for (auto& comp : child->entity->getComponents<SphereCollider>())
		//	saveSphereColliderComponent(doc, entity, comp);

		//for (auto& comp : child->entity->getComponents<CapsuleCollider>())
		//	saveCapsuleColliderComponent(doc, entity, comp);

		//for (auto& comp : child->entity->getComponents<MeshCollider>())
		//	saveMeshColliderComponent(doc, entity, comp);

		entityNode->append_node(entity);

		for (Transform* child : transform->children)
			Scene::saveEntitiesRecursively(child, doc, entity);
	}

	void Scene::loadEntities(std::string filePath) {

		std::ifstream file(filePath);

		if (file.fail()) {

			Scene::saveEntities(filePath);
			return;
		}

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* scene_node = NULL;
		rapidxml::xml_node<>* entities_node = NULL;

		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		buffer.push_back('\0');

		doc.parse<0>(&buffer[0]);

		scene_node = doc.first_node("Scene");
		//CoreContext::instance->scene->name = scene_node->first_attribute("Name")->value();
		//CoreContext::instance->scene->name = 
		entities_node = scene_node->first_node("Entities");

		Scene::loadEntitiesRecursively(entities_node, CoreContext::instance->scene->root);

		file.close();
	}
//
//	void Scene::renameFile(std::string path, std::string name) {
//
//		rapidxml::xml_document<> doc;
//		rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
//		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
//		decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
//		doc.append_node(decl);
//
//		rapidxml::xml_node<>* sceneNode = doc.allocate_node(rapidxml::node_element, "Scene");
//		sceneNode->append_attribute(doc.allocate_attribute("Name", name.c_str()));
//		doc.append_node(sceneNode);
//
//		rapidxml::xml_node<>* entitiesNode = doc.allocate_node(rapidxml::node_element, "Entities");
//		sceneNode->append_node(entitiesNode);
//
//		if (Scene* currentScene = Core::instance->sceneManager->currentScene) {
//			Transform* root = currentScene->root->transform;
//			for (Transform* child : root->children)
//				Scene::saveEntitiesRecursively(child, doc, entitiesNode);
//		}
//
//		std::string xml_as_string;
//		rapidxml::print(std::back_inserter(xml_as_string), doc);
//
//		std::ofstream file_stored(path);
//		file_stored << doc;
//		file_stored.close();
//		doc.clear();
//	}

	void Scene::loadEntitiesRecursively(rapidxml::xml_node<>* parentNode, Entity* parent) {

		for (rapidxml::xml_node<>* entityNode = parentNode->first_node("Entity"); entityNode; entityNode = entityNode->next_sibling()) {

			//Entity* ent = core->scene->newEntity(entityNode->first_attribute("Name")->value(), parent,
			//	atoi(entityNode->first_attribute("Id")->value()));
			///unsigned int id = atoi(entityNode->first_attribute("Id")->value());
			Entity* ent = new Entity(entityNode->first_attribute("Name")->value(), parent);//, id
			//CoreContext::instance->scene->entityIdToEntity.insert(std::pair<unsigned int, Entity*>(id, ent));

			//if (id >= Core::instance->sceneManager->currentScene->idCounter)
			//	Core::instance->sceneManager->currentScene->idCounter = id + 1;

			Scene::loadTransformComponent(ent, entityNode);
			//Scene::loadLightComponent(ent, entityNode);
			Scene::loadMeshRendererComponent(ent, entityNode);
			Scene::loadTerrainComponent(ent, entityNode);
			//Scene::loadBoxColliderComponents(ent, entityNode);
			//Scene::loadSphereColliderComponents(ent, entityNode);
			//Scene::loadCapsuleColliderComponents(ent, entityNode);
			//Scene::loadMeshColliderComponents(ent, entityNode);
			//Scene::loadRigidbodyComponent(ent, entityNode);
			//Scene::loadGameCameraComponent(ent, entityNode);
			//Scene::loadParticleSystemComponent(ent, entityNode);

			Scene::loadEntitiesRecursively(entityNode, ent);
		}
	}

	bool Scene::saveMeshRendererComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, MeshRenderer* meshRenderer) {

		rapidxml::xml_node<>* meshRendererNode = doc.allocate_node(rapidxml::node_element, "MeshRenderer");

		Mesh* mesh = meshRenderer->mesh;
		if (mesh == NULL)
			meshRendererNode->append_attribute(doc.allocate_attribute("Mesh", doc.allocate_string("Empty")));
		else {
			std::map<std::string, Mesh*>& meshFiles = CoreContext::instance->fileSystem->meshes;
			for (auto it : meshFiles) {
				if (it.second == mesh) {
					meshRendererNode->append_attribute(doc.allocate_attribute("Mesh", doc.allocate_string(it.first.c_str())));
					break;
				}
			}
		}
		
		Material* mat = meshRenderer->material;
		if (mat == NULL)
			meshRendererNode->append_attribute(doc.allocate_attribute("Material", doc.allocate_string("Empty")));
		else {
			std::map<std::string, Material*>& matFiles = CoreContext::instance->fileSystem->materials;
			for (auto it : matFiles) {
				if (it.second == mat) {
					meshRendererNode->append_attribute(doc.allocate_attribute("Material", doc.allocate_string(it.first.c_str())));
					break;
				}
			}
		}
		
		entNode->append_node(meshRendererNode);
		return true;
	}

	bool Scene::loadMeshRendererComponent(Entity* ent, rapidxml::xml_node<>* entNode) {

		FileSystem* fileSystem = CoreContext::instance->fileSystem;
		rapidxml::xml_node<>* meshRendererNode = entNode->first_node("MeshRenderer");

		if (meshRendererNode == NULL)
			return false;

		MeshRenderer* meshRendererComp = ent->addComponent<MeshRenderer>();
		meshRendererComp->entity = ent; ////

		std::string meshName = meshRendererNode->first_attribute("Mesh")->value();
		if (meshName == "Empty")
			meshRendererComp->setMesh(NULL);
		else {
			Mesh* mesh = CoreContext::instance->fileSystem->meshes.at(meshName);
			meshRendererComp->setMesh(mesh);
		}

		std::string matName = meshRendererNode->first_attribute("Material")->value();
		if (matName == "Empty")
			meshRendererComp->setMaterial(NULL);
		else {
			Material* mat = CoreContext::instance->fileSystem->materials.at(matName);
			meshRendererComp->setMaterial(mat);
		}

		return true;
	}

	bool Scene::saveTransformComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, Transform* transform) {

		rapidxml::xml_node<>* transformNode = doc.allocate_node(rapidxml::node_element, "Transform");

		rapidxml::xml_node<>* positionNode = doc.allocate_node(rapidxml::node_element, "Position");
		positionNode->append_attribute(doc.allocate_attribute("X", doc.allocate_string(std::to_string(transform->localPosition.x).c_str())));
		positionNode->append_attribute(doc.allocate_attribute("Y", doc.allocate_string(std::to_string(transform->localPosition.y).c_str())));
		positionNode->append_attribute(doc.allocate_attribute("Z", doc.allocate_string(std::to_string(transform->localPosition.z).c_str())));
		transformNode->append_node(positionNode);

		rapidxml::xml_node<>* rotationNode = doc.allocate_node(rapidxml::node_element, "Rotation");
		rotationNode->append_attribute(doc.allocate_attribute("X", doc.allocate_string(std::to_string(transform->localRotation.x).c_str())));
		rotationNode->append_attribute(doc.allocate_attribute("Y", doc.allocate_string(std::to_string(transform->localRotation.y).c_str())));
		rotationNode->append_attribute(doc.allocate_attribute("Z", doc.allocate_string(std::to_string(transform->localRotation.z).c_str())));
		transformNode->append_node(rotationNode);

		rapidxml::xml_node<>* scaleNode = doc.allocate_node(rapidxml::node_element, "Scale");
		scaleNode->append_attribute(doc.allocate_attribute("X", doc.allocate_string(std::to_string(transform->localScale.x).c_str())));
		scaleNode->append_attribute(doc.allocate_attribute("Y", doc.allocate_string(std::to_string(transform->localScale.y).c_str())));
		scaleNode->append_attribute(doc.allocate_attribute("Z", doc.allocate_string(std::to_string(transform->localScale.z).c_str())));
		transformNode->append_node(scaleNode);

		entNode->append_node(transformNode);

		return true;
	}

	bool Scene::loadTransformComponent(Entity* ent, rapidxml::xml_node<>* entNode) {

		rapidxml::xml_node<>* transform_node = entNode->first_node("Transform");

		ent->transform->localPosition.x = atof(transform_node->first_node("Position")->first_attribute("X")->value());
		ent->transform->localPosition.y = atof(transform_node->first_node("Position")->first_attribute("Y")->value());
		ent->transform->localPosition.z = atof(transform_node->first_node("Position")->first_attribute("Z")->value());

		ent->transform->localRotation.x = atof(transform_node->first_node("Rotation")->first_attribute("X")->value());
		ent->transform->localRotation.y = atof(transform_node->first_node("Rotation")->first_attribute("Y")->value());
		ent->transform->localRotation.z = atof(transform_node->first_node("Rotation")->first_attribute("Z")->value());

		ent->transform->localScale.x = atof(transform_node->first_node("Scale")->first_attribute("X")->value());
		ent->transform->localScale.y = atof(transform_node->first_node("Scale")->first_attribute("Y")->value());
		ent->transform->localScale.z = atof(transform_node->first_node("Scale")->first_attribute("Z")->value());

		ent->transform->updateTransform();

		return true;
	}
//
//	bool Scene::saveGameCameraComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, GameCamera* camera) {
//
//		rapidxml::xml_node<>* camNode = doc.allocate_node(rapidxml::node_element, "GameCamera");
//		camNode->append_attribute(doc.allocate_attribute("Projection", doc.allocate_string(std::to_string(camera->projectionType).c_str())));
//		camNode->append_attribute(doc.allocate_attribute("FovAxis", doc.allocate_string(std::to_string(camera->fovAxis).c_str())));
//		camNode->append_attribute(doc.allocate_attribute("Near", doc.allocate_string(std::to_string(camera->nearClip).c_str())));
//		camNode->append_attribute(doc.allocate_attribute("Far", doc.allocate_string(std::to_string(camera->farClip).c_str())));
//		camNode->append_attribute(doc.allocate_attribute("FOV", doc.allocate_string(std::to_string(camera->fov).c_str())));
//		camNode->append_attribute(doc.allocate_attribute("Width", doc.allocate_string(std::to_string(camera->width).c_str())));
//		camNode->append_attribute(doc.allocate_attribute("Height", doc.allocate_string(std::to_string(camera->height).c_str())));
//		entNode->append_node(camNode);
//
//		return true;
//	}
//
//	bool Scene::loadGameCameraComponent(Entity* ent, rapidxml::xml_node<>* entNode) {
//
//		rapidxml::xml_node<>* cameraNode = entNode->first_node("GameCamera");
//
//		if (cameraNode == NULL)
//			return false;
//
//		GameCamera* cameraComp = ent->addComponent<GameCamera>();
//		cameraComp->projectionType = atoi(cameraNode->first_attribute("Projection")->value());
//		cameraComp->fovAxis = atoi(cameraNode->first_attribute("FovAxis")->value());
//		cameraComp->nearClip = atof(cameraNode->first_attribute("Near")->value());
//		cameraComp->farClip = atof(cameraNode->first_attribute("Far")->value());
//		cameraComp->fov = atof(cameraNode->first_attribute("FOV")->value());
//		cameraComp->width = atoi(cameraNode->first_attribute("Width")->value());
//		cameraComp->height = atoi(cameraNode->first_attribute("Height")->value());
//		Core::instance->sceneManager->currentScene->primaryCamera = cameraComp;
//
//#ifdef EDITOR_MODE
//		cameraComp->init(ent->transform); // burasi degisecek, editor icin calisiyor sadece. game tarafi ? henuz netlik yok... , 1024, 768
//#else
//		cameraComp->width = Core::instance->glfwContext->mode->width;
//		cameraComp->height = Core::instance->glfwContext->mode->height;
//		cameraComp->init(ent->transform);
//#endif
//
//		return true;
//	}

	bool Scene::saveTerrainComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, Terrain* terrin) {

		rapidxml::xml_node<>* terrainNode = doc.allocate_node(rapidxml::node_element, "Terrain");

		rapidxml::xml_node<>* fogNode = doc.allocate_node(rapidxml::node_element, "Fog");
		fogNode->append_attribute(doc.allocate_attribute("maxFog", doc.allocate_string(std::to_string(terrin->maxFog).c_str())));
		fogNode->append_attribute(doc.allocate_attribute("color_r", doc.allocate_string(std::to_string(terrin->fogColor.r).c_str())));
		fogNode->append_attribute(doc.allocate_attribute("color_g", doc.allocate_string(std::to_string(terrin->fogColor.g).c_str())));
		fogNode->append_attribute(doc.allocate_attribute("color_b", doc.allocate_string(std::to_string(terrin->fogColor.b).c_str())));
		fogNode->append_attribute(doc.allocate_attribute("distanceNear", doc.allocate_string(std::to_string(terrin->distanceNear).c_str())));
		fogNode->append_attribute(doc.allocate_attribute("fogBlendDistance", doc.allocate_string(std::to_string(terrin->fogBlendDistance).c_str())));
		terrainNode->append_node(fogNode);

		rapidxml::xml_node<>* ambientNode = doc.allocate_node(rapidxml::node_element, "Ambient");
		ambientNode->append_attribute(doc.allocate_attribute("ambientAmount", doc.allocate_string(std::to_string(terrin->ambientAmount).c_str())));
		ambientNode->append_attribute(doc.allocate_attribute("specularPower", doc.allocate_string(std::to_string(terrin->specularPower).c_str())));
		ambientNode->append_attribute(doc.allocate_attribute("specularAmount", doc.allocate_string(std::to_string(terrin->specularAmount).c_str())));
		terrainNode->append_node(ambientNode);

		rapidxml::xml_node<>* distanceBlendNode = doc.allocate_node(rapidxml::node_element, "DistanceBlend");
		distanceBlendNode->append_attribute(doc.allocate_attribute("blendDistance0", doc.allocate_string(std::to_string(terrin->blendDistance0).c_str())));
		distanceBlendNode->append_attribute(doc.allocate_attribute("blendAmount0", doc.allocate_string(std::to_string(terrin->blendAmount0).c_str())));
		distanceBlendNode->append_attribute(doc.allocate_attribute("blendDistance1", doc.allocate_string(std::to_string(terrin->blendDistance1).c_str())));
		distanceBlendNode->append_attribute(doc.allocate_attribute("blendAmount1", doc.allocate_string(std::to_string(terrin->blendAmount1).c_str())));
		terrainNode->append_node(distanceBlendNode);

		rapidxml::xml_node<>* colorScaleNode = doc.allocate_node(rapidxml::node_element, "ColorScale");
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color0_dist0", doc.allocate_string(std::to_string(terrin->scale_color0_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color0_dist1", doc.allocate_string(std::to_string(terrin->scale_color0_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color0_dist2", doc.allocate_string(std::to_string(terrin->scale_color0_dist2).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color1_dist0", doc.allocate_string(std::to_string(terrin->scale_color1_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color1_dist1", doc.allocate_string(std::to_string(terrin->scale_color1_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color1_dist2", doc.allocate_string(std::to_string(terrin->scale_color1_dist2).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color2_dist0", doc.allocate_string(std::to_string(terrin->scale_color2_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color2_dist1", doc.allocate_string(std::to_string(terrin->scale_color2_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color2_dist2", doc.allocate_string(std::to_string(terrin->scale_color2_dist2).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color3_dist0", doc.allocate_string(std::to_string(terrin->scale_color3_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color3_dist1", doc.allocate_string(std::to_string(terrin->scale_color3_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color3_dist2", doc.allocate_string(std::to_string(terrin->scale_color3_dist2).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color4_dist0", doc.allocate_string(std::to_string(terrin->scale_color4_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color4_dist1", doc.allocate_string(std::to_string(terrin->scale_color4_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color4_dist2", doc.allocate_string(std::to_string(terrin->scale_color4_dist2).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color5_dist0", doc.allocate_string(std::to_string(terrin->scale_color5_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color5_dist1", doc.allocate_string(std::to_string(terrin->scale_color5_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color5_dist2", doc.allocate_string(std::to_string(terrin->scale_color5_dist2).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color6_dist0", doc.allocate_string(std::to_string(terrin->scale_color6_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color6_dist1", doc.allocate_string(std::to_string(terrin->scale_color6_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color6_dist2", doc.allocate_string(std::to_string(terrin->scale_color6_dist2).c_str())));
		terrainNode->append_node(colorScaleNode);

		rapidxml::xml_node<>* macroNode = doc.allocate_node(rapidxml::node_element, "Macro");
		macroNode->append_attribute(doc.allocate_attribute("macroScale_0", doc.allocate_string(std::to_string(terrin->macroScale_0).c_str())));
		macroNode->append_attribute(doc.allocate_attribute("macroScale_1", doc.allocate_string(std::to_string(terrin->macroScale_1).c_str())));
		macroNode->append_attribute(doc.allocate_attribute("macroScale_2", doc.allocate_string(std::to_string(terrin->macroScale_2).c_str())));
		macroNode->append_attribute(doc.allocate_attribute("macroPower", doc.allocate_string(std::to_string(terrin->macroPower).c_str())));
		macroNode->append_attribute(doc.allocate_attribute("macroAmount", doc.allocate_string(std::to_string(terrin->macroAmount).c_str())));
		macroNode->append_attribute(doc.allocate_attribute("macroOpacity", doc.allocate_string(std::to_string(terrin->macroOpacity).c_str())));
		terrainNode->append_node(macroNode);

		rapidxml::xml_node<>* overlayNode = doc.allocate_node(rapidxml::node_element, "Overlay");
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendScale0", doc.allocate_string(std::to_string(terrin->overlayBlendScale0).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendAmount0", doc.allocate_string(std::to_string(terrin->overlayBlendAmount0).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendPower0", doc.allocate_string(std::to_string(terrin->overlayBlendPower0).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendOpacity0", doc.allocate_string(std::to_string(terrin->overlayBlendOpacity0).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendScale1", doc.allocate_string(std::to_string(terrin->overlayBlendScale1).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendAmount1", doc.allocate_string(std::to_string(terrin->overlayBlendAmount1).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendPower1", doc.allocate_string(std::to_string(terrin->overlayBlendPower1).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendOpacity1", doc.allocate_string(std::to_string(terrin->overlayBlendOpacity1).c_str())));
		terrainNode->append_node(overlayNode);

		rapidxml::xml_node<>* slopeNode = doc.allocate_node(rapidxml::node_element, "Slope");
		slopeNode->append_attribute(doc.allocate_attribute("slopeSharpness0", doc.allocate_string(std::to_string(terrin->slopeSharpness0).c_str())));
		slopeNode->append_attribute(doc.allocate_attribute("slopeSharpness1", doc.allocate_string(std::to_string(terrin->slopeSharpness1).c_str())));
		slopeNode->append_attribute(doc.allocate_attribute("slopeSharpness2", doc.allocate_string(std::to_string(terrin->slopeSharpness2).c_str())));
		slopeNode->append_attribute(doc.allocate_attribute("slopeBias0", doc.allocate_string(std::to_string(terrin->slopeBias0).c_str())));
		slopeNode->append_attribute(doc.allocate_attribute("slopeBias1", doc.allocate_string(std::to_string(terrin->slopeBias1).c_str())));
		slopeNode->append_attribute(doc.allocate_attribute("slopeBias2", doc.allocate_string(std::to_string(terrin->slopeBias2).c_str())));
		terrainNode->append_node(slopeNode);

		rapidxml::xml_node<>* heightNode = doc.allocate_node(rapidxml::node_element, "Height");
		heightNode->append_attribute(doc.allocate_attribute("heightBias0", doc.allocate_string(std::to_string(terrin->heightBias0).c_str())));
		heightNode->append_attribute(doc.allocate_attribute("heightSharpness0", doc.allocate_string(std::to_string(terrin->heightSharpness0).c_str())));
		heightNode->append_attribute(doc.allocate_attribute("heightBias1", doc.allocate_string(std::to_string(terrin->heightBias1).c_str())));
		heightNode->append_attribute(doc.allocate_attribute("heightSharpness1", doc.allocate_string(std::to_string(terrin->heightSharpness1).c_str())));
		terrainNode->append_node(heightNode);

		entNode->append_node(terrainNode);

		return true;
	}

	bool Scene::loadTerrainComponent(Entity* ent, rapidxml::xml_node<>* entNode) {

		rapidxml::xml_node<>* terrainNode = entNode->first_node("Terrain");

		if (terrainNode == NULL)
			return false;

		Terrain* terrain = ent->addComponent<Terrain>();

		rapidxml::xml_node<>* fogNode = terrainNode->first_node("Fog");
		terrain->maxFog = atof(fogNode->first_attribute("maxFog")->value());
		terrain->fogColor.r = atof(fogNode->first_attribute("color_r")->value());
		terrain->fogColor.g = atof(fogNode->first_attribute("color_g")->value());
		terrain->fogColor.b = atof(fogNode->first_attribute("color_b")->value());
		
		rapidxml::xml_node<>* ambientNode = terrainNode->first_node("Ambient");
		terrain->ambientAmount = atof(ambientNode->first_attribute("ambientAmount")->value());
		terrain->specularPower = atof(ambientNode->first_attribute("specularPower")->value());
		terrain->specularAmount = atof(ambientNode->first_attribute("specularAmount")->value());

		rapidxml::xml_node<>* distanceBlendNode = terrainNode->first_node("DistanceBlend");
		terrain->blendDistance0 = atof(distanceBlendNode->first_attribute("blendDistance0")->value());
		terrain->blendAmount0 = atof(distanceBlendNode->first_attribute("blendAmount0")->value());
		terrain->blendDistance1 = atof(distanceBlendNode->first_attribute("blendDistance1")->value());
		terrain->blendAmount1 = atof(distanceBlendNode->first_attribute("blendAmount1")->value());

		rapidxml::xml_node<>* colorScaleNode = terrainNode->first_node("ColorScale");
		terrain->scale_color0_dist0 = atof(colorScaleNode->first_attribute("scale_color0_dist0")->value());
		terrain->scale_color0_dist1 = atof(colorScaleNode->first_attribute("scale_color0_dist1")->value());
		terrain->scale_color0_dist2 = atof(colorScaleNode->first_attribute("scale_color0_dist2")->value());
		terrain->scale_color1_dist0 = atof(colorScaleNode->first_attribute("scale_color1_dist0")->value());
		terrain->scale_color1_dist1 = atof(colorScaleNode->first_attribute("scale_color1_dist1")->value());
		terrain->scale_color1_dist2 = atof(colorScaleNode->first_attribute("scale_color1_dist2")->value());
		terrain->scale_color2_dist0 = atof(colorScaleNode->first_attribute("scale_color2_dist0")->value());
		terrain->scale_color2_dist1 = atof(colorScaleNode->first_attribute("scale_color2_dist1")->value());
		terrain->scale_color2_dist2 = atof(colorScaleNode->first_attribute("scale_color2_dist2")->value());
		terrain->scale_color3_dist0 = atof(colorScaleNode->first_attribute("scale_color3_dist0")->value());
		terrain->scale_color3_dist1 = atof(colorScaleNode->first_attribute("scale_color3_dist1")->value());
		terrain->scale_color3_dist2 = atof(colorScaleNode->first_attribute("scale_color3_dist2")->value());
		terrain->scale_color4_dist0 = atof(colorScaleNode->first_attribute("scale_color4_dist0")->value());
		terrain->scale_color4_dist1 = atof(colorScaleNode->first_attribute("scale_color4_dist1")->value());
		terrain->scale_color4_dist2 = atof(colorScaleNode->first_attribute("scale_color4_dist2")->value());
		terrain->scale_color5_dist0 = atof(colorScaleNode->first_attribute("scale_color5_dist0")->value());
		terrain->scale_color5_dist1 = atof(colorScaleNode->first_attribute("scale_color5_dist1")->value());
		terrain->scale_color5_dist2 = atof(colorScaleNode->first_attribute("scale_color5_dist2")->value());
		terrain->scale_color6_dist0 = atof(colorScaleNode->first_attribute("scale_color6_dist0")->value());
		terrain->scale_color6_dist1 = atof(colorScaleNode->first_attribute("scale_color6_dist1")->value());
		terrain->scale_color6_dist2 = atof(colorScaleNode->first_attribute("scale_color6_dist2")->value());

		rapidxml::xml_node<>* macroNode = terrainNode->first_node("Macro");
		terrain->macroScale_0 = atof(macroNode->first_attribute("macroScale_0")->value());
		terrain->macroScale_1 = atof(macroNode->first_attribute("macroScale_1")->value());
		terrain->macroScale_2 = atof(macroNode->first_attribute("macroScale_2")->value());
		terrain->macroPower = atof(macroNode->first_attribute("macroPower")->value());
		terrain->macroAmount = atof(macroNode->first_attribute("macroAmount")->value());
		terrain->macroOpacity = atof(macroNode->first_attribute("macroOpacity")->value());

		rapidxml::xml_node<>* overlayNode = terrainNode->first_node("Overlay");
		terrain->overlayBlendScale0 = atof(overlayNode->first_attribute("overlayBlendScale0")->value());
		terrain->overlayBlendAmount0 = atof(overlayNode->first_attribute("overlayBlendAmount0")->value());
		terrain->overlayBlendPower0 = atof(overlayNode->first_attribute("overlayBlendPower0")->value());
		terrain->overlayBlendOpacity0 = atof(overlayNode->first_attribute("overlayBlendOpacity0")->value());
		terrain->overlayBlendScale1 = atof(overlayNode->first_attribute("overlayBlendScale1")->value());
		terrain->overlayBlendAmount1 = atof(overlayNode->first_attribute("overlayBlendAmount1")->value());
		terrain->overlayBlendPower1 = atof(overlayNode->first_attribute("overlayBlendPower1")->value());
		terrain->overlayBlendOpacity1 = atof(overlayNode->first_attribute("overlayBlendOpacity1")->value());

		rapidxml::xml_node<>* slopeNode = terrainNode->first_node("Slope");
		terrain->slopeSharpness0 = atof(slopeNode->first_attribute("slopeSharpness0")->value());
		terrain->slopeSharpness1 = atof(slopeNode->first_attribute("slopeSharpness1")->value());
		terrain->slopeSharpness2 = atof(slopeNode->first_attribute("slopeSharpness2")->value());
		terrain->slopeBias0 = atof(slopeNode->first_attribute("slopeBias0")->value());
		terrain->slopeBias1 = atof(slopeNode->first_attribute("slopeBias1")->value());
		terrain->slopeBias2 = atof(slopeNode->first_attribute("slopeBias2")->value());

		rapidxml::xml_node<>* heightNode = terrainNode->first_node("Height");
		terrain->heightBias0 = atof(heightNode->first_attribute("heightBias0")->value());
		terrain->heightSharpness0 = atof(heightNode->first_attribute("heightSharpness0")->value());
		terrain->heightBias1 = atof(heightNode->first_attribute("heightBias1")->value());
		terrain->heightSharpness1 = atof(heightNode->first_attribute("heightSharpness1")->value());

		terrain->start();
		return true;
	}
//
//	bool Scene::saveParticleSystemComponent(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* entNode, ParticleSystem* particleSystem) {
//
//		rapidxml::xml_node<>* particleSystemNode = doc.allocate_node(rapidxml::node_element, "ParticleSystem");
//		//terrainNode ->append_attribute(doc.allocate_attribute("ClipmapResolution", doc.allocate_string(std::to_string(terrin->clipmapResolution).c_str())));
//		//terrainNode ->append_attribute(doc.allocate_attribute("ClipmapLevel", doc.allocate_string(std::to_string(terrin->clipmapLevel).c_str())));
//		//terrainNode ->append_attribute(doc.allocate_attribute("TriangleSize", doc.allocate_string(std::to_string(terrin->triangleSize).c_str())));
//		entNode->append_node(particleSystemNode);
//
//		return true;
//	}
//
//	bool Scene::loadParticleSystemComponent(Entity* ent, rapidxml::xml_node<>* entNode) {
//
//		rapidxml::xml_node<>* cameraNode = entNode->first_node("ParticleSystem");
//
//		if (cameraNode == NULL)
//			return false;
//
//		ParticleSystem* particleSystemComp = ent->addComponent<ParticleSystem>();
//		//particleSystemComp->clipmapResolution = 120;// atoi(cameraNode->first_attribute("ClipmapResolution")->value());
//		////terrainComp->clipmapLevel = 4;// atoi(cameraNode->first_attribute("ClipmapLevel")->value());
//		////terrainComp->triangleSize = atof(cameraNode->first_attribute("TriangleSize")->value());
//		particleSystemComp->start();
//		return true;
//	}



	Entity* Scene::newEntity(std::string name) {
		root = new Entity(name);//, idCounter
		//entityIdToEntity.insert(std::pair<unsigned int, Entity*>(idCounter, root));
		//idCounter++;
		return root;
	}

	Entity* Scene::newEntity(std::string name, Entity* parent) {
		Entity* ent = new Entity(name, parent);//, idCounter
		//entityIdToEntity.insert(std::pair<unsigned int, Entity*>(idCounter, ent));
		//idCounter++;
		return ent;
	}

	Entity* Scene::newEntity(Entity* entity, Entity* parent) {
		Entity* cpy = new Entity(entity, parent);//, idCounter
		//entityIdToEntity.insert(std::pair<unsigned int, Entity*>(idCounter, cpy));
		//idCounter++;
		return cpy;
	}

	Entity* Scene::duplicate(Entity* entity) {
		Entity* cpy = Scene::newEntity(entity, entity->transform->parent->entity);
		cloneRecursively(entity, cpy);
		return entity;
	}

	void Scene::cloneRecursively(Entity* copied, Entity* parent) {

		for (int i = 0; i < copied->transform->children.size(); i++) {

			Entity* entity = Scene::newEntity(copied->transform->children[i]->entity, parent);
			Scene::cloneRecursively(copied->transform->children[i]->entity, entity);
		}
	}
}