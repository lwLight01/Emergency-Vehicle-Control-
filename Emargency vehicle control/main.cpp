#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>

const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 800;
const float PI = 3.14159265f;

const float MAX_SPEED = 2.5f;
const float ACCELERATION = 0.05f;
const float DECELERATION = 0.08f;
const float STOPPING_DISTANCE = 100.0f;

enum LightState { LIGHT_RED, LIGHT_YELLOW, LIGHT_GREEN };
struct Point
{
    float x, y;
    Point(float x = 0, float y = 0) : x(x), y(y) {}
};
struct Vehicle
{
    float x, y;
    float speed;
    float targetSpeed;
    float wheelRotation;
    bool movingRight;
    float scale;
    float r, g, b;
    float acceleration;
};
bool emergencyMode = false;
bool isPaused = false;
float emergencyVehicleX = -600.0f;
float emergencyVehicleY = -105.0f;
float emergencySpeed = 0.0f;
float emergencyMaxSpeed = 3.5f;

LightState leftLight = LIGHT_GREEN;
LightState rightLight = LIGHT_RED;
float lightTimer = 0.0f;
float lightDuration = 200.0f;
float yellowDuration = 60.0f;

std::vector<Vehicle> vehicles;
float cloudOffset = 0.0f;
float sirenBlink = 0.0f;
int frameCount = 0;

void DDA(float x1, float y1, float x2, float y2);
void MP(int x1, int y1, int x2, int y2);
void CMP(float cx, float cy, float radius);
void FilledC(float cx, float cy, float radius);
Point bezierPoint(Point p0, Point p1, Point p2, Point p3, float t);
void drawBezierCurve(Point p0, Point p1, Point p2, Point p3);
void drawFilledBezier(Point p0, Point p1, Point p2, Point p3, float thickness);
void drawSky();
void drawGround();
void drawSun();
void drawCloud(float cx, float cy, float scale);
void drawBuilding(float x, float y, float width, float height, float r, float g, float b);
void drawCastle(float x, float y);
void drawTree(float x, float y, float scale);
void drawRoad();
void drawTrafficLight(float x, float y, LightState state);
void drawCar(float x, float y, float scale, float r, float g, float b, float wheelRot);
void plotCirclePoints(float cx, float cy, int x, int y);
void drawEmergencyVehicle(float x, float y, float scale);
void drawText(float x, float y, const char* text);
void initVehicles();
void updateTrafficLights();
void updateVehicles();
void updateEmergencyVehicle();

//DDA
void DDA(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float steps = fabs(dx) > fabs(dy) ? fabs(dx) : fabs(dy);

    float xInc = dx / steps;
    float yInc = dy / steps;

    float x = x1, y = y1;

    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; i++)
    {
        glVertex2f(round(x), round(y));
        x += xInc;
        y += yInc;
    }
    glEnd();
}

//MIDPOINT
void MP(int x1, int y1, int x2, int y2)
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    glBegin(GL_POINTS);
    while (true)
    {
        glVertex2i(x1, y1);

        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y1 += sy;
        }
    }
    glEnd();
}

//MIDPOINT CIRCLE
void plotCirclePoints(float cx, float cy, int x, int y)
{
    glVertex2f(cx + x, cy + y);
    glVertex2f(cx - x, cy + y);
    glVertex2f(cx + x, cy - y);
    glVertex2f(cx - x, cy - y);
    glVertex2f(cx + y, cy + x);
    glVertex2f(cx - y, cy + x);
    glVertex2f(cx + y, cy - x);
    glVertex2f(cx - y, cy - x);
}

