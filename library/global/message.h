/*
 * Module:	Message definition
 * Author:	Lvjianfeng
 * Date:	2013.7
 */

#ifndef _MESSAGE_H_
#define _MESSAGE_H_


#include "global.h"


//Constant define

#define MESSAGE_MAJOR_PORT_OFFSET		4
#define MESSAGE_MINOR_PORT_OFFSET		0

#define MESSAGE_GET_MAJOR_PORT(port)	REG_READ_FIELD((port), \
										MESSAGE_MAJOR_PORT_OFFSET, \
										REG_MASK_4_BIT)
#define MESSAGE_GET_MINOR_PORT(port)	REG_READ_FIELD((port), \
										MESSAGE_MINOR_PORT_OFFSET, \
										REG_MASK_4_BIT)
#define MESSAGE_GET_PORT(major, minor)	((major) << MESSAGE_MAJOR_PORT_OFFSET) | \
										((minor) << MESSAGE_MINOR_PORT_OFFSET)


//Type definition

typedef enum
{
	MESSAGE_ADDRESS_MASTER = 0,
	MESSAGE_ADDRESS_SLAVE,
	MESSAGE_COUNT_ADDRESS
} message_address;

typedef enum
{
	MESSAGE_PORT_TASK = 0,
	MESSAGE_PORT_MIDWARE,
	MESSAGE_PORT_DRIVER,
	MESSAGE_COUNT_PORT
} message_port;

typedef enum
{
	MESSAGE_OPERATION_EVENT = 0,
	MESSAGE_OPERATION_SET,
	MESSAGE_OPERATION_GET,
	MESSAGE_OPERATION_WRITE,
	MESSAGE_OPERATION_READ,
	MESSAGE_OPERATION_NOTIFY,
	MESSAGE_OPERATION_ACKNOWLEDGE,
	MESSAGE_COUNT_OPERATION
} message_operation;

typedef enum
{
	MESSAGE_MODE_ACKNOWLEDGEMENT = 0,
	MESSAGE_MODE_NO_ACKNOWLEDGEMENT,
	MESSAGE_COUNT_MODE
} message_mode;

typedef enum
{
	MESSAGE_EVENT_SEND_DONE = 0,
	MESSAGE_EVENT_ACKNOWLEDGE,
	MESSAGE_EVENT_TIMEOUT,
	MESSAGE_EVENT_RECEIVE_DONE,
	MESSAGE_COUNT_EVENT
} message_event;

typedef enum
{
	MESSAGE_COMMAND_OFFSET_OPERATION = 0,
	MESSAGE_COMMAND_OFFSET_PARAMETER,
	MESSAGE_COUNT_COMMAND_OFFSET
} message_command_offset;

typedef struct
{
	uint8 u8_Operation;
	uint8 u8_Parameter;
	uint8 *u8p_Data;
	uint8 u8_Length;
} message_command;

#endif
