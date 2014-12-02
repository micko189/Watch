#include <stdlib.h>  
#include <stdio.h>  
typedef unsigned char byte;
typedef unsigned char boolean;
#define HIGH 1
#define LOW 0

const char* weekString[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const byte daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; //standard year

short iYear = 2014;
byte iMonth = 11;
byte iDay = 27;
byte iWeek = 0;    // 1: SUN, MON, TUE, WED, THU, FRI,SAT // need to calculate this

const short firstYear = 2000; //This is our start point
const byte dayOffset = 6 - 1; //The first day of our start year may not be a Sunday ( in 2000 it was Sat)

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
	for (char i = 4; i >= 0; i--)
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


void main()
{
	short val = 1050;
	char s[5];
	stoa(val, s);
	byte index = calcDayOfWeek();

}