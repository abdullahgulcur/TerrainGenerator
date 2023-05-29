#include "pch.h"
#include "scene.h"
#include "corecontext.h"
#include "component/terrain.h"

namespace Core {

	Scene::Scene() {

	}

	Scene::~Scene() {

		delete terrain;
		delete cubemap;
	}

	void Scene::start() {

		filterFramebufferProgramID = CoreContext::instance->glewContext->loadShaders("resources/shaders/framebuffer/framebuffer.vert",
			"resources/shaders/framebuffer/framebuffer_filter.frag");
		framebufferProgramID = CoreContext::instance->glewContext->loadShaders("resources/shaders/framebuffer/framebuffer.vert",
			"resources/shaders/framebuffer/framebuffer.frag");

		glUniform1i(glGetUniformLocation(filterFramebufferProgramID, "screenTexture"), 0);
		glUniform1i(glGetUniformLocation(framebufferProgramID, "screenTexture"), 0);

		float screenQuadVertices[] = {
		   // positions   // texCoords
		   -1.0f,  1.0f,  0.0f, 1.0f,
		   -1.0f, -1.0f,  0.0f, 0.0f,
			1.0f, -1.0f,  1.0f, 0.0f,

		   -1.0f,  1.0f,  0.0f, 1.0f,
			1.0f, -1.0f,  1.0f, 0.0f,
			1.0f,  1.0f,  1.0f, 1.0f
		};
		// screen quad VAO
		unsigned int screenQuadVBO;
		glGenVertexArrays(1, &screenQuadVAO);
		glGenBuffers(1, &screenQuadVBO);
		glBindVertexArray(screenQuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuadVertices), &screenQuadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, 0, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, 0, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		Scene::setFrameBuffer(1920, 1080);

		cubemap = CoreContext::instance->fileSystem->cubemaps.at("hilly_terrain_01_puresky_4k");

		Scene::loadScene(Scene::getActiveScenePath());

		// EDITOR ONLY
		activeSceneIndex = Scene::getActiveSceneIndex();
	}

	void Scene::update(float dt) {

		if (terrain != NULL)
			terrain->update(dt);
	}

	// EDITOR ONLY
	void Scene::changeScene(int index) {

		Scene::setActiveSceneIndex(index);
		CoreContext::instance->scene->activeSceneIndex = index;

		std::string scenePath = "resources/scenes/scene_" + std::to_string(index) + ".xml";
		CoreContext::instance->scene->loadScene(scenePath);
	}

	// EDITOR ONLY
	int Scene::getActiveSceneIndex() {

		std::ifstream file("database/activescene.xml");

		/*if (file.fail())
			return;*/

		rapidxml::xml_document<> doc;
		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		buffer.push_back('\0');
		doc.parse<0>(&buffer[0]);
		int activeSceneIndex = atoi(doc.first_node("Scene")->first_attribute("Index")->value());

		file.close();
		return activeSceneIndex;
	}

	// EDITOR ONLY
	std::string Scene::getActiveScenePath() {

		return "resources/scenes/scene_" + std::to_string(Scene::getActiveSceneIndex()) + ".xml";;
	}

	// EDITOR ONLY
	void Scene::setActiveSceneIndex(int index) {

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		doc.append_node(decl);

		rapidxml::xml_node<>* sceneNode = doc.allocate_node(rapidxml::node_element, "Scene");
		sceneNode->append_attribute(doc.allocate_attribute("Index", doc.allocate_string(std::to_string(index).c_str())));
		doc.append_node(sceneNode);

		std::string xml_as_string;
		rapidxml::print(std::back_inserter(xml_as_string), doc);

		std::ofstream file_stored("database/activescene.xml");
		file_stored << doc;
		file_stored.close();
		doc.clear();
	}

	bool Scene::saveScene(std::string filePath) {

		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		doc.append_node(decl);

		rapidxml::xml_node<>* SceneNode = doc.allocate_node(rapidxml::node_element, "Scene");
		doc.append_node(SceneNode);

		if(terrain)
			Scene::saveTerrain(doc, SceneNode, terrain);

		std::string xml_as_string;
		rapidxml::print(std::back_inserter(xml_as_string), doc);

		std::ofstream file_stored(filePath);
		file_stored << doc;
		file_stored.close();
		doc.clear();
		return true;
	}

