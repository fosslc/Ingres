/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: GCCER.H - Header file for GCF Communication Server status codes
**
** Description:
**      This header file contains all mainline status codes for all layers 
**      of the GCF Communication Server.
**
** History: 
**      15-Jul-88 (jbowers)
**          Initial module creation
**      08-Mar-89 (jbowers)
**          Modified to support selective level-based logging capability.  Each
**	    status value is modified to carry a level indicator in the upper
**	    byte, to be used by the error logging facility for selectivity.
**	12-MAY-89 (pasker) v1.001
**	    Added new error messages to descriminate between different
**	    kinds of PL problems.
**      01-Aug-89 (cmorris)
**          Added error for GCA request response failure.
**      08-Nov-89 (cmorris)
**          Added error for GCA disassociate failure.
**      02-Feb-90 (cmorris)
**          Use defines instead of hard-coded constants for log masks
**          and levels. (Partially an NSE porting change)
**	05-Feb-90 (seiwald)
**	    New E_GC2219_GCA_REGISTER_FAIL.
**	06-Feb-90 (seiwald)
**	    New E_GC2815_NTWK_OPEN.
**	20-Jun-90 (seiwald)
**	    New E_GC2816_NOT_OPEN.
**      10-jan-91 (stevet)
**          New E_GC2009_CHAR_INIT.
**	16-Apr-91 (seiwald)
**	    New E_GC2818_PROT_ID for invalid names in II_GCC_PROTOCOLS.
**	16-Apr-91 (seiwald)
**	    New E_GC2819_NTWK_CONNECTION_FAIL to be returned to user
**	    for connection initiation failure rather than the generic
**	    E_GC280D_NTWK_ERROR.
**	20-May-91 (seiwald)
**	    New errors for character set negotiation.
**	16-Jul-91 (cmorris)
**	    New errors for protocol bridge.
**  	19-Dec-91 (cmorris)
**  	    Added E_GC2A08 for protocol bridge.
**  	10-Apr-92 (cmorris)
**  	    Added E_GC2A09 for protocol bridge.
**  	28-Sep-92 (brucek)
**  	    ANSI C fix: define GCLVL_MASK directly, rather than as
**	    arithmetic expression (which would overflow an int).
**	09-mar-93 (walt)
**	    Removed the leading zero from '08' and '09' in GCLVL_8 and
**	    GCLVL_9 to silence compiler complaints about illegal octal
**	    numbers.
**	24-mar-95 (forky01)
**	    Added E_GC2414_PL_COMP_MSGS_EXCEED for bug 67678.
**	06-Feb-96 (rajus01)
**	    Added  E_GC2A0A_PB_NO_VNODE, E_GC2A0B_PB_GCN_INITIATE, 
**          E_GC2A0C_PB_GCN_RET_PRIV,  E_GC2A0D_PB_GCN_RET_PUB, 
**          E_GC2A0E_PB_GCN_TERMINATE for Protocol Bridge.
**	27-Feb-96 (rajus01)
**	    Added E_GC2A0F_NO_AUTH_BRIDGE.
**	30-Jun-97 (rajus01)
**	    Added GCS messages E_GC1???.
**	20-Aug-97 (gordy)
**	    Convert E_GC1013, 1014, 1015 to E_GC281A, 281B, 281C, 281D.
**	    Added E_GC200A.
**	09-jun-1999 (somsa01)
**	    Added E_GC2A10_LOAD_CONFIG.
**	25-Sept-2004 (wansh01) 
**		Added GCC/ADMIN error messages : E_GC200B, 200C, 200D, 200E.
**/


/*
**  GCC STATUS values: these are the valid values for the generic_status
**  element throughout GCC.
**
**  There are reserved ranges of STATUS values
**
**	0x0000 ... 0x0FFF	GCA STATUS values
**	0x1000 ... 0x13FF	GCN STATUS values
**	0x2000 ... 0x2FFF	GCC STATUS values
**
**  Within the range reserved for GCC, the following sub-range assignments
**  currently exist:
**
**	0x2000 ... 0x21FF	GCC global status values
**	0x2200 ... 0x23FF	GCC AL global status values
**	0x2400 ... 0x25FF	GCC PL global status values
**	0x2600 ... 0x27FF	GCC SL global status values
**	0x2800 ... 0x29FF	GCC TL global status values
**	0x2A00 ... 0x2BFF	GCB global status values
**	
*/

