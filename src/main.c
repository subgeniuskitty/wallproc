/*
 * =====================================================================================================================
 * wallproc: A program which crops images to a specified aspect ratio.
 * =====================================================================================================================
 */

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include "SDL.h"
#include "SDL_image.h"
#include "wand/magick_wand.h"
#include "config.h"

/*
 * =====================================================================================================================
 * structs
 * =====================================================================================================================
 */

/* This linked list is intended to hold the list of all files in the source directory. */
typedef struct FILELIST {
        struct FILELIST * next; /* Pointer to next struct */
        struct FILELIST * prev; /* Pointer to previous struct */
        char * path;            /* Absolute path to file */
        int img_h;              /* Image vertical dimensions in pixels */
        int img_w;              /* Image horizontal dimensions in pixels */
        int sel_h;              /* Selection box vertical dimensions in pixels */
        int sel_w;              /* Selection box horizontal dimensions in pixels */
        int sel_x;              /* Selection box horizontal offset in pixels */
        int sel_y;              /* Selection box vertical offset in pixels */
        int valid_sdl;          /* Set to 1 when image has been verified by SDL */
        int valid_imagick;      /* Set to 1 when image has been verified by ImageMagick */
} FILE_LIST;

/*
 * This struct will contain the santized (and properly typed) command line arguments from argv[].
 * Paths in this struct should NOT contain trailing slashes.
 */
typedef struct CMDLINEARGS {
        char * src;
        char * dst;
        double aspect;
        char * archive;
} CMD_LINE_ARGS;

/* Since these pointers tend to go together, pack them in a struct to simplify function declarations. */
typedef struct SDLPOINTERS {
        SDL_Window * window;
        SDL_Renderer * renderer;
        SDL_Texture * texture;
} SDL_POINTERS;

/*
 * =====================================================================================================================
 * function declarations
 * =====================================================================================================================
 */

int process_argv( CMD_LINE_ARGS * cmd_line_args, char ** argv, int argc );
void print_usage( char * argv[] );
char * sanitize_path( char * path );

/*
 * =====================================================================================================================
 * function definitions
 * =====================================================================================================================
 */

void print_usage( char * argv[] ) {
        /* TODO: This version statement is buried in the source code. Make it more visible, perhaps in a #define. */
        printf( "wallproc 0.1.0 (www.subgeniuskitty.com)\n"
                "Usage: %s <source> <destination> <aspect> (archive)\n"
                "  source:      Directory containing images to be processed\n"
                "  destination: Directory to contain modified images\n"
                "  aspect:      Desired aspect ratio of cropped images as a float\n"
                "               Example: 2560x1600 resolution is 16:10 aspect ratio, so aspect would be 1.6\n"
                "  archive:     (optional) Directory to contain copy of modified images in their unmodified state\n"
                , argv[0] );
}


/* Removes trailing slash and verifies path exists. If so, copies path and returns pointer. Otherwise, returns NULL. */
char * sanitize_path( char * path ) {
        /* Remove trailing slash */
        int last_char = strlen(path) - 1;
        if( path[last_char] == '/' ) {
                path[last_char] = '\0';
        }

        /* Verify directory exists and can be accessed under our current environment. */
        DIR * dir = NULL;
        if(( dir = opendir(path)) == NULL ) {
                return NULL;
        }

        /* Copy string to new location, to be considered 'fully sanitized', whatever that really means. */
        int string_length = strlen(path) + 1; // +1 leaves space for null-terminator.
        char * sanitized_string = malloc( string_length );
        if( sanitized_string == NULL ) {
                fprintf( stderr, "ERROR: Failed to malloc for string in function sanitize_path().\n" );
        } else {
                snprintf( sanitized_string, string_length, "%s", path );
        }

        
        return sanitized_string;
}

/* Sanitizes arguments passed in via argv[], storing the result in a CMD_LINE_ARGS struct. */
/* Returns 1 on program-halting error, otherwise 0. */
int process_argv( CMD_LINE_ARGS * cmd_line_args, char ** argv, int argc ) {
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

        /* If it was specified, sanitize the archive directory. */
        if( argc == 5 ) {
                cmd_line_args->archive = sanitize_path( argv[4] );
                if( cmd_line_args->archive == NULL ) ret_val = 1;
        }

        if( SGK_DEBUG ) {
                printf( "DEBUG: Processing command line arguments.\n" );
                printf( "DEBUG: Aspect Ratio = %f\n", cmd_line_args->aspect );
                printf( "DEBUG: Source = %s\n", cmd_line_args->src );
                printf( "DEBUG: Destination = %s\n", cmd_line_args->dst );
                if( argc == 5 ) {
                        printf( "DEBUG: Archive = %s\n", cmd_line_args->archive );
                }
        }

        return ret_val;
}

int main( int argc, char * argv[] ) {
        /* Check for the right number of command line arguments. */
        if( argc != 5 && argc != 4 ) {
                fprintf( stderr, "ERROR: Incorrect number of arguments.\n" );
                print_usage(argv);
                exit(EXIT_FAILURE);
        }
        CMD_LINE_ARGS cmd_line_args = {"", "", 0.0, ""};
        if( process_argv( &cmd_line_args, argv, argc ) ) {
                fprintf( stderr, "ERROR: Unable to process command line arguments.\n" );
                exit(EXIT_FAILURE);
        }


        exit(EXIT_SUCCESS);
}