	void Scene::destroyContent() {

		// delete environment settings

		if (CoreContext::instance->scene->terrain)
			delete CoreContext::instance->scene->terrain;

		CoreContext::instance->scene->terrain = NULL;
	}

	void Scene::setFrameBuffer(int width, int height) {

		width = CoreContext::instance->glfwContext->mode->width;
		height = CoreContext::instance->glfwContext->mode->height;

		CoreContext::instance->glewContext->createFrameBuffer(FBO, RBO, textureBuffer, width, height);
		CoreContext::instance->glewContext->createFrameBuffer(filterFBO, filterRBO, filterTextureBuffer, width, height);
	}

	void Scene::setSize(int width, int height) {

		this->width = width;
		this->height = height;
		CoreContext::instance->glewContext->createFrameBuffer(FBO, RBO, textureBuffer, width, height);
		CoreContext::instance->glewContext->createFrameBuffer(filterFBO, filterRBO, filterTextureBuffer, width, height);
	}

	void Scene::loadScene(std::string filePath) {

		Scene::destroyContent();
		// add environment settings

		std::ifstream file(filePath);

		if (file.fail()) {
			Scene::saveScene(filePath);
			return;
		}

		rapidxml::xml_document<> doc;
		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		buffer.push_back('\0');
		doc.parse<0>(&buffer[0]);

		rapidxml::xml_node<>* scene_node = doc.first_node("Scene");
		if (!scene_node) {
			Scene::saveScene(filePath);
			return;
		}

		if (rapidxml::xml_node<>* terrain_node = scene_node->first_node("Terrain")) {
			terrain = Scene::loadTerrain(terrain_node);
			terrain->start();
		}
		
		file.close();
	}

