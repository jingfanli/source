/*
 * Module:	String operation library
 * Author:	Lvjianfeng
 * Date:	2011.9
 */

#ifndef _LIB_STRING_H_
#define _LIB_STRING_H_


#include "global.h"


//Constant definition


//Type definition


//Function declaration

uint LibString_StringToNumber
(
	const uint8 *u8p_String,
	uint ui_Length,
	uint16 *u16p_Number
);
uint LibString_NumberToString
(
	uint8 *u8p_String,
	uint *uip_Length,
	uint16 u16_Number
);

#endif
