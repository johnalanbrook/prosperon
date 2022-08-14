#include "light.h"
#include <stdbool.h>


/*
void Light::serialize(FILE * file)
{
    GameObject::serialize(file);

    SerializeFloat(file, &strength);
    SerializeVec3(file, (float *) &color);
    SerializeBool(file, &dynamic);
}

void Light::deserialize(FILE * file)
{
    GameObject::deserialize(file);

    DeserializeFloat(file, &strength);
    DeserializeVec3(file, (float *) &color);
    DeserializeBool(file, &dynamic);
}



static const mfloat_t dlight_init_rot[3] = { 80.f, 120.f, 165.f };

struct mDirectionalLight *dLight = NULL;

struct mDirectionalLight *MakeDLight()
{
    if (dLight != NULL) {
	dLight =
	    (struct mDirectionalLight *)
	    malloc(sizeof(struct mDirectionalLight));
	quat_from_euler(dLight->light.obj.transform.rotation,
			dlight_init_rot);

	return dLight;
    }

    return dLight;
}

void dlight_prepshader(struct mDirectionalLight *light,
		       struct mShader *shader)
{
    mfloat_t fwd[3] = { 0.f };
    trans_forward(fwd, &light->light.obj.transform);
    shader_setvec3(shader, "dirLight.direction", fwd);
    shader_setvec3(shader, "dirLight.color", light->light.color);
    shader_setfloat(shader, "dirLight.strength", light->light.strength);
}



static struct mPointLight *pointLights[4];
static int numLights = 0;


struct mPointLight *MakePointlight()
{
    if (numLights < 4) {
	struct mPointLight *light =
	    (struct mPointLight *) malloc(sizeof(struct mPointLight));
	pointLights[numLights++] = light;
	light->light.strength = 0.2f;
	light->constant = 1.f;
	light->linear = 0.9f;
	light->quadratic = 0.032f;
	return light;
    }

    return NULL;
}

static void prepstring(char *buffer, char *prepend, const char *append)
{
    snprintf(buffer, 100, "%s%s", prepend, append);
}

void pointlights_prepshader(struct mShader *shader)
{
    for (int i = 0; i < numLights; i++)
	pointlight_prepshader(pointLights[i], shader, i);
}

void pointlight_prepshader(struct mPointLight *light,
			   struct mShader *shader, int num)
{
    shader_use(shader);
    char prepend[100] = { '\0' };
    snprintf(prepend, 100, "%s%d%s", "pointLights[", num, "].");
    char str[100] = { '\0' };

    prepstring(str, prepend, "position");
    shader_setvec3(shader, str, light->light.obj.transform.position);

    prepstring(str, prepend, "constant");
    shader_setfloat(shader, str, light->constant);

    prepstring(str, prepend, "linear");
    shader_setfloat(shader, str, light->linear);

    prepstring(str, prepend, "quadratic");
    shader_setfloat(shader, str, light->quadratic);

    prepstring(str, prepend, "strength");
    shader_setfloat(shader, str, light->light.strength);

    prepstring(str, prepend, "color");
    shader_setvec3(shader, str, light->light.color);
}




static struct mSpotLight *spotLights[4];
static int numSpots = 0;

struct mSpotLight *MakeSpotlight()
{
    if (numSpots < 4) {
	struct mSpotLight *light =
	    (struct mSpotLight *) malloc(sizeof(struct mSpotLight));
	spotLights[numSpots++] = light;
	return light;
    }

    return NULL;
}



void spotlights_prepshader(struct mShader *shader)
{
    for (int i = 0; i < numSpots; i++)
	spotlight_prepshader(spotLights[i], shader, i);
}

void spotlight_prepshader(struct mSpotLight *light, struct mShader *shader,
			  int num)
{
    mfloat_t fwd[3] = { 0.f };
    trans_forward(fwd, &light->light.obj.transform);
    shader_use(shader);
    shader_setvec3(shader, "spotLight.position",
		   light->light.obj.transform.position);
    shader_setvec3(shader, "spotLight.direction", fwd);
    shader_setvec3(shader, "spotLight.color", light->light.color);
    shader_setfloat(shader, "spotLight.strength", light->light.strength);
    shader_setfloat(shader, "spotLight.cutoff", light->cutoff);
    shader_setfloat(shader, "spotLight.distance", light->distance);
    shader_setfloat(shader, "spotLight.outerCutoff", light->outerCutoff);
    shader_setfloat(shader, "spotLight.linear", light->linear);
    shader_setfloat(shader, "spotLight.quadratic", light->quadratic);
    shader_setfloat(shader, "spotLight.constant", light->constant);
}

*/

/*
void light_gui(struct mLight *light)
{
    object_gui(&light->obj);

    if (nk_tree_push(ctx, NK_TREE_NODE, "Light", NK_MINIMIZED)) {
	nk_property_float(ctx, "Strength", 0.f, &light->strength, 1.f, 0.01f, 0.001f);
	// ImGui::ColorEdit3("Color", &light->color[0]);
	nk_checkbox_label(ctx, "Dynamic", (bool *) &light->dynamic);
	nk_tree_pop(ctx);
    }
}


void pointlight_gui(struct mPointLight *light)
{
    light_gui(&light->light);

    if (nk_tree_push(ctx, NK_TREE_NODE, "Point Light", NK_MINIMIZED)) {
	nk_property_float(ctx, "Constant", 0.f, &light->constant, 1.f, 0.01f, 0.001f);
	nk_property_float(ctx, "Linear", 0.f, &light->linear, 0.3f, 0.01f, 0.001f);
	nk_property_float(ctx, "Quadratic", 0.f, &light->quadratic, 0.3f, 0.01f, 0.001f);
	nk_tree_pop(ctx);
    }

}

void spotlight_gui(struct mSpotLight *spot)
{
    light_gui(&spot->light);

    if (nk_tree_push(ctx, NK_TREE_NODE, "Spotlight", NK_MINIMIZED)) {
	nk_property_float(ctx, "Linear", 0.f, &spot->linear, 1.f, 0.01f, 0.001f);
	nk_property_float(ctx, "Quadratic", 0.f, &spot->quadratic, 1.f, 0.01f, 0.001f);
	nk_property_float(ctx, "Distance", 0.f, &spot->distance, 200.f, 1.f, 0.1f, 200.f);
	nk_property_float(ctx, "Cutoff Degrees", 0.f, &spot->cutoff, 0.7f, 0.01f, 0.001f);
	nk_property_float(ctx, "Outer Cutoff Degrees", 0.f, &spot->outerCutoff, 0.7f, 0.01f, 0.001f);
	nk_tree_pop(ctx);
    }
}
*/
