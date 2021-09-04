#ifndef UTILS_DEF
#define UTILS_DEF
#include "windows.h"
#include "id3.h"
#include <stdio.h>

#define FORMAT_STR_SEPERATOR 255

typedef struct {
    int scroll;
    int onLine;
} LineSelector;

int GetCharacterWidth(HDC hdc, char c);
int GetCharacterHeight(HDC hdc, char c);
int GetStringWidth(HDC hdc, char *str);
void GetFileExt(char *path, char *out, int maxLen);
int CheckForMatchingLineGetLocation(FILE *fp, char *name, int *location);
int CheckForMatchingLine(FILE *fp, char *name);
void RemoveLineFromFile(char *filePath, char *search);
void AddLineToFile(char *path, char *line);
void AddLineToFileNum(char *path, char *line, int num);
void GetNextLineInFile(FILE *fp, char *out, int max);
char *Utils_ScaleImage(SongImage *si, int drawWidth, int drawHeight);
void Utils_LineSelector_Paint(LineSelector ls, HDC hdc, void *f, int nl, int x, int y, int width);
void Utils_LineSelector_DownLine(LineSelector *l, int numLines, int numLinesScroll);
void Utils_LineSelector_UpLine(LineSelector *l);
void Utils_DrawTitleBar(RECT winRect, HDC hdc, const char *format, ...);
void Utils_DrawImage(HDC hdc, int x, int y, int width, int height, int drawWidth, int drawHeight, SongImage *si);
#endif