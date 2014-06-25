#ifdef CYGWIN
// this file uses win32 functions to define 'gettimeofday'
#include "timeval.h"
#else
#include <sys/time.h>
#include <time.h>
#endif

#include <dirent.h>
#include <stdarg.h>

#include "shad.h"

float  white[4] = {1,1,1,1};
float  black[4] = {0,0,0,1};
float    red[4] = {1,0,0,1};
float  green[4] = {0,1,0,1};
float   blue[4] = {0,0,1,1};
float yellow[4] = {1,1,0,1};
float   cyan[4] = {0,1,1,1};


// shadow map 
int shadowMapSize=512;

#define NUM_SHADOWMAPS   16
GLuint shadowMapTexture[NUM_SHADOWMAPS];

// opengl extensions
int gl_stencil_two_side = 0;          // FIXME: need these?
int gl_ext_stencil_wrap = 0;
int gl_ext_compiled_vertex_array = 0;

// opengl info
int gl_max_lights;
int gl_max_clip_planes;
int gl_max_texture_size;
int gl_max_viewport_dims;
int gl_max_texture_units_arb;

// global cvars 
cvar_t *draw_flat;
cvar_t *draw_lighting;
cvar_t *draw_texture;
cvar_t *draw_wireframe;
cvar_t *draw_shadow_mode;

cvar_t *draw_polygons;
cvar_t *draw_meshes;
cvar_t *draw_patches;
cvar_t *draw_billboards;

cvar_t *draw_translucent;

cvar_t *draw_shadows;
cvar_t *draw_silhouette;
cvar_t *draw_shadow_volumes;

cvar_t *mapping_cull_front;
cvar_t *mapping_polygon_offset;


cvar_t *edge_cull;

cvar_t *draw_light_frustum;

cvar_t *dynamic_lights;

cvar_t *info_viewpoint;

cvar_t *hack_test;



/*****************************************************************************/
// FIXME: implement this properly!!
void Shutdown()
{
	exit(1);
}

void error(char *args, ...)
{
        char out[1024];

        va_list list;
        va_start(list, args);
        vsprintf(out, args, list);
        va_end(list);
        printf("%s\n", out);

	Shutdown();
}

/****************************************************************************
  Timer/fps stuff
****************************************************************************/;
double time_last, time_cur, time_delta;
int    fps_counter;
double fps_last, fps_delta;
int    fps_num_strings;
char   fps_string[64][128];

// stats
double draw_time_pass[128][10];


double GetTime()
{
	double blah;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	blah = ((double)tv.tv_sec + tv.tv_usec*1e-6);
	return blah;
}

void InitTime()
{
	fps_last = time_last = GetTime();
	fps_counter = 0;
}

