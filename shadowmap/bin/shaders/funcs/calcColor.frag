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
	
	float currentDepth = projCoords.z; ///profundidad en el cam space
	float closestDepth = texture(depthTexture, projCoords.xy).r; ///profundidad en el light space
	
	closestDepth += 0.01; //bias

	vec2 size = textureSize(depthTexture,0);
	float delta = 2/size.x;
	
	vec3 up = vec3(0,delta,0);
	vec3 down = vec3(0,-delta,0);
	vec3 right = vec3(delta,0,0);
	vec3 left = vec3(-delta,0,0);
	
	vec3 upr = vec3(delta,delta,0);
	vec3 upl = vec3(-delta,delta,0);
	vec3 downr = vec3(delta,-delta,0);
	vec3 downl = vec3(-delta,-delta,0);
	
	vec3 upProjCoords = projCoords + up;
	vec3 downProjCoords = projCoords + down;
	vec3 leftProjCoords = projCoords + left;
	vec3 rightProjCoords = projCoords + right;
	
	vec3 uprProjCoords = projCoords + upr;
	vec3 uplProjCoords = projCoords + upl;
	vec3 downrProjCoords = projCoords + downr;
	vec3 downlProjCoords = projCoords + downl;;
	
	float intensity = 0.1;
	float bias = 0.005;
	
	float current = upProjCoords.z; ///profundidad en el cam space
	float closest = texture(depthTexture, upProjCoords.xy).r + bias; ///profundidad en el light space
	if(currentDepth > closest) intensity++;
	
	current = uprProjCoords.z; ///profundidad en el cam space
	closest = texture(depthTexture, uprProjCoords.xy).r + bias; ///profundidad en el light space
	if(currentDepth > closest) intensity++;

	current = uplProjCoords.z; ///profundidad en el cam space
	closest = texture(depthTexture, uplProjCoords.xy).r + bias; ///profundidad en el light space
	if(currentDepth > closest) intensity++;
	
	current = leftProjCoords.z; ///profundidad en el cam space
	closest = texture(depthTexture, leftProjCoords.xy).r + bias; ///profundidad en el light space
	if(currentDepth > closest) intensity++;
	
	current = rightProjCoords.z; ///profundidad en el cam space
	closest = texture(depthTexture, rightProjCoords.xy).r + bias; ///profundidad en el light space
	if(currentDepth > closest) intensity++;
	
	current = downrProjCoords.z; ///profundidad en el cam space
	closest = texture(depthTexture, downrProjCoords.xy).r + bias; ///profundidad en el light space
	if(currentDepth > closest) intensity++;
	
	current = downProjCoords.z; ///profundidad en el cam space
	closest = texture(depthTexture, downProjCoords.xy).r + bias; ///profundidad en el light space
	if(currentDepth > closest) intensity++;
	
	current = downlProjCoords.z; ///profundidad en el cam space
	closest = texture(depthTexture, downlProjCoords.xy).r + bias; ///profundidad en el light space
	if(currentDepth > closest) intensity++;
	
//
//	if(!(upProjCoords.z > 1.0 || upProjCoords.z < 0.0)){
//		float current = upProjCoords.z; ///profundidad en el cam space
//		float closest = texture(depthTexture, upProjCoords.xy).r + bias; ///profundidad en el light space
//		
//		if(current > closest) intensity++;
//	}
//	
//	if(!(uplProjCoords.z > 1.0 || uplProjCoords.z < 0.0)){
//		float current = uplProjCoords.z; ///profundidad en el cam space
//		float closest = texture(depthTexture, uplProjCoords.xy).r + bias; ///profundidad en el light space
//		
//		if(current > closest) intensity++;
//	}
//	
//	if(!(uprProjCoords.z > 1.0 || uprProjCoords.z < 0.0)){
//		float current = uprProjCoords.z; ///profundidad en el cam space
//		float closest = texture(depthTexture, uprProjCoords.xy).r + bias; ///profundidad en el light space
//		
//		if(current > closest) intensity++;
//	}
//	
//	if(!(downProjCoords.z > 1.0 || downProjCoords.z < 0.0)){
//		float current = downProjCoords.z; ///profundidad en el cam space
//		float closest = texture(depthTexture, downProjCoords.xy).r + bias; ///profundidad en el light space
//		
//		if(current > closest) intensity++;
//	}
//	
//	if(!(downrProjCoords.z > 1.0 || downrProjCoords.z < 0.0)){
//		float current = downrProjCoords.z; ///profundidad en el cam space
//		float closest = texture(depthTexture, downrProjCoords.xy).r + bias; ///profundidad en el light space
//		
//		if(current > closest) intensity++;
//	}
//	
//	if(!(downlProjCoords.z > 1.0 || downlProjCoords.z < 0.0)){
//		float current = downlProjCoords.z; ///profundidad en el cam space
//		float closest = texture(depthTexture, downlProjCoords.xy).r + bias; ///profundidad en el light space
//		
//		if(current > closest) intensity++;
//	}
//	
//	if(!(leftProjCoords.z > 1.0 || leftProjCoords.z < 0.0)){
//		float current = leftProjCoords.z; ///profundidad en el cam space
//		float closest = texture(depthTexture, leftProjCoords.xy).r + bias; ///profundidad en el light space
//		
//		if(current > closest) intensity++;
//	}
//	
//	if(!(rightProjCoords.z > 1.0 || rightProjCoords.z < 0.0)){
//		float current = rightProjCoords.z; ///profundidad en el cam space
//		float closest = texture(depthTexture, upProjCoords.xy).r + bias; ///profundidad en el light space
//		
//		if(current > closest) intensity++;
//	}

	
	/// si es mayor entonces hay sombra.
	
	
	return currentDepth > closestDepth ? intensity/9 : 0.0;
	
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
