#include <RenderDevice/BowRenderer.h>
#include <CoreSystems/BowCoreSystems.h>
#include <CoreSystems/BowBasicTimer.h>

#include <CameraUtils/CameraCalibration.h>
#include <CameraUtils/RenderingConfigs.h>
#include <EvaluationUtils/ArucoHelper.h>
#include <EvaluationUtils/DataLoader.h>

#include <Masterthesis/cuda_config.h>

#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <limits>

void runAnalysis(const std::string& calibrationFilePath, const std::string& recordingsFolderPath, const std::string& big_markerMapPath)
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
	
	cv::Mat_<cv::Vec4f> distorted_directionMat = bow::CameraCalibration::calculate_directionMatrix(ir_intrinisicCameraParameters, ir_intrinisicCameraParameters.image_height, ir_intrinisicCameraParameters.image_height);
	cv::Mat_<cv::Vec4f> undistorted_directionMat = bow::CameraCalibration::calculate_directionMatrix(ir_intrinisicCameraParameters, ir_intrinisicCameraParameters.image_height, ir_intrinisicCameraParameters.image_height, false);

	// ==============================================================
	// Load Marker Maps
	// ==============================================================

	std::vector<bow::MarkerDescription> big_MarkerMap = bow::ArucoHelper::LoadMarkerMapFromFile(std::string(PROJECT_BASE_DIR) + big_markerMapPath);

	cv::Vec3f center = cv::Vec3f(0.0f, 0.0f, 0.0f);
	unsigned int count_positions = 0;

	float sideLength = 0.0f;
	unsigned int count_sidelength = 0;

	for (unsigned int markerIndex = 0; markerIndex < big_MarkerMap.size(); markerIndex++)
	{
		sideLength += big_MarkerMap[markerIndex].sidelengthInMM;
		count_sidelength++;

		center += big_MarkerMap[markerIndex].center;
		count_positions++;
	}

	if (count_positions > 0)
		center = center / (float)count_positions;

	if (count_sidelength > 0)
		sideLength = sideLength / (float)count_sidelength;

	// ==============================================================
	// Find marker in images and estimate camera position
	// ==============================================================

	std::ofstream csv_file;
	csv_file.open(recordingsFolderPath + "\\systematic_error_center_raw.csv", std::ofstream::out | std::ofstream::trunc);

	std::ofstream datFile;
	datFile.open(recordingsFolderPath + "\\systematic_error_center_raw.dat", std::ofstream::out | std::ofstream::trunc);

	unsigned int lastPercentage = 0;
	std::map<unsigned short, std::map<unsigned short, unsigned int>> arucoDepth_to_tofDepth_map;
	std::vector<bow::DepthFileData> recordedFiles = bow::DataLoader::loadRecordedFilesFromFolder(recordingsFolderPath);
	for (unsigned int dirIndex = 0; dirIndex < recordedFiles.size(); dirIndex++)
	{
		std::cout << "Analysing files... (" << std::to_string((unsigned int)(((double)dirIndex / (double)recordedFiles.size()) * 100.0)) << "%)\t\r";

		long long lastTimeStamp = 0;
		ushort lastDistance = 0;
		if (recordedFiles[dirIndex].imageFiles.size() > 0 && recordedFiles[dirIndex].depthFiles.size() > 0)
		{

			std::map<ushort, unsigned int> measured_depth_count_map;
			std::map<float, unsigned int> aruco_depth_count_map;
			for (unsigned int frameIndex = 0; frameIndex < recordedFiles[dirIndex].imageFiles.size(); frameIndex++)
			{
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

				cv::Mat_<cv::Vec3f> coordinates = bow::CameraCalibration::calculate_coordinates_from_depth(distorted_directionMat, depthMat);

				// ====================================
				// find pose of markers
				// ====================================

				std::vector<bow::Marker> detectedMarkers = bow::ArucoHelper::detectMarker(undistortedColorMat, rgb_intrinisicCameraParameters.cameraMatrix, empty_DistCoeffs, sideLength, big_MarkerMap);

				cv::Matx<float, 4, 4> transform;
				if (bow::ArucoHelper::getTransformationFromMarkerMap(big_MarkerMap, detectedMarkers, transform))
				{
					cv::Mat invertedTransform;
					cv::invert(transform, invertedTransform);

					cv::Mat viewMat = cv::Mat::zeros(4, 4, CV_32FC1);
					viewMat.at<float>(0, 0) = 1.0f;
					viewMat.at<float>(1, 1) = 1.0f;
					viewMat.at<float>(2, 2) = -1.0f;
					viewMat.at<float>(3, 3) = 1.0f;

					// ========================================================================================
					// Calculate center point projection on depth-map to compate it with measured vaules
					// ========================================================================================

					cv::Mat tansofrmedCenter = viewMat * rgbToIrCameraViewMatrix * cv::Mat(invertedTransform) * cv::Mat(cv::Vec4f(center.val[0], center.val[1], center.val[2], 1.0f));
					cv::Mat projectedCenter = ir_intrinisicCameraParameters.cameraProjectionMatrix *  tansofrmedCenter;

					float screen_x = (((projectedCenter.at<float>(0, 0) / projectedCenter.at<float>(3, 0)) + 1.0f) * 0.5f) * ir_intrinisicCameraParameters.image_width;
					float screen_y = (((-(projectedCenter.at<float>(1, 0) / projectedCenter.at<float>(3, 0))) + 1.0f) * 0.5f) * ir_intrinisicCameraParameters.image_height;
					float depth_z = projectedCenter.at<float>(2, 0);
					float aruco_range = cv::norm(bow::CameraCalibration::calculate_coordinate_from_depth(undistorted_directionMat, depth_z, screen_y, screen_x));

					if (screen_x < depthMat.rows && screen_y < depthMat.cols)
					{
						if (aruco_depth_count_map.find(aruco_range) == aruco_depth_count_map.end())
						{
							aruco_depth_count_map.insert(std::pair<float, unsigned int>(aruco_range, 0));
						}
						aruco_depth_count_map[aruco_range] = aruco_depth_count_map[aruco_range] + 1;

						ushort measured_range = cv::norm(coordinates.at<cv::Vec3f>(screen_y, screen_x));
						if (measured_depth_count_map.find(measured_range) == measured_depth_count_map.end())
						{
							measured_depth_count_map.insert(std::pair<ushort, unsigned int>(measured_range, 0));
						}
						measured_depth_count_map[measured_range] = measured_depth_count_map[measured_range] + 1;
					}
				}
			}

			double measured_mean_value = 0;
			double max_count = 0;
			for (auto depthValueEntry = measured_depth_count_map.begin(); depthValueEntry != measured_depth_count_map.end(); depthValueEntry++)
			{
				if (depthValueEntry->first > 0 && depthValueEntry->second > 0)
				{
					measured_mean_value += (double)depthValueEntry->first * (double)depthValueEntry->second;
					max_count += depthValueEntry->second;
				}
			}
			if (max_count > 0)
			{
				measured_mean_value = measured_mean_value / max_count;
			}


			// calculate mean value
			double aruco_mean_value = 0;
			max_count = 0;
			for (auto depthValueEntry = aruco_depth_count_map.begin(); depthValueEntry != aruco_depth_count_map.end(); depthValueEntry++)
			{
				if (depthValueEntry->first > 0 && depthValueEntry->second > 0)
				{
					aruco_mean_value += (double)depthValueEntry->first * (double)depthValueEntry->second;
					max_count += depthValueEntry->second;
				}
			}
			if (max_count > 0)
			{
				aruco_mean_value = aruco_mean_value / max_count;
			}

			if (csv_file.is_open())
			{
				if (measured_mean_value > 0 && aruco_mean_value > 0)
				{
					std::string key = std::to_string(aruco_mean_value);
					std::replace(key.begin(), key.end(), '.', ',');

					std::string value = std::to_string(aruco_mean_value - measured_mean_value);
					std::replace(value.begin(), value.end(), '.', ',');

					csv_file << key
						<< ";" << value
						<< std::endl;
					datFile << std::to_string(aruco_mean_value) << " " << std::to_string(aruco_mean_value - measured_mean_value) << std::endl;
				}
			}

			if (measured_mean_value > 0 && aruco_mean_value > 0)
			{
				if (arucoDepth_to_tofDepth_map.find((unsigned short)aruco_mean_value) == arucoDepth_to_tofDepth_map.end())
				{
					arucoDepth_to_tofDepth_map.insert(std::pair<unsigned short, std::map<unsigned short, unsigned int>>((unsigned short)aruco_mean_value, std::map<unsigned short, unsigned int>()));
				}
				if (arucoDepth_to_tofDepth_map[(unsigned short)aruco_mean_value].find(measured_mean_value) == arucoDepth_to_tofDepth_map[(unsigned short)aruco_mean_value].end())
				{
					arucoDepth_to_tofDepth_map[(unsigned short)aruco_mean_value].insert(std::pair<unsigned short, unsigned int>(measured_mean_value, 0));
				}
				arucoDepth_to_tofDepth_map[(unsigned short)aruco_mean_value][measured_mean_value] = arucoDepth_to_tofDepth_map[(unsigned short)aruco_mean_value][measured_mean_value] + 1;
			}
		}
	}

	csv_file.close();
	datFile.close();

	csv_file.open(recordingsFolderPath + "\\systematic_error_center.csv", std::ofstream::out | std::ofstream::trunc);
	datFile.open(recordingsFolderPath + "\\systematic_error_center.dat", std::ofstream::out | std::ofstream::trunc);

	std::map<ushort, double> system_error_median_map;
	for (auto arucoDepthIt = arucoDepth_to_tofDepth_map.begin(); arucoDepthIt != arucoDepth_to_tofDepth_map.end(); arucoDepthIt++)
	{
		double max_count = 0;
		double median_depth_value = 0;
		for (auto noiseIt = arucoDepthIt->second.begin(); noiseIt != arucoDepthIt->second.end(); noiseIt++)
		{
			median_depth_value += noiseIt->first * noiseIt->second;
			max_count += noiseIt->second;
		}
		if (max_count > 0)
			median_depth_value = median_depth_value / (double)max_count;

		if (system_error_median_map.find(arucoDepthIt->first) == system_error_median_map.end())
		{
			system_error_median_map.insert(std::pair<ushort, double>(arucoDepthIt->first, median_depth_value));
		}

		if (csv_file.is_open())
		{
			if (arucoDepthIt->first > 0)
			{
				std::string key = std::to_string(arucoDepthIt->first);
				std::replace(key.begin(), key.end(), '.', ',');

				std::string value = std::to_string(arucoDepthIt->first - median_depth_value);
				std::replace(value.begin(), value.end(), '.', ',');

				csv_file << key
					<< ";" << value
					<< std::endl;
				datFile << std::to_string(arucoDepthIt->first) << " " << std::to_string(arucoDepthIt->first - median_depth_value) << std::endl;
			}
		}
	}

	csv_file.close();
	datFile.close();


	std::vector<unsigned int> sums_list;
	for (unsigned int i = 1; i <= 80; i++)
	{
		sums_list.push_back(i * 50);
	}

	for (unsigned int sum_index = 0; sum_index < sums_list.size(); sum_index++)
	{
		unsigned int sum_count = sums_list[sum_index];
		csv_file.open(recordingsFolderPath + "\\systematic_error_center_smoothed_" + std::to_string(sum_count) + ".csv", std::ofstream::out | std::ofstream::trunc);
		datFile.open(recordingsFolderPath + "\\systematic_error_center_smoothed_" + std::to_string(sum_count) + ".dat", std::ofstream::out | std::ofstream::trunc);
		for (auto arucoDepthIt = system_error_median_map.begin(); arucoDepthIt != system_error_median_map.end(); arucoDepthIt++)
		{
			std::vector<std::pair<ushort, double>> points_witin_range;
			for (auto internalIt = system_error_median_map.begin(); internalIt != system_error_median_map.end(); internalIt++)
			{
				if (((float)internalIt->first - (float)arucoDepthIt->first) * ((float)internalIt->first - (float)arucoDepthIt->first) < (float)sum_count * (float)sum_count)
				{
					points_witin_range.push_back(std::pair<ushort, double>(internalIt->first, internalIt->second));
				}
			}

			double newValue = 0.0f;
			double weightSum = 0.0f;
			// Calculate smoothed 3d point. Gaussian smoothing formula is applied.
			for (int j = 0; j < static_cast<int>(points_witin_range.size()); j++)
			{
				float distance = cv::norm((float)points_witin_range[j].first - (float)arucoDepthIt->first);	// Squared euclidean distance between a leaf node 3d point and its i-th neighborhood point.
				float weight = exp(-(distance * distance) / (float)sum_count); // Gaussian weights.

				newValue += points_witin_range[j].second * weight;

				weightSum += weight;
			}
			if (weightSum > 0)
				newValue = newValue / weightSum;

			if (csv_file.is_open())
			{
				if (arucoDepthIt->first > 0)
				{
					std::string key = std::to_string(arucoDepthIt->first);
					std::replace(key.begin(), key.end(), '.', ',');

					std::string value = std::to_string((double)arucoDepthIt->first - newValue);
					std::replace(value.begin(), value.end(), '.', ',');

					csv_file << key
						<< ";" << value
						<< std::endl;
					datFile << std::to_string(arucoDepthIt->first) << " " << std::to_string((double)arucoDepthIt->first - newValue) << std::endl;
				}
			}
		}
		csv_file.close();
		datFile.close();
	}

	sums_list.clear();
	for (unsigned int i = 1; i <= 20; i++)
	{
		sums_list.push_back(i * 10);
	}

	for (unsigned int sum_index = 0; sum_index < sums_list.size(); sum_index++)
	{
		unsigned int sum_count = sums_list[sum_index];
		csv_file.open(recordingsFolderPath + "\\summed_systematic_error_center_" + std::to_string(sum_count) + ".csv", std::ofstream::out | std::ofstream::trunc);
		datFile.open(recordingsFolderPath + "\\summed_systematic_error_center_" + std::to_string(sum_count) + ".dat", std::ofstream::out | std::ofstream::trunc);
		ushort last = 0;
		float depth_sum = 0.0f;
		float depth_sum_count = 0.0f;
		std::vector<ushort> values;
		for (auto arucoDepthIt = system_error_median_map.begin(); arucoDepthIt != system_error_median_map.end(); arucoDepthIt++)
		{
			if (arucoDepthIt->first / sum_count != last)
			{
				if (csv_file.is_open())
				{
					if (arucoDepthIt->first > 0 && depth_sum_count > 0)
					{
						std::string key = std::to_string(depth_sum / depth_sum_count);
						std::replace(key.begin(), key.end(), '.', ',');

						std::sort(values.begin(), values.end(), [](double left, double right) { return left < right; });
						ushort median = values[values.size() * 0.5f];

						std::string value = std::to_string((depth_sum / depth_sum_count) - (float)median);
						std::replace(value.begin(), value.end(), '.', ',');

						csv_file << key
							<< ";" << value
							<< std::endl;
						datFile << std::to_string(depth_sum / depth_sum_count) << " " << std::to_string((depth_sum / depth_sum_count) - (float)median) << std::endl;
					}
				}

				last = arucoDepthIt->first / sum_count;
				depth_sum = 0.0f;
				depth_sum_count = 0.0f;
				values.clear();
			}

			depth_sum += arucoDepthIt->first;
			depth_sum_count += 1.0;
			values.push_back(arucoDepthIt->second);
		}
		csv_file.close();
		datFile.close();
	}
}

int main()
{
	//runAnalysis("/data/IFM_O3D303_Calibration.xml", "F:\\Kamera_Evaluation\\03D303\\Recordings_SystematicError_2", "/data/map_systematic_error.yml");
	//runAnalysis("/data/Kinect_v2_Calibration.xml", "D:\\Kamera_Evaluation\\Kinect_v2\\Recordings_SystematicError_2", "/data/map_systematic_error.yml");
	//runAnalysis("/data/Xtion_2_Calibration.xml", "F:\\Kamera_Evaluation\\Xtion_2\\Recordings_SystematicError_2", "/data/map_systematic_error.yml");
	return 0;
}
