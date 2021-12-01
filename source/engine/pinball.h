#ifndef PINBALL_H
#define PINBALL_H

struct mGameObject;

struct flipper {
    float angle1;
    float angle2;
    float flipspeed;
    int left;
};

struct flipper *pinball_flipper_make(struct mGameObject *go);
void pinball_flipper_init(struct flipper *flip, struct mGameObject *go);
void pinball_flipper_update(struct flipper *flip, struct mGameObject *go);


#endif
