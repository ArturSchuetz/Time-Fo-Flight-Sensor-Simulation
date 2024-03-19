#include "CameraUtils/PCLRenderer.h"

#include <CameraUtils/FirstPersonCamera.h>

#include <mutex>

#ifdef __unix__ 
#include <unistd.h>
#endif

//opencv
#include <opencv2/opencv.hpp>

namespace bow
{

	// ######################################################################################
	// Point Shader
	// ######################################################################################

	const std::string g_pointCloudVertexShader = R"(#version 130

	precision highp float;

	in vec3 in_Position;
	in vec3 in_Color;

	out vec3 var_Color;
	out highp vec4 var_ViewPos;

	uniform mat4 u_worldView;
	uniform mat4 u_projection;

	void main(void)
	{
		var_ViewPos = vec4(in_Position, 1.0) * u_worldView;
		gl_Position = var_ViewPos * u_projection;
		var_Color = in_Color;
	})";

	const std::string g_pointCloudFragmentShader = R"(#version 130

	precision highp float;

	in vec4 var_ViewPos;
	in vec3 var_Color;

	out vec3 out_Color;
	out highp float out_Depth;

	void main(void)
	{
		out_Color = var_Color;
		out_Depth = float(-var_ViewPos.z);
	})";


	// ######################################################################################
	// Point Shader
	// ######################################################################################

	const std::string g_litPointCloudVertexShader = R"(#version 130

	precision highp float;

	in vec3 in_Position;
	in vec3 in_Normal;
	in vec3 in_Color;

	uniform mat4 u_worldViewMatrix;
	uniform mat4 u_projectionMatrix;

	out vec3 position;
	out vec3 normal;
	out vec3 color;

	void main(void)
	{
		gl_Position = vec4(in_Position, 1.0) * u_worldViewMatrix * u_projectionMatrix;
		position = vec4(vec4(in_Position, 1.0) * u_worldViewMatrix).xyz;
		normal = vec4(vec4(in_Normal, 0.0) * u_worldViewMatrix).xyz;
		color = in_Color;
	})";

	const std::string g_litPointCloudFragmentShader = R"(#version 130

	precision highp float;

	in vec3 position;
	in vec3 normal;
	in vec3 color;

	out vec3 out_Color;

	void main(void)
	{
		vec3 lightPosition = vec3(0.0, 1.0, 0.0);
		vec3 N = normalize(normal);
		vec3 L = normalize(lightPosition - position);

		vec3 Idiff = color.rgb * max(dot(N, L), 0.0);
		Idiff = clamp(Idiff, 0.0, 1.0);  

		out_Color = vec3(Idiff);
	}
	)";

	// ######################################################################################
	// marker Shader
	// ######################################################################################

	const std::string g_markerVertexShader = R"(#version 130

	precision highp float;

	in vec3 in_Position;

	uniform mat4 u_worldView;
	uniform mat4 u_projection;

	void main(void)
	{
		gl_Position = (vec4(in_Position, 1.0)) * u_worldView * u_projection;
	})";

	const std::string g_markerFragmentShader = R"(#version 130

	precision highp float; // needed only for version 1.30

	uniform vec3 u_Color;
	out vec3 out_Color;

	void main(void)
	{
		out_Color = u_Color;
	})";

	// ######################################################################################
	// ######################################################################################

	// by Martin Kallman
	// https://gist.github.com/martinkallman/5049614
	// Fast half-precision to single-precision floating point conversion
	void float32(float* __restrict out, const uint16_t in) {
		uint32_t t1;
		uint32_t t2;
		uint32_t t3;

		t1 = in & 0x7fff;                       // Non-sign bits
		t2 = in & 0x8000;                       // Sign bit
		t3 = in & 0x7c00;                       // Exponent

		t1 <<= 13;                              // Align mantissa on MSB
		t2 <<= 16;                              // Shift sign bit into position

		t1 += 0x38000000;                       // Adjust bias

		t1 = (t3 == 0 ? 0 : t1);                // Denormals-as-zero

		t1 |= t2;                               // Re-insert sign bit

		*((uint32_t*)out) = t1;
	}

	std::mutex update_data_mutex;

	struct renderThread_data
	{
		bool		stopThread;
		bool		isRunning;
		bool		shouldStop;

		std::vector<bow::Vector3<float>> newVertices;
		std::vector<bow::Vector3<float>> newColors;
		std::vector<bow::Vector3<float>> newNormals;

		std::vector<bow::Vector3<float>> newRefVertices;
		std::vector<bow::Vector3<float>> newRefColors;
		std::vector<bow::Vector3<float>> newRefNormals;

		bool waitForRender;
		bool renderFinished;
		bow::Matrix4x4<float> newViewMatrix;
		bow::Matrix4x4<float> newProjMatrix;
		unsigned int width;
		unsigned int height;
		std::vector<ushort> out_depth;
	};


	void RenderThreadProc(renderThread_data* my_data)
	{
		std::cout << "Starting Render Thread" << std::endl;

		my_data->isRunning = true;

		///////////////////////////////////////////////////////////////////
		// Creating Render Device

		// Creating Render Device
		RenderDevicePtr renderDevice = bow::RenderDeviceManager::GetInstance().GetOrCreateDevice(bow::RenderDeviceAPI::OpenGL3x);
		if (renderDevice == nullptr)
		{
			std::cout << "Could not create device!" << std::endl;
			return;
		}


		///////////////////////////////////////////////////////////////////
		// Creating Window
		bow::GraphicsWindowPtr pointCloudWindow = renderDevice->VCreateWindow(800, 600, "PointCloud", bow::WindowType::Windowed);
		if (pointCloudWindow == nullptr)
		{
			std::cout << "Could not create window!" << std::endl;
			return;
		}

		auto ContextOGL = pointCloudWindow->VGetContext();
		if (ContextOGL == nullptr)
		{
			std::cout << "ERROR: Could not create render context" << std::endl;
			return;
		}

		///////////////////////////////////////////////////////////////////
		// Preparing Scene

		// Change ClearColor
		bow::ClearState clearState;
		clearState.color = bow::ColorRGBA(0.39215686274f, 0.58431372549f, 0.9294117647f, 1.0f);

		bow::ShaderProgramPtr pointCloudShaderProgram = renderDevice->VCreateShaderProgram(g_pointCloudVertexShader, g_pointCloudFragmentShader);
		bow::ShaderProgramPtr litPointCloudShaderProgram = renderDevice->VCreateShaderProgram(g_litPointCloudVertexShader, g_litPointCloudFragmentShader);
		bow::ShaderProgramPtr markerShaderProgram = renderDevice->VCreateShaderProgram(g_markerVertexShader, g_markerFragmentShader);

		///////////////////////////////////////////////////////////////////
		// Vertex Array for Pointclouds

		bow::VertexBufferPtr vertexBuffer = renderDevice->VCreateVertexBuffer(bow::BufferHint::StaticDraw, 1 * sizeof(float) * 3);
		bow::VertexBufferPtr colorBuffer = renderDevice->VCreateVertexBuffer(bow::BufferHint::StaticDraw, 1 * sizeof(float) * 3);
		bow::VertexBufferPtr normalBuffer = renderDevice->VCreateVertexBuffer(bow::BufferHint::StaticDraw, 1 * sizeof(float) * 3);

		bow::VertexArrayPtr referencePointCloudVertexArray = ContextOGL->VCreateVertexArray();
		referencePointCloudVertexArray->VSetAttribute(pointCloudShaderProgram->VGetVertexAttribute("in_Position"), bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(vertexBuffer, bow::ComponentDatatype::Float, 3)));
		referencePointCloudVertexArray->VSetAttribute(pointCloudShaderProgram->VGetVertexAttribute("in_Color"), bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(colorBuffer, bow::ComponentDatatype::Float, 3)));

		bow::VertexArrayPtr pointCloudVertexArray = ContextOGL->VCreateVertexArray();
		pointCloudVertexArray->VSetAttribute(pointCloudShaderProgram->VGetVertexAttribute("in_Position"), bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(vertexBuffer, bow::ComponentDatatype::Float, 3)));
		pointCloudVertexArray->VSetAttribute(pointCloudShaderProgram->VGetVertexAttribute("in_Color"), bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(colorBuffer, bow::ComponentDatatype::Float, 3)));

		bow::VertexArrayPtr litPointCloudVertexArray = ContextOGL->VCreateVertexArray();
		litPointCloudVertexArray->VSetAttribute(litPointCloudShaderProgram->VGetVertexAttribute("in_Position"), bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(vertexBuffer, bow::ComponentDatatype::Float, 3)));
		litPointCloudVertexArray->VSetAttribute(litPointCloudShaderProgram->VGetVertexAttribute("in_Color"), bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(colorBuffer, bow::ComponentDatatype::Float, 3)));
		litPointCloudVertexArray->VSetAttribute(litPointCloudShaderProgram->VGetVertexAttribute("in_Normal"), bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(normalBuffer, bow::ComponentDatatype::Float, 3)));

		///////////////////////////////////////////////////////////////////
		// Vertex Array from Mesh for Center

		// Define vertex attributes as input for shader program:
		// Every attribute need to have a std::vector of the same size (3 in this example).
		// In this example every vertex (in_Position) has one corresponding color (in_Color).

		float coordinateScale = 100.0f;

		bow::MeshAttribute coordianteSystemMesh;
		{
			// Add Positions
			bow::VertexAttributeFloatVec3 *positionsAttribute = new bow::VertexAttributeFloatVec3("in_Position", 6);
			positionsAttribute->Values[0] = bow::Vector3<float>(0.0f, 0.0f, 0.0f) * coordinateScale;
			positionsAttribute->Values[1] = bow::Vector3<float>(0.1f, 0.0f, 0.0f) * coordinateScale;
			positionsAttribute->Values[2] = bow::Vector3<float>(0.0f, 0.0f, 0.0f) * coordinateScale;
			positionsAttribute->Values[3] = bow::Vector3<float>(0.0f, 0.1f, 0.0f) * coordinateScale;
			positionsAttribute->Values[4] = bow::Vector3<float>(0.0f, 0.0f, 0.0f) * coordinateScale;
			positionsAttribute->Values[5] = bow::Vector3<float>(0.0f, 0.0f, 0.1f) * coordinateScale;

			coordianteSystemMesh.AddAttribute(bow::VertexAttributePtr(positionsAttribute));

			// Add Colors
			bow::VertexAttributeFloatVec3 *colorAttribute = new bow::VertexAttributeFloatVec3("in_Color", 6);
			colorAttribute->Values[0] = bow::Vector3<float>(1.0f, 0.0f, 0.0f); // green
			colorAttribute->Values[1] = bow::Vector3<float>(1.0f, 0.0f, 0.0f); // green
			colorAttribute->Values[2] = bow::Vector3<float>(0.0f, 1.0f, 0.0f); // red
			colorAttribute->Values[3] = bow::Vector3<float>(0.0f, 1.0f, 0.0f); // red
			colorAttribute->Values[4] = bow::Vector3<float>(0.0f, 0.0f, 1.0f); // blue
			colorAttribute->Values[5] = bow::Vector3<float>(0.0f, 0.0f, 1.0f); // blue

			coordianteSystemMesh.AddAttribute(bow::VertexAttributePtr(colorAttribute));
		}
		// Create vertex attribute from mesh to be able to render them. At this point the mesh attributes are connected to the shader vertex attributes.
		bow::VertexArrayPtr coordinateSystemVertexArray = ContextOGL->VCreateVertexArray(coordianteSystemMesh, pointCloudShaderProgram->VGetVertexAttributes(), bow::BufferHint::StaticDraw);

		//======================================
		// Vertex Array from Mesh for Center
		//======================================

		float backplaneScale = 1500.0f;
		bow::MeshAttribute planesMesh;
		{
			// Add Positions
			bow::VertexAttributeFloatVec3 *positionsAttribute = new bow::VertexAttributeFloatVec3("in_Position", 6);
			positionsAttribute->Values[0] = bow::Vector3<float>(-1.0f, -1.0f, 0.0f) * backplaneScale;
			positionsAttribute->Values[1] = bow::Vector3<float>(1.0f, 1.0f, 0.0f) * backplaneScale;
			positionsAttribute->Values[2] = bow::Vector3<float>(-1.0f, 1.0f, 0.0f) * backplaneScale;
			positionsAttribute->Values[3] = bow::Vector3<float>(-1.0f, -1.0f, 0.0f) * backplaneScale;
			positionsAttribute->Values[4] = bow::Vector3<float>(1.0f, -1.0f, 0.0f) * backplaneScale;
			positionsAttribute->Values[5] = bow::Vector3<float>(1.0f, 1.0f, 0.0f) * backplaneScale;

			planesMesh.AddAttribute(bow::VertexAttributePtr(positionsAttribute));
		}
		// Create vertex attribute from mesh to be able to render them. At this point the mesh attributes are connected to the shader vertex attributes.
		bow::VertexArrayPtr backPlaneVertexArray = ContextOGL->VCreateVertexArray(planesMesh, markerShaderProgram->VGetVertexAttributes(), bow::BufferHint::StaticDraw);

		// =============================================================================================
		// =============================================================================================

		float groundplane_scale = 1500.0f;
		float groundplane_level = 210.0f;
		bow::MeshAttribute groundPlanesMesh;
		{
			// Add Positions
			bow::VertexAttributeFloatVec3 *positionsAttribute = new bow::VertexAttributeFloatVec3("in_Position", 6);
			positionsAttribute->Values[0] = bow::Vector3<float>(-groundplane_scale, groundplane_level, -groundplane_scale);
			positionsAttribute->Values[1] = bow::Vector3<float>(groundplane_scale, groundplane_level, groundplane_scale);
			positionsAttribute->Values[2] = bow::Vector3<float>(-groundplane_scale, groundplane_level, groundplane_scale);
			positionsAttribute->Values[3] = bow::Vector3<float>(-groundplane_scale, groundplane_level, -groundplane_scale);
			positionsAttribute->Values[4] = bow::Vector3<float>(groundplane_scale, groundplane_level, -groundplane_scale);
			positionsAttribute->Values[5] = bow::Vector3<float>(groundplane_scale, groundplane_level, groundplane_scale);

			groundPlanesMesh.AddAttribute(bow::VertexAttributePtr(positionsAttribute));
		}
		// Create vertex attribute from mesh to be able to render them. At this point the mesh attributes are connected to the shader vertex attributes.
		bow::VertexArrayPtr groundPlaneVertexArray = ContextOGL->VCreateVertexArray(groundPlanesMesh, markerShaderProgram->VGetVertexAttributes(), bow::BufferHint::StaticDraw);

		// =============================================================================================
		// =============================================================================================

		float x_offset = 390.0f;
		float y_offset = -50.0f;

		float width = 150.0f;
		float height = 145.0f;
		float depth = 150.0f;
		bow::MeshAttribute boxMesh;
		{
			// Add Positions
			bow::VertexAttributeFloatVec3 *positionsAttribute = new bow::VertexAttributeFloatVec3("in_Position", 6);
			positionsAttribute->Values[0] = bow::Vector3<float>(-width * 0.5f + x_offset, -height * 0.5f + y_offset, depth);
			positionsAttribute->Values[1] = bow::Vector3<float>(width * 0.5f + x_offset, height * 0.5f + y_offset, depth);
			positionsAttribute->Values[2] = bow::Vector3<float>(-width * 0.5f + x_offset, height * 0.5f + y_offset, depth);
			positionsAttribute->Values[3] = bow::Vector3<float>(-width * 0.5f + x_offset, -height * 0.5f + y_offset, depth);
			positionsAttribute->Values[4] = bow::Vector3<float>(width * 0.5f + x_offset, -height * 0.5f + y_offset, depth);
			positionsAttribute->Values[5] = bow::Vector3<float>(width * 0.5f + x_offset, height * 0.5f + y_offset, depth);

			boxMesh.AddAttribute(bow::VertexAttributePtr(positionsAttribute));
		}
		// Create vertex attribute from mesh to be able to render them. At this point the mesh attributes are connected to the shader vertex attributes.
		bow::VertexArrayPtr boxVertexArray = ContextOGL->VCreateVertexArray(boxMesh, markerShaderProgram->VGetVertexAttributes(), bow::BufferHint::StaticDraw);

		///////////////////////////////////////////////////////////////////
		// FrameBuffer

		bow::FramebufferPtr refDepthRenderingFrameBuffer = ContextOGL->VCreateFramebuffer();

		///////////////////////////////////////////////////////////////////
		// RenderState

		bow::RenderState coloredLinesRenderState;
		coloredLinesRenderState.rasterizationMode = bow::RasterizationMode::Line;

		bow::RenderState markerRenderState;
		markerRenderState.faceCulling.Enabled = false;
		markerRenderState.depthTest.Enabled = true;
		markerRenderState.rasterizationMode = bow::RasterizationMode::Fill;

		bow::RenderState pointCloudRenderState;
		pointCloudRenderState.faceCulling.Enabled = false;
		pointCloudRenderState.depthTest.Enabled = true;
		pointCloudRenderState.rasterizationMode = bow::RasterizationMode::Point;
		pointCloudRenderState.pointSize = 2.0f;

		bow::FirstPersonCamera camera(
			bow::Vector3<double>(0.0, 0.0, 0.0),	// Position
			bow::Vector3<double>(0.0, 0.0, -1.0),	// LookAt
			bow::Vector3<double>(0.0, 1.0, 0.0),	// WorldUp 
			pointCloudWindow->VGetWidth(),
			pointCloudWindow->VGetHeight()
			);
		camera.SetClippingPlanes(0.1, 100000.0);

		auto mouse = bow::InputDeviceManager::GetInstance().CreateMouseObject(pointCloudWindow);
		auto keyboard = bow::InputDeviceManager::GetInstance().CreateKeyboardObject(pointCloudWindow);

		bow::Vector3<long> lastCursorPosition = mouse->VGetAbsolutePositionInsideWindow();
		auto lastFrameTime = std::chrono::high_resolution_clock::now(); // Take time

		std::vector<bow::Vector3<float>> points;
		std::vector<bow::Vector3<float>> colors;

		bool add_pressed = false;
		bool minus_pressed = false;
		while (!my_data->stopThread)
		{
			pointCloudWindow->VPollWindowEvents();

			if (pointCloudWindow->VShouldClose())
			{
				my_data->shouldStop = true;
			}

			update_data_mutex.lock();
			if (my_data->newVertices.size() > 0 && my_data->newColors.size() > 0)
			{
				vertexBuffer = renderDevice->VCreateVertexBuffer(bow::BufferHint::StaticDraw, (int)my_data->newVertices.size() * sizeof(float) * 3);
				colorBuffer = renderDevice->VCreateVertexBuffer(bow::BufferHint::StaticDraw, (int)my_data->newColors.size() * sizeof(float) * 3);

				pointCloudVertexArray->VSetAttribute(pointCloudShaderProgram->VGetVertexAttribute("in_Position"), bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(vertexBuffer, bow::ComponentDatatype::Float, 3)));
				pointCloudVertexArray->VSetAttribute(pointCloudShaderProgram->VGetVertexAttribute("in_Color"), bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(colorBuffer, bow::ComponentDatatype::Float, 3)));

				points.resize(my_data->newVertices.size());
				for (unsigned int i = 0; i < my_data->newVertices.size(); i++)
				{
					points[i] = bow::Vector3<float>(my_data->newVertices[i]);
				}
				vertexBuffer->VCopyFromSystemMemory((char*)(&points[0]), (int)points.size() * sizeof(bow::Vector3<float>));

				colors.resize(my_data->newColors.size());
				for (unsigned int i = 0; i < my_data->newColors.size(); i++)
				{
					colors[i] = bow::Vector3<float>(my_data->newColors[i]);
				}
				colorBuffer->VCopyFromSystemMemory((char*)(&colors[0]), (int)colors.size() * sizeof(bow::Vector3<float>));

				my_data->newVertices.clear();
				my_data->newColors.clear();
			}

			if (my_data->newRefVertices.size() > 0 || my_data->newRefColors.size() > 0)
			{
				vertexBuffer = renderDevice->VCreateVertexBuffer(bow::BufferHint::StaticDraw, (int)my_data->newRefVertices.size() * sizeof(float) * 3);
				colorBuffer = renderDevice->VCreateVertexBuffer(bow::BufferHint::StaticDraw, (int)my_data->newRefColors.size() * sizeof(float) * 3);
			
				referencePointCloudVertexArray->VSetAttribute(pointCloudShaderProgram->VGetVertexAttribute("in_Position"), bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(vertexBuffer, bow::ComponentDatatype::Float, 3)));
				referencePointCloudVertexArray->VSetAttribute(pointCloudShaderProgram->VGetVertexAttribute("in_Color"), bow::VertexBufferAttributePtr(new bow::VertexBufferAttribute(colorBuffer, bow::ComponentDatatype::Float, 3)));
				
				points.resize(my_data->newRefVertices.size());
				for (unsigned int i = 0; i < my_data->newRefVertices.size(); i++)
				{
					points[i] = bow::Vector3<float>(my_data->newRefVertices[i]);
				}
				vertexBuffer->VCopyFromSystemMemory((char*)(&points[0]), (int)points.size() * sizeof(bow::Vector3<float>));

				colors.resize(my_data->newRefColors.size());
				for (unsigned int i = 0; i < my_data->newRefColors.size(); i++)
				{
					colors[i] = bow::Vector3<float>(my_data->newRefColors[i]);
				}
				colorBuffer->VCopyFromSystemMemory((char*)(&colors[0]), (int)colors.size() * sizeof(bow::Vector3<float>));

				my_data->newRefVertices.clear();
				my_data->newRefColors.clear();
			}
			update_data_mutex.unlock();

			// ============================================
			// Handle Input

			auto currentFrameTime = std::chrono::high_resolution_clock::now(); // Take time
			std::chrono::duration<double, std::milli> frameduration = currentFrameTime - lastFrameTime;
			float deltaTime = (float)frameduration.count();

			lastFrameTime = currentFrameTime; // Take time
			double moveSpeed = 100.0;

			if (keyboard->VIsPressed(bow::Key::K_LEFT_SHIFT))
			{
				moveSpeed = moveSpeed * 2.0;
			}

			if (keyboard->VIsPressed(bow::Key::K_UP))
			{
				if (!add_pressed)
				{
					add_pressed = true;
					pointCloudRenderState.pointSize++;
				}
			}
			else
			{
				add_pressed = false;
			}

			if (keyboard->VIsPressed(bow::Key::K_DOWN))
			{
				if (!minus_pressed)
				{
					minus_pressed = true;
					pointCloudRenderState.pointSize--;
					if (pointCloudRenderState.pointSize < 1)
						pointCloudRenderState.pointSize = 1;
				}
			}
			else
			{
				minus_pressed = false;
			}

			if (keyboard->VIsPressed(bow::Key::K_W))
			{
				camera.MoveForward(moveSpeed * deltaTime);
			}

			if (keyboard->VIsPressed(bow::Key::K_S))
			{
				camera.MoveBackward(moveSpeed * deltaTime);
			}

			if (keyboard->VIsPressed(bow::Key::K_D))
			{
				camera.MoveRight(moveSpeed * deltaTime);
			}

			if (keyboard->VIsPressed(bow::Key::K_A))
			{
				camera.MoveLeft(moveSpeed * deltaTime);
			}

			if (keyboard->VIsPressed(bow::Key::K_SPACE))
			{
				camera.MoveUp(moveSpeed * deltaTime);
			}

			if (keyboard->VIsPressed(bow::Key::K_LEFT_CONTROL))
			{
				camera.MoveDown(moveSpeed * deltaTime);
			}

			if (mouse->VIsPressed(bow::MouseButton::MOFS_BUTTON1))
			{
				pointCloudWindow->VHideCursor();
				bow::Vector3<long> moveVec = mouse->VGetAbsolutePositionInsideWindow() - lastCursorPosition;

				camera.rotate((float)moveVec.x, (float)moveVec.y);

				mouse->VSetCursorPosition(lastCursorPosition.x, lastCursorPosition.y);
			}
			else
			{
				pointCloudWindow->VShowCursor();
			}
			lastCursorPosition = mouse->VGetAbsolutePositionInsideWindow();

			// ============================================
			// Prepare Backbuffer

			ContextOGL->VSetFramebuffer(nullptr);
			ContextOGL->VSetViewport(bow::Viewport(0, 0, pointCloudWindow->VGetWidth(), pointCloudWindow->VGetHeight()));
			ContextOGL->VClear(clearState);
			camera.SetResolution(pointCloudWindow->VGetWidth(), pointCloudWindow->VGetHeight());

			Matrix3D<double> worldView = camera.CalculateView();
			Matrix4x4<double> projection = camera.CalculateProjection();

			pointCloudShaderProgram->VSetUniform("u_worldView", (Matrix4x4<float>)worldView);
			pointCloudShaderProgram->VSetUniform("u_projection", (Matrix4x4<float>)projection);

			// ============================================
			// Render Coordinate System Origin 

			ContextOGL->VDraw(bow::PrimitiveType::Lines, coordinateSystemVertexArray, pointCloudShaderProgram, coloredLinesRenderState);

			// ============================================
			// Render Reference Point Cloud

			ContextOGL->VDraw(bow::PrimitiveType::Points, referencePointCloudVertexArray, pointCloudShaderProgram, pointCloudRenderState);

			// ============================================
			// Render Point Cloud

			ContextOGL->VDraw(bow::PrimitiveType::Points, pointCloudVertexArray, pointCloudShaderProgram, pointCloudRenderState);

			// ============================================
			// Render Geometry 

			//markerShaderProgram->VSetUniform("u_worldView", (Matrix4x4<float>)worldView);
			//markerShaderProgram->VSetUniform("u_projection", (Matrix4x4<float>)projection);
			//markerShaderProgram->VSetUniform("u_Color", Vector3<float>(0.5f, 0.5f, 0.5f));
			//ContextOGL->VDraw(bow::PrimitiveType::Triangles, backPlaneVertexArray, markerShaderProgram, markerRenderState);
			//markerShaderProgram->VSetUniform("u_Color", Vector3<float>(0.1f, 0.1f, 0.1f));
			//ContextOGL->VDraw(bow::PrimitiveType::Triangles, groundPlaneVertexArray, markerShaderProgram, markerRenderState);
			//markerShaderProgram->VSetUniform("u_Color", Vector3<float>(0.8f, 0.8f, 0.8f));
			//ContextOGL->VDraw(bow::PrimitiveType::Triangles, boxVertexArray, markerShaderProgram, markerRenderState);
			
			// ============================================

			ContextOGL->VSwapBuffers();

			update_data_mutex.lock();
			if (my_data->waitForRender)
			{
				bow::Texture2DPtr colorRenderTarget = renderDevice->VCreateTexture2D(bow::Texture2DDescription(my_data->width, my_data->height, bow::TextureFormat::RedGreenBlue8));
				bow::Texture2DPtr depthRenderTarget = renderDevice->VCreateTexture2D(bow::Texture2DDescription(my_data->width, my_data->height, bow::TextureFormat::Red32f));
				bow::Texture2DPtr depthTarget = renderDevice->VCreateTexture2D(bow::Texture2DDescription(my_data->width, my_data->height, bow::TextureFormat::Depth16));

				refDepthRenderingFrameBuffer->VSetColorAttachment(pointCloudShaderProgram->VGetFragmentOutputLocation("out_Color"), colorRenderTarget);
				refDepthRenderingFrameBuffer->VSetColorAttachment(pointCloudShaderProgram->VGetFragmentOutputLocation("out_Depth"), depthRenderTarget);
				refDepthRenderingFrameBuffer->VSetDepthAttachment(depthTarget);

				ContextOGL->VSetFramebuffer(refDepthRenderingFrameBuffer);
				ContextOGL->VSetViewport(bow::Viewport(0, 0, my_data->width, my_data->height));
				ContextOGL->VClear(clearState);

				pointCloudShaderProgram->VSetUniform("u_worldView", (Matrix4x4<float>)my_data->newViewMatrix);
				pointCloudShaderProgram->VSetUniform("u_projection", (Matrix4x4<float>)my_data->newProjMatrix);
				ContextOGL->VDraw(bow::PrimitiveType::Points, referencePointCloudVertexArray, pointCloudShaderProgram, pointCloudRenderState);

				// ============================================
				// Color

				bow::Texture2DDescription colorTargetDescription = colorRenderTarget->VGetDescription();
				auto colorData = colorRenderTarget->VCopyToSystemMemory(bow::ImageFormat::RedGreenBlue, bow::ImageDatatype::UnsignedByte);
				cv::Mat_<cv::Vec3b> colorMat = cv::Mat_<cv::Vec3b>(colorTargetDescription.GetHeight(), colorTargetDescription.GetWidth());
				memcpy(colorMat.data, colorData.get(), colorTargetDescription.GetHeight() * colorTargetDescription.GetWidth() * sizeof(cv::Vec3b));

				// ============================================
				// Depth

				bow::Texture2DDescription depthTargetDescription = depthRenderTarget->VGetDescription();
				auto depthData = depthRenderTarget->VCopyToSystemMemory(bow::ImageFormat::Red, bow::ImageDatatype::Float);

				unsigned int marker_numElements = depthTargetDescription.GetHeight() * depthTargetDescription.GetWidth();
				my_data->out_depth = std::vector<ushort>(marker_numElements);

				float* values = (float*)(depthData.get());
				for (unsigned int i = 0; i < marker_numElements; i++)
				{
					my_data->out_depth[i] = (ushort)(values[i]);
				}

				my_data->renderFinished = true;
			}
			update_data_mutex.unlock();
		}

		std::cout << "Stopping Render Thread" << std::endl;

		my_data->isRunning = false;
		return;
	}


	PCLRenderer::PCLRenderer()
	{
		m_renderThreadData = new renderThread_data();

		m_renderThreadData->stopThread = false;
		m_renderThreadData->isRunning = false;
		m_renderThreadData->shouldStop = false;
		m_renderThreadData->waitForRender = false;
		m_renderThreadData->renderFinished = false;

		m_renderThreadData->newVertices.clear();
		m_renderThreadData->newColors.clear();
	}


	PCLRenderer::~PCLRenderer()
	{

	}

	std::vector<ushort> PCLRenderer::RenderRefDepthFromPerspective(unsigned int width, unsigned int height, bow::Matrix3D<float> viewMatrix, bow::Matrix4x4<float> projectionMatrix)
	{
		update_data_mutex.lock();
		m_renderThreadData->newViewMatrix = viewMatrix;
		m_renderThreadData->newProjMatrix = projectionMatrix;
		m_renderThreadData->width = width;
		m_renderThreadData->height = height;
		m_renderThreadData->waitForRender = true;
		update_data_mutex.unlock();
		while (true)
		{
			bool renderFinished = false;
			update_data_mutex.lock();
			renderFinished = m_renderThreadData->renderFinished;
			update_data_mutex.unlock();
			if (renderFinished)
			{
				update_data_mutex.lock();
				m_renderThreadData->waitForRender = false;
				m_renderThreadData->renderFinished = false;
				update_data_mutex.unlock();

				return m_renderThreadData->out_depth;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	bool PCLRenderer::Start()
	{
		if (!m_renderThreadData->isRunning)
		{
			std::cout << "Starting Camera..." << std::endl;

			m_renderThread = std::thread([this](){ RenderThreadProc(m_renderThreadData); });
			return true;
		}

		return false;
	}

	bool PCLRenderer::UpdateColors(std::vector<bow::Vector3<float>> colors)
	{
		update_data_mutex.lock();
		m_renderThreadData->newColors = colors;
		update_data_mutex.unlock();
		return true;
	}

	bool PCLRenderer::UpdatePointCloud(std::vector<bow::Vector3<float>> vertices)
	{
		update_data_mutex.lock();
		m_renderThreadData->newVertices = vertices;
		update_data_mutex.unlock();
		return true;
	}

	bool PCLRenderer::UpdatePointCloud(std::vector<bow::Vector3<float>> vertices, std::vector<bow::Vector3<float>> normals)
	{
		update_data_mutex.lock();
		m_renderThreadData->newVertices = vertices;
		m_renderThreadData->newNormals = normals;
		update_data_mutex.unlock();
		return true;
	}


	bool PCLRenderer::UpdateReferencePointCloud(std::vector<bow::Vector3<float>> vertices)
	{
		update_data_mutex.lock();
		m_renderThreadData->newRefVertices = vertices;
		update_data_mutex.unlock();
		return true;
	}

	bool PCLRenderer::UpdateReferencePointCloud(std::vector<bow::Vector3<float>> vertices, std::vector<bow::Vector3<float>> normals)
	{
		update_data_mutex.lock();
		m_renderThreadData->newRefVertices = vertices;
		m_renderThreadData->newRefNormals = normals;
		update_data_mutex.unlock();
		return true;
	}

	bool PCLRenderer::UpdateReferencePointCloud(std::vector<bow::Vector3<float>> vertices, std::vector<bow::Vector3<float>> colors, std::vector<bow::Vector3<float>> normals)
	{
		update_data_mutex.lock();
		m_renderThreadData->newRefVertices = vertices;
		m_renderThreadData->newRefColors = colors;
		m_renderThreadData->newRefNormals = normals;
		update_data_mutex.unlock();
		return true;
	}

	bool PCLRenderer::Stop()
	{
		if (m_renderThreadData->isRunning)
		{
			std::cout << "Stopping Rendering ..." << std::endl;

			m_renderThreadData->stopThread = true;

			std::cout << "Waiting for thread to stop..." << std::endl;
			while (m_renderThreadData->isRunning)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			m_renderThread.join();
			return true;
		}

		return false;
	}


	bool PCLRenderer::ShouldClose()
	{
		return m_renderThreadData->shouldStop;
	}
}
