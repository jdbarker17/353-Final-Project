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

#include "connect_four.h"
#include "lcd.h"

char individual_1[] = "Liam Duffy";
PS2_DIR_t Current_Direction = PS2_DIR_CENTER;
PS2_DIR_t Previous_Direction = PS2_DIR_CENTER;

volatile uint16_t X_VAL;
volatile uint16_t Y_VAL;
volatile bool ALERT_SAMPLE;
volatile bool SW2_PRESSED;
volatile bool WAS_SW2_PRESSED;
volatile TTT_SQUARE_STATUS_t board[41];
volatile uint8_t CURRENT_SPOT;
volatile int PREVIOUS_SPOT;
volatile int COUNT_MOVES;
bool taken;
bool WON;
TTT_SQUARE_STATUS_t CURRENT_PLAYER;


typedef enum 
{
  DEBOUNCE_ONE,
  DEBOUNCE_1ST_ZERO,
  DEBOUNCE_2ND_ZERO,
	DEBOUNCE_3RD_ZERO,
	DEBOUNCE_4TH_ZERO,
	DEBOUNCE_5TH_ZERO,
	DEBOUNCE_6TH_ZERO,
	DEBOUNCE_7TH_ZERO,
  DEBOUNCE_PRESSED
} DEBOUNCE_STATES;
 /******************************************************************************
* Function Name: debounce_sw2
********************************************************************************
*******************************************************************************/
bool debounce_sw2(void)
{
//*****************************************************************************
// 
//*****************************************************************************
  static DEBOUNCE_STATES state = DEBOUNCE_ONE;
  bool pin_logic_level;
 
	pin_logic_level = lp_io_read_pin(SW2_BIT);
	
  switch (state)
  {
    case DEBOUNCE_ONE:
    {
      if(pin_logic_level)
        state = DEBOUNCE_ONE;
      else
        state = DEBOUNCE_1ST_ZERO;
      break;
    }
    case DEBOUNCE_1ST_ZERO:
    {
      if(pin_logic_level)
        state = DEBOUNCE_ONE;
      else
        state = DEBOUNCE_2ND_ZERO;
      break;
    }
		case DEBOUNCE_2ND_ZERO:
    {
      if(pin_logic_level)
        state = DEBOUNCE_ONE;
      else
        state = DEBOUNCE_3RD_ZERO;
      break;
    }
		case DEBOUNCE_3RD_ZERO:
    {
      if(pin_logic_level)
        state = DEBOUNCE_ONE;
      else
        state = DEBOUNCE_4TH_ZERO;
      break;
    }
		case DEBOUNCE_4TH_ZERO:
    {
      if(pin_logic_level)
        state = DEBOUNCE_ONE;
      else
        state = DEBOUNCE_5TH_ZERO;
      break;
    }
		case DEBOUNCE_5TH_ZERO:
    {
      if(pin_logic_level)
        state = DEBOUNCE_ONE;
      else
        state = DEBOUNCE_6TH_ZERO;
      break;
    }
		
    case DEBOUNCE_6TH_ZERO:
    {
      if(pin_logic_level)
        state = DEBOUNCE_ONE;
      else
        state = DEBOUNCE_7TH_ZERO;
      break;
    }
		
		    case DEBOUNCE_7TH_ZERO:
    {
      if(pin_logic_level)
        state = DEBOUNCE_ONE;
      else
        state = DEBOUNCE_PRESSED;
      break;
    }
		
    case DEBOUNCE_PRESSED:
    {
      if(pin_logic_level)
        state = DEBOUNCE_ONE;
      else
        state = DEBOUNCE_PRESSED;
      break;
    }
    default:
    {
      while(1){};
    }
  }
 
  if(state == DEBOUNCE_7TH_ZERO)
    return true;
  else
    return false;
}



/*******************************************************************************
* Function Name: TIMER5A_Handler
********************************************************************************
*******************************************************************************/
void TIMER5A_Handler(void)
{
	 // Set the Channel
  ADC0->SSMUX2 = (PS2_X_ADC_CHANNEL | (PS2_Y_ADC_CHANNEL << 4));
  
	// Enable SS2
  ADC0->ACTSS |= ADC_ACTSS_ASEN2;  	
	
	// Start SS2
	ADC0->PSSI  =   ADC_PSSI_SS2;    
	
	//Switch debounce logic ***FIX
	SW2_PRESSED = debounce_sw2();
   	
	//acknowledges/clears Timer A timeout
	TIMER5->ICR |= TIMER_ICR_TATOCINT;
}

