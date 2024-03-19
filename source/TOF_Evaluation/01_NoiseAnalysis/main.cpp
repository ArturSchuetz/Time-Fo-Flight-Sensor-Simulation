#include <RenderDevice/BowRenderer.h>
#include <CoreSystems/BowCoreSystems.h>
#include <CoreSystems/BowBasicTimer.h>

#include <CameraUtils/CameraCalibration.h>
#include <CameraUtils/RenderingConfigs.h>
#include <EvaluationUtils/DataLoader.h>

#include <Masterthesis/cuda_config.h>

#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>
const unsigned int g_sumUpCount = 100;

void runRangeDependentAnalysis(const std::vector<bow::DepthFileData>& depthFiles, const std::string& folderPath, const std::string& calibrationFilePath)
{
	bow::RenderingConfigs configs = bow::ConfigLoader::loadConfigFromFile(std::string(PROJECT_BASE_DIR) + calibrationFilePath);
	bow::IntrinsicCameraParameters intrinisicCameraParameters = bow::CameraCalibration::intrinsicChessboardCalibration(configs.calibration_checkerboard_width, configs.calibration_checkerboard_height, configs.calibration_checkerboard_squareSize, std::string(PROJECT_BASE_DIR) + std::string("/data/") + configs.irCameraCheckerboardImagesPath);
	cv::Mat_<cv::Vec4f> directionMatrix = bow::CameraCalibration::calculate_directionMatrix(intrinisicCameraParameters, intrinisicCameraParameters.image_width, intrinisicCameraParameters.image_height);

	std::map<ushort, std::map<ushort, unsigned int>> depth_to_noise_map;
	for (unsigned int dirIndex = 0; dirIndex < depthFiles.size(); dirIndex++)
	{
		std::cout << "Analysing files... (" << std::to_string((unsigned int)(((double)dirIndex / (double)depthFiles.size()) * 100.0)) << "%)\t\r";

		// Step 1: Analyse depthfiles and counting all depth values to calculate mean value
		bool depth_count_map_initialized = false;
		std::vector<std::vector<std::map<ushort, unsigned int>>> depth_count_map;
		for (unsigned int fileIndex = 0; fileIndex < depthFiles[dirIndex].depthFiles.size(); fileIndex++)
		{
			cv::Mat_<ushort> deptMap = bow::DataLoader::loadDepthFromFile(depthFiles[dirIndex].depthFiles[fileIndex].filename);
			cv::Mat_<cv::Vec3f> coordinates = bow::CameraCalibration::calculate_coordinates_from_depth(directionMatrix, deptMap);
			if (!depth_count_map_initialized)
			{
				depth_count_map = std::vector<std::vector<std::map<ushort, unsigned int>>>(deptMap.rows);
				for (unsigned int row_ = 0; row_ < depth_count_map.size(); row_++)
				{
					depth_count_map[row_] = std::vector<std::map<ushort, unsigned int>>(deptMap.cols);
				}
				depth_count_map_initialized = true;
			}

			for (unsigned int row = 0; row < coordinates.rows; row++)
			{
				auto mapRow = &(depth_count_map[(unsigned int)(row)]);
				for (unsigned int col = 0; col < coordinates.cols; col++)
				{
					ushort rangeValue = cv::norm(coordinates.at<cv::Vec3f>(row, col));

					auto mapElement = &((*mapRow)[(unsigned int)(col)]);
					if (mapElement->find(rangeValue) == mapElement->end())
					{
						mapElement->insert(std::pair<ushort, unsigned int>(rangeValue, 0));
					}

					(*mapElement)[rangeValue] = (*mapElement)[rangeValue] + 1;
				}
			}
		}

		for (auto rowIt = depth_count_map.begin(); rowIt != depth_count_map.end(); rowIt++)
		{
			for (auto elementIt = rowIt->begin(); elementIt != rowIt->end(); elementIt++)
			{
				unsigned int max_count = 0;
				ushort mean_value = 0;

				// calculate mean value
				for (auto depthValueEntry = elementIt->begin(); depthValueEntry != elementIt->end(); depthValueEntry++)
				{
					if (depthValueEntry->second > max_count)
					{
						mean_value = depthValueEntry->first;
						max_count = depthValueEntry->second;
					}
				}

				if (depth_to_noise_map.find(mean_value) == depth_to_noise_map.end())
				{
					depth_to_noise_map.insert(std::pair<ushort, std::map<ushort, unsigned int>>(mean_value, std::map<ushort, unsigned int>()));
				}

				for (auto depthValueEntry = elementIt->begin(); depthValueEntry != elementIt->end(); depthValueEntry++)
				{
					if (depth_to_noise_map[mean_value].find(depthValueEntry->first) == depth_to_noise_map[mean_value].end())
					{
						depth_to_noise_map[mean_value].insert(std::pair<ushort, unsigned int>(depthValueEntry->first, 0));
					}
					depth_to_noise_map[mean_value][depthValueEntry->first] = depth_to_noise_map[mean_value][depthValueEntry->first] + depthValueEntry->second;
				}
			}
		}
	}
	std::cout << std::endl;

	std::map<ushort, double> depth_to_standard_deviation_map;
	for (auto meanValueIt = depth_to_noise_map.begin(); meanValueIt != depth_to_noise_map.end(); meanValueIt++)
	{
		unsigned long long N = 0;
		unsigned long long squared_sum = 0;

		// calculate sigma for standard deviation
		for (auto noiseIt = meanValueIt->second.begin(); noiseIt != meanValueIt->second.end(); noiseIt++)
		{
			for (unsigned int i = 0; i < noiseIt->second; i++)
			{
				squared_sum += (noiseIt->first - meanValueIt->first) * (noiseIt->first - meanValueIt->first);
				N++;
			}
		}

		double standard_deviation = sqrt((1.0 / (double)N) * (double)squared_sum);
		if (depth_to_standard_deviation_map.find(meanValueIt->first) == depth_to_standard_deviation_map.end())
		{
			depth_to_standard_deviation_map.insert(std::pair<ushort, double>(meanValueIt->first, standard_deviation));
		}
	}


	std::vector<double> depthValues;
	std::vector<double> deviationValues;
	unsigned int lastrange = std::numeric_limits<unsigned int>::max();

	std::ofstream csv_file;
	csv_file.open(folderPath + "\\standard_deviation_range_dependent.csv", std::ofstream::out | std::ofstream::trunc);

	std::ofstream datFile;
	datFile.open(folderPath + "\\standard_deviation_range_dependent.dat", std::ofstream::out | std::ofstream::trunc);
	for (auto meanValueIt = depth_to_standard_deviation_map.begin(); meanValueIt != depth_to_standard_deviation_map.end(); meanValueIt++)
	{
		if (meanValueIt->first / g_sumUpCount != lastrange)
		{
			if (csv_file.is_open())
			{
				double depthVal = 0.0;
				for (unsigned int i = 0; i < depthValues.size(); i++)
				{
					double a = 1.0 / (double)(i + 1);
					depthVal = (depthVal * (1.0 - a)) + (depthValues[i] * a);
				}

				double deviationVal = 0.0;
				for (unsigned int i = 0; i < deviationValues.size(); i++)
				{
					double a = 1.0 / (double)(i + 1);
					deviationVal = (deviationVal * (1.0 - a)) + (deviationValues[i] * a);
				}

				if (depthVal > 0 && depthVal < std::numeric_limits<ushort>::max() && deviationVal > 0 && deviationVal < std::numeric_limits<ushort>::max())
				{
					std::string depthString = std::to_string(depthVal);
					std::replace(depthString.begin(), depthString.end(), '.', ',');

					std::string valueString = std::to_string(deviationVal);
					std::replace(valueString.begin(), valueString.end(), '.', ',');

					csv_file << depthString
						<< ";" << valueString
						<< std::endl;
					datFile << std::to_string(depthVal) << " " << std::to_string(deviationVal) << std::endl;
				}
			}

			lastrange = meanValueIt->first / g_sumUpCount;
			depthValues.clear();
			deviationValues.clear();
		}
		else
		{
			depthValues.push_back((double)meanValueIt->first);
			deviationValues.push_back((double)meanValueIt->second);
		}
	}
	csv_file.close();
	datFile.close();
}

