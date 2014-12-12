#ifndef EEPROM_h
#define EEPROM_h

#include "u8g.h"

class EEPROMClass
{
private:
	uint8_t data[512];
public:
	byte read(int i)
	{ 
		return data[i]; 
	}
	void write(int i, uint8_t v)
	{
		data[i] = v;
	}
};

extern EEPROMClass EEPROM;

#endif