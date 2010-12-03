/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>

#include    <er.h>
#include    <ex.h>
#include    <cv.h>
#include    <cs.h>
#include    <me.h>
#include    <sp.h>
#include    <mo.h>
#include    <st.h>
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
#include    "scsint.h"

/*
** scsmo.c -- experimental MO module for SCF.
**
** Description
**
**	Defines per-session objects for SCS, and necessary
**	MO access methods.
**
** Functions:
** 
**		scs_mo_init - set up SCF MO classes.
**		scs_scb_attach - make SCD_SCB known to MO.
**		scs_scb_detach - make scb unknown to MO.
**		scs_dblockmode_get
**		scs_facility_name_get - facility name decoder
**		scs_activity_get - activity decoder
**		scs_act_detail_get - activity detail decoder
**		scs_description_get - description decoder
**		scs_query_get - decode the query.
**		scs_lquery_get - decode the last query.
**
** History:
**
**	16-Sep-92 (daveb)
**		TRformat %w doesn't NULL terminate the buffer.
**		MEfill it with 0 first to make sure.
**	03-dec-92 (andre)
**	    ics_eusername will become a pointer that will point at ics_susername
**	    unless we are inside a module with <module auth id> in which case
**	    the <module auth id> will be stored in ics_musername and
**	    ics_eusername will be made to point at ics_musername until we leave
**	    the module.
**
**	    As a part of desupporting SET GROUP/ROLE, we are removing ics_saplid
**	    and ics_sgrpid
**
**	    Initial session user name will live in ics_iusername (used to be
**	    ics_susername)
**	03-dec-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	11-Mar-1993 (daveb)
**	    Add pid object, making it easier to join
**	    correctly with LG/LK pid,sid fields.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-Jul-1993 (daveb)
**	    rework includes for prototyping.
**	18-Aug-1993 (daveb)
**	    Index needs to be MOpvget, not MOivget, 'cause it's a pointer.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	14-Sep-1993 (daveb)
**	    Add gca assoc id object
**	15-Sep-1993 (daveb)
**	    Add drop session object.
**	27-oct-93 (robf)
**          Add priority/idle/connection session objects
**	1-Nov-1993 (daveb)
**	    Rename drop to remove, and add a kill.
**      10-Nov-1993 (daveb)
**          kill->crash
**	09-mar-94 (swm)
**	    Bug #60425
**	    CS_SID value was cast to i4  which truncates on 64-bit platforms.
**	15-feb-94 (robf)
**          Mark session description and query as MO_SECURITY_READ only.
**      23-Feb-1994 (daveb)
**          Add object for gw_info, and extract out objects for
**  	    apparent host, apparent user, apparent tty, apparent pid,
**          apparent database
**       8-Mar-1994 (daveb) 60414
**          use MOlongout so -1 for no association comes out right as
**  	    -00000000001.
**      20-Apr-1994 (daveb)
**          Based on review, change names of 'apparent' objects to 'client'
**	30-may-95 (reijo01)
**      Bug #68862
**      IMA object 'exp.scs.scs.scb_activity' gives garbled string for the
**      <Terminator Thread>. Returned value is "Waiting for next check
**      timerminate" instead of "Waiting for next check time".
**	23-sep-1996 (canor01)
**	    Move global data definitions to scsdata.c.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-oct-2001 (stephenb)
**	    Add scs_lquery_get function to get last query in MO
**	13-Dec-2002 (devjo01) Bug 103257 (redux)
**	    Increased buffer size in scs_self_get.
**	13-Feb-2003 (hanje04)
**	    BUG 109555
**	    In scs_pid_get call use new PCmget() to get server PID so that it 
**	    is obtained correctly on Linux.
**	17-Jun-2003 (hanje04)
**	    Revert back to using PCpid as it now correctly returns the
**	    process ID.
**	13-oct-2003 (somsa01)
**	    Use MOsidout() for cs_self, since it is now basically the scb.
**	21-jan-03 (hayke02)
**	    Modify above change so that the two MOsidout() calls have cs_self
**	    cast to PTR to match the expected type.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**	11-Nov-2010 (kschendel) SIR 124685
**	    Refine CS prototyping.
*/

