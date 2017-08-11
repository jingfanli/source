/*
 * Module:	Button
 * Author:	Lvjianfeng
 * Date:	2012.8
 */


#include "drv.h"

#include "button.h"


//Constant definition


//Type definition


//Private variable definition

static uint m_ui_State[BUTTON_COUNT_ID] = {0};
static uint16 m_u16_PressTimer[BUTTON_COUNT_ID] = {0};
static uint16 m_u16_LongPressDelay[BUTTON_COUNT_ID] = {0};

static const uint m_ui_ButtonChannel[BUTTON_COUNT_ID] = 
{
	DRV_GPIO_PORTH | DRV_GPIO_PIN4,
	DRV_GPIO_PORTH | DRV_GPIO_PIN5,
	DRV_GPIO_PORTH | DRV_GPIO_PIN6
};


//Private function declaration


//Public function definition

uint Button_Initialize(void)
{
	uint i;
	uint ui_Value;


	for (i = 0; i < BUTTON_COUNT_ID; i++)
	{
		ui_Value = DRV_GPIO_MODE_INPUT;
		DrvGPIO_SetConfig(m_ui_ButtonChannel[i], DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
		ui_Value = 1;
		DrvGPIO_SetConfig(m_ui_ButtonChannel[i], DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
		//DrvGPIO_SetConfig(m_ui_ButtonChannel[i], DRV_GPIO_PARAM_INTERRUPT, 
			//(const uint8 *)&ui_Value);

		if (DrvGPIO_Read(m_ui_ButtonChannel[i]) != 0)
		{
			m_ui_State[i] = BUTTON_STATE_RELEASE;
		}
		else
		{
			m_ui_State[i] = BUTTON_STATE_PRESS;
		}
	}

	return FUNCTION_OK;
}


uint Button_SetConfig
(
	uint ui_ButtonID,
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	if (ui_ButtonID >= BUTTON_COUNT_ID)
	{
		return FUNCTION_FAIL;
	}	

	switch (ui_Parameter)
	{
		case BUTTON_PARAM_LONG_PRESS_DELAY:
			m_u16_LongPressDelay[ui_ButtonID] = *((uint16 *)u8p_Value);
			break;

		case BUTTON_PARAM_CALLBACK:
			DrvGPIO_SetConfig(m_ui_ButtonChannel[ui_ButtonID], 
				DRV_GPIO_PARAM_CALLBACK, u8p_Value);
			break;
		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint Button_GetConfig
(
	uint ui_ButtonID,
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	if (ui_ButtonID >= BUTTON_COUNT_ID)
	{
		return FUNCTION_FAIL;
	}	

	switch (ui_Parameter)
	{
		case BUTTON_PARAM_LONG_PRESS_DELAY:
			*((uint16 *)u8p_Value) = m_u16_LongPressDelay[ui_ButtonID];
			break;

		default:
			return FUNCTION_FAIL;
	}


	return FUNCTION_OK;
}


void Button_Tick
(
	uint16 u16_TickTime
)
{
	uint i;


	for (i = 0; i < BUTTON_COUNT_ID; i++)
	{
		if (DrvGPIO_Read(m_ui_ButtonChannel[i]) != 0)
		{
			m_ui_State[i] = BUTTON_STATE_RELEASE;
			m_u16_PressTimer[i] = 0;
		}
		else
		{
			if (m_u16_PressTimer[i] > m_u16_LongPressDelay[i])
			{
				m_ui_State[i] = BUTTON_STATE_LONG_PRESS;
			}
			else
			{
				m_ui_State[i] = BUTTON_STATE_PRESS;
				m_u16_PressTimer[i] += u16_TickTime;
			}
		}
	}
}


uint Button_Read
(
	uint ui_ButtonID
)
{
	if (ui_ButtonID >= BUTTON_COUNT_ID)
	{
		return BUTTON_STATE_INVALID;
	}	

	return m_ui_State[ui_ButtonID];
}


#if BUTTON_TEST_ENABLE == 1

void Button_Test(void)
{
}

#endif


//Private function definition
