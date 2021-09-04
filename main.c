#include "windows.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "utils.h"
#include "browser_win.h"
#include "library_win.h"
#include "visualizer_win.h"
#include "player.h"
#include "config.h"

enum {
    BROWSER_WIN,
    LIBRARY_WIN,
    VISUALIZER_WIN,
};

static int onWindow = BROWSER_WIN;
static int volume = 0xFFFF;
static int paused = 0;
static HWND window;

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow){

    Config_ReadConfig();
    BrowserWin_Init();
    BrowserWin_Refresh();
    Player_Init();

    DWORD v;
    waveOutGetVolume(NULL, &v);
    volume = v & 0xFFFF;

    WNDCLASSW wc = {
        .style         = CS_HREDRAW | CS_VREDRAW,
        .cbClsExtra    = 0,
        .cbWndExtra    = 0,
        .lpszClassName = L"CMP",
        .hInstance     = hInstance,
        .hbrBackground = NULL,
        .lpszMenuName  = NULL,
        .lpfnWndProc   = WndProc,
        .hCursor       = LoadCursor(NULL, IDC_ARROW),
        .hIcon         = LoadIcon(NULL, IDI_APPLICATION),
    };

    INITCOMMONCONTROLSEX icex = {
        .dwSize = sizeof(INITCOMMONCONTROLSEX),
        .dwICC = ICC_STANDARD_CLASSES,
    };

    InitCommonControlsEx(&icex);

    RegisterClassW(&wc);
    window = CreateWindowW( wc.lpszClassName, (LPCWSTR)"CMP",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE ,
        CW_USEDEFAULT, CW_USEDEFAULT, Config_GetWindowWidth(), Config_GetWindowHeight(), NULL, NULL, hInstance, NULL);

    ShowWindow(window, nCmdShow);
    UpdateWindow(window);

    SetTimer(window, 1, 50, NULL);

    MSG msg;
    BOOL ret;

    while((ret = GetMessage(&msg, NULL, 0, 0))) {
        if(ret == -1){
            return (int) msg.wParam;
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    KillTimer(window, 1);

    return (int) msg.wParam;
}

HWND Main_GetWindow(){
    return window;
}

static void SetVolumeUp(){
    volume += 0xFFFF / 20;
    if(volume > 0xFFFF)
        volume = 0xFFFF;

    waveOutSetVolume(NULL, (volume << 16) | volume);
}

static void SetVolumeDown(){
    volume -= 0xFFFF / 20;
    if(volume < 0)
        volume = 0;

    waveOutSetVolume(NULL, (volume << 16) | volume);
}

static void HandleKeydown(WPARAM wParam, HWND hwnd){
    switch(wParam){

        case VK_UP:
            if(onWindow == BROWSER_WIN)
                BrowserWin_UpLine();
            else
                LibraryWin_UpLine();
            break;

        case VK_DOWN:
            if(onWindow == BROWSER_WIN)
                BrowserWin_DownLine(hwnd);
            else
                LibraryWin_DownLine(hwnd);
            break;

        case VK_RETURN:
            if(onWindow == BROWSER_WIN)
                BrowserWin_OnEnter();
            if(onWindow == LIBRARY_WIN)
                LibraryWin_OnEnter();
            break;
    }
}

static void HandleChar(char wParam, HWND hwnd){

    if(wParam == '2' && onWindow != LIBRARY_WIN){
        LibraryWin_Refresh();
        onWindow = LIBRARY_WIN;
    }

    if(tolower(wParam) == 'a')
        BrowserWin_AddSelectedToLib(hwnd);

    if(tolower(wParam) == 'r')
        BrowserWin_RemoveSelectedFromLib(hwnd);

    if(tolower(wParam) == '\t')
        if(onWindow == LIBRARY_WIN)
            LibraryWin_SwitchWindow();

    if(wParam == '1') onWindow = BROWSER_WIN;
    if(wParam == '3') onWindow = VISUALIZER_WIN;
    if(wParam == '-') SetVolumeDown();
    if(wParam == '=') SetVolumeUp();
    if(wParam == ' ') LibraryWin_ToggleDrawArtistSide();
    if(wParam == 'c'){
        if(paused)
            Player_Resume();
        else
            Player_Pause();

        paused = !paused;
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){

    switch(msg){

        case WM_DESTROY:
            Player_Destroy();
            Config_DeleteObjects();
            PostQuitMessage(0);
            return 0;

        case WM_CREATE:
            break;

        case WM_PAINT:
            if(onWindow == BROWSER_WIN)
                BrowserWin_Paint(hwnd);
            else if(onWindow == LIBRARY_WIN)
                LibraryWin_Paint(hwnd);
            else if(onWindow == VISUALIZER_WIN)
                VisualizerWin_Paint(hwnd);
            break;

        case WM_TIMER:
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
            break;

        case WM_ERASEBKGND:
            return TRUE;

        case WM_KEYDOWN:
            HandleKeydown(wParam, hwnd);
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
            break;

        case WM_CHAR:
            HandleChar(wParam, hwnd);
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}