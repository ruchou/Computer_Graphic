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

#define PI 3.1415926

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
GLuint vaoHandle;
GLuint vboHandles[2];
GLuint positionBufferHandle;
GLuint colorBufferHandle;

// Shader attributes for uniform variables
GLuint iLocP;
GLuint iLocV;
GLuint iLocN;

struct iLocLightInfo
{
	GLuint position;
	GLuint ambient;
	GLuint diffuse;
	GLuint specular;
}iLocLightInfo[3];

struct Group
{
	int numTriangles;
	GLfloat *vertices;
	GLfloat *normals;
	GLfloat ambient[4];
	GLfloat diffuse[4];
	GLfloat specular[4];
	GLfloat shininess;
};

struct Model
{
	int numGroups;
	GLMmodel *obj;
	Group *group;
	Matrix4 N;
	Vector3 position = Vector3(0,0,0);
	Vector3 scale = Vector3(1,1,1);
	Vector3 rotation = Vector3(0,0,0);	// Euler form
};

Matrix4 V = Matrix4(
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1);

Matrix4 P = Matrix4(
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, -1, 0,
	0, 0, 0, 1);

Matrix4 R;

int current_x, current_y;

Model* models;	// store the models we load
vector<string> filenames; // .obj filename list
int cur_idx = 0; // represent which model should be rendered now
int color_mode = 0;
bool use_wire_mode = false;
float rotateVal = 0.0f;
int timeInterval = 33;
bool isRotate = true;
float scaleOffset = 0.65f;


struct Frustum {
	float left, right, top, bottom, cnear, cfar;
};

Matrix4 myViewingMatrix(Vector3 cameraPosition, Vector3 cameraViewDirection, Vector3 cameraUpVector)
{
	Vector3 X, Y, Z;
	Z = (cameraPosition - cameraViewDirection).normalize();
	Y = cameraUpVector.normalize();
	X = Y.cross(Z).normalize();
	cameraUpVector = Z.cross(X);

	Matrix4 mat = Matrix4(
		X.x, X.y, X.z, -X.dot(cameraPosition),
		Y.x, Y.y, Y.z, -Y.dot(cameraPosition),
		Z.x, Z.y, Z.z, -Z.dot(cameraPosition),
		0, 0, 0, 1
	);

	return mat;
}

Matrix4 myOrthogonal(Frustum frustum)
{
	GLfloat tx = -(frustum.right + frustum.left) / (frustum.right - frustum.left);
	GLfloat ty = -(frustum.top + frustum.bottom) / (frustum.top - frustum.bottom);
	GLfloat tz = -(frustum.cfar + frustum.cnear) / (frustum.cfar - frustum.cnear);
	Matrix4 mat = Matrix4(
		2 / (frustum.right - frustum.left), 0, 0, tx,
		0, 2 / (frustum.top - frustum.bottom), 0, ty,
		0, 0, -2 / (frustum.cfar - frustum.cnear), tz,
		0, 0, 0, 1
	);

	return mat;
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
	mat[0] = cosZ*cosY;
	mat[1] = -sinZ*cosX + cosZ*sinY*sinX;
	mat[2] = sinZ*sinX + cosZ*sinY*cosX;
	mat[3] = 0;
	mat[4] = sinZ*cosY;
	mat[5] = cosZ*cosX + sinZ*sinY*sinX;
	mat[6] = -cosZ*sinX + sinZ*sinY*cosX;
	mat[7] = 0;
	mat[8] = -sinY;
	mat[9] = cosY*sinX;
	mat[11] = 0;
	mat[12] = 0;
	mat[13] = 0;
	mat[14] = 0;
	mat[15] = 1;
	return mat;
}

