//------------------------------------------------------------------------
//
// file:      timer2.h
//
// purpose:   Timer 2 usage for the relays decoder
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at http://www.gnu.org/licenses/gpl.txt
//
// history:   2011-05-05 V0.1 First version
//
//--------------------------------------------------------------------------------------
// Global Data: 
volatile unsigned char T2_Seconds;			 // Used by relays.c for timing purposes

// Hardware initialisation and ISR routines
void init_timer2(void);
