#ifndef _SERIAL_H_
#define _SERIAL_H_

class SerialClass
{
public:
	void begin(int i){}
	void print(char* s){}
	void print(int i){}
	void println(char* s){}
	void println(int i){}
};

extern SerialClass Serial;

#endif