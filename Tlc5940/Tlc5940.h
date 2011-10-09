#ifndef TLC5940_H
#define TLC5940_H

#include <stdint.h>

//extern unsigned int tlc_GSData[NUM_TLCS * 6];

class Tlc5940
{
  public:
	void init(int initialValue = 0);
	void clear(void);
	int update(void);
	void set(int channel, int value);
    //int get(int channel);
	void setAll(int value);
	int needxlat(void);
#if VPRG_ENABLED
	void setAllDC(uint8_t value);
    //void setAllDC(uint8_t r, uint8_t g, uint8_t b);
    //void setAllDCtest(uint8_t r, uint8_t g, uint8_t b);
#endif

	void setRGB1(int channel, int r, int g, int b);
	void setRGB2(int channel, int r, int g, int b);
#if XERR_ENABLED
    uint8_t readXERR(void);
#endif

  private:
	void request_xlat_pulse();

};

// for the preinstantiated Tlc variable.
extern Tlc5940 Tlc;

#endif
