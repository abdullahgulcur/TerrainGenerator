#include "pch.h"
#include "renderer.h"
#include "GL/glew.h"
#include "corecontext.h"
#include "glewcontext.h"
#include "entity.h"
#include "component/terrain.h"

namespace Core {

	void Renderer::init() {

		GlewContext* glew = CoreContext::instance->glewContext;
		backgroundShaderProgramId = glew->loadShaders("resources/shaders/background.vert", "resources/shaders/background.frag");
		defaultPbrShaderProgramId = glew->loadShaders("resources/shaders/pbr.vert", "resources/shaders/pbr.frag");
		alphaBlendedPbrShaderProgramId = glew->loadShaders("resources/shaders/pbr_alpha_clipped.vert", "resources/shaders/pbr_alpha_clipped.frag");

		Renderer::createEnvironmentCubeVAO();
	}

	void Renderer::update(float dt) {

        Scene* scene = CoreContext::instance->scene;

        glBindFramebuffer(GL_FRAMEBUFFER, scene->cameraInfo.FBO);
        glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
        glDepthFunc(GL_LEQUAL); // set depth function to less than AND equal for skybox depth trick.

        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // enable depth testing (is disabled for rendering screen-space quad)
        glViewport(0, 0, scene->cameraInfo.width, scene->cameraInfo.height);
        glClearColor(0.3f, 0.3f, 0.3f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		std::stack<Entity*> entStack;
		entStack.push(scene->root);

		while (!entStack.empty()) {

			Entity* popped = entStack.top();
			entStack.pop();

			for (Transform*& child : popped->transform->children)
				entStack.push(child->entity);

			Terrain* terrain = popped->getComponent<Terrain>();
			if (terrain != NULL) {

				//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				//terrain->update(cameraInfo.camPos, dt);
				terrain->onDraw(scene->cameraInfo.VP, scene->cameraInfo.camPos);
			}

			//if (ParticleSystem* particleSystem = popped->getComponent<ParticleSystem>())
			//	particleSystem->onDraw(cameraInfo.VP, cameraInfo.camPos);

			if (MeshRenderer* renderer = popped->getComponent<MeshRenderer>())
				renderer->update(dt);

		}

        // render skybox (render as last to prevent overdraw)
        glUseProgram(backgroundShaderProgramId);
        glUniformMatrix4fv(glGetUniformLocation(backgroundShaderProgramId, "projection"), 1, GL_FALSE, &scene->cameraInfo.projection[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(backgroundShaderProgramId, "view"), 1, GL_FALSE, &scene->cameraInfo.view[0][0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, scene->cubemap->envCubemap);

        glDisable(GL_CULL_FACE);

        glBindVertexArray(envCubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glEnable(GL_CULL_FACE);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Renderer::createEnvironmentCubeVAO() {

		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left                
		};

		glGenVertexArrays(1, &envCubeVAO);
		unsigned int cubeVBO;
		glGenBuffers(1, &cubeVBO);
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindVertexArray(envCubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}


}
