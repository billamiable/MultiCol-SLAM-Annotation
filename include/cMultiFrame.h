/**
* This file is part of MultiCol-SLAM
*
* Copyright (C) 2015-2016 Steffen Urban <urbste at googlemail.com>
* For more information see <https://github.com/urbste/MultiCol-SLAM>
*
* MultiCol-SLAM is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* MultiCol-SLAM is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with MultiCol-SLAM . If not, see <http://www.gnu.org/licenses/>.
*/

/*
* MultiCol-SLAM is based on ORB-SLAM2 which was also released under GPLv3
* For more information see <https://github.com/raulmur/ORB_SLAM2>
* Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
*/

#ifndef MULTIFRAME_H
#define MULTIFRAME_H

#include "cMapPoint.h"
#include "DBoW2/DBoW2/BowVector.h"
#include "DBoW2/DBoW2/FeatureVector.h"
#include "cORBVocabulary.h"
#include "cMultiKeyFrame.h"
#include "cORBextractor.h"
#include "mdBRIEFextractorOct.h"
#include "cam_system_omni.h"

// external
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <unordered_map>


namespace MultiColSLAM
{
#define FRAME_GRID_ROWS 48
#define FRAME_GRID_COLS 64

	class cTracking;
	class cMapPoint;
	class cMultiKeyFrame;
	class cMultiKeyFrameDatabase;
	class cCamModelGeneral_;

	class cMultiFrame
	{
	public:
		cMultiFrame();
		cMultiFrame(const cMultiFrame &frame); // copy constructor

		cMultiFrame(const std::vector<cv::Mat>& images_,
			const double &timeStamp,
			std::vector<mdBRIEFextractorOct*> extractor,
			ORBVocabulary* voc,
			cMultiCamSys_& camSystem_,
			int imgCnt);

		ORBVocabulary* mpORBvocabulary;

		std::vector<mdBRIEFextractorOct*> mp_mdBRIEF_extractorOct;

		// images
		std::vector<cv::Mat> images;

		// Frame timestamp
		double mTimeStamp;

		// contains all cameras
		// camera model inside for projection
		cMultiCamSys_ camSystem;

		// Number of KeyPoints
		std::vector<int> N;
		// total number of Keypoints
		size_t totalN;

		// Vector of keypoints 2d
		// and corresponding bearing vectors 3d
		// for each camera
		// the total keypoints for all cameras, use keypoint_to_cam to distinguish between cameras
		std::vector<cv::KeyPoint> mvKeys;
		// one form of normalized plane (similar to [X,Y,Z] which indicates landmark 3D coodinate)
		// only difference is that this vector is normalized and have norm = 1
		std::vector<cv::Vec3d>    mvKeysRays; // 3D observation rays

		// Bag of Words Vector structures
		// difference: add two vectors here, thus it seems that BoW vector cannot be concat directly
		// TODO difference between mBowVec and mBowVecs
		DBoW2::BowVector mBowVec;
		std::vector<DBoW2::BowVector> mBowVecs;
		DBoW2::FeatureVector mFeatVec;
		std::vector<DBoW2::FeatureVector> mFeatVecs;
		// ORB descriptor, each row associated to a keypoint
		// descriptors for each camera
		std::vector<cv::Mat> mDescriptors;
		// learned descriptor masks TODO what's this?
		std::vector<cv::Mat> mDescriptorMasks;

		// MapPoints associated to keypoints, NULL pointer if not association
		// these are current multi-frame's landmarks, same as single-camera slam
		// but note that they are the world landmarks that are associated with current multi-frame
		// 				 also it corresponds to keypoints according to index for vector
		std::vector<cMapPoint*> mvpMapPoints;

		// Flag to identify outlier associations
		std::vector<bool> mvbOutlier;

		// TODO seems to be newly added data structure to speed up matching, similar to ours
		// Keypoints are assigned to cells in a grid 
		// to reduce matching complexity when projecting MapPoints
		std::vector<double> mfGridElementWidthInv;
		std::vector<double> mfGridElementHeightInv;
		// a grid for each camera, thus 3 vectors
		std::vector<std::vector<std::vector<std::vector<std::size_t> > > > mGrids;

		// Current and Next multi frame id
		static long unsigned int nNextId;
		long unsigned int mnId;

		cMultiKeyFrame* mpReferenceKF;

		void ComputeBoW();

		void UpdatePoseMatrices();

		// Check if a MapPoint is in the frustum of the camera 
		// and also fills variables of the MapPoint to be used by the tracking
		bool isInFrustum(int cam, cMapPoint* pMP, double viewingCosLimit);

		// Compute the cell of a keypoint (return false if outside the grid)
		bool PosInGrid(const int& cam, cv::KeyPoint &kp, int &posX, int &posY);

		std::vector<size_t> GetFeaturesInArea(const int& cam,
			const double &x, const double  &y,
			const double  &r,
			const int minLevel = -1, const int maxLevel = -1) const;

		// Scale Pyramid Info
		int mnScaleLevels;
		double mfScaleFactor;
		std::vector<double> mvScaleFactors;
		std::vector<double> mvLevelSigma2;
		std::vector<double> mvInvLevelSigma2;

		// Undistorted Image Bounds (computed once)
		static std::vector<int> mnMinX;
		static std::vector<int> mnMaxX;
		static std::vector<int> mnMinY;
		static std::vector<int> mnMaxY;

		static bool mbInitialComputations;

		// the following two variables together can relate any keypoint with its camera and local camera's keypoint id
		// this variable holds the mapping between keypoint ID and camera
		// it was observed in
		// [keypoint_id_in_all_keypoints : cam_id]
		std::unordered_map<size_t, int> keypoint_to_cam;
		// this variable holds the mapping between the continous indexing of all
		// descriptors and keypoints and the image wise indexes
		// it was observed in
		// [keypoint_id_in_all_keypoints : corresponding_local_image_keypoint_id]
		std::unordered_map<size_t, int> cont_idx_to_local_cam_idx;

		// pose-related operations are all overwritten
		cv::Matx<double, 4, 4> GetPose() { return camSystem.Get_M_t(); }
		// another form of Mt, in the format of 6d vector
		cv::Matx<double, 6, 1> GetPoseMin() { return camSystem.Get_M_t_min(); }

		void SetPose(cv::Matx44d& T) { camSystem.Set_M_t(T); }
		void SetPoseMin(cv::Matx61d& Tmin) { camSystem.Set_M_t_from_min(Tmin); }

		bool HavingMasks() { return masksLearned; }
		int DescDims() { return descDimension; }
		int Doing_mdBRIEF() { return mdBRIEF; }
		int GetImgCnt() { return imgCnt; }

	private:
		bool mdBRIEF;
		// TODO seems no place will set this as true, so we can omit it?
		bool masksLearned;
		int descDimension;
		int imgCnt;
	};
}
#endif // MULTIFRAME_H