/*******************************************************************************
* Function Name: ADC0SS2_Handler
********************************************************************************
*******************************************************************************/
void ADC0SS2_Handler(void)
{
	ALERT_SAMPLE = true;
	
	X_VAL = ADC0->SSFIFO2;   
  Y_VAL = ADC0->SSFIFO2;
	
	// ACKNOWLEDGE the conversion, CLEARS THE INTERRUPT
  ADC0->ISC  = ADC_ISC_IN2;		
}


/*******************************************************************************
* Function Name: tic_tac_toe_hw_init
********************************************************************************
*******************************************************************************/  
void tic_tac_toe_hw_init(void)
{
	uint32_t ticks = 50000;
	
	//Initialize GPIO pins for SW2
	gpio_enable_port(GPIOF_BASE);
	gpio_config_digital_enable(GPIOF_BASE, SW2_M);
	gpio_config_enable_input(GPIOF_BASE, SW2_M);
	gpio_config_enable_pullup(GPIOF_BASE, SW2_M);
	gpio_config_alternate_function(GPIOF_BASE, SW2_M);
	
	
	// Configure ADC0 sample sequencer #2 to generate a 
	// single interrupt after BOTH channels have been converted.
	ps2_initialize();
	initialize_adc(ADC0_BASE);
	gpio_enable_port(ADC0_BASE);
	gpio_config_analog_enable(ADC0_BASE, SW2_M);
	gpio_config_enable_input(ADC0_BASE, SW2_M);

	
	//Configure timer to 32 bit, periodic (1ms), countdown Timer5A 
	gp_timer_config_32(TIMER5_BASE, TIMER_TAMR_TAMR_PERIOD, false, true);
	gp_timer_wait(TIMER5_BASE, 50000);
	
	NVIC_EnableIRQ(TIMER5A_IRQn);
	NVIC_SetPriority(TIMER5A_IRQn, 0);

	
	NVIC_EnableIRQ(ADC0SS2_IRQn);
	NVIC_SetPriority(ADC0SS2_IRQn, 0);
	
	//set interval of Timer A to 50000 to create 1ms rest for timer
	TIMER5->TAILR = (ticks);
	
	//start Timer A
	TIMER5->CTL &= !(TIMER_CTL_TAEN|TIMER_CTL_TBEN);
	TIMER5->CTL |= TIMER_CTL_TAEN;

	
	//Configure the LCD screen
	lcd_config_gpio();
	lcd_config_screen();
  return;
}


 /*******************************************************************************
* Function Name: tic_tac_toe_game_init
********************************************************************************
*******************************************************************************/ 
void tic_tac_toe_game_init(TTT_SQUARE_STATUS_t active_player)
{
	int i;
	for(i=0;i<42;i++)
	{
		board[i] = NONE;
	}	
	
	SW2_PRESSED = false;
	ALERT_SAMPLE = false;
  // Modify this function as needed.  The code below is used only to show you how to draw the 
  // lines for the Tic Tac Toe board.
	
	CURRENT_PLAYER = active_player;
	COUNT_MOVES = 0;
	//Initialize the square spots on the board as empty
  lcd_clear_screen(LCD_COLOR_BLACK);
  
	
  // Horizontal Lines (Uncomment to draw lines)
  lcd_draw_rectangle_centered(SCREEN_CENTER_X, LINE_LENGTH, UPPER_HORIZONTAL_LINE_Y,LINE_WIDTH,LCD_COLOR_BLUE);
  lcd_draw_rectangle_centered(SCREEN_CENTER_X,LINE_LENGTH,LOWER_HORIZONTAL_LINE_Y,LINE_WIDTH,LCD_COLOR_BLUE);
  
  //Vertical Lines (Uncomment to draw lines)
  lcd_draw_rectangle_centered(LEFT_HORIZONTAL_LINE_X,LINE_WIDTH, SCREEN_CENTER_Y,LINE_LENGTH,LCD_COLOR_BLUE);
  lcd_draw_rectangle_centered(RIGHT_HORIZONTAL_LINE_X,LINE_WIDTH,SCREEN_CENTER_Y,LINE_LENGTH,LCD_COLOR_BLUE);

	
  tic_tac_toe_update_game_board(PS2_DIR_INIT);
}

