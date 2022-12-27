#include "anim.h"
#include "stb_ds.h"
#include "log.h"

struct anim make_anim() {
    struct anim a = {0};
    a.interp = 1;

    return a;
}

struct anim anim_add_keyframe(struct anim a, struct keyframe key) {
    arrput(a.frames, key);

    return a;
}

double interval(struct keyframe a, struct keyframe b, double t) {
    return (t - a.time) / (b.time - a.time);
}

double near_val(struct anim anim, double t) {
    for (int i = 0; i < arrlen(anim.frames) - 1; i++) {

        if (t > anim.frames[i+1].time)
            continue;

        return (interval(anim.frames[i], anim.frames[i+1], t) >= 0.5f ? anim.frames[i+1].val : anim.frames[i].val);
    }
}

double lerp_val(struct anim anim, double t) {

    for (int i = 0; i < arrlen(anim.frames) - 1; i++) {
        if (t > anim.frames[i+1].time)
            continue;

        double intv = interval(anim.frames[i], anim.frames[i+1], t);
        return ((1 - intv) * anim.frames[i].val) + (intv * anim.frames[i+1].val);
    }

    return arrlast(anim.frames).val;
}

double cubic_val(struct anim anim, double t) {

}

double anim_val(struct anim anim, double t) {
    if (anim.interp == 0)
        return near_val(anim, t);

    return lerp_val(anim, t);
}