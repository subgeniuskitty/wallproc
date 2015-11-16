/* See LICENSE file for copyright and license details. */

#ifndef SELECTION_BOX_H
#define SELECTION_BOX_H

/* 
 * Set defaults for the following elements of the file_list struct: sel_h, sel_w, sel_x, sel_y.
 * Requires that img_h and img_w are already set.
 */
void reset_sel_box( FILE_LIST * file_list );

/* 
 * Toggles selection box color as stored in file_list.
 */
void toggle_selection_color( FILE_LIST * file_list );

/* 
 * Populates 'sel_rect' with dimensions and offset from 'file_list', but scaled to
 * the current SDL window.
 */
void sdl_selection_rect( SDL_Rect * sel_rect, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );

/*
 * Modifies scaled selection box rectangle 'params' such that it respects SDL window dimensions
 * and boundaries for a given image 'file_list'.
 */
void sel_sanitize( SDL_Rect * params, FILE_LIST * file_list );

/* 
 * Changes selection box size and keeps it within image (not window) bounds.
 */
void sel_resize( DIRECTION dir, FILE_LIST * file_list );

/* 
 * Moves selection box within image (not window) bounds.
 */
void sel_move( DIRECTION dir, FILE_LIST * file_list );

#endif
