//3350
//program: PERDITION.cpp
//author:  GROUP 2
//date:    fall 2018
//
//
//Walk cycle using a sprite sheet.
//images courtesy: http://games.ucla.edu/resource/walk-cycles/
//framework courtesy: Gordon Giesel
//
//This program includes:
//  multiple sprite-sheet animations
//  a level tiling system
//  parallax scrolling of backgrounds
//
#include <stdio.h>
#include <iostream>
#include "charclass.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "log.h"
//#include "ppm.h"
#include "fonts.h"
#include </usr/include/AL/alut.h> //Sound Function Added
using namespace std;
//defined types
typedef double Flt;
typedef double Vec[3];
typedef Flt	Matrix[4][4];
typedef void (*WinResizeHandler)(int, int);
typedef struct t_mouse {
    int lbutton;
    int rbutton;
    int x;
    int y;
} Mouse;

//macros
#define rnd() (((double)rand())/(double)RAND_MAX)
#define random(a) (rand()%a)
#define MakeVector(v, x, y, z) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
#define VecDot(a,b)	((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VecSub(a,b,c) (c)[0]=(a)[0]-(b)[0]; \
			     (c)[1]=(a)[1]-(b)[1]; \
(c)[2]=(a)[2]-(b)[2]
#define SPACE_BAR 0x20
#define MENU_ANAHI // switches between menu implementations

//constants
const float timeslice = 1.0f;
const float gravity = -0.4f;
#define ALPHA 1

//function prototypes

bool push_start = false;
void initOpengl();
void checkMouse(XEvent *e);
int checkKeys(XEvent *e);
void init();
void physics();
void render();
//extern void functions
extern void init_sounds();
extern void sound_test();
extern void walking_sound();
extern void enemy_sound();
extern void jump_sound();
extern void close_sounds();
extern void music();
int i = 15;
int health = 100;
//-----------------------------------------------------------------------------
//Setup timers
class Timers {
	public:
		double physicsRate;
		double oobillion;
		struct timespec timeStart, timeEnd, timeCurrent;
		struct timespec walkTime;
		Timers() {
			physicsRate = 1.0 / 30.0;
			oobillion = 1.0 / 1e9;
		}
		double timeDiff(struct timespec *start, struct timespec *end) {
			return (double)(end->tv_sec - start->tv_sec ) +
				(double)(end->tv_nsec - start->tv_nsec) * oobillion;
		}
		void timeCopy(struct timespec *dest, struct timespec *source) {
			memcpy(dest, source, sizeof(struct timespec));
		}
		void recordTime(struct timespec *t) {
			clock_gettime(CLOCK_REALTIME, t);
		}

} timers;
//-----------------------------------------------------------------------------

class Image;

Enem *enemy1;
Enem *enemy2;
Body *player;
Fall *obj;
Fall *obj2;

class Sprite {
	public:
		int onoff;
		int frame;
		double delay;
		Vec pos;
		Image *image;
		GLuint tex;
		struct timespec time;
		Sprite() {
			onoff = 0;
			frame = 0;
			image = NULL;
			delay = 0.1;
		}
};

class Global {
	public:
		unsigned char keys[65536];
		int xres, yres;
		int mainMenu;
		int movie, movieStep;
		int walk;
		int credits;
		int walkFrame;
		int settings;
		int helpTab;
		int showRain;
		double delay;
		bool gameover;
		Image *walkImage;
		GLuint walkTexture;
		GLuint mariogm734Texture;
		GLuint tinaTexture;
		GLuint animeTexture;
		GLuint jeremyTexture;
		GLuint cactusTexture;
		GLuint enemy1Texture;
		GLuint goblinTexture;
		GLuint gameoverTexture;
		GLuint settings_icon_Texture;
		GLuint perditionTexture;
		GLuint bloodsplatterTexture;
		GLuint floorTexture;
		GLuint floorAngleTexture;
		GLuint barrierTexture;
		GLuint spikeballTexture;
		GLuint parachuteTexture;
		GLuint trophyTexture;
		Vec box[20];
		Sprite exp;
		Sprite exp44;
		Vec ball_pos;
		Vec ball_vel;
		//camera is centered at (0,0) lower-left of screen. 
		Flt camera[2];
		Mouse mouse;
		~Global() {
			logClose();
		}
		Global() {
			logOpen();
			showRain = 0;
			mainMenu = 1;
			movie=0;
			movieStep=2;
			xres=1600;
			yres=1300;
			walk=0;
			gameover = false;
			credits =0;
			settings = 0;
			walkFrame=0;
			walkImage=NULL;
			MakeVector(ball_pos, 520.0, 0, 0);
			MakeVector(ball_vel, 0, 0, 0);
			delay = 0.1;
			exp.onoff=0;
			exp.frame=0;
			exp.image=NULL;
			exp.delay = 0.02;
			exp44.onoff=0;
			exp44.frame=0;
			exp44.image=NULL;
			exp44.delay = 0.022;
			camera[0] = camera[1] = 0.0;
			for (int i=0; i<20; i++) {
				box[i][0] = rnd() * xres;
				box[i][1] = rnd() * (yres-220) + 220.0;
				box[i][2] = 0.0;
			}
			memset(keys, 0, 65536);
			//
		}
} gl;

