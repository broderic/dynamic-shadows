#include <math.h>
#include <stdio.h>
#include "shad.h"

camera_t player;                             // player viewpoint


void NormalizePlane(double *pl)
{
	double d = 1/sqrt(pl[0]*pl[0] + pl[1]*pl[1] + pl[2]*pl[2]);
	pl[0] *= d;
	pl[1] *= d;
	pl[2] *= d;
	pl[3] *= d;
}

void ComputeTransform(camera_t *cam)
{
	float mat[16];

	glPushMatrix();
	glLoadIdentity();
	glRotatef(cam->roll, 0, 0, 1);
	glRotatef(cam->pitch, 1, 0, 0);
	glRotatef(-cam->yaw, 0, 1, 0);
	glGetFloatv(GL_MODELVIEW_MATRIX,mat);
	glPopMatrix();

	cam->rt[0] = mat[ 0];
	cam->rt[1] = mat[ 4];
	cam->rt[2] = mat[ 8];
	cam->rt[3] = mat[12];
	cam->up[0] = mat[ 1];
	cam->up[1] = mat[ 5];
	cam->up[2] = mat[ 9];
	cam->up[3] = mat[13];
	cam->vn[0] = mat[ 2];
	cam->vn[1] = mat[ 6];
	cam->vn[2] = mat[10];
	cam->vn[3] = mat[14];
}

// from the "cake" engine....
void ComputeFrustum(camera_t *cam)
{    
	double *proj,*modl,clip[16];

	proj = cam->proj;
	modl = cam->modl;

	// multiply modl and proj
	clip[ 0] = modl[ 0]*proj[ 0] + modl[ 1]*proj[ 4] + modl[ 2]*proj[ 8] + modl[ 3]*proj[12];
	clip[ 1] = modl[ 0]*proj[ 1] + modl[ 1]*proj[ 5] + modl[ 2]*proj[ 9] + modl[ 3]*proj[13];
	clip[ 2] = modl[ 0]*proj[ 2] + modl[ 1]*proj[ 6] + modl[ 2]*proj[10] + modl[ 3]*proj[14];
	clip[ 3] = modl[ 0]*proj[ 3] + modl[ 1]*proj[ 7] + modl[ 2]*proj[11] + modl[ 3]*proj[15];

	clip[ 4] = modl[ 4]*proj[ 0] + modl[ 5]*proj[ 4] + modl[ 6]*proj[ 8] + modl[ 7]*proj[12];
	clip[ 5] = modl[ 4]*proj[ 1] + modl[ 5]*proj[ 5] + modl[ 6]*proj[ 9] + modl[ 7]*proj[13];
	clip[ 6] = modl[ 4]*proj[ 2] + modl[ 5]*proj[ 6] + modl[ 6]*proj[10] + modl[ 7]*proj[14];
	clip[ 7] = modl[ 4]*proj[ 3] + modl[ 5]*proj[ 7] + modl[ 6]*proj[11] + modl[ 7]*proj[15];

	clip[ 8] = modl[ 8]*proj[ 0] + modl[ 9]*proj[ 4] + modl[10]*proj[ 8] + modl[11]*proj[12];
	clip[ 9] = modl[ 8]*proj[ 1] + modl[ 9]*proj[ 5] + modl[10]*proj[ 9] + modl[11]*proj[13];
	clip[10] = modl[ 8]*proj[ 2] + modl[ 9]*proj[ 6] + modl[10]*proj[10] + modl[11]*proj[14];
	clip[11] = modl[ 8]*proj[ 3] + modl[ 9]*proj[ 7] + modl[10]*proj[11] + modl[11]*proj[15];

	clip[12] = modl[12]*proj[ 0] + modl[13]*proj[ 4] + modl[14]*proj[ 8] + modl[15]*proj[12];
	clip[13] = modl[12]*proj[ 1] + modl[13]*proj[ 5] + modl[14]*proj[ 9] + modl[15]*proj[13];
	clip[14] = modl[12]*proj[ 2] + modl[13]*proj[ 6] + modl[14]*proj[10] + modl[15]*proj[14];
	clip[15] = modl[12]*proj[ 3] + modl[13]*proj[ 7] + modl[14]*proj[11] + modl[15]*proj[15];
	
	// extract each plane
	cam->frustum[RIGHT][0] = clip[ 3] - clip[ 0];
	cam->frustum[RIGHT][1] = clip[ 7] - clip[ 4];
	cam->frustum[RIGHT][2] = clip[11] - clip[ 8];
	cam->frustum[RIGHT][3] = clip[15] - clip[12];
	NormalizePlane(cam->frustum[RIGHT]);

	cam->frustum[LEFT][0] = clip[ 3] + clip[ 0];
	cam->frustum[LEFT][1] = clip[ 7] + clip[ 4];
	cam->frustum[LEFT][2] = clip[11] + clip[ 8];
	cam->frustum[LEFT][3] = clip[15] + clip[12];
	NormalizePlane(cam->frustum[LEFT]);

	cam->frustum[BOTTOM][0] = clip[ 3] + clip[ 1];
	cam->frustum[BOTTOM][1] = clip[ 7] + clip[ 5];
	cam->frustum[BOTTOM][2] = clip[11] + clip[ 9];
	cam->frustum[BOTTOM][3] = clip[15] + clip[13];
	NormalizePlane(cam->frustum[BOTTOM]);

	cam->frustum[TOP][0] = clip[ 3] - clip[ 1];
	cam->frustum[TOP][1] = clip[ 7] - clip[ 5];
	cam->frustum[TOP][2] = clip[11] - clip[ 9];
	cam->frustum[TOP][3] = clip[15] - clip[13];
	NormalizePlane(cam->frustum[TOP]);

	cam->frustum[BACK][0] = clip[ 3] - clip[ 2];
	cam->frustum[BACK][1] = clip[ 7] - clip[ 6];
	cam->frustum[BACK][2] = clip[11] - clip[10];
	cam->frustum[BACK][3] = clip[15] - clip[14];
	NormalizePlane(cam->frustum[BACK]);

	cam->frustum[FRONT][0] = clip[ 3] + clip[ 2];
	cam->frustum[FRONT][1] = clip[ 7] + clip[ 6];
	cam->frustum[FRONT][2] = clip[11] + clip[10];
	cam->frustum[FRONT][3] = clip[15] + clip[14];
	NormalizePlane(cam->frustum[FRONT]);
}


