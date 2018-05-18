#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>

#include <GL/glew.h>
#include <freeglut/glut.h>
#include "textfile.h"
#include "glm.h"

#include "Matrices.h"

#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "freeglut.lib")

#ifndef GLUT_WHEEL_UP
# define GLUT_WHEEL_UP   0x0003
# define GLUT_WHEEL_DOWN 0x0004
#endif

#ifndef GLUT_KEY_ESC
# define GLUT_KEY_ESC 0x001B
#endif



#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;



// Shader attributes
GLint iLocPosition;
GLint iLocNormal;
GLint iLocMVP;

GLint iLocMDiffuse, iLocMAmbient, iLocMSpecular, iLocMShininess;
GLint iLocAmbientOn, iLocDiffuseOn, iLocSpecularOn;
GLint iLocDirectionalOn, iLocPointOn, iLocSpotOn;
GLint iLocModelTransform_inv_trans;
GLint iLocEyePosition;
GLint iLocPointPosition, iLocSpotPosition, iLocSpotExponent, iLocSpotCutoff, iLocSpotCosCutoff;
GLint iLocPerPixelLighting;
GLint iLocModelTrans;

//#define numOfModels 5

float scaleOffset = 0.65f;


struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};

struct model
{
	GLMmodel *obj;
	GLfloat *vertices;
	GLfloat* normals;
	Matrix4 N;

	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form
};

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;   //field of view
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};

Matrix4 view_matrix;
Matrix4 project_matrix;

project_setting proj;
camera main_camera;

int ambientOn = 1, diffuseOn = 1, specularOn = 1;
int directionalOn = 0, pointOn = 0, spotOn = 0;
/*
spotOn = 0, no spot light
spotOn = 1, normal spot light
spotOn = 2, directional spot light
spotOn = 3, point spot light
*/
int perPixelOn = 1; // 1: enable per pixel lighting

//int autoRotateMode = 0;

//rotation
bool isRotate = true;
float rotateVal = 0.0f;
int timeInterval = 33;
Matrix4 R;
float rotateSpeed = 300.0; // it is the reciprocal of actual rotate speed

bool crazy_speed = false;

//windows
//int windowHeight, windowWidth;

#define numOfLightSources 4
struct LightSourceParameters {
	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular;
	Vector4 position;
	Vector4 halfVector; //normalized vector between viewpoint and light vector
	Vector3 spotDirection;
	float spotExponent;
	float spotCutoff; // (range: [0.0,90.0], 180.0)
	float spotCosCutoff; // (range: [1.0,0.0],-1.0)
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
}typedef LightSource;
LightSource lightsource[numOfLightSources];


model* models;	// store the models we load
vector<string> filenames; // .obj filename list
int cur_idx = 0; // represent which model should be rendered now



// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		1, 0, 0, vec[0],
		0, 1, 0, vec[1],
		0, 0, 1, vec[2],
		0, 0, 0, 1
	);


	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		vec[0], 0, 0, 0,
		0, vec[1], 0, 0,
		0, 0, vec[2], 0,
		0, 0, 0, 1
	);


	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		1, 0, 0, 0,
		0, cos(val), -sin(val), 0,
		0, sin(val), cos(val), 0,
		0, 0, 0, 1
	);


	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		cos(val), 0, sin(val), 0,
		0, 1, 0, 0,
		-sin(val), 0, cos(val), 0,
		0, 0, 0, 1
	);


	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		cos(val), -sin(val), 0, 0,
		sin(val), cos(val), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);


	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{

	//viewing matrix transform
	Vector3 P1P2 = main_camera.center - main_camera.position;
	Vector3 P1P3 = main_camera.up_vector;
	Vector3 Rz = -(P1P2.normalize());
	Vector3 Rx = P1P2.cross(P1P3).normalize();
	Vector3 Ry = Rz.cross(Rx);

	Matrix4 V_rotation = Matrix4(
		Rx[0], Rx[1], Rx[2], 0,
		Ry[0], Ry[1], Ry[2], 0,
		Rz[0], Rz[1], Rz[2], 0,
		0, 0, 0, 1
	);
	Matrix4 V_translate = Matrix4(
		1, 0, 0, -main_camera.position[0],
		0, 1, 0, -main_camera.position[1],
		0, 0, 1, -main_camera.position[2],
		0, 0, 0, 1
	);

	view_matrix = V_rotation*V_translate;



}

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{


	project_matrix = Matrix4(
		2.0 / (proj.right - proj.left), 0, 0, -(proj.right + proj.left) / (proj.right - proj.left),
		0, 2.0 / (proj.top - proj.bottom), 0, -(proj.top + proj.bottom) / (proj.top - proj.bottom),
		0, 0, 2.0 / (proj.nearClip - proj.farClip), -(proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip),
		0, 0, 0, 1.0
	);


	//std::cout << project_matrix;

}

