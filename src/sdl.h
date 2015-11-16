/* See LICENSE file for copyright and license details. */

#ifndef SDL_H
#define SDL_H

/* 
 * Set SDL window solid gray.
 */
int sdl_clear( SDL_POINTERS * sdl_pointers );

/* 
 * Initializes SDL subsystem and stores relevant pointers in SDL_POINTERS struct.
 * Returns 1 on program-halting error, otherwise 0.
 */
int sdl_init( SDL_POINTERS * sdl_pointers );

/* 
 * Attempts to load the file referenced in 'file_list' with SDL.
 * On failure, removes file_list from the FILE_LIST struct loop.
 * On success, sets file_list->valid_sdl = 1.
 */
void sdl_test( FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );

/* 
 * Populates 'rect' based on file_list such that image aspect ratio is retained and image fills the SDL window.
 */
void sdl_texture_rect( SDL_Rect * rect, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );

#endif
