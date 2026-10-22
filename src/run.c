#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <time.h>

#include "chip8.h"

typedef struct {
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  SDL_AudioStream* audio_stream;
  Chip8 chip8;
  int current_sine_sample;
  uint64_t last_tick;
  bool beep;
} App;

static void SDLCALL FeedTheAudioStreamMore(void* userdata,
                                           SDL_AudioStream* audio_stream,
                                           int additional_amount,
                                           int total_amount) {
  App* app = userdata;
  total_amount = total_amount;  // Глушим, чтобы не ругались флаги
                                //
  // Считаем кол-во необходимых сэмплов
  additional_amount /= sizeof(float);

  while (additional_amount > 0) {
    float samples[128];  // Порция на одну итерацию
    const int total = SDL_min(additional_amount, SDL_arraysize(samples));
    int i;

    // Генерируем 440 Гц тон
    for (i = 0; i < total; i++) {
      const int freq = 440;
      const float phase = app->current_sine_sample * freq / 44100.0f;
      float frac = phase - SDL_floorf(phase);
      samples[i] = (frac < 0.5f) ? 0.15f : -0.15f;
      app->current_sine_sample++;
    }

    app->current_sine_sample %= 44100;

    // Отправляем новые данные в поток
    SDL_PutAudioStreamData(audio_stream, samples, total * sizeof(float));

    additional_amount -= total;
  }
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
  App* app = SDL_calloc(1, sizeof(App));
  if (!app) return SDL_APP_FAILURE;

  *appstate = app;

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("CHIP-8", kDisplayWidth * 10,
                                   kDisplayHeight * 10, 0, &app->window,
                                   &app->renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  app->texture = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING, kDisplayWidth,
                                   kDisplayHeight);
  if (!app->texture) return SDL_APP_FAILURE;

  SDL_SetTextureScaleMode(app->texture, SDL_SCALEMODE_NEAREST);

  SDL_AudioSpec spec;
  spec.channels = 1;
  spec.format = SDL_AUDIO_F32;
  spec.freq = 44100;
  app->audio_stream = SDL_OpenAudioDeviceStream(
      SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FeedTheAudioStreamMore, app);
  if (!app->audio_stream) {
    SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  InitChip8(&app->chip8);
  WriteFontsToMemory(app->chip8.memory);
  if (!WriteRomsToMemory(app->chip8.memory, argc, argv)) return SDL_APP_FAILURE;

  srand(time(NULL));

  app->last_tick = SDL_GetTicks();

  return SDL_APP_CONTINUE;
}

int FindKey(int scancode) {
  int key_map[16] = {
      SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
      SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
      SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
      SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V};

  int i = 0;
  int last = key_map[15];
  if (scancode == last) {
    i = 15;
  } else {
    key_map[15] = scancode;
    while (key_map[i] != scancode) i++;
    if (i == 15) i = -1;
    key_map[15] = last;
  }
  return i;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  App* app = appstate;
  int k = -1;
  switch (event->type) {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
      if (event->key.scancode == SDL_SCANCODE_ESCAPE) return SDL_APP_SUCCESS;

      k = FindKey(event->key.scancode);
      if (k != -1) app->chip8.keyboard[k] = 1;
      break;
    case SDL_EVENT_KEY_UP:
      k = FindKey(event->key.scancode);
      if (k != -1) app->chip8.keyboard[k] = 0;
      break;
    default:
      break;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  App* app = appstate;
  /*
  Считаем тики, чтобы обновлять таймеры и
  ограничивать выполнение скорость выполнения циклов процессора.
  При текущих настройках, мы получаем частоту таймеров примерно 60 Гц
  и скорость процессора - 600 инструкций в секунду.
  */
  uint64_t now_tick = SDL_GetTicks();
  if (now_tick - app->last_tick >= 17) {
    if (app->chip8.delay_timer > 0) --(app->chip8.delay_timer);

    if (app->chip8.sound_timer > 0) {
      if (!app->beep) {
        SDL_ResumeAudioStreamDevice(app->audio_stream);
        app->beep = true;
      }
      --(app->chip8.sound_timer);
    } else {
      if (app->beep) {
        SDL_PauseAudioStreamDevice(app->audio_stream);
        app->beep = false;
      }
    }

    app->last_tick += 17;
    app->chip8.vblank = true;

    for (int k = 0; k < 10; ++k) EmulateCycle(&app->chip8);

  } else {
    SDL_Delay(1);
  }

  if (app->chip8.update) {
    SDL_RenderClear(app->renderer);
    SDL_UpdateTexture(app->texture, NULL, app->chip8.display, 256);
    SDL_RenderTexture(app->renderer, app->texture, NULL, NULL);
    SDL_RenderPresent(app->renderer);
    app->chip8.update = false;
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  App* app = appstate;
  if (app) {
    SDL_DestroyTexture(app->texture);
    SDL_DestroyAudioStream(app->audio_stream);
    SDL_free(app);
  }

  if (result == SDL_APP_FAILURE)
    SDL_Log("Fail:(!");
  else
    SDL_Log("Success:)!");
}
