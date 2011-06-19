
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL/SDL.h>

/* SCEngine */
#include <SCE/interface/SCEInterface.h>

#define W 1024
#define H 512

/* max FPS */
#define FPS 60

#define verif(cd)\
if (cd) {\
    SCEE_LogSrc ();\
    SCEE_Out ();\
    exit (EXIT_FAILURE);\
}

int main (void)
{
    SCE_SInertVar rx, ry;
    SCE_SCamera *cam = NULL;
    SDL_Event ev;
    int loop = 1;
    float angle_y = 0., angle_x = 0., back_x = 0., back_y = 0.;
    int mouse_pressed = 0, wait, temps = 0, tm, i;
    float *matrix = NULL;
    SCE_SLight *l;
    SCE_SMesh *mesh = NULL;
    SCE_SModel *model = NULL;
    SCE_SShader *shader = NULL;
    SCE_SScene *scene = NULL;
    float dist = 6.0;

    srand (time (NULL));
    SDL_Init (SDL_INIT_VIDEO);
    SDL_WM_SetCaption ("Geometry shader test 1", NULL);
    verif (SDL_SetVideoMode (W, H, 32, SDL_OPENGL) == NULL)
    SDL_EnableKeyRepeat (1, 0);
    verif (SCE_Init_Interface (stderr, 0) < 0)
    
    scene = SCE_Scene_Create ();
    cam = SCE_Camera_Create ();
    matrix = SCE_Camera_GetView (cam);
    SCE_Camera_SetViewport (cam, 0, 0, W, H);
    SCE_Matrix4_Projection (SCE_Camera_GetProj (cam), 70.*RAD,
                            (float)W/H, 0.1, 4000.);
    
    SCE_Scene_SetOctreeSize (scene, 640.0, 640.0, 640.0);
    SCE_Scene_MakeOctree (scene, 1, SCE_FALSE, 0.0);

    SCE_Scene_AddCamera (scene, cam);

    l = SCE_Light_Create ();
    SCE_Light_SetColor (l, 1., 1., 1.);
    SCE_Light_Infinite (l, SCE_TRUE);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (SCE_Light_GetNode (l),
                                               SCE_NODE_READ_MATRIX),
                           1., 2.4, 1.);
    SCE_Scene_AddLight (scene, l);

    shader = SCE_Shader_Load ("shader.glsl", SCE_FALSE);
    if (SCE_Shader_Build (shader) < 0) {
      SCEE_Out ();
      exit (-1);
    }
    mesh = SCE_Mesh_Load ("../data/spaceship.obj", SCE_FALSE);
    SCE_Mesh_AutoBuild (mesh);
    model = SCE_Model_Create ();
    SCE_Model_AddNewEntity (model, 0, 0, mesh, shader, NULL);
    SCE_Model_AddNewInstance (model, 0, 1, NULL);
    SCE_Model_MergeInstances (model);

    SCE_Scene_AddModel (scene, model);

    verif (SCEE_HaveError ())

    SCE_Inert_Init (&rx);
    SCE_Inert_Init (&ry);

    SCE_Inert_SetCoefficient (&rx, 0.1);
    SCE_Inert_SetCoefficient (&ry, 0.1);
    SCE_Inert_Accum (&rx, 1);
    SCE_Inert_Accum (&ry, 1);

    scene->states.lighting = SCE_TRUE;
    scene->states.frustum_culling = SCE_FALSE;
    scene->states.lod = SCE_FALSE;
    temps = 0;

    while (loop) {
        tm = SDL_GetTicks ();
        while (SDL_PollEvent (&ev)) {
            switch (ev.type) {
            case SDL_QUIT: loop = 0; break;

            case SDL_MOUSEBUTTONDOWN:
                if (ev.button.button != SDL_BUTTON_WHEELUP &&
                    ev.button.button != SDL_BUTTON_WHEELDOWN) {
                    back_x = ev.button.x;
                    back_y = ev.button.y;
                    mouse_pressed = 1;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                switch (ev.button.button) {
                case SDL_BUTTON_WHEELUP: dist -= 0.3; break;
                case SDL_BUTTON_WHEELDOWN: dist += 0.3; break;
                default: mouse_pressed = 0;
                }
                break;

            case SDL_MOUSEMOTION:
                if (mouse_pressed) {
                    angle_y += back_x-ev.motion.x;
                    angle_x += back_y-ev.motion.y;
                    SCE_Inert_Operator (&ry, +=, back_x-ev.motion.x);
                    SCE_Inert_Operator (&rx, +=, back_y-ev.motion.y);
                    back_x = ev.motion.x;
                    back_y = ev.motion.y;
                }
                break;

            case SDL_KEYUP:
                switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE: loop = 0; break;
                case SDLK_t:
                    printf ("fps : %.2f\n", 1000./temps);
                    printf ("update time : %d ms\n", i);
                    printf ("total time : %d ms\n", temps);
                    break;
                default:;
                }
            default:;
            }
        } 

        i = SDL_GetTicks ();

        SCE_Inert_Compute (&rx);
        SCE_Inert_Compute (&ry);

        SCE_Matrix4_Translate (matrix, 0., 0., -dist);
        SCE_Matrix4_MulRotX (matrix, -(SCE_Inert_Get (&rx) * 0.2) * RAD);
        SCE_Matrix4_MulRotZ (matrix, -(SCE_Inert_Get (&ry) * 0.2) * RAD);

        SCE_Scene_Update (scene, cam, NULL, 0);
        i = SDL_GetTicks () - i;
        SCE_Scene_Render (scene, cam, NULL, 0);

        SDL_GL_SwapBuffers ();

        verif (SCEE_HaveError ())
            temps = SDL_GetTicks () - tm;
        wait = (1000.0/FPS) - temps;
        if (wait > 0)
            SDL_Delay (wait);
    }
    
    SCE_Scene_Delete (scene);
    SCE_Camera_Delete (cam);

    SCE_Model_Delete (model);
    
    SCE_Quit_Interface ();

    SDL_Quit ();

    return 0;
}
