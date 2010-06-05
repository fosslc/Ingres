/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/**
** Name: GCCCL.H - Header file for the CL layer of the GCF Communication Server.
**
** Description:
**	This module contains all data structure definitions required by the
**	interface to the CL routines for the GCF communication server.
**
** History: $Log-for RCS$
**      24-Feb-88 (jbowers)
**          Initial module implementation.
**	18-Jun-89 (jorge) 
**	    changed the UNIX TCP Net Software Designator to TCP_IP
**      24-Mar-89 (RBP)
**	    incrased gcc_l_pct from 1 to 3 for more protocols.
**	05-Jan-90 (seiwald)
**	    Added state flag for GCC_P_PLIST, so a protocol driver can
**	    maintain some state before allocating its own pcb.
**	08-Feb-90 (seiwald)
**	    Added lsn_port to the open parms of the GCC_P_PLIST, so
**	    the protocol driver can report the actual port on which listens.
**	12-Feb-90 (cmorris)
**	    Added function field to GCC_P_PLIST to allow callback to
**	    determine which operation was originally invoked.
**	06-Mar-90 (seiwald)
**	    Removed GCcstart and callback list operations of GCcinit
**	    and GCcterm, which IIGCC now handles.
**	13-Mar-90 (seiwald)
**	    Reorganized and documented GCC_P_PLIST.
**	8-Oct-90 (jkb)
**	    Add #ifndef GCCCL_H_INCLUDED so file is not included twice
**	16-Jan-91 (seiwald)
**	    Pulled in better commented GCC_PCE definition from the CL spec.
**  	30-Jan-91 (cmorris)
**  	    Added token for ASYNC protocol.
**	27-Mar-91 (seiwald)
**	    Prototyped function pointers in GCC_P_PLIST and GCC_PCE.
**	19-Apr-91 (seiwald)
**	    Jacked up GCC_L_PCT to 5; this symbol isn't long for the world.
**	14-Jan-92 (seiwald)
**	    Support for per-connection packet size.
**	27-Jan-92 (sweeney)
**	    Bumped GCC_L_PCT (again) for osi-oslan and osi-x25
**	30-Jan-92 (sweeney)
**	    Added protocol identifier defines for osi-oslan and osi-x25.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
**  	03-Apr-92 (cmorris)
**  	    Made protocol identifier for LU6.2 consistent with the
**  	    other SNA identifiers.
**	06-Apr-92 (seiwald)
**	    Last change was half integrated - fix define for SNA_LU62.
**  	24-Feb-93 (cmorris)
**  	    Make GCC_L_PCT match the value that we're initializing
**  	    the no. of table entries to in gccptbl.h.
**  	20-Sep-93 (brucek)
**  	    Moved from cl/hdr/hdr to gl/hdr/hdr (no system dependencies).
**  	28-Sep-93 (cmorris)
**  	    Added GCC_FLUSH_DATA option to GCC_SEND function to allow
**  	    protocol drivers to potentially optimise sequences of send
**          functions, particularly in a half-duplex environment such as
**  	    LU6.2.
**      17-Mar-97 (tlong)
**          Added constants to support dual (pseudo-duplex) conversations
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Nov_2002 (wansh01) 
**          Changed GCC_P_LIST. removed option GCC_EXP_DATA. added
**	    option GCC_LOCAL_CONNECT to indicate local connection.
**	 6-Jun-03 (gordy)
**	    Added local/remote connection addresses output to 
**	    CONNECT/LISTEN parms.  Removed unused pkt_sz.
**  29-Jan-2004 (fanra01)
**      SIR 111718
**      Add function prototype GCpportaddr and GCpaddrlist.
**     06-Feb-2007 (Ralph Loen) SIR 117590
**          Marked the "node_id" field as unused.
**	4-Sep-2008 (lunbr01) Bug/Sir 119985
**	    Update PCT structure doc as part of bigger change to make
**	    "tcp_ip" the default network protocol instead of "wintcp".
**      02-Dec-09 (rajus01) Bug: 120552, SD issue:129268
**	    Added new output field named "actual_port_id" (char *) to the 
**	    "open" structure field of the function_parms (per function 
**	    union structure) of the "GCC_P_PLIST" parameter list structure. 
**	    Also fixed the GCpportaddr() prototype.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add timeout (in milliseconds) to parmlist plus associated PCE
**	    option flag to enable it.
**	    Add PCE option flag to tell protocol driver to skip callback
**	    if request completed immediately.
*/


