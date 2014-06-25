
// Surface flags (values from the cake engine)
#define	SURF_NODAMAGE	     0x00001    /** never give falling damage */
#define	SURF_SLICK           0x00002    /** effects game physics */
#define	SURF_SKY	     0x00004    /** lighting from environment map */
#define	SURF_LADDER	     0x00008
#define	SURF_NOIMPACT	     0x00010    /** don't make missile explosions */
#define	SURF_NOMARKS	     0x00020    /** don't leave missile marks */
#define	SURF_FLESH	     0x00040    /** make flesh sounds and effects */
#define	SURF_NODRAW	     0x00080    /** don't generate a drawsurface at all */
#define	SURF_HINT	     0x00100    /** make a primary bsp splitter */
#define	SURF_SKIP	     0x00200    /** completely ignore, allowing non-closed brushes */
#define	SURF_NOLIGHTMAP	     0x00400    /** surface doesn't need a lightmap */
#define	SURF_POINTLIGHT	     0x00800    /** generate lighting info at vertexes */
#define	SURF_METALSTEPS	     0x01000    /** clanking footsteps */
#define	SURF_NOSTEPS	     0x02000    /** no footstep sounds */
#define	SURF_NONSOLID	     0x04000    /** don't collide against curves with this set */
#define	SURF_LIGHTFILTER     0x08000    /** act as a light filter during q3map -light */
#define	SURF_ALPHASHADOW     0x10000    /** do per-pixel light shadow casting in q3map */
#define	SURF_NODLIGHT	     0x20000    /** don't light even if solid (solid lava, skies) */
#define SURF_DUST	     0x40000    /** leave a dust trail when walking on this surface */

#define	CONTENTS_TELEPORTER	0x40000
#define	CONTENTS_JUMPPAD	0x80000
#define CONTENTS_CLUSTERPORTAL	0x100000
#define CONTENTS_DONOTENTER	0x200000
#define CONTENTS_BOTCLIP	0x400000
#define CONTENTS_MOVER		0x800000
#define	CONTENTS_ORIGIN		0x1000000	/** removed before bsping an entity */
#define	CONTENTS_BODY		0x2000000	/** should never be on a brush, only in game */
#define	CONTENTS_CORPSE		0x4000000
#define	CONTENTS_DETAIL		0x8000000	/** brushes not used for the bsp */
#define	CONTENTS_STRUCTURAL	0x10000000	/** brushes used for the bsp */
#define	CONTENTS_TRANSLUCENT	0x20000000	/** don't consume surface fragments inside */
#define	CONTENTS_TRIGGER	0x40000000
#define	CONTENTS_NODROP		0x80000000	/** don't leave bodies or items (death fog,lava)*/


#define SHADER_MAX_LAYERS   8
#define SHADER_MAX_TCMOD    4

#define TCGEN_NONE          0
#define TCGEN_BASE          1
#define TCGEN_LIGHTMAP      2
#define TCGEN_ENVIRONMENT   3

#define ALPHA_FUNC_NONE     0
#define ALPHA_FUNC_GT0      1
#define ALPHA_FUNC_LT128    2
#define ALPHA_FUNC_GE128    3

#define RGBGEN_IDENTITY     0
#define RGBGEN_IDENTITY_LIGHTING 1
#define RGBGEN_WAVE         2
#define RGBGEN_ENTITY       3
#define RGBGEN_ONE_MINUS_ENTITY 4
#define RGBGEN_VERTEX       5
#define RGBGEN_ONE_MINUS_VERTEX 6
#define RGBGEN_LIGHTING_DIFFUSE 7

#define WAVE_SINE           0
#define WAVE_SQUARE         1
#define WAVE_TRIANGLE       2
#define WAVE_SAWTOOTH       3
#define WAVE_INV_SAWTOOTH   4
#define WAVE_NOISE          5

/////////////////////////////
#define TCMOD_ROTATE        0
#define TCMOD_SCALE         1
#define TCMOD_SCROLL        2
#define TCMOD_STRETCH       3
#define TCMOD_TRANSFORM     4   
#define TCMOD_TURB          5

#define DEFORM_WAVE         0
#define DEFORM_NORMAL       1
#define DEFORM_BULGE        2
#define DEFORM_MOVE         3
#define DEFORM_AUTOSPRITE   4
#define DEFORM_AUTOSPRITE2  5
#define DEFORM_PROJECTION   6
#define DEFORM_TEXT0        7
#define DEFORM_TEXT1        8

typedef struct mod_s {
	int type;
	float arg[10];
	struct mod_s *next;
} mod_t;

typedef struct {

	int numtex;                    // (-1 means use face's lightmap)
	float fps;                     // number of frames per second
	texture_t *texture[8];         // the textures 
	char texname[8][64];           // the names of each texture

	int blend_src, blend_dst;      // blending function

	int rgbgen;                    // color mode 
	float rgbarg[6];

	int tcgen;                     // how are the texture coordinates generated?

	int depth_func;                // depth function
	int depth_mask;                // force depth writing on translucent surfaces
	int alpha_func;                // what alpha test to use 

	mod_t *tcmod;                  // tcmod operations 
} layer_t;

// FIXME: actually implement a real shader!!
typedef struct {

	char name[64];
	int flags;
	
	int cull;

	mod_t *vertmod;                // vertex deformations

	int numlayers;
	layer_t layer[SHADER_MAX_LAYERS];
} shader_t;


void lowercase(char *str);

shader_t *FindShader(char *);
void ListShaders();
void InitShaders();

shader_t *AllocShader();

void LoadShader(shader_t *sh, int);
void ShaderSetTextures(shader_t *);

int ShaderTranslucent(shader_t *sh);
