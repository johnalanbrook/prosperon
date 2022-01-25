#ifndef SKYBOX_H
#define SKYBOX_H

struct mCamera;

struct mSkybox {
    unsigned int VAO;
    unsigned int VBO;
    unsigned int id;
    struct mShader *shader;
};

struct mSkybox *MakeSkybox(const char *cubemap);
void skybox_draw(const struct mSkybox *skybox,
		 const struct mCamera *camera);

#endif