// [TODO] compute persepective projection matrix
void setPerspective()
{



	project_matrix = Matrix4(
		2.0*proj.nearClip / (proj.right - proj.left), 0, (proj.right + proj.left) / (proj.right - proj.left), 0,
		0, 2.0* proj.nearClip / (proj.top - proj.bottom), (proj.top + proj.bottom) / (proj.top - proj.bottom), 0,
		0, 0, (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip), 2 * proj.farClip*proj.nearClip / (proj.nearClip - proj.farClip),
		0, 0, -1, 0
	);

}

Matrix4 myRotateMatrix(float ax, float ay, float az)
{
	float cosX = cosf(ax);
	float sinX = sinf(ax);
	float cosY = cosf(ay);
	float sinY = sinf(ay);
	float cosZ = cosf(az);
	float sinZ = sinf(az);

	Matrix4 mat;
	mat[0] = cosZ * cosY;
	mat[1] = -sinZ * cosX + cosZ * sinY*sinX;
	mat[2] = sinZ * sinX + cosZ * sinY*cosX;
	mat[3] = 0;
	mat[4] = sinZ * cosY;
	mat[5] = cosZ * cosX + sinZ * sinY*sinX;
	mat[6] = -cosZ * sinX + sinZ * sinY*cosX;
	mat[7] = 0;
	mat[8] = -sinY;
	mat[9] = cosY * sinX;
	mat[11] = 0;
	mat[12] = 0;
	mat[13] = 0;
	mat[14] = 0;
	mat[15] = 1;
	return mat;
}

void timerFunc(int timer_value)
{
	if (isRotate)
		rotateVal += 0.1f;

	if (crazy_speed)
		rotateVal += 5.0f;

//	float t = glutGet(GLUT_ELAPSED_TIME);
//	float offsetTime = t / rotateSpeed;

	if (isRotate) {
		R = Matrix4(
			cos(rotateVal), 0, sin(rotateVal), 0,
			0, 1, 0, 0,
			-sin(rotateVal), 0, cos(rotateVal), 0,
			0, 0, 0, 1);

	}


	glutPostRedisplay();
	glutTimerFunc(timeInterval, timerFunc, 0);
}

void traverseColorModel(model &m)
{
	int i;

	GLfloat maxVal[3]={-100000,-10000,-100000};
	GLfloat minVal[3] = {100000,100000,1000000};


	// number of triangles
	m.vertices = (GLfloat*)malloc(sizeof(GLfloat)*m.obj->numtriangles * 9);
	m.normals = (GLfloat*)malloc(sizeof(GLfloat)*m.obj->numtriangles * 9);

	/*

	float max_x = m.obj->vertices[3];
	float max_y = m.obj->vertices[4];
	float max_z = m.obj->vertices[5];
	float min_x = m.obj->vertices[3];
	float min_y = m.obj->vertices[4];
	float min_z = m.obj->vertices[5];
	*/
	GLMgroup* group = m.obj->groups;
	int offsetIndex = 0;
	// index from 0 - 9 * group->numtriangles-1 is group 1's vertices and normals, and so on
	while (group) {
		for (int i = 0; i < group->numtriangles; i++) {

			
			int triangleIdx = group->triangles[i];

			/*
			// the index of each vertex
			int indv1 = m.obj->triangles[triangleID].vindices[0];
			int indv2 = m.obj->triangles[triangleID].vindices[1];
			int indv3 = m.obj->triangles[triangleID].vindices[2];

			// the index of each normal
			int indn1 = m.obj->triangles[triangleID].nindices[0];
			int indn2 = m.obj->triangles[triangleID].nindices[1];
			int indn3 = m.obj->triangles[triangleID].nindices[2];
			*/


			int indv[3] = {
				m.obj->triangles[triangleIdx].vindices[0],
				m.obj->triangles[triangleIdx].vindices[1],
				m.obj->triangles[triangleIdx].vindices[2]
			};
			int indn[3] = {
				m.obj->triangles[triangleIdx].nindices[0],
				m.obj->triangles[triangleIdx].nindices[1],
				m.obj->triangles[triangleIdx].nindices[2]
			};



			// vertices


			for (int k = 0; k < 3; k++) {
				m.vertices[offsetIndex + i * 9 + 3*k] = m.obj->vertices[indv[k] * 3 + 0];
				m.vertices[offsetIndex + i * 9 + 3*k+1] = m.obj->vertices[indv[k] * 3 + 1];
				m.vertices[offsetIndex + i * 9 + 3*k+2] = m.obj->vertices[indv[k] * 3 + 2];
			
				minVal[0] = min(minVal[0], m.vertices[offsetIndex + i * 9 + 3*k]);
				minVal[1]= min(minVal[1], m.vertices[offsetIndex + i * 9 + 3*k+1]);
				minVal[2] = min(minVal[2], m.vertices[offsetIndex + i * 9 + 3 * k + 2]);

				maxVal[0] = max(maxVal[0], m.vertices[offsetIndex + i * 9 + 3 * k]);
				maxVal[1] = max(maxVal[1], m.vertices[offsetIndex + i * 9 + 3 * k + 1]);
				maxVal[2] = max(maxVal[2], m.vertices[offsetIndex + i * 9 + 3 * k + 2]);

				
				m.normals[offsetIndex + i * 9 + 3 * k] = m.obj->normals[indv[k] * 3 + 0];
				m.normals[offsetIndex + i * 9 + 3 * k + 1] = m.obj->normals[indv[k] * 3 + 1];
				m.normals[offsetIndex + i * 9 + 3 * k + 2] = m.obj->normals[indv[k] * 3 + 2];





			}

		
			
		}
		offsetIndex += 9 * group->numtriangles;
		group = group->next;
		// printf("offsetIndex = %d\n", offsetIndex);
	}



	/*
	float normalize_scale = max(max(abs(max_x - min_x), abs(max_y - min_y)), abs(max_z - min_z));

	Matrix4 S, T;
	S.identity();
	T.identity();
	S[0] = 2 / normalize_scale;
	S[5] = 2 / normalize_scale;;
	S[10] = 2 / normalize_scale;
	T[3] = -(min_x + max_x) / 2;
	T[7] = -(min_y + max_y) / 2;
	T[11] = -(min_z + max_z) / 2;

	m.N = S*T;
	*/

	float norm_scale = max(max(abs(maxVal[0] - minVal[0]), abs(maxVal[1] - minVal[1])), abs(maxVal[2] - minVal[2]));
	Matrix4 S, T;

	S[0] = 2 / norm_scale * scaleOffset;	S[5] = 2 / norm_scale * scaleOffset;	S[10] = 2 / norm_scale * scaleOffset;
	T[3] = -(maxVal[0] + minVal[0]) / 2;
	T[7] = -(maxVal[1] + minVal[1]) / 2;
	T[11] = -(maxVal[2] + minVal[2]) / 2;
	m.N = S * T;

}

