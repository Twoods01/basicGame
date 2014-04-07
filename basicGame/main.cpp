/*
 *  CPE 471 Final Program - aMAZEing
 *  Generates a random maze for the user to navigate.
 *
 *  Created by Taylor Woods on 2/20/14
 *
 *****************************************************************************/
#ifdef __APPLE__
#include "GLUT/glut.h"
#include <OPENGL/gl.h>
#endif
#ifdef __unix__
#include <GL/glut.h>
#endif

#include "CMeshLoaderSimple.h"
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "GLSL_helper.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp" //perspective, trans etc
#include "glm/gtc/type_ptr.hpp" //value_ptr

#define INIT_WIDTH 600
#define INIT_HEIGHT 600
#define MAZE_HEIGHT 40
#define MAZE_WIDTH 40
#define pi 3.14159
#define WALL_COLLISION_SIZE .63

//Number of each model in the world
#define NUM_LIGHTS 2

//Material selection constants
#define GROUND_MAT 3

using namespace std;
unsigned int const StepSize = 100;

//GL basics
int ShadeProg;
static float g_width, g_height;

//Handles to the shader data
GLint h_aPosition;
GLint h_aNormal;
GLint h_uModelMatrix;
GLint h_uViewMatrix;
GLint h_uProjMatrix;
GLint h_uNormMatrix;
GLint h_uLightPos;
GLint h_uLightColor;
GLint h_uEyePos;
GLint h_uMatAmb, h_uMatDif, h_uMatSpec, h_uMatShine;

//Ground
GLuint GrndNormalBuffObj, GrndBuffObj, GIndxBuffObj, GrndTexBuffObj;
int g_GiboLen;
vector<glm::vec3> groundTiles;
static const float g_groundY = 0;

//Light
glm::vec3 lightPos[2];

//Camera
bool overheadView = false;
float overheadHeight = 5.0;
float firstPersonHeight = .5;
glm::vec3 eye = glm::vec3(-1, firstPersonHeight, -1);
glm::vec3 lookAt = glm::vec3(0, 0, 0);
glm::vec3 overheadEye = glm::vec3(-1, overheadHeight, -1);
glm::vec3 overheadLookAt = eye;
glm::vec3 upV = glm::vec3(0, 1, 0);
float pitch = -pi/4;
float yaw = pi/2;
//Save pitch and yaw when going to overheadView
float eyePitch;
float eyeYaw;

//User interaction
glm::vec2 prevMouseLoc;
bool cheatMode = false;

/* projection matrix */
void SetProjectionMatrix() {
   glm::mat4 Projection = glm::perspective(80.0f, (float)g_width/g_height, 0.1f, 100.f);
   safe_glUniformMatrix4fv(h_uProjMatrix, glm::value_ptr(Projection));
}

/* camera controls */
void SetView() {
   glm::mat4 view;
   if(overheadView)
      view = glm::lookAt(overheadEye, overheadLookAt, upV);
   else
      view = glm::lookAt(eye, lookAt, upV);
   safe_glUniformMatrix4fv(h_uViewMatrix, glm::value_ptr(view));
}

float randomFloat(float min, float max)
{
   return (max - min) * (rand() / (double) RAND_MAX) + min;
}

/* Initialization of objects in the world */
void setWorld()
{
   for(int i = 0; i < 10; i++)
   {
      groundTiles.push_back(glm::vec3(i, 0, 0));
   }
   lightPos[0] = glm::vec3(MAZE_WIDTH / 2, 5, 0);
   lightPos[1] = glm::vec3(0, 5, MAZE_HEIGHT / 2);
   
   //Send light data to shader
   glUniform1fv(h_uLightPos, NUM_LIGHTS, glm::value_ptr(lightPos[0]));
   safe_glUniform3f(h_uLightColor, 1, 1, 1);
}

/* Set up matrices to place model in the world */
void SetModel(glm::vec3 loc, glm::vec3 size, float rotation) {
   glm::mat4 Scale = glm::scale(glm::mat4(1.0f), size);
   glm::mat4 Trans = glm::translate(glm::mat4(1.0f), loc);
   glm::mat4 Rotate = glm::rotate(glm::mat4(1.0f), -90.0f, glm::vec3(1, 0, 0));
   Rotate *= glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0, 0, 1));
   
   glm::mat4 final = Trans * Rotate * Scale;
   safe_glUniformMatrix4fv(h_uModelMatrix, glm::value_ptr(final));
   safe_glUniformMatrix4fv(h_uNormMatrix, glm::value_ptr(glm::vec4(1.0f)));
}

