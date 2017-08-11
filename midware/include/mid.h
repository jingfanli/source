/*
 * Module:	Midware
 * Author:	Lvjianfeng
 * Date:	2011.10
 */

#ifndef _MID_H_
#define _MID_H_


#include "global.h"
#include "message.h"

#include "glucose.h"
#include "button.h"
#include "bluetooth.h"


//Constant definition


//Type definition

typedef enum
{
	MID_PORT_MIDWARE = 0,
	MID_PORT_GLUCOSE,
	MID_PORT_BUTTON_LEFT,
	MID_PORT_BUTTON_CENTER,
	MID_PORT_BUTTON_RIGHT,
	MID_PORT_BLUETOOTH,
	MID_COUNT_PORT
} mid_port;


//Function declaration

uint Mid_Initialize(void);
uint Mid_HandleEvent
(
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	uint8 u8_Event
);
uint Mid_HandleCommand
(
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	message_command *tp_Command
);

#endif