/*
**  Forward and/or External typedef/struct references.
*/
# ifndef        GCCCL_H_INCLUDED
# define        GCCCL_H_INCLUDED

typedef struct _GCC_ADDR	GCC_ADDR;
typedef struct _GCC_P_PLIST	GCC_P_PLIST;
typedef struct _GCC_PCE		GCC_PCE;
typedef struct _GCC_PCT		GCC_PCT;


/*
**  Defines of other constants.
*/

# define    GCC_L_PROTOCOL  	16	/* Length of a protocol ID */
# define    GCC_L_NODE		64	/* Length of a node name */
# define    GCC_L_PORT		64	/* Length of a port name */

/*
**      Following are the function request codes for the invocation of all
**      protocol handler routines.
*/

#define                 GCC_OPEN	 1
#define                 GCC_LISTEN       2
#define                 GCC_CONNECT      3
#define                 GCC_SEND         4
#define                 GCC_RECEIVE      5
#define                 GCC_DISCONNECT   6

/*
**      The following are the values which "generic_status"
**	in GCC_P_PLIST may take.
*/
# define    GC_CONNECT_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0081)
# define    GC_DISCONNECT_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0082)
# define    GC_SEND_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0083)
# define    GC_RECEIVE_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0084)
# define    GC_LISTEN_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0085)
# define    GC_PINIT_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0086)
# define    GC_OPEN_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0087)
# define    GC_NTWK_STATUS	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0088)

/*
**      Definitions of protocol identifiers 
**	Use TCP_IP instead of TCP_xxx - it's all TCP/IP!
*/

# define		DECNET		    "DECNET"
# define		SNA_LU0		    "SNA_LU0"
# define		SNA_LU2		    "SNA_LU2"
# define		SNA_LU62	    "SNA_LU62"
# define		TCP_UNIX            "TCP_IP"
# define		TCP_DEC		    "TCP_DEC"
# define		TCP_WOLLONGONG	    "TCP_WOL"
# define		TCP_IP		    "TCP_IP"
# define    	    	ISO_OSLAN	    "ISO_OSLAN"
# define    	    	ISO_X25	    	    "ISO_X25"
# define    	    	ASYNC	    	    "ASYNC"

/*
**      The following are miscellaneous constants.
*/

/*
**      These constants are related to the SNA LU 6.2
**      pseudo duplex conversation implementation.
*/

#define GC_LU62_VERSION_1      1
#define GC_LU62_VERSION_2      2

#define GC_INITIAL_SEND_LENGTH 1

#define GC_ATTN  0377
#define NOT_DUP  0
#define PEND_DUP 1
#define DUPLEX   2


/*
** Name: GCC_ADDR
**
** Description:
**	Contains the node and port identifiers for a network 
**	communications end-point.
*/

struct _GCC_ADDR
{
    char	node_id[ GCC_L_NODE + 1 ];
    char	port_id[ GCC_L_PORT + 1 ];
};


