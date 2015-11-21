/****************************************************************************

M-Parser

-----------------------------------------------------------------------

18/11/15 Paul Rademacher (song0119@e.ntu.edu.sg)

****************************************************************************/

#define _CRT_SECURE_NO_DEPRECATE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <sstream>
#include <GL/glui.h>

float xy_aspect;
int   last_x, last_y;
float rotationX = 0.0, rotationY = 0.0;
static int xform_mode = 0;

/** These are the live variables passed into GLUI ***/
int   wireframe = 0;
int   obj_type = 3;
int   load_type = 4;
int   num_verts = 0;
int   num_face = 0;
int   light0_enabled = 1;
int   light1_enabled = 1;
float light0_intensity = 1.0;
float light1_intensity = .4;
int   main_window;
double scale = 0.10;
int   show_axes = 1;
int   show_grid = 1;
float view_rotate[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
float obj_pos[] = { 0.0, 0.0, 0.0 };
int   curr_string = 0;
char ch = '1';

struct HE_vert;
struct HE_face;
struct HE_edge {
	HE_vert* vert; // starting vertex
	HE_vert* evert; // ending vertex
	HE_edge* pair; // oppositely oriented half-edge
	HE_face* face; // the incident face
	HE_edge* prev; // previous half-edge around the face
	HE_edge* next; // next half-edge around the face
	BOOLEAN paired = false;
};
struct HE_vert {
	float x, y, z; // the vertex coordinates
	float nx, ny, nz; // normal vector
	HE_edge* edge; // one of the half-edges emanating from the vertex
	int nf = 0; //number of faces contain this vert
	HE_vert(float ax, float ay, float az) : x(ax), y(ay), z(az) {}
};
struct HE_face {
	HE_edge* edge; // one of the half-edges bordering the face
};
HE_vert *verts[300000];
HE_face *faces[100000];
HE_edge *edges[300000];

/** Pointers to the windows and some of the controls we'll create **/
GLUI *glui, *glui2;
GLUI_Spinner    *light0_spinner, *light1_spinner;
GLUI_RadioGroup *radio;
GLUI_Panel      *obj_panel;
GLUI_FileBrowser *fb;

/********** User IDs for callbacks ********/
#define LIGHT0_ENABLED_ID    200
#define LIGHT1_ENABLED_ID    201
#define LIGHT0_INTENSITY_ID  250
#define LIGHT1_INTENSITY_ID  260
#define SHOW_ID              302
#define HIDE_ID              303

#define TRANSFORM_NONE    0 
#define TRANSFORM_ROTATE  1
#define TRANSFORM_SCALE 2 


/********** Miscellaneous global variables **********/

GLfloat light0_ambient[] = { 0.1f, 0.1f, 0.3f, 1.0f };
GLfloat light0_diffuse[] = { .6f, .6f, 1.0f, 1.0f };
GLfloat light0_position[] = { .5f, .5f, 1.0f, 0.0f };

GLfloat light1_ambient[] = { 0.1f, 0.1f, 0.3f, 1.0f };
GLfloat light1_diffuse[] = { .9f, .6f, 0.0f, 1.0f };
GLfloat light1_position[] = { -1.0f, -1.0f, 1.0f, 0.0f };

GLfloat lights_rotation[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

/**************************************** control_cb() *******************/
/* GLUI control callback                                                 */
void loadObj(char *fname)
{
	num_verts = 0;
	num_face = 0;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	FILE *fp;
	int read, num;
	GLfloat x, y, z;
	char ch[10];
	char buffer[80];
	fp = fopen(fname, "r");
	if (!fp)
	{
		printf("can't open file %s\n", fname);
		exit(1);
	}

	fgets(buffer, 100, fp);
	fgets(buffer, 100, fp);
	fgets(buffer, 100, fp);
	fgets(buffer, 100, fp);
	fgets(buffer, 100, fp);
	printf("Progress1: Loading Model Data...(1/3)\n");
	while (!(feof(fp)))
	{
		read = fscanf(fp, "%s %d %f %f %f\n", &ch, &num, &x, &y, &z);
		if (ch[0] == 'V')
		{
			num_verts = num_verts + 1;
			verts[num - 1] = new HE_vert(x, y, z);
		}

		if (ch[0] == 'F')
		{
			int a = (int)x;
			int b = (int)y;
			int c = (int)z;
			num_face = num_face + 1;
			faces[num - 1] = new HE_face;
			edges[3 * (num - 1)] = new HE_edge;
			edges[3 * (num - 1)+1] = new HE_edge;
			edges[3 * (num - 1)+2] = new HE_edge;
					edges[3 * (num - 1)]->vert = verts[a - 1];
					edges[3 * (num - 1)]->evert = verts[b - 1];
					verts[a - 1]->edge = edges[3 * (num - 1)];
					edges[3 * (num - 1)]->prev = edges[3 * (num - 1) + 2];
					edges[3 * (num - 1)]->next = edges[3 * (num - 1) + 1];
					edges[3 * (num - 1) + 1]->vert = verts[b - 1];
					edges[3 * (num - 1) + 1]->evert = verts[c - 1];
					verts[b - 1]->edge = edges[3 * (num - 1) + 1];
					edges[3 * (num - 1) + 1]->prev = edges[3 * (num - 1) + 0];
					edges[3 * (num - 1) + 1]->next = edges[3 * (num - 1) + 2];
					edges[3 * (num - 1) + 2]->vert = verts[c - 1];
					edges[3 * (num - 1) + 2]->evert = verts[a - 1];
					verts[c - 1]->edge = edges[3 * (num - 1) + 2];
					edges[3 * (num - 1) + 2]->prev = edges[3 * (num - 1) + 1];
					edges[3 * (num - 1) + 2]->next = edges[3 * (num - 1) + 0];
			edges[3 * (num - 1)]->face = faces[num - 1];
			edges[3 * (num - 1) + 1]->face = faces[num - 1];
			edges[3 * (num - 1) + 2]->face = faces[num - 1];
			verts[a - 1]->nf = verts[a - 1]->nf + 1;
			verts[b - 1]->nf = verts[b - 1]->nf + 1;
			verts[c - 1]->nf = verts[c - 1]->nf + 1;
			faces[num - 1]->edge = edges[3 * (num - 1)];
		}
	}
	fclose(fp);
	int num_edge = 3 * num_face;
	printf("Progress2: Anlyse Model...(2/3)\n");
	for (int i = 0; i < num_edge; ++i){
		if (edges[i]->paired == false){
			for (int j = i + 1; j < num_edge; ++j){
				if (edges[i]->vert == edges[j]->evert && edges[i]->evert == edges[j]->vert){
					edges[i]->pair = edges[j];
					edges[i]->paired = true;
					edges[j]->pair = edges[i];
					edges[j]->paired = true;
					break;
				}
			}
		}
	}
	printf("Progress3: Rendering...(3/3)\n");
	for (int i = 0; i < num_verts; ++i){
		HE_edge* outgoing_he = verts[i]->edge;
		float a = outgoing_he->vert->x;
		float b = outgoing_he->vert->y;
		float c = outgoing_he->vert->z;
		if (outgoing_he->paired){
			HE_edge* curr = outgoing_he;
			float x = curr->pair->vert->x;
			float y = curr->pair->vert->y;
			float z = curr->pair->vert->z;

			float a1 = x - a;
			float b1 = y - b;
			float c1 = z - c;
			float m = a1;
			float l = b1;
			float n = c1;
			float n1 = 0;
			float n2 = 0;
			float n3 = 0;
			int n_f = verts[i]->nf;
			for (int j = 0; j < n_f; ++j)
			{
				curr = curr->pair->next;

				if (curr->paired){
					x = curr->pair->vert->x;
					y = curr->pair->vert->y;
					z = curr->pair->vert->z;

					float a2 = x - a;
					float b2 = y - b;
					float c2 = z - c;

					n1 = n1 + a1*a2;
					n2 = n3 + b1*b2;
					n3 = n3 + c1*c2;

					a1 = a2;
					b1 = b2;
					c1 = c2;
				}
				else
					break;
			}
			n1 = n1 + a1*m;
			n2 = n3 + b1*l;
			n3 = n3 + c1*n;
			verts[i]->nx = n1 / verts[i]->nf;
			verts[i]->ny = n2 / verts[i]->nf;
			verts[i]->nz = n3 / verts[i]->nf;
		}
		else{
			verts[i]->nx = 0;
			verts[i]->ny = 0;
			verts[i]->nz = 0;
		}
	}
}

void control_cb(int control)
{
	GLUI_String text;
	std::string file_name;
	glutPostRedisplay();
	if (control == 100) {
		if (load_type==0)
		{
			loadObj("bimba.m");
		}
		if (load_type == 1)
		{
			loadObj("bottle.m");
		}
		if (load_type == 2)
		{
			loadObj("bunny.m");
		}
		if (load_type == 3)
		{
			loadObj("cap.m");
		}
		if (load_type == 4)
		{
			loadObj("eight.m");;
		}
		if (load_type == 5)
		{
			loadObj("Gargoyle.m");
		}
		if (load_type == 6)
		{
			loadObj("knot.m");;
		}
		if (load_type == 7)
		{
			loadObj("statute.m");
		}
	}
	if (control == LIGHT0_ENABLED_ID) {
		if (light0_enabled) {
			glEnable(GL_LIGHT0);
			light0_spinner->enable();
		}
		else {
			glDisable(GL_LIGHT0);
			light0_spinner->disable();
		}
	}
	else if (control == LIGHT1_ENABLED_ID) {
		if (light1_enabled) {
			glEnable(GL_LIGHT1);
			light1_spinner->enable();
		}
		else {
			glDisable(GL_LIGHT1);
			light1_spinner->disable();
		}
	}
	else if (control == LIGHT0_INTENSITY_ID) {
		float v[] = {
			light0_diffuse[0], light0_diffuse[1],
			light0_diffuse[2], light0_diffuse[3] };

		v[0] *= light0_intensity;
		v[1] *= light0_intensity;
		v[2] *= light0_intensity;

		glLightfv(GL_LIGHT0, GL_DIFFUSE, v);
	}
	else if (control == LIGHT1_INTENSITY_ID) {
		float v[] = {
			light1_diffuse[0], light1_diffuse[1],
			light1_diffuse[2], light1_diffuse[3] };

		v[0] *= light1_intensity;
		v[1] *= light1_intensity;
		v[2] *= light1_intensity;

		glLightfv(GL_LIGHT1, GL_DIFFUSE, v);
	}
	else if (control == SHOW_ID)
	{
		glui2->show();
	}
	else if (control == HIDE_ID)
	{
		glui2->hide();
	}
}

/**************************************** myGlutKeyboard() **********/

void myGlutKeyboard(unsigned char Key, int x, int y)
{
	switch (Key)
	{
	case 27:
	case 'q':
		exit(0);
		break;
	};

	glutPostRedisplay();
}


/***************************************** myGlutMenu() ***********/

void myGlutMenu(int value)
{
	myGlutKeyboard(value, 0, 0);
}


/***************************************** myGlutIdle() ***********/

void myGlutIdle(void)
{
	/* According to the GLUT specification, the current window is
	undefined during an idle callback.  So we need to explicitly change
	it if necessary */
	if (glutGetWindow() != main_window)
		glutSetWindow(main_window);

	/*  GLUI_Master.sync_live_all();  -- not needed - nothing to sync in this
	application  */

	glutPostRedisplay();
}

/***************************************** myGlutMouse() **********/

void myGlutMouse(int button, int button_state, int x, int y)
{
	if (button_state == GLUT_DOWN)
	{
		last_x = x; last_y = y;
		if (button == GLUT_LEFT_BUTTON)
			xform_mode = TRANSFORM_ROTATE;
		else if (button == GLUT_MIDDLE_BUTTON)
			xform_mode = TRANSFORM_SCALE;
	}
	else if (button_state == GLUT_UP)
	{
		xform_mode = TRANSFORM_NONE;
	}
}


/***************************************** myGlutMotion() **********/

void myGlutMotion(int x, int y)
{
	if (xform_mode == TRANSFORM_ROTATE)
	{
		rotationX += (float)(y - last_y);
		rotationY += (float)(x - last_x);

		last_x = x;
		last_y = y;
	}
	else if (xform_mode == TRANSFORM_SCALE)
	{
		double old_size = scale;

		scale *= (1 + (y - last_y) / 60.0);

		if (scale <0)
			scale = old_size;
		last_y = y;
	}

	// force the redraw function
	glutPostRedisplay();
}

/**************************************** myGlutReshape() *************/

void myGlutReshape(int x, int y)
{
	int tx, ty, tw, th;
	GLUI_Master.get_viewport_area(&tx, &ty, &tw, &th);
	glViewport(tx, ty, tw, th);

	xy_aspect = (float)tw / (float)th;

	glutPostRedisplay();
}


/************************************************** draw_axes() **********/
/* Disables lighting, then draws RGB axes                                */

void draw_axes(float scale)
{
	glDisable(GL_LIGHTING);

	glPushMatrix();
	glScalef(scale, scale, scale);

	glLineWidth(10.0f);
	glBegin(GL_LINES);

	glColor3f(1.0, 0.0, 0.0);
	glVertex3f(16.0f, 1.0f, 0.0);  glVertex3f(20, 5.0f, 0.0); /* Letter X */
	glVertex3f(16.0f, 5.0f, 0.0);  glVertex3f(20, 1.0f, 0.0);
	glVertex3f(0.0, 0.0, 0.0);  glVertex3f(20.0, 0.0, 0.0); /* X axis      */

	glColor3f(0.0, 1.0, 0.0);
	glVertex3f(3.0f, 16.0f, 0.0);  glVertex3f(3.0f, 18.0f, 0.0); /* Letter Y */
	glVertex3f(1.0f, 20.0, 0.0);  glVertex3f(3.0f, 18.0f, 0.0);
	glVertex3f(5.0f, 20.0, 0.0);  glVertex3f(3.0f, 18.0f, 0.0);
	glVertex3f(0.0, 0.0, 0.0);  glVertex3f(0.0, 20.0, 0.0); /* Y axis      */

	glColor3f(0.0, 0.0, 1.0);
	glVertex3f(1.0f, 0.0, 22.0f);  glVertex3f(5.0f, 0.0, 22.0f); /* Letter X */
	glVertex3f(5.0f, 0.0, 22.0f);  glVertex3f(1.0f, 0.0, 18.0f);
	glVertex3f(1.0f, 0.0, 18.0f);  glVertex3f(5.0f, 0.0, 18.0f);
	glVertex3f(0.0, 0.0, 0.0);  glVertex3f(0.0, 0.0, 20.0); /* Z axis    */
	glEnd();
	glLineWidth(1.0f);

	glPopMatrix();

	glEnable(GL_LIGHTING);
}


/***************************************** myGlutDisplay() *****************/

void myGlutDisplay(void)
{
	GLfloat m, l, n;
	glClearColor(.9f, .9f, .9f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-xy_aspect*.04, xy_aspect*.04, -.04, .04, .1, 15.0);

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
	glMultMatrixf(lights_rotation);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

	glLoadIdentity();
	glTranslatef(0.0, 0.0, -2.6f);
	glTranslatef(obj_pos[0], obj_pos[1], -obj_pos[2]);
	glRotatef(rotationY, 0.0, 1.0, 0.0);
	glRotatef(rotationX, 1.0, 0.0, 0.0);
	glMultMatrixf(view_rotate);
	glPointSize(10.0);

	glScalef(scale, scale, scale);

	/*** Now we render object. ***/
	if (obj_type == 0){
		for (int i = 0; i < num_verts; i++){
			m = verts[i]->x;
			l = verts[i]->y;
			n = verts[i]->z;
			glBegin(GL_POINTS);
			glColor3f(1.0, 1.0, 1.0); // use your favorite color
			glVertex3f(m, l, n);
			glEnd();
		}
	}
	if (obj_type == 1){
		for (int i = 0; i < num_face; i++){
			glBegin(GL_LINE_LOOP);
			m = faces[i]->edge->vert->x;
			l = faces[i]->edge->vert->y;
			n = faces[i]->edge->vert->z;
			glColor3f(1.0, 1.0, 1.0); // use your favorite color
			glVertex3f(m, l, n);
			m = faces[i]->edge->next->vert->x;
			l = faces[i]->edge->next->vert->y;
			n = faces[i]->edge->next->vert->z;
			glColor3f(1.0, 1.0, 1.0); // use your favorite color
			glVertex3f(m, l, n);
			m = faces[i]->edge->prev->vert->x;
			l = faces[i]->edge->prev->vert->y;
			n = faces[i]->edge->prev->vert->z;
			glColor3f(1.0, 1.0, 1.0); // use your favorite color
			glVertex3f(m, l, n);
			glEnd();
		}
	}
	if (obj_type == 2){
		glShadeModel(GL_FLAT);
		for (int i = 0; i < num_face; i++){
			glBegin(GL_TRIANGLES);
			m = faces[i]->edge->vert->x;
			l = faces[i]->edge->vert->y;
			n = faces[i]->edge->vert->z;
			glColor3f(1.0, 1.0, 1.0); // use your favorite color
			glNormal3f(faces[i]->edge->vert->nx, faces[i]->edge->vert->ny, faces[i]->edge->vert->nz);
			glVertex3f(m, l, n);
			m = faces[i]->edge->next->vert->x;
			l = faces[i]->edge->next->vert->y;
			n = faces[i]->edge->next->vert->z;
			glColor3f(1.0, 1.0, 1.0); // use your favorite color
			glNormal3f(faces[i]->edge->next->vert->nx, faces[i]->edge->next->vert->ny, faces[i]->edge->next->vert->nz);
			glVertex3f(m, l, n);
			m = faces[i]->edge->prev->vert->x;
			l = faces[i]->edge->prev->vert->y;
			n = faces[i]->edge->prev->vert->z;
			glColor3f(1.0, 1.0, 1.0); // use your favorite color
			glNormal3f(faces[i]->edge->prev->vert->nx, faces[i]->edge->prev->vert->ny, faces[i]->edge->prev->vert->nz);
			glVertex3f(m, l, n);
			glEnd();
		}
	}
	if (obj_type == 3){
		glShadeModel(GL_SMOOTH);
		for (int i = 0; i < num_face; i++){
			glBegin(GL_TRIANGLES);
			m = faces[i]->edge->vert->x;
			l = faces[i]->edge->vert->y;
			n = faces[i]->edge->vert->z;
			glColor3f(1.0, 1.0, 1.0); // use your favorite color
			glNormal3f(faces[i]->edge->vert->nx, faces[i]->edge->vert->ny, faces[i]->edge->vert->nz);
			glVertex3f(m, l, n);
			m = faces[i]->edge->next->vert->x;
			l = faces[i]->edge->next->vert->y;
			n = faces[i]->edge->next->vert->z;
			glColor3f(1.0, 1.0, 1.0); // use your favorite color
			glNormal3f(faces[i]->edge->next->vert->nx, faces[i]->edge->next->vert->ny, faces[i]->edge->next->vert->nz);
			glVertex3f(m, l, n);
			m = faces[i]->edge->prev->vert->x;
			l = faces[i]->edge->prev->vert->y;
			n = faces[i]->edge->prev->vert->z;
			glColor3f(1.0, 1.0, 1.0); // use your favorite color
			glNormal3f(faces[i]->edge->prev->vert->nx, faces[i]->edge->prev->vert->ny, faces[i]->edge->prev->vert->nz);
			glVertex3f(m, l, n);
			glEnd();
		}
	}

	if (show_axes)
		draw_axes(0.52f);
	if (show_grid)
	{
		glDisable(GL_LIGHTING);
		glBegin(GL_LINES);
		for (int i = 0; i <= 1000000; i += 100)
		{
			glColor3f(0.5, 0.5, 0.5);
			glVertex3f((float)(i/10-50000), -50000.0f, 0.0f);
			glVertex3f((float)(i/10-50000), 50000.0f, 0.0f);
			glVertex3f(-50000.0f, (float)(i/10-50000), 0.0f);
			glVertex3f(50000.0f, (float)(i/10-50000), 0.0f);
		}
		glEnd();
	}

	char *string_list[] = {"Right click to rotate. Press mouse middle button to scale."};
		glDisable(GL_LIGHTING);  /* Disable lighting while we render text */
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0, 100.0, 0.0, 100.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glColor3ub(0, 0, 0);
		glRasterPos2i(10, 10);

		/*  printf( "text: %s\n", text );              */

		/*** Render the live character array 'text' ***/
		int i;
		for (i = 0; i<(int)strlen(string_list[curr_string]); i++)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, string_list[curr_string][i]);

	glEnable(GL_LIGHTING);

	glutSwapBuffers();
}

/**************************************** main() ********************/

int main(int argc, char* argv[])
{
	/****************************************/
	/*   Initialize GLUT and create window  */
	/****************************************/
	loadObj("eight.m");
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(800, 700);

	main_window = glutCreateWindow("My Mesh Viewer");
	glutDisplayFunc(myGlutDisplay);
	GLUI_Master.set_glutReshapeFunc(myGlutReshape);
	GLUI_Master.set_glutKeyboardFunc(myGlutKeyboard);
	GLUI_Master.set_glutSpecialFunc(NULL);
	GLUI_Master.set_glutMouseFunc(myGlutMouse);
	glutMotionFunc(myGlutMotion);

	/****************************************/
	/*       Set up OpenGL lights           */
	/****************************************/

	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

	glEnable(GL_LIGHT1);
	glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, light1_position);

	/****************************************/
	/*          Enable z-buferring          */
	/****************************************/

	glEnable(GL_DEPTH_TEST);

	/****************************************/
	/*         Here's the GLUI code         */
	/****************************************/

	printf("GLUI version: %3.2f\n", GLUI_Master.get_version());

	/*** Create the side subwindow ***/
	glui = GLUI_Master.create_glui_subwindow(main_window,
		GLUI_SUBWINDOW_RIGHT);



	/***** Import *****/

	/******** Add some controls for lights ********/

	GLUI_Rollout *roll_lights = new GLUI_Rollout(glui, "Lights", false);

	GLUI_Panel *light0 = new GLUI_Panel(roll_lights, "Light 1");
	GLUI_Panel *light1 = new GLUI_Panel(roll_lights, "Light 2");

	new GLUI_Checkbox(light0, "Enabled", &light0_enabled,
		LIGHT0_ENABLED_ID, control_cb);
	light0_spinner =
		new GLUI_Spinner(light0, "Intensity:",
		&light0_intensity, LIGHT0_INTENSITY_ID,
		control_cb);
	light0_spinner->set_float_limits(0.0, 1.0);
	GLUI_Scrollbar *sb;
	sb = new GLUI_Scrollbar(light0, "Red", GLUI_SCROLL_HORIZONTAL,
		&light0_diffuse[0], LIGHT0_INTENSITY_ID, control_cb);
	sb->set_float_limits(0, 1);
	sb = new GLUI_Scrollbar(light0, "Green", GLUI_SCROLL_HORIZONTAL,
		&light0_diffuse[1], LIGHT0_INTENSITY_ID, control_cb);
	sb->set_float_limits(0, 1);
	sb = new GLUI_Scrollbar(light0, "Blue", GLUI_SCROLL_HORIZONTAL,
		&light0_diffuse[2], LIGHT0_INTENSITY_ID, control_cb);
	sb->set_float_limits(0, 1);
	new GLUI_Checkbox(light1, "Enabled", &light1_enabled,
		LIGHT1_ENABLED_ID, control_cb);
	light1_spinner =
		new GLUI_Spinner(light1, "Intensity:",
		&light1_intensity, LIGHT1_INTENSITY_ID,
		control_cb);
	light1_spinner->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(light1, "Red", GLUI_SCROLL_HORIZONTAL,
		&light1_diffuse[0], LIGHT1_INTENSITY_ID, control_cb);
	sb->set_float_limits(0, 1);
	sb = new GLUI_Scrollbar(light1, "Green", GLUI_SCROLL_HORIZONTAL,
		&light1_diffuse[1], LIGHT1_INTENSITY_ID, control_cb);
	sb->set_float_limits(0, 1);
	sb = new GLUI_Scrollbar(light1, "Blue", GLUI_SCROLL_HORIZONTAL,
		&light1_diffuse[2], LIGHT1_INTENSITY_ID, control_cb);
	sb->set_float_limits(0, 1);


	/*** Add another rollout ***/
	GLUI_Rollout *options = new GLUI_Rollout(glui, "Options", true);
	new GLUI_Checkbox(options, "Draw axes", &show_axes);
	new GLUI_Checkbox(options, "Draw grid", &show_grid);

	GLUI_Panel *obj_panel = new GLUI_Panel(glui, "Geometric Primitives");
	radio = new GLUI_RadioGroup(obj_panel, &obj_type, 4, control_cb);
	new GLUI_RadioButton(radio, "Points");
	new GLUI_RadioButton(radio, "wireframe");
	new GLUI_RadioButton(radio, "flat shading");
	new GLUI_RadioButton(radio, "smooth shading");

	GLUI_Panel *loado = new GLUI_Panel(glui, "Load Object");
	radio = new GLUI_RadioGroup(loado, &load_type, 100, control_cb);
	new GLUI_RadioButton(radio, "Bimbia");
	new GLUI_RadioButton(radio, "Bottle");
	new GLUI_RadioButton(radio, "Bunny");
	new GLUI_RadioButton(radio, "Cap");
	new GLUI_RadioButton(radio, "Eight");
	new GLUI_RadioButton(radio, "Gargoyle");
	new GLUI_RadioButton(radio, "Knot");
	new GLUI_RadioButton(radio, "Statute");

	/*** Disable/Enable buttons ***/
	new GLUI_Button(glui, "Hide", HIDE_ID, control_cb);
	new GLUI_Button(glui, "Show", SHOW_ID, control_cb);

	new GLUI_StaticText(glui, "");

	/****** A 'quit' button *****/
	new GLUI_Button(glui, "Quit", 0, (GLUI_Update_CB)exit);


	/**** Link windows to GLUI, and register idle callback ******/

	glui->set_main_gfx_window(main_window);


	/*** Create the bottom subwindow ***/
	glui2 = GLUI_Master.create_glui_subwindow(main_window,
		GLUI_SUBWINDOW_BOTTOM);
	glui2->set_main_gfx_window(main_window);

	GLUI_Rotation *view_rot = new GLUI_Rotation(glui2, "Objects", view_rotate);
	view_rot->set_spin(1.0);
	new GLUI_Column(glui2, false);
	new GLUI_Column(glui2, false);
	new GLUI_Column(glui2, false);
	GLUI_Rotation *lights_rot = new GLUI_Rotation(glui2, "Blue Light", lights_rotation);
	lights_rot->set_spin(.82);
	new GLUI_Column(glui2, false);
	GLUI_Translation *trans_xy =
		new GLUI_Translation(glui2, "Objects XY", GLUI_TRANSLATION_XY, obj_pos);
	trans_xy->set_speed(.005);
	new GLUI_Column(glui2, false);
	GLUI_Translation *trans_x =
		new GLUI_Translation(glui2, "Objects X", GLUI_TRANSLATION_X, obj_pos);
	trans_x->set_speed(.005);
	new GLUI_Column(glui2, false);
	GLUI_Translation *trans_y =
		new GLUI_Translation(glui2, "Objects Y", GLUI_TRANSLATION_Y, &obj_pos[1]);
	trans_y->set_speed(.005);
	new GLUI_Column(glui2, false);
	GLUI_Translation *trans_z =
		new GLUI_Translation(glui2, "Objects Z", GLUI_TRANSLATION_Z, &obj_pos[2]);
	trans_z->set_speed(.005);

#if 0
	/**** We register the idle callback with GLUI, *not* with GLUT ****/
	GLUI_Master.set_glutIdleFunc(myGlutIdle);
#endif

	/**** Regular GLUT main loop ****/

	glutMainLoop();

	return EXIT_SUCCESS;
}