#
# FIXME: 06/01/30
# we use -mno-cygwin when compiling, but not when we link... does this matter?
# In fact, I can't get gcc to link a simple hello world program with -mno-cygwin.
# WTF is going on here?
#

CC      = g++

# Linux
#CFLAGS  = -Wall -ansi -O4 
#LIBS    = -lGLEW -lGL -lGLU -lGLw -lglut -ljpeg 

# Cygwin
CFLAGS  = -Wall -O4 -mno-cygwin -DCYGWIN #-H
LIBS    = -lglew32 -lglu32 -lopengl32 -lglu -lglut32 -ljpeg -lm 


LDFLAGS = -Lglew/lib/ -L/usr/X11R6/lib/


# my stuff
HEADERS = shad.h main.h camera.h light.h parse.h bsp.h face.h jpeg.h entity.h shader.h \
	console.h input.h myvect.h md3.h texture.h sky.h render.h shell.h

SRCS = main.cc camera.cc light.cc parse.cc bsp.cc face.cc jpeg.cc entity.cc shader.cc \
	shell.cc console.cc input.cc myvect.cc md3.cc texture.cc sky.cc render.cc

OBJS = main.o camera.o light.o parse.o bsp.o face.o jpeg.o entity.o shader.o \
	shell.o console.o input.o myvect.o md3.o texture.o sky.o render.o

.c.o: $(HEADERS)
	$(CC) $(CFLAGS) -Iglew/include -c $< -o $@ 

.cc.o: $(HEADERS)
	$(CC) $(CFLAGS) -Iglew/include -c $< -o $@

all: sm

sm: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) 

clean:
	rm -f core *.o *~ \#*


