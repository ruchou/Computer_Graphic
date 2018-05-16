attribute vec4 av4position;
attribute vec3 av3normal;


varying vec4 vv4color;

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
// 0:ambient light  1:directional light  2:point light  3:spot light

uniform int ambientOn;
uniform int diffuseOn;
uniform int specularOn;
uniform int directionalOn;
uniform int pointOn;
uniform int spotOn;
uniform int perPixelOn;
uniform mat4 mvp;
uniform mat4 ModelTrans_inv_transpos;
uniform mat4 ModelTrans;
uniform vec3 eyePos;

vec3 vv3normal = mat3(ModelTrans_inv_transpos)*av3normal;



varying vec4 vv4position;
varying vec3 N;
varying vec3 V;

//I=IaKa+ sum fpip(kd*(N dot Lp)+ks(N dot Hp)^n)

//N=normalized vector
//L = source pos - model pos


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
	vv4color = vec4(0.0f,0.0f,0.0f,0.0f);
	vv4position = ModelTrans * av4position;
	N = normalize(vv3normal);
	V = normalize(eyePos- vv4position.xyz);
	gl_Position = mvp * av4position;

	if(perPixelOn == 0){
		// vertex lighting
		if(ambientOn == 1){
			vec4 vv4ambient_D = Material.ambient * LightSource[0].ambient;
			vv4color += vv4ambient_D;
		}
		vv4color+=(directionalOn==1)?getDirectionalLight(LightSource[1]):0;
		vv4color+=(pointOn==1)?getPointLight(LightSource[2]):0;
		vv4color+=(spotOn==1)?getSpotLight(LightSource[3]):0;

	}


} 