void loadConfigFile()
{
	ifstream fin;
	string line;
	fin.open("../../config.txt", ios::in);
	if (fin.is_open())
	{
		while (getline(fin, line))
		{
			filenames.push_back(line);
		}
		fin.close();
	}
	else
	{
		cout << "Unable to open the config file!" << endl;
	}
	for (int i = 0; i < filenames.size(); i++)
		printf("%s\n", filenames[i].c_str());
}


void loadOBJModel()
{

	/*
	// read an obj model here
	if (OBJ != NULL) {
		free(OBJ);
	}
	OBJ = glmReadOBJ((char*)filenames[modelIndex].c_str());
	printf("%s\n", filenames[modelIndex]);

	glmFacetNormals(OBJ);
	glmVertexNormals(OBJ, 90.0);

	// traverse the color model
	traverseColorModel();
	*/

	models = new model[filenames.size()];
	int idx = 0;
	for (string filename : filenames)
	{
		models[idx].obj = glmReadOBJ((char*)filename.c_str());
		traverseColorModel(models[idx++]);
	}

}

void onIdle()
{
	glutPostRedisplay();
}


void setUniformVariables(GLuint p) {

	iLocPosition = glGetAttribLocation(p, "av4position");
	iLocNormal = glGetAttribLocation(p, "av3normal");
	iLocMVP = glGetUniformLocation(p, "mvp");

	iLocMDiffuse = glGetUniformLocation(p, "Material.diffuse");
	iLocMAmbient = glGetUniformLocation(p, "Material.ambient");
	iLocMSpecular = glGetUniformLocation(p, "Material.specular");
	iLocMShininess = glGetUniformLocation(p, "Material.shininess");

	iLocAmbientOn = glGetUniformLocation(p, "ambientOn");
	iLocDiffuseOn = glGetUniformLocation(p, "diffuseOn");
	iLocSpecularOn = glGetUniformLocation(p, "specularOn");
	iLocDirectionalOn = glGetUniformLocation(p, "directionalOn");
	iLocPointOn = glGetUniformLocation(p, "pointOn");
	iLocSpotOn = glGetUniformLocation(p, "spotOn");
	iLocModelTransform_inv_trans = glGetUniformLocation(p, "ModelTrans_inv_transpos");
	iLocModelTrans = glGetUniformLocation(p, "ModelTrans");
	iLocEyePosition = glGetUniformLocation(p, "eyePos");
	iLocPerPixelLighting = glGetUniformLocation(p, "perPixelOn");

	iLocPointPosition = glGetUniformLocation(p, "LightSource[2].position");
	iLocSpotPosition = glGetUniformLocation(p, "LightSource[3].position");
	iLocSpotExponent = glGetUniformLocation(p, "LightSource[3].spotExponent");
	iLocSpotCutoff = glGetUniformLocation(p, "LightSource[3].spotCutoff");
	iLocSpotCosCutoff = glGetUniformLocation(p, "LightSource[3].spotCosCutoff");


	float LightSource_0_pos[] = {
		lightsource[0].position[0],
		lightsource[0].position[1],
		lightsource[0].position[2],
		lightsource[0].position[3],
	};
	float LightSource_0_amb[] = {
		lightsource[0].ambient[0],
		lightsource[0].ambient[1],
		lightsource[0].ambient[2],
		lightsource[0].ambient[3],
	};


	float LightSource_1_pos[] = {
		lightsource[1].position[0],
		lightsource[1].position[1],
		lightsource[1].position[2],
		lightsource[1].position[3],
	};
	float LightSource_1_amb[] = {
		lightsource[1].ambient[0],
		lightsource[1].ambient[1],
		lightsource[1].ambient[2],
		lightsource[1].ambient[3],
	};
	float LightSource_1_diff[] = {
		lightsource[1].diffuse[0],
		lightsource[1].diffuse[1],
		lightsource[1].diffuse[2],
		lightsource[1].diffuse[3],
	};
	float LightSource_1_spec[] = {
		lightsource[1].specular[0],
		lightsource[1].specular[1],
		lightsource[1].specular[2],
		lightsource[1].specular[3],
	};

	float LightSource_2_pos[] = {
		lightsource[2].position[0],
		lightsource[2].position[1],
		lightsource[2].position[2],
		lightsource[2].position[3],
	};
	float LightSource_2_amb[] = {
		lightsource[2].ambient[0],
		lightsource[2].ambient[1],
		lightsource[2].ambient[2],
		lightsource[2].ambient[3],
	};
	float LightSource_2_diff[] = {
		lightsource[2].diffuse[0],
		lightsource[2].diffuse[1],
		lightsource[2].diffuse[2],
		lightsource[2].diffuse[3],
	};
	float LightSource_2_spec[] = {
		lightsource[2].specular[0],
		lightsource[2].specular[1],
		lightsource[2].specular[2],
		lightsource[2].specular[3],
	};

	float LightSource_3_pos[] = {
		lightsource[3].position[0],
		lightsource[3].position[1],
		lightsource[3].position[2],
		lightsource[3].position[3],
	};
	float LightSource_3_amb[] = {
		lightsource[3].ambient[0],
		lightsource[3].ambient[1],
		lightsource[3].ambient[2],
		lightsource[3].ambient[3],
	};
	float LightSource_3_diff[] = {
		lightsource[3].diffuse[0],
		lightsource[3].diffuse[1],
		lightsource[3].diffuse[2],
		lightsource[3].diffuse[3],
	};
	float LightSource_3_spec[] = {
		lightsource[3].specular[0],
		lightsource[3].specular[1],
		lightsource[3].specular[2],
		lightsource[3].specular[3],
	};
	float LightSource_3_spotDir[] = {
		lightsource[3].spotDirection[0],
		lightsource[3].spotDirection[1],
		lightsource[3].spotDirection[2],
		lightsource[3].spotDirection[3],
	};

	glUniform4fv(glGetUniformLocation(p, "LightSource[0].position"), 1, LightSource_0_pos);
	glUniform4fv(glGetUniformLocation(p, "LightSource[0].ambient"), 1, LightSource_0_amb);

	glUniform4fv(glGetUniformLocation(p, "LightSource[1].position"), 1, LightSource_1_pos);
	glUniform4fv(glGetUniformLocation(p, "LightSource[1].ambient"), 1, LightSource_1_amb);
	glUniform4fv(glGetUniformLocation(p, "LightSource[1].diffuse"), 1, LightSource_1_diff);
	glUniform4fv(glGetUniformLocation(p, "LightSource[1].specular"), 1, LightSource_1_spec);
	glUniform1f(glGetUniformLocation(p, "LightSource[1].constantAttenuation"), lightsource[1].constantAttenuation);
	glUniform1f(glGetUniformLocation(p, "LightSource[1].linearAttenuation"), lightsource[1].linearAttenuation);
	glUniform1f(glGetUniformLocation(p, "LightSource[1].quadraticAttenuation"), lightsource[1].quadraticAttenuation);

	glUniform4fv(iLocPointPosition, 1, LightSource_2_pos);
	glUniform4fv(glGetUniformLocation(p, "LightSource[2].ambient"), 1, LightSource_2_amb);
	glUniform4fv(glGetUniformLocation(p, "LightSource[2].diffuse"), 1, LightSource_2_diff);
	glUniform4fv(glGetUniformLocation(p, "LightSource[2].specular"), 1, LightSource_2_spec);
	glUniform1f(glGetUniformLocation(p, "LightSource[2].constantAttenuation"), lightsource[2].constantAttenuation);
	glUniform1f(glGetUniformLocation(p, "LightSource[2].linearAttenuation"), lightsource[2].linearAttenuation);
	glUniform1f(glGetUniformLocation(p, "LightSource[2].quadraticAttenuation"), lightsource[2].quadraticAttenuation);

	glUniform4fv(iLocSpotPosition, 1, LightSource_3_pos);
	glUniform4fv(glGetUniformLocation(p, "LightSource[3].ambient"), 1, LightSource_3_amb);
	glUniform4fv(glGetUniformLocation(p, "LightSource[3].diffuse"), 1, LightSource_3_diff);
	glUniform4fv(glGetUniformLocation(p, "LightSource[3].specular"), 1, LightSource_3_spec);
	glUniform3fv(glGetUniformLocation(p, "LightSource[3].spotDirection"), 1, LightSource_3_spotDir);
	glUniform1f(glGetUniformLocation(p, "LightSource[3].constantAttenuation"), lightsource[2].constantAttenuation);
	glUniform1f(glGetUniformLocation(p, "LightSource[3].linearAttenuation"), lightsource[2].linearAttenuation);
	glUniform1f(glGetUniformLocation(p, "LightSource[3].quadraticAttenuation"), lightsource[2].quadraticAttenuation);
	glUniform1f(iLocSpotExponent, lightsource[3].spotExponent);
	glUniform1f(iLocSpotCutoff, lightsource[3].spotCutoff);
	glUniform1f(iLocSpotCosCutoff, lightsource[3].spotCosCutoff);
}

