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
#include <SoftwareSerial.h>
#include <U8glib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BH1750FVI.h>
#include <math.h>
//#include "bitmap.h"

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
DallasTemperature sensors(&oneWire);

///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
//----- LightSensor instance
BH1750FVI LightSensor;

///////////////////////////////////////////////////////////////////

//----- Message item buffer
#define MSG_COUNT_MAX 7
#define MSG_BUFFER_MAX 19
unsigned char msgBuffer[MSG_COUNT_MAX][MSG_BUFFER_MAX];
char msgParsingLine = 0;
char msgParsingChar = 0;
char msgCurDisp = 0;

//----- Emergency item buffer
#define EMG_COUNT_MAX 3
#define EMG_BUFFER_MAX 19
char emgBuffer[EMG_COUNT_MAX][EMG_BUFFER_MAX];
char emgParsingLine = 0;
char emgParsingChar = 0;
char emgCurDisp = 0;

//----- Time
#define UPDATE_TIME_INTERVAL 60000
#define UPDATE_TIME_INTERVAL_SEC 1000
short iYear = 2014;
byte iMonth = 9;
byte iDay = 20;
byte iWeek = 0;    // 1: SUN, MON, TUE, WED, THU, FRI,SAT // need to calculate this
byte iAmPm = 1;    // 0:AM, 1:PM
byte iHour = 7;
byte iMinutes = 22;
byte iSecond = 0;

#define TIME_BUFFER_MAX 6
char timeParsingIndex = 0;
PGM_P const weekString[] PROGMEM = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
PGM_P const ampmString[] PROGMEM = { "AM", "PM" };
PROGMEM const byte daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; //standard year

PGM_P const dayNames[] PROGMEM = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
PGM_P const months[] PROGMEM = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

PROGMEM const short firstYear = 2000; //This is our start point
PROGMEM const byte dayOffset = 6; //The first day of our start year may not be a Sunday ( in 1800 it was Wednesday)

//----- Display features
#define DISPLAY_MODE_START_UP 0
#define DISPLAY_MODE_CLOCK 1
#define DISPLAY_MODE_EMERGENCY_MSG 2
#define DISPLAY_MODE_NORMAL_MSG 3
#define DISPLAY_MODE_IDLE 11
byte displayMode = DISPLAY_MODE_START_UP;

#define CLOCK_STYLE_SIMPLE_ANALOG  0x01
#define CLOCK_STYLE_SIMPLE_DIGIT  0x02
#define CLOCK_STYLE_SIMPLE_MIX  0x03
#define CLOCK_STYLE_SIMPLE_DIGIT_SEC  0x04
byte clockStyle = CLOCK_STYLE_SIMPLE_DIGIT_SEC;

#define INDICATOR_ENABLE 0x01
boolean updateIndicator = true;

byte centerX = 64;
byte centerY = 32;
byte iRadius = 28;

#define IDLE_DISP_INTERVAL 60000
#define CLOCK_DISP_INTERVAL 60000
#define EMERGENCY_DISP_INTERVAL 5000
#define MESSAGE_DISP_INTERVAL 3000
unsigned long prevClockTime = 0;
unsigned long prevDisplayTime = 0;

//unsigned long next_display_interval = 0;
unsigned long mode_change_timer = 0;
#define CLOCK_DISPLAY_TIME 300000
#define EMER_DISPLAY_TIME 10000
#define MSG_DISPLAY_TIME 5000

//----- Button control
int buttonPin = 5;
boolean isClicked = false;
byte startUp = 0;

