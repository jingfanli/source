/*
 * Module:	EEPROM driver
 * Author:	Lvjianfeng
 * Date:	2011.12
 */


#include "stm8l15x.h"

#include "drv.h"


//Constant definition

#define GPIO_CHANNEL_PRESENT_N		(DRV_GPIO_PORTF | DRV_GPIO_PIN5)

#define I2C_CLOCK					100000

#define EEPROM_DEVICE_ADDRESS		0xA0
#define EEPROM_PAGE_SIZE			8
#define EEPROM_PAGE_ADDRESS_MASK	0xF8
#define EEPROM_TIME_OUT				20000


//Type definition

typedef enum
{
	DRV_EEPROM_FLAG_READ_ONGOING = 0,
	DRV_EEPROM_FLAG_ENABLE,
	DRV_EEPROM_COUNT_FLAG
} drv_eeprom_flag;


//Private variable definition

static volatile uint m_ui_Flag = {0};
static uint16 m_u16_Timeout = {0};


//Private function declaration

void DrvEEPROM_Config(void);
uint DrvEEPROM_PollWriteDone(void);


//Public function definition

uint DrvEEPROM_Initialize(void)
{
	uint ui_Value;


	ui_Value = DRV_GPIO_MODE_OUTPUT;
	DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	ui_Value = 1;
	DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);

	CLK->PCKENR1 |= CLK_PCKENR1_I2C1;

	return FUNCTION_OK;
}


uint DrvEEPROM_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{

	return FUNCTION_OK;
}


uint DrvEEPROM_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case DRV_EEPROM_PARAM_SWITCH:

			if (REG_GET_BIT(m_ui_Flag, DRV_EEPROM_FLAG_ENABLE) != 0)
			{
				*((uint *)u8p_Value) = 1;
			}
			else
			{
				*((uint *)u8p_Value) = 0;
			}

			break;

		default:
			break;
	}

	return FUNCTION_OK;
}


uint DrvEEPROM_Write
(
	uint ui_Address,
	const uint8 *u8p_Data,
	uint ui_Length
)
{





	return FUNCTION_OK;
}
uint DrvEEPROM_Read
(
	uint ui_Address,
	uint8 *u8p_Data,
	uint *uip_Length
)
{
	return FUNCTION_OK;

}


void DrvEEPROM_Enable(void)
  {
  ;
 
}


  



void DrvEEPROM_Disable(void)
{
  
  ;
	/*uint ui_Value;


	ui_Value = DRV_GPIO_MODE_OUTPUT;
	DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);*/
}


void DrvEEPROM_Interrupt(void)
{
	
  ;
  /*!< Send STOP Condition */
	/*I2C1->CR2 |= I2C_CR2_STOP;

	REG_CLEAR_BIT(m_ui_Flag, DRV_EEPROM_FLAG_READ_ONGOING);*/
}




void DrvEEPROM_Config(void)
{
	
	;
}
