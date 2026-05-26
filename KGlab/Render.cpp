#include "Render.h"

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <random>
#include <cmath>
#include <cstring>
#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static GLuint texId = 0;
static bool textureLoaded = false;
static bool lightFromCamera = false;



void ComputeNormal(double* v1, double* v2, double* v3, double* normal);
void DrawNormalForQuad(double* v1, double* v2, double* v3, double* v4);

void InitLight();
void ToggleLightFromCamera();

void InitTexture();
void GetTextureCoords(double x, double y, double& u, double& v);
void TexturedVertex(const double* p);
void DrawTexturedTop();



void ComputeNormal(double* v1, double* v2, double* v3, double* normal) {
    double u[3] = {
        v2[0] - v1[0],
        v2[1] - v1[1],
        v2[2] - v1[2]
    };

    double v[3] = {
        v3[0] - v1[0],
        v3[1] - v1[1],
        v3[2] - v1[2]
    };

    normal[0] = u[1] * v[2] - u[2] * v[1];
    normal[1] = u[2] * v[0] - u[0] * v[2];
    normal[2] = u[0] * v[1] - u[1] * v[0];

    double len = sqrt(
        normal[0] * normal[0] +
        normal[1] * normal[1] +
        normal[2] * normal[2]
    );

    if (len != 0.0) {
        normal[0] /= len;
        normal[1] /= len;
        normal[2] /= len;
    }
}

void DrawNormalForQuad(double* v1, double* v2, double* v3, double* v4) {
    const double normalLength = 0.65;

    double center[3] = {
        (v1[0] + v2[0] + v3[0] + v4[0]) / 4.0,
        (v1[1] + v2[1] + v3[1] + v4[1]) / 4.0,
        (v1[2] + v2[2] + v3[2] + v4[2]) / 4.0
    };

    double normal[3];
    ComputeNormal(v1, v2, v3, normal);

    double endPoint[3] = {
        center[0] + normal[0] * normalLength,
        center[1] + normal[1] * normalLength,
        center[2] + normal[2] * normalLength
    };

    bool lightingEnabled = glIsEnabled(GL_LIGHTING);
    bool textureEnabled = glIsEnabled(GL_TEXTURE_2D);

    if (lightingEnabled) {
        glDisable(GL_LIGHTING);
    }

    if (textureEnabled) {
        glDisable(GL_TEXTURE_2D);
    }

    glLineWidth(2.0f);
    glColor3d(0.1, 0.35, 1.0);

    glBegin(GL_LINES);
    glVertex3dv(center);
    glVertex3dv(endPoint);
    glEnd();

    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glVertex3dv(endPoint);
    glEnd();

    if (textureEnabled) {
        glEnable(GL_TEXTURE_2D);
    }

    if (lightingEnabled) {
        glEnable(GL_LIGHTING);
    }
}