//ANYTHING BELOW THIS LINE WAS COPIED INTO A FILE
/*******************************************************************************
********************************************************************************
********************************************************************************
********************************************************************************
********************************************************************************
********************************************************************************
********************************************************************************
********************************************************************************
********************************************************************************
*******************************************************************************/


/*******************************************************************************
* Function Name: tic_tac_toe_update_game_board.
********************************************************************************
*******************************************************************************/
void tic_tac_toe_update_game_board(PS2_DIR_t direction)
{
	
	Current_Direction = direction;	x
//	if( Current_Direction != Previous_Direction)
//	{
		Previous_Direction = Current_Direction;
		
		if(Current_Direction == PS2_DIR_INIT)
		{
		CURRENT_SPOT = 4;
		PREVIOUS_SPOT = 0;
		}
		else
			perform_move();
			
		
		if(PREVIOUS_SPOT != CURRENT_SPOT)
		{
			if(board[PREVIOUS_SPOT] == NONE)
			{
			clear_square(board[PREVIOUS_SPOT]);
			}
			else
				draw_taken_square();
		}
		
		draw_active_square();
	
	}	
//}


/*******************************************************************************
* Function Name: tic_tac_toe_check_for_win
********************************************************************************
*******************************************************************************/
TTT_GAME_STATUS_t tic_tac_toe_check_for_win(void)
{
	if(COUNT_MOVES<5)
	{
		return ONGOING;
	}
//was else if	
	if(COUNT_MOVES < 10)
	{
		//CHECK FOR X WINS********************************************************
		//VERTICAL
		if((board[0]== X_ENTRY) && (board[3]== X_ENTRY) && (board[6] == X_ENTRY))
		{
			WON = true;
			return X_WINNER;
		}
		if((board[1]== X_ENTRY) && (board[4]== X_ENTRY) && (board[7] == X_ENTRY))
		{
			WON = true;
			return X_WINNER;
		}
		if((board[2]== X_ENTRY) && (board[5]== X_ENTRY) && (board[8] == X_ENTRY))
		{
			WON = true;
			return X_WINNER;
		}
		
		//HORIZONTAL
		if((board[0]== X_ENTRY) && (board[1]== X_ENTRY) && (board[2] == X_ENTRY))
		{
			WON = true;
			return X_WINNER;
		}
		if((board[3]== X_ENTRY) && (board[4]== X_ENTRY) && (board[5] == X_ENTRY))
		{
			WON = true;
			return X_WINNER;
		}
		if((board[6]== X_ENTRY) && (board[7]== X_ENTRY) && (board[8] == X_ENTRY))
		{
			WON = true;
			return X_WINNER;
		}
		
		//DIAGON ALLEY (harry potter joke)
		if((board[0]== X_ENTRY) && (board[4]== X_ENTRY) && (board[8] == X_ENTRY))
		{
			WON = true;
			return X_WINNER;
		}
		
		if((board[2]== X_ENTRY) && (board[4]== X_ENTRY) && (board[6] == X_ENTRY))
		{
			WON = true;
			return X_WINNER;
		}

	//CHECK FOR O WINS************************************************************
	//VERTICAL
		if((board[0]== O_ENTRY) && (board[3]== O_ENTRY) && (board[6] == O_ENTRY))
		{
			WON = true;
			return O_WINNER;
		}
		if((board[1]== O_ENTRY) && (board[4]== O_ENTRY) && (board[7] == O_ENTRY))
		{
			WON = true;
			return O_WINNER;
		}
		if((board[2]== O_ENTRY) && (board[5]== O_ENTRY) && (board[8] == O_ENTRY))
		{
			WON = true;
			return O_WINNER;
		}
		
		//HORIZONTAL
		if((board[0]== O_ENTRY) && (board[1]== O_ENTRY) && (board[2] == O_ENTRY))
		{
			WON = true;
			return O_WINNER;
		}
		if((board[3]== O_ENTRY) && (board[4]== O_ENTRY) && (board[5] == O_ENTRY))
		{
			WON = true;
			return O_WINNER;
		}
		if((board[6]== O_ENTRY) && (board[7]== O_ENTRY) && (board[8] == O_ENTRY))
		{
			WON = true;
			return O_WINNER;
		}
		
		//DIAGON ALLEY (harry potter joke)
		if((board[0]== O_ENTRY) && (board[4]== O_ENTRY) && (board[8] == O_ENTRY))
		{
			WON = true;
			return O_WINNER;
		}
		
		if((board[2]== O_ENTRY) && (board[4]== O_ENTRY) && (board[6] == O_ENTRY))
		{
			WON = true;
			return O_WINNER;
		}
		if(COUNT_MOVES==9)
		{
			
			PREVIOUS_SPOT = CURRENT_SPOT;
			draw_taken_square();
			return TIE;
		}
  }
	

	return ONGOING;
}


