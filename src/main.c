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
 * type declarations
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
        int sel_r;              /* Selection box red color value from 0-255 */
        int sel_g;              /* Selection box greeb color value from 0-255 */
        int sel_b;              /* Selection box blue color value from 0-255 */
        int sel_a;              /* Selection box alpha value from 0-255 */
        double aspect;          /* Desired aspect ratio */
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

/* Use to improve readability. */
typedef enum DIRECTION {
        none,
        up,
        left,
        down,
        right
} DIRECTION;

/*
 * =====================================================================================================================
 * function declarations
 * =====================================================================================================================
 */

int process_argv( CMD_LINE_ARGS * cmd_line_args, char ** argv, int argc );
void print_usage( char * argv[] );
char * sanitize_path( char * path );
FILE_LIST * build_file_list( char * source, double aspect );
void clear_filelist_struct( FILE_LIST * ent );
int sdl_init( SDL_POINTERS * sdl_pointers );
int imagick_init( void );
int sdl_clear( SDL_POINTERS * sdl_pointers );
void initialize( INIT_POINTERS * init_pointers, int argc, char * argv[] );
void terminate( CMD_LINE_ARGS * cmd_line_args, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );
FILE_LIST * process_sdl_event( SDL_Event * event, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );
void sdl_test( FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );
void imagick_test( FILE_LIST * file_list );
void del_file_from_list( FILE_LIST * file_list );
FILE_LIST * draw( DIRECTION dir, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );
void reset_sel_box( FILE_LIST * file_list );
void sdl_texture_rect( SDL_Rect * rect, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );
void sdl_selection_rect( SDL_Rect * sel_rect, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );
void toggle_selection_color( FILE_LIST * file_list );
void update_titlebar( FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );
void sel_resize( DIRECTION dir, FILE_LIST * file_list );
void sel_move( DIRECTION dir, FILE_LIST * file_list );
void sel_sanitize( SDL_Rect * params, FILE_LIST * file_list );

/*
 * =====================================================================================================================
 * function definitions
 * =====================================================================================================================
 */

void clear_filelist_struct( FILE_LIST * ent ) {
        ent->next = NULL;
        ent->prev = NULL;
        ent->path = NULL;
        ent->img_h = 0;
        ent->img_w = 0;
        ent->sel_h = 0;
        ent->sel_w = 0;
        ent->sel_x = 0;
        ent->sel_y = 0;
        ent->sel_r = 255;
        ent->sel_g = 255;
        ent->sel_b = 255;
        ent->sel_a = 255;
        ent->aspect = 0.0;
        ent->valid_sdl = 0;
        ent->valid_imagick = 0;
}

