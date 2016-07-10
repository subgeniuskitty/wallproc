/* See LICENSE file for copyright and license details. */

#include "SDL.h"

#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

typedef struct FILELIST {
        struct FILELIST * next; /* Pointer to next struct */
        struct FILELIST * prev; /* Pointer to previous struct */
        char * path;            /* Full filename/path (ex: /path/to/file.png) */
        char * file;            /* Just the filename */
        int img_h;              /* Image vertical dimensions in pixels */
        int img_w;              /* Image horizontal dimensions in pixels */
        int sel_h;              /* Selection box vertical dimensions in pixels */
        int sel_w;              /* Selection box horizontal dimensions in pixels */
        int sel_x;              /* Selection box horizontal offset in pixels */
        int sel_y;              /* Selection box vertical offset in pixels */
        int sel_r;              /* Selection box red color value from 0-255 */
        int sel_g;              /* Selection box green color value from 0-255 */
        int sel_b;              /* Selection box blue color value from 0-255 */
        int sel_a;              /* Selection box alpha value from 0-255 */
        double aspect;          /* Desired aspect ratio */
        int valid_sdl;          /* Set to 1 when image has been verified by SDL */
        int valid_imagick;      /* Set to 1 when image has been verified by ImageMagick */
        int id;                 /* Image ID number. Unique and assigned sequentially */
} FILE_LIST;

typedef struct CMDLINEARGS {
        char * src;
        char * dst;
        double aspect;
} CMD_LINE_ARGS;

typedef struct SDLPOINTERS {
        SDL_Window * window;
        SDL_Renderer * renderer;
        SDL_Texture * texture;
} SDL_POINTERS;

typedef struct INITPOINTERS {
        struct FILELIST * file_list;
        struct CMDLINEARGS * cmd_line_args;
        struct SDLPOINTERS * sdl_pointers;
} INIT_POINTERS;

typedef enum DIRECTION {
        none,
        up,
        left,
        down,
        right
} DIRECTION;

#endif