void InitLight() {
    GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_diffuse[] = { 0.9f, 0.9f, 0.9f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_position[] = { 5.0f, 10.0f, 10.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glShadeModel(GL_SMOOTH);

    glEnable(GL_NORMALIZE);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

void ToggleLightFromCamera() {
    lightFromCamera = !lightFromCamera;

    if (lightFromCamera) {
        GLfloat light_pos[] = { 0.0f, 0.0f, 1.0f, 0.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    }
    else {
        GLfloat light_pos[] = { 5.0f, 10.0f, 10.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    }
}



void InitTexture() {
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    int x, y, n;
    unsigned char* data = stbi_load("texture.png", &x, &y, &n, 4);

    if (data == nullptr) {
        printf("texture.png not found. Checker texture created.\n");

        const int W = 64;
        const int H = 64;
        unsigned char checker[W * H * 4];

        for (int j = 0; j < H; ++j) {
            for (int i = 0; i < W; ++i) {
                int cell = ((i / 8) + (j / 8)) % 2;
                int id = (j * W + i) * 4;

                checker[id + 0] = cell ? 255 : 30;
                checker[id + 1] = cell ? 255 : 30;
                checker[id + 2] = cell ? 255 : 30;
                checker[id + 3] = 255;
            }
        }

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            W,
            H,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            checker
        );

        textureLoaded = true;
    }
    else {
        textureLoaded = true;
        printf("Texture loaded: %dx%d, channels: %d\n", x, y, n);

        unsigned char* tmp = new unsigned char[x * 4];

        for (int i = 0; i < y / 2; ++i) {
            std::memcpy(tmp, data + i * x * 4, x * 4);
            std::memcpy(data + i * x * 4, data + (y - 1 - i) * x * 4, x * 4);
            std::memcpy(data + (y - 1 - i) * x * 4, tmp, x * 4);
        }

        delete[] tmp;

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            x,
            y,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            data
        );

        stbi_image_free(data);
    }

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void GetTextureCoords(double x, double y, double& u, double& v) {

    double x_min = -7.0;
    double x_max = 9.0;
    double y_min = -8.0;
    double y_max = 6.0;

    u = (x - x_min) / (x_max - x_min);
    v = (y - y_min) / (y_max - y_min);
}



void DrawTexturedTop() {
    double A1[]{ 1, -1, 5 };
    double B1[]{ 2, -8, 5 };
    double C1[]{ -5, -7, 5 };
    double D1[]{ -2,  0, 5 };
    double E1[]{ -7,  3, 5 };
    double P1[]{ 0,  1, 5 };
    double V1[]{ 2,  6, 5 };
    double F1[]{ 9,  2, 5 };

    bool lightingEnabled = glIsEnabled(GL_LIGHTING);

 
    if (lightingEnabled) {
        glDisable(GL_LIGHTING);
    }

    if (textureLoaded) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }

    glColor3d(1, 1, 1);
    glNormal3d(0, 0, 1);

    glBegin(GL_QUADS);

    TexturedVertex(D1);
    TexturedVertex(C1);
    TexturedVertex(B1);
    TexturedVertex(A1);

    TexturedVertex(P1);
    TexturedVertex(E1);
    TexturedVertex(D1);
    TexturedVertex(A1);

    TexturedVertex(F1);
    TexturedVertex(V1);
    TexturedVertex(P1);
    TexturedVertex(A1);

    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    if (lightingEnabled) {
        glEnable(GL_LIGHTING);
    }
}

void TexturedVertex(const double* p) {
    if (textureLoaded) {
        double u, v;
        GetTextureCoords(p[0], p[1], u, v);
        glTexCoord2d(u, v);
    }

    glVertex3dv(p);
}

void initRender() {
    InitLight();
    InitTexture();
}



void Render(double delta_time) {
   

    std::mt19937 gen(5);
    std::uniform_real_distribution<double> r(0, 1);

    double A[]{ 1, -1, 0 };
    double B[]{ 2, -8, 0 };
    double C[]{ -5, -7, 0 };
    double D[]{ -2,  0, 0 };
    double E[]{ -7,  3, 0 };
    double P[]{ 0,  1, 0 };
    double V[]{ 2,  6, 0 };
    double F[]{ 9,  2, 0 };

    double A1[]{ 1, -1, 5 };
    double B1[]{ 2, -8, 5 };
    double C1[]{ -5, -7, 5 };
    double D1[]{ -2,  0, 5 };
    double E1[]{ -7,  3, 5 };
    double P1[]{ 0,  1, 5 };
    double V1[]{ 2,  6, 5 };
    double F1[]{ 9,  2, 5 };

    double normal[3];

    
    glDisable(GL_TEXTURE_2D);

    glNormal3d(0, 0, -1);

    glBegin(GL_QUADS);

    glColor3d(r(gen), r(gen), r(gen));
    glVertex3dv(A);
    glVertex3dv(B);
    glVertex3dv(C);
    glVertex3dv(D);

    glColor3d(r(gen), r(gen), r(gen));
    glVertex3dv(A);
    glVertex3dv(D);
    glVertex3dv(E);
    glVertex3dv(P);

    glColor3d(r(gen), r(gen), r(gen));
    glVertex3dv(A);
    glVertex3dv(P);
    glVertex3dv(V);
    glVertex3dv(F);

    glEnd();

    DrawNormalForQuad(A, B, C, D);
    DrawNormalForQuad(A, D, E, P);
    DrawNormalForQuad(A, P, V, F);


    ComputeNormal(B, B1, C1, normal);
    glNormal3dv(normal);
    glBegin(GL_QUADS);
    glColor3d(r(gen), r(gen), r(gen));
    glVertex3dv(B);
    glVertex3dv(B1);
    glVertex3dv(C1);
    glVertex3dv(C);
    glEnd();
    DrawNormalForQuad(B, B1, C1, C);

    ComputeNormal(P, P1, E1, normal);
    glNormal3dv(normal);
    glBegin(GL_QUADS);
    glColor3d(r(gen), r(gen), r(gen));
    glVertex3dv(P);
    glVertex3dv(P1);
    glVertex3dv(E1);
    glVertex3dv(E);
    glEnd();
    DrawNormalForQuad(P, P1, E1, E);

    ComputeNormal(C, C1, D1, normal);
    glNormal3dv(normal);
    glBegin(GL_QUADS);
    glColor3d(r(gen), r(gen), r(gen));
    glVertex3dv(C);
    glVertex3dv(C1);
    glVertex3dv(D1);
    glVertex3dv(D);
    glEnd();
    DrawNormalForQuad(C, C1, D1, D);

    ComputeNormal(E, E1, D1, normal);
    glNormal3dv(normal);
    glBegin(GL_QUADS);
    glColor3d(r(gen), r(gen), r(gen));
    glVertex3dv(E);
    glVertex3dv(E1);
    glVertex3dv(D1);
    glVertex3dv(D);
    glEnd();
    DrawNormalForQuad(E, E1, D1, D);

    ComputeNormal(P, P1, V1, normal);
    glNormal3dv(normal);
    glBegin(GL_QUADS);
    glColor3d(r(gen), r(gen), r(gen));
    glVertex3dv(P);
    glVertex3dv(P1);
    glVertex3dv(V1);
    glVertex3dv(V);
    glEnd();
    DrawNormalForQuad(P, P1, V1, V);

    ComputeNormal(V, V1, F1, normal);
    glNormal3dv(normal);
    glBegin(GL_QUADS);
    glColor3d(r(gen), r(gen), r(gen));
    glVertex3dv(V);
    glVertex3dv(V1);
    glVertex3dv(F1);
    glVertex3dv(F);
    glEnd();
    DrawNormalForQuad(V, V1, F1, F);

    ComputeNormal(F, F1, A1, normal);
    glNormal3dv(normal);
    glBegin(GL_QUADS);
    glColor3d(r(gen), r(gen), r(gen));
    glVertex3dv(F);
    glVertex3dv(F1);
    glVertex3dv(A1);
    glVertex3dv(A);
    glEnd();
    DrawNormalForQuad(F, F1, A1, A);

    ComputeNormal(A, A1, B1, normal);
    glNormal3dv(normal);
    glBegin(GL_QUADS);
    glColor3d(r(gen), r(gen), r(gen));
    glVertex3dv(A);
    glVertex3dv(A1);
    glVertex3dv(B1);
    glVertex3dv(B);
    glEnd();
    DrawNormalForQuad(A, A1, B1, B);

 
    DrawTexturedTop();

    
    glDisable(GL_TEXTURE_2D);

    DrawNormalForQuad(D1, C1, B1, A1);
    DrawNormalForQuad(P1, E1, D1, A1);
    DrawNormalForQuad(F1, V1, P1, A1);
}