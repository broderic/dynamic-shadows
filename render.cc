#include "shad.h"

// drawing states
int r_blend_src = GL_ONE;
int r_blend_dst = GL_ZERO;

int r_cull = -1;
int r_depth_mask = -1;
int r_depth_func = -1;

int r_alpha_func = ALPHA_FUNC_NONE;

// texture unit states
typedef struct {
	int enabled;
	GLuint texture;
	int combine;
} tu_t;
tu_t tu[8];

// lists we use for drawing
int r_numverts;
vec4_t vertlist[2024];
vec4_t normlist[2024];
vec4_t colorlist[2024];
int tclightmap;
float tclist1[2024][2];          // holds unmodified coords

int r_numelems;
unsigned int elemlist[8096];

double frame_time;


// drawing statistics
int draw_num_triangles;
int draw_num_polygons;
int draw_num_meshes;
int draw_num_patches;

// cvars and ccmds
cvar_t *vis_force_all;
cvar_t *draw_model_bbox;



void InitRender()
{
	register_cvar(&vis_force_all, "vis_force_all", "0");

	register_cvar(&draw_model_bbox, "draw_model_bbox", "0");

	tu[0].enabled = 0;
	tu[0].texture = 11019423;
	tu[1].enabled = 0;
	tu[1].texture = 12319023;
}

/****************************************************************************/
/*  Wave functions                                                          */
/****************************************************************************/
float Wave(int type, double base, double amp, double phase, double freq)
{
	double ret = 0;
	double t = fmod(frame_time * freq + phase, 1.0f);
	
	switch(type) {
	case WAVE_SINE:
		ret = sin(t*2*M_PI);
		break;

	case WAVE_TRIANGLE:
		if (t < 0.5) ret = 4*t - 1;
		else ret = -4*t + 3;
		break;

	case WAVE_SQUARE:
		ret = 1;
		if (t >= 0.5) ret = -1;
		break;

	case WAVE_SAWTOOTH:
		ret = t;
		break;
		
	case WAVE_INV_SAWTOOTH:
		ret = 1.0 - t;
		break;
 
	case WAVE_NOISE:
		ret = (float)rand()/ RAND_MAX;
	}
	return base + amp*ret;
}

void SetTextureModes()
{
	if (cvar_int(draw_flat)) {
		DisableTextures();
		return;
	}

	// Texture Unit 0  -- draws lightmap
	glActiveTextureARB(GL_TEXTURE0_ARB);
	if (!cvar_int(draw_lighting)) {
		tu[0].enabled = 0;
		glDisable(GL_TEXTURE_2D);
	} else {
		tu[0].enabled = 1;
		tu[0].texture = 0;
		glEnable(GL_TEXTURE_2D);
	}
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);	
	glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD); 

	// Texture Unit 1  -- draws surface texture
	glActiveTextureARB(GL_TEXTURE1_ARB);
	if (!cvar_int(draw_texture)) {
		tu[1].enabled = 0;
		glDisable(GL_TEXTURE_2D);
	} else {
		tu[1].enabled = 1;
		glEnable(GL_TEXTURE_2D);
	}
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	if (cvar_int(draw_lighting)) {  // modulate with lightmap
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	} else {                       // otherwise add
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
	}
}


void SetTextureModes_Lighting()
{
	if (cvar_int(draw_flat)) {
		DisableTextures();
		return;
	}

	// Texture Unit 0  -- draws lightmap
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);	
	glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE); 

	// Texture Unit 1  -- draws surface texture
	glActiveTextureARB(GL_TEXTURE1_ARB);
	if (!cvar_int(draw_texture)) {
		tu[0].enabled = 0;
		glDisable(GL_TEXTURE_2D);
	} else {
		tu[0].enabled = 1;
		glEnable(GL_TEXTURE_2D);
	}
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	if (cvar_int(draw_lighting)) {  // modulate with lightmap
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	} else {                       // otherwise add
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
	}
}

