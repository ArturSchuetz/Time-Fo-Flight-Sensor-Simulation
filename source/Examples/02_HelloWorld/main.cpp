#include <RenderDevice/BowRenderer.h>
#include <InputDevice/BowInput.h>

#include <CoreSystems/BowLogger.h>

#include <iostream>

int main(int /*argc*/, char* /*argv[]*/)
{
	// Creating Render Device
	bow::RenderDevicePtr deviceOGL = bow::RenderDeviceManager::GetInstance().GetOrCreateDevice(bow::RenderDeviceAPI::OpenGL3x);
	if (deviceOGL == nullptr)
	{
		std::cout << "Could not create device!" << std::endl;
		return -1;
	}

	// Creating Window
	bow::GraphicsWindowPtr windowOGL = deviceOGL->VCreateWindow(800, 600, "HelloWorld", bow::WindowType::Windowed);
	if (windowOGL == nullptr)
	{
		std::cout << "Could not create window!" << std::endl;
		return -1;
	}

	// Change ClearColor
	bow::ClearState clearState;
	clearState.color = bow::ColorRGBA(0.392f, 0.584f, 0.929f, 1.0f);

	///////////////////////////////////////////////////////////////////
	// Input

	bow::KeyboardPtr keyboard = bow::InputDeviceManager::GetInstance().CreateKeyboardObject(windowOGL);
	if (keyboard == nullptr)
	{
		return false;
	}

	///////////////////////////////////////////////////////////////////
	// Gameloop
	auto contextOGL = windowOGL->VGetContext();

	while (!windowOGL->VShouldClose())
	{
		keyboard->VUpdate();
		if (keyboard->VIsPressed(bow::Key::K_ESCAPE))
			break;

		// Clear Backbuffer to our ClearState
		contextOGL->VClear(clearState);

		contextOGL->VSwapBuffers();

		windowOGL->VPollWindowEvents();
	}
	return 0;
}
