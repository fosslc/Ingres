/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/**
** Name: GCC.H - Header file for GCF Communication Server
**
** Description:
**      This header file contains all data structures required globally by all
**      layers of the GCF Communication Server.  There is, in addition, a 
**      layer-specific header file for each layer.
**      
**      The following structures are defined:
**
**      GCC_GLOBAL  - Comm server global data area
**      GCC_CCB	    - Context info for a particular connection
**      GCC_MDE	    - Message Data Element, carrying SDU's and PDU's through
**		      the layer structure.
**  	GCC_BMT     - Protocol Bridge Mapping Table
**      GCC_BME     - Protocol Bridge Mapping Element
**	GCC_BPMR    - Protocol Bridge Mapping Resource 
**
** History: $Log-for RCS$
**      5-Jan-88 (jbowers)
**          Initial module creation
**     21-Sep-88 (berrow)
**          First batch of "full PL" changes.
**     13-Oct-88 (jbowers)
**          Added GCC_ERLIST error parameter list structure.
**     17-Jan-89 (jbowers)
**          Added event type GCC_A_E_SEND to ccb.ccb_aae.gca_rq_type to allow
**	    for both a normal and expedited send to be outstanding.
**     18-Feb-89 (jbowers)
**          Added queue length counters to AL and TL sections of the CCB for
**	    support of local flow control.  Added inbound and outbound
**	    connection counters in GCC_GLOBAL to support connection limits.
** 	26-May-89 (jorge/heaherm/jbowers)
**	    Increased dimensions for Generic Error support & part of GCA 2.2
**     30-May_89 (cmorris)
**          Added field to CCB to keep track of length of TOD data
**          currently in buffer hanging off CCB.
**     24-Jul-89 (cmorris)
**          Added fields to CCB to keep track of what application protocol
**          and GCA versions are pertinent to the connection.
**     08-Sep-89 (cmorris)
**          Added A_REJECT.
**     04-Oct-89 (cmorris)
**          Changed ..dc_reason fields to type STATUS.
**     08-Nov-89 (cmorris)
**          Added token GCC_A_DISASSOC for async disassociate support; removed
**          CCBRLS define and a_async_cnt CCB element.
**	07-Dec-89 (seiwald)
**	     GCC_MDE reorganised: mde_hdr no longer a substructure;
**	     some unused pieces removed; pci_offset, l_user_data,
**	     l_ad_buffer, and other variables replaced with p1, p2, 
**	     papp, and pend: pointers to the beginning and end of PDU 
**	     data, beginning of application data in the PDU, and end
**	     of PDU buffer.
**	09-Dec-89 (seiwald)
**	     Allow for variously sized MDE's: maintain a list of queues,
**	     and don't define the space at the end of the MDE: it will
**           be allocated by gcc_get_obj.
**	12-Jan-90 (seiwald)
**	     Removed references to MDE flags.  With proper MDE ownership,
**	     flags are no longer needed.
**	25-Jan-90 (seiwald)
**	     Support for message grouping:  track the peer
**	     presentation's protocol level in the CCB; allow for MDE's
**	     to be chained in the CCB for each direction and flow type;
**	     put a length indicator in the PL_TD_PCI; allow for new
**	     "end of group" indicator in the PL and AL srv_parms flags.
**      06-Feb-90 (cmorris)
**           Removed NL service parameters!
**	07-Feb-90 (cmorris)
**	     Added N_LSNCLUP service primitive.
**	11-Feb-90 (seiwald)
**	     Turned prot_indx, an index into the protocol driver table,
**	     into prot_pce, a pointer to the table entry.
**	14-Feb-90 (cmorris)
**	     Added N_ABORT event to inject a protocol exit error
**	     into the protocol machine.
**      01-Mar-90 (cmorris)
**	     Added installation fied to GCC_GLOBAL.
**	01-Mar-90 (cmorris)
**	     Renamed A_REJECT to A_I_REJECT; added A_R_REJECT.
**	07-Mar-90 (cmorris)
**	     Added T_P_ABORT to handle internal aborts within the TL.
**	03-May-90 (heather/jorge)
**	     Made GCC_GLOBAL installation fied an array.
**	19-Jun-90 (seiwald)
**	     Cured the CCB table of tables mania - active CCB's now live
**	     on a single array.
**	08-Aug-90 (seiwald)
**	     New flags GCC_AT_IBMAX and GCC_AT_OBMAX to indicate that the 
**	     network or application listens have been suspended.
**	13-May-91 (cmorris)
**	     Added PL and SL tokens to model state table variables for
**	     release collisions.
**	14-May-91 (cmorris)
**	     Got rid of unused AL flags.
**	20-May-91 (seiwald)
**	     New gcc_cset_nam_q in GCC_GLOBAL.
**	     New gcc_cset_xlt_q in GCC_GLOBAL.
**	     New gcc_cset_xlt in GCC_GLOBAL.
**	     New p_csetxlt in ccb->ccb_pce.
**	24-May-91 (cmorris)
**	     Cleaned up service parameters.
**	31-May-91 (cmorris)
**	    Got rid of remaining vestiges of attempts to multiplex.
**	31-May-91 (cmorris)
**	    Got rid of remaining vestiges of attempts to implement peer
**	    Transport layer flow control.
**	03-Jun-91 (cmorrisa)
**	    Added flag to indicate that we're stopping server.
**	30-July-91 (cmorris)
**	    Got rid of CL_SYS_ERR from mde (never used).
**  	13-Aug-91 (cmorris)
**  	    Added creation timestamp to ccb.
**	14-Aug-91 (seiwald)
**	    Renamed CONTEXT structure to PL_CONTEXT to avoid clash on OS/2.
**	17-aug-91 (leighb) DeskTop Porting Change:
**	    Removed defines of GCC_L_PROTOCOL, GCC_L_NODE & GCC_L_PORT,
**	    they are defined in gcccl.h
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	30-Jan-92 (cmorris)
**	    New protocol bridge support.
**      03-feb-92 (dchan)
**          Had the first 2 params reversed in the memset call, oops...
**  	09-Apr-92 (cmorris)
**  	    Added flag to indicate server is running in transient mode
**  	    and added pointer to the bridge's mapping table.
**	13-apr-92 (bonobo)
**	    Eliminated redundant typedefs.
**	11-Aug-92 (seiwald)
**	    Added tod_prog_ptr and tod_prog_len to the GCC_CCB to keep
**	    track of the compiled tuple object descriptor.
**  	06-Oct-92 (cmorris)
**  	    Got rid of A_CMDSESS.
**  	07-Oct-92 (cmorris)
**  	    Removed listen parmlist from global structure:- listen
**  	    parameter lists are now allocated dynamically to allow
**  	    for multiple listens in progress.
**  	14-Oct-92 (cmorris)
**  	    Message grouping support - added ability to save mdes in
**  	    PL section of CCB, and added a flag to AL section to indicate
**  	    whether GCA_LISTEN has yet been resumed.
**  	27-Nov-92 (cmorris)
**  	    Changed A_NOOP to A_LISTEN for readability.
**  	30-Nov-92 (cmorris)
**  	    Added GCM flags to AL section of CCB.
**  	02-Dec-92 (brucek)
**  	    Added #defines for GCC MIB variable names.
**  	03-Dec-92 (brucek)
**  	    Changed GCC MIB variable names to remove "common".
**  	04-Dec-92 (brucek)
**  	    Correct inconsistency in MIB naming.
**  	08-Dec-92 (cmorris)
**  	    Added fields to save fast select data in CCB.
**  	28-Dec-92 (cmorris)
**  	    Got rid of TL service parameters that are unused.
**  	04-Jan-93 (cmorris)
**  	    Moved storage of remote network address from MDE to CCB.
**  	21-Jan-93 (cmorris)
**  	    Removed unused T_CONTINUE.
**	16-Feb-93 (seiwald)
**	    GCA service parameters have been moved from hanging off the
**	    ccb to overlaying the protocol driver's p_plist in the MDE.
**	    No more GCC_ID_GCA_PLIST.
**	17-Feb-93 (seiwald)
**	    Eliminated a_gca_rq array of MDE pointers in the CCB - we no
**	    longer need the list of MDE's associated with outstanding GCA 
**	    requests.  Moved #defines for gca_rq_type down into MDE.
**	29-mar-93 (kirke)
**	    One more redundant typedef - removed from struct _GCC_BMT.
**      29-Jun-92 (fredb)
**          Integrate henrym's 6.2 changes for MPE (hp9_mpe):
**              11-Jul-91 (henrym)
**                      hp9_mpe must allocate GCA buffers based on
**                      max # sessions.
**                      Set GCC_IB_DEFAULT to 30 for hp9.mpe.
**                      Set GCC_OB_DEFAULT to 30 for hp9.mpe.
**	29-mar-93 (johnr)
**	    Fixed last submittal to remove <<YOURS >> THEIRS
**	5-apr-93 (robf)
**	    Added security label handling in AL. Extended a_flags in ccb
**	    to a i4 since all bits in a u_char were used up.
**  	28-Sep-93 (cmorris)
**  	    Additions to support passing end of message/group flags
**  	    down to protocol drivers.
**	12-nov-93 (robf)
**          Add node_id to ccb to contain remote node name  on listens
**      13-Apr-1994 (daveb) 62240
**          Add counters for statistics.
**	27-mar-95 (peeje01)
**	    Crossintegration from doublebyte label: 6500db_su4_us42
**	    13-jul-94 (kirke)
**	        Added mde_q and split elements to GCC_CCB struct for
**	        varying length character conversion.
**	14-Nov-95 (gordy)
**	    Support GCA_API_LEVEL_5 which removes GCA_INTERPRET for
**	    fast select info (now available in listen parms).
**	20-Nov-95 (gordy)
**	    Extract PCI definitions to gccpci.h.
**	21-Nov-95 (gordy)
**	    Add GCC_STANDALONE flag to CCB.  Add standalone parameter
**	    to gcc_init().
**	06-Feb-96 (rajus01)
**	    Changed the definition of GCC_BMT struct for Bridge..
**	    Added GCC_BME, GCC_BPMR  structs  for Protocol Bridge.
**	    Added GCB ( Protocol Bridge ) function definitions. 
**	27-Feb-96 (rajus01)
**          Added CI_INGRES_NET parameter to gcc_init(). This enables
**          Protocol Bridge check it's own AUTH STRING.
**	18-Apr-96 (rajus01)
**          Added MIB definitions for Bridge.
**	19-Jun-96 (gordy)
**	    Added GCC_EVQ structure to queue recursive layer requests.
**	    This will allow the output/actions of a request to be fully
**	    executed prior to processing subsequent requests.  
**	16-Sep-96 (gordy)
**	    Added gcc_protocol_vrsn to GCC_GLOBAL for PL protocol level.
**	 1-Oct-96 (gordy)
**	    Added GCC_MIB_CONNECTION_PL_PROTOCOL.
**	 3-Jun-97 (gordy)
**	    Building of initial connection info moving to SL rather than
**	    AL so that it can be conditionalized on negotiated protocol
**	    level.  AL connection info added to CCB for use by SL.
**	30-Jun-97 (rajus01)
**	    Added GCC_MECH_INFO, GCS_CBQ structures. Added few function
**	    prototypes. Added new definitions in CCB for handling 
**	    encryption/decryption in TL.
**	20-Aug-97 (gordy)
**	    Added encryption mode definitions.  Removed GCC_MECH_INFO,
**	    node_id, instance_name, gcc_ct_plist, gcc_hw_plist, 
**	    gcb_lcl_id, gcc_pb_max and GCB_STOP since not used.  
**	    Initialization routines reworked to be more generic.
**	 5-Sep-97 (gordy)
**	    Added flag for password encryption by SL (negotiated by TL).
**	10-Oct-97 (rajus01)
**	    Added gcc_encode(), gcc_decode() function definitions. 
**	17-Oct-97 (rajus01)
**	    Added GCC_L_EMODE, GCC_L_EMECH. Added auth, emech, emode in ccb.
**	    gcc_gcs_mechanisms now takes char* instead of u_char.
**	22-Oct-97 (rajus01)
**	    Added comments. Added emech_ovhd. gcc_get_obj(),
**	    gcc_rel_obj() are static (gccutil.c). Added gcc_add_mde(),
**	    gcc_del_mde() definitions.
**	17-Feb-98 (gordy)
**	    Added MIB definition for gcc tracing level.
**	31-Mar-98 (gordy)
**	    Added configuration of default encryption mechanisms.
**	 8-Sep-98 (gordy)
**	    Save installation registry protocols in global control block.
**	30-Sep-98 (gordy)
**	    Added MIB definitions for protocols and registry mode.
**	 2-Oct-98 (gordy)
**	    Moved MIB class definitions to gcm.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-oct-2000 (somsa01)
**	    The length of GCC_L_LCL_ID has been increased to 18.
**	12-May-03 (gordy)
**	    Changed doc_stacks in GCC_CCB to PTR.
**	 6-Jun-03 (gordy)
**	    Added protocol information block (PIB) for listen address info.
**	    Enhanced connection MIB information.
**	24-Sep-2003 (rajus01) Bug #110936
**	    Added GCC_GCA_LSTN_NEEDED flag.
**	16-Jan-04 (gordy)
**	    MDE's may be upto 32K, so expand MDE queue array accordingly.
**	21-Jan-04 (gordy)
**	    Added registry flags to signal registry initialization.
**	16-Jun-04 (gordy)
**	    Added session mask to CCB.
**	15-Jul-04 (gordy)
**	    Added globals for password encryption.
**	15-Sept-04 (wansh01)
**	    Added GCC/ADMIN support.
**      18-Nov-04 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Moved gcc global references intended for the private use of GCC
**          to this file from gcxdebug.h.
**	31-Apr-06 (gordy)
**	    Added GCC_L_ENCODE for max size of encoded strings.
**	18-Aug-06 (rajus01)
**	    Added stats info for each session through GCC. Current stats
**	    info is calculated for all the sessions going through GCC and
**	    stored in GLOBAL area. 
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**	22-Jan-10 (gordy)
**	    DOUBLEBYTE code generalized for multi-byte charset processing.
**      15-Nov-2010 (stial01) SIR 124685 Prototype Cleanup
**          Changes to eliminate compiler prototype warnings.
*/

