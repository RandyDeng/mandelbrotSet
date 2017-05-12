// Calculate and display the Mandelbrot set
// Randy Deng
// ECE4122 Final Project Spring 2017

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <GL/glut.h>
#include <GL/glext.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "complex.h"

#include <vector>

using namespace std;

// Min and max complex plane values
Complex  minC(-2.0, -1.2);
Complex  maxC( 1.0, 1.8);
int      maxIt = 2000;     // Max iterations for the set computations

// global variables
const int totalThreads = 16;
const int size = 512;
const int numRows = size / totalThreads;
double updateRate = 1;

int activeThreads = 0;
pthread_mutex_t activeMutex;
pthread_cond_t allDoneCondition;
int countArr[size][size];

int mouseState = 0;
vector<Complex> history;

double realInc = (maxC.real - minC.real) / size;
double imagInc = (maxC.imag - minC.imag) / size;

// convert cartesian to complex
void cRange(double &x, double &y) {
	x = x / (double) size * (maxC.real - minC.real) + minC.real;
	y = y / (double) size * (maxC.imag - minC.imag) + minC.imag;
}

// create threads to render MBSet
void* displayThread(void* v) {
	// initialize useful variables
	unsigned long threadNum = (unsigned long) v;
	int start = threadNum * numRows;
	int end = start + numRows;
	// compute each pixel
	for (int i = 0; i < size; i++) {
		for (int j = start; j < end; j++) {
			// starting pixel
			double t1 = i;
			double t2 = j;
			cRange(t1, t2);
			Complex c(t1, t2);
			Complex z0(t1, t2);
			int count = 0;
			while (count < maxIt) {
				z0 = z0 * z0 + c;
				if (z0.Mag().real > 2.0) break;
				count++;
			}
			countArr[i][j] = count;
		}
	}
	activeThreads--;
	if (activeThreads == 0) pthread_cond_signal(&allDoneCondition);
	return 0;
}

// Your OpenGL display code here
void display(void) {
	// compute count array values using threads
	pthread_mutex_init(&activeMutex, 0);
	pthread_cond_init(&allDoneCondition, 0);
	pthread_mutex_lock(&activeMutex);
	activeThreads = totalThreads;
	for (int i = 0; i < totalThreads; ++i) {
		pthread_t t;
		pthread_create(&t, 0, displayThread, (void*)i);
	}
	pthread_cond_wait(&allDoneCondition, &activeMutex);
	pthread_mutex_unlock(&activeMutex);
	// gl init
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_POINTS);
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (countArr[i][j] >= maxIt) glColor3f(0, 0, 0);
			else glColor3f(countArr[i][j]/20.,
				countArr[i][j]/50., countArr[i][j]/30.);
			glVertex2i(i, j);
		}
	}
	glEnd();
	glutSwapBuffers();
}

// Your OpenGL initialization code here
void init() {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, 512.0, 512.0, 0.0);
}

// Your OpenGL window reshape code here
void reshape(int w, int h) {}

// timer
void timer(int) {
    glutPostRedisplay();
    glutTimerFunc(1000.0 / updateRate, timer, 0);
}

// Your mouse click processing here
// state == 0 means pressed, state != 0 means released
// Note that the x and y coordinates passed in are in
// PIXELS, with y = 0 at the top.
double beforex, beforey, afterx, aftery = 0;
void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		// update data when mouse is clicked
		mouseState = 1;
		beforex = x;
		beforey = y;
	} else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
		mouseState = 0;
		afterx = x;
		aftery = y;
		// once mouse is let go, update minC and maxC
		cRange(beforex, beforey);
		cRange(afterx, aftery);
		// make the selection square
		if (beforex > afterx) {
			double temp = beforex;
			beforex = afterx;
			afterx = temp;
		}
		if (beforey > aftery) {
			double temp = beforey;
			beforey = aftery;
			aftery = temp;
		}
		if (afterx - beforex > beforey - aftery) {
			aftery = beforey + afterx - beforex;
		} else {
			afterx = beforex + beforey - aftery;
		}
		// redisplay and store history
		history.push_back(minC);
		history.push_back(maxC);
		minC = Complex(beforex, beforey);
		maxC = Complex(afterx, aftery);
		glutPostRedisplay();
	}
}

// Your mouse motion here, x and y coordinates are as above
void motion(int x, int y) {
	/*
	// update mouse end coordinates
	if (mouseState == 1) {
		afterx = x;
		aftery = y;
		// draw red rectangle
		glBegin(GL_LINES);
			glClear(GL_COLOR_BUFFER_BIT);
			glLineWidth(5);
			glColor3f(0, 0, 1);
			glVertex2f(beforex, beforey);
			glVertex2f(afterx, beforey);

			glVertex2f(afterx, aftery);
			glVertex2f(beforex, aftery);

			glVertex2f(beforex, beforey);
			glVertex2f(beforex, aftery);

			glVertex2f(afterx, beforey);
			glVertex2f(afterx, aftery);
		glEnd();
		glutSwapBuffers();
		glutPostRedisplay();
	}
	*/
}

// Your keyboard processing here
void keyboard(unsigned char c, int x, int y) {
	switch (c) {
		// exit program with escape
		case 27 :
			exit(0);
			break;
		// zoom out
		case 'b' :
			if (history.size() != 0) {
				maxC = history.back();
				history.pop_back();
				minC = history.back();
				history.pop_back();
				glutPostRedisplay();
			}
			break;
		// revert to original size;
		case 'z' :
			minC = Complex(-2.0, -1.2);
			maxC = Complex(1.0, 1.8);
			history.clear();
			glutPostRedisplay();
			break;
	}
}

// Initialize OpenGL, but only on the "master" thread or process.
// See the assignment writeup to determine which is "master" 
// and which is slave.
int main(int argc, char** argv) {
  // Initialize glut and create your window here
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(size, size);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("MBSet");
	init();
		// Set your glut callbacks here
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMotionFunc(motion);
	glutMouseFunc(mouse);
	glutTimerFunc(1000.0 / updateRate, timer, 0);
	// main loop
	glutMainLoop();
  return 0;
}