/*******************************************************************************
* Function Name: tic_tac_toe_sw2_pressed

DONE
********************************************************************************
*******************************************************************************/
bool tic_tac_toe_sw2_pressed(void)
{
	if (SW2_PRESSED == true)
	{
		SW2_PRESSED = false;
		return true;
	}
	else 
	{
		return false; 
	}
}


/*******************************************************************************
* Function Name: tic_tac_toe_return_direction
********************************************************************************
*******************************************************************************/
PS2_DIR_t tic_tac_toe_return_direction(uint16_t ps2_x, uint16_t ps2_y)
{
	//Upper and lower bounds based on the 0.41V and 2.6V, and a 3.3V high signal,
	//with a range of values 0:4095
	uint16_t low = 509;
	uint16_t high = 3226;
	
	//RIGHT
	if(ps2_x > high)
	{
		return PS2_DIR_LEFT;
	}
	
	//LEFT
	else if(ps2_x < low)
	{
		return PS2_DIR_RIGHT;
	}	
	
	//UP
	else if(ps2_y > high)
	{
		return PS2_DIR_UP;
	}	
	
	//DOWN
	else if(ps2_y < low)
	{
		return PS2_DIR_DOWN;
	}	
	
	//CENTER	
	else
		return PS2_DIR_CENTER;
	
//	PS2_DIR_UP,
//  PS2_DIR_DOWN,
//  PS2_DIR_LEFT,
//  PS2_DIR_RIGHT,
//  PS2_DIR_CENTER,
//  PS2_DIR_INIT,
}

/*******************************************************************************
* Function Name: tic_tac_toe_claim_square
********************************************************************************
*******************************************************************************/
void tic_tac_toe_claim_square(void)
{
	bool drawn;
	if(board[CURRENT_SPOT] == NONE)
	{
		board[CURRENT_SPOT] = CURRENT_PLAYER;
		draw_taken_square();
		COUNT_MOVES++;
		drawn = true;
	}

	if(drawn == true)
	{		
		if(CURRENT_PLAYER == X_ENTRY)
		{
			CURRENT_PLAYER = O_ENTRY;
		}
		else
			CURRENT_PLAYER = X_ENTRY;
	}
	else
		return;
}



/*******************************************************************************
* Function Name: tic_tac_toe_sample_ready
********************************************************************************
*******************************************************************************/
bool tic_tac_toe_sample_ready(void)
{
	if(ALERT_SAMPLE==true)
	{
		ALERT_SAMPLE = false;
		return true;
	}
	else
		return false;
}




/*******************************************************************************
* Function Name: perform_move
*
*This function is implemented to check whether or not the joystick movement
*results in a valid directional move. 
********************************************************************************
*******************************************************************************/
void perform_move()
{
	
	int current_mod;
	current_mod = (CURRENT_SPOT % 3);
	
	if((current_mod != 0) && (Current_Direction == PS2_DIR_LEFT))
	{
		PREVIOUS_SPOT = CURRENT_SPOT;
		CURRENT_SPOT = (CURRENT_SPOT-1);	
	}
	
	else if((current_mod != 2) && (Current_Direction == PS2_DIR_RIGHT))
	{
		PREVIOUS_SPOT = CURRENT_SPOT;
		CURRENT_SPOT = (CURRENT_SPOT+1);
	}
	else if((CURRENT_SPOT <= 5) && (Current_Direction == PS2_DIR_DOWN))
	{
		PREVIOUS_SPOT = CURRENT_SPOT;
		CURRENT_SPOT = (CURRENT_SPOT + 3);
	}
	else if((CURRENT_SPOT >= 3) && (Current_Direction == PS2_DIR_UP))
	{
		PREVIOUS_SPOT = CURRENT_SPOT;
		CURRENT_SPOT = (CURRENT_SPOT - 3);
	}
	else
		return;
}