/* forward decls */

MO_GET_METHOD scs_pid_get;
MO_GET_METHOD scs_dblockmode_get;
MO_GET_METHOD scs_facility_name_get;
MO_GET_METHOD scs_act_detail_get;
MO_GET_METHOD scs_activity_get;
MO_GET_METHOD scs_description_get;
MO_GET_METHOD scs_query_get;
MO_GET_METHOD scs_lquery_get;
MO_GET_METHOD scs_assoc_get;

MO_SET_METHOD scs_remove_sess_set;
MO_SET_METHOD scs_crash_sess_set;

static char index_name[] = "exp.scf.scs.scb_index";
static char index_ptr [] = "exp.scf.scs.scb_ptr";

GLOBALREF MO_CLASS_DEF Scs_classes[];

/* ---------------------------------------------------------------- */


/*{
** Name:	scs_mo_init - set up SCF MO classes.
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
**	28-Oct-1992 (daveb)
**	    documented
*/

VOID
scs_mo_init(void)
{
    (void) MOclassdef( MAXI2, Scs_classes );
}


/*{
** Name:	scs_scb_attach - make SCD_SCB known to MO.
**
** Description:
**	Attaches the SCB to the right SCF class.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	scb	the scb in question.
**
** Outputs:
**	none.
**
** Returns:
**	MOattach return.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**	08-sep-93 (swm)
**	    Call MOptrout rather than MOulongout for scb pointer.
**      13-Feb-98 (fanra01)
**          Modified to use SID instance.
**	13-oct-2003 (somsa01)
**	    Use MOsidout() for cs_self, since it is now basically the scb.
*/

void
scs_scb_attach( CS_SCB *csscb )
{
    char buf[ 80 ];

    MOptrout( 0, (PTR)csscb, sizeof(buf), buf );
    MOattach( MO_INSTANCE_VAR, index_ptr, buf, (PTR)csscb );

    MOsidout( 0, (PTR)(csscb->cs_self), sizeof(buf), buf );
    (void) MOattach( MO_INSTANCE_VAR, index_name, buf, (PTR)csscb );
}


/*{
** Name:	scs_scb_detach - make scb unknown to MO.
**
** Description:
**	Detaches the SCB from the SCFMO class.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	scb	the session in question.
**
** Outputs:
**	none
**
** Returns:
**	MOdetach return value.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**	08-sep-93 (swm)
**	    Call MOptrout rather than MOulongout for scb pointer.
**      13-Feb-98 (fanra01)
**          Modified to use SID instance and made function name consistent
**          with attach.
**	13-oct-2003 (somsa01)
**	    Use MOsidout() for cs_self, since it is now basically the scb.
*/

void
scs_scb_detach( CS_SCB *csscb )
{
    char buf[ 80 ];

    MOptrout( 0, (PTR)csscb, sizeof(buf), buf );
    MOdetach( index_ptr, buf);

    MOsidout( 0, (PTR)(csscb->cs_self), sizeof(buf), buf );
    (void) MOdetach( index_name, buf );
}


/*{
** Name:	scs_assoc_get
**
** Description:
**	return the assoc id, bumped by 1 since tye are origin 0.
**	This lets us join with the gca assoc_id index.
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
**	15-Sep-1993 (daveb)
**	    created.
**       8-Mar-1994 (daveb) 60414
**          use MOlongout so -1 for no association comes out right as
**  	    -00000000001.
*/
STATUS
scs_assoc_get(i4 offset,
	      i4  objsize,
	      PTR object,
	      i4  luserbuf,
	      char *userbuf)
{
    SCD_SCB *scb = (SCD_SCB *)object;

    return( MOlongout( MO_VALUE_TRUNCATED,
		       (i8)scb->scb_cscb.cscb_assoc_id + 1,
		       luserbuf, userbuf ) );
}


