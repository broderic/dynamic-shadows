#include <ctype.h>
#include <string.h>

#ifdef CYGWIN
#include "/usr/include/FL/filename.H"	/* scandir () */
// #include <sys/cygwin.h> /* cygwin_internal () */
#else 
#include <dirent.h>
#endif

#include "shad.h"

#define MAX_SHADERS       1<<16
int num_shaders;
shader_t *shader[MAX_SHADERS];

// FIXME: not using this currently
void lowercase(char *str)
{
	return;
	while (*str) {
		*str = tolower(*str);
		str++;
	}
}

#ifdef CYGWIN
int __fuck_cygwin_strcasecmp(char *s1, char *s2)
{
	int i,s1i,s2i;
	for (i=0; s1[i] && s2[i]; i++) {
		s1i = tolower(s1[i]);
		s2i = tolower(s2[i]);
		if (s1i < s2i) return -1;
		if (s1i > s2i) return 1;
	}
	if (s2[i]) return -1;
	if (s1[i]) return 1;
	return 0;
}
#define strcasecmp __fuck_cygwin_strcasecmp
#endif


shader_t *FindShader(char *name)
{
	for (int i=0; i<num_shaders; i++) {
		if (!strcmp(name, shader[i]->name)) return shader[i];
	}
	return NULL;
}

void ListShaders()
{
	printf("ListShaders: %d shaders\n", num_shaders);
	for (int i=0; i<num_shaders; i++) {
		printf("  '%s'\n", shader[i]->name);
	}
}

shader_t *AllocShader()
{
	shader_t *s = (shader_t*) malloc (sizeof(shader_t));
	if (!s) error("AllocShader: Insufficient memory for shader!\n");
	memset(s, 0, sizeof(shader_t));
	return s;
}

mod_t *AllocMod()
{
	mod_t *ret = (mod_t*)malloc(sizeof(mod_t));
	if (!ret) error("AllocMod: insufficient memory for mod!\n");
	memset(ret, 0, sizeof(mod_t));
	return ret;
}

// return the type of wave function
int ReadFuncType()
{
	int ret = 0;
	GetToken(0);
	if (!strcasecmp(token, "sin"))
		ret = WAVE_SINE;
	else if (!strcasecmp(token, "triangle")) 
		ret = WAVE_TRIANGLE;
	else if (!strcasecmp(token, "square"))
		ret = WAVE_SQUARE;
	else if (!strcasecmp(token, "sawtooth"))
		ret = WAVE_SAWTOOTH;
	else if (!strcasecmp(token, "inversesawtooth"))
		ret = WAVE_INV_SAWTOOTH;
	else if (!strcasecmp(token, "noise")) 
		ret = WAVE_NOISE;
	else 
		error("unknown wave function: '%s'\n", token);
	return ret;
}

// read n floats and store them sequentially
void ReadArgs(float *ptr, int n)
{
	for (int i=0; i<n; i++) {
		GetToken(0);
		sscanf(token, "%f", ptr++);
	}
}

// add parsed shader to list of shaders
// FIXME: ensure no duplicate names?
void AddShader(shader_t *s)
{
	if (num_shaders >= MAX_SHADERS) error("AddShader: too many shaders!\n");
	shader[num_shaders++] = s;
}

