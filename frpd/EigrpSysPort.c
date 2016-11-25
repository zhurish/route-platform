/************************************************************************************

	(C) COPYRIGHT 2005-2007 by Beijing Jointbest System Technology Co. Ctd.
						All rights reserved.

	This software is confidential and proprietary to Beijing Jointbest System Technology Co. Ctd.
	No part of this software may be reproduced, stored, transmitted, disclosed or used in any 
	form or by any means other than as expressly provided by the written license agreement
	between Beijing Jointbest System Technology Co. Ctd. and its licensee.

	JOINTBEST MAKES NO OTHER WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT
	LIMITATION WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
	WITH REGARD TO THIS SOFTWARE OR ANY RELATED MATERIALS.

	IN NO EVENT SHALL JOINTBEST BE LIABLE FOR ANY INDIRECT, SPECIAL, OR CONSEQUENTIAL
	DAMAGES IN CONNECTION WITH OR ARISING OUT OF THE USE OF, OR INABILITY TO USE,
	THIS SOFTWARE, WHETHER BASED ON BREACH OF CONTRACT, TORT (INCLUDING 
	NEGLIGENCE), PRODUCT LIABILITY, OR OTHERWISE, AND WHETHER OR NOT IT HAS BEEN
	ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

					IMPORTANT LIMITATION(S) ON USE

	The use of this software is limited to the Use set 	forth in the written License Agreement 
	between Jointbest and its Licensee. Among other things, the Use of this software may be limited
	to a particular type of Designated Equipment. Before any installation, use or transfer of this 
	software, please consult the written License Agreement or contact Jointbest at the location set 
	forth below in order to confirm that you are engaging in a permissible Use of the software.

	Beijing Jointbest System Technology Co. Ctd.
	1th, 10th Floor, A Building, Changyin Plaza,
	Yongding Road, Haidian District, Beijing, China

	Tel:	+86 10 58896099
	Fax:	+86 10 58895234
	Web: www.jointbest.com

************************************************************************************/

/************************************************************************************

	Module:	EigrpSysport

	Name:	EigrpSysport.c

	Desc:	

	Rev:	
	
***********************************************************************************/
//#include	"../config.h"
#include	"./include/EigrpPreDefine.h"
#include	"./include/EigrpSysPort.h"
#include	"./include/EigrpCmd.h"
#include	"./include/EigrpUtil.h"
#include	"./include/EigrpDual.h"
#include	"./include/Eigrpd.h"
#include	"./include/EigrpIntf.h"
#include	"./include/EigrpIp.h"
#include	"./include/EigrpMain.h"
#include	"./include/EigrpPacket.h"
#ifdef _EIGRP_PLAT_MODULE
#include	"./include/EigrpZebra.h"
#endif// _EIGRP_PLAT_MODULE

extern	Eigrp_pt	gpEigrp;

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
extern	struct lib_globals *nzg;
extern	int	errorFd;
extern	int	errorProto;
#endif//#ifdef EIGRP_PLAT_ZEBOS
#endif

#ifdef _EIGRP_PLAT_MODULE
static int (* Eigrp_Route_handle)(int type, ZebraEigrpRt_pt rt);
#endif//_EIGRP_PLAT_MODULE
/************************************************************************************

	Name:	EigrpPortTaskStart

	Desc:	This function is to start an eigrp system task.It is called by the system main task 
			when it initializes the protocols. It is usually happened before all the command line
			processing.
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/

int EigrpPortTaskStart(S8 *name, int pri, void(*func)())
{
#if(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	{
		int	taskId;
		
		EIGRP_FUNC_ENTER(EigrpPortTaskStart);
		taskId = taskSpawn(name, pri, 0, 50000,  (FUNCPTR)func, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		EIGRP_FUNC_LEAVE(EigrpPortTaskStart);

		return	taskId;
	}
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	{
		S32 retVal;
		void	*handle = NULL;
		EIGRP_FUNC_ENTER(EigrpPortTaskStart);
		retVal =	PILProcCreate(0, (void(*)(void *))func,
								NULL,
								name,
								SYSTEM_PROCESS_QUEUE_LEVEL_NORMAL,
								&handle,
								NULL);
		if(retVal == FAILURE){
			EIGRP_FUNC_LEAVE(EigrpPortTaskStart);
			return	0;
		}
		EIGRP_FUNC_LEAVE(EigrpPortTaskStart);
		return	handle;
}
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	{
/* Start Edit By  : AuthorName:zhurish : 2016/01/16 16 :50 :14  : Comment:????:.linux???????????? */
		S32 retVal = 1;	
		/*	
		if(pri)	
			retVal = fock();
		if(retVal <= 0)//??????????????
			return FAILURE;
		*/
		//if(pri)		
		//	daemon (0, 0);	
#ifdef EIGRP_PLAT_ZEBRA
		zebraEigrpMasterPthreadInit();
#endif// EIGRP_PLAT_ZEBRA		
/* End Edit By  : AuthorName:zhurish : 2016/01/16 16 :50 :14  */
		func();
	}
#endif
}

/************************************************************************************

	Name:	EigrpPortTaskDelete

	Desc:	This function is stop/delete the eigrp system task created by the function 
			EigrpPortTaskStart.  It is recommended that this function not being called, for we
			don't think the eigrp task needs to be deleted before the equipment is switched off.
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortTaskDelete()
{
	EigrpPortAssert((U32)gpEigrp->taskContainer, "");
	
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	{
		void	*temp;

		EIGRP_FUNC_ENTER(EigrpPortTaskDelete);
		temp		= gpEigrp->taskContainer;
		gpEigrp->taskContainer	= NULL;

		EigrpPortSleep(500);
		taskDelete((int)temp);
	}
#elif(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	{
		EIGRP_FUNC_ENTER(EigrpPortTaskDelete);
		gpEigrp->taskContainer	= NULL;

	}
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	{
		EIGRP_FUNC_ENTER(EigrpPortTaskDelete);
		gpEigrp->taskContainer	= NULL;
	}
#endif
	EIGRP_FUNC_LEAVE(EigrpPortTaskDelete);

	return;
}

/************************************************************************************

	Name:	EigrpPortMemFree

	Desc:	This function is to free a block of memory. The memory block should be alloced from
 			system before and has not been freed.
		
	Para: 	point	- pointer to the memory block to be freed
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortMemFree(void *point)
{
	EIGRP_FUNC_ENTER(EigrpPortMemFree);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	free(point);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	PILMemFree(point);
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	free(point);
#endif
	EIGRP_FUNC_LEAVE(EigrpPortMemFree);

	return;
}

/************************************************************************************

	Name:	EigrpPortMemMalloc

	Desc:	This function is alloc a block of memory from the system.	It is recommended that this
			block of memory be cleaned to zero before this function returns.
		
	Para: 	length 	- the length of the new memory block
	
	Ret:		
************************************************************************************/

void *EigrpPortMemMalloc(U32 length)
{	
	U8 *temp;
	EIGRP_FUNC_ENTER(EigrpPortMemMalloc);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	temp	= (U8 *)calloc(1, length);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	temp	= (U8 *)PILMemMalloc(length);
	if(temp){
		EigrpPortMemSet(temp, 0, length);
	}
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	temp	= (U8 *)calloc(1, length);
#endif
	EIGRP_FUNC_LEAVE(EigrpPortMemMalloc);

	return(temp);
}

/************************************************************************************

	Name:	EigrpPortMemCpy

	Desc:	This function is to copy a given length of memory from the point of 'src' to the point
			of 'dest'.
		
	Para: 	dest		- pointer to the destination memory block 
			src		- pointer to the source	memory block
			length 	- length of the memory for copying
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortMemCpy(U8 *dest, U8 *src, U32 length)
{
	EIGRP_FUNC_ENTER(EigrpPortMemCpy);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	memcpy((void *)dest, (const void *)src, length);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	PILMemCpy(dest, src, length);
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	memcpy((void *)dest, (const void *)src, length);
#endif
	EIGRP_FUNC_LEAVE(EigrpPortMemCpy);
	
	return;
}

/************************************************************************************

	Name:	EigrpPortMemCmp

	Desc:	This function  compares successive elements contained in the two memblock.	If all 
			elements are same, the zero will be return.
		
	Para: 	memA		- pointer to one memblock
			memB		- pointer to the other memblock
			length		- the length of the memory should be compared
	
	Ret:		0	for the two elements are same
			integer(not zero)	for it is not so 
************************************************************************************/

S32	EigrpPortMemCmp(U8 *memA, U8 *memB, U32 length)
{
	S32 retVal;
	
	EIGRP_FUNC_ENTER(EigrpPortMemCmp);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	retVal	= memcmp((const void *)memA, (const void *)memB, length);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	retVal	= PILMemCmp(memA, memB, length);
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	retVal	= memcmp((const void *)memA, (const void *)memB, length);
#endif
	EIGRP_FUNC_LEAVE(EigrpPortMemCmp);
	
	return retVal;
}

/************************************************************************************

	Name:	EigrpPortMemSet

	Desc:	This function is set all the bytes of a given memory to a given value. The given memory
			starts from the param 'buff' and its length is the param 'length'.
			
	Para: 	buff		- pointer to the given memory
			value	- value to be set for all the bytes of the given memory
			length	- length of the memory to be set
	
	Ret:		
************************************************************************************/

void	EigrpPortMemSet(U8 *buff, U8 value, U32 length)
{
	EIGRP_FUNC_ENTER(EigrpPortMemSet);
#if(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	memset(buff, value, length);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	PILMemSet(buff, value, length);
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	memset(buff, value, length);
#endif
	EIGRP_FUNC_LEAVE(EigrpPortMemSet);
	
	return;
}

/************************************************************************************

	Name:	EigrpPortStrLen

	Desc:	This function is to calculate the number of the given string, not including the EOS.
		
	Para: 	str		- pointer to the given string
	
	Ret:		string length
************************************************************************************/

U32	EigrpPortStrLen(U8 *str)
{
	U32 retVal;
	
	EIGRP_FUNC_ENTER(EigrpPortStrLen);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	retVal	= strlen(str);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	retVal	= PILStringLength(str);
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	retVal	= strlen(str);
#endif
	EIGRP_FUNC_LEAVE(EigrpPortStrLen);
	
	return retVal;
}
/************************************************************************************

	Name:	EigrpPortStrCpy

	Desc:	This function copies all the characters in the string 'src' to the memory pointed by the 
			param 'dest', including the EOS.
			
	Para: 	dest		- pointer to the destination memory
			src		- pointer to the source string
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortStrCpy(U8 *dest, U8 *src)
{
	EIGRP_FUNC_ENTER(EigrpPortStrCpy);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	strcpy(dest, (const U8 *)src);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	PILStringCopy(dest, src);
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	strcpy(dest, (const U8 *)src);
#endif
	EIGRP_FUNC_LEAVE(EigrpPortStrCpy);
	
	return;
}

/************************************************************************************

	Name:	EigrpPortStrCmp

	Desc:	This function  compares all the characters in the two given string.
			If all elements are same, the zero will be return.
			
	Para: 	strA		- pointer to one string
			strB		- pointer to the other string
	
	Ret:		0	for they are same
			integer(not zero)	for it is not so
************************************************************************************/

S32	EigrpPortStrCmp(U8 *strA, U8 *strB)
{
	S32	retVal;
	
	EIGRP_FUNC_ENTER(EigrpPortStrCmp);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	retVal = strcmp(strA, strB);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	retVal = PILStringCmp(strA, strB);
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	retVal = strcmp(strA, strB);
#endif
	EIGRP_FUNC_LEAVE(EigrpPortStrCmp);
	
	return retVal;
}

/************************************************************************************

	Name:	EigrpPortStrCat

	Desc:	This function is to append the string 'src' to the end of the string 'dest'.
		
	Para: 	dest		- pointer to the destination string
			src		- pointer to the source string
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortStrCat(U8 *dest, U8 *src)
{
	EIGRP_FUNC_ENTER(EigrpPortStrCat);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	strcat(dest, src);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	PILStringCat(dest, src);
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	strcat(dest, src);
#endif
	EIGRP_FUNC_LEAVE(EigrpPortStrCat);
	
	return;
}

/************************************************************************************

	Name:	EigrpPortSemBCreate

	Desc:	This function is to create a binary semaphore and returns it .
	
 	Para: 	full		- indicates that whether it is initialized to be usable or not.
	
	Ret:		NONE		
************************************************************************************/

void	*EigrpPortSemBCreate(S32 full)
{
	void	*pSem;

	EIGRP_FUNC_ENTER(EigrpPortSemBCreate);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	pSem	= semBCreate(SEM_Q_FIFO, (full == TRUE) ? SEM_FULL : SEM_EMPTY);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	pSem	= EigrpPortMemMalloc(sizeof(S32));
	*(S32 *)pSem	= (full == TRUE) ? FALSE : TRUE;
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	pSem	= EigrpPortMemMalloc(sizeof(S32));
	*(S32 *)pSem	= (full == TRUE) ? FALSE : TRUE;
#endif
	EIGRP_FUNC_LEAVE(EigrpPortSemBCreate);
	return pSem;
}

/************************************************************************************

	Name:	EigrpPortSemBGive

	Desc:	This function is to release a binary semaphore.
		
	Para: 	pSem	- pointer to the binary semaphore
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortSemBGive(void	*pSem)
{
	EIGRP_FUNC_ENTER(EigrpPortSemBGive);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	semGive((SEM_ID)pSem);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	*(S32 *)pSem	= FALSE;
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	*(S32 *)pSem    = FALSE;
#endif
	EIGRP_FUNC_LEAVE(EigrpPortSemBGive);

	return;
}

/************************************************************************************

	Name:	EigrpPortSemBTake

	Desc:	This function is to take a binary semaphore. If this semaphore is occupied, this
			function will wait. 
		
	Para: 	pSem	- pointer the the binary semaphore
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortSemBTake(void *pSem)
{
	EIGRP_FUNC_ENTER(EigrpPortSemBTake);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	semTake((SEM_ID)pSem, WAIT_FOREVER);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	while(*(S32 *)pSem == TRUE){
		EigrpPortSleep(1);
	}
	*(S32 *)pSem	= TRUE;
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	while(*(S32 *)pSem == TRUE){
		EigrpPortSleep(1);
	}
	*(S32 *)pSem    = TRUE;
#endif
	EIGRP_FUNC_LEAVE(EigrpPortSemBTake);

	return;
}

/************************************************************************************

	Name:	EigrpPortSemBDelete

	Desc:	This function is to release and delete a binary Semaphore.
		
	Para: 	pSem	- pointer the the binary semaphore
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortSemBDelete(void *pSem)
{
	EIGRP_FUNC_ENTER(EigrpPortSemBDelete);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	semDelete((SEM_ID)pSem);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	EigrpPortMemFree(pSem);
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	EigrpPortMemFree(pSem);
#endif
	EIGRP_FUNC_LEAVE(EigrpPortSemBDelete);

	return;
}

/************************************************************************************

	Name:	EigrpPortAtoi

	Desc:	This function is to transform a string into an integer.
		
	Para: 	string	- pointer to the string 
	
	Ret:		the integer 
************************************************************************************/

S32	EigrpPortAtoi(U8 *string)
{
	S32	retVal;
	
	EIGRP_FUNC_ENTER(EigrpPortAtoi);
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	retVal	= atoi(string);
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	retVal	= atoi(string);
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	retVal	= PILPortAtoi(string);
#endif
	EIGRP_FUNC_LEAVE(EigrpPortAtoi);

	return retVal;
}


/************************************************************************************

	Name:	EigrpPortTimerCreate

	Desc:	This function is to get a one time system timer and start it.
		
	Para: 	callBk	- pointer to the callback function when the timer is overtime
			para		- parameter fo callback function
			sec		- the time when timer is overtime
	
	Ret:		pointer to the new timer
************************************************************************************/

void	*EigrpPortTimerCreate(void (*callBk)(void *), void *para, U32 sec)
{ 
#ifdef _EIGRP_PLAT_MODULE_TIMER
	int i = 0;
	for(i = 0; i < _EIGRP_TIMER_MAX; i++)
	{
		if(gpEigrp && gpEigrp->timerHdr[i].used == TRUE)//????????
		{
			if( (gpEigrp->timerHdr[i].func == callBk)&&
				(gpEigrp->timerHdr[i].para == para)&&//????????
				(gpEigrp->timerHdr[i].tv.tv_sec = sec) )//??????
			return &gpEigrp->timerHdr[i];
			//return (i+1);
		}
	}
	for(i = 0; i < _EIGRP_TIMER_MAX; i++)
	{
		if(gpEigrp && gpEigrp->timerHdr[i].used == FALSE)//????????
		{
			gpEigrp->timerHdr[i].used = TRUE;//??????
			gpEigrp->timerHdr[i].active = FALSE;//??????
			gpEigrp->timerHdr[i].func = callBk;//??????
			gpEigrp->timerHdr[i].para = para;//????????

			gpEigrp->timerHdr[i].tv.tv_sec = sec;//??????
			gpEigrp->timerHdr[i].tv.tv_usec = 0;
			gpEigrp->timerHdr[i].interuppt_tv.tv_sec = 0;
			gpEigrp->timerHdr[i].interuppt_tv.tv_usec = 0;
			EigrpUtilTimervalGet(&(gpEigrp->timerHdr[i].interuppt_tv));//??????
			return &gpEigrp->timerHdr[i];
			//return (i+1);
		}
	}
	_EIGRP_DEBUG("%s: NULL\n",__func__);
	return NULL;
#endif// _EIGRP_PLAT_MODULE_TIMER
	
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	{
		S32 retVal;
		timer_t	timer;
		struct itimerspec timerValue;
		
		EIGRP_FUNC_ENTER(EigrpPortTimerCreate);

		retVal	= timer_create(CLOCK_REALTIME, NULL, &timer);
		if(retVal != 0){
			EIGRP_FUNC_LEAVE(EigrpPortTimerCreate);
			return NULL;
		}
		
		retVal	= timer_connect(timer, (VOIDFUNCPTR)callBk, (S32)para);
		if(retVal != 0){
			EIGRP_FUNC_LEAVE(EigrpPortTimerCreate);
			return NULL;
		}

		timerValue.it_value.tv_sec		= sec;
		timerValue.it_value.tv_nsec		= 0;
		timerValue.it_interval.tv_nsec	= 0;
		timerValue.it_interval.tv_nsec	= 0;
		retVal	= timer_settime(timer, 0, &timerValue, NULL);
		if(retVal != 0){
			EIGRP_FUNC_LEAVE(EigrpPortTimerCreate);
			return NULL;
		}

		EIGRP_FUNC_LEAVE(EigrpPortTimerCreate);
		return timer;
	
	}
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	{	
		void *timer;

		EIGRP_FUNC_ENTER(EigrpPortTimerCreate);
		timer	= PILTimerCreate(PIL_TIMER_PRIORITY_NORMAL, sec * 1000, callBk, para, "DOT1X_1S_TIMER");
		EIGRP_FUNC_LEAVE(EigrpPortTimerCreate);
	
		return timer;
	}
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
#ifdef EIGRP_PLAT_ZEBRA
	//return thread_add_timer (EigrpMaster->master, callBk, para, sec);
	return NULL;
#else //EIGRP_PLAT_ZEBRA
	{
	
		EigrpLinuxTimer_pt pTimer, pTem;

		EIGRP_FUNC_ENTER(EigrpPortTimerCreate);

		pTimer	= EigrpPortMemMalloc(sizeof(EigrpLinuxTimer_st));
		if(!pTimer){
			EIGRP_FUNC_LEAVE(EigrpPortTimerCreate);
			return NULL;
		}
		EigrpPortMemSet((U8 *)pTimer, 0, sizeof(EigrpLinuxTimer_st));
		pTimer->trigSec	= sec + EigrpPortGetTimeSec();
		pTimer->func		= callBk;
		pTimer->para		= para;
		pTimer->periodSec	= sec;

		EigrpPortSemBTake(gpEigrp->timerSem);
		for(pTem = gpEigrp->timerHdr; pTem; pTem = pTem->forw){
			if(pTem->trigSec < pTimer->trigSec){
				break;
			}
		}

		EigrpUtilDllInsert((EigrpDll_pt)pTimer, (EigrpDll_pt)pTem, (EigrpDll_pt *)&gpEigrp->timerHdr);
		EigrpPortSemBGive(gpEigrp->timerSem);

		EIGRP_FUNC_LEAVE(EigrpPortTimerCreate);
		return pTimer;
	}
#endif	//EIGRP_PLAT_ZEBRA
#endif
}

/************************************************************************************

	Name:	EigrpPortTimerDelete

	Desc:	This function is to delete a timer applied from system.
		
	Para: 	timer	- pointer to the timer to be deleted
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortTimerDelete(void **timer)
{
#ifdef _EIGRP_PLAT_MODULE_TIMER
	
	EigrpLinuxTimer_pt  et = (EigrpLinuxTimer_pt )(*timer);
	if(et && et->used == TRUE)
		et->used = FALSE;
	
	/*
	int index = (int)*timer;
	if(gpEigrp && index < _EIGRP_TIMER_MAX)//????????
	{
		gpEigrp->timerHdr[index].used = FALSE;//??????
		gpEigrp->timerHdr[index].active = FALSE;//??????
	}
	*/
	*timer = NULL;
	return;
#endif// _EIGRP_PLAT_MODULE_TIMER
	
	EIGRP_FUNC_ENTER(EigrpPortTimerDelete);
	
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	timer_delete((timer_t)*timer);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	PILTimerDelete(*timer);
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
#ifdef EIGRP_PLAT_ZEBRA
	//if(*timer)
	//	thread_cancel ((struct thread *)*timer);
#else //EIGRP_PLAT_ZEBRA
	{
		EigrpLinuxTimer_pt pTimer;

		EigrpPortSemBTake(gpEigrp->timerSem);
		for(pTimer = gpEigrp->timerHdr; pTimer; pTimer = pTimer->forw){
			if(pTimer == *timer){
				break;
			}
		}
		if(pTimer){
			EigrpUtilDllRemove((EigrpDll_pt)pTimer, (EigrpDll_pt *)&gpEigrp->timerHdr);
		}
		EigrpPortSemBGive(gpEigrp->timerSem);
	}
#endif //EIGRP_PLAT_ZEBRA
#endif
	*timer	= NULL;
	EIGRP_FUNC_LEAVE(EigrpPortTimerDelete);
	return;
}

/************************************************************************************

	Name:	EigrpPortGetTimeSec

	Desc:	This function is to get the the global time of this machine since it is started.
			The time is in unit of second.	
	Para: 	NONE
	
	Ret:		global time of this machine
************************************************************************************/
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
extern ULONG 	tickGet (void);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
/************************************************************************************/
U32	EigrpPortGetTimeSec()
{
	U32	sec;
	
	EIGRP_FUNC_ENTER(EigrpPortGetTimeSec);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	//zhurish 2016-2-26 sec	= tickGet() / sysClkRateGet();
	{
		time_t	timep;

		time(&timep);
		sec	= timep;
	}
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	sec	= PILSysGetCurrentTimeInSec();
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	{
		time_t	timep;

		time(&timep);
		sec	= timep;
	}
#endif
	EIGRP_FUNC_LEAVE(EigrpPortGetTimeSec);

	return sec;
}

/************************************************************************************

	Name:	EigrpPortGetTime

	Desc:	This function is to get the the global time of this machine since it is started.
	Para: 	
	
	Ret:		global time of this machine
************************************************************************************/

S32	EigrpPortGetTime(U32 *totalSec, U32 *msec, U32 *usec)
{
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
#ifdef _EIGRP_PLAT_MODULE
	#ifdef CLOCK_REALTIME
		int ret = -1;
		struct timespec rtp;
		ret = clock_gettime (CLOCK_REALTIME, &rtp);////???????j??????????????
		if(ret == -1){
			return	FAILURE;
		}
		if(totalSec){
			*totalSec = rtp.tv_sec;
		}
		ret = rtp.tv_nsec / 1000;
		if(msec){
			*msec = ret/ 1000;//???????	
		}
		if(usec){
			*usec = ret % 1000;
		}
		//CLOCK_REALTIME
	#else//CLOCK_REALTIME
		return	FAILURE;
	#endif//CLOCK_REALTIME
#else//_EIGRP_PLAT_MODULE
	return	FAILURE;
#endif//_EIGRP_PLAT_MODULE

#elif (EIGRP_OS_TYPE == EIGRP_OS_PIL)
	{
		globalTime_st		gTv;
		U16	year, mon, day, hour, min, sec;
		U32 y, m, daysInMonths[13] = {-1,31,-1,31,30,31,30,31,31,30,31,30,31};

		Win32GetSystemTime((void *)&gTv);

		year	= gTv.year;
		mon	= gTv.month;
		day	= gTv.day;
		hour	= gTv.hour;
		min	= gTv.minute;
		sec	= gTv.second;

		*totalSec = 0;
		for(y = 1970; y < year; y++){
			if(PILSysLeapYear(y)){
				*totalSec += 366 *(60*60*24);
			}else{
				*totalSec += 365 * (60*60*24);
			}
		}
		
		for(m = 1; m < mon; m++) {
			if(m == 2) {
				if(PILSysLeapYear(year)) {
					*totalSec += 29 * (60*60*24);
				}else{
					*totalSec += 28 * (60*60*24);
				}
			}else{
				*totalSec += daysInMonths[m] * (60*60*24);
			}
		}
		
		*totalSec += (day - 1) * (60*60*24);
		*totalSec += hour * 60 * 60;
		*totalSec += min * 60;
		*totalSec += sec;

		*msec = gTv.milSec;
		if(usec){
			*usec = 0;
		}
	}
#elif (EIGRP_OS_TYPE ==EIGRP_OS_LINUX)
	{
		struct timeval	tv;
		S32	retVal;

		retVal = gettimeofday(&tv, NULL);
		if(retVal){
			return	FAILURE;
		}

		if(totalSec){
			*totalSec = tv.tv_sec;
		}
		if(msec){
			*msec = tv.tv_usec / 1000;
		}
		if(usec){
			*usec = tv.tv_usec % 1000;
		}
	}
#endif

	return	SUCCESS;
}

/************************************************************************************

	Name:	EigrpPortSleep

	Desc:	This function is to release the CPU for 'mSec' milliseconds.
		
	Para: 	mSec	- CPU release time
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortSleep(U32 mSec)
{
	EIGRP_FUNC_ENTER(EigrpPortSleep);
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	{
		int msecval = 1000/sysClkRateGet();//ÿ��tick�ĺ�����
		if(mSec < msecval)
		{
			struct timespec rqtp;
			rqtp.tv_sec	= mSec / 1000;
			rqtp.tv_nsec	= (mSec % 1000) * 1000;
			nanosleep(&rqtp, NULL);
		}
		else
		{
			int value = mSec%msecval;
			struct timespec rqtp;
			taskDelay(mSec/msecval);
			rqtp.tv_sec	= value / 1000;
			rqtp.tv_nsec	= (value % 1000) * 1000;
			nanosleep(&rqtp, NULL);
		}
	}
#elif (EIGRP_OS_TYPE == EIGRP_OS_PIL)
	PILProcSleep(mSec);
#elif (EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	{
		/*
		struct timeval	timeVal;
		
		timeVal.tv_sec	= mSec / 1000;
		timeVal.tv_usec	= (mSec % 1000) * 1000;
		select(0, NULL, NULL, NULL, &timeVal);
		*/
		usleep(mSec*1000);
	}
#endif	

	EIGRP_FUNC_LEAVE(EigrpPortSleep);

	return;
}

/************************************************************************************

	Name:	EigrpPortAssert

	Desc:	This function is to suspend the caller task when the given value 'val' is zero.
 
 			We hope the string 'str' could be printed to the standard output equipment before the task
			is suspended. 
 
 			This function should only be effective in the debug version.
		
	Para: 	val		- an integer ,do assert when it values 0
			str		- pointer to the string which we hope it to be printed 
	
	Ret:		NONE
************************************************************************************/
void	EigrpPortAssert(U32 val, U8 *str)
{
	EIGRP_FUNC_ENTER(EigrpPortAssert);
	EIGRP_TRC(DEBUG_EIGRP_OTHER, "%s\n", str); 
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	if(!val){
		/*TODO		assert(val);*/
	}
#elif (EIGRP_OS_TYPE == EIGRP_OS_PIL)
	if(!val){
		PILAssert(val, str);
	}
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	if(!val){
		//gpEigrp->taskContainer	= NULL;
		//assert(val);
		//printf("%s: !val:0x%x val:0x%x\n",__func__,!val,val);
	}
#endif
	EIGRP_FUNC_LEAVE(EigrpPortAssert);
	
	return;
}

S32	EigrpPortSocket(S32 protocol)
{
	S32	s;

	s = socket(AF_INET, SOCK_DGRAM, protocol);
	if(s == -1){
		return	FAILURE;
	}

	return	s;
}

/* ?????????????"0x018"????????0x018 */
S32	EigrpPortAtoHex(S32 xHead, S8 *s, U32 *pOut)
{
	S8 *cp, *cp1;
	double	hex, pow_x;

	cp	= s;
	if(xHead == TRUE){
		if(*cp != '0' || *(cp + 1) != 'x'){
			return	FAILURE;
		}

		cp	+= 2;
	}
	
	cp1	= cp;
	while(((*cp >= '0') && (*cp <= '9')) || ((*cp >= 'A') && (*cp <= 'F')) || ((*cp >= 'a') && (*cp <= 'f'))){
		cp++;
	}

	if(cp == cp1 || cp - cp1 > 8){
		return	FAILURE;
	}

	hex	= pow_x	= 0;
	while(cp > cp1){
		cp--;
		if((*cp >= '0') && (*cp <= '9')){
			hex	+= (*cp - 0x30) * pow(16, pow_x);
		}

		if((*cp >= 'a') && (*cp <= 'f')){
			hex	+= (*cp - 0x57) * pow(16, pow_x);
		}

		if((*cp >= 'A') && (*cp <= 'F')){
			hex	+= (*cp - 0x37) * pow(16, pow_x);
		}

		pow_x++;
	}

	*pOut	= (U32)hex;

	return	SUCCESS;
	
}

S32	EigrpPortBind(S32 socket, U32 ipAddr, U16 port)
{
	struct sockaddr_in	b;
	S32	retVal;

	if(!ipAddr && !port){
		return	FAILURE;
	}

	EigrpPortMemSet((U8 *)&b, 0, sizeof(struct sockaddr_in));

	b.sin_family = AF_INET;
	if(port){
		b.sin_port = HTONS(port);
	}
	if(ipAddr){
		b.sin_addr.s_addr = HTONL(ipAddr);
	}

	EigrpPortMemSet((U8 *)&b.sin_zero, 0, 8);

	retVal = bind(socket, (struct sockaddr *)&b, sizeof(struct sockaddr_in));
	if(retVal){
		return	FAILURE;
	}

	return	SUCCESS;
}

S32 EigrpPortSocketFionbio(S32 socket)
{
	S32	flag, ret;

	flag = 1;
	ret = ioctl(socket, (U32)FIONBIO,	&flag);
	if(ret == FAILURE){
		return	FAILURE;
	}

	return	SUCCESS;
}

S32	EigrpPortSendto(S32 s, S8 *buf, U32 bufLen, U32 ipAddr, U16 ipPort)
{
	struct sockaddr_in	dstSock;
	int	retVal;

	dstSock.sin_family = AF_INET;
	dstSock.sin_port = HTONS(ipPort);
	dstSock.sin_addr.s_addr = HTONL(ipAddr);

	retVal = sendto(s, (char *)buf, bufLen, 0, (struct sockaddr *)&dstSock, sizeof(struct sockaddr_in));

	return	retVal;
}

S32 EigrpPortRecv(S32 socket, S8 *buf, S32 bufSize, S32 flags)
{
	S32	retVal;

	if(!buf || !bufSize){
		return	FAILURE;
	}

	retVal = recv(socket, buf, bufSize, flags);
	if(retVal <= 0){
		return	FAILURE;
	}

	return	retVal;
}

S32 EigrpPortRecvfrom(S32 socket, S8 *buf, S32 bufSize, S32 flags, U32 *fromAddr, U16 *fromPort)
{
	struct sockaddr_in	from;
	S32	fromSize, retVal;

	if(!buf || !bufSize){
		return	FAILURE;
	}

	fromSize = sizeof(struct sockaddr_in);
	EigrpPortMemSet((U8 *)&from, 0, fromSize);

	retVal = recvfrom(socket, buf, bufSize, flags, (struct sockaddr *)&from, &fromSize);
	if(retVal <= 0){
		return	FAILURE;
	}

	if(fromAddr){
		*fromAddr = NTOHL(from.sin_addr.s_addr);
	}
	if(fromPort){
		*fromPort = NTOHS(from.sin_port);
	}

	return	retVal;
}

/************************************************************************************

	Name:	EigrpPortSockCreate

	Desc:	This function is to apply a socket and set all its parameters needed by the eigrp.
		
	Para: 	NONE
	
	Ret:		the new socket 	for success
			-1				for failure
************************************************************************************/

