//Provides a clean interface to the Video functions from SDL2 

#ifndef VIDEO_WRAP_SDL2_H
#define VIDEO_WRAP_SDL2_H

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include "SDL.h"
#include <fenv.h>
#include <stdatomic.h>

typedef struct Video{
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	size_t w,h;
	float *realmap;
} Video;

extern inline void Video_Destroy(Video *video){
	free(video->realmap);
	if(video->texture)
		SDL_DestroyTexture(video->texture);
	if(video->renderer)
		SDL_DestroyRenderer(video->renderer);
	if(video->window)
		SDL_DestroyWindow(video->window);
	*video = (Video){0};
	SDL_Quit();
}

extern inline bool Video_Create(Video *video, const int w, const int h, const char *caption){
	*video = (Video){.w = w, .h = h};
	if(!(video->realmap = malloc( 3 * w * h * sizeof *video->realmap)))
		return false;
	if(SDL_Init(SDL_INIT_VIDEO))
		goto cleanup;
	if(!(video->window = SDL_CreateWindow(
					caption,
					SDL_WINDOWPOS_CENTERED,
					SDL_WINDOWPOS_CENTERED,
					w,
					h,
					SDL_WINDOW_RESIZABLE)))
		goto cleanup;
	if(!(video->renderer = SDL_CreateRenderer(
					video->window,
					-1,
					SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC)))
		goto cleanup;
	if(!(video->texture = SDL_CreateTexture(
					video->renderer,
					SDL_PIXELFORMAT_RGB24,
					SDL_TEXTUREACCESS_STREAMING,
					w,
					h)))
		goto cleanup;
	return true;
cleanup:
	fprintf(stderr, "%s\n", SDL_GetError());
	Video_Destroy(video);
	return false;
}



extern inline void Video_RealmapDraw(const Video video){
	uint8_t *pixels;
	int pitch;
	SDL_LockTexture(
			video.texture,
			NULL,
			(void**)&pixels,
			&pitch);
	int rd = fegetround();
	fesetround(FE_DOWNWARD);
	for(size_t j = 0; j < video.h; j++)
		for(size_t i = 0; i < video.w * 3; i++){
			float value = video.realmap[3 * j * video.w + i];
			 //Reinhard tone mapping
			float normalized_value = value / (1.0 + value);
			pixels[j * pitch + i] = lrintf(normalized_value * 255);
		}
	fesetround(rd);
	SDL_UnlockTexture(video.texture);
	if(SDL_RenderCopy(video.renderer, video.texture, NULL, NULL)){
		fprintf(stderr, "%s\n", SDL_GetError());
	}
	SDL_RenderPresent(video.renderer);
}

extern inline void WaitExit(void){

	while(true){
		SDL_Event e;
		if(!SDL_WaitEvent(&e)) continue;
		if(SDL_QUIT == e.type) break;
	}
}

#endif
