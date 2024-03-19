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

void runAnalysis(const std::string& calibrationFilePath, const std::string& imageFileName)
{
	bow::RenderingConfigs configs = bow::ConfigLoader::loadConfigFromFile(std::string(PROJECT_BASE_DIR) + calibrationFilePath);

	//bow::IntrinsicCameraParameters rgb_intrinisicCameraParameters = bow::CameraCalibration::intrinsicChessboardCalibration(configs.calibration_checkerboard_width, configs.calibration_checkerboard_height, configs.calibration_checkerboard_squareSize, std::string(PROJECT_BASE_DIR) + std::string("/data/") + configs.rgbCameraCheckerboardImagesPath);
	bow::IntrinsicCameraParameters ir_intrinisicCameraParameters = bow::CameraCalibration::intrinsicChessboardCalibration(configs.calibration_checkerboard_width, configs.calibration_checkerboard_height, configs.calibration_checkerboard_squareSize, std::string(PROJECT_BASE_DIR) + std::string("/data/") + configs.irCameraCheckerboardImagesPath);

	std::cout << calibrationFilePath << std::endl;
	std::cout << ir_intrinisicCameraParameters.cameraMatrix << std::endl;
	std::cout << ir_intrinisicCameraParameters.distCoeffs << std::endl;
	std::cout << std::endl;

	cv::Mat screenPoints = cv::Mat(1, ir_intrinisicCameraParameters.image_width * ir_intrinisicCameraParameters.image_height, CV_32FC2);
	for (unsigned int row = 0; row < ir_intrinisicCameraParameters.image_height; row++)
	{
		for (unsigned int col = 0; col < ir_intrinisicCameraParameters.image_width; col++)
		{
			unsigned int i = col + (ir_intrinisicCameraParameters.image_width * row);
			screenPoints.at<cv::Vec2f>(0, i).val[0] = ((float)col / (float)ir_intrinisicCameraParameters.image_width) * (float)ir_intrinisicCameraParameters.image_width;
			screenPoints.at<cv::Vec2f>(0, i).val[1] = ((float)row / (float)ir_intrinisicCameraParameters.image_height) * (float)ir_intrinisicCameraParameters.image_height;
		}
	}

	cv::Mat newCameraMatrix = cv::getOptimalNewCameraMatrix(ir_intrinisicCameraParameters.cameraMatrix, ir_intrinisicCameraParameters.distCoeffs, cv::Size(ir_intrinisicCameraParameters.image_width, ir_intrinisicCameraParameters.image_height), 0);

	cv::Mat undistortedScreenPoints;
	cv::undistortPoints(screenPoints, undistortedScreenPoints, ir_intrinisicCameraParameters.cameraMatrix, ir_intrinisicCameraParameters.distCoeffs, cv::noArray(), newCameraMatrix);

	cv::Mat image = cv::Mat(ir_intrinisicCameraParameters.image_height, ir_intrinisicCameraParameters.image_width, CV_8UC3);
	for (unsigned int row = 0; row < image.rows; row++)
	{
		for (unsigned int col = 0; col < image.cols; col++)
		{
			image.at<cv::Vec3b>(row, col) = cv::Vec3b(255, 255, 255);
		}
	}

	for (unsigned int row = 0; row < ir_intrinisicCameraParameters.image_height; row += 15)
	{
		for (unsigned int col = 0; col < ir_intrinisicCameraParameters.image_width; col += 15)
		{
			unsigned int i = col + (ir_intrinisicCameraParameters.image_width * row);
			if(cv::norm(cv::Vec2f(undistortedScreenPoints.at<cv::Vec2f>(0, i).val[0], undistortedScreenPoints.at<cv::Vec2f>(0, i).val[1]) - cv::Vec2f(screenPoints.at<cv::Vec2f>(0, i).val[0], screenPoints.at<cv::Vec2f>(0, i).val[1])) < 100)
				cv::line(image, cv::Point(screenPoints.at<cv::Vec2f>(0, i).val[0], screenPoints.at<cv::Vec2f>(0, i).val[1]), cv::Point(undistortedScreenPoints.at<cv::Vec2f>(0, i).val[0], undistortedScreenPoints.at<cv::Vec2f>(0, i).val[1]), cv::Scalar(255, 0, 0), 2);
		}
	}
	
	std::cout << ir_intrinisicCameraParameters.cameraMatrix.at<double>(0, 2) << std::endl;
	std::cout << ir_intrinisicCameraParameters.cameraMatrix.at<double>(1, 2) << std::endl;

	cv::circle(image, cv::Point(ir_intrinisicCameraParameters.cameraMatrix.at<double>(0, 2), ir_intrinisicCameraParameters.cameraMatrix.at<double>(1, 2)), 2.0f, cv::Scalar(0, 0, 255), 2);

	cv::imwrite(imageFileName, image);
}

int main()
{
	runAnalysis("/data/IFM_O3D303_Calibration.xml", "IFM_O3D303_Calibration.png");
	runAnalysis("/data/Kinect_v2_Calibration.xml", "Kinect_v2_Calibration.png");
	runAnalysis("/data/Xtion_2_Calibration.xml", "Xtion_2_Calibration.png");

	return 0;
}