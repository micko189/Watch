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

PGM_P const weekString[] PROGMEM = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
PGM_P const ampmString[] PROGMEM = { "AM", "PM" };
PROGMEM const byte daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; // standard year days

PGM_P const menuItems[] PROGMEM = { "Set date", "Set time", "Set time format" };
PGM_P const timeFormat[] PROGMEM = { "12h", "24h" };

//PGM_P const dayNames[] PROGMEM = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
//PGM_P const months[] PROGMEM = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

#define firstYear 2000 //This is our start point
#define dayOffset 6 //The first day of our start year may not be a Sunday ( in 1800 it was Wednesday)

//----- Display features
#define DISPLAY_MODE_START_UP 0x0
#define DISPLAY_MODE_CLOCK 0x1
#define DISPLAY_MODE_MENU 0x2
#define DISPLAY_MODE_SET_MENU 0x3
byte displayMode = DISPLAY_MODE_START_UP;

#define MENU_SET_DATE  0x0
#define MENU_SET_TIME  0x1
#define MENU_SET_TIME_FORMAT 0x2
byte menuMode = MENU_SET_DATE;

#define CLOCK_STYLE_SIMPLE_ANALOG  0x1
#define CLOCK_STYLE_SIMPLE_DIGIT  0x2
#define CLOCK_STYLE_SIMPLE_MIX  0x3
#define CLOCK_STYLE_SIMPLE_DIGIT_SEC  0x4
byte clockStyle = CLOCK_STYLE_SIMPLE_DIGIT_SEC;

#define centerX 64
#define centerY 32
#define iRadius 30

//----- Button control
#define buttonPin 5
boolean isClicked = LOW; // LOW = false = 0x0
boolean isChanged = false;

#define buttonPinUp 6
boolean isClickedUp = LOW; // LOW = false = 0x0
boolean isChangedUp = false;

#define buttonPinDown 7
boolean isClickedDown = LOW; // LOW = false = 0x0
boolean isChangedDown = false;

//----- Global
byte startUpCount = 0;
byte tempLo = 0;
byte tempHi = 0;
byte setPosition = 0; // position of value currently being set

///////////////////////////////////
//----- Arduino setup and loop methods
///////////////////////////////////

void setup()   {
	//Serial.begin(9600);    // Do not enable serial. This makes serious problem because of shortage of RAM.
	pinMode(buttonPin, INPUT);  // Defines button pin
	pinMode(buttonPinUp, INPUT);  // Defines button pin
	pinMode(buttonPinDown, INPUT);  // Defines button pin

	// By default, we'll generate the high voltage from the 3.3v line internally! (neat!)
	// Assign default color value
	display.setColorIndex(1);         // pixel on BW

	iWeek = calcDayOfWeek(iYear, iMonth, iDay);

	// Start up the temperature sensor library
	TempSensor.begin();

	// Start up the light sensor library
	LightSensor.begin();
	LightSensor.SetAddress(Device_Address_H);//Address 0x5C
	LightSensor.SetMode(Continuous_H_resolution_Mode);
}

void loop() {
	isChanged = false;
	isChangedUp = false;
	isChangedDown = false;
	unsigned long current_time_milis = 0;

	// Get button input
	getButtonInput(buttonPin, &isClicked, &isChanged);
	getButtonInput(buttonPinUp, &isClickedUp, &isChangedUp);
	getButtonInput(buttonPinDown, &isClickedDown, &isChangedDown);

	// Update clock time
	current_time_milis = millis();

	if (updateTime(current_time_milis) || displayMode == DISPLAY_MODE_START_UP || isChanged || isChangedUp || isChangedDown)
	{
		// One second has elapsed or we have input (button clicked)

		TempSensor.requestTemperaturesByIndex(0); // Send the command to get temperatures
		float temp = TempSensor.getTempCByIndex(0);

		tempLo = ((short)(temp * 100)) % 100;
		tempHi = (byte)temp;

		uint16_t lux = LightSensor.GetLightIntensity();// Get Lux value

		//dim display (Arduino\libraries\U8glib\utility\u8g_dev_ssd1306_128x64.c u8g_dev_ssd1306_128x64_fn)
		//display.setContrast(0); 

		// picture loop
		display.firstPage();
		do {
			// Display routine
			onDraw();
		} while (display.nextPage());
	}

	// Delay to get next current time (10ms), this is essentially time deviation in one second cycle (~ +-10ms)
	delay(10);
}

///////////////////////////////////
//----- Utils
///////////////////////////////////

