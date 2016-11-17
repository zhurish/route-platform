#include "zvrrpd.h"
#include "zvrrp_sched.h"


#ifndef ZVRRPD_ON_ROUTTING
/*******************************************************************************/
#define TIMER_SECOND_MICRO 1000000L
/*******************************************************************************/
int zvrrp_gettimeofday(struct timeval *tv, int opt)
{
	int ret = -1;
	struct timespec rtp;
	ret = clock_gettime (CLOCK_REALTIME, &rtp);////��ȡϵͳ��ǰʵʱʱ�ӣ���������
	if(ret < 0)
		return ERROR;
	tv->tv_sec = rtp.tv_sec;
	tv->tv_usec = rtp.tv_nsec/1000;//���뻻��Ϊ΢��	
	return OK;
}
/*******************************************************************************/
/*******************************************************************************/
/************************************************************************************

	Name:	zvrrpsemcreate

	Desc:	This function is to create a binary semaphore and returns it .
	
 	Para: 	full		- indicates that whether it is initialized to be usable or not.
	
	Ret:		NONE		
************************************************************************************/

void	*zvrrpsemcreate(int full)
{
	void	*pSem;
	pSem	= malloc(sizeof(int));
	if(pSem == NULL)
		return NULL;
	*(int *)pSem	= (full == TRUE) ? FALSE : TRUE;
	return pSem;
}

/************************************************************************************

	Name:	zvrrpsemgive

	Desc:	This function is to release a binary semaphore.
		
	Para: 	pSem	- pointer to the binary semaphore
	
	Ret:		NONE
************************************************************************************/

int	zvrrpsemgive(void	*pSem)
{
	*(int *)pSem    = FALSE;
	return OK;
}

/************************************************************************************

	Name:	zvrrpsemtake

	Desc:	This function is to take a binary semaphore. If this semaphore is occupied, this
			function will wait. 
		
	Para: 	pSem	- pointer the the binary semaphore
	
	Ret:		NONE
************************************************************************************/

int	zvrrpsemtake(void *pSem, int timeout)
{
	while(*(int *)pSem == TRUE){
		zvrrpsleep(10);
	}
	*(int *)pSem    = TRUE;
	return OK;
}

/************************************************************************************

	Name:	zvrrpsemdelete

	Desc:	This function is to release and delete a binary Semaphore.
		
	Para: 	pSem	- pointer the the binary semaphore
	
	Ret:		NONE
************************************************************************************/

int	zvrrpsemdelete(void *pSem)
{
	free(pSem);
	return OK;
}
/*******************************************************************************/
zvrrp_sched_master * zvrrputilschedinit(void)
{
	zvrrp_sched_master *master = NULL;

	master	= (zvrrp_sched_master *)malloc(sizeof(zvrrp_sched_master));
	if(!master){	 
		return NULL;
	}
	memset((char *)master, 0, sizeof(zvrrp_sched_master));
	return master;
}

int zvrrputilschedclean(zvrrp_sched_master * master)
{
	if(master){
		free(master);
	}
	return OK;
}

