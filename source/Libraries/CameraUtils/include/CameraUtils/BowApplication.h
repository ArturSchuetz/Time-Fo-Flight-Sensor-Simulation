#pragma once
#include "CameraUtils/CameraUtils_api.h"

#include "CoreSystems/BowCoreSystems.h"
#include "InputDevice/BowInput.h"
#include "RenderDevice/BowRenderer.h"

#include "CameraUtils/CameraCalibration.h"
#include "CameraUtils/PCLRenderer.h"

namespace bow {

	class CAMERAUTILS_API Application
	{
	public:
		Application(void);
		virtual ~Application(void);

		void Run(bow::IntrinsicCameraParameters cameraParameters);
		void Run_Visible_Only(bow::IntrinsicCameraParameters cameraParameters);


	protected:
		virtual std::string GetWindowTitle(void) { return "Application"; }
		virtual void OnInit(bow::IntrinsicCameraParameters cameraParameters) {}
		virtual void OnResized(unsigned int newWidth, unsigned int newHeight) {}
		virtual void OnUpdate(double deltaTime) {}
		virtual void OnRender(void) {}
		virtual void OnRelease(void) {}

		void UpdateColorBuffer(void* image, unsigned int width, unsigned int height, ImageFormat imageFormat, ImageDatatype imageDatatype);
		void UpdateIRBuffer(void* image, unsigned int width, unsigned int height, ImageFormat imageFormat, ImageDatatype imageDatatype);
		void UpdateDepthBuffer(void* image, float max_distance, unsigned int width, unsigned int height, ImageFormat imageFormat, ImageDatatype imageDatatype);

		unsigned int GetWidth() { return m_width; }
		unsigned int GetHeight() { return m_height; }
		void DisplayFramerate(double framerate);

		RenderDevicePtr		m_device;
		GraphicsWindowPtr	m_window;
		KeyboardPtr			m_keyboard;
		MousePtr			m_mouse;

	private:
		void Init(unsigned int width, unsigned int height);

		unsigned int	m_width;
		unsigned int	m_height;

		BasicTimer		m_timer;
		Texture2DPtr	m_colorTexture;
		Texture2DPtr	m_irTexture;
		Texture2DPtr	m_depthTexture;

		PCLRenderer*	m_pcl_renderer;
		double			m_frametime;
	};
}
