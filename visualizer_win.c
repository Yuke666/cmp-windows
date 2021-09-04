#include <stdio.h>
#include <math.h>
#include "visualizer_win.h"
#include "player.h"
#include "utils.h"
#include "config.h"

void VisualizerWin_Paint(HWND hwnd){

    RECT cliRect;
    GetClientRect(hwnd, &cliRect);

    PAINTSTRUCT ps;

    HDC hdc = BeginPaint(hwnd, &ps);
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(ps.hdc,cliRect.right - cliRect.left, cliRect.bottom-cliRect.top );
    HBITMAP hbmOld = SelectObject(hdcMem, hbmMem);

    HBRUSH selectedLineBrush = CreateSolidBrush((COLORREF){Config_GetColor(SELECTED_BG_COLOR)});
    HBRUSH bgBrush = CreateSolidBrush((COLORREF){Config_GetColor(DEFAULT_BG_COLOR)});

    HFONT font = Config_GetFont();
    SelectObject(hdcMem, font);

    FillRect(hdcMem, &cliRect, bgBrush);

    int size;
    short *samples = Player_GetCurrSamples(&size);

    SetBkColor(hdcMem, (COLORREF){Config_GetColor(SELECTED_BG_COLOR)});

    int lineHeight = (Config_GetFontSize() + (Config_GetLinePadding()*2));
    int totalLines = ((cliRect.bottom - cliRect.top) / lineHeight);

    int k, lastY = 0;
    for(k = 0; k < size/2; k++){

        int s = (float)((float)samples[k*2] / (float)0xFFFF) * totalLines;

        lastY = lastY > s ? lastY - 1 : lastY < s ? lastY + 1 : lastY;
        if(k == 0) lastY = s;

        int y = lastY + (totalLines / 2);

        TextOut(hdcMem, k*GetCharacterWidth(hdcMem, ' '), y*lineHeight, " ", strlen(" "));
    }

    BitBlt(ps.hdc, cliRect.left, cliRect.top, cliRect.right-cliRect.left, cliRect.bottom - cliRect.top, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hbmOld);

    EndPaint(hwnd, &ps);

    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    DeleteObject(selectedLineBrush);
    DeleteObject(bgBrush);
}