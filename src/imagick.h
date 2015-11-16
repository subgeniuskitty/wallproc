/* See LICENSE file for copyright and license details. */

#ifndef IMAGICK_H
#define IMAGICK_H

/* 
 * Initialize ImageMagick subsystem
 */
int imagick_init( void );

/* 
 * Attempts to load the file referenced in 'file_list' with ImageMagick.
 * On failure, removes file_list from the FILE_LIST struct loop.
 * On success, sets file_list->valid_imagick = 1 and relevant selection box values (example: file_list->sel_x).
 */
void imagick_test( FILE_LIST * file_list );

#endif
