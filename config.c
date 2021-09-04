#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

static HFONT    font;
static char     browserPath[MAX_PATH]   = "C:\\";
static char     fontStr[100]            = "Consolas";
static int      fontSize                = 12;
static int      fontWidth               = 12;
static int      linePadding             = 1;
static int      windowWidth             = 500;
static int      windowHeight            = 300;
static int      drawImage               = 1;
static int      drawTitleBar            = 1;
static int      titleBarOnBottom        = 0;
static Size     imageSize               = {0.5, 1};
static int      libraryLayout           = 0;
static Padding  imagePadding            = {0, 0, 0, 0};
static Padding  libraryPadding          = {0, 0, 0, 0};

static int colors[SEPARATOR_COLOR+1] = {
    0xffffff,
    0x1b1b1b,
    0xffffff,
    0x31658C,
    0xB0EAD9,
    0x1b1b1b,
    0x1b1b1b,
    0xC8CACC,
    0xffffff,
    0x31658C,
    0x31658C,
};

int Config_GetColor(int index){
    return ((colors[index] & 0xFF0000) >> 16) |(colors[index] & 0x00FF00) |
           ((colors[index] & 0x0000FF) << 16);
}

char *Config_GetFontStr(){
    return fontStr;
}

HFONT Config_GetFont(){
    return font;
}

int Config_GetFontSize(){
    return fontSize;
}

int Config_GetWindowWidth(){
    return windowWidth;
}

int Config_GetWindowHeight(){
    return windowHeight;
}

int Config_GetLibraryLayout(){
    return libraryLayout;
}

int Config_DoDrawImage(){
    return drawImage;
}

int Config_DoDrawTitleBar(){
    return drawTitleBar;
}

int Config_IsTitleBarOnBottom(){
    return titleBarOnBottom;
}

char *Config_GetBrowserPath(){
    if(browserPath[strlen(browserPath)-1] != '\\')
        strcat(browserPath, "\\");

    return browserPath;
}

int Config_GetLinePadding(){
    return linePadding;
}

int Config_GetLineHeight(){
    return Config_GetFontSize()+(Config_GetLinePadding()*2);
}

Padding Config_GetImagePadding(){
    return imagePadding;
}

Padding Config_GetLibraryPadding(){
    return libraryPadding;
}

Size Config_GetImageSize(){
    return imageSize;
}

int Config_GetFontWidth(){
    return fontWidth;
}

static void InitFont(){
    font = CreateFont(fontSize, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT(Config_GetFontStr()));

    HDC hdc = GetDC(NULL);
    SelectObject(hdc, font);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);

    ReleaseDC(NULL, hdc);

    fontSize = tm.tmHeight;
    fontWidth = tm.tmMaxCharWidth;
}

void Config_ReadConfig(){
    FILE *fp = fopen("config.cfg","r");
    if(!fp) return;

    while(!feof(fp)){

        char lineType[100];
        fscanf(fp, "%s : ", lineType);

        if(lineType[0] == '#'){
            while(!feof(fp) && fgetc(fp) != '\n'){}
            continue;
        }

        if(strcmp(lineType, "TitleBarOnBottom") == 0)
            fscanf(fp, "%i", &titleBarOnBottom);

        if(strcmp(lineType, "FontSize") == 0)
            fscanf(fp, "%i", &fontSize);

        if(strcmp(lineType, "BrowserPath") == 0)
            fscanf(fp, "%s", browserPath);

        if(strcmp(lineType, "LinePadding") == 0)
            fscanf(fp, "%i", &linePadding);

        if(strcmp(lineType, "FontFace") == 0)
            fscanf(fp, "%s", fontStr);

        if(strcmp(lineType, "WindowWidth") == 0)
            fscanf(fp, "%i", &windowWidth);

        if(strcmp(lineType, "DrawImage") == 0)
            fscanf(fp, "%i", &drawImage);

        if(strcmp(lineType, "DrawTitleBar") == 0)
            fscanf(fp, "%i", &drawTitleBar);

        if(strcmp(lineType, "LibraryLayout") == 0)
            fscanf(fp, "%i", &libraryLayout);

        if(strcmp(lineType, "WindowHeight") == 0)
            fscanf(fp, "%i", &windowHeight);

        if(strcmp(lineType, "ImageSize") == 0)
            fscanf(fp, "%f %f", &imageSize.width, &imageSize.height);

        if(strcmp(lineType, "ImagePadding") == 0)
            fscanf(fp, "%i %i %i %i", &imagePadding.left, &imagePadding.right, &imagePadding.top, &imagePadding.bottom);

        if(strcmp(lineType, "LibraryPadding") == 0)
            fscanf(fp, "%i %i %i %i", &libraryPadding.left, &libraryPadding.right, &libraryPadding.top, &libraryPadding.bottom);

        if(strcmp(lineType, "DEFAULT_FG_COLOR") == 0)
            fscanf(fp, "%x", &colors[DEFAULT_FG_COLOR]);

        if(strcmp(lineType, "DEFAULT_BG_COLOR") == 0)
            fscanf(fp, "%x", &colors[DEFAULT_BG_COLOR]);

        if(strcmp(lineType, "SELECTED_BG_COLOR") == 0)
            fscanf(fp, "%x", &colors[SELECTED_BG_COLOR]);

        if(strcmp(lineType, "SELECTED_FG_COLOR") == 0)
            fscanf(fp, "%x", &colors[SELECTED_FG_COLOR]);

        if(strcmp(lineType, "BROWSERWIN_ADDED_SONG_FG_COLOR") == 0)
            fscanf(fp, "%x", &colors[BROWSERWIN_ADDED_SONG_FG_COLOR]);

        if(strcmp(lineType, "BROWSERWIN_ADDED_SONG_BG_COLOR") == 0)
            fscanf(fp, "%x", &colors[BROWSERWIN_ADDED_SONG_BG_COLOR]);

        if(strcmp(lineType, "SELECTED_OTHER_TAB_BG_COLOR") == 0)
            fscanf(fp, "%x", &colors[SELECTED_OTHER_TAB_BG_COLOR]);

        if(strcmp(lineType, "SELECTED_OTHER_TAB_FG_COLOR") == 0)
            fscanf(fp, "%x", &colors[SELECTED_OTHER_TAB_FG_COLOR]);

        if(strcmp(lineType, "TITLEBAR_FG_COLOR") == 0)
            fscanf(fp, "%x", &colors[TITLEBAR_FG_COLOR]);

        if(strcmp(lineType, "TITLEBAR_BG_COLOR") == 0)
            fscanf(fp, "%x", &colors[TITLEBAR_BG_COLOR]);

        if(strcmp(lineType, "SEPARATOR_COLOR") == 0)
            fscanf(fp, "%x", &colors[SEPARATOR_COLOR]);

        while(fgetc(fp) != '\n' && !feof(fp)){}
    }

    fclose(fp);

    InitFont();
}

void Config_DeleteObjects(){
    DeleteObject(font);
}