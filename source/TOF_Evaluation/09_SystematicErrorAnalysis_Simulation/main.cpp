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
	cv::Mat_<double> reference_irMat;
	unsigned int lastPercentage = 0;
	std::vector<bow::DepthFileData> recordedFiles = bow::DataLoader::loadRecordedFilesFromFolder(recordingsFolderPath);
	for (unsigned int dirIndex = 0; dirIndex < recordedFiles.size(); dirIndex++)
	{
		cv::Mat_<double> mean_depthMat;
		if (recordedFiles[dirIndex].rangeFiles.size() > 0)
		{
			for (unsigned int frameIndex = 0; frameIndex < recordedFiles[dirIndex].rangeFiles.size(); frameIndex++)
			{
				unsigned int progress = (unsigned int)(((double)frameIndex / (double)recordedFiles[dirIndex].rangeFiles.size()) * 100.0);
				if (lastPercentage != progress)
				{
					std::cout << "Calculating... " << recordedFiles[dirIndex].folderName << " (" << std::to_string(progress) << "%)\t\r";
					lastPercentage = progress;
				}

				// ====================================
				// Load rgb image and depth map
				// ====================================

				cv::Mat_<ushort> depthMat = bow::DataLoader::loadDepthFromFile(recordedFiles[dirIndex].rangeFiles[frameIndex].filename);
				if (depthMat.cols == 0 || depthMat.rows == 0)
					continue;

				if (dirIndex == 0)
				{
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
				else
				{
					// ==============================================================
					// Calculate mean of depth values to reduce noise
					// ==============================================================

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

			if (mean_depthMat.cols > 0 && mean_depthMat.rows > 0 && reference_depthMat.cols > 0 && reference_depthMat.rows > 0)
			{
				cv::Mat_<float> differenceMat(mean_depthMat.rows, mean_depthMat.cols);
				cv::Mat_<ushort> depthMat(mean_depthMat.rows, mean_depthMat.cols);
				cv::Mat_<ushort> refDepthMat(reference_depthMat.rows, reference_depthMat.cols);
				for (unsigned int row = 0; row < mean_depthMat.rows; row++)
				{
					for (unsigned int col = 0; col < mean_depthMat.cols; col++)
					{
						differenceMat.at<float>(row, col) = (mean_depthMat.at<double>(row, col) - reference_depthMat.at<double>(row, col));

						refDepthMat.at<ushort>(row, col) = (ushort)reference_depthMat.at<double>(row, col);
						depthMat.at<ushort>(row, col) = (ushort)mean_depthMat.at<double>(row, col);
					}
				}

				// ==============================================================
				// Prepare maps for rendering
				// ==============================================================

				cv::Mat viewMat = cv::Mat::zeros(4, 4, CV_32FC1);
				viewMat.at<float>(0, 0) = 1.0f;
				viewMat.at<float>(1, 1) = 1.0f;
				viewMat.at<float>(2, 2) = -1.0f;
				viewMat.at<float>(3, 3) = 1.0f;

				std::vector<bow::Vector3<float>> colors((depthMat.cols * depthMat.rows) * 2);
				std::vector<bow::Vector3<float>> points((depthMat.cols * depthMat.rows) * 2);

				cv::Mat_<cv::Vec3f> coordinatesRef = bow::CameraCalibration::calculate_coordinates_from_range(ir_directionMatrix, refDepthMat);
				for (unsigned int row = 0; row < coordinatesRef.rows; row++)
				{
					for (unsigned int col = 0; col < coordinatesRef.cols; col++)
					{
						cv::Vec3f coordinate = coordinatesRef.at<cv::Vec3f>(row, col);
						cv::Mat transformedCoord = viewMat * cv::Mat(cv::Vec4f(coordinate.val[0], coordinate.val[1], coordinate.val[2], 1.0f));
						//coordinatesRef.at<cv::Vec3f>(row, col) = cv::Vec3f(transformedCoord.at<float>(0, 0), transformedCoord.at<float>(1, 0), transformedCoord.at<float>(2, 0));

						unsigned int index = col + (row * coordinatesRef.cols);
						colors[index] = bow::Vector3<float>(0.0f, 0.0f, 0.0f);
						points[index] = bow::Vector3<float>(transformedCoord.at<float>(0, 0), transformedCoord.at<float>(1, 0), transformedCoord.at<float>(2, 0));
					}
				}

				cv::Mat_<cv::Vec3f> coordinates = bow::CameraCalibration::calculate_coordinates_from_range(ir_directionMatrix, depthMat);
				for (unsigned int row = 0; row < coordinates.rows; row++)
				{
					for (unsigned int col = 0; col < coordinates.cols; col++)
					{
						float diff = differenceMat.at<float>(row, col);
						cv::Vec3f coordinate = coordinates.at<cv::Vec3f>(row, col);
						cv::Mat transformedCoord = viewMat * cv::Mat(cv::Vec4f(coordinate.val[0], coordinate.val[1], coordinate.val[2], 1.0f));
						//coordinates.at<cv::Vec3f>(row, col) = cv::Vec3f(transformedCoord.at<float>(0, 0), transformedCoord.at<float>(1, 0), transformedCoord.at<float>(2, 0));

						const float maxDiff = 10.0f;
						if (diff < 0)
							diff = -diff;

						unsigned int index = (col + (row * coordinates.cols)) + (coordinatesRef.cols * coordinatesRef.rows);
						colors[index] = bow::Vector3<float>(diff / maxDiff, diff / maxDiff, diff / maxDiff);
						points[index] = bow::Vector3<float>(transformedCoord.at<float>(0, 0), transformedCoord.at<float>(1, 0), transformedCoord.at<float>(2, 0));
					}
				}

				if (differenceMat.rows > 0 && differenceMat.cols > 0)
				{
					// ========================================================================
					// prepare difference map
					// ========================================================================

					cv::Mat_<uchar> diff_mat = cv::Mat_<uchar>(differenceMat.rows, differenceMat.cols);
					for (unsigned int row = 0; row < differenceMat.rows; row++)
					{
						for (unsigned int col = 0; col < differenceMat.cols; col++)
						{
							float value = differenceMat.at<float>(row, col) * 255.0f / 10.0f;
							if (value < 0)
								value = -value;

							if (value > 255.0f)
								value = 255.0f;

							if (value < 0.0f)
								value = 0.0f;
							diff_mat.at<uchar>(row, col) = value;
						}
					}

					// ========================================================================
					// create color maps and save results on hard disk
					// ========================================================================
					cv::Mat cm_img0;
					cv::imwrite(recordingsFolderPath + "\\" + recordedFiles[dirIndex].folderName + "_systematic_error_diffmap.png", diff_mat);

					applyColorMap(diff_mat, cm_img0, cv::COLORMAP_HOT);
					cv::imwrite(recordingsFolderPath + "\\" + recordedFiles[dirIndex].folderName + "_systematic_error_heatmap.png", cm_img0);

					applyColorMap(diff_mat, cm_img0, cv::COLORMAP_JET);
					cv::imwrite(recordingsFolderPath + "\\" + recordedFiles[dirIndex].folderName + "_systematic_error_jetmap.png", cm_img0);
				}

				// ==============================================================
				// Render pointclouds
				// ==============================================================

				g_renderer.UpdateColors(colors);
				g_renderer.UpdatePointCloud(points);

				// ==============================================================
				// Calculate output data for thesis
				// ==============================================================

				// calculate depth values for scanline view
				{
					std::ofstream csv_file;
					csv_file.open(recordingsFolderPath + "\\" + recordedFiles[dirIndex].folderName + ".csv", std::ofstream::out | std::ofstream::trunc);

					std::ofstream depth_datFile;
					depth_datFile.open(recordingsFolderPath + "\\" + recordedFiles[dirIndex].folderName + "_depth.dat", std::ofstream::out | std::ofstream::trunc);

					for (unsigned int col = 0; col < coordinates.cols; col++)
					{
						cv::Vec3f depthVal = coordinates.at<cv::Vec3f>(coordinates.rows / 2, col);
						cv::Vec3f depthRefVal = coordinatesRef.at<cv::Vec3f>(coordinatesRef.rows / 2, col);
						if (csv_file.is_open())
						{
							if (depthRefVal.val[2] > 0 && depthVal.val[2] > 0)
							{
								std::string value = std::to_string((double)depthRefVal.val[2]);
								std::replace(value.begin(), value.end(), '.', ',');

								std::string value_2 = std::to_string((double)depthVal.val[2] - (double)depthRefVal.val[2]);
								std::replace(value_2.begin(), value_2.end(), '.', ',');

								csv_file << value
									<< ";" << value_2
									<< std::endl;
								depth_datFile << std::to_string(depthRefVal.val[2]) << " " << std::to_string((double)depthVal.val[2] - (double)depthRefVal.val[2]) << std::endl;
							}
						}
					}

					csv_file.close();
					depth_datFile.close();
				}
			}
		}
	}
}

int main()
{
	g_renderer.Start();
	runAnalysis("/data/Kinect_v2_Calibration.xml", "F:\\Kamera_Evaluation\\\Simulation\\Recordings_SystematicError");
	runAnalysis("/data/Kinect_v2_Calibration.xml", "F:\\Kamera_Evaluation\\\Simulation\\Recordings_SystematicError_2");
	runAnalysis("/data/Kinect_v2_Calibration.xml", "F:\\Kamera_Evaluation\\\Simulation\\Recordings_SystematicError_3");
	runAnalysis("/data/Kinect_v2_Calibration.xml", "F:\\Kamera_Evaluation\\\Simulation\\Recordings_SystematicError_4");

	g_renderer.Stop();
	return 0;
}