S32	EigrpPortSockCreate()
{
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	{
		S32 ret;
		S32 sock;
		U8	on ;
		U32	onU32;
		struct sockaddr_in		addr;
		
		EIGRP_FUNC_ENTER(EigrpPortSockCreate);
		/* Make RAW socket. */
#ifdef EIGRP_PACKET_UDP
		sock = socket(AF_INET, SOCK_DGRAM, 0);
#else//EIGRP_PACKET_UDP
		sock = socket(AF_INET, SOCK_RAW, IPEIGRP_PROT);
#endif//EIGRP_PACKET_UDP
		if(sock == FAILURE){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EigrpPortSockCreate: socket: %s\n", sock);
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}

#ifdef IP_HDRINCL
#ifndef EIGRP_PACKET_UDP
		/* Set header include. Eigrp provide whole packet including the ip header. */
		onU32 = 1;
		ret = setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (S8 *)&onU32, sizeof(onU32));
		if(ret == FAILURE){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "Can't set IP_HDRINCL option\n"); 
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
#endif	//EIGRP_PACKET_UDP		
#endif /* IP_HDRINCL */
#ifdef IP_RECVIF
		onU32 = 1;
		if ((ret = setsockopt (sock, IPPROTO_IP, IP_RECVIF, &onU32, sizeof (onU32))) < 0)
		{
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "Can't set IP_RECVIF option\n"); 
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
#endif	//IP_RECVIF
#ifdef IP_RECVDSTADDR
		onU32 = 1;
		ret = setsockopt(sock, IPPROTO_IP, IP_RECVDSTADDR, (S8 *)&onU32, sizeof(onU32));
		if(ret == FAILURE){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "Can't set IP_RECVDSTADDR option\n"); 
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
#endif//IP_RECVDSTADDR
#ifdef SO_REUSEADDR
		onU32 = 1;
		ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (S8 *)&onU32, sizeof(onU32));
		if(ret == FAILURE){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "Can't set IP_RECVDSTADDR option\n"); 
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
#endif//SO_REUSEADDR
#ifdef _DC_
		onU32 = 1;
		ret = setsockopt(sock, IPPROTO_IP, IP_RECVIF, (S8 *)&onU32, sizeof(onU32));
		if(ret == FAILURE){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "Can't set IP_RECVIF option\n"); 
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
#endif//_DC_
		on = 1;
		ret	= setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&on,  sizeof(on));
		if(ret == FAILURE){
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}

		onU32	= 1;
		ret	= ioctl(sock, (U32)FIONBIO,	(S32)&onU32);
		if(ret == FAILURE){
			EigrpPortAssert(0, "");
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
		
#ifdef EIGRP_PACKET_UDP
		EigrpPortMemSet((U8 *)&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family	= AF_INET;
		addr.sin_port		= HTONS(EIGRP_LOCAL_PORT);
		addr.sin_addr.s_addr	= HTONL(INADDR_ANY);
		ret	= bind(sock, (struct sockaddr *)&addr, sizeof(addr));
		if(ret < 0){
			return -1;
		}
#endif//EIGRP_PACKET_UDP
		/*Initialize address constants */
		if(!gpEigrp->helloMultiAddr){
			gpEigrp->helloMultiAddr = EigrpPortSockBuildIn(0, HTONL(EIGRP_ADDR_HELLO));
		}
		EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
		return sock;
	}
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	{
		S32 ret;
		S32 sock;
		U32	on ;
		struct sockaddr_in		addr;

		EIGRP_FUNC_ENTER(EigrpPortSockCreate);
		/* Make RAW socket. */
#ifdef EIGRP_PACKET_UDP
		sock = socket(AF_INET, SOCK_DGRAM, 0);
#else//EIGRP_PACKET_UDP
		sock = socket(AF_INET, SOCK_RAW, IPEIGRP_PROT);
#endif//EIGRP_PACKET_UDP
		if(sock < 0){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EigrpPortSockCreate: socket: %s\n", sock); 
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}

#ifndef EIGRP_PACKET_UDP
		/* Set header include. Eigrp provide whole packet including the ip header.*/
		on = 1;
		ret = setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (S8 *)&on, sizeof(on));
		if(ret < 0){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, (gpEigrp->fixLenStrBuf, "Can't set IP_HDRINCL option\n"));
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		} 
#endif//EIGRP_PACKET_UDP
		on = 1;
		ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (S8 *)&on, sizeof(on));
		if(ret < 0){
			return -1;
		}
	
		on = 1;
		ret = setsockopt(sock, IPPROTO_IP, IP_RECVIF, (S8 *) & on, sizeof(on));
		if(ret < 0){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, (gpEigrp->fixLenStrBuf, "Can't set IP_RECVIF option\n"));
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
		
		on = 1;
		if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (S8 *)&on,  sizeof(on))  < 0){
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
	
#ifdef EIGRP_PACKET_UDP
		EigrpPortMemSet((U8 *)&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_port		= HTONS(EIGRP_LOCAL_PORT);
		addr.sin_addr.s_addr	= HTONL(INADDR_ANY);//inet_addr(/*"192.168.1.23"*/"127.0.0.1");
		ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
		if(ret < 0){
			return -1;
		}
#endif//EIGRP_PACKET_UDP
		/* Initialize address constants */
		if(!gpEigrp->helloMultiAddr){
			gpEigrp->helloMultiAddr = EigrpPortSockBuildIn(0, HTONL(EIGRP_ADDR_HELLO));
		}
		EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
	
		return sock;
	}
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	{
		S32 ret;
		S32 sock;
		U8	on ;
		U32	onU32;
		extern struct zebra_privs_t eigrp_privs;
		EIGRP_FUNC_ENTER(EigrpPortSockCreate);
		/* Make RAW socket. */
		if ( eigrp_privs.change (ZPRIVS_RAISE) )
		    zlog_err ("%s: could not raise privs, %s",__func__,safe_strerror (errno) );
		errno = 0;
		sock = socket(AF_INET, SOCK_RAW, IPEIGRP_PROT);
		if(sock == FAILURE){
			if (eigrp_privs.change (ZPRIVS_LOWER))
				zlog_err ("%s: could not lower privs, %s",__func__,safe_strerror (errno) );
			//printf("EigrpPortSockCreate: socket: %d(%s)\n", sock, strerror (errno) );
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EigrpPortSockCreate: socket: %d(%s)\n", sock,strerror (errno) );
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
		
		/* Set header include. Eigrp provide whole packet including the ip header. */

#ifdef IP_HDRINCL
		onU32 = 1;
		ret = setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (S8 *)&onU32, sizeof(onU32));
		if(ret !=0){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "Can't set IP_HDRINCL option\n"); 
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
#endif /* IP_HDRINCL */
		
#ifdef IP_RECVDSTADDR
		onU32 = 1;
		ret = setsockopt(sock, IPPROTO_IP, IP_RECVDSTADDR, (S8 *)&onU32, sizeof(onU32));
		if(ret !=0){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "Can't set IP_RECVDSTADDR option\n"); 
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
#endif /* IP_RECVDSTADDR */

#ifdef SO_REUSEADDR
		onU32 = 1;
		ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (S8 *)&onU32, sizeof(onU32));
		if(ret !=0){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "Can't set IP_RECVDSTADDR option\n"); 
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
#endif /* SO_REUSEADDR */
		
		on = 1;
		ret	= setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (S8 *)&on,  sizeof(on));
		if(ret !=0){
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
#ifdef EIGRP_PLAT_ZEBRA		
		set_nonblocking(sock);
#else//EIGRP_PLAT_ZEBRA

  /* According to the Single UNIX Spec, the return value for F_GETFL should never be negative. */
#ifdef HAVE_FCNTL  
		if ((onU32 = fcntl(sock, F_GETFL)) < 0)
		{
			_EIGRP_DEBUG("fcntl(F_GETFL) failed for fd %d: %s", sock, strerror(errno));
			return -1;
		}
		if (fcntl(sock, F_SETFL, (onU32 | O_NONBLOCK)) < 0)
		{
			_EIGRP_DEBUG("fcntl failed setting fd %d non-blocking: %s", sock, strerror(errno));    
			return -1;
		}
#else//#ifdef HAVE_FCNTL  
	/*optval=1???????? optval=0????*/
#ifdef FIONBIO
		onU32	= 1;
		ret	= ioctl(sock, (U32)FIONBIO,	(S32)&onU32);
		if(ret == FAILURE){
			assert(0);
			EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
			return -1;
		}
#endif /* FIONBIO */
#endif// HAVE_FCNTL  
#endif//EIGRP_PLAT_ZEBRA

		/*Initialize address constants */
		if(!gpEigrp->helloMultiAddr){
			gpEigrp->helloMultiAddr = EigrpPortSockBuildIn(0, HTONL(EIGRP_ADDR_HELLO));
		}
		EIGRP_FUNC_LEAVE(EigrpPortSockCreate);
		if (eigrp_privs.change (ZPRIVS_LOWER))
			zlog_err ("%s: could not lower privs, %s",__func__,safe_strerror (errno) );

		return sock;
	}
#endif//(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
}

void EigrpPortSockRelease(S32 socket)
{
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	Win32SysCloseSocket(socket);
#elif(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	close((int)socket);
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)//zhurish	
	close((int)socket);
#endif//(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	return;
}

U8 EigrpPortSelect(S32 sock)
{
	S32	retVal;
#if(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	struct fd_set  fdR;
#elif(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
	fd_set	fdR;
#elif (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	struct fd_set	fdR;
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	struct timeval timeout; 

	FD_ZERO(&fdR); 
	FD_SET(sock, &fdR); 
	EigrpPortMemSet((U8 *)&timeout, 0, sizeof(struct  timeval));
	timeout.tv_sec	= 0;
	timeout.tv_usec = 1000 * 10;
/* Start Edit By  : zhurish : 2016/01/17 20:0:6  : :????:.??????,???vxworks??????????READY??  */
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	//timeout.tv_usec = 1000 * 200;// 100 msec
#elif(EIGRP_OS_TYPE == EIGRP_OS_LINUX)	
	//timeout.tv_usec = 1000 * 10;// 10 msec
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
/* End Edit By  : zhurish : 2016/01/17 20:0:6  */

	retVal = select(sock + 1, &fdR, NULL, NULL, &timeout);
	if(retVal <= 0){
		return	FALSE;
	}
	if(!FD_ISSET(sock, &fdR)){
		return	FALSE;
	}
	return	TRUE;
}

/************************************************************************************

	Name:	EigrpPortSetMaxPackSize

	Desc:	This function is to figure out what the maximum value for a kernel socket buffer is and 
			set a appropriate value
	Para: 
	
	Ret:		
************************************************************************************/

U32	EigrpPortSetMaxPackSize(S32 sock)
{    
	U32 packetsize;
	
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	{
		S32 retval;
		S32 optLen;
		
		EIGRP_FUNC_ENTER(EigrpPortSetMaxPackSize);
		packetsize = 0;	
		if(sock < 0){
			packetsize = 32767;
			EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);
			return packetsize;
		}

		optLen	= sizeof(packetsize);
		retval	= getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (S8 *) &packetsize, &optLen);
		if(retval == FAILURE){
			/* it should always return 0 */
			EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);
			return 2;
		}

		EIGRP_TRC(DEBUG_EIGRP_TASK, "EIGRP-SYS: get max paket size =%d\n", packetsize);
		packetsize = packetsize > (128 *1024) ? packetsize : (128 *1024);
		/*	packetsize	= 32767;*/
		retval = setsockopt(sock,	SOL_SOCKET, SO_SNDBUF, (S8 *) &packetsize,	sizeof(packetsize));
		if(retval == FAILURE){
			EigrpPortAssert(0, "Very strong system error.\n");
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP-INIT: Can't set send buffer to %d bytes\n", packetsize);
			EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);
			return 1;
		}

		packetsize = packetsize > (128 *1024) ? packetsize : (128 *1024);
		retval = setsockopt(sock,	SOL_SOCKET, SO_RCVBUF, (S8 *) &packetsize,	sizeof(packetsize));
		if(retval == FAILURE){
			EigrpPortAssert(0, "Very strong system error.\n");
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP-INIT: Can't set receive buffer to %d bytes\n", packetsize);
			EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);
			return 1;
		}
		EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);

		return packetsize;
	}
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	{
		S32 retval;
		S32	optLen;

		EIGRP_FUNC_ENTER(EigrpPortSetMaxPackSize);
		packetsize = 0;	
		if(sock < 0){
			packetsize = 32767;
			EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);
			return packetsize;
		}

		optLen	= sizeof(packetsize);
		if( getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (S8 *) &packetsize, &optLen) < 0){
			/* it should always return 0 */
			EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);
			return 2;
		}
		EIGRP_TRC(DEBUG_EIGRP_TASK, "EIGRP-SYS: get max paket size =%d\n", packetsize);
		packetsize = packetsize > (128 *1024) ? packetsize : (128 *1024);

	 	if((retval = setsockopt(sock,  SOL_SOCKET,  SO_RCVBUF, (S8 *) &packetsize,  sizeof(packetsize))) < 0){
			EigrpPortAssert(0, "Very strong system error.\n");
			EIGRP_TRC(DEBUG_EIGRP_OTHER, 
						"EIGRP-INIT: Can't set receive buffer to %d bytes\n", packetsize);
			EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);
			return 1;
		}
		EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);

		return packetsize;
	}
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	{
		S32 retval;
		S32 optLen;
		
		EIGRP_FUNC_ENTER(EigrpPortSetMaxPackSize);
		if(sock < 0){
			packetsize = 32767;
			EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);
			return packetsize;
		}

		optLen	= sizeof(packetsize);
		retval	= getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (S8 *) &packetsize, &optLen);
		if(retval == FAILURE){
			/* it should always return 0 */
			EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);
			return 2;
		}

		EIGRP_TRC(DEBUG_EIGRP_TASK, "EIGRP-SYS: get max paket size =%d\n", packetsize);
		packetsize	= 32767;
		retval = setsockopt(sock,	SOL_SOCKET, SO_SNDBUF, (S8 *) &packetsize,	sizeof(packetsize));
		if(retval == FAILURE){
			EigrpPortAssert(0, "Very strong system error.\n");
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP-INIT: Can't set send buffer to %d bytes\n", packetsize);
			EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);
			return 1;
		}

		packetsize = packetsize > (128 *1024) ? packetsize : (128 *1024);
		retval = setsockopt(sock,	SOL_SOCKET, SO_RCVBUF, (S8 *) &packetsize,	sizeof(packetsize));
		if(retval == FAILURE){
			EigrpPortAssert(0, "Very strong system error.\n");
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP-INIT: Can't set receive buffer to %d bytes\n", packetsize);
			EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);
			return 1;
		}
		EIGRP_FUNC_LEAVE(EigrpPortSetMaxPackSize);

		return packetsize;
	}
#endif
}

/************************************************************************************

	Name:	EigrpPortMcastOption

	Desc:	This function is change the membership of a given logical interface to the eigrp common 
 			multicast group '224.0.0.10'.
 
 			If the param 'add' is true, it means that the caller want to add the given interface to
 			"receive list" of the multicast group and vice versa.
 			
	Para: 	sock		- the socket to be set
			pEigrpIntf	- pointer to the logical interface
			add			- the flag of adding the given interface to "receive list"
	
	Ret:		NONE
************************************************************************************/
void	EigrpPortMcastOption(S32 sock, EigrpIntf_pt pEigrpIntf, S32 add)
{	
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	{	
		S32 	retVal;
		struct ip_mreq	mreq;

		EIGRP_FUNC_ENTER(EigrpPortMcastOption);
		EIGRP_ASSERT((U32)pEigrpIntf);

		EigrpPortMemSet((U8 *)&mreq, 0, sizeof(mreq));
		mreq.imr_multiaddr.s_addr	= gpEigrp->helloMultiAddr->sin_addr.s_addr;
		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT))
		{
#ifndef _EIGRP_PLAT_MODULE	
			mreq.imr_index	= pEigrpIntf->ifindex;
#else     //_EIGRP_PLAT_MODULE
			mreq.imr_interface.s_addr = HTONL(pEigrpIntf->ipAddr);
#endif   //_EIGRP_PLAT_MODULE
			//if(pEigrpIntf->ipAddr == EigrpPortGetRouterId()){
			if(pEigrpIntf->ipAddr == 0){
				mreq.imr_interface.s_addr	= HTONL(pEigrpIntf->ipAddr);
				}else{
				mreq.imr_interface.s_addr	= HTONL(pEigrpIntf->remoteIpAddr);
			}
		}
		else
		{
#ifndef _EIGRP_PLAT_MODULE	
			mreq.imr_index	= pEigrpIntf->ifindex;
#else     //_EIGRP_PLAT_MODULE
			mreq.imr_interface.s_addr = HTONL(pEigrpIntf->ipAddr);
#endif   //_EIGRP_PLAT_MODULE
		}

		_EIGRP_DEBUG( "EIGRP-EVENT: interface %s %x  add into MULTICAST group %x\n",
							pEigrpIntf->name,mreq.imr_interface.s_addr,mreq.imr_multiaddr.s_addr);
		if(add)
		{
#ifndef _EIGRP_PLAT_MODULE			
			retVal = setsockopt(sock, IPPROTO_IP, (S32)IP_ADD_MEMBERSHIP_INDEX, (S8 *)&mreq, sizeof(mreq));
#else     //_EIGRP_PLAT_MODULE
			retVal = setsockopt(sock, IPPROTO_IP, (S32)IP_ADD_MEMBERSHIP, (S8 *)&mreq, sizeof(mreq));
#endif   //_EIGRP_PLAT_MODULE
			if(retVal == FAILURE){

				_EIGRP_DEBUG( "EIGRP-EVENT: interface %s can't add into MULTICAST group\n",
							pEigrpIntf->name);
				EIGRP_TRC(DEBUG_EIGRP_EVENT, "EIGRP-EVENT: interface %s can't add into MULTICAST group\n",
							pEigrpIntf->name);
				EIGRP_FUNC_LEAVE(EigrpPortMcastOption);
				return;
			}
		}
		else
		{
			setsockopt(sock,  IPPROTO_IP,  IP_DROP_MEMBERSHIP, (S8 *)&mreq, sizeof(mreq));
		}
		EIGRP_FUNC_LEAVE(EigrpPortMcastOption);
	}
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	{	
		S32				errorno;
		struct ip_mreq	mreq;

		EIGRP_FUNC_ENTER(EigrpPortMcastOption);
		EIGRP_ASSERT((U32)pEigrpIntf);

		EigrpPortMemSet((U8 *)&mreq, 0, sizeof(mreq));
		mreq.imr_multiaddr.s_addr	= gpEigrp->helloMultiAddr->sin_addr.s_addr;
		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
			mreq.circName	= pEigrpIntf->name;
			
			if(pEigrpIntf->ipAddr == EigrpPortGetRouterId()){
				mreq.imr_interface.s_addr	= HTONL(pEigrpIntf->ipAddr);
			}else{
			mreq.imr_interface.s_addr	= HTONL(pEigrpIntf->remoteIpAddr);
			}
		}else{
			mreq.imr_interface.s_addr	= HTONL(pEigrpIntf->ipAddr);
		}

		if(add){
			errorno = setsockopt(sock, IPPROTO_IP,	IP_ADD_MEMBERSHIP, (S8 *)&mreq, sizeof(mreq));

			if(errorno < 0 && (errorno != EADDRINUSE)){
				EIGRP_TRC(DEBUG_EIGRP_EVENT, "EIGRP-EVENT: interface %s can't add into MULTICAST group\n",
							pEigrpIntf->name);
				EIGRP_FUNC_LEAVE(EigrpPortMcastOption);
				return;
			}
		}else{
			setsockopt(sock,  IPPROTO_IP,  IP_DROP_MEMBERSHIP, (S8 *)&mreq, sizeof(mreq));
		}
		EIGRP_FUNC_LEAVE(EigrpPortMcastOption);
	}
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	{
#ifdef EIGRP_PLAT_ZEBRA
		S32 	retVal; 
		EIGRP_FUNC_ENTER(EigrpPortMcastOption);
		EIGRP_ASSERT((U32)pEigrpIntf);
		if(add){
			retVal = setsockopt_ipv4_multicast(sock, IP_ADD_MEMBERSHIP, 
				gpEigrp->helloMultiAddr->sin_addr.s_addr, pEigrpIntf->ifindex);
			
			if(retVal == FAILURE){

				EIGRP_TRC(DEBUG_EIGRP_EVENT,
							"EIGRP-EVENT: interface %s can't add into MULTICAST group\n", pEigrpIntf->name);
				EIGRP_FUNC_LEAVE(EigrpPortMcastOption); 
				assert(0);
				return;
			}
		}
		else
		{
			setsockopt_ipv4_multicast(sock, IP_DROP_MEMBERSHIP, 
				gpEigrp->helloMultiAddr->sin_addr.s_addr, pEigrpIntf->ifindex);
		}
		EIGRP_FUNC_LEAVE(EigrpPortMcastOption);
#else//EIGRP_PLAT_ZEBRA
		S32 	retVal; 
		struct ip_mreq	 mreq;//by lihui 20090928 ip_merq-->ip_merqn

		EIGRP_FUNC_ENTER(EigrpPortMcastOption);
		EIGRP_ASSERT((U32)pEigrpIntf);

		EigrpPortMemSet(&mreq, 0, sizeof (mreq));

		mreq.imr_multiaddr.s_addr= gpEigrp->helloMultiAddr->sin_addr.s_addr;//by lihui 

/*		inet_aton("224.0.0.10", &(mreq.imr_multiaddr.s_addr));
		inet_aton("192.168.1.128", &(mreq.imr_address.s_addr));*/
		mreq.imr_ifindex= pEigrpIntf->ifindex;

		if(add){
			 setsockopt(sock,  IPPROTO_IP,  IP_DROP_MEMBERSHIP,&mreq, sizeof(mreq));//by lihui 20090925
			retVal = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
			if(retVal == FAILURE){

				EIGRP_TRC(DEBUG_EIGRP_EVENT,
							"EIGRP-EVENT: interface %s can't add into MULTICAST group\n",
							pEigrpIntf->name);
				EIGRP_FUNC_LEAVE(EigrpPortMcastOption); 
				assert(0);
				return;
			}
		}else{
			setsockopt(sock,  IPPROTO_IP,  IP_DROP_MEMBERSHIP, (S8 *)&mreq, sizeof(mreq));
		}
		EIGRP_FUNC_LEAVE(EigrpPortMcastOption);
#endif// EIGRP_PLAT_ZEBRA			
	}
#endif//(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
}

#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
#ifdef _DC_
void	EigrpPortMcastOption_vlanIf(S32 sock, U32 vIfIndex, S32 add)
{	
	S32 	retVal;
	struct ip_mreqn	mreqn;

	_EIGRP_DEBUG("dbg>>EigrpPortMcastOption_vlanIf(%d)\n", vIfIndex);


	EIGRP_FUNC_ENTER(EigrpPortMcastOption_vlanIf);
	EIGRP_ASSERT((U32)vIfIndex);

	EigrpPortMemSet((U8 *)&mreqn, 0, sizeof(mreqn));
	mreqn.imr_multiaddr.s_addr = gpEigrp->helloMultiAddr->sin_addr.s_addr;
	mreqn.imr_ifindex = vIfIndex;
	if(add){
		_EIGRP_DEBUG("EigrpPortMcastOption_vlanIf: IP_ADD_MEMBERSHIP  1   imr_ifindex = 0x%x, imr_multiaddr.s_addr  \n",  mreqn.imr_ifindex, mreqn.imr_multiaddr.s_addr);
		retVal = setsockopt(sock, IPPROTO_IP, (S32)IP_ADD_MEMBERSHIP, (S8 *)&mreqn, sizeof(mreqn));
		if(retVal == FAILURE){
			EIGRP_FUNC_LEAVE(EigrpPortMcastOption_vlanIf);
			return;
		}
	}else{
		_EIGRP_DEBUG("EigrpPortMcastOption_vlanIf: IP_DROP_MEMBERSHIP  2   imr_ifindex = 0x%x, imr_multiaddr.s_addr  \n",  mreqn.imr_ifindex, mreqn.imr_multiaddr.s_addr);
		setsockopt(sock,  IPPROTO_IP,  IP_DROP_MEMBERSHIP, (S8 *)&mreqn, sizeof(mreqn));
	}

	EIGRP_FUNC_LEAVE(EigrpPortMcastOption_vlanIf);
}

void	EigrpPortMcastOptionWithLayer2(S32 sock, EigrpIntf_pt pEigrpIntf, S32 add)
{	
	EIGRP_FUNC_ENTER(EigrpPortMcastOptionWithLayer2);
	EIGRP_ASSERT((U32)pEigrpIntf);
	_EIGRP_DEBUG("dbg>>EigrpPortMcastOptionWithLayer2\n");

#if(DC_PLAT_TYPE == DC_PLAT_ZEBOS)
	{
		DcUai_pt	pUai;
		DcSei_pt	pSei;
	 	U32	seiCnt;

		pUai = DcUtilGetUaiByName(pEigrpIntf->name);
		if(pUai){
			for(seiCnt = 0; seiCnt <DC_SEI_PER_UAI_MAX; seiCnt++){
				if(pUai->sei[seiCnt].used == FALSE){
					continue;
				}

				pSei	= pUai->sei[seiCnt].pSei;
				if(pSei == NULL){
					continue;
				}

				EigrpPortMcastOption_vlanIf(sock, ((struct interface *)pSei->pCircSys)->ifindex, add);
			}
		}
	 }
#endif//(DC_PLAT_TYPE == DC_PLAT_ZEBOS)

	EigrpPortMcastOption(sock, pEigrpIntf, add);

	EIGRP_FUNC_LEAVE(EigrpPortMcastOptionWithLayer2);

	return;
}
#endif//_DC_
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)


/************************************************************************************

	Name:	EigrpPortSendIpPacket

	Desc:	This function is to send out an eigrp ip packet.
		
	Para: 	sock		- the socket for sending the packet
			data		- pointer to the packet to be sent
			len		- packet length
			flags		- sending sign
			addr		- destination ip address
			iidb		- the interface on which the packet to be sent
			
	Ret:		
************************************************************************************/

S32	EigrpPortSendIpPacket(S32 sock, void *data, U32 len, U8 flags, struct EigrpSockIn_ *addr, EigrpIdb_st *iidb)
{
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	{
		S32 			ret;
		struct EigrpIpHdr_	*pIph;
		struct sockaddr_in  sname;
		int	errno;

		EIGRP_FUNC_ENTER(EigrpPortSendIpPacket);
#ifndef EIGRP_PACKET_UDP
		pIph = (struct EigrpIpHdr_ *)data;
		pIph->hdrLen	= sizeof(struct EigrpIpHdr_ ) >> 2;
		pIph->version	= 4;
		pIph->tos 	= 0;
		pIph->length	= len;

		pIph->id		= 0;
		pIph->offset	= 0;
		pIph->ttl 	= 2;

		pIph->protocol	= IPEIGRP_PROT;
		pIph->chkSum		= 0;
		pIph->srcIp		= HTONL(iidb->idb->ipAddr);
		pIph->dstIp		= addr->sin_addr.s_addr;/* should be NBO */	
#endif//EIGRP_PACKET_UDP
		sname.sin_family = AF_INET;
		sname.sin_port = addr->sin_port;
		sname.sin_addr.s_addr = addr->sin_addr.s_addr;
		sname.sin_len = sizeof(struct sockaddr_in);

		errno	= 0;
		ret = sendto(sock, data, len, flags, (struct sockaddr *)&sname, sizeof(struct sockaddr_in));
		if(ret < 0){
			errno	= errnoOfTaskGet(taskIdSelf);
			printf("dbg>>EigrpPortSendIpPacket: sock:%d   dst->sin_addr.s_addr:0x%.8x, ret:0x%x, errno:%d , iidb->idb->name:%s \n", 
				sock, addr->sin_addr.s_addr, ret, errno, iidb->idb->name);
		}

		EIGRP_FUNC_LEAVE(EigrpPortSendIpPacket);

		return ret;
	}
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	{
#ifndef EIGRP_PACKET_UDP
		S32				ret;
		struct EigrpIpHdr_	iph;
		struct msghdr	msg;
		struct iovec		iov[2];

		EIGRP_FUNC_ENTER(EigrpPortSendIpPacket);
		iph.hdrLen	= sizeof(struct EigrpIpHdr_ ) >> 2;
		iph.version	= IPVERSION;
		iph.tos		= IPTOS_PREC_INTERNETCONTROL;
		iph.length	= iph.hdrLen*4 + len;

		iph.id		= 0;
		iph.offset		= 0;
		iph.ttl		= 2;

		iph.protocol	= IPEIGRP_PROT;
		iph.chkSum	= 0;
		iph.srcIp		= 0;
		iph.dstIp		= addr->sin_addr.s_addr;    /* should be NBO */

		EigrpPortMemSet((U8 *)&msg, 0, sizeof(msg));
		msg.msg_name	= addr;
		msg.msg_namelen	= sizeof(*addr);
		msg.msg_iov		= iov;
		msg.msg_iovlen	= 2;
		iov[0].iov_base	= (S8 *)&iph;
		iov[0].iov_len		= iph.hdrLen * 4;
		iov[1].iov_base	= data;
		iov[1].iov_len		= len;

		ret = sendmsg(sock, &msg, flags);
#else//EIGRP_PACKET_UDP
		S32 			ret;
		struct EigrpIpHdr_	iph, *pIph;
		struct sockaddr_in  sname;
		
		EIGRP_FUNC_ENTER(EigrpPortSendIpPacket);
		sname.sin_family = AF_INET;
		sname.sin_port = addr->sin_port;
		sname.sin_addr.s_addr = addr->sin_addr.s_addr;
		sname.sin_len = sizeof(struct sockaddr_in);
		ret = sendto(sock, data, len, flags, (struct sockaddr *)&sname, sizeof(struct sockaddr_in));
#endif//EIGRP_PACKET_UDP
		EIGRP_FUNC_LEAVE(EigrpPortSendIpPacket);

		return ret;
	}
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	{
		S32 			ret;
		struct EigrpIpHdr_	iph, *pIph;
		struct sockaddr_in  sname;

		EIGRP_FUNC_ENTER(EigrpPortSendIpPacket);
		pIph = (struct EigrpIpHdr_ *)data;
		pIph->hdrLen	= sizeof(struct EigrpIpHdr_ ) >> 2;
		pIph->version	= 4;
		pIph->tos 	= 0;
		pIph->length	= len;

		pIph->id		= 0;
		pIph->offset	= 0;
		pIph->ttl 	= 2;

		pIph->protocol	= IPEIGRP_PROT;
		pIph->chkSum		= 0;

		pIph->srcIp		= HTONL(iidb->idb->ipAddr);
		pIph->dstIp		= addr->sin_addr.s_addr;/* should be NBO */	

		sname.sin_family = AF_INET;
		sname.sin_port = addr->sin_port;
		sname.sin_addr.s_addr = addr->sin_addr.s_addr;
		/* sname.sin_len = sizeof(struct sockaddr_in);  by lihui 20090927*/

		ret = sendto(sock, data, len, flags, (struct sockaddr *)&sname, sizeof(struct sockaddr_in));
		EIGRP_FUNC_LEAVE(EigrpPortSendIpPacket);

		return ret;
	}
#endif
}

/************************************************************************************

	Name:	EigrpPortSendPacket

	Desc:	This function is to set the options of a given socket and send out an eigrp ip packet 
			using the	funciton 'EigrpPortSendIpPacket'.
			
	Para: 	eigrph		- pointer to the header of eigrp packet
			bytes		- length of the packet
			peer			- the peer node which the packet is to be sent to
			iidb			- the interface on which the packet to be sent
			priority		- unused
			
	Ret:		NONE
************************************************************************************/
//int dbg_EigrpSend = 0;
void	EigrpPortSendPacket(EigrpPktHdr_st *eigrph, S32 bytes, EigrpDualPeer_st *peer, EigrpIdb_st *iidb, S32 priority)
{
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
#ifdef _DC_
	{
		EigrpIntf_pt			pEigrpIntf;
		struct EigrpSockIn_	dest;
		struct ip_mreqn	mreqn;
		U8					flags = 0, mcastTtl;
		S32					ret;
		S8		*pktdata; /* pointer to eigrp data,not include ip header */
		U32		ipPktLen, flag, opcode;

		EIGRP_FUNC_ENTER(EigrpPortSendPacket);
		pEigrpIntf = iidb->idb;
		if(!pEigrpIntf){
			EIGRP_ASSERT(0);
			EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
			return;
		}

		if(!gpEigrp->sendBuf){
			EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
			return;
		}

#ifdef EIGRP_PACKET_UDP
		EigrpPortMemCpy((void *) gpEigrp->sendBuf, (void *)eigrph, (U32) bytes);
		ipPktLen = (U32) bytes;
#else//EIGRP_PACKET_UDP
		pktdata = (S8 *)gpEigrp->sendBuf + sizeof(struct EigrpIpHdr_);
		EigrpPortMemCpy((void *) pktdata, (void *)eigrph, (U32) bytes);
		ipPktLen = (U32) bytes + sizeof(struct EigrpIpHdr_);
#endif//EIGRP_PACKET_UDP

		if(peer || !BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST))
		{
			dest.sin_family	= AF_INET;
			
#ifdef EIGRP_PACKET_UDP
			dest.sin_port	= HTONS(EIGRP_LOCAL_PORT);
#else//EIGRP_PACKET_UDP
			dest.sin_port	= 0;
#endif//EIGRP_PACKET_UDP
			if(peer)
			{ /* we have known this peer before */			
				dest.sin_addr.s_addr = HTONL(peer->source);
			}
			else
			{ /* we should send multicast pkt,but we send it unicast pkt on P2P interface */
				dest.sin_addr.s_addr = HTONL(pEigrpIntf->remoteIpAddr);
			}

			if(!BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){/*modify_zhenxl_20120523*/
				flags = MSG_DONTROUTE;
			}
			ret = EigrpPortSendIpPacket(gpEigrp->socket, (void *) gpEigrp->sendBuf, ipPktLen,
									flags, &dest, iidb);
			
			_EIGRP_DEBUG("%d:FRP  SEND : %s\tvia %s\tto   %s\tret=%d\n",
					EigrpPortGetTimeSec(),
					((eigrph->opcode == 5 && eigrph->ack) ? "ACK" : EigrpOpercodeItoa(eigrph->opcode)),
					pEigrpIntf->name,
					EigrpUtilIp2Str(NTOHL(dest.sin_addr.s_addr)),
					ret);

			if(ret <= 0){
				EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP send unicast result = %d\n", ret);
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}

			opcode = eigrph->opcode;
			if(eigrph->opcode == EIGRP_OPC_HELLO && eigrph->ack){
				opcode	= EIGRP_OPC_ACK;
			}
			flag	= DEBUG_EIGRP_INTERNAL | EigrpDebugPacketType(opcode,0);
			EIGRP_TRC(flag, "EIGRP-SEND: %-6s PACKET \t%s->PEER %s(Unicast) \tsize =%d\n",
						EigrpOpercodeItoa(eigrph->opcode),
						iidb->idb->name,
						EigrpUtilIp2Str(dest.sin_addr.s_addr),
						ret);
			
			flag	= EigrpDebugPacketDetailType(opcode);
			EigrpDebugPacketDetail(flag, (EigrpPktHdr_st *)gpEigrp->sendBuf, bytes);
		}
		else if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST))
		{  /* Multicast on LAN */ 
			dest.sin_family		= AF_INET;
#ifdef EIGRP_PACKET_UDP
			dest.sin_port	= HTONS(EIGRP_LOCAL_PORT);
#else//EIGRP_PACKET_UDP
			dest.sin_port	= 0;
#endif//EIGRP_PACKET_UDP
			dest.sin_addr.s_addr	= HTONL(EIGRP_ADDR_HELLO);
			flags					= 0;

			/* mreqn.imr_ifindex = ((struct interface *)pEigrpIntf->sysCirc)->ifindex;*/
			mreqn.imr_ifindex = pEigrpIntf->ifindex;
			ret = setsockopt(gpEigrp->socket, IPPROTO_IP, IP_MULTICAST_IF, (S8 *)&mreqn, sizeof(mreqn));
			if(ret < 0){
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}

			mcastTtl	= 2;
			ret = setsockopt(gpEigrp->socket, IPPROTO_IP, IP_MULTICAST_TTL, (S8 *)&mcastTtl, sizeof(mcastTtl));
			if(ret < 0){
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}
			ret = EigrpPortSendIpPacket(gpEigrp->socket, (void *)gpEigrp->sendBuf, ipPktLen, flags, &dest, iidb);

			if(dbg_EigrpSend){
				printf("%d:FRP  SEND : %s\tvia %s\tto   %s\tret=%d\n",
					EigrpPortGetTimeSec(),
					((eigrph->opcode == 5 && eigrph->ack) ? "ACK" : EigrpOpercodeItoa(eigrph->opcode)),
					pEigrpIntf->name,
					EigrpUtilIp2Str(NTOHL(dest.sin_addr.s_addr)),
					ret);
				{/*added by cetc---f 20130106*/
					#ifdef EIGRP_PACKET_UDP
					debugEigrpPkt(gpEigrp->sendBuf, bytes);
					#else
					debugEigrpPkt(gpEigrp->sendBuf+sizeof(struct EigrpIpHdr_), bytes);/**/
					#endif
				}/**/
			}
			if(ret <= 0){
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}
			opcode = eigrph->opcode;
			if(eigrph->opcode == EIGRP_OPC_HELLO && eigrph->ack){
				opcode	= EIGRP_OPC_ACK;
			}
			flag	= DEBUG_EIGRP_INTERNAL | EigrpDebugPacketType(opcode,0);
			EIGRP_TRC(flag, "EIGRP-SEND: %-6s PACKET \t%s->GROUP %s(Multicast) \tsize =%d\n",
								EigrpOpercodeItoa(eigrph->opcode),
								iidb->idb->name,
								EigrpUtilIp2Str(dest.sin_addr.s_addr), ret);
			flag	= EigrpDebugPacketDetailType(opcode);
			EigrpDebugPacketDetail(flag, (struct EigrpPktHdr_ *)gpEigrp->sendBuf, bytes);
		}
		EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
	}
