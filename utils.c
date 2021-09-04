#include "id3.h"
#include <stdio.h>
#include <stdio.h>
#include <math.h>
#include "utils.h"
#include "config.h"


int GetCharacterWidth(HDC hdc, char c){
    SIZE size;
    GetTextExtentPoint32(hdc, &c, 1, &size);
    return size.cx;
}

int GetCharacterHeight(HDC hdc, char c){
    SIZE size;
    GetTextExtentPoint32(hdc, &c, 1, &size);
    return size.cy;
}

int GetStringWidth(HDC hdc, char *str){
    SIZE size;
    GetTextExtentPoint32(hdc, str, strlen(str), &size);
    return size.cx;
}

void GetFileExt(char *path, char *out, int maxLen){
    int k, j = 0;
    for(k = strlen(path); k > 0 && path[k] != '.'; k--){}
    if(strlen(path) - k > maxLen) return;
    for(; k < strlen(path); k++) out[j++] = path[k];
    out[j] = '\0';
}

void GetNextLineInFile(FILE *fp, char *out, int max){

    int k = 0;
    while((out[k++] = fgetc(fp)) != '\n' && !feof(fp) && k < max){}

    if(k-1 >= max && !feof(fp) && out[k-1] != '\n')
        while(fgetc(fp) != '\n' && !feof(fp)){}

    out[k-1] = 0;
}

int CheckForMatchingLineGetLocation(FILE *fp, char *name, int *location){
    rewind(fp);
    while(!feof(fp)){

        int sPos = ftell(fp);

        char cmpName[MAX_PATH] = {0};
        GetNextLineInFile(fp, cmpName, MAX_PATH);

        if(strcmp(cmpName, name) == 0){
            *location = sPos;
            return 1;
        }
    }

    return 0;
}

int CheckForMatchingLine(FILE *fp, char *name){
    int location;
    return CheckForMatchingLineGetLocation(fp, name, &location);
}

void RemoveLineFromFile(char *filePath, char *search){

    FILE *fp = fopen(filePath, "rb");
    if(!fp) return;

    int location = 0;
    if(!CheckForMatchingLineGetLocation(fp, search, &location)){
        fclose(fp);
        return;
    }

    fclose(fp);
    fp = fopen(filePath, "rb");

    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    rewind(fp);
    char buffer[len];
    memset(buffer, 0, sizeof(buffer));
    fread(buffer, sizeof(char), len, fp);
    fclose(fp);

    fp = fopen(filePath, "wb");
    if(!fp) return;

    int k, lineLength = 0;
    for(k = location; k < len && buffer[k] != '\n'; k++){ lineLength++; }

    memmove(&buffer[location-1], &buffer[k], len-k+1);
    fwrite(buffer, sizeof(char), len-lineLength-1,  fp);
    fclose(fp);
}

void AddLineToFile(char *path, char *line){
    FILE *aFp = fopen(path, "ab");
    if(!aFp) aFp = fopen(path, "w");
    if(!aFp) return;

    fprintf(aFp, "%s\n", line);
    fclose(aFp);
}

void AddLineToFileNum(char *path, char *line, int num){
    FILE *aFp = fopen(path, "ab");
    if(!aFp) aFp = fopen(path, "w");
    if(!aFp) return;

    int k;
    for(k = 0; k < num; k++) fputc(line[k], aFp);
    fclose(aFp);
}


char *Utils_ScaleImage(SongImage *si, int drawWidth, int drawHeight){

    float xPlus = (float)si->width / (float)drawWidth;
    float yPlus = (float)si->height / (float)drawHeight;

    char *pixels = (char*)malloc(sizeof(char) * drawWidth * drawHeight * 4);

    int pixelIndex = 0;

    float x, y;
    for(y = 0; y < si->height; y+=yPlus){
        for(x = 0; x < si->width; x+=xPlus){

            if(round(x/xPlus) >= drawWidth) continue;

            int yround = round(y)*si->width;
            int xround = round(x);
            if(xround > si->width) break;

            char r = si->pixels[((yround + xround)*si->channels)  ] & 0xFF;
            char g = si->pixels[((yround + xround)*si->channels)+1] & 0xFF;
            char b = si->pixels[((yround + xround)*si->channels)+2] & 0xFF;

            if(pixelIndex+4 > si->width/xPlus * si->height/yPlus * 4) return pixels;
            pixels[pixelIndex++] = b;
            pixels[pixelIndex++] = g;
            pixels[pixelIndex++] = r;
            pixels[pixelIndex++] = 0xff;
        }
    }

    return pixels;
}

