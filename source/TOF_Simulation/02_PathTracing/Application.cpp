#include "Application.h"
#include <OptixUtils/Mesh.h>
#include <OptixUtils/OptiXMesh.h>

#include <Masterthesis/cuda_config.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <iostream>     // std::cout, std::endl
#include <iomanip>      // std::setw

extern optix::Context g_context;

struct BasicLight
{
	optix::float3 pos;
	optix::float3 color;
	int    casts_shadow;
	int    padding;      // make this structure 32 bytes -- powers of two are your friend!
};

//------------------------------------------------------------------------------
//
//  Helper functions
//
//------------------------------------------------------------------------------


optix::Buffer getOutputBuffer()
{
	return g_context["output_buffer"]->getBuffer();
}


struct UsageReportLogger
{
	void log(int lvl, const char* tag, const char* msg)
	{
		std::cout << "[" << lvl << "][" << std::left << std::setw(12) << tag << "] " << msg;
	}
};


// Static callback
void usageReportCallback(int lvl, const char* tag, const char* msg, void* cbdata)
{
	// Route messages to a C++ object (the "logger"), as a real app might do.
	// We could have printed them directly in this simple case.

	UsageReportLogger* logger = reinterpret_cast<UsageReportLogger*>(cbdata);
	logger->log(lvl, tag, msg);
}


// ======================================================================
// ======================================================================


SimpleDirectionalLightApp::SimpleDirectionalLightApp() : m_logger(nullptr), m_usage_report_level(0), m_camera(nullptr)
{
	m_logger = new UsageReportLogger();
}


SimpleDirectionalLightApp::~SimpleDirectionalLightApp()
{
	if (g_context)
	{
		g_context->destroy();
		g_context = 0;
	}

	if (m_logger != nullptr)
	{
		delete m_logger;
		m_logger = nullptr;
	}

	if (m_camera != nullptr)
	{
		delete m_camera;
		m_camera = nullptr;
	}
}

// ======================================================================
// ======================================================================

void SimpleDirectionalLightApp::OnInit(bow::IntrinsicCameraParameters cameraParameters)
{
	createContext(m_usage_report_level, m_logger);

	optix::GeometryGroup geometry_group = g_context->createGeometryGroup();
	if (!m_environment_test)
	{
		loadMesh(std::string(PROJECT_BASE_DIR) + std::string("/data/Scenes/Sponza/sponza.obj"), geometry_group, 100.0f);
	}
	createSphere(geometry_group);

	geometry_group->setAcceleration(g_context->createAcceleration("Trbvh"));
	g_context["top_object"]->set(geometry_group);
	g_context["top_shadower"]->set(geometry_group);

	setupCamera();
	setupLights();

	m_direction_vectors = bow::CameraCalibration::calculate_directionMatrix(cameraParameters, cameraParameters.image_width * 10, cameraParameters.image_height * 10);

	optix::Buffer rayDirectionsBuffer = g_context->createBuffer(RT_BUFFER_INPUT);
	rayDirectionsBuffer->setFormat(RT_FORMAT_FLOAT4);
	rayDirectionsBuffer->setSize(m_direction_vectors.cols, m_direction_vectors.rows);

	memcpy(rayDirectionsBuffer->map(), &(m_direction_vectors.data[0]), sizeof(float) * 4 * m_direction_vectors.cols * m_direction_vectors.rows);
	rayDirectionsBuffer->unmap();

	g_context["input_rayDirections"]->set(rayDirectionsBuffer);

	g_context->validate();
}


void SimpleDirectionalLightApp::OnResized(unsigned int newWidth, unsigned int newHeight)
{

}


