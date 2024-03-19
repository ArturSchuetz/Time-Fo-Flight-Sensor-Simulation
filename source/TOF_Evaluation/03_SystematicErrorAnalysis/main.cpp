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

void runAnalysis(const std::string& calibrationFilePath, const std::string& recordingsFolderPath, const std::string& small_markerMapPath, const std::string& big_markerMapPath)
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

	std::vector<bow::MarkerDescription> small_MarkerMap = bow::ArucoHelper::LoadMarkerMapFromFile(std::string(PROJECT_BASE_DIR) + small_markerMapPath);
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

	unsigned int lastPercentage = 0;

	std::ofstream csv_aruco_file;
	csv_aruco_file.open(recordingsFolderPath + "\\aruco_distance.csv", std::ofstream::out | std::ofstream::trunc);

	std::ofstream csv_speed_file;
	csv_speed_file.open(recordingsFolderPath + "\\aruco_speed.csv", std::ofstream::out | std::ofstream::trunc);

	std::map<unsigned short, std::map<unsigned short, unsigned int>> arucoDepth_to_tofDepth_map;
	std::vector<bow::DepthFileData> recordedFiles = bow::DataLoader::loadRecordedFilesFromFolder(recordingsFolderPath);
	for (unsigned int dirIndex = 0; dirIndex < recordedFiles.size(); dirIndex++)
	{
		long long lastTimeStamp = 0;
		ushort lastDistance = 0;
		if (recordedFiles[dirIndex].imageFiles.size() > 0 && recordedFiles[dirIndex].depthFiles.size() > 0)
		{
			unsigned int start_depth_count = 0;
			unsigned int end_depth_count = 0;

			float start_depth = 0;
			float end_depth = 0;

			long long time_start = recordedFiles[dirIndex].imageFiles.front().timestamp;
			long long time_end = recordedFiles[dirIndex].imageFiles.back().timestamp;

			std::cout << "Checking Start and End-Distance" << std::endl;
			for (unsigned int frameIndex = 0; frameIndex < recordedFiles[dirIndex].imageFiles.size(); frameIndex++)
			{
				if (recordedFiles[dirIndex].imageFiles[frameIndex].timestamp - time_start < 120000 || time_end - recordedFiles[dirIndex].imageFiles[frameIndex].timestamp < 120000)
				{
					cv::Mat undistortedColorMat;
					cv::Mat empty_DistCoeffs = cv::Mat::zeros(4, 1, CV_32F);

					cv::Mat colorMat = cv::imread(recordedFiles[dirIndex].imageFiles[frameIndex].filename, CV_LOAD_IMAGE_UNCHANGED);
					cv::undistort(colorMat, undistortedColorMat, rgb_intrinisicCameraParameters.cameraMatrix, rgb_intrinisicCameraParameters.distCoeffs);

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

						float depth_z = projectedCenter.at<float>(2, 0);

						if (recordedFiles[dirIndex].imageFiles[frameIndex].timestamp - time_start < 120000)
						{
							double a = 1.0f / (double)(start_depth_count++ + 1);
							start_depth = (depth_z * (1.0 - a)) + (depth_z * a);
						}
						else if (time_end - recordedFiles[dirIndex].imageFiles[frameIndex].timestamp < 120000)
						{
							double a = 1.0f / (double)(end_depth_count++ + 1);
							end_depth = (depth_z * (1.0 - a)) + (depth_z * a);
						}
					}
				}
			}
			std::cout << "start_depth: " << start_depth << std::endl;
			std::cout << "end_depth: " << end_depth << std::endl;

			std::cout << "Analyzing..." << std::endl;
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

				cv::Mat undistortedColorMat;
				cv::Mat empty_DistCoeffs = cv::Mat::zeros(4, 1, CV_32F);

				cv::Mat colorMat = bow::DataLoader::findClosestImageFile(recordedFiles[dirIndex].depthFiles[frameIndex].timestamp, recordedFiles[dirIndex].imageFiles);
				cv::undistort(colorMat, undistortedColorMat, rgb_intrinisicCameraParameters.cameraMatrix, rgb_intrinisicCameraParameters.distCoeffs);

				cv::Mat_<ushort> depthMat = bow::DataLoader::loadDepthFromFile(recordedFiles[dirIndex].depthFiles[frameIndex].filename);
				if (depthMat.cols == 0 || depthMat.rows == 0)
					continue;

				cv::Mat_<cv::Vec3f> coordinates = bow::CameraCalibration::calculate_coordinates_from_depth(distorted_directionMat, depthMat);

				// ====================================
				// find pose of markers
				// ====================================

				std::vector<bow::Marker> detectedMarkers = bow::ArucoHelper::detectMarker(undistortedColorMat, rgb_intrinisicCameraParameters.cameraMatrix, empty_DistCoeffs, sideLength);

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

					if (lastTimeStamp == 0 && aruco_range > 0)
					{
						lastTimeStamp = recordedFiles[dirIndex].depthFiles[frameIndex].timestamp;
						lastDistance = (unsigned short)aruco_range;
						continue;
					}

					float strecke = (float)aruco_range - (float)lastDistance;
					float zeit = (float)recordedFiles[dirIndex].depthFiles[frameIndex].timestamp - (float)lastTimeStamp;
					float speed = (float)strecke / (float)zeit;

					if (csv_speed_file.is_open())
					{
						std::string value = std::to_string(speed);
						std::replace(value.begin(), value.end(), '.', ',');
						csv_speed_file << std::to_string(recordedFiles[dirIndex].depthFiles[frameIndex].timestamp) << ";" << value << std::endl;
					}

					if (screen_x < depthMat.rows && screen_y < depthMat.cols && speed < 0.05f && speed > -0.05f && ((start_depth - end_depth > 0 && aruco_range <= (lastDistance + 5) || (start_depth - end_depth < 0 && aruco_range >= (lastDistance - 5)))))
					{
						ushort measured_range = cv::norm(coordinates.at<cv::Vec3f>(screen_y, screen_x));
						if (measured_range > 0)
						{
							if (arucoDepth_to_tofDepth_map.find((unsigned short)aruco_range) == arucoDepth_to_tofDepth_map.end())
							{
								arucoDepth_to_tofDepth_map.insert(std::pair<unsigned short, std::map<unsigned short, unsigned int>>((unsigned short)aruco_range, std::map<unsigned short, unsigned int>()));
							}
							if (arucoDepth_to_tofDepth_map[(unsigned short)aruco_range].find(measured_range) == arucoDepth_to_tofDepth_map[(unsigned short)aruco_range].end())
							{
								arucoDepth_to_tofDepth_map[(unsigned short)aruco_range].insert(std::pair<unsigned short, unsigned int>(measured_range, 0));
							}
							arucoDepth_to_tofDepth_map[(unsigned short)aruco_range][measured_range] = arucoDepth_to_tofDepth_map[(unsigned short)aruco_range][measured_range] + 1;
						}

						if (csv_aruco_file.is_open())
						{
							csv_aruco_file << std::to_string(recordedFiles[dirIndex].depthFiles[frameIndex].timestamp) << ";" << std::to_string((short)aruco_range) << std::endl;
						}

						lastTimeStamp = recordedFiles[dirIndex].depthFiles[frameIndex].timestamp;
						lastDistance = (unsigned short)aruco_range;
					}
				}
			}
		}
	}
	csv_aruco_file.close();
	csv_speed_file.close();

	// ====================================
	// Write results into a csv file
	// ====================================

	std::ofstream csv_file;
	std::ofstream datFile;

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

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

