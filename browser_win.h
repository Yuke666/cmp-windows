#ifndef BROWSER_DEF
#define BROWSER_DEF

#include "windows.h"

void BrowserWin_Paint(HWND hwnd);
void BrowserWin_UpLine();
void BrowserWin_DownLine(HWND hwnd);
void BrowserWin_Refresh();
void BrowserWin_Init();
void BrowserWin_OnEnter();
void BrowserWin_RemoveSelectedFromLib(HWND hwnd);
void BrowserWin_AddSelectedToLib(HWND hwns);

#endif