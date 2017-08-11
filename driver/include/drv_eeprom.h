/*
 * Module:	EEPROM driver
 * Author:	Lvjianfeng
 * Date:	2011.12
 */

#ifndef _DRV_EEPROM_H_
#define _DRV_EEPROM_H_


#include "global.h"


//Constant define

#define DRV_EEPROM_TEST_ENABLE		1


//Type definition

typedef enum
{
	DRV_EEPROM_PARAM_SWITCH = 0,
	DRV_EEPROM_PARAM_STATE
} drv_eeprom_param;


//Function declaration

uint DrvEEPROM_Initialize(void);
uint DrvEEPROM_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint DrvEEPROM_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
uint DrvEEPROM_Write
(
	uint ui_Address,
	const uint8 *u8p_Data,
	uint ui_Length
);
uint DrvEEPROM_Read
(
	uint ui_Address,
	uint8 *u8p_Data,
	uint *uip_Length
);
void DrvEEPROM_Enable(void);
void DrvEEPROM_Disable(void);
void DrvEEPROM_Interrupt(void);

#if DRV_EEPROM_TEST_ENABLE == 1
void DrvEEPROM_Test(void);
#endif

#endif
