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
U8GLIB_SSD1306_128X64 display(U8G_I2C_OPT_NONE);	// I2C / TWI 
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
short iYear = 2014;
byte iMonth = 11;
byte iDay = 6;
byte iWeek = 0;    // need to calculate this during setup and on date change
byte iAmPm = 1;    // 0:AM, 1:PM
byte iHour = 9;
byte iMinutes = 0;
byte iSecond = 0;
byte iTimeFormat = 0;
unsigned long prevClockTime = 0;
#define TEMP_GRAPH_LEN 128
byte tempGraphHi[TEMP_GRAPH_LEN] = { 0 };
byte tempGraphLo[TEMP_GRAPH_LEN] = { 0 };
byte startTempGraphIndex = 0;
short hourCount = 0;
float tempAccum = 0;

//----- Strings
PGM_P const weekString[] PROGMEM = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
PGM_P const ampmString[] PROGMEM = { "AM", "PM" };
PGM_P const menuItems[] PROGMEM = { "Set date", "Set time", "Set format" };
PGM_P const timeFormat[] PROGMEM = { "12h", "24h" };

PROGMEM const byte daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; // standard year days

//PGM_P const dayNames[] PROGMEM = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
//PGM_P const months[] PROGMEM = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

#define firstYear 2000 //This is our start point
#define dayOffset 6 //The first day of our start year may not be a Sunday ( in 1800 it was Wednesday)

//----- Display features
#define DISPLAY_MODE_START_UP	0x0
#define DISPLAY_MODE_CLOCK		0x1
#define DISPLAY_MODE_MENU		0x2
#define DISPLAY_MODE_SET_MENU	0x3
byte displayMode = DISPLAY_MODE_START_UP;

#define MENU_SET_DATE			0x2
#define MENU_SET_TIME			0x1
#define MENU_SET_TIME_FORMAT	0x0
byte menuMode = MENU_SET_DATE;

#define CLOCK_STYLE_SIMPLE_ANALOG		0x0
#define CLOCK_STYLE_SIMPLE_DIGIT		0x1
#define CLOCK_STYLE_SIMPLE_MIX			0x2
#define CLOCK_STYLE_SIMPLE_DIGIT_SEC	0x3
#define CLOCK_STYLE_SIMPLE_GRAPH		0x4
byte clockStyle = CLOCK_STYLE_SIMPLE_DIGIT_SEC;

#define centerX 64
#define centerY 32
#define iRadius 30

//----- Button control
#define buttonPin 5
boolean isClicked = HIGH; // LOW = false = 0x0 = Pressed, HIGH = true = 0x1 = Not pressed
boolean insideDebounce = false;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled

#define buttonPinUp 6
boolean isClickedUp = HIGH; // LOW = false = 0x0 = Pressed, HIGH = true = 0x1 = Not pressed
boolean insideDebounceUp = false;
unsigned long lastDebounceTimeUp = 0;  // the last time the output pin was toggled

#define buttonPinDown 7
boolean isClickedDown = HIGH; // LOW = false = 0x0 = Pressed, HIGH = true = 0x1 = Not pressed
boolean insideDebounceDown = false;
unsigned long lastDebounceTimeDown = 0;  // the last time the output pin was toggled

#define buttonPinBack 8
boolean isClickedBack = HIGH; // LOW = false = 0x0 = Pressed, HIGH = true = 0x1 = Not pressed
boolean insideDebounceBack = false;
unsigned long lastDebounceTimeBack = 0;  // the last time the output pin was toggled

boolean isChanged = false;
#define debounceDelay 50    // the debounce time; increase if the output flickers

//----- Global
byte tempLo = 0;
byte tempHi = 0;
byte setPosition = 0; // position of value currently being set

byte maxHi = 0;
byte maxLo = 0;
byte minHi = 0;
byte minLo = 0;

float min = 0;

#define HOUR_COUNT 1 //3600

unsigned long current_time_milis = 0;

///////////////////////////////////
//----- Arduino setup and loop methods
///////////////////////////////////

