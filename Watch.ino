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
unsigned long prevClockTime = 0;

PGM_P const weekString[] PROGMEM = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
PGM_P const ampmString[] PROGMEM = { "AM", "PM" };
PROGMEM const byte daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; //standard year

//PGM_P const dayNames[] PROGMEM = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
//PGM_P const months[] PROGMEM = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

PROGMEM const short firstYear = 2000; //This is our start point
PROGMEM const byte dayOffset = 6; //The first day of our start year may not be a Sunday ( in 1800 it was Wednesday)

//----- Display features
#define DISPLAY_MODE_START_UP 0
#define DISPLAY_MODE_CLOCK 1
byte displayMode = DISPLAY_MODE_START_UP;

#define CLOCK_STYLE_SIMPLE_ANALOG  0x01
#define CLOCK_STYLE_SIMPLE_DIGIT  0x02
#define CLOCK_STYLE_SIMPLE_MIX  0x03
#define CLOCK_STYLE_SIMPLE_DIGIT_SEC  0x04
byte clockStyle = CLOCK_STYLE_SIMPLE_DIGIT_SEC;

byte centerX = 64;
byte centerY = 32;
byte iRadius = 30;

//----- Button control
int buttonPin = 5;
boolean isClicked = false;

//----- Global
byte startUpCount = 0;
byte tempLo = 0;
byte tempHi = 0;

///////////////////////////////////
//----- Arduino setup and loop methods
///////////////////////////////////

void setup()   {
	Serial.begin(9600);    // Do not enable serial. This makes serious problem because of shortage of RAM.
	pinMode(buttonPin, INPUT);  // Defines button pin

	// By default, we'll generate the high voltage from the 3.3v line internally! (neat!)
	// Assign default color value
	display.setColorIndex(1);         // pixel on BW

	iWeek = (byte)(calcDaysSoFar(iYear, iMonth, iDay) % 7);

	// Start up the temperature sensor library
	TempSensor.begin();

	// Start up the light sensor library
	LightSensor.begin();
	LightSensor.SetAddress(Device_Address_H);//Address 0x5C
	LightSensor.SetMode(Continuous_H_resolution_Mode);
}

void loop() {
	//boolean isReceived = false;
	unsigned long current_time_milis = 0;

	// Get button input
	if (digitalRead(buttonPin) == LOW)
	{
		//isClicked = LOW;
	}

	// Update clock time
	current_time_milis = millis();

	if (updateTime(current_time_milis) || displayMode == DISPLAY_MODE_START_UP)
	{ 
		// One second has elapsed

		TempSensor.requestTemperaturesByIndex(0); // Send the command to get temperatures
		float temp = TempSensor.getTempCByIndex(0);

		tempLo = ((short)(temp * 100)) % 100;
		tempHi = (short)temp;

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
	byte days = 0;
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
/// Calculates the number of days passed from some start point year.
/// </summary>
/// <param name="year">The year.</param>
/// <param name="month">The month.</param>
/// <param name="day">The day.</param>
/// <returns></returns>
int calcDaysSoFar(short year, byte month, byte day)
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

	return days;
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
/// Toggles the clock style.
/// </summary>
void toggleClockStyle()
{
	clockStyle++;
	if (clockStyle > 4)
	{
		clockStyle = 1;
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
		if (isClicked == LOW) {    // User input received

			toggleClockStyle();
		}

		drawClock();

		break;
	default:
		startClockMode();    // This means there's an error
		break;
	}

	isClicked = HIGH;
}  // End of onDraw()


/// <summary>
/// Starts the clock mode (changes displayMode).
/// </summary>
void startClockMode() {
	displayMode = DISPLAY_MODE_CLOCK;
}

////////////////////////////////////////////////////
// 16x24 Logo
////////////////////////////////////////////////////
const unsigned char PROGMEM IMG_logo_24x24[] = {
0x07,0xff,0xe0,
0x07,0xff,0xe0,
0x07,0xff,0xe0,
0x08,0x10,0x10,
0x10,0x10,0x08,
0x10,0x00,0x08,
0x10,0x00,0x08,
0x10,0x00,0x08,
0x10,0x00,0x08,
0x10,0x02,0x08,
0x10,0x04,0x08,
0x10,0x08,0x0c,
0x1c,0x10,0x3c,
0x10,0x08,0x0c,
0x10,0x08,0x08,
0x10,0x04,0x08,
0x10,0x04,0x08,
0x10,0x02,0x08,
0x10,0x02,0x08,
0x10,0x10,0x08,
0x08,0x10,0x10,
0x07,0xff,0xe0,
0x07,0xff,0xe0,
0x07,0xff,0xe0
}; 

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
	display.drawBitmapP(10, 10, 3, 24, IMG_logo_24x24);

	//display.drawStr(25, 12, "Temperature");

	display.drawStr(35, 28, "Watch");

	display.drawStr(25, 45, "Arduino v1.0");

	if (startUpCount++ > 200) // 2s start up screen
	{
		startUpCount = 0;
		startClockMode();
	}
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
		drawTemp(80, centerY + 31);

		display.setFont(u8g_font_helvB14r);
		drawDayAmPm(centerX - 34, centerY - 17);

		display.setFont(u8g_font_helvB18r);
		drawClockDigital(centerX - 29, centerY + 6);

		break;

	case CLOCK_STYLE_SIMPLE_MIX:
		drawClockAnalog(0, -30, iRadius - 4);

		display.setFont(u8g_font_helvB12);
		drawTemp(80, centerY + 31);

		display.setFont(u8g_font_helvB12);
		drawDayAmPm(centerY * 2 + 3, 23);

		display.setFont(u8g_font_helvB18r);
		drawClockDigital(centerY * 2 - 3, 45);
		break;

	case CLOCK_STYLE_SIMPLE_ANALOG:
		display.setFont(u8g_font_helvB12);
		drawTemp(80, centerY + 31);

		drawClockAnalog(0, 0, iRadius);

		break;

	case CLOCK_STYLE_SIMPLE_DIGIT_SEC:
		display.setFont(u8g_font_helvB10r);
		drawDateDigital(centerX - 35, centerY - 20);

		display.setFont(u8g_font_helvB12);
		drawTemp(80, centerY + 31);

		display.setFont(u8g_font_helvB24n);
		byte offset = drawClockDigital(centerX - 50, centerY + 13);

		display.setFont(u8g_font_helvB14r);
		drawSecondsDigital(centerX - 50 + offset + 2, centerY + 13);

		break;
	}
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

	if (tempLo < 10)
	{
		s[0] = '0';
		itoa(tempLo, s + 1, 10);
	}
	else
	{
		itoa(tempLo, s, 10);
	}
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

	if (iHour < 10)
	{
		s[0] = '0';
		itoa(iHour, s + 1, 10);
	}
	else
	{
		itoa(iHour, s, 10);
	}

	s[2] = ':';

	if (iMinutes < 10)
	{
		s[3] = '0';
		itoa(iMinutes, s + 4, 10);
	}
	else
	{
		itoa(iMinutes, s + 3, 10);
	}

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

	if (iSecond < 10)
	{
		s[0] = '0';
		itoa(iSecond, s + 1, 10);
	}
	else
	{
		itoa(iSecond, s, 10);
	}

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

	if (iDay < 10)
	{
		s[0] = '0';
		itoa(iDay, s + 1, 10);
	}
	else
	{
		itoa(iDay, s, 10);
	}

	s[2] = '/';

	if (iMonth < 10)
	{
		s[3] = '0';
		itoa(iMonth, s + 4, 10);
	}
	else
	{
		itoa(iMonth, s + 3, 10);
	}

	s[5] = '/';

	itoa(iYear, s + 6, 10);

	display.drawStr(xPos, yPos, s);
}