void FpsUpdate()
{
	time_cur = GetTime();

	// time between frames
	time_delta = time_cur - time_last;
	time_last = time_cur;

	// time from the last fps update
	fps_delta = time_cur - fps_last;

	// update the fps information
	if (fps_delta >= 1.0f) {
		double d1,d2,d3,d4;

		fps_num_strings = 0;
		sprintf(fps_string[fps_num_strings++], "FPS: %.2lf %4.1lfms", 
			(double)fps_counter/fps_delta,
			time_delta*1e3);

		d1 = (draw_time_pass[0][1] - draw_time_pass[0][0]);
		d2 = (draw_time_pass[0][3] - draw_time_pass[0][2]);
		d3 = (draw_time_pass[0][5] - draw_time_pass[0][4]);
		d4 = (draw_time_pass[0][6] - draw_time_pass[0][5]);
		sprintf(fps_string[fps_num_strings++], 
			" WLD:%4.1lfms %2d%% UD:%4.1lfms %2d%% HUD:%4.1lfms %2d%%  UGL:%4.1lfms %2d%%", 
			d1*1e3, (int)(d1/time_delta*1e2+0.5), 
			d2*1e3, (int)(d2/time_delta*1e2+0.5),
			d3*1e3, (int)(d3/time_delta*1e2+0.5),
			d4*1e3, (int)(d4/time_delta*1e2+0.5));

		if (!strcmp(draw_shadow_mode->value, "hard_volume")) {
			for (int i=1; i<=num_lights; i++) {
				if (!light[i-1].affects_scene) continue;
				d1 = (draw_time_pass[i][1]-draw_time_pass[i][0]);
				d2 = (draw_time_pass[i][2]-draw_time_pass[i][1]);
				sprintf(fps_string[fps_num_strings++], 
					"LT%2d: %4.1lfms %2d%%,  %4.1lfms %2d%%", 
					i, 
					d1*1e3, (int)(d1/time_delta*1e2 + 0.5), 
					d2*1e3, (int)(d2/time_delta*1e2 + 0.5));
			}
		}
		if (!strcmp(draw_shadow_mode->value, "hard_mapping")) {
			for (int i=1; i<=num_lights; i++) {
				if (!light[i-1].affects_scene) continue;
				d1 = (draw_time_pass[i][1]-draw_time_pass[i][0]);
				d2 = (draw_time_pass[i][2]-draw_time_pass[i][1]);
				d3 = (draw_time_pass[i][4]-draw_time_pass[i][3]);
				sprintf(fps_string[fps_num_strings++], 
					"LT%2d:%4.1lfms %2d%%,  %4.1lfms %2d%%,  %4.1lfms %2d%%",
					i, 
					d1*1e3, (int)(d1/time_delta*1e2 + 0.5), 
					d2*1e3, (int)(d2/time_delta*1e2 + 0.5),
					d3*1e3, (int)(d3/time_delta*1e2 + 0.5));
			}
		}

		fps_last = time_cur;
		fps_counter = -1;
	}

	fps_counter++;
}

// print FPS
// NOTE: assumes fps is the first string of fps_strings
void PrintFPS()
{
	glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
	DrawString(-1, 1.0, fps_string[0]);
}

// print other stats besides fps
// NOTE: assumes fps is the first string of fps_strings
void PrintStats()
{
	int i,j;
	float y = 1.0f;

	y -= font_ystep;
	glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
	for (int i=1; i<fps_num_strings; i++) {
		DrawString(-1, y, fps_string[i]);
		y -= font_ystep;
	}

	draw_num_triangles = player.drew_triangles;
	draw_num_polygons = player.drew_polygons;
	draw_num_meshes = player.drew_meshes;
	draw_num_patches = player.drew_patches;

	DrawString(-1, y, "WLD: tris: %5d polys: %5d meshes: %5d patches: %d", 
		   draw_num_triangles, draw_num_polygons, draw_num_meshes, draw_num_patches);
	y -= font_ystep;

	// print the stats for each light 
	if (!strcmp(draw_shadow_mode->value, "hard_volume")) {

		DrawString(-1, y, "TTL: volume_caps: %6d, volume_sides: %d",
			   draw_num_volume_caps, draw_num_volume_sides);
		y -= font_ystep;

		for (i=j=0; i<num_lights; i++) {
			if (!light[i].affects_scene) continue;
			DrawString(-1, y, "LT%d: volume_caps: %6d, volume_sides: %d",
				   j++, light[i].num_volume_caps, light[i].num_volume_sides);
			y -= font_ystep;
		}

	} else if (!strcmp(draw_shadow_mode->value, "hard_mapping")) {
		for (i=j=0; i<num_lights; i++) {
			if (!light[i].affects_scene) continue;
			DrawString(-1, y, "LT%d: tris: %5d polys: %5d meshes: %5d patches: %d", 
				   j++,
				   light[i].cam.drew_triangles, light[i].cam.drew_polygons, 
				   light[i].cam.drew_meshes, light[i].cam.drew_patches);
			y -= font_ystep;

		}
	}
}

