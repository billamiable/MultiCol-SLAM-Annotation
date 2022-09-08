## Data member comparison


### ORB-SLAM2

#### Similar part

1. vocabulary for relocalization: ```ORBVocabulary* mpORBvocabulary``` - checked, BoW used ORB descriptor, similar, leave multi-camera diff
2. feature extractor: ```ORBextractor* mpORBextractorLeft, *mpORBextractorRight``` - checked, used diff extractor, essentially the same
3. timestamp: ```double mTimeStamp``` - checked, same
4. total number of Keypoints: ```int N``` - checked, similar except contains all keypoints from cameras
5. keypoint vector: ```std::vector<cv::KeyPoint> mvKeys, mvKeysRight``` - checked, similar as above
6. Bag of Words Vector structures: ```DBoW2::BowVector mBowVec; DBoW2::FeatureVector mFeatVec;``` - checked, similar as above
7. mappoints: ```std::vector<MapPoint*> mvpMapPoints;``` - checked, similar as above
8. Flag to identify outlier associations: ```std::vector<bool> mvbOutlier;``` - checked, similar as above
9. Keypoints are assigned to cells in a grid to reduce matching complexity when projecting MapPoints: ```static float mfGridElementWidthInv; static float mfGridElementHeightInv; std::vector<std::size_t> mGrid[FRAME_GRID_COLS][FRAME_GRID_ROWS];``` - checked, already in orbslam, similar as above
10. Current and Next Frame id: ```static long unsigned int nNextId; long unsigned int mnId;``` - checked, same
11. Reference Keyframe: ```KeyFrame* mpReferenceKF;``` - checked, same
12. Scale pyramid info: ```int mnScaleLevels; float mfScaleFactor; vector<float> mvScaleFactors; vector<float> mvLevelSigma2; vector<float> mvInvLevelSigma2;``` - checked, same
13. Undistorted Image Bounds (computed once): ```static float mnMinX; static float mnMaxX; static float mnMinY; static float mnMaxY;``` - checked, similar as above
14. initial computation bool: ```static bool mbInitialComputations``` - checked, same meaning
15. descriptor: ```cv::Mat mDescriptors, mDescriptorsRight;``` - checked, similar as above


#### Added part

1. intrinsic & distortion matrix (stereo): - checked, included in the camera system
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
2. threshold close/far points, close points are inserted from 1 view, far points are inserted as in the monocular case from 2 views: ```float mThDepth;``` - checked, used for stereo/rgbd, leave detail usage
3. Vector of keypoints (original for visualization) and undistorted (actually used by the system). In the stereo case, mvKeysUn is redundant as images must be rectified. In the RGB-D case, RGB images can be distorted: ```std::vector<cv::KeyPoint> mvKeysUn```; - checked, should use this, similar operation
4. Corresponding stereo coordinate and depth for each keypoint. "Monocular" keypoints have a negative value: ```std::vector<float> mvuRight; std::vector<float> mvDepth;``` - checked, keep for ours
5. camera pose: ```cv::Mat mTcw;``` - checked, included in camera system
6. Rotation, translation and camera center: ```cv::Mat mRcw; cv::Mat mtcw; cv::Mat mRwc; cv::Mat mOw; //==mtwc``` - checked, included in camera system
7. scale pyramid info: ```float mfLogScaleFactor; vector<float> mvInvScaleFactors;``` - checked, direct computation, should be simple



--- 

### MultiCol-SLAM

#### Similar part

1. vocabulary for relocalization: ```ORBVocabulary* mpORBvocabulary``` - checked, similar?
2. feature extractor: ```std::vector<mdBRIEFextractorOct*> mp_mdBRIEF_extractorOct``` - checked, diff
3. timestamp: ```double mTimeStamp``` - checked, same
4. total number of Keypoints: ```size_t totalN``` - checked, similar
5. keypoint vector: ```std::vector<cv::KeyPoint> mvKeys``` - checked, similar
6. Bag of Words Vector structures: ```DBoW2::BowVector mBowVec; DBoW2::FeatureVector mFeatVec;``` - checked, similar
7. mappoints: ```std::vector<cMapPoint*> mvpMapPoints;``` - checked, similar
8. Flag to identify outlier associations: ```std::vector<bool> mvbOutlier;``` - checked, similar
9. Keypoints are assigned to cells in a grid to reduce matching complexity when projecting MapPoints: ```std::vector<double> mfGridElementWidthInv; std::vector<double> mfGridElementHeightInv; std::vector<std::vector<std::vector<std::vector<std::size_t> > > > mGrids;``` - checked, similar
10. Current and Next Frame id: ```static long unsigned int nNextId; long unsigned int mnId;``` - checked, same
11. Reference Keyframe: ```cMultiKeyFrame* mpReferenceKF;``` - checked, same
12. Scale pyramid info: ```int mnScaleLevels; double mfScaleFactor; std::vector<double> mvScaleFactors; std::vector<double> mvLevelSigma2; std::vector<double> mvInvLevelSigma2;``` - checked, same
13. Undistorted Image Bounds (computed once): ```static std::vector<int> mnMinX; static std::vector<int> mnMaxX; static std::vector<int> mnMinY; static std::vector<int> mnMaxY;``` - checked, similar
14. initial computation bool: ```static bool mbInitialComputations``` - checked, not used
15. descriptor: ```std::vector<cv::Mat> mDescriptors;``` - checked, similar

#### Added part

1. images from all cameras: ```std::vector<cv::Mat> images``` - checked, used for feature extraction, keep for future usage
2. camera projection model and intrinsic & distortion matrix: ```cMultiCamSys_ camSystem``` - checked, add for multi-camera system， change for our setting as no generic model used
3. keypoint number in each camera: ```std::vector<int> N``` - checked, add for ours
4. BoW vectors: ```std::vector<DBoW2::BowVector> mBowVecs; std::vector<DBoW2::FeatureVector> mFeatVecs;``` - checked, not used
5. learned mask: ```std::vector<cv::Mat> mDescriptorMasks;``` - checked, used in generic camera model, leave details
6. others: ```bool mdBRIEF; bool masksLearned; int descDimension; int imgCnt;``` - checked, seems to have no big usage
7. bearing vectors 3d for keypoints: ```std::vector<cv::Vec3d> mvKeysRays;``` - checked, understand meaning, also used in original system but not defined explicitly
8. mapping between keypoint ID and camera [key_id : cam_id]: ```std::unordered_map<size_t, int> keypoint_to_cam;``` - checked, understand meaning, may have better form
9. mapping between the continous indexing of all descriptors and keypoints and the image wise indexes [cont_descriptor_id : local_image_id]: ```std::unordered_map<size_t, int> cont_idx_to_local_cam_idx;```


---

### Collaborative SLAM



---

## Member func comparison

**TODO**