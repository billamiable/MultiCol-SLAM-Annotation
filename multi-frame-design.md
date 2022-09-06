## Data member comparison


### ORB-SLAM2

#### Similar part

1. vocabulary for relocalization: ```ORBVocabulary* mpORBvocabulary``` - same?
2. feature extractor: ```ORBextractor* mpORBextractorLeft, *mpORBextractorRight``` - diff
3. timestamp: ```double mTimeStamp``` - same
4. total number of Keypoints: ```int N``` - similar
5. keypoint vector: ```std::vector<cv::KeyPoint> mvKeys, mvKeysRight``` - diff


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
4. 



--- 

### MultiCol-SLAM

#### Similar part

1. vocabulary for relocalization: ```ORBVocabulary* mpORBvocabulary``` - same?
2. feature extractor: ```std::vector<mdBRIEFextractorOct*> mp_mdBRIEF_extractorOct``` - diff
3. timestamp: ```double mTimeStamp``` - same
4. total number of Keypoints: ```size_t totalN``` - similar
5. keypoint vector: ```std::vector<cv::KeyPoint> mvKeys``` - diff


#### Added part

1. images from all cameras: ```std::vector<cv::Mat> images```
2. camera projection model and intrinsic & distortion matrix: ```cMultiCamSys_ camSystem```
3. keypoint number in each camera: ```std::vector<int> N```


---

### Collaborative SLAM



---

## Member func comparison