#else//_DC_
	{
		EigrpIntf_pt			pEigrpIntf;
		struct EigrpSockIn_	dest;
		struct EigrpInaddr_	intfAddr;
		U8					flags, mcastTtl, ucastTtl;
		S32					ret;
		struct ip_mreq		mreq;
#ifdef _EIGRP_PLAT_MODULE	
		struct	in_addr sin_addr;
#endif//_EIGRP_PLAT_MODULE
		struct EigrpIpHdr_	*iph;
		S8		*pktdata; /* pointer to eigrp data,not include ip header */
		U32		ipPktLen, flag, opcode;
		U16		ifIndex;

		EIGRP_FUNC_ENTER(EigrpPortSendPacket);
		pEigrpIntf = iidb->idb;
		if(!pEigrpIntf){
			EIGRP_ASSERT(0);
			EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
			return;
		}
		if(!gpEigrp->sendBuf){
			EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
			return;
		}
		pktdata = (S8 *)gpEigrp->sendBuf + sizeof(struct EigrpIpHdr_);
		EigrpPortMemCpy((void *) pktdata, (void *)eigrph, (U32) bytes);
		ipPktLen = (U32) bytes + sizeof(struct EigrpIpHdr_);

		if(peer || !BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST)){
			dest.sin_family	= AF_INET;
			dest.sin_port	= 0;
			if(peer){ /* we have known this peer before */			
				dest.sin_addr.s_addr = HTONL(peer->source);
			}else{ /* we should send multicast pkt,but we send it unicast pkt on P2P interface */
				dest.sin_addr.s_addr = HTONL(pEigrpIntf->remoteIpAddr);
			}
			
			flags = MSG_DONTROUTE;
			ret = EigrpPortSendIpPacket(gpEigrp->socket, (void *) gpEigrp->sendBuf, ipPktLen,
									flags, &dest, iidb);
			if(ret <= 0){
				EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP send unicast result = %d\n", ret);
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}

			opcode = eigrph->opcode;
			if(eigrph->opcode == EIGRP_OPC_HELLO && eigrph->ack){
				opcode	= EIGRP_OPC_ACK;
			}
			flag	= DEBUG_EIGRP_INTERNAL | EigrpDebugPacketType(opcode,0);
			EIGRP_TRC(flag, "EIGRP-SEND: %-6s PACKET \t%s->PEER %s(Unicast) \tsize =%d\n",
						EigrpOpercodeItoa(eigrph->opcode),
						iidb->idb->name,
						EigrpUtilIp2Str(NTOHL(dest.sin_addr.s_addr)),
						ret);
			flag	= EigrpDebugPacketDetailType(opcode);
			EigrpDebugPacketDetail(flag, (EigrpPktHdr_st *)gpEigrp->sendBuf, bytes);
			
		}else if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST)){  /* Multicast on LAN */ 

			dest.sin_family		= AF_INET;
			dest.sin_port			= 0;
			dest.sin_addr.s_addr	= HTONL(EIGRP_ADDR_HELLO);
			flags					= 0;
			ifIndex	= pEigrpIntf->ifindex;
#ifndef _EIGRP_PLAT_MODULE//(????)
			ret = setsockopt(gpEigrp->socket, IPPROTO_IP, IP_MULTICAST_IFINDEX, (S8 *)&ifIndex, sizeof(ifIndex));
			if(ret < 0){
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}
#else //_EIGRP_PLAT_MODULE	
			sin_addr.s_addr = HTONL(pEigrpIntf->ipAddr);
			ret = setsockopt(gpEigrp->socket, IPPROTO_IP, IP_MULTICAST_IF, (S8 *)&sin_addr, sizeof(sin_addr));
			if(ret < 0){
				_EIGRP_DEBUG("%s: setsockopt error IP_MULTICAST_IF\n",__func__);
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}
#endif //_EIGRP_PLAT_MODULE
			mcastTtl	= 2;
			ret = setsockopt(gpEigrp->socket, IPPROTO_IP, IP_MULTICAST_TTL, (S8 *)&mcastTtl, sizeof(mcastTtl));
			if(ret < 0){
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}
			
			ret = EigrpPortSendIpPacket(gpEigrp->socket, (void *)gpEigrp->sendBuf, ipPktLen, flags, &dest, iidb);
			if(ret <= 0){
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}
			opcode = eigrph->opcode;
			if(eigrph->opcode == EIGRP_OPC_HELLO && eigrph->ack){
				opcode	= EIGRP_OPC_ACK;
			}
			flag	= DEBUG_EIGRP_INTERNAL | EigrpDebugPacketType(opcode,0);
			EIGRP_TRC(flag, "EIGRP-SEND: %-6s PACKET \t%s->GROUP %s(Multicast) \tsize =%d\n",
								EigrpOpercodeItoa(eigrph->opcode),
								iidb->idb->name,
								EigrpUtilIp2Str(NTOHL(dest.sin_addr.s_addr)), ret);
			flag	= EigrpDebugPacketDetailType(opcode);
			EigrpDebugPacketDetail(flag, (struct EigrpPktHdr_ *)gpEigrp->sendBuf, bytes);
		}
		EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
	}
#endif//_DC_	
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	{
		EigrpIntf_pt			pEigrpIntf;
		struct EigrpSockIn_	dest;
		struct EigrpInaddr_	intfAddr;
		U8					flags, mcastTtl;
		S32					ret;
		U32					flag, opcode;
		U16		ifIndex;	/* tigerwh 120518 */

		EIGRP_FUNC_ENTER(EigrpPortSendPacket);
		opcode	= 0;
		pEigrpIntf = iidb->idb;
		if(!pEigrpIntf){
			EIGRP_ASSERT(0);
			EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
			return;
		}

		if(!gpEigrp->sendBuf){
			EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
			return;
		}

		EigrpPortMemCpy((void *) gpEigrp->sendBuf, (void *)eigrph, (U32) bytes);

		if(peer|| !BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST)){
			dest.sin_family	= AF_INET;
#ifdef EIGRP_PACKET_UDP
			dest.sin_port	= HTONS(EIGRP_LOCAL_PORT);
#else
			dest.sin_port	= 0;
#endif
			dest.sin_addr.s_addr = HTONL(peer->source);
			flags = MSG_DONTROUTE;
		
			ret = EigrpPortSendIpPacket(gpEigrp->socket, (void *) gpEigrp->sendBuf, (U32) bytes,
									flags, &dest, iidb);
			if(ret <= 0){
				EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP send unicast result = %d\n", ret);
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}

			opcode = eigrph->opcode;
			if(eigrph->opcode == EIGRP_OPC_HELLO && eigrph->ack){
				opcode	= EIGRP_OPC_ACK;
			}
			flag	= DEBUG_EIGRP_INTERNAL | EigrpDebugPacketType(opcode,0);
			EIGRP_TRC(flag, "EIGRP-SEND: %-6s PACKET \t%s->PEER %s(Unicast) \tsize =%d\n",
						EigrpOpercodeItoa(eigrph->opcode),
						iidb->idb->name,
						EigrpUtilIp2Str(NTOHL(dest.sin_addr.s_addr)),	
						ret);
			flag	= EigrpDebugPacketDetailType(opcode);
			EigrpDebugPacketDetail(flag, (EigrpPktHdr_st *)gpEigrp->sendBuf, bytes);
		}else if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST)){  /* Multicast */
			dest.sin_family		= AF_INET;
#ifdef EIGRP_PACKET_UDP
			dest.sin_port	= HTONS(EIGRP_LOCAL_PORT);
#else
			dest.sin_port	= 0;
#endif
			dest.sin_addr.s_addr	= HTONL(EIGRP_ADDR_HELLO);
			flags					= 0;
//	_EIGRP_DEBUG("EigrpPortSendPacket:send hello: intf name:%s\n", ((struct circ_ *)pEigrpIntf->sysCirc)->circName);			

			/* tigerwh 120518 */
			ifIndex	= ((struct circ_ *)pEigrpIntf->sysCirc)->circId;
			ret = setsockopt(gpEigrp->socket, IPPROTO_IP, IP_MULTICAST_IFINDEX, (S8 *)&ifIndex, sizeof(ifIndex));
			if(ret < 0){
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}
						
			mcastTtl	= 2;
			ret = setsockopt(gpEigrp->socket, IPPROTO_IP, IP_MULTICAST_TTL, (S8 *)&mcastTtl, sizeof(mcastTtl));
			if(ret < 0){
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}
			
			ret = EigrpPortSendIpPacket(gpEigrp->socket, (void *)gpEigrp->sendBuf, (U32)bytes, flags, &dest, iidb);
			if(ret <= 0){
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}

			opcode = eigrph->opcode;
			if(eigrph->opcode == EIGRP_OPC_HELLO && eigrph->ack){
				opcode	= EIGRP_OPC_ACK;
			}

			flag	= DEBUG_EIGRP_INTERNAL | EigrpDebugPacketType(opcode, 0);
			EIGRP_TRC(flag, "EIGRP-SEND: %-6s PACKET \t%s->GROUP %s(Multicast) \tsize =%d\n",
								EigrpOpercodeItoa(eigrph->opcode),
								iidb->idb->name,
								EigrpUtilIp2Str(NTOHL(dest.sin_addr.s_addr)), ret); 
			flag	= EigrpDebugPacketDetailType(opcode);
			EigrpDebugPacketDetail(flag, (EigrpPktHdr_st *)gpEigrp->sendBuf, bytes);
		}
		EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
	}
#elif (EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	{
		EigrpIntf_pt			pEigrpIntf;
		struct EigrpSockIn_	dest;
		struct EigrpInaddr_	intfAddr;
		U8					flags, mcastTtl, ucastTtl;
		S32					ret;
		struct ip_mreq		mreq;
		struct EigrpIpHdr_	*iph;
		S8		*pktdata; /* pointer to eigrp data,not include ip header */
		U32		ipPktLen, flag, opcode;
		U32		ifIndex;

		EIGRP_FUNC_ENTER(EigrpPortSendPacket);
		pEigrpIntf = iidb->idb;
		if(!pEigrpIntf){
			EIGRP_ASSERT(0);
			EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
			return;
		}

		if(!gpEigrp->sendBuf){
			EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
			return;
		}
		pktdata = (S8 *)gpEigrp->sendBuf + sizeof(struct EigrpIpHdr_);
		EigrpPortMemCpy((void *) pktdata, (void *)eigrph, (U32) bytes);
		ipPktLen = (U32) bytes + sizeof(struct EigrpIpHdr_);

		if(peer || !BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST))
		{
			dest.sin_family	= AF_INET;
			dest.sin_port	= 0;
			if(peer){ /* we have known this peer before */			
				dest.sin_addr.s_addr = HTONL(peer->source);
			}else{ /* we should send multicast pkt,but we send it unicast pkt on P2P interface */
				dest.sin_addr.s_addr = HTONL(pEigrpIntf->remoteIpAddr);
			}
			
			flags = MSG_DONTROUTE;
			ret = EigrpPortSendIpPacket(gpEigrp->socket, (void *) gpEigrp->sendBuf, ipPktLen,
									flags, &dest, iidb);
			if(ret <= 0){
				EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP send unicast result = %d\n", ret);
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}

			opcode = eigrph->opcode;
			if(eigrph->opcode == EIGRP_OPC_HELLO && eigrph->ack){
				opcode	= EIGRP_OPC_ACK;
			}
			flag	= DEBUG_EIGRP_INTERNAL | EigrpDebugPacketType(opcode,0);
			EIGRP_TRC(flag, "EIGRP-SEND: %-6s PACKET \t%s->PEER %s(Unicast) \tsize =%d\n",
						EigrpOpercodeItoa(eigrph->opcode),
						iidb->idb->name,
						EigrpUtilIp2Str(NTOHL(dest.sin_addr.s_addr)),
						ret);
			flag	= EigrpDebugPacketDetailType(opcode);
			EigrpDebugPacketDetail(flag, (EigrpPktHdr_st *)gpEigrp->sendBuf, bytes);
		}
		
		else if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST))
		{  /* Multicast on LAN */ 
			dest.sin_family		= AF_INET;
			dest.sin_port		= 0;
			dest.sin_addr.s_addr	= HTONL(EIGRP_ADDR_HELLO);
			flags			= 0;

			if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
				intfAddr.s_addr	= HTONL(pEigrpIntf->ipAddr/**remoteIpAddr**/);
			}else{
				intfAddr.s_addr	= HTONL(pEigrpIntf->ipAddr);
			}

			/*	ifIndex	=HTONL(pEigrpCirc->intfLst->ipAddr);//pEigrpCirc->ifindex; by lihui 20090929
			ifIndex	= pEigrpIntf->ifindex;*/

			ret = setsockopt(gpEigrp->socket, IPPROTO_IP, IP_MULTICAST_IF, (S8 *)&intfAddr/*&ifIndex*/, sizeof(intfAddr)/*sizeof(ifIndex)*/);
			if(ret < 0){
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}

			mcastTtl	= 2;
			ret = setsockopt(gpEigrp->socket, IPPROTO_IP, IP_MULTICAST_TTL, (S8 *)&mcastTtl, sizeof(mcastTtl));
			if(ret < 0){
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}

			ret = EigrpPortSendIpPacket(gpEigrp->socket, (void *)gpEigrp->sendBuf, ipPktLen, flags, &dest, iidb);
			if(ret <= 0){
				EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
				return;
			}

			opcode = eigrph->opcode;
			if(eigrph->opcode == EIGRP_OPC_HELLO && eigrph->ack){
				opcode	= EIGRP_OPC_ACK;
			}
			flag	= DEBUG_EIGRP_INTERNAL | EigrpDebugPacketType(opcode,0);
			EIGRP_TRC(flag, "EIGRP-SEND: %-6s PACKET \t%s->GROUP %s(Multicast) \tsize =%d\n",
								EigrpOpercodeItoa(eigrph->opcode),
								iidb->idb->name,
								EigrpUtilIp2Str(NTOHL(dest.sin_addr.s_addr)), ret);
			flag	= EigrpDebugPacketDetailType(opcode);
			EigrpDebugPacketDetail(flag, (struct EigrpPktHdr_ *)gpEigrp->sendBuf, bytes);
		}
		EIGRP_FUNC_LEAVE(EigrpPortSendPacket);
	}
#endif//(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
}

/************************************************************************************

	Name:	EigrpPortRecvIpPacket

	Desc:	This function is to receive an Eigrp packet. If failed or no packet to receive, it 
			returns -1.
		
	Para: 	sock		- the socket used to receive Eigrp packet
			count	- pointer to the length of received packet
			recv_buffer	- pointer to the received buffer
			ppCirc		- pointer to the information of interface
	
	Ret:		0	for success
			-1	for failed or no packet to receive
************************************************************************************/
#ifdef _EIGRP_PLAT_MODULE
#ifndef EIGRP_PLAT_ZEBRA
static void *eigrp_getsockopt_cmsg_data (struct msghdr *msgh, int level, int type)
{
  struct cmsghdr *cmsg;
  void *ptr = NULL;
  
  for (cmsg = CMSG_FIRSTHDR(msgh); 
       cmsg != NULL;
       cmsg = CMSG_NXTHDR(msgh, cmsg))
    if (cmsg->cmsg_level == level && cmsg->cmsg_type)
      return (ptr = CMSG_DATA(cmsg));

  return NULL;
}
static int eigrp_getsockopt_ifindex (struct msghdr *msgh)
{
#ifdef IP_RECVIF	
#ifdef HAVE_NET_IF_DL_H	
	struct sockaddr_dl *sdl;	
	sdl = (struct sockaddr_dl *)eigrp_getsockopt_cmsg_data (msgh, IPPROTO_IP, IP_RECVIF);
	if (sdl != NULL)
	    return sdl->sdl_index;
	else
		return -1;
#else//HAVE_NET_IF_DL_H
	int *ifindex;
	ifindex = (int *)eigrp_getsockopt_cmsg_data (msgh, IPPROTO_IP, IP_RECVIF);
	if(ifindex)
		return *ifindex;
	else
		return -1;
#endif	
#warning "================================================!"
#warning "Can't get ifindex of recv msg in socket !"
#warning "================================================!"
#endif
	return -1;
}
#endif//EIGRP_PLAT_ZEBRA

static int	EigrpPortSelectRecvIpPacket(S32 sock, S32 *count, void *recv_buffer, void **ppCirc)
{
	if(EigrpPortSelect(sock))
	{
		S32 	ret;	
		U32		recvIfIndex;
		struct ip iph;
		struct iovec	iov;

		char buff[CMSG_SPACE(SOPT_SIZE_CMSG_RECVIF_IPV4())];//[CMSG_SPACE((sizeof (struct sockaddr_dl)))];
		struct msghdr msgh;
		
		EigrpIntf_pt	pEigrpIf;
		
		msgh.msg_name		= NULL;
		msgh.msg_namelen	= 0;
		msgh.msg_iov		= &iov;
		msgh.msg_iovlen		= 1;
		msgh.msg_control	= buff;
		msgh.msg_controllen	= sizeof (buff);
		msgh.msg_flags		= 0;
	
		*count = 0;
		if(ppCirc){
			*ppCirc = NULL;
		}
		EigrpPortMemSet((U8 *)&iph, 0, sizeof(iph));
		ret = recvfrom(sock, (S8 *)&iph, sizeof(iph), MSG_PEEK, NULL, 0);
		if(ret != sizeof(iph)){
			EIGRP_TRC(DEBUG_EIGRP_INTERNAL,"%s: Can't read ip head packet(read size:%d)->%s\n",__func__,ret,strerror(errno));
			return	FAILURE;
		}
		/*
		  printf ("ip_v %d\n", iph.ip_v);
		  printf ("ip_hl %d\n", iph.ip_hl);
		  printf ("ip_tos %d\n", iph.ip_tos);
		  printf ("ip_len %d\n", NTOHS(iph.ip_len));
		  printf ("ip_id %u\n", (u_int32_t) iph.ip_id);
		  printf ("ip_off %u\n", (u_int32_t) iph.ip_off);
		  printf ("ip_ttl %d\n", iph.ip_ttl);
		  printf ("ip_p %d\n", iph.ip_p);
		  printf ("ip_sum 0x%x\n", (u_int32_t) iph.ip_sum);
		  printf ("ip_src %s\n",  inet_ntoa (iph.ip_src));
		  printf ("ip_dst %s\n", inet_ntoa (iph.ip_dst));
		*/

		iov.iov_base = recv_buffer;
		iov.iov_len = NTOHS(iph.ip_len);
	
		ret = recvmsg(sock, &msgh, 0);
		if(ret == FAILURE){
			EIGRP_TRC(DEBUG_EIGRP_INTERNAL,"%s:recvmsg error:%s\n",__func__,strerror(errno));
			return	FAILURE;
		}
#ifndef EIGRP_PLAT_ZEBRA		
		recvIfIndex = eigrp_getsockopt_ifindex (&msgh);
#else//EIGRP_PLAT_ZEBRA
		recvIfIndex = getsockopt_ifindex (AF_INET, &msgh);
#endif//EIGRP_PLAT_ZEBRA		
		if(recvIfIndex != -1)
		{
			pEigrpIf = EigrpIntfFindByIndex(recvIfIndex);
			if(pEigrpIf){
				if(NTOHL(iph.ip_src.s_addr)==pEigrpIf->ipAddr)
				{
					EIGRP_TRC(DEBUG_EIGRP_INTERNAL,"EIGRP: Ignore packet comes from myself, source :%s size =%d\n",
							inet_ntoa (iph.ip_src), NTOHS(iph.ip_len));
					return	FAILURE;
				}
				*ppCirc = pEigrpIf->sysCirc;
			}
		}
		else
		{
			EIGRP_TRC(DEBUG_EIGRP_INTERNAL,"EIGRP: :recvmsg packet size :%s bust Can't get recv interface\n",NTOHS(iph.ip_len));
		}
		/* zhurish edit 201601015 */
#if 0
		if(ppCirc)
		{
			cmsg = CMSG_FIRSTHDR(&msgh);
			while(cmsg){
				if(cmsg->cmsg_level == IPPROTO_IP){
					if(cmsg->cmsg_type == IP_RECVIF){
						recvIfIndex = (*(U32 *)CMSG_DATA(cmsg) & 0x0000ffff);
						pEigrpIf = EigrpIntfFindByIndex(recvIfIndex);
						if(pEigrpIf){
							if(NTOHL(iph.ip_src.s_addr)==pEigrpIf->ipAddr)
							{
								return	FAILURE;
							}
							*ppCirc = pEigrpIf->sysCirc;
						}
						break;
					}
				}
				cmsg = CMSG_NXTHDR(&msgh, cmsg);
			}
		}
#endif//0
		*count = NTOHS(iph.ip_len);
		return	SUCCESS;
	}
	return	FAILURE;
}
#endif// _EIGRP_PLAT_MODULE

#ifdef EIGRP_PACKET_UDP
S32	EigrpPortRecvIpPacket(S32 sock, S32 *count, void *recv_buffer, U32 *srcAddr, U16 *srcPort, void **ppCirc)
{
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
#ifdef _DC_
	{
		struct EigrpIpHdr_	iph;
		struct iovec		iov;
		struct cmsghdr	*cmsg;
		S8		buff[sizeof(*cmsg) + 128];
		struct msghdr msgh = {NULL, 0, &iov, 1, buff, sizeof(*cmsg) + 128, 0};
		U32		recvIfIndex;
		S32 		ret;
		U16 		ip_len;
		EigrpIntf_pt	pEigrpIf;
		struct sockaddr_in	from;
		U32	fromLen;
		
		EIGRP_FUNC_ENTER(EigrpPortRecvIpPacket);
		*count = 0;
		if(ppCirc){
			*ppCirc = NULL;
		}
		fromLen = sizeof(struct sockaddr_in);
		EigrpPortMemSet((U8 *)&from, 0, fromLen);
		ret = recvfrom(sock, (S8 *)recv_buffer, gpEigrp->recvBufLen, MSG_PEEK, &from, (S32 *)&fromLen);
		if(ret < 0){
			return	FAILURE;
		}
		*srcAddr = HTONL(from.sin_addr.s_addr);
		*srcPort = HTONS(from.sin_port);
		*count = ret;
		
		ip_len = NTOHS(iph.length);
		iov.iov_base = recv_buffer;
		iov.iov_len = ret;
	
		ret = recvmsg(sock, &msgh, 0);
		if(ret == FAILURE){
			EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
			return	FAILURE;
		}
	
		if(ppCirc){
			cmsg = CMSG_FIRSTHDR(&msgh);
			while(cmsg){
				if(cmsg->cmsg_level == IPPROTO_IP){
					if(cmsg->cmsg_type == IP_RECVIF){
						recvIfIndex = (*(U32 *)CMSG_DATA(cmsg) & 0x0000ffff);
						pEigrpIf = EigrpIntfFindByIndex(recvIfIndex);
						if(pEigrpIf){
							*ppCirc = pEigrpIf->sysCirc;
						}
						break;
					}
				}
				cmsg = CMSG_NXTHDR(&msgh, cmsg);
			}
		}
		EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
		return	SUCCESS;
	}
#else//_DC_
	{
		S32		ret;
		struct EigrpIpHdr_	*pIph;
		S32		expectLen;
		U8		buf[256];
		S32		buflen = 256;
		
		EIGRP_FUNC_ENTER(EigrpPortRecvIpPacket);
		pIph = (struct EigrpIpHdr_*)recv_buffer;

		*count = 0;
		while((ret= recvfrom(sock, (void *)recv_buffer, gpEigrp->recvBufLen,
				0, (struct sockaddr *) (void *)buf, (void *) &buflen)) < 0){
			switch(errno){
				case EINTR:
					/* The call was interrupted, probably by a signal, silently retry it. */
					break;

				case EHOSTUNREACH:
				case ENETUNREACH:
					/* These errors are just an indication that an unreachable was received.  When
					  * an operation is attempted on a socket with an error pending it does not
					  * complete.  So we need to retry. */
					break;

				case EWOULDBLOCK:
#if	defined(EAGAIN) && EAGAIN != EWOULDBLOCK
				case EAGAIN:		/* System V style */
#endif	/* EAGAIN */
					/* Nothing to read */
					*count = ret;
					EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
					return FAILURE; /* JB_MODIFY 2003/5/14 error ->rc */

				default:
					/* Fall through */
					EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
					return -1;
			}
	    	}
#if 0				
		expectLen = sizeof(struct EigrpIpHdr_) +  pIph->length;
		
		if (ret != expectLen){
		EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV, "EigrpPortRecvIpPacket packet smaller than ip header\n");
			EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
			return -1;
		}
#endif//0
		*ppCirc	= NULL;
		*count = ret;		
		EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
		return	SUCCESS;		
	}
#endif//_DC_

#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	{
		S32				ret;
		struct EigrpIpHdr_	iph;
		U16				ip_len;
		struct iovec		iov;
		struct cmsghdr	*cmsg;
		S8	buff [ sizeof(*cmsg) + 16];
		U32	circId;
		struct sockaddr_in	from;
		U32	fromLen;
		void *tmpBuf;
		
		struct msghdr msgh ={
			NULL, 0, &iov, 1, buff,
			sizeof(*cmsg) + 16, 0
		};
		EIGRP_FUNC_ENTER(EigrpPortRecvIpPacket);
		*count = 0;
#if 0
		EigrpPortMemSet((U8 *)&iph, 0, sizeof(iph));
		ret = recvfrom(sock, (void *) &iph, sizeof(iph), MSG_PEEK, NULL, 0);

		if(ret != sizeof(iph)){
			EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
			return -1;
		}

		ip_len = NTOHS (iph.length);
#else//0
		fromLen = sizeof(struct sockaddr_in);
		EigrpPortMemSet((U8 *)&from, 0, fromLen);
		ret = recvfrom(sock, (S8 *)recv_buffer, gpEigrp->recvBufLen,
				MSG_PEEK, &from, (S32 *)&fromLen);
		if(ret < 0){
			EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
			return -1;
		}
		*srcAddr = HTONL(from.sin_addr.s_addr);
		*srcPort = HTONS(from.sin_port);
		*count = ret;
#endif//0
		iov.iov_base	= recv_buffer;
		iov.iov_len	= ret;

		ret = recvmsg(sock, &msgh, 0);

		cmsg = CMSG_FIRSTHDR (&msgh);

		if(cmsg != NULL && cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVIF){
			circId	= *((U32 *)CMSG_DATA(cmsg));
			if(ppCirc){
				*ppCirc = (void *)PILSysGetCircFromCircId(circId);
				if(!*ppCirc){
					EigrpPortAssert(0, "EigrpPortRecvIpPacket: Can`t get circ from circId or circId is zero.\n");
					return -1;
				}
			}else{
				EigrpPortAssert(0, "EigrpPortRecvIpPacket: ppCirc is null.\n");
				return -1;
			}
		}else{
			EigrpPortAssert(0, "EigrpPortRecvIpPacket: recv conditons are not enough.\n");
			return -1;
		}

		if(ret != 0){
		EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV, "EigrpPortRecvIpPacket read error. \n");
			EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
			return -1;
		}

		EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
		return	SUCCESS;
	}
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	{
		S32		ret,sock1;
		struct EigrpIpHdr_	*pIph;
		S32		expectLen;
		U8		buf[256];
		S32		buflen = 256;
		
		EIGRP_FUNC_ENTER(EigrpPortRecvIpPacket);
		pIph = (struct EigrpIpHdr_*)recv_buffer;

		while((ret= recvfrom(sock, (void *)recv_buffer, gpEigrp->recvBufLen,
				0, (struct sockaddr *) (void *)buf, (void *) &buflen)) < 0){
			switch(errno){
				case EINTR:
					/* The call was interrupted, probably by a signal, silently retry it. */
					break;
				case SIGALRM:
					break;
				case EHOSTUNREACH:
				case ENETUNREACH:
					/* These errors are just an indication that an unreachable was received.  When
					  * an operation is attempted on a socket with an error pending it does not
					  * complete.  So we need to retry. */
					break;

				case EWOULDBLOCK:
#if	defined(EAGAIN) && EAGAIN != EWOULDBLOCK
				case EAGAIN:		/* System V style */
#endif	/* EAGAIN */
				
					/* Nothing to read */
					*count = ret;
					EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
					return FAILURE; /* JB_MODIFY 2003/5/14 error ->rc */ 

				default:
					/* Fall through */
					EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
					return -1;
			}

	    }
				
		expectLen =  NTOHS(pIph->length);//by lihui 20090925
		if (ret != expectLen){
			EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV,
						"EigrpPortRecvIpPacket packet smaller than ip header\n");
			EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
			return -1;
		}

		*ppCirc	= NULL;
		*count = expectLen;		
		EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
		return	SUCCESS;
	}
#endif//(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	return 0;
}

#else//EIGRP_PACKET_UDP

S32	EigrpPortRecvIpPacket(S32 sock, S32 *count, void *recv_buffer, void **ppCirc)
{
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
#ifdef _DC_
	{
		struct EigrpIpHdr_	iph;
		struct iovec		iov;
		struct cmsghdr	*cmsg;
		S8		buff[sizeof(*cmsg) + 128];
		struct msghdr msgh;
		U32		recvIfIndex;
		S32 		ret;
		U16 		ip_len;
		EigrpIntf_pt	pEigrpIf;
		msgh.msg_name		= NULL;
		msgh.msg_namelen	= 0;
		msgh.msg_iov			= &iov;
		msgh.msg_iovlen		= 1;
		msgh.msg_control		= buff;
		msgh.msg_controllen	= sizeof(*cmsg) + 128;
		msgh.msg_flags		= 0;
	
		*count = 0;
		if(ppCirc){
			*ppCirc = NULL;
		}
		EigrpPortMemSet((U8 *)&iph, 0, sizeof(iph));
	
		ret = recvfrom(sock, (S8 *)&iph, sizeof(iph), MSG_PEEK, NULL, 0);
		if(ret != sizeof(iph)){
			return	FAILURE;
		}
	
		ip_len = NTOHS(iph.length);
		iov.iov_base = recv_buffer;
		iov.iov_len = ip_len;
	
		ret = recvmsg(sock, &msgh, 0);
		if(ret == FAILURE){
			return	FAILURE;
		}
		if(ppCirc){
			cmsg = CMSG_FIRSTHDR(&msgh);
			while(cmsg){
				if(cmsg->cmsg_level == IPPROTO_IP){
					if(cmsg->cmsg_type == IP_RECVIF){
						recvIfIndex = (*(U32 *)CMSG_DATA(cmsg) & 0x0000ffff);
						pEigrpIf = EigrpIntfFindByIndex(recvIfIndex);
						if(pEigrpIf){
							*ppCirc = pEigrpIf->sysCirc;
						}
						break;
					}
				}
	
				cmsg = CMSG_NXTHDR(&msgh, cmsg);
			}
		}
		*count = iph.length;
		return	SUCCESS;
	}
#else//_DC_
	{
		S32		ret;
		struct EigrpIpHdr_	*pIph;
		S32		expectLen = 0;
		U8		buf[256];
		S32		buflen = 256;
		EIGRP_FUNC_ENTER(EigrpPortRecvIpPacket);
		pIph = (struct EigrpIpHdr_*)recv_buffer;

		*count = 0;
#ifndef _EIGRP_PLAT_MODULE		
		while((ret= recvfrom(sock, (void *)recv_buffer, gpEigrp->recvBufLen,
				0, (struct sockaddr *) (void *)buf, (void *) &buflen)) < 0){
#else	//_EIGRP_PLAT_MODULE		
		ret = EigrpPortSelectRecvIpPacket(sock, count, recv_buffer, ppCirc);	
		if(ret == FAILURE) {
#endif// _EIGRP_PLAT_MODULE
			switch(errno){
				case EINTR:
					/* The call was interrupted, probably by a signal, silently retry it. */
					break;

				case EHOSTUNREACH:
				case ENETUNREACH:
					/* These errors are just an indication that an unreachable was received.  When
					  * an operation is attempted on a socket with an error pending it does not
					  * complete.  So we need to retry. */
					break;

				case EWOULDBLOCK:
#if	defined(EAGAIN) && EAGAIN != EWOULDBLOCK
				case EAGAIN:		/* System V style */
#endif	/* EAGAIN */
					/* Nothing to read */
					*count = ret;
					EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
					return FAILURE; /* JB_MODIFY 2003/5/14 error ->rc */

				default:
					/* Fall through */
					EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
					return -1;
			}
	    	}
				
		//expectLen = sizeof(struct EigrpIpHdr_) +  pIph->length;
		expectLen = NTOHS(pIph->length);		

		if (*count != expectLen){
		EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV, "EigrpPortRecvIpPacket packet smaller than ip header(ret:%d ip.len:%d)\n",*count,expectLen);
			EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
			return -1;
		}
		EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV, "EigrpPortRecvIpPacket packet ip header(ret:%d ip.len:%d)\n",*count,expectLen);
#ifndef _EIGRP_PLAT_MODULE	
		*ppCirc	= NULL;
		*count = expectLen;		
#endif// _EIGRP_PLAT_MODULE	
		EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
		return	SUCCESS;
	}
#endif//_DC_
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	{
		S32				ret;
		struct EigrpIpHdr_	iph;
		U16				ip_len;
		struct iovec		iov;
		struct cmsghdr	*cmsg;
		S8	buff [ sizeof(*cmsg) + 16];
		U32	circId;
		
		struct msghdr msgh ={
			NULL, 0, &iov, 1, buff,
			sizeof(*cmsg) + 16, 0
		};
		EIGRP_FUNC_ENTER(EigrpPortRecvIpPacket);
		*count = 0;
		EigrpPortMemSet((U8 *)&iph, 0, sizeof(iph));
		ret = recvfrom(sock, (void *) &iph, sizeof(iph), MSG_PEEK, NULL, 0);

		if(ret != sizeof(iph)){
			EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
			return -1;
		}

		ip_len = NTOHS (iph.length);

		iov.iov_base	= recv_buffer;
		iov.iov_len	= ip_len;

		ret = recvmsg(sock, &msgh, 0);

		cmsg = CMSG_FIRSTHDR (&msgh);

		if(cmsg != NULL && cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVIF){
			circId	= *((U32 *)CMSG_DATA(cmsg));
			if(ppCirc){
				*ppCirc = (void *)PILSysGetCircFromCircId(circId);
				if(!*ppCirc){
					EigrpPortAssert(0, "EigrpPortRecvIpPacket: Can`t get circ from circId or circId is zero.\n");
					return -1;
				}
			}else{
				EigrpPortAssert(0, "EigrpPortRecvIpPacket: ppCirc is null.\n");
				return -1;
			}
		}else{
			EigrpPortAssert(0, "EigrpPortRecvIpPacket: recv conditons are not enough.\n");
			return -1;
		}

		if(ret != 0){
		EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV, "EigrpPortRecvIpPacket read error. " "ip_len %d bytes read %d\n", ip_len, ret);
			EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
			return -1;
		}

		*count = sizeof(iph) + iph.length;
		EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
		return	SUCCESS;
	}
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	{
		S32		ret,sock1;
		struct EigrpIpHdr_	*pIph;
		S32		expectLen;
		U8		buf[256];
		S32		buflen = 256;
		
		EIGRP_FUNC_ENTER(EigrpPortRecvIpPacket);
		pIph = (struct EigrpIpHdr_*)recv_buffer;

		while((ret= recvfrom(sock, (void *)recv_buffer, gpEigrp->recvBufLen,
				0, (struct sockaddr *) (void *)buf, (void *) &buflen)) < 0){
			switch(errno){
				case EINTR:
					/* The call was interrupted, probably by a signal, silently retry it. */
					break;
				case SIGALRM:
					break;
				case EHOSTUNREACH:
				case ENETUNREACH:
					/* These errors are just an indication that an unreachable was received.  When
					  * an operation is attempted on a socket with an error pending it does not
					  * complete.  So we need to retry. */
					break;

				case EWOULDBLOCK:
#if	defined(EAGAIN) && EAGAIN != EWOULDBLOCK
				case EAGAIN:		/* System V style */
#endif	/* EAGAIN */
				
					/* Nothing to read */
					*count = ret;
					EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
					return FAILURE; /* JB_MODIFY 2003/5/14 error ->rc */ 

				default:
					/* Fall through */
					EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
					return -1;
			}

	    }
				
		expectLen =  NTOHS(pIph->length);//by lihui 20090925
		if (ret != expectLen){
			EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV,
						"EigrpPortRecvIpPacket packet smaller than ip header pIph:%d != %d\n",expectLen,ret);
			EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
			return -1;
		}

		*ppCirc	= NULL;
		*count = expectLen;		
		EIGRP_FUNC_LEAVE(EigrpPortRecvIpPacket);
		return	SUCCESS;
	}