void setup()
{
	Serial.begin(9600);    // Do not enable serial. This makes serious problem because of shortage of RAM.
	pinMode(buttonPin, INPUT_PULLUP);  // Defines button pin, turn on internal Pull-Up Resistor
	pinMode(buttonPinUp, INPUT_PULLUP);  // Defines button pin
	pinMode(buttonPinDown, INPUT_PULLUP);  // Defines button pin
	pinMode(buttonPinBack, INPUT_PULLUP);  // Defines button pin

	// By default, we'll generate the high voltage from the 3.3v line internally! (neat!)
	// Assign default color value
	display.setColorIndex(1);         // pixel on BW

	iWeek = calcDayOfWeek();
	(iHour > 12) ? iAmPm = 1 : iAmPm = 0;

	// Start up the temperature sensor library
	TempSensor.begin();

	// Start up the light sensor library
	//LightSensor.begin();
	//LightSensor.SetAddress(Device_Address_H);//Address 0x5C
	//LightSensor.SetMode(Continuous_H_resolution_Mode);
}

void loop()
{
	isChanged = false;

	// Update clock time
	current_time_milis = millis();

	// Get button input
	getButtonInput(buttonPin, &isClicked, &insideDebounce, &lastDebounceTime);
	getButtonInput(buttonPinUp, &isClickedUp, &insideDebounceUp, &lastDebounceTimeUp);
	getButtonInput(buttonPinDown, &isClickedDown, &insideDebounceDown, &lastDebounceTimeDown);
	getButtonInput(buttonPinBack, &isClickedBack, &insideDebounceBack, &lastDebounceTimeBack);

	boolean timeUpdated = updateTime();
	if (timeUpdated || isChanged)
	{
		// One second has elapsed or we have input (button clicked)

		if (timeUpdated)
		{
			TempSensor.requestTemperaturesByIndex(0); // Send the command to get temperatures
			float temp = TempSensor.getTempCByIndex(0);
			tempAccum += temp;

			floatToHiLo(&tempHi, &tempLo, temp);

			hourCount++;
			if (hourCount > HOUR_COUNT)
			{
				// One hour has elapsed
				floatToHiLo(&tempGraphHi[startTempGraphIndex], &tempGraphLo[startTempGraphIndex], tempAccum / HOUR_COUNT);

				startTempGraphIndex++;
				rollOverValue(&startTempGraphIndex, TEMP_GRAPH_LEN);

				tempAccum = 0;
				hourCount = 0;
			}

			//uint16_t lux = LightSensor.GetLightIntensity();// Get Lux value

			//dim display (Arduino\libraries\U8glib\utility\u8g_dev_ssd1306_128x64.c u8g_dev_ssd1306_128x64_fn)
			//display.setContrast(0);  
		}

		// picture loop
		display.firstPage();
		do {
			// Display routine
			onDraw();
		} while (display.nextPage());
	}

	// Delay to get next current time (10ms), this is essentially time deviation in one second cycle (~ +-10ms)
	delay(200);
}

///////////////////////////////////
//----- Utils
///////////////////////////////////

/// <summary>
/// Gets the button input.
/// </summary>
/// <param name="pin">The pin number.</param>
/// <param name="clicked">The clicked button pointer.</param>
/// <param name="insideDebounce">The inside debounce.</param>
/// <param name="lastDebounceTime">The last debounce time.</param>
void getButtonInput(byte pin, boolean *clicked, boolean *insideDebounce, unsigned long *lastDebounceTime)
{
	boolean reading = digitalRead(pin);

	if (*insideDebounce == false)
	{
		if (reading == HIGH && *clicked == HIGH) // not pressed now and was not pressed before
		{
			*lastDebounceTime = current_time_milis;
		}
		else if (reading == LOW && *clicked == HIGH) // pressed now and was not pressed before
		{
			*insideDebounce = true;
		}
		else if (reading == LOW && *clicked == LOW) // pressed now and was pressed before
		{
			*clicked = HIGH;
		}
	}
	else if (*clicked == HIGH && (current_time_milis - *lastDebounceTime) > debounceDelay) // button was not pressed, inside debounce period
	{
		*lastDebounceTime = current_time_milis;

		*clicked = reading;

		isChanged = true;

		*insideDebounce = false;
	}

	Serial.print(pin);
	Serial.print(*insideDebounce);
	Serial.print(reading);
	Serial.print(*clicked);
	Serial.println(isChanged);
}

/// <summary>
/// Determines whether [is leap year] [the specified year].
/// </summary>
/// <param name="year">The year.</param>
/// <returns></returns>
boolean isLeapYear(short year)
{
	return ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0));
}

