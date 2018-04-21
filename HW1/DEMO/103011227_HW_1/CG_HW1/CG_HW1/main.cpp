#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

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
GLint iLocColor;
GLint iLocFilter;
GLint iLocMVP;

struct model
{
	GLMmodel *obj;
	GLfloat *vertices;
	GLfloat *colors;
};

model* models;	// store the models we load
vector<string> filenames; // .obj filename list
int cur_idx = 0; // represent which model should be rendered now
int color_mode = 0;
bool use_wire_mode = false;


//base
Matrix4 FloorN = Matrix4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1); // normalization matrix for the floor

//M=T*S*R*N
//T = Model translation
Matrix4 T = Matrix4(
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1); 
//prev is neccessary. Otherwise, the model will rotate too fast
Matrix4 T_prev = T;
//S=scaling rotation
Matrix4 S = Matrix4(
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1);
Matrix4 S_prev = S;

//R=Model rotation
Matrix4 R = Matrix4(
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1);
Matrix4 R_prev = R;

Matrix4 N = Matrix4(
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
);

Vector3 eye_pos = Vector3(0, 0, 2);
Vector3 eye_pos_prev = eye_pos;

Vector3 center_pos = Vector3(0, 0, 0);
Vector3 center_pos_prev = center_pos;

Vector3 up_vec = Vector3(0, 1, 0);

float xmin = -1.0, xmax = 1.0, ymin = -1.0, ymax = 1.0, znear = 1.0, zfar = 3.0; // zfar\znear should be positive
float xmin0 = -1.0, xmax0 = 1.0, ymin0 = -1.0, ymax0 = 1.0, znear0 = 1.0, zfar0 = 3.0;

int mouseX=0;
int mouseY=0;

bool isLeftMousePress = false;

typedef enum {
	object_translation,
	object_rotation,
	object_scaling,
	change_eye_pos,
	change_center_pos,	
}instruction_mode;
instruction_mode Intruction_Mode;

//define orthogonal(parallel) and perspective
typedef enum {
	orthogonal_view,
	perspective_view
}projetion_mode;
projetion_mode project_Mode;



int current_instruction_mode=object_rotation; //defaul translation
int current_projection_mode = perspective_view;

Matrix4 getOrthogonalNormalizationMatrix() {
	Matrix4 mat = Matrix4(
		2.0 / (xmax - xmin), 0, 0, -(xmax + xmin) / (xmax - xmin),
		0, 2.0 / (ymax - ymin), 0, -(ymax + ymin) / (ymax - ymin),
		0, 0, 2.0 / (znear - zfar), -(zfar + znear) / (zfar - znear),
		0, 0, 0, 1.0

	);

	return mat;


}

Matrix4 getPerspectiveNormalizeionMatrix() {
	return Matrix4(
		2.0*znear / (xmax - xmin), 0, (xmax + xmin) / (xmin - xmax), 0,
		0, 2.0*znear / (ymax - ymin), (ymax + ymin) / (ymin - ymax), 0,
		0, 0, (zfar + znear) / (znear - zfar), (2.0*zfar*znear) / (znear - zfar),
		0, 0, -1, 0
	);
}



