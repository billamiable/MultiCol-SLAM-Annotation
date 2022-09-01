## My thoughts of the overall pipeline:

### tracker

1. sync between multiple camera inputs, for example ros message_filter will be needed and new callback func
2. still use *current frame* data structure, which doesn't change the overall appearance of the *track()*
3. rewritten *frame/keyframe*, which should be *multi-frame/keyframe* now
4. similarily, *reference keyframe* would be *reference multi-keyframe*, thus **frame-to-frame tracking would be the tightly-coupled optimization result by forming edge including all the cameras**
5. within each *multi-frame*, it stores only one timestamp, multiple images and corresponding keypoints, descriptors, landmarks... (all stored together in vectors with indices to distinguish)
6. to form the *virtual body frame*, *pose* for this frame will be added; meanwhile it will be also connected with *different camera frames* through extrinsic matrices (**fixed**)
7. due to the introduce of virtual body frame which has no real keypoints and landmarks, therefore all the g2o-related edges need to be re-written; moreover, the current implementation is deducted using generalized camera model, we will start with perspective camera model only for realsense (**turns out that we may be able to avoid the deduction of g2o since the only change for formualtion is the introduction of extrinsic matrix**)
8. during optimization, instead of only adding edge for single camera, we need to add edges for all the cameras; meanwhile, we may also need to add edges for nearby cameras which have view overlap (**optional**)
9. initialization stage should be designed separately: **for realsense camera, only one frame is enough for initialization, therefore maybe just directly initialize by itself for each camera at a certain starting point**
10. relocalization (**use OpenGV for re-localization (GP3P) and relative orientation estimation during initialization (Stewenius)**)


### tracker-server communication (**in the first stage, we wil only achieve the tracker part**)

1. message type should support *multi-frame* data structure


### server (**in the first stage, we will only achieve the tracker part**)

1. **loop closure should support multi-camera slam**, which should solve the problem of map merge not happening when robot moves in opposite directions
2. **map merge should also support multi-camera slam**, policy should be designed to solve the problem

---

## Several things to check:

1. diff between my thoughts and real implementation
2. diff between orbslam2 and multicol-slam (per-file diff)
3. find out if any major changing parts are missing
4. generic camera model definition (**to understand why camera model and camera extrinsic matrices will be added as vertex in optimization graph, maybe refer to other works that may simiplify the logic**)
5. g2o edge formulation and deduction (**to find out how much effort is needed to deduct by our own**)
6. map db and graph node organization structure (**to understand if large changes are needed for this part**)


---

## Comparison study

**for orbslam2, there are serveral edges:**
- EdgeSE3ProjectXYZ & EdgeStereoSE3ProjectXYZ (BA)
- EdgeSE3ProjectXYZOnlyPose & EdgeStereoSE3ProjectXYZOnlyPose (pose optimizer)
- EdgeSim3 (essential graph)
- EdgeSim3ProjectXYZ & EdgeInverseSim3ProjectXYZ (optimize sim3)

1. why pose optimizer needs extra edge? can't we just fix landmark? A: yes, it is what multicol-slam does and we can change the orbslam formulation as well. so essential multicol-slam eliminates pose optimizer related edge.
2. difference between edgesim3 with others? doesn't multicol-slam use essential graph? A: it is like pose graph edge; it uses, but it doesn't need to define new jacobian matrix since only two poses are used.
3. doesn't multicol-slam use optimizer_sim3? A: yes; it uses, **but it doesn't need to define new jacobian matrix. It's interesting and we may be able to refer to this formulation instead of se3 one. check if the assumption is valid or not? in this case, no jacobian deduction is needed!!! after comparison, I think the assumption is correct since in essence it is reprojection error except for different camera projection method!** (why multicol-slam needs to define new edge???)

---

## TODO-List

1. g2o doublecheck (sync with George) - derived jacobian deduction, only multiply fixed extrinsic, should be fine
2. match pair check - checked, remember that mvpMapPoints actually means matched landmarks
3. way to find matches - checked, simply loop through all landmarks to match by finding corresponding camera
4. multi-frame data structure check - checked, BoW and descriptor related stuff needs further examination
5. BoW & descriptor check - checked, descriptor uses vector and BoW used concated descriptors for all cameras
6. map db & graph node check - both treat multi-camera as one, same for map db, graph node is inside keyframe
7. initialization check - checked, target on realsense and follow the orbslam2 stereoinitialization pipeline
8. opengv check - checked, relocalization needs better algorithm, leaves GPNP (triangulation) for future 
9. per-file check - file-level checked, finished all the major module check, overall meet expectation
10. major part summarization - summarized all known modules, except compatible-related stuff
11. work with odom/imu - checked, define good multi-frame and set default virtual body frame as first camera's image frame
12. localization mode - how to satisfy the logic and data structure (degeneralize multi-(key)frame to (key)frame)
13. detect loop, map merge
14. rtabmap
15. GPNP
16. generic model


---
## File comparison

