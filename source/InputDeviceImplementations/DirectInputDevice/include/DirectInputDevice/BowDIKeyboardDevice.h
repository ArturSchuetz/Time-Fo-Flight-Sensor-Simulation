#pragma once
#include "DirectInputDevice/DirectInputDevice_api.h"
#include "DirectInputDevice/BowDIInputDeviceBase.h"

#include "InputDevice/IBowKeyboard.h"

#include <dinput.h>

namespace bow {

	class DIKeyboardDevice : public DIInputDeviceBase, public IKeyboard
	{
	public:
		DIKeyboardDevice(IDirectInput8 *directInput, HWND windowHandle);
		~DIKeyboardDevice(void);

		bool Initialize(void);
		bool VUpdate(void);
		bool VIsPressed(Key keyID) const;

	protected:
		char m_keys[256];
	};

	typedef std::shared_ptr<DIKeyboardDevice> DIKeyboardPtr;
}
