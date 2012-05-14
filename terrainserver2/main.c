
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

#define GRID_SIZE 128

#define GW (GRID_SIZE)
#define GH (GRID_SIZE)
#define GD (GRID_SIZE)

#define MAP_W (GW * 1)
#define MAP_H (GH * 1)
#define MAP_D (GW * 1)

#define OCTREE_SIZE 32

#define OCTREE_FNAME "octree.bin"

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
//    double derp = (-z + 40) * 0.07;
    double derp = (-z + 100) * 0.07;
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

static void generate_buffer (const SCE_SLongRect3 *r, SCEulong w, SCEulong h,
                             SCEubyte *buffer)
{
    long p1[3], p2[3];
    long x, y, z, i, j, k;

    SCE_Rectangle3_GetPointslv (r, p1, p2);

    for (z = p1[2], k = 0; z < p2[2]; z++, k++) {
        for (y = p1[1], j = 0; y < p2[1]; y++, j++) {
            for (x = p1[0], i = 0; x < p2[0]; x++, i++)
                buffer[w * (h * k + j) + i] = density_function (x, y, z);
        }
    }
}

static void generate_terrain (SCE_SVoxelWorld *vw)
{
    long x, y, z;
    SCEulong w, h, d, num;
    SCEuint levels;
    SCE_SLongRect3 r;
    SCEubyte *buffer = NULL;

    w = SCE_VWorld_GetWidth (vw);
    h = SCE_VWorld_GetHeight (vw);
    d = SCE_VWorld_GetDepth (vw);
    levels = SCE_VWorld_GetNumLevels (vw);
    num = 1 << levels;

    buffer = SCE_malloc (w * h * d);

    for (z = 0; z < num; z++) {
        for (y = 0; y < num; y++) {
            for (x = 0; x < num; x++) {
                SCE_Rectangle3_SetFromOriginl (&r, x * w, y * h, z * d, w, h, d);
                generate_buffer (&r, w, h, buffer);
                SCE_VWorld_SetRegion (vw, &r, buffer);
                SCE_VWorld_GetNextUpdatedRegion (vw, &r);
            }
        }
    }
}

#define LOLOD 1

