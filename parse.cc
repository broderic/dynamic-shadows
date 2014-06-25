#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parse.h"

int tlen;
char token[256];

char *pdata;
int plen, poffset;


// basically goes until a '\n' is found
void ReadComment()
{
	while (poffset < plen) {
		if (pdata[poffset] == '\n') return;
		poffset++;
	}
}

int my_isspace(int a)
{
	if (a == ' ' || a == '\n' || a  == '\r' || a == '\t') return 1;
	return 0;
}

void SkipWhite(int skipnewline)
{
	while (poffset < plen) {
		if (pdata[poffset] == '/' && pdata[poffset+1] == '/') {
			ReadComment();
			continue;
		} else if (pdata[poffset] == '\n' && !skipnewline) {
			break;
// FIXME: cygwin gives a compile error if we use this
// 		} else if (!isspace(pdata[poffset])) {
		} else if (!my_isspace(pdata[poffset])) {
			break;
		}
		poffset++;
	}
}

// Contiguous strings of characters are tokens
//  ", \n  break tokens
void GetToken(int skipnewline)
{
	char c;
	
	SkipWhite(skipnewline);
	tlen = 0;
	token[0] = 0;
	if (poffset >= plen) return;
	
	c = pdata[poffset];
	if (c == '\n' || c == '{' || c == '}') {
		if (!skipnewline) {
			// FIXME: use a real error!
			printf("Hit newline when we weren't supposed to!\n");
			exit(1);
		}
		token[0] = c;
		token[1] = 0;
		poffset++;
	} else if (c == '\"') {    // read a string
		poffset++;
		for (tlen=0; poffset < plen; tlen++) {
			c = pdata[poffset++];
			if (c == '\n') {
				// FIXME: use a real error function
				printf("GetToken: newline in string!\n");
				exit(1);
			}
			if (c == '\"') break;
			token[tlen] = c;
		}
		token[tlen] = 0;
	} else 	{
		// parse until whitespace or a terminal character
		token[0] = c;
		poffset++;
		for (tlen=1; poffset < plen; tlen++) {
			// stop at a comment
			if (pdata[poffset] == '/' && pdata[poffset+1] == '/') break;
			
			c = pdata[poffset];
			if (c == '\n' || c == '\"' || 
			    // FIXME: cygwin gives a compile error here
			    // isspace(c)
			    c == ' ' || c == '\r' || c == '\t'
			    )
				break;

			token[tlen] = c;
			poffset++;
		}
		token[tlen] = 0;
	}
}

void SetupParse(char *data, int length)
{
	pdata = data;
	plen = length;
	poffset = 0;
}
