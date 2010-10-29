#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <SCE/interface/SCEInterface.h>

int main(void)
{
  SDL_Event ev;
  int running = 1;

  SCE_SScene *scene;
  SCE_SCamera *cam;
  SCE_SLight *l;
  SCE_SMesh *mesh;
  SCE_SModel *model;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Can't init SDL: %s\n", SDL_GetError());
    return 1;
  }

  if (SDL_SetVideoMode(800, 600, 32, SDL_OPENGL) == NULL) {
    fprintf(stderr, "Can't init video mode: %s\n", SDL_GetError());
    return 1;
  }
  
  SCE_Init_Interface(stderr, 0);

  scene = SCE_Scene_Create();
  cam = SCE_Camera_Create();

  SCE_Scene_AddCamera(scene, cam);

  l = SCE_Light_Create();
  SCE_Light_SetColor(l, 1., 1., 1.);
  SCE_Light_Infinite(l, SCE_TRUE);
  SCE_Matrix4_Translate(SCE_Node_GetMatrix(SCE_Light_GetNode(l),
                                           SCE_NODE_READ_MATRIX),
                        1., 2.4, 1.);
  SCE_Scene_AddLight(scene, l);

  mesh = SCE_Mesh_Load("../data/spaceship.obj", 2);
  SCE_Mesh_AutoBuild(mesh);
  model = SCE_Model_Create();
  SCE_Model_AddNewEntity(model, 0, 0, mesh, NULL, NULL);
  SCE_Model_AddNewInstance(model, 0, 1, NULL);
  SCE_Model_MergeInstances(model);
  
  SCE_Scene_AddModel(scene, model);

  while (running) {
    SDL_WaitEvent(&ev);

    switch (ev.type) { 
    case SDL_QUIT: running = 0; break;
    default: break;
    }


    SCE_Scene_Update(scene, cam, NULL, 0);
    SCE_Scene_Render(scene, cam, NULL, 0);

    if (SCEE_HaveError ()) {
      SCEE_Out();
    }
    SDL_GL_SwapBuffers();
    SDL_Delay(10);
  }

  SDL_Quit();
  return 0;
}