void traverseColorModel(Model &m)
{
	m.numGroups = m.obj->numgroups;
	m.group = new Group[m.numGroups];
	GLMgroup* group = m.obj->groups;

	GLfloat maxVal[3] = {-100000, -100000, -100000 };
	GLfloat minVal[3] = { 100000, 100000, 100000 };

	int curGroupIdx = 0;

	// Iterate all the groups of this model
	while (group)
	{
		m.group[curGroupIdx].vertices = new GLfloat[group->numtriangles * 9];
		m.group[curGroupIdx].normals = new GLfloat[group->numtriangles * 9];
		m.group[curGroupIdx].numTriangles = group->numtriangles;

		// Fetch material information
		memcpy(m.group[curGroupIdx].ambient, m.obj->materials[group->material].ambient, sizeof(GLfloat) * 4);
		memcpy(m.group[curGroupIdx].diffuse, m.obj->materials[group->material].diffuse, sizeof(GLfloat) * 4);
		memcpy(m.group[curGroupIdx].specular, m.obj->materials[group->material].specular, sizeof(GLfloat) * 4);

		m.group[curGroupIdx].shininess = m.obj->materials[group->material].shininess;

		// For each triangle in this group
		for (int i = 0; i < group->numtriangles; i++)
		{
			int triangleIdx = group->triangles[i];
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

			// For each vertex in this triangle
			for (int j = 0; j < 3; j++)
			{
				m.group[curGroupIdx].vertices[i * 9 + j * 3 + 0] = m.obj->vertices[indv[j] * 3 + 0];
				m.group[curGroupIdx].vertices[i * 9 + j * 3 + 1] = m.obj->vertices[indv[j] * 3 + 1];
				m.group[curGroupIdx].vertices[i * 9 + j * 3 + 2] = m.obj->vertices[indv[j] * 3 + 2];
				m.group[curGroupIdx].normals[i * 9 + j * 3 + 0] = m.obj->normals[indn[j] * 3 + 0];
				m.group[curGroupIdx].normals[i * 9 + j * 3 + 1] = m.obj->normals[indn[j] * 3 + 1];
				m.group[curGroupIdx].normals[i * 9 + j * 3 + 2] = m.obj->normals[indn[j] * 3 + 2];

				maxVal[0] = max(maxVal[0], m.group[curGroupIdx].vertices[i * 9 + j * 3 + 0]);
				maxVal[1] = max(maxVal[1], m.group[curGroupIdx].vertices[i * 9 + j * 3 + 1]);
				maxVal[2] = max(maxVal[2], m.group[curGroupIdx].vertices[i * 9 + j * 3 + 2]);

				minVal[0] = min(minVal[0], m.group[curGroupIdx].vertices[i * 9 + j * 3 + 0]);
				minVal[1] = min(minVal[1], m.group[curGroupIdx].vertices[i * 9 + j * 3 + 1]);
				minVal[2] = min(minVal[2], m.group[curGroupIdx].vertices[i * 9 + j * 3 + 2]);
			}
		}
		group = group->next;
		curGroupIdx++;
	}

	// Normalize the model
	float norm_scale = max(max(abs(maxVal[0] - minVal[0]), abs(maxVal[1] - minVal[1])), abs(maxVal[2] - minVal[2]));
	Matrix4 S, T;

	S[0] = 2 / norm_scale*scaleOffset;	S[5] = 2 / norm_scale*scaleOffset;	S[10] = 2 / norm_scale*scaleOffset;
	T[3] = -(maxVal[0] + minVal[0]) / 2;
	T[7] = -(maxVal[1] + minVal[1]) / 2;
	T[11] = -(maxVal[2] + minVal[2]) / 2;
	m.N = S*T;
}

void loadOBJModel()
{
	models = new Model[filenames.size()];
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

void drawModel(Model &m)
{
	int groupNum = m.numGroups;

	glBindVertexArray(vaoHandle);

	for (int i = 0; i < groupNum; i++)
	{
		Matrix4 MVP = P*V*R*m.N;

		glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*m.group[i].numTriangles*9, m.group[i].vertices, GL_DYNAMIC_DRAW);

		glUniformMatrix4fv(iLocN, 1, GL_FALSE, MVP.getTranspose());

		float fakeColor[] = {
			0.5f, 0.8f, 0.3f, 1.0f
		};

		glUniform4fv(iLocLightInfo[0].position, 1, fakeColor);
		glDrawArrays(GL_TRIANGLES, 0, m.group[i].numTriangles * 3);
	}
}

