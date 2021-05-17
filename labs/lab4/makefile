# set this variable to the director in which you saved the common files
commondir = ../new-common/
osname := $(shell uname)
flags = -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-parameter -g
ifeq ($(osname),Darwin)
	osname = Mac
	linkflags = -framework Foundation -lobjc -framework OpenGl -framework GLUT -lm -framework CoreFoundation -framework ApplicationServices -framework IOKit -framework CoreServices -framework Cocoa -framework Carbon
	sources := $(commondir)GL_utilities.c $(commondir)VectorUtils3.c $(commondir)LittleOBJLoader.c $(commondir)LoadTGA.c $(commondir)$(osname)/MicroGlut.m
	headers := $(commondir)GL_utilities.h $(commondir)VectorUtils3.h $(commondir)LittleOBJLoader.h $(commondir)LoadTGA.h $(commondir)$(osname)/MicroGlut.h
else
	osname = Linux
	linkflags = -lXt -lX11 -lGL -lm
	sources := $(commondir)GL_utilities.c $(commondir)VectorUtils3.c $(commondir)LittleOBJLoader.c $(commondir)LoadTGA.c $(commondir)$(osname)/MicroGlut.c
	headers := $(commondir)GL_utilities.h $(commondir)VectorUtils3.h $(commondir)LittleOBJLoader.h $(commondir)LoadTGA.h $(commondir)$(osname)/MicroGlut.h
endif
binaries = lab4-1 lab4-2 lab4-3 lab4-4 lab4-5 lab4-6

all: $(binaries)

%: %.c $(sources) $(headers)
	gcc $(flags) -o $@ -I$(commondir) -I$(commondir)$(osname) -DGL_GLEXT_PROTOTYPES $(sources) $< $(linkflags)

clean:
	rm $(binaries)
