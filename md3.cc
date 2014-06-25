#include "shad.h"

int md3len;
void *md3data;

MD3HEADER *md3header;

// we only load the 'sarge' model
texture_t *sarge_skin;
int skin_width, skin_height, skin_comp;
unsigned char *skin_data;
md3_t *sarge_head, *sarge_upper, *sarge_lower;

enemy_t sarge;

// animations for sarge
// from its animation.cfg file
anim_t TORSO_GESTURE = {90,      40,      0,       18};
anim_t TORSO_ATTACK  = {130,     6,       0,       15};
anim_t TORSO_ATTACK2 = {136,     6,       0,       15};
anim_t TORSO_DROP    = {142,     5,       0,       20};
anim_t TORSO_RAISE   = {147,     4,       0,       20};
anim_t TORSO_STAND   = {151,     1,       0,       15};
anim_t TORSO_STAND2  = {152,     1,       0,       15};

anim_t LEGS_WALKCR = {153-63,      8,      8,      20};
anim_t LEGS_WALK   = {161-63,     12,     12,      20};
anim_t LEGS_RUN    = {173-63,     11,     11,      21};
//184     10      10      20              // LEGS_BACK
//194     10      10      15              // LEGS_SWIM
//204     10      0       18              // LEGS_JUMP
//214     6       0       20              // LEGS_LAND
//220     8       0       15              // LEGS_JUMPB
//228     1       0       15              // LEGS_LANDB
anim_t LEGS_IDLE   = {229-63,     10,     10,      15};
anim_t LEGS_IDLECR = {239-63,      8,      8,      15};
anim_t LEGS_TURN   = {247-63,      7,      7,      15};


// use mat to rotate in; storing it in out
// mat is 3x3 matrix in row major order
void RotateVertex(float mat[3][3], float *in, float *out)
{
	out[0] = mat[0][0]*in[0] + mat[0][1]*in[1] + mat[0][2]*in[2];
	out[1] = mat[1][0]*in[0] + mat[1][1]*in[1] + mat[1][2]*in[2];
	out[2] = mat[2][0]*in[0] + mat[2][1]*in[1] + mat[2][2]*in[2];
}

void MeshIdentity(mesh_t *m)
{
	for (int i=0; i<m->numverts; i++) {
		VectorCopy(m->orvert[i].p, m->vertex[i].p, 3);
	}
	memcpy(m->bbox, m->origbbox, sizeof(bbox_t));
}


// Rotates vertex[i] into vertex[i]
void RotateMesh(mesh_t *m, float mat[3][3])
{
	float tmp[3];
	for (int i=0; i<m->numverts; i++) {
		RotateVertex(mat, m->vertex[i].p, tmp);
		VectorCopy(tmp, m->vertex[i].p, 3);
	}
}

// Translates vertex[i] by off into vertex[i]
void TranslateMesh(mesh_t *m, float *off)
{
	for (int i=0; i<m->numverts; i++) {
		VectorAdd(m->vertex[i].p, off, m->vertex[i].p);
	}
	TranslateBBox(m->bbox, off, m->bbox);

}

void SwizzleMesh(mesh_t *m)
{
	float temp;
	for (int i=0; i<m->numverts; i++) {
		temp = m->vertex[i].p[1];
		m->vertex[i].p[1] = m->vertex[i].p[2];
		m->vertex[i].p[2] = -temp;

		temp = m->orvert[i].p[1];
		m->orvert[i].p[1] = m->orvert[i].p[2];
		m->orvert[i].p[2] = -temp;
	}
}

// Draw a mesh
void MD3RenderFrame(mesh_t *m)
{
	const int stride = sizeof(vertex_t);

	glColor3f(0,0,0);

	glVertexPointer(3, GL_FLOAT, stride, m->vertex[0].p);
	glNormalPointer(GL_FLOAT, stride, m->vertex[0].norm);
	
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glTexCoordPointer(2, GL_FLOAT, stride, m->vertex[0].lc);
	
	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glTexCoordPointer(2, GL_FLOAT, stride, m->vertex[0].tc);
	
	glDrawElements(GL_TRIANGLES, m->numtris*3,
		       GL_UNSIGNED_INT, m->meshvert);
}


