#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL/SDL.h>
#include <SCE/interface/SCEInterface.h>

#if 1
#define W 1024
#define H 768
#else
#define W 1680
#define H 1050
#endif

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


/* functions that calculate lights position over time */
void f2 (unsigned int msec, SCE_TVector3 pos)
{
    float x = msec * 0.0005;
    pos[0] = 6 * cos (x) - 8.0;
    pos[1] = 4 * sin (x) - 7.0;
    pos[2] = 8.0;
}

void f3 (unsigned int msec, SCE_TVector3 pos)
{
    float x = msec * 0.0007;
    pos[0] = 0.0;
    pos[1] = 11 * sin (x);
    pos[2] = 8.0;
}

void f4 (unsigned int msec, SCE_TVector3 pos)
{
    float x = msec * 0.0009;
    pos[0] = 9 * cos (x + 0.4) + 8.0;
    pos[1] = 4 * sin (x * 2.0) + 10.0;
    pos[2] = 10.0;
}

typedef void (*lightfun)(unsigned int, SCE_TVector3);
static const lightfun fl[3] = {
    f2, f3, f4
};

int main (void)
{
    SCE_SInertVar rx, ry;
    SCE_SCamera *cam = NULL, *cam2 = NULL;
    SDL_Event ev;
    int loop = 1, i;
    float angle_y = 0., angle_x = 0., back_x = 0., back_y = 0.;
    int mouse_pressed = 0, wait, temps = 0, tm;
    SCE_SLight *l[4];
    SCE_SMesh *mesh = NULL;
    float *matrix = NULL;
    SCE_SScene *scene = NULL;
    float dist = 14.0;
    SCE_SShader *srender = NULL;
    SCE_SModel *mdl = NULL;
    double temps_moy = 0.0;
    SCE_SDeferred *def = NULL;
    int shadow = SCE_FALSE;

    srand (time (NULL));
    SDL_Init (SDL_INIT_VIDEO);
    SDL_WM_SetCaption ("Deferred shading - SCEngine v0.1.0 Alpha", NULL);
    if (SDL_SetVideoMode (W, H, 32, SDL_OPENGL) == NULL) {
        fprintf (stderr, "failed to setup window: %s\n", SDL_GetError ());
        return EXIT_FAILURE;
    }
    SDL_EnableKeyRepeat (1, 0);

    SCE_Init_Interface (stderr, 0);
    verif (SCEE_HaveError())
    
    /* create the scene */
    scene = SCE_Scene_Create ();
    cam = SCE_Camera_Create ();
    cam2 = SCE_Camera_Create ();
    matrix = SCE_Camera_GetView (cam);
    /* setup camera */
    SCE_Camera_SetViewport (cam, 0, 0, W, H);
    SCE_Camera_SetProjection (cam, 70.*RAD, (float)W/H, .01, 300.);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (cam->node, SCE_NODE_WRITE_MATRIX),
                           0., 0., 2.);
    SCE_Scene_AddCamera (scene, cam);
    SCE_Scene_AddCamera (scene, cam2);

    def = SCE_Deferred_Create ();
    SCE_Deferred_SetDimensions (def, W, H);
    SCE_Deferred_SetShadowMapsDimensions (def, 1024, 1024);
    SCE_Deferred_SetCascadedSplits (def, 4);
    /*SCE_Deferred_SetCascadedFar (def, 30.0);*/
    {
      const char *fnames[SCE_NUM_LIGHT_TYPES];
      fnames[SCE_POINT_LIGHT] = "point.glsl";
      fnames[SCE_SUN_LIGHT] = "sun.glsl";
      fnames[SCE_SPOT_LIGHT] = "spot.glsl";
      SCE_Deferred_Build (def, fnames);
    }
    verif (SCEE_HaveError ())
    SCE_Scene_SetDeferred (scene, def);

    /* build the scene's unique model */
    srender = SCE_Shader_Load ("render.glsl", SCE_FALSE);
    /* load mesh */
    mesh = SCE_Mesh_Load ("../data/depot.obj", SCE_FALSE);
    SCE_Mesh_AutoBuild (mesh);
    verif (SCEE_HaveError ())
    /* create and add model to the scene */
    mdl = SCE_Model_Create ();
    SCE_Model_AddNewEntity (mdl, 0, 0, mesh, srender, NULL);
    SCE_Model_AddNewInstance (mdl, 0, 1, NULL);
    SCE_Model_MergeInstances (mdl);
    SCE_Scene_AddModel (scene, mdl);

    /* setup camera inertia */
    SCE_Inert_Init (&rx);
    SCE_Inert_Init (&ry);
    SCE_Inert_SetCoefficient (&rx, 0.1);
    SCE_Inert_SetCoefficient (&ry, 0.1);
    SCE_Inert_Accum (&rx, 1);
    SCE_Inert_Accum (&ry, 1);

    ry.var = ry.real = 130.0;
    rx.var = rx.real = 220.0;
    
    /* setup lights */
    l[0] = l[1] = l[2] = l[3] = NULL;

    l[0] = SCE_Light_Create ();
    SCE_Light_SetColor (l[0], 0.4, 0.4, 0.4);
    SCE_Light_SetType (l[0], SCE_SUN_LIGHT);
    SCE_Light_SetShadows (l[0], SCE_TRUE);
    SCE_Light_SetPosition (l[0], 2., 2., 2.);
    SCE_Light_SetSpecular (l[0], SCE_TRUE);
    SCE_Scene_AddLight (scene, l[0]);

    l[1] = SCE_Light_Create ();
    SCE_Light_SetColor (l[1], 0.7, 0.3, 0.2);
    SCE_Light_SetRadius (l[1], 20.0);
    SCE_Light_SetShadows (l[1], SCE_TRUE);
    SCE_Light_SetSpecular (l[1], SCE_TRUE);
    SCE_Scene_AddLight (scene, l[1]);

    l[2] = SCE_Light_Create ();
    SCE_Light_SetColor (l[2], 0.2, 0.5, 0.1);
    SCE_Light_SetRadius (l[2], 28.0);
    SCE_Light_SetShadows (l[2], SCE_TRUE);
    SCE_Light_SetSpecular (l[2], SCE_TRUE);
    SCE_Scene_AddLight (scene, l[2]);

    l[3] = SCE_Light_Create ();
    SCE_Light_SetColor (l[3], 0.5, 0.5, 0.1);
    SCE_Light_SetRadius (l[3], 16.0);
    SCE_Light_SetShadows (l[3], SCE_TRUE);
    SCE_Light_SetSpecular (l[3], SCE_TRUE);
    SCE_Scene_AddLight (scene, l[3]);

    verif (SCEE_HaveError ())
    scene->state->lighting = SCE_TRUE;
    /* enable deferred rendering */
    scene->state->deferred = SCE_TRUE;
    scene->rclear = scene->gclear = scene->bclear = scene->aclear = 0.05;

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
                case SDL_BUTTON_WHEELUP: dist -= .5; break;
                case SDL_BUTTON_WHEELDOWN: dist += .5; break;
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
                    printf ("total time : %d ms\n", temps);
                    break;
                case SDLK_a: SCE_Light_Activate (l[0], SCE_TRUE); break;
                case SDLK_d: SCE_Light_Activate (l[0], SCE_FALSE); break;
                case SDLK_s:
                    shadow = !shadow;
                    if (shadow)
                        SCE_Deferred_AddLightFlag (def, SCE_DEFERRED_USE_SHADOWS);
                    else
                        SCE_Deferred_RemoveLightFlag (def, SCE_DEFERRED_USE_SHADOWS);
                    break;
                default:;
                }
            default:;
            }
        } 
        
        /* update light positions */
        for (i = 0; i < 3; i++) {
            SCE_TVector3 pos;
            fl[i] (SDL_GetTicks (), pos);
            SCE_Light_SetPositionv (l[i+1], pos);
        }

        SCE_Inert_Compute (&rx);
        SCE_Inert_Compute (&ry);
        
        SCE_Matrix4_Translate (matrix, 0., 0., -dist);
        SCE_Matrix4_MulRotX (matrix, -(SCE_Inert_Get (&rx)/DIV)*RAD);
        SCE_Matrix4_MulRotZ (matrix, -(SCE_Inert_Get (&ry)/DIV)*RAD);

        SCE_Scene_Update (scene, cam, NULL, 0);
        SCE_Scene_Render (scene, cam, NULL, 0);

        temps_moy = (temps + temps_moy) / 2.0;
        
        SDL_GL_SwapBuffers ();
        verif (SCEE_HaveError ())
            temps = SDL_GetTicks () - tm;
        wait = (1000.0/FPS) - temps;
        if (wait > 0)
            SDL_Delay (wait);
    }
    
    SCE_Scene_Delete (scene);
    SCE_Camera_Delete (cam);
    for (i = 0; i < 4; i++)
        SCE_Light_Delete (l[i]);
    
    SCE_Model_Delete (mdl);
    SCE_Quit_Interface ();
    SDL_Quit ();

    printf ("fps average: %.2ffps\n", 1000./temps_moy);
    printf ("render time average: %.2fms\n", temps_moy);

    return 0;
}
