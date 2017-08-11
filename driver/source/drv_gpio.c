/*
 * Module:	General purpose IO driver
 * Author:	Lvjianfeng
 * Date:	2011.8
 */


#include "stm8l15x.h"

#include "drv.h"


//Constant definition


//Type definition


//Variable definition

static const uint m_ui_BitMap[] = 
{
	(1 << 0), (1 << 1), (1 << 2), (1 << 3), (1 << 4), (1 << 5), (1 << 6), (1 << 7)
};
static GPIO_TypeDef* m_tp_GPIO[] =
{
	GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI
}; 
static uint m_ui_InterruptPort[DRV_GPIO_PIN_COUNT] = {0}; 
static drv_gpio_callback m_t_Callback[DRV_GPIO_PIN_COUNT] = {0};


//Private function declaration


//Public function definition

uint DrvGPIO_Initialize(void)
{
	GPIOA->DDR = 0xFF;
	GPIOB->DDR = 0xCF;
	GPIOC->DDR = 0xFC;
	GPIOD->DDR = 0x3F;
	GPIOE->DDR = 0xFF;
	GPIOF->DDR = 0xF2;
	GPIOG->DDR = 0xFF;
	GPIOH->DDR = 0xFF;
	GPIOI->DDR = 0xFF;

	GPIOA->CR1 = 0xFF;
	GPIOB->CR1 = 0xCF;
	GPIOC->CR1 = 0xFC;
	GPIOD->CR1 = 0x3F;
	GPIOE->CR1 = 0xFF;
	GPIOF->CR1 = 0xF2;
	GPIOG->CR1 = 0xFF;
	GPIOH->CR1 = 0xFF;
	GPIOI->CR1 = 0xFF;

	return FUNCTION_OK;
}


uint DrvGPIO_SetConfig
(
	uint ui_Channel,
	uint ui_Parameter,
	const uint8 *u8p_Value
)
{
	uint ui_Pin;
	uint ui_Port;


	ui_Pin = ui_Channel & DRV_GPIO_MASK_PIN;
	ui_Port = (ui_Channel & DRV_GPIO_MASK_PORT) >> 4;

	switch (ui_Parameter)
	{
		case DRV_GPIO_PARAM_MODE:

			if (*((uint *)u8p_Value) == DRV_GPIO_MODE_OUTPUT)
			{
				m_tp_GPIO[ui_Port]->DDR |= m_ui_BitMap[ui_Pin];
			}
			else
			{
				m_tp_GPIO[ui_Port]->DDR &= ~m_ui_BitMap[ui_Pin];
			}

			break;

		case DRV_GPIO_PARAM_PULLUP:

			if (*((uint *)u8p_Value) != 0)
			{
				m_tp_GPIO[ui_Port]->CR1 |= m_ui_BitMap[ui_Pin];
			}
			else
			{
				m_tp_GPIO[ui_Port]->CR1 &= ~m_ui_BitMap[ui_Pin];
			}

			break;

		case DRV_GPIO_PARAM_INTERRUPT:
			Drv_DisableInterrupt();

			m_ui_InterruptPort[ui_Pin] = ui_Port;

			if (*((uint *)u8p_Value) != 0)
			{
				m_tp_GPIO[ui_Port]->CR2 |= m_ui_BitMap[ui_Pin];
			}
			else
			{
				m_tp_GPIO[ui_Port]->CR2 &= ~m_ui_BitMap[ui_Pin];
			}

			if (ui_Pin < DRV_GPIO_PIN4)
			{
				REG_WRITE_FIELD(EXTI->CR1, ui_Pin * 2, REG_MASK_2_BIT, 0x02);
			}
			else
			{
				ui_Pin -= DRV_GPIO_PIN4;
				REG_WRITE_FIELD(EXTI->CR2, ui_Pin * 2, REG_MASK_2_BIT, 0x02);
			}

			Drv_EnableInterrupt();
			break;

		case DRV_GPIO_PARAM_CALLBACK:
			m_t_Callback[ui_Pin].fp_Interrupt = 
				((const drv_gpio_callback *)u8p_Value)->fp_Interrupt;
			break;

		default:
			break;
	}

	return FUNCTION_OK;
}


