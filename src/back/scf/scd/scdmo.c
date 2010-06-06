/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>

#include    <cm.h>
#include    <cv.h>
#include    <er.h>
#include    <ex.h>
#include    <cs.h>
#include    <me.h>
#include    <sp.h>
#include    <mo.h>
#include    <pc.h>
#include    <tm.h>
#include    <tr.h>

#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>

#include    <lk.h>

#include    <adf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <scf.h>
#include    <gca.h>
#include    <duf.h>

#include    <ddb.h>
#include    <qefrcb.h>
#include    <psfparse.h>

#include    <dudbms.h>
#include    <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scserver.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>

#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>

/*
** scdmo.c -- experimental MO module for SCD.
**
** Description
**
**	Defines per-server objects for SCD, and necessary
**	MO access methods.
**
** Functions:
** 
**		scd_mo_init - set up SCF MO classes.
**
**		scd_listen_get - facility name decoder
**		scd_state_get - state decoder
**		scd_shutdown_get - shutdown state decoder
**		scd_cpabilities_get - capabilities decoder
**
**		scd_crash_set
**		scd_close_set
**		scd_open_set
**		scd_shut_set
**		scd_start_sampler_set
**		scd_stop_set
**		scd_stop_sampler_set
**
** History:
**
**	28-Jul-1993 (daveb)
**	    created to get at stuff in the Sc_main_cb, and to support
**	    portable SCF level control of the server listen and shutdown
**	    state.
**	17-dec-93 (rblumer)
**	    removed sc_fips field.
**	14-feb-94 (rblumer)
**	    removed sc_noflatten field. (B59120, 09-feb-94 LRC proposal).
**      10-Mar-1994 (daveb)
**          Add a server-wide PID object, highwater_connections.
**	23-may-95 (reijo01)
**	    Added startup_time object to conform to rfc1697, the relational dbms 
**      mib.
**      14-jun-95 (chech02)
**          Added SCF_misc_sem semaphore to protect critical sections.(MCT)
**	03-jun-1996 (canor01)
**	    Replaced external semaphore with localized one.
**	23-sep-1996 (canor01)
**	    Move global data definitions to scddata.c.
**      16-Oct-98 (fanra01)
**          Add method to return server name.
**	23-aug-1999 (somsa01)
**	    Added scd_start_sampler_set and scd_start_sampler_set to start
**	    and stop the Sampler thread, respectively.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
*/

/* forward decls */

MO_GET_METHOD scd_state_get;
MO_GET_METHOD scd_listen_get;
MO_GET_METHOD scd_shutdown_get;
MO_GET_METHOD scd_capabilities_get;
MO_GET_METHOD scd_pid_get;
MO_GET_METHOD scd_server_name_get;

MO_SET_METHOD scd_crash_set;
MO_SET_METHOD scd_close_set;
MO_SET_METHOD scd_open_set;
MO_SET_METHOD scd_shut_set;
MO_SET_METHOD scd_stop_set;
MO_SET_METHOD scd_start_sampler_set;
MO_SET_METHOD scd_stop_sampler_set;

/* this needs to be filled in when it's known, before registering */

# define SC_MAIN_CB_ADDR	NULL

GLOBALREF PTR          Sc_server_name;
GLOBALREF MO_CLASS_DEF Scd_classes[];

/* ---------------------------------------------------------------- */


/*{
** Name:	scd_mo_init - set up SCF MO classes.
**
** Description:
**	Defines the MO classes.  Should only be called once.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	29-Jul-1993 (daveb)
**	    created
*/

VOID
scd_mo_init(void)
{
    int i;

    if( Sc_main_cb != NULL )
    {
	for( i = 0; Scd_classes[i].classid != NULL; i++ )
	    Scd_classes[i].cdata = (PTR) Sc_main_cb;

	(void) MOclassdef( MAXI2, Scd_classes );
    }
}


/*{
** Name:	scd_listen_get
**
** Description:
**	decode the listen mask into a string.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset to listen_mask
**	objsize		ignored
**	object		Sc_main_cb.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with string.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	28-Jul-1993 (daveb)
**	    created.
*/

STATUS
scd_listen_get(i4 offset,
	       i4  objsize,
	       PTR object,
	       i4  luserbuf,
	       char *userbuf)
{
    return( MOstrout( MO_VALUE_TRUNCATED,
		     Sc_main_cb->sc_listen_mask & SC_LSN_REGULAR_OK ?
		     "OPEN" : "CLOSED",
		     luserbuf, userbuf ) );
}


/*{
** Name:	scd_shutdown_get
**
** Description:
**	decode the listen mask for regular listen into 1 or 0.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset to listen_mask
**	objsize		ignored
**	object		Sc_main_cb.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with string.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	28-Jul-1993 (daveb)
**	    created.
*/

