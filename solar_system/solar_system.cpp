#include <iostream>
#include <windows.h>
#include <gl/gl.h>
#include <gl/glut.h>
#include <stdlib.h>

typedef float point3[3];

const float PI = 3.14159265359f;

static GLfloat theta[] = { 0.0, 0.0, 0.0 }; // three rotation angles
static GLfloat viewer[] = { 0.0, 0.0, 10.0 }; // array containing x, y, z coordinates of the observer
static GLfloat pix2angle;     // pixel to degree conversion factor
static GLfloat thetaA = 0.0;   
static GLfloat phi = 0.0;   
GLfloat R = 30.0;




bool enablePlanetRotations = true;
bool enableOwnRotation = true;
bool enableDrawingOrbits = true;
bool enableTopView = true;
int currentPlanetView = 0;
float sunRadius = 2.5f;
float planetRadius[] = { 0.1f, 0.2f, 0.25f, 0.18f, 0.5f, 0.4f, 0.3f, 0.35f };
float planetDistance[] = { 4.0f, 7.0f, 10.0f, 13.0f, 16.0f, 19.0f, 22.0f, 25.0f };
float planetOrbitSpeed[] = { 0.05f, 0.035f, 0.025f, 0.018f, 0.01f, 0.0075f, 0.005f, 0.0025f };
float planetOwnRotationSpeed[8] = { 1.0f, 0.7f, 0.5f, 0.35f, 0.2f, 0.15f, 0.1f, 0.05f };  // object rotation angle
float planetRotationAngles[8] = { 0.0f };
float planetOwnRotationAngles[8] = { 0.0f };
GLuint sunTextureID;
GLuint planetTextureIDs[8];

static GLint status = 0;      // mouse button state
// 0 - no mouse button pressed
// 1 - left mouse button pressed
// 2 - right mouse button pressed
static int x_pos_old = 0;       // previous mouse cursor position
static int delta_x = 0;        // difference between current and previous mouse cursor position
static int y_pos_old = 0;
static int delta_y = 0;

void Axes(void)
{

	point3  x_min = { -5.0, 0.0, 0.0 };
	point3  x_max = { 5.0, 0.0, 0.0 };

	point3  y_min = { 0.0, -5.0, 0.0 }; 
	point3  y_max = { 0.0,  5.0, 0.0 };

	point3  z_min = { 0.0, 0.0, -5.0 }; 
	point3  z_max = { 0.0, 0.0,  5.0 };

	glColor3f(1.0f, 0.0f, 0.0f);  
	glBegin(GL_LINES); 
	glVertex3fv(x_min);
	glVertex3fv(x_max);
	glEnd();

	glColor3f(0.0f, 1.0f, 0.0f);  
	glBegin(GL_LINES); 

	glVertex3fv(y_min);
	glVertex3fv(y_max);
	glEnd();

	glColor3f(0.0f, 0.0f, 1.0f); 
	glBegin(GL_LINES);

	glVertex3fv(z_min);
	glVertex3fv(z_max);
	glEnd();

}

void Mouse(int btn, int state, int x, int y)
{


	if (btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		x_pos_old = x;   
		y_pos_old = y;
		status = 1;          
	}
	else if (btn == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		y_pos_old = y;
		status = 2;     
	}
	else
		status = 0;          
}

void Motion(GLsizei x, GLsizei y)
{
	const float sensitivity = 0.8f;

	delta_x = sensitivity * (x - x_pos_old);
	x_pos_old = x;
	delta_y = sensitivity * (y - y_pos_old);
	y_pos_old = y;

	if (!enableTopView) {

		glRotatef(delta_x * sensitivity, 1.0, 0.0, 0.0);
		glRotatef(delta_y * sensitivity, 0.0, 1.0, 0.0);
	}

	glutPostRedisplay();
}


