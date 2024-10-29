#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#define dag_rm(p,c) do{\
 for (int i = arrlen(p->children)-1; i--; i >=0) {\
  if (p->children[i] == c) { \
  arrdelswap(p->children,i);\
  c->parent=NULL;\
  break;\
}}}while(0)

#define dag_set(p,c) do{\
  arrpush(p->children,c);\
  if(c->parent) dag_rm(c->parent,c);\
  c->parent=p;\
}while(0)

#define dag_clip(p) do{\
  if (p->parent)\
    dag_rm(p->parent,p);\
}while(0)

struct gameobject {
  float damping;
  float timescale;
  float maxvelocity;
  float maxangularvelocity;
  unsigned int layer;
  unsigned int warp_mask;
};

/*
  Friction uses coulomb model. When shapes collide, their friction is multiplied. Some example values:
  Steel on steel: 0.0005
  Wood on steel: 0.0012
  Wood on wood: 0.0015
  => steel = 0.025
  => wood = 0.04
  => hardrubber = 0.31
  => concrete = 0.05
  => rubber = 0.5
  Hardrubber on steel: 0.0077
  Hardrubber on concrete: 0.015
  Rubber on concrete: 0.025
*/

typedef struct gameobject gameobject;

#endif