/*{
** Name:	scs_pid_get
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
*/
STATUS
scs_pid_get(i4 offset,
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
** Name:	scs_self_get
**
** Description:
**	return the displayable session ID (000nnnn)
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
**	userbuf		filled with session ID.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	7-aug-97 (inkdo01)
**	    created.
**	27-Sep-2001 (inifa01) Bug 103257 ingsrv 1324
**	    IMA ima_server_sessions contains weird values in session_id (\0001 
**	    at end of each session_id) and ima_server_sessions_extra contains 
**	    no row.
**	    Buffer in get function for session_id field; scs_self_get(), not large
**	    enough to hold resultant string on 64 bit platform. 
**	    The maximum number of decimal digits required to display the value of
**	    an integer is defined as ( [<number of bits used to represent the number> / 3] + 1).
**	    Increased buffer size to 22.
**	13-Dec-2002 (devjo01) Bug 103257 (redux)
**	    Increasing size to 22 is insufficient, since this does not
**	    allow for a '\0' terminator.  Bumped 'buf' size to 24.
**	07-Jul-2003 (devjo01)
**	    Use MOsidout to format session ID so that radix of output is configurable.
*/
STATUS
scs_self_get(i4 offset,
	    i4  objsize,
	    PTR object,
	    i4  luserbuf,
	    char *userbuf)
{
    return( MOsidout( MO_VALUE_TRUNCATED, (PTR)(((SCD_SCB *)object)->cs_scb.cs_self),
		      luserbuf, userbuf ));
}


/*{
** Name:	scs_dblockmode_get
**
** Description:
**	string get, only changes any unlikely characters
**	that might be lurking into '#'.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored
**	object		the lockmode value
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with name.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
*/
STATUS
scs_dblockmode_get(i4 offset,
		   i4  objsize,
		   PTR object,
		   i4  luserbuf,
		   char *userbuf)
{
    SCD_SCB *scb = (SCD_SCB *)object;
    STATUS rval = OK;

    if (scb->scb_sscb.sscb_stype == SCS_SMONITOR ||
	scb->scb_cscb.cscb_comm_size < 256 )
    {
	*userbuf = EOS;
    }
    else
    {
	MOstrout( MO_VALUE_TRUNCATED,
		 scb->scb_sscb.sscb_ics.ics_lock_mode & DMC_L_EXCLUSIVE ?
		 "exclusive" : "shared",
		 luserbuf, userbuf );
    }
    return( rval );
}


/*{
** Name:	scs_facility_name_get - facility name decoder
**
** Description:
**	string of the sscb_cfac, as done for CSmonitor.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored
**	object		point to an SCD_SCB.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with current facility string.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	16-Sep-92 (daveb)
**		TRformat %w doesn't NULL terminate the buffer.
**		MEfill it with 0 first to make sure.
**	28-Oct-1992 (daveb)
**	    documented
*/
STATUS
scs_facility_name_get(i4 offset,
		      i4  objsize,
		      PTR object,
		      i4  luserbuf,
		      char *userbuf)
{
    SCD_SCB *scb = (SCD_SCB *)object;
    char buf[ 7 ];
    

    MEfill( sizeof(buf), 0, buf );
    if (scb->scb_sscb.sscb_stype != SCS_SMONITOR &&
	scb->scb_cscb.cscb_comm_size >= 256 )
	TRformat( NULL, 1, buf, sizeof(buf) - 1, "%w", 
"<None>,CLF,ADF,DMF,OPF,PSF,QEF,QSF,RDF,SCF,ULF,DUF,GCF,RQF,TPF,GWF",
		 scb->scb_sscb.sscb_cfac);

    return( MOlstrout( MO_VALUE_TRUNCATED, sizeof(buf), buf,
		      luserbuf, userbuf ));
    
}


/*{
** Name:	scs_activity_get - activity decoder
**
** Description:
**	string of the sscb_ics, as done for CSmonitor.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored
**	object		pointer to an SCD_SCB.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with current activity string.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**	30-may-95 (reijo01)
**      Bug #68862
**      IMA object 'exp.scs.scs.scb_activity' gives garbled string for the
**      <Terminator Thread>. Returned value is "Waiting for next check
**      timerminate" instead of "Waiting for next check time".
*/
STATUS
scs_activity_get(i4 offset,
		 i4  objsize,
		 PTR object,
		 i4  luserbuf,
		 char *userbuf)
{
    SCD_SCB *scb = (SCD_SCB *)object;
    STATUS stat = OK;
    char *str;
    i4  len;

    str = "(unknown)";
    len = scb->scb_sscb.sscb_ics.ics_l_act1;

    if (scb->scb_sscb.sscb_stype == SCS_SMONITOR ||
	scb->scb_cscb.cscb_comm_size < 256 )
    {
	str = "";
    }
    else
    {
	if ( len || scb->scb_sscb.sscb_ics.ics_l_act2)
	{
	    if ( len )
	    {
		/*
		** If the length of the activity string is longer or equal to the
		** length of the output buffer, change the length by shortening
		** it to one less then the length of the output buffer.
		*/
		if( len >= luserbuf )
		    len = luserbuf - 1;
		MEcopy( scb->scb_sscb.sscb_ics.ics_act1, len, userbuf );
		userbuf[ len ] = EOS;
		str = NULL;
	    }
	    else if (scb->scb_sscb.sscb_cfac == DB_DMF_ID)
	    {
		str = "(Aborting)";
	    }
	    else if (scb->scb_sscb.sscb_cfac == DB_OPF_ID)
	    {
		str = "(Optimizing)";
	    }
	}
    }

    if( str != NULL )
	stat = MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf );

    return( stat );
}
    

