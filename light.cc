#include "shad.h"


int num_lights;
light_t light[MAX_LIGHTS];

int draw_num_volume_sides;
int draw_num_volume_caps;

#define MAX_SHADOW_TRIS    (1<<17)*3
#define MAX_SHADOW_QUADS   (1<<19)

#define MAX_FACES          (1<<14)
#define MAX_VERTICES       (1<<16)

char frontface[MAX_FACES];
vec4_t extvert[MAX_VERTICES];

int nshadow_quads;
vec4_t shadow_quad[MAX_SHADOW_QUADS];
unsigned int quad_elem[MAX_SHADOW_QUADS];

int nshadow_tris, ntri_elem;
vec4_t shadow_tris[MAX_SHADOW_TRIS];
unsigned int tri_elem[MAX_SHADOW_TRIS];

extern int shadowMapSize;
extern float black[4]; 

void CreateBoxFrustum(camera_t *cam, float rad)
{
	cam->frustum[0][0] =  1;
	cam->frustum[0][1] =  0;
	cam->frustum[0][2] =  0;
	cam->frustum[0][3] = -(cam->pos[0]-rad);

	cam->frustum[1][0] = -1;
	cam->frustum[1][1] =  0;
	cam->frustum[1][2] =  0;
	cam->frustum[1][3] =  (cam->pos[0]+rad);

	cam->frustum[2][0] =  0;
	cam->frustum[2][1] =  1;
	cam->frustum[2][2] =  0;
	cam->frustum[2][3] = -(cam->pos[1]-rad);

	cam->frustum[3][0] =  0;
	cam->frustum[3][1] = -1;
	cam->frustum[3][2] =  0;
	cam->frustum[3][3] =  (cam->pos[1]+rad);

	cam->frustum[4][0] =  0;
	cam->frustum[4][1] =  0;
	cam->frustum[4][2] =  1;
	cam->frustum[4][3] = -(cam->pos[2]-rad);

	cam->frustum[5][0] =  0;
	cam->frustum[5][1] =  0;
	cam->frustum[5][2] = -1;
	cam->frustum[5][3] =  (cam->pos[2]+rad);
}

/*****************************************************************************/
/*  Shadow Volumes                                                           */
/*****************************************************************************/
void ExtrudeVertex(float *from, float *to, float *dest)
{
	vec3_t dir;
	VectorSubtract(to, from, dir);
	VectorCopy3(dir, dest);
	dest[3] = 0;
}

// Create extruded quad
// winding is v1 -> ev1 -> ev2 -> v2
void AddExtrudedEdge(light_t *lt, float *v1, float *v2)
{
	int i = nshadow_quads;
	if (nshadow_quads >= MAX_SHADOW_QUADS) error("Too many shadow quads!");

	VectorCopy3(v1, shadow_quad[i+0]); shadow_quad[i+0][3]=1.0;
	ExtrudeVertex(lt->cam.pos, v1, shadow_quad[i+1]);
	ExtrudeVertex(lt->cam.pos, v2, shadow_quad[i+2]);
	VectorCopy3(v2, shadow_quad[i+3]); shadow_quad[i+3][3]=1.0;

	quad_elem[i+0] = i+0;
	quad_elem[i+1] = i+1;
	quad_elem[i+2] = i+2;
	quad_elem[i+3] = i+3;

	nshadow_quads+=4;
}