#include <gcxdebug.h>


/*
** Forward and/or External typedef/struct references.
*/

typedef struct _GCC_GLOBAL GCC_GLOBAL;
typedef struct _GCC_ERLIST GCC_ERLIST;
typedef struct _GCC_MDE GCC_MDE;
typedef struct _GCC_PIB GCC_PIB;
typedef struct _GCC_CCB GCC_CCB;
typedef struct _GCC_EVQ GCC_EVQ;
typedef struct _GCC_BMT GCC_BMT;
typedef struct _GCC_BME GCC_BME;
typedef struct _GCC_BPMR GCC_BPMR;
typedef struct _N_ADDR N_ADDR;
typedef struct _PL_CONTEXT PL_CONTEXT;
typedef struct _TL_SRV_PARMS TL_SRV_PARMS;
typedef struct _SL_SRV_PARMS SL_SRV_PARMS;
typedef struct _PL_SRV_PARMS PL_SRV_PARMS;
typedef struct _AL_SRV_PARMS AL_SRV_PARMS;
typedef struct _GCS_CBQ GCS_CBQ;


/*
** The following definitions are legitimate values for the element
** "primitive_type " in GCC_MDE.  They are defined here instead of under
** the structure element for clarity and readability.
*/

# define	    RQS		1   /* Request */
# define	    IND		2   /* Indication */
# define	    RSP		3   /* Response */
# define	    CNF		4   /* Confirm */

/*
**      The following are identifiers for data objects used by GCC.
*/

# define                 GCC_ID_CCB		1
# define                 GCC_ID_MDE		2



/*
** The following definitions are legitimate values for the element
** "service_primitive" in GCC_MDE.  They are defined here instead of under
** the structure element for clarity and readability.  The number of them
** obscures the structure if defined there.  They are the designators for 
** SDU's which are used in the present implementation.
*/

