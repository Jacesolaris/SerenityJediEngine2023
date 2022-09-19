#include "q_string.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "q_color.h"

int Q_isprint(int c)
{
	if (c >= 0x20 && c <= 0x7E)
		return 1;
	return 0;
}

int Q_isprintext(int c)
{
	if (c >= 0x20 && c <= 0x7E)
		return 1;
	if (c >= 0x80 && c <= 0xFE)
		return 1;
	return 0;
}

int Q_isgraph(int c)
{
	if (c >= 0x21 && c <= 0x7E)
		return 1;
	if (c >= 0x80 && c <= 0xFE)
		return 1;
	return 0;
}

int Q_islower(int c)
{
	if (c >= 'a' && c <= 'z')
		return 1;
	return 0;
}

int Q_isupper(int c)
{
	if (c >= 'A' && c <= 'Z')
		return 1;
	return 0;
}

int Q_isalpha(int c)
{
	if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z')
		return 1;
	return 0;
}

qboolean Q_isanumber(const char* s)
{
	char* p;

	if (*s == '\0')
		return qfalse;

	const double ret = strtod(s, &p);

	if (ret == HUGE_VAL || errno == ERANGE)
		return qfalse;

	return *p == '\0';
}

qboolean Q_isintegral(float f)
{
	return (int)f == f;
}

