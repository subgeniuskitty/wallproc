/* See LICENSE file for copyright and license details. */

#include "wand/magick_wand.h"
#include <stdio.h>
#include "data_structures.h"
#include "config.h"
#include "file_io.h"
#include "sdl.h"
#include "imagick.h"
#include "misc.h"
#include "startup_shutdown.h"

void initialize( INIT_POINTERS * init_pointers, int argc, char * argv[] ) {
        /* Check for the right number of command line arguments. */
        if( argc != 4 ) {
                fprintf( stderr, "ERROR: Incorrect number of arguments.\n" );
                print_usage(argv);
                exit(EXIT_FAILURE);
        }
        /* Malloc space to hold the command line arguements struct. */
        init_pointers->cmd_line_args = malloc(sizeof(CMD_LINE_ARGS));
        if( init_pointers->cmd_line_args == NULL ) {
                fprintf( stderr, "ERROR: Unable to malloc for cmd_line_args struct.\n" );
                exit(EXIT_FAILURE);
        }
        /* Process the command line arguments. */
        if( process_argv( init_pointers->cmd_line_args, argv ) ) {
                fprintf( stderr, "ERROR: Unable to process command line arguments.\n" );
                exit(EXIT_FAILURE);
        }
        /* Build the list of images in 'source' command line argument. */
        init_pointers->file_list = build_file_list( init_pointers->cmd_line_args->src, 
                        init_pointers->cmd_line_args->aspect );
        if( init_pointers->file_list == NULL ) {
                fprintf( stderr, "ERROR: Failed to build list of files.\n" );
                exit(EXIT_FAILURE);
        }
        /* Check (print) the files in file_list. */
        if( SGK_DEBUG ) {
                printf( "DEBUG: Files in file list:\n" );
                FILE_LIST * start = init_pointers->file_list;
                FILE_LIST * current = init_pointers->file_list;
                do {
                        printf( "DEBUG:  -- %s\n", current->path );
                        current = current->next;
                } while ( current != start );
        }
        /* Malloc space to hold the SDL pointers struct. */
        init_pointers->sdl_pointers = malloc(sizeof(SDL_POINTERS));
        if( init_pointers->sdl_pointers == NULL ) {
                fprintf( stderr, "ERROR: Unable to malloc for sdl_pointers.\n" );
                exit(EXIT_FAILURE);
        }
        /* Initialize SDL */
        if( sdl_init( init_pointers->sdl_pointers ) ) {
                fprintf( stderr, "ERROR: Unable to initialize SDL.\n" );
                exit(EXIT_FAILURE);
        }
        /* Initialize ImageMagick */
        if( imagick_init() ) {
                fprintf( stderr, "ERROR: Unable to initialize ImageMagick.\n" );
                exit(EXIT_FAILURE);
        }
}

int process_argv( CMD_LINE_ARGS * cmd_line_args, char ** argv ) {
        int ret_val = 0;
        
        /* Sanitize the aspect ratio. */
        cmd_line_args->aspect = strtof( argv[3], NULL );
        if( cmd_line_args->aspect <= 0 ) {
                fprintf( stderr, "ERROR: Unable to process command line argument 'aspect'\n" );
                ret_val = 1;
        }

        /* Sanitize the source directory. */
        cmd_line_args->src = sanitize_path( argv[1] );
        if( cmd_line_args->src == NULL ) ret_val = 1;

        /* Sanitize the destination directory. */
        cmd_line_args->dst = sanitize_path( argv[2] );
        if( cmd_line_args->dst == NULL ) ret_val = 1;

        if( SGK_DEBUG ) {
                printf( "DEBUG: Processing command line arguments.\n" );
                printf( "DEBUG:  -- Aspect Ratio = %f\n", cmd_line_args->aspect );
                printf( "DEBUG:  -- Source = %s\n", cmd_line_args->src );
                printf( "DEBUG:  -- Destination = %s\n", cmd_line_args->dst );
        }

        return ret_val;
}

void terminate( CMD_LINE_ARGS * cmd_line_args, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers ) {
        /* Free memory related to command line arguments. */
        free( cmd_line_args->src );
        free( cmd_line_args->dst );
        free( cmd_line_args );

        /* Free memory related to list of files. */
        FILE_LIST * current = file_list->next;
        file_list->next = NULL;
        while( current->next != NULL ) {
                free( current->path );
                free( current->file );
                current = current->next;
                free( current->prev );
        }

        /* Terminate SDL. */
        SDL_DestroyTexture( sdl_pointers->texture );
        sdl_pointers->texture = NULL;
        SDL_DestroyRenderer( sdl_pointers->renderer );
        sdl_pointers->renderer = NULL;
        SDL_DestroyWindow( sdl_pointers->window );
        sdl_pointers->window = NULL;
        SDL_Quit();

        /* Free memory related to SDL. */
        free( sdl_pointers );

        /* Terminate ImageMagick. */
        MagickWandTerminus();
}