void PrintViewpoint(camera_t *cam)
{
	if (!cvar_int(info_viewpoint)) return;

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	DrawString(-1, -0.75, "an: (%+7.2f, %+7.2f, %+7.2f)",
		   cam->roll, cam->pitch, cam->yaw);

	DrawString(-1, -0.80, "ps: (%+7.2f, %+7.2f, %+7.2f)", 
		   cam->pos[0], cam->pos[1], cam->pos[2]);
	
	DrawString(-1, -0.85, "rt: (%+7.2f, %+7.2f, %+7.2f)", 
		   cam->rt[0], cam->rt[1], cam->rt[2]);

	DrawString(-1, -0.90, "up: (%+7.2f, %+7.2f, %+7.2f)", 
		   cam->up[0], cam->up[1], cam->up[2]);

	DrawString(-1, -0.95, "vn: (%+7.2f, %+7.2f, %+7.2f)", 
		   cam->vn[0], cam->vn[1], cam->vn[2]);
}

void DrawHUD()
{
	// Setup for 2d viewing
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-1.0f, 1.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	DrawConsole();
	
	SetAlphaFunc(ALPHA_FUNC_NONE);
	glBindTexture(GL_TEXTURE_2D, font->name);
	SetBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	PrintFPS();
	if (bsp_loaded) {
		PrintStats();
		PrintViewpoint(&player);
	}
	glDisable(GL_BLEND);
}

void EnableTextures()
{
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);
}

void DisableTextures()
{
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE2_ARB);
	glDisable(GL_TEXTURE_2D);
}


void EnableTexGen()
{
	float idx[4] = {1, 0, 0, 0};
	float idy[4] = {0, 1, 0, 0};
	float idz[4] = {0, 0, 1, 0};
	float idw[4] = {0, 0, 0, 1};

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_S, GL_EYE_PLANE, idx);
	glEnable(GL_TEXTURE_GEN_S);

	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_T, GL_EYE_PLANE, idy);
	glEnable(GL_TEXTURE_GEN_T);

	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_R, GL_EYE_PLANE, idz);
	glEnable(GL_TEXTURE_GEN_R);

	glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_Q, GL_EYE_PLANE, idw);
	glEnable(GL_TEXTURE_GEN_Q);
}

void DisableTexGen()
{
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glDisable(GL_TEXTURE_GEN_Q);
}

void EnableClipPlanes()
{
	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);
	//glEnable(GL_CLIP_PLANE4);
	glEnable(GL_CLIP_PLANE5);
}

void DisableClipPlanes()
{
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);
}


void DrawLights()
{
	for (int i=0; i<num_lights; i++) {
		if (!light[i].affects_scene) continue;

		glPushMatrix();
		glColor3f(1, 1, 0);
		glTranslatef(light[i].cam.pos[0], light[i].cam.pos[1], light[i].cam.pos[2]);
		glCullFace(GL_BACK);
		glutSolidSphere(2, 24, 24);
		glPopMatrix();

		if (cvar_int(draw_light_frustum) && light[i].on) DrawBBox(light[i].bbox);
	}
}


/**************************************************************/
/*               Draw standard Quake3 scene                   */
/**************************************************************/
void DrawQuake3()
{
	draw_time_pass[0][0] = GetTime();

	CameraSetView(&player);

	DrawSky(&player);
	DrawWorld(&player);

	draw_time_pass[0][1] = GetTime();
}


