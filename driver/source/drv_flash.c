/*
 * Module:	FLASH driver
 * Author:	Lvjianfeng
 * Date:	2012.9
 */


#include "stm8l15x.h"

#include "drv.h"


//Constant definition


//Type definition


//Private variable definition


//Private function declaration

static void DrvFLASH_Lock
(
	uint32 u32_Address
);
static void DrvFLASH_Unlock
(
	uint32 u32_Address
);
static uint DrvFLASH_CheckAddress
(
	uint32 u32_AddressBegin,
	uint32 u32_AddressEnd
);
static uint DrvFLASH_Program
(
	uint32 u32_Address,
	const uint8 *u8p_Data,
	uint16 u16_Length
);
IN_RAM(static void DrvFLASH_WriteBlock
(
	uint32 u32_Address,
	const uint8 *u8p_Data
));
IN_RAM(static void DrvFLASH_EraseBlock
(
	uint32 u32_Address
));
IN_RAM(static void DrvFLASH_Reset(void));


//Public function definition

uint DrvFLASH_Initialize(void)
{
	FLASH->CR1 = 0;
	FLASH->CR2 = 0;

	return FUNCTION_OK;
}


uint DrvFLASH_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	//¡¡/*switch (ui_Parameter)
	/*{
		case DRV_FLASH_PARAM_RESET:
			Drv_DisableInterrupt();
			DrvFLASH_Unlock(DRV_FLASH_ADDRESS_PROGRAM_BEGIN);
			DrvFLASH_Reset();
			break;

		default:
			return FUNCTION_FAIL;*/
	//}

	return FUNCTION_OK;
}


uint DrvFLASH_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	return FUNCTION_OK;
}


uint DrvFLASH_Write
(
	uint32 u32_Address,
	const uint8 *u8p_Data,
	uint16 u16_Length
)
{
	return DrvFLASH_Program(u32_Address, u8p_Data, u16_Length);
}


uint DrvFLASH_Read
(
	uint32 u32_Address,
	uint8 *u8p_Data,
	uint16 *u16p_Length
)
{
	/*uint8 PointerAttr *u8p_Address;
	uint16 u16_Length;


	u8p_Address = (uint8 PointerAttr *)u32_Address;
	u16_Length = *u16p_Length;

	while (u16_Length > 0)
	{
		*u8p_Data++ = *u8p_Address++;
		u16_Length--;
	}

	return FUNCTION_OK;*/
}


uint DrvFLASH_Erase
(
	uint32 u32_Address,
	uint16 u16_Length
)
{
	return DrvFLASH_Program(u32_Address, (const uint8 *)0, u16_Length);
}


#if DRV_FLASH_TEST_ENABLE == 1

void DrvFLASH_Test(void)
{
}

#endif


//Private function definition

static void DrvFLASH_Lock
(
	uint32 u32_Address
)
{
/*#if DRV_FLASH_LOCK_ENABLE != 0
	if ((u32_Address >= DRV_FLASH_ADDRESS_PROGRAM_USER) &&
		(u32_Address <= DRV_FLASH_ADDRESS_PROGRAM_END))
	{
		FLASH->IAPSR &= ~FLASH_IAPSR_PUL;

		while ((FLASH->IAPSR & FLASH_IAPSR_PUL) != 0)
		{
			;
		}
	}
	else
	{
		FLASH->IAPSR &= ~FLASH_IAPSR_DUL;

		while ((FLASH->IAPSR & FLASH_IAPSR_DUL) != 0)
		{
			;
		}
	}
#endif*/
}


static void DrvFLASH_Unlock
(
	uint32 u32_Address
)
{
/*#if DRV_FLASH_LOCK_ENABLE != 0
	if ((u32_Address >= DRV_FLASH_ADDRESS_PROGRAM_USER) &&
		(u32_Address <= DRV_FLASH_ADDRESS_PROGRAM_END))
	{
		FLASH->PUKR = FLASH_DUKR_RESET_VALUE;
		FLASH->PUKR = FLASH_PUKR_RESET_VALUE;

		while ((FLASH->IAPSR & FLASH_IAPSR_PUL) == 0)
		{
			;
		}
	}
	else
	{
		FLASH->DUKR = FLASH_PUKR_RESET_VALUE;
		FLASH->DUKR = FLASH_DUKR_RESET_VALUE;

		while ((FLASH->IAPSR & FLASH_IAPSR_DUL) == 0)
		{
			;
		}
	}
#endif*/
}


