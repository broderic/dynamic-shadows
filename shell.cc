#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include <map>
#include <set>
#include <string>
#include "shad.h"
using namespace std;

// TODO:  add logging

// cvar list
map<string, cvar_t*> cvar_list;
map<string, ccmd_t*> ccmd_list;

// argument processing
int nargs;
char arg[32][128];
char *arg_pass[32];

// ccmds & cvars
ccmd_t *cmdlist;
ccmd_t *cvarlist;
cvar_t *shell_echo_to_stdout;
cvar_t *shell_auto_complete_threshold;

// shell text
#define SHELL_MAXLINES 32768
int shell_numlines;
char *shell_text[SHELL_MAXLINES];

// shell history
int shell_numhistory;
char *shell_hist[SHELL_MAXLINES];



/****************************************************************************/
/* console commands                                                         */
/****************************************************************************/
void ccmd_cmdlist(int argc, char **argv)
{
	map<string,ccmd_t*>::iterator i;
	for (i=ccmd_list.begin(); i != ccmd_list.end(); ++i) {
		conprintf("%s", i->first.c_str());
	}
	conprintf("%d ccmds", ccmd_list.size());
}

void ccmd_cvarlist(int argc, char **argv)
{
	map<string,cvar_t*>::iterator i;
	for (i=cvar_list.begin(); i != cvar_list.end(); ++i) {
		conprintf("%s", i->first.c_str());
	}
	conprintf("%d cvars", cvar_list.size());
}


/****************************************************************************/
/* cvar and ccmd stuff                                                      */
/****************************************************************************/
// Add a cvar to the current list
void register_cvar(cvar_t **cv, char *name, char *val)
{
	*cv = (cvar_t*)malloc(sizeof(cvar_t));
	if ((*cv) == NULL) error ("register_cvar: insufficient memory for cvar.\n");
	strncpy((*cv)->value, val, 31);
	strncpy((*cv)->name, name, 31);

	string n = name;
	if (cvar_list.find(n) != cvar_list.end())
		conprintf("register_cvar: shadowing cvar '%s'", name);
	cvar_list[n] = *cv;
}

// Add ccmd to current list
void register_ccmd(ccmd_t **cm, char *name, void (*func)(int,char**))
{
	*cm = (ccmd_t*)malloc(sizeof(ccmd_t));
	if ((*cm) == NULL) error ("RegisterCcmd: insufficient memory for ccmd.\n");
	(*cm)->func = func;
	strncpy((*cm)->name, name, 31);

	string n = name;
	if (ccmd_list.find(n) != ccmd_list.end()) 
		conprintf("register_ccmd: shadowing ccmd '%s'", name);
	ccmd_list[n] = *cm;
}

cvar_t *cvar_find(char *name)
{
	string n = name;
	if (cvar_list.find(n) == cvar_list.end()) return NULL;
	return cvar_list[n];
}

ccmd_t *ccmd_find(char *name)
{
	string n = name;
	if (ccmd_list.find(n) == ccmd_list.end()) return NULL;
	return ccmd_list[n];
}

/////////////////////////////////////////////////////
// Helpers
/////////////////////////////////////////////////////

void cvar_set(cvar_t *cv, char *val)
{
	strcpy(cv->value, val);
}

int cvar_int(cvar_t *cv)
{
	int ret;
	if (!sscanf(cv->value, "%d", &ret)) {
		conprintf("'%s' not integer valued.\n", cv->name);
		return 0;
	}
	return ret;
}

float cvar_float(cvar_t *cv)
{
	float ret;
	if (!sscanf(cv->value, "%f", &ret)) {
		conprintf("'%s' not a float.\n", cv->name);
	}
	return ret;
}

double cvar_double(cvar_t *cv)
{
	double ret;
	if (!sscanf(cv->value, "%lf", &ret)) {
		conprintf("'%s' not a double.\n", cv->name);
	}
	return ret;
}