void onDisplay(void)
{
	// clear canvas
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnableVertexAttribArray(iLocPosition); // av4position
	glEnableVertexAttribArray(iLocNormal); // av3normal

										   //MVP
	Matrix4 T;
	Matrix4 S;
	Matrix4 MVP;


	Matrix4 M = R*models[cur_idx].N;
	//Matrix4 V = getViewTransMatrix();
	//Matrix4 P = getPerpectiveMatrix();

	MVP = project_matrix * view_matrix*M;




	// pass lightsource and effect on/off info to shader
	glUniform1i(iLocAmbientOn, ambientOn);
	glUniform1i(iLocDiffuseOn, diffuseOn);
	glUniform1i(iLocSpecularOn, specularOn);
	glUniform1i(iLocDirectionalOn, directionalOn);
	glUniform1i(iLocPointOn, pointOn);
	glUniform1i(iLocSpotOn, spotOn);

	glUniform1i(iLocPerPixelLighting, perPixelOn);

	// matrix parameters

	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, MVP.getTranspose());
	
//	Matrix4 M_inv_trans = view_matrix*M;
	//M_inv_trans.invert();
	M.invertEuclidean();
	glUniformMatrix4fv(iLocModelTransform_inv_trans, 1, GL_FALSE,M.get());
	glUniformMatrix4fv(iLocModelTrans, 1, GL_FALSE, (R*models[cur_idx].N).getTranspose());




	GLfloat main_camera_pos_vec[3] = {
		main_camera.position.x,
		main_camera.position.y,
		main_camera.position.z
	};
	glUniform3fv(iLocEyePosition, 1, main_camera_pos_vec);



	// light source parameters which may change

	float PointPos[] = {
		lightsource[2].position[0],
		lightsource[2].position[1],
		lightsource[2].position[2],
		lightsource[2].position[3],
	};
	float SpotPos[] = {
		lightsource[3].position[0],
		lightsource[3].position[1],
		lightsource[3].position[2],
		lightsource[3].position[3],
	};


	
	glUniform4fv(iLocPointPosition, 1,PointPos );
	glUniform4fv(iLocSpotPosition, 1, SpotPos);
	glUniform1f(iLocSpotExponent, lightsource[3].spotExponent);
	glUniform1f(iLocSpotCutoff, lightsource[3].spotCutoff);
	glUniform1f(iLocSpotCosCutoff, lightsource[3].spotCosCutoff);

	//pass model material vertices and normals value to the shader
	GLMgroup* group = models[cur_idx].obj->groups;
	int offsetIndex = 0;
	while (group) {
		glUniform4fv(iLocMAmbient, 1, models[cur_idx].obj->materials[group->material].ambient); // Material.ambient
		glUniform4fv(iLocMDiffuse, 1, models[cur_idx].obj->materials[group->material].diffuse); // Material.diffuse
		glUniform4fv(iLocMSpecular, 1, models[cur_idx].obj->materials[group->material].specular); // Material.specular
		glUniform1f(iLocMShininess, models[cur_idx].obj->materials[group->material].shininess); // Material.shininess
																				//printf("shininess = %f\n", OBJ->materials[group->material].shininess);
		glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, models[cur_idx].vertices + offsetIndex);
		glVertexAttribPointer(iLocNormal, 3, GL_FLOAT, GL_FALSE, 0, models[cur_idx].normals + offsetIndex);
		glDrawArrays(GL_TRIANGLES, 0, 3 * (group->numtriangles));
		offsetIndex += 9 * group->numtriangles;
		group = group->next;
	}

	glutSwapBuffers();
}

