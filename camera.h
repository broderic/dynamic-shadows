

typedef enum {RIGHT=0,LEFT,BOTTOM,TOP,BACK,FRONT} frustum_e;

typedef struct {
	float roll,pitch,yaw;             // position and angles
	float pos[4];

	float rt[4],up[4],vn[4];          // viewing vectors
	double frustum[6][4];             // frustum planes 
	double proj[16],modl[16];         // projection and modelview matrices

	float fov;                        // perspective info
	float nearp, farp;
	
	int viewwidth, viewheight;        // viewport information

	// visibility stuff
	int visface[1<<13];               // FIXME: Allocate this!

	// stats
	int drew_triangles;
	int drew_polygons;
	int drew_meshes;
	int drew_patches;

} camera_t;

extern camera_t player;

float deg2rad(float deg);

void ComputeTransform(camera_t *cam);
void CameraSetView(camera_t *cam);
void CameraUpdateView(camera_t *cam, int infinite);

int BoxInFrustum(camera_t *cam, float bbox[8][3]);