/*}
** Name: GCC_P_PLIST - Parameter list for all GCC protocol handler routnes
**
** Description:
**	GCC_P_PLIST is the structure of the parameter list which is passed to
**	all protocol handler routines used in the GCF communiction server.
**	Following are the definitions of the parm list elements:
**
** Inputs:
**	pcb				Protocol control block
**					(SEND, RECEIVE, DISCONNECT)
**
**	compl_exit			Routine to call on operation completion
**
**	comp_id				Argument to completion routine
**
**	options				Flagword for operation options:
**	    GCC_EXP_DATA		Expedited data (unsupported)
**  	    GCC_FLUSH_DATA              Flush data (indicates whether or
**  	    	    	    	    	this is the last of a sequence
**  	    	    	    	    	of GCC_SEND functions. This allows
**  	    	    	    	    	the potential for a protocol driver
**  	    	    	    	    	to optimize, particularly where a
**  	    	    	    	    	half-duplex protocol - such as LU6.2 -
**  	    	    	    	    	is being supported.)
**					
**	function_invoked		Protocol function code
**					Duplicates first argument to protocol
**					driver.  Values defined above are:
**					GCC_OPEN
**					GCC_LISTEN   
**					GCC_CONNECT   
**					GCC_SEND       
**					GCC_RECEIVE     
**					GCC_DISCONNECT   
**
**	buffer_ptr			MDE data area
**					used for SEND, RECEIVE
**					optional for LISTEN, CONNECT, DISCONN
**
**	buffer_lng			MDE data area length
**					used for SEND, RECEIVE
**					0 for LISTEN, CONNECT, DISCONN
**
**	function_parms			per-function parameters
**
**
** Outputs:
**	generic_status			error STATUS
**
**	system_status			error CL_ERR_DESC
**
**	pcb				Protocol control block
**					(LISTEN, CONNECT)
**
**	buffer_lng			received packet length (RECEIVE)
**					
** Protocol Private Data:
**	state				State of current operation
**
** History:
**	13-Mar-90 (seiwald)
**	    Reorganised and documented.
**	10-Apr-90 (seiwald)
**	    Added pce, to allow protocol driver to access pce.
**	14-Jan-92 (seiwald)
**	    New output pkt_sz for GCC_CONNECT and GCC_LISTEN, indicating
**	    the packet size as negotiated by the protocol driver.
**	11-aug-93 (ed)
**	    changed CL_HAS_PROTOS to CL_PROTOTYPED
**      12-nov-93 (robf)
**          Add node_id to GCC_LISTEN parms union. This is an optional
**          field which if set contains the remote node name.
**	04-Nov-2002 (wansh01) 
**	    Removed option GCC_EXP_DATA. added option
**	    GCC_LOCAL_CONNECT to indicate local connection.
**	 6-Jun-03 (gordy)
**	    Added local/remote connection addresses output to 
**	    CONNECT/LISTEN parms.  Removed unused pkt_sz.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add timeout (in milliseconds) to parmlist.
*/

struct _GCC_P_PLIST
{
    PTR             pcb;		/* Protocol control block */
    GCC_PCE	    *pce;		/* Protocol control entry */

    VOID            (*compl_exit)( 
# ifdef CL_PROTOTYPED
				    PTR compl_id 
# endif 
);

    PTR             compl_id;           /* Argument to completion routine */
    u_i4	    options;            /* Flagword for operation options */

# define	GCC_LOCAL_CONNECT   0x01	/* connection is local */
# define    	GCC_FLUSH_DATA	    0x02    	/* flush */
    
    i4		    function_invoked;	/* Protocol function code */
    i4		    state;		/* State of current operation */

    char	    *buffer_ptr;	/* MDE data area */
    i4		    buffer_lng;		/* MDE data area length */

    union	/* per function parameters */
    {
	struct	/* GCC_OPEN */
	{
	    char	*port_id;		/* Input: port (symbolic)  */
	    char	*lsn_port;		/* Output: port (actual) */
	    /*
	    ** For logging purpose the "actual_port_id" field has been added 
	    ** to return the actual symbolic port for the equivalent actual 
	    ** port returned in "lsn_port" field.
	    */ 
	    char	*actual_port_id;    /* Output: actual port (symbolic)*/
	} open;