void setup()   {
	Serial.begin(9600);    // Do not enable serial. This makes serious problem because of shortage of RAM.
	pinMode(buttonPin, INPUT);  // Defines button pin

	init_emg_array();
	init_msg_array();

	//----- by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
	// assign default color value
	if (display.getMode() == U8G_MODE_R3G3B2) {
		display.setColorIndex(255);     // white
	}
	else if (display.getMode() == U8G_MODE_GRAY2BIT) {
		display.setColorIndex(3);         // max intensity
	}
	else if (display.getMode() == U8G_MODE_BW) {
		display.setColorIndex(1);         // pixel on
	}
	else if (display.getMode() == U8G_MODE_HICOLOR) {
		display.setHiColorByRGB(255, 255, 255);
	}

	//drawStartUp();    // Show RetroWatch Logo
	centerX = 128 / 2;
	centerY = 64 / 2;
	iRadius = centerY - 2;

	iWeek = calcDayOfWeekIndex();

	// Start up the temperature sensor library
	sensors.begin();

	LightSensor.begin();
	/*
	Set the address for this sensor you can use 2 different address: Device_Address_H "0x5C", Device_Address_L "0x23"
	you must connect Addr pin to A3 .
	*/
	LightSensor.SetAddress(Device_Address_H);//Address 0x5C
	// To adjust the slave on other address , uncomment this line
	// lightMeter.SetAddress(Device_Address_L); //Address 0x5C
	//-----------------------------------------------
	/*
	set the Working Mode for this sensor
	Select the following Mode: Continuous_H_resolution_Mode, Continuous_H_resolution_Mode2 ,Continuous_L_resolution_Mode ,OneTime_H_resolution_Mode, OneTime_H_resolution_Mode2, OneTime_L_resolution_Mode
	The data sheet recommanded To use Continuous_H_resolution_Mode
	*/

	LightSensor.SetMode(Continuous_H_resolution_Mode);
}


void loop() {
	boolean isReceived = false;
	unsigned long current_time_milis = 0;

	// Get button input
	if (digitalRead(buttonPin) == LOW)
	{
		//isClicked = LOW;
	}

	// Update clock time
	current_time_milis = millis();
	updateTime(current_time_milis);

	sensors.requestTemperatures(); // Send the command to get temperatures

	Serial.println(sensors.getTempCByIndex(0));

	uint16_t lux = LightSensor.GetLightIntensity();// Get Lux value
	Serial.print("Light: ");
	Serial.print(lux);
	Serial.println(" lux");

	//dim display (Arduino\libraries\U8glib\utility\u8g_dev_ssd1306_128x64.c u8g_dev_ssd1306_128x64_fn)
	//display.setContrast(0); 

	// picture loop
	display.firstPage();
	do {
		// Display routine
		onDraw(current_time_milis);
	} while (display.nextPage());

	// rebuild the picture after some delay (100ms)
	delay(100);
}


///////////////////////////////////
//----- Utils
///////////////////////////////////
void init_msg_array() {
	for (int i = 0; i < MSG_COUNT_MAX; i++)
	{
		for (int j = 0; j < MSG_BUFFER_MAX; j++)
		{
			msgBuffer[i][j] = 0x00;
		}
	}

	msgParsingLine = 0;
	msgParsingChar = 0;    // First 2 byte is management byte
	msgCurDisp = 0;
}

void init_emg_array() {
	for (int i = 0; i < EMG_COUNT_MAX; i++)
	{
		for (int j = 0; j < EMG_BUFFER_MAX; j++)
		{
			emgBuffer[i][j] = 0x00;
		}
	}

	emgParsingLine = 0;
	emgParsingChar = 0;    // First 2 byte is management byte
	emgCurDisp = 0;
}


//This function checks whether a particular year is a leap year
bool isLeapYear(short year)
{
	return ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0));
}

byte getDaysInMonth(short year, byte month)
{
	byte days = 0;
	month--; // adjust for indexing in daysInMonth
	if (month != 1)
	{
		return daysInMonth[month];
	}
	else // feb
	{
		return (isLeapYear(year)) ? 29 : daysInMonth[month];
	}
}