void UpdateEnemy(enemy_t *en)
{
	vec3_t off, off2;
	static vec3_t head_off = {0, 17, 0};
	static vec3_t upper_off = {0, -3, 0};
	int num_elapsed;
	int lcur_frame, ucur_frame;


	// update lower body
	num_elapsed = (int)((GetTime() - en->legs_start) * en->legs_anim->fps);
	lcur_frame = (num_elapsed % en->legs_anim->num_frames) + en->legs_anim->first_frame;
	en->legs_frame = lcur_frame;

	MeshIdentity(&en->lower->frame[0][lcur_frame]);
	TranslateMesh(&en->lower->frame[0][lcur_frame], en->pos);

	// update upper body
	num_elapsed = (int)((GetTime() - en->torso_start) * en->torso_anim->fps);
	ucur_frame = (num_elapsed % en->torso_anim->num_frames) + en->torso_anim->first_frame;
	en->torso_frame = ucur_frame;

	MeshIdentity(&en->upper->frame[1][ucur_frame]);
	TranslateMesh(&en->upper->frame[1][ucur_frame], en->pos);

	off2[0] = 0;
	off2[1] = en->lower->frame[0][lcur_frame].bbox[7][1] - 
		  en->upper->frame[1][ucur_frame].bbox[0][1];
	off2[2] = 0;
	VectorAdd(upper_off, off2, off2);
	TranslateMesh(&en->upper->frame[1][ucur_frame], off2);


	// update head position
	MeshIdentity(&en->head->frame[1][0]);
	VectorAdd(head_off, en->pos, off);
	VectorAdd(off, off2, off);
	TranslateMesh(&en->head->frame[1][0], off);
}


void UpdateEnemies()
{
	UpdateEnemy(&sarge);
}


// draw the sarge model at position pos
void DrawEnemy(enemy_t *en)
{
	// setup textures
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_2D, light_full->name);
	
	// texture 
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glBindTexture(GL_TEXTURE_2D, en->skin->name);

	// draw the head, body, and legs
	MD3RenderFrame( &en->head->frame[1][0]);
	MD3RenderFrame(&en->upper->frame[1][en->torso_frame]);
	MD3RenderFrame(&en->lower->frame[0][en->legs_frame]);

	//glColor3f(1, 1, 0);
	//DrawBBox(en->upper->frame[1][en->torso_frame].bbox);
	//DrawBBox(en->lower->frame[0][en->legs_frame].bbox);
}


void DrawEnemies()
{
	DrawEnemy(&sarge);
}


void ComputeShadowVolumeEnemies(light_t *lt)
{
	ComputeShadowVolumeMesh(lt, &sarge.head->frame[1][0]);
	ComputeShadowVolumeMesh(lt, &sarge.upper->frame[1][sarge.torso_frame]);
	ComputeShadowVolumeMesh(lt, &sarge.lower->frame[0][sarge.legs_frame]);
}

