#include <OpenGL3xRenderDevice/Device/BowOGL3xGraphicsWindow.h>
#include <OpenGL3xRenderDevice/BowOGL3xRenderDevice.h>
#include <OpenGL3xRenderDevice/Device/BowOGL3xRenderContext.h>
#include <CoreSystems/BowLogger.h>

#include <GL/glew.h>
#if defined(_WIN32)
#include <GL/wglew.h>
#endif
#include <GLFW/glfw3.h>

#include <unordered_map>

namespace bow {

	std::unordered_map<GLFWwindow*, OGLGraphicsWindow*> g_Windows;

	OGLGraphicsWindow::OGLGraphicsWindow() :
		m_Context(nullptr)
	{

	}

	bool OGLGraphicsWindow::Initialize(unsigned int width, unsigned int height, const std::string& title, WindowType windowType, OGLRenderDevice *device)
	{
		// Create a fullscrenn window?
		GLFWmonitor* monitor = nullptr;
		if (windowType == WindowType::Fullscreen)
			monitor = glfwGetPrimaryMonitor();

		// Creating Window with a cool custom deleter
		m_Window = glfwCreateWindow(width, height, title.c_str(), monitor, NULL);

		if (g_Windows.find(m_Window) == g_Windows.end())
		{
			g_Windows.insert(std::pair<GLFWwindow*, OGLGraphicsWindow*>(m_Window, this));
		}

		if (!m_Window)
		{
			glfwTerminate();
			LOG_ERROR("Error while creating OpenGL-Window with glfw!");
			return false;
		}

		glfwGetWindowSize(m_Window, &m_Width, &m_Height);

		glfwSetWindowSizeCallback(m_Window, OGLGraphicsWindow::ResizeCallback);

		m_ParentDevice = device;
		m_Context = OGLRenderContextPtr(new OGLRenderContext(m_Window));

		LOG_TRACE("OpenGL-Window sucessfully initialized!");
		return m_Context->Initialize(m_ParentDevice);
	}


	OGLGraphicsWindow::~OGLGraphicsWindow(void)
	{
		VRelease();
	}


	void OGLGraphicsWindow::VRelease(void)
	{
		if (m_Context.get() != nullptr)
		{
			m_Context->VRelease();
			m_Context.reset();
		}
		glfwDestroyWindow(m_Window);
		if (g_Windows.find(m_Window) != g_Windows.end())
		{
			g_Windows.erase(m_Window);
		}
		m_Window = nullptr;

		LOG_TRACE("OGLGraphicsWindow released");
	}

	void* OGLGraphicsWindow::VGetWindowHandle() const
	{
		return m_Window;
	}

	RenderContextPtr OGLGraphicsWindow::VGetContext() const
	{
		return m_Context;
	}

	void OGLGraphicsWindow::VPollWindowEvents() const
	{
		glfwPollEvents();
	}

	void OGLGraphicsWindow::VSetWindowTitle(const char* title) const
	{
		glfwSetWindowTitle(m_Window, title);
	}

	void OGLGraphicsWindow::VHideCursor() const
	{
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

	void OGLGraphicsWindow::VShowCursor() const
	{

		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	int OGLGraphicsWindow::VGetWidth() const
	{
		return m_Width;
	}

	int OGLGraphicsWindow::VGetHeight() const
	{
		return m_Height;
	}

	bool OGLGraphicsWindow::VIsVisible() const
	{
		int visible = glfwGetWindowAttrib(m_Window, GLFW_VISIBLE);
		return visible == 1;
	}

	bool OGLGraphicsWindow::VIsFocused() const
	{
		int focused = glfwGetWindowAttrib(m_Window, GLFW_FOCUSED);
		return focused == 1;
	}

	bool OGLGraphicsWindow::VShouldClose() const
	{
		glfwPollEvents();
		return glfwWindowShouldClose(m_Window) != 0;
	}

	void OGLGraphicsWindow::ResizeCallback(GLFWwindow* window, int width, int height)
	{
		if (g_Windows.find(window) != g_Windows.end())
		{
			g_Windows[window]->m_Width = width;
			g_Windows[window]->m_Height = height;
		}
	}
}