static int apply_sphere (SCE_SVoxelTerrain *vt, SCE_SVoxelWorld *vw,
                         const SCE_SSphere *s, int fill)
{
    SCE_TVector3 pos, center;
    float r, dist;
    SCEulong w, h, d, x, y, z;
    int startx, starty, startz;
    SCE_SLongRect3 rect;
    /* hope that radius doesnt exceed 14 */
    SCEubyte buf[32*32*32*4] = {0};
    SCEuint t;

    SCE_Rectangle3_Initl (&rect);

    r = SCE_Sphere_GetRadius (s);
    SCE_Sphere_GetCenterv (s, center);

#define HERP 1.0
    x = (int)(center[0] - r - HERP);
    y = (int)(center[1] - r - HERP);
    z = (int)(center[2] - r - HERP);
    w = center[0] + r + HERP + 0.5;
    h = center[1] + r + HERP + 0.5;
    d = center[2] + r + HERP + 0.5;

    startx = MAX (x, 0);
    starty = MAX (y, 0);
    startz = MAX (z, 0);

    SCE_Rectangle3_Setl (&rect, startx, starty, startz, w, h, d);

    if (SCE_VWorld_GetRegion (vw, 0, &rect, buf) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }

    w = SCE_Rectangle3_GetWidthl (&rect);
    h = SCE_Rectangle3_GetHeightl (&rect);
    d = SCE_Rectangle3_GetDepthl (&rect);

    for (z = 0; z < d; z++) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                unsigned char p;
                size_t offset = w * (h * z + y) + x;

                offset *= SCE_VOCTREE_VOXEL_ELEMENTS;

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

    if (SCE_VWorld_SetRegion (vw, &rect, buf) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }

    if (SCE_VWorld_GenerateAllLOD (vw, 0, &rect) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    return SCE_OK;
}


static void SCE_PrintMatrix (SCE_TMatrix4 m)
{
    SCEE_SendMsg ("%.3f, %.3f, %.3f, %.3f,\n"
                  "%.3f, %.3f, %.3f, %.3f,\n"
                  "%.3f, %.3f, %.3f, %.3f,\n"
                  "%.3f, %.3f, %.3f, %.3f\n\n",
                  m[0], m[1], m[2], m[3],
                  m[4], m[5], m[6], m[7],
                  m[8], m[9], m[10], m[11],
                  m[12], m[13], m[14], m[15]);
}

static void update_grid (SCE_SVoxelTerrain *vt, SCEuint level,
                         SCE_EBoxFace f, SCE_SVoxelWorld *vw)
{
    long x, y, z;
    long w, h, d;
    long origin_x, origin_y, origin_z;

    /* TODO: let's hope GW >= GH */
    unsigned char buf[GW * GW] = {0};

    SCE_SLongRect3 r;

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
        SCE_Rectangle3_Setl (&r, x, origin_y, z, x+1, origin_y + h, z+d);
        SCE_VWorld_GetRegion (vw, level, &r, buf);
        break;
    case SCE_BOX_POSY:
        y += h + 1;
    case SCE_BOX_NEGY:
        y--;
        SCE_Rectangle3_Setl (&r, origin_x, y, z, origin_x+w, y+1, z+d);
        SCE_VWorld_GetRegion (vw, level, &r, buf);
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
    SCE_SVoxelWorld *vw = NULL;
    SCE_SLongRect3 rect;
    int first_draw = SCE_FALSE;

    SCEubyte *buf = NULL;

    SCE_SDeferred *def = NULL;
    SCE_SShader *lodshader = NULL;
    int shadows = SCE_FALSE;
    time_t mdr;

#if 0
    mdr = time (NULL);
    srand (mdr);
    printf ("time: %d\n", mdr);
#else
    /* 58, 62: interesting cave */
    /* 68: relatively flat */
    /* 1334426723: big hole & stuff */
    /* 1334437699: nice shape blahblah */
    /* 1334365870: ?? */
    srand (62);
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
    
    SCE_Sphere_Init (&sphere);
    SCE_Sphere_SetCenter (&sphere, GW / 2.0, GH / 2.0, GD / 2.0);
    SCE_Sphere_SetRadius (&sphere, 4.0);

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

#if 1
    SCE_VTerrain_CompressPosition (vt, SCE_TRUE);
    SCE_VTerrain_CompressNormal (vt, SCE_TRUE);
#else
    SCE_VTerrain_CompressPosition (vt, SCE_FALSE);
    SCE_VTerrain_CompressNormal (vt, SCE_FALSE);
#endif
    SCE_VTerrain_SetAlgorithm (vt, SCE_VRENDER_MARCHING_CUBES);

#if LOLOD
    SCE_VTerrain_SetNumLevels (vt, 3);
#else
    SCE_VTerrain_SetNumLevels (vt, 1);
#endif
#if 0
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

    SCE_Scene_SetVoxelTerrain (scene, vt);

    /* create voxel data */
    vw = SCE_VWorld_Create ();
    SCE_VWorld_SetDimensions (vw, OCTREE_SIZE, OCTREE_SIZE, OCTREE_SIZE);
    SCE_VWorld_SetNumLevels (vw, 3);
    SCE_VWorld_SetPrefix (vw, "voxeldata");
    SCE_VWorld_Build (vw);

    SCE_VWorld_AddNewTree (vw, 0, 0, 0);

    buf = SCE_malloc (GW * GH * GD * 4);

#if 0
    if (SCE_VOctree_Load (vo, OCTREE_FNAME) < 0) {
        SCEE_LogSrc ();
        SCEE_Out ();
        SCEE_Clear ();
        SCE_VOctree_SetDimensions (vo, OCTREE_SIZE, OCTREE_SIZE, OCTREE_SIZE);
#if LOLOD
        SCE_VOctree_SetMaxDepth (vo, 1);
#else
        SCE_VOctree_SetMaxDepth (vo, 1);
#endif
    }

    /* upload data */
    {
        SCE_SIntRect3 terrain_ri;
        long origin_x, origin_y, origin_z;
        SCE_SGrid *grid = SCE_VTerrain_GetLevelGrid (vt, 0);
        int p1[3], p2[3];

        /* move the updated area in the terrain grid coordinates */
        SCE_VTerrain_GetOrigin (vt, 0, &origin_x, &origin_y, &origin_z);
        SCE_Rectangle3_SetFromOrigin (&terrain_ri, origin_x, origin_y, origin_z,
                                      GW, GH, GD);

        SCE_Rectangle3_LongFromInt (&rect, &terrain_ri);

        SCE_VOctree_GetRegion (vo, 0, &rect, buf);
        SCE_Grid_SetRegion (grid, &terrain_ri, SCE_VOCTREE_VOXEL_ELEMENTS, buf);

        SCE_free (buf);
    }
#else
#if 1
    printf ("generating terrain...\n");
    i = SDL_GetTicks ();
    generate_terrain (vw);
    printf ("terrain generation: %.3fs\n",
            ((float)(SDL_GetTicks () - i)) / 1000.0);

    i = SDL_GetTicks ();
    {
        SCE_SLongRect3 rect;
        SCEuint levels = SCE_VWorld_GetNumLevels (vw);
        long s = OCTREE_SIZE * (1 << (levels - 1));
        SCE_Rectangle3_Setl (&rect, 0, 0, 0, s, s, s);
        SCE_VWorld_GenerateAllLOD (vw, 0, &rect);
    }
    printf ("LOD generation: %.3fs\n",
            ((float)(SDL_GetTicks () - i)) / 1000.0);
#endif
#endif

    SCE_VTerrain_UpdateGrid (vt, 0, SCE_FALSE);
#if LOLOD
    SCE_VTerrain_UpdateGrid (vt, 1, SCE_FALSE);
    SCE_VTerrain_UpdateGrid (vt, 2, SCE_FALSE);
//    SCE_VTerrain_UpdateGrid (vt, 3, SCE_FALSE);
//    SCE_VTerrain_UpdateGrid (vt, 4, SCE_FALSE);
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
#define SPEED 2
                switch (ev.key.keysym.sym) {
                case SDLK_z: y += SPEED; break;
                case SDLK_s: y -= SPEED; break;
                case SDLK_d: x += SPEED; break;
                case SDLK_q: x -= SPEED; break;

#define MOVE 0.8
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
                case SDLK_m:
                    SCE_PrintMatrix (matrix);
                    printf ("%d %d\n____\n", x, y);
                    break;
                case SDLK_v:
                {
                    unsigned int v = SCE_VRender_GetMaxV ();
                    unsigned int i = SCE_VRender_GetMaxI ();
                    unsigned int lv = SCE_VRender_GetLimitV ();
                    unsigned int li = SCE_VRender_GetLimitI ();
                    printf ("%d of %d (%.1f\%) ; %d of %d (%.1f\%)\n",
                            v, lv, (float)100.0 * v / lv,
                            i, li, (float)100.0 * i / li);
                }
                break;
                default:;
                }
            default:;
            }
        }

        SCE_Inert_Compute (&rx);
        SCE_Inert_Compute (&ry);

