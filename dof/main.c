
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL/SDL.h>
#include <SCE/interface/SCEInterface.h>

#define W 1024
#define H 768

#define MW 1024
#define MH 1024

/* max FPS */
#define FPS 60

#define DIV 4.0

#define verif(cd)\
if (cd)\
{\
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
    int mouse_pressed = 0, wait, temps = 0, tm, i = 0;
    SCE_SLight *l = NULL;
    SCE_SMesh *mesh = NULL;
    float *matrix = NULL;
    SCE_SScene *scene = NULL;
    float dist = 6.0;
    SCE_SModel *mdl = NULL;
    SCE_SShader *shd1 = NULL, *srender = NULL;
    SCE_STexture *tex1 = NULL, *depthmap = NULL, *blurred1 = NULL, *blurred2 = NULL;
    SCE_SNode *node = NULL;
    
    srand (time (NULL));
    SDL_Init (SDL_INIT_VIDEO);
    SDL_WM_SetCaption ("Depth of field sample - SCEngine v0.0.9 Alpha", NULL);
    verif (SDL_SetVideoMode (W, H, 32, SDL_OPENGL) == NULL)
        SDL_EnableKeyRepeat (1, 0);

    SCE_Init_Interface (stderr, 0);
    verif (SCEE_HaveError())
    SCE_OBJ_ActivateIndicesGeneration (1);
    
    scene = SCE_Scene_Create ();
    cam = SCE_Camera_Create ();
    matrix = SCE_Camera_GetView (cam);

    SCE_Camera_SetViewport (cam, 0, 0, MW, MH);
    SCE_Matrix4_Projection(SCE_Camera_GetProj(cam), 70.*RAD, (float)W/H, .005, 1000.);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (cam->node, SCE_NODE_WRITE_MATRIX),
                           0., 0., 2.);
    SCE_Scene_AddCamera (scene, cam);
    
    shd1 = SCE_Shader_Load (NULL, "shd1.frag", SCE_FALSE);
    SCE_Shader_Build (shd1);
    verif (SCEE_HaveError ())
    srender = SCE_Shader_Load ("render.glsl", NULL, SCE_FALSE);
    SCE_Shader_Build (srender);
    verif (SCEE_HaveError ())

    blurred1 = SCE_Texture_Create (SCE_RENDER_COLOR, MW, MH);
    blurred2 = SCE_Texture_Create (SCE_RENDER_COLOR, MW, MH);
    tex1 = SCE_Texture_Create (SCE_RENDER_COLOR, MW, MH);
    depthmap = SCE_Texture_Create (SCE_TEX_2D, MW, MH);
    SCE_Texture_Build (depthmap, SCE_FALSE);
    verif (SCEE_HaveError ())
    SCE_Texture_AddRenderTexture (tex1, SCE_COLOR_BUFFER1, depthmap);
    verif (SCEE_HaveError ())
    SCE_Texture_SetUnit (depthmap, 1);

    verif (SCEE_HaveError ())

    SCE_Shader_Use (shd1);
    SCE_Shader_Param (SCE_PIXEL_SHADER, "colormap", 0);
    SCE_Shader_Param (SCE_PIXEL_SHADER, "depthmap", 1);
    SCE_Shader_Use (NULL);

    mesh = SCE_Mesh_Load ("../data/dofscene4.obj", SCE_FALSE);
    SCE_Mesh_AutoBuild (mesh);

    mdl = SCE_Model_Create ();
    SCE_Model_AddNewEntity (mdl, 0, 0, mesh, srender, NULL);
    SCE_Model_AddNewInstance (mdl, 0, SCE_FALSE, NULL);
    SCE_Model_MergeInstances (mdl);
    SCE_Scene_AddModel (scene, mdl);

    SCE_Inert_Init (&rx);
    SCE_Inert_Init (&ry);
    SCE_Inert_SetCoefficient (&rx, 0.1);
    SCE_Inert_SetCoefficient (&ry, 0.1);
    SCE_Inert_Accum (&rx, 1);
    SCE_Inert_Accum (&ry, 1);
    
    l = SCE_Light_Create ();
    SCE_Light_SetColor (l, 1., 0.5, 0.5);
    SCE_Light_Infinite (l, SCE_FALSE);
    node = SCE_Light_GetNode (l);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           -1., -2., 5.);
    SCE_Scene_AddLight (scene, l);

    verif (SCEE_HaveError ())

    scene->states.lighting = SCE_TRUE;
    
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
                case SDL_BUTTON_WHEELUP: dist -= .1; break;
                case SDL_BUTTON_WHEELDOWN: dist += .1; break;
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
                case SDLK_ESCAPE: loop = SCE_FALSE; break;
                case SDLK_t:
                    printf ("fps : %.2f\n", 1000./temps);
                    printf ("update time : %d ms\n", i);
                    printf ("total time : %d ms\n", temps);
                    break;
                case SDLK_a: SCE_Light_Activate (l, SCE_TRUE); break;
                case SDLK_d: SCE_Light_Activate (l, SCE_FALSE);
                default:;
                }
            default:;
            }
        } 
        
        SCE_Inert_Compute (&rx);
        SCE_Inert_Compute (&ry);
        
        SCE_Matrix4_Translate (matrix, 0., 0., -dist);
        SCE_Matrix4_MulRotX (matrix, -(SCE_Inert_Get (&rx)/DIV)*RAD);
        SCE_Matrix4_MulRotZ (matrix, -(SCE_Inert_Get (&ry)/DIV)*RAD);

        i = SDL_GetTicks ();
        SCE_Scene_Update (scene, cam, NULL, 0);
        i = SDL_GetTicks () - i;

        SCE_Scene_Render (scene, NULL, tex1, 0);
        SCE_Texture_Use (depthmap);
        SCE_Shader_Use (shd1);
        SCE_Shader_Param (SCE_PIXEL_SHADER, "sens", 0);
        SCE_Texture_Blitf (NULL, blurred1, NULL, tex1);
        SCE_Shader_Param (SCE_PIXEL_SHADER, "sens", 1);
        SCE_Texture_Blitf (NULL, blurred2, NULL, blurred1);
        SCE_Texture_Blitf (NULL, NULL, NULL, blurred2);
        
        SDL_GL_SwapBuffers ();
        verif (SCEE_HaveError ())
            temps = SDL_GetTicks () - tm;
        wait = (1000.0/FPS) - temps;
        if (wait > 0)
            SDL_Delay (wait);
    }
    
    SCE_Scene_Delete (scene);
    SCE_Camera_Delete (cam);
    SCE_Light_Delete (l);

    SCE_Model_Delete (mdl);

    SCE_Quit_Interface ();

    SDL_Quit ();

    return 0;
}
