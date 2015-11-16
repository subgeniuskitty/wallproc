/* See LICENSE file for copyright and license details. */

#include <stdio.h>
#include "data_structures.h"
#include "config.h"
#include "ui.h"
#include "file_io.h"
#include "selection_box.h"
#include "misc.h"

void print_usage( char * argv[] ) {
        printf( "wallproc %d.%d (www.subgeniuskitty.com)\n"
                "Usage: %s <source> <destination> <aspect>\n"
                "  source:      Directory containing images to be processed\n"
                "  destination: Directory to contain modified images\n"
                "  aspect:      Desired aspect ratio of cropped images as a float\n"
                "               Example: 2560x1600 resolution is 16:10 aspect ratio, so aspect would be 1.6\n"
                , VER_MAJOR, VER_MINOR, argv[0] );
}

FILE_LIST * process_sdl_event( SDL_Event * event, FILE_LIST * file_list, 
                SDL_POINTERS * sdl_pointers, CMD_LINE_ARGS * cmd_line_args ) {

        switch( event->type ) {
                case SDL_QUIT:
                        file_list = NULL;
                        break;
                case SDL_WINDOWEVENT:
                        if( event->window.event == SDL_WINDOWEVENT_SHOWN
                                || event->window.event == SDL_WINDOWEVENT_EXPOSED
                                || event->window.event == SDL_WINDOWEVENT_MOVED
                                || event->window.event == SDL_WINDOWEVENT_RESIZED
                                || event->window.event == SDL_WINDOWEVENT_MAXIMIZED
                                || event->window.event == SDL_WINDOWEVENT_RESTORED
                                ) {
                                file_list = draw( none, file_list, sdl_pointers );
                        }
                        break;
                case SDL_KEYDOWN:
                        switch( event->key.keysym.sym ) {
                                case KEY_QUIT:
                                        file_list = NULL;
                                        break;
                                case KEY_HELP:
                                        break;
                                case KEY_SAVE:
                                        crop_save( file_list, cmd_line_args );
                                        break;
                                case KEY_UNDO:
                                        del_img( file_list, cmd_line_args );
                                        break;
                                case KEY_NEXT:
                                        file_list = draw( right, file_list, sdl_pointers );
                                        break;
                                case KEY_PREV:
                                        file_list = draw( left, file_list, sdl_pointers );
                                        break;
                                case KEY_SIZEUP:
                                        sel_resize( up, file_list );
                                        file_list = draw( none, file_list, sdl_pointers );
                                        break;
                                case KEY_SIZEDOWN:
                                        sel_resize( down, file_list );
                                        file_list = draw( none, file_list, sdl_pointers );
                                        break;
                                case KEY_LEFT:
                                        sel_move( left, file_list );
                                        file_list = draw( none, file_list, sdl_pointers );
                                        break;
                                case KEY_RIGHT:
                                        sel_move( right, file_list );
                                        file_list = draw( none, file_list, sdl_pointers );
                                        break;
                                case KEY_UP:
                                        sel_move( up, file_list );
                                        file_list = draw( none, file_list, sdl_pointers );
                                        break;
                                case KEY_DOWN:
                                        sel_move( down, file_list );
                                        file_list = draw( none, file_list, sdl_pointers );
                                        break;
                                case KEY_SELECTIONBOX_RESET:
                                        reset_sel_box( file_list );
                                        file_list = draw( none, file_list, sdl_pointers );
                                        break;
                                case KEY_TOGGLE_OUTLINE_COLOR:
                                        toggle_selection_color( file_list );
                                        file_list = draw( none, file_list, sdl_pointers );
                                        break;
                                default:
                                        // TODO: Should I display help if unrecognized key is pressed?
                                        break;

                        }
                        break;
                default:
                        /* Ignore all other SDL events. */
                        break;
        }

        return file_list;
}
