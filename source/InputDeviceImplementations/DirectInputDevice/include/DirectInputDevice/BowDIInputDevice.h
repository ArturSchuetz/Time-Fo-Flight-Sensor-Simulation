#pragma once
#include "DirectInputDevice/DirectInputDevice_api.h"

#include "InputDevice/IBowInputDevice.h"

#include <dinput.h>

namespace bow {

	class DIInputDevice : public IInputDevice
	{
	public:
		DIInputDevice();
		~DIInputDevice(void);

		bool Initialize();
		void VRelease(void) sealed;

		MousePtr	VCreateMouseObject(GraphicsWindowPtr window);
		KeyboardPtr VCreateKeyboardObject(GraphicsWindowPtr window);

	private:
		IDirectInput8 *m_pDirectInput;
	};

}
