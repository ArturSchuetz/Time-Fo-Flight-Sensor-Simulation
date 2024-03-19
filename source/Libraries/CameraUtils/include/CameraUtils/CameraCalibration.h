#pragma once
#include "CameraUtils/CameraUtils_api.h"

#include "CoreSystems/BowCoreSystems.h"

//opencv
#include <opencv2/opencv.hpp>

namespace bow {

	struct IntrinsicCameraParameters
	{
	public:
		IntrinsicCameraParameters()
		{
			cameraMatrix = cv::Mat(0, 0, CV_8UC3);
			distCoeffs = cv::Mat(0, 0, CV_8UC3);
		}

		cv::Mat cameraMatrix;
		cv::Mat distCoeffs;

		cv::Mat cameraProjectionMatrix;
		cv::Mat cameraProjectionMatrixInverted;

		int image_height;
		int image_width;
	};

	class CAMERAUTILS_API CameraCalibration
	{
	public:
		~CameraCalibration();

		static IntrinsicCameraParameters	intrinsicChessboardCalibration(int boardWidth, int boardHeight, float squareSize, const std::string& CalibrationImagesFolderPath);
		static IntrinsicCameraParameters	intrinsicChessboardCalibration(int boardWidth, int boardHeight, float squareSize, const std::vector<cv::Mat>& imageList);
		static cv::Mat						calculateChessboardCameraTransformationViewMatrix(const cv::Mat& fromCameraChessboardImage, const cv::Mat& ToCameraChessboardImage, const IntrinsicCameraParameters& fromCameraParameters, const IntrinsicCameraParameters& toCameraParameters, int boardWidth, int boardHeight, float squareSize);


		static cv::Mat_<cv::Vec4f> calculate_directionMatrix(const IntrinsicCameraParameters& cameraParameters, unsigned int cols, unsigned int rows, bool lens_distorted = true);
		static cv::Mat_<cv::Vec3f> calculate_coordinates_from_depth(const cv::Mat_<cv::Vec4f>& directionMatrix, const cv::Mat_<ushort>& depthMap);
		static cv::Mat_<cv::Vec3f> calculate_coordinates_from_range(const cv::Mat_<cv::Vec4f>& directionMatrix, const cv::Mat_<ushort>& depthMap);
		static cv::Vec3f calculate_coordinate_from_depth(const cv::Mat_<cv::Vec4f>& directionMatrix, const ushort depthvalue, unsigned int col, unsigned int row);

	private:
		CameraCalibration();

		static cv::Mat extrinsicChessboardCalibration(int boardWidth, int boardHeight, float squareSize, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs, const cv::Mat& chessBoardImage);
		static cv::Mat_<float> getProjectionMatrix(const cv::Size imageSize, const cv::Mat& cameraMatrix);

		static void calcBoardCornerPositions(const cv::Size& boardSize, const float& squareSize, std::vector<cv::Point3f>& out_corners);
		static double norm(double a, double b, double c);
		static double dot(double a1, double a2, double a3, double b1, double b2, double b3);
	};
}
