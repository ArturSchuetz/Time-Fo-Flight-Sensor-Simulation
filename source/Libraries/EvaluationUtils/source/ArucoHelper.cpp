#include "EvaluationUtils/ArucoHelper.h"

//opencv
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>

namespace bow
{
	std::vector<MarkerDescription> ArucoHelper::LoadMarkerMapFromFile(const std::string& filePath)
	{
		cv::FileStorage fs;
		fs.open(filePath.c_str(), cv::FileStorage::READ);
		if (!fs.isOpened())
		{
			std::cerr << "Failed to open map.yml" << std::endl;
			return std::vector<MarkerDescription>();
		}

		int aruco_bc_nmarkers = (int)fs["aruco_bc_nmarkers"];
		int aruco_bc_mInfoType = (int)fs["aruco_bc_mInfoType"];

		std::vector<MarkerDescription> markerMap = std::vector<MarkerDescription>(aruco_bc_nmarkers);
		for (unsigned int i = 0; i < (unsigned int)aruco_bc_nmarkers; i++)
		{
			markerMap[i].id = (int)fs["aruco_bc_markers"][i]["id"];
			for (unsigned int j = 0; j < 4; j++)
			{
				// Scaling from meter to milimeter by multiplication with 1000
				markerMap[i].corners[j] = cv::Vec3f((float)fs["aruco_bc_markers"][i]["corners"][j][0], (float)fs["aruco_bc_markers"][i]["corners"][j][1], (float)fs["aruco_bc_markers"][i]["corners"][j][2]) * 1000.0f;
			}
			markerMap[i].center = cv::Vec3f((markerMap[i].corners[0] + markerMap[i].corners[1] + markerMap[i].corners[2] + markerMap[i].corners[3]) / 4.0f);
			markerMap[i].sidelengthInMM = cv::norm(cv::Vec3f(markerMap[i].corners[0] - markerMap[i].corners[1]));
			markerMap[i].up = cv::normalize(cv::Vec3f(markerMap[i].corners[1] - markerMap[i].corners[2]));
			markerMap[i].right = cv::normalize(cv::Vec3f((markerMap[i].corners[1] - markerMap[i].corners[0])));
			markerMap[i].front = cv::normalize(cv::Vec3f(markerMap[i].right.cross(markerMap[i].up)));

			markerMap[i].modelMatrix = cv::Mat(4, 4, CV_32FC1);
			markerMap[i].modelMatrix.at<float>(0, 0) = markerMap[i].right[0];
			markerMap[i].modelMatrix.at<float>(0, 1) = markerMap[i].up[0];
			markerMap[i].modelMatrix.at<float>(0, 2) = markerMap[i].front[0];
			markerMap[i].modelMatrix.at<float>(0, 3) = markerMap[i].center[0];

			markerMap[i].modelMatrix.at<float>(1, 0) = markerMap[i].right[1];
			markerMap[i].modelMatrix.at<float>(1, 1) = markerMap[i].up[1];
			markerMap[i].modelMatrix.at<float>(1, 2) = markerMap[i].front[1];
			markerMap[i].modelMatrix.at<float>(1, 3) = markerMap[i].center[1];

			markerMap[i].modelMatrix.at<float>(2, 0) = markerMap[i].right[2];
			markerMap[i].modelMatrix.at<float>(2, 1) = markerMap[i].up[2];
			markerMap[i].modelMatrix.at<float>(2, 2) = markerMap[i].front[2];
			markerMap[i].modelMatrix.at<float>(2, 3) = markerMap[i].center[2];

			markerMap[i].modelMatrix.at<float>(3, 0) = 0.0f;
			markerMap[i].modelMatrix.at<float>(3, 1) = 0.0f;
			markerMap[i].modelMatrix.at<float>(3, 2) = 0.0f;
			markerMap[i].modelMatrix.at<float>(3, 3) = 1.0f;

			cv::invert(markerMap[i].modelMatrix, markerMap[i].invModelMatrix);
		}

		return markerMap;
	}

