/*
 * Module:	DAC driver
 * Author:	Lvjianfeng
 * Date:	2014.4
 */


#include "stm8l15x.h"

#include "drv.h"


//Constant definition

#define FREQUENCY_DEFAULT				100
#define REFERENCE_DEFAULT				1224
#define REFERENCE_MAX					1242
#define REFERENCE_MIN					1202


//Type definition

typedef enum
{
	DRV_DAC_FLAG_BUSY = 0,
	DRV_DAC_COUNT_FLAG
} drv_dac_flag;


//Variable definition

static uint m_ui_Flag = {0};
static uint16 m_u16_Frequency = {0};
static uint16 m_u16_Reference = {0};


//Private function declaration


//Public function definition

uint DrvDAC_Initialize(void)
{
	m_u16_Frequency = FREQUENCY_DEFAULT;

	CLK->PCKENR1 |= CLK_PCKENR1_DAC;
	CLK->PCKENR1 |= CLK_PCKENR1_TIM4;

	DAC->CH1CR1 = (DAC_CR1_TEN | DAC_CR1_BOFF);

	TIM4->PSCR = 0x00;
	TIM4->ARR = (DRV_SYSTEM_CLOCK / 1000) / m_u16_Frequency;
	TIM4->EGR = 0x01;
	TIM4->CR2 = 0x20;

	return FUNCTION_OK;
}


uint DrvDAC_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	switch (ui_Parameter)
	{
		case DRV_DAC_PARAM_FREQUENCY:
			
			if (*((const uint16 *)u8p_Value) > 0)
			{
				m_u16_Frequency = *((const uint16 *)u8p_Value);
				TIM4->ARR = (DRV_SYSTEM_CLOCK / 1000) / m_u16_Frequency;
			}

			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint DrvDAC_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case DRV_DAC_PARAM_REFERENCE:
			*((uint16 *)u8p_Value) = m_u16_Reference;
			break;

		case DRV_DAC_PARAM_FREQUENCY:
			*((uint16 *)u8p_Value) = m_u16_Frequency;
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


void DrvDAC_Enable
(
	uint16 *u16p_Data,
	uint ui_Length
)
{
	if (ui_Length == 0)
	{
		return;
	}

	if (REG_GET_BIT(m_ui_Flag, DRV_DAC_FLAG_BUSY) != 0)
	{
		DrvDAC_Disable();
	}

	REG_SET_BIT(m_ui_Flag, DRV_DAC_FLAG_BUSY);

	/* Disable the selected DMA Channelx */
	DMA1_Channel3->CCR &= ~DMA_CCR_CE;

	/* Reset DMA Channelx control register */
	DMA1_Channel3->CCR = DMA_CCR_RESET_VALUE;

	/* Set DMA direction & Mode & Incremental Memory mode */
	DMA1_Channel3->CCR |= (DMA_CCR_IDM | DMA_CCR_ARM | DMA_CCR_DTD);

	/* Set priority and memory data size option */
	DMA1_Channel3->CSPR |= (0x20 | DMA_CSPR_16BM);

	/* Set old priority and memory data size option */

	/* Write to DMA Channelx CNDTR */
	DMA1_Channel3->CNBTR = (uint8)ui_Length;

	/* Write to DMA Channelx (0, 1 or 2) Peripheral address or Write to 
	   DMA Channel 3 Memory 1 address  */
	DMA1_Channel3->CPARH = (uint8)((uint32)&DAC->CH1RDHRH >> 8);
	DMA1_Channel3->CPARL = (uint8)((uint32)&DAC->CH1RDHRH);

	/* Write to DMA Channelx Memory address */
	DMA1_Channel3->CM0ARH = (uint8)((uint32)u16p_Data >> 8);
	DMA1_Channel3->CM0ARL = (uint8)((uint32)u16p_Data & 0xFF);

	DMA1_Channel3->CCR |= DMA_CCR_CE;
	TIM4->CNTR = TIM4->ARR;
	TIM4->SR1 &= ~TIM4_SR1_UIF;
	TIM4->CR1 |= TIM4_CR1_CEN;
	DAC->CH1CR2 |= DAC_CR2_DMAEN;
	DAC->CH1CR1 |= DAC_CR1_EN;
}


void DrvDAC_Disable(void)
{
	if (REG_GET_BIT(m_ui_Flag, DRV_DAC_FLAG_BUSY) == 0)
	{
		return;
	}

	while (DMA1_Channel3->CNBTR != 1)
	{
		;
	}

	DAC->CH1CR1 &= ~DAC_CR1_EN;
	DAC->CH1CR2 &= ~DAC_CR2_DMAEN;
	TIM4->CR1 &= ~TIM4_CR1_CEN;
	DMA1_Channel3->CCR &= ~DMA_CCR_CE;

	REG_CLEAR_BIT(m_ui_Flag, DRV_DAC_FLAG_BUSY);
}


#if DRV_DAC_TEST_ENABLE == 1

#include "glucose.h"

void DrvDAC_Test(void)
{
	uint i;
	uint ui_Value;
	uint16 u16_Value;
	uint16 u16_Reference;
	uint16 u16_VoltageRef;

	Drv_EnablePower();
	Glucose_Enable();

	ui_Value = GLUCOSE_MODE_HCT;
	Glucose_SetConfig(GLUCOSE_PARAM_MODE, (const uint8 *)&ui_Value,
		sizeof(ui_Value));
	ui_Value = 1;
	Glucose_SetConfig(GLUCOSE_PARAM_HCT_WAVEFORM, (const uint8 *)&ui_Value,
		sizeof(ui_Value));

	for (i = 0; i < 10; i++)
	{
		Drv_Delay(50000);
	}

	DrvADC_Sample();

	while (DrvADC_Read(DRV_ADC_CHANNEL_REF, &u16_Reference) != FUNCTION_OK)
	{
		;
	}

	while (DrvADC_Read(DRV_ADC_CHANNEL_HCT, &u16_Value) != FUNCTION_OK)
	{
		;
	}

	DrvADC_GetConfig(DRV_ADC_PARAM_REFERENCE, (uint8 *)&u16_VoltageRef,
		(uint *)0);

	u16_Value = DRV_ADC_GET_VOLTAGE(u16_VoltageRef, u16_Reference, u16_Value);
	ui_Value = 0;
	Glucose_SetConfig(GLUCOSE_PARAM_HCT_WAVEFORM, (const uint8 *)&ui_Value,
		sizeof(ui_Value));

	ui_Value = GLUCOSE_MODE_BG1;
	Glucose_SetConfig(GLUCOSE_PARAM_MODE, (const uint8 *)&ui_Value,
		sizeof(ui_Value));

	Glucose_Disable();
	Drv_DisablePower();

	for (i = 0; i < 100; i++)
	{
		Drv_Delay(50000);
	}
}

#endif

//Private function definition
