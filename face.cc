#include "shad.h"

void DrawBBox(bbox_t bbox)
{
	glBegin(GL_LINES);
	glVertex3fv(bbox[0]);
	glVertex3fv(bbox[1]);

	glVertex3fv(bbox[0]);
	glVertex3fv(bbox[2]);

	glVertex3fv(bbox[2]);
	glVertex3fv(bbox[3]);

	glVertex3fv(bbox[3]);
	glVertex3fv(bbox[1]);

	glVertex3fv(bbox[4]);
	glVertex3fv(bbox[5]);

	glVertex3fv(bbox[4]);
	glVertex3fv(bbox[6]);

	glVertex3fv(bbox[6]);
	glVertex3fv(bbox[7]);

	glVertex3fv(bbox[7]);
	glVertex3fv(bbox[5]);

	glVertex3fv(bbox[0]);
	glVertex3fv(bbox[4]);

	glVertex3fv(bbox[1]);
	glVertex3fv(bbox[5]);

	glVertex3fv(bbox[2]);
	glVertex3fv(bbox[6]);

	glVertex3fv(bbox[3]);
	glVertex3fv(bbox[7]);
	glEnd();
}

// translate a bounding box
void TranslateBBox(bbox_t bbox, float *dir, bbox_t out)
{
	for (int i=0; i<8; i++) VectorAdd(bbox[i], dir, out[i]);
}

// converts a min/max pair into an 8 point bbox
void ComputeBBox(float bbox[2][3], bbox_t out)
{
	int i,j,k,a=0;
	for (i=0; i<2; i++) for (j=0; j<2; j++) for (k=0; k<2; k++,a++) {
		out[a][0] = bbox[i][0];
		out[a][1] = bbox[j][1];
		out[a][2] = bbox[k][2];
	}
}

mesh_t *MeshAllocate()
{
	mesh_t *ret = (mesh_t*) malloc (sizeof(mesh_t));
	if (!ret) error ("MeshAllocate: insufficient memory for mesh!\n");
	memset(ret, 0, sizeof(mesh_t));
	return ret;
}

// Compute the normals for each triangle if 'norm == NULL'
// Otherwise point to the address of norm
void MeshComputeNormals(mesh_t *m, vec3_t norm)
{
	m->trinorm = (vec3_t*) malloc (m->numtris * sizeof(vec3_t));
	if (!m->trinorm) error ("FaceSetupMesh: No memory for triangle norms!\n");

	if (norm != NULL) {
		for (int i=0; i<m->numtris; i++) VectorCopy3(norm, m->trinorm[i]);
		return;
	}

	for (int i=0; i<m->numtris; i++) {
		int off = i*3;
		vec3_t v1, v2;
		VectorSubtract(m->vertex[m->meshvert[off+1]].p,
			       m->vertex[m->meshvert[off+0]].p,
			       v1);
		VectorSubtract(m->vertex[m->meshvert[off+2]].p,
			       m->vertex[m->meshvert[off+0]].p,
			       v2);
		CrossProduct(v2, v1, m->trinorm[i]);
		VectorNormalize(m->trinorm[i]);
	}
}

void MeshComputeEdges(mesh_t *m)
{
	int a,b,c,i,j,k,nedges;
	static int edgevert[4096][2],edgeface[4096][2];

	nedges = 0;
	for (i=0; i<m->numtris; i++) {
		for (j=0; j<3; j++) {
			k = (j+1)%3;
			a = m->meshvert[i*3+j];
			b = m->meshvert[i*3+k];

			// look for edge in current list
			for (c=0; c<nedges; c++) {
				if (a == edgevert[c][1] && b == edgevert[c][0]) {
					if (edgeface[c][1] != -1) 
						conprintf("MeshComputeEdges: Edge on more than 2 faces!!!!!\n");
					edgeface[c][1] = i;
					break;
				}
			}

			// new edge
			if (c == nedges) {
				edgevert[nedges][0] = a;
				edgevert[nedges][1] = b;
				edgeface[nedges][0] = i;
				edgeface[nedges][1] = -1;
				nedges++;
			}
		}
	}

	// copy the edge info into the mesh
	m->numedges = nedges;
	m->edgevert = (int**) malloc (m->numedges * sizeof(int*));
	m->edgeface = (int**) malloc (m->numedges * sizeof(int));
	for (i=0; i<m->numedges; i++) {
		m->edgevert[i] = (int*) malloc (2*sizeof(int));
		m->edgeface[i] = (int*) malloc (2*sizeof(int));
		if (!m->edgevert[i] || !m->edgeface[i]) error("Allocating error!\n");
		m->edgevert[i][0] = edgevert[i][0];
		m->edgevert[i][1] = edgevert[i][1];
		m->edgeface[i][0] = edgeface[i][0];
		m->edgeface[i][1] = edgeface[i][1];
	}
}