void runIntensityDependentAnalysis(const std::vector<bow::DepthFileData>& depthFiles, const std::string& folderPath)
{
	std::map<ushort, std::vector<double>> ir_intensity_to_standard_deviation_map;
	for (unsigned int dirIndex = 0; dirIndex < depthFiles.size(); dirIndex++)
	{
		if (depthFiles[dirIndex].irFiles.size() < depthFiles[dirIndex].depthFiles.size())
		{
			continue;
		}

		std::cout << "Analysing files... (" << std::to_string((unsigned int)(((double)dirIndex / (double)depthFiles.size()) * 100.0)) << "%)\t\r";

		// Step 1: Analyse depthfiles and counting all depth values to calculate mean value
		bool depth_count_map_initialized = false;
		std::vector<std::vector<std::map<ushort, unsigned int>>> depth_count_map;
		std::vector<std::vector<std::map<ushort, unsigned int>>> ir_count_map;
		for (unsigned int fileIndex = 0; fileIndex < depthFiles[dirIndex].depthFiles.size(); fileIndex++)
		{
			cv::Mat_<ushort> deptMap = bow::DataLoader::loadDepthFromFile(depthFiles[dirIndex].depthFiles[fileIndex].filename);
			cv::Mat_<ushort> intensityMap = bow::DataLoader::loadDepthFromFile(depthFiles[dirIndex].irFiles[fileIndex].filename);
			if (!depth_count_map_initialized)
			{
				depth_count_map = std::vector<std::vector<std::map<ushort, unsigned int>>>(deptMap.rows);
				for (unsigned int row_ = 0; row_ < depth_count_map.size(); row_++)
				{
					depth_count_map[row_] = std::vector<std::map<ushort, unsigned int>>(deptMap.cols);
				}

				ir_count_map = std::vector<std::vector<std::map<ushort, unsigned int>>>(intensityMap.rows);
				for (unsigned int row_ = 0; row_ < ir_count_map.size(); row_++)
				{
					ir_count_map[row_] = std::vector<std::map<ushort, unsigned int>>(intensityMap.cols);
				}
				depth_count_map_initialized = true;
			}

			for (unsigned int row = 0; row < deptMap.rows; row++)
			{
				auto depth_mapRow = &(depth_count_map[(unsigned int)(row)]);
				auto ir_mapRow = &(ir_count_map[(unsigned int)(row)]);
				for (unsigned int col = 0; col < deptMap.cols; col++)
				{
					ushort depthValue = deptMap.at<ushort>(row, col);
					ushort intensity = intensityMap.at<ushort>(row, col);

					auto depth_mapElement = &((*depth_mapRow)[(unsigned int)(col)]);
					if (depth_mapElement->find(depthValue) == depth_mapElement->end())
					{
						depth_mapElement->insert(std::pair<ushort, unsigned int>(depthValue, 0));
					}
					(*depth_mapElement)[depthValue] = (*depth_mapElement)[depthValue] + 1;


					auto ir_mapElement = &((*ir_mapRow)[(unsigned int)(col)]);
					if (ir_mapElement->find(intensity) == ir_mapElement->end())
					{
						ir_mapElement->insert(std::pair<ushort, unsigned int>(intensity, 0));
					}
					(*ir_mapElement)[intensity] = (*ir_mapElement)[intensity] + 1;
				}
			}
		}

		cv::Mat_<ushort> mean_ir_intensity;
		{
			unsigned int ir_map_rows = ir_count_map.size();
			unsigned int ir_map_cols = 0;
			if (ir_map_rows > 0)
			{
				ir_map_cols = ir_count_map.front().size();
			}

			mean_ir_intensity = cv::Mat_<ushort>(ir_map_rows, ir_map_cols);
			unsigned int row = 0;
			for (auto rowIt = ir_count_map.begin(); rowIt != ir_count_map.end(); rowIt++, row++)
			{
				unsigned int col = 0;
				for (auto elementIt = rowIt->begin(); elementIt != rowIt->end(); elementIt++, col++)
				{
					unsigned int max_count = 0;
					ushort mean_ir_value = 0;

					// calculate mean value
					for (auto depthValueEntry = elementIt->begin(); depthValueEntry != elementIt->end(); depthValueEntry++)
					{
						if (depthValueEntry->second > max_count)
						{
							mean_ir_value = depthValueEntry->first;
							max_count = depthValueEntry->second;
						}
					}

					mean_ir_intensity.at<ushort>(row, col) = mean_ir_value;
				}
			}
		}

		unsigned int row = 0;
		for (auto rowIt = depth_count_map.begin(); rowIt != depth_count_map.end(); rowIt++, row++)
		{
			unsigned int col = 0;
			for (auto elementIt = rowIt->begin(); elementIt != rowIt->end(); elementIt++, col++)
			{
				ushort ir_intensity = mean_ir_intensity.at<ushort>(row, col);
				

				unsigned int max_count = 0;
				ushort mean_depth_value = 0;
				for (auto noiseIt = elementIt->begin(); noiseIt != elementIt->end(); noiseIt++)
				{
					if (noiseIt->second > max_count)
					{
						mean_depth_value = noiseIt->first;
						max_count = noiseIt->second;
					}
				}

				unsigned long long N = 0;
				unsigned long long squared_sum = 0;

				// calculate sigma for standard deviation
				for (auto noiseIt = elementIt->begin(); noiseIt != elementIt->end(); noiseIt++)
				{
					for (unsigned int i = 0; i < noiseIt->second; i++)
					{
						squared_sum += (noiseIt->first - mean_depth_value) * (noiseIt->first - mean_depth_value);
						N++;
					}
				}

				double standard_deviation = sqrt((1.0 / (double)N) * (double)squared_sum);
				if (ir_intensity_to_standard_deviation_map.find(ir_intensity) == ir_intensity_to_standard_deviation_map.end())
				{
					ir_intensity_to_standard_deviation_map.insert(std::pair<ushort, std::vector<double>>(ir_intensity, std::vector<double>()));
				}
				ir_intensity_to_standard_deviation_map[ir_intensity].push_back(standard_deviation);
			}
		}
	}
	std::cout << std::endl;

	std::vector<double> depthValues;
	std::vector<double> deviationValues;
	unsigned int lastrange = std::numeric_limits<unsigned int>::max();

	std::ofstream csv_file;
	csv_file.open(folderPath + "\\standard_deviation_intensity_dependent.csv", std::ofstream::out | std::ofstream::trunc);

	std::ofstream datFile;
	datFile.open(folderPath + "\\standard_deviation_intensity_dependent.dat", std::ofstream::out | std::ofstream::trunc);

	for (auto meanValueIt = ir_intensity_to_standard_deviation_map.begin(); meanValueIt != ir_intensity_to_standard_deviation_map.end(); meanValueIt++)
	{
		double deviation_sum = 0.0;
		double count = 0;
		for (auto deviationLisIt = meanValueIt->second.begin(); deviationLisIt != meanValueIt->second.end(); deviationLisIt++)
		{
			deviation_sum += *deviationLisIt;
			count += 1.0;
		}

		if ((unsigned int)(std::log2((double)meanValueIt->first) * 2.0) != lastrange)
		{
			if (csv_file.is_open())
			{
				double depthVal = 0.0;
				for (unsigned int i = 0; i < depthValues.size(); i++)
				{
					double a = 1.0 / (double)(i + 1);
					depthVal = (depthVal * (1.0 - a)) + (depthValues[i] * a);
				}

				double deviationVal = 0.0;
				for (unsigned int i = 0; i < deviationValues.size(); i++)
				{
					double a = 1.0 / (double)(i + 1);
					deviationVal = (deviationVal * (1.0 - a)) + (deviationValues[i] * a);
				}

				if (depthVal > 0 && depthVal < std::numeric_limits<ushort>::max() && deviationVal > 0 && deviationVal < std::numeric_limits<ushort>::max())
				{
					std::string depthString = std::to_string(depthVal);
					std::replace(depthString.begin(), depthString.end(), '.', ',');

					std::string valueString = std::to_string(deviationVal);
					std::replace(valueString.begin(), valueString.end(), '.', ',');

					csv_file << depthString
						<< ";" << valueString
						<< std::endl;
					datFile << std::to_string(depthVal) << " " << std::to_string(deviationVal) << std::endl;
				}
			}

			lastrange = (unsigned int)(std::log2((double)meanValueIt->first) * 2.0);
			depthValues.clear();
			deviationValues.clear();
		}
		else
		{
			depthValues.push_back((double)meanValueIt->first);
			deviationValues.push_back((double)deviation_sum / count);
		}
	}
	csv_file.close();
	datFile.close();
}


