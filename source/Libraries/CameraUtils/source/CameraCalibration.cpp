#include "CameraUtils/CameraCalibration.h"

#include <algorithm>
#include <string>

namespace bow
{

	IntrinsicCameraParameters CameraCalibration::intrinsicChessboardCalibration(int boardWidth, int boardHeight, float squareSize, const std::string& CalibrationImagesFolderPath)
	{
		std::string folderPath = CalibrationImagesFolderPath;
		std::replace(folderPath.begin(), folderPath.end(), '/', '\\');

		std::vector<std::string> files;
#ifdef __unix__ 
		DIR *dp;
		struct dirent *dirp;
		if ((dp = opendir(folderPath.c_str())) == NULL) {
			std::cout << "Error(" << errno << ") opening " << folderPath << std::endl;
			return IntrinsicCameraParameters();
		}

		while ((dirp = readdir(dp)) != NULL) {
			files.push_back(std::string(dirp->d_name));
		}
		closedir(dp);
#elif defined(_WIN32) || defined(WIN32)
		FILE* pipe = NULL;
		std::string pCmd = "dir /B /S " + folderPath;
		char buf[256];

		if (NULL == (pipe = _popen(pCmd.c_str(), "rt")))
		{
			std::cout << "Error while opening " << folderPath << std::endl;
			return IntrinsicCameraParameters();
		}

		while (!feof(pipe))
		{
			if (fgets(buf, 256, pipe) != NULL)
			{
				files.push_back(std::string(buf));
			}
		}

		for (unsigned int i = 0; i < files.size(); i++)
		{
			std::replace(files[i].begin(), files[i].end(), '\\', '/');
			files[i] = files[i].substr(0, files[i].length() - 1);
		}

		_pclose(pipe);
#endif
		if (files.size() == 0)
		{
			std::cout << "No Images were found!" << std::endl;
			return IntrinsicCameraParameters();
		}

		std::string combinedImageList = "";
		for (unsigned int i = 0; i < files.size(); i++)
		{
			combinedImageList += files[i];
		}

		std::hash<std::string> hash_fn;
		size_t calibrationHash = hash_fn(combinedImageList);
		std::string calibrationFilename = std::string("./calib_") + std::to_string(calibrationHash) + std::string(".xml");
		cv::FileStorage loadFileStorage = cv::FileStorage(calibrationFilename, cv::FileStorage::READ);

		IntrinsicCameraParameters cameraParameters;
		if (loadFileStorage.isOpened())
		{
			std::cout << "\tReading calibration from file..." << std::endl;

			cameraParameters;
			loadFileStorage["cameraMatrix"] >> cameraParameters.cameraMatrix;
			loadFileStorage["distCoeffs"] >> cameraParameters.distCoeffs;
			loadFileStorage["cameraProjectionMatrix"] >> cameraParameters.cameraProjectionMatrix;
			loadFileStorage["cameraProjectionMatrixInverted"] >> cameraParameters.cameraProjectionMatrixInverted;
			loadFileStorage["image_width"] >> cameraParameters.image_width;
			loadFileStorage["image_height"] >> cameraParameters.image_height;
			loadFileStorage.release();
		}
		else
		{
			std::vector<cv::Mat> inputImages;
			inputImages.reserve(files.size());

			std::cout << "\tLoading images..." << std::endl;
			for (unsigned int i = 0; i < files.size(); i++)
			{
				inputImages.push_back(cv::imread(files[i], cv::IMREAD_UNCHANGED));
			}

			std::cout << "\tCalibrating..." << std::endl;

			cameraParameters = CameraCalibration::intrinsicChessboardCalibration(boardWidth, boardHeight, squareSize, inputImages);
			if (cameraParameters.cameraMatrix.cols > 0 && cameraParameters.cameraMatrix.rows > 0)
			{
				std::cout << "Camera sucessfully calibrated..." << std::endl;

				cv::FileStorage saveFileStorage(calibrationFilename, cv::FileStorage::WRITE);
				saveFileStorage << "cameraMatrix" << cameraParameters.cameraMatrix;
				saveFileStorage << "distCoeffs" << cameraParameters.distCoeffs;
				saveFileStorage << "cameraProjectionMatrix" << cameraParameters.cameraProjectionMatrix;
				saveFileStorage << "cameraProjectionMatrixInverted" << cameraParameters.cameraProjectionMatrixInverted;
				saveFileStorage << "image_width" << cameraParameters.image_width;
				saveFileStorage << "image_height" << cameraParameters.image_height;
				saveFileStorage.release();
			}
			else
			{
				std::cout << "Error while calibrating camera..." << std::endl;
				return IntrinsicCameraParameters();
			}
		}
		return cameraParameters;
	}


