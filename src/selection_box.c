/* See LICENSE file for copyright and license details. */

#include "data_structures.h"
#include "config.h"
#include "sdl.h"
#include "selection_box.h"

void reset_sel_box( FILE_LIST * file_list ) {
        if( SGK_DEBUG ) printf( "DEBUG: Resetting selection box for file: %s\n", file_list->path );

        double img_aspect = (double) file_list->img_w / (double) file_list->img_h;

        /* Set selection box dimensions */
        if( img_aspect > file_list->aspect ) {
                /* Image is wider than desired aspect ratio. */
                file_list->sel_h = file_list->img_h;
                file_list->sel_w = file_list->sel_h * file_list->aspect;
        } else {
                /* Image is narrower than (or identical to) desired aspect ratio. */
                file_list->sel_w = file_list->img_w;
                file_list->sel_h = file_list->sel_w / file_list->aspect;
        }

        /* Center selection box */
        file_list->sel_x = ( file_list->img_w - file_list->sel_w ) / 2;
        file_list->sel_y = ( file_list->img_h - file_list->sel_h ) / 2;
}

void toggle_selection_color( FILE_LIST * file_list ) {
        /* For now, we only toggle between black and white selection boxes. */
        if( file_list->sel_r == 0 ) {
                file_list->sel_r = 255;
                file_list->sel_g = 255;
                file_list->sel_b = 255;
        } else {
                file_list->sel_r = 0;
                file_list->sel_g = 0;
                file_list->sel_b = 0;
        }
}

void sdl_selection_rect( SDL_Rect * sel_rect, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers ) {
        /* Get SDL_Rect for the current image at the current SDL window resolution. */
        SDL_Rect img_rect = {0,0,0,0};
        sdl_texture_rect( &img_rect, file_list, sdl_pointers );

        /* Scaling factor for image -> monitor pixels. */
        double scale = (double) img_rect.w / (double) file_list->img_w;

        /* Selection box width and height scale directly and at same scale as the image itself. */
        sel_rect->w = scale * file_list->sel_w;
        sel_rect->h = scale * file_list->sel_h;

        /* The calculated offset combines the raw image offset and the scaled selection box offset. */
        sel_rect->x = img_rect.x + ( scale * file_list->sel_x );
        sel_rect->y = img_rect.y + ( scale * file_list->sel_y );

        if( SGK_DEBUG ) {
                printf( "DEBUG: Generating selection box rectangle.\n" );
                printf( "DEBUG:  -- Scale: %f\n", scale );
                printf( "DEBUG:  -- Image dimensions: %dx%d\n", file_list->img_w, file_list->img_h );
                printf( "DEBUG:  -- Image rectangle: (%d+%d)x(%d+%d)\n", 
                                img_rect.w, img_rect.x, img_rect.h, img_rect.y );
                printf( "DEBUG:  -- Selection dimensions: %dx%d\n", file_list->sel_w, file_list->sel_h );
                printf( "DEBUG:  -- Selection rectangle: (%d+%d)x(%d+%d)\n", 
                                sel_rect->w, sel_rect->x, sel_rect->h, sel_rect->y );
        }
}

void sel_sanitize( SDL_Rect * params, FILE_LIST * file_list ) {
        /* If either dimension goes below 1/SELECT_SIZE_MULT, then inc_sel_size() will be unable to function. */
        /* Although there exist four cases, we only need to handle two cases due to the aspect constraint. */
        while( params->w < 1/SELECT_SIZE_MULT || params->h < 1/SELECT_SIZE_MULT ) {
                if( file_list->aspect > 1 ) {
                        params->h = 1/SELECT_SIZE_MULT + 1;
                        params->w = params->h * file_list->aspect;
                } else {
                        params->w = 1/SELECT_SIZE_MULT + 1;
                        params->h = params->w / file_list->aspect;
                }
        }

        /* Constrain selection box size with image dimensions. */
        while( params->w > file_list->img_w || params->h > file_list->img_h ) {
                if( params->w > file_list->img_w ) {
                        params->h = params->h * ( (double) file_list->img_w / (double) params->w );
                        params->w = file_list->img_w;
                }
                if( params->h > file_list->img_h ) {
                        params->w = params->w * ( (double) file_list->img_h / (double) params->h );
                        params->h = file_list->img_h;
                }
        }

        /* Constrain selection box location such that it remains entirely within the image boundaries. */
        if( params->y < 0 ) params->y = 0;
        if( (params->y+params->h) > file_list->img_h ) params->y = file_list->img_h - params->h;
        if( (params->x+params->w) > file_list->img_w ) params->x = file_list->img_w - params->w;
        if( params->x < 0 ) params->x = 0;

}