void ComputeShadowVolumeMesh(light_t *lt, mesh_t *m)
{
	int j, front[1024];
	float d, *v1, *v2, *v3;
	if (m->numtris > 1024) error("ComputeShadowVolumeMesh: give more tris!\n");

	// determine front facing triangles
	// extrude front and back caps
	for (int j=0; j<m->numtris; j++) {
		int moff = j*3;

		v1 = m->vertex[m->meshvert[moff + 0]].p;
		v2 = m->vertex[m->meshvert[moff + 1]].p;
		v3 = m->vertex[m->meshvert[moff + 2]].p;
		
		// triangle faces light?
		front[j] = 0;
		d = DotProduct(m->trinorm[j], v1);
		if (DotProduct(m->trinorm[j], lt->cam.pos) < d) continue;

		front[j] = 1;
		draw_num_volume_caps += 2;
		lt->num_volume_caps += 2;
		
		if (nshadow_tris+6 >= MAX_SHADOW_TRIS) 
			error("Too many shadow tris!\n");
		
		// front cap
		int nst = nshadow_tris;
		shadow_tris[nst+0][3] = 1.0;
		shadow_tris[nst+1][3] = 1.0;
		shadow_tris[nst+2][3] = 1.0;
		VectorCopy3(v1, shadow_tris[nst+0]); 
		VectorCopy3(v2, shadow_tris[nst+1]);
		VectorCopy3(v3, shadow_tris[nst+2]);
		tri_elem[ntri_elem++] = nst+0;
		tri_elem[ntri_elem++] = nst+1;
		tri_elem[ntri_elem++] = nst+2;
		nshadow_tris+=3; nst+=3;

		// extrude the vertices for the backface
		// note these are put in opposite order
		ExtrudeVertex(lt->cam.pos, v1, shadow_tris[nst+2]);
		ExtrudeVertex(lt->cam.pos, v2, shadow_tris[nst+1]);
		ExtrudeVertex(lt->cam.pos, v3, shadow_tris[nst+0]);
		tri_elem[ntri_elem++] = nst+0;
		tri_elem[ntri_elem++] = nst+1;
		tri_elem[ntri_elem++] = nst+2;
		nshadow_tris+=3;
	}

	// extrude silhouette edges
	for (j = 0; j<m->numedges; j++) {
		if (m->edgeface[j][1] != -1) {  // edge on two triangles

			// silhouette edge?
			if ((front[m->edgeface[j][0]] ^ front[m->edgeface[j][1]])==0)
				continue;

			// determine order
			if (front[m->edgeface[j][0]]) {
				v1 = m->vertex[m->edgevert[j][0]].p;
				v2 = m->vertex[m->edgevert[j][1]].p;
			} else {
				v1 = m->vertex[m->edgevert[j][1]].p;
				v2 = m->vertex[m->edgevert[j][0]].p;
			}

		} else {  // edge with only one triangle
			if (!front[m->edgeface[j][0]]) continue;
			v1 = m->vertex[m->edgevert[j][0]].p;
			v2 = m->vertex[m->edgevert[j][1]].p;
		}		
		draw_num_volume_sides++;
		lt->num_volume_sides++;

		AddExtrudedEdge(lt, v1, v2);
	}
}

// add front faces to shadow volume caps
void AddFrontFaces(light_t *lt, face_t *f)
{
	if (f->numverts + nshadow_tris >= MAX_SHADOW_TRIS) 
		error("more than MAX_SHADOW_TRIS vertexes!");
	if (f->nummesh + ntri_elem >= MAX_SHADOW_TRIS) 
		error("more then MAX_SHADOW_TRIS elems");
	int j;
	int fv = f->firstvert;
	int fm = f->firstmesh;

	for (j=0; j<f->numverts; j++) {
		VectorCopy3(vertex[fv+j].p, shadow_tris[nshadow_tris+j]);
		shadow_tris[nshadow_tris+j][3] = 1.0;
	}
	for (j=0; j<f->nummesh; j++) {
		tri_elem[ntri_elem+j] = nshadow_tris+meshvert[fm+j];
	}
	nshadow_tris += f->numverts;
	ntri_elem += f->nummesh;
}

// add back faces to shadow volume caps
void AddBackFaces(light_t *lt, face_t *f)
{
	if (f->numverts + nshadow_tris >= MAX_SHADOW_TRIS) 
		error("more than MAX_SHADOW_TRIS vertexes!");
	if (f->nummesh + ntri_elem >= MAX_SHADOW_TRIS) 
		error("more then MAX_SHADOW_TRIS elems");
	int j;
	int fv = f->firstvert;
	int fm = f->firstmesh;

	for (j=0; j<f->numverts; j++) {
		ExtrudeVertex(lt->cam.pos, vertex[fv+j].p, shadow_tris[nshadow_tris+j]);
	}
	for (j=0; j<f->nummesh/3; j++) {
		tri_elem[ntri_elem+j*3+0] = nshadow_tris+meshvert[fm+j*3+2];
		tri_elem[ntri_elem+j*3+1] = nshadow_tris+meshvert[fm+j*3+1];
		tri_elem[ntri_elem+j*3+2] = nshadow_tris+meshvert[fm+j*3+0];
	}
	nshadow_tris += f->numverts;
	ntri_elem += f->nummesh;
}