/// <summary>
/// Gets the button input.
/// </summary>
/// <param name="pin">The pin.</param>
/// <param name="clicked">The clicked.</param>
/// <param name="changed">The changed.</param>
void getButtonInput(byte pin, boolean *clicked, boolean *changed)
{
	if (digitalRead(pin) == HIGH)
	{
		if (*clicked == LOW)
		{
			*changed = true;
			*clicked = HIGH;
		}
	}
}

/// <summary>
/// Determines whether [is leap year] [the specified year].
/// </summary>
/// <param name="year">The year.</param>
/// <returns></returns>
bool isLeapYear(short year)
{
	return ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0));
}

/// <summary>
/// Gets the days in month.
/// </summary>
/// <param name="year">The year.</param>
/// <param name="month">The month.</param>
/// <returns></returns>
byte getDaysInMonth(short year, byte month)
{
	month--; // Adjust for indexing in daysInMonth
	if (month != 1)
	{
		return daysInMonth[month];
	}
	else // feb
	{
		return (isLeapYear(year)) ? 29 : daysInMonth[month];
	}
}

/// <summary>
/// Gets the days passed in year.
/// </summary>
/// <param name="year">The year.</param>
/// <param name="month">The month.</param>
/// <param name="day">The day.</param>
/// <returns></returns>
short daysPassedInYear(short year, byte month, byte day)
{
	int passed = 0;
	day--; // Adjust days passed in current month
	for (size_t i = 1; i <= month; i++)
	{
		passed += getDaysInMonth(year, i);
	}

	return passed + day;
}