/****************************************************************************/
/* OpenGL render states                                                     */
/****************************************************************************/
void SetColor(layer_t *layer)
{
	float c;
	for (int i=0; i<r_numverts; i++) {
		switch(layer->rgbgen) {
		case RGBGEN_IDENTITY:
		case RGBGEN_IDENTITY_LIGHTING:
			VectorSet4(1,1,1,1,colorlist[i]);
			break;
		case RGBGEN_VERTEX:
			// FIXME: implement this
			VectorSet4(1,1,1,1,colorlist[i]);
			break;

		case RGBGEN_ONE_MINUS_VERTEX:
			// FIXME: implement this
			VectorSet4(1,1,1,1,colorlist[i]);
			break;

		case RGBGEN_ENTITY:
			// FIXME: implement this
			VectorSet4(1,1,1,1,colorlist[i]);
			break;
			
		case RGBGEN_ONE_MINUS_ENTITY:
			// FIXME: implement this
			VectorSet4(1,1,1,1,colorlist[i]);
			break;
		      
		case RGBGEN_LIGHTING_DIFFUSE:
			// FIXME: implement this
			VectorSet4(1,1,1,1,colorlist[i]);
			break;
			
		case RGBGEN_WAVE:
			c = Wave((int)layer->rgbarg[0], 
				 layer->rgbarg[1], layer->rgbarg[2], 
				 layer->rgbarg[3], layer->rgbarg[4]);
			VectorSet4(c,c,c,1.0,colorlist[i]);
			break;

		default: // FIXME: need this?
			VectorSet4(1,1,1,1,colorlist[i]);
		}
	}
}

int blendingOn(int src, int dst)
{
	if (src == GL_ZERO && dst == GL_ONE) return 0;
	if (src == GL_ONE && dst == GL_ZERO) return 0;
	return 1;
}

void SetBlend(int src, int dst)
{
	if (blendingOn(src, dst)) {
		if (!blendingOn(r_blend_src, r_blend_dst)) 
			glEnable(GL_BLEND);
		
		if (src != r_blend_src || dst != r_blend_dst) 
			glBlendFunc(src, dst);
	} else {
		if (blendingOn(r_blend_src, r_blend_dst))
			glDisable(GL_BLEND);
	}
	r_blend_src = src;
	r_blend_dst = dst;
}

void SetTexture(int unit, GLuint name)
{
	if (tu[unit].texture == name) return;

	glActiveTextureARB(GL_TEXTURE0_ARB + unit);
	glBindTexture(GL_TEXTURE_2D, name);
	tu[unit].texture = name;
	if (!tu[unit].enabled) {
		glEnable(GL_TEXTURE_2D);
		tu[unit].enabled = 1;
	}
}

void SetCulling(int cull)
{
	if (cull != GL_NONE) {
		if (r_cull == GL_NONE) glEnable(GL_CULL_FACE);
		glCullFace(cull);
	} else {
		if (r_cull != GL_NONE) glDisable(GL_CULL_FACE);
	}
	r_cull = cull;
}

void SetAlphaFunc(int func)
{
	if (func != ALPHA_FUNC_NONE) {
		if (r_alpha_func == ALPHA_FUNC_NONE) 
			glEnable(GL_ALPHA_TEST);

		switch(func) {
		case ALPHA_FUNC_GT0:
			glAlphaFunc(GL_GREATER, 0);
			break;
		case ALPHA_FUNC_LT128:
			glAlphaFunc(GL_LESS, 128);
			break;
		case ALPHA_FUNC_GE128:
			glAlphaFunc(GL_GEQUAL, 128);
			break;
		}
	} else {
		glDisable(GL_ALPHA_TEST);
	}
	r_alpha_func = func;
}

void SetDepthFunc(int func)
{
	if (func != r_depth_func) {
		glDepthFunc(func);
		r_depth_func = func;
	}
}

void SetDepthMask(shader_t *sh, layer_t *layer)
{
	int cur = 1;
	if (ShaderTranslucent(sh) && !layer->depth_mask) cur = 0;
	if (cur != r_depth_mask) {
		glDepthMask(cur);
		r_depth_mask = cur;
	}
}

