#pragma once
#include "CameraUtils/CameraUtils_api.h"

#include "CoreSystems/BowCoreSystems.h"

//opencv
#include <opencv2/opencv.hpp>

namespace bow {

	struct CAMERAUTILS_API RenderingConfigs
	{
	public:
		RenderingConfigs()
		{
			rgbCameraCheckerboardImagesPath = "";
			irCameraCheckerboardImagesPath = "";

			rgbCameraExtrinsicCheckerboardImageFilePath = "";
			irCameraExtrinsicCheckerboardImageFilePath = "";

			calibration_checkerboard_width = 8;
			calibration_checkerboard_height = 5;
			calibration_checkerboard_squareSize = 36.0f;
		}
		~RenderingConfigs(){}

		std::string rgbCameraCheckerboardImagesPath;
		std::string irCameraCheckerboardImagesPath;

		std::string rgbCameraExtrinsicCheckerboardImageFilePath;
		std::string irCameraExtrinsicCheckerboardImageFilePath;

		int calibration_checkerboard_width;
		int calibration_checkerboard_height;
		float calibration_checkerboard_squareSize;
	};

	class CAMERAUTILS_API ConfigLoader
	{
	public:
		static RenderingConfigs loadConfigFromFile(const std::string& filePath);
	};
}
