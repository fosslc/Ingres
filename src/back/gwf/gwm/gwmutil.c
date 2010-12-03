/*
**Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <dudbms.h>
# include <bt.h>
# include <cm.h>
# include <cs.h>
# include <er.h>
# include <lo.h>
# include <me.h>
# include <sp.h>
# include <st.h>
# include <tm.h>
# include <tr.h>
# include <erglf.h>
# include <mo.h>
# include <pm.h>
# include <scf.h>
# include <adf.h>
# include <dmf.h>
# include <dmrcb.h>
# include <dmtcb.h>
# include <dmucb.h>
# include <ulf.h>
# include <ulm.h>
# include <gca.h>
# include <gcm.h>

#define GM_WANT_ARRAY
# include <gwf.h>
# include <gwfint.h>

# include "gwmint.h"

/*
** Name: gwmutil.c	- utilities called by gwmmob.c and gwmxtab.c
**
** Description:
**
** Defines:
**
**		GM_my_server	- this server.
**		GM_my_vnode	- this server's vnode
**		GM_gt_dba	- who's the DBA of this database?
**		GM_gt_user	- who's the user in this database?
**		GM_my_guises	- active guises for this session
**		GM_gt_sem	- get a semaphore, logging errors.
**		GM_i_sem	- init semaphore, logging errors.
**		GM_release_sem	- unlock semaphore, logging errors.
**		GM_rm_sem	- delete a semaphore, logging errors.
**  	    	GM_nat_compare	- MO comparison function (really takes PTRs).
**		GM_alloc	- allocate global/permanent memory.
**		GM_free		- release memory obtained from GM_alloc
**		GM_att_offset	- offset of data for att in tuple buffer.
**		GM_key_offset	- offset of data for key in key buffer.
**		GM_open_sub	- Common open operations for MOB and XTAB.
**		GM_ualloc	- allocate out of a ulm stream
**  	    	GM_dumpmem  	- dump memory to log
**  	    	GM_tabf_sub 	- common code for tabf from XTAB or MOB.
**  	    	GM_dbname_gt	- get the current database name (for errs)
**  	    	GM_check_att	- is attribute a special IS clause name?
**  	    	GM_att_type 	- return type of IS clause attribute
**  	    	GM_breakpoint	- no-op, for debugging
**  	    	GM_just_vnode	- extract vnode from place into a buffer
**  	    	GM_gca_addr_from    - get gca addr from place into a buffer
**  	    	GM_dbslen   	- Length of interesting part of a MAXNAME buffer.
**  	    	GM_info_reg 	- get table structure info for register
**  	    	GM_get_pmsym	- get a PM symbol value, defaulting if not found
**  	    	GM_dump_dmt_att	- trace dump of DMT_ATT_ENTRY.
**  	    	GM_att_type_decode  - get string from gma_trace type value.
**  	    	GM_dump_gwm_att	- trace dump of att parts of GM_ATT_ENTRY.
**  	    	GM_dump_gwm_katt - trace dump of key parts of GM_ATT_ENTRY.
**              GM_chk_priv     - check user name for ownership of privilege.
**
** History:
**	17-Sep-92 (daveb)
**	    pulled from gwmxtab.c, gwmmob.c, and gwmop.c
**	29-sep-92 (daveb)
**	    Add GM_open_sub to do bulk of table open work.  Remove
**	    as bunch of complicated junk now that we have GM_ATT_ENTRYs.
**	30-sep-92 (daveb)
**	    Remove active key validation from GM_position, punting
**	    it to the G?get routines, which should know how to do
**	    it properly, and which won't waste an extra exchange
**	    if the positioned key is a match.
**	12-oct-92 (daveb)
**	    include <dmrcb.h> because of <gwfint.h> prototypes.  Include
**	    new <sc0alloc.h> to get to sc0m_gcalloc and sc0m_gcfree.  Use
**	    those routines for allocation of server-global memory.  Add
**	    GM_ualloc for ulm stream allocation.
**	17-Oct-1992 (daveb)
**	    remove sc0m refs; they are bad.  use MEreqmem for now FIXME.
**	11-Nov-1992 (daveb)
**	    Use kfirst_place to seed the cur_key.  This
**	    drives all the GCN query stuff the first time.
**	14-Nov-1992 (daveb)
**	    pull listen addr in GM_server from GCA MO.
**	    In GM_pos_sub, Set cur_key to first_place if user didn't
**	    set it.  Just clear keys in open_sub.
**	24-Nov-1992 (daveb)
**	    Figure the guises once at open and stash in the GM_RSB.
**	15-Dec-1992 (daveb)
**	    With 12/14/92 intergration SCI_SESS_USER is not SCI_SESSION_USER.
**	12-Jan-1993 (daveb)
**	    In GM_alloc, log cl_stat, not dead local_error.
**	1-Feb-1993 (daveb)
**	    Remove "PLACE" definition in GM_atts; it isn't legal anymore.
**	3-Feb-1993 (daveb)
**	    improve documentation.
**	09-apr-1993 (ralph)
**	    DELIM_IDENT:
**	    Make comparisons with DB_INGRES_NAME and DB_NDINGRES_NAME
**	    case insensitive.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	5-Jan-1994 (daveb) 58358
**	    Add GM_get_pmsym
**      26-Jan-1994 (daveb) 59125
**          Add some tracing routines
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in GM_free(),
**	    GM_dumpmem().
**      12-jun-1995 (canor01)
**          semaphore protect memory allocation (MCT)
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**	    Changed tail_const to unsigned to quiet Solaris compiler warnings:
**	    "initializer does not fit or is out of range".
**	 9-Sep-96 (gordy)
**	    The GCA MIB symbol changed due to changes in GCA global data.
**	30-sep-96 (canor01)
**	    unsigned i4 changed to u_i4 to work around compiler
**	    error ib NT.
**      06-Mar-98 (fanra01)
**          Modified to include a check for the SERVER_CONTROL privilege.
**          If the session user has this privilege the MO_SERVER_READ and
**          MO_SERVER_WRITE rights are permitted.
**          Also get the maxustat instead of ustat to include role privileges
**          in the guise check.
**	 2-Oct-98 (gordy)
**	    Moved GCA MIB definitions to gcm.h.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	01-oct-1999 (somsa01)
**	    Modified GM_chk_priv() so that it does not use PM calls to
**	    retrieve the config.dat privileges, but rather SCU_INFORMATION.
**	    This eradicates a memory leak due to continuous calls to PMget().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**	17-Jan-2007 (kibro01)
**	    Moved GM_ATT_DESC array of IS clause values to back/hdr/gwf.h
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
[@history_template@]...
*/


/*{
** Name:	GM_my_server	- this server's place.
**
** Description:
**	The place for this server.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	none.
**
** Outputs:
**	none
**
** Returns:
**	string to use for the place.
**
** History:
**	25-sep-92 (daveb)
**	    Documented.
**	14-Nov-1992 (daveb)
**	    pull listen addr from GCA MO.
**	2-Dec-1992 (daveb)
**	    only do it once, rename to GM_my_server().
**	 9-Sep-96 (gordy)
**	    The GCA MIB symbol changed due to changes in GCA global data.
*/