/*******************************************************************************
* Function Name: clear_square

Clear square is called every time a move is performed. Assuming the square we 
just moved out of was not previously taken, then it should be cleared. 
********************************************************************************
*******************************************************************************/
void clear_square(TTT_SQUARE_STATUS_t square)
{
	switch(PREVIOUS_SPOT)
	{
		case(0): //upper left
		{
			lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, UPPER_Y, 
														SQUARE_SIZE, FG_COLOR_CLAIMED);
			break; 
		}
		case(1): //upper center
		{
			lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, UPPER_Y, 
														SQUARE_SIZE, FG_COLOR_CLAIMED);
			break; 
		}
		case(2): //upper right
		{
			lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, UPPER_Y, 
														SQUARE_SIZE, FG_COLOR_CLAIMED);
			break; 
		}
		case(3): //middle left
		{
			lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, CENTER_Y, 
														SQUARE_SIZE, FG_COLOR_CLAIMED);	
			break; 
		}
		case(4): //middle center
		{
			lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, CENTER_Y, 
														SQUARE_SIZE, FG_COLOR_CLAIMED);
			break;
		}
		case(5): //middle right
		{
			lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, CENTER_Y, 
														SQUARE_SIZE, FG_COLOR_CLAIMED);
			break;
		}
		case(6): //bottom left
		{
			lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, LOWER_Y, 
														SQUARE_SIZE, FG_COLOR_CLAIMED);	
			break;
		}
		case(7): //bottom center
		{
			lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, LOWER_Y, 
														SQUARE_SIZE, FG_COLOR_CLAIMED);
			break;
		}
		case(8): //bottom right
		{
			lcd_draw_rectangle_centered(RIGHT_X, SQUARE_SIZE, LOWER_Y, 
														SQUARE_SIZE, FG_COLOR_CLAIMED);
			break;		
		}
		default:
    {
      while(1){};
    }
	}
}

