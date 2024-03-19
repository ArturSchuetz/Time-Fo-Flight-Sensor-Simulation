#include <OptixUtils/sutil.h>

#include <CameraUtils/BowApplication.h>
#include <CameraUtils/CameraCalibration.h>
#include <CameraUtils/RenderingConfigs.h>

#include <Masterthesis/cuda_config.h>

#include "Application.h"

#include <iostream>
#include <optix.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


optix::Context	g_context;
int				g_width = 320;
int				g_height = 240;

int main(int /*argc*/, char* /*argv[]*/)
{
	bow::RenderingConfigs configs = bow::ConfigLoader::loadConfigFromFile(std::string(PROJECT_BASE_DIR) + std::string("/data/Kinect_v2_Calibration.xml"));
	bow::IntrinsicCameraParameters intrinisicCameraParameters = bow::CameraCalibration::intrinsicChessboardCalibration(configs.calibration_checkerboard_width, configs.calibration_checkerboard_height, configs.calibration_checkerboard_squareSize, std::string(PROJECT_BASE_DIR) + std::string("/data/") + configs.irCameraCheckerboardImagesPath);

	std::cout << std::endl;
	std::cout << "=======================================================================" << std::endl;
	std::cout << "[Controls:]" << std::endl;
	std::cout << "Use [W][A][S][D] to move around the scene." << std::endl;
	std::cout << "Use [SPACE] to move upwards." << std::endl;
	std::cout << "Use [CTRL] to move downwards." << std::endl;
	std::cout << "Use [LEFT SHIFT] to increase moving speed." << std::endl;
	std::cout << "Move the Mouse while pressing the [Right Mousebutton] to look around." << std::endl;
	std::cout << "Use [1][2][3] on Numpad to switch between TOF Types." << std::endl;
	std::cout << "=======================================================================" << std::endl;
	std::cout << std::endl;

	g_width = intrinisicCameraParameters.image_width;
	g_height = intrinisicCameraParameters.image_height;
	try
	{
		Time_of_Flight_App app;
		app.Run(intrinisicCameraParameters);
	} SUTIL_CATCH(g_context->get())

	return 0;
}