void showShaderCompileStatus(GLuint shader, GLint *shaderCompiled)
{
	glGetShaderiv(shader, GL_COMPILE_STATUS, shaderCompiled);
	if (GL_FALSE == (*shaderCompiled))
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character.
		GLchar *errorLog = (GLchar*)malloc(sizeof(GLchar) * maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
		fprintf(stderr, "%s", errorLog);

		glDeleteShader(shader);
		free(errorLog);
	}
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vert");
	fs = textFileRead("shader.frag");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	// compile vertex shader
	glCompileShader(v);
	GLint vShaderCompiled;
	showShaderCompileStatus(v, &vShaderCompiled);
	if (!vShaderCompiled) system("pause"), exit(123);

	// compile fragment shader
	glCompileShader(f);
	GLint fShaderCompiled;
	showShaderCompileStatus(f, &fShaderCompiled);
	if (!fShaderCompiled) system("pause"), exit(456);

	p = glCreateProgram();

	// bind shader
	glAttachShader(p, f);
	glAttachShader(p, v);

	// link program
	glLinkProgram(p);

	glUseProgram(p);

	// pass lightsource parameters to shader
	//setLightingSource();

	setUniformVariables(p);
	


}


void printStatus() {
	// print current status of lighting
	cout << setw(20) << "Directional light = " << (directionalOn == 1) ;
	cout << setw(20) << "Point light = " << (pointOn == 1);
	cout << setw(20) << "Spot light = " << (spotOn == 1) ;
	cout << endl;

	cout << setw(20) << "Ambient =  " << (ambientOn == 1);
	cout << setw(20) << "Diffuse = " << (diffuseOn == 1) ;
	cout << setw(20) << "Specular = " << (specularOn == 1) ;
	cout << endl;

}

