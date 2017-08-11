/*
 * Module:	UART driver
 * Author:	Lvjianfeng
 * Date:	2012.8
 */


#include "stm8l15x.h"
#include "lib_queue.h"
#include "lib_frame.h"
#include "Drv_uart.h"
#include "drv.h"


//Constant definition

#define UART_DATA_BUFFER_SIZE			64
#define UART_LENGTH_BUFFER_SIZE			32
#define UART_BAUDRATE_DEFAULT			115200

#define FRAME_HEADER					'-'
#define FRAME_ESCAPE					'/'

#define GPIO_CHANNEL_UART1_TX			(DRV_GPIO_PORTA | DRV_GPIO_PIN2)
#define GPIO_CHANNEL_UART1_RX			(DRV_GPIO_PORTA | DRV_GPIO_PIN3)
#define GPIO_CHANNEL_UART1_PRESENTN		(DRV_GPIO_PORTF| DRV_GPIO_PIN5)
#define GPIO_CHANNEL_UART2_TX			(DRV_GPIO_PORTG | DRV_GPIO_PIN1)
#define GPIO_CHANNEL_UART2_RX			(DRV_GPIO_PORTG | DRV_GPIO_PIN0)
#define GPIO_CHANNEL_UART2_PRESENTN		(DRV_GPIO_PORTG | DRV_GPIO_PIN2)


//Type definition

typedef enum
{
	DRV_UART_FLAG_FRAME = 0,
	DRV_UART_COUNT_FLAG
} drv_uart_flag;

typedef enum
{
	DRV_UART_OPERATION_WRITE = 0,
	DRV_UART_OPERATION_READ,
	DRV_UART_COUNT_OPERATION
} drv_uart_operation;

typedef struct
{
	uint32 u32_UARTBaudrate;
	uint ui_GPIOChannelTX;
	uint ui_GPIOChannelRX;
	uint ui_GPIOChannelPresentN;
	uint ui_Reserved;
	USART_TypeDef *tp_UARTChannel;
} drv_uart_profile;

typedef struct
{
	uint ui_Flag;
	uint ui_WriteCount;
	uint ui_WriteLength;
	uint ui_ReadLength;
	uint8 *u8p_WriteAddress;
	uint8 u8_BufferData[DRV_UART_COUNT_OPERATION][UART_DATA_BUFFER_SIZE];
	uint8 u8_BufferLength[DRV_UART_COUNT_OPERATION][UART_LENGTH_BUFFER_SIZE];
	uint16 u16_WriteDelay;
	uint16 u16_DelayTimer;
	lib_queue_object t_QueueData[DRV_UART_COUNT_OPERATION];
	lib_queue_object t_QueueLength[DRV_UART_COUNT_OPERATION];
	lib_frame_object t_Frame;
	drv_uart_callback t_Callback;
} drv_uart_control;


//Private variable definition

static drv_uart_control m_t_UARTControl[DRV_UART_COUNT_DEVICE_ID] = {0};

static const drv_uart_profile m_t_UARTProfile[DRV_UART_COUNT_DEVICE_ID] = 
{
	{
		UART_BAUDRATE_DEFAULT,
		GPIO_CHANNEL_UART1_TX, GPIO_CHANNEL_UART1_RX,
		GPIO_CHANNEL_UART1_PRESENTN, 0,
		USART1
	},
	{
		UART_BAUDRATE_DEFAULT,
		GPIO_CHANNEL_UART2_TX, GPIO_CHANNEL_UART2_RX,
		GPIO_CHANNEL_UART2_PRESENTN, 0,
		USART3
	}
};


//Private function declaration

static void DrvUART_EnableInterrupt
(
	uint ui_DeviceID
);
static void DrvUART_DisableInterrupt
(
	uint ui_DeviceID
);
static uint DrvUART_CheckState
(
	uint ui_DeviceID,
	uint *uip_Value
);
static void DrvUART_HandleWriteDone
(
	uint ui_DeviceID
);
static uint DrvUART_ParsePacket
(
	uint ui_DeviceID
);
static void DrvUART_TriggerPresent(void);


