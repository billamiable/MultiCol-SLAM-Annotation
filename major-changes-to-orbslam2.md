## My thoughts of the overall pipeline:

### tracker

1. still use *current frame* data structure, which doesn't change the overall appearance of the *track()*
2. rewritten *frame/keyframe*, which should be *multi-frame/keyframe* now
3. within each *multi-frame*, it stores only one timestamp, multiple images and corresponding keypoints, descriptors, landmarks... (all stored together in vectors with indices to distinguish)
4. to form the *virtual body frame*, *pose* for this frame will be added; meanwhile it will be also connected with different cameras through extrinsic matrices (**fixed?**)
5. due to the introduce of virtual body frame which has no real keypoints and landmarks, therefore all the g2o-related edges need to be re-written (**doublecheck?**)
6. similar to item 2, *reference keyframe* would be *reference multi-keyframe*, thus **frame-to-frame tracking would be the combination of each camera's tracking result?**
7. during optimization, instead of only adding edge for single camera, we need to add edges for all the cameras; meanwhile, we may also need to add edges for nearby cameras which have view overlap
8. initialization stage should be designed separately, **maybe use one camera to initialize and others just adds on to it? but how?**


### tracker-server communication

1. message type should support *multi-frame* data structure


### server

1. **loop closure should support multi-camera**, which should solve the problem of map merge not happening when robot moves in opposite directions

---

## Several things to check:

1. diff between my thoughts and real implementation
2. diff between orbslam2 and multicol-slam (per-file diff)