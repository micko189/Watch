/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see [http://www.gnu.org/licenses/].
*/
/*
Watch Arduino v1.0
*/

#include <Wire.h>
#include <U8glib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BH1750FVI.h>
#include <math.h>
#include <EEPROM.h>

////////////////////////////////////////////////////
// 16x24 Logo
////////////////////////////////////////////////////
const unsigned char PROGMEM IMG_logo_24x24[] = {
	0x07, 0xff, 0xe0,
	0x07, 0xff, 0xe0,
	0x07, 0xff, 0xe0,
	0x08, 0x10, 0x10,
	0x10, 0x10, 0x08,
	0x10, 0x00, 0x08,
	0x10, 0x00, 0x08,
	0x10, 0x00, 0x08,
	0x10, 0x00, 0x08,
	0x10, 0x02, 0x08,
	0x10, 0x04, 0x08,
	0x10, 0x08, 0x0c,
	0x1c, 0x10, 0x3c,
	0x10, 0x08, 0x0c,
	0x10, 0x08, 0x08,
	0x10, 0x04, 0x08,
	0x10, 0x04, 0x08,
	0x10, 0x02, 0x08,
	0x10, 0x02, 0x08,
	0x10, 0x10, 0x08,
	0x08, 0x10, 0x10,
	0x07, 0xff, 0xe0,
	0x07, 0xff, 0xe0,
	0x07, 0xff, 0xe0
};

///////////////////////////////////////////////////////////////////
//----- OLED instance
U8GLIB_SSD1306_128X64_2X display(U8G_I2C_OPT_NONE);	// I2C / TWI 
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//----- Temp Sensor instance
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature TempSensor(&oneWire);
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//----- LightSensor instance
BH1750FVI LightSensor;
///////////////////////////////////////////////////////////////////

//----- Time
#define UPDATE_TIME_INTERVAL 1000 // 1s
short adjustedUpdateTimeInterval = UPDATE_TIME_INTERVAL;
unsigned long prevClockTime = 0;
unsigned long current_time_milis = 0;

short iYear = 2014;
byte iMonth = 11;
byte iDay = 6;

byte iHour = 9;
byte iMinutes = 0;
byte iSecond = 0;

byte iWeek = 0;    // need to calculate this during setup and on date change
byte iAmPm = 1;    // need to calculate this during setup and on time change 0:AM, 1:PM

#define TF_12h 1
#define TF_24h 2
byte iTimeFormat = TF_12h; // 1 - 12h, 2 - 24h

#define TEMP_GRAPH_LEN 128
byte tempGraphHi[TEMP_GRAPH_LEN] = { 0 };
byte tempGraphLo[TEMP_GRAPH_LEN] = { 0 };
byte startTempGraphIndex = 0;
short hourCount = 0;
float tempAccum = 0;

//----- Strings
PGM_P const weekString[] PROGMEM = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
PGM_P const dayString[] PROGMEM = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
PGM_P const ampmString[] PROGMEM = { "AM", "PM" };
PGM_P const menuItems[] PROGMEM = { "Set format", "Set time", "Set date" };
PGM_P const timeFormat[] PROGMEM = { "12h", "24h" };

//PROGMEM const byte daysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; // standard year days

//PGM_P const dayNames[] PROGMEM = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
//PGM_P const months[] PROGMEM = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

#define firstYear 2000 // This is our start point
#define dayOffset 5 // The first day of our start year may not be a Sunday ( in 2000 it was Sat which is index 6) offset is index - 1 = 5

//----- Display features
#define DISPLAY_MODE_START_UP			0x1
#define DISPLAY_MODE_CLOCK				0x2
#define DISPLAY_MODE_MENU				0x3
#define DISPLAY_MODE_SET_MENU			0x4
byte displayMode = DISPLAY_MODE_START_UP;

#define MENU_SET_TIME_FORMAT			0x1
#define MENU_SET_TIME					0x2
#define MENU_SET_DATE					0x3
byte menuMode = MENU_SET_DATE;

#define CLOCK_STYLE_SIMPLE_ANALOG		0x1
#define CLOCK_STYLE_SIMPLE_DIGIT		0x2
#define CLOCK_STYLE_SIMPLE_MIX			0x3
#define CLOCK_STYLE_SIMPLE_DIGIT_SEC	0x4
#define CLOCK_STYLE_SIMPLE_GRAPH		0x5
#define CLOCK_STYLE_SIMPLE_CALENDAR		0x6
byte clockStyle = CLOCK_STYLE_SIMPLE_DIGIT_SEC;

#define centerX 64
#define centerY 32
#define iRadius 30

//----- Button control
#define buttonPin 5
boolean btnPinState = HIGH; // LOW = false = 0x0 = Pressed, HIGH = true = 0x1 = Not pressed
boolean insideDebounce = false;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled

#define buttonPinUp 6
boolean btnPinStateUp = HIGH; // LOW = false = 0x0 = Pressed, HIGH = true = 0x1 = Not pressed
boolean insideDebounceUp = false;
unsigned long lastDebounceTimeUp = 0;  // the last time the output pin was toggled

#define buttonPinDown 7
boolean btnPinStateDown = HIGH; // LOW = false = 0x0 = Pressed, HIGH = true = 0x1 = Not pressed
boolean insideDebounceDown = false;
unsigned long lastDebounceTimeDown = 0;  // the last time the output pin was toggled

#define buttonPinBack 8
boolean btnPinStateBack = HIGH; // LOW = false = 0x0 = Pressed, HIGH = true = 0x1 = Not pressed
boolean insideDebounceBack = false;
unsigned long lastDebounceTimeBack = 0;  // the last time the output pin was toggled

boolean anyPinStateChanged = false;
#define debounceDelay 50    // the debounce time; increase if the output flickers

#define ouputLedPin 13

//----- Global
byte tempLo = 0;
byte tempHi = 0;

byte maxHi = 0;
byte maxLo = 0;

byte minHi = 0;
byte minLo = 0;

float minTemp = 0;

byte setPosition = 0; // position of value currently being set

#define HOUR_COUNT 1 //3600

char tempSufix[3];

DeviceAddress tempDeviceAddress;

#define resolution 12

uint16_t lux;

///////////////////////////////////
//----- Arduino setup and loop methods
///////////////////////////////////

void setup()
{
	// Start up the temperature sensor library
	TempSensor.begin();
	TempSensor.getAddress(tempDeviceAddress, 0);
	TempSensor.setResolution(tempDeviceAddress, resolution);

	// Request temperature conversion - non-blocking / async
	TempSensor.setWaitForConversion(false);
	TempSensor.requestTemperaturesByAddress(tempDeviceAddress);

	Serial.begin(9600);    // Enable serial com.

	// Define button pins, turn on internal Pull-Up Resistor
	pinMode(buttonPin, INPUT_PULLUP);
	pinMode(buttonPinUp, INPUT_PULLUP);
	pinMode(buttonPinDown, INPUT_PULLUP);
	pinMode(buttonPinBack, INPUT_PULLUP);

	// Define otput pin for button debugung
	pinMode(ouputLedPin, OUTPUT);    // Use Built-In LED for Indication

	// Assign default color value
	display.setColorIndex(1);         // pixel on BW

	// Start up the light sensor library
	LightSensor.begin();
	LightSensor.SetAddress(Device_Address_H);//Address 0x5C
	LightSensor.SetMode(Continuous_H_resolution_Mode);

	calcDayOfWeek();
	calcAmPm();

	tempSufix[0] = '°', tempSufix[1] = 'C', tempSufix[2] = 0;

	//ReadStateEPROM();

	//WriteStateEPROM();
}

void loop()
{
	anyPinStateChanged = false;

	// Update clock time
	current_time_milis = millis();

	// Get button input
	getButtonInput(buttonPin, &btnPinState, &insideDebounce, &lastDebounceTime);
	getButtonInput(buttonPinUp, &btnPinStateUp, &insideDebounceUp, &lastDebounceTimeUp);
	getButtonInput(buttonPinDown, &btnPinStateDown, &insideDebounceDown, &lastDebounceTimeDown);
	getButtonInput(buttonPinBack, &btnPinStateBack, &insideDebounceBack, &lastDebounceTimeBack);

	digitalWrite(ouputLedPin, !(btnPinState & btnPinStateUp & btnPinStateDown & btnPinStateBack));

	if (!(insideDebounce | insideDebounceUp | insideDebounceDown | insideDebounceBack))
	{
		boolean timeUpdated = updateTime();
		if (timeUpdated || anyPinStateChanged)
		{
			// One second has elapsed or we have input (button clicked)

			if (timeUpdated) // one second elapsed, no need for adiitional check >
			{
				//TempSensor.requestTemperaturesByIndex(0); // Send the command to get temperatures, sync call
				float temp = TempSensor.getTempC(tempDeviceAddress);
				tempAccum += temp;

				floatToHiLo(&tempHi, &tempLo, temp);

				hourCount++;
				if (hourCount > HOUR_COUNT)
				{
					// One hour has elapsed
					floatToHiLo(&tempGraphHi[startTempGraphIndex], &tempGraphLo[startTempGraphIndex], tempAccum / HOUR_COUNT);

					inrementRollOverValue(&startTempGraphIndex, TEMP_GRAPH_LEN);

					tempAccum = 0;
					hourCount = 0;
				}

				lux = LightSensor.GetLightIntensity();// Get Lux value
				Serial.println(lux);

				//dim display (Arduino\libraries\U8glib\utility\u8g_dev_ssd1306_128x64.c u8g_dev_ssd1306_128x64_fn)
				//display.setContrast(0);  

				TempSensor.requestTemperaturesByAddress(tempDeviceAddress);
			}

			// prepare all the date before entering the picture loop (onDraw will be called multiple times)
			prepareDraw();

			// picture loop
			display.firstPage();
			do {
				// Display routine
				onDraw();
			} while (display.nextPage());

			Serial.println(millis() - current_time_milis);
		}


	}
	else
	{
		delay(20);
	}

	// Delay to get next current time (10ms), this is essentially time deviation in one second cycle (~ ±10ms)
	delay(10);
}

///////////////////////////////////
//----- Utils
///////////////////////////////////

void ReadStateEPROM()
{
	iYear = EEPROM.read(0) << 8;
	iYear += EEPROM.read(1);
	iMonth = EEPROM.read(2);
	iDay = EEPROM.read(3);
	iHour = EEPROM.read(4);
	iMinutes = EEPROM.read(5);
	iSecond = EEPROM.read(6);
	iTimeFormat = EEPROM.read(7);
}

void WriteStateEPROM()
{
	EEPROM.write(0, iYear >> 8);
	EEPROM.write(1, iYear);
	EEPROM.write(2, iMonth);
	EEPROM.write(3, iDay);
	EEPROM.write(4, iHour);
	EEPROM.write(5, iMinutes);
	EEPROM.write(6, iSecond);
	EEPROM.write(7, iTimeFormat);
}

/// <summary>
/// Calculates the am pm value.
/// </summary>
void calcAmPm()
{
	iAmPm = (iHour > 12);
}

/// <summary>
/// Gets the button input.
/// </summary>
/// <param name="pinNo">The pin number.</param>
/// <param name="btnPinState">State of the BTN pin.</param>
/// <param name="insideDebounce">The inside debounce.</param>
/// <param name="lastDebounceTime">The last debounce time.</param>
void getButtonInput(const byte pinNo, boolean *btnPinState, boolean *insideDebounce, unsigned long *lastDebounceTime)
{
	boolean reading = digitalRead(pinNo);

	if (*insideDebounce == false)
	{
		// steady state, update last debounce time
		*lastDebounceTime = current_time_milis;

		if (reading != *btnPinState) // we have a change of state, start debouncing
		{
			*insideDebounce = true;
		}
	}
	else if ((current_time_milis - *lastDebounceTime) > debounceDelay) // inside debounce period
	{
		// it has passed debounce time since last change of reading press/release of button (start of debouncing) 
		// now read the pin state and update last debounce time

		*btnPinState = reading;
		*lastDebounceTime = current_time_milis;
		*insideDebounce = false;

		anyPinStateChanged = true;
	}

	//Serial.print(pinNo);
	//Serial.print(*insideDebounce);
	//Serial.print(reading);
	//Serial.print(*btnPinState);
	//Serial.println(anyPinStateChanged);
}

/// <summary>
/// Determines whether [is leap year] [the specified year].
/// </summary>
/// <param name="year">The year.</param>
/// <returns>Is leap year</returns>
boolean isLeapYear(short year)
{
	return ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0));
}

