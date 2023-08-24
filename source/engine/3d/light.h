#ifndef LIGHT_H
#define LIGHT_H

#include <stdint.h>

struct mLight {
  struct gameobject *go;
  uint8_t color[3];
  float strength;
  int dynamic;
  int on;
};

/*
struct mPointLight {
    struct mLight light;
    float constant;
    float linear;
    float quadratic;
};

struct mPointLight *MakePointlight();
void pointlight_prepshader(struct mPointLight *light,
                           struct shader *shader, int num);
void pointlights_prepshader(struct shader *shader);


struct mSpotLight {
    struct mLight light;
    float constant;
    float linear;
    float quadratic;
    float distance;

    float cutoff;
    float outerCutoff;
};

struct mSpotLight *MakeSpotlight();
void spotlight_gui(struct mSpotLight *light);
void spotlight_prepshader(struct mSpotLight *light, struct shader *shader,
                          int num);
void spotlights_prepshader(struct shader *shader);



struct mDirectionalLight {
    struct mLight light;
};

void dlight_prepshader(struct mDirectionalLight *light,
                       struct shader *shader);
struct mDirectionalLight *MakeDLight();

extern struct mDirectionalLight *dLight;

*/

void light_gui(struct mLight *light);
void pointlight_gui(struct mPointLight *light);
void spotlight_gui(struct mSpotLight *spot);

#endif