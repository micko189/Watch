#include <windows.h>
#include <stdlib.h>  
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>

#include "u8g.h"
#include "..\FontsBDFgen\helvBr.c"

using namespace std;

typedef unsigned char byte;
typedef unsigned char boolean;
#define HIGH 1
#define LOW 0

u8g_fntpgm_uint8_t reduced[8 * 1024] = { 0 };

/*========================================================================*/
/* low level byte and word access */

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

struct u8g_box_t
{
	u8g_uint_t x0, y0, x1, y1;
};

struct u8g_t
{
	int8_t glyph_dx;
	int8_t glyph_x;
	int8_t glyph_y;
	uint8_t glyph_width;
	uint8_t glyph_height;
	const u8g_pgm_uint8_t *font;             /* regular font for all text procedures */

	typedef u8g_uint_t(*u8g_font_calc_vref_fnptr)(u8g_t *u8g);
	u8g_font_calc_vref_fnptr font_calc_vref;

	int8_t font_ref_ascent;
	int8_t font_ref_descent;

	u8g_box_t current_page;		/* current box of the visible page */

	uint8_t screenBuffNew[128][64];
	uint8_t screenBuffOld[128][64];
};

u8g_uint_t u8g_font_calc_vref_font(u8g_t *u8g)
{
	return 0;
}

void u8g_SetFontPosBaseline(u8g_t *u8g)
{
	u8g->font_calc_vref = u8g_font_calc_vref_font;
}

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

static void u8g_FillEmptyGlyphCache(u8g_t *u8g)
{
	u8g->glyph_dx = 0;
	u8g->glyph_width = 0;
	u8g->glyph_height = 0;
	u8g->glyph_x = 0;
	u8g->glyph_y = 0;
}

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

	start = u8g_font_GetFontStartEncoding(u8g->font);
	end = u8g_font_GetFontEndEncoding(u8g->font);

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

