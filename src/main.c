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

/* This struct is intended for use in a linked-list of all files in the source directory. */
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

/* Holds pointers to each of the other structs. Should only be used during initialization, to simplify main(). */
typedef struct INITPOINTERS {
        struct FILELIST * file_list;
        struct CMDLINEARGS * cmd_line_args;
        struct SDLPOINTERS * sdl_pointers;
} INIT_POINTERS;

/*
 * =====================================================================================================================
 * function declarations
 * =====================================================================================================================
 */

int process_argv( CMD_LINE_ARGS * cmd_line_args, char ** argv, int argc );
void print_usage( char * argv[] );
char * sanitize_path( char * path );
FILE_LIST * build_file_list( char * source );
void clear_filelist_struct( FILE_LIST * ent );
int sdl_init( SDL_POINTERS * sdl_pointers );
int imagick_init( void );
int sdl_clear( SDL_POINTERS * sdl_pointers );
void initialize( INIT_POINTERS * init_pointers, int argc, char * argv[] );
void terminate( CMD_LINE_ARGS * cmd_line_args, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );

/*
 * =====================================================================================================================
 * function definitions
 * =====================================================================================================================
 */

void clear_filelist_struct( FILE_LIST * ent ) {
        ent->next = NULL;
        ent->prev = NULL;
        ent->path = "";
        ent->img_h = 0;
        ent->img_w = 0;
        ent->sel_h = 0;
        ent->sel_w = 0;
        ent->sel_x = 0;
        ent->sel_y = 0;
        ent->valid_sdl = 0;
        ent->valid_imagick = 0;
}

/* Builds list of files in source. Non-recursive. */
/* Returns NULL if no items found. Otherwise returns a double-linked list of FILE_LIST structs. */
FILE_LIST * build_file_list( char * source ) {
        DIR * dir = NULL;
        struct dirent * ent = NULL;

        if( SGK_DEBUG ) printf( "DEBUG: Building file list -- Using directory: %s\n", source );

        /* Pointers to the first and last FILE_LIST structs. In order to form a loop, we will need to join them. */
        FILE_LIST * first = NULL;
        FILE_LIST * last = NULL;


        if(( dir = opendir(source)) != NULL ) {
                while(( ent = readdir(dir)) != NULL ) {
                        if( ent->d_type == DT_REG ) { /* tests if ent is a regular file, not symlink/etc */
                                FILE_LIST * temp = malloc( sizeof( FILE_LIST ) );
                                if( temp == NULL ) {
                                        /* 
                                         * Print the error, but take no other action.
                                         * Since temp == NULL, this entry is not added to the list. 
                                         */
                                        fprintf( stderr, "ERROR: Unable to malloc for FILE_LIST entry.\n" );
                                } else {
                                        clear_filelist_struct( temp );
                                        int len = strlen(ent->d_name)   /* filename */
                                                + strlen(source)        /* path to file, relative to PWD */
                                                + 1                     /* +1 for '/' between path and filename */
                                                + 1;                    /* +1 for terminating null character */
                                        temp->path = malloc( len );
                                        if( temp->path == NULL ) {
                                                /*
                                                 * Print the error, but also free temp and set temp = NULL.
                                                 * This ensures we don't add a bogus entry to the list.
                                                 */
                                                fprintf( stderr, "ERROR: Unable to malloc for path in FILE_LIST.\n" );
                                                free(temp);
                                                temp = NULL;
                                        } else {
                                                /* Copy the string, including the relative path from PWD. */
                                                snprintf( temp->path, len, "%s/%s", source, ent->d_name );
                                        }
                                        if( temp != NULL ) { /* We have a valid entry to add. */
                                                if( first == NULL ) {
                                                        /* Start the list. */
                                                        first = temp;
                                                        last = temp;
                                                } else {
                                                        /* Extend the list, continuing from last. */
                                                        last->next = temp;
                                                        temp->prev = last;
                                                        last = temp;
                                                }
                                        }
                                }
                        }
                }
        }

        /* Close the loop */
        first->prev = last;
        last->next = first;
        
        return first;
}

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
                printf( "DEBUG:  -- Aspect Ratio = %f\n", cmd_line_args->aspect );
                printf( "DEBUG:  -- Source = %s\n", cmd_line_args->src );
                printf( "DEBUG:  -- Destination = %s\n", cmd_line_args->dst );
                if( argc == 5 ) {
                        printf( "DEBUG:  -- Archive = %s\n", cmd_line_args->archive );
                }
        }

        return ret_val;
}

/* Set SDL window solid gray. */
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

/* Initializes SDL subsystem and stores relevant pointers in struct. */
/* Returns 1 on program-halting error, otherwise 0 */
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

/* Initialize ImageMagick subsystem */
int imagick_init( void ) {
        /* ImageMagick doesn't return much feedback from functions. */
        /* Return a value anyway, to match behavior of sdl_init(). */
        MagickWandGenesis();
        return 0;
}

/* Initialize everything. No return since this function exits the program on failure. */
void initialize( INIT_POINTERS * init_pointers, int argc, char * argv[] ) {
        /* Check for the right number of command line arguments. */
        if( argc != 5 && argc != 4 ) {
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
        if( process_argv( init_pointers->cmd_line_args, argv, argc ) ) {
                fprintf( stderr, "ERROR: Unable to process command line arguments.\n" );
                exit(EXIT_FAILURE);
        }
        /* Build the list of images in 'source' command line argument. */
        init_pointers->file_list = build_file_list( init_pointers->cmd_line_args->src );
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

/* Frees relevant memory and prepares program for imminent termination. */
/* No return since we intend to exit promptly after this function is finished. */
void terminate( CMD_LINE_ARGS * cmd_line_args, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers ) {
        /* Free memory related to command line arguments. */
        free( cmd_line_args->src );
        free( cmd_line_args->dst );
        free( cmd_line_args->archive );
        free( cmd_line_args );

        /* Free memory related to list of files. */
        FILE_LIST * current = file_list->next;
        file_list->next = NULL;
        while( current->next != NULL ) {
                free( current->path );
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

int main( int argc, char * argv[] ) {
        /*
         * Create the main data structures and perform all required initialization.
         */

        /* The init_pointers struct is only intended to keep main() clean. */
        INIT_POINTERS * init_pointers = malloc(sizeof(INIT_POINTERS));
        if( init_pointers == NULL ) {
                fprintf( stderr, "ERROR: Unable to malloc for init_pointers struct.\n" );
                exit(EXIT_FAILURE);
        }
        /* Initializes everything that needs it. */
        initialize( init_pointers, argc, argv );
        /* Contains sanitized and typed command line arguments. */
        CMD_LINE_ARGS * cmd_line_args = init_pointers->cmd_line_args;
        /* Will point to current element of a cyclic, double-linked list of files in 'source'. */
        FILE_LIST * file_list = init_pointers->file_list;
        /* Contains window, renderer and texture pointers for SDL. */
        SDL_POINTERS * sdl_pointers = init_pointers->sdl_pointers;
        /* We are now finished with the init_pointers struct. */
        free( init_pointers );


        /* Temporary. Add a delay so I can see SDL window. Remove after adding user input loop. */
        sleep(3);

        /*
         * Free memory, close subsystems and exit.
         */

        terminate( cmd_line_args, file_list, sdl_pointers );
        exit(EXIT_SUCCESS);
}
