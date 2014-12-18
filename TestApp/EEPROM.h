#ifndef _EEPROM_H_
#define _EEPROM_H_

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