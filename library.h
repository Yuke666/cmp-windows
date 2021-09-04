#ifndef LIBRARY_DEF
#define LIBRARY_DEF

#define LIBRARY_FILE_PATH "music.library"

#include "windows.h"

typedef struct {
    char filePath[MAX_PATH];
    char title[64];
    char artist[64];
    char album[64];
    char imageExistsChar[1];
    int imageExists;
    char seperators[4];
} LibraryFileLine;

void Library_AddLine(char *filePath);
int Library_ProcessLine(char *line, LibraryFileLine *l);
void Library_RemoveLineFromFilename(char *fileName);
int Library_FindLineFromFilename(char *fileName, char *into);
int Library_CheckForMatchingLineFromFilename(char *fileName);

#endif