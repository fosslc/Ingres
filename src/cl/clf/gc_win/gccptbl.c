/*
**  Copyright (c) 1995, 2008 Ingres Corporation
*/

#include	<compat.h>
#include	<winsock.h>
#include	<clconfig.h>
#include	<gcccl.h>
#include	<qu.h>
#include	"gcclocal.h"

/*} 
** Protocol setup table
**
**	char		pce_pid[GCC_L_PROTOCOL];    Protocol ID
**	char		pce_port[GCC_L_PORT];	    Port ID
**	STATUS		(*pce_protocol_rtne)();	    Protocol handler
**	i4     		pce_pkt_sz;		    Packet size
**	i4     		pce_exp_pkt_sz;		    Exp. Packet size
**	u_i4 		pce_options;		    Protocol options
**	u_i4 		pce_flags;		    flags
**	PTR             pce_driver;                 handed to prot driver
**	PTR             pce_dcb;                    driver control block
**  	    	    	    	    	    	     (private to driver)
**
LIBRARY = IMPCOMPATLIBDATA
**
** History:
**	03-Nov-93 (edg)
**	    Taken from UNIX version for use with NT.
**	12-Nov-93 (edg)
**	    Added lan manager netbios to table.
**	01-Dec-93 (edg)
**	    Added Novel SPX.
**	10-jan-94 (edg)
**	    Added include <qu.h>
**      12-Dec-95 (fanra01)
**          Extracted data for DLLs on windows NT.
**	24-apr-96 (tutto01)
**	    Added support for DECnet.
**	09-may-1996 (canor01)
**	    Define IIGCc_prot_exit array of protocol exit functions.
**	01-jul-97 (somsa01)
**	    Removed DEcnet stuff. We do not need this anymore, since DECnet is
**	    contained in its own DLL, and it is loaded in at ingstart time.
**	29-sep-2003 (somsa01)
**	    Added entries for tcp_ip protocol driver.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to have this file as part of IMPCOMPATLIBDATA
**      26-Aug-2008 (lunbr01) Bug/Sir 119985
**          Change the default protocol from "wintcp" to "tcp_ip" by
**          moving "tcp_ip" to the top of the table. Also, correct pce 
**          table structure comments above.
*/

GLOBALREF	GCC_PCT		IIGCc_gcc_pct;

/*
**  NOTE! Keep in same order as IIGCc_gcc_pct in gcdata.c!
*/

GLOBALDEF	STATUS		(*IIGCc_prot_init[8])() =
{
	GCwinsock2_init,
	GCwinsock_init,
	GClanman_init,
	GCwinsock_init,
	NULL,
	NULL,
	NULL,
	NULL
};

GLOBALDEF	STATUS		(*IIGCc_prot_exit[8])() =
{
	GCwinsock2_exit,
	GCwinsock_exit,
	GClanman_exit,
	GCwinsock_exit,
	NULL,
	NULL,
	NULL,
	NULL
};