// load model from file
//#define DEBUG_MD3
md3_t *LoadMD3(char *filename)
{
	int i,j;
	FILE *f;
	md3_t *model;
	char *filepos;

	conprintf("Loading '%s'...", filename);

	// load the file into memory
	f = fopen(filename, "rb");
	if (!f)	error("LoadMD3: Couldn't open '%s' for reading.\n", filename);

	fseek(f, 0L, SEEK_END);
	md3len = ftell(f);
	fseek(f, 0L, SEEK_SET);
	
	md3data = (void*) malloc (md3len);
	if (!md3data) error("LoadMD3: Insufficient memory for md3file! (%d bytes)\n", md3len);
	if ((i = fread(md3data, 1, md3len, f)) != md3len) 
		error("LoadMD3: got only %d of %d bytes!\n", i, md3len);
	fclose(f);
	
	// dump the header info
	md3header = (MD3HEADER*)md3data;

#ifdef DEBUG_MD3
	conprintf("MD3 content (file %s)", filename);
	conprintf("  fileID: %s", md3header->fileID);
	conprintf("  version: %d", md3header->version);
	conprintf("  strFile: %s", md3header->strFile);
	conprintf("  numFrames: %d", md3header->numFrames);
	conprintf("  numMeshes: %d", md3header->numMeshes);
	conprintf("  numTags: %d", md3header->numTags);
	conprintf("  numMaxSkins: %d", md3header->numMaxSkins);
	conprintf("  frameStart: %d", md3header->frameStart);
	conprintf("  tagStart: %d", md3header->tagStart);
	conprintf("  tagEnd: %d", md3header->tagEnd);
	conprintf("  fileSize: %d", md3header->fileSize);

	conprintf(" ");
	conprintf("TAG info:");
#endif

	MD3TAG *tag = (MD3TAG*)((char*)md3data + md3header->tagStart);

#ifdef DEBUG_MD3
	for (i = 0; i<md3header->numTags; i++) {
		if (i) conprintf(" ");
		conprintf("  strName: %s", tag[i].name);
		conprintf("  pos: (%.2f, %.2f, %.2f)", tag[i].pos[0],tag[i].pos[1],tag[i].pos[2]);
		conprintf("  rot:  %.2f  %.2f  %.2f", tag[i].rot[0][0],
			  tag[i].rot[0][1],tag[i].rot[0][2]);
		conprintf("        %.2f  %.2f  %.2f", tag[i].rot[1][0],
			  tag[i].rot[1][1],tag[i].rot[1][2]);
		conprintf("        %.2f  %.2f  %.2f", tag[i].rot[2][0],
			  tag[i].rot[2][1],tag[i].rot[2][2]);
	}
#endif

	// allocate the model
	model = (md3_t*) malloc (sizeof(md3_t));
	if (!model) error("LoadMD3: insufficient memory for md3!\n");
	model->numparts = md3header->numMeshes;
	model->numframes = md3header->numFrames;
	model->frame = (mesh_t**) malloc (model->numframes*sizeof(mesh_t*));

	// copy the tags over
	model->numtags = md3header->numTags;
	model->tag = (MD3TAG*) malloc (model->numtags * sizeof(MD3TAG));
	memcpy(model->tag, tag, model->numtags * sizeof(MD3TAG));

	// copy the frames over
	MD3FRAME *frame = (MD3FRAME*)((char*)md3data + md3header->frameStart);
	model->md3frame = (MD3FRAME*) malloc (model->numframes*sizeof(MD3FRAME));
	memcpy(model->md3frame, frame, model->numframes*sizeof(MD3FRAME));

	// read in the info
	filepos = (char*)((char*)md3data + md3header->tagEnd);
	for (int m=0; m < md3header->numMeshes; m++) {
		
		// allocate all the frames for this part of the model
		model->frame[m] = (mesh_t*) malloc (model->numframes * sizeof(mesh_t));
		memset(model->frame[m], 0, model->numframes * sizeof(mesh_t));

		// read and dump the header info
		MD3MESHINFO *meshinfo = (MD3MESHINFO*) (filepos);
#ifdef DEBUG_MD3
		conprintf("");
		conprintf("  Reading mesh %d:", m);
		conprintf("    strName: %s", meshinfo->strName);
		conprintf("    numMeshFrames: %d", meshinfo->numMeshFrames);
		conprintf("    numSkins: %d", meshinfo->numSkins);
		conprintf("    numVertices: %d", meshinfo->numVertices);
		conprintf("    numTriangles: %d", meshinfo->numTriangles);
		conprintf("    triStart: %d", meshinfo->triStart);
		conprintf("    headerSize: %d", meshinfo->headerSize);
		conprintf("    uvStart: %d", meshinfo->uvStart);
		conprintf("    vertexStart: %d", meshinfo->vertexStart);
		conprintf("    meshSize: %d", meshinfo->meshSize);
#endif
		MD3SKIN *skin;
		MD3TEXCOORD *texcoord;
		MD3FACE *triangle;
		MD3VERTEX *vertex;

		skin = (MD3SKIN*)(filepos + meshinfo->headerSize);
		triangle = (MD3FACE*)(filepos + meshinfo->triStart);
		texcoord = (MD3TEXCOORD*)(filepos + meshinfo->uvStart);
		vertex = (MD3VERTEX*)(filepos + meshinfo->vertexStart);
		
		for (i=0; i<meshinfo->numMeshFrames; i++) {
			mesh_t *mesh = &model->frame[m][i];

			// allocate vertex and triangle info for mesh
			mesh->numverts = meshinfo->numVertices;
			mesh->numtris = meshinfo->numTriangles;
			mesh->meshvert = (unsigned int*) malloc(3 * mesh->numtris * sizeof(int));
			if (!mesh->meshvert) 
				error("LoadMD3: not enough memory for meshverts! (%d,%d)\n", m, i);

			mesh->vertex = (vertex_t*)malloc(mesh->numverts*sizeof(vertex_t));
			mesh->orvert = (vertex_t*)malloc(mesh->numverts*sizeof(vertex_t));
			if (!mesh->vertex || !mesh->orvert) 
				error("LoadMD3: not enough memory for vertices (%d,%d)\n", m, i);

			// convert each vertex 
			int nf = meshinfo->numVertices;
			for (j=0; j<mesh->numverts; j++) {
				float sin_a, sin_b, cos_a, cos_b;

				// position
				mesh->vertex[j].p[0] = vertex[i*nf+j].p[0]/64.0f;
				mesh->vertex[j].p[1] = vertex[i*nf+j].p[1]/64.0f;
				mesh->vertex[j].p[2] = vertex[i*nf+j].p[2]/64.0f;
				
				// normal (encoded with spherical coords)
				sin_a = (float)vertex[i*nf+j].n[0]*M_PI/128.0f;
				cos_a = cos(sin_a);
				sin_a = sin(sin_a);

				sin_b = (float)vertex[i*nf+j].n[1]*M_PI/128.0f;
				cos_b = cos(sin_b);
				sin_b = sin(sin_b);

				mesh->vertex[j].norm[0] = cos_b*sin_a;  //  x
				mesh->vertex[j].norm[1] = sin_b*sin_a;  //  y
				mesh->vertex[j].norm[2] = cos_a;        //  z

				// lightmap coords (always 0 -- always use fullbright)
				mesh->vertex[j].lc[0] = 0;
				mesh->vertex[j].lc[1] = 0;
				
				// texture coords
				mesh->vertex[j].tc[0] = texcoord[j][0];
				mesh->vertex[j].tc[1] = texcoord[j][1];			
			}
			
			// copy these vertices into the original vertex list
			memcpy(mesh->orvert, mesh->vertex, mesh->numverts*sizeof(vertex_t));

			// convert each triangle
			for (j=0; j<mesh->numtris; j++) {
				mesh->meshvert[j*3 + 0] = triangle[j][0];
				mesh->meshvert[j*3 + 1] = triangle[j][1];
				mesh->meshvert[j*3 + 2] = triangle[j][2];
			}

			// fill in rest of structure
			SwizzleMesh(mesh);
			MeshComputeBBox(mesh);
			MeshComputeNormals(mesh, NULL);
			MeshComputeEdges(mesh);
		}
		filepos += meshinfo->meshSize;
	}

	free(md3data);
	return model;
}