float deg2rad(float deg)
{
	float pi = acos(-1);
	return deg/180.0*pi;
}


#define VIEW_EPS  1e-4
void InfiniteFrustum(camera_t *cam)
{
	double *mat = cam->proj;
	float range = cam->nearp*tan(deg2rad(cam->fov/2));
	float aspect = (float)cam->viewwidth/cam->viewheight;

	// This matrix is just the limit of f at infinite of the normal projection matrix
	// NOTE: switch the commented lines out to get a normal non-infinite frustum
	mat[ 0] = 2*cam->nearp / (2*range*aspect);
	mat[ 1] = 0;
	mat[ 2] = 0;
	mat[ 3] = 0;

	mat[ 4] = 0;
	mat[ 5] = 2*cam->nearp / (2*range);
	mat[ 6] = 0;
	mat[ 7] = 0;

	mat[ 8] = 0;
	mat[ 9] = 0;
	//mat[10] = -(cam->far + cam->near)/(cam->far - cam->near);
	mat[10] = VIEW_EPS - 1;
	mat[11] = -1;
	
	mat[12] = 0;
	mat[13] = 0;
	//mat[14] = -(2*cam->far*cam->near)/(cam->far - cam->near);
	mat[14] = cam->nearp*(VIEW_EPS - 2);
	mat[15] = 0;

	glLoadMatrixd(mat);
}

void CameraSetView(camera_t *cam)
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(cam->proj);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(cam->modl);

	glViewport(0, 0, cam->viewwidth, cam->viewheight);
}

void CameraUpdateView(camera_t *cam, int infinite)
{
	// compute view matrix
	ComputeTransform(cam);

	// compute and store the projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	if (infinite) InfiniteFrustum(cam);
	else gluPerspective(cam->fov, (float)cam->viewwidth/cam->viewheight, 
			    cam->nearp, cam->farp);
	glGetDoublev(GL_PROJECTION_MATRIX, cam->proj);
	glPopMatrix();

	// compute and store the modelview matrix
	// FIXME: load the view matrix and translate by -position instead?
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	gluLookAt(cam->pos[0], cam->pos[1], cam->pos[2],
		  cam->pos[0]+cam->vn[0], cam->pos[1]+cam->vn[1], cam->pos[2]+cam->vn[2],
		  cam->up[0], cam->up[1], cam->up[2]);
	glGetDoublev(GL_MODELVIEW_MATRIX, cam->modl);
	glPopMatrix();

	ComputeFrustum(cam);
}


// If all 8 points of bbox are behind a frustum plane, 
// then bbox is not in frustum
int BoxInFrustum(camera_t *cam, float bbox[8][3])
{
	int i,a;
	float d;
	for (i=0; i<6; i++) {
		for (a=0; a<8; a++) {
			d = cam->frustum[i][0] * bbox[a][0] + 
			    cam->frustum[i][1] * bbox[a][1] + 
			    cam->frustum[i][2] * bbox[a][2];
			if (d + cam->frustum[i][3] >= -1e-3) break;
		}
		if (a==8) return 0;
	}
	return 1;
}

