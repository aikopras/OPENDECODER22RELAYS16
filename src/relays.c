//------------------------------------------------------------------------------------------------
//
// OpenDCC - OpenDecoder2.2
//
// Copyright (c) 2011, Pras
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at http://www.gnu.org/licenses/gpl.txt
// 
//------------------------------------------------------------------------------------------------
//
// file:      relays.c
// author:    Aiko Pras
// history:   2011-05-05 V0.1 ap based upon port_engine.c from the OpenDecoder2 project
//
//
// A DCC Relays Decoder for ATmega16A and other AVR.
// Two blocks of each eight relays can be switched; the two blocks are independent from each other
// (although the mode is used for both blocks).
// The application that was in mind while designing this decoder, was to control a number of video
// camera modules with standard video signal, such as sold for example by Conrad for 30 Euros.
//
// The relays decoder can operate in one of the following modes:
// 0: Default operation. An address=x with the ACTIVATE command (the "+" and "-" on the LH100):
//    - Releases all relays in that block
//    - Set the relay with address=x
//    - Note: in this mode there will always be one (and only one) relay closed
//    An address=x without the DEACTIVATE command is ignored (has no impact)
//    However: if such command is received for the last decoder address (= start address + 15),
//    the decoder behaves as in mode 3 (round-robin). It moves back to the default operation
//    after receiving an address=x with the ACTIVATE command (can be any decoder address)
// 1: Same as mode=0, but this time also the address=x with DEACTIVATE command releases the relay
//    Note: in this mode there will be one or none of the relays closed 
// 2: A relays with address=x will be set after a ACTIVATE command
//    A relays with address=x will be released after a DEACTIVATE command
//    - note: unlike the previous modes, other relays will not be released after one relays is set
//    - note: unlike the previous modes, multiple relays may be closed at the same time
// 3: Relays are closed in a round-robin fashion
//    - the time (in seconds) each relays is closed is determined by CV535 (RInter)
//    - since some of the relays may not be used, the relays that need to be involved in this
//      round-robin schema are defined by CV533 (relays 1-8) and CV534 (relays 9-16).
//      Each bit in these CVs represent one relay. If CV533=3, only relays 1 and 2 are used,
//      if CV533=5, only relais 1 and 3 are used, if CV535=255, relays 1-8 are used.
// Setting the mode can be controlled at programming time (mode = address - base_address)
// or via CV536.
// Setting the ACTIVATE command can be controlled at programming time or via CV532.
// At programming time, the ACTIVE command is the "+" or "-" on the LH100
//
// Since a command station may send a command multiple times in a row, it is necessary to keep
// track of the first command in that row, to be able to ignore the subsequent commands.
// If we wouldn't do that, relais may go on -> off -> on multiple times ("oscillation").
//
// Programming:
// The decoder print has 16 relays, and therefore listens to 16 DCC addresses.
// Programming of the decoder address is similar to programming of other decoders, however.
// The start address will therefore be a multiple of 4, and not of 16!
// Some examples (using LENZ address assignment, which is 1 higher than normal):
//   Programming address   Start address  Mode
//   -----------------------------------------
//             1                1           0
//             2                1           1
//             3                1           2
//             4                1           3
//             5                2           0
//             6                2           1
// Note that, while programming, pressing "+" or "-" gives the same address (although with a
// different ACTIVATE command. 
//------------------------------------------------------------------------------------------------

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
#include "timer2.h"
#include "main.h"
#include "relays.h"

//================================================================================================
// 1. Global variables
//================================================================================================
unsigned char mode;              // the mode in which this decoder operates (see above - CV536)
unsigned char RR_Interval;       // the interval used with round-robin (CV535)
unsigned char RR_BlockA;         // round-robin relays used, relays 9-16 (CV534)
unsigned char RR_BlockC;         // round-robin relays used, relays 1-8 (CV533)
unsigned char relaisActiveCmd;   // 0: relais active with - / 1: relais active with + (CV532)

unsigned char PreviousCommand;   // the command that has just been executed
unsigned char RRMode;            // determines if the decoder is in round-robin mode

unsigned char RBlockA[8];        // the relays in block A that need to be used for round-robin
unsigned char RBlockC[8];        // same, but now for block C
unsigned char RBlockA_Next;      // the next round-robin relay in block A     
unsigned char RBlockC_Next;      // same, but now for block C



//================================================================================================
// 2. Support functions
//================================================================================================
void set_relay_C(unsigned char relay_no) {PORTC |= (1<<relay_no);}
void clr_relay_C(unsigned char relay_no) {PORTC &= ~(1<<relay_no);}    
void clr_all_C(void) {PORTC &= 0x00;}

void set_relay_A(unsigned char relay_no) {PORTA |= (1<<relay_no);}
void clr_relay_A(unsigned char relay_no) {PORTA &= ~(1<<relay_no);}    
void clr_all_A(void) {PORTA &= 0x00;}