/* Set up matrices for ground plane */
void setGround(glm::vec3 loc)
{
   glm::mat4 ctm = glm::translate(glm::mat4(1.0f), loc);
   safe_glUniformMatrix4fv(h_uModelMatrix, glm::value_ptr(ctm));
   safe_glUniformMatrix4fv(h_uNormMatrix, glm::value_ptr(glm::mat4(1.0f)));
}

/* Code to create a large ground plane,
 * represented by a list of vertices and a list of indices  */
static void initGround() {
   
   float GrndPos[] = {
      0, g_groundY, 0,
      0, g_groundY, 1,
      1, g_groundY, 1,
      1, g_groundY, 0,
   };
   
   unsigned short idx[] =
   {
      2, 1, 0,
      3, 2, 0,
   };
   
   float grndNorm[] =
   {
      0, 1, 0,
      0, 1, 0,
      0, 1, 0,
      0, 1, 0,
      0, 1, 0,
      0, 1, 0,
   };
   
   static GLfloat GrndTex[] = {
      0, 0,
      0, 1,
      1, 0,
      1, 1
   };
   
   g_GiboLen = 6;
   glGenBuffers(1, &GrndBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);
   
   glGenBuffers(1, &GIndxBuffObj);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
   
   glGenBuffers(1, &GrndNormalBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, GrndNormalBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(grndNorm), grndNorm, GL_STATIC_DRAW);
   
   glGenBuffers(1, &GrndTexBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);
}

/* Initialize the geometry */
void InitGeom() {
   initGround();
}

/*function to help load the shaders (both vertex and fragment */
int InstallShader(const GLchar *vShaderName, const GLchar *fShaderName) {
   GLuint VS; //handles to shader object
   GLuint FS; //handles to frag shader object
   GLint vCompiled, fCompiled, linked; //status of shader
   
   VS = glCreateShader(GL_VERTEX_SHADER);
   FS = glCreateShader(GL_FRAGMENT_SHADER);
   
   //load the source
   glShaderSource(VS, 1, &vShaderName, NULL);
   glShaderSource(FS, 1, &fShaderName, NULL);
   
   //compile shader and print log
   glCompileShader(VS);
   /* check shader status requires helper functions */
   printOpenGLError();
   glGetShaderiv(VS, GL_COMPILE_STATUS, &vCompiled);
   printShaderInfoLog(VS);
   
   //compile shader and print log
   glCompileShader(FS);
   /* check shader status requires helper functions */
   printOpenGLError();
   glGetShaderiv(FS, GL_COMPILE_STATUS, &fCompiled);
   printShaderInfoLog(FS);
   
   if (!vCompiled || !fCompiled) {
      printf("Error compiling either shader %s or %s", vShaderName, fShaderName);
      return 0;
   }
   
   //create a program object and attach the compiled shader
   ShadeProg = glCreateProgram();
   glAttachShader(ShadeProg, VS);
   glAttachShader(ShadeProg, FS);
   
   glLinkProgram(ShadeProg);
   /* check shader status requires helper functions */
   printOpenGLError();
   glGetProgramiv(ShadeProg, GL_LINK_STATUS, &linked);
   printProgramInfoLog(ShadeProg);
   
   glUseProgram(ShadeProg);
   
   /* get handles to attribute and uniform data in shader */
   h_aPosition = safe_glGetAttribLocation(ShadeProg, "aPosition");
   h_aNormal = safe_glGetAttribLocation(ShadeProg,	"aNormal");
   h_uProjMatrix = safe_glGetUniformLocation(ShadeProg, "uProjMatrix");
   h_uViewMatrix = safe_glGetUniformLocation(ShadeProg, "uViewMatrix");
   h_uModelMatrix = safe_glGetUniformLocation(ShadeProg, "uModelMatrix");
   h_uNormMatrix = safe_glGetUniformLocation(ShadeProg, "uNormalMatrix");
   h_uLightPos = safe_glGetUniformLocation(ShadeProg, "uLightPos");
   h_uLightColor = safe_glGetUniformLocation(ShadeProg, "uLColor");
   h_uEyePos = safe_glGetUniformLocation(ShadeProg, "uEyePos");
   h_uMatAmb = safe_glGetUniformLocation(ShadeProg, "uMat.aColor");
   h_uMatDif = safe_glGetUniformLocation(ShadeProg, "uMat.dColor");
   h_uMatSpec = safe_glGetUniformLocation(ShadeProg, "uMat.sColor");
   h_uMatShine = safe_glGetUniformLocation(ShadeProg, "uMat.shine");
   
   printf("sucessfully installed shader %d\n", ShadeProg);
   return 1;
}

