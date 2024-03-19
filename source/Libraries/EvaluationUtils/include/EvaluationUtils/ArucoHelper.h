#pragma once
#include "EvaluationUtils/EvaluationUtils_api.h"

#include "CoreSystems/BowCoreSystems.h"

//opencv
#include <opencv2/opencv.hpp>

namespace bow {

	struct EVALUATIONUTILS_API Marker
	{
		int Id;
		cv::Vec3d RotationVector;
		cv::Vec3d TranslationVector;
		cv::Mat_<float> ModelMatrix;
		bool Ignored;
	};

	struct EVALUATIONUTILS_API MarkerDescription
	{
		unsigned int id;
		float sidelengthInMM;
		cv::Vec3f corners[4];
		cv::Vec3f center;
		cv::Vec3f up;
		cv::Vec3f right;
		cv::Vec3f front;
		cv::Mat modelMatrix;
		cv::Mat invModelMatrix;
	};

	class EVALUATIONUTILS_API ArucoHelper
	{
	public:
		ArucoHelper();
		~ArucoHelper();

		static std::vector<MarkerDescription> LoadMarkerMapFromFile(const std::string& filePath);
		static std::vector<Marker> detectMarker(const cv::Mat& inputImage, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs, float markerSideLengthInMM, const std::vector<MarkerDescription> markerMap = std::vector<MarkerDescription>());

		static bool getTransformationFromMarkerMap(const std::vector<MarkerDescription>& markerMap, const cv::Mat& inputImage, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs, cv::Matx<float, 4, 4>& global_Transform);
		static bool getTransformationFromMarkerMap(const std::vector<MarkerDescription>& markerMap, std::vector<Marker>& detectedMarker, cv::Matx<float, 4, 4>& global_Transform);
	private:

	};
}
