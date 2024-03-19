#include <RenderDevice/BowRenderer.h>
#include <CoreSystems/BowCoreSystems.h>
#include <CoreSystems/BowBasicTimer.h>

#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>

// ######################################################################################
// simple image rendering Shader
// ######################################################################################

const std::string vertexShader = R"(#version 130

in vec3 in_Position;
in vec2 in_TexCoord;

out vec2 var_TextureCoord;

void main(void)
{
    gl_Position = vec4(in_Position, 1.0);
    var_TextureCoord = in_TexCoord;
}
)";

const std::string fragmentShader = R"(#version 130
precision highp float; // needed only for version 1.30

in vec2 var_TextureCoord;
uniform sampler2D inputTexture;

out vec3 out_Color;

void main(void)
{
    out_Color = texture(inputTexture,  vec2(var_TextureCoord.x, var_TextureCoord.y)).rgb;
}
)";

int main()
{
	cv::VideoCapture cap(0); // open the default camera
	if (!cap.isOpened())  // check if we succeeded
		return -1;

	cap.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 1024);

	std::cout << "\n=================================================================" << std::endl;
	std::cout << "CAMERA INFORMATIONS =============================================" << std::endl;
	std::cout << "=================================================================" << std::endl;
	std::cout << "POS_MSEC " << std::to_string(cap.get(CV_CAP_PROP_POS_MSEC)) << std::endl;
	std::cout << "POS_FRAMES " << std::to_string(cap.get(CV_CAP_PROP_POS_FRAMES)) << std::endl;
	std::cout << "POS_AVI_RATIO " << std::to_string(cap.get(CV_CAP_PROP_POS_AVI_RATIO)) << std::endl;
	std::cout << "FRAME_WIDTH " << std::to_string(cap.get(CV_CAP_PROP_FRAME_WIDTH)) << std::endl;
	std::cout << "FRAME_HEIGHT " << std::to_string(cap.get(CV_CAP_PROP_FRAME_HEIGHT)) << std::endl;
	std::cout << "FPS " << std::to_string(cap.get(CV_CAP_PROP_FPS)) << std::endl;
	std::cout << "FRAME_COUNT " << std::to_string(cap.get(CV_CAP_PROP_FRAME_COUNT)) << std::endl;
	std::cout << "FORMAT " << std::to_string(cap.get(CV_CAP_PROP_FORMAT)) << std::endl;
	std::cout << "MODE " << std::to_string(cap.get(CV_CAP_PROP_MODE)) << std::endl;
	std::cout << "BRIGHTNESS " << std::to_string(cap.get(CV_CAP_PROP_BRIGHTNESS)) << std::endl;
	std::cout << "CONTRAST " << std::to_string(cap.get(CV_CAP_PROP_CONTRAST)) << std::endl;
	std::cout << "SATURATION " << std::to_string(cap.get(CV_CAP_PROP_SATURATION)) << std::endl;
	std::cout << "HUE " << std::to_string(cap.get(CV_CAP_PROP_HUE)) << std::endl;
	std::cout << "GAIN " << std::to_string(cap.get(CV_CAP_PROP_GAIN)) << std::endl;
	std::cout << "EXPOSURE " << std::to_string(cap.get(CV_CAP_PROP_EXPOSURE)) << std::endl;
	std::cout << "CONVERT_RGB " << std::to_string(cap.get(CV_CAP_PROP_CONVERT_RGB)) << std::endl;
	std::cout << "RECTIFICATION " << std::to_string(cap.get(CV_CAP_PROP_RECTIFICATION)) << std::endl;
	std::cout << "MONOCHROME " << std::to_string(cap.get(CV_CAP_PROP_MONOCHROME)) << std::endl;
	std::cout << "SHARPNESS " << std::to_string(cap.get(CV_CAP_PROP_SHARPNESS)) << std::endl;
	std::cout << "AUTO_EXPOSURE   " << std::to_string(cap.get(CV_CAP_PROP_AUTO_EXPOSURE)) << std::endl;
	std::cout << "GAMMA " << std::to_string(cap.get(CV_CAP_PROP_GAMMA)) << std::endl;
	std::cout << "TEMPERATURE " << std::to_string(cap.get(CV_CAP_PROP_TEMPERATURE)) << std::endl;
	std::cout << "TRIGGER " << std::to_string(cap.get(CV_CAP_PROP_TRIGGER)) << std::endl;
	std::cout << "TRIGGER_DELAY " << std::to_string(cap.get(CV_CAP_PROP_TRIGGER_DELAY)) << std::endl;
	std::cout << "WHITE_BALANCE_BLUE_U " << std::to_string(cap.get(CV_CAP_PROP_WHITE_BALANCE_BLUE_U)) << std::endl;
	std::cout << "WHITE_BALANCE_RED_V " << std::to_string(cap.get(CV_CAP_PROP_WHITE_BALANCE_RED_V)) << std::endl;
	std::cout << "ZOOM " << std::to_string(cap.get(CV_CAP_PROP_ZOOM)) << std::endl;
	std::cout << "FOCUS " << std::to_string(cap.get(CV_CAP_PROP_FOCUS)) << std::endl;
	std::cout << "GUID " << std::to_string(cap.get(CV_CAP_PROP_GUID)) << std::endl;
	std::cout << "ISO_SPEED " << std::to_string(cap.get(CV_CAP_PROP_ISO_SPEED)) << std::endl;
	std::cout << "BACKLIGHT " << std::to_string(cap.get(CV_CAP_PROP_BACKLIGHT)) << std::endl;
	std::cout << "PAN " << std::to_string(cap.get(CV_CAP_PROP_PAN)) << std::endl;
	std::cout << "TILT " << std::to_string(cap.get(CV_CAP_PROP_TILT)) << std::endl;
	std::cout << "ROLL " << std::to_string(cap.get(CV_CAP_PROP_ROLL)) << std::endl;
	std::cout << "IRIS " << std::to_string(cap.get(CV_CAP_PROP_IRIS)) << std::endl;
	std::cout << "SETTINGS " << std::to_string(cap.get(CV_CAP_PROP_SETTINGS)) << std::endl;
	std::cout << "BUFFERSIZE " << std::to_string(cap.get(CV_CAP_PROP_BUFFERSIZE)) << std::endl;
	std::cout << "AUTOFOCUS " << std::to_string(cap.get(CV_CAP_PROP_AUTOFOCUS)) << std::endl;
	std::cout << "SAR_NUM " << std::to_string(cap.get(CV_CAP_PROP_SAR_NUM)) << std::endl;
	std::cout << "SAR_DEN " << std::to_string(cap.get(CV_CAP_PROP_SAR_DEN)) << std::endl;
	std::cout << "=================================================================\n" << std::endl;

	///////////////////////////////////////////////////////////////////
	// ARUCO STUFF
	///////////////////////////////////////////////////////////////////

	//cv::Mat_<cv::Vec3b> CharucoBoardImage = cv::imread("D:\\Projects\\masterthesis\\data\\CharucoImage.jpg", cv::IMREAD_UNCHANGED);

	cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
	cv::Ptr<cv::aruco::GridBoard> board = cv::aruco::GridBoard::create(5, 6, 0.04, 0.01, dictionary);

	cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
	parameters->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;

	///////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////

	bow::RenderDeviceAPI deviceApi = bow::RenderDeviceAPI::OpenGL3x;

	// Creating Render Device
	bow::RenderDevicePtr deviceOGL = bow::RenderDeviceManager::GetInstance().GetOrCreateDevice(deviceApi);
	if (deviceOGL == nullptr)
	{
		std::cout << "Could not create device!" << std::endl;
		return 1;
	}

	// Creating Window
	bow::GraphicsWindowPtr windowOGL = deviceOGL->VCreateWindow(1280, 1024, "Triangle Sample", bow::WindowType::Windowed);
	if (windowOGL == nullptr)
	{
		std::cout << "Could not create window!" << std::endl;
		return 1;
	}

	bow::RenderContextPtr contextOGL = windowOGL->VGetContext();
	bow::ShaderProgramPtr shaderProgram = deviceOGL->VCreateShaderProgram(vertexShader, fragmentShader);

	///////////////////////////////////////////////////////////////////
	// ClearState and Color

	bow::ClearState clearState;
	clearState.color = bow::ColorRGBA(0.392f, 0.584f, 0.929f, 1.0f);

	///////////////////////////////////////////////////////////////////
	// Vertex Array

	float aspectRatio = static_cast<float>(800) / static_cast<float>(600);

	///////////////////////////////////////////////////////////////////
	// Define Full Screen Triangle

	bow::MeshAttribute triangleMesh;
	{
		// Add Positions
		bow::VertexAttributeFloatVec3 *positionsAttribute = new bow::VertexAttributeFloatVec3("in_Position", 3);
		positionsAttribute->Values[0] = bow::Vector3<float>(-1.0f, 1.0f, 0.0f);
		positionsAttribute->Values[1] = bow::Vector3<float>(3.0f, 1.0f, 0.0f);
		positionsAttribute->Values[2] = bow::Vector3<float>(-1.0f, -3.0f, 0.0f);

		triangleMesh.AddAttribute(bow::VertexAttributePtr(positionsAttribute));

		// Add TextureCoordinates
		bow::VertexAttributeFloatVec2 *texCoordAttribute = new bow::VertexAttributeFloatVec2("in_TexCoord", 3);
		texCoordAttribute->Values[0] = bow::Vector2<float>(0.0f, 0.0f);
		texCoordAttribute->Values[1] = bow::Vector2<float>(2.0f, 0.0f);
		texCoordAttribute->Values[2] = bow::Vector2<float>(0.0f, 2.0f);

		triangleMesh.AddAttribute(bow::VertexAttributePtr(texCoordAttribute));
	}
	bow::VertexArrayPtr fullScreenTriangleVertexArray = contextOGL->VCreateVertexArray(triangleMesh, shaderProgram->VGetVertexAttributes(), bow::BufferHint::StaticDraw);
	
	///////////////////////////////////////////////////////////////////
	// Texture for Image Rendering 

	bow::Texture2DPtr inputTexture = nullptr;// = deviceOGL->VCreateTexture2D(bow::Texture2DDescription(CharucoBoardImage.cols, CharucoBoardImage.rows, bow::TextureFormat::RedGreenBlue8, false));
	bow::TextureSamplerPtr sampler = deviceOGL->VCreateTexture2DSampler(bow::TextureMinificationFilter::Linear, bow::TextureMagnificationFilter::Linear, bow::TextureWrap::Clamp, bow::TextureWrap::Clamp);

	///////////////////////////////////////////////////////////////////
	// RenderState

	bow::RenderState renderState;
	renderState.faceCulling.Enabled = false;
	renderState.depthTest.Enabled = false;

	static const int TexID = 0;
	contextOGL->VSetTextureSampler(TexID, sampler);

	cv::Mat frame;
	std::vector<int> markerIds;
	std::vector<std::vector<cv::Point2f>> markerCorners;
	std::vector<std::vector<cv::Point2f>> rejectedCandidates;

	while (!windowOGL->VShouldClose())
	{
		if (cap.read(frame))
		{
			if ((inputTexture == nullptr) || (frame.cols != inputTexture->VGetDescription().GetWidth() || frame.rows != inputTexture->VGetDescription().GetHeight()))
			{
				inputTexture = deviceOGL->VCreateTexture2D(bow::Texture2DDescription(frame.cols, frame.rows, bow::TextureFormat::RedGreenBlue8, false));
				contextOGL->VSetTexture(TexID, inputTexture);
			}

			cv::aruco::detectMarkers(frame, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);
			cv::aruco::drawDetectedMarkers(frame, markerCorners, markerIds);

			inputTexture->VCopyFromSystemMemory((char*)(&frame.data[0]), frame.cols, frame.rows, bow::ImageFormat::RedGreenBlue, bow::ImageDatatype::UnsignedByte);

			// Clear Backbuffer to our ClearState
			contextOGL->VClear(clearState);

			contextOGL->VSetViewport(bow::Viewport(0, 0, windowOGL->VGetWidth(), windowOGL->VGetHeight()));

			contextOGL->VDraw(bow::PrimitiveType::Triangles, fullScreenTriangleVertexArray, shaderProgram, renderState);

			contextOGL->VSwapBuffers();
		}
	}

	return 0;
}