void CMP(float cx, float cy, float radius)
{
    int r = (int)radius;
    int x = 0;
    int y = r;
    int d = 1 - r;

    glBegin(GL_POINTS);
    while (x <= y)
    {
        plotCirclePoints(cx, cy, x, y);
        x++;
        if (d < 0)
        {
            d += 2 * x + 1;
        }
        else
        {
            y--;
            d += 2 * (x - y) + 1;
        }
    }
    glEnd();
}
void FilledC(float cx, float cy, float radius)
{
    int r = (int)radius;
    for (int dy = -r; dy <= r; dy++)
    {
        int dx = (int)sqrt(r * r - dy * dy);
        DDA(cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

//BÉZIER
Point bezierPoint(Point p0, Point p1, Point p2, Point p3, float t)
{
    float u = 1 - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    Point p;
    p.x = uuu * p0.x + 3 * uu * t * p1.x + 3 * u * tt * p2.x + ttt * p3.x;
    p.y = uuu * p0.y + 3 * uu * t * p1.y + 3 * u * tt * p2.y + ttt * p3.y;
    return p;
}

void drawBezierCurve(Point p0, Point p1, Point p2, Point p3)
{
    glBegin(GL_LINE_STRIP);
    for (float t = 0; t <= 1.0f; t += 0.01f)
    {
        Point p = bezierPoint(p0, p1, p2, p3, t);
        glVertex2f(p.x, p.y);
    }
    glEnd();
}

void drawFilledBezier(Point p0, Point p1, Point p2, Point p3, float thickness)
{
    for (float offset = -thickness/2; offset <= thickness/2; offset += 0.5f)
        {
        glBegin(GL_LINE_STRIP);
        for (float t = 0; t <= 1.0f; t += 0.01f)
        {
            Point p = bezierPoint(p0, p1, p2, p3, t);
            glVertex2f(p.x, p.y + offset);
        }
        glEnd();
    }
}

void drawSky()
{
    glBegin(GL_QUADS);
    glColor3f(0.35f, 0.65f, 0.85f);
    glVertex2f(-700, 400);
    glVertex2f(700, 400);
    glColor3f(0.60f, 0.85f, 0.95f);
    glVertex2f(700, 100);
    glVertex2f(-700, 100);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.60f, 0.85f, 0.95f);
    glVertex2f(-700, 100);
    glVertex2f(700, 100);
    glColor3f(0.85f, 0.92f, 0.98f);
    glVertex2f(700, -50);
    glVertex2f(-700, -50);
    glEnd();
}
void drawGround()
{
    glColor3f(0.35f, 0.65f, 0.35f);
    glBegin(GL_QUADS);
    glVertex2f(-700, -60);
    glVertex2f(700, -60);
    glVertex2f(700, 50);
    glVertex2f(-700, 50);
    glEnd();

    glColor3f(0.30f, 0.60f, 0.30f);
    for (int i = -700; i < 700; i += 15)
        {
        for (int j = -60; j < 50; j += 10)
        {
            if ((i + j) % 30 == 0)
            {
                DDA(i, j, i, j + 5);
            }
        }
    }
}

void drawSun()
{
    for (int i = 5; i > 0; i--)
    {
        glColor4f(1.0f, 0.95f, 0.3f, 0.1f * i);
        FilledC(550, 320, 45 + i * 3);
    }
    glColor3f(1.0f, 0.95f, 0.3f);
    FilledC(550, 320, 45);
}

void drawCloud(float cx, float cy, float scale)
{
    glColor3f(1.0f, 1.0f, 1.0f);


    FilledC(cx - 35 * scale, cy - 5 * scale, 18 * scale);
    FilledC(cx - 15 * scale, cy - 8 * scale, 20 * scale);
    FilledC(cx + 5 * scale, cy - 10 * scale, 22 * scale);
    FilledC(cx + 25 * scale, cy - 8 * scale, 19 * scale);
    FilledC(cx + 40 * scale, cy - 5 * scale, 17 * scale);

    FilledC(cx - 25 * scale, cy + 5 * scale, 20 * scale);
    FilledC(cx - 5 * scale, cy + 8 * scale, 24 * scale);
    FilledC(cx + 15 * scale, cy + 10 * scale, 22 * scale);
    FilledC(cx + 35 * scale, cy + 5 * scale, 18 * scale);

    glColor3f(0.90f, 0.92f, 0.95f);
    FilledC(cx - 10 * scale, cy - 10 * scale, 15 * scale);
    FilledC(cx + 10 * scale, cy - 12 * scale, 16 * scale);
}

void drawBuilding(float x, float y, float width, float height, float r, float g, float b)
{
    // Main building
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();

    //shadow right side
    glColor3f(r * 0.85f, g * 0.85f, b * 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(x + width, y);
    glVertex2f(x + width + 8, y - 8);
    glVertex2f(x + width + 8, y + height - 8);
    glVertex2f(x + width, y + height);
    glEnd();

    //Rooftop
    glColor3f(r * 0.7f, g * 0.7f, b * 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(x + 10, y + height);
    glVertex2f(x + width - 10, y + height);
    glVertex2f(x + width - 15, y + height + 20);
    glVertex2f(x + 15, y + height + 20);
    glEnd();

    //windows
    float windowWidth = 16;
    float windowHeight = 22;
    float spacing = 28;

    for (float wy = y + 20; wy < y + height - 25; wy += 38)
        {
        for (float wx = x + 14; wx < x + width - 14; wx += spacing)
        {
            glColor3f(0.25f, 0.25f, 0.28f);
            glBegin(GL_QUADS);
            glVertex2f(wx - 1, wy - 1);
            glVertex2f(wx + windowWidth + 1, wy - 1);
            glVertex2f(wx + windowWidth + 1, wy + windowHeight + 1);
            glVertex2f(wx - 1, wy + windowHeight + 1);
            glEnd();

            if ((int)(wx + wy) % 3 == 0)
            {
                glColor3f(0.40f, 0.55f, 0.70f);
            }
            else
            {
                glColor3f(0.30f, 0.40f, 0.55f);
            }
            glBegin(GL_QUADS);
            glVertex2f(wx, wy);
            glVertex2f(wx + windowWidth, wy);
            glVertex2f(wx + windowWidth, wy + windowHeight);
            glVertex2f(wx, wy + windowHeight);
            glEnd();

            glColor3f(0.60f, 0.75f, 0.85f);
            glBegin(GL_QUADS);
            glVertex2f(wx, wy + windowHeight - 4);
            glVertex2f(wx + 6, wy + windowHeight - 4);
            glVertex2f(wx + 6, wy + windowHeight);
            glVertex2f(wx, wy + windowHeight);
            glEnd();

            glColor3f(0.2f, 0.2f, 0.22f);
            DDA(wx + windowWidth/2, wy, wx + windowWidth/2, wy + windowHeight);
            DDA(wx, wy + windowHeight/2, wx + windowWidth, wy + windowHeight/2);
        }
    }
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(2.0f);
    MP(x, y, x + width, y);
    MP(x + width, y, x + width, y + height);
    MP(x + width, y + height, x, y + height);
    MP(x, y + height, x, y);
    glLineWidth(1.0f);
}

void drawCastle(float x, float y)
{
    glColor3f(0.82f, 0.82f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(x - 100, y);
    glVertex2f(x + 100, y);
    glVertex2f(x + 100, y + 120);
    glVertex2f(x - 100, y + 120);
    glEnd();

    glColor3f(0.75f, 0.75f, 0.78f);
    for (float sy = y; sy < y + 120; sy += 15)
    {
        for (float sx = x - 100; sx < x + 100; sx += 30)
        {
            MP(sx, sy, sx + 28, sy);
        }
    }

    glColor3f(0.80f, 0.80f, 0.83f);
    glBegin(GL_QUADS);
    glVertex2f(x - 120, y);
    glVertex2f(x - 80, y);
    glVertex2f(x - 80, y + 150);
    glVertex2f(x - 120, y + 150);
    glEnd();

    //Right
    glBegin(GL_QUADS);
    glVertex2f(x + 80, y);
    glVertex2f(x + 120, y);
    glVertex2f(x + 120, y + 150);
    glVertex2f(x + 80, y + 150);
    glEnd();

    //Center
    glColor3f(0.85f, 0.85f, 0.88f);
    glBegin(GL_QUADS);
    glVertex2f(x - 30, y + 120);
    glVertex2f(x + 30, y + 120);
    glVertex2f(x + 30, y + 180);
    glVertex2f(x - 30, y + 180);
    glEnd();

    glColor3f(0.70f, 0.70f, 0.73f);
    for (float bx = x - 120; bx < x - 80; bx += 10)
    {
        if ((int)bx % 20 == 0)
        {
            glBegin(GL_QUADS);
            glVertex2f(bx, y + 150);
            glVertex2f(bx + 8, y + 150);
            glVertex2f(bx + 8, y + 160);
            glVertex2f(bx, y + 160);
            glEnd();
        }
    }

    glColor3f(0.65f, 0.28f, 0.20f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - 130, y + 150);
    glVertex2f(x - 70, y + 150);
    glVertex2f(x - 100, y + 185);
    glEnd();

    glBegin(GL_TRIANGLES);
    glVertex2f(x - 40, y + 180);
    glVertex2f(x + 40, y + 180);
    glVertex2f(x, y + 220);
    glEnd();

    glBegin(GL_TRIANGLES);
    glVertex2f(x + 70, y + 150);
    glVertex2f(x + 130, y + 150);
    glVertex2f(x + 100, y + 185);
    glEnd();

    glColor3f(0.60f, 0.25f, 0.18f);
    for (float ty = y + 155; ty < y + 180; ty += 5)
    {
        drawBezierCurve(Point(x - 125, ty), Point(x - 110, ty - 2),
                        Point(x - 90, ty - 2), Point(x - 75, ty));
    }

    glColor3f(0.25f, 0.25f, 0.30f);
    for (float wy = y + 30; wy < y + 140; wy += 50)
    {
        //Left
        glBegin(GL_QUADS);
        glVertex2f(x - 112, wy);
        glVertex2f(x - 88, wy);
        glVertex2f(x - 88, wy + 28);
        glVertex2f(x - 112, wy + 28);
        glEnd();

        //Right
        glBegin(GL_QUADS);
        glVertex2f(x + 88, wy);
        glVertex2f(x + 112, wy);
        glVertex2f(x + 112, wy + 28);
        glVertex2f(x + 88, wy + 28);
        glEnd();
    }
    //windows
    for (float wy = y + 25; wy < y + 110; wy += 42)
    {
        for (float wx = x - 85; wx < x + 85; wx += 45)
        {
            glBegin(GL_QUADS);
            glVertex2f(wx, wy);
            glVertex2f(wx + 22, wy);
            glVertex2f(wx + 22, wy + 32);
            glVertex2f(wx, wy + 32);
            glEnd();
        }
    }

    // Main
    glColor3f(0.25f, 0.18f, 0.10f);
    glBegin(GL_QUADS);
    glVertex2f(x - 22, y);
    glVertex2f(x + 22, y);
    glVertex2f(x + 22, y + 50);
    glVertex2f(x - 22, y + 50);
    glEnd();

    glLineWidth(15.0f);
    drawBezierCurve(Point(x - 22, y + 50), Point(x - 22, y + 65),
                    Point(x + 22, y + 65), Point(x + 22, y + 50));
    glLineWidth(1.0f);
}

void drawTree(float x, float y, float scale)
{
    glColor3f(0.35f, 0.22f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(x - 12 * scale, y);
    glVertex2f(x + 12 * scale, y);
    glVertex2f(x + 10 * scale, y + 80 * scale);
    glVertex2f(x - 10 * scale, y + 80 * scale);
    glEnd();

    glColor3f(0.25f, 0.15f, 0.08f);
    glBegin(GL_QUADS);
    glVertex2f(x + 2 * scale, y);
    glVertex2f(x + 12 * scale, y);
    glVertex2f(x + 10 * scale, y + 80 * scale);
    glVertex2f(x + 2 * scale, y + 80 * scale);
    glEnd();

    glColor3f(0.28f, 0.16f, 0.09f);
    for (float ty = y; ty < y + 75 * scale; ty += 10 * scale)
    {
        DDA(x - 9 * scale, ty, x - 7 * scale, ty + 4 * scale);
        DDA(x + 7 * scale, ty, x + 9 * scale, ty + 4 * scale);
        DDA(x - 3 * scale, ty + 5 * scale, x - 1 * scale, ty + 8 * scale);
    }
    glColor3f(0.32f, 0.20f, 0.11f);
    glLineWidth(5.0f * scale);

    DDA(x, y + 50 * scale, x - 30 * scale, y + 70 * scale);
    DDA(x - 30 * scale, y + 70 * scale, x - 45 * scale, y + 85 * scale);
    DDA(x, y + 60 * scale, x - 25 * scale, y + 80 * scale);


    DDA(x, y + 55 * scale, x + 28 * scale, y + 75 * scale);
    DDA(x + 28 * scale, y + 75 * scale, x + 42 * scale, y + 90 * scale);
    DDA(x, y + 65 * scale, x + 22 * scale, y + 82 * scale);

    glLineWidth(3.0f * scale);

    DDA(x - 20 * scale, y + 75 * scale, x - 32 * scale, y + 95 * scale);
    DDA(x + 18 * scale, y + 78 * scale, x + 30 * scale, y + 98 * scale);
    glLineWidth(1.0f);


    glColor3f(0.10f, 0.38f, 0.10f);
    FilledC(x - 40 * scale, y + 75 * scale, 25 * scale);
    FilledC(x + 38 * scale, y + 78 * scale, 24 * scale);
    FilledC(x - 20 * scale, y + 95 * scale, 28 * scale);
    FilledC(x + 18 * scale, y + 92 * scale, 26 * scale);


    glColor3f(0.16f, 0.48f, 0.16f);
    FilledC(x - 35 * scale, y + 85 * scale, 23 * scale);
    FilledC(x + 32 * scale, y + 88 * scale, 22 * scale);
    FilledC(x - 10 * scale, y + 105 * scale, 26 * scale);
    FilledC(x + 8 * scale, y + 102 * scale, 24 * scale);
    FilledC(x, y + 95 * scale, 25 * scale);

    glColor3f(0.22f, 0.56f, 0.22f);
    FilledC(x - 28 * scale, y + 95 * scale, 20 * scale);
    FilledC(x + 25 * scale, y + 98 * scale, 19 * scale);
    FilledC(x - 5 * scale, y + 115 * scale, 23 * scale);
    FilledC(x + 3 * scale, y + 112 * scale, 21 * scale);


    glColor3f(0.28f, 0.64f, 0.28f);
    FilledC(x - 18 * scale, y + 105 * scale, 18 * scale);
    FilledC(x + 15 * scale, y + 108 * scale, 17 * scale);
    FilledC(x, y + 120 * scale, 20 * scale);


    glColor3f(0.35f, 0.72f, 0.35f);
    FilledC(x - 8 * scale, y + 125 * scale, 15 * scale);
    FilledC(x + 6 * scale, y + 123 * scale, 14 * scale);

    glColor3f(0.45f, 0.80f, 0.45f);
    FilledC(x - 12 * scale, y + 115 * scale, 10 * scale);
    FilledC(x + 10 * scale, y + 118 * scale, 9 * scale);
    FilledC(x - 2 * scale, y + 130 * scale, 8 * scale);

    glColor3f(0.30f, 0.68f, 0.30f);
    for (int i = 0; i < 8; i++)
    {
        float angle = i * 45;
        float clusterX = x + 20 * scale * cos(angle * PI / 180.0f);
        float clusterY = y + 105 * scale + 15 * scale * sin(angle * PI / 180.0f);
        FilledC(clusterX, clusterY, 6 * scale);
    }
}

void drawRoad()
{
    glColor3f(0.65f, 0.65f, 0.68f);
    glBegin(GL_QUADS);
    glVertex2f(-700, -280);
    glVertex2f(700, -280);
    glVertex2f(700, -225);
    glVertex2f(-700, -225);
    glEnd();

    glColor3f(0.60f, 0.60f, 0.63f);
    for (int i = -700; i < 700; i += 40)
    {
        for (int j = -280; j < -225; j += 20)
        {
            glBegin(GL_LINE_LOOP);
            glVertex2f(i, j);
            glVertex2f(i + 38, j);
            glVertex2f(i + 38, j + 18);
            glVertex2f(i, j + 18);
            glEnd();
        }
    }

    glColor3f(0.50f, 0.50f, 0.52f);
    glLineWidth(5.0f);
    DDA(-700, -225, 700, -225);
    glLineWidth(1.0f);

    glColor3f(0.30f, 0.60f, 0.30f);
    glBegin(GL_QUADS);
    glVertex2f(-700, -400);
    glVertex2f(700, -400);
    glVertex2f(700, -280);
    glVertex2f(-700, -280);
    glEnd();

    glColor3f(0.25f, 0.55f, 0.25f);
    for (int i = -700; i < 700; i += 8)
    {
        for (int j = -400; j < -280; j += 6)
        {
            if ((i + j) % 12 == 0)
            {
                DDA(i, j, i + 1, j + 4);
            }
        }
    }

    glColor3f(0.35f, 0.65f, 0.35f);
    for (int i = -700; i < 700; i += 25)
    {
        for (int j = -400; j < -280; j += 20)
        {
            if ((i + j) % 50 == 0)
            {
                FilledC(i, j, 8);
            }
        }
    }

    glColor3f(0.25f, 0.25f, 0.25f);
    glBegin(GL_QUADS);
    glVertex2f(-700, -220);
    glVertex2f(700, -220);
    glVertex2f(700, -60);
    glVertex2f(-700, -60);
    glEnd();

    glColor3f(0.22f, 0.22f, 0.22f);
    for (int i = -700; i < 700; i += 80)
    {
        DDA(i, -220, i + 40, -60);
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(4.0f);
    DDA(-700, -65, 700, -65);
    DDA(-700, -215, 700, -215);
    glLineWidth(1.0f);

    glColor3f(1.0f, 0.95f, 0.0f);
    glLineWidth(4.0f);
    for (int i = -700; i < 700; i += 60)
    {
        DDA(i, -140, i + 35, -140);
    }
    glLineWidth(1.0f);

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(5.0f);
    DDA(-450, -120, -450, -95);
    DDA(-450, -95, -460, -105);
    DDA(-450, -95, -440, -105);

    DDA(450, -160, 450, -185);
    DDA(450, -185, 440, -175);
    DDA(450, -185, 460, -175);
    glLineWidth(1.0f);
}

void drawTrafficLight(float x, float y, LightState state)
{
    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(x - 8, y);
    glVertex2f(x + 8, y);
    glVertex2f(x + 6, y + 15);
    glVertex2f(x - 6, y + 15);
    glEnd();

    glColor3f(0.20f, 0.20f, 0.20f);
    glLineWidth(6.0f);
    DDA(x, y + 15, x, y + 80);
    glLineWidth(1.0f);

    glColor3f(0.12f, 0.12f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(x - 16, y + 80);
    glVertex2f(x + 16, y + 80);
    glVertex2f(x + 16, y + 150);
    glVertex2f(x - 16, y + 150);
    glEnd();

    glColor3f(0.10f, 0.10f, 0.10f);
    glBegin(GL_QUADS);
    glVertex2f(x - 16, y + 150);
    glVertex2f(x + 16, y + 150);
    glVertex2f(x + 19, y + 155);
    glVertex2f(x - 19, y + 155);
    glEnd();

    glColor3f(0.08f, 0.08f, 0.08f);
    glLineWidth(2.0f);
    MP(x - 16, y + 80, x + 16, y + 80);
    MP(x + 16, y + 80, x + 16, y + 150);
    MP(x + 16, y + 150, x - 16, y + 150);
    MP(x - 16, y + 150, x - 16, y + 80);
    glLineWidth(1.0f);

    float lightY[] = {133, 113, 93};

    for (int i = 0; i < 3; i++)
    {
        glColor3f(0.15f, 0.15f, 0.15f);
        FilledC(x, y + lightY[i], 9);
    }

    if (state == LIGHT_RED || emergencyMode)
    {
        glColor3f(1.0f, 0.0f, 0.0f);
        FilledC(x, y + 133, 8);
        glColor3f(1.0f, 0.3f, 0.3f);
        FilledC(x, y + 133, 10);
    }
    else
    {
        glColor3f(0.3f, 0.0f, 0.0f);
        FilledC(x, y + 133, 8);
    }

    if (state == LIGHT_YELLOW)
    {
        glColor3f(1.0f, 1.0f, 0.0f);
        FilledC(x, y + 113, 8);
        glColor3f(1.0f, 1.0f, 0.5f);
        FilledC(x, y + 113, 10);
    }
    else
    {
        glColor3f(0.3f, 0.3f, 0.0f);
        FilledC(x, y + 113, 8);
    }

    if (state == LIGHT_GREEN && !emergencyMode)
    {
        glColor3f(0.0f, 1.0f, 0.0f);
        FilledC(x, y + 93, 8);
        glColor3f(0.5f, 1.0f, 0.5f);
        FilledC(x, y + 93, 10);
    }
    else
    {
        glColor3f(0.0f, 0.3f, 0.0f);
        FilledC(x, y + 93, 8);
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < 3; i++)
    {
        FilledC(x - 2, y + lightY[i] + 2, 1.5);
    }
}

void drawCar(float x, float y, float scale, float r, float g, float b, float wheelRot)
{
    glPushMatrix();

    glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(x - 55 * scale, y - 5);
    glVertex2f(x + 55 * scale, y - 5);
    glVertex2f(x + 50 * scale, y - 8);
    glVertex2f(x - 50 * scale, y - 8);
    glEnd();

    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x - 50 * scale, y);
    glVertex2f(x + 50 * scale, y);
    glVertex2f(x + 50 * scale, y + 32 * scale);
    glVertex2f(x - 50 * scale, y + 32 * scale);
    glEnd();

    glColor3f(r * 1.2f, g * 1.2f, b * 1.2f);
    glBegin(GL_QUADS);
    glVertex2f(x - 48 * scale, y + 28 * scale);
    glVertex2f(x + 48 * scale, y + 28 * scale);
    glVertex2f(x + 48 * scale, y + 32 * scale);
    glVertex2f(x - 48 * scale, y + 32 * scale);
    glEnd();

    glLineWidth(22.0f * scale);
    glColor3f(r * 0.75f, g * 0.75f, b * 0.75f);
    drawBezierCurve(Point(x - 32 * scale, y + 32 * scale),
                    Point(x - 22 * scale, y + 62 * scale),
                    Point(x + 22 * scale, y + 62 * scale),
                    Point(x + 32 * scale, y + 32 * scale));
    glLineWidth(1.0f);

    //Windows
    glColor3f(0.15f, 0.15f, 0.18f);
    glBegin(GL_QUADS);
    glVertex2f(x - 28 * scale, y + 36 * scale);
    glVertex2f(x - 6 * scale, y + 36 * scale);
    glVertex2f(x - 6 * scale, y + 52 * scale);
    glVertex2f(x - 24 * scale, y + 52 * scale);
    glEnd();

    glBegin(GL_QUADS);
    glVertex2f(x + 6 * scale, y + 36 * scale);
    glVertex2f(x + 28 * scale, y + 36 * scale);
    glVertex2f(x + 24 * scale, y + 52 * scale);
    glVertex2f(x + 6 * scale, y + 52 * scale);
    glEnd();

    //Window
    glColor3f(0.50f, 0.70f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(x - 26 * scale, y + 48 * scale);
    glVertex2f(x - 12 * scale, y + 48 * scale);
    glVertex2f(x - 12 * scale, y + 50 * scale);
    glVertex2f(x - 24 * scale, y + 50 * scale);
    glEnd();

    //Headlights
    glColor3f(1.0f, 1.0f, 0.9f);
    glBegin(GL_QUADS);
    glVertex2f(x + 48 * scale, y + 8 * scale);
    glVertex2f(x + 52 * scale, y + 8 * scale);
    glVertex2f(x + 52 * scale, y + 15 * scale);
    glVertex2f(x + 48 * scale, y + 15 * scale);
    glEnd();

    //Taillights
    glColor3f(0.8f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(x - 52 * scale, y + 8 * scale);
    glVertex2f(x - 48 * scale, y + 8 * scale);
    glVertex2f(x - 48 * scale, y + 15 * scale);
    glVertex2f(x - 52 * scale, y + 15 * scale);
    glEnd();

    //Wheels
    glColor3f(0.08f, 0.08f, 0.08f);
    FilledC(x - 30 * scale, y, 14 * scale);
    FilledC(x + 30 * scale, y, 14 * scale);

    // Wheel
    glColor3f(0.5f, 0.5f, 0.55f);
    FilledC(x - 30 * scale, y, 10 * scale);
    FilledC(x + 30 * scale, y, 10 * scale);

    //Wheel
    glColor3f(0.3f, 0.3f, 0.35f);
    glLineWidth(2.0f);
    for (int i = 0; i < 5; i++)
    {
        float angle = wheelRot + i * 72;
        float x1 = x - 30 * scale;
        float y1 = y;
        float x2 = x1 + 8 * scale * cos(angle * PI / 180.0f);
        float y2 = y1 + 8 * scale * sin(angle * PI / 180.0f);
        DDA(x1, y1, x2, y2);

        x1 = x + 30 * scale;
        x2 = x1 + 8 * scale * cos(angle * PI / 180.0f);
        y2 = y1 + 8 * scale * sin(angle * PI / 180.0f);
        DDA(x1, y1, x2, y2);
    }
    glLineWidth(1.0f);

    // Tire treads
    glColor3f(0.05f, 0.05f, 0.05f);
    for (int i = 0; i < 8; i++)
    {
        float angle = wheelRot + i * 45;
        float x1 = x - 30 * scale + 12 * scale * cos(angle * PI / 180.0f);
        float y1 = y + 12 * scale * sin(angle * PI / 180.0f);
        float x2 = x - 30 * scale + 14 * scale * cos(angle * PI / 180.0f);
        float y2 = y + 14 * scale * sin(angle * PI / 180.0f);
        DDA(x1, y1, x2, y2);
    }

    glPopMatrix();
}

void drawEmergencyVehicle(float x, float y, float scale)
{
    glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(x - 65 * scale, y - 5);
    glVertex2f(x + 65 * scale, y - 5);
    glVertex2f(x + 60 * scale, y - 8);
    glVertex2f(x - 60 * scale, y - 8);
    glEnd();

    // Body
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(x - 60 * scale, y);
    glVertex2f(x + 60 * scale, y);
    glVertex2f(x + 60 * scale, y + 42 * scale);
    glVertex2f(x - 60 * scale, y + 42 * scale);
    glEnd();

    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(x - 60 * scale, y + 18 * scale);
    glVertex2f(x + 60 * scale, y + 18 * scale);
    glVertex2f(x + 60 * scale, y + 26 * scale);
    glVertex2f(x - 60 * scale, y + 26 * scale);
    glEnd();

    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(x - 6 * scale, y + 8 * scale);
    glVertex2f(x + 6 * scale, y + 8 * scale);
    glVertex2f(x + 6 * scale, y + 34 * scale);
    glVertex2f(x - 6 * scale, y + 34 * scale);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(x - 18 * scale, y + 15 * scale);
    glVertex2f(x + 18 * scale, y + 15 * scale);
    glVertex2f(x + 18 * scale, y + 27 * scale);
    glVertex2f(x - 18 * scale, y + 27 * scale);
    glEnd();

    glColor3f(0.8f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(x + 4 * scale, y + 8 * scale);
    glVertex2f(x + 6 * scale, y + 8 * scale);
    glVertex2f(x + 6 * scale, y + 34 * scale);
    glVertex2f(x + 4 * scale, y + 34 * scale);
    glEnd();

    //Cabin
    glLineWidth(24.0f * scale);
    glColor3f(0.95f, 0.95f, 0.95f);
    drawBezierCurve(Point(x - 42 * scale, y + 42 * scale),
                    Point(x - 32 * scale, y + 72 * scale),
                    Point(x + 32 * scale, y + 72 * scale),
                    Point(x + 42 * scale, y + 42 * scale));
    glLineWidth(1.0f);

    glColor3f(0.15f, 0.15f, 0.20f);
    glLineWidth(20.0f * scale);
    drawBezierCurve(Point(x - 35 * scale, y + 45 * scale),
                    Point(x - 28 * scale, y + 65 * scale),
                    Point(x + 28 * scale, y + 65 * scale),
                    Point(x + 35 * scale, y + 45 * scale));
    glLineWidth(1.0f);

    //siren
    if (emergencyMode)
    {
        if ((int)sirenBlink % 2 == 0)
        {
            glColor3f(1.0f, 0.0f, 0.0f);
        }
        else
        {
            glColor3f(0.0f, 0.0f, 1.0f);
        }

        FilledC(x - 25 * scale, y + 58 * scale, 8 * scale);

        if ((int)sirenBlink % 2 == 0)
        {
            glColor3f(0.0f, 0.0f, 1.0f);
        }
        else
        {
            glColor3f(1.0f, 0.0f, 0.0f);
        }
        FilledC(x + 25 * scale, y + 58 * scale, 8 * scale);

        glColor4f(1.0f, 0.0f, 0.0f, 0.3f);
        FilledC(x - 25 * scale, y + 58 * scale, 12 * scale);
        glColor4f(0.0f, 0.0f, 1.0f, 0.3f);
        FilledC(x + 25 * scale, y + 58 * scale, 12 * scale);
    }
    else
    {
        glColor3f(0.5f, 0.0f, 0.0f);
        FilledC(x - 25 * scale, y + 58 * scale, 8 * scale);
        glColor3f(0.0f, 0.0f, 0.5f);
        FilledC(x + 25 * scale, y + 58 * scale, 8 * scale);
    }

    //Siren
    glColor3f(0.2f, 0.2f, 0.25f);
    glBegin(GL_QUADS);
    glVertex2f(x - 35 * scale, y + 54 * scale);
    glVertex2f(x + 35 * scale, y + 54 * scale);
    glVertex2f(x + 35 * scale, y + 62 * scale);
    glVertex2f(x - 35 * scale, y + 62 * scale);
    glEnd();

    glColor3f(1.0f, 1.0f, 0.9f);
    FilledC(x + 58 * scale, y + 12 * scale, 6 * scale);

    glColor3f(0.08f, 0.08f, 0.08f);
    FilledC(x - 35 * scale, y, 16 * scale);
    FilledC(x + 35 * scale, y, 16 * scale);

    glColor3f(0.5f, 0.5f, 0.55f);
    FilledC(x - 35 * scale, y, 12 * scale);
    FilledC(x + 35 * scale, y, 12 * scale);
}

//Display
void drawText(float x, float y, const char* text)
{
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}


void initVehicles()
{
    vehicles.clear();
    for (int i = 0; i < 3; i++)
    {
        Vehicle v;
        v.x = -600 + i * 400;
        v.y = -105;
        v.speed = 1.2f + (rand() % 8) * 0.1f;
        v.targetSpeed = v.speed;
        v.wheelRotation = 0;
        v.movingRight = true;
        v.scale = 0.75f + (rand() % 5) * 0.05f;
        v.r = 0.15f + (rand() % 7) * 0.1f;
        v.g = 0.15f + (rand() % 7) * 0.1f;
        v.b = 0.45f + (rand() % 5) * 0.1f;
        v.acceleration = ACCELERATION;
        vehicles.push_back(v);
    }

    // Lower lane (moving left)
    for (int i = 0; i < 3; i++)
    {
        Vehicle v;
        v.x = 600 - i * 400;
        v.y = -175;
        v.speed = 1.2f + (rand() % 8) * 0.1f;
        v.targetSpeed = v.speed;
        v.wheelRotation = 0;
        v.movingRight = false;
        v.scale = 0.7f + (rand() % 5) * 0.05f;
        v.r = 0.45f + (rand() % 5) * 0.1f;
        v.g = 0.15f + (rand() % 7) * 0.1f;
        v.b = 0.15f + (rand() % 7) * 0.1f;
        v.acceleration = ACCELERATION;
        vehicles.push_back(v);
    }
}

void init()
{
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-700, 700, -400, 400);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initVehicles();
}

void updateTrafficLights()
{
    if (emergencyMode)
    {
        leftLight = LIGHT_RED;
        rightLight = LIGHT_RED;
        return;
    }

    lightTimer++;

    if (leftLight == LIGHT_GREEN && lightTimer > lightDuration)
    {
        leftLight = LIGHT_YELLOW;
        lightTimer = 0;
    }
    else if (leftLight == LIGHT_YELLOW && lightTimer > yellowDuration)
    {
        leftLight = LIGHT_RED;
        rightLight = LIGHT_GREEN;
        lightTimer = 0;
    }
    else if (rightLight == LIGHT_GREEN && lightTimer > lightDuration)
    {
        rightLight = LIGHT_YELLOW;
        lightTimer = 0;
    }
    else if (rightLight == LIGHT_YELLOW && lightTimer > yellowDuration)
    {
        rightLight = LIGHT_RED;
        leftLight = LIGHT_GREEN;
        lightTimer = 0;
    }
}

void updateVehicles()
{
    for (auto& v : vehicles)
    {
        bool shouldStop = false;
        float distanceToLight = 0;

        if (v.movingRight)
        {
            distanceToLight = -250 - v.x;
            if (distanceToLight > 0 && distanceToLight < STOPPING_DISTANCE && leftLight != LIGHT_GREEN)
            {
                shouldStop = true;
            }
            if (emergencyMode && v.x > -400 && v.x < 400)
            {
                shouldStop = true;
            }
        }
        else
        {
            distanceToLight = v.x - 250;
            if (distanceToLight > 0 && distanceToLight < STOPPING_DISTANCE && rightLight != LIGHT_GREEN)
            {
                shouldStop = true;
            }
            if (emergencyMode && v.x > -400 && v.x < 400)
            {
                shouldStop = true;
            }
        }

        if (shouldStop)
        {
            v.targetSpeed = 0.0f;
            if (v.speed > 0)
            {
                v.speed -= DECELERATION;
                if (v.speed < 0) v.speed = 0;
            }
        }
        else
        {
            v.targetSpeed = 1.5f + (rand() % 10) * 0.05f;
            if (v.speed < v.targetSpeed)
            {
                v.speed += ACCELERATION;
                if (v.speed > v.targetSpeed) v.speed = v.targetSpeed;
            }
        }

        if (v.movingRight)
        {
            v.x += v.speed;
            v.wheelRotation += v.speed * 4;
        }
        else
        {
            v.x -= v.speed;
            v.wheelRotation -= v.speed * 4;
        }

        if (v.x > 850) v.x = -850;
        if (v.x < -850) v.x = 850;
    }
}

void updateEmergencyVehicle()
{
    if (emergencySpeed > emergencyMaxSpeed)
    {
        emergencySpeed = emergencyMaxSpeed;
    }
    if (emergencySpeed < -emergencyMaxSpeed)
    {
        emergencySpeed = -emergencyMaxSpeed;
    }
}


void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    drawSky();
    drawSun();

    drawCloud(-550 + cloudOffset, 310, 1.0f);
    drawCloud(-120 + cloudOffset * 0.65f, 340, 0.85f);
    drawCloud(350 + cloudOffset * 0.45f, 320, 1.1f);

    drawGround();

    drawTree(-640, -60, 0.60f);
    drawTree(-540, -60, 0.65f);
    drawTree(-440, -60, 0.62f);
    drawTree(-340, -60, 0.64f);
    drawTree(340, -60, 0.64f);
    drawTree(440, -60, 0.62f);
    drawTree(540, -60, 0.65f);
    drawTree(640, -60, 0.60f);

    drawBuilding(-650, -60, 100, 240, 0.86f, 0.88f, 0.90f);
    drawBuilding(-530, -60, 110, 270, 0.84f, 0.86f, 0.89f);

    drawCastle(0, -60);

    drawBuilding(430, -60, 110, 270, 0.85f, 0.87f, 0.88f);
    drawBuilding(560, -60, 100, 240, 0.84f, 0.87f, 0.90f);

    drawTree(-270, -60, 0.92f);
    drawTree(-140, -60, 0.98f);
    drawTree(150, -60, 0.95f);
    drawTree(280, -60, 0.90f);

    drawRoad();

    drawTrafficLight(-250, -220, leftLight);
    drawTrafficLight(250, -220, rightLight);

    for (const auto& v : vehicles)
    {
        drawCar(v.x, v.y, v.scale, v.r, v.g, v.b, v.wheelRotation);
    }

    drawEmergencyVehicle(emergencyVehicleX, emergencyVehicleY, 1.0f);

    glColor3f(0.0f, 0.0f, 0.0f);
    if (emergencyMode)
    {
        drawText(-680, 370, "EMERGENCY MODE: ACTIVE");
        drawText(-680, 345, "All lanes STOP - Emergency vehicle priority");
    }
    else
    {
        drawText(-680, 370, "NORMAL MODE - Traffic flowing");
    }
    drawText(-680, 320, "E-Emergency | N-Normal | WASD-Move | P-Pause | R-Reset | ESC-Exit");

    char speedText[50];
    sprintf(speedText, "Emergency Speed: %.1f", emergencySpeed);
    drawText(-680, 295, speedText);

    if (isPaused)
    {
        glColor3f(1.0f, 0.0f, 0.0f);
        glLineWidth(3.0f);
        drawText(-100, 0, "PAUSED");
    }

    glutSwapBuffers();
}
ER
void timer(int value)
{
    if (!isPaused)
    {
        frameCount++;
        cloudOffset += 0.25f;
        if (cloudOffset > 1500) cloudOffset = -800;

        sirenBlink += 0.5f;
        if (sirenBlink > 20) sirenBlink = 0;

        updateTrafficLights();
        updateVehicles();
        updateEmergencyVehicle();
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 'e':
        case 'E':
            emergencyMode = true;
            std::cout << "EMERGENCY MODE ACTIVATED" << std::endl;
            break;

        case 'n':
        case 'N':
            emergencyMode = false;
            std::cout << "Normal Mode - Traffic resumed" << std::endl;
            break;

        case 'w':
        case 'W':
            if (emergencyVehicleY < -90)
            {
                emergencyVehicleY += 5.0f;
            }
            break;

        case 's':
        case 'S':
            if (emergencyVehicleY > -190)
            {
                emergencyVehicleY -= 5.0f;
            }
            break;

        case 'a':
        case 'A':
            emergencyVehicleX -= 8.0f;
            emergencySpeed = -emergencyMaxSpeed;
            break;

        case 'd':
        case 'D':
            emergencyVehicleX += 8.0f;
            emergencySpeed = emergencyMaxSpeed;
            break;

        case 'p':
        case 'P':
            isPaused = !isPaused;
            std::cout << (isPaused ? "PAUSED" : "RESUMED") << std::endl;
            break;

        case 'r':
        case 'R':
            emergencyMode = false;
            isPaused = false;
            emergencyVehicleX = -600.0f;
            emergencyVehicleY = -105.0f;
            emergencySpeed = 0.0f;
            lightTimer = 0;
            leftLight = LIGHT_GREEN;
            rightLight = LIGHT_RED;
            cloudOffset = 0;
            initVehicles();
            std::cout << "System RESET" << std::endl;
            break;

        case 27:
            std::cout << "Exiting..." << std::endl;
            exit(0);
            break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("Intelligent Traffic Control System");

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, timer, 0);

    std::cout << "\n=================================================\n";
    std::cout << "INTELLIGENT TRAFFIC CONTROL SYSTEM - SIDE VIEW\n";
    std::cout << "=================================================\n\n";
    std::cout << "CONTROLS:\n";
    std::cout << "  E - Activate Emergency Mode\n";
    std::cout << "  N - Normal Mode (Deactivate Emergency)\n";
    std::cout << "  W/A/S/D - Move Emergency Vehicle\n";
    std::cout << "  P - Pause/Resume Simulation\n";
    std::cout << "  R - Reset System\n";
    std::cout << "  ESC - Exit\n\n";
    glutMainLoop();
    return 0;
}