/*******************************************************************************
* Function Name: draw_taken_square

Draw taken square checks the spot that the active cursor just left. If an X or O
player has previously taken this spot, then it should be drawn back to a yellow
X or a cyan O when the active square leaves. 
********************************************************************************
*******************************************************************************/
void draw_taken_square()
{
//GO OFF OF PREVIOUS_SPOT
		if(board[PREVIOUS_SPOT] == O_ENTRY)
		{
			switch(PREVIOUS_SPOT)
			{
				case(0): //upper left
				{
					lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, UPPER_Y, 
																SQUARE_SIZE, BG_COLOR_O);
					lcd_draw_image(LEFT_X, O_WIDTH, UPPER_Y, O_HEIGHT, Bitmaps_O, 
												FG_COLOR_O, BG_COLOR_O);
					break;
				}
				case(1): //upper center
				{
					lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, UPPER_Y, 
																SQUARE_SIZE, BG_COLOR_O);
					lcd_draw_image(CENTER_X, O_WIDTH, UPPER_Y, O_HEIGHT, Bitmaps_O, 
												FG_COLOR_O, BG_COLOR_O);
					break;
				}
				case(2): //upper right
				{
					lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, UPPER_Y, 
																SQUARE_SIZE, BG_COLOR_O);
					lcd_draw_image(RIGHT_X, O_WIDTH, UPPER_Y, O_HEIGHT, Bitmaps_O, 
												FG_COLOR_O, BG_COLOR_O);
					break;
				}
				case(3): //middle left
				{
					lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, CENTER_Y, 
																SQUARE_SIZE, BG_COLOR_O);	
					lcd_draw_image(LEFT_X, O_WIDTH, CENTER_Y, O_HEIGHT, Bitmaps_O, 
												FG_COLOR_O, BG_COLOR_O);
					break;
					
				}
				case(4): //middle center
				{
					lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, CENTER_Y, 
																SQUARE_SIZE, BG_COLOR_O);
					lcd_draw_image(CENTER_X, O_WIDTH, CENTER_Y, O_HEIGHT, Bitmaps_O, 
												FG_COLOR_O, BG_COLOR_O);
					break;
				}
				case(5): //middle right
				{
					lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, CENTER_Y, 
																SQUARE_SIZE, BG_COLOR_O);
					lcd_draw_image(RIGHT_X, O_WIDTH, CENTER_Y, O_HEIGHT, Bitmaps_O, 
												FG_COLOR_O, BG_COLOR_O);
					break;
				}
				case(6): //bottom left
				{
					lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, LOWER_Y, 
																SQUARE_SIZE, BG_COLOR_O);	
					lcd_draw_image(LEFT_X, O_WIDTH, LOWER_Y, O_HEIGHT, Bitmaps_O, 
												FG_COLOR_O, BG_COLOR_O);
					break;
				}
				case(7): //bottom center
				{
					lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, LOWER_Y, 
																SQUARE_SIZE, BG_COLOR_O);
					lcd_draw_image(CENTER_X, O_WIDTH, LOWER_Y, O_HEIGHT, Bitmaps_O, 
												FG_COLOR_O, BG_COLOR_O);
					break;
				}
				case(8): //bottom right
				{
					lcd_draw_rectangle_centered(RIGHT_X, SQUARE_SIZE, LOWER_Y, 
																SQUARE_SIZE, BG_COLOR_O);	
					lcd_draw_image(RIGHT_X, O_WIDTH, LOWER_Y, O_HEIGHT, Bitmaps_O, 
												FG_COLOR_O, BG_COLOR_O);
					break;
				}
				default:
			{
				while(1){};
			}
			
			}
		}
			
		if(board[PREVIOUS_SPOT] == X_ENTRY)
		{
			switch(PREVIOUS_SPOT)
			{
				case(0): //upper left
				{
					lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, UPPER_Y, 
																SQUARE_SIZE, BG_COLOR_X);
					lcd_draw_image(LEFT_X, X_WIDTH, UPPER_Y, X_HEIGHT, Bitmaps_X, 
												FG_COLOR_X, BG_COLOR_X);
					break;
				}
				case(1): //upper center
				{
					lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, UPPER_Y, 
																SQUARE_SIZE, BG_COLOR_X);
					lcd_draw_image(CENTER_X, X_WIDTH, UPPER_Y, X_HEIGHT, Bitmaps_X, 
												FG_COLOR_X, BG_COLOR_X);
					break;
				}
				case(2): //upper right
				{
					lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, UPPER_Y, 
																SQUARE_SIZE, BG_COLOR_X);
					lcd_draw_image(RIGHT_X, X_WIDTH, UPPER_Y, X_HEIGHT, Bitmaps_X, 
												FG_COLOR_X, BG_COLOR_X);
					break;
				}
				case(3): //middle left
				{
					lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, CENTER_Y, 
																SQUARE_SIZE, BG_COLOR_X);	
					lcd_draw_image(LEFT_X, X_WIDTH, CENTER_Y, X_HEIGHT, Bitmaps_X, 
												FG_COLOR_X, BG_COLOR_X);
					break;
				}
				case(4): //middle center
				{
					lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, CENTER_Y, 
																SQUARE_SIZE, BG_COLOR_X);
					lcd_draw_image(CENTER_X, X_WIDTH, CENTER_Y, X_HEIGHT, Bitmaps_X, 
												FG_COLOR_X, BG_COLOR_X);
					break;
				}
				case(5): //middle right
				{
					lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, CENTER_Y, 
																SQUARE_SIZE, BG_COLOR_X);
					lcd_draw_image(RIGHT_X, X_WIDTH, CENTER_Y, X_HEIGHT, Bitmaps_X, 
												FG_COLOR_X, BG_COLOR_X);
					break;
				}
				case(6): //bottom left
				{
					lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, LOWER_Y, 
																SQUARE_SIZE, BG_COLOR_X);	
					lcd_draw_image(LEFT_X, X_WIDTH, LOWER_Y, X_HEIGHT, Bitmaps_X, 
												FG_COLOR_X, BG_COLOR_X);
					break;
				}
				case(7): //bottom center
				{
					lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, LOWER_Y, 
																SQUARE_SIZE, BG_COLOR_X);
					lcd_draw_image(CENTER_X, X_WIDTH, LOWER_Y, X_HEIGHT, Bitmaps_X, 
												FG_COLOR_X, BG_COLOR_X);
					break;
				}
				case(8): //bottom right
				{
					lcd_draw_rectangle_centered(RIGHT_X, SQUARE_SIZE, LOWER_Y, 
																SQUARE_SIZE, BG_COLOR_X);	
					lcd_draw_image(RIGHT_X, X_WIDTH, LOWER_Y, X_HEIGHT, Bitmaps_X, 
												FG_COLOR_X, BG_COLOR_X);
					break;
				}
				default:
				{
					while(1){};
				}
			}
		}
