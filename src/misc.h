/* See LICENSE file for copyright and license details. */

#ifndef MISC_H
#define MISC_H

/* 
 * Prints command-line usage, not GUI usage.
 */
void print_usage( char * argv[] );

/* 
 * Processes a single SDL event (user command), initiating whatever action the event requires.
 * Returns NULL if program should terminate, otherwise returns FILE_LIST* to most current file_list.
 */
FILE_LIST * process_sdl_event( SDL_Event * event, FILE_LIST * file_list, 
                SDL_POINTERS * sdl_pointers, CMD_LINE_ARGS * cmd_line_args );

#endif