/****************************************************************************/
/* Create the compiled vertex/normal arrays                                 */
/****************************************************************************/
void SetMeshVertexAndNormalArrays(mesh_t *m)
{
	if (r_numverts + m->numverts > 2024) error("too few verts!\n");
	if (r_numelems + m->numtris*3 > 8096) error("too few elems!\n");

	int i;
	draw_num_triangles += m->numtris;
	for (i=0; i<m->numverts; i++) {
		VectorCopy3(m->vertex[i].p, vertlist[r_numverts+i]);
		VectorCopy3(m->vertex[i].norm, normlist[r_numverts+i]);
	}
	for (i=0; i<m->numtris*3; i++) {
		elemlist[r_numelems+i] = r_numverts+m->meshvert[i];
	}
	r_numverts += m->numverts;
	r_numelems += m->numtris*3;
}

// Create the list of vertices we'll use for glDrawElements
void SetVertexAndNormalArrays(face_t *f)
{
	r_numverts = 0;
	r_numelems = 0;
	
	switch(f->type) {
	case FACE_POLYGON:
	case FACE_MESH:
		SetMeshVertexAndNormalArrays(f->mesh);
		break;
	case FACE_PATCH:
		int numpatch = ((f->size[0]-1)/2) * ((f->size[1]-1)/2);
		for (int a=0; a<numpatch; a++) {
			bezier_t *bz = &f->bezier[a];
			SetMeshVertexAndNormalArrays(bz->mesh);
		}
	}
}

// Deform the vertices according the the shader 
// FIXME: implement!
void DeformVertices(shader_t *sh)
{
	mod_t *cur;
	for (int i=0; i<r_numverts; i++) {
		for (cur = sh->vertmod; cur; cur=cur->next) {
			switch(cur->type) {
			case DEFORM_WAVE:
				break;
			case DEFORM_NORMAL:
				break;
			case DEFORM_BULGE:
				break;
			case DEFORM_MOVE:
				break;
			case DEFORM_AUTOSPRITE:
				break;
			case DEFORM_AUTOSPRITE2:
				break;
			case DEFORM_PROJECTION:
				break;
			case DEFORM_TEXT0:
				break;
			case DEFORM_TEXT1:
				break;
		       }
		}
	}
}

/****************************************************************************/
/* Create the compiled texture array                                        */
/****************************************************************************/
void SetMeshTCArray(mesh_t *m, layer_t *layer, camera_t *cam, int j)
{
	vec3_t dir;
	int from = TCGEN_BASE;
	if (layer->numtex == -1) from = TCGEN_LIGHTMAP;
	if (layer->tcgen) from = layer->tcgen;

	for (int i=0; i<m->numverts; i++, j++) { 
		switch(from){
		case TCGEN_BASE:
			tclist1[j][0] = m->vertex[i].tc[0]; 
			tclist1[j][1] = m->vertex[i].tc[1]; 
			break;
		case TCGEN_LIGHTMAP:
			tclist1[j][0] = m->vertex[i].lc[0];
			tclist1[j][1] = m->vertex[i].lc[1];
			break;
		case TCGEN_ENVIRONMENT:
			VectorSubtract(cam->pos, m->vertex[i].p, dir);
			VectorNormalize(dir);
			tclist1[j][0] = dir[2]-m->vertex[i].norm[2];
			tclist1[j][1] = dir[1]-m->vertex[i].norm[1];
			break;
		}
	}
}

// Extract the proper texture coordinates for each vertex to be drawn
void SetTCArray(face_t *f, layer_t *layer, camera_t *cam)
{
	switch(f->type) {
	case FACE_POLYGON:
	case FACE_MESH:
		SetMeshTCArray(f->mesh, layer, cam, 0);
		break;
	case FACE_PATCH:
		int j=0;
		int numpatch = ((f->size[0]-1)/2) * ((f->size[1]-1)/2);
		for (int a=0; a<numpatch; a++) {
			bezier_t *bz = &f->bezier[a];
			SetMeshTCArray(bz->mesh, layer, cam, j);
			j += bz->mesh->numverts;
		}
	}
}

