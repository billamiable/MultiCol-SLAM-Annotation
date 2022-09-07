## Data member comparison


### ORB-SLAM2

#### Similar part

1. vocabulary for relocalization: ```ORBVocabulary* mpORBvocabulary``` - checked, BoW used ORB descriptor, similar, leave multi-camera diff
2. feature extractor: ```ORBextractor* mpORBextractorLeft, *mpORBextractorRight``` - diff
3. timestamp: ```double mTimeStamp``` - same
4. total number of Keypoints: ```int N``` - similar
5. keypoint vector: ```std::vector<cv::KeyPoint> mvKeys, mvKeysRight``` - diff
6. Bag of Words Vector structures: ```DBoW2::BowVector mBowVec; DBoW2::FeatureVector mFeatVec;```
7. mappoints: ```std::vector<MapPoint*> mvpMapPoints;```
8. Flag to identify outlier associations: ```std::vector<bool> mvbOutlier;```
9. Keypoints are assigned to cells in a grid to reduce matching complexity when projecting MapPoints: ```static float mfGridElementWidthInv; static float mfGridElementHeightInv; std::vector<std::size_t> mGrid[FRAME_GRID_COLS][FRAME_GRID_ROWS];```
10. Current and Next Frame id: ```static long unsigned int nNextId; long unsigned int mnId;```
11. Reference Keyframe: ```KeyFrame* mpReferenceKF;```
12. Scale pyramid info: ```int mnScaleLevels; float mfScaleFactor; vector<float> mvScaleFactors; vector<float> mvLevelSigma2; vector<float> mvInvLevelSigma2;```
13. Undistorted Image Bounds (computed once): ```static float mnMinX; static float mnMaxX; static float mnMinY; static float mnMaxY;```
14. initial computation constant: ```static bool mbInitialComputations```


#### Added part

1. intrinsic & distortion matrix (stereo): 
   1) ```cv::Mat mK;```
   2) ```static float fx;```
   3) ```static float fy;```
   4) ```static float cx;```
   5) ```static float cy;```
   6) ```static float invfx;```
   7) ```static float invfy;```
   8) ```cv::Mat mDistCoef;```
   9) Stereo baseline multiplied by fx: ```float mbf;```
   10) Stereo baseline in meters: ```float mb;```
2. threshold close/far points, close points are inserted from 1 view, far points are inserted as in the monocular case from 2 views: ```float mThDepth;```
3. Vector of keypoints (original for visualization) and undistorted (actually used by the system). In the stereo case, mvKeysUn is redundant as images must be rectified. In the RGB-D case, RGB images can be distorted: ```std::vector<cv::KeyPoint> mvKeysUn```;
4. Corresponding stereo coordinate and depth for each keypoint. "Monocular" keypoints have a negative value: ```std::vector<float> mvuRight; std::vector<float> mvDepth;```
5. descriptor: ```cv::Mat mDescriptors, mDescriptorsRight;```
6. camera pose: ```cv::Mat mTcw;```
7. scale pyramid info: ```float mfLogScaleFactor; vector<float> mvInvScaleFactors;```
8. Rotation, translation and camera center: ```cv::Mat mRcw; cv::Mat mtcw; cv::Mat mRwc; cv::Mat mOw; //==mtwc```



--- 

### MultiCol-SLAM

#### Similar part

1. vocabulary for relocalization: ```ORBVocabulary* mpORBvocabulary``` - checked
2. feature extractor: ```std::vector<mdBRIEFextractorOct*> mp_mdBRIEF_extractorOct``` - diff
3. timestamp: ```double mTimeStamp``` - same
4. total number of Keypoints: ```size_t totalN``` - similar
5. keypoint vector: ```std::vector<cv::KeyPoint> mvKeys``` - diff
6. Bag of Words Vector structures: ```DBoW2::BowVector mBowVec; DBoW2::FeatureVector mFeatVec;```
7. mappoints: ```std::vector<cMapPoint*> mvpMapPoints;```
8. Flag to identify outlier associations: ```std::vector<bool> mvbOutlier;```
9. Keypoints are assigned to cells in a grid to reduce matching complexity when projecting MapPoints: ```std::vector<double> mfGridElementWidthInv; std::vector<double> mfGridElementHeightInv; std::vector<std::vector<std::vector<std::vector<std::size_t> > > > mGrids;```
10. Current and Next Frame id: ```static long unsigned int nNextId; long unsigned int mnId;```
11. Reference Keyframe: ```cMultiKeyFrame* mpReferenceKF;```
12. Scale pyramid info: ```int mnScaleLevels; double mfScaleFactor; std::vector<double> mvScaleFactors; std::vector<double> mvLevelSigma2; std::vector<double> mvInvLevelSigma2;```
13. Undistorted Image Bounds (computed once): ```static std::vector<int> mnMinX; static std::vector<int> mnMaxX; static std::vector<int> mnMinY; static std::vector<int> mnMaxY;```
14. initial computation constant: ```static bool mbInitialComputations```


#### Added part

1. images from all cameras: ```std::vector<cv::Mat> images```
2. camera projection model and intrinsic & distortion matrix: ```cMultiCamSys_ camSystem```
3. keypoint number in each camera: ```std::vector<int> N```
4. bearing vectors 3d for keypoints: ```std::vector<cv::Vec3d> mvKeysRays;```
5. BoW vectors: ```std::vector<DBoW2::BowVector> mBowVecs; std::vector<DBoW2::FeatureVector> mFeatVecs;```
6. descriptor and learned mask: ```std::vector<cv::Mat> mDescriptors; std::vector<cv::Mat> mDescriptorMasks;```
7. mapping between keypoint ID and camera [key_id : cam_id]: ```std::unordered_map<size_t, int> keypoint_to_cam;```
8. mapping between the continous indexing of all descriptors and keypoints and the image wise indexes [cont_descriptor_id : local_image_id]: ```std::unordered_map<size_t, int> cont_idx_to_local_cam_idx;```
9. others: ```bool mdBRIEF; bool masksLearned; int descDimension; int imgCnt;```


---

### Collaborative SLAM



---

## Member func comparison

**TODO**