char * 
GM_my_server(void)
{
    STATUS	cl_stat;
    char	value[ GM_MIB_VALUE_LEN + 1 ];
    i4		len = sizeof(value);
    i4		perms;

    if( GM_globals.gwm_this_server[0] == EOS )
    {
	cl_stat = MOget( ~0, GCA_MIB_CLIENT_LISTEN_ADDRESS, 
			 "0", &len, value, &perms );
	if( cl_stat != OK )
	{
	    /* "GWM:  Internal error:  Can't find my GCA listen address:" */
	    GM_error( E_GW8301_NO_LISTEN_ADDR );
	    GM_error( cl_stat );
	    GM_globals.gwm_this_server[0] = EOS;
	}
	else
	{
	    STprintf( GM_globals.gwm_this_server, "%s::/@%s",
		     GM_my_vnode(), value );
	}
    }
    return( GM_globals.gwm_this_server );
}


/*{
** Name:	GM_my_vnode	- this vnode.
**
** Description:
**	Return a place where this server's installation-wide
**	stuff can be found.
**
**	FIXME:  must this be unique, or will any do?
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	none.
**
** Outputs:
**	none
**
** Returns:
**	string of the place.
**
** History:
**	25-sep-92 (daveb)
**	    Documented.
**	11-Nov-1992 (daveb)
**	    return def vnode.
*/
char * 
GM_my_vnode(void)
{
    return( GM_globals.gwm_def_vnode );
}


/*{
** Name:	GM_gt_dba	- return the current DBA name
**
** Description:
**	Gets the owner of the current database.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	dba		address of a DB_OWN_NAME to fill in.
**
** Outputs:
**	dba		buffer filled in with the dba name.
**
** Returns:
**	DB_STATUS
**
** History:
**	3-Feb-1993 (daveb)
**	    documented.
*/

DB_STATUS
GM_gt_dba( DB_OWN_NAME *dba )
{
    SCF_CB      scf_cb;
    SCF_SCI     sci_list;
    DB_STATUS	db_stat;

    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_len_union.scf_ilength = 1;
    /* may cause lint message */
    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*) &sci_list;
    
    sci_list.sci_code = SCI_DBA;
    sci_list.sci_length = sizeof(*dba);
    sci_list.sci_aresult = (PTR) dba;
    sci_list.sci_rlength = NULL;
    db_stat = scf_call(SCU_INFORMATION, &scf_cb);
    
    if( db_stat != OK )
    {
	GM_error( E_GW8303_DBA_ERROR );
	GM_error( scf_cb.scf_error.err_code );
    }
    return( db_stat );
}


/*{
** Name:	GM_gt_user	- return the current user name
**
** Description:
**	Gets the current session effective user name
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	user		address of a DB_OWN_NAME to fill in.
**
** Outputs:
**	user		buffer filled in with the dba name.
**
** Returns:
**	DB_STATUS
**
** History:
**	3-Feb-1993 (daveb)
**	    documented.
*/
DB_STATUS
GM_gt_user( DB_OWN_NAME *user )
{
    SCF_CB      scf_cb;
    SCF_SCI     sci_list;
    DB_STATUS	db_stat;

    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_len_union.scf_ilength = 1;
    /* may cause lint message */
    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*) &sci_list;
    
    sci_list.sci_code = SCI_SESSION_USER;
    sci_list.sci_length = sizeof(*user);
    sci_list.sci_aresult = (PTR) user;
    sci_list.sci_rlength = NULL;
    db_stat = scf_call(SCU_INFORMATION, &scf_cb);
    
    if( db_stat != OK )
    {
	GM_error( E_GW8304_USER_ERROR );
	GM_error( scf_cb.scf_error.err_code );
    }
	    
    return( db_stat );
}


/*{
** Name:	GM_my_guises	- active guises for this session
**
** Description:
**	Returns the active guises appropriate for this session,
**	in this server.  Maps SCF stuff to MO guises.
**
**	The MO guises are:
**
**		MO_SES_{READ,WRITE}
**		MO_DBA_{READ,WRITE}
**		MO_SERVER_{READ,WRITE}
**		MO_SYSTEM_{READ,WRITE}
**		MO_SECURITY_{READ,WRITE}
**
**	The SCF-relevant information available includes:
**
**	SCS_ICS.ruid	
**		.ics_susername	session username
**		.ics_rusername	real username
**		.ics_eusername	effective username
**
**		.ics_dbname	database name
**		.ics_dbowner	DBA
**		.ics_appl_code	-i flag
**		.ics_sessid	???
**
**		.ics_sgrpid	session group id
**		.ics_egrpid	effective group id
**		.ics_saplid	session application id
**		.ics_eaplid	effective application id
**
**		.ics_rustat;	real user status bits,
**		.ics_sustat;	session user status bits,
**
**      both including:
**
**		DU_UCREATEDB	can create db's
**		DU_UDRCTUPDT	can specify direct update
**		DU_UUPSYSCAT	can update system cats
**		DU_UTRACE	can use trace flags
**		DU_UQRYMODOFF	can turn off qrymod
**		DU_UAPROCTAB	can use any proctab
**		DU_UEPROCTAB	can use =proctab
**		DU_USECURITY	can perform security operations.
**		DU_UOPERATOR	can perform operator functions (backup)
**		DU_UAUDIT	AUDIT all is on for this user.
**		DU_USYSADMIN	can do system admin funcitons (create locations).
**		DU_UDOWNGRADE	can downgrade data (B1)
**		DU_USUPER	ingres superuser.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	MO guises appropriate for the sessions.
**
** History:
**	25-sep-92 (daveb)
**	    Documented.
**	23-Nov-1992 (daveb)
**	    Describe more fully, but still stubbed.
**	24-Nov-1992 (daveb)
**	    Do the mapping based in SCI_USTAT (real user) permissions.
**	7-jul-93 (robf)
**	    Remove SUPERUSER priv which is gone
**      06-Mar-98 (fanra01)
**          Modified to include a check for the SERVER_CONTROL privilege.
**          If the session user has this privilege the MO_SERVER_READ and
**          MO_SERVER_WRITE rights are permitted.
**          Also get the maxustat instead of ustat to include role privileges
**          in the guise check.
*/
i4
GM_my_guises(void)
{
    i4	guises = (MO_SES_READ | MO_SES_WRITE);

    SCF_CB      scf_cb;
    SCF_SCI     sci_list;
    DB_STATUS	db_stat;
    i4		ustat;		/* user status */
    DB_OWN_NAME	dba;		/* the DBA */
    DB_OWN_NAME	sess_user;	/* the real user */

    do				/* one time only */
    {
	/* get user status bits */
	
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_GWF_ID;
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_len_union.scf_ilength = 1;
	/* may cause lint message */
	scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*) &sci_list;
	sci_list.sci_code = SCI_MAXUSTAT;
	sci_list.sci_length = sizeof(ustat);
	sci_list.sci_aresult = (PTR) &ustat;
	sci_list.sci_rlength = NULL;
	db_stat = scf_call(SCU_INFORMATION, &scf_cb);
	if (db_stat != E_DB_OK)
	{
	    GM_error( E_GW8306_USER_PRIV_ERR );
	    break;
	}
	
	if( ustat & (DU_UTRACE | DU_USYSADMIN | DU_USECURITY) )
	    guises |= (MO_SERVER_READ | MO_SERVER_WRITE);

	if( ustat & (DU_UOPERATOR | DU_USECURITY)  )
	    guises |= (MO_SYSTEM_READ | MO_SYSTEM_WRITE);

	if( ustat & (DU_USECURITY) )
	    guises |= (MO_SECURITY_READ | MO_SECURITY_WRITE);

	if( (db_stat = GM_gt_dba( &dba )) != E_DB_OK )
	    break;

	if( (db_stat = GM_gt_user( &sess_user )) != E_DB_OK )
	    break;

	if( !STbcompare( (char *)&dba.db_own_name, sizeof(dba.db_own_name),
			(char *)&sess_user.db_own_name, 
			sizeof(sess_user.db_own_name),
			FALSE ) )
	    guises |= (MO_DBA_READ | MO_DBA_WRITE);

        if (GM_chk_priv ((char *)&sess_user.db_own_name,
                         "SERVER_CONTROL") == TRUE)
        {
            guises |= (MO_SERVER_READ | MO_SERVER_WRITE);
        }

    } while( FALSE );
    
    return( guises );
}