/// <summary>
/// Get number of days in the month.
/// </summary>
/// <param name="month">The month.</param>
/// <returns>Number of days in the given month</returns>
inline byte daysInMonth(byte month)
{
	byte odd = month % 2;

	if (month > 7)
	{
		odd = !odd;
	}

	return 30 + odd;
}

/// <summary>
/// Gets the days in month.
/// </summary>
/// <param name="month">The month.</param>
/// <returns>Days in month</returns>
byte getDaysInMonth(byte month)
{
	if (month != 2)
	{
		return daysInMonth(month);
	}
	else // feb
	{
		return  28 + (isLeapYear(iYear));
	}
}

/// <summary>
/// Gets the days passed in year.
/// </summary>
/// <returns>Days passed in year</returns>
inline short daysPassedInYear()
{
	short passed = 0;
	for (byte i = 1; i < iMonth; i++)
	{
		passed += getDaysInMonth(i);
	}

	return passed + iDay;  // Adjust days passed in current month
}

/// <summary>
/// Calculates the day of week index.
/// </summary>
/// <returns>The day of week index.</returns>
void calcDayOfWeek()
{
	short days = dayOffset;

	// Calculates basic number of days passed 
	days += (iYear - firstYear) * 365;
	days += daysPassedInYear();

	// Add on the extra leapdays for past years
	for (short i = firstYear; i < iYear; i += 4)
	{
		if (isLeapYear(i))
		{
			days++;
		}
	}

	iWeek = days % 7;
}

