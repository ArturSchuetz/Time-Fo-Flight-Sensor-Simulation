#include <DirectX11RenderDevice/BowD3D11RenderDevice.h>
#include <CoreSystems/BowLogger.h>

namespace bow {

	extern "C" __declspec(dllexport) IRenderDevice* CreateRenderDevice(EventLogger& logger)
	{
		// set the new instance of the logger to prevent the creation of a new one inside this dll
		EventLogger::SetInstance(logger);
		D3DRenderDevice* device = new D3DRenderDevice();

		if (!device->Initialize())
		{
			delete device;
			return nullptr;
		}
		return device;
	}

}
