#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/freeglut.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include "brush.h"
#include "hlbsp.h"
#include <stdlib.h>

using namespace glm;

struct Camera
{
    vec3 pos;
    vec2 ang;

    vec3 front, left, up;
};

const float pi = 3.14159265358979323846;

int width = 1920;
int height = 1080;

Camera mainCamera = {{0, 0, 0}, {0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
Camera *cam = &mainCamera;

float currentTime = 0.0f;
float deltaTime = 0.0f;
float masterSpeed = 1.0f;
float cameraSpeed = 1.0f;
float mouseSpeed = 1.0f;

bool lookMode = true;
bool flyMode = true;

float keys[256];
int nKeys = 0;

int hull = 0;
bool side = true;
BspExport bsp;

void updateCamera()
{
    if(lookMode)
    {
        if(cam->ang.y > +pi/2) cam->ang.y = +pi/2;
        if(cam->ang.y < -pi/2) cam->ang.y = -pi/2;
        if(cam->ang.x < 0) cam->ang.x = 2*pi;
        if(cam->ang.x > 2*pi) cam->ang.x = 0;
        cam->front = {cos(cam->ang.x)*cos(cam->ang.y), sin(cam->ang.x)*cos(cam->ang.y), sin(cam->ang.y)};
        cam->left = {-sin(cam->ang.x), cos(cam->ang.x), 0};
        cam->up = cross(cam->front, cam->left);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(
        cam->pos.x, cam->pos.y, cam->pos.z, 
        cam->pos.x + cam->front.x, cam->pos.y + cam->front.y, cam->pos.z + cam->front.z,
        cam->up.x, cam->up.y, cam->up.z);
}

void draw_face(int32 lastEdge, int32 plane)
{
    int32 edge = lastEdge;

    glBegin(GL_POLYGON);

    vec3 n = bsp.env.planes[plane].normal;

    {
        int k = 8;
        vec3 a = {(n.x+1)/2, (n.y+1)/2, (n.z+1)/2};
        vec3 b = {(float)(rand() % 1000) / 1000, (float)(rand() % 1000) / 1000, (float)(rand() % 1000) / 1000};
        vec3 c = {(a.x*k + b.x)/(k+1), (a.y*k + b.y)/(k+1), (a.z*k + b.z)/(k+1)};
        glColor3f(c.x, c.y, c.z);
    }

    do
    {
        int32 v = bsp.env.links[edge].data;
        vec3 p = bsp.env.vertices[v];
        glVertex3f(p.x, p.y, p.z);
        edge = bsp.env.links[edge].lNext;
    } while(edge != lastEdge);

    glEnd();
}

void draw_subface(int32 subface, int32 plane)
{
    if(bsp.env.subfaces[subface].contents == (SUBFACE_SOLID | SUBFACE_EMPTY))
        draw_face(bsp.env.subfaces[subface].lEdges, plane);
    else if(bsp.env.subfaces[subface].children[0] != nill)
    {
        for(int32 i = 0; i < 2; i++)
            draw_subface(bsp.env.subfaces[subface].children[i], plane);
    }
}

void display_rec(int32 brush)
{
    int contents = bsp.env.brushes[brush].contents;

    if(contents == 0)
    {
        for(int32 i = 0; i < 2; i++)
            display_rec(bsp.env.brushes[brush].children[i]);
        return;
    }

    if(contents != CONTENTS_SOLID) return;

    int32 lastLink = bsp.env.brushes[brush].lFaces;
    int32 link = lastLink;

    if(link == nill) return;

    do
    {
        int32 ind = bsp.env.links[link].data;
        bool sign = ind < 0;
        int32 face = sign ? ~ind : ind;

        if(sign ^ side)
        {
            glFrontFace(GL_CW);
        }
        else
        {
            glFrontFace(GL_CCW);
        }

        int32 lastLink2 = bsp.env.faces[face].lSubfaces;
        int32 link2 = lastLink2;

        if(link2 != nill)
        do
        {
            int32 subface = bsp.env.links[link2].data;
            draw_subface(subface, bsp.env.faces[face].plane);
            link2 = bsp.env.links[link2].lNext;
        } while(link2 != lastLink2);

        link = bsp.env.links[link].lNext;
    } while(link != lastLink);
}

void display()
{
    srand(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    display_rec(hull);

    glutSwapBuffers();
}

void idle()
{
    float newtime = (float)glutGet(GLUT_ELAPSED_TIME)/1000;
    deltaTime = newtime - currentTime;
    currentTime = newtime;

    if(flyMode)
    {
        vec3 vel = 
            (keys['w']-keys['s']) * cam->front + 
            (keys['a']-keys['d']) * cam->left + 
            (keys['e']-keys['q']) * cam->up;

        cam->pos += vel*masterSpeed*cameraSpeed*deltaTime;
    }

    //idle();

    updateCamera();
    glutPostRedisplay();
}

void reshape(int w, int h)
{
    if(h == 0) h == 1;
    float aspect = (float)w/h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(90, aspect, 1, 20000);
    updateCamera();
    width = w;
    height = h;
}

void motion(int x, int y)
{
    if(!lookMode) return;
    if(x == width/2 && y == height/2) return;

    cam->ang.x += (width/2 - x) * mouseSpeed / 100.0f;
    cam->ang.y += (height/2 - y) * mouseSpeed / 100.0f;

    updateCamera();

    glutWarpPointer(width/2, height/2);
}

void keyDown(unsigned char key, int x, int y)
{
    keys[(int)key] = 1.0f;

    switch(key)
    {
        case '*': 
            lookMode = !lookMode;
            if(lookMode) glutWarpPointer(width/2, height/2);
            break;
        case '+': cameraSpeed *= 2; break;
        case '-': cameraSpeed /= 2; break;
        case 'm': side = !side; break;
        case '0': hull = 0; break;
        case '1': hull = 1; break;
        case '2': hull = 2; break;
        case '3': hull = 3; break;
    }
}

void keyUp(unsigned char key, int x, int y)
{
    keys[(int)key] = 0.0f;
    //keydown(key);
}

int main(int argc, char **argv)
{
    bsp.read_bsp(argv[1]);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB | GLUT_MULTISAMPLE);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(width, height);
    glutCreateWindow("Half-Life Hull");

    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutReshapeFunc(reshape);
    glutPassiveMotionFunc(motion);
    glutIgnoreKeyRepeat(true);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);

    for(int i = 0; i < 256; i++) keys[i] = false;

    glutSetOption(GLUT_MULTISAMPLE, 8);
    glEnable(GL_MULTISAMPLE);
    glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);

    glClearColor(0.7, 0.7, 1, 0);

    updateCamera();

    glutMainLoop();

    return 0;
}