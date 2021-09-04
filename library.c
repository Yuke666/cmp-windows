#include <stdio.h>
#include "library.h"
#include "utils.h"
#include "id3.h"

void Library_AddLine(char *filePath){
    SongInfo info = {};
    ID3_Open(filePath, &info, 0);

    if(strlen(info.title) == 0) strcpy(info.title, filePath);
    if(strlen(info.album) == 0) strcpy(info.album, "Unknown Album");
    if(strlen(info.artist) == 0) strcpy(info.artist, "Unknown Artist");

    int len = strlen(filePath) + strlen(info.title) + strlen(info.artist) + strlen(info.album) + 6;
    char buf[len];
    sprintf(buf, "%s%c%s%c%s%c%s%c%i\n", filePath, 0, info.title, 0, info.artist, 0, info.album, 0, info.image.exists);
    AddLineToFileNum(LIBRARY_FILE_PATH, buf, len);
}

int Library_FindLineFromFilename(char *fileName, char *into){
    FILE *fp = fopen(LIBRARY_FILE_PATH, "r");
    if(!fp) return 0;

    while(!feof(fp)){

        GetNextLineInFile(fp, into, sizeof(LibraryFileLine));

        LibraryFileLine line = {};
        Library_ProcessLine(into, &line);

        if(strcmp(line.filePath, fileName) == 0){
            fclose(fp);
            return 1;
        }
    }

    return 0;
}

int Library_CheckForMatchingLineFromFilename(char *fileName){
    char into[sizeof(LibraryFileLine)];
    return Library_FindLineFromFilename(fileName, into);
}

static int GetBetweenNulls(char *str, char *out, int maxLen){
    int k;
    char *c = &out[0];
    for( k = 0; (*c++ = str[k]) != '\0' && k < maxLen; k++){}
    return k+1;
}

int Library_ProcessLine(char *line, LibraryFileLine *l){
    if(strlen(line) == 0) return 0;

    int k = 0;
    k += GetBetweenNulls(&line[k], l->filePath, sizeof(l->filePath));
    k += GetBetweenNulls(&line[k], l->title, sizeof(l->title));
    k += GetBetweenNulls(&line[k], l->artist, sizeof(l->artist));
    k += GetBetweenNulls(&line[k], l->album, sizeof(l->album));
    k += GetBetweenNulls(&line[k], l->imageExistsChar, sizeof(l->imageExistsChar));
    l->imageExists = atoi(l->imageExistsChar);

    return 1;
}

void Library_RemoveLineFromFilename(char *fileName){

    char lineChars[sizeof(LibraryFileLine)];
    memset(lineChars, 0, sizeof(LibraryFileLine));
    if(!Library_FindLineFromFilename(fileName, lineChars)) return;
    RemoveLineFromFile(LIBRARY_FILE_PATH, lineChars);
}