/*
**  Following are facility, sub-facility and layer mask definitions.
*/

# ifndef	E_GCF_MASK
# define	E_GCF_MASK		(DB_GCF_ID * 0x10000)
# endif

# define	E_GCC_MASK		E_GCF_MASK + 0x2000

# define	E_GCCAL_MASK		E_GCC_MASK + 0x0200
# define	E_GCCPL_MASK		E_GCC_MASK + 0x0400
# define	E_GCCSL_MASK		E_GCC_MASK + 0x0600
# define	E_GCCTL_MASK		E_GCC_MASK + 0x0800

# define	E_GCB_MASK		E_GCC_MASK + 0x0A00

/*
** The following are the message level definitions used to tag each status code
** with a "severity" level.  This level is carried in the upper byte of the
** status code.
*/

#define		GCLVL_BIT		0x01000000
#define		GCLVL_MASK              0xFF000000

# define	GCLVL_0			00 * GCLVL_BIT
# define	GCLVL_1			01 * GCLVL_BIT
# define	GCLVL_2			02 * GCLVL_BIT
# define	GCLVL_3			03 * GCLVL_BIT
# define	GCLVL_4			04 * GCLVL_BIT
# define	GCLVL_5			05 * GCLVL_BIT
# define	GCLVL_6			06 * GCLVL_BIT
# define	GCLVL_7			07 * GCLVL_BIT
# define	GCLVL_8			 8 * GCLVL_BIT
# define	GCLVL_9			 9 * GCLVL_BIT
# define	GCLVL_10		10 * GCLVL_BIT
# define	GCLVL_11		11 * GCLVL_BIT
# define	GCLVL_12		12 * GCLVL_BIT
# define	GCLVL_13		13 * GCLVL_BIT
# define	GCLVL_14		14 * GCLVL_BIT
# define	GCLVL_15		15 * GCLVL_BIT

/* Macro to get rid of the log level */

# define 	GCC_LGLVL_STRIP_MACRO(error) (error & 0xFFFFFF)

/*
** Following are the default message level values defining the levels to be
** logged to the public and private logs.
*/

# define	GCC_LGLVL_DEFAULT	4
# define	GCC_ERLVL_DEFAULT	4

/*
** Global Communication Server (GCC) Messages 
*/

/* Communication Server normal startup. */
# define    E_GC2001_STARTUP	    (STATUS)(GCLVL_1 + E_GCC_MASK + 0x0001)
/* Communication Server normal shutdown. */
# define    E_GC2002_SHUTDOWN	    (STATUS)(GCLVL_1 + E_GCC_MASK + 0x0002)
/* A Communication Server startup parameter was incorrectly specified. */
# define    E_GC2003_STARTUP_PARM   (STATUS)(GCLVL_4 + E_GCC_MASK + 0x0003)
/* Memory allocation failure in Communication Server. */
# define    E_GC2004_ALLOCN_FAIL    (STATUS)(GCLVL_4 + E_GCC_MASK + 0x0004)
/* Communication Server initialization failed. */
# define    E_GC2005_INIT_FAIL	    (STATUS)(GCLVL_1 + E_GCC_MASK + 0x0005)
/* Communication Server normal startup: server rev. level %0c. */
# define    E_GC2006_STARTUP	    (STATUS)(GCLVL_1 + E_GCC_MASK + 0x0006)
/* Communication Server startup failure: No authorization for INGRES/NET */
# define    E_GC2007_NO_AUTH_INET   (STATUS)(GCLVL_1 + E_GCC_MASK + 0x0007)
/* Comm Server memory shortage: unable to allocate object type %0c. */
# define    E_GC2008_OUT_OF_MEMORY  (STATUS)(GCLVL_1 + E_GCC_MASK + 0x0008)
/* Error while processing character set attribute file. */
# define    E_GC2009_CHAR_INIT      (STATUS)(GCLVL_1 + E_GCC_MASK + 0x0009)
/* Invalid encryption mode, '%0c', specified in config file or VNODE entry. */
# define    E_GC200A_ENCRYPT_MODE   (STATUS)(GCLVL_1 + E_GCC_MASK + 0x000A)
/* admin session error  */
# define    E_GC200B_ADM_SESS_ERR (STATUS)(GCLVL_1 + E_GCC_MASK + 0x000B)
# define    E_GC200C_ADM_INIT_ERR (STATUS)(GCLVL_1 + E_GCC_MASK + 0x000C)
# define    E_GC200D_ADM_SHUT     (STATUS)(GCLVL_1 + E_GCC_MASK + 0x000D)
# define    E_GC200E_ADM_SESS_ABT (STATUS)(GCLVL_1 + E_GCC_MASK + 0x000E)