	std::vector<Marker> ArucoHelper::detectMarker(const cv::Mat& inputImage, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs, float markerSideLengthInMM, const std::vector<MarkerDescription> markerMap)
	{
		std::vector<Marker> result;

		std::vector<int> markerIds;
		std::vector<std::vector<cv::Point2f>> markerCorners;
		std::vector<std::vector<cv::Point2f>> rejectedCandidates;
		cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
		parameters->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;

		parameters->cornerRefinementMaxIterations = 100;
		parameters->cornerRefinementMinAccuracy = 0.01;

		if (inputImage.rows < 480 && inputImage.cols < 640)
			parameters->cornerRefinementWinSize = 2;
		else
			parameters->cornerRefinementWinSize = 5;

		parameters->adaptiveThreshWinSizeMin = 3;
		parameters->adaptiveThreshWinSizeMax = 23;
		parameters->adaptiveThreshWinSizeStep = 5;

		cv::aruco::detectMarkers(inputImage, cv::aruco::getPredefinedDictionary(cv::aruco::DICT_ARUCO_ORIGINAL), markerCorners, markerIds, parameters, rejectedCandidates, cameraMatrix, distCoeffs);
		
		if (markerIds.size() == 0)
			return result;

		std::vector<cv::Vec3d> rvecs, tvecs;
		cv::aruco::estimatePoseSingleMarkers(markerCorners, markerSideLengthInMM, cameraMatrix, distCoeffs, rvecs, tvecs);

		//cv::aruco::drawDetectedMarkers(inputImage, markerCorners, markerIds);
		//cv::imshow("markerImage", inputImage);
		//cv::waitKey(1);

		for (unsigned int i = 0; i < markerIds.size(); i++)
		{
			if (markerIds[i] == 0)
				continue;

			Marker newMarker;
			newMarker.Id = markerIds[i];
			newMarker.RotationVector = rvecs[i];
			newMarker.TranslationVector = tvecs[i];

			cv::Mat cameraRotationMatrix;
			cv::Rodrigues(newMarker.RotationVector, cameraRotationMatrix);
			cv::Mat cameraTranslationVector = cv::Mat(newMarker.TranslationVector);

			newMarker.ModelMatrix = cv::Mat::zeros(4, 4, CV_32FC1);
			newMarker.ModelMatrix.at<float>(0) = cameraRotationMatrix.at<double>(0, 0);
			newMarker.ModelMatrix.at<float>(1) = cameraRotationMatrix.at<double>(0, 1);
			newMarker.ModelMatrix.at<float>(2) = cameraRotationMatrix.at<double>(0, 2);
			newMarker.ModelMatrix.at<float>(3) = cameraTranslationVector.at<double>(0, 0);

			newMarker.ModelMatrix.at<float>(4) = cameraRotationMatrix.at<double>(1, 0);
			newMarker.ModelMatrix.at<float>(5) = cameraRotationMatrix.at<double>(1, 1);
			newMarker.ModelMatrix.at<float>(6) = cameraRotationMatrix.at<double>(1, 2);
			newMarker.ModelMatrix.at<float>(7) = cameraTranslationVector.at<double>(1, 0);

			newMarker.ModelMatrix.at<float>(8) = cameraRotationMatrix.at<double>(2, 0);
			newMarker.ModelMatrix.at<float>(9) = cameraRotationMatrix.at<double>(2, 1);
			newMarker.ModelMatrix.at<float>(10) = cameraRotationMatrix.at<double>(2, 2);
			newMarker.ModelMatrix.at<float>(11) = cameraTranslationVector.at<double>(2, 0);

			newMarker.ModelMatrix.at<float>(12) = 0.0;
			newMarker.ModelMatrix.at<float>(13) = 0.0;
			newMarker.ModelMatrix.at<float>(14) = 0.0;
			newMarker.ModelMatrix.at<float>(15) = 1.0;

			//std::cout << newMarker.ModelMatrix << std::endl;
			newMarker.Ignored = false;

			result.push_back(newMarker);
		}

		return result;
	}


