#ifndef CSTRING_A_H
#define CSTRING_A_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"{
#endif

char* cstring_init(const char* str);

char* cstring_get(int len);

bool cstring_get_delim(char* dst, int len, char delim);

char char_get();

void cstring_destroy(char** c);

#define cstringlocal __attribute__((cleanup(cstring_destroy)))

char* cstring_substr(const char* text, int start, int len);

int cstring_find_first_of(const char* text, const char* delims, int start);

int cstring_find_first_not_of(const char* text, const char* delims, int start);

char* cstring_format(const char* templat, ...);

void cstring_lower(char* c);

void cstring_upper(char* c);


void orange();

void green();

void yellow();

void blue();

void magenta();

void cyan();

void colour_reset();


void orange_bg();

void green_bg();

void yellow_bg();

void blue_bg();

void magenta_bg();

void cyan_bg();

void bg_reset();


float frandom(int seed);

long lrandom(int seed);

double drandom(int seed);

#ifdef __cplusplus
}
#endif

#endif //CSTRING_A_H
