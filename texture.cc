#include "shad.h"

int num_textures = -1;
texture_t *texture_list;

char tex_min_str[32];
char tex_mag_str[32];

ccmd_t *tex_min;
ccmd_t *tex_mag;

ccmd_t *tex_list;
ccmd_t *tex_stats;

cvar_t *tex_anisotropic;

cvar_t *tex_gamma;

/*****************************************************************************/
/* Console commands                                                          */
/*****************************************************************************/
void ccmd_tex_stats(int argc, char **argv)
{
	TextureStats();
}

void ccmd_tex_list(int argc, char **argv)
{
	texture_t *tex = texture_list;
	for (int i=0; tex; i++, tex=tex->next) {
		conprintf("%03d: %s", i, tex->filename);
	}
}

void ccmd_tex_min(int argc, char **argv)
{
	switch(argc) {
	case 1: conprintf("tex_min = '%s'", tex_min_str); return;
	case 2: if (!strcmp(argv[1], "nearest") ||
		    !strcmp(argv[1], "linear") ||
		    !strcmp(argv[1], "bilinear") ||
		    !strcmp(argv[1], "trilinear"))
		{
			strcpy(tex_min_str, argv[1]);
			TexturesChangeFilters();
		}
       default: conprintf("usage: tex_min [mode]");
		conprintf("       where [mode] is {nearest,linear,bilinear,trilinear}");
	}	
}

void ccmd_tex_mag(int argc, char **argv)
{
	switch(argc) {
	case 1: conprintf("tex_mag = '%s'", tex_mag_str); return;
	case 2: if (!strcmp(argv[1], "nearest") ||
		    !strcmp(argv[1], "linear"))
		{
			strcpy(tex_mag_str, argv[1]);
			TexturesChangeFilters();
		}
       default: conprintf("usage: tex_mag [mode]");
		conprintf("       where [mode] is {nearest,linear}");
	}	
}


/*****************************************************************************/
/* Texture management                                                        */
/*****************************************************************************/
void TextureInit()
{
	// the first time we've been here
	if (num_textures == -1) {
		num_textures = 0;
		texture_list = NULL;

		strcpy(tex_min_str, "trilinear");
		strcpy(tex_mag_str, "linear");
		register_cvar(&tex_anisotropic, "tex_anisotropic", "0");

		register_cvar(&tex_gamma, "tex_gamma", "1.0");

		register_ccmd(&tex_min, "tex_min", ccmd_tex_min);
		register_ccmd(&tex_mag, "tex_mag", ccmd_tex_mag);
		register_ccmd(&tex_stats, "tex_stats", ccmd_tex_stats);
		register_ccmd(&tex_list, "tex_list", ccmd_tex_list);

		// Compute the gamma table
		ComputeGammaTable(cvar_double(tex_gamma));
		return;
	}

	// free all previous gl textures
	texture_t *prev, *tex, *next;
	for (prev = NULL, tex = texture_list; tex; tex = next) {
		next = tex->next;
		if (tex->flags & TEX_PERMANENT) {
			prev = tex;
			continue;
		}
		glDeleteTextures(1, &tex->name);
		free(tex->data);
		free(tex);
		num_textures--;
		
		if (prev) prev->next = next;
		else texture_list = next;
	}

	// Compute the gamma table (may have changed)
	ComputeGammaTable(cvar_double(tex_gamma));
}

texture_t *TextureNew(char *name)
{
	texture_t *t = (texture_t *) malloc (sizeof(texture_t));
	strcpy(t->filename, name);
	t->flags = 0;
	t->data = NULL;
	t->next = texture_list;
	texture_list = t;
	num_textures++;
	return t;
}

// takes a filled in texture_t structure and sets it up in opengl
void TextureSetup(texture_t *t, int min, int mag, int ws, int wt)
{
	// bail if we have already set this texture up
	if (t->flags & TEX_RESIDENT) return;

	// otherwise, mark that we have set it up
	t->flags |= TEX_RESIDENT;
	t->min = min;
	t->mag = mag;

	glGenTextures(1, &t->name);
	glBindTexture(GL_TEXTURE_2D, t->name);
	
	if (min == -1) { 	// use current user setting
		min = GL_NEAREST;
		if (!strcmp(tex_min_str, "linear")) min = GL_LINEAR;
		if (!strcmp(tex_min_str, "bilinear")) min = GL_LINEAR_MIPMAP_NEAREST;
		if (!strcmp(tex_min_str, "trilinear")) min = GL_LINEAR_MIPMAP_LINEAR;
	}

	if (mag == -1) {        // use current user setting
		mag = GL_NEAREST;
		if (!strcmp(tex_mag_str, "linear")) mag = GL_LINEAR;
	}
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ws);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wt);
	
	gluBuild2DMipmaps(GL_TEXTURE_2D, 
			  (t->comp==3) ? 3 : 4,
			  t->width, t->height,
			  (t->comp==3) ? GL_RGB : GL_RGBA, 
			  GL_UNSIGNED_BYTE, t->data);
}