GLbyte *LoadTGAImage(const char *FileName, GLint *ImWidth, GLint *ImHeight, GLint *ImComponents, GLenum *ImFormat)
{
#pragma pack(1)           
	typedef struct
	{
		GLbyte    idlength;
		GLbyte    colormaptype;
		GLbyte    datatypecode;
		unsigned short    colormapstart;
		unsigned short    colormaplength;
		unsigned char     colormapdepth;
		unsigned short    x_orgin;
		unsigned short    y_orgin;
		unsigned short    width;
		unsigned short    height;
		GLbyte    bitsperpixel;
		GLbyte    descriptor;
	}TGAHEADER;
#pragma pack(8)
	FILE *pFile;
	TGAHEADER tgaHeader;
	unsigned long lImageSize;
	short sDepth;
	GLbyte    *pbitsperpixel = NULL;
	*ImWidth = 0;
	*ImHeight = 0;
	*ImFormat = GL_BGR_EXT;
	*ImComponents = GL_RGB8;
	fopen_s(&pFile, FileName, "rb");
	if (pFile == NULL)
		return NULL;
	fread(&tgaHeader, sizeof(TGAHEADER), 1, pFile);
	*ImWidth = tgaHeader.width;
	*ImHeight = tgaHeader.height;
	sDepth = tgaHeader.bitsperpixel / 8;
	if (tgaHeader.bitsperpixel != 8 && tgaHeader.bitsperpixel != 24 && tgaHeader.bitsperpixel != 32)
		return NULL;
	lImageSize = tgaHeader.width * tgaHeader.height * sDepth;
	pbitsperpixel = (GLbyte*)malloc(lImageSize * sizeof(GLbyte));
	if (pbitsperpixel == NULL)
		return NULL;
	if (fread(pbitsperpixel, lImageSize, 1, pFile) != 1)
	{
		free(pbitsperpixel);
		return NULL;
	}
	switch (sDepth)
	{
	case 3:
		*ImFormat = GL_BGR_EXT;
		*ImComponents = GL_RGB8;
		break;
	case 4:
		*ImFormat = GL_BGRA_EXT;
		*ImComponents = GL_RGBA8;
		break;
	case 1:
		*ImFormat = GL_LUMINANCE;
		*ImComponents = GL_LUMINANCE8;
		break;
	};
	fclose(pFile);
	return pbitsperpixel;
}


void drawOrbitLine(float radius) {
	const int numSegments = 100;
	glBegin(GL_LINE_LOOP);

	for (int i = 0; i < numSegments; ++i) {
		float theta = 2.0f * PI * static_cast<float>(i) / static_cast<float>(numSegments);
		float x = radius * cosf(theta);
		float z = radius * sinf(theta);
		glVertex3f(x, 0.0f, z);
	}

	glEnd();
}

GLuint loadTexture(const char* filename) {
	GLint width, height, components;
	GLenum format;
	GLbyte* imageData = LoadTGAImage(filename, &width, &height, &components, &format);

	GLuint textureID;
	glGenTextures(1, &textureID);

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, components, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	free(imageData);
	return textureID;
}


void drawSphereWithTexture(float radius, GLuint textureID) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureID);

	GLUquadricObj* quadric = gluNewQuadric();
	gluQuadricTexture(quadric, GL_TRUE);
	gluQuadricNormals(quadric, GLU_SMOOTH);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	gluSphere(quadric, radius, 50, 50);

	glDisable(GL_CULL_FACE);
	gluDeleteQuadric(quadric);
	glDisable(GL_TEXTURE_2D);
}



void drawPlanetWithTexture(float radius, float orbitRadius, float orbitSpeed, float ownRotationSpeed, GLuint textureID, int planetIndex) {
	if (enablePlanetRotations) {
		planetRotationAngles[planetIndex] += 0.05 * orbitSpeed;
		if (planetRotationAngles[planetIndex] > 360.0)
			planetRotationAngles[planetIndex] -= 360.0;
	}
	if (enableOwnRotation) {
		planetOwnRotationAngles[planetIndex] += ownRotationSpeed;
		if (planetOwnRotationAngles[planetIndex] > 360.0)
			planetOwnRotationAngles[planetIndex] -= 360.0;
	}

	glPushMatrix();
	glTranslatef(orbitRadius * cos(planetRotationAngles[planetIndex]), 0.0f, orbitRadius * sin(planetRotationAngles[planetIndex]));
	glRotatef(planetOwnRotationAngles[planetIndex], 0.0f, 1.0f, 0.0f);
	drawSphereWithTexture(radius, textureID);
	glPopMatrix();
}

void drawSunWithTexture(float radius, GLuint textureID) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureID);

	GLUquadricObj* quadric = gluNewQuadric();
	gluQuadricTexture(quadric, GL_TRUE);
	gluQuadricNormals(quadric, GLU_SMOOTH);

	gluSphere(quadric, radius, 50, 50);

	gluDeleteQuadric(quadric);
	glDisable(GL_TEXTURE_2D);
}

void switchPlanetView(int planetIndex) {
	float currentViewer = planetDistance[planetIndex] - planetRadius[planetIndex];
	float planetRotation = planetRotationAngles[planetIndex];

	float viewerX = currentViewer * cos(planetRotation);
	float viewerY = 0.0;
	float viewerZ = currentViewer * sin(planetRotation);

	viewer[0] = viewerX;
	viewer[1] = viewerY;
	viewer[2] = viewerZ;

	float lookAtX = 0.0;
	float lookAtY = 0.0;
	float lookAtZ = 0.0;

	float upX = 0.0;
	float upY = 1.0;
	float upZ = 0.0;

	gluLookAt(viewer[0], viewer[1], viewer[2], lookAtX, lookAtY, lookAtZ, upX, upY, upZ);

}



