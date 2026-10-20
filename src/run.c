#define SDL_MAIN_USE_CALLBACKS 1
#include "chip_8.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

typedef struct {
  SDL_Window *win;
  SDL_Renderer *ren;
  SDL_Texture *tex;
  SDL_AudioStream *strm;
  int crnt_sn_smpl;
  bool beep;
  Chip8 chip8;
  uint64_t last_tick;
} App;

static void SDLCALL FeedTheAudioStreamMore(void *userdata,
                                           SDL_AudioStream *strm,
                                           int additional_amount, int total_amount) {

  App *app = userdata;
  additional_amount /= sizeof(float); /* convert from bytes to samples */
  while (additional_amount > 0) {
    float samples[128]; /* this will feed 128 samples each iteration until we
                           have enough. */
    total_amount = SDL_min(additional_amount, SDL_arraysize(samples));
    int i;

    /* generate a 440Hz pure tone */
    for (i = 0; i < total_amount; i++) {
      const int freq = 440;
      const float phase = app->crnt_sn_smpl * freq / 44100.0f;
      float frac = phase - SDL_floorf(phase);
      samples[i] = (frac < 0.5f) ? 0.15f : -0.15f;
      app->crnt_sn_smpl++;
    }

    /* wrapping around to avoid floating-point errors */
    app->crnt_sn_smpl %= 44100;

    /* feed the new data to the stream. It will queue at the end, and trickle
     * out as the hardware needs more data. */
    SDL_PutAudioStreamData(strm, samples, total_amount * sizeof(float));
    additional_amount -= total_amount; /* subtract what we've just fed the stream. */
  }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  App *app = SDL_calloc(1, sizeof(App));
  if (!app)
    return SDL_APP_FAILURE;
  *appstate = app;

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("CHIP-8", 640, 320, 0, &app->win,
                                   &app->ren)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!InitChip8(&app->chip8))
    return SDL_APP_FAILURE;
  if (!WriteFontsToMemory(app->chip8.memory))
    return SDL_APP_FAILURE;
  if (!WriteRomsToMemory(app->chip8.memory, argc, argv))
    return SDL_APP_FAILURE;

  app->tex = SDL_CreateTexture(app->ren, SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING, 64, 32);
  if (!app->tex)
    return SDL_APP_FAILURE;
  SDL_SetTextureScaleMode(app->tex, SDL_SCALEMODE_NEAREST);

  SDL_AudioSpec spec;
  spec.channels = 1;
  spec.format = SDL_AUDIO_F32;
  spec.freq = 44100;
  app->strm = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                        &spec, FeedTheAudioStreamMore, app);
  if (!app->strm) {
    SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  app->last_tick = SDL_GetTicks();
  srand(time(NULL));
  return SDL_APP_CONTINUE;
}

int8_t FindKey(uint16_t scancode) {
  uint16_t key_map[16] = {
    SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
    SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
    SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V
  };
  int8_t i = 0;
  uint16_t last = key_map[15];
  if(scancode == last) {
    i = 15;
  }else {
    key_map[15] = scancode;
    while(key_map[i] != scancode)
      i++;
    if(i == 15)
      i = -1;
    key_map[15] = last;
  }
  return i;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  App *app = appstate;
  int8_t k;
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return SDL_APP_SUCCESS;
  case SDL_EVENT_KEY_DOWN:
    if(event->key.scancode == SDL_SCANCODE_ESCAPE) 
      return SDL_APP_SUCCESS;

     k = FindKey(event->key.scancode);
     if(k != -1) 
       app->chip8.keyboard[k] = 1;
     break;
  case SDL_EVENT_KEY_UP:
     k = FindKey(event->key.scancode);
     if(k != -1) 
       app->chip8.keyboard[k] = 0;
     break;
  default:
    break;
  }
  return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
  App *app = appstate;

  uint64_t now_tick = SDL_GetTicks();
  if (now_tick - app->last_tick >= 17) {
    if (app->chip8.dt > 0)
      app->chip8.dt--;
    if (app->chip8.st > 0) {
      if (!app->beep) {
        SDL_ResumeAudioStreamDevice(app->strm);
        app->beep = true;
      }
      app->chip8.st--;
    } else {
      if (app->beep) {
        SDL_PauseAudioStreamDevice(app->strm);
        app->beep = false;
      }
    }
    app->last_tick += 17;
    app->chip8.vblank = true;
  }

  EmulateCycle(&app->chip8);
  if (app->chip8.update) {
    SDL_RenderClear(app->ren);
    SDL_UpdateTexture(app->tex, NULL, app->chip8.display, 256);
    SDL_RenderTexture(app->ren, app->tex, NULL, NULL);
    SDL_RenderPresent(app->ren);
    app->chip8.update = false;
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  App *app = appstate;
  if (app) {
    SDL_DestroyTexture(app->tex);
    SDL_DestroyAudioStream(app->strm);
    FreeChip8(&app->chip8);
    SDL_free(app);
  }

  if(result == SDL_APP_FAILURE)
    SDL_Log("Fail:(!");
  else
    SDL_Log("Success:)!");
}
