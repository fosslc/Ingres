/*
* (C)  Unpublished Copyright of Novell, Inc.  All Rights Reserved.  
*
*No part of this file may be duplicated, revised, translated, localized or
*modified in any manner or compiled, linked or uploaded or downloaded to or
*from any computer system without the prior written consent of Novell, Inc.
*
*WARNING:  Unauthorized reproduction of this program as well as unauthorized
*preparation of derivative works based upon the program or distribution of
*copies by sale, rental, lease or lending are violations of federal
*copyright laws and state trade secret laws, punishable by civil and
*criminal penalties.
*/
/*
 * 
 *  SCCS: @(#) common.h 1.6@(#)
 * 
 */

#ifdef CAPTURE
#define PRINT_MSG_BLK(MP) PrintMsgBlk(MP)
#else
#define PRINT_MSG_BLK(MP)
#endif

#define DATA_SIZE(mp)	((mp)->b_wptr - (mp)->b_rptr)
#define DATA_BSIZE(mp) ((mp)->b_datap->db_lim - (mp)->b_datap->db_base)
#define MTYPE(x) ((x)->b_datap->db_type)
#define BIGENOUGH(MP,SIZE) (((MP->b_datap->db_ref == 1) &&\
	(MP->b_datap->db_lim - MP->b_datap->db_base >= SIZE))?\
	MP->b_rptr = MP->b_wptr = MP->b_datap->db_base,1:0)

#ifndef STRLOG
#define STRLOG				strlog
#endif
#define PNW_ERROR            0
#define PNW_DROP_PACKET      1
#define PNW_ENTER_ROUTINE    2
#define PNW_EXIT_ROUTINE     3
#define PNW_ASSIGNMENT       4
#define PNW_SWITCH_CASE      5
#define PNW_SWITCH_DEFAULT   6
#define PNW_PROTO_HEX        7
#define PNW_DATA_HEX         8
#define PNW_DATA_ASCII       9
#define PNW_ALL             10
/*
* (C)  Unpublished Copyright of Novell, Inc.  All Rights Reserved.  
*
*No part of this file may be duplicated, revised, translated, localized or
*modified in any manner or compiled, linked or uploaded or downloaded to or
*from any computer system without the prior written consent of Novell, Inc.
*
*WARNING:  Unauthorized reproduction of this program as well as unauthorized
*preparation of derivative works based upon the program or distribution of
*copies by sale, rental, lease or lending are violations of federal
*copyright laws and state trade secret laws, punishable by civil and
*criminal penalties.
*/
/*
**	NetWare/SRC 
**
**	SCCS:	@(#) portable.h 1.2@(#)	
**
**		Header file to define those variables to ease portability of 
**		this product to various environments.
*/

/* Has this file already been defined? */
#ifndef	PORTABLE_HEADER
#define	PORTABLE_HEADER		/* nothing */

/*	Define machine type definitions. */
#ifdef	IAPX386
#define	LO_HI_MACH_TYPE
#endif

#ifdef	MC680X0
#define	HI_LO_MACH_TYPE
#endif

#ifdef	MC88000
#define	HI_LO_MACH_TYPE
#define	STRICT_ALIGNMENT
#endif

/*	Define general types. */
typedef	unsigned int		uint32;	/* 32-bit unsigned type */
typedef	unsigned short		uint16;	/* 16-bit unsigned type */
typedef	unsigned char		uint8;	/* 8-bit unsigned type */ 	

typedef	int					int32;	/* 32-bit signed type */
typedef	short				int16;	/* 16-bit signed type */
typedef char				int8;	/* 8-bit signed type */

/* Number of elements in array */
#define	DIM(a)	(sizeof(a)/sizeof(*(a)))

/* Scope control keywords */
#define	public				/* public is default in C */
#define	private	static		/* static really means private */

/* Standard constants */
#ifndef TRUE
#define	TRUE		1	
#endif
#ifndef FALSE
#define	FALSE		0
#endif