/// <summary>
/// Updates the time.
/// </summary>
/// <returns>
/// whether time is updated - one seccond is ellapsed
/// </returns>
inline boolean updateTime()
{
	short timeElapse = current_time_milis - prevClockTime;
	if (timeElapse >= adjustedUpdateTimeInterval) // check if one second has elapsed
	{
		// Adjust next update time interval in order to reduce accumulated error
		adjustedUpdateTimeInterval = UPDATE_TIME_INTERVAL - (timeElapse - adjustedUpdateTimeInterval);

		// Increase time by incrementing seconds
		if (++iSecond >= 60)
		{
			iSecond = 0;
			if (++iMinutes >= 60)
			{
				iMinutes = 0;
				if (++iHour >= 24)
				{
					iHour = 0;

					inrementRollOverValue(&iWeek, 6);
					calcAmPm();

					if (++iDay > getDaysInMonth(iMonth))
					{
						iDay = 1;
						if (++iMonth > 12)
						{
							iMonth = 1;
							iYear++;
						}
					}
				}
			}
		}

		prevClockTime = current_time_milis;

		return true;
	}

	return false;
}

#define minVal 1
/// <summary>
/// Toggles the option.
/// </summary>
/// <param name="option">The option.</param>
/// <param name="maxVal">The maximum value.</param>
void toggleOption(byte *option, const byte maxVal)
{
	if (btnPinStateUp == LOW) // pressed
	{
		(*option)++;
		if (*option > maxVal)
		{
			*option = minVal;
		}
	}

	if (btnPinStateDown == LOW) // pressed
	{
		(*option)--;
		if (*option < minVal)
		{
			*option = maxVal;
		}
	}
}

