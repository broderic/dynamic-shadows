
#define CONSOLE_HIDDEN   0
#define CONSOLE_LOWERING 1
#define CONSOLE_DOWN     2
#define CONSOLE_RISING   3
#define CONSOLE_FULL     4

extern int console_state;

extern texture_t *font;

extern float font_xstep;
extern float font_ystep;


void UpdateConsole();

void DrawString(float x, float y, char *args, ...);
void DrawConsole();

void ConRegisterChar(char c);
void ConRegisterSpecialChar(char c);

void ConsoleInit();
void ConsoleInitBuffers();
void LoadFont();
