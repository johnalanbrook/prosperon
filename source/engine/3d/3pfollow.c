#include "3pfollow.h"
#include <math.h>

// void ThirdPersonFollow::_ready() {
//      Target = dynamic_cast<Spatial*>(get_node(TargetPath));
//      ExternalFrame = dynamic_cast<Spatial*>(get_node(ExternalFramePath));
// }

// void ThirdPersonFollow::_process(float delta_time) {

//      if (Target != nullptr) {
//              // Get the frame of reference
//              if (has_node(ExternalFramePath) && get_node(ExternalFramePath) != this)
//                      TFOR = dynamic_cast<Spatial*>(get_node(ExternalFramePath))->get_global_transform();
//              else
//                      TFOR = Transform(Basis(glm::vec3::RIGHT(), glm::vec3::UP(), glm::vec3::FORWARD()), glm::vec3::ZERO());

//              CalculateTargetOffset();
//              TargetPosition = CalculatePosition();

//              translate(to_local(FrameBasedVectorLerp(get_global_translation(), TargetPosition, PositionSpeeds, delta_time)));

//              set_rotation_degrees(get_rotation_degrees().linear_interpolate(RotationOffset, RotationSpeed * delta_time));
//      }

//      // For rotation
//      // TODO: Add rotation offset in properly
//      //this->SetActorRotation(FMath::Lerp(this->GetActorRotation(), TargetRotation, RotationSpeed * DeltaTime));
//      //this->set_rotation(glm::quat(this->get_rotation().slerp(RotationOffset, RotationSpeed * DeltaTime)).get_euler());

//      //set_global_rotation_degrees(get_global_rotation_degrees().linear_interpolate(RotationOffset, RotationSpeed * delta_time));

//      // TODO: Figure out how to translate rotation to a global rotation
// }