// [check]
void traverseColorModel(model &m)
{
	GLfloat maxVal[3];
	GLfloat minVal[3];

	m.vertices = new GLfloat[m.obj->numtriangles * 9];
	m.colors = new GLfloat[m.obj->numtriangles * 9];

	// The center position of the model 
	m.obj->position[0] = 0;
	m.obj->position[1] = 0;
	m.obj->position[2] = 0;

	//printf("#triangles: %d\n", m.obj->numtriangles);

	for (int i = 0; i < (int)m.obj->numtriangles; i++)
	{
		// the index of each vertex
		int indv1 = m.obj->triangles[i].vindices[0];
		int indv2 = m.obj->triangles[i].vindices[1];
		int indv3 = m.obj->triangles[i].vindices[2];

		// the index of each color
		int indc1 = indv1;
		int indc2 = indv2;
		int indc3 = indv3;

		// assign vertices
		GLfloat vx, vy, vz;
		vx = m.obj->vertices[indv1 * 3 + 0];
		vy = m.obj->vertices[indv1 * 3 + 1];
		vz = m.obj->vertices[indv1 * 3 + 2];

		m.vertices[i * 9 + 0] = vx;
		m.vertices[i * 9 + 1] = vy;
		m.vertices[i * 9 + 2] = vz;

		vx = m.obj->vertices[indv2 * 3 + 0];
		vy = m.obj->vertices[indv2 * 3 + 1];
		vz = m.obj->vertices[indv2 * 3 + 2];

		m.vertices[i * 9 + 3] = vx;
		m.vertices[i * 9 + 4] = vy;
		m.vertices[i * 9 + 5] = vz;

		vx = m.obj->vertices[indv3 * 3 + 0];
		vy = m.obj->vertices[indv3 * 3 + 1];
		vz = m.obj->vertices[indv3 * 3 + 2];

		m.vertices[i * 9 + 6] = vx;
		m.vertices[i * 9 + 7] = vy;
		m.vertices[i * 9 + 8] = vz;

		// assign colors
		GLfloat c1, c2, c3;
		c1 = m.obj->colors[indv1 * 3 + 0];
		c2 = m.obj->colors[indv1 * 3 + 1];
		c3 = m.obj->colors[indv1 * 3 + 2];

		m.colors[i * 9 + 0] = c1;
		m.colors[i * 9 + 1] = c2;
		m.colors[i * 9 + 2] = c3;

		c1 = m.obj->colors[indv2 * 3 + 0];
		c2 = m.obj->colors[indv2 * 3 + 1];
		c3 = m.obj->colors[indv2 * 3 + 2];

		m.colors[i * 9 + 3] = c1;
		m.colors[i * 9 + 4] = c2;
		m.colors[i * 9 + 5] = c3;

		c1 = m.obj->colors[indv3 * 3 + 0];
		c2 = m.obj->colors[indv3 * 3 + 1];
		c3 = m.obj->colors[indv3 * 3 + 2];

		m.colors[i * 9 + 6] = c1;
		m.colors[i * 9 + 7] = c2;
		m.colors[i * 9 + 8] = c3;
	}

	// Find min and max value
	GLfloat meanVal[3];

	meanVal[0] = meanVal[1] = meanVal[2] = 0;
	maxVal[0] = maxVal[1] = maxVal[2] = -10e20;
	minVal[0] = minVal[1] = minVal[2] = 10e20;

	for (int i = 0; i < m.obj->numtriangles * 3; i++)
	{
		maxVal[0] = max(m.vertices[3 * i + 0], maxVal[0]);
		maxVal[1] = max(m.vertices[3 * i + 1], maxVal[1]);
		maxVal[2] = max(m.vertices[3 * i + 2], maxVal[2]);

		minVal[0] = min(m.vertices[3 * i + 0], minVal[0]);
		minVal[1] = min(m.vertices[3 * i + 1], minVal[1]);
		minVal[2] = min(m.vertices[3 * i + 2], minVal[2]);
	}
	GLfloat scale = max(maxVal[0] - minVal[0], maxVal[1] - minVal[1]);
	scale = max(scale, maxVal[2] - minVal[2]);

	// Calculate mean values
	for (int i = 0; i < 3; i++)
	{
		meanVal[i] = (maxVal[i] + minVal[i]) / 2.0;
	}

	// Normalization
	for (int i = 0; i < m.obj->numtriangles * 3; i++)
	{
		m.vertices[3 * i + 0] = 2.0*((m.vertices[3 * i + 0] - meanVal[0]) / scale);
		m.vertices[3 * i + 1] = 2.0*((m.vertices[3 * i + 1] - meanVal[1]) / scale);
		m.vertices[3 * i + 2] = 2.0*((m.vertices[3 * i + 2] - meanVal[2]) / scale);
	}
}

// [check]
void loadOBJModel()
{
	models = new model[filenames.size()];
	int idx = 0;
	for (string filename : filenames)
	{
		models[idx].obj = glmReadOBJ((char*)filename.c_str());
		traverseColorModel(models[idx++]);
	}
}

// [check]
void setColorMode()
{
	if (color_mode == 0)
	{
		glUniform3f(iLocFilter, 1.0, 1.0, 1.0);
	}
	else if (color_mode == 1)
	{
		glUniform3f(iLocFilter, 1.0, 0.0, 0.0);
	}
	else if (color_mode == 2)
	{
		glUniform3f(iLocFilter, 0.0, 1.0, 0.0);
	}
	else if (color_mode == 3)
	{
		glUniform3f(iLocFilter, 0.0, 0.0, 1.0);
	}
}

void onIdle()
{
	glutPostRedisplay();
}

