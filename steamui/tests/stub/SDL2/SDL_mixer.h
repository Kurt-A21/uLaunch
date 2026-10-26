#pragma once
#define MIX_MAX_VOLUME 128
struct Mix_Chunk { int id; int len_ms; };
Mix_Chunk *Mix_LoadWAV(const char *path);
int Mix_FadeInChannel(int ch, Mix_Chunk *c, int loops, int ms);
int Mix_FadeOutChannel(int ch, int ms);
int Mix_HaltChannel(int ch);
int Mix_Playing(int ch);
Mix_Chunk *Mix_GetChunk(int ch);
void Mix_FreeChunk(Mix_Chunk *c);
int Mix_VolumeMusic(int v);
const char *Mix_GetError();
