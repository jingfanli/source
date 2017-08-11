/*
 * Module:	Device definition
 * Author:	Lvjianfeng
 * Date:	2015.11
 */

#ifndef _DEVICE_H_
#define _DEVICE_H_


#include "global.h"


//Constant define

#define DEVICE_ID_SIZE		6


//Type definition

typedef enum
{
	DEVICE_ENDIAN_LITTLE = 0,
	DEVICE_ENDIAN_BIG,
	DEVICE_COUNT_ENDIAN
} device_endian;

typedef enum
{
	DEVICE_TYPE_OTHER = 0,
	DEVICE_TYPE_METER,
	DEVICE_TYPE_PUMP,
	DEVICE_COUNT_TYPE
} device_type;

typedef struct
{
	uint8 u8_ID[DEVICE_ID_SIZE];
	uint8 u8_Endian;
	uint8 u8_Type;
	uint32 u32_Model;
	uint32 u32_Version;
	uint32 u32_Capacity;
} device;

#endif
