#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "shad.h"

int bsp_loaded = 0;

int bsplen;
void *bspdata;

header_t *header;

int num_surfs;
bspsurf_t *bspsurf;
surf_t *surf;

int num_planes;
plane_t *plane;

int num_nodes;
node_t *node;
bbox_t *nodebox;

int num_leafs;
leaf_t *leaf;
bbox_t *leafbox;

int *leafface;

int num_models;
model_t *model, *world;
bbox_t *modelbox;

int num_brushes;
brush_t *brush;

int num_brushsides;
brushside_t *brushside;

int num_vertices;
vertex_t *vertex;

unsigned int *meshvert;

int num_effects;
effect_t *effect;

int num_faces;
bspface_t *bspface;
face_t *face;

int num_lightmaps;
ubyte *lightmapdata;
texture_t *lighttexture[1024];
texture_t *light_full, *light_half;

vis_t *vis;

int ent_length;
char *entdata;


////////////////////////////////////////////////////////

float minf(float a, float b) {return (a<b)?a:b;}
float maxf(float a, float b) {return (a>b)?a:b;}


/*****************************************************************************/
/* Console cvars and commands                                                */
/*****************************************************************************/
ccmd_t *ccmd_load_map;

void ccmd_map(int argc, char **argv)
{
	if (argc < 1) {
		conprintf("usage: map <map name>");
		return;
	}

	char file[80] = "baseq3/maps/";
	strcat(file, argv[1]);
	strcat(file, ".bsp");

	if (bsp_loaded) {
		TextureInit();       	// flush old textures
		BSP_Free();             // free old map
	}

	BSP_Load(file);
	if (bsp_loaded) 
		console_state = CONSOLE_RISING;
}

void InitBSP()
{
	register_ccmd(&ccmd_load_map, "map", ccmd_map);
}


/*****************************************************************************/
/* BSP traversal                                                             */
/*****************************************************************************/
int FindLeaf(float *p)
{
	int cur=0;
	node_t *curn;
	plane_t *curp;
	double dist;

	while (cur >= 0) {
		curn = &node[cur];
		curp = &plane[curn->plane];

		dist = DotProduct(curp->n, p) - curp->d;
		cur = curn->child[1];

		// FIXME: epsilon trouble? 
		if (dist >= 0) cur = curn->child[0];
	}
	
	return -cur - 1;
}

extern cvar_t *hack_test;

// find the faces potentially visible from this camera
void DetermineVisibleFaces(camera_t *cam, int frustum_check)
{
	int i,j,k,camleaf,from,to;
	char viscluster[1<<13];
	
	camleaf = FindLeaf(cam->pos);
	from = leaf[camleaf].cluster;
	
	memset(viscluster, 0, sizeof(viscluster));
	memset(cam->visface, 0, sizeof(cam->visface));
	
	// determine which clusters are visible
	for (i=0; i<vis->n; i++) {
		if (from == -1 || cvar_int(vis_force_all)) {
			viscluster[i] = 1;
		} else {
			j = vis->vec[from*vis->size + (i >> 3)];
			if (j) viscluster[i] = 1;
		}
	}

	// determine which faces are visible
	for (i=0; i<num_leafs; i++) {
		to = leaf[i].cluster;
		if (to != -1 && !viscluster[to]) continue;

		// frustum check
		if (frustum_check && !BoxInFrustum(cam, leafbox[i])) continue;

		// mark the faces
		// FIXME: we currently frustum check each face; needed?
		for (j=0; j<leaf[i].numfaces; j++) {
			k = leafface[leaf[i].firstface+j];

			//if (frustum_check && BoxInFrustum(cam, face[k].bbox)) {
				cam->visface[k] = 1;
			//}
		}
	}

	// mark potentially visible models
	for (i=1; i<num_models; i++) {
		if (!model[i].numfaces) continue;
		to = (BoxInFrustum(cam, modelbox[i])) ? 1 : 0;
		for (j=model[i].firstface,k=0; k<model[i].numfaces; k++,j++)
			cam->visface[j] = to;
	}
}


