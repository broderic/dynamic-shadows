#define TEX_RESIDENT       1
#define TEX_PERMANENT      2

typedef struct texture_s {
	int flags;                  

	GLuint name;                // gl name/identifier
	int min,mag;                // texmaping filters

	int width,height,comp;      // info
	ubyte *data;                // texture data

	char filename[64];          // file it was loaded from
	struct texture_s *next;
} texture_t;


extern int num_textures;

void TextureInit();
texture_t *TextureNew(char *);
void TextureSetup(texture_t *t, int min, int mag, int ws, int wt);
void TextureStats();
void TexturesChangeFilters();
texture_t *LoadTexture(char *tname);


void ComputeGammaTable(double p);
void GammaCorrect(int, int, ubyte *);