#endif//(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	return 0;
}
#endif//EIGRP_PACKET_UDP
/************************************************************************************

	Name:	EigrpPortSock2Ip

	Desc:	This function is to retrieve ip address from a sock_in data.
		
	Para: 	pSock		- pointer to the sock_in data
	
	Ret:		ip address of the sock_in data
************************************************************************************/

U32	EigrpPortSock2Ip(EigrpSockIn_pt pSock)
{
	EIGRP_FUNC_ENTER(EigrpPortSock2Ip);
	EIGRP_ASSERT((U32)pSock);
	EIGRP_FUNC_LEAVE(EigrpPortSock2Ip);
	
	return pSock->sin_addr.s_addr;
}

/************************************************************************************

	Name:	EigrpPortGetFirstAddr

	Desc:	This function is to get the first ip addrress of the given physical interface.
		
	Para: 	pCirc	- pointer to the physical interface
	
	Ret:		pointer to structure of the first ip address,if failed or the physical interface is
			not exist, return NULL
************************************************************************************/

EigrpIntfAddr_pt	EigrpPortGetFirstAddr(void *pCirc)
{
#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
#ifndef _EIGRP_PLAT_MODULE	
		struct	ifnet	*pIfNet;
		struct	ifaddr	*pIfAddr;
		
		EigrpIntfAddr_pt	pAddr;
		U32 *ipMask;
 
		EIGRP_FUNC_ENTER(EigrpPortGetFirstAddr);
		if(!pCirc){
			EIGRP_FUNC_LEAVE(EigrpPortGetFirstAddr);
			return NULL;
		}
		pIfNet	= (struct ifnet *)pCirc;
		pIfAddr	= pIfNet->if_addrlist;
		while(pIfAddr){
			if(pIfAddr->ifa_addr->sa_family != AF_INET){
				pIfAddr	= pIfAddr->ifa_next;
				continue;
			}
			break;
		}
		if(!pIfAddr){
			EIGRP_FUNC_LEAVE(EigrpPortGetFirstAddr);
			return NULL;
		}
		
		pAddr	= EigrpPortMemMalloc(sizeof(EigrpIntfAddr_st));
		if(!pAddr){
			EIGRP_FUNC_LEAVE(EigrpPortGetFirstAddr);
			return NULL;
		}
		if(BIT_TEST(pIfNet->if_flags, IFF_POINTOPOINT)){
			pAddr->ipAddr	= NTOHL(((struct sockaddr_in *)pIfAddr->ifa_addr)->sin_addr.s_addr);
			pAddr->ipDstAddr	= NTOHL(((struct sockaddr_in *)pIfAddr->ifa_dstaddr)->sin_addr.s_addr);
		}else{
			pAddr->ipAddr	= NTOHL(((struct sockaddr_in *)pIfAddr->ifa_addr)->sin_addr.s_addr);
			pAddr->ipDstAddr	= 0;
		}
		pAddr->ipMask	= NTOHL(((struct sockaddr_in *)pIfAddr->ifa_netmask)->sin_addr.s_addr);
		pAddr->curSysAdd	= pIfAddr;
		EIGRP_FUNC_LEAVE(EigrpPortGetFirstAddr);
		return pAddr;
#else// _EIGRP_PLAT_MODULE
		return zebraEigrpPortGetFirstAddr(pCirc);
#endif// _EIGRP_PLAT_MODULE		
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifndef _EIGRP_PLAT_MODULE		
		EigrpIntfAddr_pt	pAddr;
		struct connected	*pConn;
		U32 *ipMask;

		EIGRP_FUNC_ENTER(EigrpPortGetFirstAddr);
		pConn	= ((struct interface *)pCirc)->ifc_ipv4;
		while(pConn){
			if(pConn->address->family == AF_INET){
				break;
			}
			pConn	= pConn->next;
		}
		if(!pConn){
			EIGRP_FUNC_LEAVE(EigrpPortGetFirstAddr);
			return NULL;
		}
		
		pAddr	= EigrpPortMemMalloc(sizeof(EigrpIntfAddr_st));
		if(!pAddr){
			EIGRP_FUNC_LEAVE(EigrpPortGetFirstAddr);
			return NULL;
		}
		if(if_is_pointopoint ((struct interface *)pCirc)){
			pAddr->ipDstAddr	= pConn->destination->u.prefix4.s_addr;
			pAddr->ipAddr	= pConn->address->u.prefix4.s_addr;
			pAddr->ipMask	= EIGRP_PREFIX_TO_MASK(pConn->destination->prefixlen);
		}else{
			pAddr->ipAddr	= pConn->address->u.prefix4.s_addr;
			pAddr->ipDstAddr	= 0;
			pAddr->ipMask	= EIGRP_PREFIX_TO_MASK(pConn->address->prefixlen);
		}
		pAddr->ipMask	= EIGRP_PREFIX_TO_MASK(pConn->address->prefixlen);
		pAddr->intf		= pCirc;
		pAddr->curSysAdd	= pConn;
		EIGRP_FUNC_LEAVE(EigrpPortGetFirstAddr);
			
		return pAddr;
#else// _EIGRP_PLAT_MODULE
		return zebraEigrpPortGetFirstAddr(pCirc);
#endif// _EIGRP_PLAT_MODULE	
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		intf_pt	pIntf;
		EigrpIntfAddr_pt	pAddr;

		EIGRP_FUNC_ENTER(EigrpPortGetFirstAddr);
		pAddr = NULL;
		for(pIntf	 = PILIfMGetNextIntfById(0); pIntf; pIntf = PILIfMGetNextIntfById(pIntf->intfId)){
			if(pIntf->pCirc != pCirc){
				continue;
			}
			
			if(pAddr == 0 || (((struct sockaddr_in *)pIntf->laddr)->sin_addr.s_addr) < pAddr->ipAddr){
				if(!pAddr){
					pAddr	= EigrpPortMemMalloc(sizeof(EigrpIntfAddr_st));
					if(!pAddr){	 
						EIGRP_FUNC_LEAVE(EigrpPortGetFirstAddr);
						return NULL;
					}
				}
				if(BIT_TEST(pIntf->pCirc->circFlag, CIRC_STATUS_POINT2POINT)){
					pAddr->ipAddr	= (((struct sockaddr_in *)pIntf->laddr)->sin_addr.s_addr);
					pAddr->ipDstAddr	= (((struct sockaddr_in *)pIntf->dstAddr)->sin_addr.s_addr);

				}else{
					pAddr->ipAddr	= (((struct sockaddr_in *)pIntf->laddr)->sin_addr.s_addr);
				}
				pAddr->ipMask	= (((struct sockaddr_in *)pIntf->netMask)->sin_addr.s_addr);
				pAddr->intf		= pIntf;
			}
		}
		EIGRP_FUNC_LEAVE(EigrpPortGetFirstAddr);

		return pAddr;
	}
#endif
}

/************************************************************************************

	Name:	EigrpPortGetNextAddr

	Desc:	This function is to get the next ip address of the given physical interface and ip address.
		
	Para: 	pAddr	- pointer to the logical interface which supplies the information of 
			relevant physical interface
	
	Ret:		pointer to the structure of next ip address
************************************************************************************/

EigrpIntfAddr_pt	EigrpPortGetNextAddr(EigrpIntfAddr_pt pAddr)
{
#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
#ifndef _EIGRP_PLAT_MODULE	
		struct	ifaddr	*pIfAddr;

		EIGRP_FUNC_ENTER(EigrpPortGetNextAddr);
		if(!pAddr){
			EIGRP_FUNC_LEAVE(EigrpPortGetNextAddr);
			return NULL;
		}

		pIfAddr	= (struct ifaddr *)pAddr->curSysAdd;
		while(pIfAddr = pIfAddr->ifa_next){
			if(pIfAddr->ifa_addr->sa_family != AF_INET){
				continue;
			}

			break;
		}

		if(pIfAddr){
			if(BIT_TEST(pIfAddr->ifa_ifp->if_flags, IFF_POINTOPOINT)){
				pAddr->ipAddr	= NTOHL(((struct sockaddr_in *)pIfAddr->ifa_addr)->sin_addr.s_addr);
				pAddr->ipDstAddr	= NTOHL(((struct sockaddr_in *)pIfAddr->ifa_dstaddr)->sin_addr.s_addr);
			}else{
				pAddr->ipAddr	= NTOHL(((struct sockaddr_in *)pIfAddr->ifa_addr)->sin_addr.s_addr);
				pAddr->ipDstAddr	= 0;
			}

			pAddr->ipMask		= NTOHL((((struct sockaddr_in *)pIfAddr->ifa_netmask)->sin_addr.s_addr));
			pAddr->curSysAdd	= pIfAddr;
		}else{
			EigrpPortMemFree(pAddr);
			pAddr	= NULL;
		}
		EIGRP_FUNC_LEAVE(EigrpPortGetNextAddr);
		return pAddr;
#else		
		return zebraEigrpPortGetNextAddr(pAddr);
#endif//_EIGRP_PLAT_MODULE	
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifndef _EIGRP_PLAT_MODULE	
		EigrpIntfAddr_pt	pNext;
		struct connected *pConn;
		U32 *ipMask;

		EIGRP_FUNC_ENTER(EigrpPortGetNextAddr);
		
		pConn	= (struct connected *)pAddr->curSysAdd;

		if(!pConn){
			EigrpPortMemFree(pAddr);
			EIGRP_FUNC_LEAVE(EigrpPortGetNextAddr);
			return NULL;
		}

		while(pConn	= pConn->next){
			if(pConn->address->family != AF_INET){
				continue;
			}

			break;
		}
		
		if(pConn){
			if(if_is_pointopoint(pConn->ifp)){
				pAddr->ipDstAddr	= pConn->destination->u.prefix4.s_addr;
				pAddr->ipAddr	= pConn->address->u.prefix4.s_addr;
				pAddr->ipMask	= EIGRP_PREFIX_TO_MASK(pConn->destination->prefixlen);
			}else{
				pAddr->ipAddr	= pConn->address->u.prefix4.s_addr;
				pAddr->ipDstAddr	= 0;
				pAddr->ipMask	= EIGRP_PREFIX_TO_MASK(pConn->address->prefixlen);
			}
		}else{
			EigrpPortMemFree(pAddr);
			EIGRP_FUNC_LEAVE(EigrpPortGetNextAddr);
			return NULL;
		}
		
		pAddr->curSysAdd	= pConn;
		EIGRP_FUNC_LEAVE(EigrpPortGetNextAddr);
			
		return pAddr;
#else		
		return zebraEigrpPortGetNextAddr(pAddr);
#endif//_EIGRP_PLAT_MODULE	
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		intf_pt	pIntf, pIntfTem;

		EIGRP_FUNC_ENTER(EigrpPortGetNextAddr);
		pIntfTem = NULL;
		for(pIntf	 = PILIfMGetNextIntfById(0); pIntf; pIntf = PILIfMGetNextIntfById(pIntf->intfId)){
			if(pIntf->pCirc != ((intf_pt)pAddr->intf)->pCirc){
				continue;
			}

			if((((struct sockaddr_in *)pIntf->laddr)->sin_addr.s_addr) <= pAddr->ipAddr){
				continue;
			}

			if(!pIntfTem || (((struct sockaddr_in *)pIntf->laddr)->sin_addr.s_addr) < 
								(((struct sockaddr_in *)pIntfTem->laddr)->sin_addr.s_addr)){
				pIntfTem = pIntf;
			}
		}

		if(pIntfTem){
			if(pIntf){	
				pAddr->ipAddr	= (((struct sockaddr_in *)pIntf->laddr)->sin_addr.s_addr);
				pAddr->ipMask	= (((struct sockaddr_in *)pIntf->netMask)->sin_addr.s_addr);
			}
		}else{
			EigrpPortMemFree(pAddr);
			pAddr	= NULL;
		}
		EIGRP_FUNC_LEAVE(EigrpPortGetNextAddr);
		return pAddr;
	}
#endif
}

S32 EigrpPortSetIfBandwidth(void *sysIf, U32 bw)
{
#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
	struct interface	*ifp = (struct interface *)sysIf;
	if(!ifp){
		_EIGRP_DEBUG("EigrpPortSetIfBandwidth:  ifp= null\n");
		return	FAILURE;
	}
	ifp->bandwidth = bw;
	return	SUCCESS;
#else//EIGRP_PLAT_ZEBOS
	return	FAILURE;
#endif//EIGRP_PLAT_ZEBOS
#else//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	return	FAILURE;
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
}
#ifndef _EIGRP_PLAT_MODULE
S32 EigrpPortSetIfBandwidth_2(void *sysIf, void *srcIf)
{
#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
	struct interface	*ifp = (struct interface *)sysIf;
	struct interface	*ifp_src = (struct interface *)srcIf;
	if(!ifp){
		return	FAILURE;
	}
	_EIGRP_DEBUG("EigrpPortSetIfBandwidth_2:  ifp=%ld   ifp_src=%ld\n", ifp->bandwidth, ifp_src->bandwidth);
	ifp->bandwidth = ifp_src->bandwidth;
	return	SUCCESS;
#else//EIGRP_PLAT_ZEBOS
	return	FAILURE;
#endif//EIGRP_PLAT_ZEBOS
#else//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	return	FAILURE;
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
}

S32 EigrpPortSetIfBandwidthAndDelay_2(void *sysIf, void *srcIf)
{
#if  (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
	struct interface	*ifp = (struct interface *)sysIf;
	struct interface	*ifp_src = (struct interface *)srcIf;
	if(!ifp){
		return	FAILURE;
	}
	ifp->bandwidth = ifp_src->bandwidth;
	ifp->delay = ifp_src->delay;	
	return	SUCCESS;
#else//EIGRP_PLAT_ZEBOS
	return	FAILURE;
#endif//EIGRP_PLAT_ZEBOS
#else//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	return	FAILURE;
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
}
#endif// _EIGRP_PLAT_MODULE
S32 EigrpPortChangeIfBandwidth(void *sysIf, U32 bandwidth)
{
#if  (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
	struct interface	*ifp = (struct interface *)sysIf;
	if(!ifp){
		return	FAILURE;
	}
	ifp->bandwidth = bandwidth;
	_EIGRP_DEBUG("EigrpPortChangeIfBandwidth:set bandwidth=%ld for sysInterface\n ", bandwidth);
	return	SUCCESS;
#else//EIGRP_PLAT_ZEBOS
	return	FAILURE;
#endif//EIGRP_PLAT_ZEBOS
#else//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	return	FAILURE;
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
}

S32 EigrpPortSetIfDelay(void *sysIf, U32 delay)
{
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
	struct interface	*ifp = (struct interface *)sysIf;
	if(!ifp){
		return	FAILURE;
	}
	ifp->delay = delay;
	return	SUCCESS;
#else//EIGRP_PLAT_ZEBOS
	return	FAILURE;
#endif//EIGRP_PLAT_ZEBOS
#else//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	return	FAILURE;
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
}
#ifndef _EIGRP_PLAT_MODULE
S32 EigrpPortSetIfDelay_2(void *sysIf, void *srcIf)
{
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
	struct interface	*ifp = (struct interface *)sysIf;
	struct interface	*ifp_src = (struct interface *)srcIf;
	if(!ifp){
		return	FAILURE;
	}
	_EIGRP_DEBUG("EigrpPortSetIfDelay_2:----ifp->delay=%d,ifp_src->delay=%d\n",ifp->delay,ifp_src->delay);
	ifp->delay = ifp_src->delay;
	_EIGRP_DEBUG("EigrpPortSetIfDelay_2:----ifp->delay=%d,ifp_src->delay=%d\n",ifp->delay,ifp_src->delay);
	return	SUCCESS;
#else//EIGRP_PLAT_ZEBOS
	return	FAILURE;
#endif//EIGRP_PLAT_ZEBOS
#else//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	return	FAILURE;
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
}
#endif// _EIGRP_PLAT_MODULE

S32 EigrpPortChangeIfDelay(void *sysIf, U32 delay)
{
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
	struct interface	*ifp = (struct interface *)sysIf;
	if(!ifp){
		return	FAILURE;
	}
	ifp->delay = delay;
	return	SUCCESS;
#else//EIGRP_PLAT_ZEBOS
	return	FAILURE;
#endif//EIGRP_PLAT_ZEBOS
#else//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	return	FAILURE;
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
}
/************************************************************************************

	Name:	EigrpPortCopyIntfInfo

	Desc:	This function is get the infomation that eigrp interests in of the physical interface.
		
	Para: 	pEigrpIntf	- pointer to the Eigrp interface which strores received interface 
			information
			pSysIntf		- pointer to the system interface
	
	Ret:		
************************************************************************************/

void	EigrpPortCopyIntfInfo(EigrpIntf_pt pEigrpIntf, void *pSysIntf)
{
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
#ifndef _EIGRP_PLAT_MODULE	
		struct	ifnet	*pIfNet;
		struct	ifaddr	*pIfAddr;

		EIGRP_FUNC_ENTER(EigrpPortCopyIntfInfo);
		EigrpPortMemSet((U8 *)pEigrpIntf, 0, sizeof(EigrpIntf_st));
		if(!pSysIntf){
			EIGRP_FUNC_LEAVE(EigrpPortCopyIntfInfo);
			return;
		}
		
		pIfAddr	= (struct ifaddr *)pSysIntf;
		pIfNet	= (struct ifnet *)pIfAddr->ifa_ifp;
		if(!pIfNet){
			EIGRP_FUNC_LEAVE(EigrpPortCopyIntfInfo);
			return;
		}
		
		sprintf_s(pEigrpIntf->name, sizeof(pEigrpIntf->name), "%s%d", pIfNet->if_name, pIfNet->if_unit);
		pEigrpIntf->ifindex 	= pIfNet->if_index;
		
		if(BIT_TEST(pIfNet->if_flags, IFF_UP)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE);
		}
		
		if(BIT_TEST(pIfNet->if_flags, IFF_BROADCAST)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_BROADCAST);
		}else if(BIT_TEST(pIfNet->if_flags, IFF_POINTOPOINT)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT);
		}
		
		if(BIT_TEST(pIfNet->if_flags, IFF_MULTICAST)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST);
		}
		
		if(BIT_TEST(pIfNet->if_flags, IFF_LOOPBACK)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_LOOPBACK);
		}
		
		pEigrpIntf->mtu 			= pIfNet->if_mtu;
		pEigrpIntf->bandwidth	= pIfNet->if_baudrate / 1000;
		pEigrpIntf->metric		= pIfNet->if_metric;
		
		pEigrpIntf->ipAddr	= NTOHL(((struct sockaddr_in *)pIfAddr->ifa_addr)->sin_addr.s_addr);
		pEigrpIntf->ipMask	= NTOHL(((struct sockaddr_in *)pIfAddr->ifa_netmask)->sin_addr.s_addr);
		if(pEigrpIntf->ipMask == 0){
			pEigrpIntf->ipMask	= EigrpUtilGetNaturalMask(pEigrpIntf->ipMask);
		}

		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
			pEigrpIntf->remoteIpAddr	= NTOHL(((struct sockaddr_in *)pIfAddr->ifa_dstaddr)->sin_addr.s_addr);
		}
		pEigrpIntf->sysCirc = pIfNet;
#else// _EIGRP_PLAT_MODULE
		zebraEigrpPortCopyIntfInfo(pEigrpIntf, pSysIntf);		
#endif// _EIGRP_PLAT_MODULE
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifndef _EIGRP_PLAT_MODULE	
		struct connected	*pConn;
		U32 *ipMask;
		struct interface	*pInterface;
		EIGRP_FUNC_ENTER(EigrpPortCopyIntfInfo);

		EigrpPortMemSet((U8 *)pEigrpIntf,0,sizeof(EigrpIntf_st));
		pInterface	= (struct interface *)pSysIntf;
		
		EigrpPortStrCpy(pEigrpIntf->name, pInterface->name);
		pEigrpIntf->ifindex 	= pInterface->ifindex;

		if(if_is_up(pInterface)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE);
		}
		
		if(if_is_broadcast(pInterface)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_BROADCAST);
		}
		
		if(if_is_pointopoint(pInterface)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT);
		}

		if(if_is_multicast(pInterface)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST);
		}

		if(if_is_loopback(pInterface)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_LOOPBACK);
		}

		pEigrpIntf->mtu	= pInterface->mtu;
#if(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	#if(EIGRP_LINUX_USE_VM==TRUE)
		pEigrpIntf->bandwidth	= 100 * 1000;
	#else
		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
			pEigrpIntf->bandwidth	= 2048;
		}else{
			pEigrpIntf->bandwidth	= ((pInterface->bandwidth)*8)/1000;
		}
	#endif
#else
		pEigrpIntf->bandwidth	= ((pInterface->bandwidth)*8)/1000;
#endif
		pEigrpIntf->metric		= (U32)pInterface->metric;

		/* we copy addr info when adding addr 
		pEigrpIntf->ipAddr	= pConn->address->u.prefix4.s_addr;
		pEigrpIntf->ipMask	= EIGRP_PREFIX_TO_MASK(pConn->address->prefixlen);
		if(if_is_pointopoint(pConn->ifp)){
			pEigrpIntf->remoteIpAddr	= pConn->destination->u.prefix4.s_addr;
		}else{
			pEigrpIntf->remoteIpAddr	= 0;
		}
		*/
		pEigrpIntf->sysCirc	= pSysIntf;
#else// _EIGRP_PLAT_MODULE
		zebraEigrpPortCopyIntfInfo(pEigrpIntf, pSysIntf);		
#endif// _EIGRP_PLAT_MODULE
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		intf_pt	pIntf;

		EIGRP_FUNC_ENTER(EigrpPortCopyIntfInfo);
		pIntf = (intf_pt)pSysIntf;
		
		EigrpPortStrCpy(pEigrpIntf->name, pIntf->pCirc->circName);
		pEigrpIntf->ifindex 	= pIntf->intfId;

		if(PILCircIsAvailable(pIntf->pCirc)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE);
		}
		
		if(BIT_TEST(pIntf->pCirc->circFlag, CIRC_STATUS_BROADCAST)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_BROADCAST);
		}else if(BIT_TEST(pIntf->pCirc->circFlag, CIRC_STATUS_POINT2POINT)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT);
		}
		
		if(BIT_TEST(pIntf->pCirc->circFlag, CIRC_STATUS_MCAST)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST);
		}

		if(BIT_TEST(pIntf->pCirc->circFlag, CIRC_STATUS_LOOPBACK)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_LOOPBACK);
		}

		pEigrpIntf->mtu 		= pIntf->pCirc->circMtu;
		if(pIntf->pCirc->circAdmBw){
			pEigrpIntf->bandwidth	= pIntf->pCirc->circAdmBw / 1000;
		}else{
			pEigrpIntf->bandwidth	= pIntf->pCirc->circSpeed / 1000;
		}
		pEigrpIntf->ipAddr	= ((struct sockaddr_in *)pIntf->laddr)->sin_addr.s_addr;
		pEigrpIntf->ipMask	= ((struct sockaddr_in *)pIntf->netMask)->sin_addr.s_addr;
		if(pEigrpIntf->ipMask == 0){
			pEigrpIntf->ipMask	= EigrpUtilGetNaturalMask(pEigrpIntf->ipMask);
		}

		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
			pEigrpIntf->remoteIpAddr	= ((struct sockaddr_in *)pIntf->dstAddr)->sin_addr.s_addr;
		}
		pEigrpIntf->sysCirc	= pIntf->pCirc;
	}
#endif
	EIGRP_FUNC_LEAVE(EigrpPortCopyIntfInfo);

	return;
}

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#ifndef _EIGRP_PLAT_MODULE
extern struct	ifnet	*ifnet;
#endif //_EIGRP_PLAT_MODULE
#endif

/************************************************************************************

	Name:	EigrpPortGetAllSysIntf

	Desc:	This function is to get infomation of all the physical interface to build the eigrp 
			interface lsit.
		
	Para: 	NONE		
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortGetAllSysIntf()
{
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
#ifndef _EIGRP_PLAT_MODULE	
		struct	ifnet	*pIfNet;
		struct	ifaddr	*pIfAddr;
		EigrpIntf_pt	pEigrpIntf;

		EIGRP_FUNC_ENTER(EigrpPortGetAllSysIntf);
		for(pIfNet = ifnet; pIfNet; pIfNet = pIfNet->if_next){
			for(pIfAddr = pIfNet->if_addrlist; pIfAddr; pIfAddr = pIfAddr->ifa_next){
				if(pIfAddr->ifa_addr->sa_family != AF_INET){
					continue;
				}
				pEigrpIntf	= EigrpPortMemMalloc(sizeof(EigrpIntf_st));
				if(!pEigrpIntf){	 
					EIGRP_FUNC_LEAVE(EigrpPortGetAllSysIntf);
					return;
				}
				
				EigrpPortCopyIntfInfo(pEigrpIntf, pIfAddr);
				BIT_RESET(pEigrpIntf->flags, EIGRP_INTF_FLAG_EIGRPED);
				
				EigrpIntfAdd(pEigrpIntf);
			}
		}
		EIGRP_FUNC_LEAVE(EigrpPortGetAllSysIntf);
#else //_EIGRP_PLAT_MODULE
		zebraEigrpPortGetAllSysIntf();
#endif // _EIGRP_PLAT_MODULE
		return;
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifndef _EIGRP_PLAT_MODULE	
#if 0
		struct interface *ifp;
		struct route_node *rn;
		EigrpIntf_pt	pEigrpIntf;
		struct lib_globals *eg;
		struct listnode *node;
		int ret;

		eg	= (struct lib_globals *)gpEigrp->eigrpZebos;
		for (rn = route_top(((struct lib_globals *)gpEigrp->eigrpZebos)->ifg.if_table); 
				rn; rn = route_next (rn)){
					
			printf("testzebra EigrpPortGetAllSysIntf: 1 \n");
			if(ifp = rn->info){
				printf("testzebra EigrpPortGetAllSysIntf: 2 %s\n", ifp->name);
				
				pEigrpIntf	= EigrpPortMemMalloc(sizeof(EigrpIntf_st));
				
				EigrpPortCopyIntfInfo(pEigrpIntf, ifp);
				BIT_RESET(pEigrpIntf->flags, EIGRP_INTF_FLAG_EIGRPED);
				
				EigrpIntfAdd(pEigrpIntf);
			}
		}
	
		LIST_LOOP (nzg->ifg.if_list, ifp, node){
			
/*			printf("testzebra EigrpPortGetAllSysIntf: 2 %s\n", ifp->name);*/

			pEigrpIntf	= EigrpPortMemMalloc(sizeof(EigrpIntf_st));

			EigrpPortCopyIntfInfo(pEigrpIntf, ifp);
			BIT_RESET(pEigrpIntf->flags, EIGRP_INTF_FLAG_EIGRPED);

			EigrpIntfAdd(pEigrpIntf);
		}
#endif//0
#else //_EIGRP_PLAT_MODULE
		zebraEigrpPortGetAllSysIntf();
#endif // _EIGRP_PLAT_MODULE
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		intf_pt	pIntf;
		EigrpIntf_pt	pEigrpIntf;

		EIGRP_FUNC_ENTER(EigrpPortGetAllSysIntf);
		for(pIntf	 = PILIfMGetNextIntfById(0); pIntf; pIntf = PILIfMGetNextIntfById(pIntf->intfId)){
			pEigrpIntf	= EigrpPortMemMalloc(sizeof(EigrpIntf_st));
			
			EigrpPortCopyIntfInfo(pEigrpIntf, pIntf);
			BIT_RESET(pEigrpIntf->flags, EIGRP_INTF_FLAG_EIGRPED);

			EigrpIntfAdd(pEigrpIntf);
		}
		EIGRP_FUNC_LEAVE(EigrpPortGetAllSysIntf);

		return;
	}
#endif
}

#if 0
/************************************************************************************

	Name:	EigrpPortIntfCoverPeer

	Desc:	This function is to judge if the given ip address is covered by the ip params of the 
			given interface.
		
	Para: 	ipAddr	- ip address
			intf		- pointer to the given interface
	
	Ret:		TRUE	for the given ip address is covered by the given interface.
			FALSE	for it is not so 
************************************************************************************/
// TODO: ZHURISH  ???????????????? ????????
S32	EigrpPortIntfCoverPeer(U32 ipAddr,  struct EigrpIntf_ *intf)
{
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		EIGRP_FUNC_ENTER(EigrpPortIntfCoverPeer);
		if(intf->ipAddr & intf->ipMask == ipAddr & intf->ipMask){
			EIGRP_FUNC_LEAVE(EigrpPortIntfCoverPeer);
			return TRUE;
		}
		
		EIGRP_FUNC_LEAVE(EigrpPortIntfCoverPeer);
		return FALSE;
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifdef EIGRP_PLAT_ZEBOS	
		listnode cnode;
		struct connected *c;
		struct prefix *p;
		U32 cnaddr,cnmasklen,cnmask;

		EIGRP_FUNC_ENTER(EigrpPortIntfCoverPeer);
		EIGRP_ASSERT(intf);
		EIGRP_ASSERT(ipAddr);

		for(cnode = listhead(intf->connected); cnode; nextnode(cnode)){
			c = getdata(cnode);
			p = c->address;
			if(p->family == AF_INET){
				cnaddr		= connected_addr(c);
				cnmasklen	= connected_plen(c);
				cnmask		= EIGRP_PREFIX_TO_MASK(cnmasklen);
				if((ipAddr & cnmask) == (cnaddr & cnmask)){
					EIGRP_FUNC_LEAVE(EigrpPortIntfCoverPeer);
					return TRUE;
				}
			}
		  }
		EIGRP_FUNC_LEAVE(EigrpPortIntfCoverPeer);
		return FALSE;
#endif//EIGRP_PLAT_ZEBOS
#ifdef _EIGRP_PLAT_MODULE	
		return zebraEigrpPortIntfCoverPeer(ipAddr,  intf);
#endif//_EIGRP_PLAT_MODULE
	}
#endif
	return TRUE;

}
#endif
/* ????????*/
U32 EigrpPortGetRouterId()
{
#ifndef _EIGRP_PLAT_MODULE	
	return	Ipv4PortGetRouterId();
#else
	return 0;
#endif	
}
// TODO: ZHURISH ???????????
U8  EigrpPortCircIsUnnumbered(void *pCirc)
{
#ifndef _EIGRP_PLAT_MODULE	
	return	Ipv4PortCircIsUnnumbered(pCirc);
#else
	return 0;
#endif	
}

// TODO: ZHURISH ???????????
U8 *EigrpPortGetCircMac(void *pCirc)
{
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)//???
	circ_pt circ = (circ_pt)pCirc;
	
	if(!circ){
		return	NULL;
	}
	return	circ->circMac;
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	return	NULL;
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	return	NULL;
#endif
}

#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
extern S32 pal_kernel_if_set_dst_addr (S8 *ifName, U32 addr);
extern S32 pal_kernel_if_del_addr (S8 *ifName, U32 addr);
#endif
/* ???????? */
S32 EigrpPortSetIfDstAddr(S8 * ifName, U32 addr)
{
#ifdef EIGRP_PLAT_ZEBRA
//	return pal_kernel_if_set_dst_addr(ifName, addr);
#endif//EIGRP_PLAT_ZEBRA
}
#endif

/* ???????? */
/* TODO: ????????,???????*/
S32 EigrpPortSetUnnumberedIf(void *pCirc, U32 dstIp)
{
	EigrpIntf_pt	pEigrpIf;
	U32	srcIp;
	S32	retVal;

	if(!pCirc){
		return	FAILURE;
	}

	for(pEigrpIf = gpEigrp->intfLst; pEigrpIf; pEigrpIf = pEigrpIf->next){
		if(pEigrpIf->sysCirc == pCirc){
			break;
		}
	}
	if(!pEigrpIf){
		return	FAILURE;
	}

	if(pEigrpIf->remoteIpAddr == dstIp){/*?????????,???????*/
		return	SUCCESS;
	}
	pEigrpIf->remoteIpAddr = dstIp;

	srcIp	= EigrpPortGetRouterId();
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	retVal = Ipv4PortSetUnnumberedIfDst(pCirc, srcIp, dstIp);
	if(retVal != SUCCESS){
		return	FAILURE;
	}
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)		
	retVal = EigrpPortSetIfDstAddr(((struct interface *)pCirc)->name, dstIp);
	if(retVal != 0){
		return	FAILURE;
	}	
#endif
	return	SUCCESS;
}
/* ???????? */
// TODO: ZHURISH ???????????
S32 EigrpPortUnSetUnnumberedIf(void *pCirc)
{
#ifndef _EIGRP_PLAT_MODULE	
	//???	circ_pt	circ = (circ_pt)pCirc;
	EigrpIntf_pt	pEigrpIf;
	U32	srcIp;
	S32	retVal;

	for(pEigrpIf = gpEigrp->intfLst; pEigrpIf; pEigrpIf = pEigrpIf->next){
		if(pEigrpIf->sysCirc == pCirc){
			break;
		}
	}
	if(!pEigrpIf){
		return	FAILURE;
	}
	if(!pEigrpIf->remoteIpAddr){/*????????,???????*/
		return	SUCCESS;
	}
//	pEigrpIf->remoteIpAddr = 0;

	srcIp	= EigrpPortGetRouterId();
	retVal = Ipv4PortUnSetUnnumberedIfDst(pCirc, srcIp);
	if(retVal != SUCCESS){
		return	FAILURE;
	}

	return	SUCCESS;
#else
	EigrpIntf_pt	pEigrpIf;
	U32	srcIp;
	S32	retVal;

	if(!pCirc){
		return	SUCCESS;
	}

	for(pEigrpIf = gpEigrp->intfLst; pEigrpIf; pEigrpIf = pEigrpIf->next){
		if(pEigrpIf->sysCirc == pCirc){
			break;
		}
	}
	if(!pEigrpIf){
		return	FAILURE;
	}
	if(!pEigrpIf->remoteIpAddr){/*????????,???????*/
		return	SUCCESS;
	}
	pEigrpIf->remoteIpAddr = 0;

#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	srcIp	= EigrpPortGetRouterId();
	retVal = Ipv4PortUnSetUnnumberedIfDst(pCirc, srcIp);
	if(retVal != SUCCESS){
		return	FAILURE;
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS	
	pal_kernel_if_del_addr(((struct interface *)pCirc)->name, ((struct interface *)pCirc)->ifc_ipv4->destination->u.prefix4.s_addr);
#endif//EIGRP_PLAT_ZEBOS	
#ifdef EIGRP_PLAT_ZEBRA
#endif// EIGRP_PLAT_ZEBRA
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	return	SUCCESS;	
#endif		
}

/************************************************************************************

	Name:	EigrpPortCoreRtAdd

	Desc:	This function is to add one route to the core ip route table.
		
	Para: 	ipAddr	- the ip address of the new route 
			ipMask	- the ip Mask of the new route
			nexthop	- the next hop ip address of new route
			metric	- the metric of the new route
			distance	- the distance of the new route
		
	Ret:		NONE	
************************************************************************************/

void	EigrpPortCoreRtAdd(U32 ipAddr, U32 ipMask, U32 nexthop, U32 metric, U8 distance, U32 asNum)
{
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)//??????
	{
#ifndef _EIGRP_PLAT_MODULE	
		IP_ROUTE_ENTRY	IpRouteEntry;
		static int registered	= 0;

		EIGRP_FUNC_ENTER(EigrpPortCoreRtAdd);
		if(!registered){
			registered = 1;
			rtm_register_protocol(RTM_IP, RTM_CISCO_IGRP_TYPE, 90, NULL, RTM_PROTOCOL_SINGLE_ROUTE);
		}
		EIGRP_TRC(DEBUG_EIGRP_OTHER, 
					"ipaddr = 0x%x, ipmask = 0x%x, nexthop = 0x%x\n", ipAddr, ipMask, nexthop);
		EigrpPortMemSet((void*)&IpRouteEntry, 0x00, sizeof(IP_ROUTE_ENTRY));

		IpRouteEntry.target	= ipAddr & ipMask;
		IpRouteEntry.mask		= ipMask;
		IpRouteEntry.gateway	= nexthop;
		IpRouteEntry.number_of_subnet_mask_bits = 0;
		IpRouteEntry.metric	= metric;
		IpRouteEntry.ipRouteType	= 78;
		IpRouteEntry.ipRouteProto	= 88;

		if(rtm_add_route(50, (void*)&IpRouteEntry, 0, NULL, NULL, NULL) == 0){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EigrpPortCoreRtAdd:rtm_add_route error.\n");
			EIGRP_FUNC_LEAVE(EigrpPortCoreRtAdd);
			return;
		}
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtAdd);
		return;
#else// _EIGRP_PLAT_MODULE ??????
		EIGRP_FUNC_ENTER(EigrpPortCoreRtAdd);
		ZebraEigrpRt_st IpRouteEntry;
		IpRouteEntry.target	= ipAddr;
		IpRouteEntry.mask		= ipMask;
		IpRouteEntry.gateway	= nexthop;
		IpRouteEntry.metric	= metric;
		IpRouteEntry.proto		= EIGRP_ROUTE_EIGRP;
		IpRouteEntry.ifindex	= 0;
		if(Eigrp_Route_handle)
			(* Eigrp_Route_handle)(RTM_ADD, &IpRouteEntry);
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtAdd);
		return;