/**************************************************************/
/*         Draw using the shadow mapping technique            */
/*  FIXME: need a pbuffer before we can do this properly!!    */
/**************************************************************/
void DrawWithShadowMapping()
{

	/****************************************************************/
	/*           Draw from light pov into depth texture             */
	/****************************************************************/
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_CULL_FACE);       
	if (cvar_int(mapping_cull_front)) {
		glCullFace(GL_FRONT);
	} else {
		glCullFace(GL_BACK);
	}

	glShadeModel(GL_SMOOTH);      // GL_SMOOTH is faster than GL_FLAT.... huh?
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColorMask(0, 0, 0, 0);
	
	DisableTextures();
	glDisable(GL_LIGHTING);


	if (cvar_int(mapping_polygon_offset)) {
		glPolygonOffset(1.1, 8.0);
		glEnable(GL_POLYGON_OFFSET_EXT);
		glEnable(GL_POLYGON_OFFSET_FILL);
	}

	for (int i=0; i<num_lights; i++) {
		if (!light[i].affects_scene) continue;

		draw_time_pass[i+1][0] = GetTime();

		/********************************************************/
		/*             Draw into the texture                    */
		/********************************************************/
		glClear(GL_DEPTH_BUFFER_BIT);
		CameraSetView(&light[i].cam);
		DrawWorld(&light[i].cam);

		draw_time_pass[i+1][1] = GetTime();

		/********************************************************/
		/*  Read the depth buffer into the shadow map texture   */
		/********************************************************/
		glActiveTextureARB(GL_TEXTURE2_ARB);	
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture[i]);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, shadowMapSize, shadowMapSize);

		draw_time_pass[i+1][2] = GetTime();
	}

	glDisable(GL_POLYGON_OFFSET_EXT);
	glEnable(GL_POLYGON_OFFSET_FILL);

	/****************************************************************/
	/*                   Draw the unlit world                       */
	/****************************************************************/
	draw_time_pass[0][0] = GetTime();

	glColorMask(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
		
	glShadeModel(GL_SMOOTH);
	glPolygonMode(GL_FRONT_AND_BACK, (cvar_int(draw_wireframe)) ? GL_LINE : GL_FILL);

	CameraSetView(&player);

	DisableTextures();
	DrawSky(&player);
	
	SetTextureModes();
	DrawWorld(&player);
		
	draw_time_pass[0][1] = GetTime();

	if (!cvar_int(draw_shadows)) goto skipshadows;


	/****************************************************************/
	/*                    Add light from lights                     */
	/****************************************************************/
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glEnable(GL_TEXTURE_2D);
	EnableTexGen();

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glEnable(GL_LIGHTING);

	EnableClipPlanes();

#if 0
	// Set alpha test to discard false comparisons
	// FIXME: do we really need this?!
	// it messes up textures with 4 channels
	glAlphaFunc(GL_GEQUAL, 0.99f);
	glEnable(GL_ALPHA_TEST);
#endif

	for (int i=0; i<num_lights; i++) {
		if (!light[i].affects_scene) continue;

		draw_time_pass[i+1][3] = GetTime();

		// Setup light
		LightSetAttributes(&light[i]);
		LightSetClipPlanes(&light[i]);
		LightSetTextureMatrix(&light[i]);

		// Bind & enable shadow map texture
		glActiveTextureARB(GL_TEXTURE2_ARB);	
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture[i]);
		EnableTexGen();

		// Enable shadow comparison
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB,GL_COMPARE_R_TO_TEXTURE);
		// Shadow comparison should be true (ie not in shadow) if r<=texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);

		// Shadow comparison should generate an INTENSITY result
		// FIXME: this is currently off. DO WE NEED THIS?!?!
		//glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);

		SetTextureModes_Lighting();

		if (cvar_int(mapping_polygon_offset)) {
			glPolygonOffset(-3.0*(i*0.5), -14.0 - i*3);
			glEnable(GL_POLYGON_OFFSET_EXT);
			glEnable(GL_POLYGON_OFFSET_FILL);
		}

		DrawWorld(&player);        

		glDisable(GL_POLYGON_OFFSET_EXT);
		glDisable(GL_POLYGON_OFFSET_FILL);
		
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_2D);
		DisableTexGen();

		draw_time_pass[i+1][4] = GetTime();

	}

	glDisable(GL_BLEND);

	DisableClipPlanes();

 skipshadows:
	DisableTextures();
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_LIGHTING);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

}


/**************************************************************/
/*         Draw using the shadow volume technique             */
/**************************************************************/
void DrawWithShadowVolumes()
{

	/****************************************************************/
	/*          Draw the unlit world from players POV               */
	/****************************************************************/
	draw_time_pass[0][0] = GetTime();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, (cvar_int(draw_wireframe)) ? GL_LINE : GL_FILL);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glShadeModel(GL_SMOOTH);
	glColorMask(1, 1, 1, 1);

	CameraSetView(&player);

	DisableTextures();
	DrawSky(&player);

	SetTextureModes();
	DrawWorld(&player);

	draw_time_pass[0][1] = GetTime();

	if (!cvar_int(dynamic_lights)) return;

	/****************************************************************/
	/* Render shadow volumes and world with lighting for each light */
	/****************************************************************/
	draw_num_volume_caps = 0;
	draw_num_volume_sides = 0;
	for (int i=0; i<num_lights; i++) {
		if (!light[i].affects_scene) continue;

		draw_time_pass[i+1][0] = GetTime();

		if (!cvar_int(draw_shadows)) goto skipshadows;

		// Setup for the stencil buffer passes
		glClear(GL_STENCIL_BUFFER_BIT);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);
		
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, 0, ~0);
		
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		CalculateShadowVolume(&light[i]);

		glEnableClientState(GL_VERTEX_ARRAY);
		DrawShadowVolume_Stencil();
		glDisableClientState(GL_VERTEX_ARRAY);