	struct	/* GCC_CONNECT */
	{
	    char	*port_id;		/* Input: target port */
	    char	*node_id;		/* Input: node identifier */
	    GCC_ADDR	*lcl_addr;		/* Output: local address */
	    GCC_ADDR	*rmt_addr;		/* Output: remote address */
	} connect;

	struct	/* GCC_LISTEN */
	{
	    char	*port_id;		/* Input: listen port */
            /*
            **  The "node_id" field is currently ununsed, and should always
            **  be set to NULL when performing network listens.  Previously
            **  this field was set via a call to gethostbyaddr() to support
            **  security labels, but security labels are not currently used
            **  and gethostbyaddr() may slow performance significantly.
            */
            char        *node_id;               /* Output: source node */
	    GCC_ADDR	*lcl_addr;		/* Output: local address */
	    GCC_ADDR	*rmt_addr;		/* Output: remote address */
	} listen;

    } function_parms;	  

    STATUS          generic_status;     /* Status of operation execution */
    CL_SYS_ERR      system_status;      /* System-specific status code */
    i4              timeout;            /* Timeout in ms */
};


/*}
** Name: GCC_PCE - Protocol Control Table Element
**
** Description:
**      This structure defines the repeated entries in the PCT,
**	which is passed from the CL to GCC by GCpinit.  Each entry 
**	in the PCT describes a single protocol driver supported by
**	the GCC CL.  The members are:
**
**	pce_pid			Protocol ID
**				e.g. tcp_ip, decnet
**
**	pce_port		Default Port ID
**				GCC hands this value to the protocol
**				driver as the port_id parameter to
**				GCC_OPEN.
**
**	pce_protocol_rtne	Protocol driver entry point.
**
**	pce_pkt_sz		Packet size
**				This size should be the maximum (or
**				optimal) protocol message size, minus
**				the size of the NPCI (protocol specific 
**				header).
**
**	pce_exp_pkt_sz		Packet size for expedited data.
**				Unsupported.
**
**	pce_options		Flagword for protocol options:
**	    PCT_VAR_PKT_SZ	Per conn packet size (unsupported)
**	    PCT_ENABLE_TIMEOUTS Timeout field in parmlist is enabled
**	    PCT_SKIP_CALLBK_IF_DONE Skip callback if done on initial call
**
**	pce_flags		Flagword for runtime state:
**	    PCT_NO_PORT		No listen port
**	    PCT_NO_OUT		No outbound connections
**	    PCT_OPN_FAIL	Open failed
**	    PCT_OPEN		Open successful
**
**	pce_driver		Ptr to driver-specific info & functions
**
**	pce_dcb			Ptr to driver-specific control block
**
** History:
**      23-Feb-88 (jbowers)
**          Initial structure implementation
**	10-Apr-90 (seiwald)
**	    New pce_driver pointer, so a generic protocol driver given
**	    by pce_protocol_rtne can handle multiple protocols.
**	20-Jun-90 (seiwald)
**	    Extended the per-driver private area to include both the
**	    driver (pce_driver, nee pce_private) and the control block
**	    (pce_dcb).
**	14-Jan-92 (seiwald)
**	    New option PCT_VAR_PKT_SZ, indicating that the packet size
**	    will be negotiated by the protocol on each GCC_CONNECT and
**	    GCC_LISTEN, overridding pce_pkt_sz.
**	4-Sep-2008 (lunbr01) Bug/Sir 119985
**	    Update PCT structure doc as part of bigger change to make
**	    "tcp_ip" the default network protocol instead of "wintcp".
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add PCE option PCT_ENABLE_TIMEOUTS for backward compatibility
**	    with use of drivers from GCC TL, which doesn't use or set the
**	    timeout field.  GCA CL uses the timeout field.
**	    Add PCE option PCT_SKIP_CALLBK_IF_DONE to indicate support
**	    for immediate request completion (without callback).
**	    This is used by tcp_ip driver on Windows to return results
**	    of requests from GCA CL if operation completed immediately
**	    and skip the subsequent callback.
*/