void onMouse(int who, int state, int x, int y)
{
	//printf("%18s(): (%d, %d) \n", __FUNCTION__, x, y);

	switch (who)
	{
	case GLUT_LEFT_BUTTON: {
		//printf("left button   "); 
		if (spotOn == 1) {
			lightsource[3].spotExponent += 0.1;
			printf("Turn spotExponent up\n");
		}
		break;
	}
	case GLUT_MIDDLE_BUTTON:
		//printf("middle button "); 
		break;
	case GLUT_RIGHT_BUTTON:
		//printf("right button  "); 
		if (spotOn == 1) {
			lightsource[3].spotExponent -= 0.1;
			printf("Turn spotExponent down\n");
		}

		break;
	case GLUT_WHEEL_UP:
		if (spotOn == 1) {
			lightsource[3].spotCosCutoff -= 0.0005;
			printf("Turn CUT_OFF_ANGLE up\n");
		}
		//printf("wheel up      "); 
		break;
	case GLUT_WHEEL_DOWN:
		//printf("wheel down    "); 
		if (spotOn == 1) {
			lightsource[3].spotCosCutoff += 0.0005;
			printf("Turn CUT_OFF_ANGLE down\n");
		}

		break;
	default:
		//printf("0x%02X          ", who); 
		break;
	}

	switch (state)
	{
	case GLUT_DOWN:
		//printf("start "); 
		break;
	case GLUT_UP:
		//printf("end   "); 
		break;
	}

	//printf("\n");
}

void onMouseMotion(int x, int y)
{
	//printf("%18s(): (%d, %d) mouse move\n", __FUNCTION__, x, y);
	
	/*
		float x_pos, y_pos;
	if (spotOn != 0) {

		x_pos= (float)x*(1.0 / 400.0)-1.0;
		y_pos=-(float)y*(1.0 / 400.0)+1.0;
		lightsource[3].position[0] = x_pos;
		lightsource[3].position[1] = y_pos;

	}
	
	*/



}


void onPassiveMouseMotion(int x, int y) {
	float x_pos, y_pos;
	if (spotOn != 0) {

		x_pos = (float)x*(1.0 / 400.0) - 1.0;
		y_pos = -(float)y*(1.0 / 400.0) + 1.0;
		lightsource[3].position[0] = x_pos;
		lightsource[3].position[1] = y_pos;

	}
}



