#ifndef LIBRARY_WIN_DEF
#define LIBRARY_WIN_DEF

#include "windows.h"

void LibraryWin_Paint(HWND hwnd);
void LibraryWin_UpLine();
void LibraryWin_DownLine(HWND hwnd);
void LibraryWin_Refresh();
void LibraryWin_SwitchWindow();
void LibraryWin_Init();
void LibraryWin_OnEnter();
void LibraryWin_RemoveSelectedFromLib();
void LibraryWin_ToggleDrawArtistSide();

#endif