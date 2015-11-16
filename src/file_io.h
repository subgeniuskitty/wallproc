/* See LICENSE file for copyright and license details. */

#ifndef FILE_IO_H
#define FILE_IO_H

/*
 * Sets a FILE_LIST struct to default values.
 */
void clear_filelist_struct( FILE_LIST * ent );

/* 
 * Non-recursively builds a linked list of all files in 'source' directory.
 * Returns NULL if no items found; otherwise returns pointer to the linked list.
 * This function mallocs memory.
 */
FILE_LIST * build_file_list( char * source, double aspect );

/*
 * Removes 'file_list' from the linked list and stiches the previous and next entries together.
 */
void del_file_from_list( FILE_LIST * file_list );

/* 
 * In 'cmd_line_args->dst' folder, deletes image specified in 'file_list'.
 */
void del_img( FILE_LIST * file_list, CMD_LINE_ARGS * cmd_line_args );

/*
 * Crops image from 'file_list' according to selection box info in 'file_list'.
 * After cropping, saves image to 'cmd_line_args->dst' folder.
 */
void crop_save( FILE_LIST * file_list, CMD_LINE_ARGS * cmd_line_args );

/*
 * Removes trailing slash and verifies 'path' exists.
 * If so, copies path and returns pointer. Otherwise, returns NULL.
 * This function mallocs memory.
 */
char * sanitize_path( char * path );

#endif 
