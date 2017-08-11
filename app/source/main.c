/*
 * Module:	Main function
 * Author:	Lvjianfeng
 * Date:	2011.8
 */


#include "drv.h"
#include "task.h"





int main(void)
{
	
    Drv_Initialize();
	Task_Initialize();
	Task_Process();

	this is the change

	return 0;
}