# define    A_ASSOCIATE		701	/* Initiate appl'n assoc'n */
# define    A_CONNECTED		702	/* Rqstd assocn completed */
# define    A_ABORT		703	/* Abort an association */
# define    A_COMPLETE		704	/* GCA send complete */
# define    A_DATA		705	/* Normal flow data received */
# define    A_EXP_DATA		706	/* Expedited flow data rcvd */
# define    A_RELEASE		707	/* Terminate appl'n assoc'n */
# define    A_LISTEN		708	/* Place GCA listen */
# define    A_LSNCLNUP		709	/* Clean up after failed listen */
# define    A_PURGED		710	/* Receive has been purged */
# define    A_SHUT		711	/* Server shutdown command received */
# define    A_QUIESCE		712	/* Server quiesce command received */
# define    A_P_ABORT		713	/* AL internal error: abort */
# define    A_I_REJECT          714     /* Local reject after listen */
# define    A_FINISH            715     /* Disassociate complete */
# define    A_R_REJECT		716	/* Remote reject before request */
# define    A_CMDSESS		717	/* command (monitor) session */
# define    P_CONNECT		601	/* Initiate presentation connection */
# define    P_RELEASE		602	/* Release presentation connection */
# define    P_U_ABORT		603	/* PS User-initiated abort */
# define    P_P_ABORT		604	/* PS Provider-initiated abort */
# define    P_DATA		605	/* Presentation normat data */
# define    P_EXP_DATA		606	/* Presentation expedited data */
# define    P_XOFF		607	/* Flow control: stop MDE flow */
# define    P_XON		608	/* Flow control: resume MDE flow */
# define    S_CONNECT		501	/* Initiate session connection */
# define    S_RELEASE		502	/* Release session connection */
# define    S_U_ABORT		503	/* SS User-initiated abort */
# define    S_P_ABORT		504	/* SS Provider-initiated abort */
# define    S_DATA		505	/* Session normat data */
# define    S_EXP_DATA		506	/* Session expedited data */
# define    S_XOFF		507	/* Flow control: stop MDE flow */
# define    S_XON		508	/* Flow control: resume MDE flow */
# define    T_CONNECT		401	/* Initiate transport connection */
# define    T_DISCONNECT	402	/* Terminate transport connection */
# define    T_DATA		403	/* Transport normat data */
# define    T_OPEN		404	/* Initialize Transport Layer */
# define    T_XOFF		405	/* Flow control: stop MDE flow */
# define    T_XON		406	/* Flow control: resume MDE flow */
# define    T_P_ABORT		407	/* TS Provider-initiated abort */
# define    N_CONNECT		301	/* Initiate network connection */
# define    N_DISCONNECT	302	/* Terminate network connection */
# define    N_DATA		303	/* Receive network data */
# define    N_OPEN		304	/* Open access to network */
# define    N_RESET		305	/* Network failure */
# define    N_LSNCLUP		306	/* Network listen failure */
# define    N_ABORT		307	/* Network abort */
# define    N_XOFF  	    	308 	/* Flow control: stop MDE flow */
# define    N_XON   	    	309 	/* Flow control: resume MDE flow */
# define    N_P_ABORT	    	310 	/* Bridge provider abort */
# define    N_CNCLUP	    	311 	/* Bridge provider abort */


/*
**      The following are miscellaneous lengths
*/

# define    GCC_L_AE_TITLE	64	/* Length of Application Entity Title */
# define    GCC_L_SYNTAX_NM	16	/* Length of a syntax name */
# define    GCC_L_ARCH_CHAR	6	/* Length of architecture characteristic
					** -- must be less than GCC_L_SYNTAX_NM
					*/
# define    GCC_L_BUFF_DFLT	1024	/* Default GCA message size */
# define    GCC_L_LCL_ID	18	/* Length of the local server ID */
# define    GCC_L_ERLIST	4	/* Max # of error parms */
# define    GCC_L_GEN_STAT_MDE	2	/* Lng of mde_generic status array */
# define    GCC_L_MDE_Q		10	/* MDE queues */


/*
** GCC uses CIencode()/CIdecode() for decryption.  These routines
** work on 8 byte blocks of data.  Each GCC block contains 7 bytes
** of data and a random value byte.  A delimiter byte is added to
** the source data and any random padding needed to fill the final
** block.
*/

# define    GCC_CRYPT_SIZE	8
# define    GCC_KEY_LEN		GCC_CRYPT_SIZE
# define    GCC_ENCODE_LEN(len)	\
		(( ((len) + GCC_CRYPT_SIZE - 1) \
		   / (GCC_CRYPT_SIZE - 1)) * GCC_CRYPT_SIZE )

/*
**      The following are miscellaneous constants.
*/

# define	UP			 1
# define	DOWN			-1
# define	US			 0

/* PL message direction */

# define                 INFLOW                0
# define                 OUTFLOW               1

/* PL message flowtype */

# define                 NORMAL                0
# define                 EXPEDITED             1

/*
** Encryption modes.
*/

# define GCC_ENCRYPT_OFF	0		/* No encryption */
# define GCC_ENCRYPT_OPT	1		/* Encryption optional */
# define GCC_ENCRYPT_ON		2		/* Encryption desired */
# define GCC_ENCRYPT_REQ	3		/* Encryption required */

# define GCC_ENC_OFF_STR	"off"
# define GCC_ENC_OPT_STR	"optional"
# define GCC_ENC_ON_STR		"on"
# define GCC_ENC_REQ_STR	"required"


/*}
** Name: GCC_GLOBAL - Global data area
**
** Description:
**      GCC_GLOBAL contains all global data required by the GCF 
**      communication server.
**
** History:
**      5-Jan-88 (jbowers)
**          Initial structure implementation
**      29-Jun-92 (fredb)
**          Integrate henrym's 6.2 changes for MPE (hp9_mpe):
**              11-Jul-91 (henrym)
**                      hp9_mpe must allocate GCA buffers based on
**                      max # sessions.
**                      Set GCC_IB_DEFAULT to 30 for hp9.mpe.
**                      Set GCC_OB_DEFAULT to 30 for hp9.mpe.
**      13-Apr-1994 (daveb) 62240
**          Add counters for statistics.
**	21-Nov-95 (gordy)
**	    Replace GCC_ERR_INIT with GCC_STANDALONE.
**	06-Feb-96 (rajus01)
**	    Added gcb_lcl_id, GCB_STOP for Bridge.
**	16-Sep-96 (gordy)
**	    Added gcc_protocol_vrsn for PL protocol level.
**	20-Aug-97 (gordy)
**	    Removed GCC_MECH_INFO, gcc_ct_plist, gcc_hw_plist, 
**	    gcb_lcl_id, gcc_pb_max and GCB_STOP since not used.
**	31-Mar-98 (gordy)
**	    Added configuration of default encryption mechanisms.
**	 8-Sep-98 (gordy)
**	    Save installation registry protocols in global control block.
**	12-Jul-00 (gordy)
**	    Added SL protocol level.
**      25-Jun-01 (wansh01) 
**          Added gcc_trace_level to GCC_GLOBAL.
**	21-Jan-04 (gordy)
**	    Added registry flags to signal registry initialization.
**	15-Jul-04 (gordy)
**	    Added gcc_ns_id and gcc_ns_mask for password encryption.
**	21-Jul-09 (gordy)
**	    Provide room for host name terminator.  Dynamically
**	    allocate the encryption mechanisms.
**	22-Jan-10 (gordy)
**	    Remove installation ID (use CM routines to initialize charset).
*/

struct _GCC_GLOBAL
{
    QUEUE	    gcc_ccb_q;		    /* Free CCB queue */
    QUEUE	    gcc_mde_q[GCC_L_MDE_Q]; /* Free MDE queue */
    QUEUE	    gcc_doc_q;		    /* DOC_EL queue (see PL) */
    QUEUE	    gcc_pib_q;		    /* Protocol Information */
    GCC_CCB	    **gcc_tbl_ccb;	    /* active CCB table */
    i4		    gcc_max_ccb;	    /* length of gcc_tbl_ccb */
    GCC_PCT	    *gcc_pct;		    /* Ptr to protocol table */
    GCC_PCT	    gcc_reg_pct;	    /* Registry protocol table */
    u_i2	    gcc_reg_flags;	    /* Registry state */

#define	GCC_REG_ACTIVE		0x01
#define	GCC_REG_STARTUP		0x02

