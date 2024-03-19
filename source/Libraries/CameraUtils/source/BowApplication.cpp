#include "CameraUtils/BowApplication.h"

#include "CoreSystems/BowLogger.h"

#include <iostream>

const std::string g_vertexColorShader = R"(#version 130

in vec3 in_Position;
in vec2 in_TexCoord;

out vec2 var_TextureCoord;

void main(void)
{
    gl_Position = vec4(in_Position, 1.0);
    var_TextureCoord = in_TexCoord;
}
)";

const std::string g_fragmentColorShader = R"(#version 130
precision highp float; // needed only for version 1.30

in vec2 var_TextureCoord;
uniform sampler2D inputTexture;

out vec4 out_Color;

void main(void)
{
    out_Color = vec4( texture(inputTexture, vec2(var_TextureCoord.x, 1.0 - var_TextureCoord.y)).rgb, 1.0);
}
)";


const std::string g_vertexGreyscaleShader = R"(#version 130

in vec3 in_Position;
in vec2 in_TexCoord;

out vec2 var_TextureCoord;

void main(void)
{
    gl_Position = vec4(in_Position, 1.0);
    var_TextureCoord = in_TexCoord;
}
)";

const std::string g_fragmentGreyscaleShader = R"(#version 130
precision highp float; // needed only for version 1.30

in vec2 var_TextureCoord;
uniform sampler2D inputTexture;

out vec3 out_Color;

void main(void)
{
    out_Color = texture(inputTexture, vec2(var_TextureCoord.x, 1.0 - var_TextureCoord.y)).rrr;
}
)";


namespace bow
{
	cv::Mat_<cv::Vec4f>	g_direction_vectors;

	Application::Application(void) : m_pcl_renderer(nullptr), m_frametime(0.0)
	{

	}


	Application::~Application(void)
	{
		if (m_pcl_renderer != nullptr)
		{
			delete m_pcl_renderer;
			m_pcl_renderer = nullptr;
		}
	}


	void Application::Init(unsigned int width, unsigned int height)
	{
		// Creating Render Device
		m_device = bow::RenderDeviceManager::GetInstance().GetOrCreateDevice(bow::RenderDeviceAPI::OpenGL3x);
		if (m_device == nullptr)
		{
			std::cout << "Could not create device!" << std::endl;
			return;
		}

		// Creating Window
		m_window = m_device->VCreateWindow(width, height, GetWindowTitle(), bow::WindowType::Windowed);
		if (m_window == nullptr)
		{
			std::cout << "Could not create window!" << std::endl;
			return;
		}

		///////////////////////////////////////////////////////////////////
		// Input

		m_keyboard = bow::InputDeviceManager::GetInstance().CreateKeyboardObject(m_window);
		if (m_keyboard == nullptr)
		{
			return;
		}

		m_mouse = bow::InputDeviceManager::GetInstance().CreateMouseObject(m_window);
		if (m_mouse == nullptr)
		{
			return;
		}
	}


	void Application::UpdateColorBuffer(void* image, unsigned int width, unsigned int height, ImageFormat imageFormat, ImageDatatype imageDatatype)
	{
		if (g_direction_vectors.cols != width || g_direction_vectors.rows != height)
		{
			LOG_FATAL("Depth resolution was changed! Please do not use a resolution that differs from camera calculation parameters!");
		}
		else
		{
			std::vector<bow::Vector3<float>> colors(g_direction_vectors.cols * g_direction_vectors.rows);
			if (imageDatatype == ImageDatatype::UnsignedByte)
			{
				for (unsigned int i = 0; i < g_direction_vectors.cols * g_direction_vectors.rows; i++)
				{
					if (imageFormat == ImageFormat::RedGreenBlue)
					{
						unsigned char* color = &((unsigned char*)image)[i * 3];
						colors[i] = bow::Vector3<float>((float)color[0] / 255.0f, (float)color[1] / 255.0f, (float)color[2] / 255.0f);
					}
					else if (imageFormat == ImageFormat::RedGreenBlueAlpha)
					{
						unsigned char* color = &((unsigned char*)image)[i * 4];
						colors[i] = bow::Vector3<float>((float)color[0] / 255.0f, (float)color[1] / 255.0f, (float)color[2] / 255.0f);
					}
				}
			}
			else if (imageDatatype == ImageDatatype::Float)
			{
				for (unsigned int i = 0; i < g_direction_vectors.cols * g_direction_vectors.rows; i++)
				{
					if (imageFormat == ImageFormat::RedGreenBlue)
					{
						float* color = &((float*)image)[i * 3];
						colors[i] = bow::Vector3<float>(color[0], color[1], color[2]);
					}
					else if (imageFormat == ImageFormat::RedGreenBlueAlpha)
					{
						float* color = &((float*)image)[i * 4];
						colors[i] = bow::Vector3<float>(color[0], color[1], color[2]);
					}
				}
			}
			if (m_pcl_renderer != nullptr)
				m_pcl_renderer->UpdateColors(colors);
		}
		
		Texture2DDescription textureDescription = m_colorTexture->VGetDescription();

		if(textureDescription.GetWidth() != width || textureDescription.GetHeight() != height)
			m_colorTexture = m_device->VCreateTexture2D(bow::Texture2DDescription(width, height, bow::TextureFormat::RedGreenBlue8, false));

		m_colorTexture->VCopyFromSystemMemory(image, imageFormat, imageDatatype);
	}