short daysPassedInYear(short year, byte month, byte day)
{
	int passed = 0;
	day--; // adjust days passed in current month
	for (size_t i = 1; i <= month; i++)
	{
		passed += getDaysInMonth(year, i);
	}

	return passed + day;
}

//This function calculates the number of days passed from some start point year
int calcDaysSoFar(short year, byte month, byte day)
{
	int days = dayOffset;

	//calculates basic number of days passed 
	days += (year - firstYear) * 365;
	days += daysPassedInYear(year, month, day);

	//add on the extra leapdays for past years
	for (int i = firstYear; i < year; i += 4)
	{
		if (isLeapYear(i))
		{
			days++;
		}
	}

	return days;
}

byte calcDayOfWeekIndex()
{
	return (byte)(calcDaysSoFar(iYear, iMonth, iDay) % 7);
}

void updateTime(unsigned long current_time_milis) {

	if (current_time_milis - prevClockTime > UPDATE_TIME_INTERVAL_SEC) // check if one second has elapsed
	{
		iSecond++;
		if (iSecond >= 60)
		{
			iSecond = 0;
// Increase time by incrementing minutes
		iMinutes++;
		if (iMinutes >= 60)
		{
			iMinutes = 0;
			iHour++;
			if (iHour > 12)
			{
				iHour = 1;
				(iAmPm == 0) ? iAmPm = 1 : iAmPm = 0;
				if (iAmPm == 0)
				{
					iWeek++;
					if (iWeek > 6)
					{
						iWeek = 0;
					}

					iDay++;
					if (iDay > getDaysInMonth(iYear, iMonth))
					{
						iDay = 1;
						iMonth++;
						if (iMonth > 12)
						{
							iYear++;
						}
					}
				}
			}
		}
		}

		prevClockTime = current_time_milis;
	}
}

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


// Main drawing routine.
// Every drawing starts here.
void onDraw(unsigned long currentTime) {

	display.setFont(u8g_font_helvB10r);

	switch (displayMode)
	{
	case DISPLAY_MODE_START_UP:
		drawStartUp();
		break;

	case DISPLAY_MODE_CLOCK:
		if (isClicked == LOW) {    // User input received
			//startEmergencyMode();
			//setPageChangeTime(0);    // Change mode with no page-delay
			//setNextDisplayTime(currentTime, 0);    // Do not wait next re-draw time

			//toggleClockStyle();
		}

		drawClock();
		//if (isPageChangeTime(currentTime)) {  // It's time to go into idle mode
		//startIdleMode();
		//setPageChangeTime(currentTime);  // Set a short delay
		//}
		//setNextDisplayTime(currentTime, CLOCK_DISP_INTERVAL);

		break;

	case DISPLAY_MODE_EMERGENCY_MSG:
		if (findNextEmerMessage())
		{
			//drawEmergency();
			emgCurDisp++;
			if (emgCurDisp >= EMG_COUNT_MAX)
			{
				emgCurDisp = 0;
				startMessageMode();
			}
			//setNextDisplayTime(currentTime, EMERGENCY_DISP_INTERVAL);
		}
		// There's no message left to display. Go to normal message mode.
		else
		{
			startMessageMode();
			//setPageChangeTime(0);
			//setNextDisplayTime(currentTime, 0);  // with no re-draw interval
		}
		break;

	case DISPLAY_MODE_NORMAL_MSG:
		if (findNextNormalMessage())
		{
			//drawMessage();
			msgCurDisp++;
			if (msgCurDisp >= MSG_COUNT_MAX)
			{
				msgCurDisp = 0;
				startClockMode();
			}
			//setNextDisplayTime(currentTime, MESSAGE_DISP_INTERVAL);
		}
		// There's no message left to display. Go to clock mode.
		else
		{
			startClockMode();
			//setPageChangeTime(currentTime);
			//setNextDisplayTime(currentTime, 0);  // with no re-draw interval
		}
		break;

	case DISPLAY_MODE_IDLE:
		if (isClicked == LOW) {    // Wake up watch if there's an user input
			startClockMode();
			//setPageChangeTime(currentTime);
			//setNextDisplayTime(currentTime, 0);
		}
		else
		{
			drawIdleClock();
			//setNextDisplayTime(currentTime, IDLE_DISP_INTERVAL);
		}
		break;

	default:
		startClockMode();    // This means there's an error
		break;
	}

	isClicked = HIGH;

	//Serial.println("onDrawE");
}  // End of onDraw()