/*
** Communication Server Application Layer (GCCA) Messages
*/

/* Normal local association termination: session end. */
# define    E_GC2201_ASSOC_END	    (STATUS) (GCLVL_6 + E_GCCAL_MASK + 0x0001)
/* AL internal error: Invalid input event in AL FSM */
# define    E_GC2202_AL_FSM_INP	    (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0002)
/* AL internal error: Invalid state transition in AL FSM */
# define    E_GC2203_AL_FSM_STATE   (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0003)
/* Normal local association initiation: session start. */
# define    E_GC2204_ASSOC_START    (STATUS) (GCLVL_6 + E_GCCAL_MASK + 0x0004)
/* Session failure: ABORT received from remote partner. */
# define    E_GC2205_RMT_ABORT	    (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0005)
/* Session failure: association with local partner failed. */
# define    E_GC2206_LCL_ASSOC_FAIL (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0006)
/* Session failure: remote association initiation failed, status follows. */
# define    E_GC2207_RMT_INIT_FAIL  (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0007)
/* Session failure: releasing local association. */
# define    E_GC2208_CONNCTN_ABORT  (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0008)
/* GCA send failure: reason follows. */
# define    E_GC2209_SND_FAIL	    (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0009)
/* GCA normal receive failure: reason follows. */
# define    E_GC220A_NRM_RCV_FAIL   (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x000A)
/* GCA expedited receive failure: reason follows. */
# define    E_GC220B_EXP_RCV_FAIL   (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x000B)
/* GCC AL initialization failure. */
# define    E_GC220C_AL_INIT_FAIL   (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x000C)
/* GCC AL received an invalid (null) password from a remote client. */
# define    E_GC220D_AL_PASSWD_FAIL (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x000D)
/* AL internal error: Invalid state transition in AL FSM.\ */
/*  Input = %0d, state = %1d */
# define    E_GC220E_AL_FSM_STATE   (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x000E)
/* GCC Comm Server AL internal error. Please see system admimistrator. */
# define    E_GC220F_AL_INTERNAL_ERROR	\
				    (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x000F)
/* Session failure: Local ABORT condition. */
# define    E_GC2210_LCL_ABORT	    (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0010)
/* Incoming association initiation from local user %0c, remote alias %1c. */
# define    E_GC2211_INCMG_ASSOC    (STATUS) (GCLVL_6 + E_GCCAL_MASK + 0x0011)
/* Outgoing local association initiation for remote user %0c. */
# define    E_GC2212_OUTGNG_ASSOC   (STATUS) (GCLVL_6 + E_GCCAL_MASK + 0x0012)
/* GCC internal error: CCB for connection %0d cannot be found. */
# define    E_GC2213_AL_NO_CCB	    (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0013)
/* GCC server has exceeded max inbound connections. */
# define    E_GC2214_MAX_IB_CONNS   (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0014)
/* GCC server has exceeded max outbound connections. */
# define    E_GC2215_MAX_OB_CONNS   (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0015)
/* GCC AL processing of a GCA_LISTEN completion has failed. */
# define    E_GC2216_GCA_LSN_FAIL   (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0016)
/* GCC AL processing of a GCA_RQRESP completion has failed. */
# define    E_GC2217_GCA_RQRESP_FAIL (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0017)
/* GCC AL processing of a GCA_DISASSOC completion has failed. */
# define    E_GC2218_GCA_DISASSOC_FAIL (STATUS) (GCLVL_4 + E_GCCAL_MASK + 0x0018)
/* GCA registration with Name Server failed: reason follows. */
# define    E_GC2219_GCA_REGISTER_FAIL (STATUS)(GCLVL_4 + E_GCCAL_MASK + 0x0019)

/* Partner passed an invalid security label */
# define    E_GC221A_AL_SECLABEL_TYPE (STATUS)(GCLVL_4 + E_GCCAL_MASK + 0x001A)
/* server is closed and not accepting any new connection  */
# define    E_GC221B_AL_SVR_CLSD      (STATUS)(GCLVL_4 + E_GCCAL_MASK + 0x001B)