#define	FREE		0
#define	IN_USE		1

/*	Don't allow default allocation routines.  Make everyone use
 *	alternate routines found in the allocation library.
 *	By using the following declarations all links will return
 *	unsolved external errors thus informing the developer. */
/* the transport group is allowing the following calls */
/*
#define malloc(z)			MallocsNotAllowed(z)
#define free(z)				FreesNotAllowed(z)
#define calloc(y,z)			CallocsNotAllowed(y,z)
#define realloc(y,z)		ReallocsNotAllowed(y,z)
*/

/* Byte swap macros */
#ifdef LO_HI_MACH_TYPE	 
#define GETINT16(x)		(\
						((uint16)(x & 0x00FF) << 8) | \
						((uint16)(x & 0xFF00) >> 8) \
						)
#define PUTINT16(x)		GETINT16(x)

#define GETINT32(x)		(\
						((uint32)(x & 0x000000FF) << 24) | \
						((uint32)(x & 0x0000FF00) <<  8) | \
						((uint32)(x & 0x00FF0000) >>  8) | \
						((uint32)(x & 0xFF000000) >> 24) \
						) 
#define PUTINT32(x)		GETINT32(x)
#define REVGETINT16(x)		(x)
#define REVGETINT32(x)		(x)
#endif   /*  end of #ifdef LO_HI */

#ifdef HI_LO_MACH_TYPE   /* MOTOROLA 680X0 processor family */
#define GETINT16(x)			(x)
#define GETINT32(x)			(x)
#define PUTINT16(x)			(x)
#define PUTINT32(x)			(x)
#define	REVGETINT16(x)		(\
							((uint16)(x & 0x00FF) << 8) | \
							((uint16)(x & 0xFF00) >> 8) \
							)
#define REVGETINT32(x)		(\
							((uint32)(x & 0x000000FF) << 24) | \
							((uint32)(x & 0x0000FF00) <<  8) | \
							((uint32)(x & 0x00FF0000) >>  8) | \
							((uint32)(x & 0xFF000000) >> 24) \
							) 
#endif   /*  end of #ifdef HI_LO_MACH_TYPE */

/* Alignment macros */
#ifdef STRICT_ALIGNMENT
#define GETALIGN32(src, dest)	{ \
								register uint8 *sptr = (uint8 *) src; \
								register uint8 *dptr = (uint8 *) dest; \
								dptr[0] = sptr[0]; \
								dptr[1] = sptr[1]; \
								dptr[2] = sptr[2]; \
								dptr[3] = sptr[3]; \
								}
#define GETALIGN16(src, dest)	{ \
								register uint8 *sptr = (uint8 *) src; \
								register uint8 *dptr = (uint8 *) dest; \
								dptr[0] = sptr[0]; \
								dptr[1] = sptr[1]; \
								}
#else
#define GETALIGN32(src, dest) (*((uint32 *)dest) = *((uint32 *)src))
#define GETALIGN16(src, dest) (*((uint16 *)dest) = *((uint16 *)src))
#endif		/* end of STRICT_ALIGN */

#endif		/* end of #ifndef PORTABLE_HEADER */
/*
* (C)  Unpublished Copyright of Novell, Inc.  All Rights Reserved.  
*
*No part of this file may be duplicated, revised, translated, localized or
*modified in any manner or compiled, linked or uploaded or downloaded to or
*from any computer system without the prior written consent of Novell, Inc.
*
*WARNING:  Unauthorized reproduction of this program as well as unauthorized
*preparation of derivative works based upon the program or distribution of
*copies by sale, rental, lease or lending are violations of federal
*copyright laws and state trade secret laws, punishable by civil and
*criminal penalties.
*/
/*
 *
 *   SCCS:  @(#) ipx_app.h 1.26@(#)
 *
 *   The parts of IPX exposed to the application programmer.
 *
 */

