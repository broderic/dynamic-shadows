#include <stdarg.h>
#include "shad.h"

int console_state = CONSOLE_FULL;
float console_bottom = -1.0f;

// console text
#define CONSOLE_MAX_LINES     1024
#define CONSOLE_MAX_LINE_LEN  128

int c_input_len=0;
int c_hoffset=0;
int c_toffset=0;
char c_input[128];

// font stuff
texture_t *font;
float font_xstep = 0.045;
float font_ystep = 0.035;

// background shader
surf_t console_surf;
face_t console_face;
shader_t *background;

// properties
cvar_t *con_speed;



/****************************************************************************/
/* Text handling                                                            */
/****************************************************************************/
// wrap long lines into several
void wordWrapLine(int skipwhite, char *str, char *out, int max, int *outoff)
{
	int i,j,k;
	char word[1024];

	// skip leading whitespace
	i = 0;
	if (skipwhite) for (; str[i]; i++) 
		// FIXME: cygwin won't compile this
		//if (!isspace(str[i])) break;
		if (str[i]!=' ') break;

	for (j=0; j<max && str[i]; ) {
		// read the next word
		for (k=0; j+k<max && str[i+k] && 
			     // FIXME: cygwin won't compile this
			     //			     !isspace(str[i+k]);
			     str[i+k]!=' ';
		     ) word[k++] = str[i+k];
		if (j + k >= max) break;

		// didn't overflow line, so copy it over
		word[k] = 0;
		out[j] = 0;
		strcat(out, word);
		j += k;
		i += k;
		
		// read some whitespace
		while (j < max && str[i] && 
		       //FIXME: cygwin won't compile this
		       //isspace(str[i])
		       str[i] == ' '
		       ) 
		       
			out[j++] = str[i++];
	}
	out[j] = 0;
	*outoff = i;
}


/****************************************************************************/
/* Draw the console                                                         */
/****************************************************************************/
void DrawConsoleBackground()
{
	if (background == NULL) {
		glColor4f(.3, .3, .3, 1.0);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
  		        glVertex3f(-1.0, console_bottom+2, 0.0);
			glVertex3f( 1.0, console_bottom+2, 0.0);
			glVertex3f( 1.0, console_bottom+0, 0.0);
			glVertex3f(-1.0, console_bottom+0, 0.0);
		glEnd();
		glEnable(GL_TEXTURE_2D);
	} else {
		DrawFace(&console_face, &player);
	}

	// yellow line at bottom
	glDisable(GL_TEXTURE_2D);
	glColor4fv(yellow);
	glBegin(GL_LINES);
		glVertex3f(-1.0, console_bottom, 0.0);
		glVertex3f( 1.0, console_bottom, 0.0);
	glEnd();
	glEnable(GL_TEXTURE_2D);
}

void DrawConsoleText()
{
	int j;
	float y;
	char *str;

	glColor4fv(white);
	SetBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, font->name);

	DrawString(-1, console_bottom+font_ystep+0.01, "$%s_", c_input);

	y = console_bottom + 2*font_ystep + 0.01;
	for (j=c_toffset; y<1.0; y+=font_ystep, j++) {
		str = Shell_GetText(j);
		if (!str) continue;

		// FIXME: do word wrap somehow
		DrawString(-1, y, " %s", str);
	}
}

void DrawConsole()
{
	if (!console_state) return;
	//FIXME: remove this when console shader is implemented!
	SetAlphaFunc(ALPHA_FUNC_NONE);

	DrawConsoleBackground();
	DrawConsoleText();
}



void ConRegisterChar(char c)
{
	char found[33];

	switch(c) {
	case 8:  // Backspace
		if (c_input_len) c_input[--c_input_len] = 0;
		break;
		
	case 9:  // Tab
		if (Shell_AutoComplete(c_input, found)) {
			strcpy(c_input, found);
			strcat(c_input, " ");
			c_input_len = strlen(c_input);
		}
		break;

	case 127: // Delete
		  // FIXME: do anything?
		break;

	case 13:  // Enter
		conprintf(c_input);
		ProcessInput(c_input);
		Shell_AddHistory(c_input);
		c_input_len = 0;
		c_input[0] = 0;
		break;

	default:  // normal input
		if (c_input_len < CONSOLE_MAX_LINE_LEN) { 
		c_input[c_input_len++] = c;
		c_input[c_input_len] = 0;
		//strcpy(c_hist[c_nhist], c_input);
		}
	}

}

// FIXME: un-break shell history
void ConRegisterSpecialChar(char c)
{
	char *str;
	switch(c) {
	case GLUT_KEY_UP:
		str = Shell_GetHistory(++c_hoffset);
		if (str) {
			strcpy(c_input, str);
			c_input_len = strlen(c_input);
		} else {
			--c_hoffset;
		}
		break;

	case GLUT_KEY_DOWN:
		str = Shell_GetHistory(--c_hoffset);
		if (str) {
			strcpy(c_input, str);
			c_input_len = strlen(c_input);
		} else {
			++c_hoffset;
		}
		break;

	case GLUT_KEY_PAGE_UP:
		c_toffset++;
		break;

	case GLUT_KEY_PAGE_DOWN:
		c_toffset--;
		break;
	}
}

