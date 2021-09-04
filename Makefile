CC=gcc


CFLAGS=-g -Wall -mwindows
PNGLIBS = $(shell pkg-config --static --libs libpng)
LDLIBS = --static -lm -lcomctl32 -mwindows -lwinmm -lmad $(PNGLIBS) -ljpeg

SOURCES=main.c player.c utils.c browser_win.c visualizer_win.c config.c id3.c library.c library_win.c image_loader.c mp3.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=cmp.exe

all: createResourcesO $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) resources.o
	$(CC) $(OBJECTS) resources.o $(LDLIBS) -o $@

createResourcesO: resources.rc
	windres resources.rc -o resources.o

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@