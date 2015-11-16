/*
 * =====================================================================================================================
 * See LICENSE file for copyright and license details.
 *
 * wp_minsize: Sort images based on pixel count
 * =====================================================================================================================
 */

#include "data_structures.h"
#include "config.h"
#include "file_io.h"
#include "wand/magick_wand.h"

int main( int argc, char * argv[] ) {

        if( argc != 3 ) {
                printf( "min_size %d.%d (www.subgeniuskitty.com)\n"
                        "Usage: %s <source> <size>\n"
                        "  source:      Directory containing images to be processed\n"
                        "    size:      Minimum acceptable image size, in megapixels, as a float\n"
                        , VER_MAJOR, VER_MINOR, argv[0] );
                exit(EXIT_FAILURE);
        }

        /*
         * Variables/Initialization
         */

        double min_size = strtof( argv[2], NULL );
        char * path = sanitize_path( argv[1] );
        FILE_LIST * file_list = build_file_list( path, 1.0 );
        free(path);
        FILE_LIST * last_element = file_list->prev;
        MagickBooleanType status;
        int w, h;
        double img_size;
        MagickWandGenesis();
        MagickWand * wand = NewMagickWand();

        /*
         * Main program loop
         */

        while( file_list != last_element ) {
                status = MagickReadImage( wand, file_list->path );
                if( status == MagickTrue ) {
                        w = MagickGetImageWidth( wand );
                        h = MagickGetImageHeight( wand );
                        img_size = (w * h) / 1000000.0;
                        if( img_size < min_size ) {
                                printf( "%s\n", file_list->path );
                        }
                }
                file_list = file_list->next;
        }

        /*
         * Free memory, close subsystems and exit.
         */
        
        DestroyMagickWand( wand );
        MagickWandTerminus();
        exit(EXIT_SUCCESS);
}
