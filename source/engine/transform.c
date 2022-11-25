#include "transform.h"
#include <string.h>

struct mTransform MakeTransform(mfloat_t pos[3], mfloat_t rotation[4],
				float scale)
{
    struct mTransform newT;
    memcpy(newT.position, pos, sizeof(*pos));
    memcpy(newT.rotation, rotation, sizeof(*rotation));
    newT.scale = scale;
    return newT;
}

mfloat_t *trans_forward(mfloat_t * res,
			const struct mTransform *const trans)
{
    // YughLog(0, SDL_LOG_PRIORITY_WARN, "Rotation is %f", trans->rotation[0]);
    return vec3_rotate_quat(res, FORWARD, trans->rotation);
}

mfloat_t *trans_back(mfloat_t * res, const struct mTransform *trans)
{
    return vec3_rotate_quat(res, BACK, trans->rotation);
}

mfloat_t *trans_up(mfloat_t * res, const struct mTransform *trans)
{
    return vec3_rotate_quat(res, UP, trans->rotation);
}

mfloat_t *trans_down(mfloat_t * res, const struct mTransform *trans)
{
    return vec3_rotate_quat(res, DOWN, trans->rotation);
}

mfloat_t *trans_right(mfloat_t * res, const struct mTransform *trans)
{
    return vec3_rotate_quat(res, RIGHT, trans->rotation);
}

mfloat_t *trans_left(mfloat_t * res, const struct mTransform *trans)
{
    return vec3_rotate_quat(res, LEFT, trans->rotation);
}

#include "nuke.h"

void trans_drawgui(struct mTransform *T)
{
    nk_property_float3(ctx, "Position", -1000.f, T->position, 1000.f, 1.f, 1.f);
    nk_property_float3(ctx, "Rotation", 0.f, T->rotation, 360.f, 1.f, 0.1f);
    nk_property_float(ctx, "Scale", 0.f, &T->scale, 1000.f, 0.1f, 0.1f);
}