char* Q_strrchr(const char* string, int c)
{
	const char cc = c;
	char* sp = 0;

	char* s = (char*)string;

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

/*
=============
Q_strncpyz

Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz(char* dest, const char* src, int destsize)
{
	assert(src);
	assert(dest);
	assert(destsize);

	strncpy(dest, src, destsize - 1);
	dest[destsize - 1] = 0;
}

int Q_stricmpn(const char* s1, const char* s2, int n)
{
	int c1;

	if (s1 == NULL)
	{
		if (s2 == NULL)
			return 0;
		return -1;
	}
	if (s2 == NULL)
		return 1;

	do
	{
		c1 = *s1++;
		int c2 = *s2++;

		if (!n--)
		{
			return 0; // strings are equal until end point
		}

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
			{
				c1 -= 'a' - 'A';
			}
			if (c2 >= 'a' && c2 <= 'z')
			{
				c2 -= 'a' - 'A';
			}
			if (c1 != c2)
			{
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);

	return 0; // strings are equal
}

int Q_stricmp(const char* s1, const char* s2)
{
	return s1 && s2 ? Q_stricmpn(s1, s2, 99999) : -1;
}

int Q_strncmp(const char* s1, const char* s2, int n)
{
	int c1;

	do
	{
		c1 = *s1++;
		const int c2 = *s2++;

		if (!n--)
		{
			return 0; // strings are equal until end point
		}

		if (c1 != c2)
		{
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);

	return 0; // strings are equal
}

char* Q_strlwr(char* s1)
{
	char* s = s1;
	while (*s)
	{
		*s = tolower(*s);
		s++;
	}
	return s1;
}

char* Q_strupr(char* s1)
{
	char* s = s1;
	while (*s)
	{
		*s = toupper(*s);
		s++;
	}
	return s1;
}

// never goes past bounds or leaves without a terminating 0
void Q_strcat(char* dest, int size, const char* src)
{
	const int l1 = strlen(dest);
	if (l1 >= size)
	{
		//Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
		return;
	}
	if (strlen(src) + 1 > (size_t)(size - l1))
	{
		//do the error here instead of in Q_strncpyz to get a meaningful msg
		//Com_Error(ERR_FATAL,"Q_strcat: cannot append \"%s\" to \"%s\"", src, dest);
		return;
	}
	Q_strncpyz(dest + l1, src, size - l1);
}

/*
* Find the first occurrence of find in s.
*/
const char* Q_stristr(const char* s, const char* find)
{
	char c, sc;

	if ((c = *find++) != 0)
	{
		if (c >= 'a' && c <= 'z')
		{
			c -= 'a' - 'A';
		}
		const size_t len = strlen(find);
		do
		{
			do
			{
				if ((sc = *s++) == 0)
					return NULL;
				if (sc >= 'a' && sc <= 'z')
				{
					sc -= 'a' - 'A';
				}
			} while (sc != c);
		} while (Q_stricmpn(s, find, len) != 0);
		s--;
	}
	return s;
}

int Q_PrintStrlen(const char* string)
{
	if (!string)
	{
		return 0;
	}

	int len = 0;
	const char* p = string;
	while (*p)
	{
		if (Q_IsColorString(p))
		{
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}

char* Q_CleanStr(char* string)
{
	int c;

	char* s = string;
	char* d = string;

	while ((c = *s) != 0)
	{
		if (Q_IsColorString(s))
		{
			s++;
		}
		else if (c >= 0x20 && c <= 0x7E)
		{
			*d++ = c;
		}
		s++;
	}
	if (d)
	{
		*d = '\0';
	}

	return string;
}

/*
==================
Q_StripColor

Strips coloured strings in-place using multiple passes: "fgs^^56fds" -> "fgs^6fds" -> "fgsfds"

This function modifies INPUT (is mutable)

(Also strips ^8 and ^9)
==================
*/
void Q_StripColor(char* text)
{
	qboolean doPass = qtrue;
	char* write;

	while (doPass)
	{
		doPass = qfalse;
		char* read = write = text;
		while (*read)
		{
			if (Q_IsColorStringExt(read))
			{
				doPass = qtrue;
				read += 2;
			}
			else
			{
				// Avoid writing the same data over itself
				if (write != read)
				{
					*write = *read;
				}
				write++;
				read++;
			}
		}
		if (write < read)
		{
			// Add trailing NUL byte if string has shortened
			*write = '\0';
		}
	}
}

/*
Q_strstrip

Description:	Replace strip[x] in string with repl[x] or remove characters entirely
Mutates:		string
Return:			--

Examples:		Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", "123" );	// "Bo1b is h2airy33"
Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", "12" );	// "Bo1b is h2airy"
Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", NULL );	// "Bob is hairy"
*/

void Q_strstrip(char* string, const char* strip, const char* repl)
{
	char* out = string, * p = string, c;
	const int replaceLen = repl ? strlen(repl) : 0;

	while ((c = *p++) != '\0')
	{
		qboolean recordChar = qtrue;
		for (const char* s = strip; *s; s++)
		{
			const int offset = s - strip;
			if (c == *s)
			{
				if (!repl || offset >= replaceLen)
					recordChar = qfalse;
				else
					c = repl[offset];
				break;
			}
		}
		if (recordChar)
			*out++ = c;
	}
	*out = '\0';
}

/*
Q_strchrs

Description:	Find any characters in a string. Think of it as a shorthand strchr loop.
Mutates:		--
Return:			first instance of any characters found
otherwise NULL
*/

const char* Q_strchrs(const char* string, const char* search)
{
	const char* p = string;

	while (*p != '\0')
	{
		for (const char* s = search; *s; s++)
		{
			if (*p == *s)
				return p;
		}
		p++;
	}

	return NULL;
}

#if defined(_MSC_VER)
/*
=============
Q_vsnprintf

Special wrapper function for Microsoft's broken _vsnprintf() function.
MinGW comes with its own snprintf() which is not broken.
=============
*/

int Q_vsnprintf(char* str, const size_t size, const char* format, const va_list ap)
{
	const int retval = _vsnprintf(str, size, format, ap);

	if (retval < 0 || retval == size)
	{
		// Microsoft doesn't adhere to the C99 standard of vsnprintf,
		// which states that the return value must be the number of
		// bytes written if the output string had sufficient length.
		//
		// Obviously we cannot determine that value from Microsoft's
		// implementation, so we have no choice but to return size.

		str[size - 1] = '\0';
		return size;
	}

	return retval;
}
#endif