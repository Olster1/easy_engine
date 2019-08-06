/*
* Copyright (c) 2010 Erin Catto http://www.gphysics.com
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "glut.h"

#include "Distance2D.h"

namespace
{
	enum
	{
		e_maxPoints = 20
	};

	enum DrawType
	{
		e_drawPoints,
		e_drawPolygon
	};

	Vec2 points1[e_maxPoints];
	Vec2 points2[e_maxPoints];

	Polygon polygon1;
	Polygon polygon2;

	DrawType drawType1;
	DrawType drawType2;

	int demoIndex = 0;

	float angle = 0.0f;
	Vec2 position(2.0f, 0.0f);

	bool drawSimplex;
	int simplexIndex;
}

void DrawText(int x, int y, char* format, ...)
{
	char string[128];

	va_list arg;
	va_start(arg, format);
	vsprintf_s(string, 128, format, arg);
	va_end(arg);
	string[127] = 0;

	int len, i;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	int w = glutGet(GLUT_WINDOW_WIDTH);
	int h = glutGet(GLUT_WINDOW_HEIGHT);
	gluOrtho2D(0, w, h, 0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor3f(0.9f, 0.6f, 0.6f);
	glRasterPos2i(x, y);
	len = (int) strlen(string);
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, string[i]);
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void DrawAsPoints(const Polygon& polygon, const Transform& transform)
{
	glPointSize(10.0f);
	glColor3f(0.9f, 0.9f, 0.9f);
	glBegin(GL_POINTS);
	for (int i = 0; i < polygon.m_count; ++i)
	{
		Vec2 p = Mul(transform, polygon.m_points[i]);
		glVertex2f(p.x, p.y);
	}
	glEnd();
}

void DrawAsPolygon(const Polygon& polygon, const Transform& transform)
{
	glColor3f(0.9f, 0.9f, 0.9f);

	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < polygon.m_count; ++i)
	{
		Vec2 p = Mul(transform, polygon.m_points[i]);
		glVertex2f(p.x, p.y);
	}
	glEnd();
}

void DrawPolygon(const Polygon& polygon, const Transform& transform, DrawType type)
{
	if (type == e_drawPoints)
	{
		DrawAsPoints(polygon, transform);
	}
	else if (type == e_drawPolygon)
	{
		DrawAsPolygon(polygon, transform);
	}
}

// Point vs Line Segment
void Demo1()
{
	points1[0].Set(0.0f, 0.0f);
	polygon1.m_points = points1;
	polygon1.m_count = 1;
	drawType1 = e_drawPoints;

	points2[0].Set(0.0f, -1.0f);
	points2[1].Set(0.0f, 1.0f);
	polygon2.m_points = points2;
	polygon2.m_count = 2;
	drawType2 = e_drawPolygon;
}

// Point vs Triangle
void Demo2()
{
	points1[0].Set(0.0f, 0.0f);
	polygon1.m_points = points1;
	polygon1.m_count = 1;
	drawType1 = e_drawPoints;

	float angle = 0.0f;
	for (int i = 0; i < 3; ++i)
	{
		points2[i].Set(cosf(angle), sinf(angle));
		angle += 2.0f * k_pi / 3.0f;
	}
	polygon2.m_points = points2;
	polygon2.m_count = 3;
	drawType2 = e_drawPolygon;
}

// Point vs Hexagon
void Demo3()
{
	points1[0].Set(0.0f, 0.0f);
	polygon1.m_points = points1;
	polygon1.m_count = 1;
	drawType1 = e_drawPoints;

	float angle = 0.0f;
	for (int i = 0; i < 6; ++i)
	{
		points2[i].Set(cosf(angle), sinf(angle));
		angle += k_pi / 3.0f;
	}
	polygon2.m_points = points2;
	polygon2.m_count = 6;
	drawType2 = e_drawPolygon;
}

// Square vs Line Segment
void Demo4()
{
	points1[0].Set(-1.0f, -1.0f);
	points1[1].Set(1.0f, -1.0f);
	points1[2].Set(1.0f, 1.0f);
	points1[3].Set(-1.0f, 1.0f);
	polygon1.m_points = points1;
	polygon1.m_count = 4;
	drawType1 = e_drawPolygon;

	points2[0].Set(0.0f, -1.0f);
	points2[1].Set(0.0f, 1.0f);
	polygon2.m_points = points2;
	polygon2.m_count = 2;
	drawType2 = e_drawPolygon;
}

// Square vs Triangle
void Demo5()
{
	points1[0].Set(-1.0f, -1.0f);
	points1[1].Set(1.0f, -1.0f);
	points1[2].Set(1.0f, 1.0f);
	points1[3].Set(-1.0f, 1.0f);
	polygon1.m_points = points1;
	polygon1.m_count = 4;
	drawType1 = e_drawPolygon;

	float angle = 0.0f;
	for (int i = 0; i < 3; ++i)
	{
		points2[i].Set(cosf(angle), sinf(angle));
		angle += 2.0f * k_pi / 3.0f;
	}
	polygon2.m_points = points2;
	polygon2.m_count = 3;
	drawType2 = e_drawPolygon;
}

// Point vs Square with collinear vertices
void Demo6()
{
	points1[0].Set(0.0f, 0.0f);
	polygon1.m_points = points1;
	polygon1.m_count = 1;
	drawType1 = e_drawPoints;

	points2[0].Set(-1.0f, -1.0f);
	points2[1].Set(0.0f, -1.0f);
	points2[2].Set(1.0f, -1.0f);
	points2[3].Set(1.0f, 0.0f);
	points2[4].Set(1.0f, 1.0f);
	points2[5].Set(0.0f, 1.0f);
	points2[6].Set(-1.0f, 1.0f);
	points2[7].Set(-1.0f, 0.0f);
	polygon2.m_points = points2;
	polygon2.m_count = 8;
	drawType2 = e_drawPolygon;
}

void (*demos[])() = {Demo1, Demo2, Demo3, Demo4, Demo5, Demo6};
char* demoStrings[] = {
	"Demo 1: Point vs Line Segment",
	"Demo 2: Point vs Triangle",
	"Demo 3: Point vs Hexagon",
	"Demo 4: Square vs Line Segment",
	"Demo 5: Square vs Triangle",
	"Demo 6: Collinear Square vs Triangle"};

void InitDemo(int index)
{
	demoIndex = index;
	demos[index]();
	angle = 0.0f;
	position.Set(2.0f, 0.0f);
}

void SimulationLoop()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	DrawText(5, 15, demoStrings[demoIndex]);
	DrawText(5, 45, "Keys: (1-6) Demos, (ESDF) translate, (WR) rotate, (H) toggle simplex, (IK) simplex up/down");

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -10.0f);

	Input input;
	input.polygon1 = polygon1;
	input.polygon2 = polygon2;
	input.transform1.p.Set(0.0f, 0.0f);
	input.transform1.R.Set(0.0f);
	input.transform2.p = position;
	input.transform2.R.Set(angle);

	DrawPolygon(polygon1, input.transform1, drawType1);
	DrawPolygon(polygon2, input.transform2, drawType2);

	Output output;
	Distance2D(&output, input);

	if (simplexIndex < 0)
	{
		simplexIndex = 0;
	}

	if (simplexIndex >= output.simplexCount)
	{
		simplexIndex = output.simplexCount - 1;
	}

	if (drawSimplex)
	{
		DrawText(5, 60, "Simplex: %d of %d", simplexIndex + 1, output.simplexCount);

		const Simplex& s = output.simplices[simplexIndex];

		Vec2 p1, p2;
		s.GetWitnessPoints(&p1, &p2);

		glPointSize(10.0f);
		glBegin(GL_POINTS);
		glColor3f(0.4f, 0.8f, 0.4f);
		glVertex2f(p1.x, p1.y);
		glColor3f(0.8f, 0.4f, 0.4f);
		glVertex2f(p2.x, p2.y);
		glEnd();

		glColor3f(0.4f, 0.4f, 0.8f);
		glBegin(GL_LINES);
		glVertex2f(p1.x, p1.y);
		glVertex2f(p2.x, p2.y);
		glEnd();

		const SimplexVertex* vertices = &s.m_vertexA;

		for (int i = 0; i < s.m_count; ++i)
		{
			const SimplexVertex* v = vertices + i;

			glColor3f(0.9f, 0.5f, 0.0f);
			glPointSize(10.0f);
			glBegin(GL_POINTS);
			glVertex2f(v->point1.x, v->point1.y);
			glVertex2f(v->point2.x, v->point2.y);
			glEnd();
		}
	}
	else
	{
		Vec2 p1 = output.point1;
		Vec2 p2 = output.point2;

		glPointSize(10.0f);
		glBegin(GL_POINTS);
		glColor3f(0.4f, 0.8f, 0.4f);
		glVertex2f(p1.x, p1.y);
		glColor3f(0.8f, 0.4f, 0.4f);
		glVertex2f(p2.x, p2.y);
		glEnd();

		glColor3f(0.4f, 0.4f, 0.8f);
		glBegin(GL_LINES);
		glVertex2f(p1.x, p1.y);
		glVertex2f(p2.x, p2.y);
		glEnd();
	}

	glutSwapBuffers();
}

void Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		exit(0);
		break;

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
		InitDemo(key - '1');
		simplexIndex = 0;
		break;

	case 'e':
		position.y += 0.05f;
		break;

	case 'd':
		position.y -= 0.05f;
		break;

	case 's':
		position.x -= 0.05f;
		break;

	case 'f':
		position.x += 0.05f;
		break;

	case 'w':
		angle += 0.05f * k_pi;
		break;

	case 'r':
		angle -= 0.05f * k_pi;
		break;

	case 'h':
		drawSimplex = !drawSimplex;
		simplexIndex = 0;
		break;

	case 'i':
		++simplexIndex;
		break;

	case 'k':
		--simplexIndex;
		break;
	}
}

void Reshape(int width, int height)
{
	if (height == 0)
		height = 1;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (float)width/(float)height, 0.1, 100.0);
}

int main(int argc, char** argv)
{
	Vec2 A(0.021119118f, 79.584320f);
	Vec2 B(0.020964622f, -31.515678f);
	Vec2 P(0.0f, 0.0f);

	Vec2 e = B - A;
	float u = Dot(B-P, e) / Dot(e, e);
	float v = Dot(P-A, e) / Dot(e, e);

	Vec2 Q = u * A + v * B;
	Vec2 d = P - Q;

	float shouldBeZero = Dot(d, e);
	printf("Dot(d, e) = %g\n", shouldBeZero);

	InitDemo(0);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(800, 800);
	glutCreateWindow("Distance2D");

	glutReshapeFunc(Reshape);
	glutDisplayFunc(SimulationLoop);
	glutKeyboardFunc(Keyboard);
	glutIdleFunc(SimulationLoop);

	glutMainLoop();

	return 0;
}