/* Builds list of files in source. Non-recursive. */
/* Returns NULL if no items found. Otherwise returns a double-linked list of FILE_LIST structs. */
FILE_LIST * build_file_list( char * source, double aspect ) {
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
                                                /* Add the desired aspect ratio field. */
                                                temp->aspect = aspect;
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
        if( first != NULL && last != NULL ) {
                first->prev = last;
                last->next = first;
        }
        
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

/* Removes file_list from the linked list and stiches the previous and next entries together. */
void del_file_from_list( FILE_LIST * file_list ) {
        if( SGK_DEBUG ) printf( "DEBUG: Removing file from list: %s\n", file_list->path );
        /* Make the next element in the file_list loop point to the previous element and vice versa. */
        file_list->prev->next = file_list->next;
        file_list->next->prev = file_list->prev;
        /* Since we have isolated this file_list element, we can now delete it. */
        free(file_list->path);
        free(file_list);
}

/* Attempts to load the file referenced in 'file_list' with SDL. */
/* On failure, removes file_list from the FILE_LIST struct loop. */
/* On success, sets file_list->valid_sdl = 1. */
void sdl_test( FILE_LIST * file_list, SDL_POINTERS * sdl_pointers ) {
        if( SGK_DEBUG ) printf( "DEBUG: Test-loading image with SDL: %s", file_list->path );
        if( file_list->valid_sdl == 1 ) {
                if( SGK_DEBUG ) printf( " -- already tested\n" );
                return;
        }
        sdl_pointers->texture = IMG_LoadTexture( sdl_pointers->renderer, file_list->path );
        if( sdl_pointers->texture == NULL ) {
                if( SGK_DEBUG ) printf( " -- failure\n" );
                del_file_from_list( file_list );
        } else {
                if( SGK_DEBUG ) printf( " -- success\n" );
                file_list->valid_sdl = 1;
        }
}

/* Attempts to load the file referenced in 'file_list' with ImageMagick. */
/* On failure, removes file_list from the FILE_LIST struct loop. */
/* On success, sets file_list->valid_imagick = 1 and relevant selection box values (example: file_list->sel_x). */
void imagick_test( FILE_LIST * file_list ) {
        if( SGK_DEBUG ) printf( "DEBUG: Test-loading image with ImageMagick: %s", file_list->path );
        if( file_list->valid_imagick == 1 ) {
                if( SGK_DEBUG ) printf( " -- already tested\n" );
                return;
        }
        MagickBooleanType magick_status;
        MagickWand * magick_wand = NewMagickWand();
        magick_status = MagickReadImage( magick_wand, file_list->path );
        if( magick_status == MagickFalse ) {
                if( SGK_DEBUG ) printf( " -- failure\n" );
                del_file_from_list( file_list );
        } else {
                if( SGK_DEBUG ) printf( " -- success\n" );
                file_list->img_h = MagickGetImageHeight( magick_wand );
                file_list->img_w = MagickGetImageWidth( magick_wand );
                reset_sel_box( file_list );
                file_list->valid_imagick = 1;
        }
}

/* Set defaults for the following elements of the file_list struct: sel_h, sel_w, sel_x, sel_y. */
/* Requires that img_h and img_w are already set. */
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

/* Populates 'rect' based on file_list such that image aspect ratio is retained and image fills the SDL window. */
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

/* Toggles selection box color as stored in file_list. */
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

/* Populates 'sel_rect' with selection box as stored in file_list, but scaled to current window dimensions. */
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

/* Updates titlebar of SDL window for currently displayed image. */
void update_titlebar( FILE_LIST * file_list, SDL_POINTERS * sdl_pointers ) {
        char * title = NULL;
        int length = snprintf( title, 0, "wallproc -- Selection: %dx%d -- File: %s", 
                        file_list->sel_w, file_list->sel_h, file_list->path );
        title = malloc( length+1 );
        if( title == NULL ) {
                SDL_SetWindowTitle( sdl_pointers->window, "wallproc" );
        } else {
                snprintf( title, length+1, "wallproc -- Selection: %dx%d -- File: %s",
                                file_list->sel_w, file_list->sel_h, file_list->path );
                SDL_SetWindowTitle( sdl_pointers->window, title );
                free(title);
        }
}

/* Draws the next/prev/current image (based on 'dir') and returns a FILE_LIST* to the file_list that was drawn. */
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

/* Changes selection box size and keeps it within upper and lower bounds. */
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

/* Moves selection box within programmed bounds. */
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

/* Processes a single SDL event, initiating whatever action the event requires. */
/* Returns NULL if program should terminate, otherwise returns FILE_LIST* to most current file_list. */
FILE_LIST * process_sdl_event( SDL_Event * event, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers ) {
        switch( event->type ) {
                case SDL_QUIT:
                        file_list = NULL;
                        break;
                case SDL_WINDOWEVENT:
                        // TODO: Should I specify events to handle, or just leave this as a catch-all?
                        //       Test the performance while dragging the window around the desktop with a large image loaded.
                        //       Note: My computer might be a bad example since my WM only draws window outline while moving. Test with a WM that shows window contents while dragging.
                        //       https://wiki.libsdl.org/SDL_WindowEvent
                        //       Also, when maximizing window, SDL sees multiple (~3) events. Why, and what are they?
                        file_list = draw( none, file_list, sdl_pointers );
                        break;
                case SDL_KEYDOWN:
                        switch( event->key.keysym.sym ) {
                                case KEY_QUIT:
                                        file_list = NULL;
                                        break;
                                case KEY_HELP:
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
                        // TODO: Ignore all other actions?
                        break;
        }

        return file_list;
}

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
        
        /*
         * Main program loop
         */

        int quit = 0;
        FILE_LIST * temp = NULL;
        SDL_Event event;
        while( quit == 0 ) {
                if( SDL_WaitEvent( &event ) ) {
                        temp = process_sdl_event( &event, file_list, sdl_pointers );
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
