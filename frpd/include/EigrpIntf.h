#ifndef	_EIGRP_INTF_H_
#define	_EIGRP_INTF_H_

#ifdef __cplusplus
extern"C"
{
#endif

/* Start Edit By  : zhurish : 2016/01/18 22:3:47  : :修改操作:.重新定义接口状态标识 */
#ifndef _EIGRP_PLAT_MODULE
#define	EIGRP_INTF_FLAG_ACTIVE		0x00000001
#define	EIGRP_INTF_FLAG_BROADCAST	0x00000002
#define	EIGRP_INTF_FLAG_MULTICAST	0x00000004
#define	EIGRP_INTF_FLAG_LOOPBACK	0x00000008
#define	EIGRP_INTF_FLAG_POINT2POINT	0x00000010
#define	EIGRP_INTF_FLAG_EIGRPED		0x00000020
#else//_EIGRP_PLAT_MODULE
#define	EIGRP_INTF_FLAG_ACTIVE		IFF_UP			/*0x00000001	interface link is up */
#define	EIGRP_INTF_FLAG_BROADCAST	IFF_BROADCAST	/*0x00000002	broadcast address valid */
#define	EIGRP_INTF_FLAG_DEBUG		IFF_DEBUG		/*0x00000004	turn on debugging */
#define	EIGRP_INTF_FLAG_LOOPBACK	IFF_LOOPBACK	/*0x00000008	is a loopback net */
#define	EIGRP_INTF_FLAG_POINT2POINT	IFF_POINTOPOINT	/*0x00000010	interface is point-to-point link */
#define	EIGRP_INTF_FLAG_RUNNING		IFF_RUNNING		/*0x00000040	resources allocated */
#define	EIGRP_INTF_FLAG_NOARP		IFF_NOARP		/*0x00000080	no address resolution protocol */
#define	EIGRP_INTF_FLAG_PROMISC		IFF_PROMISC		/*0x00000100	receive all packets */
#define	EIGRP_INTF_FLAG_ALLMULTI	IFF_ALLMULTI	/*0x00000200	receive all multicast packets */
#define	EIGRP_INTF_FLAG_OACTIVE		IFF_OACTIVE		/*0x00000400	transmission in progress */
#define	EIGRP_INTF_FLAG_SIMPLEX		IFF_SIMPLEX		/*0x00000800	can't hear own transmissions */
#define	EIGRP_INTF_FLAG_LINK0		IFF_LINK0		/*0x00001000	forwarding disabled */
#define	EIGRP_INTF_FLAG_LINK1		IFF_LINK1		/*0x00002000	per link layer defined bit */
#define	EIGRP_INTF_FLAG_LINK2		IFF_LINK2		/*0x00004000	per link layer defined bit */
#define	EIGRP_INTF_FLAG_ALTPHYS		IFF_ALTPHYS		/*IFF_LINK2	 	use alternate physical connection */
#define	EIGRP_INTF_FLAG_MULTICAST	IFF_MULTICAST	/*0x00008000	supports multicast */
#define EIGRP_INTF_FLAG_NOTRAILERS	IFF_NOTRAILERS	/*0x00020000	avoid use of trailers */
#define	EIGRP_INTF_FLAG_EIGRPED		0x00000020
#endif// _EIGRP_PLAT_MODULE
/* End Edit By  : zhurish : 2016/01/18 22:3:47  */


typedef	struct EigrpIntf_{
	struct	EigrpIntf_	*next;

	S8	name[128];	/* Interface name. */
	U32	ifindex;

	U32	flags;	

	S32	metric;
	S32	mtu;
	U32	bandwidth;	/* In unit of kbps */
	U32	delay;	/*hanbing add 120707*/

	U32	ipAddr;
	U32	ipMask;

	U32	remoteIpAddr;

#ifdef _EIGRP_PLAT_MODULE
	/*
	 *  标注这个接口地址是否启动路由运算 
	 */
	U32	active;
#define EIGRP_INTF_FLAG_UNNUMBERED	0X00000002	
	/* 
	* connected:
	*  接口地址链表，一般情况使用较少，因为路由协议允许接口
	*  第一个地址允许路由协议，后面的地址就不允许运行路由协议
	* EigrpPortGetNextAddr
	*/
//	EigrpIntfAddr_st connected;
#endif// _EIGRP_PLAT_MODULE	
	/* 
	* 没有使用
	*/
	void	*sysIntf;
	/* 
	* sysCirc:
	* 指向接口私有数据（系统接口表元素等等）
	* 在有路由平台支持的环境下，该指针指向zebra 的struct interface  节点
	* 因此在没有路由平台支持的环境下应该执行FRP路由构造的接口链表节点
	*/
	void	*sysCirc;

	struct EigrpIdbLink_	*private;
#ifdef INCLUDE_SATELLITE_RESTRICT
	EigrpPktRecord_st		lastSentHello;
	U32		sentPeer[64];
/*	U32		delay;*/
#endif
	S32	updateFinished;
}EigrpIntf_st, *EigrpIntf_pt;

typedef struct EigrpConn_{
	struct EigrpIntf_	*ifp;

	U32	ipAddr;
	U32	ipMask;
}EigrpConn_st, *EigrpConn_pt;

#define	EIGRP_IN_MULTICAST(a)		((((U32)(a)) & 0xf0000000) == 0xe0000000)

void	EigrpIntfClean();
void	EigrpIntfInit();
EigrpIntf_pt	EigrpIntfFindByName(char *);
/* Start Edit By  : zhurish : 2016/01/18 22:5:9  : :
修改操作:.屏蔽调试函数，把这个函数的实现放置在EigrpZebra.c文件 ，
删除原来这个在EigrpIntf.c 文件实现的测试函数*/
//U32 EigrpIntfShow();
/* End Edit By  : zhurish : 2016/01/18 22:5:9  */
EigrpIntf_pt	EigrpIntfFindByIndex(U32);
EigrpIntf_pt	EigrpIntfFindByAddr(U32);
EigrpIntf_pt	EigrpIntfFindByPeer(U32);
EigrpIntf_pt	EigrpIntfFindByNetAndMask(U32, U32);
void	EigrpIntfDel(EigrpIntf_pt);
void	EigrpIntfAdd(EigrpIntf_pt);

void	EigrpSysIntfAdd_ex(void *);
void	EigrpSysIntfDel_ex(void *);
void	EigrpSysIntfDel2_ex(U32);
void	EigrpSysIntfUp_ex(void *);
void	EigrpSysIntfDown_ex(void *);
void	EigrpSysIntfAddrAdd_ex(U32, U32, U32, U8);
void	EigrpSysIntfAddrDel_ex(U32, U32, U32);

#ifdef _EIGRP_PLAT_MODULE
extern int EigrpIntfActiveByNetAndMask(U32 net, U32 ipMask, int type);
#define EIGRP_ROUTE_ACTIVE(_a,_m)	EigrpIntfActiveByNetAndMask((_a),(_m), TRUE)
#define EIGRP_ROUTE_INACTIVE(_a,_m)	EigrpIntfActiveByNetAndMask((_a),(_m), FALSE)

extern int  zebraEigrpCmdEvent(EigrpCmd_pt pCmd);
extern S32	zebraEigrpCmdInterface(S32 noFlag, U32 asNum, EigrpIntf_st *sysif);
extern S32	zebraEigrpCmdInterfaceState(S32 noFlag, U32 asNum, EigrpIntf_st *sysif);
extern S32	zebraEigrpCmdInterfaceAddress(S32 noFlag, U32 asNum, S32 index, U32 ipAddr, U32 ipMask, U8 resetIdb);
extern S32	zebraEigrpCmdRoute(S32 noFlag, U32 asNum, int proto, long ip, long mask, long next, int metric);
#endif //_EIGRP_PLAT_MODULE

#ifdef __cplusplus
}
#endif

#endif	/* _EIGRP_INTF_H_ */