/*
** Communication Server Presentation Layer (GCCP) Messages
*/

/* Normal Presentation Connection termination. */
# define    E_GC2401_ASSOC_END	    (STATUS) (GCLVL_6 + E_GCCPL_MASK + 0x0001)
/* PL internal error: Invalid input event in PL FSM */
# define    E_GC2402_PL_FSM_INP	    (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0002)
/* PL internal error: Invalid state transition in PL FSM */
# define    E_GC2403_PL_FSM_STATE   (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0003)
/* PL internal error: Invalid state transition in PL FSM.\ */
/*  Input = %0d, state = %1d */
# define    E_GC2404_PL_FSM_STATE   (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0004)
/* GCC Comm Server PL internal error. Please see sysetm admimistrator. */
# define    E_GC2405_PL_INTERNAL_ERROR\
				    (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0005)
/*
** New PERFORM_CONVERSION internal PL errors
*/
/* P_CONNECT response result not ACCEPT or REJECT */
#define E_GC2406_IE_BAD_PCON_RSP_RSLT (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0006)
/* P_CONNECT type not REQUEST or RESPONSE */
#define E_GC2407_IE_BAD_PCON_TYPE     (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0007)
/* P_RELEASE type not REQUEST or RESPONSE */
#define E_GC2408_IE_BAD_PRSL_TYPE     (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0008)
/* S_CONNECT confirm result not ACCEPT or REJECT */
#define E_GC2409_IE_BAD_SCON_RSP_RSLT (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0009)
/* S_CONNECT type not REQUEST or RESPONSE */
#define E_GC240A_IE_BAD_SCON_TYPE     (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x000A)
/* S_RELEASE confirm result not AFFIRMATIVE or NEGATIVE */
#define E_GC240B_IE_BAD_SREL_CNF_RSLT (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x000B)
/* S_RELEASE type not INDICATION or CONFIRM */
#define E_GC240C_IE_BAD_SREL_TYPE     (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x000C)
/* S_U_ABORT PPDU_ID not ARP or ARU */
#define E_GC240D_IE_BAD_SUAB_PDU      (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x000D)
/* Bad INPUT event */
#define E_GC240E_IE_BAD		      (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x000E)
/* INIT_DOC_STACK Failed */
#define E_GC240F_INIT_DOC_STACK	      (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x000F)
/* PERFORM_CONVERSION Failed */
#define E_GC2410_PERF_CONV	      (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0010)
/* OD_PTR is NULL in INIT_DOC_STACK */
#define E_GC2411_OD_PTR_NULL	      (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0011)
/* DOC_EL is NULL in PERFORM_CONVERSION */
#define E_GC2412_DOC_EL_NULL	      (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0012)
/* GET_NEXT_ATOM failed in PERFORM_CONVERSION */
#define E_GC2413_GET_NEXT_ATOM	      (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0013)
/* GCC_PROGRAMS exceeded in GCC_COMP_MSGS */
#define E_GC2414_PL_COMP_MSGS_EXCEED  (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0014)

/*
** Character set negotiation problems
*/

/* Character set initialization failed. */
#define E_GC2451_CSET_FAIL	      (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0051)
/* Errors found in GCCCSET.NAM file */
#define E_GC2452_BAD_CNAM	      (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0052)
/* Errors found in GCCCSET.XLT file */
#define E_GC2453_BAD_CXLT	      (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0053)
/* II_GCC_CHARSET names a character set not found in GCCCSET.NAM */
#define E_GC2454_BAD_LCL_CHARSET      (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0054)
/* GCCCSET.XLT has no mapping from the local character set to NSCS. */
#define E_GC2455_NO_LCL_MAPPING       (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0055)

/* Character set negotiation complete: local %0c, remote %1c." */
#define E_GC2461_CHARSET       	      (STATUS) (GCLVL_6 + E_GCCPL_MASK + 0x0061)
/* Character set negotiation failed: no common character set found. */
#define E_GC2462_NO_CHARSET           (STATUS) (GCLVL_4 + E_GCCPL_MASK + 0x0062)

/*
** Communication Server Session Layer (GCCS) Messages
*/