uint DrvGPIO_GetConfig
(
	uint ui_Channel,
	uint ui_Parameter,
	uint8 *u8p_Value
)
{

	return FUNCTION_OK;
}


uint DrvGPIO_Set
(
	uint ui_Channel
)
{
	uint ui_Pin;
	uint ui_Port;


	ui_Pin = ui_Channel & DRV_GPIO_MASK_PIN;
	ui_Port = (ui_Channel & DRV_GPIO_MASK_PORT) >> 4;
	m_tp_GPIO[ui_Port]->ODR |= m_ui_BitMap[ui_Pin];

	return FUNCTION_OK;
}


uint DrvGPIO_Clear
(
	uint ui_Channel
)
{
	uint ui_Pin;
	uint ui_Port;


	ui_Pin = ui_Channel & DRV_GPIO_MASK_PIN;
	ui_Port = (ui_Channel & DRV_GPIO_MASK_PORT) >> 4;
	m_tp_GPIO[ui_Port]->ODR &= ~m_ui_BitMap[ui_Pin];

	return FUNCTION_OK;
}


uint DrvGPIO_Toggle
(
	uint ui_Channel
)
{
	uint ui_Pin;
	uint ui_Port;


	ui_Pin = ui_Channel & DRV_GPIO_MASK_PIN;
	ui_Port = (ui_Channel & DRV_GPIO_MASK_PORT) >> 4;
	m_tp_GPIO[ui_Port]->ODR ^= m_ui_BitMap[ui_Pin];

	return FUNCTION_OK;
}


uint DrvGPIO_Write
(
	uint ui_Channel,
	uint ui_Value
)
{
	uint ui_Pin;
	uint ui_Port;


	ui_Pin = ui_Channel & DRV_GPIO_MASK_PIN;
	ui_Port = (ui_Channel & DRV_GPIO_MASK_PORT) >> 4;

	if (ui_Pin == DRV_GPIO_PINALL)
	{
		m_tp_GPIO[ui_Port]->ODR = ui_Value;
	}
	else
	{
		if (ui_Value != 0)
		{
			m_tp_GPIO[ui_Port]->ODR |= m_ui_BitMap[ui_Pin];
		}
		else
		{
			m_tp_GPIO[ui_Port]->ODR &= ~m_ui_BitMap[ui_Pin];
		}
	}

	return FUNCTION_OK;
}


uint DrvGPIO_Read
(
	uint ui_Channel
)
{
	uint ui_Pin;
	uint ui_Port;
	uint ui_Value;


	ui_Pin = ui_Channel & DRV_GPIO_MASK_PIN;
	ui_Port = (ui_Channel & DRV_GPIO_MASK_PORT) >> 4;
	ui_Value = (uint)(m_tp_GPIO[ui_Port]->IDR);

	if (ui_Pin != DRV_GPIO_PINALL)
	{
		if ((ui_Value & m_ui_BitMap[ui_Pin]) != 0)
		{
			ui_Value = 1;
		}
		else
		{
			ui_Value = 0;
		}
	}

	return ui_Value;
}


void DrvGPIO_Interrupt
(
	uint ui_Pin
)
{
	m_tp_GPIO[m_ui_InterruptPort[ui_Pin]]->CR2 &= ~m_ui_BitMap[ui_Pin];
	EXTI->SR1 = (1 << ui_Pin);

	if (m_t_Callback[ui_Pin].fp_Interrupt != 0)
	{
		m_t_Callback[ui_Pin].fp_Interrupt();
	}

	m_tp_GPIO[m_ui_InterruptPort[ui_Pin]]->CR2 |= m_ui_BitMap[ui_Pin];
}


#if DRV_GPIO_TEST_ENABLE == 1

void DrvGPIO_Test(void)
{
}

#endif

//Private function definition

