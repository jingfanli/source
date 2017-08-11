/*
 * Module:	History definition
 * Author:	Lvjianfeng
 * Date:	2015.11
 */

#ifndef _HISTORY_H_
#define _HISTORY_H_


#include "global.h"


//Constant define

#define HISTORY_STATUS_PARAMETER_SIZE	2


//Type definition

typedef enum
{
	HISTORY_URGENCY_NOTIFICATION = 0,
	HISTORY_URGENCY_ALERT,
	HISTORY_URGENCY_ALARM,
	HISTORY_COUNT_URGENCY
} history_urgency;

typedef struct
{
	uint8 u8_Year;
	uint8 u8_Month;
	uint8 u8_Day;
	uint8 u8_Hour;
	uint8 u8_Minute;
	uint8 u8_Second;
} history_date_time;

typedef struct
{
	uint8 u8_Parameter[HISTORY_STATUS_PARAMETER_SIZE];
	uint16 u16_Parameter[HISTORY_STATUS_PARAMETER_SIZE];
} history_status;

typedef struct
{
	uint16 u16_Index;
	uint8 u8_Port;
	uint8 u8_Type;
	uint8 u8_Urgency;
	uint8 u8_Value;	
} history_event;

typedef struct
{
	history_date_time t_DateTime;
	history_status t_Status;
	history_event t_Event;
} history;

#endif
