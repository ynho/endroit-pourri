[vertex shader]

varying vec4 position;

void main ()
{
    position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_Position = ftransform ();
}

[pixel shader]

varying vec4 position;

float getintensity (vec3 normal, vec3 lightvec)
{
    return max (dot (normal, -lightvec), 0.0) * 0.7;
}

float getattenuation (float d, float r)
{
    return max (1.0 - pow ((d / r), 8.), 0.0);
}

#if SCE_DEFERRED_USE_SHADOWS
bool outshadow (vec3 pos, vec3 lightpos)
{
    vec3 dir = pos - lightpos;
    float len = length (dir);
#if 0
    float dist1 = len * sce_depth_factor;
#else
    /* TODO: hardcoded value SCE_DEFERRED_POINT_LIGHT_DEPTH_FACTOR */
    float dist1 = len * (1.0/1000.0);
#endif
    /* NOTE: does the vector need to be normalized? */
    float dist2 = textureCube (sce_shadow_cube_map, dir / len).x;
    return dist1 < dist2 + 0.012;
}

float shadow_intensity (vec3 pos, vec3 lightpos)
{
    bool blbl = outshadow (pos, lightpos);
    return blbl ? 1.0 : 0.0;
}
#endif

void main ()
{
    vec2 texcoord = position.xy / position.w;
    texcoord.xy = (texcoord.xy + vec2 (1.0)) * 0.5;

    vec4 fragcolor = vec4 (0.0);
    vec3 pos = sce_unpack_position (texcoord);
    float lightradius = sce_light_radius;
    vec3 lightpos = sce_light_position;
    float dist = distance (pos, lightpos);
    float att = getattenuation (dist, lightradius);
    float intensity = att;

#if SCE_DEFERRED_USE_SHADOWS
    /* sce_light_viewproj used as invview */
    vec4 objspc_lightpos = sce_light_viewproj * vec4 (lightpos, 1.0);
    vec4 objspc_pos = sce_light_viewproj * vec4 (pos, 1.0);
    att *= shadow_intensity (objspc_pos.xyz, objspc_lightpos.xyz);
    if (att > 0.0)
#endif
    {
        vec3 normal = sce_unpack_normal (texcoord);
        vec3 lightvec = normalize (pos - lightpos);
        intensity *= getintensity (normal, lightvec);

        vec4 lightcolor = vec4 (sce_light_color, 0.0);
#if SCE_DEFERRED_USE_SPECULAR
        vec3 eyepos = vec3 (0.0);
        vec3 eyevec = normalize (pos - eyepos);
        float specular = clamp (dot (reflect (lightvec, normal), -eyevec), 0.0, 1.0);
        specular = pow (specular, 16.0);
        specular *= att;
#endif

        fragcolor = texture2D (sce_deferred_color_map, texcoord) * lightcolor;
#if SCE_DEFERRED_USE_SPECULAR
        fragcolor = fragcolor * (intensity + specular);
#else
        fragcolor *= intensity;
#endif
    }
    
    gl_FragColor = fragcolor;
}