#if 1
        SCE_Matrix4_Translate (matrix, 0., 0., -dist);
        SCE_Matrix4_MulRotX (matrix, -(SCE_Inert_Get (&rx) * 0.2) * RAD);
        SCE_Matrix4_MulRotZ (matrix, -(SCE_Inert_Get (&ry) * 0.2) * RAD);
        SCE_Matrix4_MulTranslate (matrix, -MAP_W/4.0, -MAP_W/4.0, -30.);
#else
        {
            SCE_TMatrix4 lol = {
                0.915, -0.404, 0.000, -32.731,
                0.046, 0.105, 0.993, -39.500,
                -0.401, -0.909, 0.115, 6.377,
                0.000, 0.000, 0.000, 1.000
            };
            SCE_Matrix4_Copy (matrix, lol);
        }
#endif

        i = SDL_GetTicks ();

        if (apply_mode) {
            if (apply_sphere (vt, vw, &sphere, fillmode) < 0) {
                SCEE_LogSrc ();
                SCEE_Out ();
                SCEE_Clear ();
                return 42;
            }
        }

//        SCE_VTerrain_SetPosition (vt, MAP_W / 2, MAP_H / 2, MAP_D / 2);

        if (first_draw)
        {
            long missing[3], k;

            SCE_VTerrain_SetPosition (vt, x, y, OCTREE_SIZE * 2);

            for (k = 0; k < SCE_VTerrain_GetNumLevels (vt); k++)
            {
                SCE_VTerrain_GetMissingSlices (vt, k, &missing[0],
                                               &missing[1], &missing[2]);

                if (0 /* sum of abs(missing) is too big */) {
                    /* update the whole grid */
                } else {
                    while (missing[0] > 0) {
                        update_grid (vt, k, SCE_BOX_POSX, vw);
                        missing[0]--;
                    }
                    while (missing[0] < 0) {
                        update_grid (vt, k, SCE_BOX_NEGX, vw);
                        missing[0]++;
                    }
                    while (missing[1] > 0) {
                        update_grid (vt, k, SCE_BOX_POSY, vw);
                        missing[1]--;
                    }
                    while (missing[1] < 0) {
                        update_grid (vt, k, SCE_BOX_NEGY, vw);
                        missing[1]++;
                    }
                    while (missing[2] > 0) {
                        update_grid (vt, k, SCE_BOX_POSZ, vw);
                        missing[2]--;
                    }
                    while (missing[2] < 0) {
                        update_grid (vt, k, SCE_BOX_NEGZ, vw);
                        missing[2]++;
                    }
                }
            }
        }


        while ((level = SCE_VWorld_GetNextUpdatedRegion (vw, &rect)) >= 0) {
            SCE_SIntRect3 terrain_ri;
            long origin_x, origin_y, origin_z;
            SCE_SGrid *grid = SCE_VTerrain_GetLevelGrid (vt, level);
            int p1[3], p2[3];

            SCE_Rectangle3_IntFromLong (&terrain_ri, &rect);

            /* move the updated area in the terrain grid coordinates */
            SCE_VTerrain_GetOrigin (vt, level, &origin_x, &origin_y, &origin_z);
            SCE_Rectangle3_Move (&terrain_ri, -origin_x, -origin_y, -origin_z);

            memset (buf, 0, SCE_Rectangle3_GetAreal (&rect));
            if (SCE_VWorld_GetRegion (vw, level, &rect, buf) < 0)
            {
                SCEE_LogSrc ();
                SCEE_Out ();
                return 434;
            }

            SCE_Grid_SetRegion (grid, &terrain_ri, SCE_VOCTREE_VOXEL_ELEMENTS, buf);
            SCE_VTerrain_UpdateSubGrid (vt, level, &terrain_ri, first_draw);
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

#if 0
    if (SCE_VOctree_Save (vo, OCTREE_FNAME) < 0) {
        SCEE_LogSrc ();
        SCEE_Out ();
    }
#endif
    
    SCE_Scene_Delete (scene);
    SCE_Camera_Delete (cam);

    SCE_Model_Delete (model);
    
    SCE_Quit_Interface ();

    SDL_Quit ();

    return 0;
}
