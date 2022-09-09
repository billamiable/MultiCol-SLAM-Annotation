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
8. mapping between keypoint ID and camera [keypoint_id_in_all_keypoints : cam_id]: ```std::unordered_map<size_t, int> keypoint_to_cam;``` - checked, understand meaning, may have better form, used together with the below one
9. mapping between the continous indexing of all descriptors and keypoints and the image wise indexes [keypoint_id_in_all_keypoints : corresponding_local_image_keypoint_id]: ```std::unordered_map<size_t, int> cont_idx_to_local_cam_idx;``` - checked, understand meaning, may have better form, used together with the above one


---

### Collaborative SLAM



---

## Member func comparison

**TODO**

---

## Thoughts on Our Multi-Frame Design

--- 

### Method 1: try keeping the original data structure

In this form, whenever we can simply stack stuff onto the original data structure, we will do so. 

**Examples:**

- keypoints: ```std::vector<cv::KeyPoint>``` - return+pnp*2(?)+undist_kp+kp_to_bearing+homography(?)+fundemental(?)+assign_to_grid+stereo(?)+orb_extractor
- mappoints: ```std::vector<MapPoint*>``` - pnp*2
- outlier associations: ```std::vector<bool>```
- stereo coordinates and depths for keypoints: ```std::vector<float>```
- 3d bearing vectors for keypoints: ```std::vector<cv::Vec3d>``` - pnp*2+kp_to_bearing
- BoW vector structures: ```DBoW2::BowVector```,``` DBoW2::FeatureVector```

All influenced: kf_ctor

**Pros**
   
1. All related APIs don't need to change
2. Some algorithm directly use stacked data structure, for example, BoW vector operation.


**Cons**

1. Need extra data structure to store the relationship between each keypoint in big data structure with corresponding camera and local camera's keypoint id, which looks not elegant.
2. Not all data structures can be simply stacked, for example:
   - descriptor: ```std::vector<cv::Mat>``` vs ```cv::Mat```
   - Keypoints are assigned to cells in a grid: ```std::vector<std::vector<std::vector<std::vector<std::size_t> > > > mGrids``` vs ```std::vector<std::size_t> mGrid[FRAME_GRID_COLS][FRAME_GRID_ROWS]```
   - Undistorted Image Bounds: ```static std::vector<int> mnMinX; static std::vector<int> mnMaxX; static std::vector<int> mnMinY; static std::vector<int> mnMaxY;``` vs ```static float mnMinX; static float mnMaxX; static float mnMinY; static float mnMaxY;```

---

### Method 2: use two-fold data structure

In this form, to represent data structures for multiple cameras, we will simply create a vector-of-vector/ map-of-vector data structure.

**Examples:**

- keypoints: ```std::vector<std::vector<cv::KeyPoint>>``` or ```std::unordered_map<int, std::vector<cv::KeyPoint>>```
- mappoints: ```std::vector<std::vector<MapPoint*>>``` or ```std::unordered_map<int, std::vector<MapPoint*>>```
- BoW vector structures: ```std::vector<DBoW2::BowVector>```,```std::vector<DBoW2::FeatureVector>``` or ```std::unordered_map<int, DBoW2::BowVector>```,```std::unordered_map<int, DBoW2::FeatureVector>```
- ...

**Pros**

1. All data structures follow the same scheme, the code is elegant and easier to understand
2. Don't need extra data structures to store the relationship between each keypoint in big data structure with corresponding camera and local camera's keypoint id
3. Differentiate with MultiCol-SLAM and avoid potential license issue


**Cons**

1. All API will need to change accordingly which requires much effort (unless outside function call, the data structure is changed to the form of simple stacking, which may be ugly).
2. If API is changed, inside the function call, we may still need to stack data if needed.

---

**TODO List:**

1. check places of API change - what can't not be aviod is that how we use the data inside the function
2. investigate compiler_flag based code - get brief idea, but may be more complicated for multi-camera use
3. Yin Han's suggestion: 
   1) derived class for vector, no need to change api - no recommended by Xin as too many needs to be taken into consideration, easier to use self-defined data type (i.e. vector of vector); 
   2) use compiler flag to determine which data structure to use and how to process inside function
4. Pay attention to whether the api needs two copy
5. Pay attention to code compatiblity and step-by-step implementation
6. talk with Xin and see if he has better solution

---

### Xin's suggestion

**Overall logic:**

1. Define class of Multi-Frame, let Frame class derive from Multi-Frame (Frame is a special case of Multi-Frame so Frame should be derived case)
2. Use API oriented programming: 
    - Frame-related implementations: for all member functions in the original frame class, it will be defined in the derived class as before and set as virtual function in the base class.
    - New implementations: for all member functions that will be added for multi-camera usage, it will be defined in the base class and set to empty in the derived class.
3. Modify the overall pipeline code to replace Frame class with Multi-Frame class by simply changing the variable names as all member functions are using the same func name.

**Left-over questions:**

1. How to know use base or derived class in the code?
   - Use base pointer to point at derived object, i.e. when creating frame object, use input camera num to decide creating base object or derived object.
   - If created derived object, then everything should be the same;
   - If created base object, then use all implementations in the base class (keep another modified copy of derived implementation in the base class).

2. How to remove the 2 copies and only leave 1 at last?
   - Finally only the base class's implementation will be kept as it includes the more complicated operations.
   - Much likely after few changes, the code should work with the special case of single camera. (keep in mind to make the difference as small as possible for base and derived class implementations)
   - After that, the derived class can be removed totally.

3. How to deal with data member?
    - Derived class contains the single camera's data, while base class contains N-times data.
    - Therefore, essentially the base class uses vector of vector and the derived class also use this data type except only having 1 element inside.


---

### Ways to implement

1. Data member: use map of vector data structure
2. Member function: use the same name for all member functions
3. **Keep in mind how to minimize the changing effort (need more thoughts here)**

**Questions**

1. How to deal with Keyframe class that are used within Frame class?
   - Doesn't matter, since right now we are keeping Keyframe as it was. And we can still compile before implementing Multi-Keyframe.

2. What about the Multi-Frame ctor? Should we leave their implementation as empty?
   - Instead, we can move all the Frame ctor to the Multi-Frame ctor since it is sth we will ultimately do.
   - For the first phase, we can only include the above definitions; later we can add Multi-Frame specific logics.
   - When defining the ctor for the Frame class, remember to first call the Multi-Frame ctor (reference to *univloc_tracking_module*), and then we should need to do nothing more inside Frame ctor.

3. For all the member functions defined in Frame class, we will set the corresponding virtual function in Multi-Frame class and override in Frame class?
   - Correct, that's what I've done.

4. Any better way to get the first element of data member from Multi-Frame class? Naive way is to change all the places that used the data member to index the very first element.
   - Still keep the data inside Frame class and use the N-times data to initialize the single-data during frame ctor.
   - In this way, we can avoid the ugly changes that would occur everywhere.
   - But eventually this shouldn't be a problem since we would simply remove all the frame-related parts.

5. For map of vector, is it less inefficint than vector of vector from the memory perspective? For example, when it comes to data copy.
   - To be investigated.

