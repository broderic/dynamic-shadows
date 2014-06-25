// external globals
extern float cam_pos[4];
extern float *cam_vpn;
extern float *cam_up;
extern float *cam_right;
extern float cam_yaw, cam_roll, cam_pitch;

extern cvar_t *draw_flat;
extern cvar_t *draw_leafs;
extern cvar_t *draw_lighting;
extern cvar_t *draw_texture;
extern cvar_t *draw_wireframe;
extern cvar_t *draw_shadow_mode;
extern cvar_t *draw_silhouette;

extern cvar_t *draw_polygons;
extern cvar_t *draw_meshes;
extern cvar_t *draw_patches;
extern cvar_t *draw_billboards;

extern cvar_t *draw_translucent;

extern cvar_t *draw_shadows;
extern cvar_t *draw_silhouette;
extern cvar_t *draw_shadow_volumes;
extern cvar_t *edge_cull;

extern cvar_t *hack_test;

extern int who;

extern double time_delta;


extern int shadowMapSize;

extern float  white[4];
extern float  black[4];
extern float    red[4];
extern float  green[4];
extern float   blue[4];
extern float yellow[4];
extern float   cyan[4];

// opengl extensions
extern int gl_ext_compiled_vertex_array;
extern int gl_ext_stencil_wrap;


void Shutdown();
void error(char *args, ...);

double GetTime();

//void DrawString(char *args, ...);

void DisableTextures();
void EnableTextures();