/*

glm::vec3 ThirdPersonFollow::CalculatePosition()
{
    glm::vec3 p1 = CalculateCenter();

    glm::vec3 p2 =
        XDirPosts ? GetPostsOffset(TFOR.Right(),
                                   FloatWidths.
                                   x) : GetExtentsOffset(TFOR.Right(),
                                                         FloatWidths.x,
                                                         TargetOffset.x,
                                                         AnchorWidths.x);
    glm::vec3 p3 =
        YDirPosts ? GetPostsOffset(TFOR.Up(),
                                   FloatWidths.
                                   y) : GetExtentsOffset(TFOR.Up(),
                                                         FloatWidths.y,
                                                         TargetOffset.y,
                                                         AnchorWidths.y);
    glm::vec3 p4 =
        ZDirPosts ? GetPostsOffset(TFOR.Back(),
                                   FloatWidths.
                                   z) : GetExtentsOffset(TFOR.Back(),
                                                         FloatWidths.z,
                                                         TargetOffset.z,
                                                         AnchorWidths.z);

    return p1 + p2 + p3 + p4;
}

glm::vec3 ThirdPersonFollow::CalculateCenter()
{
    return Target->get_global_translation() +
        TFOR.TransformDirection(Offset) + (mytransform->Back() * Distance);
}

glm::vec3 ThirdPersonFollow::
GetPostsOffset(const glm::vec3 & DirectionVector, float AnchorWidth)
{
    float dot = glm::dot(Target->Forward(), DirectionVector);

    return DirectionVector * (dot >= 0 ? AnchorWidth : AnchorWidth * -1);
}

glm::vec3 ThirdPersonFollow::
GetExtentsOffset(const glm::vec3 & DirectionVector, float AnchorWidth,
                 float TOffset, float Width)
{

    float negated_offset_sign = ((0 <= TOffset) - (TOffset < 0)) * -1.f;
    float TotalWidth = AnchorWidth + Width;

    if (glm::abs(TOffset) > TotalWidth
        && !glm::epsilonEqual(glm::abs(TOffset), TotalWidth, 0.5f))
        return DirectionVector * TotalWidth * negated_offset_sign;
    else {
        if (glm::abs(TOffset) >= AnchorWidth)
            return DirectionVector * AnchorWidth * negated_offset_sign;
        else
            return DirectionVector * TOffset * -1.f;
    }

    return glm::vec3(0.f);
}

glm::vec3 ThirdPersonFollow::FrameBasedVectorLerp(const glm::vec3 & From,
                                                  const glm::vec3 & To,
                                                  const glm::vec3 & Speeds,
                                                  float Tick)
{
    // Previously "FORTransform.TransformVector(Speeds)
    glm::vec3 TSpeed = glm::abs(TFOR.TransformDirection(Speeds));
    glm::vec3 TOffset = glm::abs(TFOR.TransformDirection(TargetOffset));
    glm::vec3 TAnchorWidths =
        glm::abs(TFOR.TransformDirection(AnchorWidths));
    glm::vec3 TFloatWidths =
        glm::abs(TFOR.TransformDirection(FloatWidths));



    // If these are true, that means to use the anchor speed instead of the normal speed.
    // True if the offset is beyond the anchor plus the width
    bool bUseX = GetLerpParam(TOffset.x, TAnchorWidths.x, TFloatWidths.x);
    bool bUseY = GetLerpParam(TOffset.y, TAnchorWidths.y, TFloatWidths.y);
    bool bUseZ = GetLerpParam(TOffset.z, TAnchorWidths.z, TFloatWidths.z);


    float xAlpha =
        glm::clamp((bUseX ? AnchorSpeed : TSpeed.x) * Tick, 0.f, 1.f);
    float yAlpha =
        glm::clamp((bUseY ? AnchorSpeed : TSpeed.y) * Tick, 0.f, 1.f);
    float zAlpha =
        glm::clamp((bUseZ ? AnchorSpeed : TSpeed.z) * Tick, 0.f, 1.f);

    return VectorLerpPiecewise(From, To,
                               glm::vec3(xAlpha, yAlpha, zAlpha));
}

int float_epsilon(mfloat_t a, mfloat_t b, mfloat_t e)
{
    return fabs(a - b) < e;
}

int lerpparam(float offset, float anchorwidth, float floatwidth)
{
    return (offset > (anchorwidth + floatwidth)) && !float_epsilon(anchorwidth + floatwidth, offset, 0.5f);
}

mfloat_t *vec3lerp(mfloat_t from[3], mfloat_t to[3], mfloat_t a[3])
{
    from[0] = from[0] + (to[0] - from[0]) * a[0];
    from[1] = from[1] + (to[1] - from[1]) * a[1];
    from[2] = from[2] + (to[2] - from[2]) * a[2];
}

void follow_calctargets()
{

}

void ThirdPersonFollow::CalculateTargets()
{
    // For translation
    TargetPosition = CalculatePosition();


    // For rotation
    // TODO: Check of this implementation is the same as UKismetMath FindLookAtRotation
    TargetRotation =
        RemoveLockedRotation(glm::quat
                             (mytransform->
                              get_global_transform()->get_origin() -
                              Target->
                              get_global_transform()->get_origin()));
}

follow_removelockedrot()
{

}

glm::quat ThirdPersonFollow::
RemoveLockedRotation(const glm::quat & CurrentRotation)
{
    glm::vec3 NewRotator;
    glm::vec3 CurrentRotator = glm::eulerAngles(CurrentRotation);
    //

    NewRotator.x =
        LockRoll ? mytransform->get_rotation().x : CurrentRotator.x;
    NewRotator.y =
        LockPitch ? mytransform->get_rotation().y : CurrentRotator.y;
    NewRotator.z =
        LockYaw ? mytransform->get_rotation().z : CurrentRotator.z;

    return glm::quat(NewRotator);
}

// The target offset is the value of where the target is compared to where he "should" be based on where
// the camera is. It is what the camera needs to move to be centered on the target
void ThirdPersonFollow::CalculateTargetOffset()
{
    glm::vec3 p1 =
        (mytransform->Forward() * Distance) +
        TFOR.TransformDirection(Offset) +
        mytransform->get_global_translation();
    glm::vec3 p2 =
        TFOR.InverseTransformDirection(Target->get_global_translation());
    glm::vec3 p3 = TFOR.InverseTransformDirection(p1);

    TargetOffset = p2 - p3;
}

void follow_targetoffset()
{
    mfloat_t p1[3], p2[3], p3[3] = {0};



}

*/
