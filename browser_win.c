#include "windows.h"
#include "utils.h"
#include "browser_win.h"
#include "config.h"
#include "id3.h"
#include "library.h"
#include <stdio.h>
#include <math.h>

static int numOfLines = 0;
static char inDir[MAX_PATH];
static LineSelector lineSelector;

enum {
    BROWSER_TYPE_SONG,
    BROWSER_TYPE_DIR,
};

static struct Line {
    int type;
    char fileName[MAX_PATH];
    char filePath[MAX_PATH];
    int added;
    SongInfo info;
} *lines = NULL;

void BrowserWin_Init(){
    strcpy(inDir, Config_GetBrowserPath());
}

void BrowserWin_UpLine(){
    Utils_LineSelector_UpLine(&lineSelector);
}

void BrowserWin_DownLine(HWND hwnd){
    RECT cliRect;
    GetClientRect(hwnd, &cliRect);
    cliRect.top += Config_IsTitleBarOnBottom() ? 0 : Config_GetLineHeight();
    cliRect.bottom -= !Config_IsTitleBarOnBottom() ? 0 : Config_GetLineHeight();
    Utils_LineSelector_DownLine(&lineSelector, numOfLines, (cliRect.bottom - cliRect.top) / Config_GetLineHeight());
}

void BrowserWin_OnEnter(){
    if(lines[lineSelector.onLine].type == BROWSER_TYPE_DIR){
        strcpy(inDir, lines[lineSelector.onLine].filePath);
        strcat(inDir, "\\");
        lineSelector.onLine = 0;
        BrowserWin_Refresh();
    }
}

static void GetLinesInDirectory(struct Line **dLines, char *dirPath, int *numLines){

    *numLines = 0;

    struct Line *moreLines = NULL;

    char dir[MAX_PATH];
    strcpy(dir, dirPath);
    strcat(dir, "*");

    WIN32_FIND_DATA ffd;
    HANDLE find = FindFirstFile(dir, &ffd);

    if(INVALID_HANDLE_VALUE == find)
        return;

    FILE *fp = fopen(LIBRARY_FILE_PATH, "r");

    do {

        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) continue;

        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){

            if(strcmp(ffd.cFileName, ".") == 0) continue;

            moreLines = (struct Line *)realloc(moreLines, ++*numLines*sizeof(struct Line));

            moreLines[*numLines-1] = (struct Line){};

            char dir[MAX_PATH] = {0};

            if(strcmp(ffd.cFileName, "..") == 0){

                int k;
                for(k = strlen(inDir)-2; k > 0; k--)
                    if(inDir[k] == '/' || inDir[k] == '\\')
                        break;

                memcpy(dir, inDir, k);

            } else {
                strcpy(dir, inDir);
                strcat(dir, ffd.cFileName);
            }

            strcpy(moreLines[*numLines-1].filePath, dir);
            strcpy(moreLines[*numLines-1].fileName, dir);
            moreLines[*numLines-1].type = BROWSER_TYPE_DIR;
        }

        else {
            char ext[512] = {0};
            GetFileExt(ffd.cFileName, ext, 4);
            if(strcmp(ext, ".mp3") == 0){
                moreLines = (struct Line *)realloc(moreLines, ++*numLines*sizeof(struct Line));
                moreLines[*numLines-1] = (struct Line){};
                strcpy(moreLines[*numLines-1].filePath, dirPath);
                strcat(moreLines[*numLines-1].filePath, ffd.cFileName);
                strcpy(moreLines[*numLines-1].fileName, ffd.cFileName);
                moreLines[*numLines-1].type = BROWSER_TYPE_SONG;

                if(fp){
                    if(Library_CheckForMatchingLineFromFilename(moreLines[*numLines-1].filePath))
                        moreLines[*numLines-1].added = 1;
                }
            }
        }

    } while(FindNextFile(find , &ffd));

    *dLines = moreLines;
    FindClose(find);
    if(fp) fclose(fp);
}


void BrowserWin_AddSelectedToLib(HWND hwnd){

    int addLine = lineSelector.onLine;
    BrowserWin_DownLine(hwnd);

    if(lines[addLine].type == BROWSER_TYPE_SONG && lines[addLine].added)
        return;

    if(lines[addLine].type == BROWSER_TYPE_SONG){
        if(!Library_CheckForMatchingLineFromFilename(lines[addLine].filePath)){
            Library_AddLine(lines[addLine].filePath);
            lines[addLine].added = 1;
        }
    }

    if(lines[addLine].type == BROWSER_TYPE_DIR){
        struct Line *moreLines = NULL;
        int numLines = 0;

        char dir[MAX_PATH];
        strcpy(dir, lines[addLine].filePath);
        strcat(dir, "\\");

        GetLinesInDirectory(&moreLines, dir, &numLines);

        int k;
        for(k = 0; k < numLines; k++){
            if(moreLines[k].type == BROWSER_TYPE_SONG){
                if(!Library_CheckForMatchingLineFromFilename(moreLines[k].filePath)){
                    Library_AddLine( moreLines[k].filePath);
                }
            }
        }

        free(moreLines);
    }
}

