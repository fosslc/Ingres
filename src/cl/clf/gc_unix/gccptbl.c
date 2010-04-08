/*
**Copyright (c) 2006 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	<gcccl.h>
#include	<bsi.h>

FUNC_EXTERN	STATUS	GCbs();
FUNC_EXTERN	STATUS	GCbk();

# ifdef xCL_BLAST_EXISTS
FUNC_EXTERN 	STATUS  GCblast();
# endif

# ifdef xCL_019_TCPSOCKETS_EXIST
GLOBALREF	BS_DRIVER BS_socktcp;
# endif

# ifdef xCL_TCPSOCKETS_IPV6_EXIST
GLOBALREF	BS_DRIVER BS_socktcp6;
# endif

# ifdef xCL_TLI_TCP_EXISTS
GLOBALREF	BS_DRIVER BS_tlitcp;
# endif

# ifdef xCL_TCPSOCKETS_IPV6_EXIST
#define	TCP6_NAME	TCP_IP
#define	TCP_NAME	"TCP_IPV4"
#define	TCPTLI_NAME	"TLI_TCP_IP"
# else
# ifdef xCL_TLI_TCP_EXISTS
#define	TCPTLI_NAME	TCP_IP
#define	TCP_NAME	"SOCKETS"
# else
# ifdef xCL_019_TCPSOCKETS_EXIST
#define	TCP_NAME	TCP_IP
# endif
# endif
# endif

# ifdef	xCL_041_DECNET_EXISTS
GLOBALREF	BS_DRIVER BS_sockdnet;
# endif

# if defined(xCL_TLI_OSLAN_EXISTS) || defined (xCL_TLI_X25_EXISTS)
GLOBALREF       BS_DRIVER BS_tliosi;
# endif

# ifdef xCL_SUN_LU62_EXISTS
FUNC_EXTERN     STATUS  GClu62();
# endif

# if defined(xCL_022_AIX_LU6_2)
FUNC_EXTERN   STATUS  GClu62();
# endif

# ifdef xCL_HP_LU62_EXISTS
FUNC_EXTERN   STATUS  GClu62();
#endif

# ifdef xCL_TLI_SPX_EXISTS
GLOBALREF	BS_DRIVER BS_tlispx;
# endif

/*} 
** Protocol setup table
**
**	char		pce_pid[GCC_L_PROTOCOL];    Protocol ID
**	char		pce_port[GCC_L_PORT];	    Port ID
**	STATUS		(*pce_protocol_rtne)();	    Protocol handler
**	i4		pce_pkt_sz;		    Packet size
**	i4		pce_exp_pkt_sz;		    Exp. Packet size
**	u_i4		pce_options;		    Protocol options
**	u_i4		pce_flags;		    flags
**	PTR             pce_driver;                 handed to prot driver
**  	PTR 	    	pce_dcb;    	    	    driver control block
**  	    	    	    	    	    	     (private to driver)
**
** History:
**	23-jun-89 (seiwald)
**	    Changed default port from 134 to 0.  It is ignored.
**	29-jun-89 (seiwald)
**	    Changed default port from 0 to II.  It is used as a last resort,
**	    if no environment variables (II_GCCxx_TCP_UNIX, II_INSTALLATION)
**          are set.
**	06-Feb-90 (seiwald)
**	    Removed GCC_PROTO structure, which duplicated GCC_PCT.
**	    Protocol table is declared directly as GCC_PCT.
**	13-Apr-90 (seiwald)
**	    Replaced GCtcp_ip driver with generic GCbs driver.
**	26-May-90 (seiwald)
**	    Added TLI for TCP/IP.
**	30-May-90 (seiwald)
**	    Terminate list with null pce_pid.
**	10-Apr-91 (seiwald)
**	    Added DECNET.  Use literals instead of defined constants,
**	    which just confused things.
**	21-mar-91 (seng)
**	    Add opening and closing braces to array initializer.  Some
**	    compilers complain about number of initializers out numbering
**	    the members of the struct.
**	09-apr-91 (gautam)
**	    Correct DECNET protocol driver entry.
**	23-apr-91 (gautam)
**	    Rename DECNET driver as BS_sockdnet
**  	13-Aug-91 (cmorris)
**  	    Don't advertise an expedited message size when we
**  	    don't even support expedited data!!
**  	12-Dec-91 (cmorris)
**  	    Added Async protocol entry (disabled for GCC).
**      28-Jan-91 (sweeney)
**	    Added OSI protocol entries.
**      30-Jan-91 (sweeney)
**	    increased (potential) max size of table in the unlikely
**	    event we configure them all.
**	30-Jan-92 (seiwald)
**	    Renamed GCasync to GCblast.
**	11-Feb-92 (sweeney)
**	    [ swm history comment for sweeney change ]
**	    To be conistent with other protocol names used underbar (_) as
**	    a separator rather than dash (-) for "ISO_X25" and "ISO_OSLAN".
**  	31-Mar-92 (gautam)
**          Added in entry for RS/6000's LU6.2 Ingres/NET support
**  	03-Apr-92 (cmorris)
** 	    Added entry for LU6.2.
**  	10-Apr-92 (GordonW)
**	    Added ifdef around BLAST external.
**	    Bump TCP and  TLI buffers up to the local GCA limit.
**	    There is no reason to go beyond that point. Why do other
**	    protocols do this?
**	    Fix setup where we have both Sockets and TLI, We default TLI
**	    as TCP_IP and Sockets to "SOCKETS".
**	04-May-92 (GordonW)
**	    Put back the default buffer sizes until more data to gotten on
**	    as to what the 'correct' size is.
**  	30-Apr-92 (gautam)
** 	    Change SNA_62 to SNA_LU62 for the IBM RS/6000 entry
**  	14-Sep-92 (seiwald)
**	    Added SPX entry.
**  	08-Jan-93 (cmorris)
**  	    Bump up buffer size for TCP/IP to 4096 (see 11-Apr-92 and
**  	    04-May-92 entries above...) to match local GCA buffer size.
**  	    Reduce DECNET buffer size to 8192 (as no TPDU can ever be
**  	    larger than this size). Leave Async, LU6.2 and SPX at lower
**  	    limits for now. Async is only used from the PC (which uses
**  	    smaller buffers), LU6.2 only to MVS (which currently
**  	    supports only "jumbo" 1K TPDUs), and SPX (which purportedly
**  	    has problems with bigger buffers). Note: The concatenation
**  	    of buffers within the PL might mean it makes sense to have
**  	    protocol buffer sizes _larger_ than the size provided by the
**  	    local GCA. This requires further investigation.
**  	28-Jan-93 (cmorris)
**  	    Added entry for HP UX version of LU6.2; corrected number of
**  	    entries in Async and AIX LU6.2 entry.
**  	    Made description of protocol control table entry consistent
**  	    with its definition in gcccl.h.
**  	11-Mar-93 (cmorris)
**  	    Bump up LU6.2 buffer size to 4096. If and when the MVS codeline
**  	    is updated to a newer base, this will take immediate effect.
**  	    Same change for SPX.
**      20-May-93 (edg)
**	    Bug fix 45611.  Our DECNET default buffer size of 16k was causing
**	    problems with sql copy command.  Apparently an 8k buffer size was
**	    being negotiated and when the DBMS sent across a tble descriptor
**	    causing a > 4096 buffer to be sent, a decnet write fails.  Setting
**	    a lower, 4096, default fixes it.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	05-feb-1999 (loera01) Bug 95164
**	    Changed the default listen address of DECNet to II instead of 
**	    II_GCC_0.  Otherwise, GCbssm() thinks the Comm Server is the
**	    Bridge Server.  The result is that the Comm Server never 
**	    configures its listen address properly.  II is a valid default 
**	    listen address for DECnet--it gets converted to II_GCCII_0 
**	    before the GCC does a listen.
**	07-oct-2006 (lunbr01)  Sir 116548
**	    Add new sockets TCP/IP driver that supports both IPv4 and IPv6
**	    and make it the default (protocol = "tcp_ip"), if it exists on 
**	    the platform.  Note that it supercedes both the old sockets 
**	    TCP driver and the TLI driver (the latter used to be the
**	    default if it existed...can now be accessed as "TLI_TCP_IP" ->
**	    same as local IPC).  The old sockets TCP/IP driver is
**	    left in the table as "tcp_ipv4".  For backwards compatibility
**	    for users on TLI platforms who specifically configured for
**	    "SOCKETS", this protocol string will still work but points
**	    to the new IPv6-enabled sockets driver.
*/