    u_i2	    gcc_reg_mode;	    /* Registry mode */

#define	GCC_REG_NONE		0
#define	GCC_REG_SLAVE		1
#define	GCC_REG_PEER		2
#define	GCC_REG_MASTER		3

#define	GCC_REG_NONE_STR	"none"
#define	GCC_REG_SLAVE_STR	"slave"
#define	GCC_REG_PEER_STR	"peer"
#define	GCC_REG_MASTER_STR	"master"

    i4		    gcc_ct_mde;		    /* Currently allocated MDE's */
    i4		    gcc_hw_mde;		    /* Total allocated MDE's */
    i4		    gcc_ct_ccb;		    /* Currently allocated CCB's */
    i4		    gcc_hw_ccb;		    /* Total allocated CCB's */
    char	    gcc_host[GCC_L_NODE+1]; /* Host name */
    char	    gcc_lcl_id[GCC_L_LCL_ID]; /* Local (GCA) server id */
    char	    gcc_ns_id[GCC_L_LCL_ID];/* Name Server (GCA) server id */
    u_i1	    gcc_ns_mask[ 8 ];	    /* Must be 8 bytes */
    i4		    gcc_gca_hdr_lng;	    /* Length of GCA msg hdr */
    u_char 	    gcc_flags;		    /* Comm Server flags */

# define GCC_STANDALONE		0x01	    /* Standalone Comm Server */
# define GCC_GCA_LSTN_NEEDED    0x02        /* Repost GCA_LISTEN */
# define GCC_AT_OBMAX		0x04	    /* GCA listens suspended */
# define GCC_AT_IBMAX		0x08	    /* Network listens suspended */
# define GCC_STOP		0x10	    /* Comm Server is stopping */
# define GCC_QUIESCE		0x20	    /* Comm Server is quiescing */
# define GCC_TRANSIENT		0x40	    /* Transient mode */
# define GCC_CLOSED 		0x80	    /* Comm Server is closed  */

    i4		    gcc_conn_ct;	    /* Count of current connections */
    i4		    gcc_ib_conn_ct;	    /* Count of inbound connections */
    i4		    gcc_ob_conn_ct;	    /* Count of outbound connections */
    i4		    gcc_ib_max;		    /* Max # inbound connections */

#ifdef hp9_mpe
# define GCC_IB_DEFAULT		30      /* Default max # inbound conn's */
#else
# define GCC_IB_DEFAULT		9999    /* Default max # inbound conn's */
#endif

    i4		    gcc_ob_max;		    /* Max # outbound connections */

#ifdef hp9_mpe
# define GCC_OB_DEFAULT		30      /* Default max # outbound conn's */
#else
# define GCC_OB_DEFAULT		9999    /* Default max # outbound conn's */
#endif

    i4		    gcc_ib_mode;		/* Inbound encryption mode */
    char	    *gcc_ib_mech;		/* Inbound encryption mech */
    i4		    gcc_ob_mode;		/* Outbound encryption mode */
    char	    *gcc_ob_mech;		/* Outbound encryption mech */

# define GCC_ENCRYPT_MODE	GCC_ENCRYPT_OPT
# define GCC_ANY_MECH		"*"

    i4		    gcc_lglvl;		    /* Status level for private log */
    i4		    gcc_erlvl;		    /* Status level for public log */
    i4		    gcc_trace_level;	    /* Trace level for trace log  */
    QUEUE	    gcc_cset_nam_q;	    /* Names of character sets */
    QUEUE	    gcc_cset_xlt_q;	    /* Trans tables for char sets */
    PTR		    gcc_cset_xlt;	    /* Local <-> NTS trans tbl */
    i4		    gcc_pl_protocol_vrsn;   /* PL protocol level */
    GCC_BMT 	    *gcc_pb_mt;	    	    /* Mapping table for bridge */
    u_i4	    gcc_msgs_in;	    /* stats exported via MO */
    u_i4	    gcc_msgs_out;	    /* stats exported via MO */
    u_i4	    gcc_data_in;	    /* stats exported via MO */
    u_i4	    gcc_data_out;	    /* stats exported via MO */
    i4		    gcc_secure_flags;	    /* Secure operation flags */
    i4		    gcc_admin_ct;	    /* Count of admin connections */

};  /* end GCC_GLOBAL */


/*}
** Name: GCC_ERLIST - Error parameter list for error logging
**
** Description:
**       
**      All Comm Server errors are logged in a common place, namely the routine
**	gcc_er_log.  It uses the routine ule_format to actually format the error
**	messages and log them in the log file.  An error message may have
**	optional variable parameters associated with it.  These are specified in
**	the text of the error message in the message file.  This structure
**	provides the mechanism for passing optional parameters to gcc_er_log.
**	It consists of a count of the number of parms to follow, and a list of
**	parameters.  Each parm in the list is a size/value pair.  The size
**	indicates the length in bytes of the parameter to be inserted in the
**	message.  The value is a pointer to the start of the actual parameter.
**	Note well that the user of this facility must know the actual text of
**	the message being invoked, specifically the number of parameters and
**	their type.  The parms in the list must be in the same order as the
**	parms in the message.
**
** History:
**      12-Oct-88 (jbowers)
**          Initial structure creation
*/
struct _GCC_ERLIST
{
    i4		gcc_parm_cnt;		/* # of parms to follow */
    struct
    {
	i4	    size;		/* Length of this parm */
	PTR	    value;		/* Pointer to this parm */
    } gcc_erparm[GCC_L_ERLIST];
};   


/*}
** Name: N_ADDR - Network Address
**
** Description:
**       A GCC Network Address consists of a protocol id, node id
**  	 and port id.
**
** History:
**      04-Jan-93 (cmorris)
**          Initial template creation
*/
struct		_N_ADDR
{
    char		n_sel[GCC_L_PROTOCOL + 1];  /* Protocol identifier */
    char		node_id[GCC_L_NODE + 1];    /* Node identifier */
    char		port_id[GCC_L_PORT + 1];    /* Port identifier */
};	    /* Network address */


/*
** The following structure is usec by PL to specify a presentation context.
*/

struct		_PL_CONTEXT
{
    char    abstr_syntax[GCC_L_SYNTAX_NM];	    /* Abstract syntax name */
    char    trsnfr_syntax[GCC_L_SYNTAX_NM];	    /* Transfer syntax name */

# define	SZ_CONTEXT	    2*GCC_L_SYNTAX_NM

};	    /* Presentation context identifier */



/*}
** Name: GCC_AAS_DATA - A_ASSOCIATE user data structure
**
** Description:
**	This element specifies the structure of the user data contained in an
**	A_ASSOCIATE service primitive.  It contains the data received in the
**	completed GCA parm list for the GCA_LISTEN issued by the Initiating
**	Application Layer, and used in the GCA_REQUEST issued by the Responding
**	Application Layer.
**
** History:
**      08-MAR-88 (jbowers)
**          Initial structure implementation.
**      05-Oct-89 (cmorris)
**          As structure is no longer used to build what goes out on the wire, 
**          make gcc_gca_protocol what it really should be.
*/
typedef struct _GCC_AAS_DATA
{
 
/* 
** GCA protocol was defined as a u_i4 at 6.1 and 6.2. 
** It was set but never used. At application protocol 
** level 6.3, it is both set and used.  At 6.5, the 
** first two octets are used to transport the GCA 
** association flags.
**
** This structure represents prior protocol format
** and the lengths defined here should only be used
** in reference to this structure.
*/

# define    GCC_L_UNAME		32	/* Username length (deprecated) */
# define    GCC_L_PWD		64	/* Password length (deprecated) */
# define    GCC_L_ACCTNAME	16	/* Account name length (deprecated) */
# define    GCC_L_API		16	/* Access point id len (deprecated) */
# define    GCC_L_SECLABEL	140	/* Security label len (deprecated) */

    u_i2	    gcc_gca_flags;
    u_i2	    gcc_gca_protocol;
    char            gcc_user_name[ GCC_L_UNAME ];
    char            gcc_pwd[GCC_L_PWD];
    char            gcc_account_name[GCC_L_ACCTNAME];
    char	    gcc_sec_label[GCC_L_SECLABEL];
    char            gcc_api[GCC_L_API];
}   GCC_AAS_DATA;