void fill_array(unsigned char buffer[], unsigned char RRBlock)
{ // copies the 8 values of a byte into eigth array values
  unsigned char i, total_active;
  total_active = 0;
  for (i=0; i <8; i++) {                           // copy these values in an array
    buffer[i] = RRBlock & 0b00000001;              // what is value of lowest bit?
    RRBlock = RRBlock >> 1;                        // move right
    total_active = total_active + buffer[i]; }     // check how many relays are active
  if (total_active == 0) {buffer[0] = 1;}          // make at least 1 relay active
}

//================================================================================================
// 3. Main functions
//================================================================================================
void init_relays_actions(void)
  {
  relaisActiveCmd = my_eeprom_read_byte(&CV.Ract); // cv532 - 0="-", 1="+" (on LH100)
  RR_BlockC       = my_eeprom_read_byte(&CV.RRR1); // cv533 - round-robin relays used, relays 1-8
  fill_array(RBlockC, RR_BlockC);                  // copy values to an array (easier programming)
  RR_BlockA   = my_eeprom_read_byte(&CV.RRR2);     // cv534 - round-robin relays used, relays 9-16
  fill_array(RBlockA, RR_BlockA);                  // copy values to an array (easier programming)
  RR_Interval = my_eeprom_read_byte(&CV.RInter);   // cv535                
  mode        = my_eeprom_read_byte(&CV.Mode);     // cv536   
  RRMode = 0;                                      // No round-robin
  if (mode == 3)        {RRMode = 1;}              // Except for mode == 3
  if (RR_Interval == 0) {RR_Interval = 1;}         // set minimum round-robin interval
  }


void relays_actions(unsigned int Command)
  {
    unsigned char myCommand, myOperation, myRelay;
    if (Command > 31) {return;}           // not our Address
    myOperation = Command & 0b00000001;   // 0="-", 1="+"
    myCommand = Command & 0b00011111;
    myRelay = myCommand >> 1;
    // If this command is a retransmission, just ignore
    if (myCommand == PreviousCommand) {return;}      // do not repeat previous command
    PreviousCommand = myCommand;
    // 
    if (myRelay < 8)         // first block of relays: on PCB near LED
    {
      if (myOperation == relaisActiveCmd)                 // command says: set relay 
      { if      (mode == 0) {clr_all_C(); RRMode = 0;}        // clear all relays on PORT C
        else if (mode == 1) {clr_all_C();}                    // clear all relays on PORT C    
        if      (mode == 0) {set_relay_C(myRelay);}           // set this specific relay
        else if (mode == 1) {set_relay_C(myRelay);}           // set this specific relay      
        else if (mode == 2) {set_relay_C(myRelay);}           // set this specific relay      
      }
      else                                                    // command says: clear relay
      { if      (mode == 0) {}                                // ignore
        else if (mode == 1) {clr_relay_C(myRelay);}           // clear this specific relay
        else if (mode == 2) {clr_relay_C(myRelay);}           // clear this specific relay
      }
    }
    else if (myRelay < 16)   // second block of relays: near output
    {
      myRelay = 15 - myRelay;                                 // Relay 8..15 => 7..0
      if (myOperation == relaisActiveCmd)                 // command says: set relay 
      { if      (mode == 0) {clr_all_A(); RRMode = 0;}        // clear all relays on PORT A
        else if (mode == 1) {clr_all_A();}                    // clear all relays on PORT A    
        if      (mode == 0) {set_relay_A(myRelay);}           // set this specific relay
        else if (mode == 1) {set_relay_A(myRelay);}           // set this specific relay      
        else if (mode == 2) {set_relay_A(myRelay);}           // set this specific relay      
      }    
      else                                                    // command says: clear relay
      { if      (mode == 0) {if (myRelay == 0) {RRMode = 1;}} // last relay - => round-robin
        else if (mode == 1) {clr_relay_A(myRelay);}           // clear this specific relay
        else if (mode == 2) {clr_relay_A(myRelay);}           // clear this specific relay
      }
    }
  }  // End of procedure relays_actions 


void relays_round_robin(void)
{
  if (T2_Seconds >= RR_Interval)	
  {
    T2_Seconds = 0;
    if (RRMode > 0)
    { 
      // Port A (near output connector). Note: ports are reversed order
      clr_all_A();                                            // clear all relays in this block      
      while (RBlockA[RBlockA_Next] == 0) {                   // while relay not active
        RBlockA_Next = (RBlockA_Next + 1) % 8;}               // next relay, modulus 8
      set_relay_A(7 - RBlockA_Next);                              // set relay
      RBlockA_Next = (RBlockA_Next + 1) % 8;                  // next relay, modulus 8
      // Port C (near LED)
      clr_all_C();                                            // clear all relays in this block      
      while (RBlockC[RBlockC_Next] == 0) {                   // while relay not active
        RBlockC_Next = (RBlockC_Next + 1) % 8;}               // next relay, modulus 8
      set_relay_C(RBlockC_Next);                              // set relay
      RBlockC_Next = (RBlockC_Next + 1) % 8;                  // next relay, modulus 8
    };
  }
}
