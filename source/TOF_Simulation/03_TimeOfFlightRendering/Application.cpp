#include "Application.h"
#include <OptixUtils/Mesh.h>
#include <OptixUtils/OptiXMesh.h>
#include <OptixUtils/sutil.h>

#include <Masterthesis/cuda_config.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <iostream>     // std::cout, std::endl
#include <iomanip>      // std::setw
#include <random>
#include <mutex>

extern optix::Context g_context;

const double speedOfLight = 299792458.0;
const double frequency = 30000000.0; // 30 Mhz

struct BasicLight
{
	optix::float3 pos;
	optix::float3 color;
	optix::float3 direction;
	float  intensity;
	int    casts_shadow;
};

//------------------------------------------------------------------------------
//
//  Image Saving for Evaluation
//
//------------------------------------------------------------------------------

std::mutex g_imageQueue_mutex;
std::mutex g_irQueue_mutex;
std::mutex g_depthQueue_mutex;
std::mutex g_rangeQueue_mutex;

std::string g_outputFolder = "";
std::string g_recordingsFolderPath = "/Simulated_Recordings";

bool createDirectory(const std::string& path)
{
#ifdef __unix__ 
	const int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (-1 == dir_err)
	{
		return false;
	}
	return true;
#endif
#if defined(_WIN32) || defined(WIN32)
	return CreateDirectory(path.c_str(), NULL);
#endif
}

