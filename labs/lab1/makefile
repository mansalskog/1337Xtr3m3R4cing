# set this variable to the director in which you saved the common files
commondir = ../common/
osname := $(shell uname)
warnflags = -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-parameter
ifeq ($(osname),Darwin)
	osname = Mac
	linkflags = -framework Foundation -lobjc -framework OpenGl -framework GLUT -lm -framework CoreFoundation -framework ApplicationServices -framework IOKit -framework CoreServices -framework Cocoa -framework Carbon
	sources := $(commondir)GL_utilities.c $(commondir)VectorUtils3.c $(commondir)loadobj.c $(commondir)LoadTGA.c $(commondir)$(osname)/MicroGlut.m
	headers := $(commondir)GL_utilities.h $(commondir)VectorUtils3.h $(commondir)loadobj.h $(commondir)LoadTGA.h $(commondir)$(osname)/MicroGlut.h
else
	osname = Linux
	linkflags = -lXt -lX11 -lGL -lm
	sources := $(commondir)GL_utilities.c $(commondir)VectorUtils3.c $(commondir)loadobj.c $(commondir)LoadTGA.c $(commondir)$(osname)/MicroGlut.c
	headers := $(commondir)GL_utilities.h $(commondir)VectorUtils3.h $(commondir)loadobj.h $(commondir)LoadTGA.h $(commondir)$(osname)/MicroGlut.h
endif

all: lab1-1 lab1-2 lab1-3 lab1-4 lab1-5 lab1-6

%: %.c $(sources) $(headers)
	gcc $(warnflags) -o $@ -I$(commondir) -I$(commondir)$(osname) -DGL_GLEXT_PROTOTYPES $(sources) $< $(linkflags)

clean:
	rm lab1-1 lab1-2 lab1-3 lab1-4 lab1-5 lab1-6
