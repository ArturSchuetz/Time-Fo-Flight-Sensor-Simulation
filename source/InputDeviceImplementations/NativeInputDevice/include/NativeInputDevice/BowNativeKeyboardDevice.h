#pragma once
#include <NativeInputDevice/NativeInputDevice_api.h>

#include <InputDevice/IBowKeyboard.h>

struct GLFWwindow;

namespace bow {

	class NativeKeyboardDevice : public IKeyboard
	{
	public:
		NativeKeyboardDevice(GLFWwindow* glfwWindow);
		~NativeKeyboardDevice(void);

		bool Initialize(void);
		bool VUpdate(void);
		bool VIsPressed(Key keyID) const;

	protected:
		char m_keys[256];

		GLFWwindow* m_glfwWindow;
	};

	typedef std::shared_ptr<NativeKeyboardDevice> NativeKeyboardPtr;
}
