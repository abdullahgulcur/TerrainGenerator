#include "pch.h"
#include "corecontext.h"
#include "component/meshrenderer.h"
#include "entity.h"
#include "GL/glew.h"

namespace Core {

	MeshRenderer::MeshRenderer() {
	}

	MeshRenderer::~MeshRenderer() {

	}

	void MeshRenderer::start() {

	}

	void MeshRenderer::update(float dt) {

		if (!mesh || !material)
			return;

		glm::mat4 model = entity->transform->model;
		glm::vec4 startInWorldSpace = model * mesh->aabbBox.start;
		glm::vec4 endInWorldSpace = model * mesh->aabbBox.end;

		Cubemap* cubemap = CoreContext::instance->scene->cubemap;

		CameraInfo& cameraInfo = CoreContext::instance->scene->cameraInfo;

		switch (material->shaderType) {

		case ShaderType::PBR: {

			unsigned int shaderProgramId = CoreContext::instance->renderer->defaultPbrShaderProgramId;
			glUseProgram(shaderProgramId);

			glUniform1i(glGetUniformLocation(shaderProgramId, "irradianceMap"), 0);
			glUniform1i(glGetUniformLocation(shaderProgramId, "prefilterMap"), 1);
			glUniform1i(glGetUniformLocation(shaderProgramId, "brdfLUT"), 2);
			glUniform1i(glGetUniformLocation(shaderProgramId, "texture0"), 3);
			glUniform1i(glGetUniformLocation(shaderProgramId, "texture1"), 4);
			glUniform1i(glGetUniformLocation(shaderProgramId, "texture2"), 5);
			glUniform1i(glGetUniformLocation(shaderProgramId, "texture3"), 6);
			glUniform1i(glGetUniformLocation(shaderProgramId, "texture4"), 7);

			glUniformMatrix4fv(glGetUniformLocation(shaderProgramId, "projection"), 1, GL_FALSE, &cameraInfo.projection[0][0]);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgramId, "view"), 1, GL_FALSE, &cameraInfo.view[0][0]);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgramId, "model"), 1, GL_FALSE, &model[0][0]);
			glUniform3fv(glGetUniformLocation(shaderProgramId, "camPos"), 1, &cameraInfo.camPos[0]);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->irradianceMap);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->prefilterMap);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, cubemap->brdfLUTTexture);

			for (int i = 0; i < material->textures.size(); i++) {

				std::string texStr = "texture" + std::to_string(i);
				glActiveTexture(GL_TEXTURE3 + i);
				glBindTexture(GL_TEXTURE_2D, material->textures[i]->textureId);
			}

			glBindVertexArray(mesh->VAO);
			glDrawElements(0x0004, mesh->indiceCount, 0x1405, (void*)0);
			glBindVertexArray(0);

			break;
		}
		case ShaderType::PBR_ALPHA: {

			glDisable(GL_CULL_FACE);

			unsigned int shaderProgramId = CoreContext::instance->renderer->alphaBlendedPbrShaderProgramId;
			glUseProgram(shaderProgramId);

			glUniform1i(glGetUniformLocation(shaderProgramId, "irradianceMap"), 0);
			glUniform1i(glGetUniformLocation(shaderProgramId, "prefilterMap"), 1);
			glUniform1i(glGetUniformLocation(shaderProgramId, "brdfLUT"), 2);
			glUniform1i(glGetUniformLocation(shaderProgramId, "texture0"), 3);
			glUniform1i(glGetUniformLocation(shaderProgramId, "texture1"), 4);
			glUniform1i(glGetUniformLocation(shaderProgramId, "texture2"), 5);
			glUniform1i(glGetUniformLocation(shaderProgramId, "texture3"), 6);
			glUniform1i(glGetUniformLocation(shaderProgramId, "texture4"), 7);
			glUniform1i(glGetUniformLocation(shaderProgramId, "texture5"), 8);

			glUniformMatrix4fv(glGetUniformLocation(shaderProgramId, "projection"), 1, GL_FALSE, &cameraInfo.projection[0][0]);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgramId, "view"), 1, GL_FALSE, &cameraInfo.view[0][0]);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgramId, "model"), 1, GL_FALSE, &model[0][0]);
			glUniform3fv(glGetUniformLocation(shaderProgramId, "camPos"), 1, &cameraInfo.camPos[0]);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->irradianceMap);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->prefilterMap);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, cubemap->brdfLUTTexture);

			for (int i = 0; i < material->textures.size(); i++) {

				std::string texStr = "texture" + std::to_string(i);
				glActiveTexture(GL_TEXTURE3 + i);
				glBindTexture(GL_TEXTURE_2D, material->textures[i]->textureId);
			}

			glBindVertexArray(mesh->VAO);
			glDrawElements(0x0004, mesh->indiceCount, 0x1405, (void*)0);
			glBindVertexArray(0);

			glEnable(GL_CULL_FACE);

			break;
		}
		}
	}

	void MeshRenderer::setMesh(Mesh* mesh) {
		this->mesh = mesh;
	}

	void MeshRenderer::setMaterial(Material* material) {
		this->material = material;
	}
}