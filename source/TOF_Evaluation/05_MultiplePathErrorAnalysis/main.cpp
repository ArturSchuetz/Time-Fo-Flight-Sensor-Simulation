#include <RenderDevice/BowRenderer.h>
#include <CoreSystems/BowCoreSystems.h>
#include <Resources/BowResources.h>
#include <CoreSystems/BowBasicTimer.h>

#include <CameraUtils/CameraCalibration.h>
#include <CameraUtils/RenderingConfigs.h>
#include <CameraUtils/PCLRenderer.h>
#include <EvaluationUtils/ArucoHelper.h>
#include <EvaluationUtils/DataLoader.h>

#include <Masterthesis/cuda_config.h>

#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>

bow::PCLRenderer g_renderer;

void loadGroundTruth(const std::string& pointCloudFilePath, const std::string& markerMapPath)
{
	std::vector<bow::Plane<float>> planes;
	std::vector<bow::MarkerDescription> markerMap = bow::ArucoHelper::LoadMarkerMapFromFile(std::string(PROJECT_BASE_DIR) + markerMapPath);
	for (unsigned int i = 0; i < markerMap.size(); i++)
	{
		if (markerMap[i].id != 37)
		{
			bow::Plane<float> newPlane;
			newPlane.Set(bow::Vector3<float>(markerMap[i].front.val[0], markerMap[i].front.val[1], markerMap[i].front.val[2]), bow::Vector3<float>(markerMap[i].center.val[0], markerMap[i].center.val[1], markerMap[i].center.val[2]));
			planes.push_back(newPlane);
		}
	}

	///////////////////////////////////////////////////////////////////
	// Load matched point cloud

	std::vector<bow::Vector3<float>> reference_vertices;
	std::vector<bow::Vector3<float>> reference_colors;
	std::vector<bow::Vector3<float>> reference_normals;
	{
		std::vector<std::string> pointCloudFiles = bow::DataLoader::getDirectoryContent(pointCloudFilePath.c_str());
		unsigned int nLoadPointClouds = pointCloudFiles.size();
		{
			std::streampos size;
			for (unsigned int i = 0; i < nLoadPointClouds; i++)
			{
				if (pointCloudFiles[i].find(".bin") != std::string::npos || pointCloudFiles[i].find(".xtc") != std::string::npos || pointCloudFiles[i].find(".xyz") != std::string::npos || pointCloudFiles[i].find(".xcn") != std::string::npos)
				{
					bow::PointCloudPtr pointCloud = bow::PointCloudManager::GetInstance().Load(pointCloudFilePath + "\\" + pointCloudFiles[i]);
					if (pointCloud != nullptr)
					{
						auto vertices = pointCloud->GetVertices();
						auto colors = pointCloud->GetColors();
						auto normals = pointCloud->GetNormals();
						pointCloud->VUnload();

						float move_along_normal = 0.0f;
						for (unsigned int j = 0; j < vertices.size(); j++)
						{
							bow::Ray<float> ray1 = bow::Ray<float>(vertices[j], normals[j]);
							bow::Ray<float> ray2 = bow::Ray<float>(vertices[j], -normals[j]);

							float smallest_distance = std::numeric_limits<float>::max();
							for (unsigned int k = 0; k < planes.size(); k++)
							{
								float length;
								bow::Vector3<float> hit;
								if (ray1.Intersects(planes[k], false, &length, &hit))
								{
									if ((ray1.origin - hit).Length() < smallest_distance)
									{
										smallest_distance = (ray1.origin - hit).Length();
										move_along_normal = smallest_distance;
									}
								}

								if (ray2.Intersects(planes[k], false, &length, &hit))
								{
									if ((ray2.origin - hit).Length() < smallest_distance)
									{
										smallest_distance = (ray2.origin - hit).Length();
										move_along_normal = -smallest_distance;
									}
								}
							}

							if (smallest_distance < 15.0)
							{
								reference_vertices.push_back(vertices[j] + (normals[j] * move_along_normal));
								reference_colors.push_back(colors[j]);
								reference_normals.push_back(normals[j]);
							}
						}
					}
				}
			}
		}
	}

	g_renderer.UpdateReferencePointCloud(reference_vertices, reference_colors, reference_normals);
}