	bool ArucoHelper::getTransformationFromMarkerMap(const std::vector<MarkerDescription>& markerMap, const cv::Mat& inputImage, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs, cv::Matx<float, 4, 4>& global_Transform)
	{
		global_Transform = cv::Matx<float, 4, 4>::zeros();

		global_Transform(0, 0) = 1.0;
		global_Transform(1, 1) = 1.0;
		global_Transform(2, 2) = 1.0;
		global_Transform(3, 3) = 1.0;

		std::vector<int> marker_map_Ids(markerMap.size());
		std::vector<std::vector<cv::Point3f>> marker_map_Corners(markerMap.size());
		for (unsigned int i = 0; i < markerMap.size(); i++)
		{
			marker_map_Ids[i] = markerMap[i].id;
			marker_map_Corners[i] = std::vector<cv::Point3f>(4);
			for (unsigned int c = 0; c < 4; c++)
			{
				marker_map_Corners[i][c] = markerMap[i].corners[c];
			}
		}
		cv::Ptr<cv::aruco::Board> marker_map_board = cv::aruco::Board::create(marker_map_Corners, cv::aruco::getPredefinedDictionary(cv::aruco::DICT_ARUCO_ORIGINAL), marker_map_Ids);

		std::vector<Marker> result;

		std::vector<cv::Vec3d> rvecs_ref, tvecs_ref;
		{
			std::vector<int> markerIds;
			std::vector<std::vector<cv::Point2f>> markerCorners;
			std::vector<std::vector<cv::Point2f>> rejectedCandidates;
			cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
			parameters->cornerRefinementMethod = cv::aruco::CORNER_REFINE_APRILTAG;

			cv::aruco::detectMarkers(inputImage, cv::aruco::getPredefinedDictionary(cv::aruco::DICT_ARUCO_ORIGINAL), markerCorners, markerIds, parameters, rejectedCandidates, cameraMatrix, distCoeffs);
			cv::aruco::refineDetectedMarkers(inputImage, marker_map_board, markerCorners, markerIds, rejectedCandidates, cameraMatrix, distCoeffs);

			if (markerIds.size() == 0)
				return false;

			cv::aruco::estimatePoseBoard(markerCorners, markerIds, marker_map_board, cameraMatrix, distCoeffs, rvecs_ref, tvecs_ref);
		}

		std::vector<int> markerIds;
		std::vector<std::vector<cv::Point2f>> markerCorners;
		std::vector<std::vector<cv::Point2f>> rejectedCandidates;
		cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
		parameters->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;

		cv::aruco::detectMarkers(inputImage, cv::aruco::getPredefinedDictionary(cv::aruco::DICT_ARUCO_ORIGINAL), markerCorners, markerIds, parameters, rejectedCandidates, cameraMatrix, distCoeffs);
		cv::aruco::refineDetectedMarkers(inputImage, marker_map_board, markerCorners, markerIds, rejectedCandidates, cameraMatrix, distCoeffs);

		if (markerIds.size() == 0)
			return false;

		std::vector<cv::Vec3d> rvecs, tvecs;
		cv::aruco::estimatePoseBoard(markerCorners, markerIds, marker_map_board, cameraMatrix, distCoeffs, rvecs, tvecs);

		for (unsigned int i = 0; i < markerIds.size(); i++)
		{
			std::cout << "dist: " << tvecs[i] - tvecs_ref[i] << std::endl;
		}

		for (unsigned int i = 0; i < markerIds.size(); i++)
		{
			if (markerIds[i] == 0)
				continue;

			Marker newMarker;
			newMarker.Id = markerIds[i];
			newMarker.RotationVector = rvecs[i];
			newMarker.TranslationVector = tvecs[i];

			cv::Mat cameraRotationMatrix;
			cv::Rodrigues(newMarker.RotationVector, cameraRotationMatrix);
			cv::Mat cameraTranslationVector = cv::Mat(newMarker.TranslationVector);

			newMarker.ModelMatrix = cv::Mat::zeros(4, 4, CV_32FC1);
			newMarker.ModelMatrix.at<float>(0) = cameraRotationMatrix.at<double>(0, 0);
			newMarker.ModelMatrix.at<float>(1) = cameraRotationMatrix.at<double>(0, 1);
			newMarker.ModelMatrix.at<float>(2) = cameraRotationMatrix.at<double>(0, 2);
			newMarker.ModelMatrix.at<float>(3) = cameraTranslationVector.at<double>(0, 0);

			newMarker.ModelMatrix.at<float>(4) = cameraRotationMatrix.at<double>(1, 0);
			newMarker.ModelMatrix.at<float>(5) = cameraRotationMatrix.at<double>(1, 1);
			newMarker.ModelMatrix.at<float>(6) = cameraRotationMatrix.at<double>(1, 2);
			newMarker.ModelMatrix.at<float>(7) = cameraTranslationVector.at<double>(1, 0);

			newMarker.ModelMatrix.at<float>(8) = cameraRotationMatrix.at<double>(2, 0);
			newMarker.ModelMatrix.at<float>(9) = cameraRotationMatrix.at<double>(2, 1);
			newMarker.ModelMatrix.at<float>(10) = cameraRotationMatrix.at<double>(2, 2);
			newMarker.ModelMatrix.at<float>(11) = cameraTranslationVector.at<double>(2, 0);

			newMarker.ModelMatrix.at<float>(12) = 0.0;
			newMarker.ModelMatrix.at<float>(13) = 0.0;
			newMarker.ModelMatrix.at<float>(14) = 0.0;
			newMarker.ModelMatrix.at<float>(15) = 1.0;

			//std::cout << newMarker.ModelMatrix << std::endl;
			newMarker.Ignored = false;

			result.push_back(newMarker);
		}
		
		return ArucoHelper::getTransformationFromMarkerMap(markerMap, result, global_Transform);
	}

