#pragma once
#include <NativeInputDevice/NativeInputDevice_api.h>

#include <InputDevice/IBowKeyboard.h>
#include <InputDevice/IBowMouse.h>

#include <GLFW/glfw3.h>

namespace bow {

	class NativeTypeConverter
	{
	public:
		static unsigned int To(Key key);
		static unsigned int To(MouseButton key);
	};

}
