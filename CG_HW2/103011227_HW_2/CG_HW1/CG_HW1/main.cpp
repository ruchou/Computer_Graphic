#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>


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

#ifndef GLUT_KEY_z
# define GLUT_KEY_z 0x007A
#endif

#ifndef GLUT_KEY_x
# define GLUT_KEY_x 0x0078
#endif

#ifndef GLUT_KEY_h
# define GLUT_KEY_h 0x0068
#endif

#ifndef GLUT_KEY_q
# define GLUT_KEY_q 0x0071
#endif

#ifndef GLUT_KEY_w
# define GLUT_KEY_w 0x0077
#endif

#ifndef GLUT_KEY_e
# define GLUT_KEY_e 0x0065
#endif

#ifndef GLUT_KEY_a
# define GLUT_KEY_a 0x0061
#endif

#ifndef GLUT_KEY_s
# define GLUT_KEY_s 0x0073
#endif

#ifndef GLUT_KEY_d
# define GLUT_KEY_d 0x0064
#endif

#ifndef GLUT_KEY_r
# define GLUT_KEY_r 0x0072
#endif

#ifndef GLUT_KEY_v
# define GLUT_KEY_v 0x0076
#endif

#ifndef GLUT_KEY_b
# define GLUT_KEY_b 0x0062
#endif

#ifndef GLUT_KEY_c
# define GLUT_KEY_c 0x0063
#endif

#ifndef GLUT_KEY_f
# define GLUT_KEY_f 0x0066
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
GLint iLocNormalTransform, iLocModelTransform, iLocViewTransform;
GLint iLocEyePosition;
GLint iLocPointPosition, iLocSpotPosition, iLocSpotExponent, iLocSpotCutoff, iLocSpotCosCutoff;
GLint iLocPerPixelLighting;

#define numOfModels 5



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



//int modelIndex = 0;

//GLMmodel* OBJ;
//GLfloat* vertices; // index from 0 - 9 * group->numtriangles-1 is group 1's vertices, and so on
//GLfloat* normals; // index from 0 - 9 * group->numtriangles-1 is group 1's normals, and so on
//Matrix4 N;

float xmin = -1.0, xmax = 1.0, ymin = -1.0, ymax = 1.0, znear = 1.0, zfar = 3.0; // zfar/znear should be positive
Vector3 eyePos = Vector3(0, 0, 2);
Vector3 centerPos = Vector3(0, 0, 0);
Vector3 upVec = Vector3(0, 1, 0);// in fact, this vector should be called as P1P3

int ambientOn = 1, diffuseOn = 1, specularOn = 1;
int directionalOn = 0, pointOn = 0, spotOn = 0;
/*
spotOn = 0, no spot light
spotOn = 1, normal spot light
spotOn = 2, directional spot light
spotOn = 3, point spot light
*/
int perPixelOn = 0; // 1: enable per pixel lighting

int autoRotateMode = 0;
float rotateSpeed = 300.0; // it is the reciprocal of actual rotate speed

int windowHeight, windowWidth;