void BrowserWin_RemoveSelectedFromLib(HWND hwnd){

    int addLine = lineSelector.onLine;
    BrowserWin_DownLine(hwnd);

    if(lines[addLine].type == BROWSER_TYPE_SONG && !lines[addLine].added) return;

    if(lines[addLine].type == BROWSER_TYPE_SONG){
        Library_RemoveLineFromFilename(lines[addLine].filePath);
        BrowserWin_Refresh();
    }

    if(lines[addLine].type == BROWSER_TYPE_DIR){
        struct Line *moreLines = NULL;
        int numLines = 0;

        char dir[MAX_PATH];
        strcpy(dir, lines[addLine].filePath);
        strcat(dir, "\\");

        GetLinesInDirectory(&moreLines, dir, &numLines);

        int k;
        for(k = 0; k < numLines; k++){
            if(moreLines[k].type == BROWSER_TYPE_SONG){
                Library_RemoveLineFromFilename(moreLines[k].filePath);
            }
        }

        free(moreLines);
        BrowserWin_Refresh();
    }
}

void BrowserWin_Refresh(){
    if(lines) free(lines);
    lines = NULL;
    numOfLines = 0;
    GetLinesInDirectory(&lines, inDir, &numOfLines);
}

void BrowserWin_Paint(HWND hwnd){

    RECT cliRect;
    GetClientRect(hwnd, &cliRect);

    PAINTSTRUCT ps;

    HDC hdc = BeginPaint(hwnd, &ps);
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(ps.hdc,cliRect.right - cliRect.left, cliRect.bottom-cliRect.top );
    HBITMAP hbmOld = SelectObject(hdcMem, hbmMem);

    HBRUSH selectedLineBrush = CreateSolidBrush((COLORREF){Config_GetColor(SELECTED_BG_COLOR)});
    HBRUSH winBgColorBrush = CreateSolidBrush((COLORREF){Config_GetColor(DEFAULT_BG_COLOR)});

    SetTextColor(hdcMem, (COLORREF){Config_GetColor(DEFAULT_FG_COLOR)});
    SetBkColor(hdcMem, (COLORREF){Config_GetColor(DEFAULT_BG_COLOR)});

    HFONT font = Config_GetFont();
    SelectObject(hdcMem, font);

    FillRect(hdcMem, &cliRect, winBgColorBrush);


    void getTextSetColors(int line, RECT r){

        HBRUSH brush;
        if(line == lineSelector.onLine){
            brush = selectedLineBrush;
            SetTextColor(hdcMem, (COLORREF){Config_GetColor(SELECTED_FG_COLOR)});
            SetBkColor(hdcMem, (COLORREF){Config_GetColor(SELECTED_BG_COLOR)});
        } else {
            brush = winBgColorBrush;
            if(lines[line].added){
                SetBkColor(hdcMem, (COLORREF){Config_GetColor(BROWSERWIN_ADDED_SONG_BG_COLOR)});
                SetTextColor(hdcMem, (COLORREF){Config_GetColor(BROWSERWIN_ADDED_SONG_FG_COLOR)});
            } else {
                SetBkColor(hdcMem, (COLORREF){Config_GetColor(DEFAULT_BG_COLOR)});
                SetTextColor(hdcMem, (COLORREF){Config_GetColor(DEFAULT_FG_COLOR)});
            }
        }

        FillRect(hdcMem, &r, brush);
        TextOut(hdcMem, r.left+10, r.top + Config_GetLinePadding(), lines[line].fileName, strlen(lines[line].fileName));
    }

    int titleBarBottom = Config_DoDrawTitleBar() && !Config_IsTitleBarOnBottom() ? Config_GetLineHeight() : 0;
    Utils_LineSelector_Paint(lineSelector, hdcMem, getTextSetColors, numOfLines, 0, titleBarBottom, cliRect.right);

    RECT r = cliRect;

    if(Config_IsTitleBarOnBottom())
        r.top = r.bottom-Config_GetLineHeight();


    if(Config_DoDrawTitleBar()){
        DWORD vol = 0;
        waveOutGetVolume(NULL, &vol);
        Utils_DrawTitleBar(r, hdcMem, "%s%c%s%c%s%i%c", "CMP", FORMAT_STR_SEPERATOR, "Browser window", FORMAT_STR_SEPERATOR,
            "Volume ", (int)round(((float)(vol & 0xFFFF) / (float)0xFFFF) * 100.0), '%');
    }

    DeleteObject(selectedLineBrush);
    DeleteObject(winBgColorBrush);

    BitBlt(ps.hdc, cliRect.left, cliRect.top, cliRect.right-cliRect.left, cliRect.bottom - cliRect.top, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hbmOld);

    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);
}