void MeshComputeBBox(mesh_t *m)
{
	int i,j,k;
	float bnds[2][3];
	for (i=0; i<3; i++) {
		bnds[0][i] =  1e6;
		bnds[1][i] = -1e6;
	}
	for (i=0; i<m->numtris; i++) for (j=0; j<3; j++) for (k=0; k<3; k++) {
		bnds[0][k] = minf(bnds[0][k], m->vertex[m->meshvert[i*3+j]].p[k]);
		bnds[1][k] = maxf(bnds[1][k], m->vertex[m->meshvert[i*3+j]].p[k]);
	}
	ComputeBBox(bnds, m->bbox);
	ComputeBBox(bnds, m->origbbox);
}

void FaceSetupMesh(face_t *f)
{
	mesh_t *m = f->mesh = MeshAllocate();
	m->numtris = f->nummesh/3;
	m->numverts = f->numverts;
	m->meshvert = &meshvert[f->firstmesh];
	m->vertex = &vertex[f->firstvert];
}


void PrintFace(face_t *f)
{
	printf("--------------------\n");
	printf("numverts: %d\n", f->numverts);
	for (int i = 0; i<f->numverts; i++) {
		VectorPrint(vertex[f->firstvert+i].p);
	}
	printf("--------------------\n");
}
	

