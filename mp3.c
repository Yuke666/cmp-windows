#include "windows.h"
#include <mmsystem.h>
#include <stdio.h>
#include <math.h>
#include <mad.h>
#include "mp3.h"
#include "player.h"

struct buffer {
    unsigned char *start;
    unsigned long length;
    FILE *fp;
    PlayerStruct *pStruct;
};

static int hitEOF = 0;
static char buffer[BLOCK_SIZE + MAD_BUFFER_GUARD];

static int FillBuffer(FILE *fp, struct mad_stream *stream){

    int remaining = 0;

    if(stream->next_frame != NULL){
        remaining = stream->bufend - stream->next_frame;
        memmove(buffer, stream->next_frame, remaining);
    }

    int num = fread(&buffer[remaining], sizeof(char), BLOCK_SIZE - remaining , fp);

    if(num == 0){
        if(!hitEOF){
            memset(&buffer[remaining], 0, MAD_BUFFER_GUARD);
            remaining+=MAD_BUFFER_GUARD;
            hitEOF = 1;
        } else {
            hitEOF = 0;
            return 0;
        }
    }

    mad_stream_buffer(stream, (const unsigned char*)buffer, num+remaining);

    stream->error = 0;

    return 1;
}

static enum mad_flow Error(void *data, struct mad_stream *stream, struct mad_frame *frame){
    return MAD_FLOW_CONTINUE;
}

static inline int Scale(mad_fixed_t sample)
{
    sample += 1L << (MAD_F_FRACBITS - 16);

    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    return sample >> (MAD_F_FRACBITS - 15);
}

static enum mad_flow Output(void *data, struct mad_header const *header, struct mad_pcm *pcm){
    unsigned int nchannels, nsamples;
    mad_fixed_t const *left_ch, *right_ch;

    nchannels = pcm->channels;
    nsamples  = pcm->length;
    left_ch   = pcm->samples[0];
    right_ch  = pcm->samples[1];

    PlayerStruct *p = ((struct buffer *)data)->pStruct;

    while (nsamples--) {
        signed int sample;

        sample = Scale(*left_ch++);
        Player_AddSample(((sample >> 0) & 0xFF), p);
        Player_AddSample(((sample >> 8) & 0xFF), p);

        if (nchannels == 2) {
            sample = Scale(*right_ch++);
            Player_AddSample(((sample >> 0) & 0xFF), p);
            Player_AddSample(((sample >> 8) & 0xFF), p);
        }
    }

    if(!Player_WriteSamples(p))
        return MAD_FLOW_STOP;

    return MAD_FLOW_CONTINUE;
}

static enum mad_flow Input(void *data, struct mad_stream *stream){
    struct buffer *buffer = data;
    if(!buffer->length)
        return MAD_FLOW_STOP;

    if(!FillBuffer(buffer->fp , stream))
        buffer->length = 0;

    return MAD_FLOW_CONTINUE;
}

static enum mad_flow Header(void *data, struct mad_header const *header){
    // sampleRate = header->samplerate;
    // numChannels = MAD_NCHANNELS(header);
    return MAD_FLOW_CONTINUE;
}

void MP3_Play(FILE *fp, PlayerStruct *p){
    if(fp == NULL) return;

    struct buffer buffer;
    struct mad_decoder decoder;

    buffer.length = BLOCK_SIZE;
    buffer.fp = fp;
    buffer.pStruct = p;

    mad_decoder_init(&decoder, &buffer, Input, Header, 0, Output, Error, 0  );
    mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

    mad_decoder_finish(&decoder);
}