void initTextures() {
	const char* sunTextureFile = "sun.tga";
	sunTextureID = loadTexture(sunTextureFile);

	const char* planetTextures[] = {
		"mercury.tga", "venus.tga", "earth.tga", "mars.tga",
		"jupiter.tga", "saturn.tga", "uranus.tga", "neptune.tga"
	};

	for (int i = 0; i < 8; ++i) {
		planetTextureIDs[i] = loadTexture(planetTextures[i]);
	}
}

void MyInit(void)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Parameters for material
	GLfloat mat_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_shininess = 20.0;

	// Parameters for light
	GLfloat light_position[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat light_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };

	GLfloat att_constant = 1.0;
	GLfloat att_linear = 0.05;
	GLfloat att_quadratic = 0.001;

	// Set material properties
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess);

	// Set directional light source properties 
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, att_constant);
	glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, att_linear);
	glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, att_quadratic);

	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);

}

void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	if (status == 1) 
	{
		thetaA += delta_x * pix2angle / 10.0;
		phi += delta_y * pix2angle / 10.0;
	}
	else if (status == 2) 
	{
		R += delta_y * pix2angle / 10.0;
	}
	
	if (enableTopView) {
		viewer[0] = R * sin(thetaA) * cos(phi);
		viewer[1] = R * sin(phi);
		viewer[2] = R * cos(thetaA) * cos(phi);
		gluLookAt(viewer[0], viewer[1], viewer[2], 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	}
	else {

		switchPlanetView(currentPlanetView);
	}

	Axes();
	glRotatef(theta[0], 1.0, 0.0, 0.0);
	glRotatef(theta[1], 0.0, 1.0, 0.0);
	glRotatef(theta[2], 0.0, 0.0, 1.0);

	drawSunWithTexture(sunRadius, sunTextureID);


	for (int i = 0; i < 8; ++i) {
		if (enableDrawingOrbits)
			drawOrbitLine(planetDistance[i]);
		drawPlanetWithTexture(planetRadius[i], planetDistance[i], planetOrbitSpeed[i], planetOwnRotationSpeed[i], planetTextureIDs[i], i);
	}
	glutSwapBuffers();
}
void keys(unsigned char key, int x, int y) {
	switch (key) {
	case 'r':
		enablePlanetRotations = !enablePlanetRotations;
		break;
	case 'd':
		enableDrawingOrbits = !enableDrawingOrbits;
		break;
	case 's':
		enableOwnRotation = !enableOwnRotation;
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
		currentPlanetView = key - '1';  // Convert key to planet index (0-based)
		enableTopView = false;
		break;
	case '9':
		enableTopView = true;
		break;
	}

	if (enableTopView) {
		// Top view (looking at the sun from the top)
		viewer[0] = 0.0;
		viewer[1] = R;
		viewer[2] = 0.0;
		gluLookAt(viewer[0], viewer[1], viewer[2], 0.0, 0.0, 0.0, 0.0, 0.0, -1.0);
	}
	else {
		// Switch to planet view
		switchPlanetView(currentPlanetView);
	}
	display();
}



void ChangeSize(GLsizei horizontal, GLsizei vertical)
{
	pix2angle = 360.0 / (float)horizontal;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70, 1.0, 1.0, 100.0);

	if (horizontal <= vertical)
		glViewport(0, (vertical - horizontal) / 2, horizontal, horizontal);
	else
		glViewport((horizontal - vertical) / 2, 0, vertical, vertical);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


}

void idle() {
	glutPostRedisplay();
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow("Solar System Model");
	glEnable(GL_DEPTH_TEST);
	std::cout << "Control keys:\n";
	std::cout << "r -> Toggle planet rotations\n";
	std::cout << "s -> Toggle own rotations\n";
	std::cout << "d -> Toggle drawing orbits\n";
	std::cout << "1-8 -> Switch to planet view (1-8)\n";
	std::cout << "9 -> Switch to top view\n";
	std::cout << "LMB (left mouse button)  -> Rotate camera\n";
	std::cout << "RMB (right mouse button) -> Zoom in/out\n";

	initTextures();
	MyInit();
	glutDisplayFunc(display);
	glutReshapeFunc(ChangeSize);
	glutIdleFunc(idle);
	glutMouseFunc(Mouse);
	glutMotionFunc(Motion);
	glutKeyboardFunc(keys);
	glutMainLoop();

	return 0;
}