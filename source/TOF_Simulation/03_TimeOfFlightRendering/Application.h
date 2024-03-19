#include <OptixUtils/sutil.h>

#include <CameraUtils/BowApplication.h>
#include <CameraUtils/FirstPersonCamera.h>

#include <CameraUtils/CameraCalibration.h>
#include <CameraUtils/RenderingConfigs.h>

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

struct UsageReportLogger;
struct image_save_thread_data {
	bool				stopThread;
	bool				running;
	bool				savingRunning;
	bool				busy;
	std::queue<std::pair<long long, cv::Mat>> images;
	std::queue<std::pair<long long, cv::Mat>> depth;
	std::queue<std::pair<long long, cv::Mat>> range;
	std::queue<std::pair<long long, cv::Mat>> ir;
};

class Time_of_Flight_App : public bow::Application
{
public:
	Time_of_Flight_App();
	~Time_of_Flight_App();

private:
	std::string GetWindowTitle(void) { return "Path Tracing"; }

	// overwrite functions of framework
	void OnInit(bow::IntrinsicCameraParameters cameraParameters);
	void OnResized(unsigned int newWidth, unsigned int newHeight);
	void OnUpdate(double deltaTime);
	void OnRender();
	void OnRelease();

	// helperfunctions
	void createContext(int usage_report_level, UsageReportLogger* logger);

	void loadMesh(const std::string& filename, optix::GeometryGroup geometry_group, float unitsPerMeter = 1.0f);
	void createSphere0(optix::GeometryGroup geometry_group);
	void createSphere1(optix::GeometryGroup geometry_group);
	void createSphere2(optix::GeometryGroup geometry_group);
	void createSphere3(optix::GeometryGroup geometry_group);

	void setupCamera();
	void setupLights();
	void updateCamera();
	void updateLights();

	unsigned int			m_width;
	unsigned int			m_height;

	UsageReportLogger*		m_logger;
	int						m_usage_report_level;
	bow::FirstPersonCamera*	m_camera;
	bow::Vector3<long>		m_lastCursorPosition;

	// Camera state
	optix::Aabb			m_aabb;
	int					m_frame_number;
	bool				m_camera_changed;
	unsigned int		m_use_model;

	float				m_irLightIntensity;
	float				m_ambientSunLightIntensity;

	cv::Mat_<cv::Vec4f>	m_direction_vectors;
	std::vector<double> m_filter_kernel;

	bool	m_noise_enabled;
	bool	m_lens_scattering_enabled;
	bool	m_save_data;
	bool	recording_pressed;
	bool	enable_lens_scattering_pressed;
	bool    enable_noise_pressed;

	std::thread m_imageSavingThread;
	image_save_thread_data m_threadData;
};