Matrix4 getViewingMatrixTransform() {


	//viewing matrix transform
	Vector3 P1P2 = center_pos - eye_pos;
	Vector3 P1P3 = up_vec;
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
		1, 0, 0, -eye_pos[0],
		0, 1, 0, -eye_pos[1],
		0, 0, 1, -eye_pos[2],
		0, 0, 0, 1
	);

	return V_rotation*V_translate;

}

// [check]
//display function
void onDisplay(void)
{


	// clear canvas
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// clear canvas to color(0,0,0)->black
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnableVertexAttribArray(iLocPosition);
	glEnableVertexAttribArray(iLocColor);
	//M=T*S*R*N defined in the global 
	Matrix4 M = T*S*R*N;

	Matrix4 V = Matrix4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);

	V = getViewingMatrixTransform();


	Matrix4 P= Matrix4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, -1, 0,
		0, 0, 0, 1);
	
	if (current_projection_mode == orthogonal_view) {
		P = getOrthogonalNormalizationMatrix();

	}
	else
	{
		P = getPerspectiveNormalizeionMatrix();
	}





	Matrix4 MVP = P*V*M;
	//std::cout << MVP << std::endl;

	GLfloat mvp[16];


	// row-major ---> column-major
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];



	// assign corresponding color filter to fragment shader (shader.frag)
	setColorMode();

	// bind array pointers to shader    3->stride              0->start_index
	glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, models[cur_idx].vertices);
	glVertexAttribPointer(iLocColor, 3, GL_FLOAT, GL_FALSE, 0, models[cur_idx].colors);
	
	// bind uniform matrix to shader
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);

	// draw the array we just bound
	glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].obj->numtriangles * 3);

	





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

	iLocPosition = glGetAttribLocation(p, "av4position");
	iLocColor = glGetAttribLocation(p, "av3color");
	iLocMVP = glGetUniformLocation(p, "mvp");
	glUseProgram(p);
}

// [check]
//on mouse click 
//also mouse scoll

void onMouse(int who, int state, int x, int y)
{
	printf("%18s(): (%d, %d) ", __FUNCTION__, x, y);

	switch (who)
	{

	case GLUT_LEFT_BUTTON:   printf("left button   "); break;
	case GLUT_MIDDLE_BUTTON: printf("middle button "); break;
	case GLUT_RIGHT_BUTTON:  printf("right button  "); break;
	case GLUT_WHEEL_UP:      printf("wheel up      ");
		//scaling
		if (current_instruction_mode == object_translation) {
			//move the model close to screen
			T=T_prev= Matrix4(
				1,0,0,0,
				0,1,0,0,
				0,0,1,1/50.0,
				0,0,0,1
			)*T_prev;
		}
		else if (current_instruction_mode==object_rotation)
		{
			//move the model (counter)clockwise
			R =R_prev= Matrix4(
				cos(1/20.0), -sin(1/20.0), 0, 0,
				sin(1/20.0), cos(1/20.0), 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1)*R_prev;
		}
		else if(current_instruction_mode==object_scaling)
		{
			S=S_prev = Matrix4(
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1+0.1, 0,
				0, 0, 0, 1
			)*S_prev;
		}
		else if (current_instruction_mode==change_eye_pos) {
			
			eye_pos = Vector3(0, 0, 1.0 / 100.0) + eye_pos_prev;
			eye_pos_prev = eye_pos;

		}
		else if (current_instruction_mode==change_center_pos) {
			center_pos = Vector3(0, 0, 1.0 / 100.0) + center_pos_prev;
			center_pos_prev = center_pos;

		}
		

		break;
	case GLUT_WHEEL_DOWN:    printf("wheel down    ");
		if (current_instruction_mode == object_translation) {
			//move the model out to screen
			T =T_prev= Matrix4(
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, -1 / 50.0,
				0, 0, 0, 1
			)*T_prev;
		}
		else if (current_instruction_mode == object_rotation)
		{
			//move the model (counter)clockwise
			R =R_prev= Matrix4(
				cos(-1 / 20.0), -sin(-1 / 20.0), 0, 0,
				sin(-1 / 20.0), cos(-1 / 20.0), 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1)*R_prev;
		}
		else if (current_instruction_mode == object_scaling)
		{
			S=S_prev = Matrix4(
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1 - 0.1, 0,
				0, 0, 0, 1
			)*S_prev;
		}
		else if (current_instruction_mode==change_eye_pos) {
			eye_pos = Vector3(0, 0, -1.0 / 100.0) + eye_pos_prev;
			eye_pos_prev = eye_pos;

		}
		else if(current_instruction_mode==change_center_pos)
		{
			center_pos = Vector3(0, 0, -1.0 / 100.0) + center_pos_prev;
			center_pos_prev = center_pos;
		}




		break;

	default:                 printf("0x%02X          ", who); break;
	}

	switch (state)
	{
	case GLUT_DOWN: printf("start ");
		mouseX = x;
		mouseY = y;
		if (who == GLUT_LEFT_BUTTON) {
			isLeftMousePress = true;
		}
		else
		{
			isLeftMousePress = false;
		}
		break;
	case GLUT_UP:   printf("end   ");
	 	T_prev = T;
		S_prev = S;
		R_prev = R;
		eye_pos_prev = eye_pos;
		center_pos_prev = center_pos;
		xmax0 = xmax;
		xmin0 = xmin;
		ymax0 = ymax;
		ymin0 = ymin;
		zfar0 = zfar;
		znear0 = znear;
		
		
		break;
	}

	printf("\n");
}