/// <summary>
/// Increments and rolls the value over to 0 if grater than roll over value.
/// </summary>
/// <param name="value">The value.</param>
/// <param name="rollOverVal">The roll over value.</param>
void inrementRollOverValue(byte *value, byte rollOverVal)
{
	if (++(*value) > rollOverVal)
	{
		*value = 0;
	}
}

#define SHORT_CHAR_COUNT 5
static const short stoa_tab[SHORT_CHAR_COUNT] = { 1, 10, 100, 1000, 10000 };
/// <summary>
/// Short to string convert
/// </summary>
/// <param name="v">The value.</param>
/// <param name="dest">The string destination.</param>
/// <param name="firstIndex">The index in stoa_tab to add 0 character (e.g. if converting 6 with 1, string will be 06).</param>
/// <returns>String length</returns>
byte stoa(short v, char * dest, byte firstIndex = 0)
{
	char * origDest = dest;
	byte d;
	short c;
	for (byte i = 4; i != 255; i--)
	{
		c = stoa_tab[i];
		if (v >= c)
		{
			d = '0';
			d += v / c;
			v %= c;
			*dest++ = d;
			if (firstIndex == 0)
			{
				firstIndex = i;
			}
		}
		else if (i <= firstIndex)
		{
			*dest++ = '0';
		}
	}

	*dest = '\0';

	return dest - origDest;
}

/// <summary>
/// Converts byte value to two character string.
/// </summary>
/// <param name="value">The value.</param>
/// <param name="s">The string.</param>
void byteToStr(byte value, char* s)
{
	stoa(value, s, 1);
}

/// <summary>
/// Gets the hi and lo from float.
/// </summary>
/// <param name="valHi">The hi value.</param>
/// <param name="valLo">The lo value.</param>
/// <param name="val">The value.</param>
inline void floatToHiLo(byte* valHi, byte* valLo, float val)
{
	*valHi = (byte)val;
	*valLo = ((short)(val * 100)) % 100;
}

/// <summary>
/// Converts hi and lo to float.
/// </summary>
/// <param name="valHi">The hi value.</param>
/// <param name="valLo">The lo value.</param>
/// <returns>Float value</returns>
float hiLoToFloat(byte valHi, byte valLo)
{
	return valHi + valLo / 100.0;
}