/*****************************************************************************/
void CheckHeader(int p)
{
	int i;
	header = (header_t*)bspdata;
	if (p) printf("Header information:\n");
	if (p) printf("Magic: %c%c%c%c\n", header->magic[0],header->magic[1],
		      header->magic[2],header->magic[3]);
	
	if (header->magic[0] != 'I' || header->magic[1] != 'B' ||
	    header->magic[2] != 'S' || header->magic[3] != 'P') {
		error("CheckHeader: invalid header!\n");
	}

	if (p) printf("Version: 0x%x\n", header->version);
	if (header->version != 0x2e) error("CheckHeader: invalid header!\n");
	
	if (p) {
		for (i=0; i<17; i++) {
			printf("%2d: %d %d\n", i, header->lump[i].offset, 
			       header->lump[i].length);
		}
	}
	if (p) printf("\n");
}


/*****************************************************************************/
/*  Surfaces                                                                 */
/*****************************************************************************/
void InitSurfaces()
{
	int i;
	shader_t *sh;

	conprintf("num_surfs: %d", num_surfs);

	surf = (surf_t*)malloc(num_surfs * sizeof(surf_t));
	if (!surf) error ("InitSurfaces: insufficient memory for surf.\n");

	// setup each surface
	for (i=0; i<num_surfs; i++) {
		//printf("%3d: '%s': ", i, bspsurf[i].name);
		memcpy(&surf[i], &bspsurf[i], sizeof(bspsurf_t));
		surf[i].shader = NULL;
		
		// look for pre-defined shader
		sh = FindShader(surf[i].name);
		if (sh) {
			surf[i].flags |= sh->flags;
			if (sh->flags & SURF_NODRAW) {
				//printf("nodraw\n");
				continue;
			}

			LoadShader(sh, 0);
			
		} else {
			
			// no shader specified for this surface
			// create our own dummy shader
			sh = AllocShader();

			sprintf(sh->name, "dummy_%03d", i);
			sh->numlayers = 2;

			sh->layer[0].numtex = -1;
			sh->layer[0].blend_src = GL_ONE;
			sh->layer[0].blend_dst = GL_ZERO;

			sh->layer[1].numtex = 1;
			sh->layer[1].blend_src = GL_DST_COLOR;
			sh->layer[1].blend_dst = GL_ZERO;
			lowercase(surf[i].name);
			strcpy(sh->layer[1].texname[0], surf[i].name);

			LoadShader(sh, 0);
		}

		surf[i].shader = sh;
	}

	conprintf("num_textures: %d", num_textures);
}


void InitLightmaps()
{
	conprintf("num_lightmaps: %d", num_lightmaps);

	// setup each lightmap
	for (int i=0; i<num_lightmaps; i++) {
		char name[32];
		texture_t *tx;

		sprintf(name, "###lightmap_%03d", i);
		tx = TextureNew(name);

		lighttexture[i] = tx;
		tx->width = 128;
		tx->height = 128;
		tx->comp = 3;
		tx->data = (ubyte*)malloc(128*128*3);
		memcpy(tx->data, lightmapdata + i*(128*128*3), 128*128*3);

		// user specified filtering
		TextureSetup(tx, -1, -1, GL_CLAMP, GL_CLAMP);
	}

	// create a fullbright lightmap
	light_full = TextureNew("###lightmap_fullbright");
	light_full->width = 128;
	light_full->height = 128;
	light_full->comp = 3;
	light_full->data = (ubyte*) malloc (128*128*3);
	memset(light_full->data, 255, 128*128*3);
	TextureSetup(light_full, -1, -1, GL_CLAMP, GL_CLAMP);

	// create a halfbright lightmap, actually, more than halfbright
	light_half = TextureNew("###lightmap_halfbright");
	light_half->width = 128;
	light_half->height = 128;
	light_half->comp = 3;
	light_half->data = (ubyte*) malloc (128*128*3);
	memset(light_half->data, 192, 128*128*3);
	TextureSetup(light_full, -1, -1, GL_CLAMP, GL_CLAMP);
}


void FlipCoordsf(float *v)
{
	float t = v[1];
	v[1] = v[2];
	v[2] = -t;
}

void FlipCoordsi(int *v)
{
	int t = v[1];
	v[1] = v[2];
	v[2] = -t;
}

void Swapi(int *a, int *b)
{
	int t = *a;
	*a = *b;
	*b = t;
}

void Swapf(float *a, float *b)
{
	float t = *a;
	*a = *b;
	*b = t;
}

