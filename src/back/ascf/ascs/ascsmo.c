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
#include    "ascsint.h"

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
**		ascs_mo_init - set up SCF MO classes.
**		ascs_scb_attach - make SCD_SCB known to MO.
**		ascs_scb_detach - make scb unknown to MO.
**		ascs_dblockmode_get
**		ascs_facility_name_get - facility name decoder
**		ascs_activity_get - activity decoder
**		ascs_act_detail_get - activity detail decoder
**		ascs_description_get - description decoder
**		ascs_query_get - decode the query.
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
**      04-Mar-98 (fanra01)
**          Modified to use SID instance and made function name consistent
**          with attach.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
*/

/* forward decls */

MO_GET_METHOD ascs_pid_get;
MO_GET_METHOD ascs_dblockmode_get;
MO_GET_METHOD ascs_facility_name_get;
MO_GET_METHOD ascs_act_detail_get;
MO_GET_METHOD ascs_activity_get;
MO_GET_METHOD ascs_description_get;
MO_GET_METHOD ascs_query_get;
MO_GET_METHOD ascs_assoc_get;

MO_SET_METHOD ascs_remove_sess_set;
MO_SET_METHOD ascs_crash_sess_set;

static char index_name[] = "exp.scf.scs.scb_index";
static char index_ptr [] = "exp.scf.scs.scb_ptr";

GLOBALREF MO_CLASS_DEF AScs_classes[];

/* ---------------------------------------------------------------- */


/*{
** Name:	ascs_mo_init - set up SCF MO classes.
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
ascs_mo_init(void)
{
    (void) MOclassdef( MAXI2, AScs_classes );
}


/*{
** Name:	ascs_scb_attach - make SCD_SCB known to MO.
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
**      04-Mar-98 (fanra01)
**          Modified to use SID instance.
*/

STATUS
ascs_scb_attach( SCD_SCB *scb )
{
    char buf[ 80 ];

    MOptrout( 0, (PTR)scb, sizeof(buf), buf );
    MOattach( MO_INSTANCE_VAR, index_ptr, buf, (PTR)scb );

    MOulongout( 0, scb->cs_scb.cs_self, sizeof(buf), buf );
    return( MOattach( MO_INSTANCE_VAR, index_name, buf, (PTR)scb ) );
}


/*{
** Name:	ascs_scb_detach - make scb unknown to MO.
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
**      04-Mar-98 (fanra01)
**          Modified to use SID instance and made function name consistent
**          with attach.
*/

STATUS
ascs_scb_detach( SCD_SCB *scb )
{
    char buf[ 80 ];

    MOptrout( 0, (PTR)scb, sizeof(buf), buf );
    MOdetach( index_ptr, buf);

    MOulongout( 0, scb->cs_scb.cs_self, sizeof(buf), buf );
    return( MOdetach( index_name, buf ) );
}


/*{
** Name:	ascs_assoc_get
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
ascs_assoc_get(i4 offset,
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
** Name:	ascs_pid_get
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
ascs_pid_get(i4 offset,
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
** Name:	ascs_self_get
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
*/
STATUS
ascs_self_get(i4 offset,
	    i4  objsize,
	    PTR object,
	    i4  luserbuf,
	    char *userbuf)
{
    char buf[ 20 ];
    char *str = buf;
    SCD_SCB *scb = (SCD_SCB *)object;

    (void) MOptrout(0, (PTR)scb->cs_scb.cs_self, sizeof(buf), buf);
    return( MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf ));
}


/*{
** Name:	ascs_dblockmode_get
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
ascs_dblockmode_get(i4 offset,
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
** Name:	ascs_facility_name_get - facility name decoder
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
ascs_facility_name_get(i4 offset,
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
** Name:	ascs_activity_get - activity decoder
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
ascs_activity_get(i4 offset,
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
** Name:	ascs_act_detail_get - activity detail decoder
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
ascs_act_detail_get(i4 offset,
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
** Name:	ascs_query_get - decode the query.
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
ascs_query_get(i4 offset,
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
**  Name:	ascs_remove_sess_set	-- remove connection.
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
ascs_remove_sess_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
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
**  Name:	ascs_crash_sess_set	-- crash session nastily.
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
ascs_crash_sess_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
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
** Name:	ascs_description_get - description decoder
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
ascs_description_get(i4 offset,
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
