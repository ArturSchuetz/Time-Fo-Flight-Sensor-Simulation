#pragma once
#include <NativeInputDevice/NativeInputDevice_api.h>

#include <InputDevice/IBowInputDevice.h>

namespace bow {

	class NativeInputDevice : public IInputDevice
	{
	public:
		NativeInputDevice();
		~NativeInputDevice(void);

		bool Initialize();
		void VRelease(void) sealed;

		MousePtr	VCreateMouseObject(GraphicsWindowPtr window);
		KeyboardPtr VCreateKeyboardObject(GraphicsWindowPtr window);

	private:

	};

}