STATUS
scd_shutdown_get(i4 offset,
		       i4  objsize,
		       PTR object,
		       i4  luserbuf,
		       char *userbuf)
{
    return( MOstrout( MO_VALUE_TRUNCATED,
		     Sc_main_cb->sc_listen_mask & SC_LSN_TERM_IDLE ?
		     "PENDING" : "OPEN",
		     luserbuf, userbuf ) );
}




/*{
** Name:	scd_state_get
**
** Description:
**	decode the state mask into a string.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset to state_mask
**	objsize		ignored
**	object		Sc_main_cb.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with string.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	28-Jul-1993 (daveb)
**	    created.
*/

STATUS
scd_state_get(i4 offset,
	      i4  objsize,
	      PTR object,
	      i4  luserbuf,
	      char *userbuf)
{
    char buf[80];

    MEfill( sizeof(buf), 0, buf );
    TRformat( NULL, 1, buf, sizeof(buf) - 1, "%w",
	     "UNINIT,INIT,SHUTDOWN,OPERATIONAL,2PC_RECOVER",
	     Sc_main_cb->sc_state );
	     
    return( MOstrout( MO_VALUE_TRUNCATED, buf, luserbuf, userbuf ) );
}



/*{
** Name:	scd_capabilities_get
**
** Description:
**	decode the capabilities mask into a string.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset to capabilities_mask
**	objsize		ignored
**	object		Sc_main_cb.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with string.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	28-Jul-1993 (daveb)
**	    created.
*/

STATUS
scd_capabilities_get(i4 offset,
		     i4  objsize,
		     PTR object,
		     i4  luserbuf,
		     char *userbuf)
{
    char buf[80];

    MEfill( sizeof(buf), 0, buf );
    TRformat( NULL, 1, buf, sizeof(buf) - 1, "%v",
	     "RESRC_CNTRL,C2SECURE,INGRES,RMS,STAR,RECOVERY",
	     Sc_main_cb->sc_capabilities );
	     
    return( MOstrout( MO_VALUE_TRUNCATED, buf, luserbuf, userbuf ) );
}


/*{
** Name:	scd_pid_get
**
** Description:
**	return the PID.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored
**	object		ignored.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with PID.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	11-Mar-1993 (daveb)
**	    created.
**      10-Mar-1994 (daveb)
**          swiped from scsmo.c for use here too.
*/
STATUS
scd_pid_get(i4 offset,
	    i4  objsize,
	    PTR object,
	    i4  luserbuf,
	    char *userbuf)
{
    PID	pid;

    PCpid( &pid );

    return( MOulongout( MO_VALUE_TRUNCATED,
			(u_i8)pid, luserbuf, userbuf ) );
}

/*{
** Name:	scd_server_name_get
**
** Description:
**	return the server name string.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored
**	object		ignored.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with server name.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**      16-Oct-98 (fanra01)
**	    created.
*/
STATUS
scd_server_name_get(
    i4  offset,
    i4  objsize,
    PTR object,
    i4  luserbuf,
    char *userbuf)
{
    char* name = Sc_server_name;
    char  buf[80];
    char* p = buf;

    /*
    ** skip '\' and '(' characters
    */
    while ( (*name == '\\') || (*name == '('))
    {
        CMnext (name);
    }
    while ( (*name != '\\') && (*name != EOS))
    {
        CMcpyinc (name, p);
    }
    *p = EOS;

    return( MOlstrout( MO_VALUE_TRUNCATED, objsize, buf, luserbuf, userbuf ));
}



/*{
** Name:	scd_crash_set	- set function for CS_CRASH
**
** Description:
**	Calls CSterminate( CS_CRASH, ...) on demand  to stop the
**	server dead in it's tracks.  Args ignored.  Should only
**	be called by a powerful user.  Protect the MO object!
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		ignored	
**
** Outputs:
**	none
**
** Returns:
**	CSterminate error return value.
**
** History:
**	29-Jul-1993 (daveb)
**	    
*/

STATUS
scd_crash_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object)
{
    i4  count;
    STATUS stat;
    stat = CSterminate(CS_CRASH, &count);
    if( E_CS0003_NOT_QUIESCENT == stat )
	stat = OK;
    return( stat );
}



/*{
** Name:	scd_close_set	- set function for turning off listen.
**
** Description:
**	Turn off regular user listens.
**	Args ignored.  Should only be called by a powerful
**	user.  Protect the MO object!
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		ignored	
**
** Outputs:
**	none
**
** Returns:
**	CSterminate error return value.
**
** History:
**	29-Jul-1993 (daveb)
**	    created.
*/

STATUS
scd_close_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object)
{
    STATUS stat = OK;

    Sc_main_cb->sc_listen_mask &= ~(SC_LSN_REGULAR_OK);
    return( stat );
}



