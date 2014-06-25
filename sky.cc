#include "shad.h"

texture_t *sky_up;
texture_t *sky_rt;
texture_t *sky_lf;
texture_t *sky_ft;
texture_t *sky_bk;
texture_t *sky_dn;

cvar_t *draw_sky;

// FIXME: completely temporary!

// We don't actually listen to the bsp file here.
// We load an environment map and texture simply
// draw the cube at a constant distance

// TODO:
//   - Load proper sky from map specification
//   - Approximate a sphere


// FIXME: make sure dynamic lights do not affect this!
void DrawSky(camera_t *cam)
{
	if (!cvar_int(draw_sky)) return;

	glPushMatrix();
	glTranslatef(cam->pos[0], cam->pos[1], cam->pos[2]);

	glDisable(GL_LIGHTING);
	glDepthMask(0);

	// bk
	glBindTexture(GL_TEXTURE_2D, sky_bk->name);
	glBegin(GL_QUADS);
	        glTexCoord2f(0.0, 0.0);
		glVertex3f(-128, 128, -128);

		glTexCoord2f(1.0, 0.0);
		glVertex3f( 128, 128, -128);

		glTexCoord2f(1.0, 1.0);
		glVertex3f( 128,-128, -128);

		glTexCoord2f(0.0, 1.0);
		glVertex3f(-128,-128, -128);
	glEnd();

	// right
	glBindTexture(GL_TEXTURE_2D, sky_rt->name);
	glBegin(GL_QUADS);
	        glTexCoord2f(0.0, 0.0);
		glVertex3f(128, 128, -128);

		glTexCoord2f(1.0, 0.0);
		glVertex3f(128, 128,  128);

		glTexCoord2f(1.0, 1.0);
		glVertex3f(128,-128,  128);

		glTexCoord2f(0.0, 1.0);
		glVertex3f(128,-128, -128);
	glEnd();

	// ft
	glBindTexture(GL_TEXTURE_2D, sky_ft->name);
	glBegin(GL_QUADS);
	        glTexCoord2f(0.0, 0.0);
		glVertex3f( 128, 128,  128);

		glTexCoord2f(1.0, 0.0);
		glVertex3f(-128, 128,  128);

		glTexCoord2f(1.0, 1.0);
		glVertex3f(-128,-128,  128);

		glTexCoord2f(0.0, 1.0);
		glVertex3f( 128,-128,  128);
	glEnd();
	
	// left
	glBindTexture(GL_TEXTURE_2D, sky_lf->name);
	glBegin(GL_QUADS);
	        glTexCoord2f(0.0, 0.0);
		glVertex3f(-128, 128,  128);

		glTexCoord2f(1.0, 0.0);
		glVertex3f(-128, 128, -128);

		glTexCoord2f(1.0, 1.0);
		glVertex3f(-128,-128, -128);

		glTexCoord2f(0.0, 1.0);
		glVertex3f(-128,-128,  128);
	glEnd();

	// top
	glBindTexture(GL_TEXTURE_2D, sky_up->name);
	glBegin(GL_QUADS);
	        glTexCoord2f(0.0, 0.0);
		glVertex3f(-128, 128, -128);


		glTexCoord2f(1.0, 0.0);
		glVertex3f(-128, 128,  128);

		glTexCoord2f(1.0, 1.0);
		glVertex3f( 128, 128,  128);

		glTexCoord2f(0.0, 1.0);
		glVertex3f( 128, 128, -128);
	glEnd();

	// bottom
	glBindTexture(GL_TEXTURE_2D, sky_dn->name);
	glBegin(GL_QUADS);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-128,-128, -128);

	        glTexCoord2f(0.0, 0.0);
		glVertex3f( 128,-128, -128);

		glTexCoord2f(0.0, 1.0);
		glVertex3f( 128,-128,  128);

		glTexCoord2f(1.0, 1.0);
		glVertex3f(-128,-128,  128);
	glEnd();

	glDepthMask(1);
	glPopMatrix();
}

void InitSky()
{
	register_cvar(&draw_sky, "draw_sky", "1");

	sky_up = LoadTexture("env/space1_up");
	sky_dn = LoadTexture("env/space1_dn");
	sky_rt = LoadTexture("env/space1_rt");
	sky_lf = LoadTexture("env/space1_lf");
	sky_bk = LoadTexture("env/space1_bk");
	sky_ft = LoadTexture("env/space1_ft");

	TextureSetup(sky_up, -1, -1, GL_CLAMP, GL_CLAMP);
	TextureSetup(sky_dn, -1, -1, GL_CLAMP, GL_CLAMP);
	TextureSetup(sky_rt, -1, -1, GL_CLAMP, GL_CLAMP);
	TextureSetup(sky_lf, -1, -1, GL_CLAMP, GL_CLAMP);
	TextureSetup(sky_bk, -1, -1, GL_CLAMP, GL_CLAMP);
	TextureSetup(sky_ft, -1, -1, GL_CLAMP, GL_CLAMP);

	sky_up->flags |= TEX_PERMANENT;
	sky_dn->flags |= TEX_PERMANENT;
	sky_rt->flags |= TEX_PERMANENT;
	sky_lf->flags |= TEX_PERMANENT;
	sky_bk->flags |= TEX_PERMANENT;
	sky_ft->flags |= TEX_PERMANENT;
}