void BezierTessellate(bezier_t *bz, int L) 
{
	int i,q;
	int L1 = L + 1;              // # of verts in a row
	mesh_t *m = bz->mesh;

	bz->level = L;

	m->numtris = 2*L*L;
	m->numverts = L1*L1;
	m->vertex = (vertex_t*)malloc(L1*L1*sizeof(vertex_t));

	// Compute the vertices
	for (i = 0; i <= L; ++i) {
		double a = (double)i / L;
		double b = 1 - a;
		
		for (q=0; q<3; q++) {
			m->vertex[i].p[q] = 
				bz->controls[0]->p[q] * (b * b) + 
				bz->controls[3]->p[q] * (2 * b * a) +
				bz->controls[6]->p[q] * (a * a);
			m->vertex[i].norm[q] = 
				bz->controls[0]->norm[q] * (b * b) + 
				bz->controls[3]->norm[q] * (2 * b * a) +
				bz->controls[6]->norm[q] * (a * a);
			if (q == 3) break;
			m->vertex[i].tc[q] = 
				bz->controls[0]->tc[q] * (b * b) + 
				bz->controls[3]->tc[q] * (2 * b * a) +
				bz->controls[6]->tc[q] * (a * a);
			m->vertex[i].lc[q] = 
				bz->controls[0]->lc[q] * (b * b) + 
				bz->controls[3]->lc[q] * (2 * b * a) +
				bz->controls[6]->lc[q] * (a * a);
		}
	}
	
	for (i = 1; i <= L; ++i) {
		int j;
		double a = (double)i / L;
		double b = 1.0 - a;
		double temp[3][3], temptc[3][3], templc[3][3], tempnm[3][3];

		for (j = 0; j < 3; ++j) {
			int k = 3 * j;
			for (q=0; q<3; q++) {
				temp[j][q] =
					bz->controls[k + 0]->p[q] * (b * b) + 
					bz->controls[k + 1]->p[q] * (2 * b * a) +
					bz->controls[k + 2]->p[q] * (a * a);
				tempnm[j][q] = 
					bz->controls[k + 0]->norm[q] * (b * b) + 
					bz->controls[k + 1]->norm[q] * (2 * b * a) +
					bz->controls[k + 2]->norm[q] * (a * a);
				if (q == 3) break;
				temptc[j][q] = 
					bz->controls[k + 0]->tc[q] * (b * b) + 
					bz->controls[k + 1]->tc[q] * (2 * b * a) +
					bz->controls[k + 2]->tc[q] * (a * a);
				templc[j][q] = 
					bz->controls[k + 0]->lc[q] * (b * b) + 
					bz->controls[k + 1]->lc[q] * (2 * b * a) +
					bz->controls[k + 2]->lc[q] * (a * a);
			}
		}
		
		for(j = 0; j <= L; ++j) {
			double a = (double)j / L;
			double b = 1.0 - a;
			
			for (q=0; q<3; q++) {
				m->vertex[i * L1 + j].p[q] =
					temp[0][q] * (b * b) + 
					temp[1][q] * (2 * b * a) +
					temp[2][q] * (a * a);
				m->vertex[i* L1 + j].norm[q] = 
					tempnm[0][q] * (b * b) + 
					tempnm[1][q] * (2 * b * a) + 
					tempnm[2][q] * (a * a);
				if (q == 3) break;
				m->vertex[i* L1 + j].tc[q] = 
					temptc[0][q] * (b * b) + 
					temptc[1][q] * (2 * b * a) + 
					temptc[2][q] * (a * a);
				m->vertex[i* L1 + j].lc[q] = 
					templc[0][q] * (b * b) + 
					templc[1][q] * (2 * b * a) + 
					templc[2][q] * (a * a);
			}
		}
	}
	
	// compute the meshverts
	int x,y,n;
	m->meshvert = (unsigned int*) malloc(m->numtris*3*sizeof(unsigned int));
	for (y = n = 0; y < L; y++) {
		for (x=0; x < L; x++) {
			m->meshvert[n++] = (y+0)*L1 + x;
			m->meshvert[n++] = (y+0)*L1 + x + 1;
			m->meshvert[n++] = (y+1)*L1 + x;

			m->meshvert[n++] = (y+0)*L1 + x + 1;
			m->meshvert[n++] = (y+1)*L1 + x + 1;
			m->meshvert[n++] = (y+1)*L1 + x;
		}
	}
	MeshComputeNormals(m, NULL);
}

void FaceBezierTessellate(face_t *f, int lod) 
{
	int i,j,k,r,s;
	int ncx = f->size[0];
	int nx = (f->size[0]-1)/2;
	int ny = (f->size[1]-1)/2;
	bezier_t *bz;

	f->bezier = (bezier_t*)malloc(nx*ny*sizeof(bezier_t));
	if (!f->bezier) error("FaceBezierTesselate: insufficient memory for bezier patches.\n");

	for (i=0; i<ny; i++) for (j=0; j<nx; j++) {
		k = i*nx + j;
		bz = &f->bezier[k];

		for (s=0; s<3; s++) for (r=0; r<3; r++) {
			bz->controls[s*3+r] = &vertex[f->firstvert + (i*2+s)*ncx + j*2+r];
		}

		bz->mesh = MeshAllocate();
		BezierTessellate(bz, lod);
	}
}


// FIXME: bbox's in the bsp file are different... why?
void FaceComputeBoundingBox(face_t *f)
{
	int i,j;
	for (i=0; i<3; i++) {
		f->bnds[0][i] =  1e6;
		f->bnds[1][i] = -1e6;
	}

	switch(f->type) {
	case FACE_POLYGON:
	case FACE_MESH:
		for (i=0; i<f->numverts; i++) for (j=0; j<3; j++) {
			f->bnds[0][j] = minf(f->bnds[0][j], vertex[f->firstvert+i].p[j]);
			f->bnds[1][j] = maxf(f->bnds[1][j], vertex[f->firstvert+i].p[j]);
		}
		break;

	case FACE_PATCH:
		int npx = (f->size[0]-1)/2;
		int npy = (f->size[1]-1)/2;
		for (int a=0; a<npy; a++) for (int b=0; b<npx; b++) {
			bezier_t *bz = &f->bezier[a*npx + b];
			MeshComputeBBox(bz->mesh);
			for (j=0; j<3; j++) {
				f->bnds[0][j] = minf(f->bnds[0][j], bz->mesh->bbox[0][j]);
				f->bnds[1][j] = maxf(f->bnds[1][j], bz->mesh->bbox[7][j]);
			}
		}
		break;
	}
	
	ComputeBBox(f->bnds, f->bbox);
}


