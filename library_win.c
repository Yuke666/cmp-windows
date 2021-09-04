#include "library_win.h"
#include "windows.h"
#include "utils.h"
#include "player.h"
#include "config.h"
#include "id3.h"
#include "image_loader.h"
#include "library.h"
#include "main.h"
#include <stdio.h>
#include <math.h>

enum {
    LIBRARY_ARTIST_WINDOW,
    LIBRARY_SONG_WINDOW
};

static int numOfArtists = 0;
static int inWindow = LIBRARY_ARTIST_WINDOW;
static int drawArtistSelectWin = 1;
static Artist *artists = NULL;
static LineSelector artistLS;
static LineSelector songLS;
static SongImage selectedCoverArt = {};
static Album *selectedAlbum = NULL;

static Artist *AddArtist(char *name){

    Artist *moreArtists = (Artist *)realloc(artists, ++numOfArtists*sizeof(Artist));
    if(!moreArtists){
        numOfArtists--;
        return NULL;
    }

    artists = moreArtists;
    Artist *a = &artists[numOfArtists-1];
    memset(a, 0, sizeof(Artist));
    strcpy(a->name, name);
    return a;
}

static Album *AddAlbum(Artist *a, char *name){

    Album *moreAlbums = (Album *)realloc(a->albums, ++a->numOfAlbums*sizeof(Album));
    if(!moreAlbums) {
        a->numOfAlbums--;
        return NULL;
    }

    a->albums = moreAlbums;
    memset(&a->albums[a->numOfAlbums-1], 0, sizeof(Album));
    strcpy(a->albums[a->numOfAlbums-1].name, name);

    return &a->albums[a->numOfAlbums-1];
}

static SongInfo *AddSong(Album *a, LibraryFileLine *l){

    SongInfo *moreSongs = (SongInfo *)realloc(a->songs, ++a->numOfSongs*sizeof(SongInfo));
    if(!moreSongs){
        a->numOfSongs--;
        return NULL;
    }

    a->songs = moreSongs;
    memset(&a->songs[a->numOfSongs-1], 0, sizeof(SongInfo));
    strcpy(a->songs[a->numOfSongs-1].title, l->title);
    strcpy(a->songs[a->numOfSongs-1].filepath, l->filePath);
    a->songs[a->numOfSongs-1].image.exists = l->imageExists;

    return &a->songs[a->numOfSongs-1];
}

static void HandleLibraryLine(LibraryFileLine *l){
    int k;
    for(k = 0; k < numOfArtists; k++){
        if(strcmp(l->artist, artists[k].name) == 0){

            int j;
            for(j = 0; j < artists[k].numOfAlbums; j++){

                if(strcmp(l->album, artists[k].albums[j].name) == 0){

                    AddSong(&artists[k].albums[j], l);
                    return;
                }
            }

            Album *a = AddAlbum(&artists[k], l->album);
            if(!a) return;
            AddSong(a, l);
            return;
        }
    }

    Artist *a = AddArtist(l->artist);
    if(!a) return;
    Album *al = AddAlbum(a, l->album);
    if(!al) return;
    AddSong(al, l);
}

static void FreeSongs(Album *al){
    int m;
    for(m = 0; m < al->numOfSongs; m++){
        if(al->songs[m].image.pixels){
            free(al->songs[m].image.pixels);
            al->songs[m].image.pixels = NULL;
        }
    }

    free(al->songs);
    al->numOfSongs = 0;
}

static void FreeAlbums(Artist *a){
    int j;
    for(j = 0; j < a->numOfAlbums; j++){

        FreeSongs(&a->albums[j]);

        if(a->albums[j].cover.pixels){
            free(a->albums[j].cover.pixels);
            a->albums[j].cover.pixels = NULL;
        }
    }

    free(a->albums);
    a->albums = NULL;
    a->numOfAlbums = 0;
}

static void FreeArtists(){
    if(artists){

        int k;
        for(k = 0; k < numOfArtists; k++)
            FreeAlbums(&artists[k]);

        free(artists);
        artists = NULL;
    }

    numOfArtists = 0;
}

static int GetArtistNumSongs(Artist a){

    int k, j, ret = 0;
    for(k = 0; k < a.numOfAlbums; k++)
        for(j = 0; j < a.albums[k].numOfSongs; j++, ret++){}

    return ret;
}

static SongInfo *GetArtistSong(Artist *a, int index){

    if(!a) return NULL;

    int k, j, ret = 0;
    for(k = 0; k < a->numOfAlbums; k++)
        for(j = 0; j < a->albums[k].numOfSongs; j++, ret++)
            if(ret == index) return &a->albums[k].songs[j];

    return NULL;
}

