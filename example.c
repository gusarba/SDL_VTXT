// SDL_VTXT example by Gustavo Aranda (https://github.com/gusarba).
// This example includes the [Sweet16](https://github.com/kmar/Sweet16Font) and [Open Sans](https://github.com/googlefonts/opensans) fonts.

#include <stdio.h>
#include <SDL.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define VERTEXT_IMPLEMENTATION
#include "vertext.h"
#define SDL_VTXT_IMPLEMENTATION  // maybe change?
#include "SDL_VTXT.h"

// Main code
int main(int argc, char** argv) {
  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
    printf("Error: %s\n", SDL_GetError());
    return -1;
  }

  // Setup window
  SDL_Window* window = SDL_CreateWindow("SDL_VTXT example", 
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0);

  // Setup SDL_Renderer instance
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 
      SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    SDL_Log("Error creating SDL_Renderer!");
    return 0;
  }
  SDL_RendererInfo info;
  SDL_GetRendererInfo(renderer, &info);
  SDL_Log("Current SDL_Renderer: %s", info.name);

  // We have to configure vtxt to create indices 
  vtxt_setflags(VTXT_CREATE_INDEX_BUFFER);

  // Create a couple of SDL_VTXT objects,
  // each with a different font configuration
  SDL_VTXT* vf = SDL_VTXT_Init(renderer, 24, "sweet16mono.ttf");
  SDL_VTXT* vf2 = SDL_VTXT_Init(renderer, 96, "opensans.ttf");

  // Set up the first SDL_VTXT object
  SDL_VTXT_SetCursor(vf, 100, 200);
  SDL_VTXT_AppendLine(vf, "The quick brown fox");
  SDL_VTXT_NewLine(vf, 100);
  SDL_VTXT_AppendLine(vf, "jumps over the lazy dog");
  SDL_VTXT_AppendGlyph(vf, '.');

  // Set up the second one
  SDL_VTXT_SetCursor(vf2, 100, 100);
  SDL_VTXT_AppendLine(vf2, "Hello world!");

  // Main loop
  Uint8 done = 0;
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        done = 1;
      }
      if (event.type == SDL_WINDOWEVENT && 
          event.window.event == SDL_WINDOWEVENT_CLOSE && 
          event.window.windowID == SDL_GetWindowID(window)) {
        done = 1;
      }
      if (event.type == SDL_KEYUP &&
          event.key.keysym.sym == SDLK_ESCAPE) {
        done = 1;
      }
    }

    // Rendering
    SDL_SetRenderDrawColor(renderer, 0x70, 0x60, 0x40, 0xFF);

    // First clear the renderer
    SDL_RenderClear(renderer);

    // Draw the vertext
    SDL_VTXT_Render(vf, renderer);
    SDL_VTXT_Render(vf2, renderer);

    SDL_RenderPresent(renderer);
  }

  // Cleanup
  SDL_VTXT_Release(vf);
  SDL_VTXT_Release(vf2);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