void drawTriangle()
{
	glBindVertexArray(vaoHandle);

	float fakeColor[] = {
		0.5f, 0.8f, 0.3f, 1.0f
	};

	Matrix4 MVP = P*V;
	glUniformMatrix4fv(iLocN, 1, GL_FALSE, MVP.getTranspose());
	glUniform4fv(iLocLightInfo[0].position, 1, fakeColor);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void timerFunc(int timer_value)
{
	if(isRotate)
		rotateVal += 0.1f;

	R = myRotateMatrix(0.0f, rotateVal, 0.0f);

	glutPostRedisplay();
	glutTimerFunc(timeInterval, timerFunc, 0);
}

void onDisplay(void)
{
	// clear canvas
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// clear canvas to color(0,0,0)->black
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//drawTriangle();
	drawModel(models[cur_idx]);

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

void setVertexArrayObject()
{
	// Create and setup the vertex array object
	glGenVertexArrays(1, &vaoHandle);
	glBindVertexArray(vaoHandle);

	// Enable the vertex attribute arrays
	glEnableVertexAttribArray(0);	// Vertex position
	glEnableVertexAttribArray(1);	// Vertex color

	// Map index 0 to the position buffer
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// Map index 1 to the color buffer
	glBindBuffer(GL_ARRAY_BUFFER, colorBufferHandle);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

void setVertexBufferObjects()
{
	// Triangles Data
	float positionData[] = {
		-0.8f, -0.8f, 0.0f,
		0.8f, -0.8f, 0.0f,
		0.0f, 0.8f, 0.0f };

	float colorData[] = {
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f };

	glGenBuffers(2, vboHandles);

	positionBufferHandle = vboHandles[0];
	colorBufferHandle = vboHandles[1];

	// Populate the position buffer
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9, positionData, GL_STATIC_DRAW);

	// Populate the color buffer
	glBindBuffer(GL_ARRAY_BUFFER, colorBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9, colorData, GL_STATIC_DRAW);
}

void setUniformVariables(GLuint p)
{
	iLocP = glGetUniformLocation(p, "um4p");
	iLocV = glGetUniformLocation(p, "um4v");
	iLocN = glGetUniformLocation(p, "um4n");

	iLocLightInfo[0].position = glGetUniformLocation(p, "light[0].Position");
	iLocLightInfo[0].ambient = glGetUniformLocation(p, "light[0].La");
	iLocLightInfo[0].diffuse = glGetUniformLocation(p, "light[0].Ld");
	iLocLightInfo[0].specular = glGetUniformLocation(p, "light[0].Ls");

	iLocLightInfo[1].position = glGetUniformLocation(p, "light[1].Position");
	iLocLightInfo[1].ambient = glGetUniformLocation(p, "light[1].La");
	iLocLightInfo[1].diffuse = glGetUniformLocation(p, "light[1].Ld");
	iLocLightInfo[1].specular = glGetUniformLocation(p, "light[1].Ls");

	iLocLightInfo[2].position = glGetUniformLocation(p, "light[2].Position");
	iLocLightInfo[2].ambient = glGetUniformLocation(p, "light[2].La");
	iLocLightInfo[2].diffuse = glGetUniformLocation(p, "light[2].Ld");
	iLocLightInfo[2].specular = glGetUniformLocation(p, "light[2].Ls");
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

	setUniformVariables(p);
	setVertexBufferObjects();
	setVertexArrayObject();
}

void onMouse(int who, int state, int x, int y)
{
	printf("%18s(): (%d, %d) ", __FUNCTION__, x, y);

	switch (who)
	{
		case GLUT_LEFT_BUTTON:
			current_x = x;
			current_y = y;
			break;
		case GLUT_MIDDLE_BUTTON: printf("middle button "); break;
		case GLUT_RIGHT_BUTTON:
			current_x = x;
			current_y = y;
			break;
		case GLUT_WHEEL_UP:
			printf("wheel up      \n");
			break;
		case GLUT_WHEEL_DOWN:
			printf("wheel down    \n");
			break;
		default:                 
			printf("0x%02X          ", who); break;
	}

	switch (state)
	{
		case GLUT_DOWN: printf("start "); break;
		case GLUT_UP:   printf("end   "); break;
	}

	printf("\n");
}

void onMouseMotion(int x, int y)
{
	int diff_x = x - current_x;
	int diff_y = y - current_y;
	current_x = x;
	current_y = y;

	printf("%18s(): (%d, %d) mouse move\n", __FUNCTION__, x, y);
}

void onKeyboard(unsigned char key, int x, int y)
{
	printf("%18s(): (%d, %d) key: %c(0x%02X) ", __FUNCTION__, x, y, key, key);
	switch (key)
	{
	case GLUT_KEY_ESC: /* the Esc key */
		exit(0);
		break;
	case 'z':
		cur_idx = (cur_idx + filenames.size() - 1) % filenames.size();
		break;
	case 'x':
		cur_idx = (cur_idx + 1) % filenames.size();
		break;
	case 'w':
		use_wire_mode = !use_wire_mode;
		if (use_wire_mode)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case 'g':
		break;
	case 'v':
		break;
	case 'o':
		break;
	case 'p':
		break;
	case 'e':
		break;
	case 'c':
		break;
	case 'u':
		break;
	case 'i':
		break;
	case 't':
		break;
	case 's':
		break;
	case 'r':
		isRotate = !isRotate;
		break;
	}
	printf("\n");
}

void onKeyboardSpecial(int key, int x, int y) {
	printf("%18s(): (%d, %d) ", __FUNCTION__, x, y);
	switch (key)
	{
	case GLUT_KEY_LEFT:
		printf("key: LEFT ARROW");
		break;

	case GLUT_KEY_RIGHT:
		printf("key: RIGHT ARROW");
		break;

	default:
		printf("key: 0x%02X      ", key);
		break;
	}
	printf("\n");
}

void onWindowReshape(int width, int height)
{
	printf("%18s(): %dx%d\n", __FUNCTION__, width, height);
}

void initParameter()
{
	Frustum frustum;
	Vector3 eyePosition = Vector3(0.0f, 0.0f, 2.0f);
	Vector3 centerPosition = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 upVector = Vector3(0.0f, 1.0f, 0.0f);

	frustum.left = -1.0f;	frustum.right = 1.0f;
	frustum.top = 1.0f;	frustum.bottom = -1.0f;
	frustum.cnear = 0.0f;	frustum.cfar = 5.0f;

	V = myViewingMatrix(eyePosition, centerPosition, upVector);
	P = myOrthogonal(frustum);
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

int main(int argc, char **argv)
{
	loadConfigFile();

	// glut init
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

	// create window
	glutInitWindowPosition(500, 100);
	glutInitWindowSize(800, 800);
	glutCreateWindow("CG HW1");

	glewInit();
	if (glewIsSupported("GL_VERSION_4_3")) {
		printf("Ready for OpenGL 4.3\n");
	}
	else {
		printf("OpenGL 4.3 not supported\n");
		system("pause");
		exit(1);
	}

	initParameter();

	// load obj models through glm
	loadOBJModel();

	// register glut callback functions
	glutDisplayFunc(onDisplay);
	glutIdleFunc(onIdle);
	glutKeyboardFunc(onKeyboard);
	glutSpecialFunc(onKeyboardSpecial);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMouseMotion);
	glutReshapeFunc(onWindowReshape);
	glutTimerFunc(timeInterval, timerFunc, 0);

	// set up shaders here
	setShaders();

	glEnable(GL_DEPTH_TEST);

	// main loop
	glutMainLoop();

	// delete glm objects before exit
	for (int i = 0; i < filenames.size(); i++)
	{
		glmDelete(models[i].obj);
	}

	return 0;
}