static Album *GetArtistSongsAlbum(Artist *a, int index){

    if(!a) return NULL;

    int k, j, ret = 0;
    for(k = 0; k < a->numOfAlbums; k++)
        for(j = 0; j < a->albums[k].numOfSongs; j++, ret++)
            if(ret == index) return &a->albums[k];

    return NULL;
}

static void SelectedSongSwitch(){

    if(!artists) return;

    SongInfo *song = GetArtistSong(&artists[artistLS.onLine], songLS.onLine);
    Album *a = GetArtistSongsAlbum(&artists[artistLS.onLine], songLS.onLine);

    if(!song || !a || a == selectedAlbum) return;

    if(selectedCoverArt.pixels)
        free(selectedCoverArt.pixels);

    selectedCoverArt.pixels = NULL;

    selectedAlbum = a;

    int k;
    for(k = 0; k < a->numOfSongs; k++){
        if(a->songs[k].image.exists){
            SongInfo info = {};
            ID3_Open(a->songs[k].filepath, &info, 1);
            selectedCoverArt = info.image;
            return;
        }
    }
}

static void GetAlbumCoverRect(RECT *rect, RECT winRect){
    Size imageSize = Config_GetImageSize();
    switch(Config_GetLibraryLayout()){
        case 0:
            rect->left = winRect.right - (winRect.right * imageSize.width);
            rect->top = 0;
            rect->bottom = rect->top + (winRect.bottom * imageSize.height);
            rect->right = rect->left + (winRect.right * imageSize.width);
            break;
        case 1:
            rect->left = 0;
            rect->top = winRect.bottom - (winRect.bottom * imageSize.height);
            rect->right = rect->left + (winRect.right * imageSize.width);
            rect->bottom = rect->top + (winRect.bottom * imageSize.height);
            break;
    }
}

static void GetLibrarySideRect(RECT *rect, RECT winRect){
    Size imageSize = Config_GetImageSize();
    *rect = winRect;

    if(Config_DoDrawImage() && selectedCoverArt.pixels){
        switch(Config_GetLibraryLayout()){
            case 0:
                rect->right = winRect.right - (winRect.right * imageSize.width);
                break;
            case 1:
                rect->bottom = winRect.bottom - (winRect.bottom * imageSize.height);
                break;
        }
    }

    Padding libPadding = Config_GetLibraryPadding();
    rect->right -= libPadding.right;
    rect->bottom -= libPadding.bottom;
    rect->left += libPadding.left;
    rect->top += libPadding.top;
}

void LibraryWin_UpLine(){

    if(inWindow == LIBRARY_ARTIST_WINDOW){
        Utils_LineSelector_UpLine(&artistLS);
        songLS = (LineSelector){};
        SelectedSongSwitch();
    }

    else if(inWindow == LIBRARY_SONG_WINDOW){
        Utils_LineSelector_UpLine(&songLS);
        SelectedSongSwitch();
    }
}

void LibraryWin_DownLine(HWND hwnd){

    RECT cliRect;
    GetClientRect(hwnd, &cliRect);

    RECT rect;
    GetLibrarySideRect(&rect, cliRect);

    rect.top += Config_IsTitleBarOnBottom() ? 0 : Config_GetLineHeight();
    rect.bottom -= !Config_IsTitleBarOnBottom() ? 0 : Config_GetLineHeight();

    int numLinesScroll = ((rect.bottom - rect.top) / Config_GetLineHeight()) - 1;

    if(inWindow == LIBRARY_ARTIST_WINDOW){
        Utils_LineSelector_DownLine(&artistLS, numOfArtists, numLinesScroll);
        songLS = (LineSelector){};
        SelectedSongSwitch();
    }

    else if(inWindow == LIBRARY_SONG_WINDOW){
        Utils_LineSelector_DownLine(&songLS, GetArtistNumSongs(artists[artistLS.onLine]), numLinesScroll);
        SelectedSongSwitch();
    }
}

void onSongEnd(){
    if(songLS.onLine+1 >= GetArtistNumSongs(artists[artistLS.onLine]))
        songLS.onLine = -1;

    LibraryWin_DownLine(Main_GetWindow());
    RedrawWindow(Main_GetWindow(), NULL, NULL, RDW_INVALIDATE);

    SongInfo *info = GetArtistSong(&artists[artistLS.onLine], songLS.onLine);
    Player_PlayMP3(info->filepath, onSongEnd);
}

void LibraryWin_OnEnter(){
    if(!artists) return;
    SongInfo *info = GetArtistSong(&artists[artistLS.onLine], songLS.onLine);
    Player_PlayMP3(info->filepath, onSongEnd);
}