// parse a single shader layer
int ParseLayer(layer_t *layer)
{
	int ignore = 0;
	layer->numtex = 0;
	layer->rgbgen = RGBGEN_IDENTITY;      // default: white
	layer->tcgen = TCGEN_NONE;            // default: no texture generation
	layer->tcmod = NULL;                  // default: no mods
	layer->blend_src = GL_ONE;            // default: no blending
	layer->blend_dst = GL_ZERO;
	layer->alpha_func = ALPHA_FUNC_NONE;  // default: no alpha function
	layer->depth_func = GL_LEQUAL;        // default: pass on less or equal
	layer->depth_mask = 0;                // default: on for opaque/off for trans
	                                      // if trans and 1, then we write 
	
	while (1) {
		GetToken(1);
		if (!strcmp(token, "}")) {
			break;
		}
		
		if (!strcasecmp(token, "map") ||
		    !strcasecmp(token, "clampmap")) {
			GetToken(0);

			// FIXME: handle clampmap somehow!!
			if (!strcasecmp(token, "$lightmap")) {
				layer->numtex = -1;
			} else {
				layer->numtex = 1;
				layer->fps = 1;
				lowercase(token);
				strcpy(layer->texname[0], token);
			}
		} 
		else if (!strcasecmp(token, "blendfunc")) {
			GetToken(0);
			if (!strcasecmp(token, "add") ||        // FIXME: only 1 gl_add in 
			    !strcasecmp(token, "gl_add")) {     // all id shaders (sfx.shader)
				layer->blend_src = GL_ONE;
				layer->blend_dst = GL_ONE;
			} else if (!strcasecmp(token, "filter")) {
				layer->blend_src = GL_DST_COLOR;
				layer->blend_dst = GL_ZERO;
				// OR
				// layer->blend_src = GL_ZERO;
				// layer->blend_dst = GL_SRC_COLOR;
			} else if (!strcasecmp(token, "blend")) {
				layer->blend_src = GL_SRC_ALPHA;
				layer->blend_dst = GL_ONE_MINUS_SRC_ALPHA;
			} else {
				if (!strcasecmp(token, "GL_ONE")) 
					layer->blend_src = GL_ONE;
				else if (!strcasecmp(token, "GL_ZERO")) 
					layer->blend_src = GL_ZERO;
				else if (!strcasecmp(token, "GL_DST_COLOR"))
					layer->blend_src = GL_DST_COLOR;
				else if (!strcasecmp(token, "GL_ONE_MINUS_DST_COLOR"))
					layer->blend_src = GL_ONE_MINUS_DST_COLOR;
				else if (!strcasecmp(token, "GL_SRC_ALPHA")) 
					layer->blend_src = GL_SRC_ALPHA;
				else if (!strcasecmp(token, "GL_ONE_MINUS_SRC_ALPHA"))
					layer->blend_src = GL_ONE_MINUS_SRC_ALPHA;
				else 
					error("Unknown src blend func: '%s'\n", token);
				
				GetToken(0);
				if (!strcasecmp(token, "GL_ONE")) 
					layer->blend_dst = GL_ONE;
				else if (!strcasecmp(token, "GL_ZERO")) 
					layer->blend_dst = GL_ZERO;
				else if (!strcasecmp(token, "GL_SRC_COLOR")) 
					layer->blend_dst = GL_SRC_COLOR;
				else if (!strcasecmp(token, "GL_DST_COLOR"))
					layer->blend_dst = GL_DST_COLOR;
				else if (!strcasecmp(token, "GL_ONE_MINUS_SRC_COLOR"))
					layer->blend_dst = GL_ONE_MINUS_SRC_COLOR;
				else if (!strcasecmp(token, "GL_ONE_MINUS_DST_COLOR"))
					layer->blend_dst = GL_ONE_MINUS_DST_COLOR;
				else if (!strcasecmp(token, "GL_SRC_ALPHA"))
					layer->blend_dst = GL_SRC_ALPHA;
				else if (!strcasecmp(token, "GL_DST_ALPHA")) 
					layer->blend_dst = GL_DST_ALPHA;
				else if (!strcasecmp(token, "GL_ONE_MINUS_SRC_ALPHA"))
					layer->blend_dst = GL_ONE_MINUS_SRC_ALPHA;
				else if (!strcasecmp(token, "GL_ONE_MINUS_DST_ALPHA"))
					layer->blend_dst = GL_ONE_MINUS_DST_ALPHA;
				else 
					error("Unknown dst blend func: '%s'\n", token);
			}
		}
		else if (!strcasecmp(token, "depthwrite")) {
			layer->depth_mask = 1;
		} 
		else if (!strcasecmp(token, "depthfunc")) {
			GetToken(0);
			if (!strcasecmp(token, "lequal")) {
				layer->depth_func = GL_LEQUAL;
			} else if (!strcasecmp(token, "equal")) {
				layer->depth_func = GL_EQUAL;
			} else
				error("unknown depth func: '%s'\n", token);
		}
		else if (!strcasecmp(token, "alphafunc")) {
			GetToken(0);
			if (!strcasecmp(token, "gt0")) 
				layer->alpha_func = ALPHA_FUNC_GT0;
			else if (!strcasecmp(token, "lt128")) 
				layer->alpha_func = ALPHA_FUNC_LT128;
			else if (!strcasecmp(token, "ge128")) 
				layer->alpha_func = ALPHA_FUNC_GE128;
			else 
				error("unknown alphafunc: '%s'\n", token);
		}
		else if (!strcasecmp(token, "animmap")) {
			GetToken(1);
			sscanf(token, "%f", &layer->fps);

			for (; layer->numtex < 8; ) {
				GetToken(1);

				// if keyword get out
				if (!strcmp(token, "}")) return 1;
				if (!strcasecmp(token, "rgbgen")) break;
				if (!strcasecmp(token, "tcmod")) break;
				if (!strcasecmp(token, "tcgen")) break;
				if (!strcasecmp(token, "alphagen")) break;
				if (!strcasecmp(token, "alphafunc")) break;
				if (!strcasecmp(token, "blendfunc")) break;
				if (!strcasecmp(token, "depthfunc")) break;
				if (!strcasecmp(token, "depthwrite")) break;

				// otherwise copy it over
				lowercase(token);
				strcpy(layer->texname[layer->numtex++], token);
			}
		}

		// note we may have gotten the current token from the animmap loop
		if (!strcasecmp(token, "tcgen")) {
			GetToken(0);
			if (!strcasecmp(token, "environment")) {
				layer->tcgen = TCGEN_ENVIRONMENT;
			} else if (!strcasecmp(token, "vector")) {
				// FIXME: do something here!
			} else if (!strcasecmp(token, "base")) {
				layer->tcgen = TCGEN_BASE;
			} else if (!strcasecmp(token, "lightmap")) {
				layer->tcgen = TCGEN_LIGHTMAP;
			} else 
				error("unknown tcgen type: '%s'\n",token);
		}
		if (!strcasecmp(token, "rgbgen")) {
			GetToken(0);
			if (!strcasecmp(token, "identity") ||
			    !strcasecmp(token, "identitylighting")) {
				layer->rgbgen = RGBGEN_IDENTITY;
			} else if (!strcasecmp(token, "entity")) {
				layer->rgbgen = RGBGEN_ENTITY;
			} else if (!strcasecmp(token, "oneminusentity")) {
				layer->rgbgen = RGBGEN_ONE_MINUS_ENTITY;
			} else if (!strcasecmp(token, "vertex") ||
				   !strcasecmp(token, "exactvertex")) {
				layer->rgbgen = RGBGEN_VERTEX;
			} else if (!strcasecmp(token, "oneminusvertex")) {
				layer->rgbgen = RGBGEN_ONE_MINUS_VERTEX;
			} else if (!strcasecmp(token, "lightingdiffuse")) {
				layer->rgbgen = RGBGEN_LIGHTING_DIFFUSE;
			} else if (!strcasecmp(token, "wave")) {
				layer->rgbgen = RGBGEN_WAVE;
				layer->rgbarg[0] = ReadFuncType();
				ReadArgs(&layer->rgbarg[1], 4);
			} else 
				error("unknown rgben type: '%s'\n", token);
		}
		else if (!strcasecmp(token, "tcmod")) {
			mod_t *cur = AllocMod();
			GetToken(0);
			if (!strcasecmp(token, "scale")) {
				cur->type = TCMOD_SCALE;
				ReadArgs(&cur->arg[0], 2);
			} else if (!strcasecmp(token, "rotate")) {
				cur->type = TCMOD_ROTATE;
				ReadArgs(&cur->arg[0], 1);
			} else if (!strcasecmp(token, "scroll")) {
				cur->type = TCMOD_SCROLL;
				ReadArgs(&cur->arg[0], 2);				
			} else if (!strcasecmp(token, "stretch")) {
				cur->type = TCMOD_STRETCH;
				cur->arg[0] = ReadFuncType();
				ReadArgs(&cur->arg[1], 4);
			} else if (!strcasecmp(token, "transform")) {
				cur->type = TCMOD_TRANSFORM;
				ReadArgs(&cur->arg[0], 6);
			} else if (!strcasecmp(token, "turb")) {
				cur->type = TCMOD_TURB;
				ReadArgs(&cur->arg[0], 4);
			} else 
				error("unknown tcmod function: '%s'\n", token);
			
			cur->next = NULL;
			if (layer->tcmod == NULL) layer->tcmod = cur;
			else {
				mod_t *head = layer->tcmod;
				while (head->next) head = head->next;
				head->next = cur;
			}
		}
	}
	if (ignore) return 0;
	return 1;
}