bool directoryExists(const std::string& path)
{
#ifdef __unix__ 
	struct stat statbuf;
	int isDir = 0;
	if (stat(path.c_str(), &statbuf) != -1)
	{
		if (S_ISDIR(statbuf.st_mode))
		{
			isDir = 1;
		}
	}
	return isDir == 1;
#endif
#if defined(_WIN32) || defined(WIN32)
	DWORD ftyp = GetFileAttributesA(path.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
#endif
}

void ImageSavingThreadProc(image_save_thread_data *my_data)
{
	std::cout << "Starting Thread" << std::endl;

	my_data->running = true;
	my_data->savingRunning = true;
	my_data->busy = false;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	unsigned int imageCount = 1;
	unsigned int depthCount = 1;
	unsigned int irCount = 1;
	unsigned int rangeCount = 1;

	while ((!my_data->stopThread || my_data->images.size() > 0 || my_data->depth.size() > 0 || my_data->ir.size() > 0 || my_data->range.size() > 0))
	{
		my_data->busy = my_data->images.size() > 0 || my_data->depth.size() > 0 || my_data->ir.size() > 0 || my_data->range.size() > 0;

		g_imageQueue_mutex.lock();
		bool newImageData = false;
		if (my_data->images.size() > 0)
		{
			newImageData = true;
		}
		g_imageQueue_mutex.unlock();

		if (newImageData)
		{
			g_imageQueue_mutex.lock();
			std::pair<long long, cv::Mat> temp_image = my_data->images.front();
			my_data->images.pop();
			g_imageQueue_mutex.unlock();

			if (temp_image.second.rows > 0 && temp_image.second.cols > 0)
			{
				std::string fileName = g_outputFolder + "/Image_";

				if (temp_image.first < 1000000000000)
					fileName.append(std::to_string(0));
				if (temp_image.first < 100000000000)
					fileName.append(std::to_string(0));
				if (temp_image.first < 10000000000)
					fileName.append(std::to_string(0));
				if (temp_image.first < 1000000000)
					fileName.append(std::to_string(0));
				if (temp_image.first < 100000000)
					fileName.append(std::to_string(0));
				if (temp_image.first < 10000000)
					fileName.append(std::to_string(0));
				if (temp_image.first < 1000000)
					fileName.append(std::to_string(0));
				if (temp_image.first < 100000)
					fileName.append(std::to_string(0));
				if (temp_image.first < 10000)
					fileName.append(std::to_string(0));
				if (temp_image.first < 1000)
					fileName.append(std::to_string(0));
				if (temp_image.first < 100)
					fileName.append(std::to_string(0));
				if (temp_image.first < 10)
					fileName.append(std::to_string(0));

				fileName.append(std::to_string(temp_image.first));
				fileName.append(".png");

				imageCount++;

				cv::cvtColor(temp_image.second, temp_image.second, CV_BGR2RGB);
				cv::imwrite(fileName.c_str(), temp_image.second);
			}
		}

		g_depthQueue_mutex.lock();
		bool newDepthData = false;
		if (my_data->depth.size() > 0)
		{
			newDepthData = true;
		}
		g_depthQueue_mutex.unlock();

		if (newDepthData){
			g_depthQueue_mutex.lock();
			std::pair<long long, cv::Mat_<ushort>> temp_depth = my_data->depth.front();
			my_data->depth.pop();
			g_depthQueue_mutex.unlock();

			if (temp_depth.second.rows > 0 && temp_depth.second.cols > 0)
			{
				std::string fileName = g_outputFolder + "/Depth_";

				if (temp_depth.first < 1000000000000)
					fileName.append(std::to_string(0));
				if (temp_depth.first < 100000000000)
					fileName.append(std::to_string(0));
				if (temp_depth.first < 10000000000)
					fileName.append(std::to_string(0));
				if (temp_depth.first < 1000000000)
					fileName.append(std::to_string(0));
				if (temp_depth.first < 100000000)
					fileName.append(std::to_string(0));
				if (temp_depth.first < 10000000)
					fileName.append(std::to_string(0));
				if (temp_depth.first < 1000000)
					fileName.append(std::to_string(0));
				if (temp_depth.first < 100000)
					fileName.append(std::to_string(0));
				if (temp_depth.first < 10000)
					fileName.append(std::to_string(0));
				if (temp_depth.first < 1000)
					fileName.append(std::to_string(0));
				if (temp_depth.first < 100)
					fileName.append(std::to_string(0));
				if (temp_depth.first < 10)
					fileName.append(std::to_string(0));

				fileName.append(std::to_string(temp_depth.first));
				fileName.append(".bin");

				depthCount++;

				FILE* pFile = fopen(fileName.c_str(), "wb");
				fwrite(&(temp_depth.second.data[0]), sizeof(ushort), temp_depth.second.cols * temp_depth.second.rows, pFile);
				fclose(pFile);
			}
		}

		g_irQueue_mutex.lock();
		bool newIrData = false;
		if (my_data->ir.size() > 0)
		{
			newIrData = true;
		}
		g_irQueue_mutex.unlock();

		if (newIrData){
			g_irQueue_mutex.lock();
			std::pair<long long, cv::Mat_<ushort>> temp_ir = my_data->ir.front();
			my_data->ir.pop();
			g_irQueue_mutex.unlock();

			if (temp_ir.second.rows > 0 && temp_ir.second.cols > 0)
			{
				std::string fileName = g_outputFolder + "/Ir_";

				if (temp_ir.first < 1000000000000)
					fileName.append(std::to_string(0));
				if (temp_ir.first < 100000000000)
					fileName.append(std::to_string(0));
				if (temp_ir.first < 10000000000)
					fileName.append(std::to_string(0));
				if (temp_ir.first < 1000000000)
					fileName.append(std::to_string(0));
				if (temp_ir.first < 100000000)
					fileName.append(std::to_string(0));
				if (temp_ir.first < 10000000)
					fileName.append(std::to_string(0));
				if (temp_ir.first < 1000000)
					fileName.append(std::to_string(0));
				if (temp_ir.first < 100000)
					fileName.append(std::to_string(0));
				if (temp_ir.first < 10000)
					fileName.append(std::to_string(0));
				if (temp_ir.first < 1000)
					fileName.append(std::to_string(0));
				if (temp_ir.first < 100)
					fileName.append(std::to_string(0));
				if (temp_ir.first < 10)
					fileName.append(std::to_string(0));

				fileName.append(std::to_string(temp_ir.first));
				fileName.append(".bin");

				irCount++;

				FILE* pFile = fopen(fileName.c_str(), "wb");
				fwrite(&(temp_ir.second.data[0]), sizeof(ushort), temp_ir.second.cols * temp_ir.second.rows, pFile);
				fclose(pFile);
			}
		}

		g_rangeQueue_mutex.lock();
		bool newRangeData = false;
		if (my_data->range.size() > 0)
		{
			newRangeData = true;
		}
		g_rangeQueue_mutex.unlock();

		if (newRangeData){
			g_rangeQueue_mutex.lock();
			std::pair<long long, cv::Mat_<ushort>> temp_range = my_data->range.front();
			my_data->range.pop();
			g_rangeQueue_mutex.unlock();

			if (temp_range.second.rows > 0 && temp_range.second.cols > 0)
			{
				std::string fileName = g_outputFolder + "/Range_";

				if (temp_range.first < 1000000000000)
					fileName.append(std::to_string(0));
				if (temp_range.first < 100000000000)
					fileName.append(std::to_string(0));
				if (temp_range.first < 10000000000)
					fileName.append(std::to_string(0));
				if (temp_range.first < 1000000000)
					fileName.append(std::to_string(0));
				if (temp_range.first < 100000000)
					fileName.append(std::to_string(0));
				if (temp_range.first < 10000000)
					fileName.append(std::to_string(0));
				if (temp_range.first < 1000000)
					fileName.append(std::to_string(0));
				if (temp_range.first < 100000)
					fileName.append(std::to_string(0));
				if (temp_range.first < 10000)
					fileName.append(std::to_string(0));
				if (temp_range.first < 1000)
					fileName.append(std::to_string(0));
				if (temp_range.first < 100)
					fileName.append(std::to_string(0));
				if (temp_range.first < 10)
					fileName.append(std::to_string(0));

				fileName.append(std::to_string(temp_range.first));
				fileName.append(".bin");

				rangeCount++;

				FILE* pFile = fopen(fileName.c_str(), "wb");
				fwrite(&(temp_range.second.data[0]), sizeof(ushort), temp_range.second.cols * temp_range.second.rows, pFile);
				fclose(pFile);
			}
		}
	}

	std::cout << "Stopping Thread" << std::endl;
	my_data->running = false;
	my_data->savingRunning = false;

	return;
}

//------------------------------------------------------------------------------
//
//  Helper functions
//
//------------------------------------------------------------------------------


optix::Buffer getOutputBuffer()
{
	return g_context["output_buffer"]->getBuffer();
}

optix::Buffer getBucketBufferPulse()
{
	return g_context["output_buckets_pulse"]->getBuffer();
}

optix::Buffer getBucketBufferRect()
{
	return g_context["output_buckets_rect"]->getBuffer();
}

optix::Buffer getBucketBufferSin()
{
	return g_context["output_buckets_sin"]->getBuffer();
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


Time_of_Flight_App::Time_of_Flight_App() : m_logger(nullptr), m_usage_report_level(0), m_camera(nullptr), m_noise_enabled(false), m_lens_scattering_enabled(true), m_save_data(false), recording_pressed(false), enable_lens_scattering_pressed(false), enable_noise_pressed(false)
{
	m_logger = new UsageReportLogger();

	m_frame_number = 1;
	m_camera_changed = true;
	m_use_model = 2;

	m_irLightIntensity = 10.0f;
	m_ambientSunLightIntensity = 25.0f;

	//m_filter_kernel = { 1.000000f, 0.395913f, 0.031601f, 0.009520f, 0.006271f, 0.005221f, 0.005390f, 0.004269f, 0.000251f, -0.000544f, 0.002034f, 0.002611f, 0.003903f, 0.002601f, 0.001757f, 0.001266f, 0.001923f, 0.001780f, 0.001547f, 0.001489f, 0.001970f, 0.001539f, 0.002545f, 0.001665f, 0.000565f, 0.001135f, 0.000959f, 0.001087f, 0.000871f, 0.000231f, -0.000098f, -0.001704f, 0.001173f, -0.000253f, -0.001245f, -0.000133f, -0.000365f, -0.000160f, 0.001949f, 0.002857f, 0.003433f, 0.002271f, 0.003037f, -0.003769f, -0.000920f, 0.001407f, 0.001840f, 0.000169f, -0.000299f, 0.000578f, 0.001130f, 0.001324f, 0.002545f, -0.000502f, 0.002861f, 0.004686f, 0.000670f, 0.001436f, 0.000645f, 0.001079f, 0.001155f, -0.000506f, 0.000812f, 0.000738f, -0.001952f, 0.000437f, -0.000099f, 0.000611f, -0.000579f, 0.003307f, 0.000896f, 0.001690f, 0.000336f, 0.002996f, 0.001887f, 0.003056f, 0.000465 };
	m_filter_kernel = { 1.000000, 0.395913, 0.031601, 0.009520, 0.006271, 0.005221, 0.005390, 0.004269, 0.000251, 0.002034 };

	if (!directoryExists(g_recordingsFolderPath))
	{
		while (!createDirectory(g_recordingsFolderPath) && !directoryExists(g_recordingsFolderPath))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	m_threadData.stopThread = false;
	m_threadData.savingRunning = false;
	m_imageSavingThread = std::thread([this](){ ImageSavingThreadProc(&m_threadData); });
}


Time_of_Flight_App::~Time_of_Flight_App()
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

	m_threadData.stopThread = true;

	std::cout << "Waiting for thread to stop..." << std::endl;
	while (m_threadData.savingRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	m_imageSavingThread.join();
}

// ======================================================================
// ======================================================================

void Time_of_Flight_App::OnInit(bow::IntrinsicCameraParameters cameraParameters)
{
	m_width = cameraParameters.image_width;
	m_height = cameraParameters.image_height;

	createContext(m_usage_report_level, m_logger);

	optix::GeometryGroup geometry_group = g_context->createGeometryGroup();
	//loadMesh(std::string(PROJECT_BASE_DIR) + std::string("/data/Scenes/EvaluationGeometry/BoxScene.obj"), geometry_group, 1.0f);
	//loadMesh(std::string(PROJECT_BASE_DIR) + std::string("/data/Scenes/EvaluationGeometry/Edge.obj"), geometry_group, 1.0f);
	//loadMesh(std::string(PROJECT_BASE_DIR) + std::string("/data/Scenes/EvaluationGeometry/Wall.obj"), geometry_group, 0.0001f);
	loadMesh(std::string(PROJECT_BASE_DIR) + std::string("/data/Scenes/Sponza/sponza.obj"), geometry_group, 100.0f);
	createSphere0(geometry_group);
	createSphere1(geometry_group);
	createSphere2(geometry_group);
	createSphere3(geometry_group);

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


void Time_of_Flight_App::OnResized(unsigned int newWidth, unsigned int newHeight)
{

}


void Time_of_Flight_App::OnUpdate(double deltaTime)
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

	// increase ir light
	if (m_keyboard->VIsPressed(bow::Key::K_KP_9))
	{
		m_irLightIntensity += 1.0f;
		m_camera_changed = true;
	}

	// decrease ir light
	if (m_keyboard->VIsPressed(bow::Key::K_KP_6))
	{
		m_irLightIntensity -= 1.0f;
		if (m_irLightIntensity < 0.0f)
			m_irLightIntensity = 0.0f;
		m_camera_changed = true;
	}

	// increase ambient light
	if (m_keyboard->VIsPressed(bow::Key::K_KP_8))
	{
		m_ambientSunLightIntensity += 1.0f;
		m_camera_changed = true;
	}

	// decrease ambient light
	if (m_keyboard->VIsPressed(bow::Key::K_KP_5))
	{
		m_ambientSunLightIntensity -= 1.0f;
		if (m_ambientSunLightIntensity < 0.0f)
			m_ambientSunLightIntensity = 0.0f;
		m_camera_changed = true;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_KP_1))
	{
		m_use_model = 0;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_KP_2))
	{
		m_use_model = 1;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_KP_3))
	{
		m_use_model = 2;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_R))
	{
		if (!recording_pressed)
		{
			recording_pressed = true;
			if (!m_save_data)
			{
				if (!m_threadData.busy)
				{
					unsigned int c = 0;
					while (!createDirectory(g_recordingsFolderPath + "/" + "Run_" + std::to_string(c)))
					{
						c++;
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
					}
					g_outputFolder = g_recordingsFolderPath + "/" + std::string("Run_") + std::to_string(c);

					m_save_data = true;
					std::cout << "Recording started!" << std::endl;
				}
				else
				{
					std::cout << "Thread is busy with saving files. Please wait!" << std::endl;
				}
			}
			else
			{
				m_save_data = false;
				std::cout << "Recording stopped!" << std::endl;
			}
		}
	}
	else
	{
		recording_pressed = false;
	}


	if (m_keyboard->VIsPressed(bow::Key::K_T))
	{
		if (!enable_lens_scattering_pressed)
		{
			enable_lens_scattering_pressed = true;
			if (!m_lens_scattering_enabled)
			{
				m_lens_scattering_enabled = true;
				std::cout << "Lens Scattering enabled!" << std::endl;
			}
			else
			{
				m_lens_scattering_enabled = false;
				std::cout << "Lens Scattering disabled!" << std::endl;
			}
		}
	}
	else
	{
		enable_lens_scattering_pressed = false;
	}

	if (m_keyboard->VIsPressed(bow::Key::K_Z))
	{
		if (!enable_noise_pressed)
		{
			enable_noise_pressed = true;
			if (!m_noise_enabled)
			{
				m_noise_enabled = true;
				std::cout << "Noise enabled!" << std::endl;
			}
			else
			{
				m_noise_enabled = false;
				std::cout << "Noise disabled!" << std::endl;
			}
		}
	}
	else
	{
		enable_noise_pressed = false;
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
	updateLights();
	m_camera_changed = false;
}

