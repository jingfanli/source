/*
 * Module:	Hardware driver
 * Author:	Lvjianfeng
 * Date:	2011.8
 */


#include "drv.h"


//Constant definition

#define GPIO_CHANNEL_EN_PROM		(DRV_GPIO_PORTF | DRV_GPIO_PIN6)
#define GPIO_CHANNEL_TEST			(DRV_GPIO_PORTI | DRV_GPIO_PIN2)

#define POWER_TIME_OUT				10000
#define POWER_LOW_VOLTAGE			2500
#define POWER_OFF_VOLTAGE			2400

#define LSI_CLOCK					38000
#define TICK_TIME_RATIO				1000

#define MEMORY_DMA_ENABLE			0	
#define WRITE_PROTECTION_ENABLE		1	
#define READ_PROTECTION_ENABLE		0	
#define WATCHDOG_ENABLE				1
#define BOOTLOADER_ENABLE			1

#define WATCHDOG_REFRESH_CYCLE		1500
#define WATCHDOG_KEY_ENABLE			0xCC
#define WATCHDOG_KEY_REFRESH		0xAA
#define WATCHDOG_KEY_ACCESS			0x55

#define OPTION_ADDRESS_ROP			((uint16)0x4800)
#define OPTION_ADDRESS_UBC			((uint16)0x4802)
#define OPTION_ADDRESS_WATCHDOG		((uint16)0x4808)
#define OPTION_ADDRESS_BOR			((uint16)0x480A)
#define OPTION_ADDRESS_BOOTLOADER	((uint16)0x480B)
#define OPTION_OFFSET_WATCHDOG		1
#define OPTION_THRESHOLD_BOR		0x01

#if BOOTLOADER_ENABLE != 0
#define OPTION_BOOTLOADER			0x55AA
#else
#define OPTION_BOOTLOADER			0x0000
#endif


//Type definition

typedef enum
{
	DRV_FLAG_PVD = 0,
	DRV_COUNT_FLAG
} drv_flag;

typedef enum
{
	IWDG_DIVIDER_4 = 0,
	IWDG_DIVIDER_8,
	IWDG_DIVIDER_16,
	IWDG_DIVIDER_32,
	IWDG_DIVIDER_64,
	IWDG_DIVIDER_128,
	IWDG_DIVIDER_256,
	IWDG_DIVIDER_COUNT
} drv_iwdg_divider;

typedef uint (*drv_set_config)
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);

typedef uint (*drv_get_config)
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);


//Private function declaration

static void Drv_InitializeClock(void);
static void Drv_InitializeWatchdog(void);
static void Drv_InitializeTestPort(void);
static void Drv_InitializeDelay(void);
static void Drv_Tick
(
	uint16 u16_TickTime
);
static uint16 Drv_GetVoltageVDD(void);
static uint Drv_CheckPower(void);
static uint Drv_UART1SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
static uint Drv_UART2SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
static uint Drv_UART1GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
static uint Drv_UART2GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);


//Private variable definition

static uint m_ui_Flag = {0};
static uint m_ui_SleepMode = {0};
static uint16 m_u16_TickTime = {0};
static uint16 m_u16_DelayCount = {0};
static uint16 m_u16_DelayBegin = {0};
static uint16 m_u16_DelayEnd = {0};

static const drv_set_config m_fp_SetConfig[DRV_COUNT_PORT] =
{
	Drv_SetConfig,
	DrvADC_SetConfig,
	DrvBEEP_SetConfig,
	DrvDAC_SetConfig,
	DrvEEPROM_SetConfig,
	DrvFLASH_SetConfig,
	0,
	0,
	DrvRTC_SetConfig,
	Drv_UART1SetConfig,
	Drv_UART2SetConfig
};
static const drv_get_config m_fp_GetConfig[DRV_COUNT_PORT] =
{
	Drv_GetConfig,
	DrvADC_GetConfig,
	DrvBEEP_GetConfig,
	DrvDAC_GetConfig,
	DrvEEPROM_GetConfig,
	DrvFLASH_GetConfig,
	0,
	0,
	DrvRTC_GetConfig,
	Drv_UART1GetConfig,
	Drv_UART2GetConfig
};


//Public function definition

uint Drv_Initialize(void)
{
	Drv_DisableInterrupt();

	//Initialize peripheral
	Drv_InitializeClock();
	DrvGPIO_Initialize();
	DrvRTC_Initialize();
	DrvLCD_Initialize();
	DrvUART_Initialize(DRV_UART_DEVICE_ID_1);
	DrvUART_Initialize(DRV_UART_DEVICE_ID_2);
	//DrvEEPROM_Initialize();
	DrvADC_Initialize();
	DrvDAC_Initialize();
	DrvBEEP_Initialize();
	//DrvFLASH_Initialize();
	//Drv_InitializeWatchdog();
	Drv_InitializeTestPort();
	Drv_InitializeDelay();

	Drv_EnableInterrupt();

	return FUNCTION_OK;
}


