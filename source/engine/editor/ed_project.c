#include "ed_project.h"

#include <string.h>
#include <stdlib.h>
#include "editor.h"

void editor_init_project(struct gameproject *gp)
{
/*
    cur_project = gp;
    DATA_PATH = strdup(gp->path);
    stemlen = strlen(DATA_PATH);
    findPrefabs();
    get_levels();
    get_all_files();
    */
}

void editor_make_project(char *path)
{
    FILE *f = path_open("w", "%s%s", path, "/project.yugh");
    cur_project = malloc(sizeof(struct gameproject));
    strncpy(cur_project->name, "New Game", 127);
    strncpy(cur_project->path, path, 2048);
    vec_add(projects, cur_project);
    fwrite(cur_project, sizeof(*cur_project), 1, f);
    fclose(f);

    editor_init_project(cur_project);

    editor_save_projects();
}

void editor_import_project(char *path)
{
    FILE *f = path_open("r", "%s%s", path, "/project.yugh");
    if (!f)
	return;

    struct gameproject *gp = malloc(sizeof(*gp));
    fread(gp, sizeof(*gp), 1, f);
    fclose(f);

    vec_add(projects, gp);
}

void editor_project_btn_gui(struct gameproject *gp)
{
/*
    if (ImGui::Button(gp->name))
	editor_init_project(gp);


    ImGui::SameLine();
    ImGui::Text("%s", gp->path);
    */
}

void editor_proj_select_gui()
{
/*
    ImGui::Begin("Project Select");

    vec_walk(projects, (void (*)(void *)) &editor_project_btn_gui);

    ImGui::InputText("Project import path", setpath, MAXPATH);
    ImGui::SameLine();
    if (ImGui::Button("Create")) {
	editor_make_project(setpath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Import")) {
	editor_import_project(setpath);
    }

    ImGui::End();
    */
}


void editor_load_projects()
{
    FILE *f = fopen("projects.yugh", "r");
    if (!f)
	return;

    vec_load(projects, f);
    fclose(f);
}

void editor_save_projects()
{
    FILE *f = fopen("projects.yugh", "w");
    vec_store(projects, f);
    fclose(f);
}
