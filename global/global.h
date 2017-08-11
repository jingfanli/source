/*
 * Module:	Global include file
 * Author:	Lvjianfeng
 * Date:	2011.8
 */

#ifndef _GLOBAL_H_
#define _GLOBAL_H_


#include "devcomm_config.h"
#include "devfs_config.h"
#include "devos_config.h"


//Constant define

#ifndef NULL
	#define NULL			0
#endif

#define REG_MASK_1_BIT		0x01
#define REG_MASK_2_BIT		0x03
#define REG_MASK_3_BIT		0x07
#define REG_MASK_4_BIT		0x0F
#define REG_MASK_5_BIT		0x1F
#define REG_MASK_6_BIT		0x3F
#define REG_MASK_7_BIT		0x7F
#define REG_MASK_8_BIT		0xFF

#define REG_WRITE_FIELD(reg, field, mask, value)	{(reg) = ((reg) & (~((mask) << (field)))) | \
													((value) << (field));}
#define REG_READ_FIELD(reg, field, mask)			(((reg) >> (field)) & (mask))
#define REG_SET_BIT(reg, field)						{(reg) |= (1 << (field));}
#define REG_CLEAR_BIT(reg, field)					{(reg) &= ~(1 << (field));}
#define REG_REVERSE_BIT(reg, field)					{(reg) ^= (1 << (field));}
#define REG_GET_BIT(reg, field)						((reg) & (1 << (field)))


//Type definition

typedef char				int8;
typedef unsigned char		uint8;
typedef signed char			sint8;
typedef short int			int16;
typedef unsigned short int	uint16;
typedef signed short int	sint16;
typedef long				int32;
typedef unsigned long		uint32;
typedef signed long			sint32;
typedef long long			int64;
typedef unsigned long long	uint64;
typedef signed long	long	sint64;
typedef float				float32;
typedef double				float64;

typedef uint8				uint;
typedef sint8				sint;

typedef enum 
{
	FUNCTION_OK = 1,
	FUNCTION_FAIL = 0
} function_return;

#endif
