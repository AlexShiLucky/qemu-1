/*
 * Cortex-M Graphic support.
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

#include <hw/cortexm/cortexm-graphic.h>
#include <hw/cortexm/cortexm-helper.h>
#include <hw/cortexm/cortexm-board.h>

#include <hw/display/gpio-led.h>

#include "qemu/osdep.h"
#include "qemu/timer.h"
#include "qemu/error-report.h"
#include "qemu/log.h"

#if defined(CONFIG_VERBOSE)
#include "verbosity.h"
#endif

/* ------------------------------------------------------------------------- */

/**
 * SDL event loop, called every 10 ms by the timer.
 */
int cortexm_graphic_event_loop(void)
{
    qemu_log_function_name();

#if defined(CONFIG_SDL)

    SDL_Event event;
    GPIOLEDState *state;
    bool is_on;
    BoardGraphicContext *board_graphic_context;

    // Raise graphic responsiveness.
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    while (SDL_WaitEvent(&event)) {

        switch (event.type) {

#if defined(CONFIG_SDLABI_2_0)

        case SDL_FIRSTEVENT:
            break;

        case SDL_WINDOWEVENT:
            /* Window state change */
            break;

#elif defined(CONFIG_SDLABI_1_2)

#endif

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            /* Nothing for now */
            break;

        case SDL_QUIT:
            // Quit the program
            fprintf(stderr, "Graphic window closed. Quit.\n");
            exit(1);

        case SDL_USEREVENT:

            // User events, enqueued with SDL_PushEvent().
            switch (event.user.code) {

            case GRAPHIC_EVENT_BOARD_INIT:
                board_graphic_context =
                        (BoardGraphicContext *) event.user.data1;

                cortexm_graphic_board_init_graphic_context(
                        board_graphic_context);
                break;

            case GRAPHIC_EVENT_LED_INIT:
                state = (GPIOLEDState *) event.user.data1;

                cortexm_graphic_led_init_graphic_context(
                        state->board_graphic_context,
                        &(state->led_graphic_context), state->colour.red,
                        state->colour.green, state->colour.blue);
                break;

            case GRAPHIC_EVENT_LED_TURN:
                state = (GPIOLEDState *) event.user.data1;
                is_on = (bool) event.user.data2;

                cortexm_graphic_led_turn(state->board_graphic_context,
                        &(state->led_graphic_context), is_on);
                break;

            }
            break;

        default:
            fprintf(stderr, "Other event 0x%X\n", event.type);
            break;
        }
    }

#endif /* defined(CONFIG_SDL) */

    return 0;
}

/*
 * Called from different threads to enqueue jobs via the event loop.
 * This ensures all graphic primitives are executed on the allowed thread.
 */
int cortexm_graphic_push_event(int code, void *data1, void *data2)
{
#if defined(CONFIG_SDL)
    SDL_Event event;

    event.type = SDL_USEREVENT;
    event.user.code = code;
    event.user.data1 = data1;
    event.user.data2 = data2;

    // TODO: if multi-thread access is needed, add a mutex.
    return SDL_PushEvent(&event);
#else
    return 0;
#endif /* defined(CONFIG_SDL) */

}

/* ------------------------------------------------------------------------- */

/*
 * Called via the atexit() mechanism, to clean the board graphic context.
 */
static void cortexm_graphic_quit(void)
{
    qemu_log_function_name();

#if defined(CONFIG_SDL)
    CortexMBoardState *board = cortexm_board_get();
    if (board != NULL) {
        BoardGraphicContext *board_graphic_context = &(board->graphic_context);

        if (cortexm_graphic_board_is_graphic_context_initialised(
                board_graphic_context)) {
            /* Destroy in reverse order of creation */
            if (board_graphic_context->texture != NULL) {
                SDL_DestroyTexture(board_graphic_context->texture);
            }

            if (board_graphic_context->renderer != NULL) {
                SDL_DestroyRenderer(board_graphic_context->renderer);
            }

            if (board_graphic_context->window != NULL) {
                SDL_DestroyWindow(board_graphic_context->window);
            }
        }
    }

    SDL_Quit();
#endif /* defined(CONFIG_SDL) */
}