#endif// _EIGRP_PLAT_MODULE 
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifdef EIGRP_PLAT_ZEBOS	
		struct nsm_msg_route_ipv4 *msg;
		EigrpIntf_pt	pIntf;
		int	len;

		EIGRP_FUNC_ENTER(EigrpPortCoreRtAdd);

		pIntf	= EigrpIntfFindByPeer(nexthop);
		if(!pIntf){
			return;
		}
		
		/* IPv4 route addition.  */
		len	= sizeof(struct nsm_msg_route_ipv4);	
		msg = (struct nsm_msg_route_ipv4 *)EigrpPortMemMalloc(len);
		EigrpPortMemSet((U8 *)msg, 0, len);
		BIT_SET(msg->flags, NSM_MSG_ROUTE_FLAG_ADD);
		msg->type	 = IPI_ROUTE_FRP;
		msg->distance = EIGRP_DEF_DISTANCE_INTERNAL;
		msg->metric	 = metric;
		msg->prefixlen = EigrpUtilMask2Len(ipMask);
		msg->prefix.s_addr	= HTONL(ipAddr);

		/* Set nexthop address.  */
		NSM_SET_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV4_NEXTHOP);
		msg->nexthop_num = 1;
		msg->nexthop[0].ifindex = pIntf->ifindex;
		msg->nexthop[0].addr.s_addr = HTONL(nexthop);

		/* Send message.	*/
		nsm_client_send_route_ipv4 (0, 0, ((struct lib_globals *)gpEigrp->eigrpZebos)->nc, msg);
		EigrpPortMemFree(msg);
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtAdd);
#endif//EIGRP_PLAT_ZEBOS
	//#ifdef EIGRP_PLAT_ZEBRA
#ifdef  _EIGRP_PLAT_MODULE// ??????

		ZebraEigrpRt_st IpRouteEntry;
		EIGRP_FUNC_ENTER(EigrpPortCoreRtAdd);
		IpRouteEntry.target	= ipAddr;
		IpRouteEntry.mask		= ipMask;
		IpRouteEntry.gateway	= nexthop;
		IpRouteEntry.metric	= metric;
		IpRouteEntry.proto		= EIGRP_ROUTE_EIGRP;
		IpRouteEntry.ifindex	= 0;
		if(Eigrp_Route_handle)
			(* Eigrp_Route_handle)(RTM_ADD, &IpRouteEntry);
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtAdd);
		return;
#endif// _EIGRP_PLAT_MODULE 
	//#endif//EIGRP_PLAT_ZEBRA
		return;
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		EIGRP_FUNC_ENTER(EigrpPortCoreRtAdd);
		RouteCoreAdd(ipAddr, ipMask, nexthop, metric, distance, ROUTE_TYPE_EIGRP, asNum);
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtAdd);
		
		return;
	}
#endif
}

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
extern	RtGlobal_st gstRt;
#endif

/************************************************************************************

	Name:	EigrpPortCoreRtNodeFindExact

	Desc:	This function is find a exact route from the core routing table.
		
	Para: 	ipAddr	- ip address of the exact route 
			ipMask	- ip mask of the exact route
	
	Ret:		pointer to the exact route needed, if nothing is find or failed to create a
			appropriate route, return NULL
************************************************************************************/
// TODO: ZHURISH ????????????
void	*EigrpPortCoreRtNodeFindExact(U32 ipAddr, U32 ipMask)
{
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
#ifndef _EIGRP_PLAT_MODULE	
		struct sockaddr pDestTemp;		/* IP address reachable with matching route */
		struct sockaddr_in *pDest;
		int	status;					/* route source from m2Lib.h, or 0 for any. */
	
		ROUTE_DESC * pRouteDesc, routeDescTemp; 	/* information for matching route */

		struct sockaddr_in DstAddr;	
		struct sockaddr_in NetMask;	
		struct sockaddr_in Gateway;

		EigrpRtNode_pt	pEigrpRtNode;
		
		EIGRP_FUNC_ENTER(EigrpPortCoreRtNodeFindExact);
		if(!EigrpIntfFindByNetAndMask(ipAddr, ipMask)){
		
			pRouteDesc	= &routeDescTemp;
			pDest = (struct sockaddr_in *)&pDestTemp;
			EigrpPortMemSet((void*)pRouteDesc, 0, sizeof(ROUTE_DESC));
		
			pDest = (struct sockaddr_in *)&pDestTemp;
		
			EigrpPortMemSet((void*)&DstAddr, 0, sizeof(struct sockaddr_in));
			EigrpPortMemSet((void*)&NetMask, 0, sizeof(struct sockaddr_in));	
			EigrpPortMemSet((void*)&Gateway, 0, sizeof(struct sockaddr_in));	

			pRouteDesc->pDstAddr		= (struct sockaddr *)&DstAddr;
			pRouteDesc->pNetmask	= (struct sockaddr *)&NetMask;
			pRouteDesc->pGateway	= (struct sockaddr *)&Gateway;

			EigrpPortMemSet((void*)pDest, 0, sizeof(struct sockaddr_in));
			pDest->sin_family = AF_INET;
			pDest->sin_len = sizeof(struct sockaddr_in);
			pDest->sin_addr.s_addr = HTONL(ipAddr);

			status = routeEntryLookup((struct sockaddr *)pDest, NULL, 0, pRouteDesc);
			if(status){
				EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeFindExact);
				return NULL;
			}

			if(NetMask.sin_addr.s_addr != HTONL(ipMask)){
				EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeFindExact);
				return NULL;
			}
		}

		pEigrpRtNode	= EigrpPortMemMalloc(sizeof(EigrpRtNode_st));
		if(!pEigrpRtNode){
			EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeFindExact);
			return NULL;
		}
		pEigrpRtNode->dest	= ipAddr & ipMask;
		pEigrpRtNode->mask	= ipMask;
		pEigrpRtNode->extData	= NULL;
		pEigrpRtNode->rtEntry	= NULL;
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeFindExact);
		
		return (void *)pEigrpRtNode;
		
#else// _EIGRP_PLAT_MODULE

		int status = 1;
		ZebraEigrpRt_st IpRouteEntry;
		EigrpRtNode_pt	pEigrpRtNode;
		
		EIGRP_FUNC_ENTER(EigrpPortCoreRtNodeFindExact);
		if(!EigrpIntfFindByNetAndMask(ipAddr, ipMask)){
		
			IpRouteEntry.target	= ipAddr;
			IpRouteEntry.mask		= ipMask;
			IpRouteEntry.gateway	= 0;
			IpRouteEntry.metric	= 0;
			IpRouteEntry.proto		= 0;
			IpRouteEntry.ifindex	= 0;
			
			if(Eigrp_Route_handle)
				status = (* Eigrp_Route_handle)(RTM_GET, &IpRouteEntry);
			if(status){
				EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeFindExact);
				return NULL;
			}
	
		}
		pEigrpRtNode	= EigrpPortMemMalloc(sizeof(EigrpRtNode_st));
		if(!pEigrpRtNode){
			EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeFindExact);
			return NULL;
		}
		pEigrpRtNode->dest	= ipAddr & ipMask;
		pEigrpRtNode->mask	= ipMask;
		pEigrpRtNode->extData	= NULL;
		pEigrpRtNode->rtEntry	= NULL;
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeFindExact);
		
		return (void *)pEigrpRtNode;
		
#endif// _EIGRP_PLAT_MODULE
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifdef EIGRP_PLAT_ZEBOS		
	
		struct nsm_master *nm;
		struct nsm_vrf *vrf;
		struct rib *rib;
		struct nsm_route_node *rn;
		U32	rnPrefix;
		struct nsm_msg_route_lookup_ipv4	*pMsg;
		struct nsm_msg_route_ipv4	*pRoute;
		int	ret;

		pMsg	= (struct nsm_msg_route_lookup_ipv4	*)EigrpPortMemMalloc(sizeof(struct nsm_msg_route_lookup_ipv4));
		if(!pMsg){
			return NULL;
		}
		EigrpPortMemSet((U8 *)pMsg, 0, sizeof(struct nsm_msg_route_lookup_ipv4));

		pRoute	= (struct nsm_msg_route_ipv4 *)EigrpPortMemMalloc(sizeof(struct nsm_msg_route_ipv4));
		if(!pRoute){
			EigrpPortMemFree(pMsg);
			return NULL;
		}
		EigrpPortMemSet((char *)pRoute, 0, sizeof(struct nsm_msg_route_ipv4));

		pMsg->addr.s_addr = HTONL(ipAddr);
		NSM_SET_CTYPE(pMsg->cindex, NSM_ROUTE_LOSUCCESSUP_CTYPE_PREFIXLEN);
		pMsg->prefixlen	= EigrpUtilMask2Len(ipMask);
		ret = nsm_client_route_lookup_ipv4(0, 0, (((struct lib_globals *)(gpEigrp->eigrpZebos))->nc), pMsg, EXACT_MATCH_LOSUCCESSUP, pRoute);
		if (ret<0){
			EigrpPortMemFree(pRoute);
			EigrpPortMemFree(pMsg);
			return NULL;
		}

		rib = EigrpPortMemMalloc(sizeof(struct rib));
		rn = EigrpPortMemMalloc(sizeof(struct nsm_route_node));

		rn->info = rib;

		rib->next=NULL;
		rib->prev=NULL;
		rib->nexthop=NULL;
		rib->vrf=NULL;

		rib->type = pRoute->type;	/*u_char*/
		(rn->p.u.prefix4).s_addr= pRoute->prefix.s_addr;

		rn->p.prefixlen = pRoute->prefixlen;

		EigrpPortMemFree(pRoute);
		EigrpPortMemFree(pMsg);

		return (void *)rn;
#endif//EIGRP_PLAT_ZEBOS
#ifdef _EIGRP_PLAT_MODULE

		int status = 1;
		ZebraEigrpRt_st IpRouteEntry;
		EigrpRtNode_pt	pEigrpRtNode;
		
		EIGRP_FUNC_ENTER(EigrpPortCoreRtNodeFindExact);
		if(!EigrpIntfFindByNetAndMask(ipAddr, ipMask)){
		
			IpRouteEntry.target	= ipAddr;
			IpRouteEntry.mask		= ipMask;
			IpRouteEntry.gateway	= 0;
			IpRouteEntry.metric	= 0;
			IpRouteEntry.proto		= 0;
			IpRouteEntry.ifindex	= 0;
			
			//if(Eigrp_Route_handle)
			//	status = (* Eigrp_Route_handle)(RTM_GET, &IpRouteEntry);
			if(status){
				EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeFindExact);
				return NULL;
			}
	
		}
		pEigrpRtNode	= EigrpPortMemMalloc(sizeof(EigrpRtNode_st));
		if(!pEigrpRtNode){
			EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeFindExact);
			return NULL;
		}
		pEigrpRtNode->dest	= ipAddr & ipMask;
		pEigrpRtNode->mask	= ipMask;
		pEigrpRtNode->extData	= NULL;
		pEigrpRtNode->rtEntry	= NULL;
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeFindExact);
		return NULL;
		return (void *)pEigrpRtNode;
#endif//_EIGRP_PLAT_MODULE		
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		rtNode_pt	pRtNode;

		EIGRP_FUNC_ENTER(EigrpPortCoreRtNodeFindExact);
		pRtNode	= RouteNodeFindExact(gstRt.pRtTree, ipAddr & ipMask, ipMask);
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeFindExact);
		
		return (void *)pRtNode;
	}
#endif
}

/************************************************************************************

	Name:	EigrpPortCoreRtNodeHasConnect

	Desc:	This function is to judge if the given route node has the connect route.
		
	Para: 	pNode	- pointer to the given route node
	
	Ret:		TRUE	for the given route node has connect route
			FALSE	it is not so 
************************************************************************************/
// TODO: ZHURISH ?????????????
//????????????????????????
S32	EigrpPortCoreRtNodeHasConnect(void *pNode)
{
#if	 (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		return TRUE;
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifdef EIGRP_PLAT_ZEBOS	
		struct rib *rib;
		struct nsm_route_node *rn;

		rn	= (struct nsm_route_node *)pNode;
		for (rib = rn->info; rib; rib = rib->next){
			if(rib->type == IPI_ROUTE_CONNECT){
				EigrpPortZebraUnlockRtNode((void *)rn);
				return TRUE;
			}
		}
		
		EigrpPortZebraUnlockRtNode((void *)rn);
		return FALSE;
#endif//EIGRP_PLAT_ZEBOS
#ifdef _EIGRP_PLAT_MODULE
		return TRUE;
#endif//_EIGRP_PLAT_MODULE	
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		rtNode_pt	pRtNode;
		rtEntryData_pt	pRt;

		EIGRP_FUNC_ENTER(EigrpPortCoreRtNodeHasConnect);
		pRtNode	= (rtNode_pt)pNode;

		for(pRt = pRtNode->rtEntry; pRt; pRt = pRt->next){
			if(pRt->type == ROUTE_TYPE_CONNECT){
				EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeHasConnect);
				return TRUE;
			}
		}
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeHasConnect);
		return FALSE;
	}
#endif
}

/************************************************************************************

	Name:	EigrpPortCoreRtNodeHasStatic

	Desc:	This function is to judge if the given route node has the Static route.
		
	Para: 	pNode	- pointer to the given route node
	
	Ret:		TRUE	for the given route node has connect route
			FALSE	it is not so 
************************************************************************************/
// TODO: ZHURISH ?????????????,
// ??????????????,????EigrpPortRedisProcNetworkAddStatic??
S32	EigrpPortCoreRtNodeHasStatic(void *pNode)
{
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		return TRUE;//zhurish FALSE
	}//zhurish 2016-1-9
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	/* zebra doesn`t come here. */
	return FALSE;
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		rtNode_pt	pRtNode;
		rtEntryData_pt	pRt;

		EIGRP_FUNC_ENTER(EigrpPortCoreRtNodeHasStatic);
		pRtNode	= (rtNode_pt)pNode;

		for(pRt = pRtNode->rtEntry; pRt; pRt = pRt->next){
			if(pRt->type == ROUTE_TYPE_STATIC){
				EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeHasStatic);
				return TRUE;
			}
		}
		
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtNodeHasStatic);
		return FALSE;
		
	}
#endif
}

/************************************************************************************

	Name:	EigrpPortCoreGetStaticRtGateway

	Desc:	This function is to get the gateway of the static route.
		
	Para: 	pNode	- pointer to the given route node
	
	Ret:		TRUE	for the given route node has connect route
			FALSE	it is not so 
************************************************************************************/

/************************************************************************************

	Name:	EigrpPortCoreRtGetEigrpExt

	Desc:	This function is to get the pointer to the eigrp extended data of the given route
			node.
		
	Para: 	pNode	- pointer to the given route node
	
	Ret:		pointer to the eigrp extened data of the given route node, if it is not exist,
			return NULL
************************************************************************************/
// TODO: ZHURISH ???????????
void	*EigrpPortCoreRtGetEigrpExt(void *pNode)
{
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		return NULL;
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
		return NULL;
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		rtNode_pt		pRtNode;
		rtEntryData_pt	pRt;

		EIGRP_FUNC_ENTER(EigrpPortCoreRtGetEigrpExt);
		pRtNode = (rtNode_pt)pNode;
		for(pRt	= pRtNode->rtEntry; pRt; pRt = pRt->next){
			if(pRt->type == ROUTE_TYPE_EIGRP){
				EIGRP_FUNC_LEAVE(EigrpPortCoreRtGetEigrpExt);
				return pRt->extData;
			}
		}
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtGetEigrpExt);
		return NULL;
	}
#endif
}

/************************************************************************************

	Name:	EigrpPortCoreRtGetDest

	Desc:	This function is get the destination ip  address of the given route node.
		
	Para: 	pNode	- pointer to the given route node
	
	Ret:		destination ip address of the given route node
************************************************************************************/
// TODO: ZHURISH ??????????
U32	EigrpPortCoreRtGetDest(void *pNode)
{
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{//zhurish
		EigrpRtNode_pt	pEigrpRtNode;

		pEigrpRtNode	= (EigrpRtNode_pt)pNode;
		return pEigrpRtNode->dest;
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
		EigrpRtNode_pt	pEigrpRtNode;

		pEigrpRtNode	= (EigrpRtNode_pt)pNode;
		return pEigrpRtNode->dest;
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		rtNode_pt	pRtNode;
	
		EIGRP_FUNC_ENTER(EigrpPortCoreRtGetDest);
		pRtNode = (rtNode_pt)pNode;
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtGetDest);

		return pRtNode->dest;
	}
#endif
}

/************************************************************************************

	Name:	EigrpPortCoreRtGetMask

	Desc:	This function is to get the ip mask of the given route node.
		
	Para: 	pNode	- pointer to the given route node
	
	Ret:		ip mask of the given route node
************************************************************************************/
// TODO:ZHURISH  ????????????
U32	EigrpPortCoreRtGetMask(void *pNode)
{
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{//zhurish
		EigrpRtNode_pt	pEigrpRtNode;

		pEigrpRtNode	= (EigrpRtNode_pt)pNode;
		return pEigrpRtNode->mask;
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
		EigrpRtNode_pt	pEigrpRtNode;

		pEigrpRtNode	= (EigrpRtNode_pt)pNode;
		return pEigrpRtNode->mask;
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		rtNode_pt	pRtNode;
	
		EIGRP_FUNC_ENTER(EigrpPortCoreRtGetMask);
		pRtNode = (rtNode_pt)pNode;
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtGetMask);

		return pRtNode->mask;
	}
#endif
}

/************************************************************************************

	Name:	EigrpPortCoreRtGetConnectIntfId

	Desc:	This function is to get the id of the interface which is connected to the given route.
		
	Para: 	pNode	- pointer to the given route node
	
	Ret:		the id of the interface which is connected to the given route
************************************************************************************/
// TODO: ZHURISH ????????ID
U32	EigrpPortCoreRtGetConnectIntfId(void *pNode)
{
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		EigrpRtNode_pt	pEigrpRtNode;
		EigrpIntf_pt	pTem;

		U32	ifId;

		EIGRP_FUNC_ENTER(EigrpPortCoreRtGetConnectIntfId);
		pEigrpRtNode	= (EigrpRtNode_pt)pNode;

		
		pTem	= EigrpIntfFindByNetAndMask(pEigrpRtNode->dest, pEigrpRtNode->mask);

		if(pTem){
			EIGRP_FUNC_LEAVE(EigrpPortCoreRtGetConnectIntfId);
			return pTem->ifindex;
		}
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtGetConnectIntfId);
	
		return 0;
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifdef EIGRP_PLAT_ZEBOS
		EigrpIntf_pt	pTem;
		struct nsm_route_node *rn;	
		U32 ifId;
		U32	ipAddr;
		U32	ipMask;
	
		EIGRP_FUNC_ENTER(EigrpPortCoreRtGetConnectIntfId);
		rn	= (struct nsm_route_node *)pNode;
		
		ipAddr	= NTOHL(rn->p.u.prefix4.s_addr);	/* jiangxj add 20071010 */
		ipMask	= EigrpUtilLen2Mask(rn->p.prefixlen);
		pTem	= EigrpIntfFindByNetAndMask(ipAddr, ipMask);
	
		if(pTem){
			EIGRP_FUNC_LEAVE(EigrpPortCoreRtGetConnectIntfId);
			return pTem->ifindex;
		}
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtGetConnectIntfId);
		
		return 0;
#endif//EIGRP_PLAT_ZEBOS
#ifdef EIGRP_PLAT_ZEBRA
	{
		EigrpRtNode_pt	pEigrpRtNode;
		EigrpIntf_pt	pTem;

		U32	ifId;

		EIGRP_FUNC_ENTER(EigrpPortCoreRtGetConnectIntfId);
		pEigrpRtNode	= (EigrpRtNode_pt)pNode;

		
		pTem	= EigrpIntfFindByNetAndMask(pEigrpRtNode->dest, pEigrpRtNode->mask);

		if(pTem){
			EIGRP_FUNC_LEAVE(EigrpPortCoreRtGetConnectIntfId);
			return pTem->ifindex;
		}
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtGetConnectIntfId);
	
		return 0;
	}
#endif//EIGRP_PLAT_ZEBRA
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		rtNode_pt		pRtNode;
		rtEntryData_pt	pRt;

		EIGRP_FUNC_ENTER(EigrpPortCoreRtGetConnectIntfId);
		pRtNode = (rtNode_pt)pNode;
		for(pRt	= pRtNode->rtEntry; pRt; pRt = pRt->next){
			if(pRt->type == ROUTE_TYPE_CONNECT){
				EIGRP_FUNC_LEAVE(EigrpPortCoreRtGetConnectIntfId);
				return pRt->intfId;
			}
		}
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtGetConnectIntfId);
		
		return 0;
	}
#endif
}

/************************************************************************************

	Name:	EigrpPortCoreRtDel

	Desc:	This function is to delete one eigrp route from the core route table.
		
	Para: 	ipAddr	- ip address of the deleting route 
			ipMask	- ip mask of the deleting route
			nexthop	- next hop ip address of the deleting route
			metric	- the metric of the deleting route
	
	Ret:		NONE
************************************************************************************/
// TODO: ?????
void	EigrpPortCoreRtDel(U32 ipAddr, U32 ipMask,  U32 nexthop, U32 metric)
{
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
#ifndef _EIGRP_PLAT_MODULE	
		IP_ROUTE_ENTRY	IpRouteEntry;

		EIGRP_FUNC_ENTER(EigrpPortCoreRtDel);
		
		EigrpPortMemSet((void*)&IpRouteEntry, 0x00, sizeof(IP_ROUTE_ENTRY));

		IpRouteEntry.target	= (ipAddr);
		IpRouteEntry.mask		= (ipMask);
		IpRouteEntry.gateway	= (nexthop);
		IpRouteEntry.metric	= metric;
		IpRouteEntry.ipRouteType	= 78;
		IpRouteEntry.ipRouteProto	= 88;

		if(rtm_delete_route(50, (void*)&IpRouteEntry, 0, NULL) == 0){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, 
						"EigrpPortCoreRtDel:rtm_delete_route error!,des=%x,mask=%x,gate=%x.\n", 
						ipAddr, ipMask, nexthop);
			EIGRP_FUNC_LEAVE(EigrpPortCoreRtDel);
			return;
		}
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtDel);
		return;
#else// _EIGRP_PLAT_MODULE ??????
		ZebraEigrpRt_st IpRouteEntry;
		EIGRP_FUNC_ENTER(EigrpPortCoreRtDel);
		IpRouteEntry.target	= ipAddr;
		IpRouteEntry.mask		= ipMask;
		IpRouteEntry.gateway	= nexthop;
		IpRouteEntry.metric	= metric;
		IpRouteEntry.proto		= 78;
		IpRouteEntry.ifindex	= 0;
		if(Eigrp_Route_handle)
			(* Eigrp_Route_handle)(RTM_DELETE, &IpRouteEntry);
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtDel);
		return;
#endif// _EIGRP_PLAT_MODULE 
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifdef EIGRP_PLAT_ZEBOS
		struct nsm_msg_route_ipv4 *pMsg;
		EigrpIntf_pt	pIntf;
		
		EIGRP_FUNC_ENTER(EigrpPortCoreRtDel);
		pIntf	= EigrpIntfFindByPeer(nexthop);
		if(!pIntf){
			return;
		}

		/* IPv4 route addition.  */
		pMsg	= (struct nsm_msg_route_ipv4 *)EigrpPortMemMalloc(sizeof(struct nsm_msg_route_ipv4));
		if(!pMsg){
			return;
		}
		EigrpPortMemSet((U8 *)pMsg, 0, sizeof(struct nsm_msg_route_ipv4));
		BIT_RESET(pMsg->flags, NSM_MSG_ROUTE_FLAG_ADD);
		pMsg->type = IPI_ROUTE_FRP;
		pMsg->distance = EIGRP_DEF_DISTANCE_INTERNAL;
		pMsg->metric = metric;
		pMsg->prefixlen = EigrpUtilMask2Len(ipMask);
		pMsg->prefix.s_addr	= HTONL(ipAddr);

		/* Set nexthop address.  */
		NSM_SET_CTYPE (pMsg->cindex, NSM_ROUTE_CTYPE_IPV4_NEXTHOP);
		pMsg->nexthop_num = 1;
		pMsg->nexthop[0].ifindex = pIntf->ifindex;
		pMsg->nexthop[0].addr.s_addr = HTONL(nexthop);

		/* Send message.	*/
		nsm_client_send_route_ipv4 (0, 0, ((struct lib_globals *)gpEigrp->eigrpZebos)->nc, pMsg);
		EigrpPortMemFree(pMsg);

		EIGRP_FUNC_LEAVE(EigrpPortCoreRtDel);
		return;	
#endif//EIGRP_PLAT_ZEBOS
#ifdef _EIGRP_PLAT_MODULE	
		ZebraEigrpRt_st IpRouteEntry;
		EIGRP_FUNC_ENTER(EigrpPortCoreRtDel);
		IpRouteEntry.target	= ipAddr;
		IpRouteEntry.mask		= ipMask;
		IpRouteEntry.gateway	= nexthop;
		IpRouteEntry.metric	= metric;
		IpRouteEntry.proto		= 78;
		IpRouteEntry.ifindex	= 0;
		if(Eigrp_Route_handle)
			(* Eigrp_Route_handle)(RTM_DELETE, &IpRouteEntry);
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtDel);
		return;
#endif// _EIGRP_PLAT_MODULE 
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		EIGRP_FUNC_ENTER(EigrpPortCoreRtDel);
		
		RouteCoreDel(ipAddr, ipMask, nexthop, ROUTE_TYPE_EIGRP);
		EIGRP_FUNC_LEAVE(EigrpPortCoreRtDel);
		
		return;
	}
#endif
}


S32	EigrpPortRedisPriorityCmp(U32 rtType1, U32 rtType2)
{
	if(rtType1 == rtType2){
		return 0;
	}

	if(rtType1 < rtType2){
		return 1;
	}

	return -1;
}

/************************************************************************************

	Name:	EigrpPortRedisAddRoute

	Desc:	This function is to redistribute one route into eigrp route table.
		
	Para: 	ipAddr	- ip address of the route to be redistributed
			ipMask	- ip mask of the route to be redistributed
			ipNextHop	- next hop ip address of the route to be redistributed 
			type		- redised route type
	
	Ret:		
************************************************************************************/

S32	EigrpPortRedisAddRoute(EigrpPdb_st *pdb, U32 ipAddr, U32 ipMask, U32 ipNextHop, U32 type, U32 srcProc, U32 metric)
{
//ZHURISH
//#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
//#elif ((EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER) || (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL))
	
	EigrpDualNewRt_st	newroute;
#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
	EigrpDualNewRt_st	routeDel;
	EigrpDualNdb_st	*dndb;
#endif
	EigrpDualRdb_st	*drdb;
	S32 event;

	EIGRP_FUNC_ENTER(EigrpPortRedisAddRoute);
	EigrpUtilMemZero((void *) & newroute, sizeof(EigrpDualNewRt_st));

	newroute.nextHop 		= ipNextHop;
	newroute.dest.address 	= ipAddr;
	newroute.dest.mask 	= ipMask;
#if 0
	extData	= (struct EigrpExtData_ *)EigrpPortMemMalloc(sizeof(EigrpExtData_st));
	extData.asystem	= srcProc;
#endif

	_EIGRP_DEBUG("%s:get  %x %x %x \n",__func__, ipAddr,  ipMask,  ipNextHop);

#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
	newroute.origin 		= EIGRP_ORG_REDISTRIBUTED;
#elif	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
	if(type == EIGRP_ROUTE_STATIC){
		newroute.origin 		= EIGRP_ORG_RSTATIC;
	}else{
		newroute.origin 		= EIGRP_ORG_REDISTRIBUTED;
	}
#endif
	newroute.flagExt	= TRUE;
	newroute.rtType	= type;
	newroute.asystem	= srcProc;
	if(newroute.rtType == EIGRP_ROUTE_EIGRP){
		newroute.metric	= metric;
	}
#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
	drdb = EigrpDualRouteLookup(pdb->ddb, &(newroute.dest), NULL, &(newroute.nextHop));
#elif	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
	drdb		= NULL;
	dndb	 = EigrpDualNdbLookup(pdb->ddb, &(newroute.dest));
	if(dndb){
		drdb		= dndb->rdb;
	}
#endif
	if(drdb){
#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
		if(drdb->origin != EIGRP_ORG_REDISTRIBUTED){
			EIGRP_FUNC_LEAVE(EigrpPortRedisAddRoute);
			_EIGRP_DEBUG("drdb->origin != EIGRP_ORG_REDISTRIBUTED \n");
			return -1;
		}
#elif	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
		if(drdb->origin == EIGRP_ORG_CONNECTED){
			EIGRP_FUNC_LEAVE(EigrpPortRedisAddRoute);
			
			return -1;
		}
#endif


#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
		event	= EIGRP_ROUTE_MODIF;
#elif	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
		if(drdb->origin == EIGRP_ORG_REDISTRIBUTED){
			if(EigrpPortRedisPriorityCmp(drdb->rtType, type) >= 0){
				EIGRP_FUNC_LEAVE(EigrpPortRedisAddRoute);
				return -1;
			}else{
				routeDel.nextHop 		= ipNextHop;
				routeDel.dest.address 	= ipAddr;
				routeDel.dest.mask 	= ipMask;
				routeDel.origin 		= EIGRP_ORG_REDISTRIBUTED;
				routeDel.rtType		= drdb->rtType;
				EigrpIpRtchange(pdb, &routeDel, EIGRP_ROUTE_DOWN);
				event	= EIGRP_ROUTE_MODIF;
			}
		}
		if(drdb->origin == EIGRP_ORG_RSTATIC){
			EIGRP_FUNC_LEAVE(EigrpPortRedisAddRoute);
			return -1;
		}

		event	= EIGRP_ROUTE_UP;
#endif
	}else{
		event	= EIGRP_ROUTE_UP;
	}
	EigrpIpRtchange(pdb, &newroute, event);
//#endif ZHURISH

	EIGRP_FUNC_LEAVE(EigrpPortRedisAddRoute);

	return 0;
}

/************************************************************************************

	Name:	EigrpPortRedisDelRoute

	Desc:	This function is delete one redistributed route from eigrp route table.
		
	Para: 	ipAddr	- ip address of the deleting route
			ipMask	- ip mask of the deleting route
			ipNextHop	- next hop ip address of deleting route
			type		- deleting route type
	
	Ret:		
************************************************************************************/

S32	EigrpPortRedisDelRoute(EigrpPdb_st *pdb, U32 ipAddr, U32 ipMask, U32 ipNextHop, U32 type)
{
//ZHURISH
//#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
//#elif	((EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER) || (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL))
	{
		/* note we have multi-process EIGRP */
		EigrpDualNewRt_st newroute;
#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
		EigrpDualNdb_st	*dndb;
#endif
		EigrpDualRdb_st *drdb;

		EIGRP_FUNC_ENTER(EigrpPortRedisDelRoute);
		EigrpUtilMemZero((void *) & newroute, sizeof(EigrpDualNewRt_st));

		newroute.nextHop 		= ipNextHop;
		newroute.dest.address 	= ipAddr;
		newroute.dest.mask 	= ipMask;
#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
		newroute.origin 		= EIGRP_ORG_REDISTRIBUTED;
#elif	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
		if(type == EIGRP_ROUTE_STATIC){
			newroute.origin 		= EIGRP_ORG_RSTATIC;
		}else{
			newroute.origin 		= EIGRP_ORG_REDISTRIBUTED;
		}
#endif
		newroute.flagExt 		= TRUE;
		newroute.rtType 		= type;
#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
	drdb = EigrpDualRouteLookup(pdb->ddb, &(newroute.dest), NULL, &(newroute.nextHop));
#elif	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
	drdb		= NULL;
	dndb	 = EigrpDualNdbLookup(pdb->ddb, &(newroute.dest));
	if(dndb){
		drdb		= dndb->rdb;
	}
#endif

#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
		if(! drdb || (drdb->origin != EIGRP_ORG_REDISTRIBUTED)){
				EIGRP_FUNC_LEAVE(EigrpPortRedisDelRoute);
				return -1;
		}  
#elif	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
		if(! drdb || (drdb->origin != EIGRP_ORG_REDISTRIBUTED 
			&& drdb->origin != EIGRP_ORG_RSTATIC)){
				EIGRP_FUNC_LEAVE(EigrpPortRedisDelRoute);
				return -1;
		}  
#endif

		EigrpIpRtchange(pdb, &newroute, EIGRP_ROUTE_DOWN);

	}
//#endif ZHURISH
	EIGRP_FUNC_LEAVE(EigrpPortRedisDelRoute);

	return 0;
}
//?????,???????????
void	EigrpPortRedisProcNetworkAddStatic(void *rtNode, U32 sense)
{
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)||(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
//#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER) ZHURISH
	{
#ifdef EIGRP_PLAT_ZEBOS
		struct rib *rib;
		struct nsm_route_node *rn;
		S32	hasStatic;
		U32	dest, mask, nexthop;

		rn	= (struct nsm_route_node *)rtNode;
		hasStatic	= FALSE;
		for (rib = rn->info; rib; rib = rib->next){
			if(rib->type == IPI_ROUTE_STATIC){
				hasStatic	= TRUE;
				dest	= rn->p.u.prefix4.s_addr;
				mask	= EigrpUtilLen2Mask(rn->p.prefixlen);
				nexthop	= rib->nexthop[0].gate.ipv4.s_addr;
				break;
			}
		}
		
		EigrpPortZebraUnlockRtNode((void *)rn);

		if(hasStatic == TRUE){
			if(sense == TRUE){
				EigrpPortRouteProcEvent(IPI_ROUTE_STATIC, 0, dest, mask, nexthop, 51200, EIGRP_REDIS_RT_UP);
			}else{
				EigrpPortRouteProcEvent(IPI_ROUTE_STATIC, 0, dest, mask, nexthop, 51200, EIGRP_REDIS_RT_DOWN);
			}
		}
		return;		
#endif //EIGRP_PLAT_ZEBOS
	}

#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	rtNode_pt	pNode;
	rtEntryData_pt	pRtEntry;

	pNode	= (rtNode_pt)rtNode;
	
	for(pRtEntry = pNode->rtEntry; pRtEntry; pRtEntry = pRtEntry->next){
		if(pRtEntry->type == ROUTE_TYPE_STATIC){
			if(sense == TRUE){
				EigrpPortRouteChange((void *)RT_CHANGE_TYPE_ADD, (void *)pRtEntry);
			}else{
				EigrpPortRouteChange((void *)RT_CHANGE_TYPE_DEL, (void *)pRtEntry);
			}
		}
	}
#endif
	return;
}

