/*
 * =====================================================================================================================
 * See LICENSE file for copyright and license details.
 *
 * wallproc: A program which crops images to a specific aspect ratio.
 * =====================================================================================================================
 */

#include "data_structures.h"
#include "startup_shutdown.h"
#include "misc.h"

#include "SDL.h"

int main( int argc, char * argv[] ) {
        /*
         * Create the main data structures and perform all required initialization.
         */

        /* The init_pointers struct is only intended to keep main() clean. */
        INIT_POINTERS init_pointers = {NULL, NULL, NULL};
        /* Initializes everything that needs it. */
        initialize( &init_pointers, argc, argv );
        /* Contains sanitized and typed command line arguments. */
        CMD_LINE_ARGS * cmd_line_args = init_pointers.cmd_line_args;
        /* Will point to current element of a cyclic, double-linked list of files in 'source'. */
        FILE_LIST * file_list = init_pointers.file_list;
        /* Contains window, renderer and texture pointers for SDL. */
        SDL_POINTERS * sdl_pointers = init_pointers.sdl_pointers;

        // TODO: Draw first element. Make sure a valid element actually exists. Perhaps put this in the init section?
        //       Consider using SDL_PushEvent() to push a '->' arrow key press onto the event queue.
        
        /*
         * Main program loop
         */

        int quit = 0;
        FILE_LIST * temp = NULL;
        SDL_Event event;
        while( quit == 0 ) {
                if( SDL_WaitEvent( &event ) ) {
                        temp = process_sdl_event( &event, file_list, sdl_pointers, cmd_line_args );
                        if( temp == NULL ) {
                                quit = 1;
                        } else {
                                file_list = temp;
                        }
                }
        }


        /*
         * Free memory, close subsystems and exit.
         */

        terminate( cmd_line_args, file_list, sdl_pointers );
        exit(EXIT_SUCCESS);
}
