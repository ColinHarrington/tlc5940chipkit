#include <plib.h>
#include "Tlc5940.h"

/** specify the number of TLC5940 chips that are connected */
#define NUM_TLCS 2

/** Bit-bang using any two i/o pins */
#define TLC_BITBANG        1
/** Use the much faster hardware SPI module */
#define TLC_SPI            2

#define RGB_ENABLED			1 // use this to include/exclude the RGB helper functions

/** Determines how data should be transfered to the TLCs.  Bit-banging can use
    any two i/o pins, but the hardware SPI is faster.
    - Bit-Bang = TLC_BITBANG --> *NOT YET WORKING* 
    - Hardware SPI = TLC_SPI (default) */
#define DATA_TRANSFER_MODE TLC_SPI
//#define DATA_TRANSFER_MODE TLC_BITBANG

/** Pulses a pin - high then low. */
#define pulse_pin(port, pin)   port |= pin; port &= ~ pin

/** Ports **/
#define VPRG 0x2
#define VPRG_PORT PORTF

#define SOUT 0x100
#define SOUT_PORT PORTG

#define SCLK 0x40
#define SCLK_PORT PORTG

#define GSCLK 0x1
#define GSCLK_PORT PORTD

#define BLANK 0x20
#define BLANK_PORT PORTD

#define XLAT 0x8
#define XLAT_PORT PORTD

/** Packed grayscale data, 24 bytes (16 * 12 bits) per TLC.

    Format: Lets assume we have 2 TLCs, A and B, daisy-chained with the SOUT of
    A going into the SIN of B.
    - byte 0: upper 8 bits of B.15
    - byte 1: lower 4 bits of B.15 and upper 4 bits of B.14
    - byte 2: lower 8 bits of B.0
    - ...
    - byte 24: upper 8 bits of A.15
    - byte 25: lower 4 bits of A.15 and upper 4 bits of A.14
    - ...
    - byte 47: lower 8 bits of A.0

    \note Normally packing data like this is bad practice.  But in this
          situation, shifting the data out is really fast because the format of
          the array is the same as the format of the TLC's serial interface. */
unsigned int tlc_GSData[NUM_TLCS * 6]; // 6 * 32 = 192 bits = 16x 12bit values

/** This will be true (!= 0) if update was just called and the data has not
    been latched in yet. */
volatile uint8_t tlc_needXLAT;

int Tlc5940::needxlat() 
{
	return tlc_needXLAT;
}

void Tlc5940::init(int initialValue)
{
	//Setting Directionality of ports	
	TRISFCLR = VPRG;
	TRISGCLR = SOUT;
	TRISGCLR = SCLK;
	TRISDCLR = GSCLK;
	TRISDCLR = BLANK;
	TRISDCLR = XLAT;

	// Turn on VPRG because Heath is testing interrupts
	//VPRG_PORT |= VPRG;
	
	#if DATA_TRANSFER_MODE == TLC_BITBANG

	#elif DATA_TRANSFER_MODE == TLC_SPI
	//Setting up SPI
	OpenSPI2(SPI_MODE32_ON|MASTER_ENABLE_ON|SPI_CKE_ON|PRI_PRESCAL_4_1|FRAME_ENABLE_OFF, SPI_ENABLE);
	#endif
	
	// Start the timer for GSCLK
	OpenTimer2(T2_ON | T2_PS_1_1, 0x3);

	// Start the timer for BLANK/XLAT
	OpenTimer3(T3_ON | T3_PS_1_4, 0x1003);
	
	// Start the OC for BLANK
	OpenOC5(OC_ON | OC_TIMER3_SRC | OC_CONTINUE_PULSE, 0x3, 0x0);
	
	// Start the OC for XLAT (but set the pulse time out of range, so that it doesn't trigger the pin)
	OpenOC4(OC_ON | OC_TIMER3_SRC | OC_CONTINUE_PULSE, 0xFFFF, 0xFFFF);
	
	//Interrupt for XLAT
	ConfigIntOC4(OC_INT_ON | OC_INT_PRIOR_3 | OC_INT_SUB_PRI_3);
	
	setAll(initialValue);

	update();
	
	// Wait for the first update to be sent to the TLC before starting GSCLK
	// so that the board doesn't start with random values
	while(tlc_needXLAT);

	// Start the OC for GSCLK
	OpenOC1(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE, 0x2, 0x2);	

}

