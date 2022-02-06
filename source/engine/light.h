#ifndef LIGHT_H
#define LIGHT_H

#include <stdint.h>

struct mLight {
    struct mGameObject *go;
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
			   struct mShader *shader, int num);
void pointlights_prepshader(struct mShader *shader);


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
void spotlight_prepshader(struct mSpotLight *light, struct mShader *shader,
			  int num);
void spotlights_prepshader(struct mShader *shader);



struct mDirectionalLight {
    struct mLight light;
};

void dlight_prepshader(struct mDirectionalLight *light,
		       struct mShader *shader);
struct mDirectionalLight *MakeDLight();

extern struct mDirectionalLight *dLight;

*/

#endif
