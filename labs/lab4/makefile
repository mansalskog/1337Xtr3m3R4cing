# set this variable to the director in which you saved the common files
commondir = ../common/
osname := $(shell uname)
warnflags = -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-parameter
ifeq ($(osname),Darwin)
	osname = Mac
	linkflags = -framework Foundation -lobjc -framework OpenGl -framework GLUT -lm -framework CoreFoundation -framework ApplicationServices -framework IOKit -framework CoreServices -framework Cocoa -framework Carbon
	included := $(commondir)GL_utilities.c $(commondir)VectorUtils3.c $(commondir)loadobj.c $(commondir)LoadTGA.c $(commondir)$(osname)/MicroGlut.m
else
	linkflags = -lXt -lX11 -lGL -lm
	included := $(commondir)GL_utilities.c $(commondir)VectorUtils3.c $(commondir)loadobj.c $(commondir)LoadTGA.c $(commondir)$(osname)/MicroGlut.c
endif

all: lab4-1

%: %.c $(included)
	gcc $(warnflags) -o $@ -I$(commondir) -I../common/$(osname) -DGL_GLEXT_PROTOTYPES $^ $(linkflags)

clean:
	rm lab4-1
