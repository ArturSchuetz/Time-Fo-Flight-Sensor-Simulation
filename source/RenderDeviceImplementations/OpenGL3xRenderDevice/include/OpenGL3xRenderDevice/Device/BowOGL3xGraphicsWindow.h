#pragma once
#include <RenderDevice/Device/BowGraphicsWindow.h>

struct GLFWwindow;

namespace bow {

	typedef std::shared_ptr<class OGLRenderContext> OGLRenderContextPtr;
	class OGLRenderDevice;

	class OGLGraphicsWindow : public IGraphicsWindow
	{
	public:
		OGLGraphicsWindow();
		~OGLGraphicsWindow(void);

		// =========================================================================
		// INIT/RELEASE STUFF:
		// =========================================================================
		bool Initialize(unsigned int width, unsigned int height, const std::string& title, WindowType windowType, OGLRenderDevice *device);
		void VRelease(void);

		RenderContextPtr VGetContext() const;
		void VPollWindowEvents() const;

		void* VGetWindowHandle() const;
		void VSetWindowTitle(const char* title) const;

		void VHideCursor() const;
		void VShowCursor() const;

		int VGetWidth() const;
		int VGetHeight() const;

		bool VIsVisible() const;
		bool VIsFocused() const;
		bool VShouldClose() const;

	private:
		static void ResizeCallback(GLFWwindow* window, int width, int height);

		//you shall not direct
		OGLGraphicsWindow(OGLGraphicsWindow&) : m_Context(nullptr), m_ParentDevice(nullptr) {}
		OGLGraphicsWindow& operator=(const OGLGraphicsWindow&) { return *this; }

		OGLRenderDevice*	m_ParentDevice;
		OGLRenderContextPtr m_Context;
		GLFWwindow			*m_Window;

		int m_Width;
		int m_Height;
	};

	typedef std::shared_ptr<OGLGraphicsWindow> OGLGraphicsWindowPtr;
	typedef std::unordered_map<unsigned int, OGLGraphicsWindowPtr> OGLGraphicsWindowMap;
}