// update filters on all textures to current settings
void TexturesChangeFilters()
{
	int min,mag;
	texture_t *tex;
	for (tex = texture_list; tex; tex = tex->next) {
		if (tex->min != -1 && tex->mag != -1) continue;

		min = tex->min; 
		mag = tex->mag;

		if (min == -1) { 	// use current user setting
			min = GL_NEAREST;
			if (!strcmp(tex_min_str, "linear")) min = GL_LINEAR;
			if (!strcmp(tex_min_str, "bilinear")) min = GL_LINEAR_MIPMAP_NEAREST;
			if (!strcmp(tex_min_str, "trilinear")) min = GL_LINEAR_MIPMAP_LINEAR;
		}
		if (mag == -1) {        // use current user setting
			mag = GL_NEAREST;
			if (!strcmp(tex_mag_str, "linear")) mag = GL_LINEAR;
		}
		
		glBindTexture(GL_TEXTURE_2D, tex->name);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
	}	
}

// displays the number of textures resident in all TUs
void TextureStats()
{
#if 0
	int i,j,k;
	GLuint name[MAX_TEXTURES];
	GLboolean res[MAX_TEXTURES], all[MAX_TEXTURES];
	memset(all, 0, sizeof(all));

	// collect all active textre names
	for (i=0; i<num_textures; i++) name[i] = texture[i].name;

	// which are resident in TU0?
	glActiveTextureARB(GL_TEXTURE0_ARB);
	if (glAreTexturesResident(num_textures, name, res) == GL_TRUE) {
		conprintf("TextureStats: all textures resident in TU0.");
		return;
	}
	for (i=0; i<num_textures; i++) all[i] |= res[i];
	for (i=j=0; i<num_textures; i++) if (res[i]) j++;

	// which are resident in TU1?
	glActiveTextureARB(GL_TEXTURE1_ARB);
	if (glAreTexturesResident(num_textures, name, res) == GL_TRUE) {
		conprintf("TextureStats: all textures resident in TU1.");
		return;
	}
	for (i=0; i<num_textures; i++) all[i] |= res[i];

	// display results
	for (i=k=0; i<num_textures; i++) if (res[i]) k++;
	conprintf("TextureStats: (%d,%d) of %d resident.", j, k, num_textures);

	// reset active unit
	glActiveTextureARB(GL_TEXTURE0_ARB);
#endif
}

// load a texture from a file
texture_t *LoadTexture(char *tname)
{
	FILE *f;
	texture_t *tx;
	ubyte *data;
	int w, h, c;
	char *off, filename[128];

	// if texture file has an extension, strip it off anyway.
	if ((off = strstr(tname, ".jpg"))) *off = 0;
	if ((off = strstr(tname, ".tga"))) *off = 0;

	// check if it has been loaded already
	for (tx = texture_list; tx; tx = tx->next) {
		if (!strcmp(tx->filename, tname)) break;
	}
	if (tx) return tx;

	// jpeg
	strcpy(filename, "baseq3/");
	strcat(filename, tname);
	strcat(filename, ".jpg");

	if ((f = fopen(filename, "rb"))) {
		fclose(f);
		data = loadJPEG(filename, &w, &h, &c);
		if (data) {
			tx = TextureNew(tname);
			tx->width = w;
			tx->height = h;
			tx->comp = c;
			tx->data = data;
		}
		return tx;
	}

	// targa
	strcpy(filename, "baseq3/");
	strcat(filename, tname);
	strcat(filename, ".tga");

	if ((f = fopen(filename, "rb"))) {
		fclose(f);
		data = loadTGA(filename, &w, &h, &c);
		if (data) {
			tx = TextureNew(tname);
			tx->width = w;
			tx->height = h;
			tx->comp = c;
			tx->data = data;
		}
		return tx;
	}
	return NULL;
}


// FIXME: do this better!
ubyte gamma_table[256];
void ComputeGammaTable(double p)
{
	int result;
	double power = 1/p,f;
	for (int i = 0; i<256; i++) {
		f = pow(i/255.0, power) * 255.0 + 0.5;
		result = (int)f;
		if (result < 0) result = 0;
		if (result > 255) result = 255;
		gamma_table[i] = (ubyte)result;
	}
}

// ComputeGammaTable must be called first!!
void GammaCorrect(int n, int comp, ubyte *data) 
{
	while(n--) {
		data[0] = gamma_table[(int)data[0]];
		data[1] = gamma_table[(int)data[1]];
		data[2] = gamma_table[(int)data[2]];
		data += comp;
	}
}


