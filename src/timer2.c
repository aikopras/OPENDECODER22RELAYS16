//------------------------------------------------------------------------
//
// file:      timer2.c
//
// purpose:   Timer 2 usage for the relays decoder
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at http://www.gnu.org/licenses/gpl.txt
//
// history:   2011-05-05 V0.1 First version
//
//------------------------------------------------------------------------

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <avr/pgmspace.h>        // put var to program memory
#include <avr/io.h>              // needed for UART
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/parity.h>

#include "config.h"              // general definitions the decoder, cv's
#include "hardware.h"            // port definitions for target

#include "timer2.h"
#include "main.h"

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------

// This software should run on the older AVRs (8535, 16 & 32), as well as their enhanced versions
// (164A, 324A and 644). For more info, see also Atmel's application note AVR505 (Table 7.1).
// To allow the same software to run on multiple AVRs, we define a number of processor specific
// aliases below. In the remainder of the software we refer to these aliases.
// Note: ENHANCED_PROCESSOR is defined in hardware.h 

// Interrupt specific settings
#if defined ENHANCED_PROCESSOR
  #define Interrupt_Select_Register					EIMSK    			// External Interrupt Mask Register
  #define Interrupt_Control_Register				EICRA   			// External Interrupt Control Register
#else 
  #define Interrupt_Select_Register					GICR    			// General Interrupt Control Register
  #define Interrupt_Control_Register				MCUCR   			// MCU Control Register
#endif

// Timer 2 specific settings
#if defined ENHANCED_PROCESSOR
  #define TC2_Compare_Match_Vect					TIMER2_COMPA_vect	// Interrupt vector
  #define TC2_Output_Compare_Register				OCR2A				// Register
  #define TC2_Interrupt_Mask_Register				TIMSK2				// Register
  #define TC2_Control_Register_A					TCCR2A				// Register
  #define TC2_Control_Register_B					TCCR2B				// Register
  #define TC2_Output_Compare_Match_Interrupt_Enable	OCIE2A 				// Bit definition
#else 
  #define TC2_Compare_Match_Vect					TIMER2_COMP_vect
  #define TC2_Output_Compare_Register				OCR2
  #define TC2_Interrupt_Mask_Register				TIMSK
  #define TC2_Control_Register_A					TCCR2				// Note: A and B are
  #define TC2_Control_Register_B					TCCR2				// here the same register
  #define TC2_Output_Compare_Match_Interrupt_Enable	OCIE2 
#endif

//--------------------------------------------------------------------------------------
//
// Define global variables that provide the interface between rs_bus_hardware and rs_bus
//
//--------------------------------------------------------------------------------------
// Global Data: 
volatile unsigned char T2_Seconds;			 // Seconds counted by timer 2

// local variables
unsigned int T2_MilliSeconds;
//--------------------------------------------------------------------------------------
//
// Define Interrupt Service routines (ISR) for Timer2
//
//--------------------------------------------------------------------------------------

ISR(TC2_Compare_Match_Vect)
{
  // This ISR is called whenever Timer2, which is set to roughly 1 ms, fires
  TCNT2 = 0;					// Reset counter 2 (this counter)
  T2_MilliSeconds++;            // Another millisecond has passed
  if (T2_MilliSeconds > 999) {  // Another second has passed
    T2_Seconds++;
    T2_MilliSeconds = 0;
    }
} 


//--------------------------------------------------------------------------------------
//
// Define initialisation routines
//
//--------------------------------------------------------------------------------------

void init_timer2(void)
{
  // See for example Figure 17-2 in ATMega 16A manual
  // The following registers must be initialized:
  // - Timer/Counter Control Register (TCCR2 / TCCR2A & TCCR2B)
  // - Timer/Counter (TCNT2)
  // - Output Compare Register (OCR2 / OCR2A)
  // TCNT2 increases, till it reaches OCR2(A)
  // IF (TCNT2 == OCR2(A)) => interrupt
  //
  // The following settings are needed
  // 1) calculate how far Timer2 should count before an interrupt is raised
  // 2) determine a prescaler value (to be stored later in TCCR2 / TCCR2B)
  // 3) check if this value is between 32 and 254 (note: we have an 8 bit timer!)
  // 4) set this value in the Output Compare Register
  // 5) enable interrupts for Timer/Counter2 Compare Matches (OCIE2 / OCIE2A)
  // 6) initialise the Timer/Counter Control Register
  // 7) Initialise a counter to determine if the master is inactive / resets
  // 8) initialise this Timer/Counter
  //
  // Step 1: Define timer period
  #define time_microsonds 1000L		// timer fires after 1 ms
    // Step 2: Determine prescaler
  #define T2_PRESCALER   256    // may be 1, 8, 32, 64, 128, 256, 1024
  #if   (T2_PRESCALER==1)
        #define T2_PRESCALER_BITS   ((0<<CS22)|(0<<CS21)|(1<<CS20))
  #elif (T2_PRESCALER==8)
        #define T2_PRESCALER_BITS   ((0<<CS22)|(1<<CS21)|(0<<CS20))
  #elif (T2_PRESCALER==32)
        #define T2_PRESCALER_BITS   ((0<<CS22)|(1<<CS21)|(1<<CS20))
  #elif (T2_PRESCALER==64)
        #define T2_PRESCALER_BITS   ((1<<CS22)|(0<<CS21)|(0<<CS20))
  #elif (T2_PRESCALER==128)
        #define T2_PRESCALER_BITS   ((1<<CS22)|(0<<CS21)|(1<<CS20))
  #elif (T2_PRESCALER==256)
        #define T2_PRESCALER_BITS   ((1<<CS22)|(1<<CS21)|(0<<CS20))
  #elif (T2_PRESCALER==1024)
        #define T2_PRESCALER_BITS   ((1<<CS22)|(1<<CS21)|(1<<CS20))
  #endif
  // Step 3: Check prescaler
  // Pre-processor check whether timer values are OK for 8 bit
  // Target Timer Count = (Input Frequency * Target Time / Prescale) - 1 
  #define T2_target_count (F_CPU / T2_PRESCALER * time_microsonds / 1000000L)
  #if (T2_target_count > 254)
    #warning T2_target_count too big, use either larger prescaler or slower processor
  #endif
  #if (T2_target_count < 32)
    #warning T2_target_count too small, use either smaller prescaler or faster processor
  #endif
  // Step 4: Initialize the Output Compare Register
  TC2_Output_Compare_Register = T2_target_count; 
  // Step 5: Enable the Timer/Counter2 Compare Match interrupt
  TC2_Interrupt_Mask_Register |= (1 << TC2_Output_Compare_Match_Interrupt_Enable);
  // Step 6: Initialize the Timer/Counter Control Register
  TC2_Control_Register_A |= (1 << WGM21);          // Configure Timer2 for CTC mode 
  TC2_Control_Register_B |= (T2_PRESCALER_BITS);   // Start Timer2
  // Step 7: Intialise timer specific variable
  T2_MilliSeconds = 0;
  // Step 8: Initialise the Timer/Counter
  TCNT2 = 0;  
}



//------------------------------------------------------------------------------------------------

