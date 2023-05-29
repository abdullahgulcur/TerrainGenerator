#pragma once

#include "GL/glew.h"
#include "include/GLFW/glfw3.h"

namespace Core {

	class Core;

	class __declspec(dllexport) GlfwContext {

	private:

	public:

		GLFWwindow* GLFW_window;
		const GLFWvidmode* mode;
		GLFWmonitor* monitor;
		static float scrollOffset;

		GlfwContext();
		~GlfwContext();
		void update(float dt);
		void loadTitleBarIcon();
		void end();
		bool getOpen();
		void terminateGLFW();
		void setTitle(const char* title);
		void setCursorPos(float x, float y);
		float verticalScrollOffset();
		static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
		void resetVerticalScrollOffset();
		static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	};
}