/// <summary>
/// Finds the maximum and minimum.
/// </summary>
void findMaxMin()
{
	maxHi = minHi = tempGraphHi[0];
	maxLo = minLo = tempGraphLo[0];

	for (byte i = 0; i < TEMP_GRAPH_LEN; i++)
	{
		if (tempGraphHi[i] > maxHi || (tempGraphHi[i] == maxHi && tempGraphLo[i] > maxLo))
		{
			maxHi = tempGraphHi[i];
			maxLo = tempGraphLo[i];
		}

		if (tempGraphHi[i] < minHi || (tempGraphHi[i] == minHi && tempGraphLo[i] < minLo))
		{
			minHi = tempGraphHi[i];
			minLo = tempGraphLo[i];
		}
	}
}

///////////////////////////////////
//----- Drawing methods
///////////////////////////////////

/// <summary>
/// prepares the data for draw
/// </summary>
inline void prepareDraw()
{
	byte oldDisplayMode = displayMode;
	// First determine what to display, set displayMode
	if (btnPinState == LOW) // pressed
	{
		if (displayMode < 4)
		{
			displayMode++;
		}
	}

	if (btnPinStateBack == LOW) // pressed
	{
		if (displayMode > 1)
		{
			displayMode--;
		}
	}

	// prepare the data for displaying
	switch (displayMode)
	{
	case DISPLAY_MODE_START_UP:
		// Nothing to be done
		break;

	case DISPLAY_MODE_CLOCK:
		toggleOption(&clockStyle, 6);
		prepareDrawClock();
		break;

	case DISPLAY_MODE_MENU:
		toggleOption(&menuMode, 3);
		setPosition = 0;
		break;

	case DISPLAY_MODE_SET_MENU:
		if (oldDisplayMode == displayMode && btnPinState == LOW) // pressed
		{
			// Go to next set value
			inrementRollOverValue(&setPosition, menuMode - 1);
		}

		prepareDrawSetMenu();
		break;
	}

	// TODO: do I need this?
	btnPinState = HIGH;
	btnPinStateUp = HIGH;
	btnPinStateDown = HIGH;
	btnPinStateBack = HIGH;
}  // End of prepareDraw()

/// <summary>
/// Main drawing routine.
/// Every drawing starts here.
/// </summary>
inline void onDraw()
{
	switch (displayMode)
	{
	case DISPLAY_MODE_START_UP:
		drawStartUp();
		break;

	case DISPLAY_MODE_CLOCK:
		drawClock();
		break;

	case DISPLAY_MODE_MENU:
		drawMenu();
		break;

	case DISPLAY_MODE_SET_MENU:
		drawSetMenu();
		break;

	}
}  // End of onDraw()

void prepareDrawSetMenu()
{
	switch (menuMode)
	{
	case MENU_SET_DATE:
		switch (setPosition)
		{
		case 0: // DD
			toggleOption(&iDay, getDaysInMonth(iMonth));
			break;
		case 1: // MM
			toggleOption(&iMonth, 12);
			break;
		case 2: // YYYY
			if (btnPinStateUp == LOW) // pressed
			if (++iYear > 32767)
				iYear = 2000;
			if (btnPinStateDown == LOW) // pressed
			if (--iYear < 2000)
				iYear = 2000;
			break;
		}

		// Calculate week index and update it
		calcDayOfWeek();
		prepareDrawDateDigital();
		break;

	case MENU_SET_TIME:
		switch (setPosition)
		{
		case 0: //HH
			toggleOption(&iHour, 24);
			break;
		case 1: //MM
			toggleOption(&iMinutes, 60);
			break;
		}

		prepareDrawClockDigital();
		break;

	case MENU_SET_TIME_FORMAT:
		toggleOption(&iTimeFormat, 2);

		break;
	}
}

/// <summary>
/// Draws the set menu.
/// </summary>
inline void drawSetMenu()
{
	display.setFont(helvB10r);

	display.drawHLine(29 + setPosition * 16, 12 + 14, 12);

	switch (menuMode)
	{
	case MENU_SET_DATE:
		drawDateDigital(29, 12);
		break;
	case MENU_SET_TIME:
		drawClockDigital(29, 12);
		break;
	case MENU_SET_TIME_FORMAT:
		drawTimeFormat(29, 12);
		break;
	}
}

