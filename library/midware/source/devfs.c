/*
 * Module:	Device file system
 * Author:	Lvjianfeng
 * Date:	2012.2
 */


#include "devfs.h"


//Constant definition

#define DEVFS_BLOCK_COUNT_MIN			3
#define DEVFS_BLOCK_ID_INVALID			0
#define DEVFS_VOLUME_BLOCK_ID_FIRST		0
#define DEVFS_VOLUME_BLOCK_ID_SECOND	1
#define DEVFS_TIMESTAMP_INVALID			0
#define DEVFS_TIMESTAMP_MIN				1
#define DEVFS_TIMESTAMP_MAX				((devfs_int)(~0))


//Type definition

typedef enum
{
	DEVFS_FLAG_MOUNT,
	DEVFS_FLAG_DIRTY,
	DEVFS_FLAG_NO_ROLLBACK,
	DEVFS_FLAG_SECOND_VOLUME,
	DEVFS_COUNT_FLAG
} devfs_flag;

typedef struct
{
	devfs_tag t_DirtyTag;
} devfs_block_head;

typedef struct
{
	devfs_tag t_IntactTag;
} devfs_block_tail;

typedef struct
{
	devfs_int t_Reserved;
	devfs_int t_Timestamp;
	devfs_int t_BlockSize;
	devfs_int t_BlockCount;
	devfs_int t_FreeBlockCount;
	devfs_int t_FreeBlockIDHead;
	devfs_int t_FreeBlockIDTail;
	devfs_int t_FileCount;
} devfs_volume_head;

typedef struct
{
	devfs_int t_FileSize;
	devfs_int t_FileBlockID;
} devfs_file_index;

typedef struct
{
	devfs_address t_Address;
	devfs_tag t_ErasedData;
	devfs_int t_Flag;
	devfs_int t_ObsoleteBlockCount;
	devfs_int t_ObsoleteBlockID;
	devfs_int t_DataBufferBlockID;
	devfs_int t_DataBufferFileID;
	devfs_int *tp_BlockIndex;
	devfs_callback t_Callback;
	devfs_volume_head *tp_VolumeHead;
	devfs_file_index *tp_FileIndex;
	uint8 u8_VolumeBuffer[DEVFS_BLOCK_SIZE_MAX];
	uint8 u8_DataBuffer[DEVFS_BLOCK_SIZE_MAX];
} devfs_control;


//Private variable definition

static devfs_control m_t_Control[DEVFS_VOLUME_NUMBER_MAX] = {0};


//Private function declaration

static void DevFS_InitializeBlock
(
	uint8 *u8p_Block,
	devfs_int t_BlockSize,
	devfs_tag t_ErasedData
);
static uint DevFS_CheckBlock
(
	devfs_control *tp_Control,
	devfs_int t_BlockID,
	devfs_int t_BlockSize,
	devfs_tag t_TagValue
);
static uint DevFS_SwitchVolumeBlock
(
	devfs_control *tp_Control,
	devfs_int t_BlockSize,
	devfs_int t_BlockIDSource,
	devfs_int t_BlockIDTarget
);
static void DevFS_SaveDataBuffer
(
	devfs_control *tp_Control,
	devfs_int t_BlockSize
);
static uint DevFS_CheckFile
(
	devfs_int t_VolumeID,
	devfs_int t_FileID
);
static uint DevFS_RemoveFile
(
	devfs_int t_VolumeID,
	devfs_int t_FileID
);


//Public function definition