class Raindrop {
	public:
		int type;
		int linewidth;
		Vec pos;
		Vec lastpos;
		Vec vel;
		Vec maxvel;
		Vec force;
		float length;
		float color[4];
		Raindrop *prev;
		Raindrop *next;
} *rainhad = NULL;
int ndrops = 1;
int totrain = 0;
int maxrain = 0;
void deleteRain(Raindrop *node);
void cleanupRaindrops(void);
void collisions(Body *);

class Level {
	public:
		unsigned char arr[16][80];
		int nrows, ncols;
		int tilesize[2];
		Flt ftsz[2];
		Flt tile_base;
		Level() {
			//Log("Level constructor\n");
			tilesize[0] = 32;
			tilesize[1] = 32;
			ftsz[0] = (Flt)tilesize[0];
			ftsz[1] = (Flt)tilesize[1];
			tile_base = 220.0;
			//read level
			FILE *fpi = fopen("level1.txt","r");
			if (fpi) {
				nrows=0;
				char line[100];
				while (fgets(line, 100, fpi) != NULL) {
					removeCrLf(line);
					int slen = strlen(line);
					ncols = slen;
					//Log("line: %s\n", line);
					for (int j=0; j<slen; j++) {
						arr[nrows][j] = line[j];
					}
					++nrows;
				}
				fclose(fpi);
				//printf("nrows of background data: %i\n", nrows);
			}
			for (int i=0; i<nrows; i++) {
				for (int j=0; j<ncols; j++) {
					printf("%c", arr[i][j]);
				}
				printf("\n");
			}
		}
		void removeCrLf(char *str) {
			//remove carriage return and linefeed from a Cstring
			char *p = str;
			while (*p) {
				if (*p == 10 || *p == 13) {
					*p = '\0';
					break;
				}
				++p;
			}
		}

} lev;

//X Windows variables
class X11_wrapper {

	private:
		Display *dpy;
		Window win;
		WinResizeHandler handler;
	public:
		~X11_wrapper() {
			XDestroyWindow(dpy, win);
			XCloseDisplay(dpy);
		}
		void setTitle() {
			//Set the window title bar.
			XMapWindow(dpy, win);
			XStoreName(dpy, win, "3350 - PERDITION");
		}
		void setupScreenRes(const int w, const int h) {
			gl.xres = w;
			gl.yres = h;
		}
		X11_wrapper() {
			GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
			//GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
			XSetWindowAttributes swa;
			setupScreenRes(gl.xres, gl.yres);
			dpy = XOpenDisplay(NULL);
			if (dpy == NULL) {
				printf("\n\tcannot connect to X server\n\n");
				exit(EXIT_FAILURE);
			}
			Window root = DefaultRootWindow(dpy);
			XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
			if (vi == NULL) {
				printf("\n\tno appropriate visual found\n\n");
				exit(EXIT_FAILURE);
			} 
			Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
			swa.colormap = cmap;
			swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
					ButtonPress | ButtonReleaseMask | PointerMotionMask |
					StructureNotifyMask | SubstructureNotifyMask;
			win = XCreateWindow(dpy, root, 0, 0, gl.xres, gl.yres, 0,
					vi->depth, InputOutput, vi->visual,
					CWColormap | CWEventMask, &swa);
			GLXContext glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
			glXMakeCurrent(dpy, win, glc);
			setTitle();
		}
		void reshapeWindow(int width, int height) {
			//window has been resized.
			setupScreenRes(width, height);
			glViewport(0, 0, (GLint)width, (GLint)height);
			glMatrixMode(GL_PROJECTION); glLoadIdentity();
			glMatrixMode(GL_MODELVIEW); glLoadIdentity();
			glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
			setTitle();
		}
		void checkResize(XEvent *e) {
			//The ConfigureNotify is sent by the
			//server if the window is resized.
			if (e->type != ConfigureNotify)
				return;
			XConfigureEvent xce = e->xconfigure;
			if (xce.width != gl.xres || xce.height != gl.yres) {
				//Window size did change.
				reshapeWindow(xce.width, xce.height);
				if (handler) {
					handler(xce.width, xce.height);
				}
			}
		}
		bool getXPending() {
			return XPending(dpy);
		}
		XEvent getXNextEvent() {
			XEvent e;
			XNextEvent(dpy, &e);
			return e;
		}
		void swapBuffers() {
			glXSwapBuffers(dpy, win);
		}
		void setWindowResizeHandler(WinResizeHandler handler) {
			this->handler = handler;
		}
} x11;

class Image {
	public:
		int width, height;
		unsigned char *data;
		~Image() { delete [] data; }
		Image(const char *fname) {
			if (fname[0] == '\0')
				return;
			//printf("fname **%s**\n", fname);
			int ppmFlag = 0;
			char name[40];
			strcpy(name, fname);
			int slen = strlen(name);
			char ppmname[80];
			if (strncmp(name+(slen-4), ".ppm", 4) == 0)
				ppmFlag = 1;
			if (ppmFlag) {
				strcpy(ppmname, name);
			} else {
				name[slen-4] = '\0';
				//printf("name **%s**\n", name);
				sprintf(ppmname,"%s.ppm", name);
				//printf("ppmname **%s**\n", ppmname);
				char ts[100];
				//system("convert eball.jpg eball.ppm");
				sprintf(ts, "convert %s %s", fname, ppmname);
				system(ts);
			}
			//sprintf(ts, "%s", name);
			//printf("read ppm **%s**\n", ppmname); fflush(stdout);
			FILE *fpi = fopen(ppmname, "r");
			if (fpi) {
				char line[200];
				fgets(line, 200, fpi);
				fgets(line, 200, fpi);
				//skip comments and blank lines
				while (line[0] == '#' || strlen(line) < 2)
					fgets(line, 200, fpi);
				sscanf(line, "%i %i", &width, &height);
				fgets(line, 200, fpi);
				//get pixel data
				int n = width * height * 3;			
				data = new unsigned char[n];			
				for (int i=0; i<n; i++)
					data[i] = fgetc(fpi);
				fclose(fpi);
			} else {
				printf("ERROR opening image: %s\n",ppmname);
				exit(0);
			}
			if (!ppmFlag)
				unlink(ppmname);
		}
};
Image img[19] = {
	"./images/walk.gif",
	"./images/exp.png",
	"./images/exp44.png",
	"./images/mariogm734.png",
	"./images/anime.png",
	"./images/jeremy.gif",
	"./images/tina.png",
	"./images/cactus.png",
	"./images/enemy1.png",
	"./images/goblin.png",
	"./images/settings_icon.png",
	"./images/perdition.png",
	"./images/gameover.png",
	"./images/floor.gif",
	"./images/floorAngle.gif",
	"./images/barrier.gif",
    "./images/pb.gif",
    "./images/parachute.gif",
	"./images/trophy.png"};