/// <summary>
/// Draws the start up splash screen.
/// </summary>
inline void drawStartUp()
{
	display.setFont(helvB10r);

	//Arguments:
	// u8g : Pointer to the u8g structure(C interface only).
	// x : X - position(left position of the bitmap).
	// y : Y - position(upper position of the bitmap).
	// cnt : Number of bytes of the bitmap in horizontal direction.The width of the bitmap is cnt*8.
	// h : Height of the bitmap. 
	display.drawBitmapP(10, 10, 3, 24, IMG_logo_24x24);

	display.drawStr(25, 12, "Temperature");

	display.drawStr(35, 28, "Watch");

	display.drawStr(25, 45, "Arduino v1.0");
}

/// <summary>
/// Draws the graph line.
/// </summary>
/// <param name="start">The start.</param>
/// <param name="end">The end.</param>
/// <param name="x">The x.</param>
/// <param name="rescale">The rescale.</param>
void drawGraphLine(byte start, byte end, byte* x, float rescale)
{
	for (byte i = start; i < end; i++)
	{
		display.drawPixel((*x)++, (hiLoToFloat(tempGraphHi[i], tempGraphLo[i]) - minTemp) * rescale);
	}
}

char temperatureMin[6];
byte offsetMinSuff;
char temperatureMax[6];
byte offsetMaxSuff;

float yCoord;
byte yScaleCount;
float rescale;
void prepareDrawGraph()
{
	findMaxMin();
	minTemp = hiLoToFloat(minHi, minLo);
	float diff = hiLoToFloat(maxHi, maxLo) - minTemp;
	if (diff < 0.5)
	{
		diff = 0.5;
	}

	rescale = 64.0 / diff;

	// draw scale
	// calculate first y coord
	byte startValHi, startValLo;
	startValHi = minHi;
	if ((minLo > 50))
	{
		startValHi += 1;
		startValLo = 0;
	}
	else
	{
		startValLo = 50;
	}

	yCoord = hiLoToFloat(startValHi, startValLo);
	yScaleCount = diff / 0.5;

	offsetMinSuff = prepareDrawTemp(minHi, minLo, temperatureMin);
	offsetMaxSuff = prepareDrawTemp(maxHi, maxLo, temperatureMax);
}

/// <summary>
/// Draws the start up splash screen.
/// </summary>
void drawGraph()
{
	byte i;
	byte xPos = 0;

	for (i = 0; i < yScaleCount; i++)
	{
		// calculate y coord
		display.drawHLine(0, (yCoord - minTemp) * rescale, 2);
		yCoord += 0.5;
	}

	for (i = 0; i < TEMP_GRAPH_LEN; i += 12)
	{
		display.drawVLine(i, 62, 2);
	}

	drawGraphLine(startTempGraphIndex, TEMP_GRAPH_LEN, &xPos, rescale);
	drawGraphLine(0, startTempGraphIndex, &xPos, rescale);
	drawTemp(15, 10, temperatureMax, offsetMaxSuff);
	drawTemp(15, 60, temperatureMin, offsetMinSuff);

}

byte monthOffset;
byte daysInM;
void prepareDrawCalendar()
{
	monthOffset = iWeek - iDay % 7 + 1;
	daysInM = getDaysInMonth(iMonth);
}

/// <summary>
/// Draws the start up splash screen.
/// </summary>
void drawCalendar()
{
	byte x, y;
	byte j, i, dayCnt = 1;
	for (byte i = 0; i < 6; i++)
	{
		x = 5;
		y = 10;
		for (j = 0; j < 7; j++)
		{
			if (i == 0)
			{
				// Daraw days
				display.drawStr(x, y, (const char*)pgm_read_word(&(dayString[j])));
				x += 15;
			}
			else
			{
				if (i == 1 && monthOffset > j ||
					daysInM < dayCnt)
				{
					continue;
				}

				char day[3];
				stoa(dayCnt++, day);
				display.drawStr(x, y, day);
			}
		}

		y += 10;
	}
}