uchar clamp(float value)
{
	if (value < 0.0f)
		return 0;
	else if (value > 255.0f)
		return 255;
	else
		return (uchar)value;
}

std::default_random_engine g_generator;

void Time_of_Flight_App::OnRender()
{
	long long seconds = clock();
	g_context->launch(0, m_width, m_height);
	{
		optix::Buffer image_buffer = getOutputBuffer();

		RTsize buffer_width_rts, buffer_height_rts;
		image_buffer->getSize(buffer_width_rts, buffer_height_rts);
		uint32_t image_width = static_cast<int>(buffer_width_rts);
		uint32_t image_height = static_cast<int>(buffer_height_rts);
		RTformat buffer_format = image_buffer->getFormat();

		void* imageData = image_buffer->map(0, RT_BUFFER_MAP_READ);

		if (buffer_format == RT_FORMAT_FLOAT3)
		{
			if (m_save_data)
			{
				cv::Mat imageMat = cv::Mat(image_height, image_width, CV_8UC3);
				for (unsigned int launch_index = 0; launch_index < image_width * image_height; launch_index++)
				{
					imageMat.at<cv::Vec3b>(launch_index) = cv::Vec3b(clamp(((float*)imageData)[launch_index * 3] * 255.0f), clamp(((float*)imageData)[launch_index * 3 + 1] * 255.0f), clamp(((float*)imageData)[launch_index * 3 + 2] * 255.0f));
				}

				g_imageQueue_mutex.lock();
				m_threadData.images.push(std::pair<long long, cv::Mat>(seconds, imageMat.clone()));
				g_imageQueue_mutex.unlock();
			}
			UpdateColorBuffer(imageData, image_width, image_height, bow::ImageFormat::RedGreenBlue, bow::ImageDatatype::Float);
		}
		else if (buffer_format == RT_FORMAT_FLOAT4)
		{
			if (m_save_data)
			{
				cv::Mat imageMat = cv::Mat(image_height, image_width, CV_8UC3);
				#pragma parallel for
				for (unsigned int launch_index = 0; launch_index < image_width * image_height; launch_index++)
				{
					imageMat.at<cv::Vec3b>(launch_index) = cv::Vec3b(clamp(((float*)imageData)[launch_index * 4] * 255.0f), clamp(((float*)imageData)[launch_index * 4 + 1] * 255.0f), clamp(((float*)imageData)[launch_index * 4 + 2] * 255.0f));
				}

				g_imageQueue_mutex.lock();
				m_threadData.images.push(std::pair<long long, cv::Mat>(seconds, imageMat.clone()));
				g_imageQueue_mutex.unlock();
			}
			UpdateColorBuffer(imageData, image_width, image_height, bow::ImageFormat::RedGreenBlueAlpha, bow::ImageDatatype::Float);
		}
		else if (buffer_format == RT_FORMAT_UNSIGNED_BYTE3)
		{
			if (m_save_data)
			{
				cv::Mat imageMat = cv::Mat(image_height, image_width, CV_8UC3);
				#pragma omp parallel for
				for (int launch_index = 0; launch_index < image_width * image_height; launch_index++)
				{
					imageMat.at<cv::Vec3b>(launch_index) = cv::Vec3b((uchar)((uchar*)imageData)[launch_index * 3], (uchar)((uchar*)imageData)[launch_index * 3 + 1], (uchar)((uchar*)imageData)[launch_index * 3 + 2]);
				}

				g_imageQueue_mutex.lock();
				m_threadData.images.push(std::pair<long long, cv::Mat>(seconds, imageMat.clone()));
				g_imageQueue_mutex.unlock();
			}
			UpdateColorBuffer(imageData, image_width, image_height, bow::ImageFormat::RedGreenBlue, bow::ImageDatatype::UnsignedByte);
		}
		else if (buffer_format == RT_FORMAT_UNSIGNED_BYTE4)
		{
			if (m_save_data)
			{
				cv::Mat imageMat = cv::Mat(image_height, image_width, CV_8UC3);
				#pragma omp parallel for
				for (int launch_index = 0; launch_index < image_width * image_height; launch_index++)
				{
					imageMat.at<cv::Vec3b>(launch_index) = cv::Vec3b((uchar)((uchar*)imageData)[launch_index * 4], (uchar)((uchar*)imageData)[launch_index * 4 + 1], (uchar)((uchar*)imageData)[launch_index * 4 + 2]);
				}
				g_imageQueue_mutex.lock();
				m_threadData.images.push(std::pair<long long, cv::Mat>(seconds, imageMat.clone()));
				g_imageQueue_mutex.unlock();
			}
			UpdateColorBuffer(imageData, image_width, image_height, bow::ImageFormat::RedGreenBlueAlpha, bow::ImageDatatype::UnsignedByte);
		}
		else
		{
			throw optix::Exception("Unknown Buffer Format!");
		}

		image_buffer->unmap();
	}

	{
		optix::Buffer bucket_buffer;
		if (m_use_model == 0)
		{
			bucket_buffer = getBucketBufferPulse();
		}
		else if (m_use_model == 1)
		{
			bucket_buffer = getBucketBufferRect();
		}
		else if (m_use_model == 2)
		{
			bucket_buffer = getBucketBufferSin();
		}

		RTsize buffer_width_rts, buffer_height_rts;
		bucket_buffer->getSize(buffer_width_rts, buffer_height_rts);
		uint32_t image_width = static_cast<int>(buffer_width_rts);
		uint32_t image_height = static_cast<int>(buffer_height_rts);
		RTformat buffer_format = bucket_buffer->getFormat();
		void* imageData = bucket_buffer->map(0, RT_BUFFER_MAP_READ);

		//cv::Mat_<uchar> phaseImage_C1 = cv::Mat_<uchar>(image_height, image_width);
		//cv::Mat_<uchar> phaseImage_C2 = cv::Mat_<uchar>(image_height, image_width);
		//cv::Mat_<uchar> phaseImage_C3 = cv::Mat_<uchar>(image_height, image_width);
		//cv::Mat_<uchar> phaseImage_C4 = cv::Mat_<uchar>(image_height, image_width);

		if (buffer_format == RT_FORMAT_FLOAT4)
		{
			float* output_buckets = (float*)imageData;
			float* output_intensity = new float[image_width * image_height];
			float* output_depth = new float[image_width * image_height];

			if (m_lens_scattering_enabled)
			{
				float* scattered_output_buckets = new float[image_width * image_height * 4];
				memset(scattered_output_buckets, 0, sizeof(float) * image_width * image_height * 4);
				for (int row = 0; row < image_height; row++)
				{
					#pragma omp parallel for
					for (int col = 0; col < image_width; col++)
					{
						int current_pixel_launch_index = col + (row * image_width);

						for (int i = -(m_filter_kernel.size() - 1); i < (int)m_filter_kernel.size(); i++)
						{
							int launch_index_row = (col + i);
							if (launch_index_row >= 0 && launch_index_row < image_width)
							{
								int launch_index = (col + i) + (row * image_width);
								scattered_output_buckets[(current_pixel_launch_index * 4) + 0] += output_buckets[((launch_index)* 4) + 0] * m_filter_kernel[abs(i)] * 0.5f;
								scattered_output_buckets[(current_pixel_launch_index * 4) + 1] += output_buckets[((launch_index)* 4) + 1] * m_filter_kernel[abs(i)] * 0.5f;
								scattered_output_buckets[(current_pixel_launch_index * 4) + 2] += output_buckets[((launch_index)* 4) + 2] * m_filter_kernel[abs(i)] * 0.5f;
								scattered_output_buckets[(current_pixel_launch_index * 4) + 3] += output_buckets[((launch_index)* 4) + 3] * m_filter_kernel[abs(i)] * 0.5f;
							}
						}
					}
				}

				memset(output_buckets, 0, sizeof(float) * image_width * image_height * 4);
				
				for (int col = 0; col < image_width; col++)
				{
					#pragma omp parallel for
					for (int row = 0; row < image_height; row++)
					{
						int current_pixel_launch_index = col + (row * image_width);
						for (int i = -(m_filter_kernel.size() - 1); i < (int)m_filter_kernel.size(); i++)
						{
							int launch_index_col = (row + i);
							if (launch_index_col >= 0 && launch_index_col < image_height)
							{
								int launch_index = col + ((row + i) * image_width);
								output_buckets[(current_pixel_launch_index * 4) + 0] += scattered_output_buckets[((launch_index)* 4) + 0] * m_filter_kernel[abs(i)] * 0.5f;
								output_buckets[(current_pixel_launch_index * 4) + 1] += scattered_output_buckets[((launch_index)* 4) + 1] * m_filter_kernel[abs(i)] * 0.5f;
								output_buckets[(current_pixel_launch_index * 4) + 2] += scattered_output_buckets[((launch_index)* 4) + 2] * m_filter_kernel[abs(i)] * 0.5f;
								output_buckets[(current_pixel_launch_index * 4) + 3] += scattered_output_buckets[((launch_index)* 4) + 3] * m_filter_kernel[abs(i)] * 0.5f;
							}
						}
					}
				}

				delete[] scattered_output_buckets;
			}

			float maxDistanceInMeter = (speedOfLight / (2.0 * frequency));
			#pragma omp parallel for
			for (int launch_index = 0; launch_index < image_width * image_height; launch_index++)
			{
				float* ir_buckets_sum = &output_buckets[launch_index * 4];

				if (ir_buckets_sum[0] < 0)
					ir_buckets_sum[0] = 0;
				if (ir_buckets_sum[1] < 0)
					ir_buckets_sum[1] = 0;
				if (ir_buckets_sum[2] < 0)
					ir_buckets_sum[2] = 0;
				if (ir_buckets_sum[3] < 0)
					ir_buckets_sum[3] = 0;

				float Intensity = sqrt(((ir_buckets_sum[2] - ir_buckets_sum[3]) * (ir_buckets_sum[2] - ir_buckets_sum[3])) + ((ir_buckets_sum[0] - ir_buckets_sum[1]) * (ir_buckets_sum[0] - ir_buckets_sum[1]))) * 0.5;
				float Offset = (ir_buckets_sum[0] + ir_buckets_sum[1] + ir_buckets_sum[2] + ir_buckets_sum[3]) / 4.0;

				//float C1 = (ir_buckets_sum[0]) * 255.0f;
				//if (C1 > 255.0f)
				//	C1 = 255.0f;
				//else if (C1 < 0)
				//	C1 = 0.0;
				//phaseImage_C1.at<uchar>(launch_index) = C1;

				//float C2 = (ir_buckets_sum[1]) * 255.0f;
				//if (C2 > 255.0f)
				//	C2 = 255.0f;
				//else if (C2 < 0)
				//	C2 = 0.0;
				//phaseImage_C2.at<uchar>(launch_index) = C2;

				//float C3 = (ir_buckets_sum[2]) * 255.0f;
				//if (C3 > 255.0f)
				//	C3 = 255.0f;
				//else if (C3 < 0)
				//	C3 = 0.0;
				//phaseImage_C3.at<uchar>(launch_index) = C3;

				//float C4 = (ir_buckets_sum[3]) * 255.0f;
				//if (C4 > 255.0f)
				//	C4 = 255.0f;
				//else if (C4 < 0)
				//	C4 = 0.0;
				//phaseImage_C4.at<uchar>(launch_index) = C4;

				const float modulation_contrast = 60000.0f;
				const double log_multiplicator = 2.0;
				output_intensity[launch_index] = cv::log((double)(Intensity * log_multiplicator) + 1.0) / (cv::log(log_multiplicator + 1.0));
				float standard_deviation = (speedOfLight / ((4.0f * sqrt(2.0f) * M_PIf * frequency))) * sqrt(Intensity + Offset) / (modulation_contrast * Intensity);

				if ((ir_buckets_sum[0] - ir_buckets_sum[1]) != 0.0f && Intensity > (200.0f / 65000.0f))
				{
					float phi = atan2((ir_buckets_sum[2] - ir_buckets_sum[3]), (ir_buckets_sum[0] - ir_buckets_sum[1]));

					if (phi < 0.0f)
						phi = (2.0 * M_PIf) + phi;

					float distance = (speedOfLight / (4.0f * M_PIf * frequency)) * phi;
					if (distance != 0.0f && m_noise_enabled)
					{
						// Noisce Calculation by using variance and normal distribution function
						std::normal_distribution<double> distribution(distance, sqrt(standard_deviation));
						double imperfectDistance = distribution(g_generator);

						output_depth[launch_index] = imperfectDistance * 1000.0f;
					}
					else
					{
						output_depth[launch_index] = distance * 1000.0f;
					}
				}
				else
				{
					output_depth[launch_index] = 0.0f;
				}
			}

			if (m_save_data)
			{
				cv::Mat irMat = cv::Mat(image_height, image_width, CV_16UC1);
				
				#pragma omp parallel for
				for (int launch_index = 0; launch_index < image_width * image_height; launch_index++)
				{
					irMat.at<unsigned short>(launch_index) = (unsigned short)(output_intensity[launch_index]);
				}
				g_irQueue_mutex.lock();
				m_threadData.ir.push(std::pair<long long, cv::Mat>(seconds, irMat.clone()));
				g_irQueue_mutex.unlock();

				cv::Mat rangeMat = cv::Mat(image_height, image_width, CV_16UC1);
				
				#pragma omp parallel for
				for (int launch_index = 0; launch_index < image_width * image_height; launch_index++)
				{
					unsigned short range_value = (unsigned short)(output_depth[launch_index]);
					rangeMat.at<unsigned short>(launch_index) = range_value;
				}
				g_depthQueue_mutex.lock();
				m_threadData.range.push(std::pair<long long, cv::Mat>(seconds, rangeMat.clone()));
				g_depthQueue_mutex.unlock();
			}

			UpdateIRBuffer(output_intensity, image_width, image_height, bow::ImageFormat::Red, bow::ImageDatatype::Float);
			UpdateDepthBuffer(output_depth, maxDistanceInMeter * 1000.0f, image_width, image_height, bow::ImageFormat::Red, bow::ImageDatatype::Float);

			delete[] output_depth;
			delete[] output_intensity;
		}
		else if (buffer_format == RT_FORMAT_FLOAT2)
		{
			float* output_buckets = (float*)imageData;
			float* output_depth = new float[image_width * image_height];
			float* output_intensity = new float[image_width * image_height];

			const double pulselength = (1.0 / frequency) * 0.5f;
			float maxDistanceInMeter = ((speedOfLight * pulselength) * 0.5f);
			#pragma omp parallel for
			for (int launch_index = 0; launch_index < image_width * image_height; launch_index++)
			{
				float* ir_buckets_sum = &output_buckets[launch_index * 2];
				float Intensity = (ir_buckets_sum[0] + ir_buckets_sum[1]);

				//float C1 = (ir_buckets_sum[0]) * 255.0f;
				//if (C1 > 255.0f)
				//	C1 = 255.0f;
				//else if (C1 < 0)
				//	C1 = 0.0;
				//phaseImage_C1.at<uchar>(launch_index) = C1;

				//float C2 = (ir_buckets_sum[1]) * 255.0f;
				//if (C2 > 255.0f)
				//	C2 = 255.0f;
				//else if (C2 < 0)
				//	C2 = 0.0;
				//phaseImage_C2.at<uchar>(launch_index) = C2;

				const float modulation_contrast = 10.000f;
				const double log_multiplicator = 2.0;
				output_intensity[launch_index] = cv::log((double)(Intensity * log_multiplicator) + 1.0) / (cv::log(log_multiplicator + 1.0));

				if (ir_buckets_sum[0] + ir_buckets_sum[1] > (200.0f / 65000.0f))
				{
					output_depth[launch_index] = (0.5f * speedOfLight * pulselength * (ir_buckets_sum[1] / (ir_buckets_sum[0] + ir_buckets_sum[1]))) * 1000.0f;
				}
				else
				{
					output_depth[launch_index] = 0.0f;
				}
			}

			if (m_save_data)
			{
				cv::Mat irMat = cv::Mat(image_height, image_width, CV_16UC1);
				#pragma omp parallel for
				for (int launch_index = 0; launch_index < image_width * image_height; launch_index++)
				{
					irMat.at<unsigned short>(launch_index) = (unsigned short)(output_intensity[launch_index]);
				}
				g_irQueue_mutex.lock();
				m_threadData.ir.push(std::pair<long long, cv::Mat>(seconds, irMat.clone()));
				g_irQueue_mutex.unlock();

				cv::Mat rangeMat = cv::Mat(image_height, image_width, CV_16UC1);
				#pragma omp parallel for
				for (int launch_index = 0; launch_index < image_width * image_height; launch_index++)
				{
					unsigned short range_value = (unsigned short)(output_depth[launch_index]);
					rangeMat.at<unsigned short>(launch_index) = range_value;
				}
				g_depthQueue_mutex.lock();
				m_threadData.range.push(std::pair<long long, cv::Mat>(seconds, rangeMat.clone()));
				g_depthQueue_mutex.unlock();
			}

			UpdateIRBuffer(output_intensity, image_width, image_height, bow::ImageFormat::Red, bow::ImageDatatype::Float);
			UpdateDepthBuffer(output_depth, maxDistanceInMeter * 1000.0f, image_width, image_height, bow::ImageFormat::Red, bow::ImageDatatype::Float);

			delete[] output_depth;
			delete[] output_intensity;
		}
		else
		{
			throw optix::Exception("Unknown Buffer Format!");
		}
		bucket_buffer->unmap();

		//cv::imwrite("phase_C1.png", phaseImage_C1);
		//cv::imwrite("phase_C2.png", phaseImage_C2);
		//cv::imwrite("phase_C3.png", phaseImage_C3);
		//cv::imwrite("phase_C4.png", phaseImage_C4);
	}
}


