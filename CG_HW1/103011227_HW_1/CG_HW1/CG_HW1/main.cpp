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
GLint iLocPosition;
GLint iLocColor;
GLint iLocMVP;

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
	GLfloat *colors;

	Vector3 position = Vector3(0,0,0);
	Vector3 scale = Vector3(1,1,1);
	Vector3 rotation = Vector3(0,0,0);	// Euler form
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

int current_x, current_y;

model* models;	// store the models we load
vector<string> filenames; // .obj filename list
int cur_idx = 0; // represent which model should be rendered now
bool use_wire_mode = false;

//define operation model
typedef enum {
    object_translation,
    object_rotation,
    object_scaling,
    change_eye_pos,
    change_center_pos,
	change_up_vec
}instruction_mode;
instruction_mode Intruction_Mode;

//define orthogonal(parallel) and perspective
typedef enum {
    orthogonal_view,
    perspective_view
}projetion_mode;
projetion_mode project_Mode;

int current_instruction_mode = object_rotation; //defaul translation
int current_projection_mode = orthogonal_view;



// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;
	
	mat = Matrix4(
		1,0,0,vec[0],
		0,1,0,vec[1],
		0,0,1,vec[2],
		0,0,0,1
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
			0, 0,vec[2], 0,
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
		cos(val),-sin(val),0,0,
		sin(val),cos(val),0,0,
		0,0,1,0,
		0,0,0,1
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
		0, 0, (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip), 2*proj.farClip*proj.nearClip / (proj.nearClip - proj.farClip),
		0, 0, -1, 0
	);
	
}

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

		meanVal[0] += m.vertices[3 * i + 0];
		meanVal[1] += m.vertices[3 * i + 1];
		meanVal[2] += m.vertices[3 * i + 2];
	}
	GLfloat scale = max(maxVal[0] - minVal[0], maxVal[1] - minVal[1]);
	scale = max(scale, maxVal[2] - minVal[2]);

	// Calculate mean values
	for (int i = 0; i < 3; i++)
	{
		//meanVal[i] = (maxVal[i] + minVal[i]) / 2.0;
		meanVal[i] /= (m.obj->numtriangles*3);
	}

	// Normalization
	for (int i = 0; i < m.obj->numtriangles * 3; i++)
	{
		m.vertices[3 * i + 0] = 1.0*((m.vertices[3 * i + 0] - meanVal[0]) / scale);
		m.vertices[3 * i + 1] = 1.0*((m.vertices[3 * i + 1] - meanVal[1]) / scale);
		m.vertices[3 * i + 2] = 1.0*((m.vertices[3 * i + 2] - meanVal[2]) / scale);
	}
}

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

void onIdle()
{
	glutPostRedisplay();
}

void drawModel(model* model)
{
	Matrix4 T, R, S;
	T = translate(model->position);
	R = rotate(model->rotation);
	S = scaling(model->scale);

	// [TODO] Assign MVP correct value
	// [HINT] MVP = projection_matrix * view_matrix * ??? * ??? * ???
	Matrix4 MVP;
	GLfloat mvp[16];

	Matrix4 M = T*S*R;
	//MVP=P*V*M

	

	MVP = project_matrix*view_matrix*M;


	// row-major ---> column-major
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];

	glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, model->vertices);
	glVertexAttribPointer(iLocColor, 3, GL_FLOAT, GL_FALSE, 0, model->colors);

	// assign mvp value to shader's uniform variable at location iLocMVP
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	glDrawArrays(GL_TRIANGLES, 0, model->obj->numtriangles * 3);
}