void runPulsedSimulationAnalysis()
{
	// ====================================
	// Write results into a csv file
	// ====================================
	float lightIntensity = 1.0f;
	const double speedOfLight = 299792458.0;
	const double pulselength = 0.00000005f;

	std::ofstream csv_file;
	csv_file.open("systematic_error_pulsed.csv", std::ofstream::out | std::ofstream::trunc);

	std::ofstream datFile;
	datFile.open("systematic_error_pulsed.csv", std::ofstream::out | std::ofstream::trunc);

	float maxDistanceInMeter = ((speedOfLight * pulselength) * 0.5f);
	for (unsigned int original_depth = 1; original_depth < maxDistanceInMeter * 1000 && original_depth < 3500; original_depth++)
	{
		const double lightdist = (double)original_depth / 1000.0;
		double deltaTime = (lightdist * 2.0f) / speedOfLight;

		double attenuation = 1.0 / (lightdist * lightdist);
		double Intensity = lightIntensity * attenuation;

		double C1_start = 0.0f;
		double C2_start = pulselength;

		bow::Vector2<double> ir_result = bow::Vector2<double>(0.0, 0.0);

		float begin = deltaTime;
		float end = deltaTime + pulselength;

		if ((end >= C1_start && end <= C1_start + pulselength) || (begin >= C1_start && begin <= C1_start + pulselength))
		{
			if (begin > C1_start)
				ir_result.x += (((C1_start + pulselength) - begin) / pulselength) * Intensity;
			else
				ir_result.x += ((end - C1_start) / pulselength) * Intensity;
		}

		if ((end >= C2_start && end <= C2_start + pulselength) || (begin >= C2_start && begin <= C2_start + pulselength))
		{
			if (begin > C2_start)
				ir_result.y += (((C2_start + pulselength) - begin) / pulselength) * Intensity;
			else
				ir_result.y += ((end - C2_start) / pulselength) * Intensity;
		}

		double out_distance = 0.0;
		out_distance = (0.5f * speedOfLight * pulselength * (ir_result.y / (ir_result.x + ir_result.y)));

		if (csv_file.is_open())
		{
			if (original_depth > 0)
			{
				std::string value = std::to_string((double)((double)original_depth - (out_distance * 1000.0)));
				std::replace(value.begin(), value.end(), '.', ',');

				csv_file << original_depth
					<< ";" << value
					<< std::endl;
				datFile << std::to_string(original_depth) << " " << std::to_string((double)((double)original_depth - (out_distance * 1000.0))) << std::endl;
			}
		}
	}
	csv_file.close();
	datFile.close();
}

