#include <NativeInputDevice/BowNativeInputDevice.h>

#include <NativeInputDevice/BowNativeKeyboardDevice.h>
#include <NativeInputDevice/BowNativeMouseDevice.h>

#include <RenderDevice/Device/BowGraphicsWindow.h>

#include <GLFW/glfw3.h>

namespace bow {

	NativeInputDevice::NativeInputDevice(void)
	{
	}

	NativeInputDevice::~NativeInputDevice(void)
	{
		VRelease();
	}

	bool NativeInputDevice::Initialize()
	{
		return true;
	}

	void NativeInputDevice::VRelease(void)
	{

	}

	MousePtr NativeInputDevice::VCreateMouseObject(GraphicsWindowPtr window)
	{
		GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window->VGetWindowHandle());
		if (glfwWindow == nullptr)
		{
			return NativeMousePtr(nullptr);
		}

		NativeMouseDevice *pDevice = new NativeMouseDevice(glfwWindow);
		pDevice->Initialize();
		return NativeMousePtr(pDevice);
	}

	KeyboardPtr NativeInputDevice::VCreateKeyboardObject(GraphicsWindowPtr window)
	{
		GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window->VGetWindowHandle());
		if (glfwWindow == nullptr)
		{
			return NativeKeyboardPtr(nullptr);
		}

		NativeKeyboardDevice *pDevice = new NativeKeyboardDevice(glfwWindow);
		pDevice->Initialize();
		return NativeKeyboardPtr(pDevice);
	}

}