void drawPlane()
{
	GLfloat vertices[18]{ 1.0, -1.0, -1.0,
						  1.0, -1.0, 1.0,
						 -1.0, -1.0, -1.0,
						  1.0, -1.0, 1.0,
						 -1.0, -1.0, 1.0,
						 -1.0, -1.0, -1.0 };

	GLfloat colors[18]{ 0.0,0.8,0.0,
						0.0,0.5,0.8,
						0.0,0.8,0.0,
						0.0,0.5,0.8,
						0.0,0.5,0.8, 
						0.0,0.8,0.0 };


	Matrix4 MVP = project_matrix*view_matrix;
	GLfloat mvp[16];

	// row-major ---> column-major
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];

	glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, vertices);
	glVertexAttribPointer(iLocColor, 3, GL_FLOAT, GL_FALSE, 0, colors);

	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void onDisplay(void)
{
	// clear canvas
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// clear canvas to color(0,0,0)->black
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawPlane();
	drawModel(&models[cur_idx]);

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

	glEnableVertexAttribArray(iLocPosition);
	glEnableVertexAttribArray(iLocColor);
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
			// [TODO] assign corresponding operation
			
			if (current_instruction_mode==change_eye_pos )
			{
				main_camera.position.z -= 0.025;
				setViewingMatrix();
				printf("Camera Position = ( %f , %f , %f )\n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
			}
			else if (current_instruction_mode==change_center_pos)
			{
				main_camera.center.z += 0.1;
				setViewingMatrix();
				printf("Camera Viewing Direction = ( %f , %f , %f )\n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
			}
			else if (current_instruction_mode== change_up_vec)
			{
				main_camera.up_vector.z += 0.33;
				setViewingMatrix();
				printf("Camera Up Vector = ( %f , %f , %f )\n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
			}
			else if (current_instruction_mode==object_translation)
			{
				models[cur_idx].position.z += 0.1;
			}
			else if (current_instruction_mode==object_scaling)
			{
				models[cur_idx].scale.z += 0.025;
			}
			else if (current_instruction_mode==object_rotation)
			{
				models[cur_idx].rotation.z += (PI/180.0) * 5;
			}
			
			break;
		case GLUT_WHEEL_DOWN:
			printf("wheel down    \n");
			// [TODO] assign corresponding operation
			
			if (current_instruction_mode == change_eye_pos)
			{
				main_camera.position.z += 0.025;
				setViewingMatrix();
				printf("Camera Position = ( %f , %f , %f )\n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
			}
			else if (current_instruction_mode == change_center_pos)
			{
				main_camera.center.z -= 0.33;
				setViewingMatrix();
				printf("Camera Viewing Direction = ( %f , %f , %f )\n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
			}
			else if (current_instruction_mode == change_up_vec)
			{
				main_camera.up_vector.z -= 0.33;
				setViewingMatrix();
				printf("Camera Up Vector = ( %f , %f , %f )\n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
			}
			else if (current_instruction_mode == object_translation)
			{
				models[cur_idx].position.z -= 0.33;
			}
			else if (current_instruction_mode == object_scaling)
			{
				models[cur_idx].scale.z -= 0.025;
			}
			else if (current_instruction_mode == object_rotation)
			{
				models[cur_idx].rotation.z -= (PI / 180.0) * 5;
			}
			
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

	// [TODO] assign corresponding operation
	
	if (current_instruction_mode==change_eye_pos)
	{

		main_camera.position.x += diff_x*(1.0 / 400.0);
		main_camera.position.y += diff_y*(1.0 / 400.0);
		setViewingMatrix();
		printf("Camera Position = ( %f , %f , %f )\n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
	}
	else if (current_instruction_mode== change_center_pos)
	{
		main_camera.center.x += diff_x*(1.0 / 400.0);
		main_camera.center.y += diff_y*(1.0 / 400.0);
		setViewingMatrix();
		printf("Camera Viewing Direction = ( %f , %f , %f )\n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
	}
	else if (current_instruction_mode==change_up_vec)
	{
		main_camera.up_vector.x += diff_x*0.1;
		main_camera.up_vector.y += diff_y*0.1;
		setViewingMatrix();
		printf("Camera Up Vector = ( %f , %f , %f )\n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
	}
	else if (current_instruction_mode==object_translation)
	{
		models[cur_idx].position.x += diff_x*(1.0 / 400.0);
		models[cur_idx].position.y -= diff_y*(1.0 / 400.0);
	}
	else if (current_instruction_mode==object_scaling)
	{
		models[cur_idx].scale.x += diff_x*0.025;
		models[cur_idx].scale.y += diff_y*0.025;
	}
	else if (current_instruction_mode==object_rotation)
	{
		models[cur_idx].rotation.x += PI / 180.0*diff_y*(45.0 / 400.0);
		models[cur_idx].rotation.y += PI / 180.0*diff_x*(45.0 / 400.0);
	}
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
	case 'o':
		setOrthogonal();
		current_projection_mode = orthogonal_view;
		break;
	case 'p':
		setPerspective();
		current_projection_mode = perspective_view;
		break;
	case 'e':
		std::cout << "change camera pos" << std::endl;
		current_instruction_mode = change_eye_pos;
		break;
	case 'c':
		current_instruction_mode = change_center_pos;
		break;
	case 'i':

		/*
		(此model的position、rotation、scaling值，相機之position、center，projection之矩陣模式以及其設定之left、right、up、bottom、nearClip、farClip、fov值)。不用顯示矩陣值
		*/
		std::cout << "All information:" << std::endl;
		std::cout << "model position:"<< models[cur_idx].position << std::endl;
		std::cout << "model rotation:" << models[cur_idx].rotation << std::endl;
		std::cout << "model scaling:" << models[cur_idx].scale << std::endl;
	
		std::cout << "camera position:" <<main_camera.position << std::endl;
		std::cout << "camera center:" << main_camera.center << std::endl;

		if (current_projection_mode==orthogonal_view)
		{
			std::cout << "orthogonal view" << std::endl;
		}
		else {

			std::cout << "perspective veiw" << std::endl;
		}

		std::cout << "left:" << proj.left << std::endl;
		std::cout << "right:" << proj.right << std::endl;
		std::cout << "up:" << proj.top << std::endl;
		std::cout << "bottom:" << proj.bottom << std::endl;
		std::cout << "nearClip:" << proj.nearClip << std::endl;
		std::cout << "farClip:" << proj.farClip << std::endl;
		std::cout << "fov:" << proj.fovy << std::endl;

		//debug
		/*
		std::cout << "projection matrix:" << project_matrix << std::endl;
		std::cout << "view matrix:" << view_matrix << std::endl;
		*/
		



		break;
	case 't':
		std::cout << "T press" << std::endl << "Object Translation Mode" << std::endl;
		current_instruction_mode = object_translation;
		break;
	case 's':
		std::cout << "S press" << std::endl << "Object Scaling Mode" << std::endl;
		current_instruction_mode = object_scaling;
		break;
	case 'r':
		std::cout << "R press" << std::endl << "Object Rotation Mode" << std::endl;
		current_instruction_mode = object_rotation;
		break;
	case 'u':
		std::cout << "U press" << std::endl << "Change Up Vector Mode" << std::endl;
		current_instruction_mode = change_up_vec;

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
	proj.aspect = width / height;

	printf("%18s(): %dx%d\n", __FUNCTION__, width, height);
}

// you can setup your own camera setting when testing
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

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

	// create window
	glutInitWindowPosition(500, 100);
	glutInitWindowSize(800, 800);
	glutCreateWindow("CG HW1");

	glewInit();
	if (glewIsSupported("GL_VERSION_2_0")) {
		printf("Ready for OpenGL 2.0\n");
	}
	else {
		printf("OpenGL 2.0 not supported\n");
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

