#define FACE_POLYGON   1
#define FACE_PATCH     2
#define FACE_MESH      3
#define FACE_BILLBOARD 4

#define MAX_EDGES      64
#define MAX_EDGE_FACES 8


typedef struct {
	
	int numverts;               // number of vertices
	int numtris;                // number of triangles
	vec3_t *trinorm;            // normals for each triangle

	int numedges;               // number of edges
	int **edgevert;             // vertices on each edge
	int **edgeface;             // triangle on each side of edge

	vertex_t *orvert;           // original vertices
	vertex_t *vertex;           // point to list of vertices (possibly rotated/moved)
	unsigned int *meshvert;     // 3*numtris indexes; each 3-tuple is a triangle

	bbox_t origbbox;            // model space bounding box
	bbox_t bbox;                // translated bounding box
} mesh_t;


typedef struct {
	int level;                  // number of edges in tesselation
	vertex_t *controls[9];      // control points from bsp file
	mesh_t *mesh;               // the mesh created by tessellation
} bezier_t;

typedef struct {

	// Stuff in the bspfile
	int surfnum;                // Refers to a surface
	int effect;                 // effect (index into lump 12)
	int type;                   // Type of Face
	int firstvert, numverts;    // Reference to vertices
	int firstmesh, nummesh;     // Reference to meshverts
	int lightmapnum;            // Lightmap number
	int lm_offset[2];           // X,Y offset into lightmaps
	int lm_size[2];             // Size in lightmap
	vec3_t lm_orig;             // Face Origin
	float bnds[2][3];           // Bnds of face
	vec3_t norm;                // Face normal
	int size[2];                // Patch dimensions

	// Runtime stuff
	surf_t *surface;            // DEPRECATED: pointer to runtime surface
	texture_t *lightmap;        // pointer to lightmap texture

	bezier_t *bezier;           // bezier patches

	bbox_t bbox;                // 8pt bbox
	
	// polygon edges (for silhouettes)
	int numedges;
	int edge[MAX_EDGES][2];
	int nedgefaces[MAX_EDGES];
	int edgeface[MAX_EDGES][MAX_EDGE_FACES];

	mesh_t *mesh;
} face_t;


extern face_t *face;

mesh_t *MeshAllocate();

void DrawBBox(bbox_t bbox);
void TranslateBBox(bbox_t bbox, float *dir, bbox_t out);

void PrintFace(face_t *f);

void MeshComputeNormals(mesh_t *m, vec3_t);
void MeshComputeEdges(mesh_t *m);
void MeshComputeBBox(mesh_t *m);

void FaceBezierTessellate(face_t *f, int L);
void FaceComputeBoundingBox(face_t *f);
void FaceComputeEdges(face_t *f);

void FaceSetup(face_t *, bspface_t *);

void DetermineSilhouette();
void DrawSilhouette();

int FaceTranslucent(face_t *f);