void sel_resize( DIRECTION dir, FILE_LIST * file_list ) {
        if( SGK_DEBUG ) printf( "DEBUG: Resizing selection box.\n" );

        SDL_Rect temp = {0,0,0,0};

        switch( dir ) {
                case up:
                        temp.w = file_list->sel_w * (1 + SELECT_SIZE_MULT);
                        temp.h = temp.w / file_list->aspect;
                        break;
                case down:
                        temp.w = file_list->sel_w * (1 - SELECT_SIZE_MULT);
                        temp.h = temp.w / file_list->aspect;
                        break;
                case left:
                case right:
                case none:
                        break;
        }

        /* Verify that new selection box size doesn't exceed bounds. */
        sel_sanitize( &temp, file_list );

        /* Center selection box exactly where it used to be centered. */
        temp.x = file_list->sel_x + ( file_list->sel_w / 2 ) - ( temp.w / 2 );
        temp.y = file_list->sel_y + ( file_list->sel_h / 2 ) - ( temp.h / 2 );

        /* Verify that new selection box position doesn't exceed bounds. */
        sel_sanitize( &temp, file_list );

        if( SGK_DEBUG ) {
                printf( "DEBUG:  -- Image dimensions: %dx%d\n", file_list->img_w, file_list->img_h );
                printf( "DEBUG:  -- Old selection: (%d+%d)x(%d+%d)\n", file_list->sel_w, 
                                file_list->sel_x, file_list->sel_h, file_list->sel_y );
                printf( "DEBUG:  -- New selection: (%d+%d)x(%d+%d)\n", temp.w, temp.x, temp.h, temp.y );
        }

        /* Store temporary values as new selection box dimensions. */
        file_list->sel_w = temp.w;
        file_list->sel_h = temp.h;
        file_list->sel_x = temp.x;
        file_list->sel_y = temp.y;
}

void sel_move( DIRECTION dir, FILE_LIST * file_list ) {
        if( SGK_DEBUG ) printf( "DEBUG: Moving selection box.\n" );

        SDL_Rect temp = {0,0,0,0};
        temp.h = file_list->sel_h;
        temp.w = file_list->sel_w;

        /* Populate temporary values with requested offset for selection box. */
        switch( dir ) {
                case up:
                        temp.x = file_list->sel_x;
                        temp.y = file_list->sel_y - ( SELECT_POS_MULT * file_list->img_h );
                        break;
                case down:
                        temp.x = file_list->sel_x;
                        temp.y = file_list->sel_y + ( SELECT_POS_MULT * file_list->img_h );
                        break;
                case left:
                        temp.x = file_list->sel_x - ( SELECT_POS_MULT * file_list->img_w );
                        temp.y = file_list->sel_y;
                        break;
                case right:
                        temp.x = file_list->sel_x + ( SELECT_POS_MULT * file_list->img_w );
                        temp.y = file_list->sel_y;
                        break;
                case none:
                        break;
        }

        /* Verify that new selection box position doesn't exceed bounds. */
        sel_sanitize( &temp, file_list );

        if( SGK_DEBUG ) {
                printf( "DEBUG:  -- Image dimensions: %dx%d\n", file_list->img_w, file_list->img_h );
                printf( "DEBUG:  -- Old selection: (%d+%d)x(%d+%d)\n", file_list->sel_w, 
                                file_list->sel_x, file_list->sel_h, file_list->sel_y );
                printf( "DEBUG:  -- New selection: (%d+%d)x(%d+%d)\n", temp.w, temp.x, temp.h, temp.y );
        }

        /* Store temporary values as new selection box offsets. */
        file_list->sel_x = temp.x;
        file_list->sel_y = temp.y;
}
