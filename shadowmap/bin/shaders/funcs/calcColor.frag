in vec3 fragNormal;
in vec3 fragPosition;
in vec2 fragTexCoords;
in vec4 lightVSPosition;
in vec4 fragPosLightSpace;

uniform sampler2D depthTexture; //?
uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform vec3 emissionColor;
uniform float shininess;
uniform float opacity;

uniform float ambientStrength;
uniform vec3 lightColor;

out vec4 fragColor;

#include "calcPhong.frag"

float calcShadow(vec4 fragPosLightSpace) {
	vec3 projCoords = vec3(fragPosLightSpace) / fragPosLightSpace.w; ///NDC [-1,1]
	projCoords = projCoords * 0.5 + 0.5; /// [-1,1]->[0,1] Texture Space
	
	/// z>1 o z<0 ->
	if(projCoords.z > 1.0 || projCoords.z < 0.0) return -2.0;

	vec2 size = textureSize(depthTexture,0);
	float delta = 1/size.x;
	float intensity = 0;
	vec3 lightDir = normalize(vec3(lightVSPosition) - fragPosition);
	float bias = max(0.05 * (1.0 - dot(fragNormal, lightDir)), 0.005);  ;
	vec3 pos = vec3(0,0,0);
	vec3 x = vec3(delta,0,0);
	vec3 y = vec3(0,delta,0);
	
	for(int i = -1; i<=1; i++){
		for(int j = -1; j<=1; j++){
				pos = (x*i) + (y*j);
				vec3 newProjCoords = projCoords + pos;
				
				float current = newProjCoords.z; ///profundidad en el cam space
				float closest = texture(depthTexture, newProjCoords.xy).r + bias; ///profundidad en el light space
				if(current > closest) intensity++;			
		}
	}
	/// si es mayor entonces hay sombra.
	
	return intensity/9;
	
}

vec4 calcColor(vec3 textureColor) {
	vec3 phong = calcPhong(lightVSPosition, lightColor, ambientStrength,
						   ambientColor*textureColor, diffuseColor*textureColor,
						   specularColor, shininess);
	
	float shadow = calcShadow(fragPosLightSpace);
	if(shadow < 0) {
		if (shadow==-1) return vec4(1.f,0.f,0.f,1.f); ///rojo
		else            return vec4(0.f,1.f,0.f,1.f); ///verde
	}
	
	vec3 ambientComponent = ambientColor*textureColor*ambientStrength;
	vec3 lighting = mix(phong, ambientComponent, shadow);
	return vec4(lighting+emissionColor,opacity);
}
