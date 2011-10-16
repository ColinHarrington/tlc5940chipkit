#ifndef TLC5940_H
#define TLC5940_H
#include <tlc_config.h>
#include <stdint.h>

//extern unsigned int tlc_GSData[NUM_TLCS * 6];

class Tlc5940
{
  public:
	void init(int initialValue = 0);
	void clear(void);
	int update(void);
	void set(int channel, int value);
	int get(int channel);
	void setAll(int value);
	int updateInProgress(void);
	int getNumTLCs();
	void setRGB1(int channel, int r, int g, int b);
	void setRGB2(int channel, int r, int g, int b);
	

#if VPRG_ENABLED
	void setAllDC(int value);
	void setDC(int channel, int value);
	int getDC(int channel);
	int updateDC();
	uint8_t* getDCData();
    //void setAllDC(uint8_t r, uint8_t g, uint8_t b);
    //void setAllDCtest(uint8_t r, uint8_t g, uint8_t b);
#endif

#if XERR_ENABLED
    uint8_t readXERR(void);
#endif

  private:
	void request_xlat_pulse();

};

// for the preinstantiated Tlc variable.
extern Tlc5940 Tlc;

#endif