void LoadSarge()
{
	sarge_head  = LoadMD3("baseq3/models/players/sarge/head.md3");
	sarge_upper = LoadMD3("baseq3/models/players/sarge/upper.md3");
	sarge_lower = LoadMD3("baseq3/models/players/sarge/lower.md3");

	sarge_skin = TextureNew("models/players/sarge/blue.tga");
	char skin_file[] = "baseq3/models/players/sarge/blue.tga";
	if ((sarge_skin->data = loadTGA(skin_file, &sarge_skin->width, 
					&sarge_skin->height, &sarge_skin->comp))) {
		printf("SKIN: %d x %d x %d TGA.\n", sarge_skin->width, 
		       sarge_skin->height, sarge_skin->comp);
	} else {
		error("Couldn't open skin file: '%s'\n", skin_file);
	}

	sarge_skin->flags |= TEX_PERMANENT;
	TextureSetup(sarge_skin, -1, -1, GL_CLAMP, GL_CLAMP);
}

void InitSarge()
{
	LoadSarge();
	
	sarge.yaw = 0;
	sarge.pitch = 0;
	sarge.roll = 0;

	sarge.pos[0] = -350;
	sarge.pos[1] = 22;
	sarge.pos[2] = 130;

	sarge.state = 0;

	sarge.torso_anim = &TORSO_STAND;
	sarge.legs_anim = &LEGS_IDLE;
	sarge.torso_start = GetTime();
	sarge.legs_start = GetTime();

	sarge.skin = sarge_skin;
	sarge.head = sarge_head;
	sarge.upper = sarge_upper;
	sarge.lower = sarge_lower;
}
