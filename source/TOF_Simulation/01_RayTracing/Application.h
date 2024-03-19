#include <OptixUtils/sutil.h>

#include <CameraUtils/BowApplication.h>
#include <CameraUtils/FirstPersonCamera.h>

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

struct UsageReportLogger;

class SimpleDirectionalLightApp : public bow::Application
{
public:
	SimpleDirectionalLightApp();
	~SimpleDirectionalLightApp();

private:
	std::string GetWindowTitle(void) { return "Fresnel Reflections And Specularity"; }

	// overwrite functions of framework
	void OnInit(bow::IntrinsicCameraParameters cameraParameters);
	void OnResized(unsigned int newWidth, unsigned int newHeight);
	void OnUpdate(double deltaTime);
	void OnRender();
	void OnRelease();

	// helperfunctions
	void createContext(int usage_report_level, UsageReportLogger* logger);
	void loadMesh(const std::string& filename, optix::GeometryGroup geometry_group, float unitsPerMeter = 1.0f);
	void createSphere1(optix::GeometryGroup geometry_group);
	void createSphere2(optix::GeometryGroup geometry_group);
	void createSphere3(optix::GeometryGroup geometry_group);
	void setupCamera();
	void setupLights();
	void updateCamera();

	UsageReportLogger*		m_logger;
	int						m_usage_report_level;
	bow::FirstPersonCamera*	m_camera; 
	bow::Vector3<long>		m_lastCursorPosition;

	// Camera state
	optix::Aabb			m_aabb;
};