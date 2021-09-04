#ifndef IMAGE_LOADER_DEF
#define IMAGE_LOADER_DEF

#include "id3.h"

int ImageLoader_LoadPNG(FILE *fp, SongImage *img);
int ImageLoader_LoadJPEG(FILE *fp, SongImage *image);

#endif