/************************************************************************************

	Name:	EigrpPortRedisTrigger

	Desc:	This function is to give a signal to the core route management module to make a new
			redistribution to eigrp.
			
	Para: 	proto	- the type of the redistributing route
	
	Ret:		NONE
************************************************************************************/
/* ????????????? */
void	EigrpPortRedisTrigger(U32 proto)
{
//#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
//#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
	struct nsm_msg_redistribute msg;
	struct lib_globals *pEg;
	U32	pro;
	EIGRP_FUNC_ENTER(EigrpPortRedisTrigger);

	switch(proto){
		case	EIGRP_ROUTE_CONNECT:
			pro	= IPI_ROUTE_CONNECT;
			break;
			
		case	EIGRP_ROUTE_STATIC:
			pro	= IPI_ROUTE_STATIC;
			break;
			
		case	EIGRP_ROUTE_RIP:
			pro	= IPI_ROUTE_RIP;
			break;
			
		case	EIGRP_ROUTE_OSPF:
			pro	= IPI_ROUTE_OSPF;
			break;	
	
		case	EIGRP_ROUTE_EIGRP:
			pro	= IPI_ROUTE_FRP;
			break;
			
		default:
			break;
	}
	
	EigrpPortMemSet((U8 *)&msg, 0, sizeof(struct nsm_msg_redistribute));
	msg.type = pro;
	msg.afi = AFI_IP;

	pEg	= (struct lib_globals *)gpEigrp->eigrpZebos;
	nsm_client_send_redistribute_set(0, 0, pEg->nc, &msg);

/*	nsm_client_flush_route_ipv4(((struct lib_globals *)gpEigrp->eigrpZebos)->nc);*/

	EIGRP_FUNC_LEAVE(EigrpPortRedisTrigger);
#endif //EIGRP_PLAT_ZEBOS
//#endif
	return;
}

/* ????????????? */
void	EigrpPortRedisUnTrigger(U32 proto)
{
//#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
//#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
	struct nsm_msg_redistribute msg;
	struct lib_globals *pEg;
	U32	pro;
	EIGRP_FUNC_ENTER(EigrpPortRedisUnTrigger);

	switch(proto){
		case	EIGRP_ROUTE_CONNECT:
			pro	= IPI_ROUTE_CONNECT;
			break;
			
		case	EIGRP_ROUTE_STATIC:
			pro	= IPI_ROUTE_STATIC;
			break;
			
		case	EIGRP_ROUTE_RIP:
			pro	= IPI_ROUTE_RIP;
			break;
			
		case	EIGRP_ROUTE_OSPF:
			pro	= IPI_ROUTE_OSPF;
			break;	
	
		case	EIGRP_ROUTE_EIGRP:
			pro	= IPI_ROUTE_FRP;
			break;
			
		default:
			break;
	}
	
	EigrpPortMemSet((U8 *)&msg, 0, sizeof(struct nsm_msg_redistribute));
	msg.type = pro;
	msg.afi = AFI_IP;

	pEg	= (struct lib_globals *)gpEigrp->eigrpZebos;
	nsm_client_send_redistribute_unset(0, 0, pEg->nc, &msg);

	/*	nsm_client_flush_route_ipv4(((struct lib_globals *)gpEigrp->eigrpZebos)->nc);*/

	EIGRP_FUNC_LEAVE(EigrpPortRedisUnTrigger);
#endif//EIGRP_PLAT_ZEBOS
//#endif	
	return;
}


/************************************************************************************

	Name:	EigrpPortRedisAddProto

	Desc:	This function is to add a new route proto to the core route management module to permit
			the redistribution of the route from it.
			
	Para:	proto	- the type of the route to be added
	
	Ret:		NONE
************************************************************************************/
/* ??????????? */
void	EigrpPortRedisAddProto(EigrpPdb_st *pdb, U32 proto, U32 srcProc)
{
	EIGRP_FUNC_ENTER(EigrpPortRedisAddProto);
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		rtNode_pt	pRtNode;
		struct	rtEntryData_	*pRtEntry;
		EigrpConfAs_pt	pConfAs;
		U32	pro;
		
		for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
			if(pConfAs->asNum == pdb->process){
				break;
			}
		}
		if(!pConfAs){
			return;
		}

		if(!pConfAs->defMetric && (proto !=EIGRP_ROUTE_CONNECT &&
					proto !=EIGRP_ROUTE_STATIC)){
			return;
		}
		switch(proto){
			case	EIGRP_ROUTE_CONNECT:
				pro	= ROUTE_TYPE_CONNECT;
				break;
				
			case	EIGRP_ROUTE_STATIC:
				pro	= ROUTE_TYPE_STATIC;
				break;
				
			case	EIGRP_ROUTE_RIP:
				pro	= ROUTE_TYPE_RIP;
				break;
				
			case	EIGRP_ROUTE_OSPF:
				pro	= ROUTE_TYPE_OSPF;
				break;
				
			case	EIGRP_ROUTE_BGP:
				pro	= ROUTE_TYPE_BGP;
				break;
				
			case	EIGRP_ROUTE_IGRP:
				pro	= ROUTE_TYPE_IGRP;
				break;
				
			case	EIGRP_ROUTE_EIGRP:
				pro	= ROUTE_TYPE_EIGRP;
				break;
				
			default:
				break;
		}
		
		for(pRtNode = RouteNodeGetFirst(gstRt.pRtTree); pRtNode; 
			pRtNode = RouteNodeGetNext(gstRt.pRtTree, pRtNode)){
				
			for(pRtEntry	= pRtNode->rtEntry; pRtEntry; pRtEntry = pRtEntry->next){
				if(pRtEntry->type == pro && pRtEntry->process == srcProc){
					if(pro == ROUTE_TYPE_STATIC && 
						!EigrpIpNetworkEntrySearch(&pdb->netLst, pRtEntry->dest, pRtEntry->mask)){
							continue;
					}
					
					EigrpPortRedisAddRoute(pdb, pRtEntry->dest, pRtEntry->mask, pRtEntry->gateWay, proto, srcProc, pRtEntry->metric);
				}
			}
			
		}

	}
#endif
	EIGRP_FUNC_LEAVE(EigrpPortRedisAddProto);

	return;
}

/************************************************************************************

	Name:	EigrpPortRedisDelProto

	Desc:	This function is to delete a new route proto to the core route management module to deny the 
			redistribution of the route from it.
			
	Para: 	proto	- the type of the route to be deleted
	
	Ret:		NONE
************************************************************************************/
/* ??????????? */
void	EigrpPortRedisDelProto(EigrpPdb_st *pdb, U32 proto, U32 srcProc)
{
	EIGRP_FUNC_ENTER(EigrpPortRedisDelProto);
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#elif		(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		rtNode_pt	pRtNode;
		struct	rtEntryData_	*pRtEntry;
		EigrpConfAs_pt	pConfAs;
		U32	pro;
		
		for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
			if(pConfAs->asNum == pdb->process){
				break;
			}
		}
				
		switch(proto){
			case	EIGRP_ROUTE_CONNECT:
				pro	= ROUTE_TYPE_CONNECT;
				break;
				
			case	EIGRP_ROUTE_STATIC:
				pro	= ROUTE_TYPE_STATIC;
				break;
				
			case	EIGRP_ROUTE_RIP:
				pro	= ROUTE_TYPE_RIP;
				break;
				
			case	EIGRP_ROUTE_OSPF:
				pro	= ROUTE_TYPE_OSPF;
				break;
				
			case	EIGRP_ROUTE_BGP:
				pro	= ROUTE_TYPE_BGP;
				break;
			case	EIGRP_ROUTE_IGRP:
				pro	= ROUTE_TYPE_IGRP;
				break;
				
			default:
				break;
		}
		
		for(pRtNode = RouteNodeGetFirst(gstRt.pRtTree); pRtNode; 
			pRtNode = RouteNodeGetNext(gstRt.pRtTree, pRtNode)){
				
			for(pRtEntry	= pRtNode->rtEntry; pRtEntry; pRtEntry = pRtEntry->next){
				if(pRtEntry->type == pro && pRtEntry->process == srcProc){
					if(pro == ROUTE_TYPE_STATIC && 
						!EigrpIpNetworkEntrySearch(&pdb->netLst, pRtEntry->dest, pRtEntry->mask)){
							continue;
					}
					
					EigrpPortRedisDelRoute(pdb, pRtEntry->dest, pRtEntry->mask, pRtEntry->gateWay, proto);
				}
			}
			
		}

	}
#endif

	EIGRP_FUNC_LEAVE(EigrpPortRedisDelProto);

	return;
}


#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS

U32	asNumCur	= 0;
#define FRP_REDIST_PROTO_STR " (kernel|connected|static|rip|ospf|isis|bgp)"

#define FRP_REDIST_PROTO_HELP_STR \
       "Kernel routes",\
       "Connected",\
       "Static routes",\
       "Routing Information Protocol (RIP)",\
       "Open Shortest Path First (OSPF)",\
       "ISO IS-IS", \
       "Border Gateway Protocol"


#define FRP_CLI_FRP_INFO_STR                                                \
    "FRP information"


CLI (router_frp,
     router_frp_cmd,
     "router frp <1-65535>",
     "Enable a routing process",
     "Start FRP configuration",
     "Frp as number"
     )
{
	int frpAsNum = 0;

	if (argc > 0)
	CLI_GET_INTEGER_RANGE ("frp as number", frpAsNum, argv[0], 1, 65535);

	EigrpCmdApiRouterEigrp(0, frpAsNum);

	cli->mode = FRP_MODE;
	asNumCur	= frpAsNum;
	
	return CLI_SUCCESS;
}

CLI (no_router_frp,
     no_router_frp_cmd,
     "no router frp <1-65535>",
     "Negate a command or set its defaults",
     "Enable a routing process",
     "Start FRP configuration",
     "Frp as number"
     )
{
	int frpAsNum = 0;

	if (argc > 0)
	CLI_GET_INTEGER_RANGE ("frp as number", frpAsNum, argv[0], 1, 65535);

	EigrpCmdApiRouterEigrp(1, frpAsNum);


	return CLI_SUCCESS;
}

CLI (frp_interface,
     interface_frp_cmd,
     "interface IFNAME",
     "Select an interface to configure",
     "Interface's name"
     )
{
	struct interface *ifp;
	struct lib_globals *pEg;

#if 1
	pEg	= (struct lib_globals *)gpEigrp->eigrpZebos;

	ifp = ifg_get_by_name (&pEg->ifg, argv[0]);

	cli->index = ifp;
#endif
	cli->mode = INTERFACE_MODE;
	return CLI_SUCCESS;
}




CLI (frp_network_mask,
     frp_network_mask_cmd,
     "network A.B.C.D A.B.C.D",
     "Enable routing on an IP network",
     "Network IpAddress",
     "wild card bits"
     )
{
	U32	net, mask;

	EigrpUtilConvertStr2Ipv4(&net, argv[0]);
	EigrpUtilConvertStr2Ipv4Mask(&mask, argv[1]);

	EigrpCmdApiNetwork(0, asNumCur, net ,mask);

	return CLI_SUCCESS;
}

CLI (no_frp_network_mask,
     no_frp_network_mask_cmd,
     "no network A.B.C.D A.B.C.D",
     "Negate a command or set its defaults",
     "Enable routing on an IP network",
     "Network IpAddress",
     "wild card bits"
     )
{
	U32	net, mask;

	EigrpUtilConvertStr2Ipv4(&net, argv[0]);
	EigrpUtilConvertStr2Ipv4Mask(&mask, argv[1]);
	
	EigrpCmdApiNetwork(1, asNumCur, net , mask);

	return CLI_SUCCESS;
}

CLI (frp_redistribute_type,
     frp_redistribute_type_cmd,
     "redistribute  (" "kernel|connected|static|rip|bgp|isis|ospf" ")",
     "Redistribute information from another routing protocol",
     "Kernel routes", "Connected", "Static routes", "Routing Information Protocol (RIP)", "Border Gateway Protocol (BGP)", "ISO IS-IS", "Open Shortest Path First (OSPF)"
     )
{
	EigrpCmdApiRedistType(0, asNumCur, argv[0], 0);
	return CLI_SUCCESS;
}

CLI (no_frp_redistribute_type,
     no_frp_redistribute_type_cmd,
     "no redistribute  (" "kernel|connected|static|rip|bgp|isis|ospf" ")",
     "Negate a command or set its defaults",
     "Redistribute information from another routing protocol",
     "Kernel routes", "Connected", "Static routes", "Routing Information Protocol (RIP)", "Border Gateway Protocol (BGP)", "ISO IS-IS", "Open Shortest Path First (OSPF)"
     )
{
	EigrpCmdApiRedistType(1, asNumCur, argv[0], 0);
	return CLI_SUCCESS;
}

CLI (config_network_metric,
     config_network_metric_cmd,
     "set network A.B.C.D A.B.C.D metric <0-4294967295> <1-4294967295>  <1-1500> <0-255> <1-255>",
     "Set network metric",
     "The destination network",
     "The destination network address",
     "The destination network mask",
     "set vector metric",
     "Delay metric, in 1 millisecond units",
     "Bandwidth in Mbits per second",
     "MTU",
     "Reliability metric where 255 is 100% reliable",
     "Effective bandwidth metric (Loading) where 255 is 100% loaded"
     )
{
	S32		retVal;
	U32		ipNet, mask;
	U32		vMetric[6]; /*0-delay 1-bw 2-mtu 3-hopcount 4-reliability 5-load*/

	EigrpUtilConvertStr2Ipv4(&ipNet, argv[0]);
	EigrpUtilConvertStr2Ipv4Mask(&mask, argv[1]);
	ipNet = ipNet & mask;
	if(!ipNet){
		return CLI_FAILURE;
	}
	vMetric[0]	= EigrpPortAtoi(argv[2]) * 25600;
	vMetric[1]	= EigrpPortAtoi(argv[3]) * 256;
	vMetric[2]	= EigrpPortAtoi(argv[4]);
	vMetric[3]	= 0;
	vMetric[4]	= EigrpPortAtoi(argv[5]);
	vMetric[5]	= EigrpPortAtoi(argv[6]);

	retVal = EigrpCmdApiNetMetric(0, ipNet, vMetric);
	if(retVal == FAILURE){
		printf("Failed to send eigrp command to command queue.\n");
		return CLI_FAILURE;
	}
	return CLI_SUCCESS;
}

CLI (config_no_network_metric,
     config_no_network_metric_cmd,
     "no set network A.B.C.D A.B.C.D metric",
     "Negate a command or set its defaults\n"
     "Set network metric\n"
     "The destination network\n"
     "The destination network address\n"
     "The destination network mask\n"
     "Set the metirc to destination network\n"
     )
{
	S32		retVal;
	U32		ipNet, mask;

	EigrpUtilConvertStr2Ipv4(&ipNet, argv[0]);
	EigrpUtilConvertStr2Ipv4Mask(&mask, argv[1]);
	ipNet = ipNet & mask;
	if(!ipNet){
		return CLI_FAILURE;
	}
	

	retVal = EigrpCmdApiNetMetric(1, ipNet, NULL);
	if(retVal == FAILURE){
		printf("Failed to send eigrp command to command queue.\n");
		return CLI_FAILURE;
	}
	return CLI_SUCCESS;
}

CLI (show_ip_frp_interface,
	show_ip_frp_interface_cmd,
	"show ip frp interface",
	 CLI_SHOW_STR,
	 CLI_IP_STR,
	 FRP_CLI_FRP_INFO_STR,
	"Interface information"
     )
{
	EigrpPdb_st		*pdb;
	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		EigrpShowInterface("telnet cli", (void *)cli, 0, (S32(*)(U8 *, void *, U32, U32, U8 *))EigrpPortCliOut, 
							pdb->ddb, NULL, FALSE);
	}
	return	CLI_SUCCESS;
}

CLI (show_ip_frp_neighbor,
	show_ip_frp_neighbor_all_cmd,
	"show ip frp neighbor",
	 CLI_SHOW_STR,
	 CLI_IP_STR,
	 FRP_CLI_FRP_INFO_STR,
	"neighbor information"
     )
{
	EigrpPdb_st		*pdb;

	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		EigrpShowNeighbors("telnet cli", (void *)cli, 0, (S32(*)(U8 *, void *, U32, U32, U8 *))EigrpPortCliOut, 
							pdb->ddb, NULL, FALSE);
	}
	return	CLI_SUCCESS;
}
#if 0
CLI (show_ip_frp_struct,
	show_ip_frp_struct_cmd,
	"show ip frp struct",
	 CLI_SHOW_STR,
	 CLI_IP_STR,
	 FRP_CLI_FRP_INFO_STR,
	"struct information"
     )
{
	EigrpCmdApiShowStruct();
	return	CLI_SUCCESS;
}
#endif
CLI (show_ip_frp_topology,
	show_ip_frp_topology_all_cmd,
	"show ip frp topology",
	 CLI_SHOW_STR,
	 CLI_IP_STR,
	 FRP_CLI_FRP_INFO_STR,
	"topology information"
     )
{
	EigrpPdb_st		*pdb;

	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		EigrpShowTopology("telnet cli", (void *)cli, 0, (S32 (*)(U8 *, void *, U32, U32, U8 *))EigrpPortCliOut, 
							pdb->ddb, EIGRP_TOP_ALL);
	}
	return	CLI_SUCCESS;
	
}

CLI (show_ip_frp_proto,
	show_ip_frp_protocol_cmd,
	"show ip frp protocol",
	 CLI_SHOW_STR,
	 CLI_IP_STR,
	 FRP_CLI_FRP_INFO_STR,
	"protocol information"
     )
{
	EigrpPdb_st		*pdb;

	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		EigrpUtilShowProtocol("telnet cli", (void *)cli, 0, (S32 (*)(U8 *, void *, U32, U32, U8 *))EigrpPortCliOut,
								pdb);
	}
	
	return	CLI_SUCCESS;
}

CLI (debug_ip_frp,
	debug_ip_frp_cmd,
	"debug ip frp all",
	 CLI_DEBUG_STR,
	CLI_IP_STR,
	 FRP_CLI_FRP_INFO_STR,
	"all infomation"
     )
{
	EigrpCmdApiDebugAll();

	return	CLI_SUCCESS;
}

CLI (no_debug_ip_frp,
	no_debug_ip_frp_cmd,
	"no debug ip frp all",
	CLI_NO_STR,
	CLI_DEBUG_STR,
	CLI_IP_STR,
	FRP_CLI_FRP_INFO_STR,
	"all infomation"
     )
{
	EigrpCmdApiNoDebugAll();

	return	CLI_SUCCESS;
}

CLI (debug_ip_frp_packet,
	debug_ip_frp_packet_cmd,
	"debug ip frp packet",
	 CLI_DEBUG_STR,
	 CLI_IP_STR,
	 FRP_CLI_FRP_INFO_STR,
       "packet debug switches"
     )
{
	EigrpCmdApiDebugPacket();

	return	CLI_SUCCESS;
}

CLI (no_debug_ip_frp_packet,
	no_debug_ip_frp_packet_cmd,
	"no debug ip frp packet",
	CLI_NO_STR,
	CLI_DEBUG_STR,
	CLI_IP_STR,
	FRP_CLI_FRP_INFO_STR,
       "packet debug switches"
     )
{
	EigrpCmdApiNoDebugPacket();

	return	CLI_SUCCESS;
}

CLI (debug_ip_frp_event,
	debug_ip_frp_event_cmd,
	"debug ip frp event",
	 CLI_DEBUG_STR,
	 CLI_IP_STR,
	 FRP_CLI_FRP_INFO_STR,
       "event debug switches"
     )
{
	EigrpCmdApiDebugEvent();

	return	CLI_SUCCESS;
}

CLI (no_debug_ip_frp_event,
	no_debug_ip_frp_event_cmd,
	"no debug ip frp event",
	CLI_NO_STR,
	CLI_DEBUG_STR,
	CLI_IP_STR,
	FRP_CLI_FRP_INFO_STR,
       "event debug switches"
     )
{
	EigrpCmdApiNoDebugEvent();

	return	CLI_SUCCESS;
}

CLI (debug_ip_frp_task,
	debug_ip_frp_task_cmd,
	"debug ip frp task",
	 CLI_DEBUG_STR,
	 CLI_IP_STR,
	 FRP_CLI_FRP_INFO_STR,
       "task debug switches"
     )
{
	EigrpCmdApiDebugTask();

	return	CLI_SUCCESS;
}

CLI (no_debug_ip_frp_task,
	no_debug_ip_frp_task_cmd,
	"no debug ip frp task",
	CLI_NO_STR,
	CLI_DEBUG_STR,
	CLI_IP_STR,
	FRP_CLI_FRP_INFO_STR,
       "task debug switches"
     )
{
	EigrpCmdApiNoDebugTask();

	return	CLI_SUCCESS;
}

CLI (debug_ip_frp_timer,
	debug_ip_frp_timer_cmd,
	"debug ip frp timer",
	 CLI_DEBUG_STR,
	 CLI_IP_STR,
	 FRP_CLI_FRP_INFO_STR,
       "timer debug switches"
     )
{
	EigrpCmdApiDebugTimer();

	return	CLI_SUCCESS;
}

CLI (no_debug_ip_frp_timer,
	no_debug_ip_frp_timer_cmd,
	"no debug ip frp timer",
	CLI_NO_STR,
	CLI_DEBUG_STR,
	CLI_IP_STR,
	FRP_CLI_FRP_INFO_STR,
       "timer debug switches"
     )
{
	EigrpCmdApiNoDebugTimer();

	return	CLI_SUCCESS;
}


CLI (config_if_frp_hello_interval,
	config_if_frp_hello_interval_cmd,
	"frp <1-65535> hello-interval <1-65535>",
	"Set hello time interval",
	"Frp as number",
	"Hello-interval",
	"Hello interval value in second(MUST be times of 5 when set on vlan interface)"
     )
{    
 	struct interface *ifp = cli->index;
	 
	EigrpCmdApiHelloInterval(FALSE,EigrpPortAtoi(argv[0]), ifp->name, EigrpPortAtoi(argv[1]));
	
	return	CLI_SUCCESS;
}

 CLI (config_if_no_frp_hello_interval,
	config_if_no_frp_hello_interval_cmd,
	"no frp <1-65535> hello-interval",
	CLI_NO_STR,
	"set hello time interval",
	"Frp as number",
	"hello time interval"
     )
 {
 
	struct interface *ifp = cli->index;

	EigrpCmdApiHelloInterval(TRUE,EigrpPortAtoi(argv[0]), ifp->name, 0);
	return	CLI_SUCCESS;
 }

CLI(config_if_frp_hold_interval,
	config_if_frp_hold_interval_cmd,
	"frp <1-65535> hold-time <1-65535>",
	"FRP interface specific commands",
       "Autonomous system number",
	"Time in seconds to keep neighbors",
	"Hold time value"
)
{
	struct interface *ifp = cli->index;

	EigrpCmdApiHoldtime(FALSE, EigrpPortAtoi(argv[0]), ifp->name, EigrpPortAtoi(argv[1]));

}

CLI(config_if_no_frp_hold_interval,
	config_if_no_frp_hold_interval_cmd,
	"no frp <1-65535> hold-time",
	"Negate a command or set its defaults",
	"FRP interface specific commands",
       "Autonomous system number",
	"Time in seconds to keep neighbors"
)
{
	 struct interface *ifp = cli->index;
	 
	EigrpCmdApiHoldtime(TRUE, asNumCur, ifp->name, 0);
}

CLI (config_router_frp_metric_weights,
	config_router_frp_metric_weights_cmd,
	"metric weights tos <0-65535> <0-65535> <0-65535> <0-65535> <0-65535>",
	"set metric weights tos",
	"set metric weights tos",
	"set metric weights tos",
	"k1",
	"k2",
	"k3",
	"k4",
	"k5"
     )
{
	U32	k1, k2, k3, k4, k5;

	k1	= EigrpPortAtoi(argv[0]);
	k2	= EigrpPortAtoi(argv[1]);
	k3	= EigrpPortAtoi(argv[2]);
	k4	= EigrpPortAtoi(argv[3]);
	k5	= EigrpPortAtoi(argv[4]);

	EigrpCmdApiMetricWeights(FALSE, asNumCur, k1, k2, k3, k4, k5);
	return	CLI_SUCCESS;
}


CLI (config_router_frp_no_metric_weights,
	config_router_frp_no_metric_weights_cmd,
	"no metric weights tos <0-65535> <0-65535> <0-65535> <0-65535> <0-65535>",
	CLI_NO_STR,
	"set metric weights tos",
	"set metric weights tos",
	"set metric weights tos",
	"k1",
	"k2",
	"k3",
	"k4",
	"k5"
     )
{
	U32	k1, k2, k3, k4, k5;

	k1	= EigrpPortAtoi(argv[0]);
	k2	= EigrpPortAtoi(argv[1]);
	k3	= EigrpPortAtoi(argv[2]);
	k4	= EigrpPortAtoi(argv[3]);
	k5	= EigrpPortAtoi(argv[4]);

	EigrpCmdApiMetricWeights(TRUE, asNumCur, k1, k2, k3, k4, k5);
	return	CLI_SUCCESS;
}

CLI (config_router_frp_default_metric,
	config_router_frp_default_metric_cmd,
	"default-metric <1-4294967295> <0-4294967295> <0-255> <1-255> <1-4294967295>",
	"Set metric of redistributed routes",
	"Bandwidth in Kbits per second",
	"Delay metric, in 10 microsecond units",
	"Reliability metric where 255 is 100% reliable",
	"Effective bandwidth metric (Loading) where 255 is 100% loaded",
	"Maximum Transmission Unit metric of the path"
     )
{
	S32	band, delay, reli, load, mtu;

	band	= EigrpPortAtoi(argv[0]);
	delay	= EigrpPortAtoi(argv[1]);
	reli		= EigrpPortAtoi(argv[2]);
	load		= EigrpPortAtoi(argv[3]);
	mtu		= EigrpPortAtoi(argv[4]);

	if(band < 1/* || band > 0xFFFFFFFF*/ ||
					delay < 0/* || delay > 0xFFFFFFFF*/ ||
					reli < 0 || reli > 255 ||
					load < 1 || load > 255 ||
					mtu < 1/* || mtu > 0xFFFFFFFF*/){
		printf("Invalid value.\n");
		return CLI_FAILURE;	
	}
	EigrpCmdApiDefaultMetric(FALSE, asNumCur, band, delay, reli, load, mtu);

	return	CLI_SUCCESS;
}

CLI (config_router_frp_no_default_metric,
	config_router_frp_no_default_metric_cmd,
	"no default-metric",
	CLI_NO_STR,
	"Set metric of redistributed routes"
     )
{
	EigrpCmdApiDefaultMetric(TRUE, asNumCur, 0, 0, 0, 0, 0);

	return	CLI_SUCCESS;
}

CLI(config_router_frp_auto_sum,
	config_router_frp_auto_sum_cmd,
	"auto-summary",
	"Enable automatic network number summarization"
	)
{
	EigrpCmdApiAutosummary(FALSE, asNumCur);

}

CLI(config_router_frp_no_auto_sum,
	config_router_frp_no_auto_sum_cmd,
	"no auto-summary",
	CLI_NO_STR,
	"disable automatic network number summarization"
	)
{

	EigrpCmdApiAutosummary(TRUE, asNumCur);
}

CLI(config_router_frp_neighber,
	config_router_frp_neighber_cmd,
	"neighber A.B.C.D",
	"Set neighber",
	"Neighber IP Address "
	)
{
	U32	asNum, ipAddr;
	
	asNum	= EigrpPortAtoi(argv[0]);
	EigrpUtilConvertStr2Ipv4(&ipAddr,argv[1]);
	EigrpCmdApiNeighbor(FALSE, asNumCur, ipAddr);
}
CLI(config_router_no_frp_neighber,
	config_router_no_frp_neighber_cmd,
	"no neighber A.B.C.D",
	CLI_NO_STR,
	"Set neighber",
	"Neighber IP Address "
	)
{
	U32	asNum, ipAddr;
	
	asNum	= EigrpPortAtoi(argv[0]);
	EigrpUtilConvertStr2Ipv4(&ipAddr,argv[1]);
	EigrpCmdApiNeighbor(TRUE, asNumCur, ipAddr);
}

CLI(config_if_frp_invisible,
	config_if_frp_invisible_cmd,
	"frp <1-65535> invisible",
	"FRP interface specific commands",
	"Autonomous system number",
	"Invisible to neighbors"
	)
{
	U32	asNum, retVal;
	
	struct interface *ifp = cli->index;	
	asNum	= EigrpPortAtoi(argv[0]);
	retVal = EigrpCmdApiInvisibleInterface(FALSE, asNum, ifp->name);
	if(retVal == FAILURE){
		printf("Failed to send eigrp command to command queue.\n");
		return CLI_FAILURE;
	}
	
	return 	CLI_SUCCESS;
}

CLI(config_if_no_frp_invisible,
	config_if_no_frp_invisible_cmd,
	"no frp <1-65535> invisible",
	"Negate a command or set its defaults",
	"FRP interface specific commands",
	"Autonomous system number",
	"Invisible to neighbors"
	)
{
	U32	asNum, retVal;
	
	struct interface *ifp = cli->index;	
	asNum	= EigrpPortAtoi(argv[0]);
	retVal = EigrpCmdApiInvisibleInterface(TRUE, asNum, ifp->name);
	if(retVal == FAILURE){
		printf("Failed to send eigrp command to command queue.\n");
		return CLI_FAILURE;
	}
	
	return 	CLI_SUCCESS;
}

CLI(config_if_frp_summary,
	config_if_frp_summary_cmd,
	"frp <1-65535> summary A.B.C.D A.B.C.D",
	"FRP interface specific commands",
       "Autonomous system number",
	"Perform address summarization",
	"IP Address",
	"IP Mask"
)
{	 
	struct interface *ifp = cli->index;
	U32	asNum, ipAddr, ipMask;
	
	asNum	= EigrpPortAtoi(argv[0]);
	EigrpUtilConvertStr2Ipv4(&ipAddr,argv[1]);
	EigrpUtilConvertStr2Ipv4(&ipMask,argv[2]);
	
	EigrpCmdApiSummaryAddress(FALSE, asNum, ifp->name, ipAddr, ipMask);

}


CLI(config_if_no_frp_summary,
	config_if_no_frp_summary_cmd,
	"no frp <1-65535> summary A.B.C.D A.B.C.D",
	"Negate a command or set its defaults",
	"FRP interface specific commands",
        "Autonomous system number",
	"Perform address summarization",
	"IP Address",
	"IP Mask"
)
{
	struct interface *ifp = cli->index;
	U32	asNum, ipAddr, ipMask;
	
	asNum	= EigrpPortAtoi(argv[0]);
	EigrpUtilConvertStr2Ipv4(&ipAddr,argv[1]);
	EigrpUtilConvertStr2Ipv4(&ipMask,argv[2]);
	
	EigrpCmdApiSummaryAddress(TRUE, asNum, ifp->name, ipAddr, ipMask);

}




CLI(config_if_frp_split_horizon,
	config_if_frp_split_horizon_cmd,
	"frp <1-65535> split-horizon",
	"FRP interface specific commands",
       "Autonomous system number",
	"Perform split horizon"
)
{
	struct interface *ifp = cli->index;

	EigrpCmdApiSplitHorizon(FALSE, EigrpPortAtoi(argv[0]), ifp->name);
}

CLI(config_if_no_frp_split_horizon,
	config_if_no_frp_split_horizon_cmd,
	"no frp <1-65535> split-horizon",
	"Negate a command or set its defaults",
	"FRP interface specific commands",
       "Autonomous system number",
	"Perform split horizon"
)
{
	struct interface *ifp = cli->index;

	EigrpCmdApiSplitHorizon(TRUE, EigrpPortAtoi(argv[0]), ifp->name);
}

CLI(config_if_frp_key,
	config_if_frp_key_cmd,
	"frp <1-65535>  key WORD",
	"FRP interface specific commands",
       "Autonomous system number",
	"Md5 key",
	"Key string"
)
{
	struct interface *ifp = cli->index;

	EigrpCmdApiAuthKeyStr(FALSE, EigrpPortAtoi(argv[0]), ifp->name, argv[1]);
}

CLI(config_if_no_frp_key,
	config_if_no_frp_key_cmd,
	"no frp <1-65535> key",
	"Negate a command or set its defaults",
       "Autonomous system number",
	"FRP interface specific commands",
	"Md5 key",
	"key"
)
{
	struct interface *ifp = cli->index;

	EigrpCmdApiAuthKeyStr(TRUE, EigrpPortAtoi(argv[0]), ifp->name, NULL);
}

CLI(config_if_frp_key_id,
	config_if_frp_key_id_cmd,
	"frp <1-65535> key-id <1-65535>",
	"FRP interface specific commands",
       "Autonomous system number",
	"Md5 key-idr",
	"key-id numbers"
)
{
	struct interface *ifp = cli->index;

	EigrpCmdApiAuthKeyid(FALSE, EigrpPortAtoi(argv[0]), ifp->name, EigrpPortAtoi(argv[1]));
}

CLI(config_if_no_frp_key_id,
	config_if_no_frp_key_id_cmd,
	"no frp <1-65535> key-id",
	"Negate a command or set its defaults",
	"FRP interface specific commands",
       "Autonomous system number",
	"Md5 key-idr"
)
{
	struct interface *ifp = cli->index;

	EigrpCmdApiAuthKeyid(TRUE, EigrpPortAtoi(argv[0]), ifp->name, 0);
}

CLI(config_if_frp_authmode,
	config_if_frp_authmode_cmd,
	"frp <1-65535> authmode",
	"FRP interface specific commands",
       "Autonomous system number",
	"Md5 authentication mode"
)
{	
	struct interface *ifp = cli->index;

	EigrpCmdApiAuthMd5mode(FALSE, EigrpPortAtoi(argv[0]), ifp->name);
}

CLI(config_if_no_frp_authmode,
	config_if_no_frp_authmode_cmd,
	"no frp <1-65535> authmode",
	"Negate a command or set its defaults",
	"FRP interface specific commands",
       "Autonomous system number",
	"Md5 authentication mode"
)
{
	struct interface *ifp = cli->index;

	EigrpCmdApiAuthMd5mode(TRUE, EigrpPortAtoi(argv[0]), ifp->name);
}

CLI(frp_uai_sei_map,
	frp_uai_sei_map_cmd,
	"frp u-s-map vlan-id <1-4095>",
	FRP_CLI_FRP_INFO_STR,
	"Enable a UAI and SEI mapping",
	"give a vlan ID SEI used",
	"the vlan ID value"
)
{
	U32	vlan_id;

	vlan_id = EigrpPortAtoi(argv[0]);
	if(vlan_id == wireless_board_vlan[0]){
		printf("vlan%d has been reserved and can not be map to FRP UAI.\n", vlan_id);
		return CLI_FAILURE;	
	}

	EigrpCmdApiUaiP2mpPort_vlanid(FALSE, vlan_id);
}