// read one shader from file
shader_t *ParseShader()
{
	shader_t *sh = AllocShader();
	sh->numlayers = 0;
	sh->cull = GL_BACK;

	GetToken(1);
	if (poffset >= plen) return NULL;
	strcpy(sh->name, token);
	//printf("Parsing shader: '%s'\n", sh->name);

	GetToken(1);
	if (strcmp(token, "{")) error("expected a '{', got '%s'.\n", token);

	while (1) {
		GetToken(1);
		//printf("token: '%s'\n", token);
		if (!strcasecmp(token, "surfaceparm")) {
			GetToken(0);
			if (!strcasecmp(token, "nodraw")) {
				sh->flags |= SURF_NODRAW;
			}
			if (!strcasecmp(token, "nolightmap")) {
				sh->flags |= SURF_NOLIGHTMAP;
			}
			if (!strcasecmp(token, "sky")) {
				sh->flags |= SURF_SKY;
			}
		} else if (!strcasecmp(token, "cull")) {
			GetToken(0);
			if (!strcasecmp(token, "front")) {
				sh->cull = GL_BACK;
			} else if (!strcasecmp(token, "back") ||
				   !strcasecmp(token, "backsided")) {
				sh->cull = GL_FRONT;
			} else if (!strcasecmp(token, "none") ||
				   !strcasecmp(token, "disable") ||
				   !strcasecmp(token, "twosided")) {
				sh->cull = GL_NONE;
			} else 
				error("unknown culling mode: '%s'\n", token);
		}
		else if (!strcasecmp(token, "deformvertexes")) {
			mod_t *cur = AllocMod();
			GetToken(0);
			if (!strcasecmp(token, "wave")) {
				cur->type = DEFORM_WAVE;
				ReadArgs(&cur->arg[0], 1);
				cur->arg[1] = ReadFuncType();
				ReadArgs(&cur->arg[2], 4);
			} else if (!strcasecmp(token, "normal")) {
				cur->type = DEFORM_WAVE;
				ReadArgs(&cur->arg[0], 2);
				//cur->arg[1] = ReadFuncType();
				//ReadArgs(&cur->arg[2], 3);
			} else if (!strcasecmp(token, "bulge")) {
				cur->type = DEFORM_BULGE;
				ReadArgs(&cur->arg[0], 3);				
			} else if (!strcasecmp(token, "move")) {
				cur->type = DEFORM_MOVE;
				ReadArgs(&cur->arg[0], 3);
				cur->arg[3] = ReadFuncType();
				ReadArgs(&cur->arg[4], 4);
			} else if (!strcasecmp(token, "autosprite")) {
				cur->type = DEFORM_AUTOSPRITE;
			} else if (!strcasecmp(token, "autosprite2")) {
				cur->type = DEFORM_AUTOSPRITE2;
			} else if (!strcasecmp(token, "projectionshadow")) {
				cur->type = DEFORM_PROJECTION;
			} else if (!strcasecmp(token, "text0")) {
				cur->type = DEFORM_TEXT0;
			} else if (!strcasecmp(token, "text1")) {
				cur->type = DEFORM_TEXT1;
			} else 
				error("unknown deform function: '%s'\n", token);
			
			cur->next = NULL;
			if (sh->vertmod == NULL) sh->vertmod = cur;
			else {
				mod_t *head = sh->vertmod;
				while (head->next) head = head->next;
				head->next = cur;
			}
		}

		// process a layer/stage
		if (!strcmp(token, "{")) {
			// if true: a valid layer; otherwise, don't include it
			if (ParseLayer(&sh->layer[sh->numlayers])) 
				sh->numlayers++;
		}
		else if (!strcmp(token, "}")) {
			break;
		}
	}
	return sh;
}

