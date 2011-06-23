#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL/SDL.h>
#include <SCE/interface/SCEInterface.h>

#define W 1024
#define H 768

#define MW (W * 2)
#define MH (H * 2)

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
    int mouse_pressed = 0, wait, temps = 0, tm;
    SCE_SLight *l = NULL, *l2 = NULL, *l3 = NULL;
    SCE_SMesh *mesh = NULL;
    float *matrix = NULL;
    SCE_SScene *scene = NULL;
    float dist = 6.0;
    SCE_SShader *srender = NULL, *finalrender = NULL;
    SCE_STexture *gbuffer = NULL, *depthmap = NULL;
    SCE_STexture *normalmap = NULL;
    SCE_SNode *node = NULL;
    SCE_SModel *mdl = NULL;
    SCE_SListIterator *it = NULL;
    double temps_moy = 0.0;

    srand (time (NULL));
    SDL_Init (SDL_INIT_VIDEO);
    /* parametres SDL */
    SDL_WM_SetCaption ("Deferred shading - SCEngine v0.1.0 Alpha", NULL);
    /* initialisation finale */
    if (SDL_SetVideoMode (W, H, 32, SDL_OPENGL) == NULL) {
        fprintf (stderr, "failed to setup window: %s\n", SDL_GetError ());
        return EXIT_FAILURE;
    }
    SDL_EnableKeyRepeat (1, 0);
    /* initialisation du SCEngine */
    SCE_Init_Interface (stderr, 0);
    verif (SCEE_HaveError())
    SCE_OBJ_ActivateIndicesGeneration (1);
    
    /* create the scene */
    scene = SCE_Scene_Create ();
    cam = SCE_Camera_Create ();
    matrix = SCE_Camera_GetView (cam);
    /* setup camera */
    SCE_Camera_SetViewport (cam, 0, 0, MW, MH);
    SCE_Matrix4_Projection(SCE_Camera_GetProj(cam), 70.*RAD, (float)W/H, .1, 100.);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (cam->node, SCE_NODE_WRITE_MATRIX),
                           0., 0., 2.);
    SCE_Scene_AddCamera (scene, cam);
    
    /* G-buffer creation */
    gbuffer = SCE_Texture_Create (SCE_RENDER_COLOR, MW, MH);
    SCE_Texture_Build (gbuffer, SCE_FALSE);
    depthmap = SCE_Texture_Create (SCE_RENDER_DEPTH, MW, MH);
    SCE_Texture_Build (depthmap, SCE_FALSE);
    normalmap = SCE_Texture_Create (SCE_TEX_2D, MW, MH);
    SCE_Texture_Build (normalmap, SCE_FALSE);
    verif (SCEE_HaveError ())

    SCE_Texture_AddRenderTexture (gbuffer, SCE_DEPTH_BUFFER, depthmap);
    SCE_Texture_AddRenderTexture (gbuffer, SCE_COLOR_BUFFER1, normalmap);
    verif (SCEE_HaveError ())
    /* setup texture's units */
    SCE_Texture_SetUnit (gbuffer, 0);
    SCE_Texture_SetUnit (depthmap, 1);
    SCE_Texture_SetUnit (normalmap, 2);
    /* dont smooth texels that appear bigger than a pixel */
    SCE_Texture_Pixelize (gbuffer, SCE_TRUE);
    SCE_Texture_Pixelize (depthmap, SCE_TRUE);
    SCE_Texture_Pixelize (normalmap, SCE_TRUE);
    /* dont smooth texels that appear smaller than a pixel */
    SCE_Texture_SetFilter (gbuffer, SCE_TEX_NEAREST);
    SCE_Texture_SetFilter (depthmap, SCE_TEX_NEAREST);
    SCE_Texture_SetFilter (normalmap, SCE_TEX_NEAREST);

    /* build the scene's unique model */
    srender = SCE_Shader_Load ("render.glsl", SCE_FALSE);
    SCE_Shader_Build (srender);
    verif (SCEE_HaveError ())
    finalrender = SCE_Shader_Load ("finalrender.glsl", SCE_FALSE);
    SCE_Shader_Build (finalrender);
    verif (SCEE_HaveError ())
    /* set shader's parameter according to texture's unit */
    SCE_Shader_Use (finalrender);
    SCE_Shader_Param ("colormap", 0);
    SCE_Shader_Param ("depthmap", 1);
    SCE_Shader_Param ("normalmap", 2);
    SCE_Shader_Use (NULL);
    /* load mesh */
    mesh = SCE_Mesh_Load ("../data/dofscene4.obj", SCE_FALSE);
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
    
    /* setup lights */
    l = SCE_Light_Create ();
    SCE_Light_SetColor (l, 0.1, 0.2, .5);
    SCE_Light_Infinite (l, SCE_TRUE);
    node = SCE_Light_GetNode (l);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           -2., 2., 1.);
    SCE_Scene_AddLight (scene, l);

    /* moar lights */