uint DevFS_Mount
(
	devfs_int t_VolumeID,
	devfs_volume_param *tp_Parameter,
	devfs_callback *tp_Callback
)
{
	devfs_int t_BlockSize;
	devfs_int t_TimestampFirst;
	devfs_int t_TimestampSecond;
	devfs_int t_FreeBlockID;
	devfs_control *tp_Control;
	devfs_volume_head *tp_VolumeHead;
	devfs_block_head *tp_BlockHead;
	devfs_block_tail *tp_BlockTail;


	//Check if volume ID is out of range
	if (t_VolumeID >= DEVFS_VOLUME_NUMBER_MAX)
	{
		return FUNCTION_FAIL;
	}

	tp_Control = &m_t_Control[t_VolumeID];

	//Return if volume has been mounted
	if (REG_GET_BIT(tp_Control->t_Flag, DEVFS_FLAG_MOUNT) != 0)
	{
		return FUNCTION_FAIL;
	}

	t_BlockSize = tp_Parameter->t_BlockSize;

	//Load the first volume block
	if (tp_Callback->fp_Read(tp_Parameter->t_Address, 
		tp_Control->u8_VolumeBuffer, &t_BlockSize) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	t_BlockSize = tp_Parameter->t_BlockSize;

	//Load the second volume block
	if (tp_Callback->fp_Read(tp_Parameter->t_Address + 
		t_BlockSize, tp_Control->u8_DataBuffer, &t_BlockSize) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	tp_VolumeHead = (devfs_volume_head *)(tp_Control->u8_VolumeBuffer + 
		sizeof(devfs_block_head));
	tp_BlockHead = (devfs_block_head *)tp_Control->u8_VolumeBuffer;
	tp_BlockTail = (devfs_block_tail *)(tp_Control->u8_VolumeBuffer + 
		t_BlockSize - sizeof(devfs_block_tail));

	//Get the timestamp of first volume block
	if ((tp_BlockHead->t_DirtyTag == (devfs_tag)~(tp_Parameter->t_ErasedData)) && 
		(tp_BlockTail->t_IntactTag == (devfs_tag)~(tp_Parameter->t_ErasedData)))
	{
		t_TimestampFirst = tp_VolumeHead->t_Timestamp;
	}
	else
	{
		t_TimestampFirst = DEVFS_TIMESTAMP_INVALID;
	}

	tp_VolumeHead = (devfs_volume_head *)(tp_Control->u8_DataBuffer + 
		sizeof(devfs_block_head));
	tp_BlockHead = (devfs_block_head *)tp_Control->u8_DataBuffer;
	tp_BlockTail = (devfs_block_tail *)(tp_Control->u8_DataBuffer + 
		t_BlockSize - sizeof(devfs_block_tail));

	//Get the timestamp of second volume block
	if ((tp_BlockHead->t_DirtyTag == (devfs_tag)~(tp_Parameter->t_ErasedData)) && 
		(tp_BlockTail->t_IntactTag == (devfs_tag)~(tp_Parameter->t_ErasedData)))
	{
		t_TimestampSecond = tp_VolumeHead->t_Timestamp;
	}
	else
	{
		t_TimestampSecond = DEVFS_TIMESTAMP_INVALID;
	}

	//Compare timestamp of two volume blocks, the volume block with newer 
	//timerstamp will be selected
	if (t_TimestampFirst < t_TimestampSecond)
	{
		if ((t_TimestampFirst == DEVFS_TIMESTAMP_INVALID) ||
			(t_TimestampSecond - t_TimestampFirst < DEVFS_TIMESTAMP_MAX - 1))
		{
			tp_Callback->fp_Memcpy(tp_Control->u8_VolumeBuffer, 
				tp_Control->u8_DataBuffer, t_BlockSize);
			REG_SET_BIT(tp_Control->t_Flag, DEVFS_FLAG_SECOND_VOLUME);
		}
		else
		{
			REG_CLEAR_BIT(tp_Control->t_Flag, DEVFS_FLAG_SECOND_VOLUME);
		}
	}
	else if (t_TimestampFirst > t_TimestampSecond)
	{
		if ((t_TimestampSecond != DEVFS_TIMESTAMP_INVALID) &&
			(t_TimestampFirst - t_TimestampSecond >= DEVFS_TIMESTAMP_MAX - 1))
		{
			tp_Callback->fp_Memcpy(tp_Control->u8_VolumeBuffer, 
				tp_Control->u8_DataBuffer, t_BlockSize);
			REG_SET_BIT(tp_Control->t_Flag, DEVFS_FLAG_SECOND_VOLUME);
		}
		else
		{
			REG_CLEAR_BIT(tp_Control->t_Flag, DEVFS_FLAG_SECOND_VOLUME);
		}
	}
	else
	{
		return FUNCTION_FAIL;
	}

	tp_VolumeHead = (devfs_volume_head *)(tp_Control->u8_VolumeBuffer + 
		sizeof(devfs_block_head));

	//Check paramters of volume head is matched
	if ((tp_VolumeHead->t_BlockSize != t_BlockSize) ||
		(tp_VolumeHead->t_BlockCount != tp_Parameter->t_BlockCount))
	{
		return FUNCTION_FAIL;
	}

	tp_Control->t_Address = tp_Parameter->t_Address;
	tp_Control->t_ErasedData = tp_Parameter->t_ErasedData;
	tp_Control->t_ObsoleteBlockCount = 0;
	tp_Control->t_ObsoleteBlockID = DEVFS_BLOCK_ID_INVALID;
	tp_Control->t_DataBufferBlockID = DEVFS_BLOCK_ID_INVALID;
	tp_Control->t_DataBufferFileID = tp_VolumeHead->t_FileCount;
	tp_Control->tp_VolumeHead = tp_VolumeHead;
	tp_Control->tp_BlockIndex = (devfs_int *)((uint8 *)tp_VolumeHead + 
		sizeof(devfs_volume_head));
	tp_Control->tp_FileIndex = 
		(devfs_file_index *)((uint8 *)(tp_Control->tp_BlockIndex) + 
		(sizeof(devfs_int) * tp_VolumeHead->t_BlockCount));
	tp_Callback->fp_Memcpy((uint8 *)&tp_Control->t_Callback, 
		(uint8 *)tp_Callback, sizeof(devfs_callback));

	t_FreeBlockID = tp_VolumeHead->t_FreeBlockIDHead;

	//Scan all free blocks in the volume and repair any corrupt blocks
	while (t_FreeBlockID != DEVFS_BLOCK_ID_INVALID)
	{
		if (DevFS_CheckBlock(tp_Control, t_FreeBlockID, t_BlockSize, 
			tp_Control->t_ErasedData) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}

		t_FreeBlockID = *(tp_Control->tp_BlockIndex + t_FreeBlockID);
	}

	REG_SET_BIT(tp_Control->t_Flag, DEVFS_FLAG_MOUNT);

	return FUNCTION_OK;
}


uint DevFS_Unmount
(
	devfs_int t_VolumeID,
	devfs_int t_Rollback
)
{

	if (t_Rollback != 0)
	{
		if (REG_GET_BIT(m_t_Control[t_VolumeID].t_Flag, 
			DEVFS_FLAG_NO_ROLLBACK) != 0)
		{
			return FUNCTION_FAIL;
		}
	}
	else
	{
		if (DevFS_Synchronize(t_VolumeID) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}

	REG_CLEAR_BIT(m_t_Control[t_VolumeID].t_Flag, DEVFS_FLAG_NO_ROLLBACK);
	REG_CLEAR_BIT(m_t_Control[t_VolumeID].t_Flag, DEVFS_FLAG_MOUNT);

	return FUNCTION_OK;
}


uint DevFS_Synchronize
(
	devfs_int t_VolumeID
)
{
	devfs_int t_BlockSize;
	devfs_int t_FreeBlockID;
	devfs_int *tp_BlockIndex;
	devfs_control *tp_Control;
	devfs_volume_head *tp_VolumeHead;


	//Check if volume ID is out of range
	if (t_VolumeID >= DEVFS_VOLUME_NUMBER_MAX)
	{
		return FUNCTION_FAIL;
	}

	tp_Control = &m_t_Control[t_VolumeID];

	//Check if volume is mounted
	if (REG_GET_BIT(tp_Control->t_Flag, DEVFS_FLAG_MOUNT) == 0)
	{
		return FUNCTION_FAIL;
	}

	//Check if volume is modified
	if (REG_GET_BIT(tp_Control->t_Flag, DEVFS_FLAG_DIRTY) == 0)
	{
		return FUNCTION_OK;
	}

	tp_BlockIndex = tp_Control->tp_BlockIndex;
	tp_VolumeHead = tp_Control->tp_VolumeHead;
	t_BlockSize = tp_VolumeHead->t_BlockSize;

	//Synchronize data buffer to involitale memory
	DevFS_SaveDataBuffer(tp_Control, t_BlockSize);
	tp_Control->t_DataBufferFileID = tp_VolumeHead->t_FileCount;

	t_FreeBlockID = tp_Control->t_ObsoleteBlockID;

	//Locate the last block of obsolete block list
	if (t_FreeBlockID != DEVFS_BLOCK_ID_INVALID)
	{
		while (*(tp_BlockIndex + t_FreeBlockID) != DEVFS_BLOCK_ID_INVALID)
		{
			t_FreeBlockID = *(tp_BlockIndex + t_FreeBlockID);
		}
	}

	//Add obsolete blocks to the free block list
	if (tp_VolumeHead->t_FreeBlockCount == 0)
	{
		tp_VolumeHead->t_FreeBlockIDHead = tp_Control->t_ObsoleteBlockID;
	}
	else
	{
		*(tp_BlockIndex + tp_VolumeHead->t_FreeBlockIDTail) = 
			tp_Control->t_ObsoleteBlockID;
	}

	//The tail of free block list points to the tail of obsolete block list
	if (t_FreeBlockID != DEVFS_BLOCK_ID_INVALID)
	{
		tp_VolumeHead->t_FreeBlockIDTail = t_FreeBlockID;
	}

	tp_VolumeHead->t_FreeBlockCount += tp_Control->t_ObsoleteBlockCount;

	//Update timestamp of the volume
	if (tp_VolumeHead->t_Timestamp >= DEVFS_TIMESTAMP_MAX)
	{
		tp_VolumeHead->t_Timestamp = DEVFS_TIMESTAMP_MIN;
	}
	else
	{
		tp_VolumeHead->t_Timestamp++;
	}

	//Synchronize volume block to involitale memory
	if (REG_GET_BIT(tp_Control->t_Flag, DEVFS_FLAG_SECOND_VOLUME) == 0)
	{
		if (DevFS_SwitchVolumeBlock(tp_Control, t_BlockSize, 
			DEVFS_VOLUME_BLOCK_ID_FIRST, DEVFS_VOLUME_BLOCK_ID_SECOND) != 
			FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}

		REG_SET_BIT(tp_Control->t_Flag, DEVFS_FLAG_SECOND_VOLUME);
	}
	else
	{
		if (DevFS_SwitchVolumeBlock(tp_Control, t_BlockSize, 
			DEVFS_VOLUME_BLOCK_ID_SECOND, DEVFS_VOLUME_BLOCK_ID_FIRST) != 
			FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}

		REG_CLEAR_BIT(tp_Control->t_Flag, DEVFS_FLAG_SECOND_VOLUME);
	}

	t_FreeBlockID = tp_Control->t_ObsoleteBlockID;

	//Erase all obsolete blocks
	while (t_FreeBlockID != DEVFS_BLOCK_ID_INVALID)
	{
		if (tp_Control->t_Callback.fp_Erase(tp_Control->t_Address + 
			(t_FreeBlockID * t_BlockSize), t_BlockSize) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}

		t_FreeBlockID = *(tp_BlockIndex + t_FreeBlockID);
	}

	tp_Control->t_ObsoleteBlockID = DEVFS_BLOCK_ID_INVALID;
	tp_Control->t_ObsoleteBlockCount = 0;
	REG_CLEAR_BIT(tp_Control->t_Flag, DEVFS_FLAG_DIRTY);

	return FUNCTION_OK;
}


uint DevFS_Format
(
	devfs_int t_VolumeID,
	devfs_volume_param *tp_Parameter,
	devfs_callback *tp_Callback
)
{
	devfs_int i;
	devfs_int *tp_BlockIndex;
	devfs_control *tp_Control;
	devfs_volume_head *tp_VolumeHead;
	devfs_file_index *tp_FileIndex;


	//Check if volume parameters are valid or not
	if ((tp_Parameter->t_BlockCount < DEVFS_BLOCK_COUNT_MIN) ||
		(tp_Parameter->t_BlockSize > DEVFS_BLOCK_SIZE_MAX) ||
		(tp_Parameter->t_BlockSize < (tp_Parameter->t_BlockCount * 
		sizeof(devfs_int)) + (sizeof(devfs_volume_head) + 
		sizeof(devfs_block_head) + sizeof(devfs_block_tail) + 
		sizeof(devfs_file_index))))
	{
		return FUNCTION_FAIL;
	}

	//Check if volume ID is out of range
	if (t_VolumeID >= DEVFS_VOLUME_NUMBER_MAX)
	{
		return FUNCTION_FAIL;
	}

	tp_Control = &m_t_Control[t_VolumeID];

	//Volume should be unmounted before formating
	if (REG_GET_BIT(tp_Control->t_Flag, DEVFS_FLAG_MOUNT) != 0)
	{
		return FUNCTION_FAIL;
	}

	//Erase all blocks in the volume
	if (tp_Callback->fp_Erase(tp_Parameter->t_Address, 
		tp_Parameter->t_BlockCount * tp_Parameter->t_BlockSize) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	//Initialize volume block buffer
	DevFS_InitializeBlock(tp_Control->u8_VolumeBuffer, 
		tp_Parameter->t_BlockSize, tp_Parameter->t_ErasedData);

	//Initialize volume head
	tp_VolumeHead = (devfs_volume_head *)(tp_Control->u8_VolumeBuffer + 
		sizeof(devfs_block_head));
	tp_VolumeHead->t_Timestamp = DEVFS_TIMESTAMP_MIN;
	tp_VolumeHead->t_BlockSize = tp_Parameter->t_BlockSize;
	tp_VolumeHead->t_BlockCount = tp_Parameter->t_BlockCount;
	tp_VolumeHead->t_FreeBlockCount = tp_Parameter->t_BlockCount - 
		(DEVFS_BLOCK_COUNT_MIN - 1);
	tp_VolumeHead->t_FreeBlockIDHead = DEVFS_BLOCK_COUNT_MIN - 1;
	tp_VolumeHead->t_FreeBlockIDTail = tp_Parameter->t_BlockCount - 1;
	tp_VolumeHead->t_FileCount = (tp_Parameter->t_BlockSize - 
		(tp_Parameter->t_BlockCount * sizeof(devfs_int)) - 
		(sizeof(devfs_volume_head) + sizeof(devfs_block_head) + 
		sizeof(devfs_block_tail))) / sizeof(devfs_file_index);

	//One block will belong to only one file. So the the number of files
	//will not exceed the number of free blocks
	if (tp_VolumeHead->t_FileCount > tp_VolumeHead->t_FreeBlockCount)
	{
		tp_VolumeHead->t_FileCount = tp_VolumeHead->t_FreeBlockCount;
	}

	//Initialize block index area
	tp_BlockIndex = (devfs_int *)((uint8 *)tp_VolumeHead + 
		sizeof(devfs_volume_head));

	//Block 0 and 1 are volume blocks and won't be added to the free block list
	*tp_BlockIndex = DEVFS_BLOCK_ID_INVALID;
	tp_BlockIndex++;
	*tp_BlockIndex = DEVFS_BLOCK_ID_INVALID;
	tp_BlockIndex++;

	//Build the free block list
	for (i = DEVFS_BLOCK_COUNT_MIN; i < tp_Parameter->t_BlockCount; i++)
	{
		*tp_BlockIndex = i;
		tp_BlockIndex++;
	}

	//Initialize the last block id of the free block list
	*tp_BlockIndex = DEVFS_BLOCK_ID_INVALID;
	tp_BlockIndex++;

	//Initialize file index area
	tp_FileIndex = (devfs_file_index *)tp_BlockIndex; 

	for (i = 0; i < tp_VolumeHead->t_FileCount; i++)
	{
		tp_FileIndex->t_FileSize = 0;
		tp_FileIndex->t_FileBlockID = DEVFS_BLOCK_ID_INVALID;
		tp_FileIndex++;
	}

	//Write volume block
	if (tp_Callback->fp_Write(tp_Parameter->t_Address, 
		tp_Control->u8_VolumeBuffer, tp_Parameter->t_BlockSize) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint DevFS_Write
(
	devfs_int t_VolumeID,
	devfs_int t_FileID,
	const uint8 *u8p_Data,
	devfs_int t_Offset,
	devfs_int t_Length
)
{
	devfs_int t_BlockSize;
	devfs_int t_BlockDataSize;
	devfs_int t_WriteLength;
	devfs_int t_FileBlockCount;
	devfs_int t_OffsetBlockCount;
	devfs_int t_FileBlockID;
	devfs_int t_FileNewBlockID;
	devfs_int t_FileOldBlockID;
	devfs_int t_WriteBlockCount;
	devfs_int t_ObsoleteBlockID;
	devfs_int t_ObsoleteBlockCount;
	devfs_int t_FreeBlockCount;
	devfs_int *tp_BlockIndex;
	devfs_control *tp_Control;
	devfs_callback *tp_Callback;
	devfs_file_index *tp_FileIndex;


	//Check if volume ID or file ID is valid or not
	if (DevFS_CheckFile(t_VolumeID, t_FileID) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	//If both length and offset are zero, file will be removed
	if (t_Length == 0)
	{
		if (t_Offset == 0)
		{
			return DevFS_RemoveFile(t_VolumeID, t_FileID);
		}
		else
		{
			return FUNCTION_FAIL;
		}
	}

	tp_Control = &m_t_Control[t_VolumeID];
	tp_Callback = &tp_Control->t_Callback;
	tp_BlockIndex = tp_Control->tp_BlockIndex;
	tp_FileIndex = tp_Control->tp_FileIndex + t_FileID;
	t_FreeBlockCount = tp_Control->tp_VolumeHead->t_FreeBlockCount;
	t_BlockSize = tp_Control->tp_VolumeHead->t_BlockSize;
	t_BlockDataSize = t_BlockSize - (sizeof(devfs_block_head) +
		sizeof(devfs_block_tail));

	//Check if file offset and length are valid or not
	if ((t_Offset > tp_FileIndex->t_FileSize) ||
		(t_Offset + t_Length > tp_FileIndex->t_FileSize + 
		(t_FreeBlockCount * t_BlockDataSize)))
	{
		return FUNCTION_FAIL;
	}

	REG_SET_BIT(tp_Control->t_Flag, DEVFS_FLAG_DIRTY);
	t_FileBlockCount = (tp_FileIndex->t_FileSize + t_BlockDataSize - 1) / 
		t_BlockDataSize;
	t_OffsetBlockCount = (t_Offset + t_BlockDataSize) / t_BlockDataSize; 
	t_ObsoleteBlockCount = (t_Offset + t_Length + t_BlockDataSize - 1) /
		t_BlockDataSize;
	t_FileOldBlockID = tp_FileIndex->t_FileBlockID;
	t_FileNewBlockID = tp_Control->tp_VolumeHead->t_FreeBlockIDHead;
	t_WriteBlockCount = 1;

	//Check if free blocks are enough
	if (t_FreeBlockCount > 0)
	{
		t_FreeBlockCount--;
	}
	else
	{
		return FUNCTION_FAIL;
	}

	//Update file size
	if (tp_FileIndex->t_FileSize < t_Offset + t_Length)
	{
		tp_FileIndex->t_FileSize = t_Offset + t_Length;
	}

	//Locate the block that is prior to the first block to be written
	while (t_WriteBlockCount < t_OffsetBlockCount - 1)
	{
		t_FileOldBlockID = *(tp_BlockIndex + t_FileOldBlockID);
		t_WriteBlockCount++;
		t_Offset -= t_BlockDataSize;
	}

	//Check if the first block to be written is the first block of file
	if (t_OffsetBlockCount == 1)
	{
		//Check if the first block to be written is in the data buffer
		if ((t_FileOldBlockID == tp_Control->t_DataBufferBlockID) &&
			(tp_Control->t_DataBufferBlockID != DEVFS_BLOCK_ID_INVALID))
		{
			t_WriteLength = t_BlockDataSize - t_Offset;

			//Check if data to be written crosses more than one block
			if (t_Length > t_WriteLength)
			{
				tp_Callback->fp_Memcpy(tp_Control->u8_DataBuffer + 
					sizeof(devfs_block_head) + t_Offset, u8p_Data, t_WriteLength);
				DevFS_SaveDataBuffer(tp_Control, t_BlockSize);
				t_FileBlockID = *(tp_BlockIndex + t_FileOldBlockID);
				*(tp_BlockIndex + t_FileOldBlockID) = t_FileNewBlockID;
				t_FileOldBlockID = t_FileBlockID;
				t_WriteBlockCount++;
				t_Offset = 0;
			}
			else
			{
				t_WriteLength = t_Length;
				tp_Callback->fp_Memcpy(tp_Control->u8_DataBuffer + 
					sizeof(devfs_block_head) + t_Offset, u8p_Data, t_WriteLength);
			}

			u8p_Data += t_WriteLength;
			t_Length -= t_WriteLength;
		}
		else
		{
			tp_FileIndex->t_FileBlockID = t_FileNewBlockID;
		}
	}
	else
	{
		t_WriteBlockCount++;
		t_Offset -= t_BlockDataSize;

		//Check if the first block to be written is in the data buffer
		if ((*(tp_BlockIndex + t_FileOldBlockID) == 
			tp_Control->t_DataBufferBlockID) &&
			(tp_Control->t_DataBufferBlockID != DEVFS_BLOCK_ID_INVALID))
		{
			t_WriteLength = t_BlockDataSize - t_Offset;
			t_FileOldBlockID = *(tp_BlockIndex + t_FileOldBlockID);

			//Check if data to be written crosses more than one block
			if (t_Length > t_WriteLength)
			{
				tp_Callback->fp_Memcpy(tp_Control->u8_DataBuffer + 
					sizeof(devfs_block_head) + t_Offset, u8p_Data, t_WriteLength);
				DevFS_SaveDataBuffer(tp_Control, t_BlockSize);
				t_FileBlockID = *(tp_BlockIndex + t_FileOldBlockID);
				*(tp_BlockIndex + t_FileOldBlockID) = t_FileNewBlockID;
				t_FileOldBlockID = t_FileBlockID;
				t_WriteBlockCount++;
				t_Offset = 0;
			}
			else
			{
				t_WriteLength = t_Length;
				tp_Callback->fp_Memcpy(tp_Control->u8_DataBuffer + 
					sizeof(devfs_block_head) + t_Offset, u8p_Data, t_WriteLength);
			}

			u8p_Data += t_WriteLength;
			t_Length -= t_WriteLength;
		}
		else
		{
			t_FileBlockID = *(tp_BlockIndex + t_FileOldBlockID);
			*(tp_BlockIndex + t_FileOldBlockID) = t_FileNewBlockID;
			t_FileOldBlockID = t_FileBlockID;
		}
	}

	//Check if data written is completed or not
	if (t_Length == 0)
	{
		return FUNCTION_OK;
	}	

	//Save data buffer first if it is not saved yet
	if (tp_Control->t_DataBufferBlockID != DEVFS_BLOCK_ID_INVALID)
	{
		DevFS_SaveDataBuffer(tp_Control, t_BlockSize);
	}

	//Check if the first block to be written is a new block or not
	if (t_FileOldBlockID != DEVFS_BLOCK_ID_INVALID)
	{
		t_ObsoleteBlockID = t_FileOldBlockID;

		if (t_ObsoleteBlockCount > t_FileBlockCount)
		{
			t_ObsoleteBlockCount = t_FileBlockCount - t_WriteBlockCount + 1;
		}
		else
		{
			t_ObsoleteBlockCount = t_ObsoleteBlockCount - t_WriteBlockCount + 1;
		}

		t_WriteLength = t_BlockSize;

		//Load the first block into data buffer
		if (tp_Callback->fp_Read(tp_Control->t_Address + 
			(t_FileOldBlockID * t_BlockSize), tp_Control->u8_DataBuffer, 
			&t_WriteLength) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}
	else
	{
		t_ObsoleteBlockID = DEVFS_BLOCK_ID_INVALID;
		t_ObsoleteBlockCount = 0;

		//Initialize data buffer for first block writing
		DevFS_InitializeBlock(tp_Control->u8_DataBuffer, t_BlockSize, 
			tp_Control->t_ErasedData);
	}

	t_WriteLength = t_BlockDataSize - t_Offset;

	//Check if data to be written exceeds more than one block
	if (t_Length > t_WriteLength)
	{
		tp_Callback->fp_Memcpy(tp_Control->u8_DataBuffer + 
			sizeof(devfs_block_head) + t_Offset, u8p_Data, t_WriteLength);

		if (tp_Callback->fp_Write(tp_Control->t_Address +
			t_FileNewBlockID * t_BlockSize, tp_Control->u8_DataBuffer,
			t_BlockSize) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}
	else
	{
		t_WriteLength = t_Length;
		tp_Callback->fp_Memcpy(tp_Control->u8_DataBuffer + 
			sizeof(devfs_block_head) + t_Offset, u8p_Data, t_WriteLength);
	}

	u8p_Data += t_WriteLength;
	t_Length -= t_WriteLength;

	//Write file data that can be fixed in a whole data block
	while (t_Length > t_BlockDataSize)
	{
		if (t_WriteBlockCount < t_FileBlockCount)
		{
			t_FileOldBlockID = *(tp_BlockIndex + t_FileOldBlockID);
		}

		t_FileNewBlockID = *(tp_BlockIndex + t_FileNewBlockID);
		tp_Callback->fp_Memcpy(tp_Control->u8_DataBuffer +
			sizeof(devfs_block_head), u8p_Data, t_BlockDataSize);
		t_WriteBlockCount++;
		
		if (t_FreeBlockCount > 0)
		{
			t_FreeBlockCount--;
		}
		else
		{
			return FUNCTION_FAIL;
		}

		if (tp_Callback->fp_Write(tp_Control->t_Address +
			t_FileNewBlockID * t_BlockSize, tp_Control->u8_DataBuffer,
			t_BlockSize) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}

		u8p_Data += t_BlockDataSize;
		t_Length -= t_BlockDataSize;
	}

	//Check if there is remaining data should be written to the last block
	if (t_Length > 0)
	{
		if (t_WriteBlockCount < t_FileBlockCount)
		{
			t_FileOldBlockID = *(tp_BlockIndex + t_FileOldBlockID);
			t_WriteLength = t_BlockSize;

			//Load the last block into data buffer
			if (tp_Callback->fp_Read(tp_Control->t_Address + 
				(t_FileOldBlockID * t_BlockSize), tp_Control->u8_DataBuffer, 
				&t_WriteLength) != FUNCTION_OK)
			{
				return FUNCTION_FAIL;
			}
		}
		else
		{
			//Initialize data buffer for last block writing
			DevFS_InitializeBlock(tp_Control->u8_DataBuffer, t_BlockSize, 
				tp_Control->t_ErasedData);
		}

		t_FileNewBlockID = *(tp_BlockIndex + t_FileNewBlockID);
		tp_Callback->fp_Memcpy(tp_Control->u8_DataBuffer + 
			sizeof(devfs_block_head), u8p_Data, t_Length);
		t_WriteBlockCount++;

		if (t_FreeBlockCount > 0)
		{
			t_FreeBlockCount--;
		}
		else
		{
			return FUNCTION_FAIL;
		}
	}

	tp_Control->t_DataBufferBlockID = t_FileNewBlockID;	
	tp_Control->t_DataBufferFileID = t_FileID;
	tp_Control->tp_VolumeHead->t_FreeBlockCount = t_FreeBlockCount;
	tp_Control->tp_VolumeHead->t_FreeBlockIDHead = 
		*(tp_BlockIndex + t_FileNewBlockID);

	//Check if there is any free block available
	if (t_FreeBlockCount == 0)
	{
		tp_Control->tp_VolumeHead->t_FreeBlockIDTail = DEVFS_BLOCK_ID_INVALID;
	}

	//Check if the last block exceeds the end of file
	if (t_WriteBlockCount < t_FileBlockCount)
	{
		*(tp_BlockIndex + t_FileNewBlockID) = *(tp_BlockIndex + t_FileOldBlockID);
	}
	else
	{
		*(tp_BlockIndex + t_FileNewBlockID) = DEVFS_BLOCK_ID_INVALID;
	}

	//Check if there is any old block should be obsoleted
	if (t_ObsoleteBlockCount > 0)
	{
		*(tp_BlockIndex + t_FileOldBlockID) = tp_Control->t_ObsoleteBlockID;
		tp_Control->t_ObsoleteBlockID = t_ObsoleteBlockID;
		tp_Control->t_ObsoleteBlockCount += t_ObsoleteBlockCount;
	}

	//Perform synchronization if obsolete blocks are more than free blocks
	if (tp_Control->t_ObsoleteBlockCount >= t_FreeBlockCount)
	{
		REG_SET_BIT(tp_Control->t_Flag, DEVFS_FLAG_NO_ROLLBACK);
		return DevFS_Synchronize(t_VolumeID);
	}

	return FUNCTION_OK;
}


uint DevFS_Read
(
	devfs_int t_VolumeID,
	devfs_int t_FileID,
	uint8 *u8p_Data,
	devfs_int t_Offset,
	devfs_int *tp_Length
)
{
	devfs_int t_Length;
	devfs_int t_ReadLength;
	devfs_int t_BlockSize;
	devfs_int t_BlockDataSize;
	devfs_int t_FileBlockID;
	devfs_int *tp_BlockIndex;
	devfs_control *tp_Control;
	devfs_callback *tp_Callback;
	devfs_file_index *tp_FileIndex;


	//Check if volume ID or file ID is valid or not
	if (DevFS_CheckFile(t_VolumeID, t_FileID) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	tp_Control = &m_t_Control[t_VolumeID];
	tp_Callback = &tp_Control->t_Callback;
	tp_FileIndex = tp_Control->tp_FileIndex + t_FileID;
	t_Length = *tp_Length;

	//If both length and offset are valid
	if ((t_Length == 0) || (t_Offset >= tp_FileIndex->t_FileSize))
	{
		return FUNCTION_FAIL;
	}

	//If length exceeds file size, trim it to file size
	if (t_Length + t_Offset > tp_FileIndex->t_FileSize)
	{
		t_Length = tp_FileIndex->t_FileSize - t_Offset;
		*tp_Length = t_Length;
	}

	tp_BlockIndex = tp_Control->tp_BlockIndex;
	t_BlockSize = tp_Control->tp_VolumeHead->t_BlockSize;
	t_BlockDataSize = t_BlockSize - (sizeof(devfs_block_head) +
		sizeof(devfs_block_tail));
	t_FileBlockID = tp_FileIndex->t_FileBlockID;

	//Locate to the head of file for reading
	while (t_Offset >= t_BlockDataSize)
	{
		t_Offset -= t_BlockDataSize;
		t_FileBlockID = *(tp_BlockIndex + t_FileBlockID);
	}

	t_ReadLength = t_BlockDataSize - t_Offset;

	//Read all blocks except for the last one
	while (t_Length > t_ReadLength)
	{
		//Check if the block to be read is in data buffer
		if (t_FileBlockID == tp_Control->t_DataBufferBlockID)
		{
			tp_Callback->fp_Memcpy(u8p_Data, tp_Control->u8_DataBuffer + 
				t_Offset + sizeof(devfs_block_head), t_ReadLength);
		}
		else
		{
			if (tp_Callback->fp_Read(tp_Control->t_Address + 
				(t_FileBlockID * t_BlockSize) + t_Offset + 
				sizeof(devfs_block_head), u8p_Data, &t_ReadLength) != 
				FUNCTION_OK)
			{
				return FUNCTION_FAIL;
			}
		}

		t_FileBlockID = *(tp_BlockIndex + t_FileBlockID);
		u8p_Data += t_ReadLength;
		t_Length -= t_ReadLength;
		t_ReadLength = t_BlockDataSize;
		t_Offset = 0;
	}	

	//Read the last block
	if (t_FileBlockID == tp_Control->t_DataBufferBlockID)
	{
		tp_Callback->fp_Memcpy(u8p_Data, tp_Control->u8_DataBuffer + t_Offset +
			sizeof(devfs_block_head), t_Length);
	}
	else
	{
		if (tp_Callback->fp_Read(tp_Control->t_Address + 
			(t_FileBlockID * t_BlockSize) + t_Offset + 
			sizeof(devfs_block_head), u8p_Data, &t_Length) != 
			FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}

	return FUNCTION_OK;
}


uint DevFS_Query
(
	devfs_int t_VolumeID,
	devfs_int t_Info,
	devfs_int *tp_Value
)
{
	devfs_control *tp_Control;
	devfs_volume_head *tp_VolumeHead;


	//Check if volume ID is out of range
	if (t_VolumeID >= DEVFS_VOLUME_NUMBER_MAX)
	{
		return FUNCTION_FAIL;
	}

	tp_Control = &m_t_Control[t_VolumeID];

	//Check if volume is mounted
	if (REG_GET_BIT(tp_Control->t_Flag, DEVFS_FLAG_MOUNT) == 0)
	{
		return FUNCTION_FAIL;
	}

	tp_VolumeHead = tp_Control->tp_VolumeHead;

	switch (t_Info)
	{
		case DEVFS_INFO_FILE_SIZE:

			//Check if file id is valid or not
			if (*tp_Value >= tp_VolumeHead->t_FileCount)
			{
				*tp_Value = 0;
				return FUNCTION_FAIL;
			}

			*tp_Value = (tp_Control->tp_FileIndex + *tp_Value)->t_FileSize;
			break;

		case DEVFS_INFO_FILE_COUNT:
			*tp_Value = tp_VolumeHead->t_FileCount;
			break;

		case DEVFS_INFO_BLOCK_SIZE:
			*tp_Value = tp_VolumeHead->t_BlockSize;
			break;

		case DEVFS_INFO_BLOCK_COUNT:
			*tp_Value = tp_VolumeHead->t_BlockCount;
			break;

		case DEVFS_INFO_FREE_BLOCK_COUNT:
			*tp_Value = tp_VolumeHead->t_FreeBlockCount;
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


//Private function definition

static void DevFS_InitializeBlock
(
	uint8 *u8p_Block,
	devfs_int t_BlockSize,
	devfs_tag t_ErasedData
)
{
	devfs_block_head *tp_BlockHead;
	devfs_block_tail *tp_BlockTail;


	//Initialize block tags
	tp_BlockHead = (devfs_block_head *)u8p_Block;
	tp_BlockTail = (devfs_block_tail *)(u8p_Block + t_BlockSize - 
		sizeof(devfs_block_tail));
	tp_BlockHead->t_DirtyTag = ~t_ErasedData;
	tp_BlockTail->t_IntactTag = ~t_ErasedData;

	t_BlockSize -= sizeof(devfs_block_head) + sizeof(devfs_block_tail);
	u8p_Block += sizeof(devfs_block_head);

	while (t_BlockSize > 0)
	{
		*u8p_Block++ = (uint8)t_ErasedData;
		t_BlockSize--;
	}
}


static uint DevFS_CheckBlock
(
	devfs_control *tp_Control,
	devfs_int t_BlockID,
	devfs_int t_BlockSize,
	devfs_tag t_TagValue
)
{
	devfs_address t_Address;
	devfs_int t_Length;
	devfs_callback *tp_Callback;
	devfs_block_head t_BlockHead;
	devfs_block_tail t_BlockTail;


	t_Length = sizeof(devfs_block_head);
	t_Address = tp_Control->t_Address + (t_BlockID * t_BlockSize);
	tp_Callback = &tp_Control->t_Callback;

	if (tp_Callback->fp_Read(t_Address, (uint8 *)&t_BlockHead, &t_Length) != 
		FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	t_Length = sizeof(devfs_block_tail);
	t_Address += t_BlockSize - sizeof(devfs_block_tail);

	if (tp_Callback->fp_Read(t_Address, (uint8 *)&t_BlockTail, &t_Length) != 
		FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	//Check if the block is corrupt or not
	if ((t_BlockHead.t_DirtyTag != t_TagValue) ||
		(t_BlockTail.t_IntactTag != t_TagValue))
	{
		t_Address -= t_BlockSize - sizeof(devfs_block_tail);

		//Erase corrupt block
		if (tp_Callback->fp_Erase(t_Address, t_BlockSize) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}

	return FUNCTION_OK;
}


static uint DevFS_SwitchVolumeBlock
(
	devfs_control *tp_Control,
	devfs_int t_BlockSize,
	devfs_int t_BlockIDSource,
	devfs_int t_BlockIDTarget
)
{
	//Check if the volume block to be written is free or not and clear it if
	//it's not free
	if (DevFS_CheckBlock(tp_Control, t_BlockIDTarget, t_BlockSize, 
		tp_Control->t_ErasedData) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (tp_Control->t_Callback.fp_Write(tp_Control->t_Address + 
		(t_BlockIDTarget * t_BlockSize), tp_Control->u8_VolumeBuffer, 
		t_BlockSize) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (tp_Control->t_Callback.fp_Erase(tp_Control->t_Address +
		(t_BlockIDSource * t_BlockSize), t_BlockSize) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


static void DevFS_SaveDataBuffer
(
	devfs_control *tp_Control,
	devfs_int t_BlockSize
)
{
	if (tp_Control->t_DataBufferBlockID == DEVFS_BLOCK_ID_INVALID)
	{
		return;
	}

	if (tp_Control->t_Callback.fp_Write(tp_Control->t_Address + 
		tp_Control->t_DataBufferBlockID * t_BlockSize, 
		tp_Control->u8_DataBuffer, t_BlockSize) != FUNCTION_OK)
	{
		return;
	}

	tp_Control->t_DataBufferBlockID = DEVFS_BLOCK_ID_INVALID;
}


static uint DevFS_CheckFile
(
	devfs_int t_VolumeID,
	devfs_int t_FileID
)
{
	devfs_control *tp_Control;


	//Check if volume ID is out of range
	if (t_VolumeID >= DEVFS_VOLUME_NUMBER_MAX)
	{
		return FUNCTION_FAIL;
	}

	tp_Control = &m_t_Control[t_VolumeID];

	//Check if volume is mounted
	if (REG_GET_BIT(tp_Control->t_Flag, DEVFS_FLAG_MOUNT) == 0)
	{
		return FUNCTION_FAIL;
	}

	//Check if file ID is out of range
	if (t_FileID >= tp_Control->tp_VolumeHead->t_FileCount)
	{
		return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


static uint DevFS_RemoveFile
(
	devfs_int t_VolumeID,
	devfs_int t_FileID
)
{
	devfs_int t_FileBlockID;
	devfs_int *tp_BlockIndex;
	devfs_control *tp_Control;
	devfs_file_index *tp_FileIndex;


	tp_Control = &m_t_Control[t_VolumeID];
	tp_FileIndex = tp_Control->tp_FileIndex + t_FileID;
	tp_BlockIndex = tp_Control->tp_BlockIndex;
	t_FileBlockID = tp_FileIndex->t_FileBlockID;

	//Check if the file is empty
	if (t_FileBlockID == DEVFS_BLOCK_ID_INVALID)
	{
		return FUNCTION_OK;
	}

	REG_SET_BIT(tp_Control->t_Flag, DEVFS_FLAG_DIRTY);

	//Locate the end block of the file
	while (*(tp_BlockIndex + t_FileBlockID) != DEVFS_BLOCK_ID_INVALID)
	{
		t_FileBlockID = *(tp_BlockIndex + t_FileBlockID);
		tp_Control->t_ObsoleteBlockCount++;
	}

	//Add all file blocks to obsolete block list and reset file index
	tp_Control->t_ObsoleteBlockCount++;
	*(tp_BlockIndex + t_FileBlockID) = tp_Control->t_ObsoleteBlockID;
	tp_Control->t_ObsoleteBlockID = tp_FileIndex->t_FileBlockID;
	tp_FileIndex->t_FileBlockID = DEVFS_BLOCK_ID_INVALID;
	tp_FileIndex->t_FileSize = 0;

	//Check if file block has been loaded into data buffer
	if (tp_Control->t_DataBufferFileID == t_FileID)
	{
		tp_Control->t_DataBufferBlockID = DEVFS_BLOCK_ID_INVALID;
		tp_Control->t_DataBufferFileID = tp_Control->tp_VolumeHead->t_FileCount;
	}

	//Perform synchronization if obsolete blocks are more than free blocks
	if (tp_Control->t_ObsoleteBlockCount >= 
		tp_Control->tp_VolumeHead->t_FreeBlockCount)
	{
		REG_SET_BIT(tp_Control->t_Flag, DEVFS_FLAG_NO_ROLLBACK);
		return DevFS_Synchronize(t_VolumeID);
	}

	return FUNCTION_OK;
}