/// <summary>
/// Draw the main manu screen.
/// Menu changes according to user selection.
/// </summary>
inline void drawMenu()
{
	display.setFont(helvB10r);
	display.drawStr(10, 5, (const char*)pgm_read_word(&(menuItems[MENU_SET_TIME_FORMAT - 1])));
	display.drawStr(10, 25, (const char*)pgm_read_word(&(menuItems[MENU_SET_TIME - 1])));
	display.drawStr(10, 45, (const char*)pgm_read_word(&(menuItems[MENU_SET_DATE - 1])));

	display.setFontRefHeightExtendedText();
	display.setFontPosTop();

	byte h = display.getFontAscent() - display.getFontDescent();
	byte w = display.getStrWidth((const char*)pgm_read_word(&(menuItems[menuMode - 1])));

	display.drawFrame(9, 5 + (menuMode - 1) * 20, w + 2, h + 2);
}
char temperature[6];
byte offsetSuffix;
void prepareDrawClock()
{
	switch (clockStyle)
	{
	case CLOCK_STYLE_SIMPLE_DIGIT:
		offsetSuffix = prepareDrawTemp(tempHi, tempLo, temperature);
		prepareDrawDayAmPm();
		prepareDrawClockDigital();
		prepareDrawLumens();
		break;

	case CLOCK_STYLE_SIMPLE_MIX:
		//drawClockAnalog
		offsetSuffix = prepareDrawTemp(tempHi, tempLo, temperature);
		prepareDrawDayAmPm();
		prepareDrawClockDigital();
		prepareDrawLumens();
		break;

	case CLOCK_STYLE_SIMPLE_ANALOG:
		//drawClockAnalog
		offsetSuffix = prepareDrawTemp(tempHi, tempLo, temperature);
		prepareDrawLumens();
		break;

	case CLOCK_STYLE_SIMPLE_DIGIT_SEC:
		offsetSuffix = prepareDrawTemp(tempHi, tempLo, temperature);
		prepareDrawDateDigital();
		prepareDrawClockDigital();
		prepareDrawSecondsDigital();
		prepareDrawLumens();
		break;

	case CLOCK_STYLE_SIMPLE_GRAPH:
		prepareDrawGraph();
		break;

	case CLOCK_STYLE_SIMPLE_CALENDAR:
		prepareDrawCalendar();
		break;
	}
}

/// <summary>
/// Draw the main clock screen.
/// Clock style changes according to user selection.
/// </summary>
inline void drawClock()
{
	byte offset;

	switch (clockStyle)
	{
	case CLOCK_STYLE_SIMPLE_DIGIT:
		display.setFont(helvB08r);
		drawLumens(5, 63);

		display.setFont(helvB14r);
		drawDayAmPm(30, 15);

		display.setFont(helvB18r);
		drawClockDigital(35, 38);

		display.setFont(helvB12r);
		drawTemp(80, 63, temperature, offsetSuffix);
		break;

	case CLOCK_STYLE_SIMPLE_MIX:
		display.setFont(helvB08r);
		drawLumens(5, 63);

		drawClockAnalog(centerX - 30, centerY, iRadius - 4);

		display.setFont(helvB12r);
		drawDayAmPm(67, 23);

		display.setFont(helvB18r);
		drawClockDigital(61, 45);

		display.setFont(helvB12r);
		drawTemp(80, 63, temperature, offsetSuffix);
		break;

	case CLOCK_STYLE_SIMPLE_ANALOG:
		display.setFont(helvB08r);
		drawLumens(5, 63);

		drawClockAnalog(centerX - 10, centerY, iRadius);

		display.setFont(helvB12r);
		drawTemp(80, 63, temperature, offsetSuffix);
		break;

	case CLOCK_STYLE_SIMPLE_DIGIT_SEC:
		display.setFont(helvB08r);
		drawLumens(5, 63);

		display.setFont(helvB10r);
		drawDateDigital(40, 12);
		drawDay(6, 12);

		display.setFont(helvB24r);
		offset = drawClockDigital(14, 45);

		display.setFont(helvB14r);
		drawSecondsDigital(14 + offset + 2, 45);

		display.setFont(helvB12r);
		drawTemp(80, 63, temperature, offsetSuffix);
		break;

	case CLOCK_STYLE_SIMPLE_GRAPH:
		display.setFont(helvB08r);
		drawGraph();
		break;

	case CLOCK_STYLE_SIMPLE_CALENDAR:
		display.setFont(helvB08r);
		drawCalendar();
		break;
	}
}

/// <summary>
/// Draws the time format (12/24).
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawTimeFormat(byte xPos, byte yPos)
{
	display.drawStr(xPos, yPos, (const char*)pgm_read_word(&(timeFormat[iTimeFormat - 1])));
}

byte amPmOffset;
void prepareDrawDayAmPm()
{
	if (iTimeFormat == TF_12h) // 1 = 12h, 2 = 24h
	{
		amPmOffset = display.getStrPixelWidth((const char*)pgm_read_word(&(weekString[iWeek]))) + 2;
	}
}

/// <summary>
/// Draws the day in week and am/pm.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawDayAmPm(byte xPos, byte yPos)
{
	drawDay(xPos, yPos);
	if (iTimeFormat == TF_12h) // 1 = 12h, 2 = 24h
	{
		display.drawStr(xPos + amPmOffset, yPos, (const char*)pgm_read_word(&(ampmString[iAmPm])));
	}
}