#if 1
    l2 = SCE_Light_Create ();
    SCE_Light_SetColor (l2, 0.5, 0.2, 0.1);
    SCE_Light_Infinite (l2, SCE_FALSE);
    node = SCE_Light_GetNode (l2);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           3., 2., 1.);
    SCE_Scene_AddLight (scene, l2);

    l3 = SCE_Light_Create ();
    SCE_Light_SetColor (l3, 0.2, 0.5, 0.1);
    SCE_Light_Infinite (l3, SCE_FALSE);
    node = SCE_Light_GetNode (l3);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           2., -2., 1.);
    SCE_Scene_AddLight (scene, l3);

    l3 = SCE_Light_Create ();
    SCE_Light_SetColor (l3, 0.5, 0.5, 0.1);
    SCE_Light_Infinite (l3, SCE_FALSE);
    node = SCE_Light_GetNode (l3);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           -2., -2.0, 2.);
    SCE_Scene_AddLight (scene, l3);

    l = SCE_Light_Create ();
    SCE_Light_SetColor (l, 0.1, 0.2, 0.5);
    SCE_Light_Infinite (l, SCE_FALSE);
    node = SCE_Light_GetNode (l);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           2., 2., 3.);
    SCE_Scene_AddLight (scene, l);

    l2 = SCE_Light_Create ();
    SCE_Light_SetColor (l2, 0.5, 0.2, 0.1);
    SCE_Light_Infinite (l2, SCE_FALSE);
    node = SCE_Light_GetNode (l2);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           -2., -2., 5.);
    SCE_Scene_AddLight (scene, l2);

    l3 = SCE_Light_Create ();
    SCE_Light_SetColor (l3, 0.2, 0.5, 0.1);
    SCE_Light_Infinite (l3, SCE_FALSE);
    node = SCE_Light_GetNode (l3);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           -2., 2., 5.);
    SCE_Scene_AddLight (scene, l3);

    l3 = SCE_Light_Create ();
    SCE_Light_SetColor (l3, 0.7, 0.3, 0.2);
    SCE_Light_Infinite (l3, SCE_FALSE);
    node = SCE_Light_GetNode (l3);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           1., -1., 3.);
    SCE_Scene_AddLight (scene, l3);
#endif

    /* moarmoar lights § */
#if 0
    l3 = SCE_Light_Create ();
    SCE_Light_SetColor (l3, 0.1, 0.5, 0.5);
    SCE_Light_Infinite (l3, SCE_FALSE);
    node = SCE_Light_GetNode (l3);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           2., 0., 2.);
    SCE_Scene_AddLight (scene, l3);

    l3 = SCE_Light_Create ();
    SCE_Light_SetColor (l3, 0.5, 0.1, 0.5);
    SCE_Light_Infinite (l3, SCE_FALSE);
    node = SCE_Light_GetNode (l3);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           0., 2., 1.);
    SCE_Scene_AddLight (scene, l3);

    l3 = SCE_Light_Create ();
    SCE_Light_SetColor (l3, 0.2, 0.5, 0.4);
    SCE_Light_Infinite (l3, SCE_FALSE);
    node = SCE_Light_GetNode (l3);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           -2., 0., 1.);
    SCE_Scene_AddLight (scene, l3);

    l3 = SCE_Light_Create ();
    SCE_Light_SetColor (l3, 0.4, 0.2, 0.5);
    SCE_Light_Infinite (l3, SCE_FALSE);
    node = SCE_Light_GetNode (l3);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           -0., 2., 2.);
    SCE_Scene_AddLight (scene, l3);


    l3 = SCE_Light_Create ();
    SCE_Light_SetColor (l3, 0.1, 0.5, 0.5);
    SCE_Light_Infinite (l3, SCE_FALSE);
    node = SCE_Light_GetNode (l3);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           1., 0., .5);
    SCE_Scene_AddLight (scene, l3);

    l3 = SCE_Light_Create ();
    SCE_Light_SetColor (l3, 0.5, 0.1, 0.5);
    SCE_Light_Infinite (l3, SCE_FALSE);
    node = SCE_Light_GetNode (l3);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           0., 1., .5);
    SCE_Scene_AddLight (scene, l3);

    l3 = SCE_Light_Create ();
    SCE_Light_SetColor (l3, 0.2, 0.5, 0.4);
    SCE_Light_Infinite (l3, SCE_FALSE);
    node = SCE_Light_GetNode (l3);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           -1., 0., .5);
    SCE_Scene_AddLight (scene, l3);

    l3 = SCE_Light_Create ();
    SCE_Light_SetColor (l3, 0.4, 0.2, 0.5);
    SCE_Light_Infinite (l3, SCE_FALSE);
    node = SCE_Light_GetNode (l3);
    SCE_Matrix4_Translate (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                           -0., 1., .5);
    SCE_Scene_AddLight (scene, l3);