void ComputeBBox(int bbox[2][3], bbox_t out)
{
	int i,j,k,a=0;
	for (i=0; i<2; i++) for (j=0; j<2; j++) for (k=0; k<2; k++,a++) {
		out[a][0] = bbox[i][0];
		out[a][1] = bbox[j][1];
		out[a][2] = bbox[k][2];
	}
}

void ComputeBBoxf(float bbox[2][3], bbox_t out)
{
	int i,j,k,a=0;
	for (i=0; i<2; i++) for (j=0; j<2; j++) for (k=0; k<2; k++,a++) {
		out[a][0] = bbox[i][0];
		out[a][1] = bbox[j][1];
		out[a][2] = bbox[k][2];
	}
}

int BSP_Load(char *filename)
{
	int i;
	char *data;
	lump_t *lump;
	FILE *f;

	conprintf(" ");
	conprintf("================= BSP_Load =================");
	conprintf("Loading '%s'...", filename);
	conprintf(" ");
	
	f = fopen(filename, "rb");
	if (!f)	{
		conprintf("Couldn't open '%s' for reading.\n", filename);
		return 0;
	}

	fseek(f, 0L, SEEK_END);
	bsplen = ftell(f);
	fseek(f, 0L, SEEK_SET);
	
	bspdata = (void*) malloc (bsplen);
	if (!bspdata) {
		conprintf("BSP_Load: Insufficient memory for bspfile! (%d bytes)\n", bsplen);
		return 0;
	}

	if ((i = fread(bspdata, 1, bsplen, f)) != bsplen) {
		conprintf("BSP_Load: got only %d of %d bytes!\n", i, bsplen);
		return 0;
	}
	fclose(f);

	CheckHeader(0);
	data = (char*)bspdata;
	lump = header->lump;

	ent_length = lump[L_ENTITIES].length;
	entdata = (char*)(data + lump[L_ENTITIES].offset);
	//printf("%s\n", entdata);
	ParseEntities(entdata, ent_length);

	num_lightmaps = lump[L_LIGHTMAPS].length / sizeof(lightmap_t);
	lightmapdata = (ubyte*)(data + lump[L_LIGHTMAPS].offset);
	InitLightmaps();
	
	num_surfs = lump[L_TEXTURES].length / sizeof(bspsurf_t);
	bspsurf = (bspsurf_t*)((char *)bspdata + lump[L_TEXTURES].offset);
	InitSurfaces();

	num_planes = lump[L_PLANES].length / sizeof(plane_t);
	plane = (plane_t*)(data + lump[L_PLANES].offset);
	conprintf("num_planes: %d", num_planes);
	for (i=0; i<num_planes; i++) {
		FlipCoordsf(plane[i].n);
	}


	num_nodes = lump[L_NODES].length / sizeof(node_t);
	node = (node_t*)(data + lump[L_NODES].offset);
	nodebox = (bbox_t*) malloc (num_nodes * sizeof(bbox_t));
	conprintf("num_nodes: %d", num_nodes);
	for (i=0; i<num_nodes; i++) {
		//printf("node %d:\n", i);
		//printf("%5d %5d %5d\n", node[i].min[0],node[i].min[1],node[i].min[2]);
		//printf("%5d %5d %5d\n", node[i].max[0],node[i].max[1],node[i].max[2]);
		FlipCoordsi(node[i].bbox[0]);
		FlipCoordsi(node[i].bbox[1]);
		Swapi(&node[i].bbox[0][2], &node[i].bbox[1][2]);
		ComputeBBox(node[i].bbox, nodebox[i]);

		//printf("%5d %5d %5d\n", node[i].min[0],node[i].min[1],node[i].min[2]);
		//printf("%5d %5d %5d\n", node[i].max[0],node[i].max[1],node[i].max[2]);
		//printf("-------------------------------\n");
	}


	num_leafs = lump[L_LEAFS].length / sizeof(leaf_t);
	leaf = (leaf_t*)(data + lump[L_LEAFS].offset);
	leafbox = (bbox_t*) malloc (num_leafs * sizeof(bbox_t));
	conprintf("num_leafs: %d", num_leafs);
	for (i=0; i<num_leafs; i++) {
		FlipCoordsi(leaf[i].bbox[0]);
		FlipCoordsi(leaf[i].bbox[1]);
		Swapi(&node[i].bbox[0][2], &node[i].bbox[1][2]);
		ComputeBBox(leaf[i].bbox, leafbox[i]);
	}
	
	leafface = (int*)(data + lump[L_LEAFFACES].offset);

	num_models = lump[L_MODELS].length / sizeof(model_t);
	model = (model_t*)(data + lump[L_MODELS].offset);
	modelbox = (bbox_t*) malloc (num_models * sizeof(bbox_t));
	world = model;
	conprintf("num_models: %d", num_models);
	for (i=0; i<num_models; i++) {
		FlipCoordsf(model[i].bnds[0]);
		FlipCoordsf(model[i].bnds[1]);
		Swapf(&model[i].bnds[0][2], &model[i].bnds[1][2]);
		ComputeBBoxf(model[i].bnds, modelbox[i]);
	}

	num_brushes = lump[L_BRUSHES].length / sizeof(brush_t);
	brush = (brush_t*)(data + lump[L_BRUSHES].offset);
	conprintf("num_brushes: %d", num_brushes);

	num_brushsides = lump[L_BRUSHSIDES].length / sizeof(brushside_t);
	brushside = (brushside_t*)(data + lump[L_BRUSHSIDES].offset);
	conprintf("num_brushsides: %d", num_brushsides);

	num_vertices = lump[L_VERTEXES].length / sizeof(vertex_t);
	vertex = (vertex_t*)(data + lump[L_VERTEXES].offset);
	conprintf("num_vertices: %d", num_vertices);
	for (i=0; i<num_vertices; i++) {
		FlipCoordsf(vertex[i].p);
		FlipCoordsf(vertex[i].norm);
	}

	
	meshvert = (unsigned int*)(data + lump[L_MESHVERTS].offset);


	num_effects = lump[L_EFFECTS].length / sizeof(effect_t);
	effect = (effect_t*)(data + lump[L_EFFECTS].offset);
	conprintf("num_effects: %d", num_effects);

	//////////////////// FACES ////////////////////////////
	num_faces = lump[L_FACES].length / sizeof(bspface_t);
	bspface = (bspface_t*)(data + lump[L_FACES].offset);
	face = (face_t*)malloc(sizeof(face_t)*num_faces);
	conprintf("num_faces: %d", num_faces);
	for (i=0; i<num_faces; i++) {

		// switch to our coordinate system
		FlipCoordsf(bspface[i].norm);
		
		// copy data over to runtime version
		FaceSetup(&face[i], &bspface[i]);

		// use the fullbright lightmap if no lightmap for this face
		if (face[i].lightmapnum < 0 || (face[i].surface->flags & SURF_NOLIGHTMAP)) {
			face[i].lightmap = light_full;
			for (int j=0; j<face[i].numverts; j++) {
				vertex[face[i].firstvert+j].lc[0] = 0.0;
				vertex[face[i].firstvert+j].lc[1] = 0.0;
			}
		} else {
			face[i].lightmap = lighttexture[face[i].lightmapnum];
		}

	}

	////////////////////// VIS ///////////////////////////
	vis = (vis_t*)(data + lump[L_VISDATA].offset);
	vis->vec = (ubyte*)(data + lump[L_VISDATA].offset + 2*sizeof(int));


	//////////////////////////////////////////////////////
	// END OF BSP LOADING
	//////////////////////////////////////////////////////


	// Find all possible silhouette edges and verts
	DetermineSilhouette();
	conprintf("================= BSP_Load =================");


	// place camera in world
	// FIXME: move this out of here!
	if(FindASpawnPoint(player.pos, &player.yaw)) {
		FlipCoordsf(player.pos);
		player.pos[1] += 56.0;
	} else {
		player.yaw = 0.0f;
		player.pos[0] = 0;
		player.pos[1] = 0;
		player.pos[2] = 0;
	}

	bsp_loaded = 1;

	return 1;
}

void BSP_Free()
{
	if (!bsp_loaded) return;

	// free all data from bspfile
	free(bspdata);

	// free runtime bounding boxes
	free(nodebox);
	free(leafbox);
	free(modelbox);
	
	// free runtime faces
	for (int i = 0; i<num_faces; i++) {
		// FIXME: free faces!
		//FreeFace(&face[i]);
	}
	free(face);

	// free runtime surfaces
	for (int i = 0; i<num_surfs; i++) {
		if (!surf[i].shader) continue;
		if (!strncmp(surf[i].shader->name, "dummy", 5)) {
			free(surf[i].shader);
		}
	}
	free(surf);

	bsp_loaded = 0;
}