	void Application::UpdateIRBuffer(void* image, unsigned int width, unsigned int height, ImageFormat imageFormat, ImageDatatype imageDatatype)
	{
		Texture2DDescription textureDescription = m_irTexture->VGetDescription();

		if (textureDescription.GetWidth() != width || textureDescription.GetHeight() != height)
			m_irTexture = m_device->VCreateTexture2D(bow::Texture2DDescription(width, height, bow::TextureFormat::Red8, false));

		m_irTexture->VCopyFromSystemMemory(image, imageFormat, imageDatatype);
	}


	void Application::UpdateDepthBuffer(void* image, float max_distance, unsigned int width, unsigned int height, ImageFormat imageFormat, ImageDatatype imageDatatype)
	{
		if (imageDatatype == ImageDatatype::Float)
		{
			if (g_direction_vectors.cols != width || g_direction_vectors.rows != height)
			{
				LOG_FATAL("Depth resolution was changed! Please do not use a resolution that differs from camera calculation parameters!");
			}
			else
			{
				std::vector<bow::Vector3<float>> points(g_direction_vectors.cols * g_direction_vectors.rows);
				for (unsigned int i = 0; i < g_direction_vectors.cols * g_direction_vectors.rows; i++)
				{
					cv::Vec4f vector = g_direction_vectors.at<cv::Vec4f>(i);
					bow::Vector3<float> dir = bow::Vector3<float>(vector[0], vector[1], vector[2]).Normalized();
					bow::Vector3<float> point = dir * ((float*)image)[i];
					((float*)image)[i] = ((float*)image)[i] / max_distance;
					points[i] = point;
				}

				if (m_pcl_renderer != nullptr)
					m_pcl_renderer->UpdatePointCloud(points);
			}
		}

		Texture2DDescription textureDescription = m_depthTexture->VGetDescription();

		if (textureDescription.GetWidth() != width || textureDescription.GetHeight() != height)
			m_depthTexture = m_device->VCreateTexture2D(bow::Texture2DDescription(width, height, bow::TextureFormat::Red8, false));

		m_depthTexture->VCopyFromSystemMemory(image, imageFormat, imageDatatype);
	}


	void Application::Run(bow::IntrinsicCameraParameters cameraParameters)
	{
		g_direction_vectors = bow::CameraCalibration::calculate_directionMatrix(cameraParameters, cameraParameters.image_width, cameraParameters.image_height);

		m_width = cameraParameters.image_width;
		m_height = cameraParameters.image_height;

		Init(m_width * 3, m_height);

		auto contextOGL = m_window->VGetContext();

		///////////////////////////////////////////////////////////////////
		// compile shader

		bow::ShaderProgramPtr colorShaderProgram = m_device->VCreateShaderProgram(g_vertexColorShader, g_fragmentColorShader);
		bow::ShaderProgramPtr greyscaleShaderProgram = m_device->VCreateShaderProgram(g_vertexGreyscaleShader, g_fragmentGreyscaleShader);

		///////////////////////////////////////////////////////////////////
		// Vertex Array

			///////////////////////////////////////////////////////////////////
			// Define Triangle by Hand

			bow::MeshAttribute mesh;

			// Add Positions
			bow::VertexAttributeFloatVec3 *positionsAttribute = new bow::VertexAttributeFloatVec3("in_Position", 3);
			positionsAttribute->Values[0] = bow::Vector3<float>(-1.0f, -1.0f, 0.0f);
			positionsAttribute->Values[1] = bow::Vector3<float>(3.0f, -1.0f, 0.0f);
			positionsAttribute->Values[2] = bow::Vector3<float>(-1.0f, 3.0f, 0.0f);

			mesh.AddAttribute(bow::VertexAttributePtr(positionsAttribute));

			// Add TextureCoordinates
			bow::VertexAttributeFloatVec2 *texCoordAttribute = new bow::VertexAttributeFloatVec2("in_TexCoord", 3);
			texCoordAttribute->Values[0] = bow::Vector2<float>(0.0f, 0.0f);
			texCoordAttribute->Values[1] = bow::Vector2<float>(2.0f, 0.0f);
			texCoordAttribute->Values[2] = bow::Vector2<float>(0.0f, 2.0f);

			mesh.AddAttribute(bow::VertexAttributePtr(texCoordAttribute));

			bow::VertexArrayPtr vertexArray = contextOGL->VCreateVertexArray(mesh, colorShaderProgram->VGetVertexAttributes(), bow::BufferHint::StaticDraw);

		///////////////////////////////////////////////////////////////////
		// RenderState

		bow::RenderState renderState;
		renderState.faceCulling.Enabled = false;
		renderState.depthTest.Enabled = false;
		renderState.rasterizationMode = bow::RasterizationMode::Fill;

		// Change ClearColor
		bow::ClearState clearState;
		clearState.color = bow::ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f);