CLI(no_frp_uai_sei_map,
	no_frp_uai_sei_map_cmd,
	"no frp u-s-map vlan-id <1-4095>",
	"Negate a command or set its defaults",
	FRP_CLI_FRP_INFO_STR,
	"UAI and SEI mapping",
	"give a vlan ID SEI used",
	"the vlan ID value"
)
{
	EigrpCmdApiUaiP2mpPort_vlanid(TRUE, EigrpPortAtoi(argv[0]));
}

CLI(ip_neighber_set,
	ip_neighber_set_cmd,
	"ip neighber A.B.C.D  vlan-id <1-4095>",
	"Enable a neighber",
	"Set global static neighber",
	"Neighber IP Address",
	"give a vlan ID SEI used",
	"the vlan ID value"
)
{
	U32	ipaddr;
	
	EigrpUtilConvertStr2Ipv4(&ipaddr, argv[0]);
	EigrpCmdApiUaiP2mpPort_Nei(FALSE, ipaddr,EigrpPortAtoi(argv[1]));
}

CLI(no_ip_neighber_set,
	no_ip_neighber_set_cmd,
	"no ip neighber A.B.C.D  vlan-id <1-4095>",
	"Negate a command or set its defaults",
	"Enable a neighber",
	"Set global static neighber",
	"Neighber IP Address",
	"give a vlan ID SEI used",
	"the vlan ID value"
)
{
	U32	ipaddr;
	
	EigrpUtilConvertStr2Ipv4(&ipaddr, argv[0]);
	EigrpCmdApiUaiP2mpPort_Nei(TRUE, ipaddr,EigrpPortAtoi(argv[1]));
}

CLI(config_if_frp_uai_mp2mp,
	config_if_frp_uai_mp2mp_cmd,
	"frp uai mp2mp",
	FRP_CLI_FRP_INFO_STR,
	"set ethernet port as Unumbered Auto-negotiation Interface",
	"set this port as point to multipoint port"
)
{
	struct interface *ifp = cli->index;
	U8	idStr[16];
	U16	vlanId;

	if(!ifp || EigrpPortStrLen(ifp->name) > 16){
		return FAILURE;
	}

	if(EigrpPortMemCmp((U8 *)ifp->name, (U8 *)"vlan", EigrpPortStrLen("vlan"))){
		return FAILURE;
	}

	EigrpPortMemSet((U8 *)idStr, 0, 16);
	EigrpPortMemCpy((U8 *)idStr, &ifp->name[EigrpPortStrLen("vlan")], EigrpPortStrLen(ifp->name) - EigrpPortStrLen("vlan"));
	vlanId	= EigrpPortAtoi(idStr);

	if(vlanId < 1 || vlanId > 4094){
		return FAILURE;
	}

	EigrpCmdApiUaiP2mpPort(FALSE, ifp->name);
/*zhenxl_20120801	EigrpCmdApiUaiP2mpPort_vlanid(FALSE, vlanId);*/
}

CLI(config_if_no_frp_uai_mp2mp,
	config_if_no_frp_uai_mp2mp_cmd,
	"no frp uai mp2mp",
	"Negate a command or set its defaults",
	FRP_CLI_FRP_INFO_STR,
	"set ethernet port as Unumbered Auto-negotiation Interface",
	"set this port as point to multipoint port"

)
{
	struct interface *ifp = cli->index;
	U8	idStr[16];
	U16	vlanId;

	if(!ifp || EigrpPortStrLen(ifp->name) > 16){
		return FAILURE;
	}

	if(EigrpPortMemCmp((U8 *)ifp->name, (U8 *)"vlan", sizeof("vlan"))){
		return FAILURE;
	}
	
	EigrpPortMemCpy((U8 *)idStr, ifp->name[sizeof("vlan")], EigrpPortStrLen(ifp->name) - sizeof("vlan"));
	vlanId	= EigrpPortAtoi(idStr);
	if(vlanId < 1 || vlanId > 4094){
		return FAILURE;
	}
	EigrpCmdApiUaiP2mpPort(TRUE, ifp->name);
/*	EigrpCmdApiUaiP2mpPort_vlanid(TRUE, vlanId);*/
}

CLI(config_if_bandwidth,
	config_if_bandwidth_cmd,
	"frp <1-65535> bandwidth <1-10000000>",
	FRP_CLI_FRP_INFO_STR,
	"Frp as number",
	"Set bandwidth informational parameter",
	"Bandwidth value in kbps")
{
	struct interface *ifp;
	U32 bandwidth, asNum;
	int ret;
	
	ifp = (struct interface *) cli->index;
	ifp = ifg_lookup_by_name(&nzg->ifg, ifp->name);/*zhenxl_20121111*/
	if(!ifp){
		EIGRP_ASSERT(0);
		printf("Interface not found.\n");
		return CLI_FAILURE;
	}
	asNum	= EigrpPortAtoi(argv[0]);
	bandwidth = EigrpPortAtoi(argv[1]);
	if(bandwidth < 1 || bandwidth > 10000000){
		printf("bandwidth value is error.\n");
		return CLI_FAILURE;
	}

	ifp->bandwidth = bandwidth;
	ret = EigrpCmdApiBandwidth(FALSE, asNum, ifp->name, bandwidth);
	if(ret == FAILURE){
		return CLI_FAILURE;
	}
	return CLI_SUCCESS;
}

CLI(config_if_no_bandwidth,
	config_if_no_bandwidth_cmd,
	"no frp <1-65535> bandwidth",
	"Negate a command or set its defaults",
	FRP_CLI_FRP_INFO_STR,
	"Frp as number",
	"Set bandwidth informational parameter")
{
	struct interface *ifp;
	U32 asNum;
	int ret;
	
	ifp = (struct interface *) cli->index;
	ifp = ifg_lookup_by_name(&nzg->ifg, ifp->name);/*zhenxl_20121111*/
	if(!ifp){
		EIGRP_ASSERT(0);
		printf("Interface not found.\n");
		return CLI_FAILURE;
	}
	asNum = EigrpPortAtoi(argv[0]);

	ifp->bandwidth = EIGRP_DEF_IF_BANDWIDTH;
	ret = EigrpCmdApiBandwidth(TRUE, asNum, ifp->name, 0);
	if(ret == FAILURE){
		return CLI_FAILURE;
	}
	return CLI_SUCCESS;
}

CLI(config_if_delay,
	config_if_delay_cmd,
	"frp <1-65535> delay <1-10000>",
	FRP_CLI_FRP_INFO_STR,
	"Frp as number",
	"Set delay informational parameter",
	"delay value in millisecond")
{
	struct interface *ifp;
	U32 delay, asNum;
	int ret;
	
	ifp = (struct interface *) cli->index;
	ifp = ifg_lookup_by_name(&nzg->ifg, ifp->name);/*zhenxl_20121111*/
	if(!ifp){
		EIGRP_ASSERT(0);
		printf("Interface not found.\n");
		return CLI_FAILURE;
	}
	asNum	= EigrpPortAtoi(argv[0]);
	delay = EigrpPortAtoi(argv[1]);
	if(delay < 1 || delay > 10000000){
		printf("bandwidth value is error.\n");
		return CLI_FAILURE;
	}
	delay *= 25600;/*25600 is 1ms*/
	ifp->delay = delay;
	ret = EigrpCmdApiDelay(FALSE, asNum, ifp->name, delay);
	if(ret == FAILURE){
		printf("Failed to send eigrp command to command queue.\n");
		return CLI_FAILURE;
	}
	return CLI_SUCCESS;
}

CLI(config_if_no_delay,
	config_if_no_delay_cmd,
	"no frp <1-65535> delay",
	"Negate a command or set its defaults",
	FRP_CLI_FRP_INFO_STR,
	"Frp as number",
	"Set delay informational parameter")
{
	struct interface *ifp;
	U32 asNum;
	int ret;
	
	ifp = (struct interface *) cli->index;
	ifp = ifg_lookup_by_name(&nzg->ifg, ifp->name);/*zhenxl_20121111*/
	if(!ifp){
		EIGRP_ASSERT(0);
		printf("Interface not found.\n");
		return CLI_FAILURE;
	}
	asNum	= EigrpPortAtoi(argv[0]);
	ifp->delay = EIGRP_DEF_IF_DELAY * 25600;
	ret = EigrpCmdApiDelay(FALSE, asNum, ifp->name, 0);
	if(ret == FAILURE){
		printf("Failed to send eigrp command to command queue.\n");
		return CLI_FAILURE;
	}
	return CLI_SUCCESS;
}


/*zhangming_begin_130128*/

#define DC_CLI_DC_INFO_STR                                                \
    "DC information"


void	DcCmdApiBuildRunConfMode(U8 **ppBuf, U32 *bufLen, U32 *usedLen)
{
	U32	cmdByte;	
	U8	bufTem[512];

	if(gstDc.redunType != DC_REDUNDANCY_TYPE_DEF){
		switch(gstDc.redunType){
			case	DC_REDUNDANCY_TYPE_ONLINE:
				cmdByte	= sprintf(bufTem, "!\r\ndc redundancy-type online\n");
				break;

			case	DC_REDUNDANCY_TYPE_LOADBALANCE:
				cmdByte	= sprintf(bufTem, "!\r\ndc redundancy-type load-balance\n");
				break;

			case	DC_REDUNDANCY_TYPE_DIFFSERV:
				cmdByte	= sprintf(bufTem, "!\r\ndc redundancy-type diffserv\n");
				break;

			default:
				cmdByte	= 0;
				break;
		}
		
		DcUtilEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
		DcPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
		*usedLen = *usedLen + cmdByte;
	}

	return; 
}


CLI(config_redunonline,
	config_redunonline_cmd,
	"dc redundancy-type online",
	DC_CLI_DC_INFO_STR,
       "Redundancy route type",
       "online"  )
{
	U32  ret;

	DcCmdApiRedunOnline(0);

	return CLI_SUCCESS;
}

CLI(config_no_redunonline,
	config_no_redunonline_cmd,
	"no dc redundancy-type online",
	"no",
	DC_CLI_DC_INFO_STR,
       "Redundancy route type",
       "online"  )
{
	U32  ret;

	DcCmdApiRedunOnline(1);

	return CLI_SUCCESS;
}


CLI(config_redunloadbalance,
	config_redunloadbalance_cmd,
	"dc redundancy-type load-balance",
	DC_CLI_DC_INFO_STR,
       "Redundancy route type",
       "Load balance" )
{
	U32  ret;
	
	DcCmdApiRedunLoadBalance(0);

	return CLI_SUCCESS;
}

CLI(config_no_redunloadbalance,
	config_no_redunloadbalance_cmd,
	"no dc redundancy-type load-balance",
	"no",
	DC_CLI_DC_INFO_STR,
       "Redundancy route type",
       "online"  )
{
	U32  ret;

	DcCmdApiRedunLoadBalance(1);

	return CLI_SUCCESS;
}



CLI(config_redundiffserv,
	config_redundiffserv_cmd,
	"dc redundancy-type diffserv",
	DC_CLI_DC_INFO_STR,
       "Redundancy route type",
       "diffserv" )
{
	U32  ret;
	
	DcCmdApiRedunDiffserv(0);

	return CLI_SUCCESS;
}

CLI(config_no_redundiffserv,
	config_no_redundiffserv_cmd,
	"no dc redundancy-type diffserv",
	"no",
	DC_CLI_DC_INFO_STR,
       "Redundancy route type",
       "diffserv" )
{
	U32  ret;
	
	DcCmdApiRedunDiffserv(1);

	return CLI_SUCCESS;
}


/*zhangming_end_130128*/


void	EigrpPortCliPrint(struct cli *cli, S8 *buffer)
{
	cli_out(cli, "%s", buffer);
	EigrpPortMemSet(buffer, 0, EigrpPortStrLen(buffer));
	
	return;
}

S32	EigrpPortCliOut(U8 *name, void *pCli, U32 id, U32 flag, U8 *buffer)
{
	struct cli *cli;
	cli	= (struct cli *)pCli;
 	EigrpPortCliPrint(cli, (S8 *)buffer);
	return	SUCCESS;
}

void	eigrp_lib_cli_init (void *pEg)
{
	struct lib_globals *eg	= (struct lib_globals *)pEg;
	/* Initialize signal and lib stuff. */
	host_vty_init (eg);

	/* Initialize Access List. */
	access_list_init (eg);

	/* Initialize Prefix List. */
	prefix_list_init (eg);

	return;
}

S32	frp_router_config_write (struct cli *cli)
{
	S32	write	= 0;
	EigrpPdb_st	*pdb;

	U8	*buff;
	U32	bufLen	= 0;
	U32	usedLen	= 0;


	buff	= (U8 *)EigrpPortMemMalloc(256);
	bufLen	= 256;
	EigrpCmdApiBuildRunConfMode_Router(&buff, &bufLen, &usedLen);
	
	cli_out (cli, "%s\n", buff);


	write	= usedLen;
	EigrpPortMemFree(buff);

	return	write;
}

S32	frp_router_debug_write (struct cli *cli)
{
	S32	write	= 0;

	return	write;
}

S32 frp_router_interface_write(struct cli *cli)
{
	S32	write	= 0;
	EigrpPdb_st	*pdb;
 	struct interface *ifp = cli->index;
	struct listnode *n1;

	U8	*buff;
	U32	bufLen	= 0;
	U32	usedLen	= 0;



	buff	= (U8 *)EigrpPortMemMalloc(512);
	bufLen	= 512;
	LIST_LOOP(cli->vr->ifm.if_list,ifp,n1)	
	{
		EigrpCmdApiBuildRunIntfMode_Router(&buff, &bufLen, &usedLen, ifp->name);
	}
	cli_out (cli, "%s\n", buff);


	write	= usedLen;
	EigrpPortMemFree(buff);

	return	write;
}

void	eigrp_cli_router_init (void *pCtree)
{
	struct cli_tree *ctree	= (struct cli_tree *)pCtree;

	cli_install_config (ctree, FRP_MODE, frp_router_config_write);
	cli_install_default (ctree, FRP_MODE);

  /* by lihui  */
  	cli_install_config (ctree, INTERFACE_MODE, frp_router_interface_write);

  /* by lihui . */
  	cli_install_default (ctree, INTERFACE_MODE);

	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &router_frp_cmd);
	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &no_router_frp_cmd);

	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &interface_frp_cmd);

	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &frp_uai_sei_map_cmd);
	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &no_frp_uai_sei_map_cmd);
	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &ip_neighber_set_cmd);
	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &no_ip_neighber_set_cmd);
	cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0, &config_if_bandwidth_cmd);
	cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0, &config_if_delay_cmd);

	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &config_redunonline_cmd);  /*zhangming_130128*/
	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &config_redunloadbalance_cmd);  /*zhangming_130128*/
	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &config_redundiffserv_cmd);  /*zhangming_130128*/
	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &config_no_redunonline_cmd);  /*zhangming_130128*/
	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &config_no_redunloadbalance_cmd);  /*zhangming_130128*/
	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &config_no_redundiffserv_cmd);  /*zhangming_130128*/
/*by lihui*/
	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_frp_hello_interval_cmd);  
  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_no_frp_hello_interval_cmd);  

  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_frp_hold_interval_cmd);  
  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_no_frp_hold_interval_cmd);
  
 	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_frp_summary_cmd);  
	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_no_frp_summary_cmd);  

  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_frp_split_horizon_cmd);  
  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_no_frp_split_horizon_cmd);  

  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_frp_key_cmd);
  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_no_frp_key_cmd);
  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_frp_key_id_cmd);
  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_no_frp_key_id_cmd);
  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_frp_authmode_cmd);
  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_no_frp_authmode_cmd);

  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_frp_uai_mp2mp_cmd);
  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_no_frp_uai_mp2mp_cmd);
	
	return;
}


void	eigrp_cli_eigrp_init (void *pCtree)
{
	S32	ret;
	struct cli_tree *ctree	= (struct cli_tree *)pCtree;
	cli_install_gen (ctree, FRP_MODE, PRIVILEGE_NORMAL, 0, &frp_network_mask_cmd);
	cli_install_gen (ctree, FRP_MODE, PRIVILEGE_NORMAL, 0, &no_frp_network_mask_cmd);

	cli_install_gen (ctree, FRP_MODE, PRIVILEGE_NORMAL, 0, &frp_redistribute_type_cmd);
	cli_install_gen (ctree, FRP_MODE, PRIVILEGE_NORMAL, 0, &no_frp_redistribute_type_cmd);
	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &config_network_metric_cmd);
	cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0, &config_no_network_metric_cmd);

/*by lihui*/
  	cli_install_imi (ctree, FRP_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_router_frp_metric_weights_cmd);  
  	cli_install_imi (ctree, FRP_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_router_frp_no_metric_weights_cmd);   

 	cli_install_imi (ctree, FRP_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_router_frp_auto_sum_cmd);  
  	cli_install_imi (ctree, FRP_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_router_frp_no_auto_sum_cmd); 

	/*add_zhenxl_20100908*/
  	cli_install_imi (ctree, FRP_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_router_frp_default_metric_cmd);  
  	cli_install_imi (ctree, FRP_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_router_frp_no_default_metric_cmd);   
	/*hanbing add 20120707*/
  	cli_install_imi (ctree, FRP_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_router_frp_neighber_cmd); 
  	cli_install_imi (ctree, FRP_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_router_no_frp_neighber_cmd); 

	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_frp_invisible_cmd); 
  	cli_install_imi (ctree, INTERFACE_MODE, PM_FRP,  PRIVILEGE_NORMAL,  0, &config_if_no_frp_invisible_cmd); 

	return;
}

void	eigrp_show_init (void *pCtree)
{
	S32	ret;
	struct cli_tree *ctree	= (struct cli_tree *)pCtree;

	cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
		   &show_ip_frp_interface_cmd);

	cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
		   &show_ip_frp_neighbor_all_cmd);

	/*add_zhenxl_20100908*/
	/*cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &show_ip_frp_struct_cmd);*/

	cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
		   &show_ip_frp_topology_all_cmd);

	cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
		   &show_ip_frp_protocol_cmd);

	return;
	
}

void	eigrp_debug_init(void *pCtree)
{
	S32	ret;
	struct cli_tree *ctree	= (struct cli_tree *)pCtree;

	cli_install_config (ctree, DEBUG_MODE, frp_router_debug_write);

	cli_install_gen(ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &debug_ip_frp_cmd);
	cli_install_gen(ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &no_debug_ip_frp_cmd);
	cli_install_gen(ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &debug_ip_frp_packet_cmd);
	cli_install_gen(ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &no_debug_ip_frp_packet_cmd);
	cli_install_gen(ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &debug_ip_frp_event_cmd);
	cli_install_gen(ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &no_debug_ip_frp_event_cmd);
	cli_install_gen(ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &debug_ip_frp_task_cmd);
	cli_install_gen(ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &no_debug_ip_frp_task_cmd);
	cli_install_gen(ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &debug_ip_frp_timer_cmd);
	cli_install_gen(ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &no_debug_ip_frp_timer_cmd);

	return;
}

void	eigrp_cli_init(void *pEg)
{
	struct lib_globals *eg	= (struct lib_globals *)pEg;
	struct cli_tree *ctree = eg->ctree;
	struct show_server *frp_show;

	frp_show = show_server_init (eg, ctree);
	if (! frp_show){
		return;
	}
	eigrp_cli_router_init(ctree);
	eigrp_cli_eigrp_init(ctree);
	eigrp_show_init(ctree);
	eigrp_debug_init(ctree);

	return;
}
/* 2009.09.10  by songjinliang*/
/*extern	struct thread *thread_fetch_one (struct lib_globals *, struct thread *);*/
void	EigrpPortZebraGetMsg()
{
	struct thread thread;

	if(!gpEigrp->eigrpZebos){
		return;
	}
	
	if(thread_fetch_one(gpEigrp->eigrpZebos, &thread)){
		thread_call(&thread);
	}

	return;
}


S32	EigrpPortZebraRecvService (struct nsm_msg_header *header, void *arg, void *message)
{
	return 0;
}

void	EigrpPortZebraUnlockRtNode(void *pNode)
{
#ifdef	RR_FUNC
	EIGRP_RR_FUNC_ENTER(EigrpPortZebraUnlockRtNode)
#endif	
	nsm_route_unlock_node((struct nsm_route_node *)pNode);
#ifdef	RR_FUNC
	EIGRP_RR_FUNC_LEAVE(EigrpPortZebraUnlockRtNode)
#endif	
	return;
}

S32 EigrpPortIntfLinkUp(struct nsm_msg_header *header, void *arg, void *message)
{
	struct nsm_msg_link *msg = message;
	struct EigrpIntf_	*pEigrpIntf;
	static int	i = 0;
#ifdef	RR_FUNC
	EIGRP_RR_FUNC_ENTER(EigrpPortIntfLinkUp)
#endif	
	for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
		if(pEigrpIntf->ifindex== msg->ifindex){
			break;
		}
	}

	if(!pEigrpIntf){
		EigrpPortAssert(0, "");
		return;	/* tigerwh added 120603 */
	}
	
	EigrpSysIntfUp_ex(pEigrpIntf->sysCirc);
	
#ifdef	RR_FUNC
	EIGRP_RR_FUNC_LEAVE(EigrpPortIntfLinkUp);
#endif
	return 0;
}

S32 EigrpPortIntfLinkDown(struct nsm_msg_header *header, void *arg, void *message)
{
	struct nsm_msg_link *msg = message;
	struct EigrpIntf_	*pEigrpIntf;
	static int	i = 0;
#ifdef	RR_FUNC
	EIGRP_RR_FUNC_ENTER(EigrpPortIntfLinkDown)
#endif	
	
	for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
		if(pEigrpIntf->ifindex== msg->ifindex){
			break;
		}
	}

	if(!pEigrpIntf){
		EigrpPortAssert(0, "");
		return;	/* tigerwh added 120603 */
	}

	EigrpSysIntfDown_ex((void *)pEigrpIntf->sysCirc);

#ifdef	RR_FUNC
	EIGRP_RR_FUNC_LEAVE(EigrpPortIntfLinkDown);
#endif	
	return 0;
}

S32	EigrpPortIntfAdd(struct nsm_msg_header *header, void *arg, void *message)
{
	struct interface *ifp;
	struct nsm_msg_link *msg = message;
	struct EigrpIntf_ *pEigrpIntf;

#ifdef	RR_FUNC
	EIGRP_RR_FUNC_ENTER(EigrpPortIntfAdd)
#endif	
	for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
		if(pEigrpIntf->ifindex== msg->ifindex){
			break;
		}
	}

	if(pEigrpIntf){
		EigrpPortAssert(0, "");
		return;	/* tigerwh added 120603 */
	}
#if 0
	ifp	= (struct interface *)EigrpPortMemMalloc(sizeof(struct interface));
	EigrpPortMemSet((U8 *) ifp, 0, sizeof(struct interface));

	nsm_util_link_val_set(msg, ifp);
#else/*zhenxl_20121111*/
	ifp = ifg_lookup_by_name(&nzg->ifg, msg->ifname);
	if(!ifp){
		return	0;
	}
#endif
	pEigrpIntf	= (struct EigrpIntf_ *)EigrpPortMemMalloc(sizeof(struct EigrpIntf_));
	EigrpPortMemSet((U8 *)pEigrpIntf, 0, sizeof(struct EigrpIntf_));
	EigrpPortCopyIntfInfo(pEigrpIntf, ifp);

	EigrpIntfAdd(pEigrpIntf);
/*	EigrpSysIntfAdd_ex((void *)ifp);*/
#ifdef	RR_FUNC
	EIGRP_RR_FUNC_LEAVE(EigrpPortIntfAdd);
#endif
	return 0;
}
extern U32	DcPortGetTimeInSec();
extern U32	DcPortGetTimeMSec();

S32	EigrpPortIntfDel(struct nsm_msg_header *header, void *arg, void *message)
{
	struct interface *ifp;
	struct nsm_msg_link *msg = message;
	struct EigrpIntf_ *pEigrpIntf;
	
#ifdef	RR_FUNC
	EIGRP_RR_FUNC_ENTER(EigrpPortIntfDel)
#endif	
	for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
		if(pEigrpIntf->ifindex== msg->ifindex){
			break;
		}
	}

	if(!pEigrpIntf){
		EigrpPortAssert(0, "");
		return;	/* tigerwh added 120603 */
	}

	pEigrpIntf->sysCirc = NULL;/*zhenxl_20121126*/
	
	EigrpSysIntfDel2_ex(msg->ifindex);
#ifdef	RR_FUNC
	EIGRP_RR_FUNC_LEAVE(EigrpPortIntfDel);
#endif		
	return 0;
}

S32	EigrpPortAddrAdd(struct nsm_msg_header *header, void *arg, void *message)
{
	struct nsm_client_handler *nch = arg;
	struct nsm_client *nc = nch->nc;
	struct nsm_msg_address *msg = message;
	struct interface *ifp;
	U32	mask;
	struct EigrpIntf_ *pEigrpIntf;

#ifdef	RR_FUNC
	EIGRP_RR_FUNC_ENTER(EigrpPortAddrAdd)
#endif
	for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
		if(pEigrpIntf->ifindex== msg->ifindex){
			break;
		}
	}

	if(!pEigrpIntf){
		return 0;
		EigrpPortAssert(0, "");
	}

	ifp	= (struct interface *)pEigrpIntf->sysCirc;

	/* Lookup index. */

#if 0	
/*	printf("testzebra EigrpPortAddrAdd 1 index %d\n", msg->ifindex);*/
	ifp = ifg_lookup_by_index (&nc->zg->ifg, msg->ifindex);
	if (! ifp)
		return 0;
#endif
	if(msg->afi == AFI_IP){
		if(ifp->ifc_ipv4){
			/**Some system(such as linux) may miss this interface delete opteration. tigerwh 100328 **/
			EigrpPortMemFree(ifp->ifc_ipv4);
		}

		ifp->ifc_ipv4	= (struct connected *)EigrpPortMemMalloc(sizeof(struct connected));
		ifp->ifc_ipv4->family	= AF_INET;

		ifp->ifc_ipv4->address	= (struct prefix *)EigrpPortMemMalloc(sizeof(struct prefix));
		EigrpPortMemSet((U8 *)ifp->ifc_ipv4->address, 0, sizeof(struct prefix));
		ifp->ifc_ipv4->address->family	= AF_INET;
		ifp->ifc_ipv4->address->prefixlen	= msg->prefixlen;
		ifp->ifc_ipv4->address->u.prefix4.s_addr	= NTOHL(msg->u.ipv4.src.s_addr);

		if(if_is_pointopoint(ifp)){
			ifp->ifc_ipv4->destination	= (struct prefix *)EigrpPortMemMalloc(sizeof(struct prefix));
			EigrpPortMemSet((U8 *)ifp->ifc_ipv4->destination, 0, sizeof(struct prefix));
			ifp->ifc_ipv4->destination->family	= AFI_IP;
			ifp->ifc_ipv4->destination->prefixlen	= 32;
			ifp->ifc_ipv4->destination->u.prefix4.s_addr	= NTOHL(msg->u.ipv4.dst.s_addr);
		}

		mask	= EigrpUtilLen2Mask(msg->prefixlen);
		pEigrpIntf->ipAddr	= NTOHL(msg->u.ipv4.src.s_addr);/*  */
		pEigrpIntf->ipMask	= mask;
		pEigrpIntf->remoteIpAddr	= 0;
		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
			if(pEigrpIntf->remoteIpAddr != NTOHL(msg->u.ipv4.dst.s_addr)){
				pEigrpIntf->remoteIpAddr = NTOHL(msg->u.ipv4.dst.s_addr);
			}
#if 0
			EigrpSysIntfAddrAdd_ex(msg->ifindex, NTOHL(msg->u.ipv4.dst.s_addr), 0xffffffff, resetIdb);
#else
			EigrpSysIntfAddrAdd_ex(pEigrpIntf->ifindex, pEigrpIntf->ipAddr, pEigrpIntf->ipMask, resetIdb);
#endif

#ifdef	RR_FUNC
			EIGRP_RR_FUNC_LEAVE(EigrpPortAddrAdd)
#endif			
			return	0;
		}
		
		EigrpSysIntfAddrAdd_ex(msg->ifindex, NTOHL(msg->u.ipv4.src.s_addr)/** tigerwh 100328 **/, mask);
	}

#ifdef	RR_FUNC
	EIGRP_RR_FUNC_LEAVE(EigrpPortAddrAdd);
#endif		
	return 0;
}

S32	EigrpPortAddrDel(struct nsm_msg_header *header, void *arg, void *message)
{
	struct nsm_client_handler *nch = arg;
	struct nsm_client *nc = nch->nc;
	struct nsm_msg_address *msg = message;
	struct interface *ifp;
	U32 mask;
	struct EigrpIntf_ *pEigrpIntf;

#ifdef	RR_FUNC
	EIGRP_RR_FUNC_ENTER(EigrpPortAddrDel)
#endif

	for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
		if(pEigrpIntf->ifindex== msg->ifindex){
			break;
		}
	}

	if(!pEigrpIntf){
		EigrpPortAssert(0, "");
	}

	
	if(msg->afi == AFI_IP){
		mask	= EigrpUtilLen2Mask(msg->prefixlen);

		EigrpSysIntfAddrDel_ex(msg->ifindex, NTOHL(msg->u.ipv4.src.s_addr)/** tigerwh 100328 **/, mask);
	}

#ifdef	RR_FUNC
	EIGRP_RR_FUNC_LEAVE(EigrpPortAddrDel)
#endif		
	return 0;

}

/************************************************************************************

	Name:	

	Desc:	
		
	Para: 
	
	Ret:		
************************************************************************************/
void eigrp_zebra_client_init()
{
	/* Create NSM client.  */
	struct lib_globals *pEg;
	S32	retVal;

	EIGRP_FUNC_ENTER(eigrp_zebra_client_init);

	gpEigrp->eigrpZebos	= lib_start();
	pEg	= (struct lib_globals *)gpEigrp->eigrpZebos;
	
	pEg->protocol	= IPI_PROTO_FRP;
	pEg->master = thread_master_create ();
	if(!pEg->master){
		EIGRP_FUNC_LEAVE(eigrp_zebra_client_init);
		return;
	}
	
	pEg->nc = nsm_client_create (pEg, 0);
	if (pEg->nc == NULL){
		EIGRP_FUNC_LEAVE(eigrp_zebra_client_init);
		return;
	}
	
	/* Set version and protocol.	*/
	nsm_client_set_version (pEg->nc, NSM_PROTOCOL_VERSION_1);
	nsm_client_set_protocol (pEg->nc, IPI_PROTO_FRP);

	/* Set required services.  */
	nsm_client_set_service (pEg->nc, NSM_SERVICE_INTERFACE);
	nsm_client_set_service (pEg->nc, NSM_SERVICE_ROUTE);
	nsm_client_set_service (pEg->nc, NSM_SERVICE_VRF);

	/* Register NSM callbacks.  */
	nsm_client_set_callback (pEg->nc, NSM_MSG_SERVICE_REPLY,
				 EigrpPortZebraRecvService);

	nsm_client_set_callback (pEg->nc, NSM_MSG_LINK_UP, EigrpPortIntfLinkUp);
	nsm_client_set_callback (pEg->nc, NSM_MSG_LINK_DOWN, EigrpPortIntfLinkDown);
	nsm_client_set_callback (pEg->nc, NSM_MSG_LINK_ADD, EigrpPortIntfAdd);
	nsm_client_set_callback (pEg->nc, NSM_MSG_LINK_DELETE, EigrpPortIntfDel);

	nsm_client_set_callback (pEg->nc, NSM_MSG_ADDR_ADD, EigrpPortAddrAdd);
	nsm_client_set_callback (pEg->nc, NSM_MSG_ADDR_DELETE, EigrpPortAddrDel);

	nsm_client_set_callback (pEg->nc, NSM_MSG_ROUTE_IPV4, EigrpPortRouteChange);


	/* Register interface address callbacks.	*/
	/*
	ifc_add_hook (&pEg->ifg, IFC_CALLBACK_ADDR_ADD, rip_if_addr_add_callback);
	ifc_add_hook (&pEg->ifg, IFC_CALLBACK_ADDR_DELETE,
	rip_if_addr_delete_callback);
	*/

	/* Start NSM processing.	*/

	retVal	= nsm_client_start (pEg->nc);

#ifdef HAVE_SNMP
	/* Initialize SMUX related stuff. */
	EigrpPortRouterSnmpInit (pEg);
#endif /* HAVE_SNMP */

	eigrp_lib_cli_init((void *)pEg);
	eigrp_cli_init((void *)pEg);
	host_config_start(pEg, NULL, FRP_VTY_PORT);

	EIGRP_FUNC_LEAVE(eigrp_zebra_client_init);

	return;
}

/************************************************************************************

	Name:	

	Desc:	
		
	Para: 
	
	Ret:		
************************************************************************************/

void eigrp_zebra_client_clean()
{
	EIGRP_FUNC_ENTER(eigrp_zebra_client_clean);
	if(((struct lib_globals *)gpEigrp->eigrpZebos)->nc){
		nsm_client_delete(((struct lib_globals *)gpEigrp->eigrpZebos)->nc);
	}
	EIGRP_FUNC_LEAVE(eigrp_zebra_client_clean);

	return;
}
void EigrpPortDelZebraIfp(void *ifp)
{
	struct interface *pIntf;

	EIGRP_FUNC_ENTER(EigrpPortDelZebraIfp);
	pIntf		= (struct interface *)ifp;

	if(pIntf->ifc_ipv4){
		if(pIntf->ifc_ipv4->address){
			EigrpPortMemFree(pIntf->ifc_ipv4->address);
		}

		if(pIntf->ifc_ipv4->destination){
			EigrpPortMemFree(pIntf->ifc_ipv4->destination);
		}

		EigrpPortMemFree(pIntf->ifc_ipv4);
		pIntf->ifc_ipv4	= NULL;
		
	}

	EigrpPortMemFree(pIntf);
	EIGRP_FUNC_LEAVE(EigrpPortDelZebraIfp);

	return;
}

S32	EigrpPortZebraDebugWrite(struct cli *cli)
{

	S32	write, i;
	
	cli_out(cli, "# -----------eigrp write start-----------\n");
	cli_out(cli, "gpEigrp->funcCnt = %d\n", gpEigrp->funcCnt);
	write++;
	
	for(i = 0 ; i < gpEigrp->funcMax; i++){
		cli_out(cli, "# %s\n", gpEigrp->funcStack[i]);
		write++;
	}
	cli_out(cli, "# -----------eigrp write end-----------\n");
	write++;
	
	return write;
}
void	EigrpPortRouterStackPrint()
{
	S32	i;

	printf("# -----------eigrp write start-----------\n");
	printf( "gpEigrp->funcCnt = %d\n", gpEigrp->funcCnt);
	
	for(i = 0 ; i < gpEigrp->funcMax; i++){
		printf( "# %s\n", gpEigrp->funcStack[i]);
 	}
	printf( "# -----------eigrp write end-----------\n");

	return;
}

#if		(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
extern struct	ifnet	*ifnet;
void	EigrpPortRouterModifyIntf(EigrpDualDdb_st *ddb)
{
	struct	ifnet	*pIfNet;

	for(pIfNet = ifnet; pIfNet; pIfNet = pIfNet->if_next){
		if(pIfNet->if_index == 1 || pIfNet->if_index == 2){
			continue;
		}
		if(BIT_TEST(pIfNet->if_flags, IFF_UP)){
			printf("make %s down\n", pIfNet->if_name);
			BIT_RESET(pIfNet->if_flags,IFF_UP);
		}else{
			printf("make %s up\n", pIfNet->if_name);
			BIT_SET(pIfNet->if_flags, IFF_UP);
		}
	}
/*	EigrpUtilMgdTimerStart(&ddb->ifModifyTimer, 1000);*/
	
	return;
		
}

