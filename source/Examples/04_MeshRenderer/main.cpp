#include <Masterthesis/cuda_config.h>
#include <RenderDevice/BowRenderer.h>
#include <InputDevice/BowInput.h>
#include <Resources/BowResources.h>

#include <CoreSystems/BowBasicTimer.h>
#include "FirstPersonCamera.h"

#include <iostream>

std::string vertexShader = R"(#version 140

in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_TexCoord;

out vec3 var_lightDir;
out vec3 var_Normal;
out vec3 var_Position;

uniform mat4 u_ModelView;
uniform mat4 u_View;
uniform mat4 u_Proj;

void main(void)
{
    gl_Position = vec4(in_Position, 1.0) * u_ModelView * u_Proj; // multiplying a vector from the left to a matrix corresponds to multiplying it from the right to the transposed matrix
	
	vec3 light_position = vec4(vec4(500.0, 1000.0, 500.0, 0.0) * u_View).xyz;
	var_lightDir = normalize(light_position);

	var_Normal = vec4(vec4(in_Normal.xyz, 0.0) * u_ModelView).xyz;
	var_Position = vec4(vec4(in_Position.xyz, 1.0) * u_ModelView).xyz;
})";

std::string fragmentShader = R"(#version 130
precision highp float; // needed only for version 1.30

in vec3 var_lightDir;
in vec3 var_Normal;
in vec3 var_Position;

out vec3 out_Color;
 
uniform vec4 u_diffuseColor = vec4(1.0, 1.0, 1.0, 1.0);
uniform vec3 u_emissive = vec3(0.0, 0.0, 0.0);

void main(void)
{
	if(u_diffuseColor.a < 0.1)
		discard;

    vec3 lightColor = vec3(1.0, 1.0, 1.0);
	
	vec3 diffuse = vec3(0.0, 0.0, 0.0);
	//diffuse += u_diffuseColor.rgb * max(dot(var_Normal, var_lightDir), 0.0);
	diffuse += u_diffuseColor.rgb * ((max(dot(var_Normal, normalize(-var_Position)), 0.0) / length(var_Position)) * lightColor);
	diffuse += u_emissive.rgb;

	out_Color = diffuse;
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

	bow::MeshPtr mesh = bow::MeshManager::GetInstance().Load(std::string(PROJECT_BASE_DIR) + std::string("/data/Scenes/Sponza/sponza.obj"));
	std::vector<bow::SubMesh*> subMeshes = mesh->GetSubMeshes();
	
	std::vector<bow::MaterialCollectionPtr> materialCollections;
	std::vector<std::string> materialFiles = mesh->GetMaterialFiles();

	std::map<unsigned int, bow::Material*> materials;
	for (unsigned int i = 0; i < materialFiles.size(); i++)
	{
		bow::MaterialCollectionPtr materialCollection = bow::MaterialManager::GetInstance().Load(materialFiles[i]);
		if (materialCollection != nullptr)
		{
			std::size_t foundPos = materialCollection->VGetName().find_last_of("/");
			std::string filePath = "";
			if (foundPos >= 0)
			{
				filePath = materialCollection->VGetName().substr(0, foundPos + 1);
			}

			for (unsigned int i = 0; i < subMeshes.size(); i++)
			{
				bow::Material* material = materialCollection->GetMaterial(subMeshes[i]->GetMaterialName());
				if (material != nullptr)
				{
					materials.insert(std::pair<unsigned int, bow::Material*>(i, material));
				}
			}
		}
	}

	bow::Vector3<float> bbox_min;
	bow::Vector3<float> bbox_max;
	mesh->GetBoundigBox(bbox_min, bbox_max);

	bow::MeshAttribute meshAttr = mesh->CreateAttribute("in_Position", "in_Normal");
	bow::VertexArrayPtr vertexArray = contextOGL->VCreateVertexArray(meshAttr, shaderProgram->VGetVertexAttributes(), bow::BufferHint::StaticDraw);

	///////////////////////////////////////////////////////////////////
	// ClearState and Color

	bow::ClearState clearState;
	clearState.color = bow::ColorRGBA(0.392f, 0.584f, 0.929f, 1.0f);

	///////////////////////////////////////////////////////////////////
	// Camera

	bow::Vector3<float> Position = (bbox_min + bbox_max) * 0.5f;
	bow::Vector3<float> LookAt = Position + bow::Vector3<float>(1.0f, 0.0f, 0.0f);
	bow::Vector3<float> UpVector = bow::Vector3<float>(0.0f, 1.0f, 0.0f);

	FirstPersonCamera camera = FirstPersonCamera(Position, LookAt, UpVector, windowOGL->VGetWidth(), windowOGL->VGetHeight());
	camera.SetClippingPlanes(0.1, 10000.0);

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
		// Render Frame

		contextOGL->VClear(clearState);

		contextOGL->VSetViewport(bow::Viewport(0, 0, windowOGL->VGetWidth(), windowOGL->VGetHeight()));
		camera.SetResolution(windowOGL->VGetWidth(), windowOGL->VGetHeight());

		shaderProgram->VSetUniform("u_ModelView", (bow::Matrix4x4<float>)camera.CalculateWorldView((bow::Matrix3D<float>)worldMat));
		shaderProgram->VSetUniform("u_View", (bow::Matrix4x4<float>)camera.CalculateView());
		shaderProgram->VSetUniform("u_Proj", (bow::Matrix4x4<float>)camera.CalculateProjection());

		for (unsigned int i = 0; i < subMeshes.size(); i++)
		{
			if (materials[i] != nullptr)
			{
				shaderProgram->VSetUniform("u_diffuseColor", bow::Vector4<float>(materials[i]->diffuse[0], materials[i]->diffuse[1], materials[i]->diffuse[2], materials[i]->dissolve));
				shaderProgram->VSetUniform("u_emissive", bow::Vector3<float>(materials[i]->emission[0], materials[i]->emission[1], materials[i]->emission[2]));
			}
			else
			{
				shaderProgram->VSetUniform("u_diffuseColor", bow::Vector4<float>(1.0f, 0.0f, 1.0f, 1.0f));
				shaderProgram->VSetUniform("u_emissive", bow::Vector3<float>(0.0f, 0.0f, 0.0f));
			}
			contextOGL->VDraw(bow::PrimitiveType::Triangles, subMeshes[i]->GetStartIndex(), subMeshes[i]->GetNumIndices(), vertexArray, shaderProgram, renderState);
		}

		contextOGL->VSwapBuffers();

		// =======================================================

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
	}
	return 0;
}