/*{
** Name:	scd_open_set	- undo shut
**
** Description:
**	Allow regular user listens and cancel pending shutdown.
**	Args ignored.  Should only be called by a powerful
**	user.  Protect the MO object!
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		ignored	
**
** Outputs:
**	none
**
** Returns:
**	CSterminate error return value.
**
** History:
**	29-Jul-1993 (daveb)
**	    created.
*/

STATUS
scd_open_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object)
{
    STATUS stat = OK;

    Sc_main_cb->sc_listen_mask &= ~(SC_LSN_TERM_IDLE);
    Sc_main_cb->sc_listen_mask |= SC_LSN_REGULAR_OK;

    return( stat );
}



/*{
** Name:	scd_shut_set	- set function for CS_CLOSE
**
** Description:
**	Calls CSterminate( CS_CLOSE, ...) function on demand to stop the
**	server nicely.  Args ignored.  Should only be called by a powerful
**	user.  Protect the MO object!
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		ignored	
**
** Outputs:
**	none
**
** Returns:
**	CSterminate error return value.
**
** History:
**	29-Jul-1993 (daveb)
**	    created.
*/

STATUS
scd_shut_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object)
{
    STATUS stat = OK;
    i4  count;

    Sc_main_cb->sc_listen_mask &= ~(SC_LSN_REGULAR_OK);
    Sc_main_cb->sc_listen_mask |= SC_LSN_TERM_IDLE;

    CSp_semaphore( 0, &Sc_main_cb->sc_misc_semaphore ); /* shared */
    if( Sc_main_cb->sc_current_conns == 0 )
    {
	CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );
	stat = CSterminate(CS_CLOSE, &count);
    }
    else
	CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );

    if( E_CS0003_NOT_QUIESCENT == stat )
	stat = OK;
    return( stat );
}





/*
** {
** Name:	scd_stop_set	- set function for CS_KILL.
**
** Description:
**	Calls CSterminate( CS_KILL, ... ) on demand to stop a server
**	quickly.  Args ignored.  Should only be called by a powerful
**	user.  Protect the MO object!
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		ignored	
**
** Outputs:
**	none
**
** Returns:
**	CSterminate error return value.
**
** History:
**	29-Jul-1993 (daveb)
**	    
*/

STATUS
scd_stop_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object)
{
    STATUS stat;
    i4  count;
    
    stat = CSterminate(CS_KILL, &count);
    if( E_CS0003_NOT_QUIESCENT == stat )
	stat = OK;
    return( stat );
}


/*
** {
** Name:	scd_start_sampler_set	- set function for StartSampler.
**
** Description:
**	Calls StartSampler() on demand to start the Sampler thread via
**	CSmonitor().
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		Ignored.
**	lsbuf		Ignored.
**	sbuf		Ignored.
**	size		Ignored.
**	object		The SCB for this session.	
**
** Outputs:
**	None.
**
** Returns:
**	CSmonitor() error return value.
**
** History:
**	23-aug-1999 (somsa01)
**	    Created.
**      27-May-2010 (hanal04) Bug 123810
**          Don't pass command strings as constants to CSmonitor(). A call to
**          CVlower() will SEGV. Use a local variable instead.
*/

STATUS
scd_start_sampler_set(i4 offset, i4 lsbuf, char *sbuf, i4 size, PTR object)
{
    CS_SCB		*scb  = (CS_SCB *) object;
    i4			nmode;
    STATUS		stat;
    char		cmd[15] = "start sampling";

    stat = CSmonitor(CS_INPUT, scb, &nmode, cmd, 1, NULL);
    return(stat);
}


/*
** {
** Name:	scd_stop_sampler_set	- set function for StopSampler.
**
** Description:
**	Calls StopSampler() on demand to stop the Sampler thread via
**	CSmonitor().
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		Ignored.
**	lsbuf		Ignored.
**	sbuf		Ignored.
**	size		Ignored.
**	object		The SCB for this session.	
**
** Outputs:
**	None.
**
** Returns:
**	CSmonitor() error return value.
**
** History:
**	23-aug-1999 (somsa01)
**	    Created.
**      27-May-2010 (hanal04) Bug 123810
**          Don't pass command strings as constants to CSmonitor(). A call to
**          CVlower() will SEGV. Use a local variable instead.
*/

STATUS
scd_stop_sampler_set(i4 offset, i4 lsbuf, char *sbuf, i4 size, PTR object)
{
    CS_SCB	*scb = (CS_SCB *) object;
    i4		nmode;
    STATUS	stat;
    char		cmd[14] = "stop sampling";

    stat = CSmonitor(CS_INPUT, scb, &nmode, cmd, 1, NULL);
    return(stat);
}

