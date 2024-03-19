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

void runAnalysis(const std::string& calibrationFilePath, const std::string& recordingsFolderPath)
{
	bow::RenderingConfigs configs = bow::ConfigLoader::loadConfigFromFile(std::string(PROJECT_BASE_DIR) + calibrationFilePath);
	bow::IntrinsicCameraParameters intrinisicCameraParameters = bow::CameraCalibration::intrinsicChessboardCalibration(configs.calibration_checkerboard_width, configs.calibration_checkerboard_height, configs.calibration_checkerboard_squareSize, std::string(PROJECT_BASE_DIR) + std::string("/data/") + configs.irCameraCheckerboardImagesPath);

	std::vector<bow::DepthFileData> depthFiles = bow::DataLoader::loadRecordedFilesFromFolder(recordingsFolderPath);

	std::map<unsigned int, std::vector<ushort>> depth_to_time_map;
	for (unsigned int dirIndex = 0; dirIndex < depthFiles.size(); dirIndex++)
	{
		cv::Mat_<double> firstMinuteDepthMap;
		cv::Mat_<double> lastMinuteDepthMap;

		unsigned int frameIndex_fistMinute = 0;
		unsigned int frameIndex_lastMinute = 0;
		if (depthFiles[dirIndex].depthFiles.size() > 0)
		{
			long long time_start = depthFiles[dirIndex].depthFiles.front().timestamp;
			long long time_end = depthFiles[dirIndex].depthFiles.back().timestamp;

			std::cout << "Analysing files... (" << std::to_string((unsigned int)(((double)dirIndex / (double)depthFiles.size()) * 100.0)) << "%)\t\r";
			for (unsigned int fileIndex = 0; fileIndex < depthFiles[dirIndex].depthFiles.size(); fileIndex++)
			{
				cv::Mat_<ushort> deptMap = bow::DataLoader::loadDepthFromFile(depthFiles[dirIndex].depthFiles[fileIndex].filename);
				if (depthFiles[dirIndex].depthFiles[fileIndex].timestamp - time_start < 60000)
				{
					if (firstMinuteDepthMap.cols != deptMap.cols || firstMinuteDepthMap.rows != deptMap.rows)
						firstMinuteDepthMap = cv::Mat_<double>(deptMap.rows, deptMap.cols);

					for (unsigned int row = 0; row < deptMap.rows; row++)
					{
						for (unsigned int col = 0; col < deptMap.cols; col++)
						{
							double temp_depth = (double)deptMap.at<ushort>(row, col);
							double a = 1.0f / (double)(frameIndex_fistMinute + 1);
							firstMinuteDepthMap.at<double>(row, col) = (firstMinuteDepthMap.at<double>(row, col) * (1.0 - a)) + (temp_depth * a);
						}
					}
				}
				else if (time_end - depthFiles[dirIndex].depthFiles[fileIndex].timestamp < 60000)
				{
					if (lastMinuteDepthMap.cols != deptMap.cols || lastMinuteDepthMap.rows != deptMap.rows)
						lastMinuteDepthMap = cv::Mat_<double>(deptMap.rows, deptMap.cols);

					for (unsigned int row = 0; row < deptMap.rows; row++)
					{
						for (unsigned int col = 0; col < deptMap.cols; col++)
						{
							double temp_depth = (double)deptMap.at<ushort>(row, col);
							double a = 1.0f / (double)(frameIndex_lastMinute + 1);
							lastMinuteDepthMap.at<double>(row, col) = (lastMinuteDepthMap.at<double>(row, col) * (1.0 - a)) + (temp_depth * a);
						}
					}
				}

				ushort depthValue = deptMap.at<ushort>(deptMap.rows / 2, deptMap.cols / 2);
			
				unsigned int time = depthFiles[dirIndex].depthFiles[fileIndex].timestamp / 1000; // Sum up all depth values for one second
				if (depth_to_time_map.find(time) == depth_to_time_map.end())
				{
					depth_to_time_map.insert(std::pair<unsigned int, std::vector<ushort>>(time, std::vector<ushort>()));
				}
				depth_to_time_map[time].push_back(depthValue);
			}

			const float max_diff_value = 100.0f;
			cv::Mat_<uchar> diff_mat = cv::Mat_<uchar>(firstMinuteDepthMap.rows, firstMinuteDepthMap.cols);
			for (unsigned int row = 0; row < firstMinuteDepthMap.rows; row++)
			{
				for (unsigned int col = 0; col < firstMinuteDepthMap.cols; col++)
				{
					float difference = abs(firstMinuteDepthMap.at<double>(row, col) - lastMinuteDepthMap.at<double>(row, col));
					if (difference < 0)
						difference = -difference;
					if (difference > max_diff_value)
						difference = max_diff_value;
					diff_mat.at<uchar>(row, col) = (uchar)((difference / max_diff_value) * 255.0f);
				}
			}

			cv::imwrite(recordingsFolderPath + "\\" + depthFiles[dirIndex].folderName + "_temperature_error_diffmap.png", diff_mat);

			cv::Mat cm_img0;
			applyColorMap(diff_mat, cm_img0, cv::COLORMAP_HOT);
			cv::imwrite(recordingsFolderPath + "\\" + depthFiles[dirIndex].folderName + "_temperature_error_heatmap.png", cm_img0);

			applyColorMap(diff_mat, cm_img0, cv::COLORMAP_JET);
			cv::imwrite(recordingsFolderPath + "\\" + depthFiles[dirIndex].folderName + "_temperature_error_jetmap.png", cm_img0);
		}
	}

	// ====================================
	// Write results into a csv file
	// ====================================

	std::ofstream csv_file;
	csv_file.open(recordingsFolderPath + "\\teperature_dependent_center.csv", std::ofstream::out | std::ofstream::trunc);

	std::ofstream datFile;
	datFile.open(recordingsFolderPath + "\\teperature_dependent_center.dat", std::ofstream::out | std::ofstream::trunc);

	std::map<ushort, double> median_map;
	for (auto meanValueIt = depth_to_time_map.begin(); meanValueIt != depth_to_time_map.end(); meanValueIt++)
	{
		double sum = 0;
		double count = 0;
		for (unsigned int i = 0; i < meanValueIt->second.size(); i++)
		{
			sum += meanValueIt->second[i];
			count += 1.0;
		}
		double average = sum / count;

		std::sort(meanValueIt->second.begin(), meanValueIt->second.end(), [](ushort left, ushort right) { return left < right; });
		ushort median = meanValueIt->second[meanValueIt->second.size() * 0.5f];

		if (median_map.find(meanValueIt->first) == median_map.end())
		{
			median_map.insert(std::pair<ushort, double>(meanValueIt->first, average));
		}

		if (csv_file.is_open())
		{
			std::string value = std::to_string(average);
			std::replace(value.begin(), value.end(), '.', ',');

			if (meanValueIt->first > 0)
			{
				csv_file << std::to_string(meanValueIt->first)
					<< ";" << value
					<< std::endl;
				datFile << std::to_string(meanValueIt->first) << " " << std::to_string(average) << std::endl;
			}

		}
	}

	csv_file.close();
	datFile.close();


	std::vector<unsigned int> sums_list;
	sums_list.push_back(10);
	sums_list.push_back(20);
	sums_list.push_back(30);
	sums_list.push_back(40);
	sums_list.push_back(50);
	sums_list.push_back(60);
	sums_list.push_back(70);
	sums_list.push_back(80);
	sums_list.push_back(90);
	sums_list.push_back(100);
	sums_list.push_back(110);
	sums_list.push_back(120);
	sums_list.push_back(130);
	sums_list.push_back(140);
	sums_list.push_back(150);
	sums_list.push_back(160);
	sums_list.push_back(170);
	sums_list.push_back(180);
	sums_list.push_back(190);
	sums_list.push_back(200);
	sums_list.push_back(210);
	sums_list.push_back(220);
	sums_list.push_back(230);
	sums_list.push_back(240);
	sums_list.push_back(250);
	for (unsigned int sum_index = 0; sum_index < sums_list.size(); sum_index++)
	{
		unsigned int sum_count = sums_list[sum_index];
		csv_file.open(recordingsFolderPath + "\\summed_teperature_dependent_center_" + std::to_string(sum_count) + ".csv", std::ofstream::out | std::ofstream::trunc);
		datFile.open(recordingsFolderPath + "\\summed_teperature_dependent_center_" + std::to_string(sum_count) + ".dat", std::ofstream::out | std::ofstream::trunc);
		ushort last = 0;
		float depth_sum = 0.0f;
		float depth_sum_count = 0.0f;
		std::vector<ushort> values;
		for (auto arucoDepthIt = median_map.begin(); arucoDepthIt != median_map.end(); arucoDepthIt++)
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
	runAnalysis("/data/IFM_O3D303_Calibration.xml", "F:\\Kamera_Evaluation\\03D303\\Recordings_Temperature");
	runAnalysis("/data/Kinect_v2_Calibration.xml", "F:\\Kamera_Evaluation\\Kinect_v2\\Recordings_Temperature");
	runAnalysis("/data/Xtion_2_Calibration.xml", "F:\\Kamera_Evaluation\\Xtion_2\\Recordings_Temperature");

	return 0;
}