void runRangeDependentAnalysis_CenterPixelOnly(const std::vector<bow::DepthFileData>& depthFiles, const std::string& folderPath)
{
	std::map<ushort, std::map<ushort, unsigned int>> depth_to_noise_map;
	for (unsigned int dirIndex = 0; dirIndex < depthFiles.size(); dirIndex++)
	{
		std::cout << "Analysing files... (" << std::to_string((unsigned int)(((double)dirIndex / (double)depthFiles.size()) * 100.0)) << "%)\t\r";

		// Step 1: Analyse depthfiles and counting all depth values to calculate mean value
		std::map<ushort, unsigned int> depth_count_map;
		for (unsigned int fileIndex = 0; fileIndex < depthFiles[dirIndex].depthFiles.size(); fileIndex++)
		{
			cv::Mat_<ushort> deptMap = bow::DataLoader::loadDepthFromFile(depthFiles[dirIndex].depthFiles[fileIndex].filename);
			ushort depthValue = deptMap.at<ushort>(deptMap.rows / 2, deptMap.cols / 2);

			if (depth_count_map.find(depthValue) == depth_count_map.end())
			{
				depth_count_map.insert(std::pair<ushort, unsigned int>(depthValue, 0));
			}

			depth_count_map[depthValue] = depth_count_map[depthValue] + 1;
		}


		unsigned int max_count = 0;
		ushort mean_value = 0;

		// calculate mean value
		for (auto depthValueEntry = depth_count_map.begin(); depthValueEntry != depth_count_map.end(); depthValueEntry++)
		{
			if (depthValueEntry->second > max_count)
			{
				mean_value = depthValueEntry->first;
				max_count = depthValueEntry->second;
			}
		}

		if (depth_to_noise_map.find(mean_value) == depth_to_noise_map.end())
		{
			depth_to_noise_map.insert(std::pair<ushort, std::map<ushort, unsigned int>>(mean_value, std::map<ushort, unsigned int>()));
		}

		for (auto depthValueEntry = depth_count_map.begin(); depthValueEntry != depth_count_map.end(); depthValueEntry++)
		{
			if (depth_to_noise_map[mean_value].find(depthValueEntry->first) == depth_to_noise_map[mean_value].end())
			{
				depth_to_noise_map[mean_value].insert(std::pair<ushort, unsigned int>(depthValueEntry->first, 0));
			}
			depth_to_noise_map[mean_value][depthValueEntry->first] = depth_to_noise_map[mean_value][depthValueEntry->first] + depthValueEntry->second;
		}
	}
	std::cout << std::endl;

	std::map<ushort, double> depth_to_standard_deviation_map;
	for (auto meanValueIt = depth_to_noise_map.begin(); meanValueIt != depth_to_noise_map.end(); meanValueIt++)
	{
		unsigned long long N = 0;
		unsigned long long squared_sum = 0;

		// calculate sigma for standard deviation
		for (auto noiseIt = meanValueIt->second.begin(); noiseIt != meanValueIt->second.end(); noiseIt++)
		{
			for (unsigned int i = 0; i < noiseIt->second; i++)
			{
				squared_sum += (noiseIt->first - meanValueIt->first) * (noiseIt->first - meanValueIt->first);
				N++;
			}
		}

		double standard_deviation = sqrt((1.0 / (double)N) * (double)squared_sum);
		if (depth_to_standard_deviation_map.find(meanValueIt->first) == depth_to_standard_deviation_map.end())
		{
			depth_to_standard_deviation_map.insert(std::pair<ushort, double>(meanValueIt->first, standard_deviation));
		}
	}

	std::ofstream csv_file;
	csv_file.open(folderPath + "\\standard_deviation_range_dependent_center.csv", std::ofstream::out | std::ofstream::trunc);

	std::ofstream datFile;
	datFile.open(folderPath + "\\standard_deviation_range_dependent_center.dat", std::ofstream::out | std::ofstream::trunc);

	for (auto meanValueIt = depth_to_standard_deviation_map.begin(); meanValueIt != depth_to_standard_deviation_map.end(); meanValueIt++)
	{
		if (csv_file.is_open())
		{
			if (meanValueIt->first > 0 && meanValueIt->first < std::numeric_limits<ushort>::max() && meanValueIt->second > 0 && meanValueIt->second < std::numeric_limits<ushort>::max())
			{
				std::string value = std::to_string(meanValueIt->second);
				std::replace(value.begin(), value.end(), '.', ',');

				csv_file << meanValueIt->first
					<< ";" << value
					<< std::endl;
				datFile << std::to_string(meanValueIt->first) << " " << std::to_string(meanValueIt->second) << std::endl;
			}
		}
	}
	csv_file.close();
	datFile.close();
}

