
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL/SDL.h>

/* SCEngine */
#include <SCE/interface/SCEInterface.h>

#include "perlin.h"

#define W 1024
#define H 768

/* max FPS */
#define FPS 60

#define verif(cd)\
if (cd) {\
    SCEE_LogSrc ();\
    SCEE_Out ();\
    exit (EXIT_FAILURE);\
}

#define GRID_SIZE 64

#define GW (GRID_SIZE)
#define GH (GRID_SIZE)
#define GD (GRID_SIZE)

#define MAP_W (GW * 2)
#define MAP_H (GH * 2)
#define MAP_D (GW * 1)

static float myrand (float min, float max)
{
    float r = rand ();
    r /= RAND_MAX;
    r *= (max - min);
    r += min;
    return r;
}

unsigned char density_function (long x, long y, long z)
{
    double p[3], p2[3];
//    float ground = 50.0 + 100.0 * (10.0/(x + 10) + 10.0/(y + 10));
#if 0
    double derp = (-z + 32) * 0.28;
#else
    /* TODO: first seed only work with these parameters */
    double derp = (-z + 50) * 0.07;
//    double derp = (-z + 100) * 0.07;
#endif
    double val;

    p[0] = x; p[1] = y; p[2] = z;

    SCE_Vector3_Operator1v (p2, = 0.012 *, p);
    p2[0] = noise3 (p2) * 10.0;
    p2[1] = p2[0];
    p2[2] = p2[0];


    SCE_Vector3_Operator1v (p, += 8.0 *, p2);
    SCE_Vector3_Operator1 (p, *=, 0.008);
    derp += noise3 (p) * 8.0;
//    derp += noise3 (p) * 12.0;

    SCE_Vector3_Operator1 (p, *=, 2.0);
    derp += noise3 (p) * 5.0;
    SCE_Vector3_Operator1 (p, *=, 3.5);
    derp += noise3 (p) * 0.9;
    SCE_Vector3_Operator1 (p, *=, 2.6);
    derp += noise3 (p) * 0.27;

    derp = SCE_Math_Clampf (derp, 0.0, 1.0);
    return derp * 255;
}

static void generate_terrain (SCE_SVoxelStorage *vs)
{
    int x, y, z;
    int w, h, d;
    SCE_SIntRect3 r;

    w = SCE_VStore_GetWidth (vs);
    h = SCE_VStore_GetHeight (vs);
    d = SCE_VStore_GetDepth (vs);

    for (z = 0; z < d; z++) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                unsigned char p = density_function (x, y, z);
                SCE_VStore_SetPoint (vs, x, y, z, &p);
                SCE_VStore_GetNextUpdatedZone (vs, &r);
            }
        }
    }
}

#define LOLOD 1

static void apply_sphere (SCE_SVoxelTerrain *vt, SCE_SVoxelStorage *vs,
                          const SCE_SSphere *s, int fill)
{
    SCE_TVector3 pos, center;
    float r, dist;
    int w, h, d, x, y, z;
    int startx, starty, startz;
    SCE_SIntRect3 rect;
    /* hope that radius doesnt exceed 14 */
    unsigned char buf[32*32*32] = {0};

    SCE_Rectangle3_Init (&rect);

    w = SCE_VStore_GetWidth (vs);
    h = SCE_VStore_GetHeight (vs);
    d = SCE_VStore_GetDepth (vs);

    r = SCE_Sphere_GetRadius (s);
    SCE_Sphere_GetCenterv (s, center);

#define HERP 1.0
    x = (int)(center[0] - r - HERP);
    y = (int)(center[1] - r - HERP);
    z = (int)(center[2] - r - HERP);
    w = MIN (center[0] + r + HERP + 0.5, w);
    h = MIN (center[1] + r + HERP + 0.5, h);
    d = MIN (center[2] + r + HERP + 0.5, d);

    startx = MAX (x, 0);
    starty = MAX (y, 0);
    startz = MAX (z, 0);

    SCE_Rectangle3_Set (&rect, startx, starty, startz, w, h, d);

    SCE_VStore_GetRegion (vs, 0, &rect, buf);

    w = SCE_Rectangle3_GetWidth (&rect);
    h = SCE_Rectangle3_GetHeight (&rect);
    d = SCE_Rectangle3_GetDepth (&rect);

    for (z = 0; z < d; z++) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                unsigned char p;
                size_t offset = w * (h * z + y) + x;
                p = buf[offset];

                SCE_Vector3_Set (pos, x + startx, y + starty, z + startz);
                dist = SCE_Vector3_Distance (pos, center);
                if (dist <= r + 1) {
                    float w = 0.0;
                    if (dist >= r - 1)
                        w = (dist - r + 1.0) / 2.0;

                    if (fill)
                        p = MAX (p, (1.0 - w) * 255);
                    else
                        p = MIN (p, w * 255);

                    buf[offset] = p;
                }
            }
        }
    }

    SCE_VStore_SetRegion (vs, &rect, buf);
    SCE_VStore_GenerateAllLOD (vs, 1, &rect);
}


