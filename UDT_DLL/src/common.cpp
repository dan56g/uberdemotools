#include "common.hpp"
#include "context.hpp"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>


s32 Q_isprint( s32 c )
{
	if ( c >= 0x20 && c <= 0x7E )
		return ( 1 );
	return ( 0 );
}

s32 Q_islower( s32 c )
{
	if (c >= 'a' && c <= 'z')
		return ( 1 );
	return ( 0 );
}

s32 Q_isupper( s32 c )
{
	if (c >= 'A' && c <= 'Z')
		return ( 1 );
	return ( 0 );
}

s32 Q_isalpha( s32 c )
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return ( 1 );
	return ( 0 );
}

const char* Q_strrchr( const char* string, s32 c )
{
	char cc = (s8)c;
	const char *s;
	const char *sp = (const char*)0;

	s = string;

	while (*s)
	{
		if (*s == cc)
			sp = s;
		s++;
	}
	if (cc == 0)
		sp = s;

	return sp;
}


// safe strncpy that ensures a trailing zero

void Q_strncpyz( char *dest, const char *src, s32 destsize )
{
	if(dest == NULL || src == NULL || destsize < 1)
	{
		return;
	}

	strncpy( dest, src, destsize-1 );
	dest[destsize-1] = 0;
}

s32 Q_stricmpn( const char *s1, const char *s2, s32 n )
{
	s32		c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);

	return 0;		// strings are equal
}

s32 Q_strncmp( const char *s1, const char *s2, s32 n )
{
	s32		c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		if (c1 != c2) {
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);

	return 0;		// strings are equal
}

s32 Q_stricmp( const char *s1, const char *s2 )
{
	return (s1 && s2) ? Q_stricmpn (s1, s2, 99999) : -1;
}

// never goes past bounds or leaves without a terminating 0
void Q_strcat( const udtContext* context, char *dest, s32 size, const char *src )
{
	s32		l1;

	l1 = (s32)strlen( dest );
	if ( l1 >= size ) {
		context->LogErrorAndCrash("Q_strcat: already overflowed");
	}
	Q_strncpyz( dest + l1, src, size - l1 );
}