// Using the given triangulation of the face, calculate its edges.
// These will be used for silhouette determination
// NOTE: we cannot just use the vertices in the given order because this
// results in errors.  (not always a proper winding, I guess)
void FaceComputeEdges(face_t *f)
{
	int i,j,k,first,moff,a,b;
	int edge[MAX_EDGES][2],used[MAX_EDGES];

	if ((f->surface->flags & SURF_NODRAW) ||
	    (f->surface->flags & SURF_SKY)    ||
	    (f->surface->flags & SURF_SKIP)   ||
	    (f->surface->contents & CONTENTS_TRANSLUCENT)) 
        {
		// mark this as a non-shadow creator
		f->numedges = -1;
		return;
	}

	switch(f->type)	{
	case FACE_POLYGON:
		first = f->firstvert;
		memset(used, 0, sizeof(used));
		for (j=0; j<f->nummesh; j+=3) {
			moff = f->firstmesh+j;
			
			for (k=0; k<3; k++) {
				a = first+meshvert[moff + k];
				b = first+meshvert[moff + ((k+1)%3)];
				
				// has this edge been seen before?			
				for (i=0; i<MAX_EDGES; i++) { 
					if (!used[i]) continue;
					
					if ((edge[i][0] == a && edge[i][1] == b) ||
					    (edge[i][1] == a && edge[i][0] == b)) {
						used[i] = 0;
						break;
					}
				}
				
				// add edge if not seen
				if (i == MAX_EDGES) {
					for (i=0; i<MAX_EDGES; i++) if (!used[i]) break;
					if (i == MAX_EDGES) 
						error("FaceComputeEdges: too many edges!\n");
					edge[i][0] = a;
					edge[i][1] = b;
					used[i] = 1;
				}
			}
		}	

		// gather up the edges
		f->numedges = 0;
		for (i=0; i<MAX_EDGES; i++) {
			if (!used[i]) continue;
			f->edge[f->numedges][0] = edge[i][0];
			f->edge[f->numedges][1] = edge[i][1];
			f->numedges++;
		}
		
		memset(f->nedgefaces, 0, sizeof(f->nedgefaces));
		break;


	case FACE_MESH: 
		f->numedges = 0;
		MeshComputeEdges(f->mesh);
		break;

	case FACE_PATCH: 
		f->numedges = 0;
		int npx = (f->size[0]-1)/2;
		int npy = (f->size[1]-1)/2;
		for (int a=0; a<npy; a++) for (int b=0; b<npx; b++) {
			bezier_t *bz = &f->bezier[a*npx + b];
			MeshComputeEdges(bz->mesh);
		}
		break;
	}
}


////////////////////////////////////////////////////////////////////


int BoundsIntersect(float a[2][3], float b[2][3])
{
	// FIXME: do better?
	for (int i=0; i<3; i++) if (a[0][i] > b[1][i]+1e-4) return 0;
	for (int i=0; i<3; i++) if (b[0][i] > a[1][i]+1e-4) return 0;
	return 1;
}


/*****************************************************************************/
/*  Silhouette stuff                                                         */
/*****************************************************************************/

