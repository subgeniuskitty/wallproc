/* See LICENSE file for copyright and license details. */

#ifndef STARTUP_SHUTDOWN_H
#define STARTUP_SHUTDOWN_H

/* 
 * Initialize everything. No return since this function exits the program on failure.
 * This functions mallocs memory.
 */
void initialize( INIT_POINTERS * init_pointers, int argc, char * argv[] );

/* 
 * Sanitizes arguments passed in via argv[], storing the result in a CMD_LINE_ARGS struct.
 * Returns 1 on program-halting error, otherwise 0.
 */
int process_argv( CMD_LINE_ARGS * cmd_line_args, char ** argv );

/* 
 * Frees relevant memory and prepares program for imminent termination.
 * No return since we intend to exit promptly after this function is finished.
 */
void terminate( CMD_LINE_ARGS * cmd_line_args, FILE_LIST * file_list, SDL_POINTERS * sdl_pointers );

#endif
