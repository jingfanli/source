/*
 * Module:	FLASH driver
 * Author:	Lvjianfeng
 * Date:	2012.9
 */

#ifndef _DRV_FLASH_H_
#define _DRV_FLASH_H_


#include "global.h"

#include "stm8l15x.h"


//Constant define

#define DRV_FLASH_TEST_ENABLE			0

#define DRV_FLASH_LOCK_ENABLE			1
#define DRV_FLASH_BLOCK_SIZE			128
#define DRV_FLASH_PAGE_SIZE				256
#define DRV_FLASH_ERASED_DATA			0
#define DRV_FLASH_ADDRESS_DATA_BEGIN	0x1000
#define DRV_FLASH_ADDRESS_DATA_END		0x13FF
#define DRV_FLASH_ADDRESS_OPTION_BEGIN	0x4800
#define DRV_FLASH_ADDRESS_OPTION_END	0x48FF
#define DRV_FLASH_ADDRESS_PROGRAM_BEGIN	0x8000
#define DRV_FLASH_ADDRESS_PROGRAM_USER	0x15000
#define DRV_FLASH_ADDRESS_PROGRAM_END	0x17FFF


//Type definition

typedef enum
{
	DRV_FLASH_PARAM_RESET = 0,
	DRV_FLASH_COUNT_PARAM
} drv_flash_param;


//Function declaration

uint DrvFLASH_Initialize(void);
uint DrvFLASH_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint DrvFLASH_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
uint DrvFLASH_Write
(
	uint32 u32_Address,
	const uint8 *u8p_Data,
	uint16 u16_Length
);
uint DrvFLASH_Read
(
	uint32 u32_Address,
	uint8 *u8p_Data,
	uint16 *u16p_Length
);
uint DrvFLASH_Erase
(
	uint32 u32_Address,
	uint16 u16_Length
);

#if DRV_FLASH_TEST_ENABLE == 1
void DrvFLASH_Test(void);
#endif

#endif
