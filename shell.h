typedef struct cvar_s {
	char name[32];
	char value[32];
} cvar_t;

typedef struct ccmd_s {
	char name[32];
	void (*func)(int, char**);
} ccmd_t;

void Shell_AddHistory(char *s);
char *Shell_GetText(int n);
char *Shell_GetHistory(int n);
int Shell_AutoComplete(char *str, char *out);

void InitShell();
void conprintf(char *, ...);

void ProcessInput(char *);

void register_cvar(cvar_t **, char *, char *);
void register_ccmd(ccmd_t **cm, char *name, void (*func)(int,char**));
cvar_t *cvar_find(char *);
ccmd_t *ccmd_find(char *name);

void cvar_set(cvar_t *, char *);
int cvar_int(cvar_t *);
float cvar_float(cvar_t *);
double cvar_double(cvar_t *);
void cvar_vector3(cvar_t *cv, float *f);
