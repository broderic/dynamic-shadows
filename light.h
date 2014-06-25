
typedef struct {
	camera_t cam;

	int on;
	int affects_scene;
	float radius;                      // all lights are sperical for now

	// spotlight stuff
	int spotlight;
	float spot_exponent; // GL_SPOT_EXPONENT
	float spot_angle;    // GL_SPOT_CUTOFF

	// color and other attributes
	vec4_t ambient_color;
	vec4_t diffuse_color;
	vec4_t specular_color;

	float constant_factor;
	float linear_factor;
	float quadratic_factor;

	// extent of the lighting
	// normal bbox when in shadow volume mode
	// frustum when in shadow mapping mode
	float extent;
	bbox_t bbox;

	int num_volume_caps;
	int num_volume_sides;

} light_t;

#define MAX_LIGHTS         128

extern int num_lights;
extern light_t light[MAX_LIGHTS];

extern int draw_num_volume_sides;
extern int draw_num_volume_caps;

void ComputeShadowVolumeMesh(light_t *lt, mesh_t *m);
void CalculateShadowVolume(light_t *lt);
void DrawShadowVolume_Visible(light_t *lt);
void DrawShadowVolume_Stencil();

void LightComputeBBox(light_t *lt);
void LightSetTextureMatrix(light_t *lt);
void LightSetClipPlanes(light_t *lt);
void LightSetAttributes(light_t *lt);

void UpdateLights();

void AddLight(float *pos, 
	      float fov, 
	      float *difuse, 
	      float yaw, float pitch, float roll,
	      float constant, float linear, float quadratic);