int main(void)
{
	initOpengl();
	init();
	srand(time(NULL));
	player = new Body();
	enemy1 = new Enem(200);
	enemy2 = new Enem(900);
	obj = new Fall[1];
	obj2 = new Fall[1];
	int done = 0;
	while (!done) {
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			x11.checkResize(&e);
			checkMouse(&e);
			done = checkKeys(&e);
		}
		collisions(player);
		render();
		extern void fallingObj(Fall &O, int px);
		extern bool collision(Body *p, Enem*e, bool &go);
		extern void moveEnemy(Enem *e);
		extern bool c_w_fo(Body *p, Fall &o, bool &go);
		//extern int groundCollision(Body *p);
		extern int barrierCollision(Body *p);
		//extern int groundCollision(Body *p, Floor *f);
		barrierCollision(player);
		//groundCollision(player, ground);
		moveEnemy(enemy1);
		moveEnemy(enemy2);
		if(rand() % 30 == 2)
		{
		fallingObj(obj[0], player->positionX);
		}	
		if(rand() % 40 == 2)
		{
		fallingObj(obj2[0], player->positionX-20);
		}
		c_w_fo(player, obj[0], gl.gameover);
		c_w_fo(player, obj2[0], gl.gameover);
		collision(player, enemy1, gl.gameover);
		collision(player, enemy2, gl.gameover);
		x11.swapBuffers();

    //cleanup_fonts();

  }
  close_sounds();
  return 0;
}

void onWindowResize(int width, int height) {
	Log("Window resized to %dx%d\n", width, height);
	gl.xres = width;
	gl.yres = height;
#ifdef MENU_ANAHI
	extern void calculateButtons();
	calculateButtons();
#endif
}

unsigned char *buildAlphaData(Image *img)
{
    //add 4th component to RGB stream...
    int i;
    unsigned char *newdata, *ptr;
    unsigned char *data = (unsigned char *)img->data;
    newdata = (unsigned char *)malloc(img->width * img->height * 4);
    ptr = newdata;
    unsigned char a,b,c;
    //use the first pixel in the image as the transparent color.
    unsigned char t0 = *(data+0);
    unsigned char t1 = *(data+1);
    unsigned char t2 = *(data+2);
    for (i=0; i<img->width * img->height * 3; i+=3) {
	a = *(data+0);
	b = *(data+1);
	c = *(data+2);
	*(ptr+0) = a;
	*(ptr+1) = b;
	*(ptr+2) = c;
	*(ptr+3) = 1;
	if (a==t0 && b==t1 && c==t2)
	    *(ptr+3) = 0;
	//-----------------------------------------------
	ptr += 4;
	data += 3;
    }
    return newdata;
}

