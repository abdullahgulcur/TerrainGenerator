#include "pch.h"
#include "glfwcontext.h"
#include "corecontext.h"
#include "lodepng/lodepng.h"

namespace Core {

	float GlfwContext::scrollOffset;
	
	GlfwContext::GlfwContext() {

		if (!glfwInit())
			fprintf(stderr, "Failed to initialize GLFW\n");

		//glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		monitor = glfwGetPrimaryMonitor();
		mode = glfwGetVideoMode(monitor);

		//GLFW_window = glfwCreateWindow(mode->width, mode->height, "Fury", monitor, NULL);

#ifdef EDITOR_MODE
		GLFW_window = glfwCreateWindow(mode->width, mode->height, "TerrainEngine", NULL, NULL);
#else
		//GLFW_window = glfwCreateWindow(mode->width, mode->height, "Fury", NULL, NULL); // windowed
		GLFW_window = glfwCreateWindow(mode->width, mode->height, "TerrainEngine", monitor, NULL); // fullscreen
#endif // EDITOR_MODE


		glfwMaximizeWindow(GLFW_window);
		glfwMakeContextCurrent(GLFW_window);

		glfwSetInputMode(GLFW_window, GLFW_STICKY_KEYS, GL_TRUE);
		glfwSetInputMode(GLFW_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		GlfwContext::loadTitleBarIcon();

		glfwSetScrollCallback(GLFW_window, GlfwContext::scroll_callback);
		glfwSetKeyCallback(GLFW_window, GlfwContext::key_callback);
	}

	GlfwContext::~GlfwContext() {

		GlfwContext::terminateGLFW();
	}

	void GlfwContext::loadTitleBarIcon() {

		GLFWimage image;
		unsigned width;
		unsigned height;

		std::vector<unsigned char> img;
		unsigned error = lodepng::decode(img, width, height, "resources/editor/icons/material.png");

		image.pixels = &img[0];
		image.width = width;
		image.height = height;
		glfwSetWindowIcon(GLFW_window, 1, &image);
	}

	void GlfwContext::update(float dt) {

	}

	void GlfwContext::end() {

		glfwSwapBuffers(GLFW_window);
		glfwPollEvents();
	}

	bool GlfwContext::getOpen() {

		bool open = glfwGetKey(GLFW_window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(GLFW_window) == 0;

		if (!open) {

			//core->saveLoadSystem->saveSceneCamera();
		}

		return open;
	}

	void GlfwContext::terminateGLFW() {

		glfwTerminate();
	}

	void GlfwContext::setTitle(const char* title) {

		glfwSetWindowTitle(GLFW_window, title);
	}

	void GlfwContext::setCursorPos(float x, float y) {

		glfwSetCursorPos(GLFW_window, x, y);
	}

	float GlfwContext::verticalScrollOffset() {

		return scrollOffset;
	}

	void GlfwContext::resetVerticalScrollOffset() {

		scrollOffset = 0;
	}

	void GlfwContext::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{
		scrollOffset = yoffset;
	}

	void GlfwContext::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

		/*if (action == GLFW_PRESS) {
			switch (key) {
			case GLFW_KEY_W: Input::w_pressed = true; break;
			case GLFW_KEY_A: Input::a_pressed = true; break;
			case GLFW_KEY_S: Input::s_pressed = true; break;
			case GLFW_KEY_D: Input::d_pressed = true; break;
			case GLFW_KEY_SPACE: Input::space_pressed = true; break;
			case GLFW_KEY_LEFT_SHIFT: Input::left_shift_pressed = true; break;
			case GLFW_KEY_ENTER: Input::enter_pressed = true; break;
			case GLFW_KEY_LEFT_CONTROL: Input::left_ctrl_pressed = true; break;
			case GLFW_KEY_LEFT_ALT: Input::left_alt_pressed = true; break;
			case GLFW_KEY_ESCAPE: Input::esc_pressed = true; break;
			case GLFW_KEY_UP: Input::up_pressed = true; break;
			case GLFW_KEY_DOWN: Input::down_pressed = true; break;
			case GLFW_KEY_LEFT: Input::left_pressed = true; break;
			case GLFW_KEY_RIGHT: Input::right_pressed = true; break;
			default: break;
			}
		}

		if (action == GLFW_RELEASE) {
			switch (key) {
			case GLFW_KEY_W: Input::w_pressed = false; break;
			case GLFW_KEY_A: Input::a_pressed = false; break;
			case GLFW_KEY_S: Input::s_pressed = false; break;
			case GLFW_KEY_D: Input::d_pressed = false; break;
			case GLFW_KEY_SPACE: Input::space_pressed = false; break;
			case GLFW_KEY_LEFT_SHIFT: Input::left_shift_pressed = false; break;
			case GLFW_KEY_ENTER: Input::enter_pressed = false; break;
			case GLFW_KEY_LEFT_CONTROL: Input::left_ctrl_pressed = false; break;
			case GLFW_KEY_LEFT_ALT: Input::left_alt_pressed = false; break;
			case GLFW_KEY_ESCAPE: Input::esc_pressed = false; break;
			case GLFW_KEY_UP: Input::up_pressed = false; break;
			case GLFW_KEY_DOWN: Input::down_pressed = false; break;
			case GLFW_KEY_LEFT: Input::left_pressed = false; break;
			case GLFW_KEY_RIGHT: Input::right_pressed = false; break;
			default: break;
			}
		}*/
	}
}