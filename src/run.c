#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "chip_8.h"


static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture* graph = NULL; 

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("CHIP-8", 640, 320, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    Init();
    graph = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
    SDL_SetTextureScaleMode(graph, SDL_SCALEMODE_NEAREST);

    return SDL_APP_CONTINUE; 
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; 
    }
    return SDL_APP_CONTINUE; 
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); 
    while(1) {
      EmulateCycle();
      if(update) {
        UpdateDisplay();
        SDL_UpdateTexture(graph, NULL, display2, 256);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, graph, NULL, NULL);
        SDL_RenderPresent(renderer);
      }
    }
    return SDL_APP_CONTINUE; 
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{

}




/*
int main(int argc, char *argv[])
{
	Init();
	WriteRomsToMemory();
	for(; ;) {
		update = 0;
		EmulateCycle();
		printf("0x%X ", opcode);
		if(update)
 		  UpdateDisplay();
	}
	return 0;
}
*/
