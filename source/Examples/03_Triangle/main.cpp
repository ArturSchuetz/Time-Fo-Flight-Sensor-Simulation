#include <RenderDevice/BowRenderer.h>
#include <CoreSystems/BowBasicTimer.h>

#include <iostream>

std::string vertexShader = R"(#version 130
uniform vec4 offset;

in vec4 in_Position;
in vec4 in_Color;

out vec4 var_color;
 
void main(void)
{
    gl_Position = in_Position + offset;
	var_color = in_Color;
}
)";

std::string fragmentShader = R"(#version 130
 
in vec4 var_color;

out vec3 out_Color;

void main(void)
{
	out_Color = var_color.rgb;
}
)";

int main()
{
	bow::RenderDeviceAPI deviceApi = bow::RenderDeviceAPI::OpenGL3x;

	// Creating Render Device
	bow::RenderDevicePtr deviceOGL = bow::RenderDeviceManager::GetInstance().GetOrCreateDevice(deviceApi);
	if (deviceOGL == nullptr)
	{
		std::cout << "Could not create device!" << std::endl;
		return 1;
	}

	// Creating Window
	bow::GraphicsWindowPtr windowOGL = deviceOGL->VCreateWindow(800, 600, "Triangle Sample", bow::WindowType::Windowed);
	if (windowOGL == nullptr)
	{
		std::cout << "Could not create window!" << std::endl;
		return 1;
	}

	bow::RenderContextPtr contextOGL = windowOGL->VGetContext();
	bow::ShaderProgramPtr shaderProgram = deviceOGL->VCreateShaderProgram(vertexShader, fragmentShader);

	///////////////////////////////////////////////////////////////////
	// ClearState and Color

	bow::ClearState clearState;
	clearState.color = bow::ColorRGBA(0.392f, 0.584f, 0.929f, 1.0f);

	///////////////////////////////////////////////////////////////////
	// Vertex Array

	float aspectRatio = static_cast<float>(800) / static_cast<float>(600);

	bow::Vector3<float> vertices[3];
	vertices[0] = bow::Vector3<float>(0.0f, 0.25f * aspectRatio, 0.0f);
	vertices[1] = bow::Vector3<float>(0.25f, -0.25f * aspectRatio, 0.0f);
	vertices[2] = bow::Vector3<float>(-0.25f, -0.25f * aspectRatio, 0.0f);

	// fill buffer with informations
	bow::VertexBufferPtr positionsBuffer = deviceOGL->VCreateVertexBuffer(bow::BufferHint::StaticDraw, sizeof(bow::Vector3<float>) * 3);
	positionsBuffer->VCopyFromSystemMemory(vertices, 0, sizeof(bow::Vector3<float>) * 3);
	bow::VertexBufferAttributePtr positionAttribute = bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(positionsBuffer, bow::ComponentDatatype::Float, 3));

	bow::Vector4<float> colors[3];
	colors[0] = bow::Vector4<float>(1.0f, 0.0f, 0.0f, 1.0f);
	colors[1] = bow::Vector4<float>(0.0f, 1.0f, 0.0f, 1.0f);
	colors[2] = bow::Vector4<float>(0.0f, 0.0f, 1.0f, 1.0f);

	bow::VertexBufferPtr colorBuffer = deviceOGL->VCreateVertexBuffer(bow::BufferHint::StaticDraw, sizeof(bow::Vector4<float>) * 3);
	colorBuffer->VCopyFromSystemMemory(colors, 0, sizeof(bow::Vector4<float>) * 3);
	bow::VertexBufferAttributePtr colorsAttribute = bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(colorBuffer, bow::ComponentDatatype::Float, 4));

	// create VertexArray
	bow::VertexArrayPtr vertexArray = contextOGL->VCreateVertexArray();

	// connect buffer with location in shader
	if (deviceApi == bow::RenderDeviceAPI::OpenGL3x)
	{
		vertexArray->VSetAttribute(shaderProgram->VGetVertexAttribute("in_Position"), positionAttribute);
		vertexArray->VSetAttribute(shaderProgram->VGetVertexAttribute("in_Color"), colorsAttribute);
	}
	else if (deviceApi == bow::RenderDeviceAPI::DirectX12)
	{
		vertexArray->VSetAttribute(shaderProgram->VGetVertexAttribute("POSITION0"), positionAttribute);
		vertexArray->VSetAttribute(shaderProgram->VGetVertexAttribute("COLOR0"), colorsAttribute);
	}

	///////////////////////////////////////////////////////////////////
	// RenderState

	bow::RenderState renderState;
	renderState.faceCulling.Enabled = false;
	renderState.depthTest.Enabled = false;

	bow::BasicTimer timer;
	bow::Vector4<float> offset(0.0, 0.0, 0.0, 0.0);
	while (!windowOGL->VShouldClose())
	{
		timer.Update();
		offset.x += 0.01f * timer.GetDelta();

		// Clear Backbuffer to our ClearState
		contextOGL->VClear(clearState);

		contextOGL->VSetViewport(bow::Viewport(0, 0, windowOGL->VGetWidth(), windowOGL->VGetHeight()));

		shaderProgram->VSetUniform("offset", offset);
		contextOGL->VDraw(bow::PrimitiveType::Triangles, vertexArray, shaderProgram, renderState);

		contextOGL->VSwapBuffers();
	}

	return 0;
}