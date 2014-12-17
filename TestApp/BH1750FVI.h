#ifndef _BH_H_
#define _BH_H_

#define Device_Address_H 0x5C
#define Continuous_H_resolution_Mode  0x10

class BH1750FVI {
public:
	BH1750FVI(){}
	void begin(void){}
	void SetAddress(uint8_t add){}
	void SetMode(uint8_t MODE){}
	uint16_t GetLightIntensity(void){ return 0; }
};

#endif