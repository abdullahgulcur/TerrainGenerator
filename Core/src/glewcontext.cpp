#include "pch.h"
#include "glewcontext.h"

namespace Core {

	GlewContext::GlewContext() {

		glewExperimental = true;

		if (glewInit() != GLEW_OK)
			fprintf(stderr, "Failed to initialize GLEW\n");

		glCullFace(GL_BACK);
		//glCullFace(GL_FRONT_AND_BACK);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_CULL_FACE);
		glEnable(GL_MULTISAMPLE);

		// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
		//glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	}

	GlewContext::~GlewContext() {

	}

	void GlewContext::createFrameBuffer(unsigned int& FBO, unsigned int& RBO, unsigned int& textureBuffer, int sizeX, int sizeY) {

		if (FBO != 0) {
			glDeleteRenderbuffers(1, &RBO);
			glDeleteTextures(1, &textureBuffer);
			glDeleteFramebuffers(1, &FBO);
		}
			
		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		glGenTextures(1, &textureBuffer);
		glBindTexture(GL_TEXTURE_2D, textureBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sizeX, sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureBuffer, 0);

		//unsigned int rbo;
		glGenRenderbuffers(1, &RBO);
		glBindRenderbuffer(GL_RENDERBUFFER, RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, sizeX, sizeY);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}


}