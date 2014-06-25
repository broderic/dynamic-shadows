
extern int tlen;
extern char token[256];
extern int plen, poffset;

void SkipWhite(int);
void GetToken(int);
void SetupParse(char *data, int length);