/* helper function to set up material for shading */
void SetMaterial(int i) {
   
   glUseProgram(ShadeProg);
   switch (i) {
      case 0:
         safe_glUniform3f(h_uMatAmb, 0.2, 0.2, 0.2);
         safe_glUniform3f(h_uMatDif, 0.4, 0.4, 0.4);
         safe_glUniform3f(h_uMatSpec, 0.2, 0.2, 0.2);
         safe_glUniform1f(h_uMatShine, .2);
         break;
      case GROUND_MAT:
         safe_glUniform3f(h_uMatAmb, 0.1, 0.3, 0.1);
         safe_glUniform3f(h_uMatDif, 0.1, 0.3, 0.1);
         safe_glUniform3f(h_uMatSpec, 0.3, 0.3, 0.4);
         safe_glUniform1f(h_uMatShine, 1.0);
         break;
   }
}

/* Some OpenGL initialization */
void Initialize ()
{
	// Start Of User Initialization
	glClearColor (1.0f, 1.0f, 1.0f, 1.0f);
	// Black Background
 	glClearDepth (1.0f);	// Depth Buffer Setup
 	glDepthFunc (GL_LEQUAL);	// The Type Of Depth Testing
	glEnable (GL_DEPTH_TEST);// Enable Depth Testing
}

