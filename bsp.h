
extern int draw_num_triangles;
extern int draw_num_polygons;
extern int draw_num_meshes;
extern int draw_num_patches;
 
/*
0	Entities 	Game-related object descriptions.
1	Surfaces 	Surface descriptions.
2	Planes 	        Planes used by map geometry.
3	Nodes 	        BSP tree nodes.
4	Leafs 	        BSP tree leaves.
5	Leaffaces 	Lists of face indices, one list per leaf.
6	Leafbrushes 	Lists of brush indices, one list per leaf.
7	Models 	        Descriptions of rigid world geometry in map.
8	Brushes 	Convex polyhedra used to describe solid space.
9	Brushsides 	Brush surfaces.
10	Vertexes 	Vertices used to describe faces.
11	Meshverts 	Lists of offsets, one list per mesh.
12	Shaders 	List of shaders.
13	Faces           Surface geometry.
14	Lightmaps 	Packed lightmap data.
15	Lightvols 	Local illumination data.
16	Visdata 	Cluster-cluster visibility data.
*/


typedef enum {
	L_ENTITIES=0, L_TEXTURES, L_PLANES, L_NODES, L_LEAFS, L_LEAFFACES,
	L_LEAFBRUSHES, L_MODELS, L_BRUSHES, L_BRUSHSIDES, L_VERTEXES, 
	L_MESHVERTS, L_EFFECTS, L_FACES, L_LIGHTMAPS, L_LIGHTVOLS, L_VISDATA
} lumptype_t;


typedef struct {
	int offset;
	int length;
} lump_t;

typedef struct {
	char magic[4];
	int version;
	lump_t lump[17];
} header_t;

typedef struct {
	char name[64];   	    // Texture/shader name.
	int flags; 	            // Surface flags.
	int contents; 	            // Content flags.
} bspsurf_t;

typedef struct {
	vec3_t n;
	float d;
} plane_t;

typedef struct {
	int plane;                  // Space partitioning plane
	int child[2];               // Back and front child nodes
	int bbox[2][3];
} node_t;


typedef struct {
	int cluster;                // Visibility cluster number
	int area;                   // Volume of the leaf
	int bbox[2][3];
	int firstface, numfaces;    // Lookup for the face list (indexes are for faces)
	int firstbrush, numbrushes; // Lookup for the brush list (indexes are for brushes)
} leaf_t;


typedef struct {
	float bnds[2][3];           // Bounding box of model
	int firstface, numfaces;    // Lookup for the face list (indexes are for faces)
	int firstbrush, numbrushes; // Lookup for the brush list (indexes are for brushes)
} model_t;   

typedef struct {
	int firstside;              // index of first brushside
	int numsides;               // number of brushsides
	int texture;                // texture index (??)
} brush_t;

typedef struct {
	int plane;                  // plane this side resides on
	int texture;                // texture index (??)
} brushside_t;

typedef struct {
	vec3_t p;                   // World coordinates
	float tc[2];                // Texture coordinates
	float lc[2];                // Lightmap texture coordinates
	vec3_t norm;                // Normal vector (used for lighting ?)
	ubyte color[4];             // Colour used for vertex lighting
} vertex_t;


typedef struct {
	char name[64];              // The name of the shader file 
	int brushIndex;             // The brush index for this shader 
	int unknown;                // This is 99% of the time 5
} effect_t;


typedef struct {
	int surf;                   // Refers to a surface
	int effect;                 // effect (index into lump 12)
	int type;                   // Type of Face
	int firstvert, numverts;    // Reference to vertices
	int firstmesh, nummesh;     // Reference to meshverts
	int lightmap;               // Lightmap number
	int lm_offset[2];           // X,Y offset into lightmaps
	int lm_size[2];             // Size in lightmap
	vec3_t lm_orig;             // Face Origin
	float bbox[2][3];           // Bounding box of face
	vec3_t norm;                // Face normal
	int size[2];                // Patch dimensions
} bspface_t;


typedef ubyte lightmap_t[128][128][3]; // lightmap RGB data

// Visibility data
// y can see x iff bit (1 << y % 8) of vec[x*size + y/8] is set.
typedef struct {
	int n;  	            // Number of vectors.
	int size; 	            // Size of each vector, in bytes.
	ubyte *vec;                 // n*size bytes.
} vis_t;

typedef float bbox_t[8][3];

//////////////////////////////////////////////////////////
typedef struct {
	char name[64];   	    // Texture/shader name.
	int flags; 	            // Surface flags.
	int contents; 	            // Content flags.

	shader_t *shader;           // shader for this surface
} surf_t;

///////////////////////////////////////////

extern int bsp_loaded;

// stuff in the current bspfile
extern int num_surfs;
extern surf_t *surf;

extern int num_planes;
extern plane_t *plane;

extern int num_nodes;
extern node_t *node;
extern bbox_t *nodebox;

extern int num_leafs;
extern leaf_t *leaf;
extern bbox_t *leafbox;

extern int *leafface;

extern int num_models;
extern model_t *model, *world;
extern bbox_t *modelbox;

extern int num_brushes;
extern brush_t *brush;

extern int num_brushsides;
extern brushside_t *brushside;

extern int num_vertices;
extern vertex_t *vertex;

extern unsigned int *meshvert;

extern int num_effects;
extern effect_t *effect;

extern int num_faces;
extern bspface_t *bspface;

extern int num_lightmaps;
extern ubyte *lightmap;
extern lightmap_t fullbright;

extern vis_t *vis;

extern int ent_length;
extern char *entdata;

// textures and lightmap names
extern texture_t *light_full, *light_half;

float minf(float a, float b);
float maxf(float a, float b);

void InitBSP();
int BSP_Load(char *filename);
void BSP_Free();

void DetermineVisibleFaces(camera_t *cam, int frustum_check);

void CalculateShadowVolume(camera_t *);
void DrawShadowVolume_Visible();
void DrawShadowVolume_Stencil();
void DrawShadowVolume_Stencil_Front();
void DrawShadowVolume_Stencil_Back();