#define IPX_SET_SAP 			( ( 'i' << 8 ) | 1 )
#define IPX_SET_SOCKET 			( ( 'i' << 8 ) | 2 )
#define IPX_SET_NET 			( ( 'i' << 8 ) | 3 )
#define IPX_GET_NODE_ADDR		( ( 'i' << 8 ) | 5 )
#define IPX_UNSET_SAP 			( ( 'i' << 8 ) | 6 )
#define IPX_RESET_ROUTER		( ( 'i' << 8 ) | 7 )
#define IPX_GET_ROUTER_TABLE	( ( 'i' << 8 ) | 8 )
#define IPX_SET_SPX 			( ( 'i' << 8 ) | 9 )
#define IPX_GET_NET 			( ( 'i' << 8 ) | 10 )
#define IPX_SET_LAN_INFO		( ( 'i' << 8 ) | 11 )
#define IPX_GET_LAN_INFO		( ( 'i' << 8 ) | 12 )
#define IPX_DOWN_ROUTER 		( ( 'i' << 8 ) | 13 )
#define IPX_GET_NET_LIST 		( ( 'i' << 8 ) | 14 )
#define IPX_CHECK_SAP_SOURCE 	( ( 'i' << 8 ) | 15 )

#define IPX_MAX_CONNECTED_LANS 16
#define IPX_MIN_CONNECTED_LANS 2

#define IPX_MAX_PACKET_DATA 546

#define IPX_HDR_SIZE		30
#define IPX_ADDR_SIZE		12
#define IPX_CHKSUM			0xFFFF
#define IPX_NET_SIZE		4
#define IPX_NODE_SIZE		6
#define IPX_SOCK_SIZE		2
#define IPXID 				362

/*
 * Structure defining an ipx address
 */
typedef struct ipxAddress {
	uint8  	net[IPX_NET_SIZE];		/* ipx network address */
	uint8 	node[IPX_NODE_SIZE];	/* ipx node address */
	uint8 	sock[IPX_SOCK_SIZE];	/* ipx socket */
} ipxAddr_t;

typedef struct ipxNet {
	uint8 net[IPX_NET_SIZE];	/* ipx network address */
} ipxNet_t;

typedef struct ipxNode {
	uint8 node[IPX_NODE_SIZE];	/* ipx node address */
} ipxNode_t;

typedef struct ipxSock {
	uint8 sock[IPX_SOCK_SIZE];	/* ipx socket */
} ipxSock_t;

/*
	this is the information about the lan adapter that ipx is 
	hooked to compare DL_info_ack.
*/

typedef struct ipxAdapterInfo_s {
	long PRIM_type; 
	long SDU_max;
	long SDU_min;
	long ADDR_length;
	long SUBNET_type;
	long SERV_class;
	long CURRENT_state;
	long GROWTH_field[2];
} ipxAdapterInfo_t;

typedef struct netInfo {
	uint32		lan;			/* This value returned by ioctl */
	uint32		state;			/* This value returned by ioctl */
	uint32		streamError;	/* This value returned by ioctl */
	uint32		network;		/* This value set by user */
	uint32		muxId;			/* This value set by user */
	uint8		nodeAddress[6];	/* This value set by user */
	ipxAdapterInfo_t adapInfo; /* This is given by the user */
} netInfo_t;

typedef struct netRouteEntry {
    struct netRouteEntry    *nextRouteLink;
    uint8                   forwardingRouter[6];
    uint8                   connectedLan;
    uint8                   timer;
    uint8                   routeStatus;
    uint8                   routeHops;
    uint16                  routeTime;
} netRouteEntry_t;

#define NET_ROUTE_ENTRY_SIZE 16

typedef struct netListEntry {
    struct netListEntry *nextNetLink;
    uint32              netIDNumber;
    uint8               hopsToNet;
    uint8               netStatus;
    int8                entryChanged;
    uint8               usedRouterModCount;
    int8                routerLostNetFlag;
    int8                mustAdvertise;
    int16               timeToNet;
    netRouteEntry_t     *routeListLink;
} netListEntry_t;

