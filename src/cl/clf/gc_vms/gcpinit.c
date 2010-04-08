/*
** Copyright (c) 1987, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include       <cv.h>
# include       <er.h>
# include	<me.h>
# include       <nm.h>
# include       <pm.h>
# include	<st.h>
# include	<qu.h>
# include	<gcccl.h>

# include	<bsi.h>

/*
**
**
**  History:
**      12-Feb-88 (jbowers)
**          Initial module implementation
**	18-June-1989 (jorge)
**	    Fixed Acvio in case were II_GCCBE_TCP_WOL is not defined.
**	18-June-1989 (jorge)
**	    upgraded the Wollongon symbolic port mapping to conform with NCC
**	22-June-1989 (jorge)
**	    Made the use of the Wol internet address syntax more consistent
**	10-apr-91 (seiwald)
**	    Moved GCC_PCT in from gcpinitr.roc.  
**	    Added support for BS drivers.
**	    Stripped the Wollongong specific port hocus focus.
**	    Use literals for driver names.
**	    Added DEC TCP driver.
**	15-Oct-1991 (hasty)
**	    Added protocol entries for dec's tcp/ip and changed snalu0
**	    protocol entry to point to new snalu0 driver.
**	09-Jan-1992 (alan)
**	    Added Blast async protocol entry.
**	31-Jan-92 (alan)
**	    Corrected same.
**	3-Mar-92 (seiwald)
**	    Reverted SNA_LU0 to be a non-BS protocol driver.
**	8-Mar-93 (edg)
**	    Use PMget to get listen addr.  
**      16-jul-93 (ed)
**	    added gl.h
**      05-feb-1999 (loera01) 
**          Changed the default listen address of DECNet to II instead of
**          II_GCC_0.  Otherwise, GCbssm() thinks the Comm Server is the
**          Bridge Server.  The result is that the Comm Server never
**          configures its listen address properly.  II is a valid default
**          listen address for DECnet--it gets converted to II_GCCII_0
**          before the GCC does a listen.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	26-oct-2001 (kinte01)
**	   Updated compiler warnings
**	10-oct-2003 (abbjo03)
**	    Replace obsolete EXTERN by GLOBALREF.
**      16-Jan-2007 (Ralph Loen) SIR 116548
**         In GCpinit(), revert TCP to IPv4 instead of IPv6 if 
**         II_TCPIP_VERSION is set to 4.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
*/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN STATUS   GCsnalu0();
FUNC_EXTERN STATUS   GCqiobs();
FUNC_EXTERN STATUS   GCqiobk();
FUNC_EXTERN STATUS   GCblast();

GLOBALREF BS_DRIVER GC_woltcp;
GLOBALREF BS_DRIVER GC_dectcp;       
GLOBALREF BS_DRIVER GC_dectcp_v6;       
GLOBALREF BS_DRIVER GC_decnet;

/*
**  Global variable definitions.
*/
	
static   GCC_PCT	IIGCc_pct =
{
  5,
   {
    {"SNA_LU0",		"0",  	    GCsnalu0,  4103,  0, 0, 0 },
    {"DECNET",   	"II", 	    GCqiobk,  16384,  0, 0, 0, (PTR)&GC_decnet},
    {"TCP_DEC",		"II",	    GCqiobs,   2048,  0, 0, 0, (PTR)&GC_dectcp_v6},
    {"TCP_WOL", 	"II",  	    GCqiobs,   2048,  0, 0, 0, (PTR)&GC_woltcp},
    {"ASYNC",		"",	    GCblast,   2048,  0, 0, 
						PCT_NO_OUT | PCT_NO_PORT }
   }
};

/*
**  Forward and/or External function references.
*/


/*{
** Name: GCpinit	- Protocol initialization routine
**
** Description:
**      This routine is called during the communication server initialization 
**      process.  It is guaranteed to be called before any protocol driver
**      functions are invoked.  It is responsible for allocating and  
**      initializing the Protocol Control Table (PCT), and for performing any 
**      other initialization which may be required in a specific environment. 
** 
**      The PCT is described in detail in the header file GCCCL.H.  It has an 
**      entry for each supported protocol implementation.  Entries normally, 
**      although not necessarily, correspond one-for-one to the collection of
**	protocol handler routines.
**
** Inputs:
**      none
**
** Outputs:
**      pct				Pointer to an allocated and filled-in
**					Protocol Contro Table (PCT).
**	generic_status			System-independent status (output)
**	system_status			Environment-specific status (output)
**
**	Returns:
**	    STATUS:
**		OK
**		FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-Feb-88 (jbowers)
**          Initial function implementation
**      21-dec-88 (ham)
**	    Took GCpinit out of GCCCL.  Added protocol array.
**      08-Jun-88 (ham)
**	    Allow user to set node_name for listen addres for Wollongng
**	    TCP.
*/

STATUS
GCpinit( pct, generic_status, system_status )
GCC_PCT		**pct;
STATUS		*generic_status;
CL_ERR_DESC	*system_status;
{
    char		pmsym[ 128 ];
    char		*symval;
    int			i;
    char                *ptr;
    i4        tcpip_version=0;

    CL_CLEAR_ERR(system_status);
    *pct = &IIGCc_pct;
    /*
    **  Check to see if driver being restricted to IPv4.  If IPv4 is specified,
    **  use only IPv4 routines.
    */
    NMgtAt( "II_TCPIP_VERSION", &ptr );
    if ( !(ptr && *ptr) && PMget("!.tcp_ip.version", &ptr) != OK )
        tcpip_version = 0;  /* Default is both ipv6 and ipv4 ("all") */
    else 
    {
        CVal(ptr, &tcpip_version);     /* 4=IPv4 or 6=IPv6 */
        if (tcpip_version == 4)
           IIGCc_pct.pct_pce[2].pce_driver = (PTR)&GC_dectcp;
    }

    /*
    ** Set the listen_id for each protocol
    */
    for( i = 0; i < IIGCc_pct.pct_n_entries; i++) 
    {
	GCC_PCE	*pce = IIGCc_pct.pct_pce + i;

	STprintf( pmsym, "!.%s.port", pce->pce_pid );

	if ( PMget( pmsym, &symval ) != OK )
	    continue;

	STcopy( symval, pce->pce_port );
    }

    return OK;
}