void cvar_vector3(cvar_t *cv, float *f)
{
	if (sscanf(cv->value, "%f_%f_%f", &f[0], &f[1], &f[2])!=3) {
		conprintf("'%s' not a vector!\n");
	}
}

/****************************************************************************/
/* Input handling                                                           */
/****************************************************************************/

// stores individual arguments in global args[]
void ExtractArguments(char *str)
{
	int at,off;
	char temp[128];
	int len = strlen(str);
	nargs = off = 0;
	for (at=0; at<len; ) {
		if (sscanf(&str[at], "%s%n ", temp, &off) == 1) {
			strcpy(arg[nargs++], temp);
			at += off;
		} else {
			break;
		}
	}	
#if 0
	printf("nargs = %d\n", nargs);
	for (int i=0; i<nargs; i++) {
		printf("'%s'\n", arg[i]);
	}
#endif
}

int ProcessCcmds()
{
	int i;
	ccmd_t *cm = ccmd_find(arg[0]);
	if (cm) {
		cm->func(nargs, arg_pass);
		return 1;
	}
	if (!strcmp(arg[0], "light_state") && nargs >= 3) {
		sscanf(arg[1], "%d", &i);
		if (i >= num_lights) { conprintf("Not a valid light."); return 1; }
		sscanf(arg[2], "%d", &light[i].on);
		return 1;
	}
	if (!strcmp(arg[0], "light_att_const") && nargs >= 3) {
		sscanf(arg[1], "%d", &i);
		if (i >= num_lights) { conprintf("Not a valid light."); return 1; }
		sscanf(arg[2], "%f", &light[i].constant_factor);
		
		printf("%f %f %f\n", light[i].constant_factor,
		       light[i].linear_factor, light[i].quadratic_factor);
		return 1;
	}
	if (!strcmp(arg[0], "light_att_linear") && nargs >= 3) {
		sscanf(arg[1], "%d", &i);
		if (i >= num_lights) { conprintf("Not a valid light."); return 1; }
		sscanf(arg[2], "%f", &light[i].linear_factor);
		
		printf("%f %f %f\n", light[i].constant_factor,
		       light[i].linear_factor, light[i].quadratic_factor);
		
		return 1;
	}
	if (!strcmp(arg[0], "light_att_quad") && nargs >= 3) {
		sscanf(arg[1], "%d", &i);
		if (i >= num_lights) { conprintf("Not a valid light."); return 1; }
		sscanf(arg[2], "%f", &light[i].quadratic_factor);
		
		printf("%f %f %f\n", light[i].constant_factor,
		       light[i].linear_factor, light[i].quadratic_factor);
		return 1;
	}
	if (!strcmp(arg[0], "move_light_here") && nargs >= 2) {
		sscanf(arg[1], "%d", &i);
		if (i >= num_lights) { conprintf("Not a valid light."); return 1; }
		VectorCopy(player.pos, light[i].cam.pos, 3);
	}
	if (!strcmp(arg[0], "move_sarge_here")) {
		VectorCopy(player.pos, sarge.pos, 3);
		return 1;
	}
	if (!strcmp(arg[0], "add_light_here")) {
		vec4_t pos;
		VectorCopy(player.pos, pos, 3); pos[3] = 1.0;
		
		AddLight(pos, 
			 45, 
			 white, 
			 player.yaw, player.pitch, player.roll,
			 0, 0, 0.00005);
		return 1;
	}
	return 0;
}

int ProcessCvars()
{
	cvar_t *cv = cvar_find(arg[0]);
	if (!cv) return 0;
	switch(nargs) {
	case 1:	conprintf("%s '%s'", cv->name, cv->value); break;
	case 2:	cvar_set(cv, arg[1]); break;
	}
	return 1;
}

void ProcessInput(char *str)
{
	ExtractArguments(str);
	if (ProcessCcmds()) return;
	if (ProcessCvars()) return;
	conprintf("Unknown command/cvar: '%s'", arg[0]);
}


