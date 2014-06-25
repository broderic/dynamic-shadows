#include <ctype.h>
#include <math.h>
#include "shad.h"


// key track of which keys are down
int keystate[256];
int specstate[256];

// player movement rates
cvar_t *p_interval;
cvar_t *p_fwd_speed;
cvar_t *p_side_speed;
cvar_t *p_up_speed;
cvar_t *p_turn_speed;

// mouse stuff
int mouse_down;
int mouse_x, mouse_y;
int mouse_old_x = -1;
int mouse_old_y = -1;

cvar_t *mouse_sensitivity;
cvar_t *mouse_horz_sens;
cvar_t *mouse_vert_sens;

void HandleInput()
{
	double s = time_delta / (double)cvar_double(p_interval);
	double ms = cvar_double(mouse_sensitivity);
	double mh = cvar_double(mouse_horz_sens);
	double mv = cvar_double(mouse_vert_sens);
	double ps = cvar_double(p_side_speed);
	double pf = cvar_double(p_fwd_speed);
	double pu = cvar_double(p_up_speed);
	double pt = cvar_double(p_turn_speed);

	if (mouse_old_x != -1) {
		player.yaw   -= s*ms*mh*(mouse_x - mouse_old_x);
		player.pitch -= s*ms*mv*(mouse_y - mouse_old_y);
	}

	mouse_old_x = mouse_x;
	mouse_old_y = mouse_y;

	if (console_state) return;

	// move
	if (keystate[(int)'z']) player.pos[2] += 0.1;
	if (keystate[(int)'s']) VectorMA(player.pos,  ps*s, player.rt, player.pos);
	if (keystate[(int)'f']) VectorMA(player.pos, -ps*s, player.rt, player.pos);
	if (keystate[(int)'d']) VectorMA(player.pos, -pf*s, player.vn, player.pos);
	if (keystate[(int)'e']) VectorMA(player.pos,  pf*s, player.vn, player.pos);
	if (specstate[GLUT_KEY_PAGE_UP])   VectorMA(player.pos, pu*s,player.up,player.pos);
	if (specstate[GLUT_KEY_PAGE_DOWN]) VectorMA(player.pos,-pu*s,player.up,player.pos);

	// look 
	if (keystate[(int)'q']) player.roll += pt*s;
	if (keystate[(int)'r']) player.roll -= pt*s;
	if (specstate[GLUT_KEY_RIGHT]) player.yaw -= pt*s; 
	if (specstate[GLUT_KEY_LEFT])  player.yaw += pt*s; 
	if (specstate[GLUT_KEY_UP])    player.pitch += pt*s; 
	if (specstate[GLUT_KEY_DOWN])  player.pitch -= pt*s;
	    
	player.yaw = fmod(player.yaw, 360);
	player.pitch = fmod(player.pitch, 360);
	player.roll = fmod(player.roll, 360);
}


// Called when a key is pressed
void glutKeyboardUpCallback(unsigned char key, int x, int y)
{
        //printf( "keyup=%i\n", key );
        keystate[tolower(key)] = keystate[toupper(key)] = 0;
}

void Keyboard(unsigned char key, int x, int y)
{
	if (key == 27) Shutdown();
		

	keystate[toupper(key)] = keystate[tolower(key)] = 1;

	if (key == '`' || key == '~') {
		switch(console_state) {
		case CONSOLE_HIDDEN:
			console_state = CONSOLE_LOWERING;
			break;

		case CONSOLE_LOWERING:
			console_state = CONSOLE_RISING;
			break;

		case CONSOLE_RISING:
			console_state = CONSOLE_LOWERING;
			break;

		case CONSOLE_DOWN:
			console_state = CONSOLE_RISING;
			break;
		}
		return;
	}

	if (console_state) {
		ConRegisterChar(key);
		return;
	}

	switch(key) {
	case 'w': case 'W': 
		if (cvar_int(draw_wireframe)) cvar_set(draw_wireframe, "0");
		else cvar_set(draw_wireframe, "1");
		break;
	}
}

void glutSpecialUpCallback(int key, int x, int y)
{
	specstate[key] = 0;
}

void SpecialKeys(int key, int x, int y) {
	specstate[key] = 1;
	if (console_state) ConRegisterSpecialChar(key);
}

void handleMouseMotion (int x, int y) {
	mouse_x = x;
	mouse_y = y;
}

// FIXME: currently does nothing
void handleMouseClick(int btn, int state, int x, int y) 
{
	switch (btn) {
	case (GLUT_LEFT_BUTTON):
		if (state == GLUT_UP) {
			mouse_down = 0;
		} else if (state == GLUT_DOWN) {
			mouse_down = 1;
			mouse_x = x;
			mouse_y = y;
		}
		break;
	case (GLUT_MIDDLE_BUTTON):
		if (state == GLUT_UP) {
			mouse_down = 0;
		} else if (state == GLUT_DOWN) {
			mouse_down = 3;
			mouse_x = x;
			mouse_y = y;
		}
		break;
	case (GLUT_RIGHT_BUTTON):
		if (state == GLUT_UP) {
			mouse_down = 0;
		} else if (state == GLUT_DOWN) {
			mouse_down = 2;
			mouse_x = x;
			mouse_y = y;
		}
		break;
	default: break;
	}
}

void InputInit()
{
	register_cvar(&p_interval, "p_interval", "0.01f");
	register_cvar(&p_fwd_speed, "p_fwd_speed", "5.0");
	register_cvar(&p_side_speed, "p_side_speed", "5.0");
	register_cvar(&p_up_speed, "p_up_speed", "5.0");
	register_cvar(&p_turn_speed, "p_turn_speed", "1.15");

	register_cvar(&mouse_sensitivity, "mouse_sensitivity", "20.0");
	register_cvar(&mouse_horz_sens, "mouse_horz_sens", "0.035");
	register_cvar(&mouse_vert_sens, "mouse_vert_sens", "0.015");
}
