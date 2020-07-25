//------------------------------------------------------------------------
//
// OpenDCC - OpenDecoder2.2
//
// Copyright (c) 2011, Pras
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at
// http://www.gnu.org/licenses/gpl.txt
// 
//------------------------------------------------------------------------
//
// file:      timer_led.c
// author:    Aiko Pras
// history:   2011-05-05 V0.1 ap based upon port_engine.c from the OpenDecoder2 project
//
//------------------------------------------------------------------------

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <avr/pgmspace.h>        // put var to program memory
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <string.h>

#include "config.h"
#include "myeeprom.h"            // wrapper for eeprom
#include "hardware.h"
#include "dcc_receiver.h"
#include "main.h"
#include "timer_led.h"

//=============================================================================
// 1. Definitions
//=============================================================================

// Define all hardware specific settings at the beginning

// Timer 1 specific settings
#if defined ENHANCED_PROCESSOR
  #define TC1_Interrupt_Mask_Register				TIMSK1				// Register
  #define TC1_Input_Capture_Interrupt_Enable		ICIE1				// Bit definition
#else 
  #define TC1_Interrupt_Mask_Register				TIMSK
  #define TC1_Input_Capture_Interrupt_Enable		TICIE1				// Bit definition
  #define TC1_Output_Compare_Match_Interrupt_Enable	OCIE0 
#endif


// Timing Definitions:
#define TICK_PERIOD 20000L       // 20ms tick for Timing Engine
                                 // => possible values for timings up to
                                 //    5.1s (=255/0.020)
                                 // note: this is also used as frame for
                                 // Servo-Outputs (OC1A and OC1B)

//----------------------------------------------------------------------------
// Global Data
//----------------------------
volatile struct
  {
    unsigned char rest;     // Zeit bis zum wechsel in Ticks (20ms)
    unsigned char ontime;   // Einschaltzeit
    unsigned char offtime;  // Ausschaltzeit
    unsigned char pause;    
    unsigned char flashes;  // Anzahl Pulse
    unsigned char act_flash;    
  } led;


void turn_led_on(void)
  {
    led.rest = 0;
    LED_ON;
  }

void turn_led_off(void)
  {
    led.rest = 0;
    LED_OFF;
  }


// make a series of flashes, then a longer pause
void flash_led_fast(unsigned char count)
  {
    led.act_flash = 1;
    led.flashes = count;
    led.pause   = 700000L / TICK_PERIOD;
    led.offtime = 240000L / TICK_PERIOD;
    led.ontime  = 120000L / TICK_PERIOD;
    led.rest    = 120000L / TICK_PERIOD;
    LED_ON;
  }



static inline void disable_timer_interrupt(void)   __attribute__((always_inline));
void disable_timer_interrupt(void)
  {
      TC1_Interrupt_Mask_Register &= ~(1<<TOIE1);        // Timer1 Overflow
  }

static inline void enable_timer_interrupt(void)   __attribute__((always_inline));
void enable_timer_interrupt(void) 
  {
      TC1_Interrupt_Mask_Register |= (1<<TOIE1);        // Timer1 Overflow
  }

//==============================================================================
//
// Section 2
//
//------------------------------------------------------------------------------
//
// init_timer1
//   a) setup timer (with TICK_PERIOD)

void init_timer1(void)
  {
    // Init Timer1 as Fast PWM with a CLKDIV (prescaler) of 8

    #define T1_PRESCALER   8    // may be 1, 8, 64, 256, 1024
                                // see also servo.c
    #if   (T1_PRESCALER==1)
        #define T1_PRESCALER_BITS   ((0<<CS12)|(0<<CS11)|(1<<CS10))
    #elif (T1_PRESCALER==8)
        #define T1_PRESCALER_BITS   ((0<<CS12)|(1<<CS11)|(0<<CS10))
    #elif (T1_PRESCALER==64)
        #define T1_PRESCALER_BITS   ((0<<CS12)|(1<<CS11)|(1<<CS10))
    #elif (T1_PRESCALER==256)
        #define T1_PRESCALER_BITS   ((1<<CS12)|(0<<CS11)|(0<<CS10))
    #elif (T1_PRESCALER==1024)
        #define T1_PRESCALER_BITS   ((1<<CS12)|(0<<CS11)|(1<<CS10))
    #endif


    // check TICK_PERIOD and F_CPU

    #if (F_CPU / 1000000L * TICK_PERIOD / T1_PRESCALER) > 65535L
      #warning: overflow in ICR1 - check TICK_PERIOD and F_CPU
      #warning: suggestion: use a larger T1_PRESCALER
    #endif    
    #if (F_CPU / 1000000L * TICK_PERIOD / T1_PRESCALER) < 5000L
      #warning: resolution accuracy in ICR1 too low - check TICK_PERIOD and F_CPU
      #warning: suggestion: use a smaller T1_PRESCALER
    #endif    

    // Timer 1 runs in FAST-PWM-Mode with ICR1 as TOP-Value (WGM13:0 = 14).
    // note: due to a bug in AVRstudio this can't be simulated !!

    ICR1 = F_CPU / 1000000L * TICK_PERIOD / T1_PRESCALER ;  

    OCR1A = F_CPU / 1000000L * TICK_PERIOD / T1_PRESCALER / 20;  // removed 24.12.2008 ???
    OCR1B = F_CPU / 1000000L * TICK_PERIOD / T1_PRESCALER / 15;   

    TCCR1A = (0 << COM1A1)          // compare match A
           | (0 << COM1A0)          // not activated yet -> this is done in init_servo();
           | (0 << COM1B1)          // compare match B
           | (0 << COM1B0) 
           | 0                      // reserved
           | 0                      // reserved
           | (1 << WGM11)  
           | (0 << WGM10);          // Timer1 Mode 14 = FastPWM - Int on Top (ICR1)
    TCCR1B = (0 << ICNC1) 
           | (0 << ICES1) 
           | (1 << WGM13) 
           | (1 << WGM12) 
           | (T1_PRESCALER_BITS);   // clkdiv

    TC1_Interrupt_Mask_Register |= (1<<TOIE1)             // Timer1 Overflow
           | (0<<OCIE1A)            // Timer1 Compare A
           | (0<<OCIE1B)            // Timer1 Compare B
           | 0                      // reserved
           | (0<<TC1_Input_Capture_Interrupt_Enable);     // Timer1 Input Capture
      

    timerval = 0;
  }


//==============================================================================
//
// Section 3
//
// Timing Engine
//
// Howto:    Timer1 (with prescaler 8 and 16 bit total count) triggers
//           an interrupt every TICK_PERIOD (=20ms @8MHz);
// 
  

ISR(TIMER1_OVF_vect)                        // Timer1 Overflow Int
  {
    
    disable_timer_interrupt();
    
    sei();                                  // allow DCC interrupt

    timerval++;                             // advance global clock

    // now process led

    if (led.rest != 0)
      {
        if (--led.rest == 0)
          {
             if (LED_STATE)
              {
                if (led.act_flash == led.flashes)
                  {
                    led.rest = led.pause;
                    led.act_flash = 0;
                  }
                else
                  {
                    led.rest = led.offtime;
                  }
                LED_OFF;
              }
            else
              {
                led.act_flash++;
                led.rest = led.ontime;
                LED_ON;
              }
          }
      }
    enable_timer_interrupt();
  }