// To avoid re-draw on every drawing time
// wait for time interval according to current mode 
// But user input(button) breaks this sleep
//boolean isDisplayTime(unsigned long currentTime) {
//	if (currentTime - prevDisplayTime > next_display_interval)
//	{
//		return true;
//	}
//
//	if (isClicked == LOW)
//	{
//		return true;
//	}
//
//	return false;
//}

// Set next re-draw time 
//void setNextDisplayTime(unsigned long currentTime, unsigned long nextUpdateTime) {
//	next_display_interval = nextUpdateTime;
//	prevDisplayTime = currentTime;
//}

// Decide if it's the time to change page(mode)
//boolean isPageChangeTime(unsigned long currentTime) {
//	if (displayMode == DISPLAY_MODE_CLOCK) 
//	{
//		if (currentTime - mode_change_timer > CLOCK_DISPLAY_TIME)
//			return true;
//	}
//	return false;
//}

// Set time interval to next page(mode)
//void setPageChangeTime(unsigned long currentTime) {
//	mode_change_timer = currentTime;
//}

// Check if available emergency message exists or not
boolean findNextEmerMessage() {
	if (emgCurDisp < 0 || emgCurDisp >= EMG_COUNT_MAX) emgCurDisp = 0;
	while (true)
	{
		if (emgBuffer[emgCurDisp][0] == 0x00) {  // 0x00 means disabled
			emgCurDisp++;
			if (emgCurDisp >= EMG_COUNT_MAX)
			{
				emgCurDisp = 0;
				return false;
			}
		}
		else
		{
			break;
		}
	}  // End of while()

	return true;
}

// Check if available normal message exists or not
boolean findNextNormalMessage() {
	if (msgCurDisp < 0 || msgCurDisp >= MSG_COUNT_MAX) msgCurDisp = 0;
	while (true)
	{
		if (msgBuffer[msgCurDisp][0] == 0x00)
		{
			msgCurDisp++;
			if (msgCurDisp >= MSG_COUNT_MAX)
			{
				msgCurDisp = 0;
				return false;
			}
		}
		else
		{
			break;
		}
	}  // End of while()

	return true;
}

// Count all available emergency messages
int countEmergency() {
	int count = 0;
	for (int i = 0; i < EMG_COUNT_MAX; i++)
	{
		if (emgBuffer[i][0] != 0x00)
			count++;
	}
	return count;
}

// Count all available normal messages
int countMessage() {
	int count = 0;
	for (int i = 0; i < MSG_COUNT_MAX; i++)
	{
		if (msgBuffer[i][0] != 0x00)
			count++;
	}
	return count;
}

void startClockMode() {
	displayMode = DISPLAY_MODE_CLOCK;
}

void startEmergencyMode() {
	displayMode = DISPLAY_MODE_EMERGENCY_MSG;
	emgCurDisp = 0;
}

void startMessageMode() {
	displayMode = DISPLAY_MODE_NORMAL_MSG;
	msgCurDisp = 0;
}

