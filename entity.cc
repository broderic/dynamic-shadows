// parses the quake3 entity lump

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "parse.h"

#define MAX_FIELDS     16
#define MAX_ENTITIES   16536

typedef struct {
	int id;
	int nfields;
	char *field[MAX_FIELDS][2];
} entity_t;

int num_entities;
entity_t entity[MAX_ENTITIES];



int ParseField(entity_t *cur)
{
	int len;

	GetToken(1);
	if (!strcmp(token, "}")) return 0;
	len = strlen(token);
	cur->field[cur->nfields][0] = (char*)malloc(len+1);
	strcpy(cur->field[cur->nfields][0], token);

	GetToken(1);
	len = strlen(token);
	cur->field[cur->nfields][1] = (char*)malloc(len+1);
	strcpy(cur->field[cur->nfields][1], token);

	//printf("%s -> %s\n", cur->field[cur->nfields][0],
	//       cur->field[cur->nfields][1]);

	cur->nfields++;
	return 1;
}

void ParseEntity()
{
	entity_t *cur = &entity[num_entities];
	cur->id = num_entities++;
	cur->nfields = 0;

	// throw crap out until we hit a { brace
	while (poffset < plen) {
		GetToken(1);
		if (!strcmp(token, "{")) break;
	}
	if (poffset >= plen) return;

	// read fields until a }
	while (poffset < plen) {
		if (!ParseField(cur)) break;
	}
}

void ParseEntities(char *data, int length)
{
	SetupParse(data, length);
	
	num_entities = 0;
	while (poffset < plen) {
		ParseEntity();
	}
	printf("num_entities: %d\n", num_entities);
}


/*****************************************************************************/

int FloatFromField(char *str, float *out)
{
	return (sscanf(str, "%f", out)==1);
}

int Vec3FromField(char *str, float *out)
{
	return (sscanf(str, "%f %f %f ", &out[0], &out[1], &out[2]) == 3);
}

char *EntityField(entity_t *cur, char *f1)
{
	for (int i=0; i<cur->nfields; i++) {
		if (!strcmp(cur->field[i][0], f1)) 
			return cur->field[i][1];
	}
	return NULL;
}

entity_t *FindEntityClass(int start, char *classname)
{
	char *cl;
	entity_t *cur = entity;
	if (start >= num_entities) return NULL;
	
	for (int i=start; i<num_entities; i++, cur++) {
		cl = EntityField(cur, "classname");
		if (cl && !strcmp(cl, classname)) return cur;
	}
	return NULL;
}


// FIXME: HACK Alert!
int FindASpawnPoint(float *out, float *yaw)
{
	char *str;
	entity_t *spawn = FindEntityClass(0, "info_player_deathmatch");
	if (spawn == NULL) return 0;  // Uh oh!

	str = EntityField(spawn, "origin");
	if (!str) return 0;
	if (!Vec3FromField(str, out)) return 0;

	str = EntityField(spawn, "angle");
	if (!str) return 0;
	if (!FloatFromField(str, yaw)) return 0;
	*yaw = 360 - *yaw;
	//printf("yaw = %f\n", *yaw);
		
	return 1;
}
