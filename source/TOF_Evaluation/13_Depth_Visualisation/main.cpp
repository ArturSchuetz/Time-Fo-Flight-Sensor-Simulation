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
#include <limits>

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

	cv::Mat_<double> reference_depthMat;
	unsigned int lastPercentage = 0;
	std::vector<bow::DepthFileData> recordedFiles = bow::DataLoader::loadRecordedFilesFromFolder(recordingsFolderPath);
	for (unsigned int dirIndex = 0; dirIndex < recordedFiles.size(); dirIndex++)
	{
		cv::Mat_<double> mean_depthMat;
		cv::Mat_<cv::Vec3b> mean_colorMat;
		if (recordedFiles[dirIndex].depthFiles.size() > 0)
		{
			for (unsigned int frameIndex = 0; frameIndex < recordedFiles[dirIndex].depthFiles.size(); frameIndex++)
			{
				unsigned int progress = (unsigned int)(((double)frameIndex / (double)recordedFiles[dirIndex].depthFiles.size()) * 100.0);
				if (lastPercentage != progress)
				{
					std::cout << "Calculating... " << recordedFiles[dirIndex].folderName << " (" << std::to_string(progress) << "%)\t\r";
					lastPercentage = progress;
				}

				// ====================================
				// Load rgb image and depth map
				// ====================================

				// depth
				// ====================================

				cv::Mat_<ushort> depthMat = bow::DataLoader::loadDepthFromFile(recordedFiles[dirIndex].depthFiles[frameIndex].filename);
				if (depthMat.cols == 0 || depthMat.rows == 0)
					continue;

				// ==============================================================
				// If this ist Run_0, then it is our background for reference
				// ==============================================================

				if (reference_depthMat.cols != depthMat.cols || reference_depthMat.rows != depthMat.rows)
					reference_depthMat = cv::Mat_<double>(depthMat.rows, depthMat.cols);

				for (unsigned int row = 0; row < depthMat.rows; row++)
				{
					for (unsigned int col = 0; col < depthMat.cols; col++)
					{
						double temp_depth = (double)depthMat.at<ushort>(row, col);
						double a = 1.0f / (double)(frameIndex + 1);
						reference_depthMat.at<double>(row, col) = (reference_depthMat.at<double>(row, col) * (1.0 - a)) + (temp_depth * a);
					}
				}
			}

			// ====================================
			// calculate difference from reference
			// ====================================

			if (reference_depthMat.cols > 0 && reference_depthMat.rows > 0)
			{
				cv::Mat_<ushort> refDepthMat(reference_depthMat.rows, reference_depthMat.cols);
				for (unsigned int row = 0; row < reference_depthMat.rows; row++)
				{
					for (unsigned int col = 0; col < reference_depthMat.cols; col++)
					{
						refDepthMat.at<ushort>(row, col) = (ushort)reference_depthMat.at<double>(row, col);
					}
				}

				// ==============================================================
				// Prepare maps for rendering
				// ==============================================================

				std::vector<bow::Vector3<float>> colors((refDepthMat.cols * refDepthMat.rows));
				std::vector<bow::Vector3<float>> points((refDepthMat.cols * refDepthMat.rows));

				cv::Mat_<cv::Vec3f> coordinatesRef = bow::CameraCalibration::calculate_coordinates_from_depth(ir_directionMatrix, refDepthMat);
				for (unsigned int row = 0; row < coordinatesRef.rows; row++)
				{
					for (unsigned int col = 0; col < coordinatesRef.cols; col++)
					{
						cv::Vec3f coordinate = coordinatesRef.at<cv::Vec3f>(row, col);
						cv::Mat transformedCoord = cv::Mat(cv::Vec4f(coordinate.val[0], coordinate.val[1], coordinate.val[2], 1.0f));

						unsigned int index = col + (row * coordinatesRef.cols);
						colors[index] = bow::Vector3<float>(0.2f, 0.2f, 0.2f);
						points[index] = bow::Vector3<float>(transformedCoord.at<float>(0, 0), transformedCoord.at<float>(1, 0), transformedCoord.at<float>(2, 0));
					}
				}

				// ==============================================================
				// Render pointclouds
				// ==============================================================

				g_renderer.UpdateColors(colors);
				g_renderer.UpdatePointCloud(points);

				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
			}
		}
	}
}

int main()
{
	g_renderer.Start();
	runAnalysis("/data/Kinect_v2_Calibration.xml", "F:\\Kamera_Evaluation\\Kinect_v2\\Recordings_Evaluation");

	g_renderer.Stop();
	return 0;
}