	IntrinsicCameraParameters CameraCalibration::intrinsicChessboardCalibration(int boardWidth, int boardHeight, float squareSize, const std::vector<cv::Mat>& imageList)
	{
		std::vector<std::vector<cv::Point2f>> imagePoints;

		cv::Size imageSize;

		// =================================
		// Prepare Input
		// =================================

		for (unsigned int i = 0; i < imageList.size(); i++)
		{
			cv::Mat view = imageList[i];

			imageSize = view.size();  // Format input image.

			std::vector<cv::Point2f> pointBuf;

			int chessBoardFlags = cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE;

			bool found = cv::findChessboardCorners(view, cv::Size(boardWidth, boardHeight), pointBuf, chessBoardFlags);
			if (found) // If done with success,
			{
				// improve the found corners' coordinate accuracy for chessboard
				cv::Mat viewGray;
				if (view.channels() == 3) {
					cv::cvtColor(view, viewGray, cv::COLOR_BGR2GRAY);
				}
				else
				{
					viewGray = view;
				}

				cv::cornerSubPix(viewGray, pointBuf, cv::Size(11, 11), cv::Size(-1, -1), cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));

				imagePoints.push_back(pointBuf);
			}
		}

		// =================================
		// Run Calibration
		// =================================

		std::vector<cv::Mat> rvecs, tvecs;

		// fixed_aspect
		cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
		//out_cameraMatrix.at<double>(0,0) = s.aspectRatio;

		cv::Mat distCoeffs = cv::Mat::zeros(8, 1, CV_64F);

		std::vector<std::vector<cv::Point3f>> objectPoints(1);
		calcBoardCornerPositions(cv::Size(boardWidth, boardHeight), squareSize, objectPoints[0]);

		objectPoints.resize(imagePoints.size(), objectPoints[0]);