/*{
** Name:	GM_gt_sem - get a semaphore, logging errors.
**
** Description:
**	get sem, log errors.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem	sem to grab.
**
** Outputs:
**	none.
**
** Returns:
**	OK or error status.
**
** Side Effects:
**	Logs any errors with GM_error.
**
** History:
**	22-sep-92 (daveb)
**	    Created, untested.
**      24-Mar-1994 (daveb)
**          show sem on error.
*/

STATUS
GM_gt_sem( CS_SEMAPHORE *sem )
{
    STATUS cl_stat;

    if( OK != (cl_stat = CSp_semaphore( TRUE, sem ) ) )
    {
	GM_error( cl_stat );
	TRdisplay( "[GWM Error getting semaphore 0x%p]\n", sem );
    }
    return( cl_stat );
}


/*{
** Name:	GM_i_sem - init semaphore, logging errors.
**
** Description:
**	init sem, log errors.
**	
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem	sem to init.
**
** Outputs:
**	none.
**
** Returns:
**	OK or error status.
**
** History:
**	22-sep-92 (daveb)
**	    created.
*/
STATUS
GM_i_sem( CS_SEMAPHORE *sem )
{
    STATUS cl_stat;

    if( OK != (cl_stat = CSi_semaphore( sem, CS_SEM_SINGLE ) ) )
	GM_error( cl_stat );

    return( cl_stat );
}


/*{
** Name:	GM_release_sem - unlock semaphore, logging errors.
**
** Description:
**	release sem, log errors.
**	
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem	sem to release.
**
** Outputs:
**	none.
**
** Returns:
**	OK or error status.
**
** History:
**	22-sep-92 (daveb)
**	    created.
*/
STATUS
GM_release_sem( CS_SEMAPHORE *sem )
{
    STATUS cl_stat;

    if( OK != (cl_stat = CSv_semaphore( sem ) ) )
	GM_error( cl_stat );

    return( cl_stat );
}



/*{
** Name:	GM_rm_sem - delete a semaphore, logging errors.
**
** Description:
**	delete sem, log errors.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem	sem to delete
**
** Outputs:
**	none.
**
** Returns:
**	OK or error status.
**
** Side Effects:
**	Logs any errors with GM_error.
**
** History:
**	22-sep-92 (daveb)
**	    Created, untested.
*/
STATUS
GM_rm_sem( CS_SEMAPHORE *sem )
{
    STATUS cl_stat;

    if( OK != (cl_stat = CSr_semaphore( sem ) ) )
	GM_error( cl_stat );

    return( cl_stat );
}



/*{
** Name:	GM_nat_compare	- compare two pointers as integers.
**
** Description:
**	Comaprison function of use with SPinit.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	a	PTR holding a i4.
**	b	PTR holding a i4
**
** Outputs:
**	none
**
** Returns:
**	< 0, 0, > 0, depending on ordering.
**
** History:
**	9-oct-92 (daveb)
**	    documented.
**	14-sep-93 (swm)
**	    Altered code to work on platforms where size of pointer is
**	    greater than size of int. Also, rather than returning the
**	    difference between the supplied addresses, return +1, 0 or -1
**	    indication - this is OK because this function is only used
**	    to implement an order relation.
*/
i4
GM_nat_compare( const char *a, const char *b )
{
    if (a == b)
    {
	return 0;
    }
    else
    {
	if (a > b)
		return 1;
	else	return -1;
    }
}



/*{
** Name:	GM_alloc	- allocate global/permanent memory.
**
** Description:
**
**	Allocates server-permanent memory for GM, which is not
**	session specific.  This is used for place/server/connection
**	blocks.  Servers and connections may be freed as things
**	come and go.  This doesn't fit well with existing server
**	models:  SCF memory is in big chunks, and ULM memory can't
**	be release in small pieces.  We use MEreqmem, which also
**	isn't good.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	len		bytes to allocate, 32k Max.
**
** Outputs:
**	None.
**
** Returns:
**	PTR		of newly allocated memory.
**	NULL		if allocation failed.
**
** Side Effects:
**	Logs any errors.
**
** History:
**	23-sep-92 (daveb)
**	    Created.
**	17-Oct-1992 (daveb)
**	    remove sc0m refs; they are bad.  use MEreqmem for now FIXME.
**	11-Nov-1992 (daveb)
**	    No semcolon after if!  Was always logging errors on
**	    any operation!
**	12-Jan-1993 (daveb)
**	    Log cl_stat, not dead local_error.
**	03-jun-1996 (canor01)
**	    Changed tail_const to unsigned to quiet compiler warnings.
*/

# define GUARD_STRING "GUARD"
# define GUARD_HEAD_CONST  0xfeedbeef
# define GUARD_TAIL_CONST  0xfc21fc22

static u_i4 tail_const = GUARD_TAIL_CONST;

# define EXTRA (3 * sizeof(i4) )

PTR
GM_alloc( i4  len )
{
    STATUS	cl_stat;
    PTR		ret_block;
    i4	*block;

    /* round up len to ALIGN_RESTRICT */

    ret_block = MEreqmem( 0, len + EXTRA, 0, &cl_stat );
    if( ret_block == NULL )
    {
	/* "GWM:  Internal error:  Couldn't allocate %0d bytes of memory" */

	GM_1error( (GWX_RCB *)0, E_GW8302_ALLOC_ERROR, GM_ERR_INTERNAL|GM_ERR_LOG,
		  sizeof( len ), (PTR)&len );
	GM_error( cl_stat );
    }
    else			/* setup guards */
    {
	block = (i4 *)ret_block;
	block[0] = GUARD_HEAD_CONST;
	block[1] = len;
	ret_block = (PTR)&block[2];
	block = (i4 *) ((char *)ret_block + len);
	MEcopy( (PTR)&tail_const, sizeof(tail_const), (PTR)block );
    }
	
    return( ret_block );
}


	
/*{
** Name:	GM_free	- release memory obtained from GM_alloc
**
** Description:
**	Releases memory obtained with GM_free for other use.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	block		the memory to free.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	7-oct-92 (daveb)
**	    Documented.
**	17-Oct-1992 (daveb)
**	    remove sc0m refs; they are bad.  use MEreqmem for now FIXME.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/
void
GM_free( PTR ret_block )
{
    i4 *block;
    u_i4 tail;

    block = (i4 *)((char *)ret_block - (2 * sizeof(i4)));

    if( block[0] != GUARD_HEAD_CONST )
    {
	TRdisplay("GWM:  corrupt head guard %x for %p\n", block[0], block );
    }
    if( block[1] < 0 || block[1] > MAXI2 )
    {
	TRdisplay("GWM:  bad block size %d for %p\n", block[1], block );
    }
    else
    {
	MEcopy((PTR)((char *)ret_block + block[1]), sizeof(tail), (PTR)&tail );
	if( tail != tail_const )
	    TRdisplay("GWM:  corrupt tail guard %x for %p\n", tail, block );
    }
    MEfree( (PTR)block );
}


/*{
** Name:	GM_att_offset	- offset of data for att in tuple
**
** Description:
**
**	Return the byte offset into a tuple where the data
**	for att_number is located.  This is called once per att per
**	table open.
**
**	Note that att_numbers wrun from 1..Ncols, and exclude 0.
**	Thus there is high liklihood of off-by-one and fencepost
**	errors.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwm_rsb.
**		gwm_gwm_atts	array indexed by column, not key_seq.
**
**	att_number		the key of interest, 1..Ncols.
**
** Outputs:
**	none.	
**
** Returns:
**	byte offset of the key in a tuple buffer.
**
** History:
**	11-Nov-1992 (daveb)
**	    modified from GM_key_offset.
*/