// NOTE: not used!
// Returns true if f1 is "inside" f2
int FaceInsideFace(face_t *f1, face_t *f2)
{
	float d = DotProduct(f1->norm, f2->norm);
	if (fabs(d) < 1-1e-3) return 0; 

	// make sure they are on the same plane 
	d = DotProduct(f2->norm, vertex[f2->firstvert].p);
	for (int i=0; i < f1->numverts; i++) {
		if (fabs(DotProduct(vertex[f1->firstvert+i].p, f2->norm) - d) > 1e-3)
			return 0;
	}

	// check if f1 is inside f2
	// do this by checking each vertex of f1 is inside the edgeplanes of f2
	for (int i=0; i < f2->numverts; i++) {
		int j = (i+1)%f2->numverts;
		vec3_t edgenormal, dir;
		float edist;

		VectorSubtract(vertex[f2->firstvert+j].p, vertex[f2->firstvert+i].p, dir);
		VectorNormalize(dir);
		CrossProduct(f2->norm, dir, edgenormal);
		edist = DotProduct(vertex[f2->firstvert+i].p, edgenormal);
		
		for (int k=0; k<f1->numverts; k++) {
			if (DotProduct(vertex[f1->firstvert+k].p, edgenormal) > edist-1e-3) 
				return 0;
		}
	}
	return 1;
}	

// Record edge connectivity between faces of world
void DetermineSilhouette()
{
	int a,b,c1,d1,c2,d2,i,j;
	face_t *f1, *f2;

	conprintf(" ");
	conprintf("DetermineSilhouette: calculating edges... ");
	for (i=0, f1=&face[0]; i<num_faces; i++, f1++) {
		if (f1->numedges == -1) continue;
		if (f1->type != FACE_POLYGON) continue;

		for (j=i+1, f2=&face[j]; j<num_faces; j++, f2++) {
			if (f2->numedges == -1) continue;
			if (f2->type != FACE_POLYGON) continue;

			if (!BoundsIntersect(f1->bnds, f2->bnds)) continue;

			for (a=0; a<f1->numedges; a++) {
				c1 = f1->edge[a][0];
				d1 = f1->edge[a][1];

				for (b=0; b<f2->numedges; b++) {
					c2 = f2->edge[b][0];
					d2 = f2->edge[b][1];
					if (c2 == -1) continue;

					if ((VectorEqual(vertex[c1].p, vertex[c2].p) &&
					     VectorEqual(vertex[d1].p, vertex[d2].p))    ||
					    (VectorEqual(vertex[c1].p, vertex[d2].p) && 
					     VectorEqual(vertex[d1].p, vertex[c2].p)))  {
				
						// they share an edge; add info to each
						if (f1->nedgefaces[a] >= MAX_EDGE_FACES) 
							error ("Too many faces on edge!\n");
						if (f2->nedgefaces[b] >= MAX_EDGE_FACES)
							error ("Too many faces on edge!\n");
						
						//printf("f1 %d, f2 %d\n", f1->nedgefaces[a],
						//       f2->nedgefaces[b]);

						f1->edgeface[a][f1->nedgefaces[a]] = j;
						f2->edgeface[b][f2->nedgefaces[b]] = i;
						f1->nedgefaces[a]++;
						f2->nedgefaces[b]++;
					} 
				}
			}
		}
	}
	conprintf("DetermineSilhouette: ...complete.");	
}


void FaceSetup(face_t *f, bspface_t *b)
{
	// copy data from bsp file
	memcpy(f, b, sizeof(bspface_t));
	f->surface = &surf[f->surfnum];
	f->bezier = NULL;
	f->mesh = NULL;

	// setup meshes and tesslate patches into meshes
	switch(f->type) {
	case FACE_POLYGON:
		FaceSetupMesh(f);
		MeshComputeNormals(f->mesh, f->norm);
		break;
	case FACE_MESH:
		FaceSetupMesh(f);
		MeshComputeNormals(f->mesh, NULL);
		break;
	case FACE_PATCH:
		FaceBezierTessellate(f, 7);
	}
	
	FaceComputeBoundingBox(f);
	FaceComputeEdges(f);
}


////////////////////////////////////////////

int FaceTranslucent(face_t *f)
{
	if (f->surface->flags & CONTENTS_TRANSLUCENT) return 1;
	return ShaderTranslucent(f->surface->shader);
}
