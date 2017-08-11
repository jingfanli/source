/*
 * Module:	UART driver
 * Author:	Lvjianfeng
 * Date:	2012.8
 */

#ifndef _DRV_UART_H_
#define _DRV_UART_H_


#include "global.h"


//Constant define

#ifndef DRV_UART_TEST_ENABLE
#define DRV_UART_TEST_ENABLE		0
#endif


//Type definition

typedef enum
{
	DRV_UART_DEVICE_ID_1 = 0,
	DRV_UART_DEVICE_ID_2,
	DRV_UART_COUNT_DEVICE_ID
} drv_uart_device_id;

typedef enum
{
	DRV_UART_PARAM_SWITCH = 0,
	DRV_UART_PARAM_CALLBACK,
	DRV_UART_PARAM_FRAME,
	DRV_UART_PARAM_BUSY,
	DRV_UART_PARAM_WRITE_DELAY,
	DRV_UART_PARAM_SIGNAL_PRESENT,
	DRV_UART_COUNT_PARAM
} drv_uart_param;

typedef enum
{
	DRV_UART_EVENT_WRITE_DONE = 0,
	DRV_UART_EVENT_READ_DONE,
	DRV_UART_EVENT_INTERRUPT_TRIGGER,
	DRV_UART_EVENT_INTERRUPT_WRITE_DONE,
	DRV_UART_COUNT_EVENT
} drv_uart_event;

typedef uint (*drv_uart_callback_handle_event)
(
	uint ui_DeviceID,
	uint ui_Event,
	const uint8 *u8p_Data,
	uint ui_Length
);

typedef struct
{
	drv_uart_callback_handle_event fp_HandleEvent;
} drv_uart_callback;


//Function declaration

uint DrvUART_Initialize
(
	uint ui_DeviceID
);
uint DrvUART_SetConfig
(
	uint ui_DeviceID,
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint DrvUART_GetConfig
(
	uint ui_DeviceID,
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
uint DrvUART_Write
(
	uint ui_DeviceID,
	const uint8 *u8p_Data,
	uint ui_Length
);
uint DrvUART_Read
(
	uint ui_DeviceID,
	uint8 *u8p_Data,
	uint *uip_Length
);
void DrvUART_Tick
(
	uint ui_DeviceID,
	uint16 u16_TickTime
);
uint DrvUART_HandleEvent
(
	uint ui_DeviceID,
	uint ui_Event,
	const uint8 *u8p_Data,
	uint ui_Length
);
void DrvUART_Interrupt
(
	uint ui_DeviceID
);

#if DRV_UART_TEST_ENABLE == 1
void DrvUART_Test(void);
#endif

#endif