void Time_of_Flight_App::OnRelease()
{

	if (g_context)
	{
		g_context->destroy();
		g_context = 0;
	}
}


// ======================================================================
// ======================================================================

void Time_of_Flight_App::createContext(int usage_report_level, UsageReportLogger* logger)
{
	// Set up g_context
	g_context = optix::Context::create();
	g_context->setRayTypeCount(2);
	g_context->setEntryPointCount(1);
	if (usage_report_level > 0)
	{
		g_context->setUsageReportCallback(usageReportCallback, usage_report_level, logger);
	}

	g_context["scene_epsilon"]->setFloat(1.e-6f);
	g_context["radiance_ray_type"]->setUint(0u);
	g_context["shadow_ray_type"]->setUint(1u);
	g_context["sqrt_num_samples"]->setUint(1);
	g_context["rr_begin_depth"]->setUint(3);
	g_context["max_depth"]->setUint(8);
	g_context["frequency"]->setFloat((float)frequency);

	optix::Buffer buffer = sutil::createOutputBuffer(g_context, RT_FORMAT_FLOAT4, m_width, m_height);
	g_context["output_buffer"]->set(buffer);

	optix::Buffer buckets_pulse = sutil::createOutputBuffer(g_context, RT_FORMAT_FLOAT2, m_width, m_height);
	g_context["output_buckets_pulse"]->set(buckets_pulse);

	optix::Buffer buckets_rect = sutil::createOutputBuffer(g_context, RT_FORMAT_FLOAT4, m_width, m_height);
	g_context["output_buckets_rect"]->set(buckets_rect);

	optix::Buffer buckets_sin = sutil::createOutputBuffer(g_context, RT_FORMAT_FLOAT4, m_width, m_height);
	g_context["output_buckets_sin"]->set(buckets_sin);

	// Ray generation program
	const char *ptx = sutil::getPtxString("TimeOfFlightRendering/pathtracer.cu");
	optix::Program ray_gen_program = g_context->createProgramFromPTXString(ptx, "pathtrace_camera");
	g_context->setRayGenerationProgram(0, ray_gen_program);

	// Exception program
	optix::Program exception_program = g_context->createProgramFromPTXString(ptx, "exception");
	g_context->setExceptionProgram(0, exception_program);
	g_context["bad_color"]->setFloat(1000000.0f, 0.0f, 1000000.0f); // Super magenta to make sure it doesn't get

	// Miss program
	optix::Program miss_program = g_context->createProgramFromPTXString(ptx, "envmap_miss");
	g_context->setMissProgram(0, miss_program);
	g_context["bg_color"]->setFloat(optix::make_float3(m_ambientSunLightIntensity));
	g_context["envmap"]->setTextureSampler(sutil::loadTexture(g_context, std::string(PROJECT_BASE_DIR) + std::string("/data/CedarCity.hdr"), optix::make_float3(m_ambientSunLightIntensity)));
}