void runIntensityDependentAnalysis_CenterPixelOnly(const std::vector<bow::DepthFileData>& depthFiles, const std::string& folderPath)
{
	std::map<ushort, std::vector<double>> ir_intensity_to_standard_deviation_map;
	for (unsigned int dirIndex = 0; dirIndex < depthFiles.size(); dirIndex++)
	{
		if (depthFiles[dirIndex].irFiles.size() < depthFiles[dirIndex].depthFiles.size())
		{
			continue;
		}

		std::cout << "Analysing files... (" << std::to_string((unsigned int)(((double)dirIndex / (double)depthFiles.size()) * 100.0)) << "%)\t\r";

		std::map<ushort, unsigned int> ir_count_map;
		std::map<ushort, unsigned int> depth_count_map;
		for (unsigned int fileIndex = 0; fileIndex < depthFiles[dirIndex].depthFiles.size(); fileIndex++)
		{
			cv::Mat_<ushort> intensityMap = bow::DataLoader::loadDepthFromFile(depthFiles[dirIndex].irFiles[fileIndex].filename);
			ushort intensity = intensityMap.at<ushort>(intensityMap.rows / 2, intensityMap.cols / 2);

			if (ir_count_map.find(intensity) == ir_count_map.end())
			{
				ir_count_map.insert(std::pair<ushort, unsigned int>(intensity, 0));
			}
			ir_count_map[intensity] = ir_count_map[intensity] + 1;

			cv::Mat_<ushort> depthMap = bow::DataLoader::loadDepthFromFile(depthFiles[dirIndex].depthFiles[fileIndex].filename);
			ushort depth = depthMap.at<ushort>(depthMap.rows / 2, depthMap.cols / 2);

			if (depth_count_map.find(depth) == depth_count_map.end())
			{
				depth_count_map.insert(std::pair<ushort, unsigned int>(depth, 0));
			}
			depth_count_map[depth] = depth_count_map[depth] + 1;
		}

		unsigned int max_count = 0;
		ushort mean_ir_value = 0;

		// calculate mean value
		for (auto irValueEntry = ir_count_map.begin(); irValueEntry != ir_count_map.end(); irValueEntry++)
		{
			if (irValueEntry->second > max_count)
			{
				mean_ir_value = irValueEntry->first;
				max_count = irValueEntry->second;
			}
		}


		max_count = 0;
		ushort mean_depth_value = 0;
		for (auto noiseIt = depth_count_map.begin(); noiseIt != depth_count_map.end(); noiseIt++)
		{
			if (noiseIt->second > max_count)
			{
				mean_depth_value = noiseIt->first;
				max_count = noiseIt->second;
			}
		}

		unsigned long long N = 0;
		unsigned long long squared_sum = 0;

		// calculate sigma for standard deviation
		for (auto noiseIt = depth_count_map.begin(); noiseIt != depth_count_map.end(); noiseIt++)
		{
			for (unsigned int i = 0; i < noiseIt->second; i++)
			{
				squared_sum += (noiseIt->first - mean_depth_value) * (noiseIt->first - mean_depth_value);
				N++;
			}
		}

		double standard_deviation = sqrt((1.0 / (double)N) * (double)squared_sum);
		if (ir_intensity_to_standard_deviation_map.find(mean_ir_value) == ir_intensity_to_standard_deviation_map.end())
		{
			ir_intensity_to_standard_deviation_map.insert(std::pair<ushort, std::vector<double>>(mean_ir_value, std::vector<double>()));
		}
		ir_intensity_to_standard_deviation_map[mean_ir_value].push_back(standard_deviation);
	}
	std::cout << std::endl;

	std::ofstream csv_file;
	csv_file.open(folderPath + "\\standard_deviation_intensity_dependent_center.csv", std::ofstream::out | std::ofstream::trunc);

	std::ofstream datFile;
	datFile.open(folderPath + "\\standard_deviation_intensity_dependent_center.dat", std::ofstream::out | std::ofstream::trunc);

	for (auto meanValueIt = ir_intensity_to_standard_deviation_map.begin(); meanValueIt != ir_intensity_to_standard_deviation_map.end(); meanValueIt++)
	{
		double deviation_sum = 0.0;
		double count = 0;
		for (auto deviationLisIt = meanValueIt->second.begin(); deviationLisIt != meanValueIt->second.end(); deviationLisIt++)
		{
			deviation_sum += *deviationLisIt;
			count += 1.0;
		}

		if (csv_file.is_open())
		{
			if (meanValueIt->first > 0 && meanValueIt->first < std::numeric_limits<ushort>::max())
			{
				std::string value = std::to_string(deviation_sum / count);
				std::replace(value.begin(), value.end(), '.', ',');

				csv_file << meanValueIt->first
					<< ";" << value
					<< std::endl;
				datFile << std::to_string(meanValueIt->first) << " " << std::to_string(deviation_sum / count) << std::endl;
			}
		}
	}
	csv_file.close();
	datFile.close();
}

