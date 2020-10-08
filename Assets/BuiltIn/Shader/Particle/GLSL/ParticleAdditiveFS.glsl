// File Generated by Assets/BuildShader.py - source: [ParticleAdditiveFS.d.glsl]
precision mediump float;
uniform sampler2D uTexture;
in vec2 varTexCoord0;
in vec4 varColor;
out vec4 FragColor;
const float gamma = 2.2;
const float invGamma = 1.0 / 2.2;
vec3 sRGB(vec3 color)
{
	return pow(color, vec3(gamma));
}
vec3 linearRGB(vec3 color)
{
	return pow(color, vec3(invGamma));
}
void main(void)
{
	vec4 texColor = texture(uTexture, varTexCoord0.xy);
	vec3 color = mix(vec3(0.0, 0.0, 0.0), texColor.rgb, texColor.a);
	vec3 result = sRGB(color) * sRGB(varColor.rgb);
	FragColor = vec4(result * varColor.a, 1.0);
}
