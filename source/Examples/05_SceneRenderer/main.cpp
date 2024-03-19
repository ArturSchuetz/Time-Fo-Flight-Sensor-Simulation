#include <RenderDevice/BowRenderer.h>
#include <InputDevice/BowInput.h>
#include <Resources/BowResources.h>

#include <CoreSystems/BowBasicTimer.h>
#include "FirstPersonCamera.h"
#include "PbrtScene.h"

#include <iostream>

std::string vertexShader = R"(#version 140

in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_TexCoord;

out vec3 var_lightDir;
out vec3 var_Normal;

uniform mat4 u_ModelView;
uniform mat4 u_View;
uniform mat4 u_Proj;

void main(void)
{
    gl_Position = vec4(in_Position, 1.0) * u_ModelView * u_Proj; // multiplying a vector from the left to a matrix corresponds to multiplying it from the right to the transposed matrix
	
	vec3 light_position = vec4(vec4(500.0, 1000.0, 500.0, 0.0) * u_View).xyz;
	var_lightDir = normalize(light_position);

	var_Normal = vec4(vec4(in_Normal.xyz, 0.0) * u_ModelView).xyz;
}

)";

std::string fragmentShader = R"(#version 130
precision highp float; // needed only for version 1.30

in vec3 var_lightDir;
in vec3 var_Normal;

out vec3 out_Color;
 
void main(void)
{
    vec4 diffuseColor = vec4(1.0, 1.0, 1.0, 1.0);
	
	float NdotL = max(dot(var_Normal, var_lightDir), 0.0);

	if(diffuseColor.a < 0.1)
		discard;

	out_Color = (NdotL * diffuseColor.rgb);
	out_Color += (0.1 * diffuseColor.rgb);
})";