//Public function definition

uint DrvUART_Initialize
(
	uint ui_DeviceID
)
{
	uint i;
	uint ui_Value;
	drv_gpio_callback t_Callback;
	drv_uart_control *tp_Control;
	const drv_uart_profile *tp_Profile;
	USART_TypeDef *tp_UARTChannel;


	if (ui_DeviceID >= DRV_UART_COUNT_DEVICE_ID)
	{
		return FUNCTION_FAIL;
	}

	tp_Control = &m_t_UARTControl[ui_DeviceID];
	tp_Profile = &m_t_UARTProfile[ui_DeviceID];

	for (i = 0; i < DRV_UART_COUNT_OPERATION; i++)
	{
		if (LibQueue_Initialize(&tp_Control->t_QueueData[i], 
			tp_Control->u8_BufferData[i], 
			sizeof(tp_Control->u8_BufferData[i])) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}

		if (LibQueue_Initialize(&tp_Control->t_QueueLength[i], 
			tp_Control->u8_BufferLength[i], 
			sizeof(tp_Control->u8_BufferLength[i])) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}

	tp_Control->t_Frame.u8_Header = FRAME_HEADER;
	tp_Control->t_Frame.u8_Escape = FRAME_ESCAPE;
	ui_Value = 0;
	DrvUART_SetConfig(ui_DeviceID, DRV_UART_PARAM_FRAME, 
		(const uint8 *)&ui_Value, sizeof(ui_Value));

	ui_Value = DRV_GPIO_MODE_OUTPUT;
	DrvGPIO_SetConfig(tp_Profile->ui_GPIOChannelTX, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(tp_Profile->ui_GPIOChannelRX, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	ui_Value = DRV_GPIO_MODE_INPUT;
	DrvGPIO_SetConfig(tp_Profile->ui_GPIOChannelPresentN, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	ui_Value = 0;
	DrvGPIO_SetConfig(tp_Profile->ui_GPIOChannelRX, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(tp_Profile->ui_GPIOChannelTX, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
	ui_Value = 1;
	DrvGPIO_SetConfig(tp_Profile->ui_GPIOChannelPresentN, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);

	switch (ui_DeviceID)
	{
 		case DRV_UART_DEVICE_ID_1:
			t_Callback.fp_Interrupt = DrvUART_TriggerPresent;
			DrvGPIO_SetConfig(tp_Profile->ui_GPIOChannelPresentN, 
				DRV_GPIO_PARAM_CALLBACK, (const uint8 *)&t_Callback);
			ui_Value = 1;
			DrvGPIO_SetConfig(tp_Profile->ui_GPIOChannelPresentN, 
				DRV_GPIO_PARAM_INTERRUPT, (const uint8 *)&ui_Value);

			/* Enable USART clock */
			CLK->PCKENR1 |= CLK_PCKENR1_USART1;

			//Remap UART port to PA2 and PA3
			SYSCFG->RMPCR1 |= 0x10;
			break;

		case DRV_UART_DEVICE_ID_2:
	
			if (DrvGPIO_Read(tp_Profile->ui_GPIOChannelPresentN) == 0)
			{
				ui_Value = 0;
				DrvGPIO_SetConfig(tp_Profile->ui_GPIOChannelPresentN, 
					DRV_GPIO_PARAM_PULLUP, (const uint8 *)&ui_Value);
			}
			else
			{
				return FUNCTION_OK;
			}


			/* Enable USART clock */
			CLK->PCKENR3 |= CLK_PCKENR3_USART3;
			break;

		default:
			break;
	}

	tp_UARTChannel = tp_Profile->tp_UARTChannel; 

	
	//Baudrate setup
	tp_UARTChannel->BRR2 = (uint8)((((uint32)DRV_SYSTEM_CLOCK / 
		tp_Profile->u32_UARTBaudrate) >> 8) & 0xF0);
	tp_UARTChannel->BRR2 |= (uint8)(((uint32)DRV_SYSTEM_CLOCK / 
		tp_Profile->u32_UARTBaudrate) & 0x0F);
	tp_UARTChannel->BRR1 = (uint8)(((uint32)DRV_SYSTEM_CLOCK / 
		tp_Profile->u32_UARTBaudrate) >> 4);

	//Transmit and receive setup
	tp_UARTChannel->SR &= ~USART_SR_RXNE;
	tp_UARTChannel->CR2 |= (USART_CR2_RIEN | USART_CR2_REN | USART_CR2_TEN);

	//UART Enable
	tp_UARTChannel->CR1 &= ~USART_CR1_USARTD;
//	tp_UARTChannel->CR1 |= USART_CR1_PCEN | USART_CR1_M;

	return FUNCTION_OK;
}


uint DrvUART_SetConfig
(
	uint ui_DeviceID,
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	uint ui_Value;


	switch (ui_Parameter)
	{
		case DRV_UART_PARAM_SWITCH:
			DrvGPIO_SetConfig(m_t_UARTProfile[ui_DeviceID].ui_GPIOChannelTX, 
				DRV_GPIO_PARAM_PULLUP, u8p_Value);

			if (*((const uint *)u8p_Value) == 0)
			{	
				ui_Value = DRV_GPIO_MODE_OUTPUT;
			}
			else
			{
				ui_Value = DRV_GPIO_MODE_INPUT;
			}

			DrvGPIO_SetConfig(m_t_UARTProfile[ui_DeviceID].ui_GPIOChannelRX, 
				DRV_GPIO_PARAM_MODE, (const uint8 *)&ui_Value);
			break;

		case DRV_UART_PARAM_CALLBACK:
			Drv_Memcpy((uint8 *)&m_t_UARTControl[ui_DeviceID].t_Callback,
				u8p_Value, sizeof(m_t_UARTControl[ui_DeviceID].t_Callback));
			break;

		case DRV_UART_PARAM_FRAME:
			LibFrame_Initialize(&m_t_UARTControl[ui_DeviceID].t_Frame); 

			if (*((const uint *)u8p_Value) != 0)
			{
				REG_SET_BIT(m_t_UARTControl[ui_DeviceID].ui_Flag,
					DRV_UART_FLAG_FRAME);
			}
			else
			{
				REG_CLEAR_BIT(m_t_UARTControl[ui_DeviceID].ui_Flag,
					DRV_UART_FLAG_FRAME);
			}

			break;

		case DRV_UART_PARAM_WRITE_DELAY:
			m_t_UARTControl[ui_DeviceID].u16_WriteDelay = 
				*((const uint16 *)u8p_Value);
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint DrvUART_GetConfig
(
	uint ui_DeviceID,
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	uint ui_Value;


	switch (ui_Parameter)
	{
		case DRV_UART_PARAM_CALLBACK:
			Drv_Memcpy(u8p_Value,
				(uint8 *)&m_t_UARTControl[ui_DeviceID].t_Callback, 
				sizeof(m_t_UARTControl[ui_DeviceID].t_Callback));
			break;

		case DRV_UART_PARAM_BUSY:

			if (DrvUART_CheckState(ui_DeviceID, (uint *)u8p_Value) != 
				FUNCTION_OK)
			{
				return FUNCTION_FAIL;
			}

			break;

		case DRV_UART_PARAM_SIGNAL_PRESENT:

			if (ui_DeviceID == DRV_UART_DEVICE_ID_2)
			{
				ui_Value = 1;
				DrvGPIO_SetConfig(m_t_UARTProfile[ui_DeviceID].ui_GPIOChannelPresentN, 
					DRV_GPIO_PARAM_PULLUP, (const uint8 *)&ui_Value);
			}

			*((uint *)u8p_Value) = 
				DrvGPIO_Read(m_t_UARTProfile[ui_DeviceID].ui_GPIOChannelPresentN);

			if (ui_DeviceID == DRV_UART_DEVICE_ID_2)
			{
				if (*((uint *)u8p_Value) == 0)
				{
					ui_Value = 0;
					DrvGPIO_SetConfig(m_t_UARTProfile[ui_DeviceID].ui_GPIOChannelPresentN, 
						DRV_GPIO_PARAM_PULLUP, (const uint8 *)&ui_Value);
				}
			}

			break;

		case DRV_UART_PARAM_FRAME:

			if (REG_GET_BIT(m_t_UARTControl[ui_DeviceID].ui_Flag, 
				DRV_UART_FLAG_FRAME) != 0)
			{
				*((uint *)u8p_Value) = 1;
			}
			else
			{
				*((uint *)u8p_Value) = 0;
			}

			break;

		case DRV_UART_PARAM_WRITE_DELAY:
			*((uint16 *)u8p_Value) = m_t_UARTControl[ui_DeviceID].u16_WriteDelay;
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint DrvUART_Write
(
	uint ui_DeviceID,
	const uint8 *u8p_Data,
	uint ui_Length
)
{
	uint ui_Value;
	uint ui_WriteLength;
	lib_frame_int t_Length;
	lib_queue_object *tp_QueueData;
	lib_queue_object *tp_QueueLength;
	drv_uart_control *tp_Control;
	const drv_uart_profile *tp_Profile;


	//Check if length of data is out of range
	if ((ui_DeviceID >= DRV_UART_COUNT_DEVICE_ID) || (ui_Length == 0))
	{
		return FUNCTION_FAIL;
	}

	tp_Control = &m_t_UARTControl[ui_DeviceID];
	tp_Profile = &m_t_UARTProfile[ui_DeviceID];
	tp_QueueData = &tp_Control->t_QueueData[DRV_UART_OPERATION_WRITE];
	tp_QueueLength = &tp_Control->t_QueueLength[DRV_UART_OPERATION_WRITE];

	if (u8p_Data != (const uint8 *)0)
	{
		if (LibQueue_GetConfig(tp_QueueData, LIB_QUEUE_PARAM_TAIL, 
			(void *)&ui_Value) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}

		ui_Value = sizeof(tp_Control->u8_BufferData[DRV_UART_OPERATION_WRITE]) -
			ui_Value;

		if (REG_GET_BIT(tp_Control->ui_Flag, DRV_UART_FLAG_FRAME) != 0)
		{
			//Check if length of data is out of range
			if (ui_Length + LIB_FRAME_HEADER_LENGTH > (ui_Value / 2))
			{
				return FUNCTION_FAIL;
			}

			t_Length = (lib_frame_int)ui_Length;
			LibFrame_Pack(&tp_Control->t_Frame, u8p_Data, 
				&tp_Control->u8_BufferData[DRV_UART_OPERATION_WRITE][sizeof(tp_Control->u8_BufferData[DRV_UART_OPERATION_WRITE]) - 
				ui_Value], &t_Length);
			ui_Length = (uint)t_Length;
			u8p_Data = (const uint8 *)0;
		}
		else
		{
			//Check if length of data is out of range
			if (ui_Length > ui_Value)
			{
				return FUNCTION_FAIL;
			}
		}

		ui_WriteLength = ui_Length;
		ui_Length = sizeof(ui_WriteLength);

		if (LibQueue_PushTail(tp_QueueLength, (const uint8 *)&ui_WriteLength, 
			&ui_Length) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}

		if (LibQueue_PushTail(tp_QueueData, u8p_Data, &ui_WriteLength) != 
			FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}

		if (ui_Value < 
			sizeof(tp_Control->u8_BufferData[DRV_UART_OPERATION_WRITE]))
		{
			return FUNCTION_OK;
		}
	}

	//Check if the last writing is finished or not
	if (tp_Control->ui_WriteCount == 0)
	{
		tp_Control->ui_WriteCount++;
	}
	else
	{
		return FUNCTION_FAIL;
	}

	ui_Length = sizeof(tp_Control->ui_WriteLength);

	if (LibQueue_PeekHead(tp_QueueLength, (uint8 *)&tp_Control->ui_WriteLength, 
		&ui_Length) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (LibQueue_GetConfig(tp_QueueData, LIB_QUEUE_PARAM_HEAD, 
		(void *)&ui_Value) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	tp_Control->u8p_WriteAddress = 
		&tp_Control->u8_BufferData[DRV_UART_OPERATION_WRITE][ui_Value];
	tp_Profile->tp_UARTChannel->CR2 |= USART_CR2_TIEN;

	return FUNCTION_OK;
}


uint DrvUART_Read
(
	uint ui_DeviceID,
	uint8 *u8p_Data,
	uint *uip_Length
)
{
	uint ui_Value;
	uint ui_Length;
	lib_queue_object *tp_QueueData;
	lib_queue_object *tp_QueueLength;
	drv_uart_control *tp_Control;


	if ((ui_DeviceID >= DRV_UART_COUNT_DEVICE_ID) || (*uip_Length == 0))
	{
		return FUNCTION_FAIL;
	}

	tp_Control = &m_t_UARTControl[ui_DeviceID];
	tp_QueueData = &tp_Control->t_QueueData[DRV_UART_OPERATION_READ];
	tp_QueueLength = &tp_Control->t_QueueLength[DRV_UART_OPERATION_READ];

	if (REG_GET_BIT(tp_Control->ui_Flag, DRV_UART_FLAG_FRAME) == 0)
	{
		DrvUART_DisableInterrupt(ui_DeviceID);
		ui_Value = LibQueue_PopHead(tp_QueueData, u8p_Data, uip_Length);
		DrvUART_EnableInterrupt(ui_DeviceID);

		return ui_Value;
	}

	ui_Value = sizeof(ui_Length);

	//Check if there is any frame can be read
	DrvUART_DisableInterrupt(ui_DeviceID);
	ui_Value = LibQueue_PopHead(tp_QueueLength, (uint8 *)&ui_Length, &ui_Value);
	DrvUART_EnableInterrupt(ui_DeviceID);
   
	if (ui_Value != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (*uip_Length > ui_Length)
	{
		*uip_Length = ui_Length;
	}

	DrvUART_DisableInterrupt(ui_DeviceID);
	ui_Value = LibQueue_PopHead(tp_QueueData, u8p_Data, uip_Length);
	DrvUART_EnableInterrupt(ui_DeviceID);
   
	if (ui_Value != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	ui_Length -= *uip_Length;

	if (ui_Length > 0)
	{
		ui_Value = sizeof(ui_Length);

		DrvUART_DisableInterrupt(ui_DeviceID);
		ui_Value = LibQueue_PushHead(tp_QueueLength, (const uint8 *)&ui_Length, 
			&ui_Value);
		DrvUART_EnableInterrupt(ui_DeviceID);
	   
		if (ui_Value != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}

	DrvUART_ParsePacket(ui_DeviceID);

	return FUNCTION_OK;
}


void DrvUART_Tick
(
	uint ui_DeviceID,
	uint16 u16_TickTime
)
{
	uint ui_Value;
	uint ui_Length;
	lib_queue_object *tp_QueueData;
	lib_queue_object *tp_QueueLength;
	drv_uart_control *tp_Control;


	if (ui_DeviceID >= DRV_UART_COUNT_DEVICE_ID)
	{
		return;
	}

	tp_Control = &m_t_UARTControl[ui_DeviceID];
	tp_QueueData = &tp_Control->t_QueueData[DRV_UART_OPERATION_WRITE];
	tp_QueueLength = &tp_Control->t_QueueLength[DRV_UART_OPERATION_WRITE];

	if (LibQueue_GetConfig(tp_QueueLength, LIB_QUEUE_PARAM_BUFFER_SPACE, 
		(void *)&ui_Value) != FUNCTION_OK)
	{
		return;
	}

	if (ui_Value < sizeof(tp_Control->u8_BufferLength[DRV_UART_OPERATION_WRITE]))
	{
		tp_Control->u16_DelayTimer += u16_TickTime;

		if ((tp_Control->ui_WriteCount == 0) && (tp_Control->u16_DelayTimer > 
			tp_Control->u16_WriteDelay))
		{
			tp_Control->u16_DelayTimer = 0;
			ui_Length = sizeof(ui_Value);

			if (LibQueue_PopHead(tp_QueueLength, (uint8 *)&ui_Value, 
				&ui_Length) != FUNCTION_OK)
			{
				return;
			}

			if (LibQueue_PopHead(tp_QueueData, (uint8 *)NULL, &ui_Value) != 
				FUNCTION_OK)
			{
				return;
			}

			//Reset write buffer if there is not any remaining data
			if (LibQueue_PeekHead(tp_QueueLength, (uint8 *)&ui_Value, 
				&ui_Length) != FUNCTION_OK)
			{
				ui_Value = 1;
				LibQueue_SetConfig(tp_QueueLength, 
					LIB_QUEUE_PARAM_BUFFER_CLEAR, (void *)&ui_Value);
				LibQueue_SetConfig(tp_QueueData, 
					LIB_QUEUE_PARAM_BUFFER_CLEAR, (void *)&ui_Value);
			}

			if (tp_Control->t_Callback.fp_HandleEvent != 0)
			{
				tp_Control->t_Callback.fp_HandleEvent(ui_DeviceID, 
					DRV_UART_EVENT_WRITE_DONE, (const uint8 *)0, 0);
			}

			if (tp_Control->ui_WriteCount == 0)
			{
				//Check if there is any remaining data in write buffer
				if (LibQueue_PeekHead(tp_QueueLength, (uint8 *)&ui_Value, 
					&ui_Length) == FUNCTION_OK)
				{
					DrvUART_Write(ui_DeviceID, (const uint8 *)0, ui_Value);
				}
			}
		}
	}
}


uint DrvUART_HandleEvent
(
	uint ui_DeviceID,
	uint ui_Event,
	const uint8 *u8p_Data,
	uint ui_Length
)
{
	if (ui_DeviceID >= DRV_UART_COUNT_DEVICE_ID)
	{
		return FUNCTION_FAIL;
	}

	switch (ui_Event)
	{
		case  DRV_UART_EVENT_INTERRUPT_WRITE_DONE:
			DrvUART_HandleWriteDone(ui_DeviceID);
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


void DrvUART_Interrupt
(
	uint ui_DeviceID
)
{
	uint ui_Value;
	uint ui_Length;
	uint8 u8_Data;
	uint8 *u8p_BufferData;
	lib_frame_int t_Length;
	lib_frame_object *tp_Frame;
	lib_queue_object *tp_QueueData;
	lib_queue_object *tp_QueueLength;
	drv_uart_control *tp_Control;
	USART_TypeDef *tp_UARTChannel;


	tp_Control = &m_t_UARTControl[ui_DeviceID];
	tp_Frame = &tp_Control->t_Frame;
	tp_QueueData = &tp_Control->t_QueueData[DRV_UART_OPERATION_READ];
	tp_QueueLength = &tp_Control->t_QueueLength[DRV_UART_OPERATION_READ];
	tp_UARTChannel = m_t_UARTProfile[ui_DeviceID].tp_UARTChannel;

	if ((tp_UARTChannel->SR & USART_SR_RXNE) != 0)
	{
		u8_Data = tp_UARTChannel->DR;

		if (REG_GET_BIT(tp_Control->ui_Flag, DRV_UART_FLAG_FRAME) == 0)
		{
			ui_Length = sizeof(u8_Data);
			LibQueue_PushTail(tp_QueueData, &u8_Data, &ui_Length);
		}
		else
		{
			if (LibQueue_GetConfig(tp_QueueData, LIB_QUEUE_PARAM_TAIL, 
				(void *)&ui_Value) == FUNCTION_OK)
			{
				if (ui_Value + tp_Control->ui_ReadLength < 
					sizeof(tp_Control->u8_BufferData[DRV_UART_OPERATION_READ]) -
					1)
				{
					u8p_BufferData =
						&tp_Control->u8_BufferData[DRV_UART_OPERATION_READ][ui_Value];
					t_Length = sizeof(u8_Data);
					ui_Length = (uint)LibFrame_Unpack(tp_Frame, &u8_Data, 
						u8p_BufferData, &t_Length);
					tp_Control->ui_ReadLength += sizeof(u8_Data) - (uint)t_Length;

					if (ui_Length > 0)
					{
						tp_Control->ui_ReadLength = 0;
						ui_Value = sizeof(ui_Length);
						LibQueue_PushTail(tp_QueueLength, 
							(const uint8 *)&ui_Length, &ui_Value);
						LibQueue_PushTail(tp_QueueData, (const uint8 *)0, 
							&ui_Length);

						if (LibQueue_GetConfig(tp_QueueData, 
							LIB_QUEUE_PARAM_HEAD, (void *)&ui_Value) == 
							FUNCTION_OK)
						{
							if ((u8p_BufferData == 
								&tp_Control->u8_BufferData[DRV_UART_OPERATION_READ][ui_Value]) &&
								(tp_Control->t_Callback.fp_HandleEvent != 0))
							{
								tp_Control->t_Callback.fp_HandleEvent(ui_DeviceID,
									DRV_UART_EVENT_READ_DONE, u8p_BufferData,
									ui_Length);
							}
						}
					}
				}
				else
				{
					tp_Control->ui_ReadLength = 0;
					ui_Value = 1;
					LibQueue_SetConfig(tp_QueueLength, 
						LIB_QUEUE_PARAM_BUFFER_CLEAR, (void *)&ui_Value);
					LibQueue_SetConfig(tp_QueueData, 
						LIB_QUEUE_PARAM_BUFFER_CLEAR, (void *)&ui_Value);
					LibFrame_Initialize(tp_Frame); 
				}
			}
		}
	}

	if  ((tp_UARTChannel->SR & USART_SR_OR) != 0)
	{
		(void)tp_UARTChannel->DR;
	}

	if (((tp_UARTChannel->CR2 & USART_CR2_TIEN) != 0) && 
		((tp_UARTChannel->SR & USART_SR_TXE) != 0))
	{
		if (tp_Control->ui_WriteLength > 0)
		{
			tp_UARTChannel->DR = *tp_Control->u8p_WriteAddress;
			tp_Control->u8p_WriteAddress++;
			tp_Control->ui_WriteLength--;
		}

		if (tp_Control->ui_WriteLength == 0)
		{
			tp_UARTChannel->CR2 &= ~USART_CR2_TIEN;
			tp_UARTChannel->CR2 |= USART_CR2_TCIEN;
		}
	}

	if (((tp_UARTChannel->CR2 & USART_CR2_TCIEN) != 0) && 
		((tp_UARTChannel->SR & USART_SR_TC) != 0))
	{
		tp_UARTChannel->CR2 &= ~USART_CR2_TCIEN;

		if (tp_Control->t_Callback.fp_HandleEvent != 0)
		{
			tp_Control->t_Callback.fp_HandleEvent(ui_DeviceID, 
				DRV_UART_EVENT_INTERRUPT_WRITE_DONE, (const uint8 *)0, 0);
		}
	}
}


#if DRV_UART_TEST_ENABLE == 1

void DrvUART_Test(void)
{
	uint8 u8_Output[] = "at\r\n";


	DrvUART_Write(DRV_UART_DEVICE_ID_2, u8_Output, sizeof(u8_Output) - 1);
}

#endif


//Private function definition

static void DrvUART_EnableInterrupt
(
	uint ui_DeviceID
)
{
	Drv_EnableInterrupt();
}


static void DrvUART_DisableInterrupt
(
	uint ui_DeviceID
)
{
	Drv_DisableInterrupt();
}


static uint DrvUART_CheckState
(
	uint ui_DeviceID,
	uint *uip_Value
)
{
	uint i;
	uint ui_Value;
	drv_uart_control *tp_Control;


	tp_Control = &m_t_UARTControl[ui_DeviceID];
	*uip_Value = 0;

	for (i = 0; i < DRV_UART_COUNT_OPERATION; i++)
	{
		DrvUART_DisableInterrupt(ui_DeviceID);

		if (LibQueue_GetConfig(&tp_Control->t_QueueLength[i], 
			LIB_QUEUE_PARAM_BUFFER_SPACE, (void *)&ui_Value) != FUNCTION_OK)
		{
			DrvUART_EnableInterrupt(ui_DeviceID);
			return FUNCTION_FAIL;
		}

		if (ui_Value < sizeof(tp_Control->u8_BufferLength[i]))
		{
			*uip_Value = 1;
			DrvUART_EnableInterrupt(ui_DeviceID);
			return FUNCTION_OK;
		}

		DrvUART_EnableInterrupt(ui_DeviceID);
	}

	return FUNCTION_OK;
}


static void DrvUART_HandleWriteDone
(
	uint ui_DeviceID
)
{
	drv_uart_control *tp_Control;


	tp_Control = &m_t_UARTControl[ui_DeviceID];

	if (tp_Control->ui_WriteCount > 0)
	{				
		tp_Control->ui_WriteCount--;
	}
}


static uint DrvUART_ParsePacket
(
	uint ui_DeviceID
)
{
	uint ui_Value;
	uint ui_Length;
	lib_queue_object *tp_QueueData;
	lib_queue_object *tp_QueueLength;
	drv_uart_control *tp_Control;


	tp_Control = &m_t_UARTControl[ui_DeviceID];
	tp_QueueData = &tp_Control->t_QueueData[DRV_UART_OPERATION_READ];
	tp_QueueLength = &tp_Control->t_QueueLength[DRV_UART_OPERATION_READ];

	DrvUART_DisableInterrupt(ui_DeviceID);

	if (LibQueue_GetConfig(tp_QueueData, LIB_QUEUE_PARAM_BUFFER_SPACE, 
		(void *)&ui_Value) != FUNCTION_OK)
	{
		DrvUART_EnableInterrupt(ui_DeviceID);
		return FUNCTION_FAIL;
	}

	//Check if there is packet can be read
	if (ui_Value < sizeof(tp_Control->u8_BufferData[DRV_UART_OPERATION_READ]))
	{
		DrvUART_EnableInterrupt(ui_DeviceID);
		ui_Value = sizeof(ui_Length);

		if (LibQueue_PeekHead(tp_QueueLength, (uint8 *)&ui_Length, &ui_Value) == 
			FUNCTION_OK)
		{
			if (LibQueue_GetConfig(tp_QueueData, LIB_QUEUE_PARAM_HEAD, 
				(void *)&ui_Value) != FUNCTION_OK)
			{
				return FUNCTION_FAIL;
			}

			if (tp_Control->t_Callback.fp_HandleEvent != 0)
			{
				tp_Control->t_Callback.fp_HandleEvent(ui_DeviceID, 
					DRV_UART_EVENT_READ_DONE, 
					&tp_Control->u8_BufferData[DRV_UART_OPERATION_READ][ui_Value], 
					ui_Length);
			}
		}
	}
	else
	{
		if (tp_Control->ui_ReadLength == 0)
		{
			ui_Value = 1;
			LibQueue_SetConfig(tp_QueueLength, LIB_QUEUE_PARAM_BUFFER_CLEAR, 
				(void *)&ui_Value);
			LibQueue_SetConfig(tp_QueueData, LIB_QUEUE_PARAM_BUFFER_CLEAR, 
				(void *)&ui_Value);
		}

		DrvUART_EnableInterrupt(ui_DeviceID);
	}

	return FUNCTION_OK;
}


static void DrvUART_TriggerPresent(void)
{
	drv_uart_control *tp_Control;


	tp_Control = &m_t_UARTControl[DRV_UART_DEVICE_ID_1];

      if (tp_Control->t_Callback.fp_HandleEvent != 0)
	{
		tp_Control->t_Callback.fp_HandleEvent(DRV_UART_DEVICE_ID_1, 
			DRV_UART_EVENT_INTERRUPT_TRIGGER, (const uint8 *)0, 0);
	}
}
