#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio_ext.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <stdlib.h>

#include <cstring_a.h>

char* cstring_init(const char *str)
{
	if(!str)
		return (char*)0;

	int len = strlen(str);
	char* c = (char*)malloc(len+1);

	for(int i = 0; i < len; i++)
		c[i] = str[i];

	c[len] = '\0';

	return c;
}

bool cstring_get_delim(char* dst, int len, char del)
{
	if(!dst || len < 2)
		return false;

	char chr = '\0';
	__fpurge(stdin);
	int i;
	for(i = 0; i < len-1; i++)
	{
		if(fread(&chr, sizeof(char), 1, stdin))
		{
			if(chr == del)
				break;
			dst[i] = chr;
		}
		else
			return false;
	}

	dst[i] = '\0';
	return true;
}

char* cstring_get(int len)
{
	if(len < 2)
		return (char*)0;

	char chr = '\0';
	char buff[len];

	__fpurge(stdin);
	int i;
	for(i = 0; i < len-1; i++)
	{
		if(fread(&chr, sizeof(char), 1, stdin))
		{
			if(chr == '\n')
				break;
			buff[i] = chr;
		}
		else
			return (char*)0;
	}

	if(i == 0)
		return (char*)0;

	buff[i] = '\0';
	return cstring_init(buff);
}

char char_get()
{
	char chr = '\0', ret = '\0';
	bool got_it = false;

	__fpurge(stdin);
	while(true)
	{
		if(fread(&chr, sizeof(char), 1, stdin))
		{
			if(chr == '\n')
				break;

			if(!got_it)
			{
				ret = chr;
				got_it = true;
			}
		}
	}

return ret;
}

void cstring_destroy(char** c)
{
	if(*c)
	{
		free(*c);
		*c = (char*)0;
	}
}

char* cstring_substr(const char* text, int start, int len)
{
	if(!text || start < 0)
		return (char*)0;

	int text_len = strlen(text);
	int end  = start +len;
	if(end > text_len)
		return (char*)0;

	char* c  = (char*)malloc(len+1);
	for(int i = start, j = 0; j < len; i++, j++)
		c[j] = text[i];

	c[len] = '\0';

	return c;
}

int cstring_find_first_of(const char* text, const char* delims, int start)
{
	if(!text || !delims)
		return -1;

	int del_len  = strlen(delims);

	for(int i = start;; i++)
	{
		if(text[i] == '\0')
			return INT_MAX;

		for(int j = 0; j < del_len; j++)
			if(text[i] == delims[j])
				return i;
	}
}

int cstring_find_first_not_of(const char* text, const char* delims, int start)
{
	if(!text || !delims)
		return -1;

	int del_len  = strlen(delims);

	for(int i = start;; i++)
	{
		if(text[i] == '\0')
			return INT_MAX;

		for(int j = 0; j < del_len; j++)
		{
			if(text[i] == delims[j])
				break;
			else if(j == del_len-1)
				return i;
		}
	}
}

char* cstring_format(const char* templat, ...)
{
	va_list ap;
	va_start(ap, templat);

	char* ret = (char*)0;

	vasprintf(&ret, templat, ap);
	va_end(ap);

	return ret;
}

void cstring_lower(char* c)
{
	if(!c)
		return;

	int len = strlen(c);
	for(int i = 0; i < len; i++)
		c[i] = isupper(c[i]) ? tolower(c[i]) : c[i];
}

void cstring_upper(char* c)
{
	if(!c)
		return;

	int len = strlen(c);
	for(int i = 0; i < len; i++)
		c[i] = islower(c[i]) ? toupper(c[i]) : c[i];
}

void orange()
{
	printf("\033[0;31m");
}
void green()
{
	printf("\033[0;32m");
}
void yellow()
{
	printf("\033[0;33m");
}
void blue()
{
	printf("\033[0;34m");
}
void magenta()
{
	printf("\033[0;35m");	
}
void cyan()
{
	printf("\033[0;36m");
}
void colour_reset()
{
	printf("\033[0;39m");
}


void orange_bg()
{
	printf("\033[0;41m");
}
void green_bg()
{
	printf("\033[0;42m");
}
void yellow_bg()
{
	printf("\033[0;43m");
}
void blue_bg()
{
	printf("\033[0;44m");
}
void magenta_bg()
{
	printf("\033[0;45m");	
}
void cyan_bg()
{
	printf("\033[0;46m");
}
void bg_reset()
{
	printf("\033[0;49m");
}

float frandom(int seed)
{
	srand48(time(0)+seed);
	return (float)drand48();
}

long lrandom(int seed)
{
	srand48(time(0)+seed);
	return (long)lrand48();
}

double drandom(int seed)
{
	srand48(time(0)+seed);
	long int strong =  lrand48();
	return (double)strong/10000000*0.0210054;
}
