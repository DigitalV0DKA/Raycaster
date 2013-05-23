/*
 * api_interface.cc
 *
 *
 * API interface for simple raycaster. Can use to switch between OpenGL,
 * SDL, DirectX, etc without having to change the main source files
 *
 *
 * Copyright (C) 2013  Bryant Moscon - bmoscon@gmail.com
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution, and in the same 
 *    place and form as other copyright, license and disclaimer information.
 *
 * 3. The end-user documentation included with the redistribution, if any, must 
 *    include the following acknowledgment: "This product includes software 
 *    developed by Bryant Moscon (http://www.bryantmoscon.org/)", in the same 
 *    place and form as other third-party acknowledgments. Alternately, this 
 *    acknowledgment may appear in the software itself, in the same form and 
 *    location as other such third-party acknowledgments.
 *
 * 4. Except as contained in this notice, the name of the author, Bryant Moscon,
 *    shall not be used in advertising or otherwise to promote the sale, use or 
 *    other dealings in this Software without prior written authorization from 
 *    the author.
 *
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 */

#include <SDL/SDL.h>

#include "api_interface.h"


void init()
{
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
    exit(1);
  }
}

void deinit()
{
  SDL_Quit();
}

void init_screen(uint32_t w, uint32_t h, const char* title, const char* icon)
{
  SDL_Surface* screen;
  
  screen = SDL_SetVideoMode(w, h, 32, SDL_HWSURFACE|SDL_ASYNCBLIT|SDL_DOUBLEBUF|SDL_HWPALETTE);
  
  if (!screen) {
    fprintf(stderr, "SDL SetVideoMode failed: %s\n", SDL_GetError());
    SDL_Quit();
    exit(1);
  }

  SDL_WM_SetCaption(title, icon);
}

void vertical_line(int x, int y, int h, const color_t *color)
{
  SDL_Surface *screen = SDL_GetVideoSurface();
  if (SDL_MUSTLOCK(screen)) {
    if (SDL_LockSurface(screen) != 0) {
      fprintf(stderr, "SDL SetVideoMode failed: %s\n", SDL_GetError());
      SDL_Quit();
      exit(1);
    }
  }
  
  // convert color_t to SDL color value
  uint32_t converted_color = SDL_MapRGB(screen->format, color->r, color->g, color->b);
  uint32_t *buffer;
  
  // get pointer to pixel in video memory. Need to exact pixel location.
  // pitch = video buffer width, which can be much larger than surface/window. 
  // PixelAddr = (y * pitch) + x. Since this is an SDL Surface, the pixels are 32 bits (4 bytes) 
  // so must divide by 4 to make the pitch be the image's width in pixels
  buffer = (uint32_t *)screen->pixels + y * screen->pitch / 4 + x;

  for (; y <= h; ++y, buffer+= screen->pitch / 4) {
    *buffer = converted_color;
  }
  
  if (SDL_MUSTLOCK(screen)) {
    SDL_UnlockSurface(screen);
  }
}

void clear_screen()
{
  SDL_Surface* screen = SDL_GetVideoSurface();
  
  if (SDL_MUSTLOCK(screen)) {
    SDL_LockSurface(screen);
  }

  SDL_FillRect(screen, NULL, 0);

  if (SDL_MUSTLOCK(screen)) {
    SDL_UnlockSurface(screen);
  }
}

void refresh()
{
  SDL_Surface* screen = SDL_GetVideoSurface();

  if (SDL_MUSTLOCK(screen)) {
    SDL_LockSurface(screen);
  }

  SDL_UpdateRect(screen, 0, 0, 0, 0);

  if (SDL_MUSTLOCK(screen)) {
    SDL_UnlockSurface(screen);
  }
}


events_e handle_events()
{
  SDL_Event event = {};
  
  if (!SDL_PollEvent(&event)) {
    return (NONE);
  }
  
  switch (event.type) {
    case SDL_QUIT:
      return (EXIT);
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      switch (event.key.keysym.sym) {
        case SDLK_w:
	  return (KEY_W);
	  break;
        case SDLK_a:
	  return (KEY_A);
	  break;
        case SDLK_s:
	  return (KEY_S);
	  break;
        case SDLK_d:
	  return (KEY_D);
	  break;
        default:
	  return (NONE);
      }
      break;
  }
  
  return (NONE);
}

uint64_t get_ticks()
{
  return (SDL_GetTicks());
}

void sleep_ticks(uint64_t ticks)
{
  SDL_Delay(ticks);
}
