# set this variable to the director in which you saved the common files
commondir = ../common/
osname := $(shell uname)
ifeq ($(osname),Darwin)
	osname = Mac
	flags = -framework Foundation -lobjc -framework OpenGl -framework GLUT -lm -framework CoreFoundation -framework ApplicationServices -framework IOKit -framework CoreServices -framework Cocoa -framework Carbon
	included := $(commondir)GL_utilities.c $(commondir)VectorUtils3.c $(commondir)loadobj.c $(commondir)LoadTGA.c $(commondir)$(osname)/MicroGlut.m
else
	flags = -lXt -lX11 -lGL -lm
	included := $(commondir)GL_utilities.c $(commondir)VectorUtils3.c $(commondir)loadobj.c $(commondir)LoadTGA.c $(commondir)$(osname)/MicroGlut.c
endif

all : lab1-1

lab1-1 : lab1-1.c $(included)
	gcc -Wall -o lab1-1 -I$(commondir) -I../common/$(osname) -DGL_GLEXT_PROTOTYPES lab1-1.c $(included) $(flags)

clean :
	rm lab1-1