// Calculate volumes for light
void CalculateShadowVolume(light_t *lt)
{
	int i,j,fv,fm;
	float d;
	face_t *f;

	if (num_vertices >= MAX_VERTICES) error("Too many vertices!");
	if (num_faces >= MAX_FACES) error("Too many faces!");

	// go through and mark all used silhouette edges
	nshadow_quads = 0;
	nshadow_tris = 0;
	ntri_elem = 0;
	memset(frontface,0, sizeof(frontface));

	// find all valid front-facing faces
	for (i=0, f=&face[0]; i<num_faces; i++, f++) {
		if (f->numedges == -1) continue;
		if (!lt->cam.visface[i]) continue;

		// Make sure light is on front side of face
		if (f->type == FACE_POLYGON) {
			d = DotProduct(f->norm, vertex[f->firstvert].p);
			if (DotProduct(f->norm, lt->cam.pos) < d) continue;
		}
		frontface[i] = 1;
	}

	for (i=0, f=&face[0]; i<num_faces; i++, f++) {
		if (!frontface[i]) continue;
		
		switch(f->type) {
		case FACE_POLYGON:
			// if edge is shared between exactly two faces:
			//   -- if both faces are visible to light; skip it.
			//   -- otherwise include it.
			// if this edge is shared between more then two faces:
			//   -- FIXME: we force the extrusion...?
			
			// FIXME: polygons can overlap.  This means "edge_cull" can
			// cause some visual anomalies. 
			// If we can check for overlap (PolyInPoly or some such) then 
			// this should be fine. 

			for (j=0; j<f->numedges; j++) {

				if ((f->nedgefaces[j] == 1) && cvar_int(edge_cull)) {
					if (frontface[f->edgeface[j][0]])
						continue;
				}
				
				draw_num_volume_sides++;
				lt->num_volume_sides++;

				AddExtrudedEdge(lt, vertex[f->edge[j][0]].p, 
						    vertex[f->edge[j][1]].p);
			}
			
			// Create front and back caps
			draw_num_volume_caps += 2;
			lt->num_volume_caps += 2;

			AddFrontFaces(lt, f);
			AddBackFaces(lt, f);
			break;

		case FACE_MESH:
			ComputeShadowVolumeMesh(lt, f->mesh);
			break;

		case FACE_PATCH:
			int npx = (f->size[0]-1)/2;
			int npy = (f->size[1]-1)/2;
			for (int a=0; a<npy; a++) for (int b=0; b<npx; b++) {
				bezier_t *bz = &f->bezier[a*npx + b];
				ComputeShadowVolumeMesh(lt, bz->mesh);
			}
			break;
		}
	}

	// compute the volumes for any enemies
	ComputeShadowVolumeEnemies(lt);
}

