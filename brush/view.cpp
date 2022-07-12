#include <GL/gl.h>
#include <GL/freeglut.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include "brush.hpp"

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
bool faceSide = false;
bool full = true;
bool wireframe = false;

Brush::Environment env;

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

namespace Brush
{

void drawFace(Face *face, int side, vec3 color)
{
    if(face->sub[side^1].plane)
    {
        for(int i = 0; i < 2; i++)
            drawFace(face->sub[side^1].faces[i], side, color);
        return;
    }

    if(face->sub[side^1].brush->contents == CONTENTS_SOLID)
    {
        return;
    }

    glColor3f(color.x, color.y, color.z);
    glBegin(GL_POLYGON);
    for(Link<vec3*>::Iterator edges = face->edges; !edges.end(); ++edges)
    {
        vec3 v = *edges.link->data;
        glVertex3f(v.x, v.y, v.z);
    }
    glEnd();

    glColor3f(0, 0, 0);
    glBegin(GL_LINES);
    for(Link<vec3*>::Iterator edges = face->edges; !edges.end(); ++edges)
    {
        vec3 v = *edges.link->data;
        glVertex3f(v.x, v.y, v.z);
        v = *edges.link->next->data;
        glVertex3f(v.x, v.y, v.z);
    }
    glEnd();
}

void drawBrush(Brush *brush)
{
    if(brush->sub.plane)
    {
        for(int i = 0; i < 2; i++)
            drawBrush(brush->sub.brushes[i]);
        return;
    }

    if(brush->contents != CONTENTS_SOLID) return;

    for(Link<SignedFace>::Iterator face = brush->faces; !face.end(); ++face)
    {
        Plane *plane = face.link->data.face->plane;
        if(!plane) continue;
        
        vec3 n = plane->normal;

        int side = face.link->data.side;
        if(side ^ faceSide)
        {
            glCullFace(GL_BACK);
        } 
        else
        {
            glCullFace(GL_FRONT);
            n = {-n.x, -n.y, -n.z};
        }

        n = { (n.x+1)/2, (n.y+1)/2, (n.z+1)/2 };

        drawFace(face.link->data.face, side, n);
    }

}

}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for(Brush::Environment::Model &m : env.models)
    {
        Brush::drawBrush(m.hulls[hull]);
    }

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

    updateCamera();
    glutPostRedisplay();
}

void reshape(int w, int h)
{
    if(h == 0) h = 1;
    float aspect = (float)w/h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(120, aspect, 1, 20000);
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
        case 'x': faceSide = !faceSide; break;
        case 'f': full = !full; break;
        case 'r': wireframe = !wireframe; break;
        case '1': hull = 0; break;
        case '2': hull = 1; break;
        case '3': hull = 2; break;
        case '4': hull = 3; break;
    }
}

void keyUp(unsigned char key, int x, int y)
{
    keys[(int)key] = 0.0f;
}

void mouse(int button, int state, int x, int y)
{
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        lookMode = !lookMode;
        if(lookMode) glutWarpPointer(width/2, height/2);
    }
}

int main(int argc, char *argv[])
{
    if(argc != 2) return 1;

    try
    {
        Brush::BspData bsp;
        if(!bsp.readBsp(argv[1])) return 1;
        Brush::processBsp(env, bsp);
    }
    catch(const char *err)
    {
        printf("%s\n", err);
        return 1;
    }


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
    glutMouseFunc(mouse);


    for(int i = 0; i < 256; i++) keys[i] = false;

    glutSetOption(GLUT_MULTISAMPLE, 8);
    glEnable(GL_MULTISAMPLE);
    glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glClearColor(0.7, 0.7, 1, 0);

    updateCamera();

    try
    {
        glutMainLoop();
    }
    catch(const char *err)
    {
        printf("%s\n", err);
    }

    return 0;
}