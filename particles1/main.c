
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

/* emission rate */
#define EMIT_PARTICLE 400.0f

/* maximum and minimum life duration of particles */
#define MIN_LIFE 0.8f
#define MAX_LIFE 1.2f

float myrand (float min, float max)
{
    float r = rand ();
    r = (r/RAND_MAX) * (max - min) + min;
    return r;
}

void init_array (const SCE_SParticle *part, void *array, void *data)
{
    GLubyte *c;
    SCEvertices *p = array;

    c = &p[3];
    (void)data;
    c[0] = 90;
    c[1] = 50;
    c[2] = 20;
    SCE_Vector3_Copy (p, part->position);
}

void update_array (const SCE_SParticle *part, void *array, void *data)
{
    const float limit = 0.6;    /* when particles have this time left to live
                                   they begin to fade out */
    GLubyte *c;
    SCEvertices *p = array;

    c = &p[3];
    (void)data;
    c[0] = 90;
    c[1] = 50;
    c[2] = 20;

    if (part->age < limit) {
        c[0] *= part->age / limit;
        c[1] *= part->age / limit;
        c[2] *= part->age / limit;
    }

    SCE_Vector3_Copy (p, part->position);
}

void emit_particle (SCE_SParticle *part, SCE_SParticleEmitter *emitter, void *data)
{
    float *mat, pos[2], angle;
    (void)data;
    angle = myrand (0.0f, 2.0f * M_PI);
    pos[0] = cos (angle);
    pos[1] = sin (angle);
    angle = myrand (1.8f, 2.2f);
    SCE_Vector2_Operator2 (part->velocity, =, pos, *, angle);
    part->velocity[2] = 6.7f;
    SCE_Vector3_Normalize (part->velocity);
    angle = myrand (5.8f, 9.5f);
    SCE_Vector3_Operator1 (part->velocity, *=, angle);
    SCE_Vector3_Operator1v (part->velocity, +=, SCE_ParticleEmit_GetVelocity (emitter));
    mat = SCE_Node_GetFinalMatrix (SCE_ParticleEmit_GetNode (emitter));
    SCE_Matrix4_GetTranslation (mat, part->position);
    part->age = myrand (SCE_ParticleEmit_GetMinLife (emitter),
                        SCE_ParticleEmit_GetMaxLife (emitter));
}
void update_particle (SCE_SParticle *part, float sec, void *data)
{
    const float g = 9.8f, mass = 0.5f;
    part->age -= sec;
    part->velocity[2] -= sec * g / mass;
#if 1
#define ROUGHNESS 0.1f
    part->velocity[0] += myrand (-ROUGHNESS, ROUGHNESS);
    part->velocity[1] += myrand (-ROUGHNESS, ROUGHNESS);
#endif
    SCE_Vector3_Operator2 (part->position, +=, part->velocity, *, sec);
}