		//Find intrinsic and extrinsic camera parameters
		double rms = cv::calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs);

		std::cout << "Re-projection error reported by calibrateCamera: " << rms << std::endl;

		bool success = cv::checkRange(cameraMatrix) && cv::checkRange(distCoeffs);

		if (success)
		{
			IntrinsicCameraParameters parameters;
			parameters.cameraMatrix = cameraMatrix;
			parameters.distCoeffs = distCoeffs;
			parameters.image_width = imageList[0].cols;
			parameters.image_height = imageList[0].rows;
			parameters.cameraProjectionMatrix = getProjectionMatrix(cv::Size(parameters.image_width, parameters.image_height), parameters.cameraMatrix);
			cv::invert(parameters.cameraProjectionMatrix, parameters.cameraProjectionMatrixInverted);
			return parameters;
		}
		else
		{
			IntrinsicCameraParameters parameters;
			return parameters;
		}
	}

	cv::Mat CameraCalibration::calculateChessboardCameraTransformationViewMatrix(const cv::Mat& fromCameraChessboardImage, const cv::Mat& ToCameraChessboardImage, const IntrinsicCameraParameters& fromCameraParameters, const IntrinsicCameraParameters& toCameraParameters, int boardWidth, int boardHeight, float squareSize)
	{
		cv::Mat from_DistCoeffs = cv::Mat::zeros(4, 1, CV_32F);
		cv::Mat to_DistCoeffs = cv::Mat::zeros(4, 1, CV_32F);

		cv::Mat fromCamera_image;
		cv::Mat toCamera_image;

		cv::undistort(fromCameraChessboardImage, fromCamera_image, fromCameraParameters.cameraMatrix, fromCameraParameters.distCoeffs);
		cv::undistort(ToCameraChessboardImage, toCamera_image, toCameraParameters.cameraMatrix, toCameraParameters.distCoeffs);

		cv::Mat_<float> fromCameraModelMatrix = extrinsicChessboardCalibration(boardWidth, boardHeight, squareSize, fromCameraParameters.cameraMatrix, from_DistCoeffs, fromCamera_image);
		cv::Mat_<float> toCameraModelMatrix = extrinsicChessboardCalibration(boardWidth, boardHeight, squareSize, toCameraParameters.cameraMatrix, to_DistCoeffs, toCamera_image);

		bool fromSuccess = false;
		bool toSuccess = false;

		if (fromCameraModelMatrix.rows > 0 && fromCameraModelMatrix.cols > 0)
			fromSuccess = true;

		if (toCameraModelMatrix.rows > 0 && toCameraModelMatrix.cols > 0)
			toSuccess = true;

		if (!fromSuccess || !toSuccess)
		{
			cv::Mat identity = cv::Mat::zeros(4, 4, CV_32FC1);
			identity.at<float>(0, 0) = 1.0f;
			identity.at<float>(1, 1) = 1.0f;
			identity.at<float>(2, 2) = 1.0f;
			identity.at<float>(3, 3) = 1.0f;
			return identity;
		}

		cv::Mat invFromCameraModelMatrix;
		cv::invert(fromCameraModelMatrix, invFromCameraModelMatrix);

		cv::Mat invToCameraModelMat;
		cv::invert(toCameraModelMatrix, invToCameraModelMat);

		return toCameraModelMatrix * invFromCameraModelMatrix;
	}

	cv::Mat_<cv::Vec4f> CameraCalibration::calculate_directionMatrix(const IntrinsicCameraParameters& cameraParameters, unsigned int cols, unsigned int rows, bool lens_distorted)
	{
		cv::Mat screenPoints = cv::Mat(1, cols * rows, CV_32FC2);
		for (unsigned int row = 0; row < rows; row++)
		{
			for (unsigned int col = 0; col < cols; col++)
			{
				unsigned int i = col + (cols * row);
				screenPoints.at<cv::Vec2f>(0, i).val[0] = ((float)col / (float)cols) * (float)cameraParameters.image_width;
				screenPoints.at<cv::Vec2f>(0, i).val[1] = ((float)row / (float)rows) * (float)cameraParameters.image_height;
			}
		}

		cv::Mat newCameraMatrix = cv::getOptimalNewCameraMatrix(cameraParameters.cameraMatrix, cameraParameters.distCoeffs, cv::Size(cameraParameters.image_width, cameraParameters.image_height), 0);

		if (lens_distorted)
		{
			cv::Mat undistortedScreenPoints;
			cv::undistortPoints(screenPoints, undistortedScreenPoints, cameraParameters.cameraMatrix, cameraParameters.distCoeffs, cv::noArray(), newCameraMatrix);
			undistortedScreenPoints.copyTo(screenPoints);
		}

		// modify input
		cv::Mat_<cv::Vec4f> undistortedScreenPointDirections = cv::Mat_<cv::Vec4f>(rows, cols);
		for (unsigned int row = 0; row < rows; row++)
		{
			for (unsigned int col = 0; col < cols; col++)
			{
				unsigned int i = col + (cols * row);

				undistortedScreenPointDirections.at<cv::Vec4f>(row, col).val[0] = ((screenPoints.at<cv::Vec2f>(0, i).val[0] / (float)(cameraParameters.image_width)) * 2.0f) - 1.0f;
				undistortedScreenPointDirections.at<cv::Vec4f>(row, col).val[1] = ((screenPoints.at<cv::Vec2f>(0, i).val[1] / (float)(cameraParameters.image_height)) * 2.0f) - 1.0f;

				undistortedScreenPointDirections.at<cv::Vec4f>(row, col).val[2] = 1.0f;
				undistortedScreenPointDirections.at<cv::Vec4f>(row, col).val[3] = 1.0f;

				cv::Mat viewSpaceDirection = cameraParameters.cameraProjectionMatrixInverted * cv::Mat(undistortedScreenPointDirections.at<cv::Vec4f>(row, col));

				float magnitude = std::sqrt((viewSpaceDirection.at<float>(0, 0)*viewSpaceDirection.at<float>(0, 0)) + (viewSpaceDirection.at<float>(1, 0)*viewSpaceDirection.at<float>(1, 0)) + (viewSpaceDirection.at<float>(2, 0)*viewSpaceDirection.at<float>(2, 0)));
				undistortedScreenPointDirections.at<cv::Vec4f>(row, col) = cv::Vec4f(viewSpaceDirection.at<float>(0, 0) / magnitude, viewSpaceDirection.at<float>(1, 0) / magnitude, viewSpaceDirection.at<float>(2, 0) / magnitude, 1.0f);
			}
		}

		return undistortedScreenPointDirections;
	}

	cv::Mat_<cv::Vec3f> CameraCalibration::calculate_coordinates_from_depth(const cv::Mat_<cv::Vec4f>& directionMatrix, const cv::Mat_<ushort>& depthMap)
	{
		cv::Mat_<cv::Vec3f> out_coordinates = cv::Mat_<cv::Vec3f>(depthMap.rows, depthMap.cols);
		for (unsigned int row = 0; row < depthMap.rows; row++)
		{
			for (unsigned int col = 0; col < depthMap.cols; col++)
			{
				cv::Vec3f direction = cv::Vec3f(directionMatrix.at<cv::Vec4f>(row, col).val[0], directionMatrix.at<cv::Vec4f>(row, col).val[1], directionMatrix.at<cv::Vec4f>(row, col).val[2]);
				cv::Vec3f normal = cv::Vec3f(0.0f, 0.0f, 1.0f);
				cv::Vec3f origin = cv::Vec3f(0.0f, 0.0f, 0.0f);

				cv::Vec3f point;
				double denom = normal.dot(direction);
				if (abs(denom) > 0.0001f) // your favorite epsilon
				{
					double t = (cv::Vec3f(0.0f, 0.0f, -1.0f) * depthMap.at<ushort>(row, col)).dot(normal) / denom;
					if (t >= 0)
					{
						point = direction * t;
					}
					else
					{
						point = cv::Vec3f(0.0f, 0.0f, 0.0f);
					}
				}

				out_coordinates.at<cv::Vec3f>(row, col) = cv::Vec3f(point.val[0], point.val[1], -point.val[2]);
			}
		}
		return out_coordinates;
	}

	cv::Mat_<cv::Vec3f> CameraCalibration::calculate_coordinates_from_range(const cv::Mat_<cv::Vec4f>& directionMatrix, const cv::Mat_<ushort>& depthMap)
	{
		cv::Mat_<cv::Vec3f> out_coordinates = cv::Mat_<cv::Vec3f>(depthMap.rows, depthMap.cols);
		for (unsigned int row = 0; row < depthMap.rows; row++)
		{
			for (unsigned int col = 0; col < depthMap.cols; col++)
			{
				cv::Vec3f direction = cv::Vec3f(directionMatrix.at<cv::Vec4f>(row, col).val[0], directionMatrix.at<cv::Vec4f>(row, col).val[1], directionMatrix.at<cv::Vec4f>(row, col).val[2]);
				out_coordinates.at<cv::Vec3f>(row, col) = direction * (float)depthMap.at<ushort>(row, col);
				out_coordinates.at<cv::Vec3f>(row, col).val[2] = -out_coordinates.at<cv::Vec3f>(row, col).val[2];
			}
		}
		return out_coordinates;
	}

	cv::Vec3f CameraCalibration::calculate_coordinate_from_depth(const cv::Mat_<cv::Vec4f>& directionMatrix, const ushort depthvalue, unsigned int col, unsigned int row)
	{
		cv::Vec3f direction = cv::Vec3f(directionMatrix.at<cv::Vec4f>(row, col).val[0], directionMatrix.at<cv::Vec4f>(row, col).val[1], directionMatrix.at<cv::Vec4f>(row, col).val[2]);
		cv::Vec3f normal = cv::Vec3f(0.0f, 0.0f, 1.0f);
		cv::Vec3f origin = cv::Vec3f(0.0f, 0.0f, 0.0f);

		cv::Vec3f point;
		double denom = normal.dot(direction);
		if (abs(denom) > 0.0001f) // your favorite epsilon
		{
			double t = (cv::Vec3f(0.0f, 0.0f, -1.0f) * (float)depthvalue).dot(normal) / denom;
			if (t >= 0)
			{
				point = direction * t;
			}
			else
			{
				point = cv::Vec3f(0.0f, 0.0f, 0.0f);
			}
		}

		return cv::Vec3f(point.val[0], point.val[1], -point.val[2]);
	}

	cv::Mat CameraCalibration::extrinsicChessboardCalibration(int boardWidth, int boardHeight, float squareSize, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs, const cv::Mat& chessBoardImage)
	{
		std::vector<cv::Point3f> boardPoints(boardWidth * boardHeight);
		for (unsigned int row = 0; row < boardHeight; row++)
		{
			for (unsigned int col = 0; col < boardWidth; col++)
			{
				boardPoints[col + (row * boardWidth)] = cv::Point3f((col * squareSize), (row * squareSize), 0.0);
			}
		}

		cv::Mat grey;
		if (chessBoardImage.channels() == 3)
			cv::cvtColor(chessBoardImage, grey, cv::COLOR_BGR2GRAY);
		else
			chessBoardImage.copyTo(grey);

		std::vector<cv::Point2f> pointBuf;
		bool found = cv::findChessboardCorners(grey, cv::Size(boardWidth, boardHeight), pointBuf, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);
		if (found) // If done with success,
		{
			// improve the found corners' coordinate accuracy for chessboard
			cv::cornerSubPix(grey, pointBuf, cv::Size(11, 11), cv::Size(-1, -1), cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));

			cv::Vec3d rvec, tvec;
			cv::solvePnP(cv::Mat(boardPoints), cv::Mat(pointBuf), cameraMatrix, distCoeffs, rvec, tvec, false);

			cv::Mat cameraRotationMatrix;
			cv::Rodrigues(rvec, cameraRotationMatrix);
			cv::Mat cameraTranslationVector = cv::Mat(tvec);

			cv::Mat ModelMatrix = cv::Mat::zeros(4, 4, CV_32FC1);
			ModelMatrix.at<float>(0) = cameraRotationMatrix.at<double>(0, 0);
			ModelMatrix.at<float>(1) = cameraRotationMatrix.at<double>(0, 1);
			ModelMatrix.at<float>(2) = cameraRotationMatrix.at<double>(0, 2);
			ModelMatrix.at<float>(3) = cameraTranslationVector.at<double>(0, 0);

			ModelMatrix.at<float>(4) = cameraRotationMatrix.at<double>(1, 0);
			ModelMatrix.at<float>(5) = cameraRotationMatrix.at<double>(1, 1);
			ModelMatrix.at<float>(6) = cameraRotationMatrix.at<double>(1, 2);
			ModelMatrix.at<float>(7) = cameraTranslationVector.at<double>(1, 0);

			ModelMatrix.at<float>(8) = cameraRotationMatrix.at<double>(2, 0);
			ModelMatrix.at<float>(9) = cameraRotationMatrix.at<double>(2, 1);
			ModelMatrix.at<float>(10) = cameraRotationMatrix.at<double>(2, 2);
			ModelMatrix.at<float>(11) = cameraTranslationVector.at<double>(2, 0);

			ModelMatrix.at<float>(12) = 0.0;
			ModelMatrix.at<float>(13) = 0.0;
			ModelMatrix.at<float>(14) = 0.0;
			ModelMatrix.at<float>(15) = 1.0;

			return ModelMatrix;
		}
		return cv::Mat();
	}

	cv::Mat_<float> CameraCalibration::getProjectionMatrix(const cv::Size imageSize, const cv::Mat& cameraMatrix)
	{
		double znear = 1.0;
		double zfar = 100000.0;
		bool invert = false;

		double proj_matrix[16];

		//Deterime the rsized info
		double _fx = cameraMatrix.at<double>(0, 0);
		double _cx = cameraMatrix.at<double>(0, 2);
		double _fy = cameraMatrix.at<double>(1, 1);
		double _cy = cameraMatrix.at<double>(1, 2);

		double cparam[3][4] =
		{
			{ _fx, 0, _cx, 0 },
			{ 0, _fy, _cy, 0 },
			{ 0, 0, 1, 0 }
		};

		{
			double   icpara[3][4];
			double   trans[3][4];
			double   p[3][3], q[4][4];
			int      i, j;

			cparam[0][2] *= -1.0;
			cparam[1][2] *= -1.0;
			cparam[2][2] *= -1.0;

			// arParamDecompMat
			{
				int       r, c;
				double    Cpara[3][4];
				double    rem1, rem2, rem3;

				if (cparam[2][3] >= 0)
				{
					for (r = 0; r < 3; r++)
					{
						for (c = 0; c < 4; c++)
						{
							Cpara[r][c] = cparam[r][c];
						}
					}
				}
				else
				{
					for (r = 0; r < 3; r++)
					{
						for (c = 0; c < 4; c++)
						{
							Cpara[r][c] = -(cparam[r][c]);
						}
					}
				}

				for (r = 0; r < 3; r++)
				{
					for (c = 0; c < 4; c++)
					{
						icpara[r][c] = 0.0;
					}
				}

				icpara[2][2] = norm(Cpara[2][0], Cpara[2][1], Cpara[2][2]);
				trans[2][0] = Cpara[2][0] / icpara[2][2];
				trans[2][1] = Cpara[2][1] / icpara[2][2];
				trans[2][2] = Cpara[2][2] / icpara[2][2];
				trans[2][3] = Cpara[2][3] / icpara[2][2];

				icpara[1][2] = dot(trans[2][0], trans[2][1], trans[2][2], Cpara[1][0], Cpara[1][1], Cpara[1][2]);
				rem1 = Cpara[1][0] - icpara[1][2] * trans[2][0];
				rem2 = Cpara[1][1] - icpara[1][2] * trans[2][1];
				rem3 = Cpara[1][2] - icpara[1][2] * trans[2][2];
				icpara[1][1] = norm(rem1, rem2, rem3);
				trans[1][0] = rem1 / icpara[1][1];
				trans[1][1] = rem2 / icpara[1][1];
				trans[1][2] = rem3 / icpara[1][1];

				icpara[0][2] = dot(trans[2][0], trans[2][1], trans[2][2], Cpara[0][0], Cpara[0][1], Cpara[0][2]);
				icpara[0][1] = dot(trans[1][0], trans[1][1], trans[1][2], Cpara[0][0], Cpara[0][1], Cpara[0][2]);
				rem1 = Cpara[0][0] - icpara[0][1] * trans[1][0] - icpara[0][2] * trans[2][0];
				rem2 = Cpara[0][1] - icpara[0][1] * trans[1][1] - icpara[0][2] * trans[2][1];
				rem3 = Cpara[0][2] - icpara[0][1] * trans[1][2] - icpara[0][2] * trans[2][2];
				icpara[0][0] = norm(rem1, rem2, rem3);
				trans[0][0] = rem1 / icpara[0][0];
				trans[0][1] = rem2 / icpara[0][0];
				trans[0][2] = rem3 / icpara[0][0];

				trans[1][3] = (Cpara[1][3] - icpara[1][2] * trans[2][3]) / icpara[1][1];
				trans[0][3] = (Cpara[0][3] - icpara[0][1] * trans[1][3] - icpara[0][2] * trans[2][3]) / icpara[0][0];

				for (r = 0; r < 3; r++)
				{
					for (c = 0; c < 3; c++)
					{
						icpara[r][c] /= icpara[2][2];
					}
				}
			}
			// arParamDecompMat end

			for (i = 0; i < 3; i++)
			{
				for (j = 0; j < 3; j++)
				{
					p[i][j] = icpara[i][j] / icpara[2][2];
				}
			}

			q[0][0] = (2.0 * p[0][0] / imageSize.width);
			q[0][1] = (2.0 * p[0][1] / imageSize.width);
			q[0][2] = ((2.0 * p[0][2] / imageSize.width) - 1.0);
			q[0][3] = 0.0;

			q[1][0] = 0.0;
			q[1][1] = (2.0 * p[1][1] / imageSize.height);
			q[1][2] = ((2.0 * p[1][2] / imageSize.height) - 1.0);
			q[1][3] = 0.0;

			q[2][0] = 0.0;
			q[2][1] = 0.0;
			q[2][2] = (zfar + znear) / (zfar - znear);
			q[2][3] = -2.0 * zfar * znear / (zfar - znear);

			q[3][0] = 0.0;
			q[3][1] = 0.0;
			q[3][2] = 1.0;
			q[3][3] = 0.0;

			for (i = 0; i < 4; i++)
			{
				for (j = 0; j < 3; j++)
				{
					proj_matrix[i + j * 4] = q[i][0] * trans[0][j] + q[i][1] * trans[1][j] + q[i][2] * trans[2][j];
				}

				proj_matrix[i + 3 * 4] = q[i][0] * trans[0][3] + q[i][1] * trans[1][3] + q[i][2] * trans[2][3] + q[i][3];
			}

			if (!invert)
			{
				proj_matrix[13] = -proj_matrix[13];
				proj_matrix[1] = -proj_matrix[1];
				proj_matrix[5] = -proj_matrix[5];
				proj_matrix[9] = -proj_matrix[9];
			}
		}

		if (false)
		{
			proj_matrix[10] = -proj_matrix[10];
			proj_matrix[11] = -proj_matrix[11];
		}

		cv::Mat_<float> projMat = cv::Mat_<float>::zeros(4, 4);
		for (unsigned int i = 0; i < 4 * 4; i++)
		{
			projMat.at<float>(i) = (float)proj_matrix[i];
		}

		cv::Mat_<float> transposed;
		cv::transpose(projMat, transposed);
		return transposed;
	}


	void CameraCalibration::calcBoardCornerPositions(const cv::Size& boardSize, const float& squareSize, std::vector<cv::Point3f>& out_corners)
	{
		out_corners.clear();

		for (int i = 0; i < boardSize.height; ++i)
		{
			for (int j = 0; j < boardSize.width; ++j)
			{
				out_corners.push_back(cv::Point3f(j*squareSize, i*squareSize, 0));
			}
		}
	}


	double CameraCalibration::norm(double a, double b, double c)
	{
		return(sqrt(a*a + b*b + c*c));
	}


	double CameraCalibration::dot(double a1, double a2, double a3, double b1, double b2, double b3)
	{
		return(a1 * b1 + a2 * b2 + a3 * b3);
	}
}