i4
GM_att_offset( GM_RSB *gwm_rsb, i4  att_number )
{
    i4  i;
    i4  offset;
    DMT_ATT_ENTRY	*dmt_att;
    
    offset = 0;

    /* loop over all columns.  For each one that has a
       attno less than the one of interest, add it's width to the
       accumulating offset */
       
    for( i = 0; i < gwm_rsb->ncols; i++ )
    {
	dmt_att = gwm_rsb->gwm_gwm_atts[ i ].gma_dmt_att;
	if( dmt_att->att_number > 0 && dmt_att->att_number < att_number )
	{
	    offset += dmt_att->att_width;
	}
    }
    return( offset );
}



/*{
** Name:	GM_key_offset	- offset of data for key in key buffer.
**
** Description:
**
**	Return the byte offset into a key buffer where the data
**	for key_seq N may be located.  This is called once per key per
**	table open.
**
**	Note that key_seq numbers wrun from 1..Ncols, and exclude 0.
**	Thus there is high liklihood of off-by-one and fencepost
**	errors.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwm_rsb.
**		gwm_gwm_atts	array indexed by column, not key_seq.
**
**	key_seq			the key of interest, 1..Ncols.
**
** Outputs:
**	none.	
**
** Returns:
**	byte offset of the key in a key buffer.s
**
** History:
**	29-sep-92 (daveb)
**	    Created
*/

i4
GM_key_offset( GM_RSB *gwm_rsb, i4  key_seq )
{
    i4  i;
    i4  offset;
    DMT_ATT_ENTRY	*dmt_att;
    
    offset = 0;

    /* loop over all columns.  For each one that is a key, and has a
       key_seq less than the one of interest, add it's width to the
       accumulating offset */
       
    for( i = 0; i < gwm_rsb->ncols; i++ )
    {
	dmt_att = gwm_rsb->gwm_gwm_atts[ i ].gma_dmt_att;
	if( dmt_att->att_key_seq_number > 0 &&
	   dmt_att->att_key_seq_number < key_seq )
	    offset += dmt_att->att_width;
    }
    return( offset );
}


/*
** Name: GM_open_sub	- Common open operations for MOB and XTAB.
**
** Description:
**
**	Allocate appropriate RSBs and initialize the standard
**	data.  The MOB and XTAB gateways may subsequently set
**	up data in the rsb that is module-specific.
**
** Inputs:
**
**   gwx_rcb->
**	xrcb_access_mode
**			table access mode, from gwr_access_mode
**	xrcb_rsb
**			GW_RSB for this table.
**
**	    .gwrsb_rsb_mstream
**			Stream to use for memory.
**	    .gwrsb_tcb->gwt_attr
**			INGRES and extended attributes for this table.
**
**	xrcb_data_base_id
**			PTR database id from gwr_database_id.
**	xrcb_xact_id
**			transaction id, from gwr_xact_id.
**	xrcb_tab_name
**			table name, from gwr_tab_name.
**	xrcb_tab_id
**			table id, from gwr_tab_id.
**
**   rsb_size		The size of the rsb to allocate.
**
** Outputs:
**	xrcb_rsb.gwrsb_page_cnt
**			set to a likely value.
**	xrcb_rsb.gwrsb_internal_rsb
**			Gets pointer to our allocated and set up GX_RSB.
**	xrcb_error.err_code
**			E_DB_OK or error status.
**
** Side Effects:
**	Memory is allocated from xrcb_rsb.gwrsb_rsb_mstream.
**
** Returns:
**	E_DB_OK or E_DB_ERROR.
**
** History:
**	19-feb-91 (daveb)
**	    Created.
**	5-dec-91 (daveb)
**	    modified for vectored out table handling.
**	20-Jul-92 (daveb)
**	    Record where "PLACE" goes in the out tuple.
**	28-sep-92 (daveb)
**	    Created from GXopen and GOopen.
**	11-Nov-1992 (daveb)
**	    Use kfirst_place to seed the cur_key.  This
**	    drives all the GCN query stuff the first time.
**	14-Nov-1992 (daveb)
**	    just clear keys here; they will be reset in pos_sub.
**	18-Nov-1992 (daveb)
**	    zap status in opblks to make sure caching isn't primed.
**	24-Nov-1992 (daveb)
**	    Figure the guises once, here at open.
**	25-Nov-1992 (daveb)
**	    Fill in first guess at att_type here too.
**	2-Dec-1992 (daveb)
**	    Fill in dom_type.
**	3-Dec-1992 (daveb)
**	    Update dom type based only on the #1 key.  Others don't count.
**	8-Dec-1992 (daveb)
**	    Add nkeys, and gma_key_type.
**	13-Jan-1993 (daveb)
**	    Correct handling of gma_key_type setting -- was looking for
**	    key 0 instead of proper key 1, so domain type was being set
**	    incorrectly.  This resulted in off-server stuff failing!
**	06-oct-93 (swm)
**	    Bug #56443
**	    Changed comment "PTR/i4 database id from gwr_database_id."
**	    to "PTR database id from gwr_database_id." as gwr_database_id
**	    is now a PTR. On axp_osf PTR and i4 are not the same size.
*/