/** Clears the grayscale data array, #tlc_GSData, but does not shift in any
    data.  This call should be followed by update() if you are turning off
    all the outputs. */
void Tlc5940::clear(void)
{
    for(int i = 0; i < (NUM_TLCS * 6); i++) {
		tlc_GSData[i] = 0x0;
	}
}

#if DATA_TRANSFER_MODE == TLC_BITBANG

// THIS FUNCTION IS CURRENTLY A BUCKET OF FAAAAAAAAAIL
int Tlc5940::update(void)
{
	pulse_pin(SCLK_PORT, SCLK);
	
	for(int i = 0; i < (NUM_TLCS * 6); i++) {
		for(int s = 31; s >= 0; s--)
		{
			if(tlc_GSData[i] >> s & 0x1) {
				PORTGSET |= SOUT;
			} else {
				PORTGCLR |= SOUT;
			}
			
			pulse_pin(SCLK_PORT, SCLK);
		}
	}
	
	request_xlat_pulse();

	return 0;
}

#elif DATA_TRANSFER_MODE == TLC_SPI

int Tlc5940::update(void)
{
	if (tlc_needXLAT){
		return 1;
	}
	pulse_pin(SCLK_PORT, SCLK);
	
	//TODO use Interrupt driven SPI for a non-blocking performance boost
	putsSPI2(6*NUM_TLCS, tlc_GSData);

	while(SpiChnIsBusy(SPI_CHANNEL2));

	request_xlat_pulse();
	
    return 0;
}

#endif


/** Sets channel to value in the grayscale data array, #tlc_GSData.
    \param channel (0 to #NUM_TLCS * 16 - 1).  OUT0 of the first TLC is
           channel 0, OUT0 of the next TLC is channel 16, etc.
    \param value (0-4095).  The grayscale value, 4095 is maximum.
    \see get */
void Tlc5940::set(int channel, int value)
{
	// Data is packed into tlc_GSData as pictured below. 
	// Each letter represents 4 bits and the last channel is stored first. 
	// So |A|A|A| would be the 12-bit value of the last TLC channel
	// 
	// As picture below, there are 8 possible positions for the 12-bit value to be stored in the array (Cases A-H)
	//	               _________________________________________________
	//  Channel data: |A|A|A|B|B|B|C|C|C|D|D|D|E|E|E|F|F|F|G|G|G|H|H|H|
	// 32-bit borders |‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|


	// Since the last channel is stored first in the array, index32 is used as the inverse of channel
	// i.e. assuming NUM_TLCs = 1, when channel = 15, then index32 = 0 and when channel = 0, then index32 = 15
	unsigned int index32 = (NUM_TLCS * 16 - 1) - channel;

	// index12p = index32 * 3 / 8 = the index into tlc_GSData where a 32-bit elment exists that will hold our 12-bit value
	unsigned int *index12p = tlc_GSData + ((index32 * 3) >> 3);


	// caseNum  = index32 mod 8 = which of the 8 possible cases we need to handle
	int caseNum = index32 % 8;

	switch(caseNum){
		case 0:	// A: 12 bits start at the beginning of the element

			// zero out the 12 bits and OR those bits with the value
			*index12p = (*index12p & 0x000FFFFF) | value << 20;
			break;
		case 1:	// B: 12 bits start 12 bits into the element

			// zero out the 12 bits and OR those bits with the value
			*index12p = (*index12p & 0xFFF000FF) | value << 8;
			break;
		case 2:	// C: 12 bits start 24 bits into the element and continue 4 bits into the next

			// zero out the 8 bits that are in this element and OR them with the 8 MSB bits of value
			*index12p = (*index12p & 0xFFFFFF00) | value >> 4;

			// move to the next element, and zero out the remaining 4 bits, then OR them with the remaining 4 bits of value
			index12p++;
			*index12p = (*index12p & 0x0FFFFFFF) | value << 28;
			break;
		case 3:	// D: 12 bits start 4 bits into the element

			// zero out the 12 bits and OR those bits with the value
			*index12p = (*index12p & 0xF000FFFF) | value << 16; 			
			break;
		case 4: // E: 12 bits start 16 bits into the element

			// zero out the 12 bits and OR those bits with the value
			*index12p = (*index12p & 0xFFFF000F) | value << 4;
			break;
		case 5:	// F: 12 bits start 28 bits into the element and continue 8 bits into the next

			// set the 4 bits that apply to this element
			*index12p = (*index12p & 0xFFFFFFF0) | value >> 8;

			// move to the next element, and set the appropriate bits
			index12p++;
			*index12p = (*index12p & 0x00FFFFFF) | value << 24;
			break;
		case 6:	// G: 12 bits start 8 bits into the element
			*index12p = (*index12p & 0xFF000FFF) | value << 12;
			break;
		case 7:	// H: 12 bits start 20 bits into the element
			*index12p = (*index12p & 0xFFFFF000) | value;
			break;
	}
}

