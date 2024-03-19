#pragma once
#include "DirectInputDevice/DirectInputDevice_api.h"

#include "InputDevice/IBowKeyboard.h"
#include "InputDevice/IBowMouse.h"

#include <dinput.h>

namespace bow {

	class DITypeConverter
	{
	public:
		static unsigned int To(Key key);
		static unsigned int To(MouseButton key);
	};

}
