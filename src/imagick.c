/* See LICENSE file for copyright and license details. */

#include "wand/magick_wand.h"
#include "data_structures.h"
#include "config.h"
#include "file_io.h"
#include "selection_box.h"
#include "imagick.h"

int imagick_init( void ) {
        /* ImageMagick doesn't return much feedback from functions. */
        /* Return a value anyway, to match behavior of sdl_init(). */
        MagickWandGenesis();
        return 0;
}

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
                magick_wand = DestroyMagickWand( magick_wand );
        }
}
