#include <stdlib.h>  
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>

#include "u8g.h"
#include "..\FontsBDFgen\helvBr.c"

using namespace std;

typedef unsigned char byte;
typedef unsigned char boolean;
#define HIGH 1
#define LOW 0

const char* weekString[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const byte daysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; //standard year

short iYear = 2014;
byte iMonth = 12;
byte iDay = 2;
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


byte dayInM(byte month)
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
		return daysInMonth[month];
	}
	else // feb
	{
		return (isLeapYear(iYear)) ? 29 : 28;
	}
}

byte getDaysInMonth2(byte month)
{
	if (month != 2)
	{
		return dayInM(month);
	}
	else // feb
	{
		return 28 + (isLeapYear(iYear));
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
	byte firstIndex = 1;
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




u8g_fntpgm_uint8_t reduced[8 * 1024] = { 0 };

/*========================================================================*/
/* low level byte and word access */

/* removed NOINLINE, because it leads to smaller code, might also be faster */
//static uint8_t u8g_font_get_byte(const u8g_fntpgm_uint8_t *font, uint8_t offset) U8G_NOINLINE;
static uint8_t u8g_font_get_byte(const u8g_fntpgm_uint8_t *font, uint8_t offset)
{
	font += offset;
	return u8g_pgm_read((u8g_pgm_uint8_t *)font);
}

static uint16_t u8g_font_get_word(const u8g_fntpgm_uint8_t *font, uint8_t offset)
{
	uint16_t pos;
	font += offset;
	pos = u8g_pgm_read((u8g_pgm_uint8_t *)font);
	font++;
	pos <<= 8;
	pos += u8g_pgm_read((u8g_pgm_uint8_t *)font);
	return pos;
}

/*========================================================================*/
/* direct access on the font */

static uint8_t u8g_font_GetFormat(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 0);
}

static uint8_t u8g_font_GetFontGlyphStructureSize(const u8g_fntpgm_uint8_t *font)
{
	switch (u8g_font_GetFormat(font))
	{
	case 0: return 6;
	case 1: return 3;
	case 2: return 6;
	}
	return 3;
}

static uint8_t u8g_font_GetBBXWidth(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 1);
}

static uint8_t u8g_font_GetBBXHeight(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 2);
}

static int8_t u8g_font_GetBBXOffX(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 3);
}

static int8_t u8g_font_GetBBXOffY(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 4);
}

uint8_t u8g_font_GetCapitalAHeight(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 5);
}

uint16_t u8g_font_GetEncoding65Pos(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_word(font, 6);
}

uint16_t u8g_font_GetEncoding97Pos(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_word(font, 8);
}

uint8_t u8g_font_GetFontStartEncoding(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 10);
}

uint8_t u8g_font_GetFontEndEncoding(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 11);
}

int8_t u8g_font_GetLowerGDescent(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 12);
}

int8_t u8g_font_GetFontAscent(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 13);
}

int8_t u8g_font_GetFontDescent(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 14);
}

int8_t u8g_font_GetFontXAscent(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 15);
}

int8_t u8g_font_GetFontXDescent(const u8g_fntpgm_uint8_t *font)
{
	return u8g_font_get_byte(font, 16);
}


/* calculate the overall length of the font, only used to create the picture for the google wiki */
size_t u8g_font_GetSize(const u8g_fntpgm_uint8_t *font)
{
	uint8_t *p = (uint8_t *)(font);
	uint8_t font_format = u8g_font_GetFormat(font);
	uint8_t data_structure_size = u8g_font_GetFontGlyphStructureSize(font);
	uint8_t start, end;
	uint8_t i;
	uint8_t mask = 255;

	start = u8g_font_GetFontStartEncoding(font);
	end = u8g_font_GetFontEndEncoding(font);

	if (font_format == 1)
		mask = 15;

	p += U8G_FONT_DATA_STRUCT_SIZE;       /* skip font general information */

	i = start;
	for (;;)
	{
		if (u8g_pgm_read((u8g_pgm_uint8_t *)(p)) == 255)
		{
			p += 1;
		}
		else
		{
			p += u8g_pgm_read(((u8g_pgm_uint8_t *)(p)) + 2) & mask;
			p += data_structure_size;
		}
		if (i == end)
			break;
		i++;
	}

	return p - (uint8_t *)font;
}