#define numOfLightSources 4
struct LightSourceParameters {
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float position[4];
	float halfVector[4];
	float spotDirection[3];
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




Matrix4 getPerpectiveMatrix() {
	// Zfar > Znear > 0
	// in OpenGL api
	// Xmax = Right, Xmin = Left
	// Ymax = Top, Ymin = Bottom
	// Znear = -Near, Zfar = -far
	return Matrix4(
		2.0*znear / (xmax - xmin), 0, (xmax + xmin) / (xmin - xmax), 0,
		0, 2.0*znear / (ymax - ymin), (ymax + ymin) / (ymin - ymax), 0,
		0, 0, (zfar + znear) / (znear - zfar), (2.0*zfar*znear) / (znear - zfar),
		0, 0, -1, 0
	);
}

Matrix4 getViewTransMatrix() {
	/*
	use global parameter eyePos, centerPos, upVec to compute Viewing Transformation Matrix
	*/
	Vector3 P1P2 = centerPos - eyePos;
	Vector3 P1P3 = upVec;
	Vector3 Rz = P1P2.normalize();
	Vector3 Rx = P1P2.cross(P1P3).normalize();
	Vector3 Ry = Rx.cross(Rz).normalize();

	Matrix4 Rv = Matrix4(
		Rx[0], Rx[1], Rx[2], 0, // new X axis
		Ry[0], Ry[1], Ry[2], 0, // new up vector
		-Rz[0], -Rz[1], -Rz[2], 0,// new direction/forward vector
		0, 0, 0, 1);
	Matrix4 Rt = Matrix4(
		1, 0, 0, -eyePos[0],
		0, 1, 0, -eyePos[1],
		0, 0, 1, -eyePos[2],
		0, 0, 0, 1);
	//std::cout <<"Rv"<< Rv << std::endl;
	//std::cout <<"Rt"<< Rt << std::endl;
	//upVec = Ry;
	return Rv*Rt;
}


void traverseColorModel(model &m)
{
	int i;

	GLfloat maxVal[3];
	GLfloat minVal[3];


	// number of triangles
	m.vertices = (GLfloat*)malloc(sizeof(GLfloat)*m.obj->numtriangles * 9);
	m.normals = (GLfloat*)malloc(sizeof(GLfloat)*m.obj->numtriangles * 9);

	float max_x = m.obj->vertices[3];
	float max_y = m.obj->vertices[4];
	float max_z = m.obj->vertices[5];
	float min_x = m.obj->vertices[3];
	float min_y = m.obj->vertices[4];
	float min_z = m.obj->vertices[5];

	GLMgroup* group = m.obj->groups;
	int offsetIndex = 0;
	// index from 0 - 9 * group->numtriangles-1 is group 1's vertices and normals, and so on
	while (group) {
		for (int i = 0; i < group->numtriangles; i++) {
			int triangleID = group->triangles[i];
			// the index of each vertex
			int indv1 = m.obj->triangles[triangleID].vindices[0];
			int indv2 = m.obj->triangles[triangleID].vindices[1];
			int indv3 = m.obj->triangles[triangleID].vindices[2];

			// the index of each normal
			int indn1 = m.obj->triangles[triangleID].nindices[0];
			int indn2 = m.obj->triangles[triangleID].nindices[1];
			int indn3 = m.obj->triangles[triangleID].nindices[2];


			// vertices

			m.vertices[offsetIndex + i * 9 + 0] = m.obj->vertices[indv1 * 3 + 0];
			m.vertices[offsetIndex + i * 9 + 1] = m.obj->vertices[indv1 * 3 + 1];
			m.vertices[offsetIndex + i * 9 + 2] = m.obj->vertices[indv1 * 3 + 2];
			if (m.vertices[offsetIndex + i * 9 + 0] > max_x) max_x = m.vertices[offsetIndex + i * 9 + 0];
			if (m.vertices[offsetIndex + i * 9 + 1] > max_y) max_y = m.vertices[offsetIndex + i * 9 + 1];
			if (m.vertices[offsetIndex + i * 9 + 2] > max_z) max_z = m.vertices[offsetIndex + i * 9 + 2];
			if (m.vertices[offsetIndex + i * 9 + 0] < min_x) min_x = m.vertices[offsetIndex + i * 9 + 0];
			if (m.vertices[offsetIndex + i * 9 + 1] < min_y) min_y = m.vertices[offsetIndex + i * 9 + 1];
			if (m.vertices[offsetIndex + i * 9 + 2] < min_z) min_z = m.vertices[offsetIndex + i * 9 + 2];

			m.vertices[offsetIndex + i * 9 + 3] = m.obj->vertices[indv2 * 3 + 0];
			m.vertices[offsetIndex + i * 9 + 4] = m.obj->vertices[indv2 * 3 + 1];
			m.vertices[offsetIndex + i * 9 + 5] = m.obj->vertices[indv2 * 3 + 2];
			if (m.vertices[offsetIndex + i * 9 + 3] > max_x) max_x = m.vertices[offsetIndex + i * 9 + 3];
			if (m.vertices[offsetIndex + i * 9 + 4] > max_y) max_y = m.vertices[offsetIndex + i * 9 + 4];
			if (m.vertices[offsetIndex + i * 9 + 5] > max_z) max_z = m.vertices[offsetIndex + i * 9 + 5];
			if (m.vertices[offsetIndex + i * 9 + 3] < min_x) min_x = m.vertices[offsetIndex + i * 9 + 3];
			if (m.vertices[offsetIndex + i * 9 + 4] < min_y) min_y = m.vertices[offsetIndex + i * 9 + 4];
			if (m.vertices[offsetIndex + i * 9 + 5] < min_z) min_z = m.vertices[offsetIndex + i * 9 + 5];

			m.vertices[offsetIndex + i * 9 + 6] = m.obj->vertices[indv3 * 3 + 0];
			m.vertices[offsetIndex + i * 9 + 7] = m.obj->vertices[indv3 * 3 + 1];
			m.vertices[offsetIndex + i * 9 + 8] = m.obj->vertices[indv3 * 3 + 2];
			if (m.vertices[offsetIndex + i * 9 + 6] > max_x) max_x = m.vertices[offsetIndex + i * 9 + 6];
			if (m.vertices[offsetIndex + i * 9 + 7] > max_y) max_y = m.vertices[offsetIndex + i * 9 + 7];
			if (m.vertices[offsetIndex + i * 9 + 8] > max_z) max_z = m.vertices[offsetIndex + i * 9 + 8];
			if (m.vertices[offsetIndex + i * 9 + 6] < min_x) min_x = m.vertices[offsetIndex + i * 9 + 6];
			if (m.vertices[offsetIndex + i * 9 + 7] < min_y) min_y = m.vertices[offsetIndex + i * 9 + 7];
			if (m.vertices[offsetIndex + i * 9 + 8] < min_z) min_z = m.vertices[offsetIndex + i * 9 + 8];

			// colors

			m.normals[offsetIndex + i * 9 + 0] = m.obj->normals[indn1 * 3 + 0];
			m.normals[offsetIndex + i * 9 + 1] = m.obj->normals[indn1 * 3 + 1];
			m.normals[offsetIndex + i * 9 + 2] = m.obj->normals[indn1 * 3 + 2];

			m.normals[offsetIndex + i * 9 + 3] = m.obj->normals[indn2 * 3 + 0];
			m.normals[offsetIndex + i * 9 + 4] = m.obj->normals[indn2 * 3 + 1];
			m.normals[offsetIndex + i * 9 + 5] = m.obj->normals[indn2 * 3 + 2];

			m.normals[offsetIndex + i * 9 + 6] = m.obj->normals[indn3 * 3 + 0];
			m.normals[offsetIndex + i * 9 + 7] = m.obj->normals[indn3 * 3 + 1];
			m.normals[offsetIndex + i * 9 + 8] = m.obj->normals[indn3 * 3 + 2];

		}
		offsetIndex += 9 * group->numtriangles;
		group = group->next;
		// printf("offsetIndex = %d\n", offsetIndex);
	}



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

void passMatrixToShader(GLint iLoc, Matrix4 matrix) {
	// pass 4x4 matrix to shader, row-major --> column major
	GLfloat MATRIX[16];
	// row-major ---> column-major
	MATRIX[0] = matrix[0];  MATRIX[4] = matrix[1];   MATRIX[8] = matrix[2];    MATRIX[12] = matrix[3];
	MATRIX[1] = matrix[4];  MATRIX[5] = matrix[5];   MATRIX[9] = matrix[6];    MATRIX[13] = matrix[7];
	MATRIX[2] = matrix[8];  MATRIX[6] = matrix[9];   MATRIX[10] = matrix[10];   MATRIX[14] = matrix[11];
	MATRIX[3] = matrix[12]; MATRIX[7] = matrix[13];  MATRIX[11] = matrix[14];   MATRIX[15] = matrix[15];
	glUniformMatrix4fv(iLoc, 1, GL_FALSE, MATRIX);
}

void passVector3ToShader(GLint iLoc, Vector3 vector) {
	GLfloat vec[3];
	vec[0] = vector[0];
	vec[1] = vector[1];
	vec[2] = vector[2];
	glUniform3fv(iLoc, 1, vec);
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
	Matrix4 R;
	if (autoRotateMode == 0) {
		R = Matrix4(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);
	}
	else {
		float t = glutGet(GLUT_ELAPSED_TIME);
		float offsetTime = t / rotateSpeed;
		R = Matrix4(
			cos(offsetTime), 0, sin(offsetTime), 0,
			0, 1, 0, 0,
			-sin(offsetTime), 0, cos(offsetTime), 0,
			0, 0, 0, 1);
		// rotate around
	}
	Matrix4 M = R*models[cur_idx].N;
	Matrix4 V = getViewTransMatrix();
	Matrix4 P = getPerpectiveMatrix();

	Matrix4 MVP = P*V*M;




	// pass lightsource and effect on/off info to shader
	glUniform1i(iLocAmbientOn, ambientOn);
	glUniform1i(iLocDiffuseOn, diffuseOn);
	glUniform1i(iLocSpecularOn, specularOn);
	glUniform1i(iLocDirectionalOn, directionalOn);
	glUniform1i(iLocPointOn, pointOn);
	glUniform1i(iLocSpotOn, spotOn);

	glUniform1i(iLocPerPixelLighting, perPixelOn);

	// matrix parameters
	passMatrixToShader(iLocMVP, MVP);
	passMatrixToShader(iLocNormalTransform,models[cur_idx].N );
	passMatrixToShader(iLocViewTransform, V);
	passMatrixToShader(iLocModelTransform, M);

	passVector3ToShader(iLocEyePosition, eyePos);

	// light source parameters which may change
	glUniform4fv(iLocPointPosition, 1, lightsource[2].position);
	glUniform4fv(iLocSpotPosition, 1, lightsource[3].position);
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
void setLightingSource() {
	float PLRange = 70.0; // todo
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
	lightsource[1].position[0] = 0;
	lightsource[1].position[1] = 1;
	lightsource[1].position[2] = 1;
	lightsource[1].position[3] = 1;
	lightsource[1].ambient[0] = 0;
	lightsource[1].ambient[1] = 0;
	lightsource[1].ambient[2] = 0;
	lightsource[1].ambient[3] = 1;
	lightsource[1].diffuse[0] = 0.8;
	lightsource[1].diffuse[1] = 0.8;
	lightsource[1].diffuse[2] = 0.8;
	lightsource[1].diffuse[3] = 1;
	lightsource[1].specular[0] = 0.8;
	lightsource[1].specular[1] = 0.8;
	lightsource[1].specular[2] = 0.8;
	lightsource[1].specular[3] = 1;
	lightsource[1].constantAttenuation = 1;
	lightsource[1].linearAttenuation = 4.5 / PLRange;
	lightsource[1].quadraticAttenuation = 75 / (PLRange*PLRange);

	// 2: point light
	lightsource[2].position[0] = 1;
	lightsource[2].position[1] = 2;
	lightsource[2].position[2] = 0;
	lightsource[2].position[3] = 1;
	lightsource[2].ambient[0] = 0;
	lightsource[2].ambient[1] = 0;
	lightsource[2].ambient[2] = 0;
	lightsource[2].ambient[3] = 1;
	lightsource[2].diffuse[0] = 0.8;
	lightsource[2].diffuse[1] = 0.8;
	lightsource[2].diffuse[2] = 0.8;
	lightsource[2].diffuse[3] = 1;
	lightsource[2].specular[0] = 0.8;
	lightsource[2].specular[1] = 0.8;
	lightsource[2].specular[2] = 0.8;
	lightsource[2].specular[3] = 1;
	lightsource[2].constantAttenuation = 1;
	lightsource[2].linearAttenuation = 4.5 / PLRange;
	lightsource[2].quadraticAttenuation = 75 / (PLRange*PLRange);

	// 3: spot light
	lightsource[3].position[0] = 0;
	lightsource[3].position[1] = 0;
	lightsource[3].position[2] = 1;
	lightsource[3].position[3] = 1;
	lightsource[3].ambient[0] = 0;
	lightsource[3].ambient[1] = 0;
	lightsource[3].ambient[2] = 0;
	lightsource[3].ambient[3] = 1;
	lightsource[3].diffuse[0] = 0.8;
	lightsource[3].diffuse[1] = 0.8;
	lightsource[3].diffuse[2] = 0.8;
	lightsource[3].diffuse[3] = 1;
	lightsource[3].specular[0] = 0.8;
	lightsource[3].specular[1] = 0.8;
	lightsource[3].specular[2] = 0.8;
	lightsource[3].specular[3] = 1;
	lightsource[3].spotDirection[0] = 0;
	lightsource[3].spotDirection[1] = 0;
	lightsource[3].spotDirection[2] = -2;
	lightsource[3].spotExponent = 0.5;
	lightsource[3].spotCutoff = 45;
	lightsource[3].spotCosCutoff = 0.99; // 1/12 pi
	lightsource[3].constantAttenuation = 1;
	lightsource[3].linearAttenuation = 4.5 / PLRange;
	lightsource[3].quadraticAttenuation = 75 / (PLRange*PLRange);

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
	iLocNormalTransform = glGetUniformLocation(p, "NormalTransMatrix");
	iLocModelTransform = glGetUniformLocation(p, "ModelTransMatrix");
	iLocViewTransform = glGetUniformLocation(p, "ViewTransMatrix");
	iLocEyePosition = glGetUniformLocation(p, "eyePos");
	iLocPerPixelLighting = glGetUniformLocation(p, "perPixelOn");

	iLocPointPosition = glGetUniformLocation(p, "LightSource[2].position");
	iLocSpotPosition = glGetUniformLocation(p, "LightSource[3].position");
	iLocSpotExponent = glGetUniformLocation(p, "LightSource[3].spotExponent");
	iLocSpotCutoff = glGetUniformLocation(p, "LightSource[3].spotCutoff");
	iLocSpotCosCutoff = glGetUniformLocation(p, "LightSource[3].spotCosCutoff");

	glUseProgram(p);

	// pass lightsource parameters to shader
	setLightingSource();

	glUniform4fv(glGetUniformLocation(p, "LightSource[0].position"), 1, lightsource[0].position);
	glUniform4fv(glGetUniformLocation(p, "LightSource[0].ambient"), 1, lightsource[0].ambient);

	glUniform4fv(glGetUniformLocation(p, "LightSource[1].position"), 1, lightsource[1].position);
	glUniform4fv(glGetUniformLocation(p, "LightSource[1].ambient"), 1, lightsource[1].ambient);
	glUniform4fv(glGetUniformLocation(p, "LightSource[1].diffuse"), 1, lightsource[1].diffuse);
	glUniform4fv(glGetUniformLocation(p, "LightSource[1].specular"), 1, lightsource[1].specular);
	glUniform1f(glGetUniformLocation(p, "LightSource[1].constantAttenuation"), lightsource[1].constantAttenuation);
	glUniform1f(glGetUniformLocation(p, "LightSource[1].linearAttenuation"), lightsource[1].linearAttenuation);
	glUniform1f(glGetUniformLocation(p, "LightSource[1].quadraticAttenuation"), lightsource[1].quadraticAttenuation);

	glUniform4fv(iLocPointPosition, 1, lightsource[2].position);
	glUniform4fv(glGetUniformLocation(p, "LightSource[2].ambient"), 1, lightsource[2].ambient);
	glUniform4fv(glGetUniformLocation(p, "LightSource[2].diffuse"), 1, lightsource[2].diffuse);
	glUniform4fv(glGetUniformLocation(p, "LightSource[2].specular"), 1, lightsource[2].specular);
	glUniform1f(glGetUniformLocation(p, "LightSource[2].constantAttenuation"), lightsource[2].constantAttenuation);
	glUniform1f(glGetUniformLocation(p, "LightSource[2].linearAttenuation"), lightsource[2].linearAttenuation);
	glUniform1f(glGetUniformLocation(p, "LightSource[2].quadraticAttenuation"), lightsource[2].quadraticAttenuation);

	glUniform4fv(iLocSpotPosition, 1, lightsource[3].position);
	glUniform4fv(glGetUniformLocation(p, "LightSource[3].ambient"), 1, lightsource[3].ambient);
	glUniform4fv(glGetUniformLocation(p, "LightSource[3].diffuse"), 1, lightsource[3].diffuse);
	glUniform4fv(glGetUniformLocation(p, "LightSource[3].specular"), 1, lightsource[3].specular);
	glUniform3fv(glGetUniformLocation(p, "LightSource[3].spotDirection"), 1, lightsource[3].spotDirection);
	glUniform1f(glGetUniformLocation(p, "LightSource[3].constantAttenuation"), lightsource[2].constantAttenuation);
	glUniform1f(glGetUniformLocation(p, "LightSource[3].linearAttenuation"), lightsource[2].linearAttenuation);
	glUniform1f(glGetUniformLocation(p, "LightSource[3].quadraticAttenuation"), lightsource[2].quadraticAttenuation);
	glUniform1f(iLocSpotExponent, lightsource[3].spotExponent);
	glUniform1f(iLocSpotCutoff, lightsource[3].spotCutoff);
	glUniform1f(iLocSpotCosCutoff, lightsource[3].spotCosCutoff);


}


void printStatus() {
	// print current status of lighting
	printf("Directional light = %s, Point light = %s, Spot light = %s\n",
		directionalOn ? "ON" : "OFF", pointOn ? "ON" : "OFF", spotOn ? "ON" : "OFF");
	printf("Ambient = %s, Diffuse = %s, Specular = %s\n",
		ambientOn ? "ON" : "OFF", diffuseOn ? "ON" : "OFF", specularOn ? "ON" : "OFF");
	printf("\n");
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
			lightsource[3].spotCosCutoff -= 0.003;
			printf("Turn CUT_OFF_ANGLE up\n");
		}
		//printf("wheel up      "); 
		break;
	case GLUT_WHEEL_DOWN:
		//printf("wheel down    "); 
		if (spotOn == 1) {
			lightsource[3].spotCosCutoff += 0.003;
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
}

void onPassiveMouseMotion(int x, int y) {
	// move the position of spot light
	if (spotOn != 0) {
		float wx = (float)x*2.0 / (float)windowWidth - 1.0;
		float wy = -(float)y*2.0 / (float)windowHeight + 1.0;
		// printf("new x = %f, y= %f\n", wx, wy);
		lightsource[3].position[0] = wx;
		lightsource[3].position[1] = wy;
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
	case GLUT_KEY_z:
		// switch to the previous model
		cur_idx = (cur_idx + filenames.size() - 1) % filenames.size();
		cout << cur_idx << endl;

		break;
	case GLUT_KEY_x:
		// switch to the next model
		cur_idx = (cur_idx + 1) % filenames.size();
		cout << cur_idx << endl;

		break;

	case GLUT_KEY_h:
		// show help menu
		printf("----------Help Menu----------\n");
		printf("press 'q' 'w' 'e' to toggle the light source\n");
		printf("press 'a' 's' 'd' to toggle the light arrtibute\n");
		printf("press 'z' 'x' to change model\n");
		printf("press 'r' to toggle auto rotation\n");
		printf("press 'v' 'b' to change the rotatation speed\n");
		printf("press 'c' to change the spot light into another type\n");
		printf("press 'f' to toggle per-pixel rendering\n");
		printf("use arrow buttom to move the point light\n");
		printf("hover mouse to move the spot light\n");
		printf("click mouse to tune EXP\n");
		printf("scroll mouse to tune CUT_OFF_ANGLE\n");
		printf("----------Help Menu----------\n");
		break;
	case GLUT_KEY_q:
		directionalOn = (directionalOn + 1) % 2;
		printf("Turn %s directional light\n", directionalOn ? "ON" : "OFF");
		printStatus();
		break;
	case GLUT_KEY_w:
		pointOn = (pointOn + 1) % 2;
		printf("Turn %s point light\n", pointOn ? "ON" : "OFF");
		printStatus();
		break;
	case GLUT_KEY_e:
		if (spotOn == 0) {
			spotOn = 1;
		}
		else {
			spotOn = 0;
		}
		printf("Turn %s spot light\n", spotOn ? "ON" : "OFF");
		printStatus();
		break;
	case GLUT_KEY_a:
		ambientOn = (ambientOn + 1) % 2;
		printf("Turn %s ambient effect\n", ambientOn ? "ON" : "OFF");
		printStatus();
		break;
	case GLUT_KEY_s:
		diffuseOn = (diffuseOn + 1) % 2;
		printf("Turn %s diffuse effect\n", diffuseOn ? "ON" : "OFF");
		printStatus();
		break;
	case GLUT_KEY_d:
		specularOn = (specularOn + 1) % 2;
		printf("Turn %s specular effect\n", specularOn ? "ON" : "OFF");
		printStatus();
		break;
	case GLUT_KEY_r:
		autoRotateMode = (autoRotateMode + 1) % 2;
		printf("Turn %s auto rotate\n", autoRotateMode ? "ON" : "OFF");
		break;
	case GLUT_KEY_v:
		if (autoRotateMode == 1) {
			if (rotateSpeed > 50.0) {
				printf("Speed up the auto rotation\n");
				rotateSpeed -= 50.0;
			}
			else {
				printf("Please do not try to rotate too fast\n");
			}
		}
		break;
	case GLUT_KEY_b:
		if (autoRotateMode == 1) {
			rotateSpeed += 50.0;
			printf("Slow down the auot rotation\n");
		}
		break;
	case GLUT_KEY_c:
		switch (spotOn) {
		case 1:
			spotOn = 2;
			printf("change spot light into directional mode\n");
			break;
		case 2:
			spotOn = 3;
			printf("change spot light into point mode\n");
			break;
		case 3:
			spotOn = 1;
			printf("change spot light into normal spotlight mode\n");
			break;
		}
		break;
	case GLUT_KEY_f:
		perPixelOn = (perPixelOn + 1) % 2;
		if (perPixelOn == 0) {
			printf("switch to vertex lighting\n");
		}
		else {
			printf("switch to per pixel lighting\n");
		}
		break;
	}
	//printf("\n");
}

void onKeyboardSpecial(int key, int x, int y) {
	//printf("%18s(): (%d, %d) ", __FUNCTION__, x, y);
	switch (key)
	{
	case GLUT_KEY_LEFT:
		//printf("key: LEFT ARROW");
		if (pointOn == 1) {
			lightsource[2].position[0] -= 0.5;
		}
		break;

	case GLUT_KEY_RIGHT:
		//printf("key: RIGHT ARROW");
		if (pointOn == 1) {
			lightsource[2].position[0] += 0.5;
		}
		break;
	case GLUT_KEY_UP:
		if (pointOn == 1) {
			lightsource[2].position[1] += 0.5;
		}
		break;
	case GLUT_KEY_DOWN:
		if (pointOn == 1) {
			lightsource[2].position[1] -= 0.5;
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
	windowHeight = height;
	windowWidth = width;
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
}


int main(int argc, char **argv)
{

	loadConfigFile();
	initParameter();



	// glut init
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

	// create window
	windowHeight = windowWidth = 800;
	glutInitWindowPosition(500, 100);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("10420 CS550000 CG HW2 103011227");

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
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// register glut callback functions
	glutDisplayFunc(onDisplay);
	glutIdleFunc(onIdle);
	glutKeyboardFunc(onKeyboard);
	glutSpecialFunc(onKeyboardSpecial);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMouseMotion);
	glutPassiveMotionFunc(onPassiveMouseMotion);
	glutReshapeFunc(onWindowReshape);

	// set up lighting parameters
	// setLightingSource(); 
	// call in setShaders();

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

