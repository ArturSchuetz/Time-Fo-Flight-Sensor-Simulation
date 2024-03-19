#include <iostream>
#include <CoreSystems/BowLogger.h>

#include <RenderDevice/BowRenderer.h>
#include <InputDevice/BowInput.h>

int main(int /*argc*/, char* /*argv[]*/)
{
	bow::RenderDevicePtr renderDevice = bow::RenderDeviceManager::GetInstance().GetOrCreateDevice(bow::RenderDeviceAPI::OpenGL3x);
	if(renderDevice == nullptr)
	{
		LOG_ERROR("Failed to create Render Device!");
		return 1;
	}

	bow::GraphicsWindowPtr window = renderDevice->VCreateWindow();
	if (window == nullptr)
	{
		LOG_ERROR("Failed to create Window!");
		return 1;
	}

	bow::KeyboardPtr keyboard = bow::InputDeviceManager::GetInstance().CreateKeyboardObject(window);
	bow::MousePtr mouse = bow::InputDeviceManager::GetInstance().CreateMouseObject(window);

	std::cout << std::endl;

	while (!window->VShouldClose())
	{
		keyboard->VUpdate();
		mouse->VUpdate();

		bow::Vector3<long> mousePos = mouse->VGetAbsolutePositionInsideWindow();
		std::cout << "x: " << mousePos.x << " y: " << mousePos.y << " ";

		if (keyboard->VIsPressed(bow::Key::K_UP))
		{
			std::cout << "UP\t\t\t\t";
		}
		else if (keyboard->VIsPressed(bow::Key::K_DOWN))
		{
			std::cout << "DOWN\t\t\t\t";
		}
		else if (keyboard->VIsPressed(bow::Key::K_LEFT))
		{
			std::cout << "LEFT\t\t\t\t";
		}
		else if (keyboard->VIsPressed(bow::Key::K_RIGHT))
		{
			std::cout << "RIGHT\t\t\t\t";
		}
		else if (mouse->VIsPressed(bow::MouseButton::MOFS_BUTTON_LEFT))
		{
			std::cout << "LEFT Mouse Button\t\t";
		}
		else if (mouse->VIsPressed(bow::MouseButton::MOFS_BUTTON_RIGHT))
		{
			std::cout << "RIGHT Mouse Button\t\t";
		}
		else if (mouse->VIsPressed(bow::MouseButton::MOFS_BUTTON_MIDDLE))
		{
			std::cout << "MIDDLE Mouse Button\t\t";
		}
		else
		{
			std::cout << "\t\t\t\t\t";
		}
		std::cout << "\r";
	}
	return 0;
}
