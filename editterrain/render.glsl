[vertex shader]

#define SCE_VRENDER_HIGHP_VERTEX_POS 1
#define SCE_VRENDER_HIGHP_VERTEX_NOR 1

#if SCE_VRENDER_HIGHP_VERTEX_POS
in vec3 sce_position;
#else
in uvec4 sce_position;
#endif

#if SCE_VRENDER_HIGHP_VERTEX_NOR
in vec3 sce_normal;
#else
in uvec4 sce_normal;
#endif

out vec3 csnor;
out vec3 wspos;
out vec3 nor;
out vec4 pos;

uniform mat4 sce_objectmatrix;
uniform mat4 sce_cameramatrix;
uniform mat4 sce_projectionmatrix;

uniform bool enabled;

#if !SCE_VRENDER_HIGHP_VERTEX_POS
vec3 decode_pos (uvec4 pos)
{
    vec3 v;
    const float factor = 256.0 * 5.0;
    v = vec3 (pos.wzy) / factor;
    return v;
}
#endif
#if !SCE_VRENDER_HIGHP_VERTEX_NOR
vec3 decode_nor (uvec4 nor)
{
    vec3 v;
    v = vec3 (nor.wzy) / 127.0 - vec3 (1.0);
    return v;
}
#endif


void main ()
{
#if SCE_VRENDER_HIGHP_VERTEX_POS
    vec3 position = sce_position;
#else
    vec3 position = decode_pos (sce_position);
#endif
#if SCE_VRENDER_HIGHP_VERTEX_NOR
    vec3 normal = normalize (sce_normal);
#else
    vec3 normal = normalize (decode_nor (sce_normal));
#endif

#if SCE_VTERRAIN_USE_LOD
    if (enabled)
        sce_vterrain_seamlesslod (position, normal);
#endif

    nor = normal;
    csnor = normalize (mat3(sce_cameramatrix * sce_objectmatrix) * normal);
#if SCE_VTERRAIN_USE_SHADOWS
    position = sce_vterrain_shadowoffset (position, normal);
#endif

    vec4 p = sce_objectmatrix * vec4 (position, 1.0);
    wspos = p.xyz;
    p = sce_cameramatrix * p;
#if SCE_VTERRAIN_USE_POINT_SHADOWS
    sce_camera_space_pos = p;
#endif
    p = sce_projectionmatrix * p;
    pos = p;
    gl_Position = p;
}

[pixel shader]

#if SCE_VTERRAIN_USE_SHADOWS
#if SCE_VTERRAIN_USE_POINT_SHADOWS

void main (void)
{
    gl_FragDepth = sce_deferred_getdepth ();
}

#endif
#else

in vec3 csnor;
in vec3 wspos;
in vec3 nor;
in vec4 pos;

/* names are hardcoded, that's nice. */
uniform sampler2D sce_side_diffuse;
uniform sampler2D sce_top_diffuse;

void main ()
{
    vec2 tc1, tc2, tc3;
    vec3 weights;
    vec3 normal = nor;

    weights = sce_triplanar (wspos, normal, tc1, tc2, tc3);
    weights = pow (weights, vec3 (8.0));
    weights /= dot (weights, vec3 (1.0));
    /* TODO: if normal has been modified, project it in view space and
       use it instead of nor in sce_pack_normal() */

    const float scale = 0.08;
    vec4 col1 = texture2D (sce_side_diffuse, tc1 * scale);
    vec4 col2 = texture2D (sce_side_diffuse, tc2 * scale);
    vec4 col3 = texture2D (sce_top_diffuse, tc3 * scale);

    vec4 color = weights.x * col1 + weights.y * col2 + weights.z * col3;

    sce_pack_position (pos);
    sce_pack_normal (normalize (csnor));
    sce_pack_color (color.xyz);
}

#endif
