set COMD=common\
set INCL=/I %COMD%Windows /I %COMD%
set SRCS=game.c %COMD%GL_utilities.c %COMD%VectorUtils3.c %COMD%LittleOBJLoader.c %COMD%LoadTGA.c %COMD%Windows\MicroGlut.c %COMD%Windows\glew.c
set LIBS=vcruntime.lib libucrt.lib kernel32.lib user32.lib opengl32.lib gdi32.lib 
cl %INCL% %SRCS% /link /subsystem:CONSOLE %LIBS% /debug:FULL /out:game.exe