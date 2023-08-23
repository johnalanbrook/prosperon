#ifndef ANIM_H
#define ANIM_H

struct keyframe {
  double time;
  double val;
};

struct anim {
  struct keyframe *frames;
  int loop;
  int interp;
};

struct anim make_anim();
struct anim anim_add_keyframe(struct anim a, struct keyframe f);
double anim_val(struct anim anim, double t);

#endif