skipshadows:
		// Draw scene with this light
		draw_time_pass[i+1][1] = GetTime();

		glCullFace(GL_BACK);
		glColorMask(1, 1, 1, 1);
		glDepthFunc(GL_EQUAL);
		glDepthMask(1);

		glStencilFunc(GL_EQUAL, 0, ~0);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
	
		glEnable(GL_LIGHTING);
		LightSetAttributes(&light[i]);
		
		SetTextureModes_Lighting();

		DrawWorld(&player);
	
		DisableTextures();

		glDepthFunc(GL_LEQUAL);
		glDisable(GL_LIGHTING);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_BLEND);
		
		// Draw the shadow volumes if desired
		if (cvar_int(draw_shadow_volumes)) {
			DrawShadowVolume_Visible(&light[i]);
		}

		draw_time_pass[i+1][2] = GetTime();
	}
}

void UpdatePlayer()
{
	int infinite = 0;
	if (!strcmp(draw_shadow_mode->value, "hard_volume")) infinite = 1;
	CameraUpdateView(&player, infinite);
	DetermineVisibleFaces(&player, 1);
}

void UpdateWorld()
{
	HandleInput();

	if (bsp_loaded) {
		UpdatePlayer();
		UpdateEnemies();
		UpdateLights();
	}

	UpdateConsole();
}

void DrawEverything()
{
	draw_time_pass[0][2] = GetTime();
	UpdateWorld();
	draw_time_pass[0][3] = GetTime();

	PreFrame();
	if (bsp_loaded) {
		if (!strcmp(draw_shadow_mode->value, "hard_volume")) {
			
			DrawWithShadowVolumes();
			DrawLights();

		} else if (!strcmp(draw_shadow_mode->value, "hard_mapping")) {
			
			DrawWithShadowMapping();
			DrawLights();
			
		} else {
			
			DrawQuake3();
		}
	}
	FpsUpdate();

	draw_time_pass[0][4] = GetTime();
	DrawHUD();
	PostFrame();
	draw_time_pass[0][5] = GetTime();

	glFinish();
	draw_time_pass[0][6] = GetTime();

	glutSwapBuffers();
	glutPostRedisplay();
}

// called on window resize
void Reshape(int w, int h)
{
	player.viewwidth = w;
	player.viewheight = h;
}


/**************************************************************/
/*         Initialization stuff                               */
/**************************************************************/

// version information
// FIXME: is there a way to get the svn build number automatically?
// FIXME: get the date automatically?
char about_author[]  = "Broderick Arneson";
char about_title[]   = "Quake 3 Viewer / C511 Project";
char about_version[] = "Version 0.10b";
char about_date[]    = "06/01/27 01:23am"; 
char about_build[]   = "Build 126";

void About()
{
	conprintf(about_author);
	conprintf(about_title);
	conprintf(about_version);
	conprintf(about_date);
	conprintf(about_build);
	conprintf(" ");
}