void Time_of_Flight_App::loadMesh(const std::string& filename, optix::GeometryGroup geometry_group, float unitsPerMeter)
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


void Time_of_Flight_App::createSphere0(optix::GeometryGroup geometry_group)
{
	optix::Geometry sphere = g_context->createGeometry();
	sphere->setPrimitiveCount(1u);

	const char *ptx = sutil::getPtxString("sphere.cu");
	sphere->setBoundingBoxProgram(g_context->createProgramFromPTXString(ptx, "bounds"));
	sphere->setIntersectionProgram(g_context->createProgramFromPTXString(ptx, "intersect"));

	float sphere_loc[4] = { m_aabb.center().x - 6.0f, m_aabb.center().y, m_aabb.center().z, 1.0f };
	optix::Variable sphere_Position = sphere->declareVariable("sphere");
	sphere_Position->set4fv(&sphere_loc[0]);

	optix::Material mat = g_context->createMaterial();
	ptx = sutil::getPtxString("TimeOfFlightRendering/pathtracer.cu");
	mat->setClosestHitProgram(0u, g_context->createProgramFromPTXString(ptx, "microfacet_closest_hit"));
	mat->setAnyHitProgram(1u, g_context->createProgramFromPTXString(ptx, "any_hit_shadow"));

	float roughness = 0.0f;
	float metallic = 1.0f;

	// Silver
	mat["Kd_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(0.972f, 0.960f, 0.915f, 1.0f)));
	mat["Ks_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f - roughness, 1.0f - roughness, 1.0f - roughness, 1.0f)));
	mat["Kn_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(0.5f, 0.5f, 1.0f, 1.0f)));
	mat["Tr_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f, 1.0f, 1.0f, 1.0f)));
	mat["Pm_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(metallic, metallic, metallic, 1.0f)));
	mat["Ke_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(0.0f, 0.0f, 0.0f, 1.0f)));

	mat["index_of_refraction"]->setFloat(0.15016f);
	mat["absorption_coefficien"]->setFloat(3.4727f);

	optix::GeometryInstance geom_instance = g_context->createGeometryInstance();
	geom_instance->setGeometry(sphere);
	geom_instance->addMaterial(mat);

	geometry_group->addChild(geom_instance);
}


void Time_of_Flight_App::createSphere1(optix::GeometryGroup geometry_group)
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


void Time_of_Flight_App::createSphere2(optix::GeometryGroup geometry_group)
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

	float roughness = 0.0f;
	float metallic = 0.0f;

	mat["Kd_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f, 0.0f, 0.0f, 1.0f)));
	mat["Ks_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f - roughness, 1.0f - roughness, 1.0f - roughness, 1.0f)));
	mat["Kn_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(0.5f, 0.5f, 1.0f, 1.0f)));
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


void Time_of_Flight_App::createSphere3(optix::GeometryGroup geometry_group)
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

	float roughness = 0.0f;
	float metallic = 0.0f;

	mat["Kd_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f, 1.0f, 1.0f, 1.0f)));
	mat["Ks_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(1.0f - roughness, 1.0f - roughness, 1.0f - roughness, 1.0f)));
	mat["Kn_map"]->setTextureSampler(createSampler(g_context, optix::make_float4(0.5f, 0.5f, 1.0f, 1.0f)));
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


void Time_of_Flight_App::setupCamera()
{
	const float max_dim = fmaxf(m_aabb.extent(0), m_aabb.extent(1)); // max of x, y components

	m_camera = new bow::FirstPersonCamera(
		bow::Vector3<double>(m_aabb.center().x, m_aabb.center().y, m_aabb.center().z),	// Position
		bow::Vector3<double>(m_aabb.center().x, m_aabb.center().y, m_aabb.center().z) + bow::Vector3<double>(0.0f, 0.0f, -1.0f),	// LookAt
		bow::Vector3<double>(0.0, 1.0, 0.0),	// WorldUp
		GetWidth(), GetHeight()
		);

	m_camera->SetClippingPlanes(0.01, 10000.0);

	m_lastCursorPosition = m_mouse->VGetAbsolutePositionInsideWindow();
}


void Time_of_Flight_App::setupLights()
{
	bow::Vector3<double> cameraPosition = m_camera->GetPosition();
	bow::Vector3<double> cameraViewDirection = m_camera->GetViewDirection();

	// =====================================
	// Setting up visible Light Sources

	BasicLight lights[] = {
		{ optix::make_float3(cameraPosition.x, cameraPosition.y, cameraPosition.z), optix::make_float3(1.0), optix::make_float3(cameraViewDirection.x, cameraViewDirection.y, cameraViewDirection.z), 0.0f, 1 }
	};

	optix::Buffer light_buffer = g_context->createBuffer(RT_BUFFER_INPUT);
	light_buffer->setFormat(RT_FORMAT_USER);
	light_buffer->setElementSize(sizeof(BasicLight));
	light_buffer->setSize(sizeof(lights) / sizeof(lights[0]));

	memcpy(light_buffer->map(), lights, sizeof(lights));
	light_buffer->unmap();

	g_context["lights"]->set(light_buffer);

	// =====================================
	// Setting up IR Light Sources
	//bow::Matrix3D<float> cameraModelMatrix = bow::Matrix3D<float>(0.99992257f, 0.0093336096f, 0.0079520401f, 0.43033228f, 0.0091540832f, -0.99970341f, 0.022324426f, -0.1501429f, 0.008158138f, -0.022250004f, -0.99971139f, 1.1344646f, 0.0f, 0.0f, 0.0f, 1.0f);

	bow::Matrix3D<float> cameraViewMatrix = m_camera->CalculateView();
	bow::Matrix3D<float> cameraModelMatrix = cameraViewMatrix.Inverse();
	
	bow::Vector4<float> transformed_light_position1 = cameraModelMatrix * bow::Vector4<float>(-0.02f, 0.0f, 0.0f, 1.0f);
	bow::Vector4<float> transformed_light_position2 = cameraModelMatrix * bow::Vector4<float>(-0.025f, 0.0f, 0.0f, 1.0f);
	bow::Vector4<float> transformed_light_position3 = cameraModelMatrix * bow::Vector4<float>(-0.03f, 0.0f, 0.0f, 1.0f);

	BasicLight ir_lights[] = {
		{ optix::make_float3(transformed_light_position1.x, transformed_light_position1.y, transformed_light_position1.z), optix::make_float3(1.0), optix::make_float3(cameraViewDirection.x, cameraViewDirection.y, cameraViewDirection.z), m_irLightIntensity, 1 },
		{ optix::make_float3(transformed_light_position2.x, transformed_light_position2.y, transformed_light_position2.z), optix::make_float3(1.0), optix::make_float3(cameraViewDirection.x, cameraViewDirection.y, cameraViewDirection.z), m_irLightIntensity, 1 },
		{ optix::make_float3(transformed_light_position3.x, transformed_light_position3.y, transformed_light_position3.z), optix::make_float3(1.0), optix::make_float3(cameraViewDirection.x, cameraViewDirection.y, cameraViewDirection.z), m_irLightIntensity, 1 }
	};

	optix::Buffer ir_light_buffer = g_context->createBuffer(RT_BUFFER_INPUT);
	ir_light_buffer->setFormat(RT_FORMAT_USER);
	ir_light_buffer->setElementSize(sizeof(BasicLight));
	ir_light_buffer->setSize(sizeof(ir_lights) / sizeof(ir_lights[0]));

	memcpy(ir_light_buffer->map(), ir_lights, sizeof(ir_lights));
	ir_light_buffer->unmap();

	g_context["ir_lights"]->set(ir_light_buffer);
}


void Time_of_Flight_App::updateCamera()
{
	//bow::Matrix3D<float> rayViewMatrix = bow::Matrix3D<float>(0.99992257f, 0.0093336096f, 0.0079520401f, 0.43033228f, 0.0091540832f, -0.99970341f, 0.022324426f, -0.1501429f, 0.008158138f, -0.022250004f, -0.99971139f, 1.1344646f, 0.0f, 0.0f, 0.0f, 1.0f);

	bow::Matrix3D<double> viewMatrix = m_camera->CalculateView();
	bow::Matrix3D<double> rayViewMatrix = viewMatrix.Inverse();
	
	if (m_camera_changed) // reset accumulation
		m_frame_number = 1;

	g_context["frame_number"]->setUint(m_frame_number++);
	g_context["U"]->setFloat(optix::make_float3(rayViewMatrix._11, rayViewMatrix._21, rayViewMatrix._31));
	g_context["V"]->setFloat(optix::make_float3(rayViewMatrix._12, rayViewMatrix._22, rayViewMatrix._32));
	g_context["W"]->setFloat(optix::make_float3(rayViewMatrix._13, rayViewMatrix._23, rayViewMatrix._33));
	g_context["eye"]->setFloat(optix::make_float3(rayViewMatrix._14, rayViewMatrix._24, rayViewMatrix._34));
}


void Time_of_Flight_App::updateLights()
{
	if (m_camera_changed)
	{
		//bow::Matrix3D<float> cameraModelMatrix = bow::Matrix3D<float>(0.99992257f, 0.0093336096f, 0.0079520401f, 0.43033228f, 0.0091540832f, -0.99970341f, 0.022324426f, -0.1501429f, 0.008158138f, -0.022250004f, -0.99971139f, 1.1344646f, 0.0f, 0.0f, 0.0f, 1.0f);

		bow::Matrix3D<float> cameraViewMatrix = m_camera->CalculateView();
		bow::Matrix3D<float> cameraModelMatrix = cameraViewMatrix.Inverse();
		
		bow::Vector4<float> transformed_light_position1 = cameraModelMatrix * bow::Vector4<float>(-0.025f, 0.0f, 0.0f, 1.0f);
		bow::Vector4<float> transformed_light_position2 = cameraModelMatrix * bow::Vector4<float>(-0.03f, 0.0f, 0.0f, 1.0f);
		bow::Vector4<float> transformed_light_position3 = cameraModelMatrix * bow::Vector4<float>(-0.035f, 0.0f, 0.0f, 1.0f);

		bow::Vector3<double> cameraViewDirection = m_camera->GetViewDirection();
		BasicLight ir_lights[] = {
			{ optix::make_float3(transformed_light_position1.x, transformed_light_position1.y, transformed_light_position1.z), optix::make_float3(1.0), optix::make_float3(cameraViewDirection.x, cameraViewDirection.y, cameraViewDirection.z), m_irLightIntensity, 1 },
			{ optix::make_float3(transformed_light_position2.x, transformed_light_position2.y, transformed_light_position2.z), optix::make_float3(1.0), optix::make_float3(cameraViewDirection.x, cameraViewDirection.y, cameraViewDirection.z), m_irLightIntensity, 1 },
			{ optix::make_float3(transformed_light_position3.x, transformed_light_position3.y, transformed_light_position3.z), optix::make_float3(1.0), optix::make_float3(cameraViewDirection.x, cameraViewDirection.y, cameraViewDirection.z), m_irLightIntensity, 1 }
		};

		optix::Buffer light_buffer = g_context["ir_lights"]->getBuffer();

		memcpy(light_buffer->map(), ir_lights, sizeof(ir_lights));
		light_buffer->unmap();

		g_context["ir_lights"]->set(light_buffer);

		g_context["bg_color"]->setFloat(optix::make_float3(m_ambientSunLightIntensity));
	}
}
