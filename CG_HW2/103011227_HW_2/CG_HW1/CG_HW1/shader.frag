

struct LightSourceParameters {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 position;
	vec4 halfVector;
	vec3 spotDirection;
	float spotExponent;
	float spotCutoff; // (range: [0.0f,90.0f], 180.0f)
	float spotCosCutoff; // (range: [1.0f,0.0f],-1.0f)
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
};

struct MaterialParameters {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform MaterialParameters Material;
uniform LightSourceParameters LightSource[4];
uniform int ambientOn;
uniform int diffuseOn;
uniform int specularOn;
uniform int directionalOn;
uniform int pointOn;
uniform int spotOn;
uniform int perPixelOn;

varying vec4 vv4color;
varying vec4 vv4position;
varying vec3 N;
varying vec3 V;

vec4 getDirectionalLight(LightSourceParameters lightSource){
	vec4 color = vec4(0.0f,0.0f,0.0f,0.0f);
	vec3 L = normalize(lightSource.position.xyz-vv4position.xyz);

	// angle between L and N must less than 90
	if(diffuseOn == 1 && dot(L,N)>=0){
		vec4 diffuse = lightSource.diffuse * Material.diffuse * dot(L,N);
		diffuse = clamp(diffuse,0.0f,1.0f);
		color += diffuse;
	}

	if(specularOn == 1){
		vec3 H= normalize(L+V);
		if(dot(N,H)>0){
			float spec=pow(dot(N,H),50.0f);
			vec4 specular = Material.specular * lightSource.specular * spec;
			specular = clamp(specular,0.0f,1.0f);
			color += specular;
		}

	}

	return color;
}

vec4 getPointLight(LightSourceParameters lightSource){
	vec4 color = vec4(0.0f,0.0f,0.0f,0.0f);
	float distance = length(lightSource.position.xyz-vv4position.xyz);
	float fatt = 1.0f/(lightSource.constantAttenuation + lightSource.linearAttenuation * distance + lightSource.quadraticAttenuation * distance * distance);
	vec3 L = normalize(lightSource.position.xyz-vv4position.xyz);

	if(diffuseOn == 1 && dot(L,N)>=0 ){
		vec4 diffuse = lightSource.diffuse * Material.diffuse * dot(L,N);
		diffuse *= fatt;
		diffuse = clamp(diffuse,0.0f,1.0f);
		color += diffuse;
	}
	if(specularOn == 1){

		vec3 H= normalize(L+V);
		if(dot(N,H)>0){
			float spec=pow(dot(N,H),50.0f);
			vec4 specular = Material.specular * lightSource.specular * spec;
			specular *= fatt;
			specular = clamp(specular,0.0f,1.0f);
			color += specular;
		}		
	}

	return color;
}

vec4 getSpotLight(LightSourceParameters lightSource){
	vec4 color = vec4(0.0f,0.0f,0.0f,0.0f);
	float distance = length(lightSource.position.xyz-vv4position.xyz);
	vec3 L = normalize(lightSource.position.xyz-vv4position.xyz);
	float theta = dot(L,normalize(-lightSource.spotDirection));
	float effect = pow(max(dot(L,-lightSource.spotDirection),0.0f),lightSource.spotExponent);
	float fatt = 1.0f/(lightSource.constantAttenuation + lightSource.linearAttenuation * distance + lightSource.quadraticAttenuation * distance * distance);

	if(theta >= lightSource.spotCosCutoff){
		if(diffuseOn == 1 && dot(L,N)>=0 ){
			vec4 diffuse = lightSource.diffuse * Material.diffuse * dot(L,N);
			diffuse *= effect;
			diffuse = clamp(diffuse,0.0f,1.0f);
			color += diffuse;
		}
		if(specularOn == 1){
			
			vec3 H= normalize(L+V);
			float spec=pow(max( dot(N,H),0.0f ) ,20.0f );

			vec4 specular = Material.specular * lightSource.specular * spec;
			specular *= fatt;
			specular = clamp(specular,0.0f,1.0f);
			color += specular;

		}
	}
	return color;
}


void main() {
	if(perPixelOn == 1){
		vec4 color = vec4(0.0f,0.0f,0.0f,0.0f);
		if(ambientOn == 1){
			vec4 vv4ambient_D = Material.ambient * LightSource[0].ambient;
			color += vv4ambient_D;
		}
		
		color+=(directionalOn==1)?getDirectionalLight(LightSource[1]):0;
		color+=(pointOn==1)?getPointLight(LightSource[2]):0;
		color+=(spotOn==1)?getSpotLight(LightSource[3]):0;
		gl_FragColor = color;
	}else{
		gl_FragColor = vv4color;
	}
}