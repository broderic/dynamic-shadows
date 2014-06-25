
// NOTE:  from the cake engine
// This holds the header information that is read in at the beginning of the file

typedef struct { 
	char	fileID[4];	//  file ID - Must be "IDP3"
	int	version;	//  file version - Must be 15
	char	strFile[68];	//  name of the file
	int	numFrames;	//  number of animation frames
	int	numTags;	//  tag count
	int	numMeshes;	//  number of sub-objects in the mesh
	int	numMaxSkins;	//  number of skins for the mesh
	int	frameStart;	//  offset into frames
	int	tagStart;	//  offset into the file for tags
	int	tagEnd;		//  end offset into the file for tags
	int	fileSize;	//  file size
} MD3HEADER;

// This structure is used to read in the mesh data for the .md3 models
typedef struct {
	char	meshID[4];	//  mesh ID (We don't care)
	char	strName[68];	//  mesh name (We do care)
	int	numMeshFrames;	//  mesh aniamtion frame count
	int	numSkins;	//  mesh skin count
	int     numVertices;	//  mesh vertex count
	int	numTriangles;	//  mesh face count
	int	triStart;	//  starting offset for the triangles
	int	headerSize;	//  header size for the mesh
	int     uvStart;	//  starting offset for the UV coordinates
	int	vertexStart;	//  starting offset for the vertex indices
	int	meshSize;	//  total mesh size
} MD3MESHINFO;

// frame info
typedef struct {
	float bnds[2][3];       // min/max bounds of frame
	vec3_t orig;            // local origin
	float radius;           // radius of bounding sphere
	char name[16];          // name of frame
} MD3FRAME;

// This is our tag structure for the .MD3 file format.  These are used link other
// models to and the rotate and transate the child models of that model.
typedef struct {
	char	name[64];	//  name of the tag (I.E. "tag_torso")
	vec3_t	pos;	        //  translation that should be performed
	float	rot[3][3];	//  3x3 rotation matrix for this frame
} MD3TAG;

//  bone information (useless as far as I can see...)
typedef struct
{
	float   bbox[2][3];     // This is the (x, y, z) mins and maxs for the bone
	float	position[3];	// This supposedly stores the bone position???
	float	scale;		// This stores the  scale of the bone
	char	creator[16];	// The modeler used to create the model (I.E. "3DS Max")
} MD3BONE;

typedef char MD3SKIN[68];	// shader name
typedef float MD3TEXCOORD[2];	// 2 coords
typedef int MD3FACE[3];		// 3 indices

typedef struct 
{
	signed short p[3];	// The vertex for this face
	unsigned char n[2];     // This stores some crazy normal value (not sure...)
} MD3VERTEX;

////////////////////////////////////////////////////////////////////

// my md3 stuff
typedef struct {

	int numparts;           // number of independent meshes

	int numframes;          // number of frames in mesh
	MD3FRAME *md3frame;     // frame information from md3 file

	mesh_t **frame;         // each frame of each part

	int numtags;
	MD3TAG *tag;            // the tags


} md3_t;

typedef struct {
	int first_frame;
	int num_frames;
	int looping_frames;
	int fps;
} anim_t;

// enemy structure
typedef struct {
	
	float yaw, pitch, roll;
	vec3_t pos;

	int state;
	anim_t *torso_anim;
	anim_t *legs_anim;
	double torso_start;
	double legs_start;
	int torso_frame;
	int legs_frame;

	texture_t *skin;
	md3_t *head, *upper, *lower;

} enemy_t;

extern enemy_t sarge;

void MD3Render(md3_t *md3, float *pos);

void UpdateEnemies();
void DrawEnemies();

void ComputeShadowVolumeEnemies(light_t *lt);

md3_t *LoadMD3(char *filename);
void InitSarge();


