#version 430

// Inputs from Vertex Shader
in vec3 v2fNormal;
in vec3 v2fColor;
in vec2 v2fTexCoord;
in vec3 v2fFragPos; // from vertex shader

// Output Color
layout(location = 0) out vec3 oColor;

// Uniforms
layout(location = 2) uniform vec3 uLightDir; // should be normalized! ||uLightDir|| = 1
layout(location = 3) uniform vec3 uLightDiffuse;
layout(location = 4) uniform vec3 uSceneAmbient;

layout(location = 5) uniform vec3 uPointLightPos[3];
layout(location = 8) uniform vec3 uPointLightColor[3];
layout(location = 11) uniform vec3 uViewpos;
layout(location = 12) uniform float uShininess;

layout(binding = 0) uniform sampler2D uTexture;

void main()
{
    // Texture Color
    vec3 texColor = texture(uTexture, v2fTexCoord).rgb;

    // Normalize the interpolated normal
    vec3 normal = normalize(v2fNormal);

    // Compute ambient and directional diffuse components
    float nDotL = max(dot(normal, uLightDir), 0.0);
    vec3 ambient = uSceneAmbient;
    vec3 diffuse = uLightDiffuse * nDotL;
    vec3 baseColor = (ambient + diffuse) * texColor;

    //https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model
    // Calculate the view direction
    vec3 viewDir = normalize(uViewpos - v2fFragPos);
    vec3 pointLightColor = vec3(0.0);

    // Iterate over each light
    for (int i = 0; i < 3; i++)
    {
        // Calculate light direction and distance
        vec3 lightDir = uPointLightPos[i] - v2fFragPos;
        float distance = dot(lightDir, lightDir);
        lightDir = normalize(lightDir);
        
        //atteanuation
        float attenuation = 1.0/distance;

        // Diffuse component for the point light
        float lambertian = max(dot(normal, lightDir), 0.0);

        // Specular component using Blinn-Phong
        float specular = 0.0;

        vec3 halfwayDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(normal, halfwayDir), 0.0);
        specular = pow(specAngle, uShininess);
        
        // Combine diffuse and specular contributions
        vec3 lightColor = uPointLightColor[i];
        vec3 diffuseColor = lambertian * uPointLightColor[i] * texColor;
        vec3 specularColor = specular * uPointLightColor[i];

        // Apply attenuation
        vec3 Colorattenuation = attenuation * (diffuseColor + specularColor);

        pointLightColor += Colorattenuation;
    }

    // Combine base color with accumulated point light contributions
    vec3 finalColor = baseColor + pointLightColor;

    // Output the final color without gamma correction
    oColor = finalColor;
}