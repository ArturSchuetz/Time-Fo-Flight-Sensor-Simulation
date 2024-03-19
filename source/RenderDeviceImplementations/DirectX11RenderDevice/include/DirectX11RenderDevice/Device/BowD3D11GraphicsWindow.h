#pragma once
#include <DirectX11RenderDevice/DirectX11RenderDevice_api.h>
#include <DirectX11RenderDevice/BowD3D11RenderDevicePredeclares.h>

#include <RenderDevice/Device/BowGraphicsWindow.h>

namespace bow {

	typedef std::shared_ptr<class D3DRenderContext> D3DRenderContextPtr;
	class D3DRenderDevice;

	class D3DGraphicsWindow : public IGraphicsWindow
	{
		friend class D3DRenderContext;
	public:
		D3DGraphicsWindow();
		~D3DGraphicsWindow(void);

		// =========================================================================
		// INIT/RELEASE STUFF:
		// =========================================================================

		bool Initialize(unsigned int width, unsigned int height, const std::string& title, WindowType windowType, D3DRenderDevice *device);
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
		//you shall not direct
		D3DGraphicsWindow(D3DGraphicsWindow&) : m_Context(nullptr), m_ParentDevice(nullptr) {}
		D3DGraphicsWindow& operator=(const D3DGraphicsWindow&) { return *this; }

		void Resize(unsigned int width, unsigned int height);
		void SetFullscreen(bool fullscreen);

		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		D3DRenderDevice*	m_ParentDevice;
		D3DRenderContextPtr m_Context;

		unsigned int m_width;
		unsigned int m_height;

		HWND m_hwnd;
		RECT m_windowRect;
		bool m_shouldClose;
		bool m_fullScreen;
	};

	typedef std::shared_ptr<D3DGraphicsWindow> D3DGraphicsWindowPtr;
	typedef std::unordered_map<unsigned int, D3DGraphicsWindowPtr> D3DGraphicsWindowMap;
}