// There is most certainly a faster way to do this, but this will work for now
void Tlc5940::setAll(int value){
	for (int i=0; i<NUM_TLCS*6; i++){
		Tlc.set(i, value);
	}
}

int Tlc5940::getNumTLCs(){
	return NUM_TLCS;
}

#if RGB_ENABLED

// RGB LEDs are connected to the TLC sequentially
// i.e. ch 0 = R1, ch 1 = G1, ch 2 = B1, ch 3 = R2, ch 4 = G2, ch 5 = B2
void Tlc5940::setRGB1(int channel, int r, int g, int b){
	int tlc_channel = channel * 3;

	set(tlc_channel++, r);
	set(tlc_channel++, g);
	set(tlc_channel, b);
}

// Each RGB LED is connected to multiple TLCs.
// i.e. ch 0 = R1, ch 1 = R2, ch 16 = G1, ch 17 = G2, ch 32 = B1, ch 33 = B2
void Tlc5940::setRGB2(int channel, int r, int g, int b){

	int tlc_channel = channel / 16 * 48 + channel % 16;

	set(tlc_channel, r);
	tlc_channel += 16;
	set(tlc_channel, g);
	tlc_channel += 16;
	set(tlc_channel, b);
}
#endif



/**
Triggering XLAT starts with enabling the Blank interrrupt
this interrupt will fire after BLANK is pulsed
the Blank interrupt handler sets the XLAT pulse time to occur the next time BLANK is high.
After XLAT is pulsed the XLAT Interrupt fires 
which resets the need_XLAT flag and causes the XLAT pulse time to be set out of range 
(so that neither the pin or the interrupt with be fired)

*/
void Tlc5940::request_xlat_pulse(){

	// Set this flag so that we can keep track of whether or not we are waiting for an XLAT pulse
	tlc_needXLAT = 1;

	// Enable the interrupt on BLANK to fire
	ConfigIntOC5(OC_INT_ON | OC_INT_PRIOR_3 | OC_INT_SUB_PRI_3);
}

#ifdef __cplusplus
extern "C"	// So c++ doesn't mangle the function names
{
#endif

	// Handle the interrupt triggered by BLANK
	void __ISR(_OUTPUT_COMPARE_5_VECTOR, ipl3) IntOC5Handler(void)	
	{
		// Stop BLANK from firing any more interrupts
		ConfigIntOC5(OC_INT_OFF);

		// Set the XLAT pulse to occur during the next time BLANK is high
		SetPulseOC4(0x1, 0x2);
	}

	// Handle the interrupt triggered by XLAT
	void __ISR(_OUTPUT_COMPARE_4_VECTOR, ipl3) IntOC4Handler(void)
	{
		// Rather than turning off the interrupt for XLAT, just set the pulse time to a value that will never be matched
		SetPulseOC4(0xFFFF, 0xFFFF);
		
		// Reset our flag
		tlc_needXLAT = 0;
			
		mOC4ClearIntFlag();
	}

#ifdef __cplusplus
}
#endif


/** Preinstantiated Tlc variable. */
Tlc5940 Tlc;