double getMinimumDistance(std::vector<std::vector<double>>& distancesByFrequencies, unsigned int depth, double compareDistance)
{
	double newDistance = 0;
	double difference = std::numeric_limits<double>::max();
	for (unsigned int i = 0; i < distancesByFrequencies[depth].size(); i++)
	{
		if ((compareDistance - distancesByFrequencies[depth][i]) * (compareDistance - distancesByFrequencies[depth][i]) < difference)
		{
			difference = (compareDistance - distancesByFrequencies[depth][i]) * (compareDistance - distancesByFrequencies[depth][i]);
			newDistance = distancesByFrequencies[depth][i];
		}
	}

	if (distancesByFrequencies.size() > depth + 1)
		return getMinimumDistance(distancesByFrequencies, depth + 1, newDistance);
	else
		return newDistance;
}

void runCwSimulationAnalysis()
{
	// ====================================
	// Write results into a csv file
	// ====================================
	double lightIntensity = 1.0f;
	const double speedOfLight = 299792458.0;

	std::vector<double> frequencies;
	frequencies.push_back(16  * 1000.0 * 1000.0);// 16 Mhz 
	frequencies.push_back(80  * 1000.0 * 1000.0);// 80 Mhz 
	frequencies.push_back(120 * 1000.0 * 1000.0);// 120 Mhz 

	double beatFrequency = 8 * 1000.0 * 1000.0;
	double maxDistanceInMeter = (speedOfLight / (2.0 * beatFrequency));

	std::map<unsigned int, double> depthValueMap;
	for (unsigned int original_depth = 1; original_depth < (maxDistanceInMeter * 1000); original_depth++)
	{
		const double lightdist = (double)original_depth / 1000.0;
		const double attenuation = 1.0 / (lightdist * lightdist);
		const double Intensity = lightIntensity * attenuation;

		std::map<double, double> depthValuesByFrequencies;
		for (unsigned int i = 0; i < frequencies.size(); i++)
		{
			const double frequency = frequencies[i];
			const double pulselength = (1.0 / frequency) * 0.5;

			const double C1_start = 0.0;
			const double C2_start = pulselength;

			const double C3_start = pulselength * 0.5;
			const double C4_start = (pulselength * 0.5) + pulselength;

			double deltaTime = (lightdist * 2.0) / speedOfLight;
			bow::Vector4<double> ir_result = bow::Vector4<double>(0.0, 0.0, 0.0, 0.0);
			while (deltaTime < C4_start + pulselength)
			{
				deltaTime = deltaTime + (pulselength * 2.0);
			}

			while (deltaTime + pulselength > 0.0)
			{
				double begin = deltaTime;
				double end = deltaTime + pulselength;

				if ((end >= C1_start && end <= C1_start + pulselength) || (begin >= C1_start && begin <= C1_start + pulselength))
				{
					if (begin > C1_start)
						ir_result.x += (((C1_start + pulselength) - begin) / pulselength) * Intensity;
					else
						ir_result.x += ((end - C1_start) / pulselength) * Intensity;
				}

				if ((end >= C2_start && end <= C2_start + pulselength) || (begin >= C2_start && begin <= C2_start + pulselength))
				{
					if (begin > C2_start)
						ir_result.y += (((C2_start + pulselength) - begin) / pulselength) * Intensity;
					else
						ir_result.y += ((end - C2_start) / pulselength) * Intensity;
				}

				if ((end >= C3_start && end <= C3_start + pulselength) || (begin >= C3_start && begin <= C3_start + pulselength))
				{
					if (begin > C3_start)
						ir_result.z += (((C3_start + pulselength) - begin) / pulselength) * Intensity;
					else
						ir_result.z += ((end - C3_start) / pulselength) * Intensity;
				}

				if ((end >= C4_start && end <= C4_start + pulselength) || (begin >= C4_start && begin <= C4_start + pulselength))
				{
					if (begin > C4_start)
						ir_result.w += (((C4_start + pulselength) - begin) / pulselength) * Intensity;
					else
						ir_result.w += ((end - C4_start) / pulselength) * Intensity;
				}

				deltaTime = deltaTime - (pulselength * 2.0);
			}

			double out_distance = 0.0;
			if ((ir_result.x - ir_result.y) != 0.0)
			{
				double phi = atan2((ir_result.z - ir_result.w), (ir_result.x - ir_result.y));

				if (phi < 0.0)
					phi = (2.0 * M_PI) + phi;

				out_distance = (speedOfLight / (4.0 * M_PI * frequency)) * phi;
			}
			else
			{
				out_distance = 0.0;
			}

			depthValuesByFrequencies.insert(std::pair<double, double>(frequency, out_distance));
		}

		float best_distance = 0.0;
		std::map<double, double> bestMatch;
		if (false)
		{
			double currentDistance = -1;
			for (auto depthByFrequencyOuterIt = depthValuesByFrequencies.begin(); depthByFrequencyOuterIt != depthValuesByFrequencies.end(); depthByFrequencyOuterIt++)
			{
				if (currentDistance < 0)
				{
					currentDistance = depthByFrequencyOuterIt->second;
				}
				else
				{
					double difference = std::numeric_limits<double>::max();

					double temp_new_distance = 0.0;
					double referenceDist = depthByFrequencyOuterIt->second;
					while (referenceDist < maxDistanceInMeter)
					{
						if ((referenceDist - currentDistance) * (referenceDist - currentDistance) < difference)
						{
							temp_new_distance = referenceDist;
							difference = (referenceDist - currentDistance) * (referenceDist - currentDistance);
						}
						referenceDist += (speedOfLight / (2.0 * (depthByFrequencyOuterIt->first)));
					}
					currentDistance = temp_new_distance;
				}

				if (bestMatch.find(depthByFrequencyOuterIt->first) == bestMatch.end())
				{
					bestMatch.insert(std::pair<double, double>(depthByFrequencyOuterIt->first, currentDistance));
				}
			}

			double highest_frequency = 0;
			for (auto depthByFrequencyIt = bestMatch.begin(); depthByFrequencyIt != bestMatch.end(); depthByFrequencyIt++)
			{
				if (depthByFrequencyIt->first > highest_frequency)
				{
					highest_frequency = depthByFrequencyIt->first;
					best_distance = depthByFrequencyIt->second;
				}
			}
		}
		else
		{
			std::vector<std::vector<double>> distancesByFrequencies;
			double currentDistance = -1;
			for (auto outer_depthByFrequencyIt = depthValuesByFrequencies.begin(); outer_depthByFrequencyIt != depthValuesByFrequencies.end(); outer_depthByFrequencyIt++)
			{
				distancesByFrequencies.push_back(std::vector<double>());
				double outer_referenceDist = outer_depthByFrequencyIt->second;
				while (outer_referenceDist < maxDistanceInMeter)
				{
					distancesByFrequencies.back().push_back(outer_referenceDist);
					outer_referenceDist += (speedOfLight / (2.0 * (outer_depthByFrequencyIt->first)));
				}
			}

			float min_difference = std::numeric_limits<double>::max();
			if (frequencies.size() > 1)
			{
				for (unsigned int i = 0; i < distancesByFrequencies.front().size(); i++)
				{
					double distance = getMinimumDistance(distancesByFrequencies, 1, distancesByFrequencies[0][i]);
					if ((distance - distancesByFrequencies[0][i]) * (distance - distancesByFrequencies[0][i]) < min_difference)
					{
						min_difference = (distance - distancesByFrequencies[0][i]) * (distance - distancesByFrequencies[0][i]);
						best_distance = distance;
					}
				}
			}
			else
			{
				for (unsigned int i = 0; i < distancesByFrequencies.front().size(); i++)
				{
					double distance = distancesByFrequencies.front().front();
					if ((distance - distancesByFrequencies[0][i]) * (distance - distancesByFrequencies[0][i]) < min_difference)
					{
						min_difference = (distance - distancesByFrequencies[0][i]) * (distance - distancesByFrequencies[0][i]);
						best_distance = distance;
					}
				}
			}
		}

		if (depthValueMap.find(original_depth) == depthValueMap.end())
		{
			depthValueMap.insert(std::pair<unsigned int, double>(original_depth, 0));
		}
		depthValueMap[original_depth] = (double)(best_distance * 1000.0);
	}

	std::ofstream csv_file;
	csv_file.open("systematic_error_cw.csv", std::ofstream::out | std::ofstream::trunc);

	std::ofstream datFile;
	datFile.open("systematic_error_cw.csv", std::ofstream::out | std::ofstream::trunc);

	for (auto errorIt = depthValueMap.begin(); errorIt != depthValueMap.end(); errorIt++)
	{
		if (csv_file.is_open())
		{
			if (errorIt->first > 0)
			{
				std::string value = std::to_string((double)errorIt->first - (double)errorIt->second);
				std::replace(value.begin(), value.end(), '.', ',');

				csv_file << errorIt->first
					<< ";" << value
					<< std::endl;

				datFile << std::to_string(errorIt->first) << " " << std::to_string((double)errorIt->first - (double)errorIt->second) << std::endl;
			}
		}
	}
	csv_file.close();
	datFile.close();
}