DB_STATUS
GM_open_sub( GWX_RCB *gwx_rcb, i4  rsb_size )
{
    GW_RSB		*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    DB_STATUS		db_stat;
    GM_RSB		*gwm_rsb;
    GW_ATT_ENTRY	*gwatt;
    GM_ATT_ENTRY	*gatt;
    GM_ATT_ENTRY	*gwm_atts;
    DMT_ATT_ENTRY	*dmt_att;
    GM_XATT_TUPLE	*xatp;
    i4			ncols;
    i4			nkeys;
    i4			att_col;
    i4			key_num;
    i4			i;
    i4			type;

    /*
    ** Per server conventions, loop is used to break out of an error.
    ** Thus all returns are through the bottom of the procedure.
    */
    db_stat = E_DB_OK;
    do
    {
	/*
	** Allocate and initialize the MO gateway specific RSB.
	** This control information is allocated from the memory stream
	** associated with the record stream control block.
	**
	** This grabs all the memory we need for whatever RSB and an
	** associated array of attributes in one allocation.  We split
	** it up by hand.
	*/
 	ncols = rsb->gwrsb_tcb->gwt_table.tbl_attr_count;
	gwm_rsb = (GM_RSB *)GM_ualloc( rsb_size +
				      (ncols * sizeof(GM_ATT_ENTRY)),
				      &rsb->gwrsb_rsb_mstream,
				      (CS_SEMAPHORE *)NULL );
	if ( gwm_rsb == NULL )
	{
	    db_stat = E_DB_ERROR;
	    break;
	}
	gwm_rsb->gwm_gwm_atts = (GM_ATT_ENTRY *)((char *)gwm_rsb + rsb_size);
	gwm_atts = gwm_rsb->gwm_gwm_atts;
 	gwm_rsb->ncols = ncols;
	gwm_rsb->gwm_att_list = rsb->gwrsb_tcb->gwt_attr;

	gwm_rsb->guises = GM_my_guises();
	    
	/* First scan for attr data */
	
	for( i = 0; i < ncols ; i++ )
	{
	    gwatt = &gwm_rsb->gwm_att_list[ i ];
	    dmt_att = &gwatt->gwat_attr;
	    xatp = (GM_XATT_TUPLE *)gwatt->gwat_xattr.data_address;

	    /* note: att_numbers run 1..N instead of 0..N */
	    
	    att_col = dmt_att->att_number - 1;
	    type = GM_att_type( xatp->xatt_classid );
	    gatt = &gwm_atts[ att_col ];
	    gatt->gma_dmt_att = dmt_att;
	    gatt->gma_col_classid = xatp->xatt_classid;
	    gatt->gma_type = type;

	}
	
	/* now scan for buffer offsets and key related stuff.
	   This is slow,  but better once here than on every
	   position or lookup.  We don't do this in the previous
	   pass because we had to locate all the dmt_att's first. */
	
	gwm_rsb->dom_type = GMA_UNKNOWN;
	for( nkeys = i = 0; i < ncols ; i++ )
	{
	    dmt_att = gwm_atts[ i ].gma_dmt_att;

	    /* attribute offsets */

	    gwm_atts[ dmt_att->att_number - 1 ].gma_att_offset = 
		    GM_att_offset( gwm_rsb, dmt_att->att_number );

	    /* key stuff:  buffer offsets and dmt_att of key */

	    if( (key_num = dmt_att->att_key_seq_number) > 0 )
	    {
		nkeys++;
		gatt = &gwm_atts[ key_num - 1 ];
		gatt->gma_key_offset = GM_key_offset( gwm_rsb, key_num );
		gatt->gma_key_type = gwm_atts[ i ].gma_type;
		gatt->gma_key_dmt_att = gwm_atts[ i ].gma_dmt_att;

		/* Check 'place' as first key column, if any */

		if( key_num == 1 )
		{
		    switch( gatt->gma_type )
		    {
		    case GMA_VNODE:
			gwm_rsb->dom_type = GMA_VNODE;
			break;
			
		    case GMA_SERVER:
		    case GMA_PLACE:
			if( gwm_rsb->dom_type != GMA_VNODE )
			    gwm_rsb->dom_type = gatt->gma_type;
			break;
			
		    default:	/* doesn't affect place iteration */
			break;
		    }
		}
	    }

	}
	gwm_rsb->nkeys = nkeys;

# ifdef xDEBUG
	if( GM_globals.gwm_trace_opens )
	{
	    TRdisplay("  GMOPENSUB: table gwm_rsb %p:\n", gwm_rsb );
	    TRdisplay("  attributes:\n");
	    for( i = 0; i < ncols ; i++ )
	    {
		TRdisplay("    ==== attr %d ====\n", i );
		GM_dump_gwm_att( "", &gwm_atts[ i ]);
		GM_dump_dmt_att( "", gwm_atts[ i ].gma_dmt_att );
	    }

	    TRdisplay( nkeys == 0 ? "    (no keys)\n" : "\n    keys:\n");
	    for( i = 0; i < nkeys ; i++ )
	    {
		TRdisplay("    ==== key %d ===\n", i );
		GM_dump_gwm_katt( "", &gwm_atts[ i ] );
		GM_dump_dmt_att( "", gwm_atts[ i ].gma_key_dmt_att );
	    }
	}
# endif

	/* now init the current and last keys for the stream */

	GM_clearkey( &gwm_rsb->cur_key );
	GM_clearkey( &gwm_rsb->last_key );
	
	gwm_rsb->didfirst = FALSE;
	gwm_rsb->request.status = FAIL;
	gwm_rsb->request.used = 0;
	gwm_rsb->response.status = FAIL;
	gwm_rsb->response.used = 0;
	
	/* wrap up output variables */

	rsb->gwrsb_internal_rsb = (PTR)gwm_rsb;
	rsb->gwrsb_page_cnt = GM_DEF_PAGE_CNT;

    } while( FALSE );

    return( db_stat );
}



/*{
** Name:	GM_ualloc	- allocate out of a ulm stream
**
** Description:
**	allocate memory in a stream that will vanish at an
**	appropriate time.  No explicit free is provided
**	Returns PTR to the memory, or NULL and logged the error.
**
** Re-entrancy:
**	yes, assuming sem is held.
**
** Inputs:
**	len		memory to allocate
**	ulm_rcb		open stream identifier.
**	sem		semaphore to lock stream.
**
** Outputs:
**	none
**
** Returns:
**	PTR		to the new memory, or NULL with errors logged.
**
** History:
**	7-oct-92 (daveb)
**	    created.
*/

PTR
GM_ualloc( i4  len, ULM_RCB *ulm_rcb, CS_SEMAPHORE *sem )
{
    PTR		ret_block = NULL;

    if( sem == NULL || GM_gt_sem( sem ) == OK )
    {
	ulm_rcb->ulm_psize = len;
	if( ulm_palloc( ulm_rcb ) != E_DB_OK )
	{
	    /* "GWM:  Internal error:  Couldn't get %0d bytes out of
	       session memory" */

	    GM_1error( (GWX_RCB *)0, E_GW8305_SESS_ALLOC_ERROR,
		      GM_ERR_INTERNAL|GM_ERR_LOG, sizeof(len), (PTR)&len );
	    GM_error( ulm_rcb->ulm_error.err_code );
	}
	else
	{
	    ret_block = ulm_rcb->ulm_pptr;
	}
	if( sem != NULL )
	    (void) GM_release_sem( sem );
    }
    return( ret_block );
}



/*{
** Name:	GM_dumpmem	-- dump memory using TRdisplay
**
** Description:
**	Dumps memory in hex and char format using TRdisplay.  For
**	calling inside a debugger.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	mem	pointer to first byte to dump.
**	len	number of bytes to dump.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	13-Nov-1992 (daveb)
**	    use unsigned char to pick out bytes to avoid
**	    sign-extension problems.  Was printing badness 
**	    for values > 0x7f
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/

void
GM_dumpmem( char *smem, i4  len )
{
    i4  i, j;
    i4  c;
    char hbuf[ 12 ];
    unsigned char *mem = (unsigned char *)smem;

    /* print a line */
    TRdisplay("dumping %d bytes at %p\n", len, mem );
    for( i = 0; i < len; i+= 16 )
    {
	TRdisplay("%p ", mem + i );
	for( j = 0; j < 16 && (i+j) < len; j++ )
	{
	    c =  mem[ i + j ];
	    STprintf(hbuf, "%02x ", c );
	    TRdisplay("%p ", hbuf );
	}
	TRdisplay("\n         ");
	for( j = 0; j < 16 && (i+j) < len; j++ )
	{
	    c =  mem[ i + j ];
	    STprintf(hbuf, "%c  ", CMprint(&c) ? c : ' ' );
	    TRdisplay("%s ", hbuf );
	}
	TRdisplay("\n");
    }
}


