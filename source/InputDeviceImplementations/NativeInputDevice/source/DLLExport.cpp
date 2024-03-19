#include <NativeInputDevice/BowNativeInputDevice.h>
#include <CoreSystems/BowLogger.h>

namespace bow {

	extern "C" __declspec(dllexport) IInputDevice* CreateInputDevice(EventLogger& logger)
	{
		// set the new instance of the logger to prevent the creation of a new one inside this dll
		EventLogger::SetInstance(logger);
		NativeInputDevice* device = new NativeInputDevice();

		if (!device->Initialize())
		{
			delete device;
			return nullptr;
		}
		return device;
	}

}
