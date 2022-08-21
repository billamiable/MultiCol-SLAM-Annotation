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
4. generic camera model definition (**to understand why camera model will be added as a vertex in optimization graph**)
5. g2o edge formulation and deduction (**to find out how much effort is needed to deduct by our own; meanwhile understand why loop-related edge doesn't need to define new jacobian matrix?**)
6. map db and graph node organization structure (**to understand if large changes are needed for this part**)


---

## Comparison study

**for orbslam2, there are serveral edges:**
- EdgeSE3ProjectXYZ & EdgeStereoSE3ProjectXYZ (BA)
- EdgeSE3ProjectXYZOnlyPose & EdgeStereoSE3ProjectXYZOnlyPose (pose optimizer)
- EdgeSim3 (essential graph)
- EdgeSim3ProjectXYZ & EdgeInverseSim3ProjectXYZ (optimize sim3)

1. why pose optimizer needs extra edge? can't we just fix landmark? A: yes, it is what multicol-slam does and we can change the orbslam formulation as well. so essential multicol-slam eliminates pose optimizer related edge.
2. difference between edgesim3 with others? doesn't multicol-slam use essential graph? A: it is like pose graph edge; it uses.
3. doesn't multicol-slam use optimizer_sim3? A: yes; it uses.