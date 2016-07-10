/* See LICENSE file for copyright and license details. */

#include "SDL_image.h"
#include "data_structures.h"
#include "config.h"
#include "sdl.h"
#include "imagick.h"
#include "file_io.h"
#include "selection_box.h"
#include "ui.h"

void update_titlebar( FILE_LIST * file_list, SDL_POINTERS * sdl_pointers ) {
        /* First, calculate the selection size in megapixels. */
        double sel_size = (file_list->sel_w * file_list->sel_h) / 1000000.0;

        /* Now build the titlebar string and display it. */
        char * title = NULL;
        int length = snprintf( title, 0, "wallproc -- Image: %d -- Size: %.3f Mpx -- Selection: %dx%d -- File: %s", 
                        file_list->id, sel_size, file_list->sel_w, file_list->sel_h, file_list->path );
        title = malloc( length+1 );
        if( title == NULL ) {
                SDL_SetWindowTitle( sdl_pointers->window, "wallproc" );
        } else {
                snprintf( title, length+1, "wallproc -- Image %d -- Size: %.3f Mpx -- Selection: %dx%d -- File: %s",
                                file_list->id, sel_size, file_list->sel_w, file_list->sel_h, file_list->path );
                SDL_SetWindowTitle( sdl_pointers->window, title );
                free(title);
        }
}

FILE_LIST * draw( DIRECTION dir, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers ) {
        if( SGK_DEBUG ) printf( "DEBUG: Entering function draw().\n" );

        switch( dir ) { /* Traverse file_list in requested direction until a valid image is found. */
                case left:
                        while( file_list->prev->valid_sdl != 1 || file_list->prev->valid_imagick != 1 ) {
                                if( file_list->prev->valid_sdl != 1 ) sdl_test( file_list->prev, sdl_pointers );
                                if( file_list->prev->valid_imagick != 1 ) imagick_test( file_list->prev );
                        }
                        file_list = file_list->prev;
                        break;
                case right:
                        while( file_list->next->valid_sdl != 1 || file_list->next->valid_imagick != 1 ) {
                                if( file_list->next->valid_sdl != 1 ) sdl_test( file_list->next, sdl_pointers );
                                if( file_list->next->valid_imagick != 1 ) imagick_test( file_list->next );
                        }
                        file_list = file_list->next;
                        break;
                case up:
                case down:
                case none:
                        break;
        }

        /* Clear the screen. */
        sdl_clear( sdl_pointers );

        /* Load the image texture. */
        int window_w = 0;
        int window_h = 0;
        int temp = 0;
        SDL_GetWindowSize( sdl_pointers->window, &window_w, &window_h );
        temp = SDL_RenderSetLogicalSize( sdl_pointers->renderer, window_w, window_h );
        if( temp ) fprintf( stderr, "ERROR: Unable to set renderer size: %s\n", SDL_GetError() );
        SDL_DestroyTexture( sdl_pointers->texture );
        sdl_pointers->texture = IMG_LoadTexture( sdl_pointers->renderer, file_list->path );
        if( sdl_pointers->texture == NULL ) {
                /* 
                 * The texture *should* always load since we loaded it before setting the valid_sdl flag.
                 * However, if the texture fails to load, try to draw() in the same direction, and then 
                 * remove the failed image file_list struct.
                 */
                FILE_LIST * bad = file_list;
                file_list = draw( dir, file_list, sdl_pointers );
                del_file_from_list( bad );
        } else {
                /* Texture loaded successfully. */
                /* Calculate scaling to load texture in current SDL window. */
                SDL_Rect texture_dest_box = {0,0,0,0};
                sdl_texture_rect( &texture_dest_box, file_list, sdl_pointers );
                /* Load texture to renderer. */
                temp = SDL_RenderCopy( sdl_pointers->renderer, sdl_pointers->texture, NULL, &texture_dest_box );
                if( temp ) fprintf( stderr, "ERROR: Unable to copy texture to renderer: %s\n", SDL_GetError() );
                /* Calculate scaling to draw selection box in current SDL window. */
                SDL_Rect selection_dest_box = {0,0,0,0};
                sdl_selection_rect( &selection_dest_box, file_list, sdl_pointers );
                /* Load selection box to renderer. */
                temp = SDL_SetRenderDrawColor( sdl_pointers->renderer, file_list->sel_r, file_list->sel_g, 
                                file_list->sel_b, file_list->sel_a );
                if( temp ) fprintf( stderr, "ERROR: Unable to set renderer color: %s\n", SDL_GetError() );
                temp = SDL_RenderDrawRect( sdl_pointers->renderer, &selection_dest_box );
                if( temp ) fprintf( stderr, "ERROR: Unable to draw rectangle on renderer: %s\n", SDL_GetError() );
                /* Update titlebar and render SDL renderer to SDL window. */
                update_titlebar( file_list, sdl_pointers );
                SDL_RenderPresent( sdl_pointers->renderer );
        }
        
        if( SGK_DEBUG ) printf( "DEBUG: Leaving function draw().\n" );

        return file_list;
}
