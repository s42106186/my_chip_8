#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "chip_8.h"

typedef struct { 
  SDL_Window* win;
  SDL_Renderer* ren;
  SDL_Texture* tex; 
  Chip8 chip8;
} App;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
  App *app = SDL_calloc(1, sizeof(App));
  if(!app) return SDL_APP_FAILURE;
  *appstate = app;

  if(!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if(!SDL_CreateWindowAndRenderer("CHIP-8", 640, 320,
			  0, &app->win, &app->ren)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  
  if(!InitChip8(&app->chip8)) return SDL_APP_FAILURE;
  if(!WriteFontsToMemory(app->chip8.memory)) return SDL_APP_FAILURE;
  if(!WriteRomsToMemory(app->chip8.memory)) return SDL_APP_FAILURE;

  app->tex =  SDL_CreateTexture(app->ren, SDL_PIXELFORMAT_ARGB8888, 
		  SDL_TEXTUREACCESS_STREAMING, 64, 32);
  if (!app->tex) return SDL_APP_FAILURE;
  SDL_SetTextureScaleMode(app->tex, SDL_SCALEMODE_NEAREST);

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
    App *app = appstate;
    EmulateCycle(&app->chip8);
    //printf("%X\n", app->chip8.opcode);
    if(app->chip8.update) {
      SDL_RenderClear(app->ren);
      SDL_UpdateTexture(app->tex, NULL, app->chip8.display, 256);
      SDL_RenderTexture(app->ren, app->tex, NULL, NULL);
      SDL_RenderPresent(app->ren);
    }
    return SDL_APP_CONTINUE; 
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    App *app = appstate;
    if(app) {
      SDL_DestroyTexture(app->tex);
      SDL_free(app);
    }
}
