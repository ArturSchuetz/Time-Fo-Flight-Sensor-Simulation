#include "EvaluationUtils/DataLoader.h"

#include <iostream> 
#include <chrono>
#include <thread>
#include <sys/stat.h>
#include <string>

#ifdef __unix__ 
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#elif defined(_WIN32) || defined(WIN32)
#include <Windows.h>
#endif

namespace bow
{
	void getDirectory(const char* d, std::vector<std::string> & f)
	{
#ifdef __unix__ 
		DIR *dp;
		struct dirent *dirp;
		if ((dp = opendir(d)) == NULL) {
			std::cout << "Error(" << errno << ") opening " << d << std::endl;
			return;
		}

		while ((dirp = readdir(dp)) != NULL) {
			f.push_back(std::string(dirp->d_name));
		}
		closedir(dp);
#elif defined(_WIN32) || defined(WIN32)
		FILE* pipe = NULL;
		std::string pCmd = "dir /B " + std::string(d);
		char buf[256];

		if (NULL == (pipe = _popen(pCmd.c_str(), "rt")))
		{
			std::cout << "Shit" << std::endl;
			return;
		}

		while (!feof(pipe))
		{
			if (fgets(buf, 256, pipe) != NULL)
			{
				std::string filepath = std::string(buf);
				if (filepath.length() > 3)
				{
					f.push_back(filepath);
				}
			}
		}

		for (unsigned int i = 0; i < f.size(); i++)
		{
			std::replace(f[i].begin(), f[i].end(), '\\', '/');
			f[i] = f[i].substr(0, f[i].length() - 1);
		}

		_pclose(pipe);
#endif
	}

	bool createDirectory(const std::string& path)
	{
#ifdef __unix__ 
		const int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (-1 == dir_err)
		{
			return false;
		}
		return true;
#endif
#if defined(_WIN32) || defined(WIN32)
		return CreateDirectory(path.c_str(), NULL);
#endif
	}

	bool directoryExists(const std::string& path)
	{
#ifdef __unix__ 
		struct stat statbuf;
		int isDir = 0;
		if (stat(path.c_str(), &statbuf) != -1)
		{
			if (S_ISDIR(statbuf.st_mode))
			{
				isDir = 1;
			}
		}
		return isDir == 1;
#endif
#if defined(_WIN32) || defined(WIN32)
		DWORD ftyp = GetFileAttributesA(path.c_str());
		if (ftyp == INVALID_FILE_ATTRIBUTES)
			return false;  //something is wrong with your path!

		if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
			return true;   // this is a directory!

		return false;    // this is not a directory!
#endif
	}

	bool fileExists(const std::string& name) {
		struct stat buffer;
		return (stat(name.c_str(), &buffer) == 0);
	}

	DataLoader::DataLoader()
	{

	}

	DataLoader::~DataLoader()
	{

	}

	std::vector<std::string> DataLoader::getDirectoryContent(const std::string& folderPath)
	{
		std::vector<std::string> f;
		getDirectory(folderPath.c_str(), f);
		return f;
	}

	std::vector<DepthFileData> DataLoader::loadRecordedFilesFromFolder(const std::string& folderPath)
	{
		std::vector<DepthFileData> result;

		std::string folderPath_copy = folderPath;
		std::replace(folderPath_copy.begin(), folderPath_copy.end(), '/', '\\');

		std::vector<std::string> subdirectories;
		getDirectory(folderPath_copy.c_str(), subdirectories);

		for (unsigned int dirIndex = 0; dirIndex < subdirectories.size(); dirIndex++)
		{
			std::cout << "Loading files... (" << std::to_string((unsigned int)(((double)dirIndex / (double)subdirectories.size()) * 100.0)) << "%)\t\r";

			result.push_back(DepthFileData());
			result.back().folderName = subdirectories[dirIndex];

			std::vector<std::string> filenames;
			getDirectory((folderPath_copy + "\\" + subdirectories[dirIndex]).c_str(), filenames);

			for (unsigned int i = 0; i < filenames.size(); i++)
			{
				if (filenames[i].find("Image_") != std::string::npos)
				{
					FrameData frame;
					std::size_t found = filenames[i].find("Image_");
					std::string fileTimeStamp = filenames[i].substr(found + 6);
					frame.timestamp = std::atoll(fileTimeStamp.c_str());

					frame.filename = (folderPath_copy + "\\" + subdirectories[dirIndex] + "\\" + filenames[i]);
					result.back().imageFiles.push_back(frame);
				}

				if (filenames[i].find("Ir_") != std::string::npos)
				{
					FrameData frame;
					std::size_t found = filenames[i].find("Ir_");
					std::string fileTimeStamp = filenames[i].substr(found + 6);
					frame.timestamp = std::atoll(fileTimeStamp.c_str());

					frame.filename = (folderPath_copy + "\\" + subdirectories[dirIndex] + "\\" + filenames[i]);
					result.back().irFiles.push_back(frame);
				}

				if (filenames[i].find("Range_") != std::string::npos)
				{
					FrameData frame;
					std::size_t found = filenames[i].find("Range_");
					std::string fileTimeStamp = filenames[i].substr(found + 6);
					frame.timestamp = std::atoll(fileTimeStamp.c_str());

					frame.filename = (folderPath_copy + "\\" + subdirectories[dirIndex] + "\\" + filenames[i]);
					result.back().rangeFiles.push_back(frame);
				}

				if (filenames[i].find("Depth_") != std::string::npos)
				{
					FrameData frame;
					std::size_t found = filenames[i].find("Depth_");
					std::string fileTimeStamp = filenames[i].substr(found + 6);
					frame.timestamp = std::atoll(fileTimeStamp.c_str());

					frame.filename = (folderPath_copy + "\\" + subdirectories[dirIndex] + "\\" + filenames[i]);
					result.back().depthFiles.push_back(frame);
				}
			}

			std::sort(result.back().depthFiles.begin(), result.back().depthFiles.end(), [](FrameData a, FrameData b) {return a.timestamp < b.timestamp; });
			std::sort(result.back().imageFiles.begin(), result.back().imageFiles.end(), [](FrameData a, FrameData b) {return a.timestamp < b.timestamp; });
			std::sort(result.back().irFiles.begin(), result.back().irFiles.end(), [](FrameData a, FrameData b) {return a.timestamp < b.timestamp; });
			std::sort(result.back().rangeFiles.begin(), result.back().rangeFiles.end(), [](FrameData a, FrameData b) {return a.timestamp < b.timestamp; });
		}
		std::cout << std::endl;

		return result;
	}

