// Copyright (c) 2015-17, Joe Krachey
// All rights reserved.
//
// Redistribution and use in source or binary form, with or without modification, 
// are permitted provided that the following conditions are met:
//
// 1. Redistributions in source form must reproduce the above copyright 
//    notice, this list of conditions and the following disclaimer in 
//    the documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#ifndef __TIC_TAC_TOE_H__
#define __TIC_TAC_TOE_H__

#include "main.h"
#include "ps2.h"
#include "timers.h"
#include "tic_tac_toe_images.h"

#define SCREEN_X            240
#define SCREEN_Y            320

#define SCREEN_CENTER_X    ((SCREEN_X/2)-1)
#define SCREEN_CENTER_Y    ((SCREEN_Y/2)-1)

#define LINE_WIDTH          11
#define LINE_LENGTH         (SCREEN_X - 1)

#define SQUARE_SIZE         64

#define PADDING             4

#define LEFT_X                      (SCREEN_CENTER_X - SQUARE_SIZE - (2*PADDING) - LINE_WIDTH)
#define CENTER_X                    (SCREEN_CENTER_X)
#define RIGHT_X                     (SCREEN_CENTER_X + SQUARE_SIZE + (2*PADDING) + LINE_WIDTH)

#define UPPER_Y                     (SCREEN_CENTER_Y - SQUARE_SIZE - (2*PADDING) - LINE_WIDTH)
#define CENTER_Y                    (SCREEN_CENTER_Y)
#define LOWER_Y                     (SCREEN_CENTER_Y + SQUARE_SIZE + (2*PADDING) + LINE_WIDTH)

#define UPPER_HORIZONTAL_LINE_Y     (SCREEN_CENTER_Y - (SQUARE_SIZE/2) - PADDING - LINE_WIDTH/2)
#define LOWER_HORIZONTAL_LINE_Y     (SCREEN_CENTER_Y + (SQUARE_SIZE/2) + PADDING + LINE_WIDTH/2)

#define LEFT_HORIZONTAL_LINE_X      (SCREEN_CENTER_X - (SQUARE_SIZE/2) - PADDING - LINE_WIDTH/2)
#define RIGHT_HORIZONTAL_LINE_X     (SCREEN_CENTER_X + (SQUARE_SIZE/2) + PADDING + LINE_WIDTH/2)

#define FG_COLOR_X                LCD_COLOR_YELLOW
#define BG_COLOR_X                LCD_COLOR_BLACK
#define FG_COLOR_O                LCD_COLOR_CYAN
#define BG_COLOR_O                LCD_COLOR_BLACK

#define FG_COLOR_CLAIMED          LCD_COLOR_BLACK
#define BG_COLOR_CLAIMED          LCD_COLOR_RED

#define FG_COLOR_UNCLAIMED        LCD_COLOR_BLACK
#define BG_COLOR_UNCLAIMED        LCD_COLOR_GREEN

typedef enum{
  PS2_DIR_UP,
  PS2_DIR_DOWN,
  PS2_DIR_LEFT,
  PS2_DIR_RIGHT,
  PS2_DIR_CENTER,
  PS2_DIR_INIT,
} PS2_DIR_t;

typedef enum{
  NONE,
  O_ENTRY,
  X_ENTRY
} TTT_SQUARE_STATUS_t;

typedef enum{
  TIE,
  X_WINNER,
  O_WINNER,
  ONGOING
} TTT_GAME_STATUS_t;

extern char individual_1[];

extern PS2_DIR_t Current_Direction;
extern PS2_DIR_t Previous_Direction;

/*******************************************************************************
* Function Name: tic_tac_toe_hw_init 
********************************************************************************
* Summary: Initializes all the hardware resources required for tic tac toe game
*          (GPIO pins, ADC0, Timer5A, etc). 
* Returns:
*  Nothing
*******************************************************************************/ 
void tic_tac_toe_hw_init(void); 

 /*******************************************************************************
* Function Name: tic_tac_toe_game_init 
*
********************************************************************************
* Summary: Initializes all game logic/data structures in your implementation. 
*          This function should call tic_tac_toe_update_game_board()
*          to update the display so that the center square displays the active
*          user's image and the color of that image represents an unclaimed 
*          square.
*           
* Parameters: active_player
*             Determines if the center square is an X or an O.
* Returns:
*  Nothing
*******************************************************************************/ 
void tic_tac_toe_game_init(TTT_SQUARE_STATUS_t active_player);

/*******************************************************************************
* Function Name: tic_tac_toe_update_game_board.
********************************************************************************
* Summary: Checks to see if the LCD should be updated based on PS2 position.
*          
*          If the direction passed in is PS2_DIR_INIT, then initialize the 
*          screen so that all non-center squares display nothing AND the center
*          square should display the unclaimed image for the active player.
*
* Parameters: direction
*             The direction the PS2 joystick has been moved in
* Returns:
*  Nothing
*******************************************************************************/
void tic_tac_toe_update_game_board(PS2_DIR_t direction);


/*******************************************************************************
* Function Name: tic_tac_toe_check_for_win 
********************************************************************************
* Summary: Checks to see if the game has been won, is a tie, or is still ongoing.
*
* Returns:
*  The current status of the game.
*******************************************************************************/
TTT_GAME_STATUS_t tic_tac_toe_check_for_win(void);


