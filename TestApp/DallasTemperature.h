#ifndef _DT_H_
#define _DT_H_

#include "u8g.h"
#include "OneWire.h"

typedef uint8_t DeviceAddress[8];

class DallasTemperature
{
public:

	DallasTemperature(OneWire*){}

	// initalise bus
	void begin(void){}

	bool getAddress(uint8_t*, const uint8_t){ return true; }
	bool setResolution(uint8_t*, uint8_t){ return true; }
	void setWaitForConversion(bool){}
	bool requestTemperaturesByAddress(uint8_t*){ return true; }
	float getTempC(uint8_t*){ return (float)25.5 + (rand()%100)/(float)100.0; }
};

#endif