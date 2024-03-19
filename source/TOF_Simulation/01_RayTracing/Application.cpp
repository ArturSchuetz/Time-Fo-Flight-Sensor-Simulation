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
	loadMesh(std::string(PROJECT_BASE_DIR) + std::string("/data/Scenes/Sponza/sponza.obj"), geometry_group, 100.0f);
	createSphere1(geometry_group);
	createSphere2(geometry_group);
	createSphere3(geometry_group);

	geometry_group->setAcceleration(g_context->createAcceleration("Trbvh"));
	g_context["top_object"]->set(geometry_group);
	g_context["top_shadower"]->set(geometry_group);

	setupCamera();
	setupLights();

	g_context->validate();
}


void SimpleDirectionalLightApp::OnResized(unsigned int newWidth, unsigned int newHeight)
{
	sutil::ensureMinimumSize(newWidth, newHeight);
	sutil::resizeBuffer(getOutputBuffer(), newWidth, newHeight);

	m_camera->SetResolution(newWidth, newHeight);
}


void SimpleDirectionalLightApp::OnUpdate(double deltaTime)
{
	double moveSpeed = 10000.0;

	if (m_keyboard->VIsPressed(bow::Key::K_LEFT_SHIFT))
	{
		moveSpeed = moveSpeed * 2.0;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_W))
	{
		m_camera->MoveForward(moveSpeed * deltaTime);
	}

	if (m_keyboard->VIsPressed(bow::Key::K_S))
	{
		m_camera->MoveBackward(moveSpeed * deltaTime);
	}

	if (m_keyboard->VIsPressed(bow::Key::K_D))
	{
		m_camera->MoveRight(moveSpeed * deltaTime);
	}

	if (m_keyboard->VIsPressed(bow::Key::K_A))
	{
		m_camera->MoveLeft(moveSpeed * deltaTime);
	}

	if (m_keyboard->VIsPressed(bow::Key::K_SPACE))
	{
		m_camera->MoveUp(moveSpeed * deltaTime);
	}

	if (m_keyboard->VIsPressed(bow::Key::K_LEFT_CONTROL))
	{
		m_camera->MoveDown(moveSpeed * deltaTime);
	}

	if (m_mouse->VIsPressed(bow::MouseButton::MOFS_BUTTON1))
	{
		m_window->VHideCursor();
		bow::Vector3<long> moveVec = m_mouse->VGetAbsolutePositionInsideWindow() - m_lastCursorPosition;
		m_camera->rotate((float)moveVec.x, (float)moveVec.y);
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

	g_context["max_depth"]->setInt(1);
	g_context["radiance_ray_type"]->setUint(0u);
	g_context["shadow_ray_type"]->setUint(1u);
	g_context["scene_epsilon"]->setFloat(0.01f);

	optix::Buffer buffer = sutil::createOutputBuffer(g_context, RT_FORMAT_UNSIGNED_BYTE4, GetWidth(), GetHeight());
	g_context["output_buffer"]->set(buffer);

	// Ray generation program
	const char *ptx = sutil::getPtxString("rayTracing/camera.cu");
	optix::Program ray_gen_program = g_context->createProgramFromPTXString(ptx, "pinhole_camera");
	g_context->setRayGenerationProgram(0, ray_gen_program);

	// Exception program
	optix::Program exception_program = g_context->createProgramFromPTXString(ptx, "exception");
	g_context->setExceptionProgram(0, exception_program);
	g_context["bad_color"]->setFloat(1.0f, 0.0f, 1.0f);

	// Miss program
	optix::Program miss_program = g_context->createProgramFromPTXString(ptx, "miss");
	g_context->setMissProgram(0, miss_program);
	g_context["bg_color"]->setFloat(0.34f, 0.55f, 0.85f);
}

