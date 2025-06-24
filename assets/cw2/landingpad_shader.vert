#version 430

// Input attributes
layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iColor;
layout(location = 2) in vec3 iNormal;

// Uniforms
layout(location = 0) uniform mat4 uProjCameraWorld;
layout(location = 1) uniform mat3 uNormalMatrix;
layout(location = 13) uniform mat4 uModel;

// Output attributes
out vec3 v2fColor;
out vec3 v2fNormal;
out vec3 v2fFragPos;

void main()
{
    v2fFragPos = vec3(uModel * vec4(iPosition, 1.0));
    // Copy input color to the output color attribute.
    v2fColor = iColor;
    v2fNormal = normalize(uNormalMatrix * iNormal);

    // Transform the input position with the uniform matrix
    gl_Position = uProjCameraWorld * vec4(iPosition, 1.0);
}