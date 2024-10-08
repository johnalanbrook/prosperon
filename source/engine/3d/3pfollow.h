#ifndef THIRDPERSONFOLLOW_H
#define THIRDPERSONFOLLOW_H

#include "transform.h"
#include "HandmadeMath.h"

/*

class ThirdPersonFollow {

  public:
    enum CameraType {
        STATIONARY,
        TRANSLATING,
        ROTATING,
        SPLINE
    };

    enum CameraTransition {
        NONE,
        CROSSDISSOLVE,
        WIPE,
        DIP
    };

    enum FrameOfReference {
        LOCAL,
        WORLD,
        EXTERNAL
    };

     ThirdPersonFollow() {
        // Rotation
        RotationSpeed = 10.0f;
        LockPitch = LockYaw = LockRoll = true;

        XDirPosts = false;
        YDirPosts = false;
        ZDirPosts = false;


        // Translation
        //FloatWidths = AnchorWidths = CenterVector = glm::vec3(0, 0, 0);
        PositionSpeeds = glm::vec3(2.f, 2.f, 2.f);
        //TranslationScales = glm::vec3(1, 1, 1);

        // Frame settings
        Offset = glm::vec3(0.f, 0.f, 0.f);
        Distance = 10;

        AnchorSpeed = 80;
    } ~ThirdPersonFollow() {
    }

    Transform *mytransform;

    // An actor that can be given for the camera to base its movement around
    // instead of itself. Makes most sense for this to be stationary
    Transform *ExternalFrame = nullptr;


    void SetExternalFrame(Transform * val) {
        ExternalFrame = val;
    }

    // The target the camera "looks" at, used for calculations
    Transform *Target = nullptr;

    void SetTarget(Transform * val) {
        Target = val;
    }

    // Offset from the target
    glm::vec3 Offset;

    // How far away should the camera act from the target
    float Distance;

    ///////////////////////////////////////////////////////////////////////////
    // Translation variables. In this mode, the camera doesn't rotate to look
    // at the target, it only moves around the world.
    ///////////////////////////////////////////////////////////////////////////

    /// "Posts" for each direction in 3D space. These are items that the target
    /// is allowed to move within without the camera following along.
    bool XDirPosts;
    bool YDirPosts;
    bool ZDirPosts;

    /// The range in ecah direction the camera floats. While within this range,
    /// the camera will smoothly glide to the desired position.
    glm::vec3 FloatWidths;

    /// The clamp range for each direction. If the camera reaches this range,
    /// it will stick and not move any further.
    glm::vec3 AnchorWidths;

    /// When floating to the target, the speed to float.
    glm::vec3 PositionSpeeds;

    //////////////////////////////////////////////////////////////////////////
    // Rotation variables. Used for the camera's rotation mode, where it
    // follows the Target without translating.
    //////////////////////////////////////////////////////////////////////////

    /// Variables to lock its rotation in any of the three directions
    bool LockRoll;
    bool LockPitch;
    bool LockYaw;

    glm::vec3 RotationOffset;

    /// The speed of rotation
    float RotationSpeed;

  private:
    void CalculateTargetOffset();

    // Transform of frame of reference
    Transform TFOR;

    /// The calculated offset based on frames of reference
    glm::vec3 TargetOffset;

    glm::vec3 TargetPosition;

    glm::quat TargetRotation;


    void CalculateTargets();

    // Calculates

    glm::vec3 CalculatePosition();

    glm::vec3 CalculateCenter();

    /// Given a direction and width, find the offsets.
    glm::vec3 GetPostsOffset(const glm::vec3 & DirectionVector,
                             float AnchorWidth);

    /// Given anchors, what's the anchor width?
    glm::vec3 GetExtentsOffset(const glm::vec3 & DirectionVector,
                               float AnchorWidth, float TOffset,
                               float Width);

    glm::quat RemoveLockedRotation(const glm::quat & CurrentRotation);

    glm::vec3 FrameBasedVectorLerp(const glm::vec3 & From,
                                   const glm::vec3 & To,
                                   const glm::vec3 & Speeds, float Tick);

    glm::vec3 VectorLerpPiecewise(const glm::vec3 & From,
                                  const glm::vec3 & To,
                                  const glm::vec3 & Alpha);

    bool GetLerpParam(const float Offst, const float AnchorWidth,
                      const float FloatWidth);

    /// Set to a value that gives good clamping, smoothly. Activates when
    /// the target is out of range.
    float AnchorSpeed;

};

*/

#endif