void SimpleDirectionalLightApp::loadMesh(const std::string& filename, optix::GeometryGroup geometry_group, float unitsPerMeter)
{
	OptiXMesh mesh;
	mesh.context = g_context;

	const char *ptx = sutil::getPtxString("TimeOfFlightRendering/pathtracer.cu");

	mesh.closest_hit = g_context->createProgramFromPTXString(ptx, "microfacet_closest_hit");
	mesh.any_hit = g_context->createProgramFromPTXString(ptx, "any_hit_shadow");

	::loadMesh(filename, mesh, unitsPerMeter);

	m_aabb.set(mesh.bbox_min, mesh.bbox_max);

	geometry_group->addChild(mesh.geom_instance);
}

void SimpleDirectionalLightApp::createSphere1(optix::GeometryGroup geometry_group)
{
	optix::Geometry sphere = g_context->createGeometry();
	sphere->setPrimitiveCount(1u);

	const char *ptx = sutil::getPtxString("sphere.cu");
	sphere->setBoundingBoxProgram(g_context->createProgramFromPTXString(ptx, "bounds"));
	sphere->setIntersectionProgram(g_context->createProgramFromPTXString(ptx, "intersect"));

	float sphere_loc[4] = { m_aabb.center().x - 2.0f, m_aabb.center().y, m_aabb.center().z, 1.0f };
	optix::Variable sphere_Position = sphere->declareVariable("sphere");
	sphere_Position->set4fv(&sphere_loc[0]);

	optix::Material mat = g_context->createMaterial();
	ptx = sutil::getPtxString("TimeOfFlightRendering/pathtracer.cu");
	mat->setClosestHitProgram(0u, g_context->createProgramFromPTXString(ptx, "microfacet_closest_hit"));
	mat->setAnyHitProgram(1u, g_context->createProgramFromPTXString(ptx, "any_hit_shadow"));

	float roughness = 0.01f;
	float metallic = 1.0f;

	mat["Kd_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(0.921f, 0.925f, 0.913f, 1.0f)));
	mat["Ks_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f - roughness, 1.0f - roughness, 1.0f - roughness, 1.0f)));
	mat["Kn_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f, 0.5f, 0.5f, 1.0f)));
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

void SimpleDirectionalLightApp::createSphere2(optix::GeometryGroup geometry_group)
{
	optix::Geometry sphere = g_context->createGeometry();
	sphere->setPrimitiveCount(1u);

	const char *ptx = sutil::getPtxString("sphere.cu");
	sphere->setBoundingBoxProgram(g_context->createProgramFromPTXString(ptx, "bounds"));
	sphere->setIntersectionProgram(g_context->createProgramFromPTXString(ptx, "intersect"));

	float sphere_loc[4] = { m_aabb.center().x + 2.0f, m_aabb.center().y, m_aabb.center().z, 1.0f };
	optix::Variable sphere_Position = sphere->declareVariable("sphere");
	sphere_Position->set4fv(&sphere_loc[0]);

	optix::Material mat = g_context->createMaterial();
	ptx = sutil::getPtxString("TimeOfFlightRendering/pathtracer.cu");
	mat->setClosestHitProgram(0u, g_context->createProgramFromPTXString(ptx, "microfacet_closest_hit"));
	mat->setAnyHitProgram(1u, g_context->createProgramFromPTXString(ptx, "any_hit_shadow"));

	float roughness = 0.01f;
	float metallic = 0.0f;

	mat["Kd_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(0.0f, 0.0f, 1.0f, 1.0f)));
	mat["Ks_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f - roughness, 1.0f - roughness, 1.0f - roughness, 1.0f)));
	mat["Kn_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f, 0.5f, 0.5f, 1.0f)));
	mat["Tr_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f, 1.0f, 1.0f, 1.0f)));
	mat["Pm_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(metallic, metallic, metallic, 1.0f)));
	mat["Ke_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(0.0f, 0.0f, 0.0f, 1.0f)));

	mat["index_of_refraction"]->setFloat(1.4906f);
	mat["absorption_coefficien"]->setFloat(0.0f);

	optix::GeometryInstance geom_instance = g_context->createGeometryInstance();
	geom_instance->setGeometry(sphere);
	geom_instance->addMaterial(mat);

	geometry_group->addChild(geom_instance);
}

void SimpleDirectionalLightApp::createSphere3(optix::GeometryGroup geometry_group)
{
	optix::Geometry sphere = g_context->createGeometry();
	sphere->setPrimitiveCount(1u);

	const char *ptx = sutil::getPtxString("sphere.cu");
	sphere->setBoundingBoxProgram(g_context->createProgramFromPTXString(ptx, "bounds"));
	sphere->setIntersectionProgram(g_context->createProgramFromPTXString(ptx, "intersect"));

	float sphere_loc[4] = { m_aabb.center().x + 6.0f, m_aabb.center().y, m_aabb.center().z, 1.0f };
	optix::Variable sphere_Position = sphere->declareVariable("sphere");
	sphere_Position->set4fv(&sphere_loc[0]);

	optix::Material mat = g_context->createMaterial();
	ptx = sutil::getPtxString("TimeOfFlightRendering/pathtracer.cu");
	mat->setClosestHitProgram(0u, g_context->createProgramFromPTXString(ptx, "microfacet_closest_hit"));
	mat->setAnyHitProgram(1u, g_context->createProgramFromPTXString(ptx, "any_hit_shadow"));

	float roughness = 0.01f;
	float metallic = 0.0f;

	mat["Kd_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f, 1.0f, 1.0f, 1.0f)));
	mat["Ks_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f - roughness, 1.0f - roughness, 1.0f - roughness, 1.0f)));
	mat["Kn_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f, 0.5f, 0.5f, 1.0f)));
	mat["Tr_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f, 1.0f, 1.0f, 1.0f)));
	mat["Pm_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(metallic, metallic, metallic, 1.0f)));
	mat["Ke_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(0.0f, 0.0f, 0.0f, 1.0f)));

	mat["index_of_refraction"]->setFloat(1.4906f);
	mat["absorption_coefficien"]->setFloat(0.0f);

	optix::GeometryInstance geom_instance = g_context->createGeometryInstance();
	geom_instance->setGeometry(sphere);
	geom_instance->addMaterial(mat);

	geometry_group->addChild(geom_instance);
}

void SimpleDirectionalLightApp::setupCamera()
{
	const float max_dim = fmaxf(m_aabb.extent(0), m_aabb.extent(1)); // max of x, y components

	m_camera = new bow::FirstPersonCamera(
		bow::Vector3<double>(m_aabb.center().x, m_aabb.center().y, m_aabb.center().z) + bow::Vector3<double>(0.0f, 0.0f, 0.0f),	// Position
		bow::Vector3<double>(m_aabb.center().x, m_aabb.center().y, m_aabb.center().z),	// LookAt
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
		{ optix::make_float3(-0.5f, 0.25f, -1.0f), optix::make_float3(0.2f, 0.2f, 0.25f), 0, 0 },
		{ optix::make_float3(-0.5f, 0.0f, 1.0f), optix::make_float3(0.1f, 0.1f, 0.10f), 0, 0 },
		{ optix::make_float3(0.3f, 0.9f, 0.3f), optix::make_float3(0.7f, 0.7f, 0.65f), 1, 0 }
	};

	lights[0].pos *= max_dim * 10.0f;
	lights[1].pos *= max_dim * 10.0f;
	lights[2].pos *= max_dim * 10.0f;

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
	bow::Matrix3D<double> viewProjectionMatrix = m_camera->CalculateViewProjection();
	bow::Matrix3D<double> rayProjectionMatrix = viewProjectionMatrix.Inverse();

	g_context["U"]->setFloat(optix::make_float3(rayProjectionMatrix._11, rayProjectionMatrix._21, rayProjectionMatrix._31));
	g_context["V"]->setFloat(optix::make_float3(rayProjectionMatrix._12, rayProjectionMatrix._22, rayProjectionMatrix._32));
	g_context["W"]->setFloat(optix::make_float3(rayProjectionMatrix._13, rayProjectionMatrix._23, rayProjectionMatrix._33));
	g_context["eye"]->setFloat(optix::make_float3(rayProjectionMatrix._14, rayProjectionMatrix._24, rayProjectionMatrix._34));
}
