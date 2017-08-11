/*
 * Module:	Heap library
 * Author:	Lvjianfeng
 * Date:	2012.11
 */


#include "lib_heap.h"


//Constant definition


//Type definition

typedef struct
{
	void *vp_Previous;
	void *vp_Next;
} lib_heap_free_block;


//Private variable definition


//Public function definition

uint LibHeap_Initialize
(
	lib_heap_object *tp_Heap
)
{
	lib_heap_int *tp_BlockCount;
	lib_heap_free_block *tp_FreeBlock;


	if ((tp_Heap->t_BlockCount == 0) || (tp_Heap->t_BlockLength < 
		sizeof(lib_heap_free_block) + 2 * sizeof(lib_heap_int)))
	{
		return FUNCTION_FAIL;
	}

	//Set head block count of the block
	tp_BlockCount = (lib_heap_int *)tp_Heap->vp_HeapAddress;
	*tp_BlockCount = tp_Heap->t_BlockCount;

	//Initialize free block list
	tp_FreeBlock = (lib_heap_free_block *)((uint8 *)tp_Heap->vp_HeapAddress +
		sizeof(lib_heap_int));
	tp_FreeBlock->vp_Previous = 0;
	tp_FreeBlock->vp_Next = 0;
	tp_Heap->vp_FreeBlock = (void *)tp_FreeBlock;

	//Set tail block count of the block
	tp_BlockCount = (lib_heap_int *)((uint8 *)tp_Heap->vp_HeapAddress + 
		(tp_Heap->t_BlockCount * tp_Heap->t_BlockLength) - sizeof(lib_heap_int));
	*tp_BlockCount = tp_Heap->t_BlockCount;

	return FUNCTION_OK;
}


void *LibHeap_Allocate
(
	lib_heap_object *tp_Heap,
	lib_heap_int t_Size
)
{
	lib_heap_int t_BlockLength;
	lib_heap_int t_AllocatedCount;
	lib_heap_int *tp_BlockCount;
	lib_heap_int *tp_NewBlockCount;
	lib_heap_free_block *tp_FreeBlock;
	lib_heap_free_block *tp_NewFreeBlock;


	if ((t_Size == 0) || (tp_Heap->vp_FreeBlock == (void *)0))
	{
		return (void *)0;
	}

	t_BlockLength = tp_Heap->t_BlockLength;
	t_AllocatedCount = (t_Size + t_BlockLength + 
		(2 * sizeof(lib_heap_int) - 1)) / t_BlockLength;
	tp_FreeBlock = (lib_heap_free_block *)tp_Heap->vp_FreeBlock;
	tp_BlockCount = (lib_heap_int *)((uint8 *)tp_FreeBlock - 
		sizeof(lib_heap_int));
	tp_NewBlockCount = tp_BlockCount;

	do
	{
		if ((*tp_BlockCount >= t_AllocatedCount) &&
			(*tp_BlockCount < *tp_NewBlockCount))
		{
			tp_NewBlockCount = tp_BlockCount;

			if (*tp_BlockCount == t_AllocatedCount)
			{
				break;
			}
		}

		if (tp_FreeBlock->vp_Next != (void *)0)
		{
			tp_FreeBlock = (lib_heap_free_block *)tp_FreeBlock->vp_Next;
			tp_BlockCount = (lib_heap_int *)((uint8 *)tp_FreeBlock - 
				sizeof(lib_heap_int));
		}

	} while (tp_FreeBlock->vp_Next != (void *)0);

	//Check if there is any free block can fit the data
	if (*tp_NewBlockCount < t_AllocatedCount)
	{
		return (void *)0;
	}

	if (*tp_NewBlockCount > t_AllocatedCount)
	{
		//Adjust free block list
		tp_NewFreeBlock = (lib_heap_free_block *)((uint8 *)tp_NewBlockCount +
			t_AllocatedCount * t_BlockLength + sizeof(lib_heap_int));
		tp_FreeBlock = (lib_heap_free_block *)((uint8 *)tp_NewBlockCount +
			sizeof(lib_heap_int));
		tp_NewFreeBlock->vp_Next = tp_FreeBlock->vp_Next;
		tp_NewFreeBlock->vp_Previous = tp_FreeBlock->vp_Previous;
		tp_FreeBlock = (lib_heap_free_block *)tp_FreeBlock->vp_Previous;

		if (tp_FreeBlock != (lib_heap_free_block *)0)
		{
			tp_FreeBlock->vp_Next = (void *)tp_NewFreeBlock;
		}
		else
		{
			tp_Heap->vp_FreeBlock = (void *)tp_NewFreeBlock;
		}

		//Adjust block count
		tp_BlockCount = (lib_heap_int *)((uint8 *)tp_NewBlockCount + 
			t_AllocatedCount * t_BlockLength - sizeof(lib_heap_int));
		*tp_BlockCount = t_AllocatedCount;
		tp_BlockCount = (lib_heap_int *)((uint8 *)tp_BlockCount + 
			sizeof(lib_heap_int));
		*tp_BlockCount = *tp_NewBlockCount - t_AllocatedCount;
		tp_BlockCount = (lib_heap_int *)((uint8 *)tp_BlockCount + 
			(*tp_BlockCount * t_BlockLength) - 2 * sizeof(lib_heap_int));
		*tp_BlockCount = *tp_NewBlockCount - t_AllocatedCount;
		*tp_NewBlockCount = t_AllocatedCount;
	}
	else
	{
		//Adjust free block list
		tp_NewFreeBlock = (lib_heap_free_block *)((uint8 *)tp_NewBlockCount +
			sizeof(lib_heap_int));
		tp_FreeBlock = (lib_heap_free_block *)tp_NewFreeBlock->vp_Previous;
		tp_NewFreeBlock = (lib_heap_free_block *)tp_NewFreeBlock->vp_Next;

		if (tp_FreeBlock != (lib_heap_free_block *)0)
		{
			tp_FreeBlock->vp_Next = (void *)tp_NewFreeBlock;
		}
		else
		{
			tp_Heap->vp_FreeBlock = (void *)tp_NewFreeBlock;
		}

		if (tp_NewFreeBlock != (lib_heap_free_block *)0)
		{
			tp_NewFreeBlock->vp_Previous = (void *)tp_FreeBlock;
		}
	}
	
	return (void *)((uint8 *)tp_NewBlockCount + sizeof(lib_heap_int));
}


void LibHeap_Free
(
	lib_heap_object *tp_Heap,
	void *vp_Address
)
{
}


//Private function declaration