/*{
** Name:	GM_tabf_sub	- common operations for MOB and XTAB TABF.
**
** Description:
**	The guts of register table.
**
**	Setup the extended relation and attribute entries for the appropriate
**	sub-gateway.  Enforce as much as possible the restrictions on ingres
**	registration in ingres-owned databases.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	 gwx_rcb->xrcb_tab_name
**			The name of the table being registered.
**
**	 gwx_rcb->xrcb_tab_id
**			INGRES table id.
**
**	 gwx_rcb->xrcb_gw_id
**			Gateway id, derived from DBMS type.
**			clause of the register statement.
**
**	 gwx_rcb->xrcb_var_data1
**			source of the gateway table, in the 'from'
**			clause of the 'register' statement.  Ignored here.
**
**	 gwx_rcb->xrcb_var_data2
**			path of the gateway table.
**			How does this differ from the source? FIXME
**			Ignored here anyway.
**
**	 gwx_rcb->xrcb_column_cnt
**			column count of the table, number of elements
**			in column_attr and var_data_list.
**
**	 gwx_rcb->column_attr
**			INGRES column information (array of DMT_ATT_ENTRYs).
**
**	 gwx_rcb->xrcb_var_data_list
**			Array of extended column attribute strings,
**			from 'is' clause of the target list of the
**			register statement.  A null indicates no
**			extended attributes.  
**
**	 gwx_rcb->xrcb_var_data3
**			tuple buffer for the iigwX_relation
**
**	 gwx_rcb->xrcb_mtuple_buffs
**			An allocated array of iigwX_attribute tuple buffers.
**
**	xrel_flags	flags to put in extended relation to identify the
**			sub-gateway, either GM_MOB_TABLE or GM_XTAB_TABLE.
**
**  Outputs:
**	 gwx_rcb->xrcb_var_data3
**			iigwX_relation tuple.
**
**	 gwx_rcb->xrcb_mtuple_buffs
**			iigwX_attribute tuples.
**
**	 gwx_rcb->xrcb_error.err_code
**			E_GW0000_OK or E_GW0001_NO_MEM.
**
**  Returns:
**	 E_DB_OK
**	 E_DB_ERROR.
**
** History:
**	3-Feb-1993 (daveb)
**	    documented
**	09-apr-1993 (ralph)
**	    DELIM_IDENT:
**	    Make comparisons with DB_INGRES_NAME and DB_NDINGRES_NAME
**	    case insensitive.
*/

DB_STATUS
GM_tabf_sub( GWX_RCB *gwx_rcb, i4 xrel_flags )
{
    DB_OWN_NAME	dba;
    DB_OWN_NAME	user;
    DB_NAME dbname;
    DB_STATUS db_stat = E_DB_OK;

    GM_XREL_TUPLE	*xrtp;
    GM_XATT_TUPLE	*xatp;
    DM_DATA		*data;
    DM_PTR		*ptr;
    i4			numatts;
    i4			errnat;

    i4			journaled;
    i4			structure;
    i4			unique;
    i4			update;


    do
    {
	if( (db_stat = GM_gt_dba( &dba ) ) != E_DB_OK )
	    break;

	if( (db_stat = GM_gt_user( &user ) ) != E_DB_OK )
	    break;

	if( (db_stat = GM_dbname_gt( &dbname ) ) != E_DB_OK )
	    break;

	if( STbcompare(  DB_NDINGRES_NAME, sizeof(DB_NDINGRES_NAME)-1,
		       (char *)&dba.db_own_name, sizeof( dba.db_own_name ), 
		       TRUE ) &&
	   STbcompare(  DB_INGRES_NAME, sizeof(DB_INGRES_NAME)-1,
		      (char *)&dba.db_own_name, sizeof( dba.db_own_name ), 
		      TRUE ) )
	{
	   /* "IMA:  You may only open IMA tables in a database owned
	      by ingres. The current database '%0c' is owned by '%1c'" */

	    GM_2error( (GWX_RCB *)0, E_GW80C3_INGRES_NOT_DBA,
		      GM_ERR_USER|GM_ERR_LOG,
		      GM_dbslen( dbname.db_name ), (PTR)&dbname.db_name,
		      GM_dbslen( dba.db_own_name ), (PTR)&dba.db_own_name );
	    db_stat = E_DB_ERROR;
	    break;
	}

	if( STbcompare( DB_NDINGRES_NAME, sizeof(DB_NDINGRES_NAME) - 1,
		       (char *)&user.db_own_name, sizeof( user.db_own_name ), 
		       TRUE )
	   &&
	   STbcompare(  DB_INGRES_NAME, sizeof(DB_INGRES_NAME) - 1,
		      (char *)&user.db_own_name, sizeof( user.db_own_name ), 
		      TRUE ) )
	{
	    /* "IMA:  Only ingres may register IMA tables.  You are '%0c'." */

	    GM_1error( (GWX_RCB *)0, E_GW80C5_INGRES_MUST_REGISTER,
		      GM_ERR_USER|GM_ERR_LOG,
		      GM_dbslen(user.db_own_name), (PTR)&user.db_own_name);
	    db_stat = E_DB_ERROR;
	}


	/* stuff from GOtabf and GXtabf */

	data = &gwx_rcb->xrcb_var_data3;
	if( data->data_in_size != sizeof( *xrtp ) )
	{
	    /* "GWM: Internal error: Wrong width for iigw07_relation.
	       Got %0d, wanted %1d.  Maybe the SQL declaration is wrong?" */

	    errnat = sizeof( *xrtp );
	    GM_2error( gwx_rcb, E_GW8184_BAD_XREL_BUF,
		      GM_ERR_INTERNAL|GM_ERR_LOG,
		      sizeof( data->data_in_size ),
		      (PTR)&data->data_in_size,
		      sizeof( errnat ), (PTR)&errnat );
	    db_stat = E_DB_ERROR;
	    break;
	}

	GM_info_reg( gwx_rcb, &journaled, &structure, &unique, &update );

	xrtp = (GM_XREL_TUPLE *)data->data_address;
	xrtp->xrel_tblid = *gwx_rcb->xrcb_tab_id;
	xrtp->xrel_flags = xrel_flags;
	if( journaled )
	    xrtp->xrel_flags |= GM_JOURNALED;
	if( update )
	    xrtp->xrel_flags |= GM_UPDATE_OK;
	if( unique )
	    xrtp->xrel_flags |= GM_UNIQUE;
	if( structure == DB_BTRE_STORE )
	    xrtp->xrel_flags |= (GM_SORTED|GM_KEYED);
	if( structure == DB_HASH_STORE )
	    xrtp->xrel_flags |= GM_KEYED;

	/* check attribute inputs */

	numatts = gwx_rcb->xrcb_column_cnt;
	ptr = &gwx_rcb->xrcb_mtuple_buffs;

	if( ptr->ptr_in_count != numatts )
	{
	    /*  "GWM: Internal error: caller gave wrong number of attributes.
		Wanted %0d, got %1d." */

	    GM_2error( gwx_rcb, E_GW8184_BAD_XREL_BUF, 
		      GM_ERR_INTERNAL|GM_ERR_LOG,
		      sizeof( ptr->ptr_in_count ), (PTR)&ptr->ptr_in_count,
		      sizeof(numatts), (PTR)&numatts );
	    db_stat = E_DB_ERROR;
	    break;
	}
	else if ( ptr->ptr_size != sizeof(*xatp) )
	{
	    /* "GWM: Internal error: Wrong width for iigw07_attribute.
	       Got %0d, wanted %1d.  Maybe the SQL declaration is wrong?" */

	    errnat = sizeof( *xatp );
	    GM_2error( gwx_rcb, E_GW8187_BAD_XATT_BUF, 
		      GM_ERR_INTERNAL|GM_ERR_LOG,
		      sizeof( ptr->ptr_size ), (PTR)&ptr->ptr_size,
		      sizeof(errnat), (PTR)&errnat );
	    db_stat = E_DB_ERROR;
	}

    } while( FALSE );
    return( db_stat );
}