// read all shaders in file
int ParseShaderFile(char *filename)
{
	int len, c;
	FILE *f;
	char *data;
	shader_t *sh;

	f = fopen(filename, "rb");
	if (!f)	{
		printf("Couldn't open '%s' for reading.\n", filename);
		return 0;
	}

	fseek(f, 0L, SEEK_END);
	len = ftell(f);
	fseek(f, 0L, SEEK_SET);
	
	data = (char*) malloc (len);
	if (!data) error("ParseShaderFile: Insufficient memory! (%d bytes)\n", len);
	if ((c = fread(data, 1, len, f)) != len) 
		error("ParseShaderFile: got only %d of %d bytes!\n", c, len);
	fclose(f);
	
	SetupParse(data, len);
	for (c = 0; poffset < len; ) {
		sh = ParseShader();
		if (sh) {
			AddShader(sh);
			c++;
		}
	}
	free(data);
	return c;
}

// Read shaders from 'baseq3/scripts/'
char script_names[][64] = {
	"base.shader",
	"base_button.shader",
	"base_floor.shader",
	"base_light.shader",
	"base_object.shader",
	"base_support.shader",
	"base_trim.shader",
	"base_wall.shader",
	"common.shader",
	"ctf.shader",
	"eerie.shader",
	//"base2.shader",
	"gfx.shader",
	"gothic_block.shader",
	"gothic_floor.shader",
	"gothic_light.shader",
	"gothic_trim.shader",
	"gothic_wall.shader",
	"hell.shader",
	"liquid.shader",
	"menu.shader",
	"models.shader",
	"organics.shader",
	"sfx.shader",
	"shrine.shader",
	"skin.shader",
	"sky.shader",
	"test.shader"
};