void SimpleDirectionalLightApp::OnUpdate(double deltaTime)
{
	double moveSpeed = 100.0;

	if (m_keyboard->VIsPressed(bow::Key::K_LEFT_SHIFT))
	{
		moveSpeed = moveSpeed * 2.0;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_W))
	{
		m_camera->MoveForward(moveSpeed * deltaTime);
		m_camera_changed = true;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_S))
	{
		m_camera->MoveBackward(moveSpeed * deltaTime);
		m_camera_changed = true;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_D))
	{
		m_camera->MoveRight(moveSpeed * deltaTime);
		m_camera_changed = true;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_A))
	{
		m_camera->MoveLeft(moveSpeed * deltaTime);
		m_camera_changed = true;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_SPACE))
	{
		m_camera->MoveUp(moveSpeed * deltaTime);
		m_camera_changed = true;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_LEFT_CONTROL))
	{
		m_camera->MoveDown(moveSpeed * deltaTime);
		m_camera_changed = true;
	}


	if (m_keyboard->VIsPressed(bow::Key::K_KP_SUBTRACT))
	{
		m_lightIntensity -= 1.0f;
		if (m_lightIntensity < 0.0f)
			m_lightIntensity = 0.0f;
		g_context["bg_color"]->setFloat(optix::make_float3(m_lightIntensity));
		m_camera_changed = true;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_KP_ADD))
	{
		m_lightIntensity += 1.0f;
		g_context["bg_color"]->setFloat(optix::make_float3(m_lightIntensity));
		m_camera_changed = true;
	}

	if (m_mouse->VIsPressed(bow::MouseButton::MOFS_BUTTON1))
	{
		m_window->VHideCursor();
		bow::Vector3<long> moveVec = m_mouse->VGetAbsolutePositionInsideWindow() - m_lastCursorPosition;

		m_camera->rotate((float)moveVec.x, (float)moveVec.y);
		if (moveVec.x != 0 || moveVec.y != 0)
		{
			m_camera_changed = true;
		}

		m_mouse->VSetCursorPosition(m_lastCursorPosition.x, m_lastCursorPosition.y);
	}
	else
	{
		m_window->VShowCursor();
	}
	m_lastCursorPosition = m_mouse->VGetAbsolutePositionInsideWindow();

	updateCamera();
}


void SimpleDirectionalLightApp::OnRender()
{
	optix::Buffer image_buffer = getOutputBuffer();

	RTsize buffer_width_rts, buffer_height_rts;
	image_buffer->getSize(buffer_width_rts, buffer_height_rts);
	uint32_t image_width = static_cast<int>(buffer_width_rts);
	uint32_t image_height = static_cast<int>(buffer_height_rts);
	RTformat buffer_format = image_buffer->getFormat();

	g_context->launch(0, image_width, image_height);

	void* imageData = image_buffer->map(0, RT_BUFFER_MAP_READ);

	if (buffer_format == RT_FORMAT_FLOAT3)
	{
		UpdateColorBuffer(imageData, image_width, image_height, bow::ImageFormat::RedGreenBlue, bow::ImageDatatype::Float);
	}
	else if (buffer_format == RT_FORMAT_FLOAT4)
	{
		UpdateColorBuffer(imageData, image_width, image_height, bow::ImageFormat::RedGreenBlueAlpha, bow::ImageDatatype::Float);
	}
	else if (buffer_format == RT_FORMAT_UNSIGNED_BYTE3)
	{
		UpdateColorBuffer(imageData, image_width, image_height, bow::ImageFormat::RedGreenBlue, bow::ImageDatatype::UnsignedByte);
	}
	else if (buffer_format == RT_FORMAT_UNSIGNED_BYTE4)
	{
		UpdateColorBuffer(imageData, image_width, image_height, bow::ImageFormat::RedGreenBlueAlpha, bow::ImageDatatype::UnsignedByte);
	}
	else
	{
		throw optix::Exception("Unknown Buffer Format!");
	}

	image_buffer->unmap();

	auto differecne = std::chrono::high_resolution_clock::now() - last_time;
	last_time = std::chrono::high_resolution_clock::now();
	const float a = 0.1f;
	m_average_frametime = (((float)differecne.count() / 1000000.0f) * a) + (m_average_frametime * (1.0f - a));
	DisplayFramerate(m_average_frametime);
}