/*
** Name: GCC_EVQ
**
** Description:
**	Connection event queue.  To ensure that all actions and outputs
**	associated with an input event of a given layer are processed
**	prior to processing resulting events, nested calls to a layer
**	queue the events so that they may be processed by the outermost
**	instance of the layer.
**
** History:
**	19-Jun-96 (gordy)
**	    Created.
*/

struct _GCC_EVQ
{

    bool	active;
    QUEUE	evq;

}; /* end _GCC_EVQ */

struct _GCS_CBQ
{
    QUEUE	cbq;
    u_char	mech_id;
    PTR 	cb;
};


/*
** Name: GCC_PIB
**
** Description:
**	Protocol Information Block.  
*/

struct _GCC_PIB
{
    QUEUE	q;
    u_i4	flags;				/* PCT status flags */
    char	pid[ GCC_L_PROTOCOL + 1 ];	/* Protocol identifier */
    char	host[ GCC_L_NODE + 1 ];		/* Host identifier */
    char	addr[ GCC_L_PORT + 1 ];		/* Listen address */
    char	port[ GCC_L_PORT + 1 ];		/* Actual listen port */
};


/*}
** Name: GCC_CCB - Connection Control Block
**
** Description:
**      The Connection Control Block (CCB) represents a logical connection
**      passing through the Communication Server.  It simultaneously represents
**      the Presentation, Session and Transport connections, since these all
**      map one-to-one.  The same CEI designator is used for all.  There are 
**      local sections within the CCB for the representation of the connection
**      at each layer.  The CCB is the complete embodiment of the context 
**      information required to be known for a connection at any layer.
**
**      In the Network Connection Element of the CCB is a pointer to a Protocol
**      Control Block (PCB).  This is used by the protocol driver module, and
**      is a local, environment-specific collection of information needed by
**      the protocol driver to manage the connection.  The form, content
**      and size are known only to the protocol module.
**
** History:
**      05-Jan-88 (jbowers)
**          Initial structure implementation
**	14-Nov-95 (gordy)
**	    Added fast select message type for API level 5 which
**	    removes GCA_INTERPRET.
**	19-Jun-96 (gordy)
**	    Added request queues for each layer.  Added flags for CCB
**	    deletion in AL, TL.  Instead of freeing the CCB in a layer
**	    action, a flag is set so that the CCB will be freed once
**	    the request queue for the layer has been completely
**	    processed.
**	20-Aug-97 (gordy)
**	    Removed node_id (overloaded usage of rmt_addr.node_id).
**	 5-Sep-97 (gordy)
**	    Added flag for password encryption by SL (negotiated by TL).
**	 4-Dec-97 (rajus01)
**	    Added authlen. Changed the type of auth to be PTR.
**	12-Jul-00 (gordy)
**	    Added SL protocol level to ccb_sce.
**	12-May-03 (gordy)
**	    Changed doc_stacks to PTR rather than QUEUE since only
**	    one IN/OUT NRM/EXP DOC is active at any one time.
**	 6-Jun-03 (gordy)
**	    Enhanced MIB information.  Remote target address info moved
**	    to trg_addr while rmt_addr has actual remote address along
**	    with actual local address in lcl_addr.  
**	16-Jun-04 (gordy)
**	    Added session mask.
**	21-Jul-09 (gordy)
**	    Remove unused security label stuff.  Add flag to indicate
**	    username has been received.  Dynamically allocate target,
**	    username, password and mechanism strings.
**	22-Jan-10 (gordy)
**	    DOUBLEBYTE code generalized for multi-byte charset processing.
*/

struct _GCC_CCB
{
    QUEUE	q_link;	    /* Header for enqueueing */

    /*
    ** Global header structure: used by all layers 
    */
    struct 
    {
    	struct TM_SYSTIME timestamp;		/* creation timestamp */
	i4		async_cnt;		/* # current async rqsts */
	u_char		flags;			/* Global connection status */

# define	CCB_FREE	    0x0001	/* CCB currently free */
# define	CCB_IB_CONN	    0x0002	/* Inbound connection */
# define	CCB_OB_CONN	    0x0004	/* Outbound connection */
# define	CCB_MIB_ATTACH	    0x0008	/* MIB Attached */
# define	CCB_USERNAME	    0x0010	/* Username received */
# define	CCB_PWD_ENCRYPT     0x0020	/* Password encryption */

	u_i4		lcl_cei;		/* Local Conn. Endpt. ID */
	u_i4		rmt_cei;		/* Remote Conn. Endpt. ID */
	char		*inbound;		/* Inbound connection? */
	N_ADDR		trg_addr;		/* Request/Listen Address */
    	N_ADDR		lcl_addr;		/* Local Network Address */
    	N_ADDR		rmt_addr;		/* Remote Network Address */
    	char		*target;		/* Target DBMS */
	char		*username;		/* User ID */
	char		*password;		/* Password */
	i4		gca_protocol;		/* GCA protocol level */
	u_i4		gca_flags;		/* GCA listen flags */
	u_i4		authlen;		/* Remote auth len */
	PTR		auth;			/* Remote authentication */
	char		*emech; 		/* Encryption Mechanism */
	i4		emode;			/* Encryption Mode */
	i4		emech_ovhd;		/* Encry_mechanism Overhead */

	/*
	** The session mask is used to modify the password
	** encryption key.  It is currently defined to match 
	** the fixed key length used by CIencode()/CIdecode().
	** Mask length should be set to 0 if not received 
	** from partner GCC.
	*/

# define	GCC_SESS_MASK_MAX	8

	i4		mask_len;		/* Session mask & length */
	u_i1		mask[ GCC_SESS_MASK_MAX ];

    } ccb_hdr;		/* CCB global header */

    /*
    ********************************
    ** Application Layer section. **
    ********************************
    */

    struct 
    {
	GCC_EVQ            al_evq;		/* Event Queue */
	i4		   assoc_id;		/* GCA association ID */
	i4		   buf_size;		/* GCA suggested buffer size */
	QUEUE              a_rcv_q;		/* SDU's received from PL */
	i4		   a_rq_cnt;		/* Current size of a_rcv_q  */

# define	GCCAL_RQMAX	(i4) 1     	/* Max receive q size */

	i4        	a_flags;		/* Status flags for AL */

# define        GCCAL_GOTOHET	        0x01	/* GOTOHET to be sent */
# define	GCCAL_RXOF		0x02	/* Received XOFF */
# define	GCCAL_SXOF		0x04	/* Sent XOFF */
# define	GCCAL_ABT		0x08	/* Internal AL abort */
# define	GCCAL_RGRLS		0x10	/* Recvd GCA_RELEASE */
# define    	GCCAL_LSNRES	    	0x20    /* GCA_LISTEN resumed */
# define    	GCCAL_GCM   	    	0x40    /* GCM connection */
# define    	GCCAL_FSEL  	    	0x80    /* GCM fastselect */
# define	GCCAL_CCB_RELEASE	0x200	/* AL done with CCB */

	u_char              a_state;            /* FSM state for AL */

        i4                  a_version;          /* Protocol version in use */
    	PTR 	    	    fsel_ptr;	    	/* Saved fast select data */
    	u_i4	    	    fsel_len;	    	/* length of saved data */
	i4		    fsel_type;		/* fast select message type */
    }               ccb_aae;		/* Application Association Element */

    /*
    *********************************
    ** Presentation Layer section. **
    *********************************
    */

    struct
    {
	GCC_EVQ            pl_evq;		/* Event Queue */

    	/* Saved mdes
        ** Subscript is INFLOW/OUTFLOW          (flow direction)
    	*/
	GCC_MDE	           *data_mde[2];	/* [flow direction] */
	QUEUE		   mde_q[2];		/* Output MDE process queue */