void DrawShadowVolume_Visible(light_t *lt)
{
	int i;

#if 0
	// front and back caps
	glColor3f(0,0,1);
	glEnableClientState(GL_VERTEX_ARRAY);	
	glVertexPointer(4, GL_FLOAT, sizeof(vec4_t), shadow_tris[0]);
	glDrawElements(GL_TRIANGLES, ntri_elem,
		       GL_UNSIGNED_INT, tri_elem);
	glDisableClientState(GL_VERTEX_ARRAY);
#endif

	// sides
	glColor3f(1, 0, 1);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(4, GL_FLOAT, sizeof(vec4_t), shadow_quad[0]);
	glDrawElements(GL_QUADS, nshadow_quads,
		       GL_UNSIGNED_INT, quad_elem);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void DrawShadowVolume_Stencil_old()
{
	// front and back caps
	glVertexPointer(4, GL_FLOAT, sizeof(vec4_t), shadow_tris[0]);
	glDrawElements(GL_TRIANGLES, ntri_elem,
		       GL_UNSIGNED_INT, tri_elem);

	// sides
	glVertexPointer(4, GL_FLOAT, sizeof(vec4_t), shadow_quad[0]);
	glDrawElements(GL_QUADS, nshadow_quads,
		       GL_UNSIGNED_INT, quad_elem);
}

// Draws back facing then front face shadow volume caps/sides
// Tries to use CVAs for speed increase
void DrawShadowVolume_Stencil()
{
	// without CVAs
	if (!gl_ext_compiled_vertex_array || !gl_ext_stencil_wrap) {
		glCullFace(GL_FRONT);                      // backfacing + fail = increment
		glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
		DrawShadowVolume_Stencil_old();
		
		glCullFace(GL_BACK);                       // frontfacing + fail = decrement
		glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
		DrawShadowVolume_Stencil_old();
		return;
	}

	/////////////////////////////
	// volume front and back caps
	/////////////////////////////
	glVertexPointer(4, GL_FLOAT, sizeof(vec4_t), shadow_tris[0]);
	glLockArraysEXT(0, nshadow_tris);

	// back faces first
	glCullFace(GL_FRONT);
	glStencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);
	glDrawElements(GL_TRIANGLES, ntri_elem, GL_UNSIGNED_INT, tri_elem);

	// front faces second
	glCullFace(GL_BACK); 
	glStencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
	glDrawElements(GL_TRIANGLES, ntri_elem, GL_UNSIGNED_INT, tri_elem);

	glUnlockArraysEXT();
	
	//////////////////////////
	// volume sides
	//////////////////////////
	glVertexPointer(4, GL_FLOAT, sizeof(vec4_t), shadow_quad[0]);
	glLockArraysEXT(0, nshadow_tris);

	// back faces first
	glCullFace(GL_FRONT);
	glStencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);
	glDrawElements(GL_QUADS, nshadow_quads, GL_UNSIGNED_INT, quad_elem);

	// front faces second
	glCullFace(GL_BACK); 
	glStencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
	glDrawElements(GL_QUADS, nshadow_quads, GL_UNSIGNED_INT, quad_elem);

	glUnlockArraysEXT();
}


/*****************************************************************************/
/*  General Light Stuff                                                      */
/*****************************************************************************/

// setup the eye-space to light-clip-space transform
void LightSetTextureMatrix(light_t *lt)
{
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(0.5, 0.5, 0.5);
	glScalef(0.5, 0.5, 0.5);
	gluPerspective(lt->cam.fov, (float)lt->cam.viewwidth/lt->cam.viewheight, 1, 1024);
	gluLookAt(lt->cam.pos[0],  lt->cam.pos[1], lt->cam.pos[2],
		  lt->cam.pos[0] + lt->cam.vn[0], 
		  lt->cam.pos[1] + lt->cam.vn[1], 
		  lt->cam.pos[2] + lt->cam.vn[2],
		  lt->cam.up[0], lt->cam.up[1], lt->cam.up[2]);
	glMatrixMode(GL_MODELVIEW);
}


void LightSetClipPlanes(light_t *lt)
{
	glClipPlane(GL_CLIP_PLANE0, lt->cam.frustum[RIGHT]);
	glClipPlane(GL_CLIP_PLANE1, lt->cam.frustum[LEFT]);
	glClipPlane(GL_CLIP_PLANE2, lt->cam.frustum[BOTTOM]);
	glClipPlane(GL_CLIP_PLANE3, lt->cam.frustum[TOP]);
	glClipPlane(GL_CLIP_PLANE4, lt->cam.frustum[FRONT]);
	glClipPlane(GL_CLIP_PLANE5, lt->cam.frustum[BACK]);
}


void LightSetAttributes(light_t *lt)
{
	glLightfv(GL_LIGHT1, GL_POSITION, lt->cam.pos);
	glLightfv(GL_LIGHT1, GL_AMBIENT, lt->ambient_color);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, lt->diffuse_color);
	glLightfv(GL_LIGHT1, GL_SPECULAR, lt->specular_color);
	glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, lt->constant_factor);
	glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, lt->linear_factor);
	glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, lt->quadratic_factor);

	// FIXME: spotlights not used... need per-pixel lighting to look nice.
	if (lt->spotlight) {
		glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 0);
		glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 180);