struct u8g_t
{
	int8_t glyph_dx;
	int8_t glyph_x;
	int8_t glyph_y;
	uint8_t glyph_width;
	uint8_t glyph_height;
	const u8g_pgm_uint8_t *font;             /* regular font for all text procedures */
};

/*========================================================================*/
/* u8g interface, font access */

uint8_t u8g_GetFontBBXWidth(u8g_t *u8g)
{
	return u8g_font_GetBBXWidth(u8g->font);
}

uint8_t u8g_GetFontBBXHeight(u8g_t *u8g)
{
	return u8g_font_GetBBXHeight(u8g->font);
}

int8_t u8g_GetFontBBXOffX(u8g_t *u8g)
{
	return u8g_font_GetBBXOffX(u8g->font);
}

int8_t u8g_GetFontBBXOffY(u8g_t *u8g)
{
	return u8g_font_GetBBXOffY(u8g->font);
}

uint8_t u8g_GetFontCapitalAHeight(u8g_t *u8g)
{
	return u8g_font_GetCapitalAHeight(u8g->font);
}

typedef void * u8g_glyph_t;

/*========================================================================*/
/* glyph handling */

static void u8g_CopyGlyphDataToCache(u8g_t *u8g, u8g_glyph_t g)
{
	uint8_t tmp;
	switch (u8g_font_GetFormat(u8g->font))
	{
	case 0:
	case 2:
		/*
		format 0
		glyph information
		offset
		0             BBX width                                       unsigned
		1             BBX height                                      unsigned
		2             data size                                          unsigned    (BBX width + 7)/8 * BBX height
		3             DWIDTH                                          signed
		4             BBX xoffset                                    signed
		5             BBX yoffset                                    signed
		byte 0 == 255 indicates empty glyph
		*/
		u8g->glyph_width = u8g_pgm_read(((u8g_pgm_uint8_t *)g) + 0);
		u8g->glyph_height = u8g_pgm_read(((u8g_pgm_uint8_t *)g) + 1);
		u8g->glyph_dx = u8g_pgm_read(((u8g_pgm_uint8_t *)g) + 3);
		u8g->glyph_x = u8g_pgm_read(((u8g_pgm_uint8_t *)g) + 4);
		u8g->glyph_y = u8g_pgm_read(((u8g_pgm_uint8_t *)g) + 5);
		break;
	case 1:
	default:
		/*
		format 1
		0             BBX xoffset                                    signed   --> upper 4 Bit
		0             BBX yoffset                                    signed --> lower 4 Bit
		1             BBX width                                       unsigned --> upper 4 Bit
		1             BBX height                                      unsigned --> lower 4 Bit
		2             data size                                           unsigned -(BBX width + 7)/8 * BBX height  --> lower 4 Bit
		2             DWIDTH                                          signed --> upper  4 Bit
		byte 0 == 255 indicates empty glyph
		*/

		tmp = u8g_pgm_read(((u8g_pgm_uint8_t *)g) + 0);
		u8g->glyph_y = tmp & 15;
		u8g->glyph_y -= 2;
		tmp >>= 4;
		u8g->glyph_x = tmp;

		tmp = u8g_pgm_read(((u8g_pgm_uint8_t *)g) + 1);
		u8g->glyph_height = tmp & 15;
		tmp >>= 4;
		u8g->glyph_width = tmp;

		tmp = u8g_pgm_read(((u8g_pgm_uint8_t *)g) + 2);
		tmp >>= 4;
		u8g->glyph_dx = tmp;


		break;
	}
}