/*
 * Start the graphic subsystem. From this moment on, the event queue
 * must be available to enqueue requests, even if the requests will
 * be processed when the event loop is entered (at the end of main()).
 */
void cortexm_graphic_start(void)
{
    qemu_log_function_name();

#if defined(CONFIG_SDL)
    SDL_Init(SDL_INIT_VIDEO);

    // CortexMBoardState *board = cortexm_board_get();

    // cortexm_graphic_board_init_graphic_context(&(board->graphic_context));

    atexit(cortexm_graphic_quit);
#endif /* defined(CONFIG_SDL) */
}

/* ------------------------------------------------------------------------- */

void cortexm_graphic_board_clear_graphic_context(
        BoardGraphicContext *board_graphic_context)
{
    qemu_log_function_name();

    board_graphic_context->picture_file_name = NULL;
    board_graphic_context->picture_file_absolute_path = NULL;
    board_graphic_context->window_caption = NULL;

#if defined(CONFIG_SDL)

#if defined(CONFIG_SDLABI_2_0)
    board_graphic_context->window = NULL;
    board_graphic_context->renderer = NULL;
    board_graphic_context->texture = NULL;
    board_graphic_context->surface = NULL;
#elif defined(CONFIG_SDLABI_1_2)
    board_graphic_context->surface = NULL;
#endif

#endif /* defined(CONFIG_SDL) */
}

bool cortexm_graphic_board_is_graphic_context_initialised(
        BoardGraphicContext *board_graphic_context)
{
    return (board_graphic_context->surface != NULL);
}

void cortexm_graphic_board_init_graphic_context(
        BoardGraphicContext *board_graphic_context)
{
    qemu_log_function_name();

#if defined(CONFIG_SDL)

    const char *fullname = qemu_find_file(QEMU_FILE_TYPE_IMAGES,
            board_graphic_context->picture_file_name);
    if (fullname == NULL) {
        error_printf("Image file '%s' not found.\n",
                board_graphic_context->picture_file_name);
        exit(1);
    }

    int res = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    if ((res & (IMG_INIT_JPG | IMG_INIT_PNG))
            != (IMG_INIT_JPG | IMG_INIT_PNG)) {
        error_printf("IMG_init failed (%s).\n", IMG_GetError());
        exit(1);
    }
    /* A better SDL_LoadBMP(). */
    SDL_Surface* board_bitmap = IMG_Load(
            board_graphic_context->picture_file_absolute_path);
    if (board_bitmap == NULL) {
        error_printf("Cannot load image file '%s' (%s).\n",
                board_graphic_context->picture_file_absolute_path,
                IMG_GetError());
        exit(1);
    }

#if defined(CONFIG_SDLABI_2_0)

    SDL_Window* win = SDL_CreateWindow(board_graphic_context->window_caption,
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, board_bitmap->w,
            board_bitmap->h, 0);
    if (win == NULL) {
        error_printf("Could not create window: %s\n", SDL_GetError());
        exit(1);
    }
    board_graphic_context->window = win;

    SDL_Renderer* renderer = SDL_CreateRenderer(win, -1,
            SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        error_printf("Could not create renderer: %s\n", SDL_GetError());
        exit(1);
    }
    board_graphic_context->renderer = renderer;

    SDL_Surface* board_bitmap_rgb = SDL_ConvertSurfaceFormat(board_bitmap,
            SDL_PIXELFORMAT_RGB888, 0);
    if (board_bitmap_rgb == 0) {
        error_printf("Could not create surface: %s\n", SDL_GetError());
        exit(1);
    }
    board_graphic_context->surface = board_bitmap_rgb;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer,
            board_bitmap_rgb);
    if (texture == NULL) {
        error_printf("Could not create texture: %s\n", SDL_GetError());
        exit(1);
    }
    board_graphic_context->texture = texture;

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

