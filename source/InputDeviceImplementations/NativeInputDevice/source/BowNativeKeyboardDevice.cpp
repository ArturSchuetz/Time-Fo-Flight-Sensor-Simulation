#include <NativeInputDevice/BowNativeKeyboardDevice.h>
#include <NativeInputDevice/BowNativeTypeConverter.h>
#include <GLFW/glfw3.h>

namespace bow {

	NativeKeyboardDevice::NativeKeyboardDevice(GLFWwindow* glfwWindow) : m_glfwWindow(glfwWindow)
	{
	}

	NativeKeyboardDevice::~NativeKeyboardDevice()
	{
	}

	bool NativeKeyboardDevice::Initialize(void)
	{
		return true;
	}

	bool NativeKeyboardDevice::VUpdate(void)
	{
		return true;
	}

	bool NativeKeyboardDevice::VIsPressed(Key keyID) const
	{
		int state = glfwGetKey(m_glfwWindow, NativeTypeConverter::To(keyID));
		return state == GLFW_PRESS;
	}

}