/*{
** Name:	scs_act_detail_get - activity detail decoder
**
** Description:
**	string of the sscb_ics.ics_act2, as done for CSmonitor.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored
**	object		point to an SCD_SCB.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with more activity string.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
*/

STATUS
scs_act_detail_get(i4 offset,
		   i4  objsize,
		   PTR object,
		   i4  luserbuf,
		   char *userbuf)
{
    SCD_SCB *scb = (SCD_SCB *)object;
    i4  len;

    *userbuf = EOS;
    if (scb->scb_sscb.sscb_stype != SCS_SMONITOR &&
	scb->scb_cscb.cscb_comm_size >= 256 &&
	( len = scb->scb_sscb.sscb_ics.ics_l_act2 ) )
    {
	if( len <= luserbuf )
	    len = luserbuf - 1;
	MEcopy( scb->scb_sscb.sscb_ics.ics_act2, len, userbuf );
	userbuf[ len ] = EOS;
    }
    return( OK );
}


/*{
** Name:	scs_query_get - decode the query.
**
** Description:
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored
**	object		point to a SCD_SCB.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with query for the session.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
*/

STATUS
scs_query_get(i4 offset,
	      i4  objsize,
	      PTR object,
	      i4  luserbuf,
	      char *userbuf)
{
    SCD_SCB *scb = (SCD_SCB *)object;
    char *qbuf;
    i4  len;

    *userbuf = EOS;
    if (scb->scb_sscb.sscb_stype != SCS_SMONITOR &&
	scb->scb_cscb.cscb_comm_size >= 256 &&
	( len = scb->scb_sscb.sscb_ics.ics_l_qbuf ) )
    {
	qbuf = len ? scb->scb_sscb.sscb_ics.ics_qbuf : "";
	if( len >= luserbuf )
	    len = luserbuf - 1;
	MEcopy( qbuf, len, userbuf );
	userbuf[ len ] = EOS;
    }
    return( OK );
}
/*{
** Name:	scs_lquery_get - decode the last query.
**
** Description:
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored
**	object		point to a SCD_SCB.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with query for the session.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	24-oct-2001 (stephenb)
**	    created from scs_query_get
*/

STATUS
scs_lquery_get(i4 offset,
	      i4  objsize,
	      PTR object,
	      i4  luserbuf,
	      char *userbuf)
{
    SCD_SCB *scb = (SCD_SCB *)object;
    char *qbuf;
    i4  len;

    *userbuf = EOS;
    if (scb->scb_sscb.sscb_stype != SCS_SMONITOR &&
	scb->scb_cscb.cscb_comm_size >= 256 &&
	( len = scb->scb_sscb.sscb_ics.ics_l_lqbuf ) )
    {
	qbuf = len ? scb->scb_sscb.sscb_ics.ics_lqbuf : "";
	if( len >= luserbuf )
	    len = luserbuf - 1;
	MEcopy( qbuf, len, userbuf );
	userbuf[ len ] = EOS;
    }
    return( OK );
}



