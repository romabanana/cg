#version 330 core

// propiedades del material
uniform vec3 objectColor;
uniform float shininess;
uniform float specularStrength;

// propiedades de la luz
uniform float ambientStrength;
uniform vec3 lightColor;
uniform vec4 lightPosition;

// propiedades de la camara
uniform vec3 cameraPosition;

// propiedades del fragmento interpoladas por el vertex shader
in vec3 fragNormal;
in vec3 fragPosition;

// resultado
out vec4 fragColor;


// phong simplificado
void main() {
	
	
	// ambient
	vec3 ambient = lightColor * ambientStrength * objectColor ;
	
	// diffuse
	vec3 norm = normalize(fragNormal);
	vec3 lightDir = normalize(vec3(lightPosition)); // luz directional (en el inf.)
	
	float coseno_dif_real = dot(norm,lightDir);
	
	float coseno_dif = 0.8f;
	if(coseno_dif_real<0.3){
		coseno_dif = 0.4f;
	}
	
	vec3 diffuse = lightColor * objectColor * max(coseno_dif,0.f);
	
	// specular
	vec3 specularColor = specularStrength * vec3(1.f,1.f,1.f);
	vec3 viewDir = normalize(cameraPosition-fragPosition);
	vec3 halfV = normalize(lightDir + viewDir); // blinn
	
	//	CODIGO PARA 2 CAPAS (QUEDA  MEJOR SOLO 1 CAPA)
	float coseno_b = dot(norm,halfV);
	float rango1 = 1 - 1/shininess;
	float rango2 = rango1 *0.9;
	float espec = 1;
	if(coseno_b < rango1) espec = 0.95;
	if(coseno_b < rango2) espec = 0;
	
//	
//	float coseno_b = dot(norm,halfV);
//	float rango1 = 1 - 1/shininess;
//	float espec = 1;
//	if(coseno_b < rango1) espec = 0;
//	
	vec3 specular = lightColor * specularColor * pow(max(espec,0.f),shininess);
	
	// result
	fragColor = vec4(ambient+diffuse+specular,1.f);
}

