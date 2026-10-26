#pragma once
struct SDL_Surface { int id; };
SDL_Surface *IMG_Load(const char *path);
void SDL_FreeSurface(SDL_Surface *s);