// [check]
// mouse movement 
void onMouseMotion(int x, int y)
{
	printf("%18s(): (%d, %d) mouse move\n", __FUNCTION__, x, y);
	float offset_X = (float)(x - mouseX) * 2 / 800; // windows width and height is 800
	float offset_Y = (float)(y - mouseY) * 2 / 800;

	if (current_instruction_mode == object_translation) {
		//model translation

		T = Matrix4(
			1, 0, 0, offset_X,
			0, 1, 0, -offset_Y,// -y is more human friendly
			0, 0, 1, 0,
			0, 0, 0, 1
		)*T_prev;
	}
	else if (current_instruction_mode==object_rotation)
	{
		Matrix4 RX = Matrix4(
			1, 0, 0, 0,
			0, cos(offset_Y), -sin(offset_Y), 0,
			0, sin(offset_Y), cos(offset_Y), 0,
			0, 0, 0, 1
		);
		Matrix4 RY = Matrix4(
			cos(offset_X), 0, sin(offset_X), 0,
			0, 1, 0, 0,
			-sin(offset_X), 0, cos(offset_X), 0,
			0, 0, 0, 1
		);
		R = RX*RY*R_prev;


	}
	else if (current_instruction_mode==object_scaling)
	{
		S = Matrix4(
			1 + offset_X, 0, 0, 0,
			0, 1 + offset_Y, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		)*S_prev;
	}
	else if (current_instruction_mode == change_eye_pos) {
		eye_pos = Vector3(offset_X, offset_Y, 0) + eye_pos_prev;

	}
	else if(current_instruction_mode==change_center_pos)
	{
		center_pos = Vector3(offset_X, offset_Y, 0) + center_pos_prev;
	}







}

// [check]
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
	case 'c':
		color_mode = (color_mode + 1) % 4;
		break;
	case 't':
		std::cout << "T press" << std::endl << "Object Translation Mode" << std::endl;
		current_instruction_mode = object_translation;
		break;
	case 'r':
		std::cout << "R press" << std::endl << "Object Rotation Mode" << std::endl;
		current_instruction_mode = object_rotation;
		break;
	case 's':
		std::cout << "S press" << std::endl << "Object Scaling Mode" << std::endl;
		current_instruction_mode = object_scaling;
		break;
	case 'e':
		std::cout << "E press" << std::endl << "Change Eye Position" << std::endl;
		current_instruction_mode = change_eye_pos;
		break;
	case 'l':
		std::cout << "L press" << std::endl << "Change center(look at) Position" << std::endl;
		current_instruction_mode = change_center_pos;
		break;
	case 'o':
		std::cout << "O press" << std::endl << "Change to parallel projection " << std::endl;
		current_projection_mode = orthogonal_view;
		break;
	case 'p':
		std::cout << "O press" << std::endl << "Change to perspective projection " << std::endl;
		current_projection_mode = perspective_view;
		break;

	}
	printf("\n");
}

// [check]
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

// [check]
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

// [check]
int main(int argc, char **argv)
{
	loadConfigFile();

	// glut init
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

	// create window
	glutInitWindowPosition(500, 100);
	glutInitWindowSize(800, 800);
	glutCreateWindow("10420 CS550000 CG HW1 TA");

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
	glutReshapeFunc(onWindowReshape);

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