		///////////////////////////////////////////////////////////////////
		// Prepare render Texture

		m_colorTexture = m_device->VCreateTexture2D(bow::Texture2DDescription(m_width, m_height, bow::TextureFormat::RedGreenBlue8, false));
		m_irTexture = m_device->VCreateTexture2D(bow::Texture2DDescription(m_width, m_height, bow::TextureFormat::Red8, false));
		m_depthTexture = m_device->VCreateTexture2D(bow::Texture2DDescription(m_width, m_height, bow::TextureFormat::Red8, false));
		bow::TextureSamplerPtr imageSampler = m_device->VCreateTexture2DSampler(bow::TextureMinificationFilter::Nearest, bow::TextureMagnificationFilter::Nearest, bow::TextureWrap::Clamp, bow::TextureWrap::Clamp);
		bow::TextureSamplerPtr bucketSampler = m_device->VCreateTexture2DSampler(bow::TextureMinificationFilter::Nearest, bow::TextureMagnificationFilter::Nearest, bow::TextureWrap::Clamp, bow::TextureWrap::Clamp);

		OnInit(cameraParameters);

		m_pcl_renderer = new bow::PCLRenderer();
		m_pcl_renderer->Start();

		bow::BasicTimer timer;
		while (!m_window->VShouldClose())
		{
			if (m_width != m_window->VGetWidth() || m_height != m_window->VGetHeight())
			{
				m_width = m_window->VGetWidth();
				m_height = m_window->VGetHeight();

				OnResized(m_width, m_height);
			}

			timer.Update();

			m_keyboard->VUpdate();
			m_mouse->VUpdate();

			OnUpdate(timer.GetDelta());

			OnRender();

			int TexID = 0;
			contextOGL->VSetTexture(TexID, m_colorTexture);
			contextOGL->VSetTextureSampler(TexID, imageSampler);

			// Clear Backbuffer to our ClearState
			contextOGL->VClear(clearState);
			contextOGL->VSetViewport(bow::Viewport(0, 0, m_width * (1.0 / 3.0), m_height));

			colorShaderProgram->VSetUniform("inputTexture", TexID);
			contextOGL->VDraw(bow::PrimitiveType::Triangles, vertexArray, colorShaderProgram, renderState);


			contextOGL->VSetTexture(TexID, m_irTexture);
			contextOGL->VSetTextureSampler(TexID, bucketSampler);

			contextOGL->VSetViewport(bow::Viewport(m_width * (1.0 / 3.0), 0, m_width * (1.0 / 3.0), m_height));

			greyscaleShaderProgram->VSetUniform("inputTexture", TexID);
			contextOGL->VDraw(bow::PrimitiveType::Triangles, vertexArray, greyscaleShaderProgram, renderState);


			contextOGL->VSetTexture(TexID, m_depthTexture);
			contextOGL->VSetTextureSampler(TexID, bucketSampler);

			contextOGL->VSetViewport(bow::Viewport(m_width * (2.0 / 3.0), 0, m_width * (1.0 / 3.0), m_height));

			greyscaleShaderProgram->VSetUniform("inputTexture", TexID);
			contextOGL->VDraw(bow::PrimitiveType::Triangles, vertexArray, greyscaleShaderProgram, renderState);

			contextOGL->GUI_NewFrame();

			contextOGL->GUI_Begin("fps");
			contextOGL->GUI_Text("fps: %7.2f", 1000.0 / m_frametime);
			contextOGL->GUI_Text("frametime: %7.2f ms", m_frametime);
			contextOGL->GUI_End();

			contextOGL->GUI_Render();

			contextOGL->VSwapBuffers();

			m_window->VPollWindowEvents();
		}

