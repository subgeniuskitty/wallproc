/* See LICENSE file for copyright and license details. */

#ifndef UI_H
#define UI_H

/* 
 * Updates titlebar of SDL window for currently displayed image.
 */
void update_titlebar( FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );

/* 
 * Draws the next/prev/current image (based on 'dir') and returns a FILE_LIST* to the file_list that was drawn.
 */
FILE_LIST * draw( DIRECTION dir, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );

#endif