	/* Data object context stacks
	** Subscript 1 is INFLOW/OUTFLOW        (flow direction)
	** Subscript 2 is NORMAL/EXPEDITED	(flow type)
	*/
	PTR		   doc_stacks[2][2];	/* [flow direction][flow type]*/
	PTR		   tod_ptr;		/* Ptr. to tuple object desc. */
	u_i4		   tod_len;		/* Length of tod area */
	u_i4		   tod_data_len;	/* Length of current tod data */
	PTR		   tod_prog_ptr;	/* Ptr. to compiled tod */
	u_i4		   tod_prog_len;	/* length of tod_prog */
	i4		   het_mask;		/* Heterogenous data mask */

# define                 GCC_HOMOGENEOUS                0x0000

	PTR		   p_csetxlt;		/* character trns table */
	PTR                p_context;		/* Conversion context */
	u_char             p_flags;		/* Status flags for PL*/

# define	GCCPL_ABT		0x01	/* Internal PL abort */
# define	GCCPL_CR		0x02	/* Release Collision */

	u_char		   p_state;		/* FSM state for PL*/
	i4		   p_version;		/* peer's protocol level */
    }               ccb_pce;		/* Presentation Connection Element */

    /*
    ****************************
    ** Session Layer section. **
    ****************************
    */

    struct 
    {
	GCC_EVQ            sl_evq;		/* Event queue */
	u_char		   s_flags;		/* Status flags for SL */

# define	GCCSL_ABT		0x01	/* Internal SL abort */
# define	GCCSL_COLL		0x02	/* Release Collision */
# define	GCCSL_DNR		0x04	/* DN SPDU received in collision */

	u_char             s_state;		/* FSM state for SL */
	GCC_MDE		   *s_con_mde;		/* Saving place for S_CONNECT */
	i1		   s_version;		/* Session protocol level */
    }               ccb_sce;		/* Session Connection Element */

    /*
    ******************************
    ** Transport Layer section. **
    ******************************
    */

    struct 
    {
	GCC_EVQ            tl_evq;		/* Event Queue */
	u_char		   t_flags;		/* Status flags for TL */

# define	GCCTL_RXOF		0x01	/* Received XOFF */
# define	GCCTL_SXOF		0x02	/* Sent XOFF */
# define	GCCTL_ABT		0x04	/* Internal TL abort */
# define	GCCTL_CCB_RELEASE	0x08	/* TL done with CCB */
# define	GCCTL_ENCRYPT		0x10	/* Encryption flag for TL */

	u_char             t_state;		/* FSM state for TL */
	u_i4               src_ref;		/* Source reference ID */
	u_i4               dst_ref;		/* Destination reference ID */
	GCC_PCE		   *prot_pce;		/* GCCL protocol definition */
	GCC_PIB		   *pib;		/* Protocol Information */
	u_i4		   version_no;		/* TL protocol version id */
	u_char	   	   tpdu_size;		/* TPDU size */
	PTR                pcb;			/* Ptr to Prococol Ctl Blk */
	QUEUE              t_snd_q;		/* Q of MDE's to send */
	i4		   t_sq_cnt;		/* Current size of t_snd_q  */

# define	GCCTL_SQMAX	(i4) 1	    /* Max send q size */

	i4		   lsn_retry_cnt;	/* # of GCC_LISTEN retries */

# define	GCCTL_LSNMAX	(i4) 8	    /* Max GCC_LISTEN retries */
 	PTR		  gcs_cb; 	 /* Ptr to GCS Ctl Blk */
    	u_char  	  mech_id; 	 /* Mechanism ID for encryption*/
	QUEUE		  t_gcs_cbq;
        u_i4	    msgs_in;	    /* stats duplicate from GCC_GLOBAL */
        u_i4	    msgs_out;	    /* stats duplicate from GCC_GLOBAL */
        u_i4	    data_in;	    /* stats duplicate from GCC_GLOBAL */
        u_i4	    data_out;	    /* stats duplicate from GCC_GLOBAL */

    }               ccb_tce;		/* Transport Connection Element */

    /*
    ******************************
    ** Protocol Bridge section. **
    ******************************
    */

    struct 
    {
	u_char		   b_flags;		/* Status flags for PB */

# define	GCCPB_RXOF		0x01	/* Received XOFF */
# define	GCCPB_SXOF		0x02	/* Sent XOFF */
# define	GCCPB_ABT		0x04	/* Internal PB abort */

	u_char             b_state;		/* FSM state for PB */
	GCC_PCE            *prot_pce;		/* ptr to prot. hndlr */
	u_char	   	   block_size;		/* block size */
	PTR                pcb;			/* Ptr to Prococol Ctl Blk */
	QUEUE              b_snd_q;		/* Q of MDE's to send */
	i4		   b_sq_cnt;		/* Current size of b_snd_q  */
	i4 		   table_index;		/* Mapping table index */

# define	GCCPB_SQMAX	(i4) 1	    /* Max send q size */

    }ccb_bce;		/* Bridge Connection Element */

};  /* end GCC_CCB */


/*}
** Name: AL_SRV_PARMS - Service parameter layout for Application Layer
**
** Description:
**      This structure maps the set of parameters for Application Layer service
**      primitives.
**
**	The service primitive parameters defined here are those for ACSE, or
**	the Association Control Service Element.
**	Most of the service primitive parameters defined in the ISO 
**	international standards for ACSE are not in fact used, and so are not
**	included in this definition.
*/
struct _AL_SRV_PARMS
{
    i4	    as_result;		/* A_ASSOCIATE rsp result, or */
					/* A_RELEASE reason */
# define	    AARQ_ACCEPTED	    0
# define	    AARQ_REJECTED	    1

    u_char	    as_msg_type;	/* Msg type for A_DATA and A_RELEASE */
    u_char	    as_flags;

# define	AS_EOD		    0x01 /* last segment in message */
# define	AS_EOG		    0x02 /* last message in group */

};  /* end AL_SRV_PARMS */

/*}
** Name: PL_SRV_PARMS - Service parameter layout for Presentation Layer
**
** Description:
**      This structure maps the set of parameters for Presentation Layer service
**      primitives.
**
**	Most of the service primitive parameters defined in the ISO 
**	international standard are not in fact used, and so are not included
**	in this definition.
*/
struct _PL_SRV_PARMS
{
    i4	    ps_result;		/* Reason code for provider abort & */
					/* result for connection confirm */

# define	PS_ACCEPT	    1	/* In P_CONNECT rsp and ind */
# define	PS_USER_REJECT	    0	/* In P_CONNECT rsp and ind */
# define	PS_AFFIRMATIVE	    1	/* In P_RELEASE rsp and ind */
# define	PS_NEGATIVE	    0	/* In P_RELEASE rsp and ind */

    u_char	    ps_msg_type;	/* GCA msg type for P_(EXP_)DATA */
    u_char	    ps_flags;

# define	PS_EOD		    0x01 /* last segment in message */
# define	PS_EOG		    0x02 /* last message in group */

}; /* end PL_SRV_PARMS */

/*}
** Name: SL_SRV_PARMS - Service pamameter layout for Session Layer
**
** Description:
**      This structure maps the set of parameters for Session Layer service
**      primitives.
*/
struct _SL_SRV_PARMS
{
    i4	    ss_result;		/* Reason code for provider abort & */
					/* connection refusal */

# define	SS_ACCEPT	    1	/* In S_CONNECT rsp and ind */
# define	SS_USER_REJECT	    0	/* In S_CONNECT rsp and ind */
# define	SS_AFFIRMATIVE	    1	/* In S_RELEASE rsp and ind */
# define	SS_NEGATIVE	    0	/* In S_RELEASE rsp and ind */

    u_char  	    ss_flags;

# define	SS_FLUSH	    0x01 /* Flush message */

}; /* end SL_SRV_PARMS */

/*}
** Name: TL_SRV_PARMS - Service pamameter layout for Transport Layer
**
** Description:
**      This structure maps the set of parameters for Transport Layer service
**      primitives.
*/
struct _TL_SRV_PARMS
{
    u_char  	    ts_flags;

# define	TS_FLUSH	    0x01 /* Flush message */

}; /* end TL_SRV_PARMS */


