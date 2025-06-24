#version 430

in vec2 vTexcoord;

uniform sampler2D uParticleTexture;

out vec4 fragColor;

void main()
{
    vec4 texColor = texture(uParticleTexture, vTexcoord);
    fragColor = texColor;
}