uint Drv_HandleEvent
(
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	uint8 u8_Event
)
{
	if (MESSAGE_GET_MAJOR_PORT(u8_TargetPort) != MESSAGE_PORT_DRIVER)
	{
		return FUNCTION_FAIL;
	}

	switch (u8_Event)
	{
		case MESSAGE_EVENT_SEND_DONE:
			break;

		case MESSAGE_EVENT_ACKNOWLEDGE:
			break;

		case MESSAGE_EVENT_TIMEOUT:
			break;

		case MESSAGE_EVENT_RECEIVE_DONE:
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint Drv_HandleCommand
(
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	message_command *tp_Command
)
{
	uint ui_Length;
	uint ui_Acknowledge;
	uint8 u8_Port;


	if (MESSAGE_GET_MAJOR_PORT(u8_TargetPort) != MESSAGE_PORT_DRIVER)
	{
		return FUNCTION_FAIL;
	}

	u8_Port = MESSAGE_GET_MINOR_PORT(u8_TargetPort);

	if (u8_Port >= DRV_COUNT_PORT)
	{
		return FUNCTION_FAIL;
	}

	ui_Acknowledge = FUNCTION_FAIL;

	switch (tp_Command->u8_Operation)
	{
		case MESSAGE_OPERATION_SET:

			if (m_fp_SetConfig[u8_Port] != (drv_set_config)0)
			{
				ui_Acknowledge = 
					m_fp_SetConfig[u8_Port]((uint)tp_Command->u8_Parameter,
					tp_Command->u8p_Data, (uint)tp_Command->u8_Length);

				if (ui_Acknowledge == FUNCTION_OK)
				{
					tp_Command->u8p_Data[0] = (uint8)ui_Acknowledge;
					tp_Command->u8_Operation = MESSAGE_OPERATION_ACKNOWLEDGE;
					tp_Command->u8_Length = sizeof(tp_Command->u8p_Data[0]);
				}
			}

			break;

		case MESSAGE_OPERATION_GET:

			if (m_fp_GetConfig[u8_Port] != (drv_get_config)0)
			{
				ui_Length = (uint)tp_Command->u8_Length;
				
				ui_Acknowledge = 
					m_fp_GetConfig[u8_Port]((uint)tp_Command->u8_Parameter,
					tp_Command->u8p_Data, &ui_Length);

				if (ui_Acknowledge == FUNCTION_OK)
				{
					tp_Command->u8_Operation = MESSAGE_OPERATION_NOTIFY;
					tp_Command->u8_Length = (uint8)ui_Length;
				}
			}

			break;

		case MESSAGE_OPERATION_NOTIFY:
		case MESSAGE_OPERATION_ACKNOWLEDGE:
			ui_Acknowledge = FUNCTION_OK;
			tp_Command->u8p_Data[0] = (uint8)MESSAGE_EVENT_ACKNOWLEDGE;
			tp_Command->u8_Operation = MESSAGE_OPERATION_EVENT;
			tp_Command->u8_Length = sizeof(tp_Command->u8p_Data[0]);
			break;

		default:
			break;
	}
	
	return ui_Acknowledge;
}


uint Drv_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	switch (ui_Parameter)
	{
		case DRV_PARAM_SLEEP_MODE:
			m_ui_SleepMode = *((uint *)u8p_Value);
			break;
			
		case DRV_PARAM_PVD:
			Drv_DisableInterrupt();

			if (*((uint *)u8p_Value) != 0)
			{
				REG_SET_BIT(m_ui_Flag, DRV_FLAG_PVD);
			}
			else
			{
				REG_CLEAR_BIT(m_ui_Flag, DRV_FLAG_PVD);
			}

			Drv_EnableInterrupt();
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint Drv_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case DRV_PARAM_POWER_MODE:
			*((uint *)u8p_Value) = Drv_CheckPower();
			break;
			
		case DRV_PARAM_SLEEP_MODE:
			*((uint *)u8p_Value) = m_ui_SleepMode;
			break;
			
		case DRV_PARAM_VOLTAGE_VDD:
			*((uint16 *)u8p_Value) = Drv_GetVoltageVDD();
			break;
			
		case DRV_PARAM_PVD:

			if (REG_GET_BIT(m_ui_Flag, DRV_FLAG_PVD) != 0)
			{
				*((uint *)u8p_Value) = 1;
			}
			else
			{
				*((uint *)u8p_Value) = 0;
			}

			break;

		case DRV_PARAM_DELAY:
			*((uint16 *)u8p_Value) = m_u16_DelayCount;
			break;
			
		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


void Drv_Delay
(
	uint16 u16_DelayTime
)
{
	uint16 u16_DelayCount;


	u16_DelayTime = (u16_DelayTime * m_u16_DelayCount) / TICK_TIME_RATIO;

	for (u16_DelayCount = 0; u16_DelayCount < u16_DelayTime; u16_DelayCount++)
	{
		nop();
	}
}


void Drv_Sleep(void)
{
	if (m_ui_SleepMode == DRV_SLEEP_MODE_WAIT)
	{
		//wfi();
	}
	else
	{
		//CLK->CKDIVR = 0x01;
		//halt();
		//CLK->CKDIVR = 0x00;
	}
}


void Drv_PVDInterrupt(void)
{
	if ((PWR->CSR1 & PWR_CSR1_PVDIF) != 0)
	{
		PWR->CSR1 |= PWR_CSR1_PVDIF;

		if ((PWR->CSR1 & PWR_CSR1_PVDOF) != 0)
		{
			REG_SET_BIT(m_ui_Flag, DRV_FLAG_PVD);
		}
	}
}


void Drv_RefreshWatchdog(void)
{
#if WATCHDOG_ENABLE != 0
	IWDG->KR = WATCHDOG_KEY_REFRESH;
#endif
}


void Drv_Memcpy
(
	uint8 *u8p_Target,
	const uint8 *u8p_Source,
	uint16 u16_Length
)
{
#if MEMORY_DMA_ENABLE != 0
	/* Disable the selected DMA Channelx */
	DMA1_Channel3->CCR &= ~DMA_CCR_CE;

	/* Reset DMA Channelx control register */
	DMA1_Channel3->CCR = DMA_CCR_RESET_VALUE;

	/* Set DMA direction & Mode & Incremental Memory mode */
	DMA1_Channel3->CCR |= (DMA_CCR_IDM | DMA_CCR_MEM);

	/*Clear old priority and memory data size  option */
	DMA1_Channel3->CSPR &= ~(DMA_CSPR_PL | DMA_CSPR_16BM);

	/* Set old priority and memory data size  option */

	/* Write to DMA Channelx CNDTR */
	DMA1_Channel3->CNBTR = (uint8)u16_Length;

	/* Write to DMA Channelx (0, 1 or 2)  Peripheral address  or  Write to 
	   DMA Channel 3 Memory 1 address  */
	DMA1_Channel3->CPARH = (uint8)((uint32)u8p_Target >> 8);
	DMA1_Channel3->CPARL = (uint8)((uint32)u8p_Target);

	/* Write to DMA Channelx Memory address */
	DMA1_Channel3->CM0ARH = (uint8)((uint32)u8p_Source >> 8);
	DMA1_Channel3->CM0ARL = (uint8)((uint32)u8p_Source);

	DMA1_Channel3->CSPR &= ~DMA_CSPR_TCIF;
	DMA1_Channel3->CCR |= DMA_CCR_CE;

	while ((DMA1_Channel3->CSPR & DMA_CSPR_TCIF) == 0)
	{
		;
	}

	DMA1_Channel3->CSPR &= ~DMA_CSPR_TCIF;
	DMA1_Channel3->CCR &= ~DMA_CCR_CE;
#else
	while (u16_Length > 0)
	{
		*u8p_Target++ = *u8p_Source++;
		u16_Length--;
	}
#endif
}


void Drv_Memset
(
	uint8 *u8p_Target,
	uint8 u8_Value,
	uint16 u16_Length
)
{
	while (u16_Length > 0)
	{
		*u8p_Target++ = u8_Value;
		u16_Length--;
	}
}


uint Drv_Memcmp
(
	const uint8 *u8p_Target,
	const uint8 *u8p_Source,
	uint16 u16_Length
)
{
	while (u16_Length > 0)
	{
		if (*u8p_Target++ != *u8p_Source++)
		{
			return FUNCTION_FAIL;
		}

		u16_Length--;
	}

	return FUNCTION_OK;
}


void Drv_EnablePower(void)
{
	uint16 u16_Timeout;


	DrvGPIO_Set(GPIO_CHANNEL_EN_PROM);
	u16_Timeout = 0;

	while (DrvGPIO_Read(GPIO_CHANNEL_EN_PROM) == 0)
	{
		u16_Timeout++;

		if (u16_Timeout > POWER_TIME_OUT)
		{
			return;
		}
	}
}


void Drv_DisablePower(void)
{
	DrvGPIO_Clear(GPIO_CHANNEL_EN_PROM);
}


void Drv_SetTestPort(void)
{
	DrvGPIO_Set(GPIO_CHANNEL_TEST);
}


void Drv_ClearTestPort(void)
{
	DrvGPIO_Clear(GPIO_CHANNEL_TEST);
}


void Drv_ToggleTestPort(void)
{
	DrvGPIO_Toggle(GPIO_CHANNEL_TEST);
}


//Private function definition

static void Drv_InitializeClock(void)
{
	uint ui_Value;


	/* High speed internal clock prescaler: 1*/
	CLK->CKDIVR = 0x00;

	/* Enable RTC clock */
	CLK->CRTCR = 0x10;

	/* Wait for LSE clock to be ready */
	while ((CLK->ECKCR & CLK_ECKCR_LSERDY) == 0)
	{
		;
	}

	CLK->ICKCR |= CLK_ICKCR_LSION;

	/* Wait for LSI clock to be ready */
	while ((CLK->ICKCR & CLK_ICKCR_LSIRDY) == 0)
	{
		;
	}

	/* Enable DMA1 clock */
	CLK->PCKENR2 |= CLK_PCKENR2_DMA1;

	/* DMA1 enable */
	DMA1->GCSR |= DMA_GCSR_GE;

	ui_Value = DRV_GPIO_MODE_OUTPUT;
	DrvGPIO_SetConfig(GPIO_CHANNEL_EN_PROM, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	ui_Value = 1;
	DrvGPIO_SetConfig(GPIO_CHANNEL_EN_PROM, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
}


static void Drv_InitializeWatchdog(void)
{
	uint8 u8_Value;
	uint16 u16_Value;
	uint16 u16_Length;


	PWR->CSR1 = 0x15;

	u16_Length = sizeof(u8_Value);
	DrvFLASH_Read(OPTION_ADDRESS_BOR, &u8_Value, &u16_Length);

	if (u8_Value != OPTION_THRESHOLD_BOR)
	{
		u8_Value = OPTION_THRESHOLD_BOR;
		DrvFLASH_Write(OPTION_ADDRESS_BOR, &u8_Value, 1);
	}

	u16_Length = sizeof(u16_Value);
	DrvFLASH_Read(OPTION_ADDRESS_BOOTLOADER, (uint8 *)&u16_Value, &u16_Length);

	if (u16_Value != OPTION_BOOTLOADER)
	{
		u16_Value = OPTION_BOOTLOADER;
		DrvFLASH_Write(OPTION_ADDRESS_BOOTLOADER, (const uint8 *)&u16_Value, 
			sizeof(u16_Value));
	}

#if WRITE_PROTECTION_ENABLE != 0
	u16_Length = sizeof(u8_Value);
	DrvFLASH_Read(OPTION_ADDRESS_UBC, &u8_Value, &u16_Length);

	if (u8_Value != ((DRV_FLASH_ADDRESS_PROGRAM_USER - 
		DRV_FLASH_ADDRESS_PROGRAM_BEGIN) / DRV_FLASH_PAGE_SIZE))
	{
		u8_Value = ((DRV_FLASH_ADDRESS_PROGRAM_USER - 
			DRV_FLASH_ADDRESS_PROGRAM_BEGIN) / DRV_FLASH_PAGE_SIZE);
		DrvFLASH_Write(OPTION_ADDRESS_UBC, &u8_Value, 1);
	}
#endif

#if READ_PROTECTION_ENABLE != 0
	u16_Length = sizeof(u8_Value);
	DrvFLASH_Read(OPTION_ADDRESS_ROP, &u8_Value, &u16_Length);

	if (u8_Value != 0)
	{
		u8_Value = 0;
		DrvFLASH_Write(OPTION_ADDRESS_ROP, &u8_Value, 1);
	}
#endif

#if WATCHDOG_ENABLE != 0
	u16_Length = sizeof(u8_Value);
	DrvFLASH_Read(OPTION_ADDRESS_WATCHDOG, &u8_Value, &u16_Length);

	if (REG_GET_BIT(u8_Value, OPTION_OFFSET_WATCHDOG) == 0)
	{
		REG_SET_BIT(u8_Value, OPTION_OFFSET_WATCHDOG);
		DrvFLASH_Write(OPTION_ADDRESS_WATCHDOG, &u8_Value, 1);
	}

	IWDG->KR = WATCHDOG_KEY_ENABLE;
	IWDG->KR = WATCHDOG_KEY_ACCESS;
	IWDG->PR = IWDG_DIVIDER_256;
	IWDG->RLR = (uint8)(((uint32)LSI_CLOCK * (uint32)WATCHDOG_REFRESH_CYCLE) / 
		((uint32)1000 * (uint32)256)) - 1;
	IWDG->KR = WATCHDOG_KEY_REFRESH;
#endif
}


static void Drv_InitializeTestPort(void)
{
	uint ui_Value;


	ui_Value = DRV_GPIO_MODE_OUTPUT;
	DrvGPIO_SetConfig(GPIO_CHANNEL_TEST, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
}


static void Drv_InitializeDelay(void)
{
	uint ui_Value;
	drv_rtc_callback t_Callback;


	t_Callback.fp_Wakeup = Drv_Tick;
	t_Callback.fp_Alarm = (drv_rtc_callback_alarm)0;
	DrvRTC_SetConfig(DRV_RTC_PARAM_CALLBACK, (const uint8 *)&t_Callback, 0);
	ui_Value = 1;
	DrvRTC_SetConfig(DRV_RTC_PARAM_SWITCH_WAKEUP, (const uint8 *)&ui_Value,
		sizeof(ui_Value));
	Drv_EnableInterrupt();

	for (m_u16_DelayCount = 0; m_u16_DelayCount < ~0; m_u16_DelayCount++)
	{
		nop();
	}

	Drv_DisableInterrupt();
	ui_Value = 0;
	DrvRTC_SetConfig(DRV_RTC_PARAM_SWITCH_WAKEUP, (const uint8 *)&ui_Value,
		sizeof(ui_Value));

	if (m_u16_DelayEnd >= m_u16_DelayBegin)
	{
		m_u16_DelayCount = (m_u16_DelayEnd - m_u16_DelayBegin) / m_u16_TickTime;
	}
}


static void Drv_Tick
(
	uint16 u16_TickTime
)
{
	m_u16_TickTime = u16_TickTime;

	if (m_u16_DelayCount > 0)
	{
		if (m_u16_DelayBegin == 0)
		{
			m_u16_DelayBegin = m_u16_DelayCount;
		}
		else
		{
			if (m_u16_DelayEnd == 0)
			{
				m_u16_DelayEnd = m_u16_DelayCount;
			}
		}
	}
}


static uint16 Drv_GetVoltageVDD(void)
{
	uint16 u16_Reference;
	uint16 u16_VoltageVDD;
	uint16 u16_VoltageRef;


	while (DrvADC_Read(DRV_ADC_CHANNEL_REF, &u16_Reference) != FUNCTION_OK)
	{
		;
	}

	while (DrvADC_Read(DRV_ADC_CHANNEL_VDD, &u16_VoltageVDD) != FUNCTION_OK)
	{
		;
	}

	DrvADC_GetConfig(DRV_ADC_PARAM_REFERENCE, (uint8 *)&u16_VoltageRef, 
		(uint *)0);

	u16_VoltageVDD = (uint16)((((uint32)u16_VoltageRef * 43) / 10) * 
		(uint32)u16_VoltageVDD / (uint32)u16_Reference);

	return u16_VoltageVDD;
}


static uint Drv_CheckPower(void)
{
	uint16 u16_VoltageVDD;


	Drv_EnablePower();
	DrvADC_Enable();
	DrvADC_Sample();
	u16_VoltageVDD = Drv_GetVoltageVDD();
	DrvADC_Disable();
	Drv_DisablePower();

	if (u16_VoltageVDD < POWER_OFF_VOLTAGE)
	{
		return DRV_POWER_MODE_NORMAL;
	}
	else if (u16_VoltageVDD < POWER_LOW_VOLTAGE)
	{
		return DRV_POWER_MODE_NORMAL;
	}
	else
	{
		return DRV_POWER_MODE_NORMAL;
	}
}


static uint Drv_UART1SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	return DrvUART_SetConfig(DRV_UART_DEVICE_ID_1, ui_Parameter, u8p_Value, 
		ui_Length);
}


static uint Drv_UART2SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	return DrvUART_SetConfig(DRV_UART_DEVICE_ID_2, ui_Parameter, u8p_Value, 
		ui_Length);
}


static uint Drv_UART1GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	return DrvUART_GetConfig(DRV_UART_DEVICE_ID_1, ui_Parameter, u8p_Value, 
		uip_Length);
}


static uint Drv_UART2GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	return DrvUART_GetConfig(DRV_UART_DEVICE_ID_2, ui_Parameter, u8p_Value, 
		uip_Length);
}
