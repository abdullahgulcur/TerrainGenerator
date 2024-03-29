#include "pch.h"
#include "renderer.h"
#include "GL/glew.h"
#include "corecontext.h"
#include "shader.h"
#include "glewcontext.h"
#include "component/terrain.h"

using namespace std::chrono;

namespace Core {

	void Renderer::init() {

		backgroundShaderProgramId = Shader::loadShaders("resources/shaders/cubemap/background.vert", "resources/shaders/cubemap/background.frag");
		defaultPbrShaderProgramId = Shader::loadShaders("resources/shaders/pbr/pbr.vert", "resources/shaders/pbr/pbr.frag");
		lineShaderProgramId = Shader::loadShaders("resources/shaders/gizmo/line.vert", "resources/shaders/gizmo/line.frag");

		Renderer::createEnvironmentCubeVAO();

		// EDITOR
		Renderer::createBoundingBoxVAO();
	}

	void Renderer::update(float dt) {

        Scene* scene = CoreContext::instance->scene;

        glBindFramebuffer(GL_FRAMEBUFFER, scene->FBO);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glViewport(0, 0, scene->width, scene->height);
        glClearColor(0.3f, 0.3f, 0.3f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (Terrain* terrain = scene->terrain) {
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			auto start = high_resolution_clock::now();
			terrain->onDraw();
			auto stop = high_resolution_clock::now();
			unsigned int duration = duration_cast<microseconds>(stop - start).count();
			terrainRenderTotalTime += duration;
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


		// ------------------ FRAMEBUFFER PASS (FILTERS) ------------------

#ifdef EDITOR_MODE
		glBindFramebuffer(GL_FRAMEBUFFER, scene->filterFBO);
#else
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(scene->filterFramebufferProgramID);
		glUniform2f(glGetUniformLocation(scene->filterFramebufferProgramID, "resolution"), scene->width, scene->height);
		glBindVertexArray(scene->screenQuadVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, scene->textureBuffer);
		glDrawArrays(GL_TRIANGLES, 0, 6);

#ifdef EDITOR_MODE
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif	

		frameCounter++;
		terrainRenderDuration = terrainRenderTotalTime / frameCounter;
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

	void Renderer::createBoundingBoxVAO() {

		float vertices[] = {

			0.5f,	0.5f,	-0.5f,   
			-0.5f,	0.5f,	-0.5f,
			-0.5f,	0.5f,	-0.5f,
			-0.5f,	-0.5f,	-0.5f,
			-0.5f,	-0.5f,	-0.5f,
			0.5f,	-0.5f,	-0.5f,
			0.5f,	-0.5f,	-0.5f,
			0.5f,	0.5f,	-0.5f,

			0.5f,	0.5f,	0.5f,
			-0.5f,	0.5f,	0.5f,
			-0.5f,	0.5f,	0.5f,
			-0.5f,	-0.5f,	0.5f,
			-0.5f,	-0.5f,	0.5f,
			0.5f,	-0.5f,	0.5f,
			0.5f,	-0.5f,	0.5f,
			0.5f,	0.5f,	0.5f,

			0.5f,	0.5f,	-0.5f,
			0.5f,	0.5f,	0.5f,
			-0.5f,	0.5f,	-0.5f,
			-0.5f,	0.5f,	0.5f,
			-0.5f,	-0.5f,	-0.5f,
			-0.5f,	-0.5f,	0.5f,
			0.5f,	-0.5f,	-0.5f,
			0.5f,	-0.5f,	0.5f,
		};

		glGenVertexArrays(1, &boundingBoxVAO);
		unsigned int boundingBoxVBO;
		glGenBuffers(1, &boundingBoxVBO);
		glBindBuffer(GL_ARRAY_BUFFER, boundingBoxVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindVertexArray(boundingBoxVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glBindVertexArray(0);
	}

	void Renderer::drawBoundingBoxVAO(glm::mat4& PVM, glm::vec3& color) {

		glUseProgram(lineShaderProgramId);
		glUniformMatrix4fv(glGetUniformLocation(lineShaderProgramId, "PVM"), 1, 0, &PVM[0][0]);
		glUniform3fv(glGetUniformLocation(lineShaderProgramId, "color"), 1, &color[0]);

		glBindVertexArray(boundingBoxVAO);
		glDrawArrays(GL_LINES, 0, 24);
		glBindVertexArray(0);
	}
}