void onKeyboard(unsigned char key, int x, int y)
{
	//printf("%18s(): (%d, %d) key: %c(0x%02X) ", __FUNCTION__, x, y, key, key);
	switch (key)
	{
	case GLUT_KEY_ESC: /* the Esc key */
		exit(0);
		break;
	case 'z':
	case 'Z':
		// switch to the previous model
		cur_idx = (cur_idx + filenames.size() - 1) % filenames.size();
		cout << "Switch to model " << cur_idx << endl;
		break;
	case 'x':
	case 'X':

		// switch to the next model
		cur_idx = (cur_idx + 1) % filenames.size();
		cout << "Switch to model " << cur_idx << endl;

		break;

	case 'h':
	case 'H':
		// show help menu
		printf("----------Instruction Manual----------\n");
		cout << "1:Type 'q' 'w' 'e' to toggle light to source to Direction, Point, and Spot light source" << endl;
		cout << "1.1: Different light source can exist at the same time"<<endl;
		cout << "2:Type 'a' 's' 'd' to toggle lighting parameter to Ambient, Diffuse, and specular  " << endl;
		cout << "2.1: Different lighting parameter can exist at the same time" << endl;
		cout << "3:Type 'z' 'x' to switch models " << endl;
		cout << "4:Type 'r' to  determine whether the model rotate or not " << endl;
		printf("-----------Instruction Manual---------\n");
		break;
	case 'q':
	case 'Q':
		//directionalOn = (directionalOn + 1) % 2;

		directionalOn = (directionalOn == 1) ? 0 : 1;

		printf("Turn %s directional light\n", directionalOn ? "ON" : "OFF");
		printStatus();
		break;
	case 'w':
		//pointOn = (pointOn + 1) % 2;

		pointOn = (pointOn == 1) ? 0 : 1;

		printf("Turn %s point light\n", pointOn ? "ON" : "OFF");
		printStatus();
		break;
	case 'e':
	case 'E':
		spotOn = (spotOn == 1) ? 0 : 1;;
		printf("Turn %s spot light\n", spotOn ? "ON" : "OFF");
		printStatus();
		break;
	case 'a':
	case 'A':
		ambientOn = (ambientOn == 1) ? 0 : 1;
		printf("Turn %s ambient effect\n", ambientOn ? "ON" : "OFF");
		printStatus();
		break;
	case 's':
	case 'S':		
		diffuseOn = (diffuseOn == 1) ? 0 : 1;
		printf("Turn %s diffuse effect\n", diffuseOn ? "ON" : "OFF");
		printStatus();
		break;
	case 'd':
	case 'D':
		specularOn = (specularOn == 1) ? 0 : 1;
		printf("Turn %s specular effect\n", specularOn ? "ON" : "OFF");
		printStatus();
		break;
	case 'r':
	case 'R':
		isRotate = !isRotate;
		printf("Turn %s auto rotate\n", isRotate ? "ON" : "OFF");
		break;


	case 'f':
	case 'F':

		perPixelOn = (perPixelOn == 1) ? 0 : 1;

		(perPixelOn==0)? printf("switch to vertex lighting\n"): printf("switch to per pixel lighting\n");

		break;

	case 'i':

		cout << "-------------developer only-------------" << endl;
		/*
		
		GLint iLocPosition;
		GLint iLocNormal;
		GLint iLocMVP;

		GLint iLocMDiffuse, iLocMAmbient, iLocMSpecular, iLocMShininess;
		GLint iLocAmbientOn, iLocDiffuseOn, iLocSpecularOn;
		GLint iLocDirectionalOn, iLocPointOn, iLocSpotOn;
		GLint iLocNormalTransform, iLocModelTransform, iLocViewTransform;
		GLint iLocEyePosition;
		GLint iLocPointPosition, iLocSpotPosition, iLocSpotExponent, iLocSpotCutoff, iLocSpotCosCutoff;
		GLint iLocPerPixelLighting;
		*/
		cout << "iLocPosition: " << iLocPosition << endl;
		cout << "iLocNormal: " << iLocNormal << endl;
		cout << "iLocMVP: " << iLocMVP << endl;
		cout << "iLocMDiffuse: " << iLocMDiffuse << endl;
		cout << "iLocMAmbient: " << iLocMAmbient << endl;
		cout << "iLocMSpecular: " << iLocMSpecular << endl;
		cout << "iLocMShininess: " << iLocMShininess << endl;
		cout << "iLocAmbientOn: " << iLocAmbientOn << endl;
		cout << "iLocDiffuseOn: " << iLocDiffuseOn << endl;
		cout << "iLocSpecularOn: " << iLocSpecularOn << endl;


		break;
	case 'p':
		crazy_speed = !crazy_speed;
		break;

	}


	//printf("\n");
}

void onKeyboardSpecial(int key, int x, int y) {
	//printf("%18s(): (%d, %d) ", __FUNCTION__, x, y);

	//move the directioal light position
	switch (key)
	{
	case GLUT_KEY_LEFT:
		//printf("key: LEFT ARROW");
		if (pointOn == 1) {
			lightsource[2].position[0] -= 0.2;
		}
		break;

	case GLUT_KEY_RIGHT:
		//printf("key: RIGHT ARROW");
		if (pointOn == 1) {
			lightsource[2].position[0] += 0.2;
		}
		break;
	case GLUT_KEY_UP:
		if (pointOn == 1) {
			lightsource[2].position[1] += 0.2;
		}
		break;
	case GLUT_KEY_DOWN:
		if (pointOn == 1) {
			lightsource[2].position[1] -= 0.2;
		}
		break;
	default:
		//printf("key: 0x%02X      ", key);
		break;
	}
	//printf("\n");
}