struct _GCC_PCE 
{
    char		pce_pid[GCC_L_PROTOCOL];    
				/* Protocol ID
				A null-terminated string corresponding
				to the "protocol" component of the 
				"protocol, host, port" triple entered
				for a vnode via NETU.  See Protocol
				Identifiers listed above. */

    char		pce_port[GCC_L_PORT];
				/* Default Port ID
				GCC hands this value to the protocol
				driver as the port_id parameter to
				GCC_OPEN. */

    STATUS		(*pce_protocol_rtne)( 
# ifdef CL_PROTOTYPED
				i4 func_code,
				GCC_P_PLIST *parms 
# endif
);
				/* Protocol driver entry point. */

    i4		pce_pkt_sz;
				/* Packet size
				This size should be the maximum (or
				optimal) protocol message size, minus
				the size of the NPCI (protocol specific 
				header). */

    i4		pce_exp_pkt_sz;	
				/* Packet size for expedited data.
				Unsupported. */

    u_i4		pce_options;
				/* Flagword for protocol options. */

#define			PCT_VAR_PKT_SZ      0x0001  /* Per conn packet size.
						       Unsupported.  */
#define			PCT_ENABLE_TIMEOUTS 0x0002  /* Use parmlist timeout */
#define			PCT_SKIP_CALLBK_IF_DONE 0x0004 /* if IO (request) done,
					         skip callback to compl exit */

    u_i4		pce_flags;			    
				/* Flagword for runtime state: */

				/* GCC CL Interface Flags */

#define			PCT_NO_PORT	    0x0001  /* No listen port */
#define			PCT_NO_OUT	    0x0002  /* No outbound conn's */

				/* GCC Private Flags. */

#define			PCT_OPN_FAIL	    0x0004  /* Open failed */
#define			PCT_OPEN	    0x0008  /* Open successful */

    PTR			pce_driver;		    
				/* driver specific (private to driver) */

    PTR			pce_dcb;		    
				/* driver control block (private to driver) */
};

/*}
** Name: GCC_PCT - Protocol Control Table (PCT)
**
** Description:
**	An array of GCC_PCE entries.  This structure is handed to GCC
**	by GCpinit, to enumerate the supported protocols.
**
** History:
**      24-Feb-88 (jbowers)
**          Initial structure implementation.
**      14-mar-96 (chech02)
**          changed GCC_L_PCT to 10 for win 3.1 port.
*/

# define    GCC_L_PCT		10	/* Number of entries in the PCT */

struct _GCC_PCT
{
    i4              pct_n_entries;		/* Number of table entries */
    GCC_PCE         pct_pce[GCC_L_PCT];		/* Protocol control entry */
};  /* end GCC_PCT */

/*
** Function definitions.
*/

FUNC_EXTERN STATUS	GCcinit( STATUS *generic_status, 
				 CL_ERR_DESC *system_status );

FUNC_EXTERN STATUS	GCcterm( STATUS *generic_status, 
				 CL_ERR_DESC *system_status );

FUNC_EXTERN STATUS	GCpinit( GCC_PCT **pct, STATUS *generic_status, 
				 CL_ERR_DESC *system_status );

FUNC_EXTERN STATUS	GCpterm( GCC_PCT *pct, STATUS *generic_status, 
				 CL_ERR_DESC *system_status );

FUNC_EXTERN STATUS	GCpdrvr( i4  func_code, GCC_P_PLIST *parm_list );

FUNC_EXTERN STATUS GCpportaddr( char* protname, i4 addrlen, char* portaddr, char *port_out_symbolic );

FUNC_EXTERN STATUS GCpaddrlist( i4* entry, i4 addrlen, char* portaddr );

#endif /* GCCCL_H_INCLUDED */
