#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <GL/glew.h>
#include <GL/glut.h>

typedef unsigned char ubyte;
typedef float vec2_t[2];
typedef float vec3_t[3];
typedef float vec4_t[4];

#include "myvect.h"
#include "parse.h"
#include "jpeg.h"
#include "camera.h"
#include "texture.h"
#include "shell.h"
#include "console.h"
#include "shader.h"
#include "bsp.h"
#include "entity.h"
#include "input.h"
#include "main.h"
#include "face.h"
#include "light.h"
#include "md3.h"
#include "sky.h"
#include "render.h"