void onWindowReshape(int width, int height)
{
	printf("%18s(): %dx%d\n", __FUNCTION__, width, height);
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 1.0;
	proj.farClip = 10.0;
	proj.fovy = 60;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	setViewingMatrix();
	setOrthogonal();	//set default projection matrix as orthogonal matrix


	//Hw2

	//float PLRange = 70.0; // todo
						  // 0: Ambient
	lightsource[0].position[0] = 0;
	lightsource[0].position[1] = 0;
	lightsource[0].position[2] = -1;
	lightsource[0].position[3] = 1;
	lightsource[0].ambient[0] = 0.75;
	lightsource[0].ambient[1] = 0.75;
	lightsource[0].ambient[2] = 0.75;
	lightsource[0].ambient[3] = 1;

	// 1: directional light
	lightsource[1].position[0] = -1;
	lightsource[1].position[1] = 1;
	lightsource[1].position[2] = 1;
	lightsource[1].position[3] = 1;
	lightsource[1].ambient[0] = 0.15;
	lightsource[1].ambient[1] = 0.15;
	lightsource[1].ambient[2] = 0.15;
	lightsource[1].ambient[3] = 1.0;
	lightsource[1].diffuse[0] = 1.0;
	lightsource[1].diffuse[1] = 1.0;
	lightsource[1].diffuse[2] = 1.0;
	lightsource[1].diffuse[3] = 1;
	lightsource[1].specular[0] = 1;
	lightsource[1].specular[1] = 1;
	lightsource[1].specular[2] = 1;
	lightsource[1].specular[3] = 1;
	lightsource[1].constantAttenuation = 0.05;
	lightsource[1].linearAttenuation = 0.3;
	lightsource[1].quadraticAttenuation = 0.6;

	// 2: point light
	lightsource[2].position[0] = 0;
	lightsource[2].position[1] = -1;
	lightsource[2].position[2] = 0;
	lightsource[2].position[3] = 1;
	lightsource[2].ambient[0] = 0.15;
	lightsource[2].ambient[1] = 0.15;
	lightsource[2].ambient[2] = 0.15;
	lightsource[2].ambient[3] = 1.0;
	lightsource[2].diffuse[0] = 1;
	lightsource[2].diffuse[1] = 1;
	lightsource[2].diffuse[2] = 1;
	lightsource[2].diffuse[3] = 1;
	lightsource[2].specular[0] = 1;
	lightsource[2].specular[1] = 1;
	lightsource[2].specular[2] = 1;
	lightsource[2].specular[3] = 1;
	lightsource[2].constantAttenuation = 0.05;
	lightsource[2].linearAttenuation = 0.3;
	lightsource[2].quadraticAttenuation = 0.6;

	// 3: spot light
	lightsource[3].position[0] = 0;
	lightsource[3].position[1] = 0;
	lightsource[3].position[2] = 2;
	lightsource[3].position[3] = 1;
	lightsource[3].ambient[0] = 0.15;
	lightsource[3].ambient[1] = 0.15;
	lightsource[3].ambient[2] = 0.15;
	lightsource[3].ambient[3] = 1;
	lightsource[3].diffuse[0] = 1;
	lightsource[3].diffuse[1] = 1;
	lightsource[3].diffuse[2] = 1;
	lightsource[3].diffuse[3] = 1;
	lightsource[3].specular[0] = 1;
	lightsource[3].specular[1] = 1;
	lightsource[3].specular[2] = 1;
	lightsource[3].specular[3] = 1;
	lightsource[3].spotDirection[0] = 0;
	lightsource[3].spotDirection[1] = 0;
	lightsource[3].spotDirection[2] = -2;
	lightsource[3].spotExponent = 0.5;
	lightsource[3].spotCutoff = 45;
	lightsource[3].spotCosCutoff = 0.99; // 1/12 pi
	lightsource[3].constantAttenuation = 0.05;
	lightsource[3].linearAttenuation = 0.3;
	lightsource[3].quadraticAttenuation = 0.6;


}


int main(int argc, char **argv)
{

	loadConfigFile();
	initParameter();



	// glut init
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

	// create window
	glutInitWindowPosition(500, 100);
	glutInitWindowSize(800, 800);

	glutCreateWindow(" CS550000 CG HW2 103011227");

	glewInit();
	if (glewIsSupported("GL_VERSION_2_0")) {
		printf("Ready for OpenGL 2.0\n");
	}
	else {
		printf("OpenGL 2.0 not supported\n");
		system("pause");
		exit(1);
	}

	// load obj models through glm
	loadOBJModel();

	// register glut callback functions
	glutDisplayFunc(onDisplay);
	glutIdleFunc(onIdle);
	glutKeyboardFunc(onKeyboard);
	glutSpecialFunc(onKeyboardSpecial);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMouseMotion);
	glutPassiveMotionFunc(onPassiveMouseMotion);
	glutReshapeFunc(onWindowReshape);
	glutTimerFunc(timeInterval, timerFunc, 0);

	

	// set up shaders here
	setShaders();

	glEnable(GL_DEPTH_TEST);

	// main loop
	glutMainLoop();

	// free

	//glmDelete(OBJ);
	for (int i = 0; i < filenames.size(); i++)
	{
		glmDelete(models[i].obj);
	}

	return 0;
}