/// <summary>
/// Calculates the day of week index.
/// </summary>
/// <param name="year">The year.</param>
/// <param name="month">The month.</param>
/// <param name="day">The day.</param>
/// <returns>The day of week index.</returns>
byte calcDayOfWeek(short year, byte month, byte day)
{
	int days = dayOffset;

	// Calculates basic number of days passed 
	days += (year - firstYear) * 365;
	days += daysPassedInYear(year, month, day);

	// Add on the extra leapdays for past years
	for (int i = firstYear; i < year; i += 4)
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
/// <param name="current_time_milis">The current_time_milis.</param>
/// <returns> whether time is updated - one seccond is ellapsed </returns>
bool updateTime(unsigned long current_time_milis) {
	short timeElapse = current_time_milis - prevClockTime;
	if (timeElapse >= adjustedUpdateTimeInterval) // check if one second has elapsed
	{
		// Adjust next update time interval in order to reduce accumulated error
		adjustedUpdateTimeInterval = UPDATE_TIME_INTERVAL - (timeElapse - adjustedUpdateTimeInterval);

		if (++iSecond >= 60)
		{
			iSecond = 0;
			// Increase time by incrementing minutes
			if (++iMinutes >= 60)
			{
				iMinutes = 0;
				if (++iHour > 12)
				{
					iHour = 1;
					(iAmPm == 0) ? iAmPm = 1 : iAmPm = 0;
					if (iAmPm == 0)
					{
						if (++iWeek > 6)
						{
							iWeek = 0;
						}

						iDay++;
						if (iDay > getDaysInMonth(iYear, iMonth))
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
	if (isClickedUp == HIGH)
	{
		(*option)++;
		if (*option > maxVal)
		{
			*option = minVal;
		}
	}

	if (isClickedDown == HIGH)
	{
		(*option)--;
		if (*option < minVal)
		{
			*option = maxVal;
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
void onDraw() {

	display.setFont(u8g_font_helvB10r);

	switch (displayMode)
	{
	case DISPLAY_MODE_START_UP:
		drawStartUp();
		break;

	case DISPLAY_MODE_CLOCK:
		if (isClicked == HIGH)
		{
			displayMode = DISPLAY_MODE_MENU;
			break;
		}

		if (isClickedUp == HIGH || isClickedDown == HIGH)
		{
			// User input received
			toggleOption(&clockStyle, 0, 4);
		}

		drawClock();

		break;

	case DISPLAY_MODE_MENU:
		if (isClicked == HIGH)
		{
			displayMode = DISPLAY_MODE_SET_MENU;
			break;
		}

		if (isClickedUp == HIGH && isClickedDown == HIGH)
		{
			// Go back
			displayMode = DISPLAY_MODE_CLOCK;
			menuMode = 0;
			break;
		}

		if (isClickedUp == HIGH || isClickedDown == HIGH)
		{
			// User input received
			toggleOption(&menuMode, 0, 3);
		}

		drawMenu();
		break;

	case DISPLAY_MODE_SET_MENU:
		if (isClickedUp == HIGH && isClickedDown == HIGH)
		{
			// Go back
			displayMode = DISPLAY_MODE_MENU;
			setPosition = 0;
			break;
		}

		if (isClicked == HIGH)
		{
			// Go to next value
			setPosition++;
		}

		drawSetMenu();

		break;
	default:
		displayMode = DISPLAY_MODE_CLOCK;    // This means there's an error
		break;
	}

	isClicked = LOW;
	isClickedUp = LOW;
	isClickedDown = LOW;
}  // End of onDraw()

/// <summary>
/// Rolls the value over to 0.
/// </summary>
/// <param name="value">The value.</param>
/// <param name="rollOverVal">The roll over value.</param>
void rollOver(byte *value, byte rollOverVal)
{
	if (*value > rollOverVal)
	{
		*value = 0;
	}
}

/// <summary>
/// Draws the set menu.
/// </summary>
void drawSetMenu()
{
	switch (menuMode)
	{
	case MENU_SET_DATE:
		rollOver(&setPosition, 2); // DD MM YY

		if (isClickedUp == HIGH || isClickedDown == HIGH)
		{
			switch (setPosition)
			{
			case 0:
				toggleOption(&iDay, 1, 12);
				break;
			case 1:
				toggleOption(&iMonth, 1, 60);
				break;
			case 2:
				if (isClickedUp == HIGH)
				{
					iYear++;
					if (iYear > 32767)
						iYear = 2000;
				}

				if (isClickedDown == HIGH)
				{
					iYear--;
					if (iYear < 2000)
						iYear = 32767;
				}

				break;
			}

			// Calculate week index
			iWeek = calcDayOfWeek(iYear, iMonth, iDay);
		}

		drawDateDigital(29, 12);
		display.drawLine(29, 12, 39, 12);

		break;
	case MENU_SET_TIME:
		rollOver(&setPosition, 1); // HH MM

		if (isClickedUp == HIGH || isClickedDown == HIGH)
		{
			switch (setPosition)
			{
			case 0:
				toggleOption(&iHour, 1, 12);
				break;
			case 1:
				toggleOption(&iMinutes, 1, 60);
				break;
			}
		}

		display.drawLine(29, 12, 39, 12);
		drawClockDigital(14, 45);

		break;
	case MENU_SET_TIME_FORMAT:
		rollOver(&setPosition, 0); // TF

		if (isClickedUp == HIGH || isClickedDown == HIGH)
		{
			toggleOption(&iTimeFormat, 0, 1);
		}

		display.drawLine(29, 12, 39, 12);
		drawTimeFormat(14, 45);

		break;
	}
}

/// <summary>
/// Draws the start up splash screen.
/// </summary>
void drawStartUp() {
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

	if (startUpCount++ > 200) // 2s start up screen
	{
		startUpCount = 0;
		displayMode = DISPLAY_MODE_CLOCK;
	}
}

/// <summary>
/// Draw the main manu screen.
/// Menu changes according to user selection.
/// </summary>
void drawMenu() {
	display.setFont(u8g_font_helvB10r);
	display.drawStr(10, 10, (const char*)pgm_read_word(&(menuItems[MENU_SET_DATE])));
	display.drawStr(10, 30, (const char*)pgm_read_word(&(menuItems[MENU_SET_TIME])));
	display.drawStr(10, 50, (const char*)pgm_read_word(&(menuItems[MENU_SET_TIME_FORMAT])));

	display.setFontRefHeightExtendedText();
	display.setFontPosTop();

	byte h = display.getFontAscent() - display.getFontDescent();
	byte w = display.getStrWidth((const char*)pgm_read_word(&(menuItems[menuMode])));;

	display.drawFrame(10 - 1, 10 + menuMode * 20, w + 2, h + 2);
}

/// <summary>
/// Draw the main clock screen.
/// Clock style changes according to user selection.
/// </summary>
void drawClock() {
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
		drawClockAnalog(centerX, centerY - 30, iRadius - 4);

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

		drawClockAnalog(centerX, centerY, iRadius);

		break;

	case CLOCK_STYLE_SIMPLE_DIGIT_SEC:
		display.setFont(u8g_font_helvB10r);
		drawDateDigital(29, 12);

		display.setFont(u8g_font_helvB12);
		drawTemp(80, 63);

		display.setFont(u8g_font_helvB24n);
		byte offset = drawClockDigital(14, 45);

		display.setFont(u8g_font_helvB14r);
		drawSecondsDigital(14 + offset + 2, 45);

		break;
	}
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
		itoa(value, s + 1, 10);
	}
	else
	{
		itoa(value, s, 10);
	}
}

/// <summary>
/// Draws the time format (12/24).
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawTimeFormat(byte xPos, byte yPos) {
	display.drawStr(xPos, yPos, (const char*)pgm_read_word(&(timeFormat[iTimeFormat])));
}

/// <summary>
/// Draws the day in week and am/pm.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawDayAmPm(byte xPos, byte yPos) {
	display.drawStr(xPos, yPos, (const char*)pgm_read_word(&(weekString[iWeek])));
	display.drawStr(xPos + display.getStrPixelWidth((const char*)pgm_read_word(&(weekString[iWeek]))) + 2, yPos, (const char*)pgm_read_word(&(ampmString[iAmPm])));
}

/// <summary>
/// Draws the temperature (x,y is the possition of '.').
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
void drawTemp(byte xPos, byte yPos) {
	char s[4] = { 0 };
	byte offset = 0;

	itoa(tempHi, s, 10);
	display.drawStr(xPos - display.getStrPixelWidth(s) + 1, yPos, s);

	s[0] = '.', s[1] = 0;
	display.drawStr(xPos, yPos, s);
	offset += display.getStrPixelWidth(s) + 1;

	byteToStr(tempLo, s);

	display.drawStr(xPos + offset, yPos, s);
	offset += display.getStrPixelWidth(s) + 1;

	s[0] = '°', s[1] = 'C', s[2] = 0;
	display.drawStr(xPos + offset, yPos, s);
}

/// <summary>
/// Draws the clock digital.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
/// <returns>Length in pixels of the clock</returns>
byte drawClockDigital(byte xPos, byte yPos) {
	char s[6] = { 0 };

	byteToStr(iHour, s);

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
void drawSecondsDigital(byte xPos, byte yPos) {
	char s[3] = { 0 };

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
	char s[11] = { 0 };

	byteToStr(iDay, s);

	s[2] = '/';

	byteToStr(iMonth, s + 3);

	s[5] = '/';

	itoa(iYear, s + 6, 10);

	display.drawStr(xPos, yPos, s);
}

/// <summary>
/// Draws the clock analog.
/// </summary>
/// <param name="xPos">The x position.</param>
/// <param name="yPos">The y position.</param>
/// <param name="radius">The radius.</param>
void drawClockAnalog(byte xPos, byte yPos, byte radius) {
	display.drawCircle(xPos, yPos, radius);

	// print hour pin lines
	for (size_t i = 0; i < 60; i++)
	{
		showTimePin(xPos, yPos, 0.9, 1, i * 5, radius, 1);
	}

	double hourAngleOffset = iMinutes / 12.0;
	showTimePin(xPos, yPos, 0.1, 0.5, iHour * 5 + hourAngleOffset, radius, -1);
	showTimePin(xPos, yPos, 0.1, 0.78, iMinutes, radius, 1);
	// showTimePin(xPos, yPos, 0.1, 0.9, iSecond, radius);
}

// Calculate clock pin position
#define RAD 0.01745329251994329576923690768489 // = Pi / 180;
#define LR 89.99

int getPointCoordinate(byte center, byte radius, double pl, double angle, byte sign, double(*trigFn)(double))
{
	return center + (radius * pl) * (*trigFn)((6 * angle + LR * sign) * RAD);
}

void showTimePin(byte center_x, byte center_y, double pl1, double pl2, double angle, byte radius, byte sign) {
	int x1, x2, y1, y2;

	//x1 = center_x + (radius * pl1) * cos((6 * angle + LR) * RAD);
	x1 = getPointCoordinate(center_x, radius, pl1, angle, 1, &cos);

	//y1 = center_y + (radius * pl1) * sin((6 * angle + LR) * RAD);
	y1 = getPointCoordinate(center_y, radius, pl1, angle, 1, &sin);

	//x2 = center_x + (radius * pl2) * cos((6 * angle + LR * sign) * RAD);
	x2 = getPointCoordinate(center_x, radius, pl2, angle, sign, &cos);

	//y2 = center_y + (radius * pl2) * sin((6 * angle + LR * sign) * RAD);
	y2 = getPointCoordinate(center_y, radius, pl2, angle, sign, &sin);

	display.drawLine(x1, y1, x2, y2);
}