void InitShaders()
{
	int i,j,n;
	char tmp[128],base[] = "baseq3/scripts/";
	struct dirent **namelist = NULL;
	struct dirent *t, fuck;

#ifdef CYGWIN
	//FIXME: how to get this to work with cygwin?
	//n = fl_filename_list("baseq3/scripts/", &namelist, fl_alphasort);
	n = 27;
	namelist = (dirent**)malloc(sizeof(dirent*)*30);
	for (i=0; i<n; i++) {
		// FIXME: WTF!??!?! sizeof(dirent) = 1??!?!?!
		t = (dirent*)malloc(512);
		strcpy(t->d_name, script_names[i]);
		namelist[i] = t;
	}
#else
	n = scandir("baseq3/scripts/", &namelist, 0, alphasort);
#endif
	if (n < 0) {
		perror("scandir");
		exit(1);
	} 

	conprintf(" ");
	conprintf("=============== InitShaders ===============");
	conprintf("Reading shaders in 'baseq3/scripts/':");
	for (i=0; i<n; i++) {
		if (strstr(namelist[i]->d_name, "shader")) {
			strcpy(tmp, base);
			strcat(tmp, namelist[i]->d_name);
			j = ParseShaderFile(tmp);
			conprintf("   ... '%s': %d shaders.", namelist[i]->d_name, j);
		}
		free(namelist[n]);
	}
	if (namelist) free(namelist);

	conprintf("Total shaders: %d", num_shaders);
	conprintf("=============== InitShaders ===============");
	conprintf(" ");
}

// takes a parsed shader and loads its textures 
void LoadShader(shader_t *sh, int permanent)
{
	for (int i=0; i<sh->numlayers; i++) {
		if (sh->layer[i].numtex == -1) continue;
		if (!sh->layer[i].numtex) continue;
		for (int j=0; j<sh->layer[i].numtex; j++) {
			texture_t *tx; 
			sh->layer[i].texture[j] = tx = LoadTexture(sh->layer[i].texname[j]);
			if (!sh->layer[i].texture[j]) continue;

			// mark these textures as permanent if 'permanent' is set
			sh->layer[i].texture[j]->flags |= TEX_PERMANENT;

			// perform gamma correction
			GammaCorrect(tx->width*tx->height, tx->comp, tx->data);
			
			// FIXME: anisotropic filtering?
			// setup the texture to use user specified filtering
			TextureSetup(sh->layer[i].texture[j], -1, -1, GL_REPEAT, GL_REPEAT);
		}
	}
}

// returns true if layer is translucent
int LayerTranslucent(layer_t *layer)
{
	if ((layer->blend_src == GL_ONE && layer->blend_dst == GL_ZERO) ||
	    (layer->blend_src == GL_ZERO && layer->blend_dst == GL_ONE)) return 0;
	return 1;
}

// returns true if shader defines translucent surface
int ShaderTranslucent(shader_t *sh)
{
	if (!sh->numlayers) return 0;
	return LayerTranslucent(&sh->layer[0]);
}