		m_pcl_renderer->Stop();
		OnRelease();
	}


	void Application::Run_Visible_Only(bow::IntrinsicCameraParameters cameraParameters)
	{
		g_direction_vectors = bow::CameraCalibration::calculate_directionMatrix(cameraParameters, cameraParameters.image_width, cameraParameters.image_height);

		m_width = cameraParameters.image_width;
		m_height = cameraParameters.image_height;

		Init(m_width, m_height);

		auto contextOGL = m_window->VGetContext();

		///////////////////////////////////////////////////////////////////
		// compile shader

		bow::ShaderProgramPtr colorShaderProgram = m_device->VCreateShaderProgram(g_vertexColorShader, g_fragmentColorShader);
		bow::ShaderProgramPtr greyscaleShaderProgram = m_device->VCreateShaderProgram(g_vertexGreyscaleShader, g_fragmentGreyscaleShader);

		///////////////////////////////////////////////////////////////////
		// Vertex Array

		///////////////////////////////////////////////////////////////////
		// Define Triangle by Hand

		bow::MeshAttribute mesh;

		// Add Positions
		bow::VertexAttributeFloatVec3 *positionsAttribute = new bow::VertexAttributeFloatVec3("in_Position", 3);
		positionsAttribute->Values[0] = bow::Vector3<float>(-1.0f, -1.0f, 0.0f);
		positionsAttribute->Values[1] = bow::Vector3<float>(3.0f, -1.0f, 0.0f);
		positionsAttribute->Values[2] = bow::Vector3<float>(-1.0f, 3.0f, 0.0f);

		mesh.AddAttribute(bow::VertexAttributePtr(positionsAttribute));

		// Add TextureCoordinates
		bow::VertexAttributeFloatVec2 *texCoordAttribute = new bow::VertexAttributeFloatVec2("in_TexCoord", 3);
		texCoordAttribute->Values[0] = bow::Vector2<float>(0.0f, 0.0f);
		texCoordAttribute->Values[1] = bow::Vector2<float>(2.0f, 0.0f);
		texCoordAttribute->Values[2] = bow::Vector2<float>(0.0f, 2.0f);

		mesh.AddAttribute(bow::VertexAttributePtr(texCoordAttribute));

		bow::VertexArrayPtr vertexArray = contextOGL->VCreateVertexArray(mesh, colorShaderProgram->VGetVertexAttributes(), bow::BufferHint::StaticDraw);

		///////////////////////////////////////////////////////////////////
		// RenderState

		bow::RenderState renderState;
		renderState.faceCulling.Enabled = false;
		renderState.depthTest.Enabled = false;
		renderState.rasterizationMode = bow::RasterizationMode::Fill;

		// Change ClearColor
		bow::ClearState clearState;
		clearState.color = bow::ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f);

		///////////////////////////////////////////////////////////////////
		// Prepare render Texture

		m_colorTexture = m_device->VCreateTexture2D(bow::Texture2DDescription(m_width, m_height, bow::TextureFormat::RedGreenBlue8, false));
		bow::TextureSamplerPtr imageSampler = m_device->VCreateTexture2DSampler(bow::TextureMinificationFilter::Nearest, bow::TextureMagnificationFilter::Nearest, bow::TextureWrap::Clamp, bow::TextureWrap::Clamp);

		OnInit(cameraParameters);

		bow::BasicTimer timer;
		while (!m_window->VShouldClose())
		{
			if (m_width != m_window->VGetWidth() || m_height != m_window->VGetHeight())
			{
				m_width = m_window->VGetWidth();
				m_height = m_window->VGetHeight();

				OnResized(m_width, m_height);
			}

			timer.Update();

			m_keyboard->VUpdate();
			m_mouse->VUpdate();

			OnUpdate(timer.GetDelta());

			OnRender();

			int TexID = 0;
			contextOGL->VSetTexture(TexID, m_colorTexture);
			contextOGL->VSetTextureSampler(TexID, imageSampler);

			// Clear Backbuffer to our ClearState
			contextOGL->VClear(clearState);
			contextOGL->VSetViewport(bow::Viewport(0, 0, m_width, m_height));

			colorShaderProgram->VSetUniform("inputTexture", TexID);
			contextOGL->VDraw(bow::PrimitiveType::Triangles, vertexArray, colorShaderProgram, renderState);

			contextOGL->GUI_NewFrame();

			contextOGL->GUI_Begin("fps");
			contextOGL->GUI_Text("fps: %7.2f", 1000.0 / m_frametime);
			contextOGL->GUI_Text("frametime: %7.2f ms", m_frametime);
			contextOGL->GUI_End();

			contextOGL->GUI_Render();

			contextOGL->VSwapBuffers(true);

			m_window->VPollWindowEvents();
		}

		OnRelease();
	}

	void Application::DisplayFramerate(double frametime)
	{
		m_frametime = frametime;
	}
}
