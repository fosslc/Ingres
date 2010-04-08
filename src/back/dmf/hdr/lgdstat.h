/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: LGDSTATUS.H - Definitions of the Logging System LGD status values.
**
** Description:
**	This file contains the definitions of the Logging System status
**	values set into the lgd_status field of the Logging System LGD
**	control block.
**
**	These are defined here rather than in lgdef.h so that they can
**	be viewed by callers of LG_S_LGSTS.
**
** History: 
**	17-mar-1993 (rogerk)
**	    Created to allow dmfrcp to use the lgd_status constants rather
**	    than hard-wired bitmasks.
**	20-jun-1994 (jnash)
**        Eliminate unused LGD_COPY_STALL.
**	08-Oct-1996 (jenjo02)
**	    Added LGD_CLUSTER_SYSTEM.
**      01-may-2008 (horda03) Bug 120349
**          Add LGD_STATES define.
**/

#define		LGD_ONLINE          0x00000001
#define		LGD_CPNEEDED        0x00000002
#define		LGD_LOGFULL         0x00000004
#define		LGD_STALL_MASK      (LGD_LOGFULL | LGD_BCPSTALL)
#define		LGD_FORCE_ABORT     0x00000008
#define		LGD_RECOVER         0x00000010
#define		LGD_ARCHIVE         0x00000020
#define		LGD_ACP_SHUTDOWN    0x00000040
#define		LGD_IMM_SHUTDOWN    0x00000080
#define		LGD_START_ARCHIVER  0x00000100
#define		LGD_PURGEDB         0x00000200
#define		LGD_OPENDB          0x00000400
#define		LGD_CLOSEDB         0x00000800
#define		LGD_START_SHUTDOWN  0x00001000
#define		LGD_BCPDONE         0x00002000
#define		LGD_CPFLUSH         0x00004000
#define		LGD_ECP             0x00008000
#define		LGD_ECPDONE         0x00010000
#define		LGD_CPWAKEUP        0x00020000
#define		LGD_BCPSTALL        0x00040000
#define		LGD_CKP_SBACKUP     0x00080000
#define		LGD_MAN_ABORT       0x00400000
#define		LGD_MAN_COMMIT      0x00800000
#define		LGD_RCP_RECOVER     0x01000000
#define		LGD_JSWITCH         0x02000000
#define		LGD_JSWITCHDONE     0x04000000
#define		LGD_DUAL_LOGGING    0x08000000
#define		LGD_DISABLE_DUAL_LOGGING	0x10000000
#define		LGD_EMER_SHUTDOWN   0x20000000
#define		LGD_CLUSTER_SYSTEM  0x80000000

#define LGD_STATES "\
ONLINE,CPNEEDED,LOGFULL,FORCE_ABORT,RECOVER,\
ARCHIVE,ACP_SHUTDOWN,IMM_SHUTDOWN,START_ARCHIVER,PURGEDB,\
OPEN_DB,CLOSE_DB,START_SHUTDOWN,BCPDONE,CPFLUSH,ECP,ECPDONE,\
CPWAKEUP,BCPSTALL,CKP_SBACKUP,0x00100000,0x00200000,MAN_ABORT,MAN_COMMIT,RCP_RECOVER,\
JSWITCH,JSWITCHDONE,DUAL_LOGGING,DISABLE_DUAL_LOGGING,EMER_SHUTDOWN,\
0x40000000,CLUSTER"