#elif defined(CONFIG_SDLABI_1_2)

    SDL_WM_SetCaption(caption, NULL);
    SDL_Surface* screen = SDL_SetVideoMode(board_bitmap->w, board_bitmap->h,
            32, SDL_DOUBLEBUF);
    SDL_Surface* board_surface = SDL_DisplayFormat(board_bitmap);

    /* Apply image to screen */
    SDL_BlitSurface(board_surface, NULL, screen, NULL);
    /* Update screen */
    SDL_Flip(screen);

    board_graphic_context->surface = screen;

#endif

    SDL_FreeSurface(board_bitmap);

#endif /* defined(CONFIG_SDL) */
}

/* ------------------------------------------------------------------------- */

void cortexm_graphic_led_clear_graphic_context(
        LEDGraphicContext *led_graphic_context)
{
    qemu_log_function_name();

    led_graphic_context->crop_on = NULL;
    led_graphic_context->crop_off = NULL;
}

bool cortexm_graphic_led_is_graphic_context_initialised(
        LEDGraphicContext *led_graphic_context)
{
    return (led_graphic_context->crop_on != NULL);
}

void cortexm_graphic_led_init_graphic_context(
        BoardGraphicContext *board_graphic_context,
        LEDGraphicContext *led_graphic_context, uint8_t red, uint8_t green,
        uint8_t blue)
{
    qemu_log_function_name();

#if defined(CONFIG_SDL)

    SDL_Surface *surface = board_graphic_context->surface;
    SDL_Rect *rectangle = &(led_graphic_context->rectangle);

#if defined(CONFIG_SDLABI_2_0)

    led_graphic_context->crop_off = SDL_ConvertSurfaceFormat(surface,
            SDL_PIXELFORMAT_RGB888, 0);

    led_graphic_context->crop_on = SDL_ConvertSurfaceFormat(surface,
            SDL_PIXELFORMAT_RGB888, 0);

#elif defined(CONFIG_SDLABI_1_2)

    led_graphic_context->crop_off = SDL_CreateRGBSurface(surface->flags,
            rectangle->w, rectangle->h, surface->format->BitsPerPixel,
            surface->format->Rmask, surface->format->Gmask,
            surface->format->Bmask, surface->format->Amask);

    led_graphic_context->crop_on = SDL_CreateRGBSurface(surface->flags,
            rectangle->w, rectangle->h, surface->format->BitsPerPixel,
            surface->format->Rmask, surface->format->Gmask,
            surface->format->Bmask, surface->format->Amask);

#endif

    /* Copy bitmap from original picture. */
    SDL_BlitSurface(board_graphic_context->surface, rectangle,
            led_graphic_context->crop_off, 0);

    Uint32 colour = SDL_MapRGB(led_graphic_context->crop_on->format, red, green,
            blue);

    /* Fill with uniform colour. */
    SDL_FillRect(led_graphic_context->crop_on, NULL, colour);

#endif /* defined(CONFIG_SDL) */
}

void cortexm_graphic_led_turn(BoardGraphicContext *board_graphic_context,
        LEDGraphicContext *led_graphic_context, bool is_on)
{
#if defined(CONFIG_SDL)

    SDL_Surface *crop =
            is_on ?
                    led_graphic_context->crop_on :
                    led_graphic_context->crop_off;

#if defined(CONFIG_SDLABI_2_0)
    SDL_UpdateTexture(board_graphic_context->texture,
            &(led_graphic_context->rectangle), crop->pixels, crop->pitch);
    SDL_RenderCopy(board_graphic_context->renderer,
            board_graphic_context->texture, NULL, NULL);
    SDL_RenderPresent(board_graphic_context->renderer);
#elif defined(CONFIG_SDLABI_1_2)
    SDL_BlitSurface(crop, NULL, board_graphic_context->surface,
            &(led_graphic_context->rectangle));
    SDL_Flip(board_graphic_context->surface);
#endif

#endif /* defined(CONFIG_SDL) */
}

/* ------------------------------------------------------------------------- */

