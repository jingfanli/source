/*
 * Module:	Bluetooth
 * Author:	Lvjianfeng
 * Date:	2012.8
 */


#include "drv.h"

#include "bluetooth.h"


//Constant definition

#define GPIO_CHANNEL_EN_BT_N			(DRV_GPIO_PORTH | DRV_GPIO_PIN7)
#define GPIO_CHANNEL_INT				(DRV_GPIO_PORTF | DRV_GPIO_PIN1)
#define GPIO_CHANNEL_LED				(DRV_GPIO_PORTA| DRV_GPIO_PIN7)
#define CONNECTION_TIME					100


//Type definition

typedef enum
{
	BLUETOOTH_FLAG_ENABLE = 0,
	BLUETOOTH_FLAG_CONNECTION,
	BLUETOOTH_COUNT_FLAG
} bluetooth_flag;


//Private variable definition

static uint m_ui_Flag = {0};
static uint16 m_u16_ConnectionTimer = {0};


//Private function declaration


//Public function definition

uint Bluetooth_Initialize(void)
{
	uint ui_Value;


	ui_Value = DRV_GPIO_MODE_OUTPUT;
	DrvGPIO_SetConfig(GPIO_CHANNEL_EN_BT_N, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);

	if (Bluetooth_GetConfig(BLUETOOTH_PARAM_SIGNAL_PRESENT, (uint8 *)&ui_Value,
		(uint *)0) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (ui_Value == 0)
	{
		ui_Value = DRV_GPIO_MODE_INPUT;
	}
	else
	{
		ui_Value = DRV_GPIO_MODE_OUTPUT;
	}

	DrvGPIO_SetConfig(GPIO_CHANNEL_INT, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_LED, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	ui_Value = 1;
	DrvGPIO_SetConfig(GPIO_CHANNEL_EN_BT_N, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
	ui_Value = 0;
	DrvGPIO_SetConfig(GPIO_CHANNEL_INT, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_LED, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);

	DrvGPIO_Set(GPIO_CHANNEL_EN_BT_N);

	return FUNCTION_OK;
}


uint Bluetooth_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	switch (ui_Parameter)
	{
		case BLUETOOTH_PARAM_SWITCH:

			if (*((uint *)u8p_Value) != 0)
			{
				DrvGPIO_Clear(GPIO_CHANNEL_EN_BT_N);
				REG_SET_BIT(m_ui_Flag, BLUETOOTH_FLAG_ENABLE);
			}
			else
			{
				DrvGPIO_Set(GPIO_CHANNEL_EN_BT_N);
				REG_CLEAR_BIT(m_ui_Flag, BLUETOOTH_FLAG_ENABLE);
			}

			DrvUART_SetConfig(DRV_UART_DEVICE_ID_2, DRV_UART_PARAM_SWITCH, 
				u8p_Value, ui_Length);
			break;

		case BLUETOOTH_PARAM_CALLBACK:
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint Bluetooth_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case BLUETOOTH_PARAM_SWITCH:

			if (REG_GET_BIT(m_ui_Flag, BLUETOOTH_FLAG_ENABLE) != 0)
			{
				*((uint *)u8p_Value) = 1;
			}
			else
			{
				*((uint *)u8p_Value) = 0;
			}

			break;

		case BLUETOOTH_PARAM_SIGNAL_PRESENT:
			DrvUART_GetConfig(DRV_UART_DEVICE_ID_2, 
				DRV_UART_PARAM_SIGNAL_PRESENT, u8p_Value, uip_Length);
			break;

		case BLUETOOTH_PARAM_CONNECTION:
			
			if (REG_GET_BIT(m_ui_Flag, BLUETOOTH_FLAG_CONNECTION) != 0)
			{
				*((uint *)u8p_Value) = 1;
			}
			else
			{
				*((uint *)u8p_Value) = 0;
			}

			break;

		default:
			return FUNCTION_FAIL;
	}


	return FUNCTION_OK;
}


void Bluetooth_Tick
(
	uint16 u16_TickTime
)
{
	if (REG_GET_BIT(m_ui_Flag, BLUETOOTH_FLAG_ENABLE) == 0)
	{
		m_u16_ConnectionTimer = 0;
		REG_CLEAR_BIT(m_ui_Flag, BLUETOOTH_FLAG_CONNECTION);
	}
	else
	{
		if (DrvGPIO_Read(GPIO_CHANNEL_LED) == 0)
		{
			m_u16_ConnectionTimer = 0;
			REG_CLEAR_BIT(m_ui_Flag, BLUETOOTH_FLAG_CONNECTION);
		}
		else
		{
			if (m_u16_ConnectionTimer < CONNECTION_TIME)
			{
				m_u16_ConnectionTimer += u16_TickTime;
			}

			if (m_u16_ConnectionTimer >= CONNECTION_TIME)
			{
				REG_SET_BIT(m_ui_Flag, BLUETOOTH_FLAG_CONNECTION);
			}
		}
	}
}


#if BLUETOOTH_TEST_ENABLE == 1

void Bluetooth_Test(void)
{
}

#endif


//Private function definition