newly added (with cpp file)
- cam_model_omni.h - checked, generic camera model definition, no use for now
- cam_system_omni.h - checked, multi-camera system definition, need to add
- cMapPublisher.h - checked, essentially same as MapDrawer.h
- cMultiFramePublisher.h - checked, essentially same as FrameDrawer.h
- g2o_MultiCol_sim3_expmap.h - checked, need to add
- g2o_Multicol_vertices_edges.h - checked, need to add partly
- mdBRIEFextractor.h (sorely) - checked, no use for now
- mdBRIEFextractor1.h (sorely) - checked, no use for now
- mdBRIEFextractorOct.h - checked, no use for now
- misc.h - checked, all kinds of definitions, easy to reproduce
- cOptimizerLoopStuff.cpp - checked, belong to optimizer, need to add

deleted (with cpp file)
- FrameDrawer.h - checked, see above
- MapDrawer.h - checked, see above
- PnPsolver.h - checked, they don't use traditional PnP

---

## Detailed file comparison

- misc - checked, no big deal
- cConverter - checked, no big deal
- cORBextractor - checked, nothing to change right now
- cORBVocabulary - checked, nothing to change right now
- cORBmatcher - checked, overall doable, leaves out new descriptor distance and search method
- cSim3Solver - checked, overall the same, except used refine() in pnp_solver, need to change
- cam_system_omni - checked, we can avoid majority since no use for generic camera model
- cMappoint - checked, overall doable, more operations for observation, descriptor-related stuff no big deal - mask not used & CurrentDescriptor not used
- cMultiFrame - will do
- cMultiKeyFrame - will do
- cMultiKeyFrameDatabase - checked, same as BoW_db, func are the same, leave detailed algorithm
- cMap - checked, overall same, leave detailed implementation
- g2o_MultiCol_sim3_expmap - checked, adjust accordingly
- g2o_MultiCol_vertices_edges - checked, adjust accordingly
- cOptimizer - checked, more adjustments are needed
- cOptimizerLoopStuff - checked, more adjustments are needed
- cSystem - checked, overall the same, less mode supported, leave save traj format
- cTracking - checked, diff: forcerelocalization (done, don't know why), initialization (not much to do), trackpreviousframe vs trackreferencekeyframe (new window search, don't know why, not difficult to implement), update reference(points, keyframes) vs local (map, points, keyframes) (done, no big diff with changes in local map definition), SearchReferencePointsInFrustum vs SearchLocalPoints (done, no big diff, similar to before)
- cLocalMapping - checked, overall the same, changes in detailed func
- cLoopClosing - checked, overall the same except not using global BA, changes in detailed func


---

## Detailed case study for Multi-frame data structure

Take bool Tracking::TrackWithMotionModel() as an example, let's just make a detailed comparison:

1. mCurrentFrame.SetPose:
   1) don't know why velocity multiplication is different?
   2) camSystem (including MtMc - pose for each camera) need to be introduced 
2. matcher.SearchByProjection:
   1) threshold and search times are different, which indicates changes needed for multi-camera slam (performance tuning time)
   2) extra work to make the code compatible for mono and stereo/rgbd
   3) api changes to add camera system as input as well
   4) changes for condition check (performance tuning)
   5) descriptor (it's aimed for landmark) management method - newly added method for MultiKeyframe?
3. PoseOptimization
   1) g2o early stop strategy
   2) extra work to make the code compatible for mono and stereo/rgbd


---

## Compatible with odom and imu module

First, list potential parts that need to be changed:

### Odom

- get odom data (frame definition) - checked, finally you are using the relative pose of specific frame, therefore to obtain the value for other frames, only multiply by the extrinsic matrix should be fine.
- mono init - checked, no use in multi-camera support, make it compatible with multi-frame class
- g2o definition (pose graph edge) - checked, don't need to change
- mapping mode (local ba) - checked, defined for virtual body frame should be fine
- localization mode (pose optimizer) - checked, same as local ba
- tracking lost logic - checked, should be similar, essentially is the relative pose


### Imu

- preintegration - checked, should be fine, as long as world coordinate is the same as single-camera image frame
- g2o definition (vertex and edge) - checked, changes are mainly on the keyframe vertex interface to accept multi-keyframe
- imu initialization (coordinate transform) - checked, coordinate transform is correct as long as world coordinate is same 
- local inertial ba - checked, no big change except feeding multi-keyframe instead of keyframe
- tracking lost logic - checked, same as preintegration, coordinate is VIP

All the things would be affected if we can have a good design of the multi-frame data structure, which can degeneralize into traditional frame with the right coordinate (i.e. set default body frame as first camera's image frame) when only one camera is used as the input.


---

## Compatible with localization mode

Similarly, list all items that might need to change:

- tracker-server communication
  1) relocalization request
  2) relocalization result
  3) server landmark request
  4) server landmark result
- tracker send relocalization request
- server perform relocalization - checked, won't change if we don't change server for now
- tracker initialization by server
- tracker request server landmark
- server obtain server landmark - checked, won't change if we don't change server for now
- tracker receive server landmark
- tracker use server landmark while tracking