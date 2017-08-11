/*
 * Module:	Heap library
 * Author:	Lvjianfeng
 * Date:	2012.11
 */

#ifndef _LIB_HEAP_H_
#define _LIB_HEAP_H_


#include "global.h"


//Constant definition


//Type definition

#ifndef lib_heap_int
#define lib_heap_int				uint
#endif

typedef struct
{
	lib_heap_int t_BlockCount;
	lib_heap_int t_BlockLength;
	void *vp_HeapAddress;
	void *vp_FreeBlock;
} lib_heap_object;


//Function declaration

uint LibHeap_Initialize
(
	lib_heap_object *tp_Heap
);
void *LibHeap_Allocate
(
	lib_heap_object *tp_Heap,
	lib_heap_int t_Size
);
void LibHeap_Free
(
	lib_heap_object *tp_Heap,
	void *vp_Address
);

#endif
