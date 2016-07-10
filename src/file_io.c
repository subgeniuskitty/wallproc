/* See LICENSE file for copyright and license details. */

#include <stdio.h>
#include <dirent.h>
#include "wand/magick_wand.h"
#include "config.h"
#include "data_structures.h"
#include "file_io.h"

void clear_filelist_struct( FILE_LIST * ent ) {
        ent->next = NULL;
        ent->prev = NULL;
        ent->path = NULL;
        ent->file = NULL;
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
        ent->id = 0;
}

FILE_LIST * build_file_list( char * source, double aspect ) {
        DIR * dir = NULL;
        struct dirent * ent = NULL;
        int count = 0;

        if( SGK_DEBUG ) printf( "DEBUG: Building file list -- Using directory: %s\n", source );

        /* 
         * Pointers to the first and last FILE_LIST structs.
         * In order to form a loop, we will need to join them.
         */
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
                                        temp->file = malloc( strlen(ent->d_name) + 1 );
                                        if( temp->path == NULL || temp->file == NULL) {
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
                                                /* Copy string containing only the filename. */
                                                snprintf( temp->file, strlen(ent->d_name)+1, ent->d_name );
                                                /* Add the desired aspect ratio field. */
                                                temp->aspect = aspect;
                                                /* Add the image count field */
                                                temp->id = count;
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
                                                count += 1;
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

void del_file_from_list( FILE_LIST * file_list ) {
        if( SGK_DEBUG ) printf( "DEBUG: Removing file from list: %s\n", file_list->path );
        /* Make the next element in the file_list loop point to the previous element and vice versa. */
        file_list->prev->next = file_list->next;
        file_list->next->prev = file_list->prev;
        /* Since we have isolated this file_list element, we can now delete it. */
        free(file_list->path);
        free(file_list->file);
        free(file_list);
}

void del_img( FILE_LIST * file_list, CMD_LINE_ARGS * cmd_line_args ) {
        /* Build the destination path. */
        int len = strlen( cmd_line_args->dst ) + strlen( file_list->file ) + 1 + 1; /* +2 for '/' separator and '\0' */
        char * path = malloc( len );
        if( path == NULL ) {
                fprintf( stderr, "ERROR: Unable to malloc for file path string.\n" );
                return;
        } else {
                snprintf( path, len, "%s/%s", cmd_line_args->dst, file_list->file );
        }

        /* Delete the file */
        int temp = remove( path );
        if( temp != 0 ) fprintf( stderr, "WARN: Unable to delete image: %s\n", path );

        /* Debug info */
        if( SGK_DEBUG ) {
                printf( "DEBUG: Deleting image %s", path );
                if( temp == 0 ) {
                        printf( " -- success\n" );
                } else {
                        printf( " -- failure\n" );
                }
        }

        /* Clean up */
        free( path );
}

void crop_save( FILE_LIST * file_list, CMD_LINE_ARGS * cmd_line_args ) {
        if( SGK_DEBUG ) printf( "DEBUG: Cropping and saving image.\n" );

        /* Open the file */
        MagickBooleanType magick_status;
        MagickWand * magick_wand = NewMagickWand();
        magick_status = MagickReadImage( magick_wand, file_list->path );
        if( magick_status == MagickFalse ) {
                fprintf( stderr, "ERROR:  -- Failed to open file: %s\n", file_list->path );
                return;
        }

        /* Build the destination path */
        int len = strlen( cmd_line_args->dst ) + strlen( file_list->file ) + 1 + 1; /* +2 for '/' separator and '\0' */
        char * dest_path = malloc( len );
        if( dest_path == NULL ) {
                fprintf( stderr, "ERROR: Unable to malloc for destination string.\n" );
                return;
        } else {
                snprintf( dest_path, len, "%s/%s", cmd_line_args->dst, file_list->file );
        }

        /* Crop the image */
        magick_status = MagickCropImage( magick_wand, file_list->sel_w, file_list->sel_h, 
                                         file_list->sel_x, file_list->sel_y );
        if( magick_status == MagickFalse ) fprintf( stderr, "WARN: Problem cropping image.\n" );
        magick_status = MagickSetImagePage( magick_wand, file_list->sel_w, file_list->sel_h, 0, 0 );
        if( magick_status == MagickFalse ) fprintf( stderr, "WARN: Problem setting image page geometry.\n" );
        magick_status = MagickWriteImage( magick_wand, dest_path );
        if( magick_status == MagickFalse ) fprintf( stderr, "WARN: Problem saving cropped image.\n" );

        /* Clean up */
        free( dest_path );
        DestroyMagickWand( magick_wand );
}

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