void runAnalysis(const std::string& calibrationFilePath, const std::string& recordingsFolderPath, const std::string& markerMapPath)
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

	// ==============================================================
	// Load Marker Maps
	// ==============================================================

	std::vector<bow::MarkerDescription> markerMap = bow::ArucoHelper::LoadMarkerMapFromFile(std::string(PROJECT_BASE_DIR) + markerMapPath);

	// ==============================================================
	// Find marker in images and estimate camera position
	// ==============================================================

	unsigned int lastPercentage = 0;

	std::map<ushort, std::map<ushort, unsigned int>> arucoDepth_to_tofDepth_map;
	std::vector<bow::DepthFileData> recordedFiles = bow::DataLoader::loadRecordedFilesFromFolder(recordingsFolderPath);
	for (unsigned int dirIndex = 0; dirIndex < recordedFiles.size(); dirIndex++)
	{
		cv::Mat_<double> mean_depthMat;
		cv::Mat_<cv::Vec3b> mean_colorMat;

		if (recordedFiles[dirIndex].imageFiles.size() > 0 && recordedFiles[dirIndex].depthFiles.size() > 0)
		{
			for (unsigned int frameIndex = 0; frameIndex < recordedFiles[dirIndex].imageFiles.size(); frameIndex++)
			{
				unsigned int progress = (unsigned int)(((double)frameIndex / (double)recordedFiles[dirIndex].imageFiles.size()) * 100.0);
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

				cv::Mat colorMat = cv::imread(recordedFiles[dirIndex].imageFiles[frameIndex].filename, CV_LOAD_IMAGE_UNCHANGED);
				if (colorMat.cols == 0 || colorMat.rows == 0)
					continue;

				cv::undistort(colorMat, undistortedColorMat, rgb_intrinisicCameraParameters.cameraMatrix, rgb_intrinisicCameraParameters.distCoeffs);

				cv::Mat_<ushort> depthMat = bow::DataLoader::findClosestDepthFile(recordedFiles[dirIndex].imageFiles[frameIndex].timestamp, recordedFiles[dirIndex].depthFiles);
				if (depthMat.cols == 0 || depthMat.rows == 0)
					continue;

				// ====================================
				// calculate mean color and depth map do reduce noise
				// ====================================

				if (mean_colorMat.cols != colorMat.cols || mean_colorMat.rows != colorMat.rows)
					mean_colorMat = cv::Mat_<cv::Vec3b>::zeros(undistortedColorMat.rows, undistortedColorMat.cols);

				for (unsigned int row = 0; row < undistortedColorMat.rows; row++)
				{
					for (unsigned int col = 0; col < undistortedColorMat.cols; col++)
					{
						cv::Vec3b temp_color = undistortedColorMat.at<cv::Vec3b>(row, col);
						double a = 1.0f / (double)(frameIndex + 1);
						mean_colorMat.at<cv::Vec3b>(row, col) = (mean_colorMat.at<cv::Vec3b>(row, col) * (1.0 - a)) + (temp_color * a);
					}
				}

				if (mean_depthMat.cols != depthMat.cols || mean_depthMat.rows != depthMat.rows)
					mean_depthMat = cv::Mat_<double>(depthMat.rows, depthMat.cols);

				for (unsigned int row = 0; row < depthMat.rows; row++)
				{
					for (unsigned int col = 0; col < depthMat.cols; col++)
					{
						double temp_depth = (double)depthMat.at<ushort>(row, col);
						double a = 1.0f / (double)(frameIndex + 1);
						mean_depthMat.at<double>(row, col) = (mean_depthMat.at<double>(row, col) * (1.0 - a)) + (temp_depth * a);
					}
				}
			}
		}

		// ====================================
		// type conversion from double to ushort
		// ====================================

		cv::Mat_<ushort> depthMat(mean_depthMat.rows, mean_depthMat.cols);
		for (unsigned int row = 0; row < mean_depthMat.rows; row++)
		{
			for (unsigned int col = 0; col < mean_depthMat.cols; col++)
			{
				depthMat.at<ushort>(row, col) = (ushort)mean_depthMat.at<double>(row, col);
			}
		}

		// ====================================
		// calculate difference from reference
		// ====================================

		if (mean_colorMat.cols > 0 && mean_colorMat.rows > 0)
		{
			cv::Mat_<cv::Vec4f> distorted_directionMatrix = bow::CameraCalibration::calculate_directionMatrix(ir_intrinisicCameraParameters, ir_intrinisicCameraParameters.image_width, ir_intrinisicCameraParameters.image_height);
			
			cv::Matx<float, 4, 4> global_Transform;
			cv::Mat empty_DistCoeffs = cv::Mat::zeros(4, 1, CV_32F);
			std::vector<bow::Marker> detectedMarker = bow::ArucoHelper::detectMarker(mean_colorMat, rgb_intrinisicCameraParameters.cameraMatrix, empty_DistCoeffs, markerMap[0].sidelengthInMM);
			if (bow::ArucoHelper::getTransformationFromMarkerMap(markerMap, detectedMarker, global_Transform))
			{
				cv::Mat_<cv::Vec3f> coordinates = bow::CameraCalibration::calculate_coordinates_from_depth(distorted_directionMatrix, depthMat);
				std::vector<bow::Vector3<float>> colors(coordinates.cols * coordinates.rows);
				std::vector<bow::Vector3<float>> points(coordinates.cols * coordinates.rows);

				// ====================================
				// transform points into world space
				// ====================================

				for (unsigned int row = 0; row < coordinates.rows; row++)
				{
					for (unsigned int col = 0; col < coordinates.cols; col++)
					{
						unsigned int index = col + (row * coordinates.cols);

						cv::Vec3f coordinate = coordinates.at<cv::Vec3f>(row, col);
						cv::Mat transformedCoord = cv::Mat(global_Transform) * irToRgbCameraViewMatrix * cv::Mat(cv::Vec4f(coordinate.val[0], coordinate.val[1], coordinate.val[2], 1.0f));

						colors[index] = bow::Vector3<float>(0.5f, 0.5f, 0.5f);
						points[index] = bow::Vector3<float>(transformedCoord.at<float>(0, 0), transformedCoord.at<float>(1, 0), transformedCoord.at<float>(2, 0));
					}
				}

				// ========================================================================
				// calculate view and projection matrix to render reference scene from camera view
				// ========================================================================

				cv::Mat projMat = ir_intrinisicCameraParameters.cameraProjectionMatrix;
				bow::Matrix4x4<float> _projMat;
				for (unsigned matIndex = 0; matIndex < 4 * 4; matIndex++)
				{
					_projMat.a[matIndex] = ((float*)projMat.data)[matIndex];
				}

				cv::Mat z_flipMat = cv::Mat::zeros(4, 4, CV_32FC1);
				z_flipMat.at<float>(0, 0) = 1.0f;
				z_flipMat.at<float>(1, 1) = 1.0f;
				z_flipMat.at<float>(2, 2) = -1.0f;
				z_flipMat.at<float>(3, 3) = 1.0f;

				cv::Mat inverted_transform;
				cv::invert(cv::Mat(global_Transform), inverted_transform);
				cv::Mat worldView = z_flipMat * cv::Mat(rgbToIrCameraViewMatrix) * cv::Mat(inverted_transform);
				bow::Matrix3D<float> _viewMat;
				for (unsigned matIndex = 0; matIndex < 4 * 4; matIndex++)
				{
					_viewMat.a[matIndex] = ((float*)worldView.data)[matIndex];
				}
				
				// ========================================================================
				// rendering reference from camere view and calculate difference map
				// ========================================================================

				std::vector<ushort> undistorted_ref_depth = g_renderer.RenderRefDepthFromPerspective(ir_intrinisicCameraParameters.image_width , ir_intrinisicCameraParameters.image_height, _viewMat, _projMat);
				cv::Mat_<ushort> undistorted_ref_depth_mat(ir_intrinisicCameraParameters.image_height, ir_intrinisicCameraParameters.image_width);

				memcpy(undistorted_ref_depth_mat.data, &undistorted_ref_depth[0], undistorted_ref_depth_mat.rows * undistorted_ref_depth_mat.cols * sizeof(ushort));
				cv::flip(undistorted_ref_depth_mat, undistorted_ref_depth_mat, 0);

				cv::Mat_<ushort> undistorted_depth;
				cv::undistort(depthMat, undistorted_depth, ir_intrinisicCameraParameters.cameraMatrix, ir_intrinisicCameraParameters.distCoeffs);
				
				const float max_diff_value = 100.0f;
				cv::Mat_<uchar> diff_mat = cv::Mat_<uchar>(undistorted_depth.rows, undistorted_depth.cols);
				for (unsigned int i = 0; i < undistorted_depth.rows * undistorted_depth.cols; i++)
				{
					float tof = (float)undistorted_depth.at<ushort>(i);
					float sl = (float)undistorted_ref_depth_mat.at<ushort>(i);
					if (sl < 10.0f || tof < 10.0f)
					{
						diff_mat.at<uchar>(i) = 0.0;
					}
					else
					{
						float difference = abs(tof - sl);
						if (difference > max_diff_value)
							difference = max_diff_value;
						diff_mat.at<uchar>(i) = (uchar)((difference / max_diff_value) * 255.0f);
					}
				}

				// ========================================================================
				// create color maps and save results on hard disk
				// ========================================================================

				cv::imwrite(recordingsFolderPath + "\\multi_path_error_diffmap.png", diff_mat);

				cv::Mat cm_img0;
				applyColorMap(diff_mat, cm_img0, cv::COLORMAP_HOT);
				cv::imwrite(recordingsFolderPath + "\\multi_path_error_heatmap.png", cm_img0);

				applyColorMap(diff_mat, cm_img0, cv::COLORMAP_JET);
				cv::imwrite(recordingsFolderPath + "\\multi_path_error_jetmap.png", cm_img0);

				g_renderer.UpdateColors(colors);
				g_renderer.UpdatePointCloud(points);

				cv::Mat_<cv::Vec4f> undistorted_directionMatrix = bow::CameraCalibration::calculate_directionMatrix(ir_intrinisicCameraParameters, ir_intrinisicCameraParameters.image_width, ir_intrinisicCameraParameters.image_height, false);
				cv::Mat_<cv::Vec3f> tof_coordinates = bow::CameraCalibration::calculate_coordinates_from_depth(undistorted_directionMatrix, undistorted_depth);
				cv::Mat_<cv::Vec3f> ref_coordinates = bow::CameraCalibration::calculate_coordinates_from_depth(undistorted_directionMatrix, undistorted_ref_depth_mat);

				std::ofstream csv_file;
				csv_file.open(recordingsFolderPath + "\\multi_path_error_" + recordedFiles[dirIndex].folderName + ".csv", std::ofstream::out | std::ofstream::trunc);

				std::ofstream depth_datFile;
				depth_datFile.open(recordingsFolderPath + "\\multi_path_error_" + recordedFiles[dirIndex].folderName + "_depth.dat", std::ofstream::out | std::ofstream::trunc);

				std::ofstream ref_datFile;
				ref_datFile.open(recordingsFolderPath + "\\multi_path_error_" + recordedFiles[dirIndex].folderName + "_ref.dat", std::ofstream::out | std::ofstream::trunc);

				unsigned int row = undistorted_depth.rows / 2;
				for (unsigned int col = 0; col < undistorted_depth.cols; col++)
				{
					cv::Vec3f depthVal = tof_coordinates.at<cv::Vec3f>(row, col);
					cv::Vec3f depthRefVal = ref_coordinates.at<cv::Vec3f>(row, col);
					if (depthVal.val[2] > 0.0 && depthVal.val[2] < 1500)
					{
						if (csv_file.is_open())
						{
							csv_file << depthRefVal.val[0]
								<< ";" << depthVal.val[2]
								<< ";" << depthRefVal.val[2]
								<< std::endl;
							depth_datFile << std::to_string(depthVal.val[0]) << " " << std::to_string(depthVal.val[2]) << std::endl;
							ref_datFile << std::to_string(depthRefVal.val[0]) << " " << std::to_string(depthRefVal.val[2]) << std::endl;
						}
					}
				}
				csv_file.close();
				depth_datFile.close();
				ref_datFile.close();
			}
		}
	}
}

int main()
{
	g_renderer.Start();
	loadGroundTruth("F:\\Kamera_Evaluation\\Xtion_1\\MatchedOutputClouds_Multiple_Path", "/data/map_multiple_path.yml");
	runAnalysis("/data/IFM_O3D303_Calibration.xml", "F:\\Kamera_Evaluation\\03D303\\Recordings_Multiple_Path", "/data/map_multiple_path.yml");
	runAnalysis("/data/Kinect_v2_Calibration.xml", "F:\\Kamera_Evaluation\\Kinect_v2\\Recordings_Multiple_Path", "/data/map_multiple_path.yml");
	runAnalysis("/data/Xtion_2_Calibration.xml", "F:\\Kamera_Evaluation\\Xtion_2\\Recordings_Multiple_Path", "/data/map_multiple_path.yml");
	g_renderer.Stop();
	return 0;
}