
extern cvar_t *vis_force_all;

void SetTextureModes();
void SetTextureModes_Lighting();

void SetBlend(int, int);
void SetAlphaFunc(int );

void DrawFace(face_t *f, camera_t *);

void DrawWorld(camera_t *);

void PreFrame();
void PostFrame();

void InitRender();