static uint8_t U8G_ALWAYS_INLINE u8g_is_intersection_decision_tree(u8g_uint_t a0, u8g_uint_t a1, u8g_uint_t v0, u8g_uint_t v1)
{
	return 1;
	/* surprisingly the macro leads to larger code */
	/* return U8G_IS_INTERSECTION_MACRO(a0,a1,v0,v1); */
	if (v0 <= a1)
	{
		if (v1 >= a0)
		{
			return 1;
		}
		else
		{
			if (v0 > v1)
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
	}
	else
	{
		if (v1 >= a0)
		{
			if (v0 > v1)
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}
}

uint8_t u8g_IsBBXIntersection(u8g_t *u8g, u8g_uint_t x, u8g_uint_t y, u8g_uint_t w, u8g_uint_t h)
{
	register u8g_uint_t tmp;
	tmp = y;
	tmp += h;
	tmp--;
	if (u8g_is_intersection_decision_tree(u8g->current_page.y0, u8g->current_page.y1, y, tmp) == 0)
		return 0;

	tmp = x;
	tmp += w;
	tmp--;
	return u8g_is_intersection_decision_tree(u8g->current_page.x0, u8g->current_page.x1, x, tmp);
}

/* return the data start for a font and the glyph pointer */
static uint8_t *u8g_font_GetGlyphDataStart(const void *font, u8g_glyph_t g)
{
	return ((u8g_fntpgm_uint8_t *)g) + u8g_font_GetFontGlyphStructureSize((const u8g_fntpgm_uint8_t *)font);
}

void GotoXY(byte x, byte y)
{
	COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void printPixel(byte x, byte y)
{
	GotoXY(x, y);
	printf("%c", 178);
}

void clearPixel(byte x, byte y)
{
	GotoXY(x, y);
	printf("%c", ' ');
}

void setPixel(u8g_t *u, byte x, byte y)
{
	u->screenBuffNew[x][y] = 1;
}

void u8g_Draw8Pixel(u8g_t *u8g, u8g_uint_t x, u8g_uint_t y, uint8_t dir, uint8_t pixel)
{
	byte mask = 0x80;
	for (size_t i = 0; i < 8; i++)
	{
		if (pixel & mask)
		{
			if (dir == 0)
			{
				setPixel(u8g, x + i, y);
			}
			else
			{
				setPixel(u8g, x, y + i);
			}
		}

		mask = mask >> 1;
	}

}

int8_t u8g_draw_glyph(u8g_t *u8g, u8g_uint_t x, u8g_uint_t y, uint8_t encoding)
{
	const u8g_pgm_uint8_t *data;
	uint8_t w, h;
	uint8_t i, j;
	u8g_uint_t ix, iy;

	{
		u8g_glyph_t g = u8g_GetGlyph(u8g, encoding);
		if (g == NULL)
			return 0;
		data = u8g_font_GetGlyphDataStart(u8g->font, g);
	}

	w = u8g->glyph_width;
	h = u8g->glyph_height;

	x += u8g->glyph_x;
	y -= u8g->glyph_y;
	y--;

	if (u8g_IsBBXIntersection(u8g, x, y - h + 1, w, h) == 0)
		return u8g->glyph_dx;

	/* now, w is reused as bytes per line */
	w += 7;
	w /= 8;

	iy = y;
	iy -= h;
	iy++;

	for (j = 0; j < h; j++)
	{
		ix = x;
		for (i = 0; i < w; i++)
		{
			u8g_Draw8Pixel(u8g, ix, iy, 0, u8g_pgm_read(data));
			data++;
			ix += 8;
		}
		iy++;
	}
	return u8g->glyph_dx;
}

int8_t u8g_DrawGlyph(u8g_t *u8g, u8g_uint_t x, u8g_uint_t y, uint8_t encoding)
{
	y += u8g->font_calc_vref(u8g);
	return u8g_draw_glyph(u8g, x, y, encoding);
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

	p += U8G_FONT_DATA_STRUCT_SIZE;       /* skip font general information */
	r += U8G_FONT_DATA_STRUCT_SIZE;       /* skip font general information */

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

				*(red + 6) = (pos65 & 0xFF00) >> 8;
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

byte getStrPixelWidth(char* s)
{
	return strlen(s);
}

#define pgm_read_word(x) *(x)

void u8g_draw_hline(u8g_t *u8g, u8g_uint_t x, u8g_uint_t y, u8g_uint_t w)
{
	uint8_t pixel = 0x0ff;
	while (w >= 8)
	{
		u8g_Draw8Pixel(u8g, x, y, 0, pixel);
		w -= 8;
		x += 8;
	}
	if (w != 0)
	{
		w ^= 7;
		w++;
		pixel <<= w & 7;
		u8g_Draw8Pixel(u8g, x, y, 0, pixel);
	}
}

void u8g_draw_vline(u8g_t *u8g, u8g_uint_t x, u8g_uint_t y, u8g_uint_t h)
{
	uint8_t pixel = 0x0ff;
	while (h >= 8)
	{
		u8g_Draw8Pixel(u8g, x, y, 1, pixel);
		h -= 8;
		y += 8;
	}
	if (h != 0)
	{
		h ^= 7;
		h++;
		pixel <<= h & 7;
		u8g_Draw8Pixel(u8g, x, y, 1, pixel);
	}
}

void u8g_DrawHBitmapP(u8g_t *u8g, u8g_uint_t x, u8g_uint_t y, u8g_uint_t cnt, const u8g_pgm_uint8_t *bitmap)
{
	while (cnt > 0)
	{
		u8g_Draw8Pixel(u8g, x, y, 0, u8g_pgm_read(bitmap));
		bitmap++;
		cnt--;
		x += 8;
	}
}

static void u8g_draw_circle_section(u8g_t *u8g, u8g_uint_t x, u8g_uint_t y, u8g_uint_t x0, u8g_uint_t y0, uint8_t option)
{
	/* upper right */
	if (option & U8G_DRAW_UPPER_RIGHT)
	{
		setPixel(u8g, x0 + x, y0 - y);
		setPixel(u8g, x0 + y, y0 - x);
	}

	/* upper left */
	if (option & U8G_DRAW_UPPER_LEFT)
	{
		setPixel(u8g, x0 - x, y0 - y);
		setPixel(u8g, x0 - y, y0 - x);
	}

	/* lower right */
	if (option & U8G_DRAW_LOWER_RIGHT)
	{
		setPixel(u8g, x0 + x, y0 + y);
		setPixel(u8g, x0 + y, y0 + x);
	}

	/* lower left */
	if (option & U8G_DRAW_LOWER_LEFT)
	{
		setPixel(u8g, x0 - x, y0 + y);
		setPixel(u8g, x0 - y, y0 + x);
	}
}

void u8g_draw_circle(u8g_t *u8g, u8g_uint_t x0, u8g_uint_t y0, u8g_uint_t rad, uint8_t option)
{
	u8g_int_t f;
	u8g_int_t ddF_x;
	u8g_int_t ddF_y;
	u8g_uint_t x;
	u8g_uint_t y;

	f = 1;
	f -= rad;
	ddF_x = 1;
	ddF_y = 0;
	ddF_y -= rad;
	ddF_y *= 2;
	x = 0;
	y = rad;

	u8g_draw_circle_section(u8g, x, y, x0, y0, option);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		u8g_draw_circle_section(u8g, x, y, x0, y0, option);
	}
}
typedef char(*u8g_font_get_char_fn)(const void *s);
u8g_uint_t u8g_font_calc_str_pixel_width(u8g_t *u8g, const char *s, u8g_font_get_char_fn get_char)
{
	u8g_uint_t  w;
	uint8_t enc;

	/* reset the total minimal width to zero, this will be expanded during calculation */
	w = 0;

	enc = get_char(s);

	/* check for empty string, width is already 0 */
	if (enc == '\0')
	{
		return w;
	}

	/* get the glyph information of the first char. This must be valid, because we already checked for the empty string */
	/* if *s is not inside the font, then the cached parameters of the glyph are all zero */
	u8g_GetGlyph(u8g, enc);

	/* strlen(s) == 1:       width = width(s[0]) */
	/* strlen(s) == 2:       width = - offx(s[0]) + deltax(s[0]) + offx(s[1]) + width(s[1]) */
	/* strlen(s) == 3:       width = - offx(s[0]) + deltax(s[0]) + deltax(s[1]) + offx(s[2]) + width(s[2]) */

	/* assume that the string has size 2 or more, than start with negative offset-x */
	/* for string with size 1, this will be nullified after the loop */
	w = -u8g->glyph_x;
	for (;;)
	{

		/* check and stop if the end of the string is reached */
		s++;
		if (get_char(s) == '\0')
			break;

		/* if there are still more characters, add the delta to the next glyph */
		w += u8g->glyph_dx;

		/* store the encoding in a local variable, used also after the for(;;) loop */
		enc = get_char(s);

		/* load the next glyph information */
		u8g_GetGlyph(u8g, enc);
	}

	/* finally calculate the width of the last char */
	/* here is another exception, if the last char is a black, use the dx value instead */
	if (enc != ' ')
	{
		/* if g was not updated in the for loop (strlen() == 1), then the initial offset x gets removed */
		w += u8g->glyph_width;
		w += u8g->glyph_x;
	}
	else
	{
		w += u8g->glyph_dx;
	}


	return w;
}

char u8g_font_get_char(const void *s)
{
	return *(const char *)(s);
}

u8g_uint_t u8g_font_calc_vref_top(u8g_t *u8g)
{
	u8g_uint_t tmp;
	/* reference pos is one pixel above the upper edge of the reference glyph */

	/*
	y += (u8g_uint_t)(u8g_int_t)(u8g->font_ref_ascent);
	y++;
	*/
	tmp = (u8g_uint_t)(u8g_int_t)(u8g->font_ref_ascent);
	tmp++;
	return tmp;
}

class U8GLIB_SSD1306_128X64_2X
{
public:
	u8g_t u;

public:
	U8GLIB_SSD1306_128X64_2X(uint8_t options = 0)
	{
		//u.font_ref_ascent = u8g_font_GetFontAscent(u.font);
		//u.font_ref_descent = u8g_font_GetFontDescent(u.font);

		u.font_calc_vref = u8g_font_calc_vref_font;
	}

	void drawStr(byte x, byte y, const char* s)
	{
		int8_t g_dx = 0;
		while (*s != '\0')
		{
			g_dx = u8g_DrawGlyph(&u, x, y, *s);
			x += g_dx;
			s++;
		}

	}

	void setFont(const u8g_fntpgm_uint8_t *font)
	{
		u.font = font;
	}

	void drawPixel(u8g_uint_t x, u8g_uint_t y)
	{
		setPixel(&u, x, y);
	}

	void firstPage(void)
	{
		memcpy(u.screenBuffOld, u.screenBuffNew, 128 * 64);
		memset(u.screenBuffNew, 0, 128 * 64);
	}

	uint8_t nextPage(void)
	{
		for (size_t x = 0; x < 128; x++)
		{
			for (size_t y = 0; y < 64; y++)
			{
				if (u.screenBuffOld[x][y] == 1 && u.screenBuffNew[x][y] == 0)
				{
					clearPixel(x, y);
				}
				if (u.screenBuffOld[x][y] == 0 && u.screenBuffNew[x][y] == 1)
				{
					printPixel(x, y);
				}
			}
		}

		return 0;
	}

	void drawHLine(u8g_uint_t x, u8g_uint_t y, u8g_uint_t w)
	{
		if (u8g_IsBBXIntersection(&u, x, y, w, 1) == 0)
			return;
		u8g_draw_hline(&u, x, y, w);
	}

	void drawVLine(u8g_uint_t x, u8g_uint_t y, u8g_uint_t h)
	{
		if (u8g_IsBBXIntersection(&u, x, y, 1, h) == 0)
			return;
		u8g_draw_vline(&u, x, y, h);
	}

	void drawLine(u8g_uint_t x1, u8g_uint_t y1, u8g_uint_t x2, u8g_uint_t y2){
		u8g_uint_t tmp;
		u8g_uint_t x, y;
		u8g_uint_t dx, dy;
		u8g_int_t err;
		u8g_int_t ystep;

		uint8_t swapxy = 0;

		/* no BBX intersection check at the moment, should be added... */

		if (x1 > x2) dx = x1 - x2; else dx = x2 - x1;
		if (y1 > y2) dy = y1 - y2; else dy = y2 - y1;

		if (dy > dx)
		{
			swapxy = 1;
			tmp = dx; dx = dy; dy = tmp;
			tmp = x1; x1 = y1; y1 = tmp;
			tmp = x2; x2 = y2; y2 = tmp;
		}
		if (x1 > x2)
		{
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
		}
		err = dx >> 1;
		if (y2 > y1) ystep = 1; else ystep = -1;
		y = y1;
		for (x = x1; x <= x2; x++)
		{
			if (swapxy == 0)
				setPixel(&u, x, y);
			else
				setPixel(&u, y, x);
			err -= (uint8_t)dy;
			if (err < 0)
			{
				y += (u8g_uint_t)ystep;
				err += (u8g_uint_t)dx;
			}
		}
	}

	void drawBitmapP(u8g_uint_t x, u8g_uint_t y, u8g_uint_t cnt, u8g_uint_t h, const u8g_pgm_uint8_t *bitmap)
	{
		if (u8g_IsBBXIntersection(&u, x, y, cnt * 8, h) == 0)
			return;
		while (h > 0)
		{
			u8g_DrawHBitmapP(&u, x, y, cnt, bitmap);
			bitmap += cnt;
			y++;
			h--;
		}
	}

	void drawCircle(u8g_uint_t x0, u8g_uint_t y0, u8g_uint_t rad, uint8_t opt = U8G_DRAW_ALL)
	{
		/* check for bounding box */
		{
			u8g_uint_t radp, radp2;

			radp = rad;
			radp++;
			radp2 = radp;
			radp2 *= 2;

			if (u8g_IsBBXIntersection(&u, x0 - radp, y0 - radp, radp2, radp2) == 0)
				return;
		}

		/* draw circle */
		u8g_draw_circle(&u, x0, y0, rad, opt);
	}

	void drawFrame(u8g_uint_t x, u8g_uint_t y, u8g_uint_t w, u8g_uint_t h)
	{
		u8g_uint_t xtmp = x;

		if (u8g_IsBBXIntersection(&u, x, y, w, h) == 0)
			return;


		u8g_draw_hline(&u, x, y, w);
		u8g_draw_vline(&u, x, y, h);
		x += w;
		x--;
		u8g_draw_vline(&u, x, y, h);
		y += h;
		y--;
		u8g_draw_hline(&u, xtmp, y, w);
	}

	u8g_uint_t getStrPixelWidth(const char *s)
	{
		return u8g_font_calc_str_pixel_width(&u, s, u8g_font_get_char);
	}

	u8g_uint_t getStrWidth(const char *s){
		u8g_uint_t  w;
		uint8_t encoding;

		/* reset the total width to zero, this will be expanded during calculation */
		w = 0;

		for (;;)
		{
			encoding = *s;
			if (encoding == 0)
				break;

			/* load glyph information */
			u8g_GetGlyph(&u, encoding);
			w += u.glyph_dx;

			/* goto next char */
			s++;
		}

		return w;
	}

	int8_t getFontAscent(void) { return u8g_font_GetFontAscent(u.font); }
	int8_t getFontDescent(void) { return u8g_font_GetFontDescent(u.font); }
	void setFontPosTop(void){ u.font_calc_vref = u8g_font_calc_vref_top; }
	void setFontRefHeightExtendedText(void){
		u.font_ref_ascent = u8g_font_GetCapitalAHeight(u.font);
		u.font_ref_descent = u8g_font_GetLowerGDescent(u.font);
	}

	void setColorIndex(uint8_t color_index){}
};

unsigned long millis(void)
{
	return (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

}

void delay(unsigned long t)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(t));
}

void pinMode(uint8_t, uint8_t)
{

}
void digitalWrite(uint8_t, uint8_t)
{

}
int digitalRead(uint8_t)
{
	return 1;
}

#include "Watch.ino"

EEPROMClass EEPROM;

void GenerateReducedFontsCFile()
{
	uint8_t requested_encoding[256] = { 0 };
	ofstream fout("helvBr_gen.c");
	uint16_t r_size = 0;
	u8g_t u;

	// 08
	u.font = helvB08r;
	SetRequestEncoding(true, true, true, requested_encoding);
	requested_encoding[46] = 1; // 46		.	 	Period, dot or full stop
	requested_encoding[176] = 1; // 176		°		Degree sign
	r_size = u8g_CreateReduced(&u, reduced, requested_encoding);
	generateCFile("helvB08r", reduced, r_size, fout);

	// 10
	u.font = helvB10r;
	SetRequestEncoding(true, true, true, requested_encoding);
	requested_encoding[46] = 1; // 46		.	 	Period, dot or full stop
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
}

void main()
{
	GenerateReducedFontsCFile();
	setup();
	for (;;)
	{
		loop();
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