void SimpleDirectionalLightApp::OnRelease()
{
	if (g_context)
	{
		g_context->destroy();
		g_context = 0;
	}
}

// ======================================================================
// ======================================================================

void SimpleDirectionalLightApp::createContext(int usage_report_level, UsageReportLogger* logger)
{
	// Set up g_context
	g_context = optix::Context::create();
	g_context->setRayTypeCount(2);
	g_context->setEntryPointCount(1);
	if (usage_report_level > 0)
	{
		g_context->setUsageReportCallback(usageReportCallback, usage_report_level, logger);
	}

	g_context["scene_epsilon"]->setFloat(1.e-2f);
	g_context["radiance_ray_type"]->setUint(0u);
	g_context["shadow_ray_type"]->setUint(1u);
	g_context["sqrt_num_samples"]->setUint(4);
	g_context["rr_begin_depth"]->setUint(3);

	optix::Buffer buffer = sutil::createOutputBuffer(g_context, RT_FORMAT_FLOAT4, GetWidth(), GetHeight());
	g_context["output_buffer"]->set(buffer);

	// Ray generation program
	const char *ptx = sutil::getPtxString("pathTracing/pathtracer.cu");
	optix::Program ray_gen_program = g_context->createProgramFromPTXString(ptx, "pathtrace_pinhole_camera");
	g_context->setRayGenerationProgram(0, ray_gen_program);

	// Exception program
	optix::Program exception_program = g_context->createProgramFromPTXString(ptx, "exception");
	g_context->setExceptionProgram(0, exception_program);
	g_context["bad_color"]->setFloat(1000000.0f, 0.0f, 1000000.0f); // Super magenta to make sure it doesn't get

	// Miss program
	if (!m_environment_test)
	{
		optix::Program miss_program = g_context->createProgramFromPTXString(ptx, "miss");
		g_context->setMissProgram(0, miss_program);
	}
	else
	{
		optix::Program miss_program = g_context->createProgramFromPTXString(ptx, "envmap_miss");
		g_context->setMissProgram(0, miss_program);
	}
	g_context["bg_color"]->setFloat(optix::make_float3(m_lightIntensity));
	g_context["envmap"]->setTextureSampler(sutil::loadTexture(g_context, std::string(PROJECT_BASE_DIR) + "/" + std::string("/data/CedarCity.hdr"), optix::make_float3(m_lightIntensity)));
}


void SimpleDirectionalLightApp::loadMesh(const std::string& filename, optix::GeometryGroup geometry_group, float unitsPerMeter)
{
	OptiXMesh mesh;
	mesh.context = g_context;

	const char *ptx = sutil::getPtxString("pathTracing/pathtracer.cu");

	mesh.closest_hit = g_context->createProgramFromPTXString(ptx, "microfacet_closest_hit");
	mesh.any_hit = g_context->createProgramFromPTXString(ptx, "any_hit_shadow");

	::loadMesh(filename, mesh, unitsPerMeter);

	m_aabb.set(mesh.bbox_min, mesh.bbox_max);

	geometry_group->addChild(mesh.geom_instance);
}