// Deform the current list by tcmods in 'layer'
// NOTE: modification is in place
void DeformTCList(layer_t *layer)
{
	if (!layer->tcmod) return;

	float s, t, s1, t1, freq,angle;
	mod_t *cur;
	double tt = frame_time;

	for (int i=0; i<r_numverts; i++) {
		s = tclist1[i][0];
		t = tclist1[i][1];
		for (cur = layer->tcmod; cur; cur=cur->next) {
			switch(cur->type) {
			case TCMOD_SCALE:
				s *= cur->arg[0];
				t *= cur->arg[1];
				break;

			case TCMOD_SCROLL:
				s += fmod(cur->arg[0]*tt, 1.0);
				t -= fmod(cur->arg[1]*tt, 1.0);
				break;

			case TCMOD_ROTATE:
				freq = -cur->arg[0] / 360.0;
				angle = fmod(tt*freq, 1.0)*2*M_PI;
				s1 = s - 0.5;	
				t1 = t - 0.5;
				s = s1*cos(angle) - t1*sin(angle) + 0.5;
				t = s1*sin(angle) + t1*cos(angle) + 0.5;
				break;

			case TCMOD_STRETCH:
				// TODO: implement!
				break;

			case TCMOD_TURB:
				// FIXME: is this correct?
				t1 = cur->arg[2] + tt*cur->arg[3];
				s += sin(s*cur->arg[1]+t1)*cur->arg[1];
				t += sin(t*cur->arg[1]+t1)*cur->arg[1];
				break;

			case TCMOD_TRANSFORM:
				s1 = s;
				t1 = t;
				s = s1*cur->arg[0] + s1*cur->arg[2] + cur->arg[4];
				t = t1*cur->arg[1] + t1*cur->arg[3] + cur->arg[5];
			}
		}
		tclist1[i][0] = s;
		tclist1[i][1] = t;
	}
}

// Determine texture for this layer
GLuint LayerGetTexture(face_t *f, layer_t *layer)
{
	if (layer->numtex == -1) {
		return f->lightmap->name;
	}

	if (layer->numtex == 0) error ("no texture on surface '%s'!\n", f->surface->shader->name);

	int current = 0;
	if (layer->numtex > 1) {
		double t = fmod(frame_time, 100.0);
		current = ((int)(t * layer->fps)) % layer->numtex;
		if (current < 0) current += layer->numtex;
	}

	if (!layer->texture[current]) {
		printf("WARNING! %s: %s: no texture!\n", 
		       f->surface->shader->name, layer->texname[current]);
		return 1;
	}

	return layer->texture[current]->name;
}

/*****************************************************************************/
/* Draw a world face using its shader                                        */
/*****************************************************************************/
void DrawFace(face_t *f, camera_t *cam)
{
	// Don't draw this surface.
	// Note that skies are drawn first before everything else.
	if ((f->surface->flags & SURF_SKIP) || 
	    (f->surface->flags & SURF_NODRAW) ||
	    (f->surface->flags & SURF_SKY)) return;

	switch(f->type) {
	case FACE_POLYGON:   
		if (!cvar_int(draw_polygons)) return; 
		draw_num_polygons++;
		break;
	case FACE_MESH:      
		if (!cvar_int(draw_meshes)) return; 
		draw_num_meshes++;
		break;
	case FACE_PATCH:
		draw_num_patches++;     
		if (!cvar_int(draw_patches)) return; 
		break;
	case FACE_BILLBOARD: 
		if (!cvar_int(draw_billboards)) return; 
		break;
	}

	shader_t *sh = f->surface->shader;

	// Setup compiled Vertex and Normal arrays
	SetVertexAndNormalArrays(f);
	DeformVertices(sh);

	// lock the arrays
	glVertexPointer(3, GL_FLOAT, 16, vertlist);
	glNormalPointer(GL_FLOAT, 16, normlist);
	glLockArraysEXT(0, r_numverts);

	// Set the culling mode for this shader
	SetCulling(sh->cull);

	//printf("%s\n", sh->name);
	for (int i=0; i<sh->numlayers; i++) {
		layer_t *layer = &sh->layer[i];
		//printf("%d: %s\n", i, layer->texname);

		// set vertex colors
		SetColor(layer);
		glColorPointer(4, GL_FLOAT, 16, colorlist);

		// bind the texture
		SetTexture(0, LayerGetTexture(f, layer));
		
		// set alpha comparison function
		SetAlphaFunc(layer->alpha_func);

		// set the blending function
		SetBlend(layer->blend_src, layer->blend_dst);

		// set whether or not we write to depth buffer
		SetDepthMask(sh, layer);

		// set the texture coordinate array
		SetTCArray(f, layer, cam);
		DeformTCList(layer);
		glTexCoordPointer(2, GL_FLOAT, 8, tclist1);

		// render this pass
		glDrawElements(GL_TRIANGLES, r_numelems,
			       GL_UNSIGNED_INT, elemlist);
	}
	glUnlockArraysEXT();
	//printf("done.\n");
}

