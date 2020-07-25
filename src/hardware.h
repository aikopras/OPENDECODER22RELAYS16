//------------------------------------------------------------------------
//
// OpenDCC - OpenDecoder2
//
// Copyright (c) 2006 Kufer
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at
// http://www.gnu.org/licenses/gpl.txt
// 
//
// file:      hardware.h
//------------------------------------------------------------------------

//===============================================================================
// CPU Definitions:
//
#if (__AVR_ATmega8535__)
  // atmega8535:   512 Byte SRAM, 512 Byte EEPROM
  #define SRAM_SIZE		512
  #define EEPROM_SIZE	512
  #define EEPROM_BASE	0x810000L
#elif (__AVR_ATmega16__)
  // atmega16A:   1024 Byte SRAM, 512 Byte EEPROM
  #define SRAM_SIZE		1024
  #define EEPROM_SIZE	512
  #define EEPROM_BASE	0x810000L
#elif (__AVR_ATmega32__)
  // atmega32A:   2048 Byte SRAM, 1024 Byte EEPROM
  #define SRAM_SIZE		2048
  #define EEPROM_SIZE	1024
  #define EEPROM_BASE	0x810000L
#elif (__AVR_ATmega164A__)
  // atmega164A:   1024 Byte SRAM, 512 Byte EEPROM, 2 UARTS
  #define SRAM_SIZE		1024
  #define EEPROM_SIZE	512
  #define EEPROM_BASE	0x810000L
  #define ENHANCED_PROCESSOR	// other timer, interrupt and UART structure
#elif (__AVR_ATmega324A__)
  // atmega324A:   2048 Byte SRAM, 1024 Byte EEPROM, 2 UARTS
  #define SRAM_SIZE		2048
  #define EEPROM_SIZE	1024
  #define EEPROM_BASE	0x810000L
  #define ENHANCED_PROCESSOR	// other timer, interrupt and UART structure
#elif (__AVR_ATmega644P__)
  // atmega644:   1024 Byte SRAM, 512 Byte EEPROM, 2 UARTS
  #define SRAM_SIZE		4096
  #define EEPROM_SIZE	2048
  #define EEPROM_BASE	0x810000L
  #define ENHANCED_PROCESSOR	// other timer, interrupt and UART structure
#else
  #warning Processor
#endif


#ifndef F_CPU
   // prevent compiler error by supplying a default 
   # warning "F_CPU not defined, set default to 11059200 Hz"
   # define F_CPU 11059200UL
   // if changed: check every place where it is used
   // (possible range underflow/overflow in preprocessor!)
#endif


//===============================================================================
// PORT Definitions:
//
// PORTA:
// Used for the second set of eight relays (connections at side of video output)

// PORTB:
// Goes to flat cable connector
// Can be used for additional output, for example LCD display, LEDs or relais

// PORTC:
// Used for the first set of eight relays (connections at side of the LED)


// PORTD:
#define LED             0       // output, 1 turns on LED
#define RSBUS_TX        1       // UART for sending feedback via RS-bus
#define RSBUS_RX        2       // must be located on INT0
#define DCCIN           3       // must be located on INT1
#define SERVO1          4       // output (OC1B)
#define SERVO2          5       // output (OC1A)
#define PROGTASTER      6       //
#define DCC_ACK         7       // output, sending 1 makes an ACK

#define DCC_PORT        PORTD   // must be defined to have portable code
#define DCC_PORT_IN     PIND    // must be defined to have portable code

#define DCCIN_STATE     (PIND & (1<<DCCIN))

#define PROG_PRESSED    (!(PIND & (1<<PROGTASTER)))
#define LED_OFF         PORTD &= ~(1<<LED)
#define LED_ON          PORTD |= (1<<LED)
#define DCC_ACK_OFF     PORTD &= ~(1<<DCC_ACK)
#define DCC_ACK_ON      PORTD |= (1<<DCC_ACK)

// Note LEDs is active high -> state on == pin high!
#define LED_STATE       ((PIND & (1<<LED)))



