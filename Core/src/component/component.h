#pragma once

namespace Core {

	class __declspec(dllexport) Component {

	private:

	protected:

	public:

		Component();
		virtual ~Component();
		virtual void start(); // void* params, unsigned int length
		virtual void update(float dt);
		virtual void destroy();
	};
}