/* Normal Session Connection termination. */
# define    E_GC2601_ASSOC_END	    (STATUS) (GCLVL_6 + E_GCCSL_MASK + 0x0001)
/* SL internal error: Invalid input event in SL FSM */
# define    E_GC2602_SL_FSM_INP	    (STATUS) (GCLVL_4 + E_GCCSL_MASK + 0x0002)
/* SL internal error: Invalid state transition in SL FSM */
# define    E_GC2603_SL_FSM_STATE   (STATUS) (GCLVL_4 + E_GCCSL_MASK + 0x0003)
/* SL internal error: Invalid state transition in SL FSM.\ */
/*  Input = %0d, state = %1d */
# define    E_GC2604_SL_FSM_STATE   (STATUS) (GCLVL_4 + E_GCCSL_MASK + 0x0004)
/* GCC Comm Server SL internal error. Please see sysetm admimistrator. */
# define    E_GC2605_SL_INTERNAL_ERROR\
				    (STATUS) (GCLVL_4 + E_GCCSL_MASK + 0x0005)

/*
** Communication Server Transport Layer (GCCT) Messages
*/

/* Normal Network Connection initiation. */
# define    E_GC2801_NTWK_CONNCTN_START\
				    (STATUS) (GCLVL_6 + E_GCCTL_MASK + 0x0001)
/* Normal Network Connection termination. */
# define    E_GC2802_NTWK_CONNCTN_END\
				    (STATUS) (GCLVL_6 + E_GCCTL_MASK + 0x0002)
/* Invalid (unknown) protocol identifier. */
# define    E_GC2803_PROT_ID	    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0003)
# define    E_GC2804_PROT_ID	    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0004)
/* GCC Comm Server TL internal error. Please see sysetm administrator. */
# define    E_GC2805_TL_INTERNAL_ERROR\
				    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0005)
/* TL internal error: Invalid input event in TL FSM */
# define    E_GC2806_TL_FSM_INP	    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0006)
/* TL internal error: Invalid state transition in TL FSM */
# define    E_GC2807_TL_FSM_STATE   (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0007)
/* Network open failed for protocol %0c, listen port %1c; status follows. */
# define    E_GC2808_NTWK_OPEN_FAIL (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0008)
/* Network connection failed for protocol %0c, to node %1c, port %2c;\
 status follows. */
# define    E_GC2809_NTWK_CONNECTION_FAIL\
				    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0009)
/* Network initilization failure: all protocol opens failed */
# define    E_GC280A_NTWK_INIT_FAIL (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x000A)
/* TL initialization failure. */
# define    E_GC280B_TL_INIT_FAIL   (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x000B)
/* TL internal error: Invalid state transition in TL FSM.\ */
/*  Input = %0d, state = %1d */
# define    E_GC280C_TL_FSM_STATE   (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x000C)
/* Network operation error: See Comm. Server log for detail. */
# define    E_GC280D_NTWK_ERROR	    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x000D)
/* GCC Comm Server TL internal error. Please see sysetm admimistrator. */
# define    E_GC280E_TL_INTERNAL_ERROR\
				    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x000E)
/* Outgoing network connection: remote connection id = %0d. */
# define    E_GC280F_OUTGNG_NTWK_CONNCTN\
				    (STATUS) (GCLVL_6 + E_GCCTL_MASK + 0x000F)
/* Incoming network connection: remote connection id = %0d. */
# define    E_GC2810_INCMG_NTWK_CONNCTN\
				    (STATUS) (GCLVL_6 + E_GCCTL_MASK + 0x0010)
/* TL internal error: Received invalid TPDU type %0x */
# define    E_GC2811_TL_INVALID_TPDU\
				    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0011)
/* TL internal error: Received unknown connection id %0d from partner. */
# define    E_GC2812_TL_INVALID_CONNCTN\
				    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0012)
/* Network listen failed for protocol %0c, listen port %1c; status follows. */
# define    E_GC2813_NTWK_LSN_FAIL  (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0013)

/* Invalid network receive: received data count = %0d. */
# define    E_GC2814_INVALID_RCV    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0014)

/* Network open complete for protocol %0c, listen port %1c. */
# define    E_GC2815_NTWK_OPEN	    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0015)

/* Network connection not allowed on protocol %0c where listen had failed. */
# define    E_GC2816_NOT_OPEN	    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0016)

/* Network connection not allowed on a protocol where listen had failed. */
# define    E_GC2817_NOT_OPEN	    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0017)

/* Invalid (unknown) protocol(s) listed in II_GCC_PROTOCOLS... */
# define    E_GC2818_PROT_ID	    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0018)