// Set the initial rendering states 
void PreFrame()
{
	r_blend_src = GL_ONE;            // no blending
	r_blend_dst = GL_ZERO;
	glDisable(GL_BLEND);

	SetCulling(GL_BACK);

	glEnable(GL_DEPTH_TEST);
	SetDepthFunc(GL_LEQUAL);
        
	r_depth_mask = 1;
	glDepthMask(1);

	glColorMask(1, 1, 1, 1);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, (cvar_int(draw_wireframe)) ? GL_LINE : GL_FILL);

	glDisable(GL_STENCIL_TEST);
	glDisable(GL_LIGHTING);
	glShadeModel(GL_SMOOTH);

	glActiveTextureARB(GL_TEXTURE0_ARB);

	tu[0].enabled = 1;
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);	
	glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE); 
	tu[0].texture = 11019423;
	
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);
	tu[1].enabled = 0;
	tu[1].texture = 12319023;

	glActiveTextureARB(GL_TEXTURE0_ARB);

	frame_time = GetTime();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
}

// Cleanup the rendering states
void PostFrame()
{
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

/****************************************************************************/
/* Stuff to render world                                                    */
/****************************************************************************/

typedef struct {
	int faceid;
	float dist;
} facepair_t;

int r_numtrans;
facepair_t tfaces[4096];

// sort faces in decreasing order of distance from viewer
int facepair_compare(const void * p1, const void *p2)
{
	facepair_t *fp1 = (facepair_t*)p1;
	facepair_t *fp2 = (facepair_t*)p2; 
	if (fabs(fp1->dist - fp2->dist) < 1e-3) return 0;
	return (fp1->dist < fp2->dist) ? 1 : -1;
}

// sorts and draws all translucent faces in list created by DrawSolid
void DrawTranslucent(camera_t *cam)
{
	if (!cvar_int(draw_translucent)) return;
	
	glDisableClientState(GL_NORMAL_ARRAY);

	// FIXME: do better sorting; some anomalies are possible
	qsort(tfaces, r_numtrans, sizeof(facepair_t), facepair_compare);
	for (int i=0; i<r_numtrans; i++) {
		DrawFace(&face[tfaces[i].faceid], cam);
	}

	glEnableClientState(GL_NORMAL_ARRAY);
}

// draws all faces marked as visible to this camera
// stores all translucent faces into separate list
void DrawSolid(camera_t *cam)
{
	vec3_t v;
	face_t *f = face;

	r_numtrans = 0;
	for (int i=0; i<num_faces; i++, f++) {
		if (!cam->visface[i]) continue;
		if (FaceTranslucent(f)){
			VectorSubtract(vertex[f->firstvert].p, cam->pos, v);
			tfaces[r_numtrans].faceid = i;
			tfaces[r_numtrans].dist = DotProduct(v, v);
			r_numtrans++;
		} else {

			DrawFace(f, cam);

		}
	}
}

// Draw each model's bounding box
void DrawModelBBoxes(camera_t *cam)
{
	glColor4fv(white);
	glDisable(GL_TEXTURE_2D);
	for (int i=0; i<num_models; i++) {
		if (!model[i].numfaces) continue;
		if (!BoxInFrustum(cam, modelbox[i])) continue;
		DrawBBox(modelbox[i]);
	}
	glEnable(GL_TEXTURE_2D);
}

// Draw the world. First draws the solid faces, then
// draws any translucent faces encountered on first pass.
void DrawWorld(camera_t *cam)
{
	draw_num_triangles = 0;
	draw_num_polygons = 0;
	draw_num_meshes = 0;
	draw_num_patches = 0;

	DrawSolid(cam);
	//DrawEnemies();

	DrawTranslucent(cam);

	if (cvar_int(draw_model_bbox)) DrawModelBBoxes(cam);

	cam->drew_triangles = draw_num_triangles;
	cam->drew_polygons = draw_num_polygons;
	cam->drew_meshes = draw_num_meshes;
	cam->drew_patches = draw_num_patches;
}


//////////////////////////////////////////////////////////////////////////////
///////////// OLD ////////////////////////////////////////////////////////////
void DrawFace_old(face_t *f)
{
	int offset;
	int stride = sizeof(vertex_t);
	surf_t *sf = f->surface;
	shader_t *sh = sf->shader;

	// Don't draw this surface.
	// Note that skies are drawn first before everything else.
	if ((sf->flags & SURF_SKIP) || 
	    (sf->flags & SURF_NODRAW) ||
	    (sf->flags & SURF_SKY)) return;

	switch(f->type) {
	case FACE_POLYGON:   
		if (!cvar_int(draw_polygons)) return; 
		draw_num_polygons++;
		break;
	case FACE_MESH:      
		if (!cvar_int(draw_meshes)) return; 
		draw_num_meshes++;
		break;
	case FACE_PATCH:
		draw_num_patches++;     
		if (!cvar_int(draw_patches)) return; 
		break;
	case FACE_BILLBOARD: 
		if (!cvar_int(draw_billboards)) return; 
		break;
	}

	// lightmap
	SetTexture(0, f->lightmap->name);
	
	// texture 
	SetTexture(1, LayerGetTexture(f, &sh->layer[1]));

	switch(f->type) {
	case FACE_POLYGON:
	case FACE_MESH:
		offset = f->firstvert;
		
		draw_num_triangles += f->nummesh / 3;
		
		glVertexPointer(3, GL_FLOAT, stride, vertex[offset].p);
		glNormalPointer(GL_FLOAT, stride, vertex[offset].norm);
		glTexCoordPointer(2, GL_FLOAT, stride, vertex[offset].tc);

		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		glTexCoordPointer(2, GL_FLOAT, stride, vertex[offset].lc);

		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glTexCoordPointer(2, GL_FLOAT, stride, vertex[offset].tc);

		glDrawElements(GL_TRIANGLES, f->nummesh,
			       GL_UNSIGNED_INT, &meshvert[f->firstmesh]);
		break;

	case FACE_PATCH:
		int npx = (f->size[0]-1)/2;
		int npy = (f->size[1]-1)/2;

		for (int a=0; a<npy; a++) for (int b=0; b<npx; b++) {
			int pnum = a*npx + b;
			bezier_t *bz = &f->bezier[pnum];
			mesh_t *m = bz->mesh;

			draw_num_triangles += 2*bz->level*bz->level;
			
			glVertexPointer(3, GL_FLOAT, stride, m->vertex[0].p);
			glNormalPointer(GL_FLOAT, stride, m->vertex[0].norm);
			
			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			glTexCoordPointer(2, GL_FLOAT, stride, m->vertex[0].lc);
			
			glClientActiveTextureARB(GL_TEXTURE1_ARB);
			glTexCoordPointer(2, GL_FLOAT, stride, m->vertex[0].tc);
			
			glDrawElements(GL_TRIANGLES, m->numtris*3,
				       GL_UNSIGNED_INT, m->meshvert);
		}
	}
}
////////////// END OF OLD ///////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