void Utils_DrawTitleBar(RECT winRect, HDC hdc, const char *format, ...){

    int len = 1024;
    char buffer[len];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, len, format, args);
    va_end(args);

    char strs[3][1024];
    int separators = 0;

    int lastSeparatorPos = 0;

    int k;
    for(k = 0; k < len; k++){
        if(buffer[k] == (char)FORMAT_STR_SEPERATOR || k == strlen(buffer)) {

            int nextSeparatorPos = k;

            int strLength = nextSeparatorPos - lastSeparatorPos;

            memcpy(strs[separators++], &buffer[lastSeparatorPos], strLength);
            strs[separators-1][strLength] = '\0';

            lastSeparatorPos = k+1;
            if(separators == 3) break;
        }
    }

    HBRUSH bgBrush = CreateSolidBrush((COLORREF){Config_GetColor(TITLEBAR_BG_COLOR)});
    RECT r = (RECT){winRect.left, winRect.top, winRect.right, winRect.top + Config_GetLineHeight()};
    FillRect(hdc, &r, bgBrush);
    SetTextColor(hdc, (COLORREF){Config_GetColor(TITLEBAR_FG_COLOR)});
    SetBkColor(hdc, (COLORREF){Config_GetColor(TITLEBAR_BG_COLOR)});

    TextOut(hdc, 10,                                                    r.top + Config_GetLinePadding(), strs[0], strlen(strs[0]));
    TextOut(hdc, (winRect.right/2) - (GetStringWidth(hdc, strs[1])/2),  r.top + Config_GetLinePadding(), strs[1], strlen(strs[1]));
    TextOut(hdc, winRect.right - GetStringWidth(hdc, strs[2]) - 10,     r.top + Config_GetLinePadding(), strs[2], strlen(strs[2]));

    DeleteObject(bgBrush);
}

void Utils_LineSelector_Paint(LineSelector ls, HDC hdc, void *f, int nl, int x, int y, int width){

    void (*func)(int , RECT ) = f;

    int k;
    for(k = ls.scroll; k < nl; k++){

        int lineY = y + ((k-ls.scroll) * (Config_GetFontSize() + (Config_GetLinePadding()*2)));

        RECT r = {x, lineY, x+width, lineY + Config_GetFontSize()+(Config_GetLinePadding()*2)};

        func(k, r);
    }
}

void Utils_LineSelector_DownLine(LineSelector *l, int numLines, int numLinesScroll){
    if(l->onLine+1 < numLines)
        l->onLine++;

    if(l->onLine - l->scroll > numLinesScroll)
        l->scroll++;
}

void Utils_LineSelector_UpLine(LineSelector *l){
    if(l->onLine-1 >= 0)
        l->onLine--;

    if(l->onLine - l->scroll < 0)
        l->scroll--;
}

void Utils_DrawImage(HDC hdc, int x, int y, int width, int height, int drawWidth, int drawHeight, SongImage *si){
    HBITMAP img;

    char *pixels = Utils_ScaleImage(si, drawWidth, drawHeight);

    img = CreateBitmap(drawWidth, drawHeight, 1, 32, pixels);

    HDC hdcM = CreateCompatibleDC(hdc);
    SelectObject(hdcM, img);
    StretchBlt(hdc, x, y, width, height, hdcM, 0, 0, drawWidth, drawHeight, SRCCOPY);

    DeleteObject(img);
    DeleteDC(hdcM);

    free(pixels);
}