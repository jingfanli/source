/*
 * Module:	String operation library
 * Author:	Lvjianfeng
 * Date:	2011.9
 */


#include "lib_string.h"


//Constant definition


//Type definition


//Private variable definition


//Private function declaration


//Public function definition

uint LibString_StringToNumber
(
	const uint8 *u8p_String,
	uint ui_Length,
	uint16 *u16p_Number
)
{
	const uint8 *u8p_Character;
	uint16 u16_Number;
	uint16 u16_Base;


	if (ui_Length <= 0)
	{
		return FUNCTION_FAIL;
	}
	
	u8p_Character = &u8p_String[ui_Length - 1];
	u16_Number = 0;
	u16_Base = 1;

	while (ui_Length > 0)
	{
		if ((*u8p_Character >= '0') && (*u8p_Character <= '9'))
		{
			u16_Number += ((*u8p_Character) - '0') * u16_Base;
			u16_Base *= 10;
			ui_Length--;
			u8p_Character--;
		}
		else
		{
			return FUNCTION_FAIL;
		}
	}

	*u16p_Number = u16_Number;

	return FUNCTION_OK;
}


uint LibString_NumberToString
(
	uint8 *u8p_String,
	uint *uip_Length,
	uint16 u16_Number
)
{
	const uint8 u8_Decimal[] = "0123456789";
	uint8 *u8p_Character;
	uint ui_Length;

	
	ui_Length = *uip_Length;

	if (ui_Length <= 0)
	{
		return FUNCTION_FAIL;
	}

	u8p_Character = &u8p_String[ui_Length - 1];

	if (u16_Number > 0)
	{
		while ((ui_Length > 0) && (u16_Number > 0))
		{
			*u8p_Character = u8_Decimal[u16_Number % 10];
			u16_Number /= 10;
			ui_Length--;
			u8p_Character--;
		}

		if (ui_Length > 0)
		{
			ui_Length = *uip_Length - ui_Length;
			*uip_Length = ui_Length;
			u8p_Character++;

			while (ui_Length > 0)
			{
				*u8p_String++ = *u8p_Character++;
				ui_Length--;
			}
		}
		else
		{
			if (u16_Number > 0)
			{
				return FUNCTION_FAIL;
			}
		}
	}
	else
	{
		*uip_Length = 1;
		u8p_String[0] = u8_Decimal[0];
	}

	return FUNCTION_OK;
}
