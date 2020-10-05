//------------------------------------------------------------------------
//
// OpenDCC - OpenDecoder2
//
// Copyright (c) 2006, 2007 Kufer
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at
// http://www.gnu.org/licenses/gpl.txt
// 
//------------------------------------------------------------------------
//
// file:      main.c
// author:    Wolfgang Kufer, modified by Aiko Pras
// contact:   kufer@gmx.de
// webpage:   http://www.opendcc.de
// history:   2011-05-05 V0.01 ap copied from main.c from OpenDecoder2 project
//
//------------------------------------------------------------------------
//
// purpose:   flexible general purpose relays decoder for dcc
//            here: init, mainloop, globals
// For the detailed description, see relays.c
//
//
// ====== >>>> config.h  is the central definition file for the project
//
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

#include "config.h"              // general definitions the decoder, cv's
#include "myeeprom.h"            // wrapper for eeprom
#include "hardware.h"            // port definitions for target
#include "dcc_receiver.h"        // receiver for dcc
#include "dcc_decode.h"          // decode dcc and handle CV access
#include "timer_led.h"           // LED control
#include "keyboard.h"            // button control
#include "timer2.h"              // timer used for relays in round-robin fashion
#include "relays.h"              // handling of relays

#include "relays.h"              // handling of relays

#include "main.h"


//--------------------------------------------------------------------------------------

void init_main(void)
  {
    PORTD = (0<<LED)                   // LED off
          | (0<<RSBUS_TX)              // output default off (UART controlled)
          | (1<<RSBUS_RX)              // 1 = pullup
          | (1<<DCCIN)                 // 1 = pullup
          | (SERVO_INIT_PULS<<SERVO1)  // output (OC1A)  // 23.12.2008 changed to 1
          | (SERVO_INIT_PULS<<SERVO2)  // output (OC1B)
          | (1<<PROGTASTER)            // 1 = pullup
          | (0<<DCC_ACK);              // ACK off

    DDRD  = (1<<LED)            // output
          | (0<<RSBUS_TX)       // input, overwrite later if RSBUS_ENABLED
          | (0<<RSBUS_RX)       // input (INT0)
          | (0<<DCCIN)          // input (INT1)
          | (1<<SERVO1)         // output (OC1B)
          | (1<<SERVO2)         // output (OC1A)
          | (0<<PROGTASTER)     // input    
          | (1<<DCC_ACK);       // output, sending 1 makes an ACK
    
    DDRA  = 0xFF;               // PORTA: All Bits as Output
    DDRB  = 0xFF;               // PortB: All Bits as Output
    DDRC  = 0xFF;               // PORTC: All Bits as Output
       
    PORTA = 0;                  // output: all off
    PORTB = 0;                  // output: all off     
    PORTC = 0;                  // output: all off
  }
  


//------------------------------------------------------------------------
// This Routine is called when PROG is pressed
// -- manual programming and accordingly setting of CV's
//
#define DEBOUNCE  (50000L / TICK_PERIOD)
#if (DEBOUNCE == 0)
 #define DEBOUNCE  1
#endif


void DoProgramming(void)
  {
    signed char my_timerval, myCommand, myMode;

    my_timerval = timerval;
    while(timerval - my_timerval < DEBOUNCE) ;          // wait

    if (PROG_PRESSED)                                   // still pressed?
      {
        turn_led_on();
        
        while(PROG_PRESSED) ;                           // wait for release

        my_timerval = timerval;
        while(timerval - my_timerval < DEBOUNCE) ;      // wait
        
        while(!PROG_PRESSED)
          {
            if (semaphor_get(C_Received))
              {                                         // Message
                if (analyze_message(&incoming))         // yes, any accessory
                  {
                    // write the address in EEPROM
                    my_eeprom_write_byte(&CV.myAddrL, (unsigned char) ReceivedAddr & 0b00111111  );     
                    my_eeprom_write_byte(&CV.myAddrH, (unsigned char) (ReceivedAddr >> 6) & 0b00000111);
                    // write the relays activate command in EEPROM. LH100 "-": 0 / "+": 1
                    my_eeprom_write_byte(&CV.Ract, (unsigned char) ReceivedCommand & 0b00000001);
                    // write the decoder mode in EEPROM (see relays.c for description of modes)
                    myCommand = ReceivedCommand & 0b00000111;
                    myMode = myCommand >> 1;
                    my_eeprom_write_byte(&CV.Mode, myMode);                       
                    // wait for write to complete
                    do {} while (!eeprom_is_ready());
                    
                    LED_OFF;

                    // we got reprogrammed ->
                    // forget everthing running and restart decoder!                    
                    
                    cli();
                    
                    // laut diversen Internetseiten sollte folgender Code laufen -
                    // tuts aber nicht, wenn man das Assemblerlistung ansieht.
                    // void (*funcptr)( void ) = 0x0000;    // Set up function pointer
                    // funcptr();                        // Jump to Reset vector 0x0000
                    
                    __asm__ __volatile 
                    (
                       "ldi r30,0"  "\n\t"
                       "ldi r31,0"  "\n\t"
                       "icall" "\n\t"
                     );
                    
                    // return;  
                  }
              }
          }  // while
        turn_led_off();
        my_timerval = timerval;
        while(timerval - my_timerval < DEBOUNCE) ;     // wait    
        while(PROG_PRESSED) ;       // wait for release
      }
    return;   
  }


//--------------------------------------------------------------------------------------------
int main(void)
  {
    init_main();                                        // setup hardware ports (to do!!)
    init_timer1();                                      // setup timer for LeD
    init_timer2();                                      // setup timer for relays (mode 3)
    init_dcc_receiver();                                // setup dcc receiver
    init_dcc_decode();                                  // setup dcc decoder
    init_keyboard();                                    // local tracers
    init_relays_actions();                              // relays related setup

    if (my_eeprom_read_byte(&CV.myAddrH) & 0x80)        // Check if address has been programmed
      {
        flash_led_fast(5);                              // warning - we are unprogrammed
      }

    sei();                                              // Global enable interrupts

    // Check if the EEPROM has been initialised. In case the program is compiled
    // and flashed using "make flash", the EEPROM should have been initialised during flash.
    // However, the Arduino IDE does not flash the EPPROM during program upload.
    // In that case we need to initialise from here.
    if (my_eeprom_read_byte(&CV.VID) != 0x0D) {
      ResetDecoder();                             // Copy all default values to EEPROM
      _restart();                                 // really hard exit
    }
    
    
    
    while(1)
      {
        if (semaphor_query(C_Received))
          {
            if (analyze_message(&incoming) >= 2)        // MyAddr or greater received
              {
                relays_actions(ReceivedCommand);
              }
            semaphor_get(C_Received);                   // now take away the protection
          }
        if (PROG_PRESSED) DoProgramming();
        relays_round_robin();                           // check if the relays should be changed
      }
  }

