#pragma once
#include "InputDevice/InputDevice_api.h"
#include "InputDevice/BowInputPredeclares.h"

#include "RenderDevice/Device/BowGraphicsWindow.h"

namespace bow {

	enum class InputDeviceAPI : char
	{
		NativeInput,
		DirectInput
	};

	//! \brief InputDeviceManager is a singleton and creates input devices. 
	class InputDeviceManager
	{
	public:
		~InputDeviceManager(void);

		static InputDeviceManager& GetInstance();

		KeyboardPtr CreateKeyboardObject(GraphicsWindowPtr window);
		MousePtr CreateMouseObject(GraphicsWindowPtr window);

	protected:
		InputDeviceManager() {}

	private:
		InputDevicePtr GetOrCreateDevice(InputDeviceAPI api);
		void ReleaseDevice(InputDeviceAPI api);

		InputDeviceManager(const InputDeviceManager&) {}; //!< You shall not copy
		InputDeviceManager& operator=(const InputDeviceManager&) { return *this; }
	};
}