// add the string to shell text
void Shell_AddText(char *str)
{
	if (shell_numlines >= SHELL_MAXLINES)
		error("shell_numlines >= SHELL_MAXLINES\n");	
	
	shell_text[shell_numlines] = (char*) malloc (strlen(str)+1);
	if (!shell_text[shell_numlines])
		error("Insufficient memory for string\n");

	strcpy(shell_text[shell_numlines], str);
	shell_numlines++;	
}

// add the string to shell text
void Shell_AddHistory(char *str)
{
	if (shell_numhistory >= SHELL_MAXLINES)
		error("shell_numhistory >= SHELL_MAXLINES\n");	
	
	shell_hist[shell_numhistory] = (char*) malloc (strlen(str)+1);
	if (!shell_hist[shell_numhistory])
		error("Insufficient memory for string\n");

	strcpy(shell_hist[shell_numhistory], str);
	shell_numhistory++;	
}


// off is offset from the last line entered
char *Shell_GetText(int off)
{
	int n = shell_numlines - off;
	if (n < 0 || n >= shell_numlines) return NULL;
	return shell_text[n];
}

char *Shell_GetHistory(int off)
{
	int n = shell_numhistory - off;
	if (n < 0 || n >= shell_numhistory) return NULL;
	return shell_hist[n];
}

//    if we find a unique completion, we store it in out and return 1
//    if more than one completion we display the list and return 0
//    else we return 0
int is_prefix(string a, string b)
{
	int i, len = (a.size() > b.size())? b.size() : a.size();
	for (i=0; i<len; i++) {
		if (a[i] != b[i]) break;
	}
	return (i < (int)a.size()) ? 0 : 1;
}

int Shell_AutoComplete(char *str, char *out)
{
	string n = str;
	map<string, cvar_t*>::iterator cv_l, cv;
	map<string, ccmd_t*>::iterator cm_l, cm;
	set<string> found;

	cv_l = cvar_list.lower_bound(n);
	cm_l = ccmd_list.lower_bound(n);
	for (cv = cv_l; cv != cvar_list.end(); ++cv) {
		if(is_prefix(n, cv->first)) found.insert(cv->first);
		else break;
	}
	for (cm = cm_l; cm != ccmd_list.end(); ++cm) {
		if (is_prefix(n, cm->first)) found.insert(cm->first);
		else break;
	}

	switch(found.size()) {
	case 0: return 0;
	case 1: strncpy(out, found.begin()->c_str(), 32);
		return 1;
	default: // display list of possibilities
		conprintf(" ");
		conprintf("%d possibilities", found.size());
		if (cvar_int(shell_auto_complete_threshold) < (int)found.size()) {
			conprintf("too many to show; set shell_auto_complete_threshold");
		} else {
			set<string>::iterator i;
			for (i=found.begin(); i!=found.end(); ++i)
				conprintf("  %s", i->c_str());
			return 0;
		}
	}
	return 0;
}


void conprintf(char *str, ...)
{
        char out[1024];
	if (!strlen(str)) return;

        va_list list;
        va_start(list, str);
        vsnprintf(out, 1023, str, list);
        va_end(list);

	if (cvar_int(shell_echo_to_stdout)) printf("%s\n", out);
	Shell_AddText(out);
}


void InitShell()
{
	for (int i=0; i<32; i++) arg_pass[i] = arg[i];

	shell_numlines = 0;
	shell_numhistory = 0;
	memset(shell_text, 0, SHELL_MAXLINES*sizeof(char*));
	memset(shell_hist, 0, SHELL_MAXLINES*sizeof(char*));
	
	// register our cvars
	register_cvar(&shell_echo_to_stdout, "shell_echo_to_stdout", "1");
	register_cvar(&shell_auto_complete_threshold, "shell_auto_complete_threshold","10");
	register_ccmd(&cmdlist, "cmdlist", ccmd_cmdlist);
	register_ccmd(&cvarlist, "cvarlist", ccmd_cvarlist);
}
