// Copyright (c) 2014-16, Joe Krachey
// All rights reserved.
//
// Redistribution and use in binary form, with or without modification, 
// are permitted provided that the following conditions are met:
//
// 1. Redistributions in binary form must reproduce the above copyright 
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

#include "ps2.h"

static volatile uint16_t PS2_X_DATA = 0;
static volatile uint16_t PS2_Y_DATA = 0;
volatile bool ALERT_SAMPLE = false;

//*****************************************************************************
//*****************************************************************************
//                         Configuration
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
// Initializes the ADC GPIO Port
//*****************************************************************************
static void initialize_adc_gpio_pins(void)
{
	gpio_enable_port(PS2_GPIO_ANALOG_BASE);
  gpio_config_enable_input(PS2_GPIO_ANALOG_BASE, PS2_X_DIR_PIN | PS2_Y_DIR_PIN);
  gpio_config_analog_enable(PS2_GPIO_ANALOG_BASE, PS2_X_DIR_PIN | PS2_Y_DIR_PIN);
  gpio_config_alternate_function(PS2_GPIO_ANALOG_BASE, PS2_X_DIR_PIN | PS2_Y_DIR_PIN);
}



/*******************************************************************************
* Function Name: ps2_configure_digital_comparitors
********************************************************************************
* Summary:
* Configures SSMUX2 to convert the raw ADC values.
*
* Return:
*  void
*
*******************************************************************************/
static void ps2_configure_adc(void)
{
 // Configure SSMUX2 to read the raw ADC values
		
  ADC0->SSMUX2 = PS2_X_DIR_CH << 4 | PS2_Y_DIR_CH;  // Set Channels
    
  // Enable the interrupts for SS2
  ADC0->IM |= ADC_IM_MASK2;
  
  // Set the end of the sequence and when to generate interrupts
  ADC0->SSCTL2 = ADC_SSCTL2_IE1 | ADC_SSCTL2_END1;
  
  NVIC_EnableIRQ(ADC0SS2_IRQn);
  NVIC_SetPriority(ADC0SS2_IRQn ,0);
  
  ADC0->ACTSS |= ADC_ACTSS_ASEN2;  // Enable SS2
  
}


//*****************************************************************************
// Initializes the ADC0 to use SS2 to convert both the X and Y pots for the
// PS2 joystick.  The ADC is triggered by TIMER5 interrupts.
//*****************************************************************************
bool ps2_initialize_adc(void)
{

 // Turn on the ADC Clock
  SYSCTL->RCGCADC |= SYSCTL_RCGCADC_R0;
  
  // Wait for ADCx to become ready
  while( (SYSCTL_PRADC_R0 & SYSCTL->PRADC) != SYSCTL_PRADC_R0){}
  
  // disable all the sample sequencers
  ADC0->ACTSS &= ~(ADC_ACTSS_ASEN0 | ADC_ACTSS_ASEN1| ADC_ACTSS_ASEN2| ADC_ACTSS_ASEN3);

	ADC0->ISC = 0xFFFFFFFF;

  // Sequencer 3 is the lowest priority
  ADC0->SSPRI = ADC_SSPRI_SS3_4TH | ADC_SSPRI_SS2_3RD | ADC_SSPRI_SS1_2ND | ADC_SSPRI_SS0_1ST;

	// Set all the sample sequencers to be triggered by software.
  ADC0->EMUX = 0 ;
  
    // Clear Averaging Bits
  ADC0->SAC &= ~ADC_SAC_AVG_M  ;
  
  // Average 8 samples
  ADC0->SAC |= ADC_SAC_AVG_8X;

  ps2_configure_adc();
  
  return true;
}

//*****************************************************************************
// Initializes TIMER 5
//*****************************************************************************
static void initialize_adc_timer(void)
{

   GPTIMER_CONFIG timer_config;
	
   // Set the timer that is used to determine when the ADC
   // will be sampled.
   timer_config.base = TIMER5_BASE;
   timer_config.mode = TIMER_CFG_32_BIT_TIMER;
   // Configure Sub timer A
   timer_config.enableA = true;
   timer_config.countA = PS2_SAMPLE_TICKS;
   timer_config.countModeA = TIMER_TAMR_TAMR_PERIOD;
   timer_config.priorityA = 1;
   timer_config.intEnA = true;
   timer_config.irqNumA = TIMER5A_IRQn;
   // Configure Sub timer B
   timer_config.enableB = false;
   timer_config.countB = 0;
   timer_config.countModeB = 0;
   timer_config.priorityB = 0;
   timer_config.intEnB = false;
   timer_config.irqNumB = TIMER5B_IRQn;

   // Enabled the timers.
   gp_timer_config(&timer_config);
}


//*****************************************************************************
// Used to initialize the PS2 joystick for both the analog inputs and the 
// push button.  
//
// Configuration Info
//		Fill out relevant information in boardUtil.h.  boardUtil.h defines 
//		how various peripherals are physically connected to the board.
//
//	Resources Used
//		ADC
//		The PS2 analog channels will be configured to use SS2 of the selected ADC
//		The ADC will be configured to start on a processor event.  The timer
//		below will be used to generate interrupts and set the correct bit in the 
//		ISR.
//
//		TIMERx
//		A 32-bit timer that will generate periodic interrupts.
//  
//*****************************************************************************
void ps2_initialize(void)
{
	initialize_adc_gpio_pins();
	ps2_initialize_adc();
	initialize_adc_timer();
}

//*****************************************************************************
// Returns the most current reading of the X direction  Only the lower 12-bits
// contain data.
//*****************************************************************************
uint16_t ps2_get_x(void)
{
	return PS2_X_DATA;
}

//*****************************************************************************
// Returns the most current reading of the Y direction.  Only the lower 12-bits
// contain data.
//*****************************************************************************
uint16_t ps2_get_y(void)
{
	return PS2_Y_DATA;
}



//*****************************************************************************
//*****************************************************************************
//                         Interrupt Service Routines
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
// TIMER5 ISR
//*****************************************************************************
void TIMER5A_Handler(void)
{
	// Start the next ADC sample sequencers
	ADC0->PSSI |=   ADC_PSSI_SS2;
	
	// Clear the interrupt
	TIMER5->ICR |= TIMER_ICR_TATOCINT;
}


//*****************************************************************************
// ADC0 SS2 ISR
//*****************************************************************************
void ADC0SS2_Handler(void)
{
  ALERT_SAMPLE = true;
  
  // Read from SS2 FIFO
  PS2_X_DATA = ADC0->SSFIFO2;
	PS2_Y_DATA = ADC0->SSFIFO2;
  
  // Clear the interrupt
  ADC0->ISC |= ADC_ISC_IN2;
}


