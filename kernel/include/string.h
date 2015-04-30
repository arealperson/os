#ifndef _STRING_H
#define _STRING_H

#include <system.h>

uint32_t strlen(uint8_t*);
uint8_t *strdup(uint8_t*);
uint32_t strcmp(uint8_t*, uint8_t*);
uint8_t *strcat(uint8_t*, uint8_t*);
uint8_t *itoa(uint32_t);
void *memset(void*, uint8_t, uint32_t);

#endif