	bool ArucoHelper::getTransformationFromMarkerMap(const std::vector<MarkerDescription>& markerMap, std::vector<Marker>& detectedMarker, cv::Matx<float, 4, 4>& global_Transform)
	{
		global_Transform = cv::Matx<float, 4, 4>::zeros();

		global_Transform(0, 0) = 1.0;
		global_Transform(1, 1) = 1.0;
		global_Transform(2, 2) = 1.0;
		global_Transform(3, 3) = 1.0;

		// Calculate camera matrix
		if (detectedMarker.size() > 0)
		{
			std::vector<float> distances;
			std::vector<cv::Vec3f> rights;
			std::vector<cv::Vec3f> ups;
			std::vector<cv::Vec3f> fronts;
			std::vector<cv::Vec3f> positions;
			float medianDistance = 0.0f;

			std::vector<Marker*> usedMarkers;
			for (unsigned int i = 0; i < detectedMarker.size(); i++)
			{
				if (detectedMarker[i].Ignored)
					continue;

				const MarkerDescription* referencedMarker = nullptr;
				for (unsigned int j = 0; j < markerMap.size(); j++)
				{
					if ((int)markerMap[j].id == detectedMarker[i].Id)
					{
						referencedMarker = &(markerMap[j]);
						break;
					}
				}

				if (referencedMarker == nullptr)
				{
					detectedMarker[i].Ignored = true;
					continue;
				}

				cv::Mat invMat;
				cv::invert(detectedMarker[i].ModelMatrix, invMat);
				cv::Mat temp_Transform = referencedMarker->modelMatrix * invMat;

				//std::cout << "temp_Transform:" << std::endl << temp_Transform << std::endl << std::endl;

				usedMarkers.push_back(&detectedMarker[i]);
				rights.push_back(cv::Vec3f(temp_Transform.at<float>(0, 0), temp_Transform.at<float>(1, 0), temp_Transform.at<float>(2, 0)));
				ups.push_back(cv::Vec3f(temp_Transform.at<float>(0, 1), temp_Transform.at<float>(1, 1), temp_Transform.at<float>(2, 1)));
				fronts.push_back(cv::Vec3f(temp_Transform.at<float>(0, 2), temp_Transform.at<float>(1, 2), temp_Transform.at<float>(2, 2)));
				positions.push_back(cv::Vec3f(temp_Transform.at<float>(0, 3), temp_Transform.at<float>(1, 3), temp_Transform.at<float>(2, 3)));
			}

			if (usedMarkers.size() > 0)
			{
				// Calculate median
				for (unsigned int i = 0; i < positions.size(); i++)
				{
					distances.push_back(cv::norm(positions[i]));
				}
				std::sort(distances.begin(), distances.end());
				float median = distances[distances.size() / 2];

				int detectedMarkerCount = 0;
				cv::Vec3f rightVecSum = cv::Vec3f(0, 0, 0);
				cv::Vec3f upVecSum = cv::Vec3f(0, 0, 0);
				cv::Vec3f frontVecSum = cv::Vec3f(0, 0, 0);
				cv::Vec3f positionVecSum = cv::Vec3f(0, 0, 0);
				for (unsigned int i = 0; i < positions.size(); i++)
				{
					const static bow::Vector3<float> viewDirection = bow::Vector3<float>(0.0f, 0.0f, -1.0f);
					bow::Vector3<float> frontVec = bow::Vector3<float>(usedMarkers[i]->ModelMatrix.at<float>(0, 2), usedMarkers[i]->ModelMatrix.at<float>(1, 2), usedMarkers[i]->ModelMatrix.at<float>(2, 2));
					float angle = viewDirection.DotP(frontVec);

					cv::Mat_<float> center = (cv::Mat_<float>)usedMarkers[i]->ModelMatrix * cv::Mat_<float>(cv::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
					float distance = cv::norm(cv::Vec3f(center.at<float>(0, 0), center.at<float>(1, 0), center.at<float>(2, 0)));

					// 5 cm threshold
					float validationDistances = distances[i] - median;
					if (validationDistances * validationDistances > 10.0f * 10.0f)
					{
						usedMarkers[i]->Ignored = true;
					}
					// acceptable view angle
					else if (angle < 0.45f)
					{
						usedMarkers[i]->Ignored = true;
					}
					else
					{
						detectedMarkerCount++;
						rightVecSum += rights[i];
						upVecSum += ups[i];
						frontVecSum += fronts[i];
						positionVecSum += positions[i];
					}
				}


				float distanceSum = 0;
				unsigned int distanceCount = 0;

				if (detectedMarkerCount > 0)
				{
					cv::Vec3f rightVec = rightVecSum / (float)detectedMarkerCount;
					cv::Vec3f upVec = upVecSum / (float)detectedMarkerCount;
					cv::Vec3f frontVec = frontVecSum / (float)detectedMarkerCount;
					cv::Vec3f positionVec = positionVecSum / (float)detectedMarkerCount;

					global_Transform(0, 0) = rightVec[0];
					global_Transform(1, 0) = rightVec[1];
					global_Transform(2, 0) = rightVec[2];

					cv::Vec3f newUpVec = frontVec.cross(rightVec);

					global_Transform(0, 1) = newUpVec[0];
					global_Transform(1, 1) = newUpVec[1];
					global_Transform(2, 1) = newUpVec[2];

					cv::Vec3f newfrontVec = rightVec.cross(newUpVec);

					global_Transform(0, 2) = newfrontVec[0];
					global_Transform(1, 2) = newfrontVec[1];
					global_Transform(2, 2) = newfrontVec[2];

					global_Transform(0, 3) = positionVec[0];
					global_Transform(1, 3) = positionVec[1];
					global_Transform(2, 3) = positionVec[2];

					//std::cout << "global_Transform:" << std::endl << global_Transform << std::endl << std::endl;

					// last sanity check
					for (unsigned int i = 0; i < usedMarkers.size(); i++)
					{
						if (!usedMarkers[i]->Ignored)
						{
							const MarkerDescription* referencedMarker = nullptr;
							for (unsigned int j = 0; j < markerMap.size(); j++)
							{
								if ((int)markerMap[j].id == usedMarkers[i]->Id)
								{
									referencedMarker = &(markerMap[j]);
									break;
								}
							}

							if (referencedMarker != nullptr)
							{
								cv::Mat center = (cv::Mat)usedMarkers[i]->ModelMatrix * cv::Mat(cv::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
								cv::Mat worldPosition = (cv::Mat)global_Transform * cv::Mat(center);
								distanceSum += cv::norm(cv::Vec3f(worldPosition.at<float>(0, 0), worldPosition.at<float>(1, 0), worldPosition.at<float>(2, 0)) - referencedMarker->center);
								distanceCount++;
							}
						}
					}

					if (distanceCount > 0)
					{
						float meanDistance = distanceSum / (float)distanceCount;
						if (meanDistance > 5.0)
						{
							return false;
						}
						return true;
					}
					else
					{
						return false;
					}

					return true;
				}
			}
		}

		return false;
	}
}