//void u8g_FillEmptyGlyphCache(u8g_t *u8g) U8G_NOINLINE;
static void u8g_FillEmptyGlyphCache(u8g_t *u8g)
{
	u8g->glyph_dx = 0;
	u8g->glyph_width = 0;
	u8g->glyph_height = 0;
	u8g->glyph_x = 0;
	u8g->glyph_y = 0;
}

uint8_t startF, endF;

/*
Find (with some speed optimization) and return a pointer to the glyph data structure
Also uncompress (format 1) and copy the content of the data structure to the u8g structure
*/
u8g_glyph_t u8g_GetGlyph(u8g_t *u8g, uint8_t requested_encoding)
{
	uint8_t *p = (uint8_t *)(u8g->font);
	uint8_t font_format = u8g_font_GetFormat(u8g->font);
	uint8_t data_structure_size = u8g_font_GetFontGlyphStructureSize(u8g->font);
	uint8_t start, end;
	uint16_t pos;
	uint8_t i;
	uint8_t mask = 255;

	if (font_format == 1)
		mask = 15;

	startF = start = u8g_font_GetFontStartEncoding(u8g->font);
	endF = end = u8g_font_GetFontEndEncoding(u8g->font);

	pos = u8g_font_GetEncoding97Pos(u8g->font);
	if (requested_encoding >= 97 && pos > 0)
	{
		p += pos;
		start = 97;
	}
	else
	{
		pos = u8g_font_GetEncoding65Pos(u8g->font);
		if (requested_encoding >= 65 && pos > 0)
		{
			p += pos;
			start = 65;
		}
		else
			p += U8G_FONT_DATA_STRUCT_SIZE;       /* skip font general information */
	}

	if (requested_encoding > end)
	{
		u8g_FillEmptyGlyphCache(u8g);
		return NULL;                      /* not found */
	}

	i = start;
	if (i <= end)
	{
		for (;;)
		{
			if (u8g_pgm_read((u8g_pgm_uint8_t *)(p)) == 255)
			{
				p += 1;
			}
			else
			{
				if (i == requested_encoding)
				{
					u8g_CopyGlyphDataToCache(u8g, p);
					return p;
				}
				p += u8g_pgm_read(((u8g_pgm_uint8_t *)(p)) + 2) & mask;
				p += data_structure_size;
			}
			if (i == end)
				break;
			i++;
		}
	}

	u8g_FillEmptyGlyphCache(u8g);

	return NULL;
}


uint16_t u8g_CreateReduced(u8g_t *u8g, uint8_t *red, uint8_t requested_encoding[])
{
	uint8_t *r = (uint8_t *)(red);

	uint8_t *p = (uint8_t *)(u8g->font);
	uint8_t font_format = u8g_font_GetFormat(u8g->font);
	uint8_t data_structure_size = u8g_font_GetFontGlyphStructureSize(u8g->font);
	uint8_t start, end;
	uint16_t pos;
	uint8_t i;
	uint8_t mask = 255;

	if (font_format == 1)
		mask = 15;

	start = u8g_font_GetFontStartEncoding(u8g->font);
	end = u8g_font_GetFontEndEncoding(u8g->font);

	// Copy header
	memcpy(r, p, U8G_FONT_DATA_STRUCT_SIZE);

	//pos = u8g_font_GetEncoding97Pos(u8g->font);
	//if (requested_encoding >= 97 && pos > 0)
	//{
	//	p += pos;
	//	start = 97;
	//}
	//else
	//{
	//	pos = u8g_font_GetEncoding65Pos(u8g->font);
	//	if (requested_encoding >= 65 && pos > 0)
	//	{
	//		p += pos;
	//		start = 65;
	//	}
	//	else
			p += U8G_FONT_DATA_STRUCT_SIZE;       /* skip font general information */
			r += U8G_FONT_DATA_STRUCT_SIZE;       /* skip font general information */
	//}

	//if (requested_encoding > end)
	//{
	//	u8g_FillEmptyGlyphCache(u8g);
	//	return NULL;                      /* not found */
	//}

	uint16_t pos65 = 0;
	uint16_t pos97 = 0;

	i = start;
	if (i <= end)
	{
		for (;;)
		{
			if (i == 65)
			{
				pos65 = r - red;

				*(red + 6) = (pos65 & 0xFF00)>>8;
				*(red + 7) = pos65 & 0xFF;

			}

			if (i == 97)
			{
				pos97 = r - red;

				*(red + 8) = (pos97 & 0xFF00) >> 8;
				*(red + 9) = pos97 & 0xFF;
			}

			if (u8g_pgm_read((u8g_pgm_uint8_t *)(p)) == 255)
			{
				*r = *p;
				p += 1;
				r += 1;
			}
			else
			{
				if (requested_encoding[i] == 1) // take this glyph
				{
					u8g_CopyGlyphDataToCache(u8g, p);

					memcpy(r, p, u8g_pgm_read(((u8g_pgm_uint8_t *)(p)) + 2) & mask);
					memcpy(r, p, data_structure_size);

					r += u8g_pgm_read(((u8g_pgm_uint8_t *)(p)) + 2) & mask;
					r += data_structure_size;
					//return p;
				}
				else
				{
					// Skip this character 
					*r = 255;
					r += 1;
				}

				p += u8g_pgm_read(((u8g_pgm_uint8_t *)(p)) + 2) & mask;
				p += data_structure_size;
			}
			if (i == end)
				break;
			i++;
		}
	}

	//u8g_FillEmptyGlyphCache(u8g);

	return r - red;
}

