// File Generated by Assets/BuildShader.py - source: [SkySunFS.d.glsl : _]
precision mediump float;
uniform vec4 uColor;
uniform vec2 uIntensity;
uniform vec4 uLightDirection;
uniform vec4 uCamPosition;
in vec2 varTexCoord0;
in vec4 varColor;
in vec4 varWorldPos;
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
vec3 GetSkyColor(
	vec3 viewDir,
	vec3 sunDirection,
	float intensity,
	vec4 atmosphericColor,
	vec4 sunColor,
	vec4 glowColor1,
	vec4 glowColor2,
	float sunRadius)
{
	float y = 1.0 - (max(viewDir.y, 0.0) * 0.8 + 0.2) * 0.8;
	vec3 skyColor = vec3(pow(y, 2.0), y, 0.6 + y*0.4) * intensity;
	float sunAmount = max(dot(sunDirection, viewDir), 0.0);
	skyColor += atmosphericColor.rgb * sunAmount * sunAmount * atmosphericColor.a;
	skyColor += sunColor.rgb * pow(sunAmount, sunRadius) * sunColor.a;
	skyColor += glowColor1.rgb * pow(sunAmount, 8.0) * glowColor1.a;
	skyColor += glowColor2.rgb * pow(sunAmount, 3.0) * glowColor2.a;
	return skyColor;
}
void main(void)
{
	vec3 viewDir = normalize(varWorldPos.xyz - uCamPosition.xyz);
	vec3 skyColor = GetSkyColor(
		viewDir,
		uLightDirection.xyz,
		1.1,
		vec4(1.0, 0.8, 0.7, 0.1),
		vec4(1.0, 0.6, 0.1, 0.5),
		vec4(1.0, 0.6, 0.1, 0.4),
		vec4(1.0, 0.4, 0.2, 0.2),
		800.0
	);
	vec3 groundColor = vec3(0.4, 0.4, 0.4);
	vec3 result = mix(skyColor, sRGB(groundColor), pow(smoothstep(0.0,-0.025, viewDir.y), 0.2));
	vec4 blend = varColor * uColor;
	FragColor = vec4(result * sRGB(blend.rgb), blend.a);
}