/*}
** Name: GCC_MDE - GCC Message Data Element
**
** Description:
**      The MDE carries a "message element" through the GCC layer structure.
**      It functions as a Service Data Unit for communication between layers
**      and  as a Protocol Data Unit (PDU) within a layer.  It carries all 
**      data and control information required by each layer.  Each layer adds
**      its protocol information, and the service parameters required for 
**      invocation of service from an adjoining layer, and passes the MDE to 
**      that layer.  The structure and organization of the MDE is designed to 
**      eliminate the necessity for data copying at each layer.  A layer need
**      only add information to the MDE before sending it to the next layer.
**
**      The layout of the MDE is as follows:
**
**      +--------+------------------------+-----------------------------------
**      | HEADER | SERVICE PARAMETER AREA | PDU AREA ......................
**      +--------+------------------------+-----------------------------------
**
**      The header is a global area containing information used by all layers.
**      The Service Parameter area is used by a layer to construct a Service
**      Data Unit to be passed to an adjacent layer, either above or below.
**      It is a union of all service parameter lists for all service primitives 
**      for all layers, and thus reserves space sufficient for any layer to use
**      for any service primitive.  A layer constructs in this area the list of 
**      parameters for the service primitive it wishes to send to an adjacent 
**      layer.
**
**      Within the PDU area the actual Protocol Data Unit which is to be sent 
**      on the network to the peer node is progressively constructed by 
**	successive layers as the MDE passes down through them.  Each layer adds
**      its Protocol Control Information (PCI) in front of the preceding layer's
**      data.  This is then the current layer's PDU.  The process is repeated 
**      as the MDE flows dounward through the layers.  It has the following
**      structure when it is ready to be transmitted:
**
**      +------+------+------+------+------------------------------------------
**      | TPCI | SPCI | PPCI | APCI |   Application message data ..............
**      +------+------+------+------+------------------------------------------
**	|      |      |      |------- APDU ------------------------------------
**      |      |      |-------------- PPDU ------------------------------------
**      |      |--------------------- SPDU ------------------------------------
**      |---------------------------- TPDU ------------------------------------
**
**      When a layer has constructed its PCI it sets a pointer in the global 
**      header area to point  to the start of the  PCI area it has constructed.
**      The next layer then constructs its PCI in the space immediately ahead.
**
**	When a PDU is received into a MDE and percolates upward, the opposite
**	process takes place: as each layer processes its Protocol Control
**	Information, it moves the pointer past its PCI area to the start of the
**	PCI area for the next layer above.  Thus each layer receives from the
**	layer below a pointer to the start of its own PDU.
**
** History:
**      05-Jan-88 (jbowers)
**          Initial structure creation
**	22-Oct-97 (rajus01)
**	    Added pxtend for the encryption overhead.
*/

struct _GCC_MDE
{
    QUEUE	mde_q_link;	/* Q hdr for enqing this MDE on input Q's */
    i4		queue;		/* which Q this MDE is from */

    /*
    ** Following is the global header section, used by all LE's.
    */

    i4	    	service_primitive;  /* SDU primitive designator */
    i4	    	primitive_type;	    /* Service primitive type */

    GCC_CCB	*ccb;		    /* Pointer to CCB for this msg */

    char	*p1;		    /* start of PDU data */
    char	*p2;		    /* end of PDU data */
    char	*ptpdu;		    /* start of TPDU */
    char	*papp;		    /* start of AL data */
    char	*pdata;		    /* start of user data */
    char	*pend;		    /* end of PDU buffer */
    char	*pxtend;	    /* extended end of PDU buffer */

    /*
    ** Following is the section in which the service primitive parms are
    ** specified by one LE sending an SDU to another.
    */

    STATUS	mde_generic_status[GCC_L_GEN_STAT_MDE];

    u_i4	gca_rq_type;	    /* GCA rqst type indicator */

# define                 GCC_A_LISTEN	0
# define                 GCC_A_REQUEST	1
# define                 GCC_A_RESPONSE 2
# define                 GCC_A_SEND	3
# define                 GCC_A_E_SEND	4
# define                 GCC_A_NRM_RCV	5
# define                 GCC_A_EXP_RCV	6
# define                 GCC_A_DISASSOC 7

    union
    {
	AL_SRV_PARMS       al_parms;	    /* AL service parm list */
	PL_SRV_PARMS       pl_parms;	    /* PL service parm list */
	SL_SRV_PARMS       sl_parms;	    /* SL service parm list */
    	TL_SRV_PARMS	   tl_parms;        /* TL service parm list */
    } mde_srv_parms;			/* Service primitive parm lists */

    struct
    {
	GCA_PARMLIST	   gca_parms;		/* GCA service parms */
	GCC_P_PLIST	   p_plist;		/* protocol driver parms */
    } mde_act_parms;

    /*
    ** The remainder of the MDE is the PDU, which consists of all PCI's
    ** and the user data area.  This space is computed by gcc_get_obj, and
    ** so is not defined here.  The max size of the TPDU (everything but the
    ** NL PCI) is negotiated on connection initiation and placed in the ccb.
    **
    ** Some layers need to know certain offsets into the PDU;  they are:
    **
    **	GCC_OFF_TPDU	The beginning of the TPDU: because of the irregular
    **			design of the NL, the TL doesn't issue NL reads
    **			from the start of the NPDU, but rather from the 
    ** 			start of the TPDU.
    **
    **  GCC_OFF_APP	The offset end of the largest potential combined
    **	    	        NL, TL, SL, PL and AL PCI's. This combination
    **	    	    	occurs on an A-Associate request. For most PDU's, AL 
    **                  data is built forward from this offset and PCI's
    **	    	        are built backward from this offset. 
    **
    **	GCC_OFF_DATA	The offset of the end of the user data carrying NL,
    **			TL, SL and PL PCI's (AL has no PCI for user data).
    **			For user data carrying PDU's, user data is built
    **			forward from this offset and PCI's are built backward
    **			from this offset.
    */ 

};  /* end GCC_MDE */


/*
** Name: GCC_BPMR - Bridge PM Resources
**
** Description:
**      The Bridge PM Resources contains variables to for the  PM resources
**  	defined for  protocol bridge.
**
** History:
**	15-Nov-95 (rajus01)
**         Initial structure implementation
**	20-Aug-97 (gordy)
**	    Removed instance_name since not used.
*/
struct _GCC_BPMR
{
#define	GCB_L_INSTANCE  2
#define GCB_PMREC_COUNT 8

    i4     	    	pmrec_count;
    char		*pmrec_name[GCB_PMREC_COUNT];
    char		*pmrec_value[GCB_PMREC_COUNT];
    char		*pm_vnode[8];
    bool		cmd_line;
};  /* end GCC_BPMR */


/*}
** Name: GCC_BMT - Bridge Mapping Table, GCC_BME - Bridge Mapping ELEMENT
**
** Description:
**	GCC_BMT is a structure containing array of 8 pointers to
**	GCC_BME.
**      The Bridge Mapping ELEMENT contains mappings of 'to' and 'from'
**  	connections within the protocol bridge.
**
** History:
**      09-Apr-92 (cmorris)
**          Initial structure implementation
**	06-Feb-96 (rajus01)
**	    GCC_BMT is a structure containing array of 8 pointers to
**	    GCC_BME.
*/
struct  _GCC_BMT
{
  /* The number of mapping elements is equal to to the number
  ** of entries in the Protocol Control Table.
  */
  GCC_BME      *bme[ 8 ];

};  /* end GCC_BMT */
	
struct _GCC_BME
{
    bool                in_use;
    GCC_CCB             *from_ccb;
    GCC_CCB             *to_ccb;
};  /* end GCC_BME */
		     