int zvrrputilschedcancel(zvrrp_sched_master * master, zvrrp_sched * pSched)
{
	if(master && pSched){
		memset(pSched, 0, sizeof(zvrrp_sched));
	}
	return OK;
}
static int zvrrputilschedadd_onenode(zvrrp_sched * sched, int (*func)(void *), void *param, int type, int cmd, int mtime)
{
	int ret = -1;
	int index = 0;
	for(index = 0; index < ZVRRPD_SCHED_MAX; index++)
	{
		if(sched[index].used == 0)
		{
			memset(&sched[index], 0, sizeof(zvrrp_sched));
			sched[index].used	= 1;
			sched[index].type	= type;
			sched[index].cmd	= cmd;
			sched[index].func	= func;
			sched[index].data	= param;
			if(mtime)
			{
				ret = zvrrp_gettimeofday(&(sched[index].next_timer), 0);
				if(ret == ERROR)
				{
					return ERROR;
				}
				sched[index].value = mtime;

				sched[index].next_timer.tv_sec += mtime/1000;
				sched[index].next_timer.tv_usec += (mtime%1000)*1000;
			}
			return OK;
		}
	}
	return ERROR;
}
int zvrrputilschedaddread(zvrrp_sched_master * master, int (*func)(void *), void *param, int size)
{
	return zvrrputilschedadd_onenode(master->sched_read, func, param, ZVRRPD_SCHED_READ, 0, 0);
}
/************************************************************************************/
int zvrrputilschedcmdadd(zvrrp_sched_master * master, int (*func)(void *), void *param)
{
	return zvrrputilschedadd_onenode(master->sched_cmd, func, param, ZVRRPD_SCHED_CMD, 0, 0);
}
/************************************************************************************/
int zvrrputilschedaddtimer(zvrrp_sched_master * master, int (*func)(void *), void *param, int mtimer)
{
	return zvrrputilschedadd_onenode(master->sched_timer, func, param, ZVRRPD_SCHED_TIME, 0, mtimer);
}
/************************************************************************************/
static int zvrrputilschedtimerhandle(zvrrp_sched * pSched)
{		
	int ret = -1;
	struct timeval now;
	if(pSched)
	{
		if(pSched->type == ZVRRPD_SCHED_TIME)
		{
			ret = zvrrp_gettimeofday(&now, 0);
			if(ret == ERROR)
				return ERROR;
			
			if(now.tv_sec < pSched->next_timer.tv_sec)
				return ERROR;
			
			else if(now.tv_sec == pSched->next_timer.tv_sec)
			{
				if(now.tv_usec < pSched->next_timer.tv_usec)
					return ERROR;
			}
			pSched->next_timer.tv_sec = now.tv_sec + pSched->value/1000;
			pSched->next_timer.tv_usec = now.tv_usec + (pSched->value%1000)*1000;
			return OK;
		}
	}
	return ERROR;
}
/************************************************************************************/
static int zvrrputilsched_onenode(zvrrp_sched_master *master, zvrrp_sched *	pSched)
{		
	int	(*func)(void *) = NULL;
	if((pSched)&&(pSched->used == 1))
	{			
		if( (pSched->type == ZVRRPD_SCHED_FUNC)||(pSched->type == ZVRRPD_SCHED_READ) )
		{
			func	= (int (*)(void *))pSched->func;
			if(func)
				(func)(pSched->data);
		}
		else if(pSched->type == ZVRRPD_SCHED_TIME)
		{
			if(zvrrputilschedtimerhandle(pSched) == OK)
			{
				func	= (int (*)(void *))pSched->func;
				if(func)
					(func)(pSched->data);
			}
		}
		
		else if(pSched->type == ZVRRPD_SCHED_CMD)
		{
			func	= (int (*)(void *))pSched->func;
			if(func)
				(func)(pSched->data);
		}
		return OK;
	}
	return ERROR;
}
/************************************************************************************/
int zvrrputilsched(zvrrp_sched_master *master, zvrrp_sched * sched)
{		
	int index = 0;
	for(index = 0; index < ZVRRPD_SCHED_MAX; index++)
	{
		if(sched[index].used == 1)
		{
			zvrrputilsched_onenode(master, &sched[index]);
		}
	}
	return OK;
}
/************************************************************************************/
/************************************************************************************/
#endif//ZVRRPD_ON_ROUTTING
/************************************************************************************/
/************************************************************************************/
int zvrrp_cmd_init(struct zvrrp_master * zvrrp)
{
	if(zvrrp == NULL)
		return ERROR;	
	zvrrp->opcode = malloc(sizeof(zvrrp_opcode));
	if(zvrrp->opcode == NULL)
	{
		return ERROR;
	}
	memset(zvrrp->opcode, 0, sizeof(zvrrp_opcode));
	return OK;
}
int zvrrp_cmd_uninit(struct zvrrp_master * zvrrp)
{
	if( (zvrrp == NULL)||(zvrrp->opcode == NULL) )
		return ERROR;	
	free(zvrrp->opcode);
	zvrrp->opcode = NULL;
	return OK;
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
int zvrrp_cmd_vrrp(int type, int vrid)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	gVrrpMatser->opcode->vrid = vrid;
	if(type)
		gVrrpMatser->opcode->cmd = zvrrp_opcode_add_vrs;
	else
		gVrrpMatser->opcode->cmd = zvrrp_opcode_del_vrs;
	return zvrrp_cmd_config(gVrrpMatser);
}
int zvrrp_cmd_enable(int type, int vrid)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;	
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	gVrrpMatser->opcode->vrid = vrid;
	if(type)
		gVrrpMatser->opcode->cmd = zvrrp_opcode_enable_vrs;
	else	
		gVrrpMatser->opcode->cmd = zvrrp_opcode_disable_vrs;
	return zvrrp_cmd_config(gVrrpMatser);
}
int zvrrp_cmd_vip(int type, int vrid, long ip)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;	
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	gVrrpMatser->opcode->vrid = vrid;
	if(type)
		gVrrpMatser->opcode->cmd = zvrrp_opcode_add_ip;
	else
		gVrrpMatser->opcode->cmd = zvrrp_opcode_del_ip;
	gVrrpMatser->opcode->ipaddress = ip;
	return zvrrp_cmd_config(gVrrpMatser);
}
int zvrrp_cmd_interface(int type, int vrid, char *ifname)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;	
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	gVrrpMatser->opcode->vrid = vrid;
	if(type)
		gVrrpMatser->opcode->cmd = zvrrp_opcode_add_interface;
	else
		gVrrpMatser->opcode->cmd = zvrrp_opcode_del_interface;	
	strcpy(gVrrpMatser->opcode->ifname, ifname);
	return zvrrp_cmd_config(gVrrpMatser);
}
int zvrrp_cmd_set_adev_time(int type, int vrid, int sec)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;	
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	gVrrpMatser->opcode->vrid = vrid;
	if(type)
		gVrrpMatser->opcode->cmd = zvrrp_opcode_set_interval;
	else	
		gVrrpMatser->opcode->cmd = zvrrp_opcode_unset_interval;	
	gVrrpMatser->opcode->value = sec;
	return zvrrp_cmd_config(gVrrpMatser);
}
int zvrrp_cmd_set_adev_mtime(int type, int vrid, int msec)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;	
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	gVrrpMatser->opcode->vrid = vrid;
	if(type)
		gVrrpMatser->opcode->cmd = zvrrp_opcode_set_interval_msec;
	else	
		gVrrpMatser->opcode->cmd = zvrrp_opcode_unset_interval_msec;	
	gVrrpMatser->opcode->value = msec;
	return zvrrp_cmd_config(gVrrpMatser);
}
int zvrrp_cmd_priority(int type, int vrid, int pri)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;	
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	gVrrpMatser->opcode->vrid = vrid;
	if(type)
		gVrrpMatser->opcode->cmd = zvrrp_opcode_set_pri;
	else	
		gVrrpMatser->opcode->cmd = zvrrp_opcode_unset_pri;	
	gVrrpMatser->opcode->value = pri;
	return zvrrp_cmd_config(gVrrpMatser);
}
int zvrrp_cmd_ping(int type)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;	
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	if(type)
		gVrrpMatser->opcode->cmd = zvrrp_opcode_enable_ping;
	else	
		gVrrpMatser->opcode->cmd = zvrrp_opcode_disable_ping;	
	return zvrrp_cmd_config(gVrrpMatser);
}
int zvrrp_cmd_learn_master(int type, int vrid)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;	
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	gVrrpMatser->opcode->vrid = vrid;
	if(type)
		gVrrpMatser->opcode->cmd = zvrrp_opcode_set_learn_master;
	else	
		gVrrpMatser->opcode->cmd = zvrrp_opcode_unset_learn_master;	
	return zvrrp_cmd_config(gVrrpMatser);
}
int zvrrp_cmd_preempt(int type, int vrid)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;	
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	gVrrpMatser->opcode->vrid = vrid;
	if(type)
		gVrrpMatser->opcode->cmd = zvrrp_opcode_set_preempt;
	else	
		gVrrpMatser->opcode->cmd = zvrrp_opcode_unset_preempt;	
	return zvrrp_cmd_config(gVrrpMatser);
}
int zvrrp_cmd_preempt_delay(int type, int vrid, int value)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;	
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	gVrrpMatser->opcode->vrid = vrid;
	if(type)
		gVrrpMatser->opcode->cmd = zvrrp_opcode_set_preempt_delay;
	else	
		gVrrpMatser->opcode->cmd = zvrrp_opcode_unset_preempt_delay;
	gVrrpMatser->opcode->value = value;
	return zvrrp_cmd_config(gVrrpMatser);
}
int zvrrp_cmd_track(int type, int vrid, int id, char *ifname, int vlaue)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;	
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	gVrrpMatser->opcode->vrid = vrid;
	if(type)
		gVrrpMatser->opcode->cmd = zvrrp_opcode_set_track;
	else	
		gVrrpMatser->opcode->cmd = zvrrp_opcode_unset_track;
	//gVrrpMatser->opcode->ifindex = id;
	gVrrpMatser->opcode->value = vlaue;
	if(ifname)
		strcpy(gVrrpMatser->opcode->ifname, ifname);
	return zvrrp_cmd_config(gVrrpMatser);
}
int zvrrp_cmd_show(int vrid)
{
	if( (gVrrpMatser == NULL)||(gVrrpMatser->opcode == NULL) )
		return ERROR;	
	memset(gVrrpMatser->opcode, 0, sizeof(zvrrp_opcode));
	gVrrpMatser->opcode->vrid = vrid;
	gVrrpMatser->opcode->cmd = zvrrp_opcode_show;
	return zvrrp_cmd_config(gVrrpMatser);
}