static uint DrvFLASH_CheckAddress
(
	uint32 u32_AddressBegin,
	uint32 u32_AddressEnd
)
{
	/*if (u32_AddressBegin <= u32_AddressEnd)
	{
		if ((u32_AddressBegin >= DRV_FLASH_ADDRESS_DATA_BEGIN) &&
			(u32_AddressEnd <= DRV_FLASH_ADDRESS_DATA_END))
		{
			return FUNCTION_OK;
		}

		if ((u32_AddressBegin >= DRV_FLASH_ADDRESS_OPTION_BEGIN) &&
			(u32_AddressEnd <= DRV_FLASH_ADDRESS_OPTION_END))
		{
			return FUNCTION_OK;
		}

		if ((u32_AddressBegin >= DRV_FLASH_ADDRESS_PROGRAM_USER) &&
			(u32_AddressEnd <= DRV_FLASH_ADDRESS_PROGRAM_END))
		{
			return FUNCTION_OK;
		}
	}

	return FUNCTION_FAIL;*/
}


static uint DrvFLASH_Program
(
	uint32 u32_Address,
	const uint8 *u8p_Data,
	uint16 u16_Length
)
{
	/*uint i;


	if (DrvFLASH_CheckAddress(u32_Address, u32_Address + u16_Length - 1) !=
		FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	DrvFLASH_Unlock(u32_Address);

	if ((u32_Address >= DRV_FLASH_ADDRESS_OPTION_BEGIN) &&
		(u32_Address <= DRV_FLASH_ADDRESS_OPTION_END))
	{
		FLASH->CR2 |= FLASH_CR2_OPT;
	}

	//Use block programming if data size is larger than one block
	while (u16_Length >= DRV_FLASH_BLOCK_SIZE)
	{
		Drv_DisableInterrupt();

		if (u8p_Data != (const uint *)0)
		{
			DrvFLASH_WriteBlock(u32_Address, u8p_Data);	
		}
		else
		{
			DrvFLASH_EraseBlock(u32_Address);
		}

		Drv_EnableInterrupt();
		u32_Address += DRV_FLASH_BLOCK_SIZE;

		if (u8p_Data != (const uint *)0)
		{
			u8p_Data += DRV_FLASH_BLOCK_SIZE;
		}

		u16_Length -= DRV_FLASH_BLOCK_SIZE;
	}

	if (u8p_Data != (const uint *)0)
	{
		if ((FLASH->CR2 & FLASH_CR2_FPRG) != 0)
		{
			FLASH->CR2 &= ~FLASH_CR2_FPRG;
		}
	}
	else
	{
		if ((FLASH->CR2 & FLASH_CR2_ERASE) != 0)
		{
			FLASH->CR2 &= ~FLASH_CR2_ERASE;
		}
	}

	//Use byte programming for remaining data
	for (i = 0; i < u16_Length; i++)
	{

		if (u8p_Data != (const uint *)0)
		{
			((uint8 PointerAttr *)u32_Address)[i] = u8p_Data[i];
		}
		else
		{
			((uint8 PointerAttr *)u32_Address)[i] = DRV_FLASH_ERASED_DATA;
		}

		while ((FLASH->IAPSR & (FLASH_IAPSR_EOP | FLASH_IAPSR_WR_PG_DIS)) == 0)
		{
			;
		}
	}

	if ((u32_Address >= DRV_FLASH_ADDRESS_OPTION_BEGIN) &&
		(u32_Address <= DRV_FLASH_ADDRESS_OPTION_END))
	{
		FLASH->CR2 &= ~FLASH_CR2_OPT;
	}

	DrvFLASH_Lock(u32_Address);

	return FUNCTION_OK;*/
}


IN_RAM(static void DrvFLASH_WriteBlock
(
	uint32 u32_Address,
	const uint8 *u8p_Data
))
{
	uint16 i;


	FLASH->CR2 |= FLASH_CR2_FPRG;

	for (i = 0; i < DRV_FLASH_BLOCK_SIZE; i++)
	{
		((uint8 PointerAttr *)u32_Address)[i] = u8p_Data[i];
	}

	while ((FLASH->IAPSR & (FLASH_IAPSR_EOP | FLASH_IAPSR_WR_PG_DIS)) == 0)
	{
		;
	}
}


IN_RAM(static void DrvFLASH_EraseBlock
(
	uint32 u32_Address
))
{
	FLASH->CR2 |= FLASH_CR2_ERASE;
	*((uint32 PointerAttr *)u32_Address) = (uint32)DRV_FLASH_ERASED_DATA;

	while ((FLASH->IAPSR & (FLASH_IAPSR_EOP | FLASH_IAPSR_WR_PG_DIS)) == 0)
	{
		;
	}
}


IN_RAM(static void DrvFLASH_Reset(void))
{
	DrvFLASH_EraseBlock(DRV_FLASH_ADDRESS_PROGRAM_BEGIN);

	//Reset MCU
	WWDG->CR = WWDG_CR_WDGA; 

	while (1)
	{
		;
	}
}