GLOBALDEF	GCC_PCT		IIGCc_gcc_pct =
{
	8,
  {
# ifdef xCL_TCPSOCKETS_IPV6_EXIST
	/* TCP/IP for IPv4 + IPv6 via BSD sockets */
	{TCP6_NAME, "II", GCbs, 4096, 0, 0, 0, (PTR)&BS_socktcp6, (PTR)0},
#   ifdef xCL_TLI_TCP_EXISTS
	{"SOCKETS", "II", GCbs, 4096, 0, 0, 0, (PTR)&BS_socktcp6, (PTR)0},
#   endif /* xCL_TLI_TCP_EXISTS */
# endif /* xCL_TCPSOCKETS_IPV6_EXIST */

# ifdef xCL_019_TCPSOCKETS_EXIST
	/* TCP/IP via BSD sockets */
	{TCP_NAME, "II", GCbs, 4096, 0, 0, 0, (PTR)&BS_socktcp, (PTR)0},
# endif /* xCL_019_TCPSOCKETS_EXIST */

# ifdef xCL_TLI_TCP_EXISTS
	/* TCP/IP via TLI */
	{TCPTLI_NAME, "II", GCbs, 4096, 0, 0, 0, (PTR)&BS_tlitcp, (PTR)0},
# endif /* xCL_TLI_TCP_EXISTS */

# ifdef	xCL_041_DECNET_EXISTS
	/* DECNET */
	{DECNET,"II",GCbk, 4096, 0,0, 0, (PTR)&BS_sockdnet,(PTR)0},
# endif

# ifdef xCL_TLI_OSLAN_EXISTS
        {ISO_OSLAN, "OSLAN_II", GCbk, 8192, 0,0,0, (PTR)&BS_tliosi, (PTR)NULL},
#endif

# ifdef xCL_TLI_X25_EXISTS
        {ISO_X25, "X25_II", GCbk, 8192, 0, 0, 0, (PTR)&BS_tliosi, (PTR)NULL},
#endif

# ifdef xCL_BLAST_EXISTS
        {ASYNC, "", GCblast, 2048, 0, 0, PCT_NO_OUT | PCT_NO_PORT, (PTR) 0,
    	 (PTR) NULL},
# endif

# ifdef xCL_SUN_LU62_EXISTS
        {SNA_LU62, "", GClu62, 4096, 0, 0, 0, (PTR) 0, (PTR) NULL},
# endif

#ifdef  xCL_022_AIX_LU6_2
        {SNA_LU62, "", GClu62, 4096, 0, 0, 0, (PTR)0, (PTR) NULL},
#endif

# ifdef xCL_HP_LU62_EXISTS
    	{SNA_LU62, "", GClu62, 4096, 0, 0, 0, (PTR) 0, (PTR) NULL},
#endif

# ifdef xCL_TLI_SPX_EXISTS
	{"SPX", "8265", GCbk, 4096, 0, 0, 0, (PTR)&BS_tlispx, (PTR)NULL },
# endif 

	{ "", "", 0, 0, 0, 0, 0 , (PTR)NULL, (PTR)NULL}
   }
};