void InitGLEW()
{
	conprintf("Using GLEW to initialize extensions:");
	GLenum err = glewInit();
	if (GLEW_OK != err) { 
		// FIXME: cygwin uses __imp___iob on the following. It doesn't like it.
		//fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		printf("Error: %s\n", glewGetErrorString(err));
		exit(1);
	}
	// FIXME: we use __imb___iob for the following. Cygwin doesn't like it
	//	fprintf(stdout, "Status: GLEW %s\n", glewGetString(GLEW_VERSION));
	conprintf("Status: GLEW %s", glewGetString(GLEW_VERSION));

	// check for necessary extensions
	if (!GLEW_ARB_multitexture) {
		error("ARB_multitexture is not supported.");
	} else {
		conprintf("-- ARB_multitexture supported.");
	}

	if (!GLEW_ARB_shadow) {
		error("ARB_shadow is not supported.");
	} else {
		conprintf("-- ARB_shadow supported.");
	}

	if (!GLEW_ARB_depth_texture) {
		error("ARB_depth_texture is not supported.");
	} else {
		conprintf("-- ARB_depth_texture supported.");
	}

	if (GLEW_EXT_compiled_vertex_array) {
		gl_ext_compiled_vertex_array = 1;
		conprintf("-- EXT_compiled_vertex_array supported.");
	} else {
		conprintf("-* EXT_compiled_vertex_array not supported.");
	}

	if (GLEW_EXT_stencil_two_side) {
		conprintf("-- EXT_stencil_two_side supported.");
	} 
	if (GLEW_EXT_stencil_wrap) {
		gl_ext_stencil_wrap = 1;
		conprintf("-- EXT_stencil_wrap supported.");
	}
	if (GLEW_EXT_stencil_two_side && GLEW_EXT_stencil_wrap) {
		gl_stencil_two_side = 1;
		conprintf("** Could use two-sided stencilling!");
	} else {
		conprintf("** Not using two-sided stencilling.");
	}

	conprintf("\n");
}

void GetOpenGLInfo()
{
	int r,g,b,a,d,s;
	char *gl_vendor, *gl_renderer, *gl_version, *gl_extensions;

	gl_vendor = (char*)glGetString(GL_VENDOR);
	gl_renderer = (char*)glGetString(GL_RENDERER);
	gl_version = (char*)glGetString(GL_VERSION);
	gl_extensions = (char*)glGetString(GL_EXTENSIONS);

	conprintf("=============== OpenGL Info =======================");
	conprintf("Vendor: %s", gl_vendor);
	conprintf("Renderer: %s", gl_renderer);
	conprintf("OpenGL Version: %s", gl_version);
	conprintf("Extensions: %s", gl_extensions);
	conprintf(" ");

	glGetIntegerv(GL_RED_BITS, &r);
	glGetIntegerv(GL_GREEN_BITS, &g);
	glGetIntegerv(GL_BLUE_BITS, &b);
	glGetIntegerv(GL_ALPHA_BITS, &a);
	glGetIntegerv(GL_DEPTH_BITS, &d);
	glGetIntegerv(GL_STENCIL_BITS, &s);

	glGetIntegerv(GL_MAX_LIGHTS, &gl_max_lights);
	glGetIntegerv(GL_MAX_CLIP_PLANES, &gl_max_clip_planes);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max_texture_size);
	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &gl_max_viewport_dims);
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max_texture_units_arb);

	conprintf(" ");
	conprintf("GL_MAX_VIEWPORT_DIMS:     %d", gl_max_viewport_dims);
	conprintf("GL_MAX_CLIP_PLANES:       %d", gl_max_clip_planes);
	conprintf("GL_MAX_LIGHTS:            %d", gl_max_lights);
	conprintf("GL_MAX_TEXTURE_SIZE:      %d", gl_max_texture_size);
	conprintf("GL_MAX_TEXTURE_UNITS_ARB: %d", gl_max_texture_units_arb);
	conprintf(" ");
	conprintf("FRAMEBUFFER (rgba:d:s) %dx%dx%dx%d:%d:%d", r,g,b,a,d,s);
	conprintf("=============== OpenGL Info =======================");
	conprintf(" ");
}