/*******************************************************************************
* Function Name: tic_tac_toe_sw2_pressed 
********************************************************************************
* Summary: Used to de-bounce SW2.  SW2 will be de-bounced if the value returned 
*          on the GPIO pin is logic zero (0) for 7 consecutive calls to 
*          tic_tac_toe_sw2_pressed, followed by SW2 being a value of one (1) on 
*          the 8th call to tic_tac_toe_sw2_pressed.  
*
* Returns:
*  True for SW2 initially pressed.  False for all other conditions
*******************************************************************************/
bool tic_tac_toe_sw2_pressed(void);

/*******************************************************************************
* Function Name: tic_tac_toe_return_direction.
********************************************************************************
* Summary: Uses the x and y values of the PS2 joystick to determine which
*          direction the PS2 joystick is currently in.  The voltage returned
*          by the ADCs must be less than 0.41V OR greater than 2.6V in order to 
*          return a direction that is not PS2_DIR_CENTER.  Give priority to 
*          changes in the X direction.
*
* Parameters:  ps2_x 
*                 A 12-bit value that represents X position of the PS2 joystick
*              ps2_y 
*                 A 12-bit value that represents Y position of the PS2 joystick
* Returns:
*  Current direction of PS2 joystick.
*******************************************************************************/
PS2_DIR_t tic_tac_toe_return_direction(uint16_t ps2_x, uint16_t ps2_y);








/*******************************************************************************
* Function Name: tic_tac_toe_claim_square
********************************************************************************
* Summary: Sets the currently active square to an X or an O based which player's
*          turn it is.  The square must be unclaimed to change the status of the
*          square.  If the square is already claimed, do nothing and return.
*
*          If a square is successfully claimed, be sure upate the LCD display 
*          so that the current square is displayed as claimed.
* Returns:
*  Nothing
*******************************************************************************/
void tic_tac_toe_claim_square(void);

 /*******************************************************************************
* Function Name: tic_tac_toe_sample_ready
********************************************************************************
* Summary: Returns if the 1ms timer has generated an interrupt by checking 
*          ALERT_SAMPLE.  If ALERT_SAMPLE is true, it will set ALERT_SAMPLE 
*          to false, and return true.  If ALERT_SAMPLE is false, the function 
*          will return false.
*
* Returns:
*  Nothing
*******************************************************************************/
bool tic_tac_toe_sample_ready(void);


 /*******************************************************************************
* Function Name: debounce_this_bitch
********************************************************************************
* Summary: Implement the debounce logic. Make sure this is called in Timer
* 					Handler to make sure we're checking every 1ms to see if the SW2
*						button is being pressed
*
* Returns:
*  True if button is pressed, else false.
*******************************************************************************/
bool debounce_sw2(void);


/*******************************************************************************
* Function Name: perform_move
*
* This function is implemented to check whether or not the joystick movement
* results in a valid directional move, and if so, moves in that direction.
********************************************************************************
*******************************************************************************/
void perform_move(void);





/*******************************************************************************
* Function Name: execute_move
********************************************************************************
*******************************************************************************/
void execute_move(void);

/*******************************************************************************
* Function Name: clear_square
****passed in a square to clear, switches based off of the square we want to clear. 
********************************************************************************
*******************************************************************************/
void clear_square(TTT_SQUARE_STATUS_t square);



/*******************************************************************************
* Function Name: draw_taken_square
*
* Draws an X or an O appropriately in a claimed spot. 
********************************************************************************
*******************************************************************************/
void draw_taken_square(void);




/*******************************************************************************
* Function Name: active_square

Draws the active square at the appropriate location on the board, based on 
global variables CURRENT_SPOT and PREVIOUS_SPOT
********************************************************************************
*******************************************************************************/
void draw_active_square(void);







/*******************************************************************************
* Function Name: draw_letter_red
*			Draws a red square with the letter of the taken square.
********************************************************************************
*******************************************************************************/
void draw_letter_red(TTT_SQUARE_STATUS_t square);



















/*IF YOU'RE WONDERING WHY I HARD CODED EVERYTHING LIKE A MORON - FOR MOST OF THE
TIME THAT I WAS WRITING THIS PROJECT, MY #ENDIF WAS STUCK IN THE MIDDLE OF MY 
TIC_TAC_TOE.H HEADER FILE. THIS MEANS EVERY TIME I TRIED TO CREATE A NEW FUNCTION 
DEFINITION WITH ARGUMENTS, IT THREW ME AN ERROR. I COULDN'T FIGURE OUT WHY, SO HENCE,
HARD CODING. I KNOW IT'S UGLY. SORRY. */




//HELPER FUNCTIONS THAT ENDED UP NOT BEING USED














/*******************************************************************************
* Function Name: is_valid_move
*
*This function is implemented to check whether or not the joystick movement
*results in a valid directional move. 
********************************************************************************
*******************************************************************************/
//bool is_valid_move(void);





/*******************************************************************************
* Function Name: is_valid_spot
*				return true if the square is not claimed
********************************************************************************
*******************************************************************************/
//bool is_valid_spot(void);



/*******************************************************************************
* taken_square_overlay was doing the same thing as draw_letter_red()
********************************************************************************
*******************************************************************************/

#endif

