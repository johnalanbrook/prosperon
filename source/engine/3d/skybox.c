#include "skybox.h"

#include "camera.h"
#include "shader.h"
#include <stdlib.h>
#include <string.h>

#include "render.h"

static const float skyboxVertices[216] = {
    -1.0f, 1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,

    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f};

struct mSkybox *MakeSkybox(const char *cubemap) {
  /*
      struct mSkybox *newskybox = malloc(sizeof(struct mSkybox));

      newskybox->shader = MakeShader("skyvert.glsl", "skyfrag.glsl");
      shader_compile(newskybox->shader);

      glGenVertexArrays(1, &newskybox->VAO);
      glGenBuffers(1, &newskybox->VBO);
      glBindVertexArray(newskybox->VAO);
      glBindBuffer(GL_ARRAY_BUFFER, newskybox->VBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                            (void *) 0);

      shader_use(newskybox->shader);
      shader_setint(newskybox->shader, "skybox", 0);
  */
  /*
      const char *faces[6] =
          { "right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg",
          "back.jpg"
      };
  */
  /*
      glGenTextures(1, &newskybox->id);
      glBindTexture(GL_TEXTURE_CUBE_MAP, newskybox->id);
  */
  /*char buf[100] = { '\0' };*/

  for (int i = 0; i < 6; i++) {
    /*
        buf[0] = '\0';
        strcat(buf, cubemap);
        strcat(buf, "/");
        strcat(buf, faces[i]);
        IMG_Load(buf);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 2048,
                     2048, 0, GL_RGB, GL_UNSIGNED_BYTE, data->pixels);
    */
  }
  /*
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                      GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                      GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                      GL_CLAMP_TO_EDGE);


      return newskybox;
  */
  return NULL;
}

void skybox_draw(const struct mSkybox *skybox,
                 const struct mCamera *camera) {
  /*
      shader_use(skybox->shader);

      mfloat_t view[16] = { 0.f };
      getviewmatrix(view, camera);
      shader_setmat4(skybox->shader, "skyview", view);

      // skybox cube
      glBindVertexArray(skybox->VAO);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->id);
      glDrawArrays(GL_TRIANGLES, 0, 36);
      glBindVertexArray(0);
  */
}
