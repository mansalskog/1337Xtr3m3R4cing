# set this variable to the director in which you saved the common files
commondir = ../common/
osname = Mac
linuxflags = -lXt -lX11 -lGL -lm
macflags = -framework Foundation -lobjc -framework OpenGl -framework GLUT -lm -framework CoreFoundation -framework ApplicationServices -framework IOKit -framework CoreServices -framework Cocoa -framework Carbon 

all : lab1-1

lab1-1 : lab1-1.c $(commondir)GL_utilities.c $(commondir)VectorUtils3.c $(commondir)loadobj.c $(commondir)LoadTGA.c $(commondir)$(osname)/MicroGlut.c
	gcc -Wall -o lab1-1 -I$(commondir) -I../common/$(osname) -DGL_GLEXT_PROTOTYPES lab1-1.c $(commondir)GL_utilities.c $(commondir)loadobj.c $(commondir)VectorUtils3.c $(commondir)LoadTGA.c $(commondir)$(osname)/MicroGlut.m $(macflags)

clean :
	rm lab1-1

