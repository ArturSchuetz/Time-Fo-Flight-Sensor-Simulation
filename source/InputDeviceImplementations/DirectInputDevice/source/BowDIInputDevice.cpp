#include <DirectInputDevice/BowDIInputDevice.h>

#include <DirectInputDevice/BowDIMouseDevice.h>
#include <DirectInputDevice/BowDIKeyboardDevice.h>

#include <RenderDevice/Device/BowGraphicsWindow.h>

#include <Windows.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace bow {

	DIInputDevice::DIInputDevice(void)
	{
		m_pDirectInput = nullptr;
	}

	DIInputDevice::~DIInputDevice(void)
	{
		VRelease();
	}

	bool DIInputDevice::Initialize()
	{
		HINSTANCE hInstance = GetModuleHandle(NULL);
		if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_pDirectInput, NULL)))
		{
			return false;
		}
		return true;
	}

	void DIInputDevice::VRelease(void)
	{
		if (m_pDirectInput)
		{
			m_pDirectInput->Release();
			m_pDirectInput = nullptr;
		}
	}

	MousePtr DIInputDevice::VCreateMouseObject(GraphicsWindowPtr window)
	{
		GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window->VGetWindowHandle());
		if (glfwWindow == nullptr)
		{
			return DIMousePtr(nullptr);
		}

		DIMouseDevice *pDevice = new DIMouseDevice(m_pDirectInput, glfwGetWin32Window(glfwWindow));
		pDevice->Initialize();
		return DIMousePtr(pDevice);
	}

	KeyboardPtr DIInputDevice::VCreateKeyboardObject(GraphicsWindowPtr window)
	{
		GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window->VGetWindowHandle());
		if (glfwWindow == nullptr)
		{
			return DIKeyboardPtr(nullptr);
		}


		DIKeyboardDevice *pDevice = new DIKeyboardDevice(m_pDirectInput, glfwGetWin32Window(glfwWindow));
		pDevice->Initialize();
		return DIKeyboardPtr(pDevice);
	}

}
