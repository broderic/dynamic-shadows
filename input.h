

extern cvar_t *p_interval;

void HandleInput();

void glutKeyboardUpCallback(unsigned char key, int x, int y);
void Keyboard(unsigned char key, int x, int y);
void glutSpecialUpCallback(int key, int x, int y);
void SpecialKeys(int key, int x, int y);
void handleMouseMotion (int x, int y);
void handleMouseClick(int btn, int state, int x, int y); 

void InputInit();
