/* See LICENSE file for copyright and license details. */

#include "SDL_image.h"
#include "data_structures.h"
#include "config.h"
#include "file_io.h"
#include "sdl.h"

int sdl_clear( SDL_POINTERS * sdl_pointers ) {
        int ret_val = 0;

        /* '128' is for R,G,B, respectively. */
        int temp = SDL_SetRenderDrawColor( sdl_pointers->renderer, 128, 128, 128, SDL_ALPHA_OPAQUE );
        if( temp != 0 ) {
                fprintf( stderr, "ERROR: Unable to set renderer draw color: %s\n", SDL_GetError() );
                ret_val = 1;
        }
        temp = SDL_RenderClear( sdl_pointers->renderer );
        if( temp != 0 ) {
                fprintf( stderr, "ERROR: Unable to clear renderer: %s\n", SDL_GetError() );
                ret_val = 1;
        }
        SDL_RenderPresent( sdl_pointers->renderer );

        return ret_val;
}

int sdl_init( SDL_POINTERS * sdl_pointers ) {
        int ret_val = 0;

        /* Initialize SDL subsystems. */
        /* We require the VIDEO subsystem for display. */
        /* The EVENTS and FILE I/O subsystems are initialized by default. */
        if( SDL_Init( SDL_INIT_VIDEO ) ) {
                fprintf( stderr, "ERROR: Unable to initialize SDL: %s\n", SDL_GetError() );
                ret_val = 1;
        }

        /* Print list of displays. Example: Display #2 Mode: 1600x2560 px @ 59 hz */
        if( SGK_DEBUG ) {
                printf( "DEBUG: Initializing SDL\n" );
                SDL_DisplayMode current;
                int num_displays = SDL_GetNumVideoDisplays();
                if( num_displays <= 0 ) {
                        fprintf( stderr, "ERROR: Unable to get number of displays: %s\n", SDL_GetError() );
                } else {
                        for( int i=0; i < num_displays; i++ ) {
                                int temp = SDL_GetCurrentDisplayMode( i, &current );
                                if( temp != 0 ) {
                                        fprintf( stderr, "ERROR: Unable to get current display mode: %s\n", 
                                                        SDL_GetError() 
                                                        );
                                } else {
                                        printf( "DEBUG:  -- Display #%d Mode: %dx%d @ %d hz\n", i, current.w,
                                                        current.h, current.refresh_rate
                                                        );
                                }
                        }
                }
        }

        /* Create an SDL window and renderer, and populate the sdl_pointers struct. */
        /* These two variables will hold the initial values for the window we shall create in a few lines. */
        int width = 0;
        int height = 0;
        SDL_DisplayMode disp_mode;
        /* TODO: Is there a way to find the 'primary' display like in Windows? For now use display 0. */
        int temp = SDL_GetCurrentDisplayMode( 0, &disp_mode );
        if( temp != 0 ) {
                fprintf( stderr, "ERROR: Unable to get current display mode: %s\n", SDL_GetError() );
                ret_val = 1;
        } else {
                width = disp_mode.w / 2;
                height = disp_mode.h / 2;
        }
        temp = SDL_CreateWindowAndRenderer( width, height, SDL_WINDOW_RESIZABLE, &sdl_pointers->window, 
                        &sdl_pointers->renderer 
                        );
        if( temp != 0 ) {
                fprintf( stderr, "ERROR: Unable to create SDL window: %s\n", SDL_GetError() );
                ret_val = 1;
        }
        /* Clear window so it appears normal to user. */
        temp = sdl_clear( sdl_pointers );
        if( temp != 0 ) {
                fprintf( stderr, "ERROR: Unable to clear SDL window: %s\n", SDL_GetError() );
                /* Not a fatal error. Do not flag. */
        }

        return ret_val;
}

void sdl_test( FILE_LIST * file_list, SDL_POINTERS * sdl_pointers ) {
        if( SGK_DEBUG ) printf( "DEBUG: Test-loading image with SDL: %s", file_list->path );
        if( file_list->valid_sdl == 1 ) {
                if( SGK_DEBUG ) printf( " -- already tested\n" );
                return;
        }
        SDL_DestroyTexture( sdl_pointers->texture );
        sdl_pointers->texture = NULL;
        sdl_pointers->texture = IMG_LoadTexture( sdl_pointers->renderer, file_list->path );
        if( sdl_pointers->texture == NULL ) {
                if( SGK_DEBUG ) printf( " -- failure\n" );
                del_file_from_list( file_list );
        } else {
                if( SGK_DEBUG ) printf( " -- success\n" );
                file_list->valid_sdl = 1;
                SDL_DestroyTexture( sdl_pointers->texture );
        }
}

void sdl_texture_rect( SDL_Rect * rect, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers ) {
        int window_w = 0;
        int window_h = 0;
        SDL_GetWindowSize( sdl_pointers->window, &window_w, &window_h );

        double window_aspect = (double) window_w / (double) window_h;
        double texture_aspect = (double) file_list->img_w / (double) file_list->img_h;
        if( window_aspect > texture_aspect ) {
                /* Window is wider aspect ratio than texture */
                rect->y = 0;                            /* No vertical offset */
                rect->h = window_h;                     /* Fill vertical space */
                rect->w = window_h * texture_aspect;    /* Maintain image aspect ratio */
                rect->x = ( window_w - rect->w ) / 2;   /* Horizontal offset to center image */
        } else {
                /* Window is narrower (or identical) aspect ratio than texture */
                rect->x = 0;                            /* No horizontal offset */
                rect->w = window_w;                     /* Fill horizontal space */
                rect->h = window_w / texture_aspect;    /* Maintain image aspect ratio */
                rect->y = ( window_h - rect->h ) / 2;   /* Vertical offset to center image */
        }

        if( SGK_DEBUG ) {
                printf( "DEBUG: For SDL window size %dx%d and image size %dx%d, setting texture render box to: "
                        "(%d+%d)x(%d+%d)\n", window_w, window_h, file_list->img_w, file_list->img_h, rect->w,
                        rect->x, rect->h, rect->y );
        }
}
