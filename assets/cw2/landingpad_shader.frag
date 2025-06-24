#version 430

// Inputs from Vertex Shader
in vec3 v2fNormal;
in vec3 v2fColor;
in vec3 v2fFragPos;

layout(location = 0) out vec3 oColor;

// Uniforms for Directional Light
layout(location = 2) uniform vec3 uLightDir; 
layout(location = 3) uniform vec3 uLightDiffuse;
layout(location = 4) uniform vec3 uSceneAmbient;

// Uniforms for Point Lights
// light position occupies 5,6,7 (because it is for free 3 cameras)
// light color occupies 8,9,10
layout(location = 5) uniform vec3 uPointLightPos[3];
layout(location = 8) uniform vec3 uPointLightColor[3];

// Uniforms for View Position and Material
layout(location = 11) uniform vec3 uViewPos;
layout(location = 12) uniform float uShininess;

void main()
{
    // Normalize the interpolated normal
    vec3 normal = normalize(v2fNormal);
    float nDotL = max(dot(normal, uLightDir), 0.0);
    vec3 ambient = uSceneAmbient;
    vec3 diffuse = uLightDiffuse * nDotL;
    vec3 baseColor = (ambient + diffuse);

    vec3 viewDir = normalize(uViewPos - v2fFragPos);

    vec3 pointLightColor = vec3(0.0);

    for(int i = 0; i < 3; i++)
    {
        // Calculate the vector from the fragment to the light
        vec3 lightDir = normalize(uPointLightPos[i] - v2fFragPos);
        //standard squared distance
        float distance = dot(lightDir, lightDir);
        lightDir = normalize(lightDir);

        // Attenuation factors
        float attenuation = 1.0/distance;

        float lambertian = max(dot(normal, lightDir), 0.0);

        // Specular component
        float specular = 0.0;
        // Diffuse component
        float nDotL = max(dot(normal, lightDir), 0.0);

        vec3 halfwayDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(normal, halfwayDir), 0.0);
        specular = pow(specAngle, uShininess);

        vec3 lightColor = uPointLightColor[i];
        vec3 diffuseColor = lambertian * uPointLightColor[i];
        vec3 specularColor = specular * uPointLightColor[i];



        vec3 Colorattenuation = attenuation * (diffuseColor + specularColor);

        pointLightColor += Colorattenuation;
    }
    // Combine base color with accumulated point light contributions
    vec3 finalColor = (baseColor + pointLightColor) *v2fColor;

    // Output the final color without gamma correction
    oColor = finalColor;
}