/* Network connection initiation failed: See Comm. Server log for detail. */
# define    E_GC2819_NTWK_CONNECTION_FAIL\
				    (STATUS) (GCLVL_4 + E_GCCTL_MASK + 0x0019)

/* TL internal error: received invalid data format for TPDU type %0x. */
# define    E_GC281A_TL_INV_TPDU_DATA\
				(STATUS)(GCLVL_4 + E_GCCTL_MASK + 0x001A)

/* Error initializing security mechanism %0d for encryption. */
# define    E_GC281B_TL_INIT_ENCRYPT\
				(STATUS)(GCLVL_4 + E_GCCTL_MASK + 0x001B)

/* Unable to establish data encryption for session. */
# define    E_GC281C_TL_NO_ENCRYPTION\
				(STATUS)(GCLVL_4 + E_GCCTL_MASK + 0x001C)

/* Data encryption/decryption failure. */
# define    E_GC281D_TL_ENCRYPTION_FAIL\
				(STATUS)(GCLVL_4 + E_GCCTL_MASK + 0x001D)


/*
** Protocol Bridge Messages
*/

/* Protocol Bridge initialization failed. */
# define    E_GC2A01_INIT_FAIL	    (STATUS)(GCLVL_1 + E_GCB_MASK + 0x0001)
/* Protocol Bridge startup failure: No authorization for INGRES/NET */
# define    E_GC2A02_NO_AUTH_INET   (STATUS)(GCLVL_1 + E_GCB_MASK + 0x0002)
/* Protocol Bridge normal startup: server rev. level %0c. */
# define    E_GC2A03_STARTUP	    (STATUS)(GCLVL_1 + E_GCB_MASK + 0x0003)
/* Protocol Bridge normal shutdown. */
# define    E_GC2A04_SHUTDOWN	    (STATUS)(GCLVL_1 + E_GCB_MASK + 0x0004)
/* Protocol Bridge internal error. Please see sysetm admimistrator. */
# define    E_GC2A05_PB_INTERNAL_ERROR\
				    (STATUS) (GCLVL_4 + E_GCB_MASK + 0x0005)
/* PB internal error: Invalid state transition in PB FSM.\ */
/*  Input = %0d, state = %1d */
# define    E_GC2A06_PB_FSM_STATE   (STATUS) (GCLVL_4 + E_GCB_MASK + 0x0006)
/* PB internal error: Invalid input event in PB FSM */
# define    E_GC2A07_PB_FSM_INP	    (STATUS) (GCLVL_4 + E_GCB_MASK + 0x0007)
/* Network Initialization failure: protocol open(s) failed */
# define    E_GC2A08_NTWK_INIT_FAIL (STATUS) (GCLVL_4 + E_GCB_MASK + 0x0008)
/* GCC Protocol Bridge has exceeded max connections */
#define     E_GC2A09_MAX_PB_CONNS   (STATUS) (GCLVL_4 + E_GCB_MASK + 0x0009)
/* GCC Protocol Bridge has no vnode info available */
#define E_GC2A0A_PB_NO_VNODE          (STATUS) (GCLVL_4 + E_GCB_MASK + 0x000A) 
/* GCC Protocol Bridge has failed for GCN_INITIATE call */
#define E_GC2A0B_PB_GCN_INITIATE       (STATUS) (GCLVL_4 + E_GCB_MASK + 0x000B)
/* GCC Protocol Bridge has failed for GCN_RET call to private entries. */
#define E_GC2A0C_PB_GCN_RET_PRIV       (STATUS) (GCLVL_4 + E_GCB_MASK + 0x000C)
/* GCC Protocol Bridge has failed for GCN_RET call to public entries. */
#define E_GC2A0D_PB_GCN_RET_PUB        (STATUS) (GCLVL_4 + E_GCB_MASK + 0x000D)
/* GCC Protocol Bridge has failed for GCN_TERMINATE call.*/
#define E_GC2A0E_PB_GCN_TERMINATE      (STATUS) (GCLVL_4 + E_GCB_MASK + 0x000E)
/* GCC Protocol Bridge has failed for AUTHENTICATION.*/
#define E_GC2A0F_NO_AUTH_BRIDGE      (STATUS) (GCLVL_4 + E_GCB_MASK + 0x000F)
/* Configuration parameters loaded for given configuration name.*/
#define E_GC2A10_LOAD_CONFIG      (STATUS) (GCLVL_4 + E_GCB_MASK + 0x0010)
