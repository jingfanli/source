/*
 * Module:	Hardware driver
 * Author:	Lvjianfeng
 * Date:	2011.8
 */

#ifndef _DRV_H_
#define _DRV_H_


#include "global.h"
#include "message.h"

#include "drv_adc.h"
#include "drv_beep.h"
#include "drv_dac.h"
#include "drv_eeprom.h"
#include "drv_gpio.h"
#include "drv_lcd.h"
#include "drv_rtc.h"
#include "drv_uart.h"
#include "drv_flash.h"


//Constant definition

#define DRV_SYSTEM_CLOCK			16000000


//Type definition

typedef enum
{
	DRV_PORT_DRIVER = 0,
	DRV_PORT_ADC,
	DRV_PORT_BEEP,
	DRV_PORT_DAC,
	DRV_PORT_EEPROM,
	DRV_PORT_FLASH,
	DRV_PORT_GPIO,
	DRV_PORT_LCD,
	DRV_PORT_RTC,
	DRV_PORT_UART1,
	DRV_PORT_UART2,
	DRV_COUNT_PORT
} drv_port;

typedef enum
{
	DRV_PARAM_POWER_MODE = 0,
	DRV_PARAM_SLEEP_MODE,
	DRV_PARAM_VOLTAGE_VDD,
	DRV_PARAM_PVD,
	DRV_PARAM_DELAY,
	DRV_COUNT_PARAM
} drv_param;

typedef enum
{
	DRV_POWER_MODE_NORMAL = 0,
	DRV_POWER_MODE_LOW,
	DRV_POWER_MODE_OFF,
	DEV_COUNT_POWER_MODE
} drv_power_mode;

typedef enum
{
	DRV_SLEEP_MODE_HALT = 0,
	DRV_SLEEP_MODE_WAIT,
	DRV_COUNT_SLEEP_MODE
} drv_sleep_mode;
 

//Function declaration

#define Drv_EnableInterrupt()		enableInterrupts()
#define Drv_DisableInterrupt()		disableInterrupts()

uint Drv_Initialize(void);
uint Drv_HandleEvent
(
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	uint8 u8_Event
);
uint Drv_HandleCommand
(
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	message_command *tp_Command
);
uint Drv_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint Drv_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
void Drv_Delay
(
	uint16 u16_DelayTime
);
void Drv_Sleep(void);
void Drv_PVDInterrupt(void);
void Drv_RefreshWatchdog(void);
void Drv_Memcpy
(
	uint8 *u8p_Target,
	const uint8 *u8p_Source,
	uint16 u16_Length
);
void Drv_Memset
(
	uint8 *u8p_Target,
	uint8 u8_Value,
	uint16 u16_Length
);
uint Drv_Memcmp
(
	const uint8 *u8p_Target,
	const uint8 *u8p_Source,
	uint16 u16_Length
);
void Drv_EnablePower(void);
void Drv_DisablePower(void);
void Drv_SetTestPort(void);
void Drv_ClearTestPort(void);
void Drv_ToggleTestPort(void);

#endif