typedef struct checkSapSource_s {
	uint8 serverAddress[IPX_ADDR_SIZE];
	uint8 reporterAddress[IPX_NODE_SIZE];
	uint16 hops;
	uint32 connectedLan;
	int result; /* -1 not a good source  0 good source */
} checkSapSource_t;

#define NET_LIST_ENTRY_SIZE 20


#define IPX_TRANSPORT_CONTROL  	0x00
#define IPX_NULL_PACKET_TYPE	0x00
#define IPX_ECHO_PACKET_TYPE	0x02
#define IPX_PEP_PACKET_TYPE		0x04
#define IPX_SPX_PACKET_TYPE		0x05
#define IPX_NOVELL_PACKET_TYPE	0x11
#define IPX_WAN_PACKET_TYPE		0x14

/*
 * Structure defining an ipx header
 */
typedef struct ipxHeader{
	uint16		chksum;		/* checksum FFFF if not done */
	uint16		len;		/* length of data and ipx header */
	uint8		tc;			/* transport control */
	uint8		pt;			/* packet type */
	ipxAddr_t	dest;		/* destination address */
	ipxAddr_t	src;		/* source address */
} ipxHdr_t;

#define ROUTE_TABLE_SIZE 512

typedef struct routeInfo {
	uint32		net;
	uint16		hops;
	uint16		time;
	uint16		endOfTable;
	uint8		node[IPX_NODE_SIZE];
} routeInfo_t;

#define ROUTE_INFO_SIZE 16

/*
* (C)  Unpublished Copyright of Novell, Inc.  All Rights Reserved.  
*
*No part of this file may be duplicated, revised, translated, localized or
*modified in any manner or compiled, linked or uploaded or downloaded to or
*from any computer system without the prior written consent of Novell, Inc.
*
*WARNING:  Unauthorized reproduction of this program as well as unauthorized
*preparation of derivative works based upon the program or distribution of
*copies by sale, rental, lease or lending are violations of federal
*copyright laws and state trade secret laws, punishable by civil and
*criminal penalties.
*/
#define SPXID 392 
#define TLI_SPX_CONNECTION_FAILED ((uint8) 0xED)
#define TLI_SPX_CONNECTION_TERMINATED ((uint8) 0xEC)
#define TLI_SPX_MALFORMED_PACKET ((uint8) 0xFD)
#define SPX_GET_CONN_INFO  ( ( 's' << 8 ) | 1 ) 

typedef struct spx_optmgmt {
	uint8 spxo_retry_count;
	uint8 spxo_watchdog_flag;
	uint16 spxo_min_retry_delay;
} SPX_OPTMGMT;

typedef struct spxopt_s {
	unsigned char spx_connectionID[2];
	unsigned char spx_allocationNumber[2];
} SPX_OPTS;

typedef struct SpxHdr {
	ipxHdr_t ipxHdr;
	uint8 	connectionControl;
	uint8 	dataStreamType;
	uint16 	sourceConnectionId;
	uint16 	destinationConnectionId;
	uint16 	sequenceNumber;
	uint16 	acknowledgeNumber;
	uint16 	allocationNumber;
	} spxHdr_t;

typedef struct connectionEntry {
	spxHdr_t spxHdr;
	queue_t *upperReadQueue;
	mblk_t *inTransit;
	mblk_t *goingUp;
	uint8 needAck;
	long state;
	uint16 windowSize;
	uint16 tMaxRetries;
	uint16 tRetries;
	uint16 rRetries;
	uint32 tTicksToWait;
	uint32 rTicksToWait;
	uint32 beginTicks;
	uint32 endTicks;
	uint32 remoteActiveTicks;
	uint32 allocRetryCount;
	int tTimeOutId;
	int rTimeOutId;
	} spxConnectionEntry_t;

