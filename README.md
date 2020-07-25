# DCC decoder for 16 relays

Decoder for the DCC (Digital Command Control) system for 16 relays. 
Written in C as one of my first ATMEGA projects in May 2011 and heavily based on software from the OpenDecoder2 project. 
It runs on the ATMEGA 16A and similar AVRs.

Two blocks of each eight relays can be switched; both blocks are independent from each other.
The application in mind while designing this decoder was to control a number of video camera modules with standard (PAL) video signal, 
such as available for a couple of Euros in China.

The relays decoder can operate in one of the following modes:

<b>Mode=0:</b> Default operation. An address=x with the ACTIVATE command (the "+" and "-" on the LH100):
* Releases all relays in that block
* Set the relay with address=x
* Note: in this mode there will always be one (and only one) relay closed
An address=x without the DEACTIVATE command is ignored (has no impact).
However: if such command is received for the last decoder address (= start address + 15),
the decoder behaves as in mode 3 (round-robin). It moves back to the default operation
after receiving an address=x with the ACTIVATE command (can be any decoder address)

<b>Mode=1:</b>  Same as mode=0, but this time also the address=x with DEACTIVATE command releases the relay
Note: in this mode there will be one or none of the relays closed 

<b>Mode=2:</b> A relays with address=x will be set after a ACTIVATE command
A relays with address=x will be released after a DEACTIVATE command
* note: unlike the previous modes, other relays will not be released after one relays is set
* note: unlike the previous modes, multiple relays may be closed at the same time

<b>Mode=3:</b> Relays are closed in a round-robin fashion
* the time (in seconds) each relays is closed is determined by CV535 (RInter)
* since some of the relays may not be used, the relays that need to be involved in this
round-robin schema are defined by CV533 (relays 1-8) and CV534 (relays 9-16).
Each bit in these CVs represent one relay. If CV533=3, only relays 1 and 2 are used,
if CV533=5, only relais 1 and 3 are used, if CV535=255, relays 1-8 are used.
Setting the mode can be controlled at programming time (mode = address - base_address) or via CV536.
Setting the ACTIVATE command can be controlled at programming time or via CV532.
At programming time, the ACTIVE command is the "+" or "-" on the LH100

Since a command station may send a command multiple times in a row, it is necessary to keep track of the first command in that row, 
to be able to ignore the subsequent commands.
If we wouldn't do that, relais may go on -> off -> on multiple times ("oscillation").

Programming:
The decoder print has 16 relays, and therefore listens to 16 DCC addresses.
Programming of the decoder address is similar to programming of other decoders, however.
The start address will therefore be a multiple of 4, and not of 16!

# Hardware #
The hardware and schematics can be downloaded from [my EasyEda homepage](https://easyeda.com/aikopras/relays-16-decoder).
A description of this decoder and related decoders can be found on [https://sites.google.com/site/dcctrains](https://sites.google.com/site/dcctrains).
