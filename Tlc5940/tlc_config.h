/** specify the number of TLC5940 chips that are connected */
#ifndef NUM_TLCS
	#define NUM_TLCS				1
#endif
/** Bit-bang using any two i/o pins */
#define TLC_BITBANG			1

/** Use the much faster hardware SPI module */
#define TLC_SPI            2

#ifndef TLC_SPI_PRESCALER_FLAGS
	#define TLC_SPI_PRESCALER_FLAGS PRI_PRESCAL_4_1|SEC_PRESCAL_4_1
#endif

/** use this to include/exclude the RGB helper functions */
#ifndef RGB_ENABLED
	#define RGB_ENABLED			1 
#endif

/** Determines how data should be transfered to the TLCs.  Bit-banging can use
    any two i/o pins, but the hardware SPI is faster.
    - Bit-Bang = TLC_BITBANG --> *NOT YET WORKING* 
    - Hardware SPI = TLC_SPI (default) */
#ifndef DATA_TRANSFER_MODE
	#define DATA_TRANSFER_MODE TLC_SPI
#endif

/** Defines whether or not you will be able to set Dot Correction
    The TLC5940 defaults to all channels at 100% if you decide to not set Dot Correction */
#ifndef VPRG_ENABLED
	#define VPRG_ENABLED			1
#endif

/** XERR is not yet implemented */
#ifndef XERR_ENABLED
	#define XERR_ENABLED 0
#endif
