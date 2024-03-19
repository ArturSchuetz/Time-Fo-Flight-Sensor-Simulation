#pragma once
#include "EvaluationUtils/EvaluationUtils_api.h"

#include "CoreSystems/BowCoreSystems.h"

//opencv
#include <opencv2/opencv.hpp>

namespace bow {

	struct FrameData{
		long long timestamp;
		std::string filename;
	};

	struct DepthFileData {
		std::string folderName;
		std::vector<FrameData> imageFiles;
		std::vector<FrameData> irFiles;
		std::vector<FrameData> depthFiles;
		std::vector<FrameData> rangeFiles;
	};

	class EVALUATIONUTILS_API DataLoader
	{
	public:
		DataLoader();
		~DataLoader();

		static std::vector<std::string> getDirectoryContent(const std::string& folderPath);
		static std::vector<DepthFileData> loadRecordedFilesFromFolder(const std::string& folderPath);
		static cv::Mat_<ushort> findClosestDepthFile(long long timestamp, std::vector<bow::FrameData> depthFiles);
		static cv::Mat			findClosestImageFile(long long timestamp, std::vector<bow::FrameData> imageFiles);
		static cv::Mat_<ushort> loadDepthFromFile(const std::string& depth_filePath);
	private:

	};
}