/// <summary>
/// Draws the day.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawDay(byte xPos, byte yPos)
{
	display.drawStr(xPos, yPos, (const char*)pgm_read_word(&(weekString[iWeek])));
}

byte prepareDrawTemp(byte tmpHi, byte tmpLo, char* temperatureStr)
{
	byte offset = stoa(tmpHi, temperatureStr);

	temperatureStr[offset] = '.';

	byteToStr(tmpLo, temperatureStr + offset + 1);

	return display.getStrPixelWidth(temperatureStr) + 1;
}

/// <summary>
/// Draws the temperature (x,y is the possition of '.').
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
/// <param name="temperatureStr">The temperature string.</param>
/// <param name="offsetSuff">The offset suff.</param>
void drawTemp(byte xPos, byte yPos, char* temperatureStr, byte offsetSuff)
{
	display.drawStr(xPos, yPos, temperatureStr);
	display.drawStr(xPos + offsetSuff, yPos, tempSufix);
}

char clockDigital[6];
byte prepareDrawClockDigital()
{
	if (iTimeFormat == TF_12h && iAmPm == 1) // 1 = PM
	{
		byteToStr(iHour - 12, clockDigital);
	}
	else
	{
		byteToStr(iHour, clockDigital);
	}

	clockDigital[2] = ':';

	byteToStr(iMinutes, clockDigital + 3);
}

/// <summary>
/// Draws the clock digital.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
/// <returns>Length in pixels of the clock</returns>
byte drawClockDigital(byte xPos, byte yPos)
{
	display.drawStr(xPos, yPos, clockDigital);
	return display.getStrPixelWidth(clockDigital);
}

char lumens[7];
void prepareDrawLumens()
{
	byte len = stoa(lux, lumens);
	lumens[len] = 'l';
	lumens[++len] = 'm';
	lumens[++len] = '\0';
}

/// <summary>
/// Draws the lumens.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawLumens(byte xPos, byte yPos)
{
	display.drawStr(xPos, yPos, lumens);
}

char seconds[3];
void prepareDrawSecondsDigital()
{
	byteToStr(iSecond, seconds);
}

/// <summary>
/// Draws the seconds digital.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawSecondsDigital(byte xPos, byte yPos)
{
	display.drawStr(xPos, yPos, seconds);
}

char dateDigital[11];
void prepareDrawDateDigital()
{
	byteToStr(iDay, dateDigital);

	dateDigital[2] = '/';

	byteToStr(iMonth, dateDigital + 3);

	dateDigital[5] = '/';

	stoa(iYear, dateDigital + 6);
}

/// <summary>
/// Draws the date digital.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawDateDigital(byte xPos, byte yPos)
{
	display.drawStr(xPos, yPos, dateDigital);
}

/// <summary>
/// Draws the clock analog.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
/// <param name="radius">The radius.</param>
void drawClockAnalog(byte xPos, byte yPos, byte radius)
{
	display.drawCircle(xPos, yPos, radius);

	// print hour pin lines
	for (byte i = 0; i < 60; i += 5)
	{
		showTimePin(xPos, yPos, 0.9, 1, i, radius, 1);
	}

	showTimePin(xPos, yPos, 0.1, 0.5, iHour * 5 + iMinutes / 12, radius, -1);
	showTimePin(xPos, yPos, 0.1, 0.78, iMinutes, radius, -1);
	// showTimePin(xPos, yPos, 0.1, 0.9, iSecond, radius, -1);
}

// Calculate clock pin position
#define RAD 0.01745329251994329576923690768489 // = Pi / 180;
#define LR 89.999
#define roundToByte(x) (byte)((x)+0.5)

byte getPointCoordinate(byte center, byte radius, double pl, byte angle, byte sign, double(*trigFn)(double))
{
	return center + roundToByte((radius * pl) * (*trigFn)((6 * angle + LR * sign) * RAD));
}

void showTimePin(byte center_x, byte center_y, double pl1, double pl2, byte angle, byte radius, byte sign)
{
	byte x1, x2, y1, y2;

	x1 = getPointCoordinate(center_x, radius, pl1, angle, 1, &cos);
	y1 = getPointCoordinate(center_y, radius, pl1, angle, 1, &sin);
	x2 = getPointCoordinate(center_x, radius, pl2, angle, sign, &cos);
	y2 = getPointCoordinate(center_y, radius, pl2, angle, sign, &sin);

	display.drawLine(x1, y1, x2, y2);
}