void startIdleMode() {
	displayMode = DISPLAY_MODE_IDLE;
}
/*
// Draw indicator. Indicator shows count of emergency and normal message
void drawIndicator() {
	if (updateIndicator) {
		char s[3] = " ";

		int msgCount = countMessage();
		int emgCount = countEmergency();
		int drawCount = 1;

		if (msgCount > 0)
		{
			display.drawBitmap(127 - 8, 1, 1, 8, IMG_indicator_msg);
			display.drawStr(127 - 15, 1, itoa(msgCount, s, 10));
			drawCount++;
		}

		if (emgCount > 0)
		{
			display.drawBitmap(127 - 8 * drawCount - 7 * (drawCount - 1), 1, 1, 8, IMG_indicator_emg);

			display.drawStr(127 - 8 * drawCount - 7 * drawCount, 1, itoa(emgCount, s, 10));
		}

	}
}
*/
// RetroWatch splash screen
void drawStartUp() {
	//Arguments:
	// u8g : Pointer to the u8g structure(C interface only).
	// x : X - position(left position of the bitmap).
	// y : Y - position(upper position of the bitmap).
	// cnt : Number of bytes of the bitmap in horizontal direction.The width of the bitmap is cnt*8.
	// h : Height of the bitmap. 
	//display.drawBitmap(10, 15, 3, 24, IMG_logo_24x24);

	display.drawStr(45, 12, "Retro");

	display.drawStr(35, 28, "Watch");

	display.drawStr(25, 45, "Arduino v1.0");

	if (startUp++ > 20) // 2s start up screen
	{
		startUp = 0;
		//Serial.println("startClockMode");
		startClockMode();
	}
}
/*
// Draw emergency message page
void drawEmergency() {
	int icon_num = 60;

	if (updateIndicator)
		drawIndicator();

	if (emgBuffer[emgCurDisp][2] > -1 && emgBuffer[emgCurDisp][2] < ICON_ARRAY_SIZE)
		icon_num = (int)(emgBuffer[emgCurDisp][2]);

	drawIcon(centerX - 8, centerY - 20, icon_num);

	char s[2] = " ";
	for (int i = 3; i < EMG_BUFFER_MAX; i++)
	{
		char curChar = emgBuffer[emgCurDisp][i];
		if (curChar == 0x00) break;
		if (curChar >= 0xf0) continue;
		s[0] = curChar;
		display.drawStr(10, centerY + 10 + i * 5, s);
	}
}
*/
/*
// Draw normal message page
void drawMessage() {
	int icon_num = 0;

	if (updateIndicator)
		drawIndicator();

	if (msgBuffer[msgCurDisp][2] > -1 && msgBuffer[msgCurDisp][2] < ICON_ARRAY_SIZE)
		icon_num = (int)(msgBuffer[msgCurDisp][2]);

	drawIcon(centerX - 8, centerY - 20, icon_num);

	char s[2] = " ";
	//  display.print(msgCurDisp);  // For debug
	for (int i = 3; i < MSG_BUFFER_MAX; i++)
	{
		char curChar = msgBuffer[msgCurDisp][i];
		if (curChar == 0x00) break;
		if (curChar >= 0xf0) continue;
		s[0] = curChar;
		display.drawStr(20, centerY + 10 + i * 5, s);
	}
}
*/
// Draw main clock screen
// Clock style changes according to user selection
void drawClock() {

	//Serial.println("drawClock");
	switch (clockStyle)
	{

	case CLOCK_STYLE_SIMPLE_DIGIT:
		//Serial.println("drawClock1");
		display.setFont(u8g_font_helvB14r);
		drawDayAmPm(centerX - 34, centerY - 17);

		display.setFont(u8g_font_helvB18r);
		drawClockDigital(centerX - 29, centerY + 6);

		break;

	case CLOCK_STYLE_SIMPLE_MIX:
		//Serial.println("drawClock2");
		drawClockAnalog(0, -30, iRadius - 6);

		display.setFont(u8g_font_helvB12r);
		drawDayAmPm(centerY * 2 + 3, 23);


		display.setFont(u8g_font_helvB18r);
		drawClockDigital(centerY * 2 - 3, 45);
		break;

	case CLOCK_STYLE_SIMPLE_ANALOG:
		//Serial.println("drawClock3");
		drawClockAnalog(0, 0, iRadius);

		break;

	case CLOCK_STYLE_SIMPLE_DIGIT_SEC:
		display.setFont(u8g_font_helvB10r);
		drawDateDigital(centerX - 33, centerY - 20);

		display.setFont(u8g_font_helvB18r);
		byte offset = drawClockDigital(centerX - 45, centerY + 6);

		display.setFont(u8g_font_helvB12r);

		drawSecondsDigital(centerX - 45 + offset + 2, centerY + 6);

		break;
	}
	//Serial.println("drawClockEnd");
}