	bool Scene::saveTerrain(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* sceneNode, Terrain* terrain) {

		rapidxml::xml_node<>* terrainNode = doc.allocate_node(rapidxml::node_element, "Terrain");
		sceneNode->append_node(terrainNode);

		rapidxml::xml_node<>* lightNode = doc.allocate_node(rapidxml::node_element, "Light");
		lightNode ->append_attribute(doc.allocate_attribute("lightPow", doc.allocate_string(std::to_string(terrain->lightPow).c_str())));
		lightNode ->append_attribute(doc.allocate_attribute("lightDir_x", doc.allocate_string(std::to_string(terrain->lightDir.x).c_str())));
		lightNode ->append_attribute(doc.allocate_attribute("lightDir_y", doc.allocate_string(std::to_string(terrain->lightDir.y).c_str())));
		lightNode ->append_attribute(doc.allocate_attribute("lightDir_z", doc.allocate_string(std::to_string(terrain->lightDir.z).c_str())));
		terrainNode->append_node(lightNode);

		rapidxml::xml_node<>* fogNode = doc.allocate_node(rapidxml::node_element, "Fog");
		fogNode->append_attribute(doc.allocate_attribute("maxFog", doc.allocate_string(std::to_string(terrain->maxFog).c_str())));
		fogNode->append_attribute(doc.allocate_attribute("color_r", doc.allocate_string(std::to_string(terrain->fogColor.r).c_str())));
		fogNode->append_attribute(doc.allocate_attribute("color_g", doc.allocate_string(std::to_string(terrain->fogColor.g).c_str())));
		fogNode->append_attribute(doc.allocate_attribute("color_b", doc.allocate_string(std::to_string(terrain->fogColor.b).c_str())));
		fogNode->append_attribute(doc.allocate_attribute("distanceNear", doc.allocate_string(std::to_string(terrain->distanceNear).c_str())));
		fogNode->append_attribute(doc.allocate_attribute("fogBlendDistance", doc.allocate_string(std::to_string(terrain->fogBlendDistance).c_str())));
		terrainNode->append_node(fogNode);

		rapidxml::xml_node<>* ambientNode = doc.allocate_node(rapidxml::node_element, "Ambient");
		ambientNode->append_attribute(doc.allocate_attribute("ambientAmount", doc.allocate_string(std::to_string(terrain->ambientAmount).c_str())));
		ambientNode->append_attribute(doc.allocate_attribute("specularPower", doc.allocate_string(std::to_string(terrain->specularPower).c_str())));
		ambientNode->append_attribute(doc.allocate_attribute("specularAmount", doc.allocate_string(std::to_string(terrain->specularAmount).c_str())));
		terrainNode->append_node(ambientNode);

		rapidxml::xml_node<>* distanceBlendNode = doc.allocate_node(rapidxml::node_element, "DistanceBlend");
		distanceBlendNode->append_attribute(doc.allocate_attribute("blendDistance0", doc.allocate_string(std::to_string(terrain->blendDistance).c_str())));
		distanceBlendNode->append_attribute(doc.allocate_attribute("blendAmount0", doc.allocate_string(std::to_string(terrain->blendAmount).c_str())));
		terrainNode->append_node(distanceBlendNode);

		rapidxml::xml_node<>* colorScaleNode = doc.allocate_node(rapidxml::node_element, "ColorScale");
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color0_dist0", doc.allocate_string(std::to_string(terrain->scale_color0_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color0_dist1", doc.allocate_string(std::to_string(terrain->scale_color0_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color1_dist0", doc.allocate_string(std::to_string(terrain->scale_color1_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color1_dist1", doc.allocate_string(std::to_string(terrain->scale_color1_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color2_dist0", doc.allocate_string(std::to_string(terrain->scale_color2_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color2_dist1", doc.allocate_string(std::to_string(terrain->scale_color2_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color3_dist0", doc.allocate_string(std::to_string(terrain->scale_color3_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color3_dist1", doc.allocate_string(std::to_string(terrain->scale_color3_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color4_dist0", doc.allocate_string(std::to_string(terrain->scale_color4_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color4_dist1", doc.allocate_string(std::to_string(terrain->scale_color4_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color5_dist0", doc.allocate_string(std::to_string(terrain->scale_color5_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color5_dist1", doc.allocate_string(std::to_string(terrain->scale_color5_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color6_dist0", doc.allocate_string(std::to_string(terrain->scale_color6_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color6_dist1", doc.allocate_string(std::to_string(terrain->scale_color6_dist1).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color7_dist0", doc.allocate_string(std::to_string(terrain->scale_color7_dist0).c_str())));
		colorScaleNode->append_attribute(doc.allocate_attribute("scale_color7_dist1", doc.allocate_string(std::to_string(terrain->scale_color7_dist1).c_str())));
		terrainNode->append_node(colorScaleNode);

		rapidxml::xml_node<>* macroNode = doc.allocate_node(rapidxml::node_element, "Macro");
		macroNode->append_attribute(doc.allocate_attribute("macroScale_0", doc.allocate_string(std::to_string(terrain->macroScale_0).c_str())));
		macroNode->append_attribute(doc.allocate_attribute("macroScale_1", doc.allocate_string(std::to_string(terrain->macroScale_1).c_str())));
		macroNode->append_attribute(doc.allocate_attribute("macroScale_2", doc.allocate_string(std::to_string(terrain->macroScale_2).c_str())));
		macroNode->append_attribute(doc.allocate_attribute("macroPower", doc.allocate_string(std::to_string(terrain->macroPower).c_str())));
		macroNode->append_attribute(doc.allocate_attribute("macroAmount", doc.allocate_string(std::to_string(terrain->macroAmount).c_str())));
		macroNode->append_attribute(doc.allocate_attribute("macroOpacity", doc.allocate_string(std::to_string(terrain->macroOpacity).c_str())));
		terrainNode->append_node(macroNode);

		rapidxml::xml_node<>* overlayNode = doc.allocate_node(rapidxml::node_element, "Overlay");
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendScale0", doc.allocate_string(std::to_string(terrain->overlayBlendScale0).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendAmount0", doc.allocate_string(std::to_string(terrain->overlayBlendAmount0).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendPower0", doc.allocate_string(std::to_string(terrain->overlayBlendPower0).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendOpacity0", doc.allocate_string(std::to_string(terrain->overlayBlendOpacity0).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendScale1", doc.allocate_string(std::to_string(terrain->overlayBlendScale1).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendAmount1", doc.allocate_string(std::to_string(terrain->overlayBlendAmount1).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendPower1", doc.allocate_string(std::to_string(terrain->overlayBlendPower1).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendOpacity1", doc.allocate_string(std::to_string(terrain->overlayBlendOpacity1).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendScale2", doc.allocate_string(std::to_string(terrain->overlayBlendScale2).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendAmount2", doc.allocate_string(std::to_string(terrain->overlayBlendAmount2).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendPower2", doc.allocate_string(std::to_string(terrain->overlayBlendPower2).c_str())));
		overlayNode->append_attribute(doc.allocate_attribute("overlayBlendOpacity2", doc.allocate_string(std::to_string(terrain->overlayBlendOpacity2).c_str())));
		terrainNode->append_node(overlayNode);

		rapidxml::xml_node<>* color0Node = doc.allocate_node(rapidxml::node_element, "Color0");
		color0Node->append_attribute(doc.allocate_attribute("color_r", doc.allocate_string(std::to_string(terrain->color0.r).c_str())));
		color0Node->append_attribute(doc.allocate_attribute("color_g", doc.allocate_string(std::to_string(terrain->color0.g).c_str())));
		color0Node->append_attribute(doc.allocate_attribute("color_b", doc.allocate_string(std::to_string(terrain->color0.b).c_str())));
		terrainNode->append_node(color0Node);

		rapidxml::xml_node<>* color1Node = doc.allocate_node(rapidxml::node_element, "Color1");
		color1Node->append_attribute(doc.allocate_attribute("color_r", doc.allocate_string(std::to_string(terrain->color1.r).c_str())));
		color1Node->append_attribute(doc.allocate_attribute("color_g", doc.allocate_string(std::to_string(terrain->color1.g).c_str())));
		color1Node->append_attribute(doc.allocate_attribute("color_b", doc.allocate_string(std::to_string(terrain->color1.b).c_str())));
		terrainNode->append_node(color1Node);

		rapidxml::xml_node<>* slopeNode = doc.allocate_node(rapidxml::node_element, "Slope");
		slopeNode->append_attribute(doc.allocate_attribute("slopeSharpness0", doc.allocate_string(std::to_string(terrain->slopeSharpness0).c_str())));
		slopeNode->append_attribute(doc.allocate_attribute("slopeSharpness1", doc.allocate_string(std::to_string(terrain->slopeSharpness1).c_str())));
		slopeNode->append_attribute(doc.allocate_attribute("slopeBias0", doc.allocate_string(std::to_string(terrain->slopeBias0).c_str())));
		slopeNode->append_attribute(doc.allocate_attribute("slopeBias1", doc.allocate_string(std::to_string(terrain->slopeBias1).c_str())));
		terrainNode->append_node(slopeNode);

		rapidxml::xml_node<>* heightNode = doc.allocate_node(rapidxml::node_element, "Height");
		heightNode->append_attribute(doc.allocate_attribute("heightBias0", doc.allocate_string(std::to_string(terrain->heightBias0).c_str())));
		heightNode->append_attribute(doc.allocate_attribute("heightSharpness0", doc.allocate_string(std::to_string(terrain->heightSharpness0).c_str())));
		heightNode->append_attribute(doc.allocate_attribute("heightBias1", doc.allocate_string(std::to_string(terrain->heightBias1).c_str())));
		heightNode->append_attribute(doc.allocate_attribute("heightSharpness1", doc.allocate_string(std::to_string(terrain->heightSharpness1).c_str())));
		terrainNode->append_node(heightNode);

		return true;
	}

	Terrain* Scene::loadTerrain(rapidxml::xml_node<>* terrainNode) {

		Terrain* terrain = new Terrain;

		rapidxml::xml_node<>* lightNode = terrainNode->first_node("Light");
		terrain->lightPow = atof(lightNode->first_attribute("lightPow")->value());
		terrain->lightDir.x = atof(lightNode->first_attribute("lightDir_x")->value());
		terrain->lightDir.y = atof(lightNode->first_attribute("lightDir_y")->value());
		terrain->lightDir.z = atof(lightNode->first_attribute("lightDir_z")->value());

		rapidxml::xml_node<>* fogNode = terrainNode->first_node("Fog");
		terrain->maxFog = atof(fogNode->first_attribute("maxFog")->value());
		terrain->fogColor.r = atof(fogNode->first_attribute("color_r")->value());
		terrain->fogColor.g = atof(fogNode->first_attribute("color_g")->value());
		terrain->fogColor.b = atof(fogNode->first_attribute("color_b")->value());
		
		rapidxml::xml_node<>* color0Node = terrainNode->first_node("Color0");
		terrain->color0.r = atof(color0Node->first_attribute("color_r")->value());
		terrain->color0.g = atof(color0Node->first_attribute("color_g")->value());
		terrain->color0.b = atof(color0Node->first_attribute("color_b")->value());

		rapidxml::xml_node<>* color1Node = terrainNode->first_node("Color1");
		terrain->color1.r = atof(color1Node->first_attribute("color_r")->value());
		terrain->color1.g = atof(color1Node->first_attribute("color_g")->value());
		terrain->color1.b = atof(color1Node->first_attribute("color_b")->value());

		rapidxml::xml_node<>* ambientNode = terrainNode->first_node("Ambient");
		terrain->ambientAmount = atof(ambientNode->first_attribute("ambientAmount")->value());
		terrain->specularPower = atof(ambientNode->first_attribute("specularPower")->value());
		terrain->specularAmount = atof(ambientNode->first_attribute("specularAmount")->value());

		rapidxml::xml_node<>* distanceBlendNode = terrainNode->first_node("DistanceBlend");
		terrain->blendDistance = atof(distanceBlendNode->first_attribute("blendDistance0")->value());
		terrain->blendAmount = atof(distanceBlendNode->first_attribute("blendAmount0")->value());

		rapidxml::xml_node<>* colorScaleNode = terrainNode->first_node("ColorScale");
		terrain->scale_color0_dist0 = atof(colorScaleNode->first_attribute("scale_color0_dist0")->value());
		terrain->scale_color0_dist1 = atof(colorScaleNode->first_attribute("scale_color0_dist1")->value());
		terrain->scale_color1_dist0 = atof(colorScaleNode->first_attribute("scale_color1_dist0")->value());
		terrain->scale_color1_dist1 = atof(colorScaleNode->first_attribute("scale_color1_dist1")->value());
		terrain->scale_color2_dist0 = atof(colorScaleNode->first_attribute("scale_color2_dist0")->value());
		terrain->scale_color2_dist1 = atof(colorScaleNode->first_attribute("scale_color2_dist1")->value());
		terrain->scale_color3_dist0 = atof(colorScaleNode->first_attribute("scale_color3_dist0")->value());
		terrain->scale_color3_dist1 = atof(colorScaleNode->first_attribute("scale_color3_dist1")->value());
		terrain->scale_color4_dist0 = atof(colorScaleNode->first_attribute("scale_color4_dist0")->value());
		terrain->scale_color4_dist1 = atof(colorScaleNode->first_attribute("scale_color4_dist1")->value());
		terrain->scale_color5_dist0 = atof(colorScaleNode->first_attribute("scale_color5_dist0")->value());
		terrain->scale_color5_dist1 = atof(colorScaleNode->first_attribute("scale_color5_dist1")->value());
		terrain->scale_color6_dist0 = atof(colorScaleNode->first_attribute("scale_color6_dist0")->value());
		terrain->scale_color6_dist1 = atof(colorScaleNode->first_attribute("scale_color6_dist1")->value());
		terrain->scale_color7_dist0 = atof(colorScaleNode->first_attribute("scale_color7_dist0")->value());
		terrain->scale_color7_dist1 = atof(colorScaleNode->first_attribute("scale_color7_dist1")->value());

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
		terrain->overlayBlendScale2 = atof(overlayNode->first_attribute("overlayBlendScale2")->value());
		terrain->overlayBlendAmount2 = atof(overlayNode->first_attribute("overlayBlendAmount2")->value());
		terrain->overlayBlendPower2 = atof(overlayNode->first_attribute("overlayBlendPower2")->value());
		terrain->overlayBlendOpacity2 = atof(overlayNode->first_attribute("overlayBlendOpacity2")->value());

		rapidxml::xml_node<>* slopeNode = terrainNode->first_node("Slope");
		terrain->slopeSharpness0 = atof(slopeNode->first_attribute("slopeSharpness0")->value());
		terrain->slopeSharpness1 = atof(slopeNode->first_attribute("slopeSharpness1")->value());
		terrain->slopeBias0 = atof(slopeNode->first_attribute("slopeBias0")->value());
		terrain->slopeBias1 = atof(slopeNode->first_attribute("slopeBias1")->value());

		rapidxml::xml_node<>* heightNode = terrainNode->first_node("Height");
		terrain->heightBias0 = atof(heightNode->first_attribute("heightBias0")->value());
		terrain->heightSharpness0 = atof(heightNode->first_attribute("heightSharpness0")->value());
		terrain->heightBias1 = atof(heightNode->first_attribute("heightBias1")->value());
		terrain->heightSharpness1 = atof(heightNode->first_attribute("heightSharpness1")->value());

		return terrain;
	}
}