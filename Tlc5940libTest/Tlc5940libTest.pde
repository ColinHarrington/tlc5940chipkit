#include <Tlc5940.h>

/*
	Don't forget to set JP4!
*/

void setup() {
	Tlc.init(0);	
}

void loop()
{
  	for (int i=0; i<Tlc.getNumTLCs()*16; i++){
		Tlc.set(i, 4095);
		Tlc.update();
		delay(10);
	}
	for (int i=0; i<Tlc.getNumTLCs()*16; i++){
		Tlc.set(i, 0);
		Tlc.update();
		delay(10);
	}
}

