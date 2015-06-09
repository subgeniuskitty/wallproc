/* See LICENSE file for copyright and license details. */

/*
 * =====================================================================================================================
 * keybindings
 * =====================================================================================================================
 */

/* 
 * To change keybindings, change the corresponding #define to the desired SDL keycode.
 * SDL keycode reference located at: https://wiki.libsdl.org/SDL_Keycode
 */

#define KEY_NEXT SDLK_PERIOD                    // Using period since it's the same key as '>', a forward arrow.
#define KEY_PREV SDLK_COMMA                     // Using comma since it's the same key as '<', a back arrow.

#define KEY_SIZEUP SDLK_KP_PLUS                 // Increase selection box size
#define KEY_SIZEDOWN SDLK_KP_MINUS              // Decrease selection box size

                                                // Move selection box:
#define KEY_UP SDLK_KP_8                        //   Up
#define KEY_DOWN SDLK_KP_2                      //   Down
#define KEY_LEFT SDLK_KP_4                      //   Left
#define KEY_RIGHT SDLK_KP_6                     //   Right

#define KEY_SELECTIONBOX_RESET SDLK_KP_0        // Reset size and location of selection box to defaults

#define KEY_QUIT SDLK_q                         // Exit this application
#define KEY_HELP SDLK_h                         // Pops up a dialog box with key commands after the GUI has launched. 
#define KEY_TOGGLE_OUTLINE_COLOR SDLK_KP_5      // Toggles the selection box color between light and dark.

/*
 * =====================================================================================================================
 * misc
 * =====================================================================================================================
 */

/*
 * Multiplicative step size for selection box resizing. 
 * Example: A value of 0.05 means the selection box will scale up or down 
 * in size by 5% each time the relevant key is depressed. 
 */
#define SELECT_SIZE_MULT 0.05

/* Set to 1 to enable debugging info on console. */
#define SGK_DEBUG 1 