/*{
**  Name:	scs_remove_sess_set	-- remove connection.
**
**  Description:
**
**	Drop the connection for the scb given in object, safely.
**
**	Marks the session as DROP_PENDING, and raises an
**	interrupt.
**
**  Inputs:
**	 offset
**		ignored.
**	 luserbuf
**		ignored.
**	 userbuf
**		ignored.
**	 objsize
**		ignored.
**	 object
**		a pointer to the SCD_SCB, as a PTR.
**  Outputs:
**	 object
**		Modified by the method.
**  Returns:
**	 OK
**		if the operation succeseded
**	 other
**		other failure status as appropriate.
**  History:
**	15-Sep-1993 (daveb)
**	    created.
**	1-Nov-1993 (daveb)
**	    renamed.
*/

STATUS 
scs_remove_sess_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    SCD_SCB *scb = (SCD_SCB *)object;

    /* don't allow kill of special sessions */

    if( scb->cs_scb.cs_client_type == SCD_SCB_TYPE &&
       scb->scb_sscb.sscb_stype == SCS_SNORMAL ||
        scb->scb_sscb.sscb_stype == SCS_SMONITOR )
    {
	scb->scb_sscb.sscb_flags |= SCS_DROP_PENDING;
	CSattn( CS_ATTN_EVENT, scb->cs_scb.cs_self );
    }

    return( OK );
}



/*{
**  Name:	scs_crash_sess_set	-- crash session nastily.
**
**  Description:
**
**	Kill a thread, unsafely.
**
**	Marks the session as DROP_PENDING, and raises an
**	interrupt.
**
**  Inputs:
**	 offset
**		ignored.
**	 luserbuf
**		ignored.
**	 userbuf
**		ignored.
**	 objsize
**		ignored.
**	 object
**		a pointer to the SCD_SCB, as a PTR.
**  Outputs:
**	 object
**		Modified by the method.
**  Returns:
**	 OK
**		if the operation succeseded
**	 other
**		other failure status as appropriate.
**  History:
**	1-Nov-1993 (daveb)
**	    created.
**       8-Mar-1994 (daveb)
**          remove int cast on CSremove -- incorrect and not needed.
*/

STATUS 
scs_crash_sess_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    SCD_SCB *scb = (SCD_SCB *)object;

    /* don't allow kill of special sessions */

    if( scb->cs_scb.cs_client_type == SCD_SCB_TYPE &&
       scb->scb_sscb.sscb_stype == SCS_SNORMAL ||
        scb->scb_sscb.sscb_stype == SCS_SMONITOR )
    {
	/* FIXME:  this should be logged W_SC0328_REMOVE_SESSION? */

	CSremove(scb->cs_scb.cs_self);
	CSattn( CS_ATTN_EVENT, scb->cs_scb.cs_self );
    }

    return( OK );
}


/*{
** Name:	scs_description_get - description decoder
**
** Description:
**	string of the sscb_ics, as done for CSmonitor.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored
**	object		pointer to an SCD_SCB.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with current description string.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	9-nov-93 (robf)
**	    created, based on scs_activity_get
*/
STATUS
scs_description_get(i4 offset,
		 i4  objsize,
		 PTR object,
		 i4  luserbuf,
		 char *userbuf)
{
    SCD_SCB *scb = (SCD_SCB *)object;
    STATUS stat = OK;
    char *str;
    i4  len;

    len = scb->scb_sscb.sscb_ics.ics_l_desc;
    if ( len >0)
    {
	if( len >= luserbuf )
	    len = luserbuf - 1;
        MEcopy( scb->scb_sscb.sscb_ics.ics_description, len, userbuf );
	userbuf[ len ] = EOS;
    }
    else
    {
	str = "(unknown)";
	stat = MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf );
    }

    return( stat );
}
