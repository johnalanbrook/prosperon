#include "pinball.h"
#include "gameobject.h"
#include "input.h"

struct flipper *pinball_flipper_make(struct mGameObject *go)
{
    struct flipper *new = calloc(1, sizeof(struct flipper));
    pinball_flipper_init(new, go);

    return new;
}

void pinball_flipper_init(struct flipper *flip, struct mGameObject *go)
{
    cpBodySetAngle(go->body, flip->angle1);
}

void pinball_flipper_update(struct flipper *flip, struct mGameObject *go)
{
    if ((flip->left && currentKeystates[SDL_SCANCODE_LSHIFT])
	|| currentKeystates[SDL_SCANCODE_RSHIFT]) {
	cpBodySetAngle(go->body, flip->angle2);
    } else
	cpBodySetAngle(go->body, flip->angle1);
}