/////////////////////////////////////////
// Global cvars
/////////////////////////////////////////
void RegisterCvars()
{
	register_cvar(&info_viewpoint, "info_viewpoint", "1");

	register_cvar(&draw_wireframe, "draw_wireframe", "0");
	register_cvar(&draw_flat, "draw_flat", "0");
	register_cvar(&draw_lighting, "draw_lighting", "1");
	register_cvar(&draw_texture, "draw_texture", "1");

	register_cvar(&draw_polygons, "draw_polygons", "1");
	register_cvar(&draw_meshes, "draw_meshes", "1");
	register_cvar(&draw_patches, "draw_patches", "1");
	register_cvar(&draw_billboards, "draw_billboards", "0");

	register_cvar(&draw_translucent, "draw_translucent", "1");

	register_cvar(&dynamic_lights, "dynamic_lights", "1");
	register_cvar(&draw_shadows, "draw_shadows", "1");
	register_cvar(&draw_light_frustum, "draw_light_frustum", "0");
	register_cvar(&hack_test, "hack_test", "0");

	// Either "quake3", "hard_volume", "hard_mapping" 
	register_cvar(&draw_shadow_mode, "draw_shadow_mode", "quake3");
	//register_cvar(&draw_shadow_mode, "draw_shadow_mode", "hard_mapping");
	//register_cvar(&draw_shadow_mode, "draw_shadow_mode", "hard_volume");

	// shadow volume variables
	register_cvar(&draw_silhouette, "draw_silhouette", "0");
	register_cvar(&draw_shadow_volumes, "draw_shadow_volumes", "0");
	register_cvar(&edge_cull, "edge_cull", "1");

	// shadow mapping variables
	register_cvar(&mapping_cull_front, "mapping_cull_front", "0");
	register_cvar(&mapping_polygon_offset, "mapping_polygon_offset", "1");
	

}

////////////////////////////////////////////
// opengl initialization
////////////////////////////////////////////
void InitOpenGL()
{
	// Set clockwise windings to define front faces
	glFrontFace(GL_CW);
	glClearDepth(1.0f);
	glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// Setup light model
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);

	// White specular material color
	glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
	glMaterialfv(GL_FRONT, GL_SPECULAR, white);
	glMaterialf(GL_FRONT, GL_SHININESS, 16.0f);
}

// Create the shadow map textures
// FIXME: use a single pbuffer instead!!
static int created_shadowmaps = 0;
void CreateShadowMaps()
{
	if (created_shadowmaps) return;
	for (int i=0; i<NUM_SHADOWMAPS; i++) {
		glGenTextures(1, &shadowMapTexture[i]);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
			     shadowMapSize, shadowMapSize, 0,
			     GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		
	}
	created_shadowmaps = 1;
}

// startup glut
void InitGLUT(int argc, char **argv)
{
	char str[128];
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(800, 600);

        strcpy(str, about_title);
	strcat(str, ", ");
	strcat(str, about_version);
	strcat(str, ", ");
	strcat(str, about_date);
	strcat(str, ", ");
	strcat(str, about_build);
	glutCreateWindow(str);

	glutDisplayFunc(DrawEverything);
	glutReshapeFunc(Reshape);

	glutIgnoreKeyRepeat(1);
	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(glutKeyboardUpCallback);
	glutSpecialFunc(SpecialKeys);
	glutSpecialUpFunc(glutSpecialUpCallback);
	glutMouseFunc(handleMouseClick);
	glutPassiveMotionFunc(handleMouseMotion);
}

// FIXME: pass command-line arguments to console at some point
void Initialize(int argc, char **argv)
{
	InitShell();              // startup shell system
	About();                  // display some info about us
	RegisterCvars();          // declare some cvars

	// setup viewport
	// register callback functions
	InitGLUT(argc, argv);     
	InitGLEW();               // start glew and get opengl info
	GetOpenGLInfo();

	TextureInit();            // start subsystems
	InitShaders();
	ConsoleInit();
	InputInit();
	InitSky();
	InitRender();
	InitBSP();
	InitTime();

	InitSarge();              // FIXME: remove me!

	InitOpenGL();             // set some gl settings
	//CreateShadowMaps();     // FIXME: move this out of here
}

int main(int argc, char **argv)
{

	// TEMPORARY PLAYER SETUP
	// FIXME: put this into cvars
        player.fov = 45.0f;
	player.nearp = 1.0f;
	player.farp = 8000.0f;
	player.viewwidth = 800;
	player.viewheight= 600;

	Initialize(argc, argv);

	// GOGOGOGO
	glutMainLoop();
	return 0;
}