void SimpleDirectionalLightApp::createSphere(optix::GeometryGroup geometry_group)
{
	optix::Geometry sphere = g_context->createGeometry();
	sphere->setPrimitiveCount(1u);

	const char *ptx = sutil::getPtxString("sphere.cu");
	sphere->setBoundingBoxProgram(g_context->createProgramFromPTXString(ptx, "bounds"));
	sphere->setIntersectionProgram(g_context->createProgramFromPTXString(ptx, "intersect"));

	float sphere_loc[4] = { m_aabb.center().x + 3.0f, m_aabb.center().y, m_aabb.center().z, 1.0f };
	optix::Variable sphere_Position = sphere->declareVariable("sphere");
	sphere_Position->set4fv(&sphere_loc[0]);

	optix::Material mat = g_context->createMaterial();
	ptx = sutil::getPtxString("pathTracing/pathtracer.cu");
	mat->setClosestHitProgram(0u, g_context->createProgramFromPTXString(ptx, "microfacet_closest_hit"));
	mat->setAnyHitProgram(1u, g_context->createProgramFromPTXString(ptx, "any_hit_shadow"));

	float roughness = 0.0f;
	float metallic = 1.0f;

	// Aluminium
	mat["Kd_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(0.921f, 0.925f, 0.913f, 1.0f)));
	mat["Ks_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f - roughness, 1.0f - roughness, 1.0f - roughness, 1.0f)));
	mat["Kn_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(0.5f, 0.5f, 1.0f, 1.0f)));
	mat["Tr_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f, 1.0f, 1.0f, 1.0f)));
	mat["Pm_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(metallic, metallic, metallic, 1.0f)));
	mat["Ke_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(0.0f, 0.0f, 0.0f, 1.0f)));

	mat["index_of_refraction"]->setFloat(1.0972f);
	mat["absorption_coefficien"]->setFloat(6.7942f);


	optix::GeometryInstance geom_instance = g_context->createGeometryInstance();
	geom_instance->setGeometry(sphere);
	geom_instance->addMaterial(mat);

	geometry_group->addChild(geom_instance);
}


void SimpleDirectionalLightApp::setupCamera()
{
	const float max_dim = fmaxf(m_aabb.extent(0), m_aabb.extent(1)); // max of x, y components

	m_camera = new bow::FirstPersonCamera(
		bow::Vector3<double>(m_aabb.center().x, m_aabb.center().y, m_aabb.center().z),	// Position
		bow::Vector3<double>(m_aabb.center().x, m_aabb.center().y, m_aabb.center().z) + bow::Vector3<double>(1.0f, 0.0f, 0.0f),	// LookAt
		bow::Vector3<double>(0.0, 1.0, 0.0),	// WorldUp
		GetWidth(), GetHeight()
		);

	m_camera->SetClippingPlanes(0.01, 10000.0);

	m_lastCursorPosition = m_mouse->VGetAbsolutePositionInsideWindow();
}


void SimpleDirectionalLightApp::setupLights()
{
	const float max_dim = fmaxf(m_aabb.extent(0), m_aabb.extent(1)); // max of x, y components

	BasicLight lights[] = {
		{ optix::make_float3(0.3f, 0.9f, 0.3f), optix::make_float3(0.7f, 0.7f, 0.65f), 1, 0 }
	};

	lights[0].pos *= max_dim * 10.0f;

	optix::Buffer light_buffer = g_context->createBuffer(RT_BUFFER_INPUT);
	light_buffer->setFormat(RT_FORMAT_USER);
	light_buffer->setElementSize(sizeof(BasicLight));
	light_buffer->setSize(sizeof(lights) / sizeof(lights[0]));

	memcpy(light_buffer->map(), lights, sizeof(lights));
	light_buffer->unmap();

	g_context["lights"]->set(light_buffer);
}


void SimpleDirectionalLightApp::updateCamera()
{
	bow::Matrix3D<double> viewMatrix = m_camera->CalculateView();
	bow::Matrix3D<double> rayViewMatrix = viewMatrix.Inverse();

	if (m_camera_changed) // reset accumulation
		m_frame_number = 1;
	m_camera_changed = false;

	g_context["frame_number"]->setUint(m_frame_number++);
	g_context["U"]->setFloat(optix::make_float3(rayViewMatrix._11, rayViewMatrix._21, rayViewMatrix._31));
	g_context["V"]->setFloat(optix::make_float3(rayViewMatrix._12, rayViewMatrix._22, rayViewMatrix._32));
	g_context["W"]->setFloat(optix::make_float3(rayViewMatrix._13, rayViewMatrix._23, rayViewMatrix._33));
	g_context["eye"]->setFloat(optix::make_float3(rayViewMatrix._14, rayViewMatrix._24, rayViewMatrix._34));
}
