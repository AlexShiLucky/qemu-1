/*
 * Cortex-M graphics definitions.
 *
 * Copyright (c) 2016 Liviu Ionescu.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CORTEXM_GRAPHIC_H_
#define CORTEXM_GRAPHIC_H_

#include "qemu/osdep.h"

#if defined(CONFIG_SDL)
#if defined(CONFIG_SDLABI_2_0)
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#elif defined(CONFIG_SDLABI_1_2)
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#endif
#endif /* defined(CONFIG_SDL) */

/* ------------------------------------------------------------------------- */

/* Storage for board graphic context, stored in CortexMBoardState */
typedef struct {

    const char *picture_file_name;
    const char *picture_file_absolute_path;
    const char *window_caption;

#if defined(CONFIG_SDL)
#if defined(CONFIG_SDLABI_2_0)
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Surface *surface;
#elif defined(CONFIG_SDLABI_1_2)
    SDL_Surface *surface;
#endif
#endif /* defined(CONFIG_SDL) */

} BoardGraphicContext;

/* ------------------------------------------------------------------------- */

/* Storage for LED graphic context, stored in GPIOLEDState */
typedef struct {
    SDL_Rect rectangle;
    SDL_Surface *crop_off;
    SDL_Surface *crop_on;
} LEDGraphicContext;

/* ------------------------------------------------------------------------- */

enum {
    GRAPHIC_EVENT_NONE = 0,
    GRAPHIC_EVENT_BOARD_INIT,
    GRAPHIC_EVENT_LED_INIT,
    GRAPHIC_EVENT_LED_TURN,
};

/* ------------------------------------------------------------------------- */

void cortexm_graphic_start(void);

int cortexm_graphic_push_event(int code, void *data1, void *data2);

int cortexm_graphic_event_loop(void);

/* ----- Board graphic functions ----- */
void cortexm_graphic_board_clear_graphic_context(
        BoardGraphicContext *board_graphic_context);

bool cortexm_graphic_board_is_graphic_context_initialised(
        BoardGraphicContext *board_graphic_context);

void cortexm_graphic_board_init_graphic_context(
        BoardGraphicContext *board_graphic_context);

/* ----- LED graphic function ----- */
void cortexm_graphic_led_clear_graphic_context(
        LEDGraphicContext *led_graphic_context);

bool cortexm_graphic_led_is_graphic_context_initialised(
        LEDGraphicContext *led_graphic_context);

void cortexm_graphic_led_init_graphic_context(
        BoardGraphicContext *board_graphic_context,
        LEDGraphicContext *led_graphic_context, uint8_t red, uint8_t green,
        uint8_t blue);

void cortexm_graphic_led_turn(BoardGraphicContext *board_graphic_context,
        LEDGraphicContext *led_graphic_context, bool is_on);

/* ------------------------------------------------------------------------- */

#endif /* CORTEXM_GRAPHIC_H_ */

