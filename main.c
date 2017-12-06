// Copyright (c) 2015-16, Joe Krachey
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

#include "main.h"
#include "hw3_lib.h"


extern bool validate_hw3(bool test);

//*****************************************************************************
//*****************************************************************************
int 
main(void)
{
  uint16_t x,y;
  char msg[80];
  
  TTT_GAME_STATUS_t game_status = ONGOING;
  TTT_SQUARE_STATUS_t starting_letter = O_ENTRY;
  
  ece353_hw3_init_serial();

  put_string("\n\r");
  put_string("************************************\n\r");
  put_string("ECE353 - Fall 2017 HW3\n\r  ");
  sprintf(msg,"\n\r     Name: %s",individual_1);
  put_string(msg);
  put_string("\n\r");  
  put_string("************************************\n\r");

    tic_tac_toe_hw_init();
 put_string("************************************\n\r");
    // To run functional test 0, change the parameter to true
    validate_hw3(true);
  
    tic_tac_toe_game_init(starting_letter);


  // Reach infinite loop
  while(1){
    
    if( tic_tac_toe_sample_ready())
    { 
      // Read the ADC to determine the position of the PS2 Joystick
      x = ps2_get_x();
      y = ps2_get_y();
      
			
			
/*		if the global boolean is true, Reset_IRQn to false, return true
			otherwise return false
					
			debounce logic goes in the handler, a helper function if you want. 
			make sure the debounce is only being checked once ever 1ms, using the handler. 
			set the global boolean inside the handler with debounce logic
*/			
			
			

      // Get the current direction the Joystick has been pressed.
      // The valid directions are UP, DOWN, LEFT, RIGHT, and CENTER.
      Current_Direction = tic_tac_toe_return_direction(x,y);
      
      // Only update the screen if the current direction and previous
      // direction were not the same.
      if( Current_Direction != Previous_Direction)
      {
        tic_tac_toe_update_game_board(Current_Direction);
        
        Previous_Direction = Current_Direction;
      }
      
      // Check to see if SW2 was pressed.  If it was, log the 
      // entry from the current player.  Check to see if the resulting
      // move wins the game or results in a tie.  
      //
      // If the game ends, print out the corresponding message to the user
      // and wait until the user presses SW2.  Pressing SW2 should reset the
      // game.  Be sure to switch the player, X or O, that begins each game.
      if( tic_tac_toe_sw2_pressed())
      {
        tic_tac_toe_claim_square();
        
        game_status = tic_tac_toe_check_for_win();
        
        if( game_status != ONGOING)
        {
          if(game_status == X_WINNER)
          {
            ece353_hw3_lcd_X_wins();
          }
          else if ( game_status == O_WINNER)
          {
            ece353_hw3_lcd_O_wins();
          }
          
          else
          {
            ece353_hw3_lcd_tie();
          }
          
          while(!tic_tac_toe_sw2_pressed()){
            // busy wait
          };
          
          if(starting_letter == O_ENTRY)
          {
            starting_letter = X_ENTRY;
          }
          else
          {
            starting_letter = O_ENTRY;
          }
          
          tic_tac_toe_game_init(starting_letter);
          
        }
      }
    }
      
  };
}
