#include "CameraUtils/RenderingConfigs.h"

namespace bow
{
	RenderingConfigs ConfigLoader::loadConfigFromFile(const std::string& filePath)
	{
		RenderingConfigs config;
		cv::FileStorage loadFileStorage = cv::FileStorage(filePath, cv::FileStorage::READ);
		if (loadFileStorage.isOpened())
		{
			loadFileStorage["rgbCameraCheckerboardImagesPath"] >> config.rgbCameraCheckerboardImagesPath;
			loadFileStorage["irCameraCheckerboardImagesPath"] >> config.irCameraCheckerboardImagesPath;

			loadFileStorage["rgbCameraExtrinsicCheckerboardImageFilePath"] >> config.rgbCameraExtrinsicCheckerboardImageFilePath;
			loadFileStorage["irCameraExtrinsicCheckerboardImageFilePath"] >> config.irCameraExtrinsicCheckerboardImageFilePath;

			loadFileStorage["calibration_checkerboard_width"] >> config.calibration_checkerboard_width;
			loadFileStorage["calibration_checkerboard_height"] >> config.calibration_checkerboard_height;
			loadFileStorage["calibration_checkerboard_squareSize"] >> config.calibration_checkerboard_squareSize;
			loadFileStorage.release();
		}

		cv::FileStorage saveFileStorage(filePath, cv::FileStorage::WRITE);
		if (saveFileStorage.isOpened())
		{
			saveFileStorage << "rgbCameraCheckerboardImagesPath" << config.rgbCameraCheckerboardImagesPath;
			saveFileStorage << "irCameraCheckerboardImagesPath" << config.irCameraCheckerboardImagesPath;

			saveFileStorage << "rgbCameraExtrinsicCheckerboardImageFilePath" << config.rgbCameraExtrinsicCheckerboardImageFilePath;
			saveFileStorage << "irCameraExtrinsicCheckerboardImageFilePath" << config.irCameraExtrinsicCheckerboardImageFilePath;

			saveFileStorage << "calibration_checkerboard_width" << config.calibration_checkerboard_width;
			saveFileStorage << "calibration_checkerboard_height" << config.calibration_checkerboard_height;
			saveFileStorage << "calibration_checkerboard_squareSize" << config.calibration_checkerboard_squareSize;
			saveFileStorage.release();
		}

		return config;
	}
}
