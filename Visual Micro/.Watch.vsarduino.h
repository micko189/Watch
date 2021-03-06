/* 
	Editor: http://www.visualmicro.com
	        visual micro and the arduino ide ignore this code during compilation. this code is automatically maintained by visualmicro, manual changes to this file will be overwritten
	        the contents of the Visual Micro sketch sub folder can be deleted prior to publishing a project
	        all non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
	        note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: Arduino Pro or Pro Mini w/ ATmega328 (5V, 16 MHz), Platform=avr, Package=arduino
*/

#ifndef _VSARDUINO_H_
#define _VSARDUINO_H_
#define __AVR_ATmega328p__
#define __AVR_ATmega328P__
#define ARDUINO 158
#define ARDUINO_MAIN
#define __AVR__
#define __avr__
#define F_CPU 16000000L
#define __cplusplus
#define __inline__
#define __asm__(x)
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __asm__ 
#define __volatile__

#define __builtin_va_list
#define __builtin_va_start
#define __builtin_va_end
#define __DOXYGEN__
#define __attribute__(x)
#define NOINLINE __attribute__((noinline))
#define prog_void
#define PGM_VOID_P int
            
typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {;}

const u8g_fntpgm_uint8_t * helv08r();
const u8g_fntpgm_uint8_t * helv10r();
const u8g_fntpgm_uint8_t * helv12r();
const u8g_fntpgm_uint8_t * helv14r();
const u8g_fntpgm_uint8_t * helv18r();
const u8g_fntpgm_uint8_t * helv24r();
//
//
void ReadStateEPROM();
void WriteStateEPROM();
void calcAmPm();
void getButtonInput(const byte pinNo, boolean *btnPinState, boolean *insideDebounce, unsigned long *lastDebounceTime);
boolean isLeapYear(short year);
inline byte daysInMonth(byte month);
byte getDaysInMonth(byte month);
inline short daysPassedInYear();
void calcDayOfWeek();
inline boolean updateTime();
void toggleOption(byte *option, const byte maxVal);
void inrementRollOverValue(byte *value, byte rollOverVal);
void byteToStr(byte value, char* s);
inline void floatToHiLo(byte* valHi, byte* valLo, float val);
float hiLoToFloat(byte valHi, byte valLo);
void findMaxMin();
inline void prepareDraw();
inline void onDraw();
void prepareDrawSetMenu();
inline void drawSetMenu();
inline void drawStartUp();
void drawGraphLine(byte start, byte end, byte* x, float rescale);
void prepareDrawGraph();
void drawGraph();
void prepareDrawCalendar();
void drawCalendar();
inline void drawMenu();
void prepareDrawClock();
inline void drawClock();
void drawTimeFormat(byte xPos, byte yPos);
void prepareDrawDayAmPm();
void drawDayAmPm(byte xPos, byte yPos);
void drawDay(byte xPos, byte yPos);
byte prepareDrawTemp(byte tmpHi, byte tmpLo, char* temperatureStr);
void drawTemp(byte xPos, byte yPos, char* temperatureStr, byte offsetSuff, byte offsetY);
void prepareDrawClockDigital();
byte drawClockDigital(byte xPos, byte yPos);
void prepareDrawLumens();
void drawLumens(byte xPos, byte yPos);
void prepareDrawSecondsDigital();
void drawSecondsDigital(byte xPos, byte yPos);
void prepareDrawDateDigital();
void drawDateDigital(byte xPos, byte yPos);
void drawClockAnalog(byte xPos, byte yPos, byte radius);
void showTimePin(byte center_x, byte center_y, double pl1, double pl2, byte angle, byte radius, byte sign);

#include "C:\Program Files (x86)\Arduino\hardware\arduino\avr\cores\arduino\arduino.h"
#include "C:\Program Files (x86)\Arduino\hardware\arduino\avr\variants\eightanaloginputs\pins_arduino.h" 
#include "C:\Users\milan.ajdinovic\Documents\GitHub\Watch\Watch.ino"
#include "C:\Users\milan.ajdinovic\Documents\GitHub\Watch\BuildDefs.h"
#include "C:\Users\milan.ajdinovic\Documents\GitHub\Watch\bitmap.h"
#endif