void LibraryWin_SwitchWindow(){
    drawArtistSelectWin = 1;
    inWindow++;
    inWindow %= 2;
}

void LibraryWin_ToggleDrawArtistSide(){
    drawArtistSelectWin = !drawArtistSelectWin;
    if(inWindow != LIBRARY_SONG_WINDOW && !drawArtistSelectWin)
        inWindow = LIBRARY_SONG_WINDOW;
}

void LibraryWin_Refresh(){

    FreeArtists();
    artistLS = (LineSelector){};
    songLS = (LineSelector){};

    if(selectedCoverArt.pixels)
        free(selectedCoverArt.pixels);

    selectedCoverArt = (SongImage){};

    selectedAlbum = NULL;

    FILE *fp = fopen(LIBRARY_FILE_PATH, "r");
    if(!fp) return;

    while(!feof(fp)){

        char lineChars[sizeof(LibraryFileLine)];
        memset(lineChars, 0, sizeof(LibraryFileLine));

        GetNextLineInFile(fp, lineChars, sizeof(LibraryFileLine));

        LibraryFileLine line = {};
        if(Library_ProcessLine(lineChars, &line))
            HandleLibraryLine(&line);
    }

    fclose(fp);

    SelectedSongSwitch();
}

static int IsSelectedLine(int line, int window){
    if((window == LIBRARY_ARTIST_WINDOW && line == artistLS.onLine) ||
       (window == LIBRARY_SONG_WINDOW && line == songLS.onLine)){
            return 1;
    }

    return 0;
}

static void PaintLineSelectors(HDC hdcMem, HBRUSH selectedOtherTabBrush, HBRUSH bgBrush,
    HBRUSH selectedLineBrush, HBRUSH sepBrush, RECT rect){

    void setColors(int line, HBRUSH *brush, int window){
        if(inWindow != window){
            if(IsSelectedLine(line, window)){
                SetTextColor(hdcMem, (COLORREF){Config_GetColor(SELECTED_OTHER_TAB_FG_COLOR)});
                SetBkColor(hdcMem, (COLORREF){Config_GetColor(SELECTED_OTHER_TAB_BG_COLOR)});
                *brush = selectedOtherTabBrush;
            } else {
                SetTextColor(hdcMem, (COLORREF){Config_GetColor(DEFAULT_FG_COLOR)});
                SetBkColor(hdcMem, (COLORREF){Config_GetColor(DEFAULT_BG_COLOR)});
                *brush = bgBrush;
            }
        } else {
            if(IsSelectedLine(line, window)){
                SetTextColor(hdcMem, (COLORREF){Config_GetColor(SELECTED_FG_COLOR)});
                SetBkColor(hdcMem, (COLORREF){Config_GetColor(SELECTED_BG_COLOR)});
                *brush = selectedLineBrush;
            } else {
                SetTextColor(hdcMem, (COLORREF){Config_GetColor(DEFAULT_FG_COLOR)});
                SetBkColor(hdcMem, (COLORREF){Config_GetColor(DEFAULT_BG_COLOR)});
                *brush = bgBrush;
            }
        }
    }

    int maxArtistWidth = rect.right / 10;

    int k;
    for(k = 0; k < numOfArtists; k++){
        int width = GetStringWidth(hdcMem, artists[k].name);
        if(width > maxArtistWidth) maxArtistWidth = width;
    }

    if(maxArtistWidth > rect.right / 4)
        maxArtistWidth = rect.right/4;

    maxArtistWidth += 20;

    void artistFunc(int line, RECT r){
        HBRUSH brush;
        setColors(line, &brush, LIBRARY_ARTIST_WINDOW);

        if(r.right - r.left > maxArtistWidth)
            r.right = r.left + maxArtistWidth;

        FillRect(hdcMem, &r, brush);

        int chars = strlen(artists[line].name);

        if(chars * Config_GetFontWidth() > maxArtistWidth)
            chars = (maxArtistWidth-(Config_GetFontWidth()*2)) / Config_GetFontWidth();

        TextOut(hdcMem, r.left+10, r.top + Config_GetLinePadding(), artists[line].name, chars);
    }

    void songFunc(int line, RECT r){
        HBRUSH brush;
        setColors(line, &brush, LIBRARY_SONG_WINDOW);

        SongInfo *info = GetArtistSong(&artists[artistLS.onLine], line);

        FillRect(hdcMem, &r, brush);

        TextOut(hdcMem, r.left+10, r.top + Config_GetLinePadding(), info->title, strlen(info->title));
        Album *a = GetArtistSongsAlbum(&artists[artistLS.onLine], line);
        TextOut(hdcMem, r.right-10-GetStringWidth(hdcMem, a->name), r.top + Config_GetLinePadding(), a->name, strlen(a->name));
    }

    int titleBarBottom = Config_DoDrawTitleBar() && !Config_IsTitleBarOnBottom() ? Config_GetLineHeight() : 0;

    if(artists){
        Artist a = artists[artistLS.onLine];

        if(drawArtistSelectWin){
            Utils_LineSelector_Paint(artistLS, hdcMem, artistFunc, numOfArtists, rect.left, rect.top + titleBarBottom, maxArtistWidth);
            Utils_LineSelector_Paint(songLS, hdcMem, songFunc, GetArtistNumSongs(a), rect.left + maxArtistWidth,
                rect.top + titleBarBottom, rect.right - rect.left - maxArtistWidth);
        } else {
            Utils_LineSelector_Paint(songLS, hdcMem, songFunc, GetArtistNumSongs(a), rect.left,
                rect.top + titleBarBottom, rect.right - rect.left);
        }
    }

    if(drawArtistSelectWin){
        RECT r = {rect.left + maxArtistWidth, rect.top + titleBarBottom, rect.left + (maxArtistWidth+1), rect.bottom};
        FillRect(hdcMem, &r, sepBrush);
    }
}

