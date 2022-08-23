## My thoughts of the overall pipeline:

### tracker

1. sync between multiple camera inputs, for example ros message_filter will be needed and new callback func
2. still use *current frame* data structure, which doesn't change the overall appearance of the *track()*
3. rewritten *frame/keyframe*, which should be *multi-frame/keyframe* now
4. similarily, *reference keyframe* would be *reference multi-keyframe*, thus **frame-to-frame tracking would be the tightly-coupled optimization result by forming edge including all the cameras**
5. within each *multi-frame*, it stores only one timestamp, multiple images and corresponding keypoints, descriptors, landmarks... (all stored together in vectors with indices to distinguish)
6. to form the *virtual body frame*, *pose* for this frame will be added; meanwhile it will be also connected with *different camera frames* through extrinsic matrices (**fixed**)
7. due to the introduce of virtual body frame which has no real keypoints and landmarks, therefore all the g2o-related edges need to be re-written; moreover, the current implementation is deducted using generalized camera model, we will start with perspective camera model only for realsense, so new deduction should be derived (**biggest challenge for now**)
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
9. per-file check - file-level checked, doesn't see much change
10. work with odom/imu
11. major part summarization
12. rtabmap
13. GPNP
14. generic model


---
## File comparison

### include

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