void runAnalysis(const std::string& calibrationFilePath, const std::string& recordingsFolderPath)
{
	std::vector<bow::DepthFileData> depthFiles = bow::DataLoader::loadRecordedFilesFromFolder(recordingsFolderPath);

	runRangeDependentAnalysis_CenterPixelOnly(depthFiles, recordingsFolderPath);
	runIntensityDependentAnalysis_CenterPixelOnly(depthFiles, recordingsFolderPath);

	runRangeDependentAnalysis(depthFiles, recordingsFolderPath, calibrationFilePath);
	runIntensityDependentAnalysis(depthFiles, recordingsFolderPath);
}

int main()
{
	runAnalysis("/data/IFM_O3D303_Calibration.xml", "F:\\Kamera_Evaluation\\03D303\\Recordings_Noise");
	runAnalysis("/data/IFM_O3D303_Calibration.xml", "F:\\Kamera_Evaluation\\03D303\\Recordings_Noise_2");
	runAnalysis("/data/Kinect_v2_Calibration.xml", "F:\\Kamera_Evaluation\\Kinect_v2\\Recordings_Noise");
	runAnalysis("/data/Xtion_2_Calibration.xml", "F:\\Kamera_Evaluation\\Xtion_2\\Recordings_Noise");

	return 0;
}