#version 430

in vec3 f_vertexInView;
in vec3 f_normalInView;

out vec4 fragColor;

struct LightInfo{
	vec4 position;
	vec4 spotDirection;
	vec4 La;			// Ambient light intensity
	vec4 Ld;			// Diffuse light intensity
	vec4 Ls;			// Specular light intensity
	float spotExponent;
	float spotCosCutoff;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
};

struct MaterialInfo
{
	vec4 Ka;
	vec4 Kd;
	vec4 Ks;
	float shininess;
};

uniform int lightIdx;			// Use this variable to contrl lighting mode
uniform mat4 um4v;				// Camera viewing transformation matrix
uniform LightInfo light[3];
uniform MaterialInfo material;

vec4 directionalLight(vec3 N, vec3 V){

	vec4 lightInView = um4v * light[0].position;	// the position of the light in camera space
	vec3 S = normalize(lightInView.xyz);			// Normalized lightInView
	vec3 H = normalize(S + V);						// Half vector

	// [TODO] calculate diffuse coefficient and specular coefficient here
	float dc = clamp( dot(S,N), 0.0f,1.0f) ;
	float sc = clamp( pow(max(dot(N, H), 0), 64.0f), 0.0f,1.0f );

	return light[0].La * material.Ka + dc * light[0].Ld * material.Kd + sc * light[0].Ls * material.Ks;
}

vec4 pointLight(vec3 N, vec3 V){

	// [TODO] Calculate point light intensity here
	
	vec4 lightInView = um4v * light[1].position;	// the position of the light in camera space
	float distance = length(lightInView.xyz);
	float fatt = 1.0f/(light[1].constantAttenuation + light[1].linearAttenuation * distance + light[1].quadraticAttenuation * distance * distance);

	vec3 S = normalize(lightInView.xyz);			// Normalized lightInView
	vec3 H = normalize(S + V);						// Half vector

	float dc = clamp( dot(S,N)*fatt, 0.0f,1.0f) ;
	float sc = clamp( pow(max(dot(N, H), 0), 64.0f)*fatt, 0.0f,1.0f );

	return light[1].La * material.Ka + dc * light[1].Ld * material.Kd + sc * light[1].Ls * material.Ks;



}

vec4 spotLight(vec3 N, vec3 V){
	
	//[TODO] Calculate spot light intensity here
	vec4 lightInView = um4v * light[2].position;	// the position of the light in camera space
	vec4 spot_dir=   um4v* light[2].spotDirection;
	float distance = length(lightInView.xyz);


	float fatt = 1.0f/(light[2].constantAttenuation + light[2].linearAttenuation * distance + light[2].quadraticAttenuation * distance * distance);

	vec3 S = normalize(lightInView.xyz);			// Normalized lightInView
	vec3 H = normalize(S + V);						// Half vector
	vec3 L =normalize(lightInView.xyz-f_vertexInView);



	float cos_theta = dot( -L , normalize(spot_dir.xyz) );
	float effect = pow(max(dot(-L, normalize(spot_dir.xyz)),0.0f),light[2].spotExponent);


	float dc = 0;
	float sc =0;

	if (cos_theta>light[2].spotCosCutoff ){
			 dc = clamp( dot(L,N)*fatt*effect, 0.0f,1.0f) ;
			 sc = clamp( pow(max(dot(N, H), 0), 64.0f)*fatt*effect, 0.0f,1.0f );
	
	}
	

	return light[2].La * material.Ka + dc * light[2].Ld * material.Kd + sc * light[2].Ls * material.Ks;

	

}

void main() {

	vec3 N = normalize(f_normalInView);		// N represents normalized normal of the model in camera space
	vec3 V = normalize(-f_vertexInView);	// V represents the vector from the vertex of the model to the camera position
	
	vec4 color = vec4(0, 0, 0, 0);

	// Handle lighting mode
	if(lightIdx == 0)
	{
		color += directionalLight(N, V);
	}
	else if(lightIdx == 1)
	{
		color += pointLight(N, V);
	}
	else if(lightIdx == 2)
	{
		color += spotLight(N ,V);
	}

	fragColor = color;
}