void generateCFile(char* name, u8g_fntpgm_uint8_t* data, int r_size, ofstream &fout)
{
	fout << "#include \"u8g.h\"" << endl;
	fout << "const u8g_fntpgm_uint8_t " << name << "[" << (int)r_size << "] U8G_FONT_SECTION(\"" << name << "\") = {";

	for (size_t i = 0; i < r_size; i++)
	{
		if (i % 16 == 0)
		{
			fout << endl << "  ";
		}

		fout << (int)data[i];

		if (i != r_size - 1)
		{
			fout << ",";
		}

	}

	fout << "};" << endl;
}

void SetRequestEncoding(bool digits, bool uppercase, bool lovercase, uint8_t* requested_encoding)
{
	for (size_t i = 0; i < 256; i++)
	{
		requested_encoding[i] = 0;
	}

	if (digits)
	{
		// Set digits
		for (size_t i = 48; i <= 57; i++)
		{
			requested_encoding[i] = 1;
		}
	}

	if (uppercase)
	{
		// Set upersase letters
		for (size_t i = 65; i <= 90; i++)
		{
			requested_encoding[i] = 1;
		}
	}

	if (lovercase)
	{
		// Set lowercase letters
		for (size_t i = 97; i <= 122; i++)
		{
			requested_encoding[i] = 1;
		}
	}
}


void main()
{
	uint8_t requested_encoding[256] = { 0 };
	ofstream fout("helvBr_gen.c");
	uint16_t r_size = 0;
	u8g_t u;

	// 08
	u.font = helvB10r;
	SetRequestEncoding(true, false, true, requested_encoding);
	r_size = u8g_CreateReduced(&u, reduced, requested_encoding);
	generateCFile("helvB08r", reduced, r_size, fout);

	// 10
	u.font = helvB10r;
	SetRequestEncoding(true, true, true, requested_encoding);
	requested_encoding[47] = 1; // 47		/	 	Slash or divide
	requested_encoding[56] = 1; // 58		:	 	Colon
	r_size = u8g_CreateReduced(&u, reduced, requested_encoding);
	generateCFile("helvB10r", reduced, r_size, fout);

	// 12
	u.font = helvB12r;
	SetRequestEncoding(true, false, false, requested_encoding);
	requested_encoding[46] = 1; // 46		.	 	Period, dot or full stop
	requested_encoding[67] = 1; // 67		C	 	Uppercase C
	requested_encoding[176] = 1; // 176		°		Degree sign
	r_size = u8g_CreateReduced(&u, reduced, requested_encoding);
	generateCFile("helvB12r", reduced, r_size, fout);

	// 14
	u.font = helvB14r;
	SetRequestEncoding(true, true, true, requested_encoding);
	r_size = u8g_CreateReduced(&u, reduced, requested_encoding);
	generateCFile("helvB14r", reduced, r_size, fout);

	// close file
	fout.close();

	u.font = reduced;
	u8g_GetGlyph(&u, 46);
	u.font = helvB12r;
	u8g_GetGlyph(&u, 46);

	u.font = reduced;
	u8g_GetGlyph(&u, 67);
	u.font = helvB12r;
	u8g_GetGlyph(&u, 67);

	u.font = reduced;
	u8g_GetGlyph(&u, 176);
	u.font = helvB12r;
	u8g_GetGlyph(&u, 176);

	short val = 0;
	char s[5];
	stoa(val, s);
	calcDayOfWeek();

	for (byte i = 1; i <= 12; i++)
	{
		if (getDaysInMonth(i) != getDaysInMonth2(i))
		{
			int j = 0;
		}
	}

}