int main (void)
{
    SCE_SInertVar rx, ry;
    SCE_SCamera *cam = NULL;
    SDL_Event ev;
    int loop = 1;
    float angle_y = 0., angle_x = 0., back_x = 0., back_y = 0.;
    int mouse_pressed = 0, wait, temps = 0, tm, i;
    SCE_SMesh *mesh = NULL;
    SCE_SGeometry *geom = NULL;
    float *matrix = NULL;
    SCE_SScene *scene = NULL;
    float dist = 6.0;
    SCE_SSceneEntity *entity = NULL;
    SCE_SSceneEntityInstance *inst;
    SCE_SSceneEntityGroup *egroup;
    int animate = SCE_FALSE;
    SCE_SParticleSystem *psys = NULL;
    SCE_RPointSprite ps;
    SCE_SMaterial *material = NULL;
    SCE_STexture *tex = NULL;
    SCE_SParticleModifier mod;

    srand (time (NULL));
    SDL_Init (SDL_INIT_VIDEO);
    SDL_WM_SetCaption ("Particles system demo 1", NULL);
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

    scene->rclear = scene->gclear = scene->bclear = scene->aclear = 0.0f;
    SCE_Scene_AddCamera (scene, cam);

    psys = SCE_ParticleSys_Create ();

    SCE_ParticleSys_SetEmissionFrequency (psys, EMIT_PARTICLE);
    SCE_Particle_SetInitArraysCallback (&psys->buf, init_array, NULL);
    SCE_Particle_SetUpdateArraysCallback (&psys->buf, update_array, NULL);
    SCE_ParticleMod_Init (&mod);
    SCE_ParticleMod_SetEmitCallback (&mod, emit_particle);
    SCE_ParticleMod_SetUpdateCallback (&mod, update_particle);
    SCE_ParticleProc_AddModifier (&psys->proc, &mod);
    SCE_ParticleEmit_SetMinLife (&psys->emitter, MIN_LIFE);
    SCE_ParticleEmit_SetMaxLife (&psys->emitter, MAX_LIFE);
    /* tricky maths § */
    SCE_Particle_SetMaxParticles (&psys->buf, EMIT_PARTICLE * (0.1 + MIN_LIFE + (MAX_LIFE - MIN_LIFE) / 2));
    {
        int prout[7] = {1, SCE_POSITION, SCE_VERTICES_TYPE, 3,
                        SCE_ROLOR, SCE_UNSIGNED_BYTE, 3};
        SCE_Particle_BuildArrays (&psys->buf, SCE_POINTS, prout, 2);
    }
    verif (SCEE_HaveError())

    SCE_ParticleSys_Build (psys);

    geom = SCE_Particle_CreateGeometry (&psys->buf);
    SCE_Geometry_SetPrimitiveType (geom, SCE_POINTS);
    entity = SCE_SceneEntity_Create ();
    egroup = SCE_SceneEntity_CreateGroup ();
    mesh = SCE_Mesh_CreateFrom (geom, SCE_FALSE);
    SCE_Mesh_AutoBuild (mesh);
    SCE_Mesh_SetRenderMode (mesh, SCE_VA_RENDER_MODE);
    SCE_SceneEntity_SetMesh (entity, mesh);

#if 1
    {
        SCE_SSceneEntityProperties *prop;
        prop = SCE_SceneEntity_GetProperties (entity);
        prop->cullface = SCE_FALSE;
        prop->depthtest = SCE_FALSE;
    }

    SCE_RInitPointSprite (&ps);
    SCE_RSetPointSpriteSize (&ps, 6.0f);
    SCE_RSetPointSpriteAttenuations (&ps, 0.0f, 0.f, 0.01f);
    SCE_REnablePointSpriteTexture (&ps);
    material = SCE_Material_Create ();
    SCE_Material_SetPointSpriteModel (material, &ps);
    SCE_Material_EnablePointSprite (material);
    SCE_Material_SetBlending (material, GL_ONE, GL_ONE);
    SCE_Material_EnableBlending (material);
    tex = SCE_Texture_Load (0, 0, 0, 0, 0, "../data/sprite.png", NULL);
    SCE_Texture_Build (tex, 1);
    SCE_SceneEntity_SetMaterial (entity, material);
    SCE_SceneEntity_AddTexture (entity, tex);
#endif
    SCE_SceneEntity_AddEntity (egroup, 0, entity);

    verif (SCEE_HaveError ())

    SCE_Inert_Init (&rx);
    SCE_Inert_Init (&ry);

    SCE_Inert_SetCoefficient (&rx, 0.1);
    SCE_Inert_SetCoefficient (&ry, 0.1);
    SCE_Inert_Accum (&rx, 1);
    SCE_Inert_Accum (&ry, 1);

    inst = SCE_SceneEntity_CreateInstance ();
    SCE_SceneEntity_AddInstance (egroup, inst);
    SCE_Scene_AddInstance (scene, inst);
    SCE_Scene_AddEntity (scene, entity);
    SCE_Scene_AddEntityResources (scene, entity);
    verif (SCEE_HaveError ())

    SCE_ParticleEmit_SetNode (&psys->emitter, SCE_SceneEntity_GetInstanceNode (inst));

    scene->states.lighting = SCE_FALSE;
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
                case SDL_BUTTON_WHEELUP: dist -= 1.; break;
                case SDL_BUTTON_WHEELDOWN: dist += 1.; break;
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
                case SDLK_f:
                    scene->states.frustum_culling = !scene->states.frustum_culling; break;
                case SDLK_l:
                    scene->states.lod = !scene->states.lod; break;
                case SDLK_t:
                    printf ("fps : %.2f\n", 1000./temps);
                    printf ("update time : %d ms\n", i);
                    printf ("total time : %d ms\n", temps);
                    break;
                case SDLK_a: animate = !animate;
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

        SCE_Node_HasMoved (inst->node);

#define TIME_ELAPSED 0.001 /* that's not true obviously */
        SCE_ParticleSys_Update (psys, TIME_ELAPSED);
        SCE_Geometry_Update (geom);
        SCE_RUpdateModifiedBuffers ();

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
    
    SCE_SceneEntity_DeleteInstance (inst);
    
    SCE_SceneEntity_Delete (entity);
    SCE_Mesh_Delete (mesh);

    SCE_Quit_Interface ();

    SDL_Quit ();

    return 0;
}
