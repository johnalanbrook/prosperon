#ifndef LEVEL_H
#define LEVEL_H

#include "config.h"

// This class holds all of the entities and options for a level. Really it's nothing more than a container and access point for all the entities currently loaded into the game.

struct level {
  char name[MAXNAME];
};

void save_level(char name[MAXNAME]);
void load_level(char name[MAXNAME]);
void new_level();

#endif
