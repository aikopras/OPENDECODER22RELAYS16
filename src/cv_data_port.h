//------------------------------------------------------------------------
//
// OpenDCC - OpenDecoder3
//
// Copyright (c) 2007 Kufer
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at
// http://www.gnu.org/licenses/gpl.txt
// 
//------------------------------------------------------------------------
//
// file:      cv_data_port.h
// author:    Wolfgang Kufer
// contact:   kufer@gmx.de
// webpage:   http://www.opendcc.de
// history:   2007-08-03 V0.1 kw start
//            2007-08-18 V0.2 kw default for myADDR hight changed to 0x80
//                               -> this means: unprogrammed    
//            2011-01-15 V0.3 ap initial values have been included for the new CVs: LenzCor and
//                               SkipUnEven. CV546 supports a new feedback method: RS-bus

//
//------------------------------------------------------------------------
//
// purpose:   flexible general purpose decoder for dcc
//            This is the default cv definition for the project
//            cv_define_xxxxx.h contains the definitions.
//
//------------------------------------------------------------------------
//
// content:   1. ....
//
//========================================================================
////-----------------------------------------------------------------------------
// data in EEPROM:
// Note: the order of these data corresponds to physical CV-Address
//       CV1 is coded at #00
//       see RP 9.2.2 for more information


//-----------------------------------------------------------------------------
// data in EEPROM:
// Note: the order of these data corresponds to physical CV-Address
//       CV1 is coded at #00
//       see RP 9.2.2 for more information

// Content          Name       CV  -alt  type    comment
   0x01,        //  myAddrL     513   1   M       Decoder Adresse low, 6 bits
   0,           //  auxActiv    514   2  O       Auxiliary activation of outputs: 0=no aux activation
   15,          //  T_on_F1     515   3  O       Time on F1
   15,          //  T_on_F2     516   4  O       Time on F2
   15,          //  T_on_F3     517   5  O       Time on F3
   15,          //  T_on_F4     518   6  O       Time on F4
   0x07,        //  version     519   7  M       Version
   0x0D,        //  VID         520   8  M       Vendor ID (0x0D = DIY Decoder)
                                                //        (0x3E = TAMS)
   0x80,        //  myAddrH     521   9  M       Decoder Adresse high (3 bits)
   0,           //  cv522       522  10  -       reserved
   0,           //  cv523       523  11  -       reserved
   0,           //  cv524       524  12  -       reserved
   0,           //  cv525       525  13  -       reserved
   0,           //  cv526       526  14  -       reserved
   0,           //  cv527       527  15  -       reserved
   0,           //  cv528       528  16  -       reserved
   0,           //  cv529       529  17  -       reserved
   0,           //  cv530       530  18  -       reserved
   0,           //  cv531       531  19  -       reserved
                //                               CVs below are relays decoder specific (relays.c) 
                //                               Reserved, according to the NMRA specs 
   1,           //  Ract        532  20  -       If relais switches with - (=0) or with + (=1)
   15,          //  RRR1        533  21  -       Relays used for round-robin, relays 1-8  (Port C)
   7,           //  RRR2        534  22  -       Relays used for round-robin, relays 9-16 (Port A)
   6,           //  RInter      535  23  -       Relays decoder, round-robin interval (in seconds)
   0,           //  Mode        536  24  -       Relais decoder mode (see relays.c)
                //                               CVs below are RS-bus specific (see: rs-bus.c)
                //                               Reserved, according to the NMRA specs 
   0,           //  RSRetry     537  25  -       Number of RS-Bus retransmissions
   1,           //  LenzCor     538  26  -       LENZ acc. decoder address correction 
   1,           //  SkipUnEven  539  27  -       Skip uneven addresses
                //                               CVs below are standard (for OpenDecoder)
   0,           //  BiDi        540  28  -       Bi-Directional Communication Config - keep at 0
                //  Config      541  29  -       similar to CV#29; for acc. decoders
                     (1<<7)                   // 1 = we are accessory
                   | (0<<6)                   // 0 = we do 9 bit decoder adressing
                   | (0<<5)                   // 0 = we are basic accessory decoder
                   | 0,                       // 4..0: all reserved
   0,           //  cv542       542  30  -       reserved
   0,           //  cv543       543  31  -       reserved
   0,           //  LastState   544  32  -       saved last port state (was reserved)
                //
                // 545-593 - Manufacturer Unique
                //
   0,           //  MODE        545  33  -      global decoder mode
                                               // 00 = turnout decoder with feedback
                                               // 01 = dual servo decoder
                                               // 02 = multiposition servo decoder
                                               // 03 = direct output control
                                               // 04 = reserved
                                               // ...
                                               // 08 = dmx decoder
                                               // 16 = kirmes decoder
                                               // 128 + above: use LENZ address correction
   3,           //  FM          546  34  -      global feedback mode
                                               // 00 = no feedback
                                               // 01 = pos. feedback (ack)
                                               // 02 = neg. feedback (emergency stop)
                                               // 03 = Feedback using RS-bus 
   1,           //  FBM_F1      547  35  -      feedback mode Func 1
                                               // 00 = command acknowledge
                                               // 01 = turnout feedback with intrinsic endswitches
                                               // 02 = feedback with extra detector
                                               // 03 = feedback with extra detector
   1,           //  FBM_F2      548  36  -      feedback mode Func 2
   1,           //  FBM_F3      549  37  -      feedback mode Func 3
   1,           //  FBM_F4      550  38  -      feedback mode Func 4