/*		if(WON)
		{
			return;
		}*/
		draw_active_square();
	}

/*******************************************************************************
* Function Name: draw_active_square
********************************************************************************
*******************************************************************************/
void draw_active_square()
{
//	if(WON)
//		return;
	if((CURRENT_PLAYER == O_ENTRY) && (board[CURRENT_SPOT] == NONE))
	{
		switch(CURRENT_SPOT)
		{
			case(0): //upper left
			{
				lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, UPPER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);
				lcd_draw_image(LEFT_X, O_WIDTH, UPPER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				
				break;
			}
			case(1): //upper center
			{
				lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, UPPER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);
				lcd_draw_image(CENTER_X, O_WIDTH, UPPER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(2): //upper right
			{
				lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, UPPER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);
				lcd_draw_image(RIGHT_X, O_WIDTH, UPPER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(3): //middle left
			{
				lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, CENTER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);	
				lcd_draw_image(LEFT_X, O_WIDTH, CENTER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(4): //middle center
			{
				lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, CENTER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);
				lcd_draw_image(CENTER_X, O_WIDTH, CENTER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(5): //middle right
			{
				lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, CENTER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);
				lcd_draw_image(RIGHT_X, O_WIDTH, CENTER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(6): //bottom left
			{
				lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, LOWER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);	
				lcd_draw_image(LEFT_X, O_WIDTH, LOWER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(7): //bottom center
			{
				lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, LOWER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);
				lcd_draw_image(CENTER_X, O_WIDTH, LOWER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(8): //bottom right
			{
				lcd_draw_rectangle_centered(RIGHT_X, SQUARE_SIZE, LOWER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);	
				lcd_draw_image(RIGHT_X, O_WIDTH, LOWER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			default:
			{
				while(1){};
			}
		}
	}
	
	if((CURRENT_PLAYER == X_ENTRY) && (board[CURRENT_SPOT] == NONE))
	{
		switch(CURRENT_SPOT)
		{
			case(0): //upper left
			{
				lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, UPPER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);
				lcd_draw_image(LEFT_X, X_WIDTH, UPPER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(1): //upper center
			{
				lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, UPPER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);
				lcd_draw_image(CENTER_X, X_WIDTH, UPPER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(2): //upper right
			{
				lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, UPPER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);
				lcd_draw_image(RIGHT_X, X_WIDTH, UPPER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(3): //middle left
			{
				lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, CENTER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);	
				lcd_draw_image(LEFT_X, X_WIDTH, CENTER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(4): //middle center
			{
				lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, CENTER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);
				lcd_draw_image(CENTER_X, X_WIDTH, CENTER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(5): //middle right
			{
				lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, CENTER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);
				lcd_draw_image(RIGHT_X, X_WIDTH, CENTER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(6): //bottom left
			{
				lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, LOWER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);	
				lcd_draw_image(LEFT_X, X_WIDTH, LOWER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(7): //bottom center
			{
				lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, LOWER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);
				lcd_draw_image(CENTER_X, X_WIDTH, LOWER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			case(8): //bottom right
			{
				lcd_draw_rectangle_centered(RIGHT_X, SQUARE_SIZE, LOWER_Y, 
															SQUARE_SIZE, BG_COLOR_UNCLAIMED);	
				lcd_draw_image(RIGHT_X, X_WIDTH, LOWER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_UNCLAIMED);
				break;
			}
			default:
			{
				while(1){};
			}
		}
	}
	
	if((board[CURRENT_SPOT] != NONE))
	{
		draw_letter_red(board[CURRENT_SPOT]);
	}
}



/*******************************************************************************
* Function Name: draw_letter_red
*				draw a red letter on the square number passed in
********************************************************************************
*******************************************************************************/
void draw_letter_red(TTT_SQUARE_STATUS_t square)
{
	if(square == O_ENTRY)
	{
		switch(CURRENT_SPOT)
		{
			case(0): //upper left
			{
				lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, UPPER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);
				lcd_draw_image(LEFT_X, O_WIDTH, UPPER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
					break;
				}
			case(1): //upper center
			{
				lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, UPPER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);
				lcd_draw_image(CENTER_X, O_WIDTH, UPPER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break;
			}
			case(2): //upper right
			{
				lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, UPPER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);
				lcd_draw_image(RIGHT_X, O_WIDTH, UPPER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break;
			}
			case(3): //middle left
			{
				lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, CENTER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);	
				lcd_draw_image(LEFT_X, O_WIDTH, CENTER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break;
			}
			case(4): //middle center
			{
				lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, CENTER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);
				lcd_draw_image(CENTER_X, O_WIDTH, CENTER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break;
			}
			case(5): //middle right
			{
				lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, CENTER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);
				lcd_draw_image(RIGHT_X, O_WIDTH, CENTER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break;
			}
			case(6): //bottom left
			{
				lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, LOWER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);	
				lcd_draw_image(LEFT_X, O_WIDTH, LOWER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break;
			}
			case(7): //bottom center
			{
				lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, LOWER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);
				lcd_draw_image(CENTER_X, O_WIDTH, LOWER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break;
			}
			case(8): //bottom right
			{
				lcd_draw_rectangle_centered(RIGHT_X, SQUARE_SIZE, LOWER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);	
				lcd_draw_image(RIGHT_X, O_WIDTH, LOWER_Y, O_HEIGHT, Bitmaps_O, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break;
			}
			default:
			{
				while(1){};
			}
		}
	}
	
	if(square == X_ENTRY)
	{
		switch(CURRENT_SPOT)
		{
			case(0): //upper left
			{
				lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, UPPER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);
				lcd_draw_image(LEFT_X, X_WIDTH, UPPER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break; 
			}
			case(1): //upper center
			{
				lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, UPPER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);
				lcd_draw_image(CENTER_X, X_WIDTH, UPPER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break; 
			}
			case(2): //upper right
			{
				lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, UPPER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);
				lcd_draw_image(RIGHT_X, X_WIDTH, UPPER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break; 
			}
			case(3): //middle left
			{
				lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, CENTER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);	
				lcd_draw_image(LEFT_X, X_WIDTH, CENTER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break; 
			}
			case(4): //middle center
			{
				lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, CENTER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);
				lcd_draw_image(CENTER_X, X_WIDTH, CENTER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break; 
			}
			case(5): //middle right
			{
				lcd_draw_rectangle_centered(RIGHT_X,SQUARE_SIZE, CENTER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);
				lcd_draw_image(RIGHT_X, X_WIDTH, CENTER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break; 
			}
			case(6): //bottom left
			{
				lcd_draw_rectangle_centered(LEFT_X,SQUARE_SIZE, LOWER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);	
				lcd_draw_image(LEFT_X, X_WIDTH, LOWER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break; 
			}
			case(7): //bottom center
			{
				lcd_draw_rectangle_centered(CENTER_X,SQUARE_SIZE, LOWER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);
				lcd_draw_image(CENTER_X, X_WIDTH, LOWER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break; 
			}
			case(8): //bottom right
			{
				lcd_draw_rectangle_centered(RIGHT_X, SQUARE_SIZE, LOWER_Y, 
															SQUARE_SIZE, BG_COLOR_CLAIMED);	
				lcd_draw_image(RIGHT_X, X_WIDTH, LOWER_Y, X_HEIGHT, Bitmaps_X, 
											FG_COLOR_UNCLAIMED, BG_COLOR_CLAIMED);
				break; 
			}
			default:
			{
      while(1){};
			}
		}
	}	
	
}




/*******************************************************************************
* Function Name: is_valid_spot
*				return true if the square is not claimed
********************************************************************************
*******************************************************************************
bool is_valid_spot()
{
	if(board[CURRENT_SPOT] == NONE)
	{
		return true;
	}
	else
		return false;
}
*/
