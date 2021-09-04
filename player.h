#ifndef PLAYER_DEF
#define PLAYER_DEF

#define BLOCK_SIZE (5 * 8192)
#define NUM_BLOCKS 20

typedef struct {
    char samples[BLOCK_SIZE];
    char playingSamples[BLOCK_SIZE];
    int currentHeader;
    int waveFreeCount;
    int playingHeader;
    int samplesSize;
    WAVEHDR headers[NUM_BLOCKS];
    HWAVEOUT hWaveOut;
} PlayerStruct;

void Player_PlayMP3(char *path, void *callback);
void Player_Resume();
void Player_Pause();
void Player_Init();
void Player_Destroy();
void Player_AddSample(char sample, PlayerStruct *p);
int Player_WriteSamples(PlayerStruct *p);
short *Player_GetCurrSamples(int *size);

#endif