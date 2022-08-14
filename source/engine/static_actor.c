#include "static_actor.h"

//ADDMAKE(StaticActor);

static struct mStaticActor *models[100];
static int numModels = 0;
static struct mStaticActor *shadow_casters[100];
static int numShadowCasters = 0;

struct mStaticActor *curActor = NULL;

void staticactor_draw_dbg_color_pick(struct mShader *s)
{
    for (int i = 0; i < numModels; i++) {
	shader_setvec3(s, "PickingColor", models[i]->obj.editor.color);
	setup_model_transform(&models[i]->obj.transform, s, 1.f);
	//models[i]->obj.draw(s);
    }

}

void staticactor_draw_models(struct mShader *s)
{
    for (int i = 0; i < numModels; i++) {
	setup_model_transform(&models[i]->obj.transform, s, 1.f);
	draw_model(models[i]->model, s);
    }
}

void staticactor_draw_shadowcasters(struct mShader *s)
{
    for (int i = 0; i < numShadowCasters; i++) {
	setup_model_transform(&shadow_casters[i]->obj.transform, s, 1.f);
	//models[i]->obj.draw(s);
    }
}

/*
void StaticActor::serialize(FILE * file)
{
    GameObject::serialize(file);
    SerializeBool(file, &castShadows);
    Serializecstr(file, currentModelPath);
}

void StaticActor::deserialize(FILE * file)
{
    GameObject::deserialize(file);
    DeserializeBool(file, &castShadows);

    Deserializecstr(file, currentModelPath, MAXPATH);
    curActor = this;
    set_new_model(currentModelPath);
}
*/



struct mStaticActor *MakeStaticActor(const char *modelPath)
{
    struct mStaticActor *newsa =
	(struct mStaticActor *) malloc(sizeof(struct mStaticActor));
    newsa->model = GetExistingModel(modelPath);
    models[numModels++] = newsa;

    return newsa;
}

/*
Serialize *make_staticactor()
{
    StaticActor *nactor = (StaticActor *) malloc(sizeof(StaticActor));

    return nactor;
}
*/

#include "nuke.h"

void staticactor_gui(struct mStaticActor *sa)
{
    object_gui(&sa->obj);
    if (nk_tree_push(ctx, NK_TREE_NODE, "Model", NK_MINIMIZED)) {
	nk_checkbox_label(ctx, "Cast Shadows", &sa->castShadows);
	nk_labelf(ctx, NK_TEXT_LEFT, "Model path: %s", sa->currentModelPath);

	//ImGui::SameLine();
	if (nk_button_label(ctx, "Load model")) {
	    //asset_command = set_new_model;
	    curActor = sa;
	}

    }
}