//------------------------------------------------------------------------
//
// OpenDCC - OpenDecoder
//
// Copyright (c) 2007 Kufer
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at
// http://www.gnu.org/licenses/gpl.txt
// 
//------------------------------------------------------------------------
//
// file:      timer_led.h
// author:    Wolfgang Kufer
// contact:   kufer@gmx.de
// webpage:   http://www.opendcc.de
//
//------------------------------------------------------------------------

void init_timer1(void);

void turn_led_on(void);
  
void turn_led_off(void);
  
void flash_led_fast(unsigned char count);

void local_alarm(unsigned char count);


