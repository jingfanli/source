/*
 * Module:	ADC driver
 * Author:	Lvjianfeng
 * Date:	2012.8
 */


#include "stm8l15x.h"

#include "drv.h"


//Constant definition

//#define FACTORY_SETTING_ENABLE

#define SAMPLE_COUNT					1
#define REFERENCE_DEFAULT				1224
#define REFERENCE_MAX					1242
#define REFERENCE_MIN					1202
#define FACTORY_VOLTAGE					3000
#define FACTORY_BASE					0x0600

#define OPTION_ADDRESS_REFERENCE		((uint16)0x4910)


//Type definition

typedef enum
{
	DRV_ADC_CLOCK_CYCLES_4 = 0,
	DRV_ADC_CLOCK_CYCLES_9,
	DRV_ADC_CLOCK_CYCLES_16,
	DRV_ADC_CLOCK_CYCLES_24,
	DRV_ADC_CLOCK_CYCLES_48,
	DRV_ADC_CLOCK_CYCLES_96,
	DRV_ADC_CLOCK_CYCLES_192,
	DRV_ADC_CLOCK_CYCLES_384,
	DRV_ADC_CLOCK_CYCLES
} drv_adc_clock_cycles;


//Variable definition

static uint m_ui_SampleCount = {0};
static uint16 m_u16_Reference = {0};
static uint16 m_u16_ADCValue[DRV_ADC_COUNT_CHANNEL] = {0};
static uint16 m_u16_ADCSum[DRV_ADC_COUNT_CHANNEL] = {0};


//Private function declaration

static void DrvADC_StartConversion(void);


//Public function definition

uint DrvADC_Initialize(void)
{
#ifdef FACTORY_SETTING_ENABLE
	uint8 u8_Value;
	uint16 u16_Length;
#endif


	CLK->PCKENR2 |= CLK_PCKENR2_ADC1;

	/* Initialize and configure ADC1 */
	ADC1->CR2 |= ADC_CR2_PRESC | DRV_ADC_CLOCK_CYCLES_16;
	ADC1->CR3 |= DRV_ADC_CLOCK_CYCLES_16 << 5;
	ADC1->TRIGR[0] |= 0x0C;
	ADC1->TRIGR[2] |= 0x20;
	ADC1->TRIGR[3] |= 0x80;

	//Connect ADC to DMA channel 2
	SYSCFG->RMPCR1 |= 0x02;

	CLK->PCKENR2 |= CLK_PCKENR2_COMP;

	RI->IOSR3 |= 0x04;

#ifdef FACTORY_SETTING_ENABLE
	u16_Length = 1;
	DrvFLASH_Read(OPTION_ADDRESS_REFERENCE, &u8_Value, &u16_Length);
	m_u16_Reference = FACTORY_BASE + (uint16)u8_Value;
	m_u16_Reference = ((uint32)m_u16_Reference * FACTORY_VOLTAGE) / 
		(1 << DRV_ADC_RESOLUTION);

	if ((m_u16_Reference < REFERENCE_MIN) ||
		(m_u16_Reference > REFERENCE_MAX))
	{
		m_u16_Reference = REFERENCE_DEFAULT;
	}
#else
	m_u16_Reference = REFERENCE_DEFAULT;
#endif

	return FUNCTION_OK;
}


uint DrvADC_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	return FUNCTION_FAIL;
}


uint DrvADC_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case DRV_ADC_PARAM_REFERENCE:
			*((uint16 *)u8p_Value) = m_u16_Reference;
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint DrvADC_Read
(
	uint ui_Channel,
	uint16 *u16p_Value
)
{
	if ((ui_Channel >= DRV_ADC_COUNT_CHANNEL) || (m_ui_SampleCount > 0))
	{
		return FUNCTION_FAIL;
	}

	*u16p_Value = m_u16_ADCSum[ui_Channel] / SAMPLE_COUNT;

	return FUNCTION_OK;
}