double getArea(double amplitude, double frequency, double offset, double from, double to)
{
	return ((amplitude * (cos(2.0 * M_PI * frequency * from) - cos(2.0 * M_PI * frequency * to))) / (2.0 * M_PI * frequency)) - (offset * from) + (offset * to);
}

void runSineCwSimulationAnalysis()
{
	// ====================================
	// Write results into a csv file
	// ====================================
	double lightIntensity = 1.0f;
	const double speedOfLight = 299792458.0;

	double frequency = 16 * 1000.0 * 1000.0;// 16 Mhz

	double beatFrequency = 16 * 1000.0 * 1000.0;
	double maxDistanceInMeter = (speedOfLight / (2.0 * beatFrequency));

	std::map<unsigned int, double> depthValueMap;
	for (unsigned int original_depth = 1; original_depth < (maxDistanceInMeter * 1000); original_depth++)
	{
		const double lightdist = (double)original_depth / 1000.0;
		const double attenuation = 1.0 / (lightdist * lightdist);
		const double Intensity = lightIntensity * attenuation;

		const double quarterlength = 1.0 / (4.0 * frequency);

		const double C1_start = 0.0 * quarterlength;
		const double C2_start = 1.0 * quarterlength;

		const double C3_start = 2.0 * quarterlength;
		const double C4_start = 3.0 * quarterlength;

		double deltaTime = (lightdist * 2.0) / speedOfLight;
		bow::Vector4<double> ir_result = bow::Vector4<double>(0.0, 0.0, 0.0, 0.0);

		// Calculate intensity for Q1, Q2, Q3 and Q4
		ir_result.x = getArea(Intensity, frequency, Intensity, C1_start - deltaTime, (C1_start - deltaTime) + (2.0 * quarterlength));
		ir_result.y = getArea(Intensity, frequency, Intensity, C2_start - deltaTime, (C2_start - deltaTime) + (2.0 * quarterlength));
		ir_result.z = getArea(Intensity, frequency, Intensity, C3_start - deltaTime, (C3_start - deltaTime) + (2.0 * quarterlength));
		ir_result.w = getArea(Intensity, frequency, Intensity, C4_start - deltaTime, (C4_start - deltaTime) + (2.0 * quarterlength));

		double out_distance = 0.0;
		if ((ir_result.x - ir_result.y) != 0.0)
		{
			double phi = atan2((ir_result.y - ir_result.w), (ir_result.x - ir_result.z));

			if (phi < 0.0)
				phi = (2.0 * M_PI) + phi;

			out_distance = (speedOfLight / (4.0 * M_PI * frequency)) * phi;
		}
		else
		{
			out_distance = 0.0;
		}

		if (depthValueMap.find(original_depth) == depthValueMap.end())
		{
			depthValueMap.insert(std::pair<unsigned int, double>(original_depth, 0));
		}
		depthValueMap[original_depth] = (double)(out_distance * 1000.0);
	}

	std::ofstream csv_file;
	csv_file.open("systematic_error_sine_cw.csv", std::ofstream::out | std::ofstream::trunc);

	std::ofstream datFile;
	datFile.open("systematic_error_sine_cw.csv", std::ofstream::out | std::ofstream::trunc);

	for (auto errorIt = depthValueMap.begin(); errorIt != depthValueMap.end(); errorIt++)
	{
		if (csv_file.is_open())
		{
			if (errorIt->first > 0)
			{
				std::string value = std::to_string((double)errorIt->first - (double)errorIt->second);
				std::replace(value.begin(), value.end(), '.', ',');

				csv_file << errorIt->first
					<< ";" << value
					<< std::endl;
				datFile << std::to_string(errorIt->first) << " " << std::to_string((double)errorIt->first - (double)errorIt->second) << std::endl;
			}
		}
	}
	csv_file.close();
	datFile.close();
}

int main()
{
	runAnalysis("/data/IFM_O3D303_Calibration.xml", "F:\\Kamera_Evaluation\\03D303\\Recordings_SystematicError", "/data/map_small.yml", "/data/map_big.yml");
	runAnalysis("/data/IFM_O3D303_Calibration.xml", "F:\\Kamera_Evaluation\\03D303\\Recordings_SystematicError_1_2", "/data/map_small.yml", "/data/map_systematic_error.yml");
	runAnalysis("/data/Frankenstein_Calibration.xml", "F:\\Kamera_Evaluation\\03D303\\Recordings_SystematicError_Frankenstein", "/data/map_small.yml", "/data/map_systematic_error.yml");
	runAnalysis("/data/Kinect_v2_Calibration.xml", "F:\\Kamera_Evaluation\\Kinect_v2\\Recordings_SystematicError", "/data/map_small.yml", "/data/map_big.yml");
	runAnalysis("/data/Xtion_2_Calibration.xml", "F:\\Kamera_Evaluation\\Xtion_2\\Recordings_SystematicError", "/data/map_small.yml", "/data/map_big.yml");
	//runPulsedSimulationAnalysis();
	//runCwSimulationAnalysis();
	//runSineCwSimulationAnalysis();
	
	return 0;
}