/// <summary>
/// Gets the days in month.
/// </summary>
/// <param name="month">The month.</param>
/// <returns>Days in month</returns>
byte getDaysInMonth(byte month)
{
	month--; // Adjust for indexing in daysInMonth
	if (month != 1)
	{
		return daysInMonth[month];
	}
	else // feb
	{
		return (isLeapYear(iYear)) ? 29 : 28;
	}
}

/// <summary>
/// Gets the days passed in year.
/// </summary>
/// <returns>Days passed in year</returns>
short daysPassedInYear()
{
	short passed = 0;
	for (byte i = 1; i <= iMonth; i++)
	{
		passed += getDaysInMonth(i);
	}

	return passed + iDay - 1;  // Adjust days passed in current month
}

/// <summary>
/// Calculates the day of week index.
/// </summary>
/// <returns>The day of week index.</returns>
byte calcDayOfWeek()
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

	return days % 7;
}

/// <summary>
/// Updates the time.
/// </summary>
/// <returns>
/// whether time is updated - one seccond is ellapsed
/// </returns>
boolean updateTime()
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
					if (++iWeek > 6)
					{
						iWeek = 0;
					}

					(iHour > 12) ? iAmPm = 1 : iAmPm = 0;

					iDay++;
					if (iDay > getDaysInMonth(iMonth))
					{
						iDay = 1;
						if (++iMonth > 12)
						{
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

/// <summary>
/// Toggles the option.
/// </summary>
/// <param name="option">The option.</param>
/// <param name="minVal">The minimum value.</param>
/// <param name="maxVal">The maximum value.</param>
void toggleOption(byte *option, byte minVal, byte maxVal)
{
	if (isClickedUp == LOW) // pressed
	{
		(*option)++;
		if (*option > maxVal)
		{
			*option = minVal;
		}
	}

	if (isClickedDown == LOW) // pressed
	{
		(*option)--;
		if (*option < minVal)
		{
			*option = maxVal;
		}
	}
}

/// <summary>
/// Rolls the value over to 0 if grater than roll over value.
/// </summary>
/// <param name="value">The value.</param>
/// <param name="rollOverVal">The roll over value.</param>
void rollOverValue(byte *value, byte rollOverVal)
{
	if (*value > rollOverVal)
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
void stoa(short v, char * dest)
{
	byte d;
	short c;
	byte firstIndex = 0;
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
}

/// <summary>
/// Converts byte value to two character string.
/// </summary>
/// <param name="value">The value.</param>
/// <param name="s">The string.</param>
void byteToStr(byte value, char* s)
{
	if (value < 10)
	{
		s[0] = '0';
		stoa(value, s + 1);
	}
	else
	{
		stoa(value, s);
	}
}

/// <summary>
/// Gets the hi and lo from float.
/// </summary>
/// <param name="hiVal">The hi value.</param>
/// <param name="loVal">The lo value.</param>
/// <param name="val">The value.</param>
inline void floatToHiLo(byte* hiVal, byte* loVal, float val)
{
	*hiVal = (byte)val;
	*loVal = ((short)(val * 100)) % 100;
}

/// <summary>
/// Converts hi and lo to float.
/// </summary>
/// <param name="hiVal">The hi value.</param>
/// <param name="loVal">The lo value.</param>
/// <returns></returns>
float hiLoToFloat(byte hiVal, byte loVal)
{
	return hiVal + loVal / 100.0;
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
/// Main drawing routine.
/// Every drawing starts here.
/// </summary>
void onDraw()
{
	display.setFont(u8g_font_helvB10r);

	switch (displayMode)
	{
	case DISPLAY_MODE_START_UP:
		drawStartUp();
		break;

	case DISPLAY_MODE_CLOCK:
		if (isClicked == LOW) // pressed
		{
			displayMode = DISPLAY_MODE_MENU;
			break;
		}

		toggleOption(&clockStyle, 0, 4);

		drawClock();

		break;

	case DISPLAY_MODE_MENU:
		if (isClicked == LOW) // pressed
		{
			displayMode = DISPLAY_MODE_SET_MENU;
			break;
		}

		if (isClickedBack == LOW) // pressed
		{
			// Go back
			displayMode = DISPLAY_MODE_CLOCK;
			menuMode = 0;
			break;
		}

		toggleOption(&menuMode, 0, 2);

		drawMenu();

		break;

	case DISPLAY_MODE_SET_MENU:
		if (isClickedBack == LOW) // pressed
		{
			// Go back
			displayMode = DISPLAY_MODE_MENU;
			setPosition = 0;
			break;
		}

		if (isClicked == LOW) // pressed
		{
			// Go to next set value
			setPosition++;
			rollOverValue(&setPosition, menuMode);
		}

		drawSetMenu();

		break;
	}

	isClicked = HIGH;
	isClickedUp = HIGH;
	isClickedDown = HIGH;
}  // End of onDraw()

/// <summary>
/// Draws the set menu.
/// </summary>
void drawSetMenu()
{
	switch (menuMode)
	{
	case MENU_SET_DATE:
		switch (setPosition)
		{
		case 0:
			toggleOption(&iDay, 1, 12);
			break;
		case 1:
			toggleOption(&iMonth, 1, 60);
			break;
		case 2:
			if (isClickedUp == LOW) // pressed
			if (++iYear > 32767)
				iYear = 2000;
			if (isClickedDown == LOW) // pressed
			if (--iYear < 2000)
				iYear = 2000;
			break;
		}

		// Calculate week index
		iWeek = calcDayOfWeek();

		drawDateDigital(29, 12);
		display.drawHLine(29, 12 + 10, 5);

		break;
	case MENU_SET_TIME:
		if (setPosition) // 1 = MM
		{
			toggleOption(&iMinutes, 1, 60);
		}
		else // 0 = HH
		{
			toggleOption(&iHour, 1, 24);
		}

		drawClockDigital(29, 12);
		display.drawHLine(29, 12 + 10, 5);

		break;
	case MENU_SET_TIME_FORMAT:
		toggleOption(&iTimeFormat, 0, 1);

		drawTimeFormat(29, 12);
		display.drawHLine(29, 12 + 10, 5);

		break;
	}
}

/// <summary>
/// Draws the start up splash screen.
/// </summary>
void drawStartUp()
{
	//Arguments:
	// u8g : Pointer to the u8g structure(C interface only).
	// x : X - position(left position of the bitmap).
	// y : Y - position(upper position of the bitmap).
	// cnt : Number of bytes of the bitmap in horizontal direction.The width of the bitmap is cnt*8.
	// h : Height of the bitmap. 
	//display.drawBitmapP(10, 10, 3, 24, IMG_logo_24x24);

	//display.drawStr(25, 12, "Temperature");

	display.drawStr(35, 28, "Watch");

	//display.drawStr(25, 45, "Arduino v1.0");

	displayMode = DISPLAY_MODE_CLOCK;
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
		display.drawPixel((*x)++, (hiLoToFloat(tempGraphHi[i], tempGraphLo[i]) - min) * rescale);
	}
}

/// <summary>
/// Draws the start up splash screen.
/// </summary>
void drawGraph()
{
	byte i;
	byte xPos = 0;

	findMaxMin();

	min = hiLoToFloat(minHi, minLo);

	float diff = hiLoToFloat(maxHi, maxLo) - min;
	float rescale = 64.0 / diff;

	// draw scale
	// calculate first y coord
	byte hiVal, loVal;
	if ((minLo > 50))
	{
		hiVal = minHi + 1;
		loVal = 0;
	}
	else
	{
		hiVal = minHi;
		loVal = 50;
	}

	float yCoord = hiLoToFloat(hiVal, loVal);

	byte yScaleCount = diff / 0.5;
	for (i = 0; i < yScaleCount; i++)
	{
		// calculate y coord
		display.drawHLine(0, (yCoord - min) * rescale, 2);
		yCoord += 0.5;
	}

	for (i = 0; i < TEMP_GRAPH_LEN; i += 12)
	{
		display.drawVLine(i, 60, 2);
	}

	drawGraphLine(startTempGraphIndex, TEMP_GRAPH_LEN, &xPos, rescale);
	drawGraphLine(0, startTempGraphIndex, &xPos, rescale);
}

/// <summary>
/// Draw the main manu screen.
/// Menu changes according to user selection.
/// </summary>
void drawMenu()
{
	display.setFont(u8g_font_helvB10r);
	display.drawStr(10, 45, (const char*)pgm_read_word(&(menuItems[MENU_SET_DATE])));
	display.drawStr(10, 25, (const char*)pgm_read_word(&(menuItems[MENU_SET_TIME])));
	display.drawStr(10, 5, (const char*)pgm_read_word(&(menuItems[MENU_SET_TIME_FORMAT])));

	display.setFontRefHeightExtendedText();
	display.setFontPosTop();

	byte h = display.getFontAscent() - display.getFontDescent();
	byte w = display.getStrWidth((const char*)pgm_read_word(&(menuItems[menuMode])));;

	display.drawFrame(9, 5 + menuMode * 20, w + 2, h + 2);
}

/// <summary>
/// Draw the main clock screen.
/// Clock style changes according to user selection.
/// </summary>
void drawClock()
{
	byte offset;

	switch (clockStyle)
	{
	case CLOCK_STYLE_SIMPLE_DIGIT:
		display.setFont(u8g_font_helvB12);
		drawTemp(80, 63);

		display.setFont(u8g_font_helvB14r);
		drawDayAmPm(30, 15);

		display.setFont(u8g_font_helvB18r);
		drawClockDigital(35, 38);

		break;

	case CLOCK_STYLE_SIMPLE_MIX:
		drawClockAnalog(centerX - 30, centerY, iRadius - 4);

		display.setFont(u8g_font_helvB12);
		drawTemp(80, 63);

		//display.setFont(u8g_font_helvB12);
		drawDayAmPm(67, 23);

		display.setFont(u8g_font_helvB18r);
		drawClockDigital(61, 45);
		break;

	case CLOCK_STYLE_SIMPLE_ANALOG:
		display.setFont(u8g_font_helvB12);
		drawTemp(80, 63);

		drawClockAnalog(centerX - 10, centerY, iRadius);

		break;

	case CLOCK_STYLE_SIMPLE_DIGIT_SEC:
		display.setFont(u8g_font_helvB10r);
		drawDateDigital(29, 12);

		display.setFont(u8g_font_helvB12);
		drawTemp(80, 63);
		drawDay(5, 16);

		display.setFont(u8g_font_helvB24n);
		offset = drawClockDigital(14, 45);

		display.setFont(u8g_font_helvB14r);
		drawSecondsDigital(14 + offset + 2, 45);

		break;

	case CLOCK_STYLE_SIMPLE_GRAPH:
		drawGraph();

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
	display.drawStr(xPos, yPos, (const char*)pgm_read_word(&(timeFormat[iTimeFormat])));
}

/// <summary>
/// Draws the day in week and am/pm.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawDayAmPm(byte xPos, byte yPos)
{
	drawDay(xPos, yPos);
	if (iTimeFormat == 0) // 0 = 12h
	{
		display.drawStr(xPos + display.getStrPixelWidth((const char*)pgm_read_word(&(weekString[iWeek]))) + 2, yPos, (const char*)pgm_read_word(&(ampmString[iAmPm])));
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

/// <summary>
/// Draws the temperature (x,y is the possition of '.').
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawTemp(byte xPos, byte yPos)
{
	char s[4];
	byte offset;

	stoa(tempHi, s);
	display.drawStr(xPos - display.getStrPixelWidth(s) + 1, yPos, s);

	display.drawStr(xPos, yPos, ".");
	offset = display.getStrPixelWidth(".") + 1;

	byteToStr(tempLo, s);

	display.drawStr(xPos + offset, yPos, s);
	offset += display.getStrPixelWidth(s) + 1;

	s[0] = '°', s[1] = 'C', s[3] = 0;
	display.drawStr(xPos + offset, yPos, s);
}

/// <summary>
/// Draws the clock digital.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
/// <returns>Length in pixels of the clock</returns>
byte drawClockDigital(byte xPos, byte yPos)
{
	char s[6];

	if (iAmPm && iTimeFormat == 0) // 1 = PM && 0 = 12h
	{
		byteToStr(iHour - 12, s);
	}
	else
	{
		byteToStr(iHour, s);
	}

	s[2] = ':';

	byteToStr(iMinutes, s + 3);

	display.drawStr(xPos, yPos, s);

	return display.getStrPixelWidth(s);
}

/// <summary>
/// Draws the seconds digital.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawSecondsDigital(byte xPos, byte yPos)
{
	char s[3];

	byteToStr(iSecond, s);

	display.drawStr(xPos, yPos, s);
}

/// <summary>
/// Draws the date digital.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawDateDigital(byte xPos, byte yPos)
{
	char s[11];

	byteToStr(iDay, s);

	s[2] = '/';

	byteToStr(iMonth, s + 3);

	s[5] = '/';

	stoa(iYear, s + 6);

	display.drawStr(xPos, yPos, s);
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
