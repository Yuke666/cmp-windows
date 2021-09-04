#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "id3.h"
#include "image_loader.h"
#include "utils.h"

struct id3_header {
    unsigned char file_identifier[3];
    unsigned char flags;
    uint16_t version;
    uint32_t size;
};

struct id3_frame {
    char id[5];
    uint32_t size;
    uint16_t flags;
};

static int getU32( uint32_t *val){
    uint32_t b0 = (*val & 0x000000FF);
    uint32_t b1 = (*val & 0x0000FF00) >> 8;
    uint32_t b2 = (*val & 0x00FF0000) >> 16;
    uint32_t b3 = (*val & 0xFF000000) >> 24;

    *val = b0 << 24 | b1 << 16 | b2 << 8 | b3;
    return 1;
}

static int getU24( uint32_t *val){
    uint32_t b0 = (*val & 0x000000FF);
    uint32_t b1 = (*val & 0x0000FF00) >> 8;
    uint32_t b2 = (*val & 0x00FF0000) >> 16;

    *val =  b0 << 16 | b1 << 8 | b2;
    return 1;
}

static int unsync( uint32_t *val){
    uint32_t b0 = (*val & 0x000000FF);
    uint32_t b1 = (*val & 0x0000FF00) >> 8;
    uint32_t b2 = (*val & 0x00FF0000) >> 16;
    uint32_t b3 = (*val & 0xFF000000) >> 24;

    if(b0 >= 0x80 || b1 >= 0x80 || b2 >= 0x80 || b3 >= 0x80) return 0;
    *val = b0 << 21 | b1 << 14 | b2 << 7 | b3;

    return 1;
}

static int ReadFrame(char *into, FILE *fp, int intoSize, struct id3_frame f){

    int size = abs(f.size);
    if(!size) return 0;

    memset(into, 0, size);

    char temp[size];
    fread(temp,sizeof(char), size, fp);

    int k, j = 0;
    for(k = 0; k < size && j < intoSize; k++)
        if(temp[k] > 31 && temp[k] < 127)
            into[j++] = temp[k];

    k = strlen(into);
    while(into[k-1] == ' '){ k--; }
    into[k] = '\0';

    return 1;
}

static int ReadPicture(SongImage *image, FILE *fp, int doReadImage, struct id3_frame frame){

    int size = abs(frame.size);
    int endPos = ftell(fp) + size;

    char text_encoding;
    char mime_type[64];
    char picture_type;
    // char discription[64];

    fread(&text_encoding, sizeof(char), 1, fp);

    int k;
    char ch = ' ';
    for(k = 0; k < 64 && ch != 0; k++) {
        ch = fgetc(fp);
        mime_type[k] = ch;
    }

    fread(&picture_type, sizeof(char), 1, fp);

    ch = ' ';
    for(k = 0; k < 64 && ch != 0; k++) {
        ch = fgetc(fp);
        // discription[k] = ch;
    }

    if(strcmp((const char *)mime_type, "image/jpeg") == 0){
        image->exists = 1;

        if(doReadImage)
            ImageLoader_LoadJPEG(fp, image);
    }

    if(strcmp((const char *)mime_type, "image/png") == 0){
        image->exists = 1;

        if(doReadImage)
            ImageLoader_LoadPNG(fp, image);
    }

    fseek(fp, endPos, SEEK_SET);

    return 0;
}

static int isFrameIDchar(char c){
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z');
}