/*{
** Name:	GM_dbname_gt	- get the current database name.
**
** Description:
**	Return the database name, for use in error messages.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	dbname		DB_NAME to fill in.
**
** Outputs:
**	dbname		is filled in.
**
** Returns:
**	DB_STATUS
**
** History:
**	3-Feb-1993 (daveb)
**	    documented.
*/

DB_STATUS
GM_dbname_gt( DB_NAME *dbname )
{
    SCF_CB      scf_cb;
    SCF_SCI     sci_list;
    DB_STATUS	db_stat;

    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_len_union.scf_ilength = 1;
    /* may cause lint message */
    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*) &sci_list;
    
    sci_list.sci_code = SCI_DBNAME;
    sci_list.sci_length = sizeof(*dbname);
    sci_list.sci_aresult = (PTR) dbname;
    sci_list.sci_rlength = NULL;
    db_stat = scf_call(SCU_INFORMATION, &scf_cb);
    
    if( db_stat != OK )
    {
	GM_error( E_GW8307_DBNAME_ERR );
	GM_error( scf_cb.scf_error.err_code );
    }
    return( db_stat );
}


/*{
** Name:	GM_check_att	- is attribute a special IS clause name?
**
** Description:
**	Lookup the given name in the GM_atts[] array.  If present,
**	return E_DB_OK; if not, return E_DB_ERROR.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	name		the name to lookup.
**
** Outputs:
**	none.
**
** Returns:
**	E_DB_OK or E_DB_ERROR.
**
** History:
**	3-Feb-1993 (daveb)
**	    documented.
*/

DB_STATUS
GM_check_att( char *name )
{
    GM_ATT_DESC	*attp;

    for( attp = GM_atts; attp->name ; attp++ )
	if( !STncmp( name, attp->name, attp->namelen ) )
	    return( E_DB_OK );

    return( E_DB_ERROR );
}



/*{
** Name:	GM_att_type	- return type of IS clause attribute
**
** Description:
**	Lookup the given name in the GM_atts[] array.  Return 
**	either it's special type, or default to GMA_USER type on
**	the presumption it's an MO classid.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	name		the name to lookup.
**
** Outputs:
**	none.
**
** Returns:
**	A GMA_ type constant.
**
** History:
**	3-Feb-1993 (daveb)
**	    documented.
*/

i4
GM_att_type( char *name )
{
    GM_ATT_DESC	*attp;

    for( attp = GM_atts; attp->name ; attp++ )
	if( !STncmp( name, attp->name, attp->namelen ) )
	    return( attp->type );

    return( GMA_USER );
}


/*{
** Name:	GM_breakpoint	- no-op function to set a breakpoint on.
**
** Description:
**	Does nothing, but called in a few cases thought interesting
**	to debuggers.  
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
**	3-Feb-1993 (daveb)
**	    documented.
*/

void
GM_breakpoint()
{
}


/*{
** Name:	GM_just_vnode	-- extract vnode from place string.
**
** Description:
**
**	Pull out the vnode, leaving empty outbuf if place wasn't vnode:stuff
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	place		the source string.
**	olen		length of out buffer.
**
** Outputs:
**	outbuf		output buffer, always terminated.
**
** Returns:
**	none.
**
** History:
**	3-Feb-1993 (daveb)
**	    documented.
*/

void
GM_just_vnode( char *place, i4  olen, char *outbuf )
{
    char *src = place;
    char *dst = outbuf;
    
    while( olen-- && *src != EOS && *src != ':' )
	*dst++ = *src++;

    if( olen && *src == ':' )
	*dst = EOS;
    else
	*outbuf = EOS;
}


/*{
** Name:	GM_gca_addr_from	-- get GCA addr from place string.
**
** Description:
**	Pull out the GCA address from a place string of the form
**	[vnode::]/gca_address.  Leave empty outbuf if no gca address
**	was present.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	place		the source string.
**	olen		length of out buffer.
**
** Outputs:
**	outbuf		output buffer, always terminated.
**
** Returns:
**	none.
**
** History:
**	3-Feb-1993 (daveb)
**	    documented.
*/

void
GM_gca_addr_from( char *place, i4  olen, char *outbuf )
{
    char *src = place;
    
    while( --olen > 0 && *src && *src != '/' )
	src++;
    if( *src == '/' )
	STcopy( src, outbuf );
    else
	*outbuf = EOS;
}


/*{
** Name:	GM_dbslen	- DB_MAXNAME limited string length.
**
** Description:
**	Return length of string that can't exceed DB_MAXNAME, stopping
**	at a blank.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	str		the string to examine.
**
** Outputs:
**	none.
**
** Returns:
**	the length, not exceeding DB_MAXNAME.
**
** History:
**	3-Feb-1993 (daveb)
**	    documented.
*/

i4
GM_dbslen( char *str )
{
    i4  len;

    for( len = 0 ; len < DB_MAXNAME && *str != EOS && !CMspace( str ) ; len++ )
	CMnext( str );

    return( len );
}



/*{
** Name:	GM_info_reg	- get table structure info for register
**
** Description:
**	Pull out the relevant data structure information from the
**	control blocks handed a tabf or idxf function.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb->xrcb_dmu_chars		DMF table characteristics struct.
**
** Outputs:
**	journaled		set 0 if not journaled.
**	structure		set to DB_HEAP_STORE, DB_ISAM_STORE,
**					DB_HASH_STORE, or DB_BTRE_STORE
**	unique			set 0 if not unique.
**	update			set 0 if not updatable.
**
** Returns:
**	none.
**
** History:
**	3-Feb-1993 (daveb)
**	    documented.
**	12-Oct-2010 (kschendel) SIR 124544
**	    dmu_char_array replaced with DMU_CHARACTERISTICS.
**	    Fix the loop here.
**	27-Oct-2010 (kschendel)
**	    Fix above to positively init all the return elements.
*/

void
GM_info_reg( GWX_RCB *gwx_rcb, i4  *journaled, i4  *structure,
	    i4  *unique, i4  *update )
{
    DMU_CHARACTERISTICS *dmuchars;
    i4 indicator;

    /* Init the return values.  We should always see structure, but be
    ** safe.
    */
    *journaled = *unique = *update = 0;
    *structure = DB_BTRE_STORE;

    /* Now inspect the dmu-characteristics and set what was sent */
    dmuchars = gwx_rcb->xrcb_dmu_chars;
    if (dmuchars == NULL)
	return;

    indicator = -1;
    while ((indicator = BTnext(indicator, dmuchars->dmu_indicators, DMU_CHARIND_LAST)) != -1)
    {
	switch( indicator )
	{
	case DMU_JOURNALED:
	    *journaled = (dmuchars->dmu_journaled != DMU_JOURNAL_OFF);
	    break;

	case DMU_STRUCTURE:

	    /* DB_HEAP_STORE, DB_ISAM_STORE, DB_HASH_STORE, DB_BTRE_STORE */

	    *structure = dmuchars->dmu_struct;
	    break;

	case DMU_GW_UPDT:

	    *update = (dmuchars->dmu_flags & DMU_FLAG_UPDATE) != 0;
	    break;

	case DMU_UNIQUE:
	    *unique = TRUE;

	default:
	    break;
	}
    }
}