	cv::Mat_<ushort> DataLoader::findClosestDepthFile(long long timestamp, std::vector<bow::FrameData> depthFiles)
	{
		long long closestTimeStamp = 0;
		bow::FrameData* closestDepthFileName = nullptr;

		for (unsigned int i = 0; i < depthFiles.size(); i++)
		{
			if ((timestamp - depthFiles[i].timestamp) * (timestamp - depthFiles[i].timestamp) < (closestTimeStamp - depthFiles[i].timestamp) * (closestTimeStamp - depthFiles[i].timestamp))
			{
				closestTimeStamp = depthFiles[i].timestamp;
				closestDepthFileName = &depthFiles[i];
			}
		}

		if (closestDepthFileName != nullptr)
		{
			if (!closestDepthFileName->filename.empty())
			{
				return bow::DataLoader::loadDepthFromFile(closestDepthFileName->filename);
			}
		}
		return cv::Mat_<ushort>();
	}

	cv::Mat DataLoader::findClosestImageFile(long long timestamp, std::vector<bow::FrameData> imageFiles)
	{
		long long closestTimeStamp = 0;
		bow::FrameData* closestImageFileName = nullptr;

		for (unsigned int i = 0; i < imageFiles.size(); i++)
		{
			if ((timestamp - imageFiles[i].timestamp) * (timestamp - imageFiles[i].timestamp) < (closestTimeStamp - imageFiles[i].timestamp) * (closestTimeStamp - imageFiles[i].timestamp))
			{
				closestTimeStamp = imageFiles[i].timestamp;
				closestImageFileName = &imageFiles[i];
			}
		}

		if (closestImageFileName != nullptr)
		{
			if (!closestImageFileName->filename.empty())
			{
				return cv::imread(closestImageFileName->filename, CV_LOAD_IMAGE_UNCHANGED);
			}
		}
		return cv::Mat();
	}

	cv::Mat_<ushort> DataLoader::loadDepthFromFile(const std::string& depth_filePath)
	{
		cv::Mat_<ushort> depthMat;

		std::ifstream file(depth_filePath, std::ios::in | std::ios::binary | std::ios::ate);
		if (file.is_open())
		{
			std::streampos size = file.tellg();

			if (1280 * 960 * sizeof(unsigned short) == size)
			{
				depthMat = cv::Mat_<ushort>(960, 1280);
			}
			else if (640 * 480 * sizeof(unsigned short) == size)
			{
				depthMat = cv::Mat_<ushort>(480, 640);
			}
			else if (512 * 424 * sizeof(unsigned short) == size)
			{
				depthMat = cv::Mat_<ushort>(424, 512);
			}
			else if (352 * 264 * sizeof(unsigned short) == size)
			{
				depthMat = cv::Mat_<ushort>(264, 352);
			}
			else if (320 * 240 * sizeof(unsigned short) == size)
			{
				depthMat = cv::Mat_<ushort>(240, 320);
			}
			else if (176 * 132 * sizeof(unsigned short) == size)
			{
				depthMat = cv::Mat_<ushort>(132, 176);
			}

			file.seekg(0, std::ios::beg);
			file.read((char*)&(depthMat.data[0]), size);
			file.close();
		}

		return depthMat;
	}
}
