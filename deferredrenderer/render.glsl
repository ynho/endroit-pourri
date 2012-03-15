[vertex shader]

varying vec3 normal;
varying vec4 pos;

uniform mat4 sce_modelviewmatrix;
uniform mat4 sce_projectionmatrix;

void main ()
{
    normal = mat3(sce_modelviewmatrix) * gl_Normal;
    normal = normalize (normal);
    gl_FrontColor = vec4 (1.0);
    gl_Position = pos = sce_projectionmatrix * (sce_modelviewmatrix * gl_Vertex);
}

[pixel shader]

varying vec3 normal;
varying vec4 pos;

void main ()
{
    sce_pack_position (pos);
    sce_pack_normal (normalize (normal));
    sce_pack_color (gl_Color.xyz);
}
