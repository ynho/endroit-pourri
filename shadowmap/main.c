
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL/SDL.h>
#include <SCE/interface/SCEInterface.h>

#define W 1024
#define H 768

#define DW (512*4)
#define DH (512*4)

/* max FPS */
#define FPS 60

#define DIV 4.0

#define verif(cd)\
if (cd) {\
    SCEE_LogSrc ();\
    SCEE_Out ();\
    exit (EXIT_FAILURE);\
}

int main (void)
{
    SCE_SInertVar rx, ry;
    SCE_SCamera *cam = NULL, *lcam = NULL;
    SDL_Event ev;
    int loop = 1;
    SCE_SMesh *mesh = NULL;
    SCE_SModel *mdl = NULL;
    float back_x = 0., back_y = 0.;
    int mouse_pressed = 0, wait, temps = 0, tm;
    SCE_SLight *l = NULL;
    float *p = NULL;
    float *matrix = NULL, *mat = NULL; /* la seconde est la matrice de lcam */
    SCE_SScene *scene = NULL;
    float dist = 10.0;
    SCE_SNode *node = NULL;
    SCE_STexture *fb = NULL;
    SCE_SShader *rendershadow = NULL, *genshadow = NULL;
    int smooth = 0;

    SDL_Init (SDL_INIT_VIDEO);
    SDL_WM_SetCaption ("Shadow mapping - SCEngine v0.1.0 Alpha", NULL);
    if (SDL_SetVideoMode (W, H, 32, SDL_OPENGL) == NULL) {
        fprintf (stderr, "failed to setup window: %s\n", SDL_GetError ());
        return EXIT_FAILURE;
    }
    SDL_EnableKeyRepeat (1, 0);
    SCE_Init_Interface (stderr, 0);
    verif (SCEE_HaveError())

    /* scene */
    scene = SCE_Scene_Create ();
    cam = SCE_Camera_Create ();
    lcam = SCE_Camera_Create ();
    matrix = SCE_Camera_GetView (cam);
    mat = SCE_Camera_GetView (lcam);

    /* usual camera */
    SCE_Camera_SetViewport (cam, 0, 0, W, H);
    SCE_Matrix4_Projection (SCE_Camera_GetProj (cam), 70.*RAD, (float)W/H, .1, 1000.);

    /* light camera */
    SCE_Camera_SetViewport (lcam, 0, 0, DW, DH);
    SCE_Matrix4_Projection (SCE_Camera_GetProj (lcam), 70.*RAD, (float)DW/DH, 1.0, 1000.);

    /* render buffer (depth) */
    fb = SCE_Texture_Create (SCE_RENDER_DEPTH, DW, DH);
    verif (SCEE_HaveError ())

    /* final render shader */
    rendershadow = SCE_Shader_Load ("shadow.glsl", SCE_FALSE);
    SCE_Shader_Build (rendershadow);
    verif (SCEE_HaveError ())
    SCE_Shader_Use (rendershadow);
    SCE_Shader_Param ("depthmap", 0);
    SCE_Shader_Use (NULL);
    SCE_Shader_AddParamv (rendershadow, "dosmooth", &smooth);
    verif (SCEE_HaveError ())

    /* depth map render shader */
    genshadow = SCE_Shader_Load ("genshadow.glsl", SCE_FALSE);
    SCE_Shader_Build (genshadow);
    verif (SCEE_HaveError ())

    /* create model */
    mesh = SCE_Mesh_Load ("../data/dofscene4.obj", SCE_FALSE);
    SCE_Mesh_AutoBuild (mesh);
    mdl = SCE_Model_Create ();
    SCE_Model_AddNewEntity (mdl, 0, 0, mesh, NULL, NULL);
    SCE_Model_AddNewInstance (mdl, 0, 0, NULL);
    SCE_Model_MergeInstances (mdl);
    SCE_Scene_AddModel (scene, mdl);
    verif (SCEE_HaveError ())

    SCE_Inert_Init (&rx);
    SCE_Inert_Init (&ry);
    SCE_Inert_SetCoefficient (&rx, 0.1);
    SCE_Inert_SetCoefficient (&ry, 0.1);
    SCE_Inert_Accum (&rx, 1);
    SCE_Inert_Accum (&ry, 1);

    /* create light */
    l = SCE_Light_Create ();
    node = SCE_Light_GetNode (l);
    p = SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX);
    SCE_Matrix4_Translate (p, 0., -14., 14.0);
    SCE_Matrix4_MulRotX (p, 40.0*RAD);
    SCE_Light_SetColor (l, 0.9, 0.9, 0.9);
    SCE_Light_SetRadius (l, -40.0);
    SCE_Light_Infinite (l, 0);

    SCE_Scene_AddLight (scene, l);
    verif (SCEE_HaveError ())

    /* attach light camera's node to light's node */
    SCE_Node_Attach (node, SCE_Camera_GetNode (lcam));

    SCE_Scene_AddCamera (scene, cam);
    SCE_Scene_AddCamera (scene, lcam);

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
                case SDL_BUTTON_WHEELUP: dist -= .2; break;
                case SDL_BUTTON_WHEELDOWN: dist += .2; break;
                default: mouse_pressed = 0;
                }
                break;
                
            case SDL_MOUSEMOTION:
                if (mouse_pressed) {
                    SCE_Inert_Operator (&ry, +=, back_x-ev.motion.x);
                    SCE_Inert_Operator (&rx, +=, back_y-ev.motion.y);
                    back_x = ev.motion.x;
                    back_y = ev.motion.y;
                }
                break;
                
            case SDL_KEYDOWN:
                switch (ev.key.keysym.sym) {
                case SDLK_p: SCE_Matrix4_MulTranslate (p, 0.2, 0., 0.);
                    SCE_Node_HasMoved (node);break;
                case SDLK_m: SCE_Matrix4_MulTranslate (p, -0.2, 0., 0.);
                    SCE_Node_HasMoved (node); break;
                case SDLK_i: SCE_Matrix4_MulTranslate (p, 0., 0.2, 0.);
                    SCE_Node_HasMoved (node);break;
                case SDLK_k: SCE_Matrix4_MulTranslate (p, 0., -0.2, 0.);
                    SCE_Node_HasMoved (node); break;
                case SDLK_o: SCE_Matrix4_MulTranslate (p, 0., 0., 0.2); 
                    SCE_Node_HasMoved (node);break;
                case SDLK_l: SCE_Matrix4_MulTranslate (p, 0., 0., -0.2);
                    SCE_Node_HasMoved (node);
                default:;
                }
                break;
                
            case SDL_KEYUP:
                switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE: loop = 0; break;
                case SDLK_t: printf ("fps : %.2f\n", 1000./temps);
                             printf ("render time : %d ms\n", temps); break;
                case SDLK_s: smooth = !smooth; break;
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
        SCE_Matrix4_MulTranslate (matrix, 0.0, 0.0, -2.0);
        
        /* render depth map */
        SCE_Shader_Use (genshadow);
        SCE_Shader_Lock ();
        SCE_Scene_Update (scene, lcam, fb, 0);
        SCE_Scene_Render (scene, lcam, fb, 0);
        SCE_Shader_Unlock ();

        /* final render */
        mat = SCE_Camera_GetFinalViewProj (lcam);
        SCE_Shader_Use (rendershadow);
        SCE_Shader_SetMatrix4 (SCE_Shader_GetIndex(rendershadow, "matrix0"),
                               mat);
        SCE_Shader_SetMatrix4 (SCE_Shader_GetIndex(rendershadow, "matrix1"),
                               SCE_Node_GetFinalMatrix(SCE_Model_GetRootNode(mdl)));
        SCE_Texture_Use (fb);
        SCE_Texture_Lock ();
        SCE_Shader_Lock ();
        SCE_Scene_Update (scene, cam, NULL, 0);
        SCE_Scene_Render (scene, cam, NULL, 0);
        SCE_Shader_Unlock ();
        SCE_Texture_Unlock ();
        
        SDL_GL_SwapBuffers ();
        verif (SCEE_HaveError ())
        temps = SDL_GetTicks () - tm;
        wait = (1000.0/FPS) - temps;
        if (wait > 0)
            SDL_Delay (wait);
    }
    
    SCE_Scene_Delete (scene);
    SCE_Camera_Delete (cam);
    SCE_Camera_Delete (lcam);
    
    SCE_Shader_Delete (genshadow);
    SCE_Shader_Delete (rendershadow);
    
    SCE_Light_Delete (l);
    SCE_Model_Delete (mdl);
    SCE_Texture_Delete (fb);

    SCE_Quit_Interface ();

    SDL_Quit ();

    return 0;
}