#if 0
		glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, lt->cam.vn);
		glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, lt->spot_exponent);
		glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, lt->spot_angle);
#endif
	} else {
		glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 0);
		glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 180);
	}

	glEnable(GL_LIGHT1);
}

void InverseMatrixd(double *mat, double *out)
{
	int i,j;
	for (i=0; i<4; i++) for (j=0; j<4; j++) {
		out[i*4+j] = mat[j*4+i];		
	}
}


void ComputeCenteredBBox(vec3_t center, float rad, bbox_t out)
{
	int a,i,j,k,s[] = {-1,1};
	for (i=a=0; i<2; i++) for (j=0; j<2; j++) for (k=0; k<2; k++,a++) {
		VectorCopy3(center, out[a]);
		out[a][0] += s[i]*rad;
		out[a][1] += s[j]*rad;
		out[a][2] += s[k]*rad;
	}

}

// FIXME: handle aspect ratio properly
// FIXME: speed this up!
void LightComputeBBox(light_t *lt)
{
	if (!strcmp(draw_shadow_mode->value,"hard_volume")) {

		ComputeCenteredBBox(lt->cam.pos, lt->extent, lt->bbox);
		CreateBoxFrustum(&lt->cam, lt->extent);
		return;

	}

	double ax = tan(deg2rad(lt->cam.fov/2));
	double ay = tan(deg2rad(lt->cam.fov/2));

	VectorCopy3(lt->cam.pos, lt->bbox[0]);
	VectorMA(lt->bbox[0],     lt->cam.nearp, lt->cam.vn, lt->bbox[0]);
	VectorMA(lt->bbox[0],  lt->cam.nearp*ay, lt->cam.up, lt->bbox[0]);
	VectorMA(lt->bbox[0], -lt->cam.nearp*ax, lt->cam.rt, lt->bbox[0]);
	glVertex3fv(lt->bbox[0]);

	VectorCopy(lt->cam.pos, lt->bbox[1], 3);
	VectorMA(lt->bbox[1],     lt->cam.nearp, lt->cam.vn, lt->bbox[1]);
	VectorMA(lt->bbox[1],  lt->cam.nearp*ay, lt->cam.up, lt->bbox[1]);
	VectorMA(lt->bbox[1],  lt->cam.nearp*ax, lt->cam.rt, lt->bbox[1]);
	glVertex3fv(lt->bbox[1]);

	VectorCopy(lt->cam.pos, lt->bbox[2], 3);
	VectorMA(lt->bbox[2],     lt->cam.nearp, lt->cam.vn, lt->bbox[2]);
	VectorMA(lt->bbox[2], -lt->cam.nearp*ay, lt->cam.up, lt->bbox[2]);
	VectorMA(lt->bbox[2], -lt->cam.nearp*ax, lt->cam.rt, lt->bbox[2]);
	glVertex3fv(lt->bbox[2]);

	VectorCopy(lt->cam.pos, lt->bbox[3], 3);
	VectorMA(lt->bbox[3],     lt->cam.nearp, lt->cam.vn, lt->bbox[3]);
	VectorMA(lt->bbox[3], -lt->cam.nearp*ay, lt->cam.up, lt->bbox[3]);
	VectorMA(lt->bbox[3],  lt->cam.nearp*ax, lt->cam.rt, lt->bbox[3]);
	glVertex3fv(lt->bbox[3]);

	VectorCopy(lt->cam.pos, lt->bbox[4], 3);
	VectorMA(lt->bbox[4],     lt->cam.farp, lt->cam.vn, lt->bbox[4]);
	VectorMA(lt->bbox[4],  lt->cam.farp*ay, lt->cam.up, lt->bbox[4]);
	VectorMA(lt->bbox[4], -lt->cam.farp*ax, lt->cam.rt, lt->bbox[4]);
	glVertex3fv(lt->bbox[4]);

	VectorCopy(lt->cam.pos, lt->bbox[5], 3);
	VectorMA(lt->bbox[5],     lt->cam.farp, lt->cam.vn, lt->bbox[5]);
	VectorMA(lt->bbox[5],  lt->cam.farp*ay, lt->cam.up, lt->bbox[5]);
	VectorMA(lt->bbox[5],  lt->cam.farp*ax, lt->cam.rt, lt->bbox[5]);
	glVertex3fv(lt->bbox[5]);

	VectorCopy(lt->cam.pos, lt->bbox[6], 3);
	VectorMA(lt->bbox[6],     lt->cam.farp, lt->cam.vn, lt->bbox[6]);
	VectorMA(lt->bbox[6], -lt->cam.farp*ay, lt->cam.up, lt->bbox[6]);
	VectorMA(lt->bbox[6], -lt->cam.farp*ax, lt->cam.rt, lt->bbox[6]);
	glVertex3fv(lt->bbox[6]);

	VectorCopy(lt->cam.pos, lt->bbox[7], 3);
	VectorMA(lt->bbox[7],    lt->cam.farp, lt->cam.vn, lt->bbox[7]);
	VectorMA(lt->bbox[7], -lt->cam.farp*ay, lt->cam.up, lt->bbox[7]);
	VectorMA(lt->bbox[7],  lt->cam.farp*ax, lt->cam.rt, lt->bbox[7]);
	glVertex3fv(lt->bbox[7]);
}