static void update_grid (SCE_SVoxelTerrain *vt, SCEuint level,
                         SCE_EBoxFace f, SCE_SVoxelStorage *vs)
{
    long x, y, z;
    int w, h, d;
    long origin_x, origin_y, origin_z;

    /* TODO: let's hope GW >= GH */
    unsigned char buf[GW * GW];

    SCE_SIntRect3 r;

    w = SCE_VTerrain_GetWidth (vt);
    h = SCE_VTerrain_GetHeight (vt);
    d = SCE_VTerrain_GetDepth (vt);

    SCE_VTerrain_GetOrigin (vt, level, &origin_x, &origin_y, &origin_z);
    x = origin_x;
    y = origin_y;
    z = origin_z;

    switch (f) {
    case SCE_BOX_POSX:
        x += w + 1;
    case SCE_BOX_NEGX:
        x--;
        SCE_Rectangle3_Set (&r, x, origin_y, z, x+1, origin_y + h, z+d);
        SCE_VStore_GetRegion (vs, level, &r, buf);
        break;
    case SCE_BOX_POSY:
        y += h + 1;
    case SCE_BOX_NEGY:
        y--;
        SCE_Rectangle3_Set (&r, origin_x, y, z, origin_x+w, y+1, z+d);
        SCE_VStore_GetRegion (vs, level, &r, buf);
    }

    SCE_VTerrain_AppendSlice (vt, level, f, buf);
}