static void DraWCover(HDC hdcMem,  RECT rect, HBRUSH fillBrush){
    if(Config_DoDrawImage() && selectedCoverArt.pixels){

        FillRect(hdcMem, &rect, fillBrush);

        Padding padding = Config_GetImagePadding();
        int drawWidth = (rect.right - rect.left) - padding.right - padding.left;
        int drawHeight = (rect.bottom - rect.top) - padding.top - padding.bottom;
        Utils_DrawImage(hdcMem, rect.left + padding.left, rect.top + padding.top, drawWidth,
            drawHeight, drawWidth, drawHeight, &selectedCoverArt);
    }
}

void LibraryWin_Paint(HWND hwnd){

    RECT cliRect;
    GetClientRect(hwnd, &cliRect);

    PAINTSTRUCT ps;

    HDC hdc = BeginPaint(hwnd, &ps);
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(ps.hdc,cliRect.right - cliRect.left, cliRect.bottom-cliRect.top );
    HBITMAP hbmOld = SelectObject(hdcMem, hbmMem);

    HBRUSH selectedLineBrush = CreateSolidBrush((COLORREF){Config_GetColor(SELECTED_BG_COLOR)});
    HBRUSH selectedOtherTabBrush = CreateSolidBrush((COLORREF){Config_GetColor(SELECTED_OTHER_TAB_BG_COLOR)});
    HBRUSH bgBrush = CreateSolidBrush((COLORREF){Config_GetColor(DEFAULT_BG_COLOR)});
    HBRUSH sepBrush = CreateSolidBrush((COLORREF){Config_GetColor(SEPARATOR_COLOR)});

    HFONT font = Config_GetFont();
    SelectObject(hdcMem, font);

    FillRect(hdcMem, &cliRect, bgBrush);

    RECT librarySideRect = {};
    GetLibrarySideRect(&librarySideRect, cliRect);

    if(Config_DoDrawTitleBar()){

        RECT r = librarySideRect;

        if(Config_IsTitleBarOnBottom()){
            r.top = librarySideRect.bottom - Config_GetLineHeight();
            r.bottom = r.top + Config_GetLineHeight();
        }

        DWORD vol = 0;
        waveOutGetVolume(NULL, &vol);
        Utils_DrawTitleBar(r, hdcMem, "%s%c%s%c%s%i%c", "CMP", FORMAT_STR_SEPERATOR, "Library window",
            FORMAT_STR_SEPERATOR, "Volume ", (int)round(((float)(vol & 0xFFFF) / (float)0xFFFF) * 100.0), '%');
    }

    RECT r = librarySideRect;
    if(Config_DoDrawTitleBar() && Config_IsTitleBarOnBottom())
        r.bottom -= Config_GetLineHeight();

    PaintLineSelectors(hdcMem, selectedOtherTabBrush, bgBrush, selectedLineBrush, sepBrush, r);

    RECT rectCover = {};

    GetAlbumCoverRect(&rectCover, cliRect);
    DraWCover(hdcMem, rectCover, bgBrush);

    BitBlt(ps.hdc, cliRect.left, cliRect.top, cliRect.right-cliRect.left, cliRect.bottom - cliRect.top, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hbmOld);

    EndPaint(hwnd, &ps);

    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    DeleteObject(selectedLineBrush);
    DeleteObject(selectedOtherTabBrush);
    DeleteObject(bgBrush);
    DeleteObject(sepBrush);
}