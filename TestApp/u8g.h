#ifndef _U8G_H_
#define _U8G_H_

// glyph u8glib
typedef signed char int8_t;

/** \ingroup avr_stdint
8-bit unsigned type. */

typedef unsigned char uint8_t;

/** \ingroup avr_stdint
16-bit signed type. */

typedef signed int int16_t;

/** \ingroup avr_stdint
16-bit unsigned type. */

typedef unsigned int uint16_t;

/** \ingroup avr_stdint
32-bit signed type. */

typedef signed long int int32_t;

/** \ingroup avr_stdint
32-bit unsigned type. */

typedef unsigned long int uint32_t;

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

/* helvreducedfont */

#endif