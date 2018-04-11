//
//  main.cpp
//  OpenGLonMac
//
//  Created by 周延儒 on 2018/4/11.
//  Copyright © 2018 周延儒. All rights reserved.
//

#include <iostream>
#include <GLUT/GLUT.h>

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POLYGON);
    glVertex2f(-0.5, -0.5);
    glVertex2f(-0.5, 0.5);
    glVertex2f(0.5, 0.5);
    glVertex2f(0.5, -0.5);
    glEnd();
    glFlush();
}
int main(int argc, char ** argv)
{
    glutInit(&argc, argv);
    glutCreateWindow("Xcode Glut Demo");
    glutDisplayFunc(display);
    glutMainLoop();
}
