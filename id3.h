#ifndef ID3_DEF
#define ID3_DEF

typedef struct {
    int exists;
    char *pixels;
    int width;
    int height;
    char channels;
} SongImage;

typedef struct {
    char title[1024];
    char album[1024];
    char year[4];
    int track;
    char contentType[1024];
    char composer[1024];
    char artist[1024];
    SongImage image;
    char filepath[260];
    int ext;
} SongInfo;

typedef struct {
    SongInfo *songs;
    int numOfSongs;
    SongImage cover;
    char name[1024];
} Album;

typedef struct {
    Album *albums;
    int numOfAlbums;
    char name[1024];
} Artist;

void ID3_Open(const char *filename, SongInfo *info, int doReadImage);

#endif