void	*ifModifyContainer;
void	ifModifyMain()
{
	U32	secPrev, secNow;

	secPrev	= secNow	= EigrpPortGetTimeSec();

	while(ifModifyContainer){
		secNow	= EigrpPortGetTimeSec();
		if(secNow - secPrev >= 1){
			EigrpPortRouterModifyIntf(NULL);
			secPrev	= secNow;
		}
	}
}
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
void	EigrpPortRouterStartIfModifyTask()
{

#if(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	ifModifyContainer	= (void *)taskSpawn("tEigrp", 130, 0, 50000,  (FUNCPTR) ifModifyMain, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)

/*	gpEigrp->ifModifyTimer = EigrpPortTimerCreate(EigrpPortRouterModifyIntf, NULL, 1);
*/	
	return;
}
#endif//EIGRP_PLAT_ZEBOS
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)

/************************************************************************************

	Name:	EigrpPortDistCallBackInit

	Desc:	This function is to register redistribute functions to the system.
		
	Para: 	NONE	
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortDistCallBackInit()
{
	EIGRP_FUNC_ENTER(EigrpPortDistCallBackInit);
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#endif
	EIGRP_FUNC_LEAVE(EigrpPortDistCallBackInit);

	return;
}

/************************************************************************************

	Name:	EigrpPortDistCallBackClean

	Desc:	This function is to unregister redistribute functions from the system. 
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortDistCallBackClean()
{
	EIGRP_FUNC_ENTER(EigrpPortDistCallBackClean);
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#endif
	EIGRP_FUNC_LEAVE(EigrpPortDistCallBackClean);

	return;
}

/************************************************************************************

	Name:	EigrpPortCallBackInit

	Desc:	This function is to register call back functions to the system.
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortCallBackInit()
{
	EIGRP_FUNC_ENTER(EigrpPortCallBackInit);
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS	
	eigrp_zebra_client_init();
#endif//EIGRP_PLAT_ZEBOS	
#endif

	EigrpPortDistCallBackInit();
	EIGRP_FUNC_LEAVE(EigrpPortCallBackInit);
	
	return;
}

/************************************************************************************

	Name:	EigrpPortCallBackClean

	Desc:	This function is to unregister call back functions from the system.
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortCallBackClean()
{
	EIGRP_FUNC_ENTER(EigrpPortCallBackClean);
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS	
	eigrp_zebra_client_clean();
#endif//EIGRP_PLAT_ZEBOS	
#endif

	EigrpPortDistCallBackClean();
	EIGRP_FUNC_LEAVE(EigrpPortCallBackClean);
	
	return;
}

/************************************************************************************

	Name:	EigrpPortSockBuildIn

	Desc:	This function is to build an type of " eigrp sock in " data, using the given ip address
			and port.
		
	Para: 	port		- the given port 
			addr		- the given ip address
	
	Ret:		pointer to the new "eigrp sock in" data structure
************************************************************************************/

struct EigrpSockIn_ *EigrpPortSockBuildIn(U16 port, U32 addr)
{
	struct EigrpSockIn_ *s;

	EIGRP_FUNC_ENTER(EigrpPortSockBuildIn);
	s = (struct EigrpSockIn_*) EigrpPortMemMalloc(sizeof(struct EigrpSockIn_ ));
	if(!s){
		EIGRP_FUNC_LEAVE(EigrpPortSockBuildIn);
		return NULL;
	}
	s->sin_port	= port;
	s->sin_family = AF_INET;
	s->sin_addr.s_addr = addr;
	EIGRP_FUNC_LEAVE(EigrpPortSockBuildIn);

	return s;
}

/************************************************************************************

	Name:	EigrpPortGetRouteMapByName

	Desc:	This function is to get a point to a route map, given the name of the route map.
		
	Para: 	pName	- pointer to the name of route map
	
	Ret:		
************************************************************************************/

void	*EigrpPortGetRouteMapByName(U8 *pName)
{
	EIGRP_FUNC_ENTER(EigrpPortGetRouteMapByName);
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#endif

	EIGRP_FUNC_LEAVE(EigrpPortGetRouteMapByName);

	return NULL;
}

/************************************************************************************

	Name:	EigrpPortRouteMapJudge

	Desc:	This function is to judge if the given route is permitted, given the route and route
			map.
		
	Para: 	rtMap	- pointer to the route map
			ipAddr	- destination ip adress of the route
			ipMask	- destination ip mask of the route
			rt		- pointer to the given route
	
	Ret:		TRUE	for the given route is permitted
			FALSE	for it is not so
************************************************************************************/

S32	EigrpPortRouteMapJudge(void *rtMap, U32 ipAddr, U32 ipMask, void *rt)
{
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
/* Start Edit By  : AuthorName:zhurish : 2016/01/14 20 :18 :14  : Comment:????:.???? */
	U32	ipNet;
	EigrpDualNewRt_st	*pTmpRt = NULL;
	EigrpNetMetric_pt	pNet = NULL;
	
	pTmpRt = (EigrpDualNewRt_st *)rt;
	for(pNet = gpEigrp->pNetMetric; pNet; pNet = pNet->forw){
		if((ipAddr & ipMask) == pNet->ipNet){
			EigrpPortMemCpy((U8 *)&pTmpRt->vecMetric, (U8 *)pNet->vecmetric, sizeof(EigrpVmetric_st));
			pTmpRt->metric = 0;
			break;
		}
	}
	if(!pNet){
		pTmpRt->metric = 0;
	}
/* End Edit By  : AuthorName:zhurish : 2016/01/14 20 :18 :14  */
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	U32	ipNet;
	EigrpDualNewRt_st	*pTmpRt = NULL;
	EigrpNetMetric_pt	pNet = NULL;
	
	pTmpRt = (EigrpDualNewRt_st *)rt;
	for(pNet = gpEigrp->pNetMetric; pNet; pNet = pNet->forw){
		if((ipAddr & ipMask) == pNet->ipNet){
			EigrpPortMemCpy((U8 *)&pTmpRt->vecMetric, (U8 *)pNet->vecmetric, sizeof(EigrpVmetric_st));
			pTmpRt->metric = 0;
			break;
		}
	}
	if(!pNet){
		pTmpRt->metric = 0;
	}
#endif
	return TRUE;

}
void	usrAppShowNetMetricList()
{
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
/* Start Edit By  : AuthorName:zhurish : 2016/01/14 20 :17 :54  : Comment:????:.???? */
	U32	ipNet;
	EigrpDualNewRt_st	*pTmpRt = NULL;
	EigrpNetMetric_pt	pNet = NULL;
	
	for(pNet = gpEigrp->pNetMetric; pNet; pNet = pNet->forw){
		_EIGRP_DEBUG("\t%s\n", EigrpUtilIp2Str(pNet->ipNet));
	}
/* End Edit By  : AuthorName:zhurish : 2016/01/14 20 :17 :54  */
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	U32	ipNet;
	EigrpDualNewRt_st	*pTmpRt = NULL;
	EigrpNetMetric_pt	pNet = NULL;
	
	for(pNet = gpEigrp->pNetMetric; pNet; pNet = pNet->forw){
		_EIGRP_DEBUG("\t%s\n", EigrpUtilIp2Str(pNet->ipNet));
	}
#endif
	return;
}
/************************************************************************************

	Name:	

	Desc:	This function is to do the init of the route map , which is to be used by eigrp.
 
 			Note that this function is not necessary for every system, even if eigrp will use its
 			route map.
 			
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/
/*  
void	EigrpPortRouteMapInit()
{
	EIGRP_FUNC_ENTER(EigrpPortRouteMapInit);
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#endif//EIGRP_PLAT_ZEBOS
	eigrp_route_map_zebra_init();
#endif//EIGRP_PLAT_ZEBOS	
#endif
	EIGRP_FUNC_LEAVE(EigrpPortRouteMapInit);

	return;
}
*/

/************************************************************************************

	Name:	EigrpPortPermitIncoming

	Desc:	This function is to judge if the  incoming route is permitted to be accepted by the 
			eigrp, given the route and the iidb which contains the point to acl and prefix list.
			
	Para: 	ipAddr	- destination ip address of the given route
			ipMask	- destination ip mask of the given route
			iidb		- pointer to the eigrp interface data structure which contains the pointer
			acl and prefix list
	
	Ret:		TRUE	for the incoming route is permitted
			FALSE 	for it is not so
************************************************************************************/

S32	EigrpPortPermitIncoming(U32 ipAddr, U32 ipMask, EigrpIdb_st *iidb)
{
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#endif

	return TRUE;
}

/************************************************************************************

	Name:	EigrpPortPermitOutgoing

	Desc:	This function is to judge if the given eigrp route is permitted to be advertised to 
			other eigrp routers, given the route and the iidb which contains the point to the 
			acl and prefix list.
			
	Para: 	ipAddr	- destination ip address of the given route
			ipMask	- destination ip mask of the given route
			iidb		- pointer to the eigrp interface data structure which contains the pointer
					   acl and prefix list
	
	Ret:		TRUE	for the Outgoing route is permitted
			FALSE 	for it is not so
************************************************************************************/

S32	EigrpPortPermitOutgoing(U32 ipAddr, U32 ipMask, EigrpIdb_st *iidb)
{
		EIGRP_FUNC_ENTER(EigrpPortPermitOutgoing);
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#endif
	EIGRP_FUNC_LEAVE(EigrpPortPermitOutgoing);

	return TRUE;
}

/************************************************************************************

	Name:	EigrpPortGetDistByIfname

	Desc:	This function is to get a pointer to the route distribute infomation of an interface,
			given the name of the interface.
			
	Para: 	ifName	- pointer to the interface name
	
	Ret:		pointer to the route distribute information of an initerface
************************************************************************************/

void	*EigrpPortGetDistByIfname(U8 *ifName)
{
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		return NULL;
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
		return NULL;
	}
#endif
	return NULL;

}

/************************************************************************************

	Name:	EigrpPortDistributeUpdate

	Desc:	This function is to update the given distribute list.
		
	Para: 	pdist		- pointer to the given distribute list
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortDistributeUpdate(void *pdist)
{
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#endif
}

/************************************************************************************

	Name:	EigrpPortCheckAclPermission

	Desc:	This function is to judge if the given route is permitted by the acl, given the acl 
			index.
		
	Para: 	aclNum	- identifier of Eigrp routing process
			ipAddr	- destination ip address of the given route
			ipMask	- destination ip mask of the given route
	
	Ret:		SUCCESS	for the given route is permitted
			FAILURE	 	for it is not so
************************************************************************************/

S32	EigrpPortCheckAclPermission(U32 aclNum, U32 ipAddr, U32 ipMask)
{
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		return SUCCESS;
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
		return SUCCESS;
	}
#endif
	return TRUE;

}

/************************************************************************************

	Name:	EigrpPortIntfChange

	Desc:	This function is to process the signal of interface changing, coming from interface 
			mangement module.
		
	Para: 	index	- the index of interface
			event	- pointer to the event of interface changing
			
	Ret:		NONE
************************************************************************************/
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
void	EigrpPortIntfChange(void *pSysIntf, void *index, void *event)
{
	{
		intf_pt	pIntf;
#ifdef	RR_FUNC
		EIGRP_RR_FUNC_ENTER(EigrpPortIntfChange);
#endif
		pIntf		= (struct intf_ *)pSysIntf;
		switch((U32)event){	
			case	INTF_CHANGE_TYPE_CREATE:
				EigrpSysIntfAdd_ex(pIntf);
				break;

			case	INTF_CHANGE_TYPE_REMOVE:
				EigrpSysIntfDel_ex(pIntf);
				break;

			case	INTF_CHANGE_TYPE_UP:
				EigrpSysIntfUp_ex(pIntf);
				break;

			case	INTF_CHANGE_TYPE_DOWN:
				EigrpSysIntfDown_ex(pIntf);
				break;
		}
#ifdef	RR_FUNC
		EIGRP_RR_FUNC_LEAVE(EigrpPortIntfChange);
#endif
		return;
	}
	return;
}
#endif

/* ?????????FRP */
void	EigrpPortRouteProcEvent(U32 proto, U32 asNum, U32 destAddr, U32 destMask, 
								U32 gateway, U32 metric, U32 event)
{
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		U32	pro;
		struct EigrpPdb_ *pdb;
		struct EigrpConfAs_	*pConfAs;
		struct EigrpConfAsRedis_	*pRedis;

		EIGRP_FUNC_ENTER(EigrpPortRouteProcEvent);
		pro	= 0;
		
		switch(proto){
			case	ROUTE_TYPE_CONNECT:
				pro	= EIGRP_ROUTE_CONNECT;
				break;
				
			case	ROUTE_TYPE_STATIC:
				pro	= EIGRP_ROUTE_STATIC;
				break;
				
			case	ROUTE_TYPE_RIP:
				pro	= EIGRP_ROUTE_RIP;
				break;
				
			case	ROUTE_TYPE_OSPF:
				pro	= EIGRP_ROUTE_OSPF;
				break;
				
			case	ROUTE_TYPE_BGP:
				pro	= EIGRP_ROUTE_BGP;
				break;
			case	ROUTE_TYPE_IGRP:
				pro	= EIGRP_ROUTE_IGRP;
				break;
				
			default:
				break;
		}
		if(!pro){
			EIGRP_FUNC_LEAVE(EigrpPortRouteProcEvent);
			return;
		}

		switch((U32)event){
			case	EIGRP_REDIS_RT_UP:

				for(pdb = (struct EigrpPdb_ *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
					for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
						if(pConfAs->asNum == pdb->process){
							for(pRedis = pConfAs->redis; pRedis; pRedis = pRedis->next){
								if(!EigrpPortStrCmp(pRedis->protoName, EigrpProto2str(pro))&& 
									pRedis->srcProc == asNum){
									break;
								}
							}
							if(!pRedis){
								break;
							}
							EigrpPortRedisAddRoute(pdb, (U32)destAddr, (U32)destMask, (U32)gateway, pro, (U32)asNum, (U32)metric);
						}
						
					}

				}
				break;
				
			case	EIGRP_REDIS_RT_DOWN:
				for(pdb = (struct EigrpPdb_ *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
					for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
						if(pConfAs->asNum != pdb->process){
							continue;
						}
						for(pRedis = pConfAs->redis; pRedis; pRedis = pRedis->next){
							if(!EigrpPortStrCmp(pRedis->protoName, EigrpProto2str(pro))){
								break;
							}
						}
						if(!pRedis){
							break;
						}
						EigrpPortRedisDelRoute(pdb, (U32)destAddr, (U32)destMask, (U32)gateway, pro);
					}

				}
				break;
		}
		EIGRP_FUNC_LEAVE(EigrpPortRouteProcEvent);
		return;
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
		U32 pro;
		struct EigrpPdb_ *pdb;
		struct EigrpConfAs_ *pConfAs;
		struct EigrpConfAsRedis_	*pRedis;

		EIGRP_FUNC_ENTER(EigrpPortRouteProcEvent);
		pro = 0;
#ifdef EIGRP_PLAT_ZEBOS			
		switch(proto){
			case	IPI_ROUTE_CONNECT:
				pro = EIGRP_ROUTE_CONNECT;
				break;
				
			case	IPI_ROUTE_STATIC:
				pro = EIGRP_ROUTE_STATIC;
				break;
				
			case	IPI_ROUTE_RIP:
				pro = EIGRP_ROUTE_RIP;
				break;
				
			case	IPI_ROUTE_OSPF:
				pro = EIGRP_ROUTE_OSPF;
				break;
				
			case	IPI_ROUTE_BGP:
				pro = EIGRP_ROUTE_BGP;
				break;
				
			default:
				break;
		}
#endif//EIGRP_PLAT_ZEBOS
#ifdef EIGRP_PLAT_ZEBRA			
		switch(proto){
			case	ZEBRA_ROUTE_CONNECT:
				pro = EIGRP_ROUTE_CONNECT;
				break;
				
			case	ZEBRA_ROUTE_STATIC:
				pro = EIGRP_ROUTE_STATIC;
				break;
				
			case	ZEBRA_ROUTE_RIP:
				pro = EIGRP_ROUTE_RIP;
				break;
				
			case	ZEBRA_ROUTE_OSPF:
				pro = EIGRP_ROUTE_OSPF;
				break;
				
			case	ZEBRA_ROUTE_BGP:
				pro = EIGRP_ROUTE_BGP;
				break;
				
			default:
				break;
		}
		_EIGRP_DEBUG("%s:get redist %s\n",__func__, zebra_route_string(proto));
#endif//EIGRP_PLAT_ZEBRA
		if(!pro){
			_EIGRP_DEBUG("%s:if(!pro)\n",__func__);
			EIGRP_FUNC_LEAVE(EigrpPortRouteProcEvent);
			return;
		}

		switch((U32)event){
			case	EIGRP_REDIS_RT_UP:

				for(pdb = (struct EigrpPdb_ *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
					for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
						if(pConfAs->asNum == pdb->process){
							for(pRedis = pConfAs->redis; pRedis; pRedis = pRedis->next){
								
								_EIGRP_DEBUG("%s:%s %s\n",__func__,pRedis->protoName,EigrpProto2str(pro));
								
								if(!EigrpPortStrCmp(pRedis->protoName, EigrpProto2str(pro))){
									_EIGRP_DEBUG("%s:if(!EigrpPortStrCmp)\n",__func__);
									break;
								}
							}
							if(!pRedis){
								_EIGRP_DEBUG("%s:if(!pRedis)\n",__func__);
								break;
							}
							EigrpPortRedisAddRoute(pdb, (U32)destAddr, (U32)destMask, (U32)gateway, pro, (U32)asNum, (U32)metric);
						}
						
					}

				}
				break;
				
			case	EIGRP_REDIS_RT_DOWN:
				for(pdb = (struct EigrpPdb_ *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
					for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
						if(pConfAs->asNum != pdb->process){
							continue;
						}
						for(pRedis = pConfAs->redis; pRedis; pRedis = pRedis->next){
							if(!EigrpPortStrCmp(pRedis->protoName, EigrpProto2str(pro))){
								break;
							}
						}
						if(!pRedis){
							break;
						}
						EigrpPortRedisDelRoute(pdb, (U32)destAddr, (U32)destMask, (U32)gateway, pro);
					}

				}
				break;
		}
		EIGRP_FUNC_LEAVE(EigrpPortRouteProcEvent);
		return;
	}
#endif
}

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
void EigrpPortRouteChange(void *type, void *rtEntry)
{
	rtEntryData_pt	pRtEntry;
	U32	eventType;

	pRtEntry	= (rtEntryData_pt)rtEntry;
#ifdef	RR_FUNC
	EIGRP_RR_FUNC_ENTER(EigrpPortRouteChange);
#endif

	switch((U32)type){
		case	RT_CHANGE_TYPE_ADD:
			eventType	= EIGRP_REDIS_RT_UP;
			break;
			
		case	RT_CHANGE_TYPE_DEL:
			eventType	= EIGRP_REDIS_RT_DOWN;
			break;
			
	}
	
	EigrpPortRouteProcEvent(pRtEntry->type, pRtEntry->process, pRtEntry->dest, pRtEntry->mask, 
							pRtEntry->gateWay, pRtEntry->metric, eventType);
#ifdef	RR_FUNC
	EIGRP_RR_FUNC_LEAVE(EigrpPortRouteChange);
#endif
	return;
}

#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS	
S32 EigrpPortRouteChange(struct nsm_msg_header *header, void *arg, void *message)
{
	U32	eventType;
	struct nsm_msg_route_ipv4 *msg = message;

#ifdef	RR_FUNC
	EIGRP_RR_FUNC_ENTER(EigrpPortRouteChange);
#endif
	if(BIT_TEST(msg->flags, NSM_MSG_ROUTE_FLAG_ADD)){
		eventType	= EIGRP_REDIS_RT_UP;
	}else{
		eventType	= EIGRP_REDIS_RT_DOWN;
	}
printf("EigrpPortRouteChange: addr:0x%x, mask:0x%x, nexthop:0x%x\n", /** tigerwh 100328 **/
	 	NTOHL(msg->prefix.s_addr), EigrpUtilLen2Mask(msg->prefixlen), NTOHL(msg->nexthop[0].addr.s_addr));
	EigrpPortRouteProcEvent(msg->type, 0, NTOHL(msg->prefix.s_addr), EigrpUtilLen2Mask(msg->prefixlen), 
							NTOHL(msg->nexthop[0].addr.s_addr)/** tigerwh 100328 **/, 51200, eventType);
#ifdef	RR_FUNC
	EIGRP_RR_FUNC_LEAVE(EigrpPortRouteChange);
#endif
	return 0;
}
#endif// EIGRP_PLAT_ZEBOS	
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)

#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)||(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
//?????EIGRP????(????)
//#ifdef  EIGRP_PLAT_ZEBRA
void EigrpPortRouteChange(int as, int type, int proto, long ip, long mask, long next, int metric)
{
	U32	eventType = 0;	
	switch(type){
		case	ZEBRA_IPV4_ROUTE_ADD:
			eventType	= EIGRP_REDIS_RT_UP;
			break;
			
		case	ZEBRA_IPV4_ROUTE_DELETE:
			eventType	= EIGRP_REDIS_RT_DOWN;
			break;
	}
	_EIGRP_DEBUG("EigrpPortRouteChange: addr:0x%x, mask:0x%x, nexthop:0x%x\n", ip, mask, next);
	
	if(eventType != 0)
		EigrpPortRouteProcEvent(proto, as, ip, mask, 
							next/** tigerwh 100328 **/, metric, eventType);
}
//#endif//EIGRP_PLAT_ZEBRA
#endif//#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)

/************************************************************************************

	Name:	EigrpPortRegIntfCallbk

	Desc:	This function is to register the interface-changement-processing function to the system. 
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortRegIntfCallbk()
{
	EIGRP_FUNC_ENTER(EigrpPortRegIntfCallbk);
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		EIGRP_FUNC_LEAVE(EigrpPortRegIntfCallbk);
		return;
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		PILIfMRegisterCallBkOnChange(EigrpPortIntfChange);
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	/*
		Done in eigrp_zebra_client_init;
	*/
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	EIGRP_FUNC_LEAVE(EigrpPortRegIntfCallbk);

	return;
}

void	EigrpPortRegRtCallbk()
{
	EIGRP_FUNC_ENTER(EigrpPortRegRtCallbk);
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		EIGRP_FUNC_LEAVE(EigrpPortRegRtCallbk);
		return;
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		RtChgRegister((void(*)(U32, U32, U32))EigrpPortRouteChange);
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	/*
		Done in eigrp_zebra_client_init;
	*/
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	EIGRP_FUNC_LEAVE(EigrpPortRegRtCallbk);

	return;
}


/************************************************************************************

	Name:	EigrpPortUnRegIntfCallbk

	Desc:	This function is to register the interface-changement-processing function to the system. 
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/

void	EigrpPortUnRegIntfCallbk()
{
	EIGRP_FUNC_ENTER(EigrpPortUnRegIntfCallbk);
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		EIGRP_FUNC_LEAVE(EigrpPortUnRegIntfCallbk);
		return;
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		PILIfMUnRegisterCallBkOnChange(EigrpPortIntfChange);
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	/*
		Done in eigrp_zebra_client_clean;
	*/
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	EIGRP_FUNC_LEAVE(EigrpPortUnRegIntfCallbk);

	return;
}

void	EigrpPortUnRegRtCallbk()
{
	EIGRP_FUNC_ENTER(EigrpPortUnRegRtCallbk);
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		EIGRP_FUNC_LEAVE(EigrpPortUnRegRtCallbk);
		return;
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	{
		RtChgUnRegister((void(*)(U32, U32, U32))EigrpPortRouteChange);
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	/*
		Done in eigrp_zebra_client_clean;
	*/
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	EIGRP_FUNC_LEAVE(EigrpPortUnRegRtCallbk);

	return;
}

void EigrpPortPrintDbg(U8 *buffer, EigrpDbgCtrl_pt pDbg)
{
	EIGRP_FUNC_ENTER(EigrpPortPrintDbg);

	if(pDbg->funcShow){
		pDbg->funcShow(pDbg->name, pDbg->term, pDbg->id, pDbg->flag, buffer);
	}else{
		printf("%s ", buffer);

	}

	EIGRP_FUNC_LEAVE(EigrpPortPrintDbg);
	return;
}

void *EigrpPortGetGateIntf(U32 ipAddr)
{
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
#ifdef _EIGRP_PLAT_MODULE
		EigrpIntf_pt	pEigrpIntf, pTem;
		EIGRP_FUNC_ENTER(EigrpPortGetIntfByAddr);
		for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
			if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
				if(pEigrpIntf->remoteIpAddr == ipAddr){
					break;
				}
			}else{
				if((pEigrpIntf->ipAddr & pEigrpIntf->ipMask) == (ipAddr & pEigrpIntf->ipMask)){
					break;
				}
			}
		}
		if(pEigrpIntf){
			pTem	= EigrpPortMemMalloc(sizeof(EigrpIntf_st));
			if(!pTem){
				EIGRP_FUNC_LEAVE(EigrpPortGetIntfByAddr);
				return NULL;
			}
			EigrpPortMemCpy((U8 *)pTem, (U8 *)pEigrpIntf, sizeof(EigrpIntf_st));
			EIGRP_FUNC_LEAVE(EigrpPortGetIntfByAddr);
			return pTem;
		}
		return	NULL;
#endif// _EIGRP_PLAT_MODULE
#if 0
		EigrpIntf_pt	pEigrpIntf;
		struct	ifnet	*pIfNet;
		struct	ifaddr	*pIfAddr;
		U32		mask;
		for(pIfNet = ifnet; pIfNet; pIfNet = pIfNet->if_next){
			for(pIfAddr = pIfNet->if_addrlist; pIfAddr; pIfAddr = pIfAddr->ifa_next){
				if(pIfAddr->ifa_addr->sa_family != AF_INET){
					continue;
				}
				if(BIT_TEST(pIfNet->if_flags, IFF_POINTOPOINT)){
					mask	= EIGRP_PREFIX_TO_MASK(pConn->address->prefixlen);
					if(NTOHL((pIfAddr->ifa_addr)->sin_addr.s_addr) == ipAddr){
						pEigrpIntf	= EigrpPortMemMalloc(sizeof(EigrpIntf_st));
						
						pEigrpIntf->mtu 		= pIfNet->if_mtu;	
						pEigrpIntf->bandwidth	= pIfNet->if_baudrate / 1000;
						pEigrpIntf->metric		= pIfNet->if_metric;
					
						
						if(BIT_TEST(pIfNet->if_flags, IFF_BROADCAST)){
							BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_BROADCAST);
						}
						return	pEigrpIntf;
					}
				}
				pEigrpIntf->ipMask	= EIGRP_PREFIX_TO_MASK(pConn->address->prefixlen);
			}
		}
	#endif//0
		return	NULL;
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL || EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
		EigrpIntf_pt	pEigrpIntf, pTem;
		EIGRP_FUNC_ENTER(EigrpPortGetIntfByAddr);
		for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
			if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
				if(pEigrpIntf->remoteIpAddr == ipAddr){
					break;
				}
			}else{
				if((pEigrpIntf->ipAddr & pEigrpIntf->ipMask) == (ipAddr & pEigrpIntf->ipMask)){
					break;
				}
			}
		}
		if(pEigrpIntf){
			pTem	= EigrpPortMemMalloc(sizeof(EigrpIntf_st));
			if(!pTem){
				EIGRP_FUNC_LEAVE(EigrpPortGetIntfByAddr);
				return NULL;
			}

			EigrpPortMemCpy((U8 *)pTem, (U8 *)pEigrpIntf, sizeof(EigrpIntf_st));
			EIGRP_FUNC_LEAVE(EigrpPortGetIntfByAddr);
			return pTem;
		}
		return	NULL;
	}
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL || EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
}


#ifdef _EIGRP_PLAT_MODULE
int zebraEigrpRoutehandle(int (* func)(int , void *))
{
	Eigrp_Route_handle = func;
	return SUCCESS;
}
#endif//_EIGRP_PLAT_MODULE


#ifdef _DC_

#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
extern S32	Win32SysCreateUdpRecvSocket(U32, U16);
extern S32	Win32SysUdpRecvfrom(S32, S8 *, U32, U32 *, U16 *, U8);
extern U32	Win32SysUdpSendto(S32, S8 *, U32, U32, U16);
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)

S32	EigrpPortDcSockCreate()
{
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	if(gpEigrp->dcSock){
		printf("EIGRP DC data error!\n");
		return FAILURE;
	}

	gpEigrp->dcSock	= Win32SysCreateUdpRecvSocket(FALSE, DC_FRP_LOCAL_PORT);
	if(gpEigrp->dcSock <= 0){
		printf("EIGRP DC socket create failed!\n");
		return FAILURE;
	}

	gpEigrp->dcFDR	= Win32SysMallocFdSet();
	if(!gpEigrp->dcFDR){
		printf("EIGRP DC FDR create failed!\n");
		EigrpPortSockRelease(gpEigrp->dcSock);
		return FAILURE;
	}
#elif(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	{
		S32	sock, ret;
		U32	onU32, pktSize;
	
		if(gpEigrp->dcSock){
			printf("EIGRP DC data error!\n");
			return FAILURE;
		}
	
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if(sock == FAILURE){
			return -1;
		}
	
		onU32 = 1;
		ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (S8 *)&onU32, sizeof(onU32));
		if(ret == FAILURE){
			EigrpPortSockRelease(sock);
			return -1;
		}
	
		pktSize = 8 * 1024 * 1024;
		setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *) &pktSize, sizeof(pktSize));	/* JB_INTERNAL added by jiangxj 20080408 */
		if(ret == FAILURE){
			printf("SO_SNDBUF error");
		}
	
		pktSize = 8 * 1024 * 1024;
		setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *) &pktSize, sizeof(pktSize));	/* JB_INTERNAL added by jiangxj 20080408 */
		if(ret == FAILURE){
			printf("SO_RCVBUF error");
		}
	
	
		onU32	= 1;
		ret = ioctl(sock, (U32)FIONBIO, (S32)&onU32);
		if(ret == FAILURE){
			EIGRP_ASSERT(0);
			EigrpPortSockRelease(sock);
			return -1;
		}
	
		ret = EigrpPortBind(sock, 0, DC_FRP_LOCAL_PORT);
		if(ret != SUCCESS){
			EigrpPortSockRelease(sock);
			return -1;
		}
	
		gpEigrp->dcSock = sock;
	}
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	return SUCCESS;
}

S32	EigrpPortDcSendOne(S32 sockId, U8 *packet, U32 length)
{
	S32	retVal;
#if(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	retVal = Win32SysUdpSendto(sockId, packet, length, /*0xc0a80107*/0x7f000001, DC_CFG_PORT);
#elif(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	retVal = EigrpPortSendto(sockId, (S8 *)packet, length, 0x7f000001, DC_CFG_PORT);
#endif
	return	retVal;
}

S32	EigrpPortDcRecv(U8 *packet, U32 *length)
{
	S32	retVal;

#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	if(!gpEigrp->dcSock ||!gpEigrp->dcFDR){
		return FAILURE;
	}

	Win32SysFdZero(gpEigrp->dcFDR);

	Win32SysFdSet(gpEigrp->dcSock, gpEigrp->dcFDR);

	retVal	= Win32SysSelect(gpEigrp->dcSock + 1, gpEigrp->dcFDR, NULL, NULL, 0, 0);
	if(retVal <= 0){
		return FAILURE;
	}

	retVal	= Win32SysFdIsSet(gpEigrp->dcSock, gpEigrp->dcFDR);
	if(retVal == FALSE){
		return FAILURE;
	}

	retVal	= Win32SysUdpRcv(gpEigrp->dcSock, packet, *length);
	if(retVal <= 0){
		return FAILURE;
	}

	*length	= retVal;
#elif(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	if(!gpEigrp->dcSock){
		return FAILURE;
	}

	if(EigrpPortSelect(gpEigrp->dcSock) == TRUE){
		retVal = EigrpPortRecvfrom(gpEigrp->dcSock, (S8 *)packet, *length, 0, NULL, NULL);
		if(retVal <= 0){
			return FAILURE;
		}
	}
#endif

	return SUCCESS;
}

void	EigrpPortProcDcPkt(U8 *pktBuf, U32 pktLen)
{
#if	0
	U32	i;

	printf("\npkt:\n");
	
	for(i = 1; i < pktLen+1; i++){	
		printf("%.2x ", *(pktBuf + i-1));
		if(!(i%16)){
			printf("\n");
		}
	}
#endif	
}


void	EigrpPortSndDcUaiP2mp(S32 noFlag, U8 *seiName)
{
	DcPkt_st	pkt;

	EigrpPortMemSet((U8 *)&pkt, 0, sizeof(DcPkt_st));
	pkt.type	= DC_PKT_TYPE_FRP2DC_SEI;
	EigrpPortStrCpy(pkt.ifname, seiName);
	pkt.noFlag	= noFlag;

	EigrpPortDcSendOne(gpEigrp->dcSock, (U8 *)&pkt, sizeof(DcPkt_st));

	return;
}

void	EigrpPortSndDcUaiP2mp_vlanid(S32 noFlag, U32 vlan_id)
{
	DcPkt_st	pkt;
	S32	retVal;

	EigrpPortMemSet((U8 *)&pkt, 0, sizeof(DcPkt_st));
	pkt.type	= DC_PKT_TYPE_FRP2DC_SEI;
	pkt.noFlag = noFlag;
	pkt.u32Para = vlan_id;

	retVal = EigrpPortDcSendOne(gpEigrp->dcSock, (U8 *)&pkt, sizeof(DcPkt_st));

	return;
}
void	EigrpPortSndDcUaiP2mp_Nei(S32 noFlag, U32 ipaddr, U32 vlan_id)
{
	DcPkt_st	pkt;
	S32	retVal;

	EigrpPortMemSet((U8 *)&pkt, 0, sizeof(DcPkt_st));
	pkt.type	= DC_PKT_TYPE_NEI_SET;
	pkt.noFlag = noFlag;
	pkt.u32Para = vlan_id;
	pkt.u32Para2 = ipaddr;

	retVal = EigrpPortDcSendOne(gpEigrp->dcSock, (U8 *)&pkt, sizeof(DcPkt_st));

	return;
}

extern	void	DcCmdApiHelloInterval(S32, U8 *, U32);	/*VLAN bandwidth set*/
extern	void	DcCmdApiHoldTime(S32, U8 *, U32);	/*VLAN bandwidth set*/
extern	void	DcCmdApiBandwidth(S32, U8 *, U32);	/*VLAN bandwidth set*/
extern	void	DcCmdApiDelay(S32, U8 *, U32);	/*VLAN bandwidth set*/
void	EigrpPortVlanBandwidth(S32 noFlag, U8* ifName, U32 bandwidth)	/*VLAN bandwidth set*/
{
	DcCmdApiBandwidth(noFlag, ifName, bandwidth);
}
void	EigrpPortVlanDelay(S32 noFlag, U8* ifName, U32 delay)	/*VLAN bandwidth set*/
{
	DcCmdApiDelay(noFlag, ifName, delay);
}
void	EigrpPortVlanHelloInterval(S32 noFlag, U8* ifName, U32 helloInterval)	/*VLAN bandwidth set*/
{
	DcCmdApiHelloInterval(noFlag, ifName, helloInterval);
}

void	EigrpPortVlanHoldTime(S32 noFlag, U8* ifName, U32 holdTime)	/*VLAN bandwidth set*/
{
	DcCmdApiHoldTime(noFlag, ifName, holdTime);
}
#endif//_DC_