// Draw idle page
void drawIdleClock() {

	//if (updateIndicator)
		//drawIndicator();
}

void drawDayAmPm(byte xPos, byte yPos) {
	display.drawStr(xPos, yPos, (const char*)pgm_read_word(&(weekString[iWeek])));
	display.drawStr(xPos + display.getStrPixelWidth((const char*)pgm_read_word(&(weekString[iWeek]))) + 2, yPos, (const char*)pgm_read_word(&(ampmString[iAmPm])));
}

byte drawClockDigital(byte xPos, byte yPos) {
	//Serial.println("drawClockDigital");
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

	//Serial.println("drawClockDigital2");
}

void drawSecondsDigital(byte xPos, byte yPos) {
	//Serial.println("drawSecondsDigital");
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
	//Serial.println("drawSecondsDigital2");
}

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

void drawClockAnalog(short offsetY, short offsetX, byte radius) {
	//Serial.println("drawClockAnalog");
	display.drawCircle(centerX + offsetX, centerY + offsetY, radius);
	showTimePin(centerX + offsetX, centerY + offsetY, 0.1, 0.5, iHour * 5 + (int)(iMinutes * 5 / 60));
	showTimePin(centerX + offsetX, centerY + offsetY, 0.1, 0.78, iMinutes);
	//Serial.println("drawClockAnalog2");
	// showTimePin(centerX, centerY, 0.1, 0.9, iSecond);
}

// Returns starting point of normal string to display
int getCenterAlignedXOfMsg(int msgIndex) {
	int pointX = centerX;
	for (int i = 3; i < MSG_BUFFER_MAX; i++) {
		char curChar = msgBuffer[msgIndex][i];
		if (curChar == 0x00) break;
		if (curChar >= 0xf0) continue;
		pointX -= 3;
	}
	if (pointX < 0) pointX = 0;
	return pointX;
}

// Returns starting point of emergency string to display
int getCenterAlignedXOfEmg(int emgIndex) {
	int pointX = centerX;
	for (int i = 3; i < EMG_BUFFER_MAX; i++) {
		char curChar = emgBuffer[emgIndex][i];
		if (curChar == 0x00) break;
		if (curChar >= 0xf0) continue;
		pointX -= 3;
	}
	if (pointX < 0) pointX = 0;
	return pointX;
}

// Calculate clock pin position
double RAD = 3.141592 / 180;
double LR = 89.99;
void showTimePin(int center_x, int center_y, double pl1, double pl2, double pl3) {
	double x1, x2, y1, y2;
	x1 = center_x + (iRadius * pl1) * cos((6 * pl3 + LR) * RAD);
	y1 = center_y + (iRadius * pl1) * sin((6 * pl3 + LR) * RAD);
	x2 = center_x + (iRadius * pl2) * cos((6 * pl3 - LR) * RAD);
	y2 = center_y + (iRadius * pl2) * sin((6 * pl3 - LR) * RAD);

	display.drawLine((int)x1, (int)y1, (int)x2, (int)y2);
}
/*
// Icon drawing tool
void drawIcon(int posx, int posy, int icon_num) {
	if (icon_num < 0 || icon_num >= ICON_ARRAY_SIZE)
		return;

	//display.drawBitmap(posx, posy, 16, 16, (const unsigned char*)pgm_read_word(&(bitmap_array[icon_num])));
}
*/