void initOpengl(void)
{
	//-------------------------------------------------------------------------
	glGenTextures(1, &gl.mariogm734Texture);
	//-------------------------------------------------------------------------
	//mario texture
	//
	int w_mario = img[3].width;
	int h_mario = img[3].height;
	//
	glBindTexture(GL_TEXTURE_2D, gl.mariogm734Texture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w_mario, h_mario, 0,
			GL_RGB, GL_UNSIGNED_BYTE, img[3].data);
	//-------------------------------------------------------------------------
	//


	glGenTextures(1, &gl.animeTexture);
	//-------------------------------------------------------------------------
	//anime texture
	//
	int w_anime = img[4].width;
	int h_anime = img[4].height;
	//
	glBindTexture(GL_TEXTURE_2D, gl.animeTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w_anime, h_anime, 0,
			GL_RGB, GL_UNSIGNED_BYTE, img[4].data);
	//-------------------------------------------------------------------------
	//


	glGenTextures(1, &gl.jeremyTexture);
	//-------------------------------------------------------------------------
	//jeremy texture
	//
	int w_jeremy = img[5].width;
	int h_jeremy = img[5].height;
	//
	glBindTexture(GL_TEXTURE_2D, gl.jeremyTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w_jeremy, h_jeremy, 0,
			GL_RGB, GL_UNSIGNED_BYTE, img[5].data);
	//-------------------------------------------------------------------------


	//OpenGL initialization
	glGenTextures(1, &gl.tinaTexture);
	//-------------------------------------------------------------------------
	//tina texture
	//
	int w_tina = img[6].width;
	int h_tina  = img[6].height;
	//
	glBindTexture(GL_TEXTURE_2D, gl.tinaTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w_tina, h_tina, 0,
			GL_RGB, GL_UNSIGNED_BYTE, img[6].data);
	//-------------------------------------------------------------------------

	//helpTab
	//cactus texture
	//
	glGenTextures(1, &gl.cactusTexture);

	int w_cactus = img[7].width;
	int h_cactus = img[7].height;

	glBindTexture(GL_TEXTURE_2D, gl.cactusTexture);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w_cactus, h_cactus, 0,
			GL_RGB, GL_UNSIGNED_BYTE, img[7].data);
	//-------------------------------------------------------------------------


	glGenTextures(1, &gl.enemy1Texture);
	//-------------------------------------------------------------------------
	//enemy1 texture
	//
	int w_enemy1 = img[8].width;
	int h_enemy1 = img[8].height;
	//
	glBindTexture(GL_TEXTURE_2D, gl.enemy1Texture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w_enemy1, h_enemy1, 0,
			GL_RGB, GL_UNSIGNED_BYTE, img[8].data);
	
	
	//must build a new set of data...
	unsigned char *enemy1Data = buildAlphaData(&img[8]);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w_enemy1, h_enemy1, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, enemy1Data);
	free(enemy1Data);
    //-------------------------------------------------------------------------


	
	//-------------------------------------------------------------------------
	//goblin texture
	//
	glGenTextures(1, &gl.goblinTexture);
	int w_goblin = img[9].width;
	int h_goblin = img[9].height;
	//
	glBindTexture(GL_TEXTURE_2D, gl.goblinTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w_goblin, h_goblin, 0,
			GL_RGB, GL_UNSIGNED_BYTE, img[9].data);
	
	//must build a new set of data...
	unsigned char *goblinData = buildAlphaData(&img[9]);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w_goblin, h_goblin, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, goblinData);
	free(goblinData);
	
	//-------------------------------------------------------------------------	
	//settings icon texture
	glGenTextures(1, &gl.settings_icon_Texture);
	int w_settings_icon = img[10].width;
	int h_settings_icon  = img[10].height;
	
	glBindTexture(GL_TEXTURE_2D, gl.settings_icon_Texture);
	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	
	
	//must build a new set of data...
	unsigned char *iconData = buildAlphaData(&img[10]);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w_settings_icon, h_settings_icon, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, iconData);
	free(iconData);
    
	//-------------------------------------------------------------------------
	// barrier texture
	glGenTextures(1, &gl.barrierTexture);
	int w_barrier = img[15].width;
	int h_barrier  = img[15].height;
	glBindTexture(GL_TEXTURE_2D, gl.barrierTexture);	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *xBarrier = buildAlphaData(&img[15]);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	w_barrier, h_barrier,
	0, GL_RGBA, GL_UNSIGNED_BYTE, xBarrier);
	free(xBarrier);
	//-----------------------------------------------------------------------
	glGenTextures(1, &gl.parachuteTexture);
	int w_parachute = img[17].width;
	int h_parachute  = img[17].height;
	glBindTexture(GL_TEXTURE_2D, gl.parachuteTexture);	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *xParachute = buildAlphaData(&img[17]);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	w_parachute, h_parachute,
	0, GL_RGBA, GL_UNSIGNED_BYTE, xParachute);
	free(xParachute);
	//-------------------------------------------------------------------------
	// level texture
	glGenTextures(1, &gl.floorTexture);
	int w_floor = img[13].width;
	int h_floor  = img[13].height;
	glBindTexture(GL_TEXTURE_2D, gl.floorTexture);	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *xFloor = buildAlphaData(&img[13]);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	w_floor, h_floor,
	0, GL_RGBA, GL_UNSIGNED_BYTE, xFloor);
	free(xFloor);
	//------------------------------------------------------------------------
	// level texture angle
	glGenTextures(1, &gl.floorAngleTexture);
	int w_floorAngle = img[14].width;
	int h_floorAngle  = img[14].height;
	glBindTexture(GL_TEXTURE_2D, gl.floorAngleTexture);	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *xFloorAngle = buildAlphaData(&img[14]);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	w_floorAngle, h_floorAngle,
	0, GL_RGBA, GL_UNSIGNED_BYTE, xFloorAngle);
	free(xFloorAngle);
	//-----------------------------------------------------------------------
	// level texture
	glGenTextures(1, &gl.trophyTexture);
	int w_trophy = img[18].width;
	int h_trophy  = img[18].height;
	glBindTexture(GL_TEXTURE_2D, gl.trophyTexture);	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	unsigned char *xTrophy = buildAlphaData(&img[18]);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	w_trophy, h_trophy,
	0, GL_RGBA, GL_UNSIGNED_BYTE, xTrophy);
	free(xTrophy);

	//-----------------------------------------------------------------------
	//perdition texture - title screen

	glGenTextures(1, &gl.perditionTexture);
	//-------------------------------------------------------------------------
	int w_p = img[11].width;
	int h_p = img[11].height;
	//
	glBindTexture(GL_TEXTURE_2D, gl.perditionTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w_p, h_p, 0,
			GL_RGB, GL_UNSIGNED_BYTE, img[11].data);
	//--------------------------------------------------------------------------- 
	
	glGenTextures(1, &gl.bloodsplatterTexture);
	//-------------------------------------------------------------------------
	//gameover texture
	//
	int w_g = img[12].width;
	int h_g = img[12].height;
	//
	glBindTexture(GL_TEXTURE_2D, gl.gameoverTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w_g, h_g, 0,
			GL_RGB, GL_UNSIGNED_BYTE, img[12].data);
	//-------------------------------------------------------------------------

	glGenTextures(1, &gl.spikeballTexture);
	//-------------------------------------------------------------------------
	//
	int w_sb = img[16].width;
	int h_sb = img[16].height;
	//
	glBindTexture(GL_TEXTURE_2D, gl.spikeballTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w_sb, h_sb, 0,
			GL_RGB, GL_UNSIGNED_BYTE, img[16].data);
	//-------------------------------------------------------------------------
	
	glViewport(0, 0, gl.xres, gl.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//This sets 2D mode (no perspective)
	glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
	//
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glDisable(GL_CULL_FACE);
	//
	//Clear the screen
	glClearColor(1.0, 1.0, 1.0, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT);
	//Do this to allow fonts
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
	//
	//load the images file into a ppm structure.
	//
	int w = img[0].width;
	int h = img[0].height;
	//
	//create opengl texture elements
	glGenTextures(1, &gl.walkTexture);
	//-------------------------------------------------------------------------
	//silhouette
	//this is similar to a sprite graphic
	//
	glBindTexture(GL_TEXTURE_2D, gl.walkTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	//
	//must build a new set of data...
	unsigned char *walkData = buildAlphaData(&img[0]);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, walkData);
	free(walkData);
	//-------------------------------------------------------------------------
	//create opengl texture elements
	w = img[1].width;
	h = img[1].height;
	glGenTextures(1, &gl.exp.tex);
	//-------------------------------------------------------------------------
	//this is similar to a sprite graphic
	glBindTexture(GL_TEXTURE_2D, gl.exp.tex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	//must build a new set of data...
	unsigned char *xData = buildAlphaData(&img[1]);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, xData);
	free(xData);
	//-------------------------------------------------------------------------
	w = img[2].width;
	h = img[2].height;
	//create opengl texture elements
	glGenTextures(1, &gl.exp44.tex);
	//-------------------------------------------------------------------------
	//this is similar to a sprite graphic
	glBindTexture(GL_TEXTURE_2D, gl.exp44.tex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	//must build a new set of data...
	xData = buildAlphaData(&img[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, xData);
	free(xData);

}

void init() {
	x11.setWindowResizeHandler(onWindowResize);
    init_sounds();
}

void checkMouse(XEvent *e)
{
	//printf("checkMouse()...\n"); fflush(stdout);
	//Did the mouse move?
	//Was a mouse button clicked?
	static int savex = 0;
	static int savey = 0;
	//
	if (e->type != ButtonRelease && e->type != ButtonPress &&
			e->type != MotionNotify)
		return;
	if (e->type == ButtonRelease) {
		if (e->xbutton.button == 1) {
			//Left button is down
            gl.mouse.lbutton = 0;
        }
		if (e->xbutton.button == 3) {
            //Right button is down
            gl.mouse.rbutton = 0;
        }
		return;
	}
	if (e->type == ButtonPress) {

		if (e->xbutton.button==1) {
			//Left button is down
			gl.mouse.lbutton = 1;
			push_start = true;

		}
		if (e->xbutton.button==3) {
			//Right button is down
			gl.mouse.rbutton = 1;
			push_start = true;

		}
	}
	if (e->type == MotionNotify) {
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			//Mouse moved
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			gl.mouse.x = e->xbutton.x;
            gl.mouse.y = e->xbutton.y;
			Log("checkMouse(): gl.mouse.y -- %d\n", gl.mouse.y);
		}
	}

}

void screenCapture()
{
	static int fnum = 0;
	static int vid = 0;
	if (!vid) {
		system("mkdir ./vid");
		vid = 1;
	}
	unsigned char *data = (unsigned char *)malloc(gl.xres * gl.yres * 3);
	glReadPixels(0, 0, gl.xres, gl.yres, GL_RGB, GL_UNSIGNED_BYTE, data);
	char ts[32];
	sprintf(ts, "./vid/pic%03i.ppm", fnum);
	FILE *fpo = fopen(ts,"w");	
	if (fpo) {
		fprintf(fpo, "P6\n%i %i\n255\n", gl.xres, gl.yres);
		unsigned char *p = data;
		//go backwards a row at a time...
		p = p + ((gl.yres-1) * gl.xres * 3);
		unsigned char *start = p;
		for (int i=0; i<gl.yres; i++) {
			for (int j=0; j<gl.xres*3; j++) {
				fprintf(fpo, "%c",*p);
				++p;
			}
			start = start - (gl.xres*3);
			p = start;
		}
		fclose(fpo);
		char s[256];
		sprintf(s, "convert ./vid/pic%03i.ppm ./vid/pic%03i.gif", fnum, fnum);
		system(s);
		unlink(ts);
	}
	++fnum;
}

int checkKeys(XEvent *e)
{
	//keyboard input?
	static int shift=0;
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	gl.keys[key]=1;
	if (e->type == KeyRelease) {
		gl.keys[key]=0;
		if (key == XK_Shift_L || key == XK_Shift_R)
			shift=0;
		return 0;
	}
	gl.keys[key]=1;
	if (key == XK_Shift_L || key == XK_Shift_R) {
		shift=1;
		return 0;
	}
	(void)shift;
	switch (key) {
		case XK_s:
			//screenCapture();
			push_start = true;
			music();
			break;
		case XK_m:
			gl.movie ^= 1;
			break;
		case XK_w:
			timers.recordTime(&timers.walkTime);
			gl.walk ^= 1;

			/*
			   extern void Rcollision(int x, int y,Body *p, Enem *e, GLuint texid);

			   if (gl.keys[XK_w]) {
			   Rcollision(1600/2, 1300/2, player, enem, gl.bloodsplatterTexture);
			   }*/
			break;
		case XK_e:
			exit(0);
			break;
		case XK_f:
			gl.exp44.pos[0] = 200.0;
			gl.exp44.pos[1] = -60.0;
			gl.exp44.pos[2] =   0.0;
			timers.recordTime(&gl.exp44.time);
			gl.exp44.onoff ^= 1;
			break;
		case XK_n:
			sound_test();
			break;
		case XK_Left:
			walking_sound();
			player->positionX -= 5;
			break;
		case XK_Right:
			walking_sound();
			player->positionX += 5;
			break;
		case XK_Up:
			break;
		case XK_Down:
			break;
		case XK_equal:
			gl.delay -= 0.005;
			if (gl.delay < 0.005)
				gl.delay = 0.005;
			break;
		case XK_minus:
			gl.delay += 0.005;
			break;
		case XK_Escape:
			return 1;
			break;
		case XK_c:
			gl.credits ^= 1;
			break;
		case XK_h:
			gl.helpTab ^= 1;
			break;	
		case XK_space:
			jump_sound();
			//if spacebar is hit jump (?)
				if (gl.keys[XK_space]) {
					extern void jump(Body *p);
					extern void parachute(Body *p, GLuint textid);
					jump(player);
					parachute(player, gl.parachuteTexture);
				}
			break;
		case XK_a:
			//walking_sound();
			//player->positionX -= 5;
			break;
		case XK_d:
			//walking_sound();
			//player->positionX += 5;
			break;
	}
	return 0;
}

Flt VecNormalize(Vec vec)
{
	Flt len, tlen;
	Flt xlen = vec[0];
	Flt ylen = vec[1];
	Flt zlen = vec[2];
	len = xlen*xlen + ylen*ylen + zlen*zlen;
	if (len == 0.0) {
		MakeVector(vec, 0.0, 0.0, 1.0);
		return 1.0;
	}
	len = sqrt(len);
	tlen = 1.0 / len;
	vec[0] = xlen * tlen;
	vec[1] = ylen * tlen;
	vec[2] = zlen * tlen;
	return(len);
}

void collisions(Body *player)
{
	player->positionY += gravity;
	//test for the character staying on the screen
	if(player->positionY < 0)
	{
		player->positionY = 0;
	}

}
/*
   void cleanupRaindrops() {
   Raindrop *s;
   while(rainhead) {
   s = rainhead->next;
   free(rainhead);
   rainhead = s;
   }
   rainhead=NULL;
   }

   void deleteRain(Raindrop *node) {
   if (node->prev == NULL) {
   if (node->next == NULL) {
   rainhead = NULL;
   }
   } else {
   if (node->next == NULL) {
   node ->prev = NULL;
   } else {
   node->prev->next = node->next;
   node->next->prev = node->prev;
   }
   }
   free(node);
   node = NULL;
   }
   */

void render(void)
{	
#ifdef MENU_ANAHI
	if (gl.mainMenu) {
		extern void showMenu();
		showMenu();
	} else
#else
	if(!push_start)	{

		extern void menu(int x, int y);
		menu(100, gl.yres-155);
#endif
		if (gl.credits) {

			//display names
			extern void showFranciscoName(int x, int y);
			extern void showAnahiName(int x, int y);
			extern void showTheodoreName(int x, int y);
			extern void ShowArielleName(int x, int y);

			showFranciscoName(100, gl.yres-155);
			showAnahiName(100, gl.yres-175);
			showTheodoreName(100, gl.yres-105);
			ShowArielleName(100, gl.yres-135);

			//displays images
			extern void showAnahiPicture(int x, int y, GLuint texid);
			extern void showFranciscoPicture(int x, int y, GLuint texid);
			extern void showTheodorePicture(int x, int y, GLuint texid);
			extern void showAriellePic(int x, int y, GLuint texid);

			showAnahiPicture(250, gl.yres-475, gl.tinaTexture);
			showFranciscoPicture(250, gl.yres-350, gl.jeremyTexture);
			showTheodorePicture(250, gl.yres-100, gl.mariogm734Texture);
			showAriellePic(250, gl.yres-220, gl.animeTexture);
			
			extern void credits();
			credits();

			return;
		}
#ifndef MENU_ANAHI
	} 
#endif
	else if (gl.gameover == false) {
		Rect r;
		//Clear the screen
		glClearColor(0.1, 0.1, 0.1, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		
		extern void showBackground(int x, int y, GLuint texid);
		showBackground(1600/2, 1300/2, gl.perditionTexture);
		//this is for the enemy1
		glPushMatrix();
		glColor3f(1.0, 1.0, 1.0);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.0f);
		glTranslatef(enemy1->posX, enemy1->posY+115, 0.0f);
		glBindTexture(GL_TEXTURE_2D, gl.enemy1Texture);
		
 		glBegin(GL_QUADS);

		glTexCoord2f(0.0f, 1.0f); glVertex2i(-enemy1->wid, -enemy1->hgt);
		glTexCoord2f(0.0f, 0.0f); glVertex2i(-enemy1->wid, enemy1->hgt);
		glTexCoord2f(1.0f, 0.0f); glVertex2i(enemy1->wid, enemy1->hgt);
		glTexCoord2f(1.0f, 1.0f); glVertex2i(enemy1->wid, -enemy1->hgt);
		glEnd();
		glPopMatrix();

				//this is for the enem2
		glPushMatrix();
		glColor3f(1.0, 1.0, 1.0);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.0f);
		glTranslatef(enemy2->posX, enemy2->posY+115, 0.0f);
		glBindTexture(GL_TEXTURE_2D, gl.goblinTexture);
		
		glBegin(GL_QUADS);

		glTexCoord2f(0.0f, 1.0f); glVertex2i(-enemy2->wid, -enemy2->hgt);
		glTexCoord2f(0.0f, 0.0f); glVertex2i(-enemy2->wid, enemy2->hgt);
		glTexCoord2f(1.0f, 0.0f); glVertex2i(enemy2->wid, enemy2->hgt);
		glTexCoord2f(1.0f, 1.0f); glVertex2i(enemy2->wid, -enemy2->hgt);
		glEnd();
		glPopMatrix();
		
                //this is for falling obj
                glPushMatrix();
                glColor3f(1.0, 1.0, 1.0);
                glEnable(GL_ALPHA_TEST);
                glAlphaFunc(GL_GREATER, 0.0f);
                glTranslatef(obj->pX, obj->pY, 0.0f);
				glBindTexture(GL_TEXTURE_2D, gl.barrierTexture);

		glBegin(GL_TRIANGLE_FAN);
                
                glTexCoord2f(0.0f, 1.0f); glVertex2i(-obj->w, -obj->h);
                glTexCoord2f(0.0f, 0.0f); glVertex2i(-obj->w, obj->h);
                glTexCoord2f(1.0f, 0.0f); glVertex2i(obj->w, obj->h);
                glTexCoord2f(1.0f, 1.0f); glVertex2i(obj->w, -obj->h);

                glEnd();
                glPopMatrix();

		//this is for falling obj
                glPushMatrix();
                glColor3f(1.0, 1.0, 1.0);
                glEnable(GL_ALPHA_TEST);
                glAlphaFunc(GL_GREATER, 0.0f);
                glTranslatef(obj2->pX, obj2->pY, 0.0f);
                                glBindTexture(GL_TEXTURE_2D, gl.barrierTexture);

                glBegin(GL_TRIANGLE_FAN);

                glTexCoord2f(0.0f, 1.0f); glVertex2i(-obj2->w, -obj2->h);
                glTexCoord2f(0.0f, 0.0f); glVertex2i(-obj2->w, obj2->h);
                glTexCoord2f(1.0f, 0.0f); glVertex2i(obj2->w, obj2->h);
                glTexCoord2f(1.0f, 1.0f); glVertex2i(obj2->w, -obj2->h);

                glEnd();
                glPopMatrix();



		// show settings icon top right
		extern void showSettingsIcon(int x, int y, GLuint texid);
		showSettingsIcon(gl.xres-50, gl.yres-45, gl.settings_icon_Texture);

		
		if (gl.helpTab) {
			extern void showHelpTab(int x, int y, GLuint texid);
			extern void showHelpText(int x, int y);
			showHelpTab(250, 475, gl.cactusTexture);
			showHelpText(450, 450); 
			
			extern void showControls(int x, int y);
			showControls(900, gl.yres-350);
			return;
		}

		//float cx = gl.xres/2.0;
		//float cy = gl.yres/2.0;
		//
		extern void trophy(int x, int y, GLuint texid);
		//trophy(1000, 1000, gl.trophyTexture);
		//create floor
		extern void createFloor(int x, int y, GLuint texid);
		extern void createFloorAngle(int x, int y, GLuint texid);
		int xGround=0;
		int yGround=10; //10, so it can be on screen
		
		//each ground block is 32 pixels wide
		//1st floor
	    for (int i=0; i<51; i++) {
	    	createFloor(xGround, yGround, gl.floorTexture);
	    	xGround += 32;
	    }
	    
	    //steps to get to 2nd
	    createFloor(1344, 330, gl.floorTexture);
	    createFloor(1376, 298, gl.floorTexture);
	    createFloor(1568, 138, gl.floorTexture);
	    createFloor(1568, 170, gl.floorTexture);
	    createFloor(1568, 202, gl.floorTexture);

	    //2nd floor
	    int y2ndFloor =362;
	    int x2ndFloor =0;
	    for (int i=0; i<8; i++) {
	    	createFloor(x2ndFloor, y2ndFloor, gl.floorTexture);
	    	createFloor((x2ndFloor)+416, y2ndFloor, gl.floorTexture);
	    	createFloor((x2ndFloor)+416, y2ndFloor, gl.floorTexture);
	    	createFloor((x2ndFloor)+1056, y2ndFloor, gl.floorTexture);
	    	x2ndFloor +=32;
	    }
	    createFloor(800, y2ndFloor, gl.floorTexture);
	    int t3 =32;
	    for (int i=0; i<3; i++) {
	    	createFloor(800, (y2ndFloor)+t3, gl.floorTexture);
	    	createFloor(864, y2ndFloor, gl.floorTexture);
	    	createFloor(896, y2ndFloor, gl.floorTexture);
	    	createFloor(928, y2ndFloor, gl.floorTexture);
	    	t3+=32;
	    }
	    int t =32;
	    for (int i=0; i<4;i++) {
	    	createFloor(32, (y2ndFloor)+t, gl.floorTexture);
	   		t +=32;
	    }
	    int t1 =32;
	    for (int i=0; i<2;i++) {
	    	createFloor(96, (y2ndFloor)+t1, gl.floorTexture);
	   		t1 +=32;
	    }
	    
	    //3rd floor
	    int x3rdFloor =192;
	    int y3rdFloor =618;
	    createFloor(160, 554, gl.floorTexture);
	    for (int i=0; i<8; i++) {
	    	createFloor(x3rdFloor, y3rdFloor, gl.floorTexture);
	    	createFloor((x3rdFloor)+992, y3rdFloor, gl.floorTexture);
	    	x3rdFloor +=32;
	    }
	    for (int i=0; i<3;i++) {
	    	createFloor((x3rdFloor)+96, y3rdFloor, gl.floorTexture);
	    	createFloor((x3rdFloor)+320, y3rdFloor, gl.floorTexture);
	    	x3rdFloor +=32;
	    }
	    createFloor(928, (y3rdFloor)+32, gl.floorTexture);
	    createFloor(928, (y3rdFloor)+64, gl.floorTexture);
	    createFloor(928, y3rdFloor, gl.floorTexture);
	    createFloor(992, y3rdFloor, gl.floorTexture);
	    createFloor(1056, y3rdFloor, gl.floorTexture);
	    createFloor(1088, y3rdFloor, gl.floorTexture);
	    
	     //stairs
	     int xStairs =0;
	    for (int i=0; i<3; i++) {
	    	createFloor((xStairs)+704, yGround, gl.floorTexture);	    		
	    	createFloor	((xStairs)+704, (yGround)+32, gl.floorTexture);
    		createFloor((xStairs)+416, yGround, gl.floorTexture);
	   		createFloor((xStairs)+448, (yGround)+32, gl.floorTexture);
	   		createFloor((xStairs)+1408, yGround, gl.floorTexture);
	   		createFloor((xStairs)+1408, (yGround)+32, gl.floorTexture);
	   		createFloor((xStairs)+1408, (yGround)+64, gl.floorTexture);//
	    	xStairs += 64;
	    	yGround += 32;		
	    }
	    
	    
	    
	    yGround =10;
	    //vertical walls
	    for (int i=0; i<5; i++) {
	    	//createFloor(300, (yGround)+340 , gl.floorTexture);
	    	createFloor(576, yGround , gl.floorTexture);
	    	yGround +=32;
	    }
	    //Barrier
	    int barrierWall =0;
	    for (int i=0; i<32; i++) {
	    	createFloor(0, barrierWall , gl.barrierTexture);
	    	createFloor(1600, barrierWall , gl.barrierTexture);
	    	barrierWall += 32;
	    }
	   
		
		//this is for the player

		glPushMatrix();
		glColor3f(1.0, 1.0, 1.0);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.0f);
		glTranslatef(player->positionX, player->positionY+105, 0.0f);
		glBindTexture(GL_TEXTURE_2D, gl.walkTexture);

		//added for other walking pics	
		int ix = gl.walkFrame % 8;
		int iy = 0;
		if (gl.walkFrame >= 8)
			iy = 1;
		float fx = (float)ix / 8.0;
		float fy = (float)iy / 2.0;

		glBegin(GL_QUADS);

		if (gl.keys[XK_Left]) {
			glTexCoord2f(fx+.125, fy+.5); glVertex2i(-player->width, -player->height);
			glTexCoord2f(fx+.125, fy);    glVertex2i(-player->width, player->height);
			glTexCoord2f(fx, fy);    glVertex2i(player->width, player->height);
			glTexCoord2f(fx, fy+.5); glVertex2i(player->width, -player->height);
		} else {
			glTexCoord2f(fx, fy+.5); glVertex2i(-player->width, -player->height);
			glTexCoord2f(fx, fy);    glVertex2i(-player->width, player->height);
			glTexCoord2f(fx+.125, fy);    glVertex2i(player->width, player->height);
			glTexCoord2f(fx+.125, fy+.5); glVertex2i(player->width, -player->height);
		}

		/*
		   glTexCoord2f(0.0f, 1.0f); glVertex2i(-player->width, -player->height);
		   glTexCoord2f(0.0f, 0.0f); glVertex2i(-player->width, player->height);
		   glTexCoord2f(0.125f, 0.0f); glVertex2i(player->width, player->height);
		   glTexCoord2f(0.125f, 1.0f); glVertex2i(player->width, -player->height);*/
		glEnd();
		glPopMatrix();

		

/*
		//move enemy back and fourth on screen
		extern void moveEnemy(Enem *e);
		moveEnemy(enem);

		//show enemies	
		//extern void showEnemy1(int x, int y, GLuint Texid);
		//showEnemy1(500, 30, gl.enemy1Texture);

		extern void showGoblin(int x, int y, GLuint Texid);
		showGoblin(700, 30, gl.goblinTexture);
*/		

		r.bot = gl.yres - 20;
		r.left = 10;
		r.center = 0;
		ggprint8b(&r, 16, 0x00ffff44, "H    	Help/Info");
		ggprint8b(&r, 16, 0x00ffff44, "N        Sound Test");
		ggprint8b(&r, 16, 0x00ffff44, "E        Exit");

		if (gl.movie) {
			screenCapture();
		}
	}
	else if (gl.gameover == true)
	{
		cout << "gameover" << endl;
		extern void gameover(int x, int y, GLuint texid);
		gameover(1600/2, 1300/2, gl.gameoverTexture);
	}
}
