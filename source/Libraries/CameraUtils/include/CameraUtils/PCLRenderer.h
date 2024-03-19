#pragma once
#include "CameraUtils/CameraUtils_api.h"

#include "CoreSystems/BowCoreSystems.h"
#include "InputDevice/BowInput.h"
#include "RenderDevice/BowRenderer.h"

#include <iostream> 
#include <string>
#include <chrono>
#include <thread>

#if defined(_WIN32) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // i'm sorry
#endif

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#ifdef CreateWindow
#undef CreateWindow
#endif

namespace bow {
	struct renderThread_data;

	class CAMERAUTILS_API PCLRenderer
	{
	public:
		PCLRenderer();
		~PCLRenderer();

		std::vector<unsigned short> RenderRefDepthFromPerspective(unsigned int width, unsigned int height, bow::Matrix3D<float> viewMatrix, bow::Matrix4x4<float> projectionMatrix);

		bool Start();
		bool UpdateColors(std::vector<bow::Vector3<float>> colors);
		bool UpdatePointCloud(std::vector<bow::Vector3<float>> vertices);
		bool UpdatePointCloud(std::vector<bow::Vector3<float>> vertices, std::vector<bow::Vector3<float>> normals);

		bool UpdateReferencePointCloud(std::vector<bow::Vector3<float>> vertices);
		bool UpdateReferencePointCloud(std::vector<bow::Vector3<float>> vertices, std::vector<bow::Vector3<float>> normals);
		bool UpdateReferencePointCloud(std::vector<bow::Vector3<float>> vertices, std::vector<bow::Vector3<float>> colors, std::vector<bow::Vector3<float>> normals);
		bool Stop();
		bool ShouldClose();

	private:
		std::thread 		m_renderThread;
		renderThread_data* 	m_renderThreadData;
	};
}