void DrawCharacter(float x1, float y1, float x2, float y2, int c)
{
	float ss = 1.0 / 16.0;
	float tx = (c & 15) / 16.0;
	float ty = (c / 16) / 16.0;

	if (c == '\n' || c == '\r') return;

	glBegin(GL_QUADS);
	glTexCoord2f(tx, ty);
	glVertex2f(x1, y1);

	glTexCoord2f(tx+ss, ty);
	glVertex2f(x2, y1);

	glTexCoord2f(tx+ss, ty+ss);
	glVertex2f(x2, y2);

	glTexCoord2f(tx, ty+ss);
	glVertex2f(x1, y2);
	glEnd();

}

void DrawString(float x, float y, char *args, ...)
{
	int i, len;
	float cx, sx, sy;
	char out[1024];

        va_list list;
        va_start(list, args);
        vsnprintf(out, 1023, args, list);
        va_end(list);

	// fit 75 characters onto the screen
	sx = 2 * 1/75.0;
	sy = sx * player.viewwidth / player.viewheight;

	len = strlen(out);
	for(i=0, cx = x; i<len; i++) {
		DrawCharacter(cx, y, cx+sx, y-sy, out[i]);
		cx += sx;
	}
}

void LoadFont()
{
	char font_file[] = "gfx/2d/bigchars.tga";
	conprintf("Loading font: '%s'", font_file);
	font = LoadTexture(font_file);
	if (!font) error("Couldn't load font file!\n");
	font->flags |= TEX_PERMANENT;
	TextureSetup(font,  GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP, GL_CLAMP);
}

// load the console shader and setup the face for drawing
void LoadBackground()
{
	mesh_t *mesh;
	char name[] = "console";
	conprintf("Loading background shader: '%s'...", name);
	background = FindShader(name);
	if (!background) {
		conprintf("Console shader not found!");
	} else {
		LoadShader(background, 1);
	}
	
	console_surf.flags = 0;
	console_surf.contents = 0;
	console_surf.shader = background;

	console_face.type = FACE_POLYGON;
	console_face.surface = &console_surf;
	console_face.mesh = mesh = MeshAllocate();

	mesh->numverts = 4;
	mesh->numtris = 2;
	mesh->trinorm = NULL;
	mesh->numedges = 0;
	mesh->orvert = NULL;
	mesh->vertex = (vertex_t*)malloc(4*sizeof(vertex_t));
	mesh->meshvert = (unsigned int*)malloc(6*sizeof(unsigned int));

	// setup the vertices
	// FIXME: specify vertex colors?
	VectorSet3(-1, 0, 0, mesh->vertex[0].p);
	VectorSet3( 0, 0, 1, mesh->vertex[0].norm);
	VectorSet2( 0, 0, mesh->vertex[0].tc);

	VectorSet3( 1, 0, 0, mesh->vertex[1].p);
	VectorSet3( 0, 0, 1, mesh->vertex[1].norm);
	VectorSet2( 1, 0, mesh->vertex[1].tc);

	VectorSet3( 1, 0, 0, mesh->vertex[2].p);
	VectorSet3( 0, 0, 1, mesh->vertex[2].norm);
	VectorSet2( 1, 1, mesh->vertex[2].tc);

	VectorSet3(-1, 0, 0, mesh->vertex[3].p);
	VectorSet3( 0, 0, 1, mesh->vertex[3].norm);
	VectorSet2( 0, 1, mesh->vertex[3].tc);

	// set the meshverts
	mesh->meshvert[0] = 0;
	mesh->meshvert[1] = 1;
	mesh->meshvert[2] = 2;
	mesh->meshvert[3] = 0;
	mesh->meshvert[4] = 2;
	mesh->meshvert[5] = 3;
}

void ConsoleInit()
{
	conprintf("================= ConsoleInit =================");
	c_input_len = 0;
	c_input[0] = 0;
	register_cvar(&con_speed, "con_speed", "0.07");

	LoadFont();
	LoadBackground();

	conprintf("================= ConsoleInit =================");
	conprintf(" ");
}

// update position of console
void UpdateConsole()
{
	if (!console_state) return;
	if (!bsp_loaded) console_state = CONSOLE_FULL;

	double s = time_delta / (double)cvar_double(p_interval);
	if (console_state == CONSOLE_LOWERING) {
		console_bottom -= cvar_float(con_speed)*s;
		if (console_bottom < -0.5) {
			console_bottom = -0.5;
			console_state = CONSOLE_DOWN;
		}
	} else if (console_state == CONSOLE_RISING) {
		console_bottom += cvar_float(con_speed)*s;
		if (console_bottom >= 1.0) {
			console_bottom = 1.0;
			console_state = CONSOLE_HIDDEN;
		}
	} else if (console_state == CONSOLE_FULL) {
		console_bottom = -1.0;
	}

	// move the console vertices 
	console_face.mesh->vertex[0].p[1] = console_bottom+2;
	console_face.mesh->vertex[1].p[1] = console_bottom+2;
	console_face.mesh->vertex[2].p[1] = console_bottom;
	console_face.mesh->vertex[3].p[1] = console_bottom;	
}