int ldir = -1;
void UpdateLight(light_t *lt, int move)
{
	// update position
	if (move) {
		lt->cam.pos[0] += 0.0*ldir;
		lt->cam.pos[2] += 0.5*ldir;

		if (lt->cam.pos[0] > 900) {
			ldir = -1;
		} else if (lt->cam.pos[0] < 500) {
			ldir = 1;
		}
		if (lt->cam.pos[2] > 700) {
			ldir = -1;
		} else if (lt->cam.pos[2] < 200) {
			ldir = 1;
		}
	}

	// compute view, determine visible faces
	lt->num_volume_caps = 0;
	lt->num_volume_sides = 0;

	lt->cam.drew_triangles = 0;
	lt->cam.drew_polygons = 0;
	lt->cam.drew_meshes = 0;
	lt->cam.drew_patches = 0;

	CameraUpdateView(&lt->cam, 0);
	LightComputeBBox(lt);
	DetermineVisibleFaces(&lt->cam, 1);

	lt->affects_scene = 0;
	if (lt->on) {
		int i;

		// if the light and player share no visible faces
		// we can ignore this light
		for (i=0; i<num_faces; i++) {
			if (lt->cam.visface[i] && player.visface[i]) break;
		}
		if (i == num_faces) return;

		// if light extent is in frustum we need to consider it 
		if (BoxInFrustum(&player, lt->bbox))
			lt->affects_scene = 1;
	}
}

void UpdateLights()
{
	for (int i=0; i<num_lights; i++) {
		UpdateLight(&light[i], i==0);
	}
}


void AddLight(float *pos, 
	      float fov, 
	      float *diffuse, 
	      float yaw, float pitch, float roll,
	      float constant, float linear, float quadratic)
{
	light_t *lt = &light[num_lights++];

	lt->cam.pos[0] = pos[0];
	lt->cam.pos[1] = pos[1];
	lt->cam.pos[2] = pos[2];
	lt->cam.pos[3] = 1.0;

	lt->cam.yaw = yaw;
	lt->cam.pitch = pitch;
	lt->cam.roll = roll;

	lt->cam.fov = fov;
	lt->cam.nearp = 1.0;
	lt->cam.farp = 1024.0f;
	lt->cam.viewwidth = lt->cam.viewheight = shadowMapSize;

	VectorCopy(black,   lt->ambient_color,  4);
	VectorCopy(diffuse, lt->diffuse_color,  4);
	VectorCopy(black,   lt->specular_color, 4);

	lt->constant_factor = constant;
	lt->linear_factor = linear;
	lt->quadratic_factor = quadratic;

	lt->on = 1;

	// FIXME: calculate this properly
	lt->extent = 256;
}

