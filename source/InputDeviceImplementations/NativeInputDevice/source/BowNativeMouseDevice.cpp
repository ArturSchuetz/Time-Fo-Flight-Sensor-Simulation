#include <NativeInputDevice/BowNativeMouseDevice.h>
#include <NativeInputDevice/BowNativeTypeConverter.h>
#include <GLFW/glfw3.h>

#ifdef _WIN32 
#include <Windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <map>

namespace bow {

	std::map<GLFWwindow*, NativeMouseDevice*> g_device_window_map;

	void handleScrollInput(GLFWwindow* window, double xoffset, double yoffset)
	{
		if (g_device_window_map.find(window) != g_device_window_map.end())
		{
			g_device_window_map[window]->m_scroll += yoffset;
		}
	}

	NativeMouseDevice::NativeMouseDevice(GLFWwindow* glfwWindow) : m_glfwWindow(glfwWindow)
	{
		m_x = 0;
		m_y = 0;

		m_relativeX = 0;
		m_relativeY = 0;

		m_shouldCage = false;
		m_cageBounds_left = m_cageBounds_top = 0;
		m_cageBounds_right = m_cageBounds_bottom = 999999;
	}

	NativeMouseDevice::~NativeMouseDevice()
	{
		if (m_glfwWindow != nullptr)
		{
			g_device_window_map.erase(m_glfwWindow);
			m_glfwWindow = nullptr;
		}
	}

	bool NativeMouseDevice::Initialize(void)
	{
		g_device_window_map.insert(std::pair<GLFWwindow*, NativeMouseDevice*>(m_glfwWindow, this));
		glfwSetScrollCallback(m_glfwWindow, handleScrollInput);

		m_x = m_y = m_scroll = 0;
		return true;
	}

	bool NativeMouseDevice::VUpdate(void)
	{
		double xpos, ypos;
		glfwGetCursorPos(m_glfwWindow, &xpos, &ypos);

		m_x = xpos;
		m_y = ypos;

		if (m_shouldCage)
		{
			if (m_x < m_cageBounds_left)
			{
				m_x = m_cageBounds_left;
			}
			else if (m_x > m_cageBounds_right)
			{
				m_x = m_cageBounds_right;
			}

			if (m_y < m_cageBounds_top)
			{
				m_y = m_cageBounds_top;
			}
			else if (m_y > m_cageBounds_bottom)
			{
				m_y = m_cageBounds_bottom;
			}

			glfwSetCursorPos(m_glfwWindow, m_x, m_y);
		}

		m_relativeX = m_relativeY = m_relativeScrollWheel = 0;

		m_relativeX = m_x - m_lastX;
		m_relativeY = m_y - m_lastY;
		m_relativeScrollWheel = m_scroll - m_lastScroll;

		m_lastX = m_x;
		m_lastY = m_y;
		m_lastScroll = m_scroll;
		return true;
	}

	bool NativeMouseDevice::VIsPressed(MouseButton btnID) const
	{
		int state = glfwGetMouseButton(m_glfwWindow, NativeTypeConverter::To(btnID));
		return state == GLFW_PRESS;
	}

	Vector3<long> NativeMouseDevice::VGetRelativePosition() const
	{
		return Vector3<long>(m_relativeX, m_relativeY, m_relativeScrollWheel);
	}

	Vector3<long> NativeMouseDevice::VGetAbsolutePosition() const
	{
		double xpos, ypos;
		glfwGetCursorPos(m_glfwWindow, &xpos, &ypos);
		return Vector3<long>(xpos, ypos, m_scroll);
	}

	Vector3<long> NativeMouseDevice::VGetAbsolutePositionInsideWindow() const
	{
		double xpos, ypos;
		glfwGetCursorPos(m_glfwWindow, &xpos, &ypos);
		return Vector3<long>(xpos, ypos, m_scroll);
	}

	void NativeMouseDevice::VHideCursor()
	{
		glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

	void NativeMouseDevice::VShowCursor()
	{
		glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	bool NativeMouseDevice::VSetCursorPosition(int x, int y)
	{
		glfwSetCursorPos(m_glfwWindow, x, y);

		m_x = x;
		m_y = y;

		m_lastX = m_x;
		m_lastY = m_y;

		return true;
	}

	void NativeMouseDevice::VCageMouse(bool cage)
	{
		m_shouldCage = cage;
	}

	void NativeMouseDevice::VSetCage(long left, long top, long right, long bottom)
	{
		m_cageBounds_left = left;
		m_cageBounds_top = top;
		m_cageBounds_right = right;
		m_cageBounds_bottom = bottom;
	}

}