/* Main display function */
void Draw (void)
{
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//Start our shader
 	glUseProgram(ShadeProg);
   
   /* set up the projection and camera - do not change */
   SetProjectionMatrix();
   SetView();
   
   safe_glUniform3f(h_uEyePos, eye.x, eye.y, eye.z);
   //-------------------------------Ground Plane --------------------------
   safe_glEnableVertexAttribArray(h_aPosition);
   safe_glEnableVertexAttribArray(h_aNormal);
   SetMaterial(0);
   
   for (std::vector<glm::vec3>::iterator it = groundTiles.begin(); it != groundTiles.end(); ++ it) {
      setGround(glm::vec3(it->x, it->y, it->z));
      SetMaterial(GROUND_MAT);
      
      glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
      safe_glVertexAttribPointer(h_aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
      
      safe_glEnableVertexAttribArray(h_aNormal);
      glBindBuffer(GL_ARRAY_BUFFER, GrndNormalBuffObj);
      safe_glVertexAttribPointer(h_aNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
      
      glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);
   }
   
   //clean up
	safe_glDisableVertexAttribArray(h_aPosition);
	safe_glDisableVertexAttribArray(h_aNormal);
   
	//Disable the shader
	glUseProgram(0);
	glutSwapBuffers();
}

/* Reshape - note no scaling as perspective viewing*/
void ReshapeGL (int width, int height)
{
	g_width = (float)width;
	g_height = (float)height;
	glViewport (0, 0, (GLsizei)(width), (GLsizei)(height));
}

/* Convert pixel x coordinates into world space */
float p2wx(int in_x) {
   //fill in with the correct return value
   float l, r;
   float c, d;
   
   if(g_width > g_height)
   {
      l = -1 *(g_width) / (g_height);
      r = g_width / g_height;
   }
   else{
      l = -1;
      r = 1;
   }
   
   c = (1 - g_width) / (l - r);
   d = ((g_width - 1) / (l - r)) * l;
   
   return (in_x - d) / c;
}

/* Convert pixel y coordinates into world space */
float p2wy(int in_y) {
   float b, t;
   float e, f;
   //flip glut y
   in_y = g_height - in_y;
   
   if(g_width > g_height)
   {
      b = -1;
      t = 1;
   }
   else{
      b = -1 * (g_height) / (g_width);
      t = g_height / g_width;
   }
   e = (1 - g_height) / (b - t);
   f = ((g_height - 1) / (b - t)) * b;
   
   return (in_y - f) / e;
}

/* Records the mouse position upon click */
void mouseClick(int button, int state, int x, int y)
{
   if (button == GLUT_LEFT_BUTTON) {
      if (state == GLUT_DOWN) {
         prevMouseLoc.x = x;
         prevMouseLoc.y = y;
      }
   }
}

/* Tracks mouse movement for the camera */
void mouse(int x, int y)
{
   glm::vec2 currentPos = glm::vec2(x, y);
   glm::vec2 delta = currentPos - prevMouseLoc;
   
   pitch += delta.y * (pi / g_height);
   yaw += delta.x * (pi / g_width);
   
   if(pitch > (4*pi/9))
      pitch = 4*pi/9;
   else if(pitch < -(4*pi/9))
      pitch = -4*pi/9;
   
   if(!overheadView)
   {
      lookAt.x = cos(pitch) * cos(yaw) + eye.x;
      lookAt.y = sin(pitch) + eye.y;
      lookAt.z = cos(pitch)*(cos((pi/2) - yaw)) + eye.z;
   }
   else
   {
      overheadLookAt.x = cos(pitch) * cos(yaw) + overheadEye.x;
      overheadLookAt.y = sin(pitch) + overheadEye.y;
      overheadLookAt.z = cos(pitch)*(cos((pi/2) - yaw)) + overheadEye.z;
   }
   
   prevMouseLoc = currentPos;
   glutPostRedisplay();
}

/*Given a position and a distance from that position calculates
 *If that position would be in the world */
bool detectCollision(glm::vec3 eye, glm::vec3 delta)
{
   float moveX = eye.x + (.1) * delta.x;
   float moveZ = eye.z + (.1) * delta.z;
   
   //Check for collision with a wall
   /*
   for (std::vector<glm::vec3>::iterator it = wallPos.begin(); it != wallPos.end(); ++ it) {
      if ((moveX <= it->x + WALL_COLLISION_SIZE && moveX >= it->x - WALL_COLLISION_SIZE) &&
          (moveZ <= it->z + WALL_COLLISION_SIZE && moveZ >= it->z - WALL_COLLISION_SIZE)) {
         return true;
      }
   }
   */
   return false;
}

void move(glm::vec3 delta)
{
   //Don't move if there is a collision, unless we're in overheadView
   //In which case collision detection is ignored
   if(overheadView)
   {
      overheadEye.x += (.1) * delta.x;
      overheadEye.z += (.1) * delta.z;
      overheadLookAt.x += (.1) * delta.x;
      overheadLookAt.z += (.1) * delta.z;
   }
   else if(!detectCollision(eye, delta))
   {
      eye.x += (.1) * delta.x;
      eye.z += (.1) * delta.z;
      lookAt.x += (.1) * delta.x;
      lookAt.z += (.1) * delta.z;
   }
}

void Timer(int param)
{
   if(cheatMode)
   {
      glutPostRedisplay();
      glutTimerFunc(StepSize, Timer, 1);
   }
}

//the keyboard callback
void keyboard(unsigned char key, int x, int y ){
   glm::vec3 delta;
   switch( key ) {
      case 'w':
         if(!overheadView)
            delta = glm::normalize(lookAt - eye);
         else
            delta = glm::normalize(overheadLookAt - overheadEye);
         move(delta);
         break;
      case 's':
         if(!overheadView)
            delta = glm::normalize(-(lookAt - eye));
         else
            delta = glm::normalize(-(overheadLookAt - overheadEye));
         move(delta);
         break;
      case 'd':
         if(!overheadView)
            delta = glm::normalize(glm::cross(upV, -(lookAt - eye)));
         else
            delta = glm::normalize(glm::cross(upV, -(overheadLookAt - overheadEye)));
         move(delta);
         break;
      case 'a':
         if(!overheadView)
            delta = glm::normalize(glm::cross(upV, (lookAt - eye)));
         else
            delta = glm::normalize(glm::cross(upV, (overheadLookAt - overheadEye)));
         move(delta);
         break;
      case 'r':
         setWorld();
         break;
      case 'q': case 'Q' :
         exit( EXIT_SUCCESS );
         break;
      case ' ':
         overheadView = !overheadView;
         if(overheadView)
         {
            overheadEye.x = eye.x;
            overheadEye.z = eye.z;
            eyePitch = pitch;
            eyeYaw = yaw;
            pitch = -pi/2 + .1;
            overheadLookAt.x = eye.x;// cos(pitch) * cos(yaw) + eye.x;
            overheadLookAt.y = 0;
            overheadLookAt.z = eye.z;//cos(pitch)*(cos((pi/2) - yaw)) + eye.z;
         }
         else
         {
            pitch = eyePitch;
            yaw = eyeYaw;
         }
         SetView();
         break;
      case 'h':
         cheatMode = !cheatMode;
         break;
   }
   glutPostRedisplay();
}

void SpecialInput(int key, int x, int y)
{
   cout << key;
   
   glutPostRedisplay();
}

int main( int argc, char *argv[] )
{
   g_width = INIT_WIDTH;
   g_height = INIT_HEIGHT;
   
   glutInit( &argc, argv );
   glutInitWindowPosition( 20, 20 );
   glutInitWindowSize( g_width, g_height );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   glutCreateWindow("aMAZEing!");
   glutReshapeFunc( ReshapeGL );
   glutDisplayFunc( Draw );
   glutKeyboardFunc( keyboard );
   glutSpecialFunc(SpecialInput);
   glutMotionFunc(mouse);
   glutMouseFunc(mouseClick);
   glutTimerFunc(StepSize, Timer, 1);
   Initialize();
	
	//test the openGL version
	getGLversion();
	//install the shader
	if (!InstallShader(textFileRead((char *)"Phong_vert.glsl"), textFileRead((char *)"Phong_frag.glsl"))) {
		printf("Error installing shader!\n");
		return 0;
	}
   
	InitGeom();
   setWorld();
  	glutMainLoop();
   return 0;
}
