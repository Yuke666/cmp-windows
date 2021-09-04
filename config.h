#ifndef CONFIG_DEF
#define CONFIG_DEF

enum {
    DEFAULT_FG_COLOR = 0,
    DEFAULT_BG_COLOR,
    SELECTED_FG_COLOR,
    SELECTED_BG_COLOR,
    BROWSERWIN_ADDED_SONG_FG_COLOR,
    BROWSERWIN_ADDED_SONG_BG_COLOR,
    SELECTED_OTHER_TAB_FG_COLOR,
    SELECTED_OTHER_TAB_BG_COLOR,
    TITLEBAR_FG_COLOR,
    TITLEBAR_BG_COLOR,
    SEPARATOR_COLOR,
};

#include "windows.h"

typedef struct {
    float width;
    float height;
} Size;

typedef struct {
    int top;
    int bottom;
    int left;
    int right;
} Padding;

char *Config_GetBrowserPath();
void Config_ReadConfig();
int Config_GetColor(int type);
char *Config_GetFontStr();
HFONT Config_GetFont();
int Config_GetFontSize();
int Config_GetLinePadding();
void Config_CreateFont(HDC hdc);
void Config_DeleteObjects();
int Config_GetWindowWidth();
int Config_GetWindowHeight();
int Config_DoDrawImage();
int Config_GetLibraryLayout();
int Config_DoDrawTitleBar();
int Config_IsTitleBarOnBottom();
int Config_GetLineHeight();
Padding Config_GetImagePadding();
Padding Config_GetLibraryPadding();
Size Config_GetImageSize();
int Config_GetFontWidth();

#endif