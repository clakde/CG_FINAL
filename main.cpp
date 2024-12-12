#include <GL/glut.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct AUX_RGBImageRec {
    int sizeX, sizeY;
    unsigned char* data;
};

AUX_RGBImageRec* LoadBMP(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open BMP file: " << filename << std::endl;
        return nullptr;
    }

    unsigned char header[54];
    file.read(reinterpret_cast<char*>(header), 54);

    if (header[0] != 'B' || header[1] != 'M') {
        std::cerr << "Error: Invalid BMP file: " << filename << std::endl;
        return nullptr;
    }

    int dataOffset = *(int*)&header[10];
    int width = *(int*)&header[18];
    int height = *(int*)&header[22];
    int dataSize = *(int*)&header[34];

    if (dataSize == 0) {
        dataSize = width * height * 3;
    }

    unsigned char* data = new unsigned char[dataSize];
    file.seekg(dataOffset, std::ios::beg);
    file.read(reinterpret_cast<char*>(data), dataSize);
    file.close();

    // BGR to RGB conversion
    for (int i = 0; i < dataSize; i += 3) {
        std::swap(data[i], data[i + 2]);
    }

    AUX_RGBImageRec* image = new AUX_RGBImageRec;
    image->sizeX = width;
    image->sizeY = height;
    image->data = data;

    return image;
}

GLuint texture;

void loadTexture(const char* filename) {
    AUX_RGBImageRec* image = LoadBMP(filename);
    if (!image) {
        std::cerr << "Failed to load texture." << std::endl;
        return;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->sizeX, image->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image->data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    delete[] image->data;
    delete image;
}

volatile float cameraX = 0.0f, cameraZ = 5.0f;
volatile float cameraYaw = 0.0f;
volatile float cameraSpeed = 0.2f;
volatile float sensitivity = 0.3f;
int lastMouseX = -1;

void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = { 10.0f, 10.0f, 10.0f, 1.0f }; // Light position
    GLfloat lightColor[] = { 0.4f, 0.4f, 0.3f, 0.5f }; // Light color
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightColor);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}

void drawShadow(float x, float z) {
    glPushMatrix();
    glTranslatef(x, -0.25f, z);
    glScalef(0.5f, 0.0f, 0.5f);
    glColor4f(0.1f, 0.1f, 0.1f, 0.7f); // Semi-transparent dark gray
    glutSolidSphere(0.5f, 20, 20);
    glPopMatrix();
}

void drawTree(float x, float z) {
    // Draw shadow first
    drawShadow(x, z);

    glPushMatrix();
    glTranslatef(x, 0.5f, z);
    glScalef(0.2f, 1.5f, 0.2f);
    glColor3f(0.55f, 0.27f, 0.07f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(x, 1.5f, z);
    glColor3f(0.0f, 0.8f, 0.0f);
    glutSolidSphere(0.5f, 20, 20);
    glPopMatrix();
}

void drawPath() {
    glPushMatrix();
    glTranslatef(0.0f, -0.45f, 0.0f);
    glScalef(2.0f, 0.1f, 100.0f);
    glColor3f(0.6f, 0.6f, 0.6f);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawGround() {
    glPushMatrix();
    glTranslatef(0.0f, -0.5f, 0.0f);
    glScalef(200.0f, 0.1f, 200.0f);
    glColor3f(0.3f, 0.7f, 0.3f);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawSign() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glPushMatrix();
    glTranslatef(3.0f, 1.0f, -20.0f);
    glScalef(2.0f, 1.0f, 0.1f);

    glColor3f(0.87f, 0.72f, 0.53f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.0f);
    glEnd();

    glPopMatrix();

    glPushMatrix();
    glTranslatef(3.0f, 0.0f, -20.0f);
    glScalef(0.1f, 1.0f, 0.1f);
    glColor3f(0.55f, 0.27f, 0.07f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
}

void display() {
    glClearColor(0.53f, 0.8f, 0.92f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float cameraLookX = sin(cameraYaw);
    float cameraLookZ = -cos(cameraYaw);

    gluLookAt(cameraX, 1.0f, cameraZ, cameraX + cameraLookX, 1.0f, cameraZ + cameraLookZ, 0.0f, 1.0f, 0.0f);

    drawGround();
    drawPath();

    for (int i = 0; i < 3; ++i) {
        drawTree(-1.0f, -10.0f * (i + 1));
        drawTree(1.0f, -10.0f * (i + 1));
    }

    drawSign();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    float cameraLookX = sin(cameraYaw);
    float cameraLookZ = -cos(cameraYaw);

    switch (key) {
    case 'w':
        cameraX += cameraLookX * cameraSpeed;
        cameraZ += cameraLookZ * cameraSpeed;
        break;
    case 's':
        cameraX -= cameraLookX * cameraSpeed;
        cameraZ -= cameraLookZ * cameraSpeed;
        break;
    case 'd':
        cameraX -= cameraLookZ * cameraSpeed;
        cameraZ += cameraLookX * cameraSpeed;
        break;
    case 'a':
        cameraX += cameraLookZ * cameraSpeed;
        cameraZ -= cameraLookX * cameraSpeed;
        break;
    case 'h':
        std::cout << "===  도움말  ===:\n";
        std::cout << "W: 앞으로 이동\n";
        std::cout << "S: 뒤로 이동\n";
        std::cout << "A: 왼쪽으로 이동\n";
        std::cout << "D: 오른쪽으로 이동\n";
        std::cout << "Mouse: 시점 회전\n";
        std::cout << "H: 도움말 호출\n";
        std::cout << "Q or Esc: 프로그램 종료\n";
        break;
    case 'q':
    case 'Q':
    case 27:
        exit(0);
        break;

    }
    glutPostRedisplay();
}

void mouseMotion(int x, int y) {
    if (lastMouseX == -1) {
        lastMouseX = x;
        return;
    }

    int dx = x - lastMouseX;
    cameraYaw += dx * sensitivity * (M_PI / 180.0f);

    lastMouseX = x;

    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Park Walk Simulation with Shadows");

    glEnable(GL_DEPTH_TEST);

    setupLighting();
    loadTexture("sign.bmp");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(mouseMotion);

    glutMainLoop();
    return 0;
}