void DrvADC_Enable(void)
{
	//Enable reference voltage output
	COMP->CSR3 |= COMP_CSR3_VREFOUTEN;

	//Enable reference voltage and temperature sensor
	ADC1->TRIGR[0] |= ADC_TRIGR1_VREFINTON;
	ADC1->TRIGR[0] |= ADC_TRIGR1_TSON;

	ADC1->CR1 |= ADC_CR1_ADON;

	/* Enable ADC1 Channels */
	ADC1->SQR[0] |= 0x3C;
	ADC1->SQR[2] |= 0xf8;
	ADC1->SQR[3] |= 0x80;
}


void DrvADC_Disable(void)
{
	while (m_ui_SampleCount > 0)
	{
		;
	}

	ADC1->CR1 &= ~ADC_CR1_ADON;

	//Disable reference voltage and temperature sensor
	ADC1->TRIGR[0] &= ~ADC_TRIGR1_VREFINTON;
	ADC1->TRIGR[0] &= ~ADC_TRIGR1_TSON;

	//Disable reference voltage output
	COMP->CSR3 &= ~COMP_CSR3_VREFOUTEN;
}


void DrvADC_Interrupt(void)
{
	uint i;


	for (i = 0; i < DRV_ADC_COUNT_CHANNEL; i++)
	{
		m_u16_ADCSum[i] += m_u16_ADCValue[i];
	}

	if ((m_ui_SampleCount > 0) && (m_ui_SampleCount < SAMPLE_COUNT))
	{
		m_ui_SampleCount++;
		DrvADC_StartConversion();
	}
	else
	{
		m_ui_SampleCount = 0;
	}
}


void DrvADC_Sample(void)
{
	if (m_ui_SampleCount == 0)
	{
		Drv_Memset((uint8 *)m_u16_ADCSum, 0, sizeof(m_u16_ADCSum));
		m_ui_SampleCount++;
		DrvADC_StartConversion();
	}
}


#if DRV_ADC_TEST_ENABLE == 1

#include "drv_uart.h"


void DrvADC_Test(void)
{
	uint16 u16_ADCValue[DRV_ADC_COUNT_CHANNEL];
	uint i;

	while (1)
	{
		DrvADC_Sample();

		for (i = 0; i < DRV_ADC_COUNT_CHANNEL; i++)
		{
			while (DrvADC_Read(i, &u16_ADCValue[i]) != FUNCTION_OK)
			{
				;
			}

		}

		i = 0;
	}
}

#endif

//Private function definition

static void DrvADC_StartConversion(void)
{
	/* Disable the selected DMA Channelx */
	DMA1_Channel2->CCR &= ~DMA_CCR_CE;

	/* Reset DMA Channelx control register */
	DMA1_Channel2->CCR = DMA_CCR_RESET_VALUE;

	/* Set DMA direction & Mode & Incremental Memory mode */
	DMA1_Channel2->CCR |= DMA_CCR_IDM;

	/*Clear old priority and memory data size  option */
	DMA1_Channel2->CSPR &= ~(DMA_CSPR_PL | DMA_CSPR_16BM);

	/* Set old priority and memory data size  option */
	DMA1_Channel2->CSPR |= DMA_CSPR_16BM;

	/* Write to DMA Channelx CNDTR */
	DMA1_Channel2->CNBTR = DRV_ADC_COUNT_CHANNEL;

	/* Write to DMA Channelx (0, 1 or 2)  Peripheral address  or  Write to 
	   DMA Channel 3 Memory 1 address  */
	DMA1_Channel2->CPARH = (uint8)((uint32)&ADC1->DRH >> 8);
	DMA1_Channel2->CPARL = (uint8)((uint32)&ADC1->DRH);

	/* Write to DMA Channelx Memory address */
	DMA1_Channel2->CM0ARH = (uint8)((uint32)m_u16_ADCValue >> 8);
	DMA1_Channel2->CM0ARL = (uint8)((uint32)m_u16_ADCValue & 0xFF);

	DMA1_Channel2->CSPR &= ~DMA_CSPR_TCIF;
	DMA1_Channel2->CCR |= DMA_ITx_TC;
	DMA1_Channel2->CCR |= DMA_CCR_CE;

	//Start ADC conversion
	ADC1->CR1 |= ADC_CR1_START;
}