GLOBALREF GCXLIST gcx_gca_rq[];		/* gca requests */
GLOBALREF GCXLIST gcx_mde_service[];	/* mde service primitives */
GLOBALREF GCXLIST gcx_mde_primitive[];	/* mde service sub-primitives */
GLOBALREF GCXLIST gcx_gcc_sal[];	/* states for AL */
GLOBALREF GCXLIST gcx_gcc_ial[];	/* input events for AL */
GLOBALREF GCXLIST gcx_gcc_oal[];	/* output events for AL */
GLOBALREF GCXLIST gcx_gcc_aal[];	/* output actions for AL */
GLOBALREF GCXLIST gcx_gcc_spl[];	/* states for PL */
GLOBALREF GCXLIST gcx_gcc_ipl[];	/* input events for PL */
GLOBALREF GCXLIST gcx_gcc_opl[];	/* output events for PL */
GLOBALREF GCXLIST gcx_gcc_ssl[];	/* states for SL */
GLOBALREF GCXLIST gcx_gcc_isl[];	/* input events for SL */
GLOBALREF GCXLIST gcx_gcc_osl[];	/* output events for SL */
GLOBALREF GCXLIST gcx_gcc_stl[];	/* states for TL */
GLOBALREF GCXLIST gcx_gcc_itl[];	/* input events for TL */
GLOBALREF GCXLIST gcx_gcc_otl[];	/* output events for TL */
GLOBALREF GCXLIST gcx_gcc_atl[];        /* output actions for TL */
GLOBALREF GCXLIST gcx_gcc_rq[];	        /* gcc requests */
GLOBALREF GCXLIST gcx_gcc_spb[];        /* states for NB */
GLOBALREF GCXLIST gcx_gcc_ipb[];        /* input events for NB */
GLOBALREF GCXLIST gcx_gcc_opb[];        /* output events for NB */
GLOBALREF GCXLIST gcx_gcc_apb[];        /* output actions for NB */
GLOBALREF GCXLIST gcx_gcb_ial[];      /* input events for PB AL */
GLOBALREF GCXLIST gcx_gcb_sal[];      /* states for PB AL */


/*
**  GCC_SESS_INFO
*/

typedef struct
{
    PTR         sess_id;
    i4          assoc_id;
    char        *user_id;
    char        *state;
    char        *target_db;
    char 	srv_class[ GCC_L_LCL_ID + 1 ];
    char 	gca_addr[ GCC_L_LCL_ID + 1 ];
    bool	is_system;
    N_ADDR	trg_addr;	/* Request/Listen Address */
    N_ADDR	lcl_addr;	/* Local Address */
    N_ADDR	rmt_addr;	/* Remote Address */
    u_i4	msgs_in;	/* stats */
    u_i4	msgs_out;       /* stats */
    u_i4	data_in;        /* stats */
    u_i4	data_out;      /* stats */
} GCC_SESS_INFO;


/*
* GCB function definitions
*/
FUNC_EXTERN STATUS	gcb_al( GCC_MDE * );
FUNC_EXTERN STATUS	gcb_alinit( STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcb_alterm( STATUS *, CL_ERR_DESC * );
FUNC_EXTERN VOID	gcb_gca_exit( PTR async_id );

/*
** GCC function definitions
*/
FUNC_EXTERN STATUS	gcc_call( i4  *call_count, STATUS (**call_list)(), 
				  STATUS *status, CL_ERR_DESC *system_status );
FUNC_EXTERN STATUS	gcc_init( bool, i4, i4, char **, 
				  STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcc_term( STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcc_init_registry( STATUS *, CL_ERR_DESC * );

FUNC_EXTERN STATUS	gcc_alinit( STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcc_alterm( STATUS *, CL_ERR_DESC * );

FUNC_EXTERN STATUS	gcc_adm_init( void );
FUNC_EXTERN STATUS	gcc_adm_term( void );
FUNC_EXTERN STATUS	gcc_adm_session( i4, PTR, char * );

FUNC_EXTERN STATUS	gcc_al( GCC_MDE * );
FUNC_EXTERN VOID	gcc_gca_exit( PTR async_id );

FUNC_EXTERN i4		GCngetnat( char *src, i4  *dst, STATUS *status );
FUNC_EXTERN i4		GCnaddnat( i4  *src, char *dst, STATUS *status );
FUNC_EXTERN i4		GCngetlong( char *src, i4 *dst, STATUS *status );
FUNC_EXTERN i4		GCnaddlong( i4 *src, char *dst, STATUS *status );
FUNC_EXTERN i4		GCnadds( char *src, i4  len, char *dst );
FUNC_EXTERN i4		GCngets( char *src, i4  len, char *dst );

FUNC_EXTERN STATUS	gcc_plinit(STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcc_plterm(STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcc_pl( GCC_MDE * );

FUNC_EXTERN STATUS	gcc_init_cset( VOID );
FUNC_EXTERN VOID	gcc_rqst_av( PL_CONTEXT *lcl_cntxt );
FUNC_EXTERN STATUS	gcc_ind_av( GCC_CCB *ccb, PL_CONTEXT *rmt_cntxt );
FUNC_EXTERN VOID	gcc_rsp_av( GCC_CCB *ccb, PL_CONTEXT *lcl_cntxt );
FUNC_EXTERN STATUS	gcc_cnf_av( GCC_CCB *ccb, PL_CONTEXT *rmt_cntxt );

FUNC_EXTERN STATUS	gcc_slinit(STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcc_slterm(STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcc_sl( GCC_MDE * );

FUNC_EXTERN STATUS	gcc_tlinit(STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcc_tlterm(STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcc_tl_registry( VOID );
FUNC_EXTERN STATUS	gcc_tl( GCC_MDE * );
FUNC_EXTERN VOID	gcc_tl_abort( GCC_MDE *, GCC_CCB *, 
				      STATUS *, CL_ERR_DESC *, GCC_ERLIST * );

FUNC_EXTERN GCC_CCB *	gcc_add_ccb( VOID );
FUNC_EXTERN STATUS	gcc_del_ccb( GCC_CCB *ccb );
FUNC_EXTERN GCC_MDE *	gcc_add_mde( GCC_CCB *ccb, u_char size );
FUNC_EXTERN STATUS	gcc_del_mde( GCC_MDE *mde );
FUNC_EXTERN PTR		gcc_alloc( u_i4 size );
FUNC_EXTERN STATUS	gcc_free( PTR block );
FUNC_EXTERN char *	gcc_str_alloc( char *str, i4 len );
FUNC_EXTERN VOID	gcc_er_init( char *server_name, char *server_id );
FUNC_EXTERN STATUS	gcc_er_open( STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcc_er_close( STATUS *, CL_ERR_DESC * );
FUNC_EXTERN VOID	gcc_er_log( STATUS *, CL_ERR_DESC *, 
				    GCC_MDE *, GCC_ERLIST * );
FUNC_EXTERN VOID	gcc_set_conn_info( GCC_CCB *ccb, GCC_MDE *mde );
FUNC_EXTERN VOID	gcc_get_conn_info( GCC_CCB *ccb, GCC_MDE *mde );
FUNC_EXTERN STATUS	gcc_encrypt_mode( char *, i4  * );

FUNC_EXTERN u_char	gcc_gcs_mechanisms( char *mname, u_char *mech_ids );
FUNC_EXTERN VOID	gcc_rqst_encrypt( GCC_MDE *mde, GCC_CCB *ccb, 
					  u_char mech_count, u_char *mech_ids );
FUNC_EXTERN STATUS	gcc_ind_encrypt( GCC_MDE *, GCC_CCB *,
					 u_char, u_char *, char * );
FUNC_EXTERN VOID	gcc_rsp_encrypt( GCC_MDE *mde, GCC_CCB *ccb );
FUNC_EXTERN STATUS	gcc_cnf_encrypt( GCC_MDE *mde, GCC_CCB *ccb );
FUNC_EXTERN i4		gcc_encode( char *, char *, u_i1 *, u_i1 * );
FUNC_EXTERN VOID	gcc_decode( u_i1 *, i4, char *, u_i1 *, char * );
FUNC_EXTERN STATUS	gcc_sess_abort( GCC_CCB * );
FUNC_EXTERN STATUS	gcc_pbinit ( STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcc_pbterm ( STATUS *, CL_ERR_DESC * );
FUNC_EXTERN STATUS	gcc_pb ( GCC_MDE *, STATUS *, CL_ERR_DESC * );
