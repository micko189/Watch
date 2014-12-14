#ifndef _U8G_H_
#define _U8G_H_

// glyph u8glib
typedef signed char int8_t;

/** \ingroup avr_stdint
8-bit unsigned type. */

typedef unsigned char uint8_t;

/** \ingroup avr_stdint
16-bit signed type. */

//typedef signed int int16_t;

/** \ingroup avr_stdint
16-bit unsigned type. */

typedef unsigned int uint16_t;

/** \ingroup avr_stdint
32-bit signed type. */

//typedef signed long int int32_t;

/** \ingroup avr_stdint
32-bit unsigned type. */

//typedef unsigned long int uint32_t;

/** \ingroup avr_stdint
64-bit signed type.
\note This type is not available when the compiler
option -mint8 is in effect. */

typedef signed long long int int64_t;

/** \ingroup avr_stdint
64-bit unsigned type.
\note This type is not available when the compiler
option -mint8 is in effect. */

typedef unsigned long long int uint64_t;

typedef uint8_t u8g_pgm_uint8_t;
typedef uint8_t u8g_fntpgm_uint8_t;
typedef uint8_t u8g_uint_t;
typedef int8_t u8g_int_t;

#define u8g_pgm_read (uint8_t)*

#define U8G_FONT_SECTION(x) 

#define U8G_FONT_DATA_STRUCT_SIZE 17

#define U8G_ALWAYS_INLINE

#define U8G_DRAW_UPPER_RIGHT 0x01
#define U8G_DRAW_UPPER_LEFT  0x02
#define U8G_DRAW_LOWER_LEFT 0x04
#define U8G_DRAW_LOWER_RIGHT  0x08
#define U8G_DRAW_ALL (U8G_DRAW_UPPER_RIGHT|U8G_DRAW_UPPER_LEFT|U8G_DRAW_LOWER_RIGHT|U8G_DRAW_LOWER_LEFT)

#define U8G_I2C_OPT_NONE 0

#define PROGMEM

#define PGM_P char*

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

typedef unsigned char byte;
typedef unsigned char boolean;
#define HIGH 1
#define LOW 0



/* method def */

void ReadStateEPROM();
void WriteStateEPROM();
void calcAmPm();
void getButtonInput(const byte pinNo, boolean *btnPinState, boolean *insideDebounce, unsigned long *lastDebounceTime);
boolean isLeapYear(short year);
byte daysInMonth(byte month);
byte getDaysInMonth(byte month);
short daysPassedInYear();
void calcDayOfWeek();
boolean updateTime();
void toggleOption(byte *option, const byte maxVal);
void inrementRollOverValue(byte *value, byte rollOverVal);
void byteToStr(byte value, char* s);
void floatToHiLo(byte* valHi, byte* valLo, float val);
float hiLoToFloat(byte valHi, byte valLo);
void findMaxMin();
void prepareDraw();
void onDraw();
void prepareDrawSetMenu();
void drawSetMenu();
void drawStartUp();
void drawGraphLine(byte start, byte end, byte* x, float rescale);
void prepareDrawGraph();
void drawGraph();
void prepareDrawCalendar();
void drawCalendar();
void drawMenu();
void prepareDrawClock();
void drawClock();
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

#endif