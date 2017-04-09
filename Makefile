TARGET		= circleclock
OBJS_TARGET	= main.o fontstash.o stb_truetype.o tsprintf.o

CFLAGS = -O0 -g -std=gnu99 -fpermissive -ffast-math `sdl-config --cflags` 
LDFLAGS = 
LIBS = `sdl-config --libs` -lc -lm -lGL -lGLU -lglut

include Makefile.in