/*
32		 	 	Space
33		!	 	Exclamation mark
34		"	&quot;	Double quotes (or speech marks)
35		#	 	Number
36		$	 	Dollar
37		%	 	Procenttecken
38		&	&amp;	Ampersand
39		'	 	Single quote
40		(	 	Open parenthesis (or open bracket)
41		)	 	Close parenthesis (or close bracket)
42		*	 	Asterisk
43		+	 	Plus
44		,	 	Comma
45		-	 	Hyphen
46		.	 	Period, dot or full stop
47		/	 	Slash or divide
48		0	 	Zero
49		1	 	One
50		2	 	Two
51		3	 	Three
52		4	 	Four
53		5	 	Five
54		6	 	Six
55		7	 	Seven
56		8	 	Eight
57		9	 	Nine
58		:	 	Colon
59		;	 	Semicolon
60		<	&lt;	Less than (or open angled bracket)
61		=	 	Equals
62		>	&gt;	Greater than (or close angled bracket)
63		?	 	Question mark
64		@	 	At symbol
65		A	 	Uppercase A
66		B	 	Uppercase B
67		C	 	Uppercase C
68		D	 	Uppercase D
69		E	 	Uppercase E
70		F	 	Uppercase F
71		G	 	Uppercase G
72		H	 	Uppercase H
73		I	 	Uppercase I
74		J	 	Uppercase J
75		K	 	Uppercase K
76		L	 	Uppercase L
77		M	 	Uppercase M
78		N	 	Uppercase N
79		O	 	Uppercase O
80		P	 	Uppercase P
81		Q	 	Uppercase Q
82		R	 	Uppercase R
83		S	 	Uppercase S
84		T	 	Uppercase T
85		U	 	Uppercase U
86		V	 	Uppercase V
87		W	 	Uppercase W
88		X	 	Uppercase X
89		Y	 	Uppercase Y
90		Z	 	Uppercase Z
91		[	 	Opening bracket
92		\	 	Backslash
93		]	 	Closing bracket
94		^	 	Caret - circumflex
95		_	 	Underscore
96		`	 	Grave accent
97		a	 	Lowercase a
98		b	 	Lowercase b
99		c	 	Lowercase c
100		d	 	Lowercase d
101		e	 	Lowercase e
102		f	 	Lowercase f
103		g	 	Lowercase g
104		h	 	Lowercase h
105		i	 	Lowercase i
106		j	 	Lowercase j
107		k	 	Lowercase k
108		l	 	Lowercase l
109		m	 	Lowercase m
110		n	 	Lowercase n
111		o	 	Lowercase o
112		p	 	Lowercase p
113		q	 	Lowercase q
114		r	 	Lowercase r
115		s	 	Lowercase s
116		t	 	Lowercase t
117		u	 	Lowercase u
118		v	 	Lowercase v
119		w	 	Lowercase w
120		x	 	Lowercase x
121		y	 	Lowercase y
122		z	 	Lowercase z
123		{	 	Opening brace
124		|	 	Vertical bar
125		}	 	Closing brace
126		~	 	Equivalency sign - tilde
127			 	Delete

161		¡		Inverted exclamation mark
162		¢		Cent sign
163		£		Pound sign
164		¤		Currency sign
165		¥		Yen sign
166		¦		Pipe, Broken vertical bar
167		§		Section sign
168		¨		Spacing diaeresis - umlaut
169		©		Copyright sign
170		ª		Feminine ordinal indicator
171		«		Left double angle quotes
172		¬		Not sign
173				Soft hyphen
174		®		Registered trade mark sign
175		¯		Spacing macron - overline
176		°		Degree sign
177		±		Plus-or-minus sign
178		²		Superscript two - squared
179		³		Superscript three - cubed
180		´		Acute accent - spacing acute
181		µ		Micro sign
182		¶		Pilcrow sign - paragraph sign
183		·		Middle dot - Georgian comma
184		¸		Spacing cedilla
185		¹		Superscript one
186		º		Masculine ordinal indicator
187		»		Right double angle quotes
188		¼		Fraction one quarter
189		½		Fraction one half
190		¾		Fraction three quarters
191		¿		Inverted question mark
192		À		Latin capital letter A with grave
193		Á		Latin capital letter A with acute
194		Â		Latin capital letter A with circumflex
195		Ã		Latin capital letter A with tilde
196		Ä		Latin capital letter A with diaeresis
197		Å		Latin capital letter A with ring above
198		Æ		Latin capital letter AE
199		Ç		Latin capital letter C with cedilla
200		È		Latin capital letter E with grave
201		É		Latin capital letter E with acute
202		Ê		Latin capital letter E with circumflex
203		Ë		Latin capital letter E with diaeresis
204		Ì		Latin capital letter I with grave
205		Í		Latin capital letter I with acute
206		Î		Latin capital letter I with circumflex
207		Ï		Latin capital letter I with diaeresis
208		Ð		Latin capital letter ETH
209		Ñ		Latin capital letter N with tilde
210		Ò		Latin capital letter O with grave
211		Ó		Latin capital letter O with acute
212		Ô		Latin capital letter O with circumflex
213		Õ		Latin capital letter O with tilde
214		Ö		Latin capital letter O with diaeresis
215		×		Multiplication sign
216		Ø		Latin capital letter O with slash
217		Ù		Latin capital letter U with grave
218		Ú		Latin capital letter U with acute
219		Û		Latin capital letter U with circumflex
220		Ü		Latin capital letter U with diaeresis
221		Ý		Latin capital letter Y with acute
222		Þ		Latin capital letter THORN
223		ß		Latin small letter sharp s - ess-zed
224		à		Latin small letter a with grave
225		á		Latin small letter a with acute
226		â		Latin small letter a with circumflex
227		ã		Latin small letter a with tilde
228		ä		Latin small letter a with diaeresis
229		å		Latin small letter a with ring above
230		æ		Latin small letter ae
231		ç		Latin small letter c with cedilla
232		è		Latin small letter e with grave
233		é		Latin small letter e with acute
234		ê		Latin small letter e with circumflex
235		ë		Latin small letter e with diaeresis
236		ì		Latin small letter i with grave
237		í		Latin small letter i with acute
238		î		Latin small letter i with circumflex
239		ï		Latin small letter i with diaeresis
240		ð		Latin small letter eth
241		ñ		Latin small letter n with tilde
242		ò		Latin small letter o with grave
243		ó		Latin small letter o with acute
244		ô		Latin small letter o with circumflex
245		õ		Latin small letter o with tilde
246		ö		Latin small letter o with diaeresis
247		÷		Division sign
248		ø		Latin small letter o with slash
249		ù		Latin small letter u with grave
250		ú		Latin small letter u with acute
251		û		Latin small letter u with circumflex
252		ü		Latin small letter u with diaeresis
253		ý		Latin small letter y with acute
254		þ		Latin small letter thorn
255		ÿ		Latin small letter y with diaeresis
*/