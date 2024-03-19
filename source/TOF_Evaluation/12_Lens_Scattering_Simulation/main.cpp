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

void runAnalysis(const std::string& calibrationFilePath, const std::string& recordingsFolderPath, const std::string& simulationsFolderPath, const std::string& small_markerMapPath)
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
	// Load Marker Maps
	// ==============================================================

	std::vector<bow::MarkerDescription> markerMap = bow::ArucoHelper::LoadMarkerMapFromFile(std::string(PROJECT_BASE_DIR) + small_markerMapPath);

	float sideLength = 0.0f;
	unsigned int count_sidelength = 0;

	for (unsigned int markerIndex = 0; markerIndex < markerMap.size(); markerIndex++)
	{
		sideLength += markerMap[markerIndex].sidelengthInMM;
		count_sidelength++;
	}

	if (count_sidelength > 0)
		sideLength = sideLength / (float)count_sidelength;

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

					// color
					// ====================================

					cv::Mat colorMat = cv::imread(recordedFiles[dirIndex].imageFiles[frameIndex].filename, CV_LOAD_IMAGE_UNCHANGED);
					if (colorMat.cols == 0 || colorMat.rows == 0)
						continue;

					if (mean_colorMat.cols != colorMat.cols || mean_colorMat.rows != colorMat.rows)
						mean_colorMat = cv::Mat_<cv::Vec3b>::zeros(colorMat.rows, colorMat.cols);

					for (unsigned int row = 0; row < colorMat.rows; row++)
					{
						for (unsigned int col = 0; col < colorMat.cols; col++)
						{
							cv::Vec3b temp_color = colorMat.at<cv::Vec3b>(row, col);
							double a = 1.0f / (double)(frameIndex + 1);
							mean_colorMat.at<cv::Vec3b>(row, col) = (mean_colorMat.at<cv::Vec3b>(row, col) * (1.0 - a)) + (temp_color * a);
						}
					}
				}
			}

			// ====================================
			// calculate difference from reference
			// ====================================

			if (mean_depthMat.cols > 0 && mean_depthMat.rows > 0 && reference_depthMat.cols > 0 && reference_depthMat.rows > 0)
			{
				cv::Matx<float, 4, 4> global_Transform;
				if (mean_colorMat.cols > 0 && mean_colorMat.rows > 0)
				{
					cv::Mat undistortedColorMat;
					cv::Mat_<cv::Vec4f> distorted_directionMatrix = bow::CameraCalibration::calculate_directionMatrix(ir_intrinisicCameraParameters, ir_intrinisicCameraParameters.image_width, ir_intrinisicCameraParameters.image_height);

					cv::undistort(mean_colorMat, undistortedColorMat, rgb_intrinisicCameraParameters.cameraMatrix, rgb_intrinisicCameraParameters.distCoeffs);

					cv::Mat empty_DistCoeffs = cv::Mat::zeros(4, 1, CV_32F);
					std::vector<bow::Marker> detectedMarker = bow::ArucoHelper::detectMarker(undistortedColorMat, rgb_intrinisicCameraParameters.cameraMatrix, empty_DistCoeffs, markerMap[0].sidelengthInMM);
					if (bow::ArucoHelper::getTransformationFromMarkerMap(markerMap, detectedMarker, global_Transform))
					{
						std::cout << global_Transform << std::endl;
						// ====================================
						// transform points into world space
						// ====================================

						cv::Mat_<float> differenceMat(mean_depthMat.rows, mean_depthMat.cols);
						cv::Mat_<ushort> refDepthMat(reference_depthMat.rows, reference_depthMat.cols);
						cv::Mat_<ushort> depthMat(mean_depthMat.rows, mean_depthMat.cols);
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

						std::vector<bow::Vector3<float>> colors((depthMat.cols * depthMat.rows));
						std::vector<bow::Vector3<float>> points((depthMat.cols * depthMat.rows));

						cv::Mat_<cv::Vec3f> coordinatesRef = bow::CameraCalibration::calculate_coordinates_from_depth(ir_directionMatrix, refDepthMat);
						for (unsigned int row = 0; row < coordinatesRef.rows; row++)
						{
							for (unsigned int col = 0; col < coordinatesRef.cols; col++)
							{
								cv::Vec3f coordinate = coordinatesRef.at<cv::Vec3f>(row, col);
								cv::Mat transformedCoord = cv::Mat(global_Transform) * irToRgbCameraViewMatrix * cv::Mat(cv::Vec4f(coordinate.val[0], coordinate.val[1], coordinate.val[2], 1.0f));

								unsigned int index = col + (row * coordinatesRef.cols);
								colors[index] = bow::Vector3<float>(0.0f, 0.0f, 0.0f);
								points[index] = bow::Vector3<float>(transformedCoord.at<float>(0, 0), transformedCoord.at<float>(1, 0), transformedCoord.at<float>(2, 0));
							}
						}

						cv::Mat_<cv::Vec3f> coordinates = bow::CameraCalibration::calculate_coordinates_from_depth(ir_directionMatrix, depthMat);
						for (unsigned int row = 0; row < coordinates.rows; row++)
						{
							for (unsigned int col = 0; col < coordinates.cols; col++)
							{
								float diff = differenceMat.at<float>(row, col);
								cv::Vec3f coordinate = coordinates.at<cv::Vec3f>(row, col);
								cv::Mat transformedCoord = cv::Mat(global_Transform) * irToRgbCameraViewMatrix * cv::Mat(cv::Vec4f(coordinate.val[0], coordinate.val[1], coordinate.val[2], 1.0f));

								const float maxDiff = 10.0f;
								if (diff < 0)
									diff = -diff;

								unsigned int index = (col + (row * coordinates.cols));
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
							cv::imwrite(recordingsFolderPath + "\\" + recordedFiles[dirIndex].folderName + "_multi_path_error_diffmap.png", diff_mat);

							applyColorMap(diff_mat, cm_img0, cv::COLORMAP_HOT);
							cv::imwrite(recordingsFolderPath + "\\" + recordedFiles[dirIndex].folderName + "_multi_path_error_heatmap.png", cm_img0);

							applyColorMap(diff_mat, cm_img0, cv::COLORMAP_JET);
							cv::imwrite(recordingsFolderPath + "\\" + recordedFiles[dirIndex].folderName + "_multi_path_error_jetmap.png", cm_img0);
						}

						// ==============================================================
						// Render pointclouds
						// ==============================================================

						g_renderer.UpdateColors(colors);
						g_renderer.UpdatePointCloud(points);

						//std::this_thread::sleep_for(std::chrono::milliseconds(10000));

						// ==============================================================
						// Calculate output data for thesis
						// ==============================================================

						// calculate depth values for scanline view
						{
							std::ofstream csv_file;
							csv_file.open(recordingsFolderPath + "\\" + recordedFiles[dirIndex].folderName + ".csv", std::ofstream::out | std::ofstream::trunc);

							std::ofstream depth_datFile;
							depth_datFile.open(recordingsFolderPath + "\\" + recordedFiles[dirIndex].folderName + "_depth.dat", std::ofstream::out | std::ofstream::trunc);

							std::ofstream ref_datFile;
							ref_datFile.open(recordingsFolderPath + "\\" + recordedFiles[dirIndex].folderName + "_ref.dat", std::ofstream::out | std::ofstream::trunc);

							int current_row = 235;
							for (unsigned int col = 0; col < coordinates.cols; col++)
							{
								cv::Vec3f depthVal = coordinates.at<cv::Vec3f>(current_row, col);
								cv::Vec3f depthRefVal = coordinatesRef.at<cv::Vec3f>(current_row, col);
								if (csv_file.is_open())
								{
									if (depthRefVal.val[2] > 0 && depthVal.val[2] > 0)
									{
										std::string value = std::to_string((double)depthRefVal.val[0]);
										std::replace(value.begin(), value.end(), '.', ',');

										csv_file << value
											<< ";" << depthVal.val[2]
											<< ";" << depthRefVal.val[2]
											<< std::endl;
										depth_datFile << std::to_string(depthVal.val[0]) << " " << std::to_string(depthVal.val[2]) << std::endl;
										ref_datFile << std::to_string(depthRefVal.val[0]) << " " << std::to_string(depthRefVal.val[2]) << std::endl;
									}
								}
							}

							csv_file.close();
							depth_datFile.close();
							ref_datFile.close();
						}
					}
				}
			}
		}
	}
}

int main()
{
	g_renderer.Start();
	runAnalysis("/data/Kinect_v2_Calibration.xml", "F:\\Kamera_Evaluation\\Kinect_v2\\Recordings_Evaluation", "F:\\Kamera_Evaluation\\\Simulation\\Recordings_Lens_Scattering", "/data/map_evaluation.yml");

	g_renderer.Stop();
	return 0;
}