/// <summary>
/// Draws the clock analog.
/// </summary>
/// <param name="offsetY">The offset y.</param>
/// <param name="offsetX">The offset x.</param>
/// <param name="radius">The radius.</param>
void drawClockAnalog(short offsetY, short offsetX, byte radius) {
	display.drawCircle(centerX + offsetX, centerY + offsetY, radius);

	// print hour pin lines
	for (size_t i = 0; i < 12; i++)
	{
		showHourPin(centerX + offsetX, centerY + offsetY, 0.9, 1, i * 5, radius);
	}

	double hourAngleOffset = iMinutes / 12.0;
	showTimePin(centerX + offsetX, centerY + offsetY, 0.1, 0.5, iHour * 5 + hourAngleOffset, radius);
	showTimePin(centerX + offsetX, centerY + offsetY, 0.1, 0.78, iMinutes, radius);
	// showTimePin(centerX + offsetX, centerY + offsetY, 0.1, 0.9, iSecond, radius);
}

// Calculate clock pin position
double RAD = 0.01745329251994329576923690768489; // = Pi / 180;
double LR = 89.99;

void showTimePin(int center_x, int center_y, double pl1, double pl2, double pl3, byte radius) {
	double x1, x2, y1, y2;
	x1 = center_x + (radius * pl1) * cos((6 * pl3 + LR) * RAD);
	y1 = center_y + (radius * pl1) * sin((6 * pl3 + LR) * RAD);
	x2 = center_x + (radius * pl2) * cos((6 * pl3 - LR) * RAD);
	y2 = center_y + (radius * pl2) * sin((6 * pl3 - LR) * RAD);

	display.drawLine((int)x1, (int)y1, (int)x2, (int)y2);
}

void showHourPin(int center_x, int center_y, double pl1, double pl2, double pl3, byte radius) {
	double x1, x2, y1, y2;
	x1 = center_x + (radius * pl1) * cos((6 * pl3 + LR) * RAD);
	y1 = center_y + (radius * pl1) * sin((6 * pl3 + LR) * RAD);
	x2 = center_x + (radius * pl2) * cos((6 * pl3 + LR) * RAD);
	y2 = center_y + (radius * pl2) * sin((6 * pl3 + LR) * RAD);

	display.drawLine((int)x1, (int)y1, (int)x2, (int)y2);
}