void ID3_Open(const char *filename, SongInfo *info, int doReadImage){

    int k;
    char track[1024];

    memset(info, 0, sizeof(SongInfo));
    struct id3_frame currFrame;
    struct id3_header currHeader;

    FILE *mp3 = fopen(filename, "rb");
    if(!mp3) return;
    fread(&currHeader.file_identifier,sizeof(unsigned char), 3, mp3);
    fread(&currHeader.version,sizeof(uint16_t), 1, mp3);
    fread(&currHeader.flags,sizeof(unsigned char), 1, mp3);
    fread(&currHeader.size,sizeof(uint32_t), 1, mp3);

    if(currHeader.version == 4) unsync(&currHeader.size);
    if(currHeader.version == 3) getU32(&currHeader.size);
    if(currHeader.version == 2) getU24(&currHeader.size);
    if(currHeader.version < 2) { fclose(mp3); return; }

    while(ftell(mp3) < currHeader.size && !feof(mp3)){

        if(currHeader.version != 2){
            fread(&currFrame.id,sizeof(char), 4, mp3);
            fread(&currFrame.size,sizeof(char), 4, mp3);
            fread(&currFrame.flags,sizeof(char), 2, mp3);
            currFrame.id[4] = 0;
        } else {
            fread(&currFrame.id,sizeof(char), 3, mp3);
            fread(&currFrame.size,sizeof(char), 3, mp3);
            currFrame.flags = 0;
            currFrame.id[3] = 0;
        }

        if(currHeader.version == 4) unsync(&currFrame.size);
        if(currHeader.version == 3) getU32(&currFrame.size);
        if(currHeader.version == 2) getU24(&currFrame.size);

        int lessThan = currHeader.version > 2 ? 4 : 3;

        int k;
        for(k = 0; k < lessThan; k++)
            if(!isFrameIDchar(currFrame.id[k]) || (k == lessThan-1 && currFrame.id[k] == '\0'))
                goto out;

        if(!currFrame.size) break;
        if(currFrame.size > ftell(mp3) + currHeader.size) break;

        if(strcmp(currFrame.id, "TIT2") == 0) ReadFrame(info->title, mp3, sizeof(info->title), currFrame);
        else if(strcmp(currFrame.id, "TALB") == 0) ReadFrame(info->album, mp3, sizeof(info->album), currFrame);
        else if(strcmp(currFrame.id, "TYER") == 0) ReadFrame(info->year, mp3, sizeof(info->year), currFrame);
        else if(strcmp(currFrame.id, "TRCK") == 0) ReadFrame(track, mp3, sizeof(track), currFrame);
        else if(strcmp(currFrame.id, "TCON") == 0) ReadFrame(info->contentType, mp3, sizeof(info->contentType), currFrame);
        else if(strcmp(currFrame.id, "TCOM") == 0) ReadFrame(info->composer, mp3, sizeof(info->composer), currFrame);
        // else if(strcmp(currFrame.id, "TPE3") == 0) ReadFrame(info->artist, mp3, sizeof(info->artist), currFrame);
        // else if(strcmp(currFrame.id, "TPE2") == 0) ReadFrame(info->artist, mp3, sizeof(info->artist), currFrame);
        else if(strcmp(currFrame.id, "TPE1") == 0) ReadFrame(info->artist, mp3, sizeof(info->artist), currFrame);
        else if(strcmp(currFrame.id, "APIC") == 0) ReadPicture(&info->image, mp3, doReadImage, currFrame);
        else if(strcmp(currFrame.id, "PIC") == 0) ReadPicture(&info->image, mp3, doReadImage, currFrame);
        else if(strcmp(currFrame.id, "TAL") == 0) ReadFrame(info->album, mp3, sizeof(info->album), currFrame);
        else if(strcmp(currFrame.id, "TP1") == 0) ReadFrame(info->artist, mp3, sizeof(info->artist), currFrame);
        else if(strcmp(currFrame.id, "TP2") == 0) ReadFrame(info->artist, mp3, sizeof(info->artist), currFrame);
        else if(strcmp(currFrame.id, "TRK") == 0) ReadFrame(track, mp3, sizeof(track), currFrame);
        else if(strcmp(currFrame.id, "TT2") == 0) ReadFrame(info->title, mp3, sizeof(info->title), currFrame);
        else if(strcmp(currFrame.id, "TYE") == 0) ReadFrame(info->year, mp3, sizeof(info->year), currFrame);
        else if(strcmp(currFrame.id, "TCO") == 0) ReadFrame(info->contentType, mp3, sizeof(info->contentType), currFrame);
        else {
            char temp[2048];
            ReadFrame(temp, mp3, sizeof(temp), currFrame);
        }
    }

    out:

    k = 0;
    while(k < strlen(track) && track[k] != '/'){ k++; }

    track[k] = 0;
    info->track = atoi(track);

    fclose(mp3);
}