#endif

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
                case SDLK_ESCAPE: loop = 0; break;
                case SDLK_t:
                    printf ("fps : %.2f\n", 1000./temps);
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

        /* render to G-buffer */
        SCE_Scene_Update (scene, cam, NULL, 0);
        SCE_Scene_Render (scene, cam, gbuffer, 0);

        /* update matrix */
        SCE_Matrix4_Mul (SCE_Camera_GetFinalViewInverse (cam),
                         SCE_Camera_GetProjInverse (cam),
                         SCE_Texture_GetMatrix (gbuffer));

        /* process lighting */
        SCE_Shader_Use (finalrender);
        /* setup states */
        SCE_RSetState2 (GL_DEPTH_TEST, GL_CULL_FACE, SCE_FALSE);
        SCE_RSetState (GL_BLEND, SCE_TRUE);
        SCE_RSetBlending (GL_ONE, GL_ONE);
        /* setup gbuffer */
        SCE_Texture_Use (gbuffer);
        SCE_Texture_Use (depthmap);
        SCE_Texture_Use (normalmap);
        SCE_Texture_RenderTo (NULL, 0);
        SCE_RClearColor (0.0, 0.0, 0.0, 0.0);
        SCE_RClear (GL_COLOR_BUFFER_BIT);

        SCE_Light_ActivateLighting (SCE_TRUE);
        SCE_RActivateDepthBuffer (SCE_FALSE);

        SCE_RLoadMatrix (SCE_MAT_CAMERA, sce_matrix4_id);
        SCE_RLoadMatrix (SCE_MAT_PROJECTION, sce_matrix4_id);
        SCE_List_ForEach (it, &scene->lights) {
            SCE_Light_Use (SCE_List_GetData (it));
            SCE_Quad_Draw (-1.0, -1.0, 2.0, 2.0);
            SCE_Light_Use (NULL);
        }

        SCE_RActivateDepthBuffer (SCE_TRUE);
        SCE_Light_ActivateLighting (SCE_FALSE);

        SCE_Texture_Use (NULL);
        SCE_RSetState (GL_BLEND, SCE_FALSE);
        SCE_RSetState2 (GL_DEPTH_TEST, GL_CULL_FACE, SCE_TRUE);
        SCE_Shader_Use (NULL);

        /* debug stuff */
#if 0
        SCE_Texture_SetUnit (depthmap, 0);
        SCE_Texture_Blitf (NULL, NULL, NULL, depthmap);
        SCE_Texture_SetUnit (depthmap, 1);
#elif 0
        SCE_Texture_SetUnit (normalmap, 0);
        SCE_Texture_Blitf (NULL, NULL, NULL, normalmap);
        SCE_Texture_SetUnit (normalmap, 2);
#endif
        
        temps_moy = (temps + temps_moy) / 2.0;
        
        SDL_GL_SwapBuffers ();
        verif (SCEE_HaveError ())
            temps = SDL_GetTicks () - tm;
        wait = (1000.0/FPS) - temps;
        if (wait > 0)
            SDL_Delay (wait);
    }
    
    /* we're done */
    SCE_Scene_Delete (scene);
    SCE_Camera_Delete (cam);
    SCE_Light_Delete (l);
    
    SCE_Model_Delete (mdl);

    SCE_Quit_Interface ();

    SDL_Quit ();

    printf ("fps average: %.2ffps\n", 1000./temps_moy);
    printf ("render time average: %.2fms\n", temps_moy);

    return 0;
}
