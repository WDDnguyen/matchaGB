#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

#define TEST_BIT(value, position) ((value) & (1 << (position)))
#define SET_BIT(value, position) ((value) | (1 << (position)))
#define CLEAR_BIT(value, position) ((value) & (~(1 << position)))	

typedef int bool;
typedef unsigned char byte;
typedef char signed_byte;
typedef unsigned short word;
typedef short signed_word;

#endif

