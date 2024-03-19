#include <RenderDevice/BowRenderer.h>
#include <CoreSystems/BowCoreSystems.h>
#include <CoreSystems/BowBasicTimer.h>

#include <CameraUtils/CameraCalibration.h>
#include <CameraUtils/RenderingConfigs.h>
#include <CameraUtils/PCLRenderer.h>
#include <EvaluationUtils/ArucoHelper.h>
#include <EvaluationUtils/DataLoader.h>

#include <Masterthesis/cuda_config.h>

#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>

bow::PCLRenderer g_renderer;

void runAnalysis(const std::string& calibrationFilePath, const std::string& recordingsFolderPath)
{
	////////////////////////////////////////////////////////////////
	// Preperations
	////////////////////////////////////////////////////////////////

	bow::RenderingConfigs configs = bow::ConfigLoader::loadConfigFromFile(std::string(PROJECT_BASE_DIR) + calibrationFilePath);

	bow::IntrinsicCameraParameters rgb_intrinisicCameraParameters = bow::CameraCalibration::intrinsicChessboardCalibration(configs.calibration_checkerboard_width, configs.calibration_checkerboard_height, configs.calibration_checkerboard_squareSize, std::string(PROJECT_BASE_DIR) + std::string("/data/") + configs.rgbCameraCheckerboardImagesPath);
	bow::IntrinsicCameraParameters ir_intrinisicCameraParameters = bow::CameraCalibration::intrinsicChessboardCalibration(configs.calibration_checkerboard_width, configs.calibration_checkerboard_height, configs.calibration_checkerboard_squareSize, std::string(PROJECT_BASE_DIR) + std::string("/data/") + configs.irCameraCheckerboardImagesPath);

	cv::Mat rgb_ChessboardImage = cv::imread(std::string(PROJECT_BASE_DIR) + std::string("/data/") + configs.rgbCameraExtrinsicCheckerboardImageFilePath, cv::IMREAD_UNCHANGED);
	cv::Mat ir_ChessboardImage = cv::imread(std::string(PROJECT_BASE_DIR) + std::string("/data/") + configs.irCameraExtrinsicCheckerboardImageFilePath, cv::IMREAD_UNCHANGED);

	cv::Mat irToRgbCameraViewMatrix = bow::CameraCalibration::calculateChessboardCameraTransformationViewMatrix(ir_ChessboardImage, rgb_ChessboardImage, ir_intrinisicCameraParameters, rgb_intrinisicCameraParameters, configs.calibration_checkerboard_width, configs.calibration_checkerboard_height, configs.calibration_checkerboard_squareSize);
	cv::Mat rgbToIrCameraViewMatrix = bow::CameraCalibration::calculateChessboardCameraTransformationViewMatrix(rgb_ChessboardImage, ir_ChessboardImage, rgb_intrinisicCameraParameters, ir_intrinisicCameraParameters, configs.calibration_checkerboard_width, configs.calibration_checkerboard_height, configs.calibration_checkerboard_squareSize);

	cv::Mat_<cv::Vec4f> ir_directionMatrix = bow::CameraCalibration::calculate_directionMatrix(ir_intrinisicCameraParameters, ir_intrinisicCameraParameters.image_width, ir_intrinisicCameraParameters.image_height);
	// ==============================================================
	// Find marker in images and estimate camera position
	// ==============================================================

	unsigned int lastPercentage = 0;

	std::map<ushort, std::map<ushort, unsigned int>> arucoDepth_to_tofDepth_map;
	std::vector<bow::DepthFileData> recordedFiles = bow::DataLoader::loadRecordedFilesFromFolder(recordingsFolderPath);
	for (unsigned int dirIndex = 0; dirIndex < recordedFiles.size(); dirIndex++)
	{
		cv::Mat_<double> mean_depthMat;
		if (recordedFiles[dirIndex].imageFiles.size() > 0 && recordedFiles[dirIndex].depthFiles.size() > 0)
		{
			for (unsigned int frameIndex = 0; frameIndex < recordedFiles[dirIndex].imageFiles.size(); frameIndex++)
			{
				unsigned int progress = (unsigned int)(((double)frameIndex / (double)recordedFiles[dirIndex].imageFiles.size()) * 100.0);
				if (lastPercentage != progress)
				{
					std::cout << "Calculating... " << recordedFiles[dirIndex].folderName << " (" << std::to_string(progress) << "%)\t\r";
					lastPercentage = progress;
				}

				// ====================================
				// Load rgb image and depth map
				// ====================================

				cv::Mat undistortedColorMat;
				cv::Mat empty_DistCoeffs = cv::Mat::zeros(4, 1, CV_32F);

				cv::Mat colorMat = cv::imread(recordedFiles[dirIndex].imageFiles[frameIndex].filename, CV_LOAD_IMAGE_UNCHANGED);
				cv::undistort(colorMat, undistortedColorMat, rgb_intrinisicCameraParameters.cameraMatrix, rgb_intrinisicCameraParameters.distCoeffs);

				cv::Mat_<ushort> depthMat = bow::DataLoader::findClosestDepthFile(recordedFiles[dirIndex].imageFiles[frameIndex].timestamp, recordedFiles[dirIndex].depthFiles);
				if (depthMat.cols == 0 || depthMat.rows == 0)
					continue;

				if (mean_depthMat.cols != depthMat.cols || mean_depthMat.rows != depthMat.rows)
					mean_depthMat = cv::Mat_<double>(depthMat.rows, depthMat.cols);

				for (unsigned int row = 0; row < depthMat.rows; row++)
				{
					for (unsigned int col = 0; col < depthMat.cols; col++)
					{
						double temp_depth = (double)depthMat.at<ushort>(row, col);
						double a = 1.0f / (double)(frameIndex + 1);
						mean_depthMat.at<double>(row, col) = (mean_depthMat.at<double>(row, col) * (1.0 - a)) + (temp_depth * a);
					}
				}
			}
		}

		cv::Mat_<ushort> depthMat(mean_depthMat.rows, mean_depthMat.cols);
		for (unsigned int row = 0; row < mean_depthMat.rows; row++)
		{
			for (unsigned int col = 0; col < mean_depthMat.cols; col++)
			{
				depthMat.at<ushort>(row, col) = (ushort)mean_depthMat.at<double>(row, col);
			}
		}

		cv::Mat viewMat = cv::Mat::zeros(4, 4, CV_32FC1);
		viewMat.at<float>(0, 0) = 1.0f;
		viewMat.at<float>(1, 1) = 1.0f;
		viewMat.at<float>(2, 2) = -1.0f;
		viewMat.at<float>(3, 3) = 1.0f;

		cv::Mat_<cv::Vec3f> coordinates = bow::CameraCalibration::calculate_coordinates_from_depth(ir_directionMatrix, depthMat);
		std::vector<bow::Vector3<float>> colors(coordinates.cols * coordinates.rows);
		std::vector<bow::Vector3<float>> points(coordinates.cols * coordinates.rows);

		for (unsigned int row = 0; row < coordinates.rows; row++)
		{
			for (unsigned int col = 0; col < coordinates.cols; col++)
			{
				unsigned int index = col + (row * coordinates.cols);

				cv::Vec3f coordinate = coordinates.at<cv::Vec3f>(row, col);
				cv::Mat transformedCoord = viewMat * cv::Mat(cv::Vec4f(coordinate.val[0], coordinate.val[1], coordinate.val[2], 1.0f));

				colors[index] = bow::Vector3<float>(0.5f, 0.5f, 0.5f);
				points[index] = bow::Vector3<float>(transformedCoord.at<float>(0, 0), -transformedCoord.at<float>(1, 0), transformedCoord.at<float>(2, 0)) / 1000.0f;
			}
		}

		g_renderer.UpdateColors(colors);
		g_renderer.UpdatePointCloud(points);
	}
}

int main()
{
	g_renderer.Start();
	runAnalysis("/data/IFM_O3D303_Calibration.xml", "F:\\Kamera_Evaluation\\03D303\\Recordings_Mixed_Pixels");
	runAnalysis("/data/Kinect_v2_Calibration.xml", "F:\\Kamera_Evaluation\\Kinect_v2\\Recordings_Mixed_Pixels");
	runAnalysis("/data/Xtion_2_Calibration.xml", "F:\\Kamera_Evaluation\\Xtion_2\\Recordings_Mixed_Pixels");
	g_renderer.Stop();
	return 0;
}