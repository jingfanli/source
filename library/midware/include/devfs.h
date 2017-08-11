/*
 * Module:	Device file system
 * Author:	Lvjianfeng
 * Date:	2012.2
 */

#ifndef _DEVFS_H_
#define _DEVFS_H_


#include "global.h"


//Constant definition

#ifndef DEVFS_VOLUME_NUMBER_MAX
#define DEVFS_VOLUME_NUMBER_MAX			1
#endif

#ifndef DEVFS_BLOCK_SIZE_MAX
#define DEVFS_BLOCK_SIZE_MAX			256
#endif


//Type definition

#ifndef devfs_address
#define devfs_address 					uint8*
#endif

#ifndef devfs_int
#define devfs_int 						uint
#endif

#ifndef devfs_tag
#define devfs_tag 						uint16
#endif

typedef void (*devfs_callback_memcpy)
(
	uint8 *u8p_Target,
	const uint8 *u8p_Source,
	devfs_int t_Length
);

typedef uint (*devfs_callback_write)
(
	devfs_address t_Address,
	const uint8 *u8p_Data,
	devfs_int t_Length
);

typedef uint (*devfs_callback_read)
(
	devfs_address t_Address,
	uint8 *u8p_Data,
	devfs_int *tp_Length
);

typedef uint (*devfs_callback_erase)
(
	devfs_address t_Address,
	devfs_int t_Length
);

typedef enum
{
	DEVFS_INFO_FILE_SIZE = 0,
	DEVFS_INFO_FILE_COUNT,
	DEVFS_INFO_BLOCK_SIZE,
	DEVFS_INFO_BLOCK_COUNT,
	DEVFS_INFO_FREE_BLOCK_COUNT,
	DEVFS_COUNT_INFO
} devfs_info;

typedef struct
{
	devfs_callback_memcpy fp_Memcpy;
	devfs_callback_write fp_Write;
	devfs_callback_read fp_Read;
	devfs_callback_erase fp_Erase;
} devfs_callback;

typedef struct
{
	devfs_address t_Address;
	devfs_int t_BlockSize;
	devfs_int t_BlockCount;
	devfs_tag t_ErasedData;
} devfs_volume_param;


//Function declaration

uint DevFS_Mount
(
	devfs_int t_VolumeID,
	devfs_volume_param *tp_Parameter,
	devfs_callback *tp_Callback
);
uint DevFS_Unmount
(
	devfs_int t_VolumeID,
	devfs_int t_Rollback
);
uint DevFS_Synchronize
(
	devfs_int t_VolumeID
);
uint DevFS_Format
(
	devfs_int t_VolumeID,
	devfs_volume_param *tp_Parameter,
	devfs_callback *tp_Callback
);
uint DevFS_Write
(
	devfs_int t_VolumeID,
	devfs_int t_FileID,
	const uint8 *u8p_Data,
    devfs_int t_Offset,
    devfs_int t_Length
);
uint DevFS_Read
(
	devfs_int t_VolumeID,
	devfs_int t_FileID,
	uint8 *u8p_Data,
    devfs_int t_Offset,
    devfs_int *tp_Length
);
uint DevFS_Query
(
	devfs_int t_VolumeID,
	devfs_int t_Info,
	devfs_int *tp_Value
);

#endif
