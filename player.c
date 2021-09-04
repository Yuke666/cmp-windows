#include "windows.h"
#include <mmsystem.h>
#include <stdio.h>
#include <math.h>
#include <mad.h>
#include <process.h>
#include "player.h"
#include "mp3.h"

static CRITICAL_SECTION critSection;
static HANDLE playEvent;
static int hasInit = 0;
static int paused = 0;
static int newSong = 0;
static char playingSongPath[MAX_PATH];
static void (*onEndCallback)() = NULL;
static PlayerStruct mainPlayerStruct;

static void IfPausedWait(PlayerStruct *p){
    if(paused){
        waveOutPause(p->hWaveOut);
        WaitForSingleObject(playEvent, INFINITE);
    }
    waveOutRestart(p->hWaveOut);
    paused = 0;
}

static int IfNewSong(PlayerStruct *p){
    if(!newSong) return 1;
    p->samplesSize = 0;
    p->waveFreeCount = NUM_BLOCKS;
    return 0;
}

int Player_WriteSamples(PlayerStruct *p){

    WAVEHDR *header = &p->headers[p->currentHeader];

    int index = 0;

    while(p->samplesSize > 0){

        if(header->dwFlags & WHDR_PREPARED)
            waveOutUnprepareHeader(p->hWaveOut, header, sizeof(WAVEHDR));

        if(p->samplesSize < BLOCK_SIZE - header->dwUser){
            memcpy(header->lpData + header->dwUser, &p->samples[index], p->samplesSize);
            header->dwUser += p->samplesSize;
            p->samplesSize = 0;
            if(!IfNewSong(p)) return 0;
            return 1;
        }

        int remainingSize = BLOCK_SIZE - header->dwUser;

        memcpy(header->lpData + header->dwUser, &p->samples[index], remainingSize);

        p->samplesSize -= remainingSize;
        index += remainingSize;

        waveOutPrepareHeader(p->hWaveOut, header, sizeof(WAVEHDR));
        waveOutWrite(p->hWaveOut, header, sizeof(WAVEHDR));

        EnterCriticalSection(&critSection);
        p->waveFreeCount--;
        LeaveCriticalSection(&critSection);

        p->currentHeader++;
        p->currentHeader %= NUM_BLOCKS;
        header = &p->headers[p->currentHeader];
        header->dwUser = 0;

        while(!p->waveFreeCount){
            IfPausedWait(p);
            if(!IfNewSong(p)) return 0;
            Sleep(10);
        }
    }

    p->samplesSize = 0;
    if(!IfNewSong(p)) return 0;

    return 1;
}

short *Player_GetCurrSamples(int *size){
    PlayerStruct *p = &mainPlayerStruct;

    MMTIME pmmt = { .wType = TIME_SAMPLES };
    waveOutGetPosition(p->hWaveOut, &pmmt, sizeof(MMTIME));
    pmmt.u.sample %= (BLOCK_SIZE/2);
    *size = (BLOCK_SIZE/2);
    int charSize = BLOCK_SIZE - (pmmt.u.sample * 2);

    memcpy(p->playingSamples, p->headers[p->playingHeader].lpData, charSize);
    int nextHeader = p->playingHeader+1;
    nextHeader %= NUM_BLOCKS;
    memcpy(&p->playingSamples[charSize], p->headers[nextHeader].lpData, BLOCK_SIZE - charSize);
    return &((short *)p->playingSamples)[pmmt.u.sample];
}

void Player_AddSample(char sample, PlayerStruct *p){
    p->samplesSize++;
    p->samples[p->samplesSize-1] = sample;
    if(p->samplesSize >= BLOCK_SIZE)
        Player_WriteSamples(p);
}

static void CALLBACK waveOutProc(HWAVEOUT ho, UINT msg, DWORD dwInstance, DWORD dwParam, DWORD dwParam2){
    if(msg != WOM_DONE) return;

    PlayerStruct *pStryct = (PlayerStruct *)dwInstance;

    EnterCriticalSection(&critSection);
    pStryct->waveFreeCount++;
    pStryct->playingHeader++;
    pStryct->playingHeader %= NUM_BLOCKS;
    LeaveCriticalSection(&critSection);
}

static void MallocHeaders(PlayerStruct *p){
    int k;
    for(k = 0; k < NUM_BLOCKS; k++){
        p->headers[k] = (WAVEHDR){};
        p->headers[k].lpData = (char *)malloc(BLOCK_SIZE);
        p->headers[k].dwBufferLength = BLOCK_SIZE;
    }
}

static void FreeHeaders(PlayerStruct *p){
    int k;
    for(k = 0; k < NUM_BLOCKS; k++)
        if(p->headers[k].lpData)
            free(p->headers[k].lpData);
}

static void PlayMp3(WAVEFORMATEX *pwfx){

    mainPlayerStruct = (PlayerStruct){ .waveFreeCount = NUM_BLOCKS };

    MallocHeaders(&mainPlayerStruct);
    if(waveOutOpen(&mainPlayerStruct.hWaveOut, WAVE_MAPPER, pwfx, (DWORD_PTR)waveOutProc,
        (DWORD_PTR)&mainPlayerStruct, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
        return;


    while(1){

        while(!newSong) Sleep(10);

        newSong = 0;

        FILE *fp;
        if(!(fp = fopen(playingSongPath, "rb"))) continue;
        MP3_Play(fp, &mainPlayerStruct);
        fclose(fp);

        if(onEndCallback && !newSong) onEndCallback();
    }

    while(mainPlayerStruct.waveFreeCount < NUM_BLOCKS){}

    waveOutClose(mainPlayerStruct.hWaveOut);
    FreeHeaders(&mainPlayerStruct);
}

static void mainThread(){

    WAVEFORMATEX pwfx = {
        .nSamplesPerSec = 44100,
        .wBitsPerSample = 16,
        .nChannels = 2,
        .cbSize = 0,
        .wFormatTag = WAVE_FORMAT_PCM,
    };

    pwfx.nBlockAlign = (pwfx.wBitsPerSample * pwfx.nChannels) / 8;
    pwfx.nAvgBytesPerSec = pwfx.nBlockAlign * pwfx.nSamplesPerSec;
    InitializeCriticalSection(&critSection);

    PlayMp3(&pwfx);

    DeleteCriticalSection(&critSection);
}

void Player_Init(){
    playEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("PlayEvent"));
    _beginthread(mainThread, 0, NULL);
    hasInit = 1;
}

void Player_Destroy(){
    if(!hasInit) return;
    CloseHandle(&playEvent);
}

void Player_PlayMP3(char *path, void *callback){
    if(!hasInit) return;
    onEndCallback = callback;
    memset(playingSongPath, 0, sizeof(playingSongPath));
    strcpy(playingSongPath, path);
    newSong = 1;
}

void Player_Pause(){
    if(!hasInit) return;
    paused = 1;
}

void Player_Resume(){
    if(!hasInit) return;
    PulseEvent(playEvent);
}