int main()
{
	// Creating Render Device
	bow::RenderDevicePtr deviceOGL = bow::RenderDeviceManager::GetInstance().GetOrCreateDevice(bow::RenderDeviceAPI::OpenGL3x);
	if (deviceOGL == nullptr)
	{
		return 0;
	}

	// Creating Window
	bow::GraphicsWindowPtr windowOGL = deviceOGL->VCreateWindow(800, 600, "Mesh Rendering Sample", bow::WindowType::Windowed);
	if (windowOGL == nullptr)
	{
		return 0;
	}
	bow::RenderContextPtr contextOGL = windowOGL->VGetContext();
	bow::ShaderProgramPtr shaderProgram = deviceOGL->VCreateShaderProgram(vertexShader, fragmentShader);

	///////////////////////////////////////////////////////////////////
	// Vertex Array from Mesh

	PbrtScene scene;
	scene.parseFile("C:/Users/Artur/Downloads/staircase2/scene.pbrt");

	std::vector<bow::MeshPtr> meshes = scene.GetMeshes();
	std::vector<bow::VertexArrayPtr> vertexArrays;
	for (unsigned int i = 0; i < meshes.size(); i++)
	{
		bow::MeshAttribute meshAttr = meshes[i]->CreateAttribute("in_Position", "in_Normal");
		vertexArrays.push_back(contextOGL->VCreateVertexArray(meshAttr, shaderProgram->VGetVertexAttributes(), bow::BufferHint::StaticDraw));
	}

	///////////////////////////////////////////////////////////////////
	// ClearState and Color

	bow::ClearState clearState;
	clearState.color = bow::ColorRGBA(0.392f, 0.584f, 0.929f, 1.0f);

	///////////////////////////////////////////////////////////////////
	// Camera

	bow::Vector3<float> Position = bow::Vector3<float>(0.0f, 0.0f, 0.0f);
	bow::Vector3<float> LookAt = bow::Vector3<float>(0.0f, 0.0f, 0.0f);
	bow::Vector3<float> UpVector = bow::Vector3<float>(0.0f, 1.0f, 0.0f);

	FirstPersonCamera camera = FirstPersonCamera(Position, LookAt, UpVector, windowOGL->VGetWidth(), windowOGL->VGetHeight());
	camera.SetClippingPlanes(0.001, 100.0);

	///////////////////////////////////////////////////////////////////
	// Input

	bow::KeyboardPtr keyboard = bow::InputDeviceManager::GetInstance().CreateKeyboardObject(windowOGL);
	if (keyboard == nullptr)
	{
		return false;
	}

	bow::MousePtr mouse = bow::InputDeviceManager::GetInstance().CreateMouseObject(windowOGL);
	if (mouse == nullptr)
	{
		return false;
	}

	///////////////////////////////////////////////////////////////////
	// RenderState

	bow::RenderState renderState;
	renderState.rasterizationMode = bow::RasterizationMode::Fill;
	renderState.faceCulling.Enabled = false;

	///////////////////////////////////////////////////////////////////
	// Gameloop

	bow::Matrix3D<float> worldMat;
	worldMat.Translate(bow::Vector3<float>(0.0f, 0.0f, 0.0f));

	bow::BasicTimer timer;
	float m_moveSpeed;
	bow::Vector3<long> lastCursorPosition;

	while (!windowOGL->VShouldClose())
	{
		// =======================================================
		// Handle Input

		timer.Update();

		keyboard->VUpdate();
		mouse->VUpdate();

		m_moveSpeed = 100.0;

		if (keyboard->VIsPressed(bow::Key::K_W))
		{
			if (keyboard->VIsPressed(bow::Key::K_LEFT_SHIFT))
			{
				camera.MoveForward(m_moveSpeed * (float)timer.GetDelta() * 2);
			}
			else
			{
				camera.MoveForward(m_moveSpeed * (float)timer.GetDelta());
			}
		}

		if (keyboard->VIsPressed(bow::Key::K_S))
		{
			camera.MoveBackward(m_moveSpeed * (float)timer.GetDelta());
		}

		if (keyboard->VIsPressed(bow::Key::K_D))
		{
			camera.MoveRight(m_moveSpeed * (float)timer.GetDelta());
		}

		if (keyboard->VIsPressed(bow::Key::K_A))
		{
			camera.MoveLeft(m_moveSpeed * (float)timer.GetDelta());
		}

		if (keyboard->VIsPressed(bow::Key::K_SPACE))
		{
			camera.MoveUp(m_moveSpeed * (float)timer.GetDelta());
		}

		if (keyboard->VIsPressed(bow::Key::K_LEFT_CONTROL))
		{
			camera.MoveDown(m_moveSpeed * (float)timer.GetDelta());
		}

		if (mouse->VIsPressed(bow::MouseButton::MOFS_BUTTON1))
		{
			windowOGL->VHideCursor();
			bow::Vector3<long> moveVec = mouse->VGetRelativePosition();
			camera.rotate((float)moveVec.x, (float)moveVec.y);
			mouse->VSetCursorPosition(lastCursorPosition.x, lastCursorPosition.y);
		}
		else
		{
			windowOGL->VShowCursor();
		}

		lastCursorPosition = mouse->VGetAbsolutePosition();

		// =======================================================
		// Render Frame

		contextOGL->VClear(clearState);

		contextOGL->VSetViewport(bow::Viewport(0, 0, windowOGL->VGetWidth(), windowOGL->VGetHeight()));
		camera.SetResolution(windowOGL->VGetWidth(), windowOGL->VGetHeight());

		shaderProgram->VSetUniform("u_ModelView", (bow::Matrix4x4<float>)camera.CalculateWorldView((bow::Matrix3D<float>)worldMat));
		shaderProgram->VSetUniform("u_View", (bow::Matrix4x4<float>)camera.CalculateView());
		shaderProgram->VSetUniform("u_Proj", (bow::Matrix4x4<float>)camera.CalculateProjection());

		for (unsigned int i = 0; i < vertexArrays.size(); i++)
		{
			contextOGL->VDraw(bow::PrimitiveType::Triangles, vertexArrays[i], shaderProgram, renderState);
		}
		contextOGL->VSwapBuffers();
	}
	return 0;
}