int main (void)
{
    SCE_SInertVar rx, ry;
    SCE_SCamera *cam = NULL;
    SDL_Event ev;
    int loop = 1;
    float angle_y = 0., angle_x = 0., back_x = 0., back_y = 0.;
    int mouse_pressed = 0, wait, temps = 0, tm, i, j;
    float *matrix = NULL;
    SCE_SLight *l;
    SCE_SGeometry *geom = NULL;
    SCE_SMesh *mesh = NULL;
    SCE_SModel *model = NULL;
    SCE_SScene *scene = NULL;
    float dist = GRID_SIZE;
    int x, y;
    SCE_SVoxelTerrain *vt = NULL;
    SCE_STexture *diffuse = NULL;
    SCE_SSphere sphere;
    SCE_TVector3 center;
    int fillmode = SCE_FALSE;
    int apply_mode = SCE_FALSE;
    SCE_SVoxelStorage *vs = NULL;
    SCE_SIntRect3 rect;
    int first_draw = SCE_FALSE;

    SCE_SDeferred *def = NULL;
    SCE_SShader *lodshader = NULL;
    int shadows = SCE_FALSE;
    time_t mdr;

#if 1
    mdr = time (NULL);
    srand (mdr);
    printf ("time: %d\n", mdr);
#else
    /* 58, 62: interesting cave */
    /* 68: relatively flat */
    /* 1334426723: big hole & stuff */
    /* 1334437699: nice shape blahblah */
    /* 1334365870: ?? */
    srand (1334451274);
#endif

    SDL_Init (SDL_INIT_VIDEO);
    SDL_WM_SetCaption ("Terrain editing", NULL);
    if (SDL_SetVideoMode (W, H, 32, SDL_OPENGL) == NULL) {
        fprintf (stderr, "failed to setup window: %s\n", SDL_GetError ());
        return EXIT_FAILURE;
    }
    SDL_EnableKeyRepeat (100, 15);
    verif (SCE_Init_Interface (stderr, 0) < 0)
    init ();
    
    scene = SCE_Scene_Create ();
    cam = SCE_Camera_Create ();
    matrix = SCE_Camera_GetView (cam);
    SCE_Camera_SetViewport (cam, 0, 0, W, H);
    SCE_Camera_SetProjection (cam, 70.*RAD, (float)W/H, .1, 1000.);
    SCE_Scene_AddCamera (scene, cam);
    
#if 0
    SCE_Scene_SetOctreeSize (scene, 640.0, 640.0, 640.0);
    SCE_Scene_MakeOctree (scene, 1, SCE_FALSE, 0.0);
#endif

    SCE_Sphere_Init (&sphere);
    SCE_Sphere_SetCenter (&sphere, GW / 2.0, GH / 2.0, GD / 2.0);
    SCE_Sphere_SetRadius (&sphere, 3.6);

    def = SCE_Deferred_Create ();
    SCE_Deferred_SetDimensions (def, W, H);
    SCE_Deferred_SetShadowMapsDimensions (def, 1024, 1024);
    SCE_Deferred_SetCascadedSplits (def, 3);
    SCE_Deferred_SetCascadedFar (def, 200.0);
    {
      const char *fnames[SCE_NUM_LIGHT_TYPES];
      fnames[SCE_POINT_LIGHT] = "point.glsl";
      fnames[SCE_SUN_LIGHT] = "sun.glsl";
      fnames[SCE_SPOT_LIGHT] = "spot.glsl";
      SCE_Deferred_Build (def, fnames);
    }
#if 0
    {
        const char *msg = SCE_RGetError ();
        if (msg) {
            SCEE_Log (42);
            SCEE_LogMsg ("gl error occurred belol: %s", msg);
            return SCE_ERROR;
        }
    }
#endif
    verif (SCEE_HaveError ())
    SCE_Scene_SetDeferred (scene, def);

    lodshader = SCE_Shader_Load ("render.glsl", SCE_FALSE);
    SCE_Deferred_BuildShader (def, lodshader);
    SCE_Deferred_BuildPointShadowShader (def, lodshader);

    vt = SCE_VTerrain_Create ();
    /* distance between voxels is 0.5 meters */
    SCE_VTerrain_SetUnit (vt, 0.5);
    SCE_VTerrain_SetDimensions (vt, GW, GH, GD);

#if 0
    SCE_VTerrain_CompressPosition (vt, SCE_TRUE);
    SCE_VTerrain_CompressNormal (vt, SCE_TRUE);
#else
    SCE_VTerrain_CompressPosition (vt, SCE_FALSE);
    SCE_VTerrain_CompressNormal (vt, SCE_FALSE);
#endif

#if LOLOD
    SCE_VTerrain_SetNumLevels (vt, 3);
#else
    SCE_VTerrain_SetNumLevels (vt, 2);
#endif
#if 1
    SCE_VTerrain_SetSubRegionDimension (vt, 11);
    SCE_VTerrain_SetNumSubRegions (vt, 5);
#elif 1
    SCE_VTerrain_SetSubRegionDimension (vt, 21);
    SCE_VTerrain_SetNumSubRegions (vt, 5);
#endif
    SCE_VTerrain_BuildShader (vt, lodshader);
    SCE_VTerrain_SetShader (vt, lodshader);

    if (SCE_VTerrain_Build (vt) < 0) {
        SCEE_Out ();
        return EXIT_FAILURE;
    }

    /* load and setup diffuse texture of the terrain */
    if (!(diffuse = SCE_Texture_Load (SCE_TEX_2D, 0, 0, 0, 0,
                                      "../data/grass2.jpg", NULL))) {
        SCEE_Out ();
        return 42;
    }
    SCE_Texture_Build (diffuse, SCE_TRUE);
    SCE_VTerrain_SetTopDiffuseTexture (vt, diffuse);

    if (!(diffuse = SCE_Texture_Load (SCE_TEX_2D, 0, 0, 0, 0,
                                      "../data/rock1.jpg", NULL))) {
        SCEE_Out ();
        return 42;
    }
    SCE_Texture_Build (diffuse, SCE_TRUE);
    SCE_VTerrain_SetSideDiffuseTexture (vt, diffuse);


    /* create voxel data */
    vs = SCE_VStore_Create ();
    SCE_VStore_SetDimensions (vs, MAP_W, MAP_H, MAP_D);
    SCE_VStore_SetDataSize (vs, 1);
#if LOLOD
    SCE_VStore_SetNumLevels (vs, 3);
#else
    SCE_VStore_SetNumLevels (vs, 2);
#endif
    {
        unsigned char vacuum = 64;
        SCE_VStore_Build (vs, &vacuum);
    }

    printf ("generating terrain...\n");
    i = SDL_GetTicks ();
    generate_terrain (vs);
    printf ("terrain generation: %.3fs\n",
            ((float)(SDL_GetTicks () - i)) / 1000.0);

    i = SDL_GetTicks ();
    SCE_Rectangle3_Set (&rect, 0, 0, 0, MAP_W+1, MAP_H+1, MAP_D+1);
    SCE_VStore_GenerateAllLOD (vs, 1, &rect);
    SCE_Rectangle3_Set (&rect, 0, 0, 0, MAP_W, MAP_H, MAP_D);
    SCE_VStore_ForceUpdate (vs, &rect, 0);
    SCE_Scene_SetVoxelTerrain (scene, vt);
    printf ("LOD generation: %.3fs\n",
            ((float)(SDL_GetTicks () - i)) / 1000.0);

    SCE_VTerrain_UpdateGrid (vt, 0, SCE_FALSE);
    SCE_VTerrain_UpdateGrid (vt, 1, SCE_FALSE);
#if LOLOD
    SCE_VTerrain_UpdateGrid (vt, 2, SCE_FALSE);
//    SCE_VTerrain_UpdateGrid (vt, 3, SCE_FALSE);
#endif

    /* sky lighting */
    l = SCE_Light_Create ();
    SCE_Light_SetColor (l, 0.7, 0.8, 1.0);
    SCE_Light_SetIntensity (l, 0.3);
    SCE_Light_SetType (l, SCE_SUN_LIGHT);
    SCE_Light_SetPosition (l, 0., 0., 1.);
    SCE_Scene_AddLight (scene, l);

    /* sun lighting */
    l = SCE_Light_Create ();
    SCE_Light_SetColor (l, 1.0, 0.9, 0.85);
    SCE_Light_SetType (l, SCE_SUN_LIGHT);
    SCE_Light_SetPosition (l, 2., 2., 2.);
    SCE_Light_SetShadows (l, shadows);
    SCE_Scene_AddLight (scene, l);

    verif (SCEE_HaveError ())

    SCE_Inert_Init (&rx);
    SCE_Inert_Init (&ry);

    SCE_Inert_SetCoefficient (&rx, 0.1);
    SCE_Inert_SetCoefficient (&ry, 0.1);
    SCE_Inert_Accum (&rx, 1);
    SCE_Inert_Accum (&ry, 1);

    scene->state->deferred = SCE_TRUE;
    scene->state->lighting = SCE_TRUE;
#if 1
    scene->state->frustum_culling = SCE_TRUE;
    scene->state->lod = SCE_TRUE;
#endif
    scene->rclear = 0.6;
    scene->gclear = 0.7;
    scene->bclear = 1.0;
    temps = 0;

    x = y = 0;

    while (loop) {
        int level;

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

            case SDL_KEYDOWN:
                switch (ev.key.keysym.sym) {
                case SDLK_z: y += 1; break;
                case SDLK_s: y -= 1; break;
                case SDLK_d: x += 1; break;
                case SDLK_q: x -= 1; break;

#define MOVE 0.2
                case SDLK_LEFT:
                    SCE_Sphere_GetCenterv (&sphere, center);
                    center[0] -= MOVE;
                    SCE_Sphere_SetCenterv (&sphere, center);
                    break;
                case SDLK_RIGHT:
                    SCE_Sphere_GetCenterv (&sphere, center);
                    center[0] += MOVE;
                    SCE_Sphere_SetCenterv (&sphere, center);
                    break;
                case SDLK_UP:
                    SCE_Sphere_GetCenterv (&sphere, center);
                    center[1] += MOVE;
                    SCE_Sphere_SetCenterv (&sphere, center);
                    break;
                case SDLK_DOWN:
                    SCE_Sphere_GetCenterv (&sphere, center);
                    center[1] -= MOVE;
                    SCE_Sphere_SetCenterv (&sphere, center);
                    break;
                case SDLK_i:
                    SCE_Sphere_GetCenterv (&sphere, center);
                    center[2] += MOVE;
                    SCE_Sphere_SetCenterv (&sphere, center);
                    break;
                case SDLK_k:
                    SCE_Sphere_GetCenterv (&sphere, center);
                    center[2] -= MOVE;
                    SCE_Sphere_SetCenterv (&sphere, center);
                    break;


                case SDLK_SPACE:
                    apply_mode = !apply_mode;
                    break;

                default:;
                }
                break;

            case SDL_KEYUP:
                switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE: loop = 0; break;
                case SDLK_t:
                    printf ("fps : %.2f\n", 1000./temps);
                    printf ("update time : %d and %d ms\n", i, j);
                    printf ("total time : %d ms\n", temps);
                    break;
                case SDLK_f:
                    fillmode = !fillmode;
                    break;
                case SDLK_l:
                    vt->trans_enabled = !vt->trans_enabled;
                    printf ("smooth lod transition: %s\n",
                            vt->trans_enabled ? "ENABLED" : "DISABLED");
                    break;
                case SDLK_e:
                    shadows = !shadows;
                    SCE_Light_SetShadows (l, shadows);
                    break;
                default:;
                }
            default:;
            }
        }

        SCE_Inert_Compute (&rx);
        SCE_Inert_Compute (&ry);

        SCE_Matrix4_Translate (matrix, 0., 0., -dist);
        SCE_Matrix4_MulRotX (matrix, -(SCE_Inert_Get (&rx) * 0.2) * RAD);
        SCE_Matrix4_MulRotZ (matrix, -(SCE_Inert_Get (&ry) * 0.2) * RAD);
        SCE_Matrix4_MulTranslate (matrix, -MAP_W/4.0, -MAP_W/4.0, -30.);

        i = SDL_GetTicks ();

        if (apply_mode)
            apply_sphere (vt, vs, &sphere, fillmode);

        {
            long missing[3], k;

            SCE_VTerrain_SetPosition (vt, x, y, MAP_D / 3);

            for (k = 0; k < SCE_VTerrain_GetNumLevels (vt); k++)
            {
                SCE_VTerrain_GetMissingSlices (vt, k, &missing[0],
                                               &missing[1], &missing[2]);

                if (0 /* sum of abs(missing) is too big */) {
                    /* update the whole grid */
                } else {
                    while (missing[0] > 0) {
                        update_grid (vt, k, SCE_BOX_POSX, vs);
                        missing[0]--;
                    }
                    while (missing[0] < 0) {
                        update_grid (vt, k, SCE_BOX_NEGX, vs);
                        missing[0]++;
                    }
                    while (missing[1] > 0) {
                        update_grid (vt, k, SCE_BOX_POSY, vs);
                        missing[1]--;
                    }
                    while (missing[1] < 0) {
                        update_grid (vt, k, SCE_BOX_NEGY, vs);
                        missing[1]++;
                    }
                    while (missing[2] > 0) {
                        update_grid (vt, k, SCE_BOX_POSZ, vs);
                        missing[2]--;
                    }
                    while (missing[2] < 0) {
                        update_grid (vt, k, SCE_BOX_NEGZ, vs);
                        missing[2]++;
                    }
                }
            }
        }

        while ((level = SCE_VStore_GetNextUpdatedZone (vs, &rect)) >= 0) {
            int p1[3], p2[3];
            long origin_x, origin_y, origin_z;
            SCE_SGrid *grid = SCE_VTerrain_GetLevelGrid (vt, level);

            SCE_VTerrain_GetOrigin (vt, level, &origin_x, &origin_y, &origin_z);
            SCE_Rectangle3_GetPointsv (&rect, p1, p2);
            SCE_VStore_GetGridRegion (vs, level, &rect, grid, p1[0] - origin_x,
                                      p1[1] - origin_y, p1[2] - origin_z);

            /* move the updated area in the terrain grid coordinates */
            SCE_Rectangle3_Move (&rect, -origin_x, -origin_y, -origin_z);
            SCE_VTerrain_UpdateSubGrid (vt, level, &rect, first_draw);
        }
        first_draw = SCE_TRUE;
        j = SDL_GetTicks ();
        i = j - i;

        SCE_VTerrain_Update (vt);

        j = SDL_GetTicks () - j;

        SCE_Scene_Update (scene, cam, NULL, 0);
        SCE_Scene_Render (scene, cam, NULL, 0);

#if 1
        SCE_RLoadMatrix (SCE_MAT_OBJECT, sce_matrix4_id);
        SCE_Scene_UseCamera (cam);
        glDisable (GL_DEPTH_TEST);
        glPointSize (3.0);
        glBegin (GL_POINTS);
        glColor3f (1.0, 0.0, 0.0);
        SCE_Sphere_GetCenterv (&sphere, center);
        SCE_Vector3_Operator1 (center, /=, 2.0);
        glVertex3fv (center);
        glEnd ();
        glEnable (GL_DEPTH_TEST);
#endif

        glFlush ();
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