/*
** Name: GM_get_pmsym() - get PM resources from the resource file
**
** Description:
**	gcn_get_pmsym() makes PM calls to get the resource name
**
**	gcn_get_pmsym() copies the symbol's value into the result buffer.  If
**	the symbol is not set, the default value is copied.
**
** Inputs:
**	sym	- symbol to look up
**	dflt	- default result
**	len	- max length of result
**
** Outputs:
**	result	- value of symbol placed here
**
** History:
**	04-Dec-92 (gautam)
**	    Written
**	5-Jan-1994 (daveb) 58358
**	    swiped from gcn_get_pmsym for GWM.
*/	

VOID
GM_get_pmsym( char *sym, char *dflt, char *result, i4  len )
{
	char		*buf;
	char		*val;

	if( PMget( sym,  &buf) != OK )
	{
	    STncpy( result, dflt, len );
	}
	else
	{
	    STncpy( result, buf, len );
	}
	result[ len ] = '\0';
}



/*{
** Name:	GM_dump_dmt_att -- print dmt att to log.
**
** Description:
**	Prefixes the message, then dumps to the log nicely.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	msg 	string to prefix the contents with.
**  	key 	key to show; may be invalid.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** Exceptions: (if any)
**	none.
**
** History:
**	21-Jan-1994 (daveb) 59125
**	    created.
*/

void 
GM_dump_dmt_att( char *msg, DMT_ATT_ENTRY *dmt_att )
{
    TRdisplay("    DMT_ATT_ENTRY %s %p:\n", msg, dmt_att );
    TRdisplay("    att_name '%~t'\n", dmt_att->att_nmlen, dmt_att->att_nmstr);
    TRdisplay("    att_number %d, att_offset %d, att_type %d, att_width %d\n",
	      dmt_att->att_number, 
	      dmt_att->att_offset, 
	      dmt_att->att_type, 
	      dmt_att->att_width  );
    TRdisplay("    att_prec %d, att_flags %d\n", 
	      dmt_att->att_prec, 
	      dmt_att->att_flags  );
    TRdisplay("    att_key_seq_number %d\n",
	      dmt_att->att_key_seq_number );
}


/*{
** Name:	GM_att_type_decode -- get string from gma_type field.
**
** Description:
**	returns a string decoding the gma_type field of a GM_ATT_ENTRY.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	msg 	string to prefix the contents with.
**  	key 	key to show; may be invalid.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** Exceptions: (if any)
**	none.
**
** History:
**	21-Jan-1994 (daveb) 59125
**	    created.
*/

static char *
GM_att_type_decode( i4  type )
{
    char *s;
    switch( type )
    {
    case GMA_UNKNOWN:	s = "unknown"; break;
    case GMA_CLASSID:	s = "classid"; break;
    case GMA_PLACE:	s = "place"; break;
    case GMA_VNODE:	s = "vnode"; break;
    case GMA_SERVER:	s = "server"; break;
    case GMA_INSTANCE:	s = "intstance"; break;
    case GMA_VALUE: 	s = "value"; break;
    case GMA_PERMS: 	s = "perms"; break;
    case GMA_USER:    	s = "user classid"; break;
    default:	    	s = "BAD gma_type"; break;
    }
    return( s );
}



/*{
** Name:	GM_dump_gwm_att -- print gwm att to log.
**
** Description:
**	Prefixes the message, then dumps to the log nicely.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	msg 	string to prefix the contents with.
**  	key 	gwm_att to show; may be invalid.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** Exceptions: (if any)
**	none.
**
** History:
**	21-Jan-1994 (daveb) 59125
**	    created.
*/

void 
GM_dump_gwm_att( char *msg, GM_ATT_ENTRY *gwm_att )
{
    TRdisplay("    GM_ATT_ENTRY %s %p:\n", msg, gwm_att );
    TRdisplay("    gma_col_classid '%50s'\n", gwm_att->gma_col_classid );
    TRdisplay("    gma_att_offset %d, gma_type '%s', gma_dmt_att %p\n",
	      gwm_att->gma_att_offset,
	      GM_att_type_decode( gwm_att->gma_type),
	      gwm_att->gma_dmt_att );
}


/*{
** Name:	GM_dump_gmw_katt -- print gwm key attr to log
**
** Description:
**	Prefixes the message, then dumps to the log nicely.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	msg 	string to prefix the contents with.
**  	gwm_att key attribute to show.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** Exceptions: (if any)
**	none.
**
** History:
**	21-Jan-1994 (daveb) 59125
**	    created.
*/

void 
GM_dump_gwm_katt( char *msg, GM_ATT_ENTRY *gwm_att )
{
    TRdisplay("    GM_ATT_ENTRY KEY s %p:\n", msg, gwm_att );
    TRdisplay("    gma_key_offset %d, gma_key_type '%s', gma_key_dmt_att %p\n",
	      gwm_att->gma_key_offset,
	      GM_att_type_decode( gwm_att->gma_key_type ),
	      gwm_att->gma_key_dmt_att );
}

/*{
** Name: GM_chk_priv - check the user name for ownership of privilege.
**
** Description:
**      Given the user name and the privilege name scan the PM entries for
**      a match.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**      user_name   user name
**      priv_name   privilege name
**
** Outputs:
**	none.
**
** Returns:
**      TRUE        user owns privilege
**      FALSE       user does not own privilege
**
** History:
**      11-Mar-98 (fanra01)
**          Created based on scs_chk_priv.
**	01-oct-1999 (somsa01)
**	    Modified so that it does not use PM calls to retrieve the
**	    config.dat privileges, but rather SCU_INFORMATION. This
**	    eradicates a memory leak due to continuous calls to PMget().
*/
bool
GM_chk_priv( char *user_name, char *priv_name )
{
    SCF_CB		scf_cb;
    SCF_SCI		sci_list;
    DB_STATUS		status;
    SCS_ICS_UFLAGS	result;

    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_len_union.scf_ilength = 1;
    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*) &sci_list;

    sci_list.sci_code = SCI_SPECIAL_UFLAGS;
    sci_list.sci_length = sizeof(SCS_ICS_UFLAGS);
    sci_list.sci_aresult = (PTR) &result;
    sci_list.sci_rlength = NULL;
    status = scf_call(SCU_INFORMATION, &scf_cb);

    if (status != OK)
    {
	GM_error (E_GW8307_DBNAME_ERR);
	GM_error (scf_cb.scf_error.err_code);
	return FALSE;
    }
    else
    {
	/* 
	** privileges entries are of the form
	**   ii.<host>.*.privilege.<user>:   SERVER_CONTROL,NET_ADMIN,...
	**
	** Currently known privs are:  SERVER_CONTROL,NET_ADMIN,
	**  	    MONITOR,TRUSTED
	*/
	if (!STcompare(priv_name, "SERVER_CONTROL"))
	    return (result.scs_usr_svr_control);
	else if (!STcompare(priv_name, "NET_ADMIN"))
	    return (result.scs_usr_net_admin);
	else if (!STcompare(priv_name, "MONITOR"))
	    return (result.scs_usr_monitor);
	else if (!STcompare(priv_name, "TRUSTED"))
	    return (result.scs_usr_trusted);
	else
	    return FALSE;
    }
}
