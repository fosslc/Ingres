/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <bt.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmxcb.h>
#include    <dmtcb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dm0m.h>
#include    <dm2d.h>
#include    <dmp.h>
#include    <st.h>

/**
** Name: DMCOPEN.C - Functions used to open a database for a session.
**
** Description:
**      This file contains the functions necessary to open a database
**      for a session.  This file defines:
**    
**      dmc_open_db() -  Routine to perform the open 
**                       database operation.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.      
**	18-nov-85 (derek)
**	    Completed code for dmc_open_db().
**	8-Mar-89 (ac)
**	    Added 2PC support.
**	2-may-89 (anton)
**	    local collation support
**	27-aug-89 (ralph
**	    Ignore DMC_CVCFG for config file conversion
**      29-jun-89 (jennifer)
**          Added MAC check for openning database for B1.
**	26-apr-1991 (bryanp)
**	    Correct typo:
**		odcb->odcb_access_mode == ODCB_A_READ (== should be =).
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements: initialize ODCB pending queue.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	30-October-1992 (rmuth)
**	    Pass the DMC_CVCFG flagto dm2d_open_db, as this tells us that
**	    the process connecting is upgardedb and hence we want to
**	    convert the table to new 6.5 format.
**	04-nov-92 (jrb)
**	    Initialize scb location usage bit masks for multi-location sorts.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	18-dec-1992 (robf)
**          Remove dm0a.h
**	15-mar-1993 (rogerk)
**	    Fix compile warnings.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system type definitions.
**	28-may-93 (robf)
**	    Secure 2.0: Reworked ORANGE code. Database label is no longer
**	    the iirelation tuple label, its handled as a discrete attribute
**	    in iidatabase (managed by SCF)
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	29-sep-93 (jrb)
**	    Add DMC_ADMIN_DB as a valid bit in dmc_flags_mask in dmc_open_db.
**	22-nov-93 (jrb)
**          Moved code from dm2d_open_db to here for checking whether we should
**          log a warning if the iidbdb isn't journaled when accessing it.
**	09-oct-93 (swm)
**	    Bug #56437
**	    Get scf_session_id from new dmc_session_id rather than dmc_id.
**	31-jan-1994 (bryanp) B58096, B58097
**	    Log scf_error.err_code if scf_call fails.
**	    Move allocation of LOC_MASKS up slightly so that failure to
**		allocate loc masks won't leave deallocated odcb on linked lists.
**	23-may-1994 (kbref) B59690
**	    When checking if the iidbdb is not journaled, check for both
**	    $ingres and $INGRES usernames.
**	20-mar-1995 (canor01) B67535
**	    When checking if the iidbdb is not journaled, check for 
**	    Security Audit Thread.
**	21-Jan-1997 (jenjo02)
**	    Remove DM0M_ZERO from dm0m_allocate() for ODCB.
**	    All fields are individually initialized.
** 	09-jun-1997 (wonst02)
** 	    Set SCB_NOLOGGING for readonly database.
**	05-may-1998 (hanch04)
**	    Added () around to get correct divsion.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Find DML_SCB with macro instead of calling SCF.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	18-Nov-2008 (jonj)
**	    dm2d_? functions converted to DB_ERROR *
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**       8-Jun-2008 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Set DM2D_MUST_LOG if caller has DMC2_MUST_LOG to allow
**          SET NOLOGGING to be blocked if required.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*{
** Name: dmc_open_db - Opens a database for a session.
**
**  INTERNAL DMF call format:    status = dmc_open_db(&dmc_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMC_OPEN_DB,&dmc_cb);
**
** Description:
**    The dmc_open_db function handles the opening of a database for a user
**    session.  This causes DMF to determine if the database specified is
**    still known to the server and creates the control information needed
**    to access the database.  If the database is no longer known to the server 
**    or conflicting access is requested, the caller will be returned the
**    apporpriate error.
**
** Inputs:
**      dmc_cb 
**          .type                           Must be set to DMC_CONTROL_CB.
**          .length                         Must be at least sizeof(DMC_CB).
**          .dmc_session_id		    Must be a valid SCF session 
**                                          identifier obtained from SCF.
**          .dmc_op_type                    Must be DMC_DATABASE_OP.
**          .dmc_flag_mask                  Must be zero, DMC_NOWAIT,
**					    DMC_PRETEND, DMC_MAKE_CONSISTENT,
**					    DMC_JOURNAL, DMC_NOJOURNAL, or
**					    DMC_NLK_SESS.
**          .dmc_db_id                      Must be a valid database 
**                                          identifier returned from an
**                                          add database operation.
**          .dmc_db_access_mode             Must be zero or one of the 
**                                          following:
**                                          DMC_A_READ
**                                          DMC_A_WRITE
**					    DMC_A_CREATE
**	    .dmc_lock_mode		    Must be one of the following:
**					    DMC_L_SHARED
**					    DMC_L_EXCLUSIVE
** Output:
**      dmc_cb 
**          .dmc_db_id                      Internal open database identifier.
**	    .dmc_collation		    Collation name
**	    .dmc_coldesc		    Collation descriptor
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM000F_BAD_DB_ACCESS_MODE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002D_BAD_SERVER_ID
**                                          E_DM002F_BAD_SESSION_ID
**                                          E_DM003E_DB_ACCESS_CONFLICT
**                                          E_DM0040_DB_OPEN_QUOTA_EXCEEDED
**                                          E_DM0041_DB_QUOTA_EXCEEDED
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**					    E_DM004C_LOCK_RESOURCE_BUSY
**                                          E_DM0053_NONEXISTENT_DB
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**					    E_DM0086_ERROR_OPENING_DB
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**                     
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmc_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmc_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmc_cb.error.err_code.
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	18-nov-1985 (derek)
**	    Completed code.
**	8-Mar-89 (ac)
**	    Added 2PC support.
**	2-may-89 (anton)
**	    Local collation support
**	27-aug-89 (ralph
**	    Ignore DMC_CVCFG for config file conversion
**      29-jun-89 (jennifer)
**          Added MAC check for openning database for B1.
**	26-apr-1991 (bryanp)
**	    Correct typo:
**		odcb->odcb_access_mode == ODCB_A_READ (== should be =).
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements: initialize ODCB pending queue.
**	30-October-1992 (rmuth)
**	    Pass the DMC_CVCFG flagto dm2d_open_db, as this tells us that
**	    the process connecting is upgardedb and hence we want to
**	    convert the table to new 6.5 format.
**	04-nov-92 (jrb)
**	    Allocate DML_LOC_MASKS block, fill in, and attach to scb.  For
**	    multi-location sorts.
**	28-may-93 (robf)
**	    Secure 2.0: Reworked ORANGE code. Database label is no longer
**	    the iirelation tuple label, its handled as a discrete attribute
**	    in iidatabase (managed by SCF)
**	29-sep-93 (jrb)
**	    Add DMC_ADMIN_DB as a valid bit in dmc_flags_mask.
**	22-nov-93 (jrb)
**          Moved code from dm2d_open_db to here for checking whether we should
**          log a warning if the iidbdb isn't journaled when accessing it.
**	09-oct-93 (swm)
**	    Bug #56437
**	    Get scf_session_id from new dmc_session_id rather than dmc_id.
**	31-jan-1994 (bryanp) B58096, B58097
**	    Log scf_error.err_code if scf_call fails.
**	    Move allocation of LOC_MASKS up slightly so that failure to
**		allocate loc masks won't leave deallocated odcb on linked lists.
**	23-may-1994 (kbref) B59690
**	    When checking if the iidbdb is not journaled, check for both
**	    $ingres and $INGRES usernames.
**	21-Jan-1997 (jenjo02)
**	    Remove DM0M_ZERO from dm0m_allocate() for ODCB.
**	    All fields are individually initialized.
** 	09-jun-1997 (wonst02)
** 	    Set SCB_NOLOGGING for readonly database.
**	26-mar-2001 (stephenb)
**	    Set dmc_ucoldesc.
**	11-Jun-2004 (schka24)
**	    Init the odcb cq mutex.
**	6-Jul-2006 (kschendel)
**	    Return the infodb database ID number (session independent)
**	    for everyone else to use.
**	13-May-2008 (kibro01) b120369
**	    Add DM2D_ADMIN_CMD to register this as an open_db which doesn't
**	    need to check replication.
**      30-May-2008 (huazh01)
**          after successfully openning the db, update the collation 
**          sequence ptr for the adf_cb saved under 'scb'. 
**          This fixes b120441. 
**	10-Nov-2009 (kschendel)
**	    Jenjo's error update accidently unconditionally set DM0086;
**	    it should only be set for "internal" errors.  The effect was that
**	    the first time an error db was opened and marked inconsistent,
**	    the user would see "error initializing DMF".
**      09-aug-2010 (maspa05) b123189, b123960
**          Pass flag for readonlydb through to dm2d_open_db
*/

DB_STATUS
dmc_open_db(
DMC_CB    *dmc_cb)
{
    char		sem_name[8+16+10+1];	/* +10 is slop */
    DMC_CB		*dmc = dmc_cb;
    DML_SCB		*scb;
    DML_ODCB		*odcb = 0;
    DMP_DCB		*dcb;
    i4		error, local_error;
    DB_STATUS		status, local_status;
    i4		flag_mask = 0;
    DMP_EXT		*ext;
    DML_LOC_MASKS	*lm = 0;
    i4		size;
    i4		i;
    i4			db_opened = 0;
    DB_OWN_NAME		own;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmc->error);

    for (;;)
    {
	if (dmc->dmc_op_type != DMC_DATABASE_OP)
	{
	    SETDBERR(&dmc->error, 0, E_DM000C_BAD_CB_TYPE);
	    status = E_DB_ERROR;
	    break;
	}

	if ((dmc->dmc_flags_mask & ~(DMC_NOWAIT | DMC_JOURNAL | DMC_NOJOURNAL |
	     DMC_PRETEND | DMC_MAKE_CONSISTENT | DMC_NLK_SESS | DMC_CVCFG |
	     DMC_ADMIN_DB)) ||
	    (dmc->dmc_flags_mask & (DMC_JOURNAL|DMC_NOJOURNAL)) == 
	    (DMC_JOURNAL | DMC_NOJOURNAL))
	{
	    SETDBERR(&dmc->error, 0, E_DM001A_BAD_FLAG);
	    status = E_DB_ERROR;
	    break;
	}

	flag_mask |= (dmc->dmc_flags_mask & DMC_NOWAIT)  ? DM2D_NOWAIT : 0;
	flag_mask |= (dmc->dmc_flags_mask & DMC_MAKE_CONSISTENT) ? 
	    DM2D_FORCE : 0;
	flag_mask |= (dmc->dmc_flags_mask & DMC_PRETEND) ? DM2D_NOCK   : 0;
	flag_mask |= (dmc->dmc_flags_mask & DMC_NLK_SESS) 
			    ? (DM2D_NLK_SESS ) : 0;
	flag_mask |= (dmc->dmc_flags_mask & DMC_CVCFG)  ? DM2D_CVCFG : 0;

	if (dmc->dmc_flags_mask2 & DMC2_ADMIN_CMD)
		flag_mask |= DM2D_ADMIN_CMD;

        if (dmc->dmc_flags_mask2 & DMC2_MUST_LOG)
                flag_mask |= DM2D_MUST_LOG;

	if ((dmc->dmc_db_access_mode != DMC_A_READ &&
	    dmc->dmc_db_access_mode != DMC_A_WRITE &&
	    dmc->dmc_db_access_mode != DMC_A_CREATE) ||
	    (dmc->dmc_lock_mode != DMC_L_EXCLUSIVE &&
	    dmc->dmc_lock_mode != DMC_L_SHARE))
	{
	    SETDBERR(&dmc->error, 0, E_DM002A_BAD_PARAMETER);
	    status = E_DB_ERROR;
	    break;
	}

	if ( (scb = GET_DML_SCB(dmc->dmc_session_id)) == 0 ||
	     (dm0m_check((DM_OBJECT *)scb, SCB_CB) != 0) )
	{
	    SETDBERR(&dmc->error, 0, E_DM002F_BAD_SESSION_ID);
	    status = E_DB_ERROR;
	    break;
	}

	if (dm0m_check((DM_OBJECT *)dmc->dmc_db_id, DCB_CB) != 0)
	{
	    SETDBERR(&dmc->error, 0, E_DM0010_BAD_DB_ID);
	    status = E_DB_ERROR;
	    break;
	}

	/* Allocate an ODCB. */

	status = dm0m_allocate((i4)sizeof(DML_ODCB), 0, 
            (i4)ODCB_CB,
	    (i4)ODCB_ASCII_ID, (char *)scb, (DM_OBJECT **)&odcb, &dmc->error);
	if (status != E_DB_OK)
	{
	    uleFormat( &dmc->error, 0, NULL, ULE_LOG , NULL, 
	    		(char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
	    SETDBERR(&dmc->error, 0, E_DM0086_ERROR_OPENING_DB);
	    break;
	}

	odcb->odcb_db_id = dmc->dmc_db_id;
	odcb->odcb_dcb_ptr = (DMP_DCB *)odcb->odcb_db_id;
	odcb->odcb_access_mode = ODCB_A_READ;
	odcb->odcb_lk_mode = ODCB_IX;
	odcb->odcb_scb_ptr = scb;
	odcb->odcb_cq_next = odcb->odcb_cq_prev =
	    (DML_XCCB *) &odcb->odcb_cq_next;
	dm0s_minit(&odcb->odcb_cq_mutex,
		STprintf(sem_name,"ODCB cq %lx",odcb));
	if (dmc->dmc_db_access_mode == DMC_A_WRITE ||
	    dmc->dmc_db_access_mode == DMC_A_CREATE)
	{
	    odcb->odcb_access_mode = ODCB_A_WRITE;
	}
	if (dmc->dmc_lock_mode == DMC_L_EXCLUSIVE)
	    odcb->odcb_lk_mode = ODCB_X;

	if (dmc->dmc_flags_mask2 &  DMC2_READONLYDB)
	    flag_mask |= DM2D_READONLYDB;

	status = dm2d_open_db(odcb->odcb_dcb_ptr, odcb->odcb_access_mode,
	    odcb->odcb_lk_mode, scb->scb_lock_list, flag_mask, &dmc->error);
	if (status != E_DB_OK)
	{
	    if (dmc->error.err_code > E_DM_INTERNAL)
	    {
		uleFormat( &dmc->error, 0, NULL, ULE_LOG , NULL, 
			    (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		SETDBERR(&dmc->error, 0, E_DM0086_ERROR_OPENING_DB);
	    }
	    break;
	}
	dcb = odcb->odcb_dcb_ptr;
	db_opened = 1;
	if (dmc->dmc_db_access_mode == DMC_A_READ)
	{
	    scb->scb_sess_mask |= SCB_NOLOGGING;
	    scb->scb_sess_am = dmc->dmc_db_access_mode;
	}

	/* allocate location usage bit masks block; note that we are allocating
	** all masks in contiguous memory, and this is depended on in dmc_alter
	**
	** The allocation of the location masks cannot occur until after the
	** database has been opened, since dcb_ext->ext_count is an output of
	** dm2d_open_db.
	*/
	ext = odcb->odcb_dcb_ptr->dcb_ext;
	size = sizeof(i4) *
		((ext->ext_count+BITS_IN(i4)-1)/BITS_IN(i4));
	status = dm0m_allocate((i4)sizeof(DML_LOC_MASKS) + 3*size,
	    DM0M_ZERO, (i4)LOCM_CB, (i4)LOCM_ASCII_ID, (char *)scb,
	    (DM_OBJECT **)&lm, &dmc->error);
	if (status != E_DB_OK)
	{
	    uleFormat( &dmc->error, 0, NULL, ULE_LOG , NULL, 
			(char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
	    SETDBERR(&dmc->error, 0, E_DM0086_ERROR_OPENING_DB);
	    break;
	}

	/*
	** NOTE: Beyond this point, any errors which are encountered cannot
	** simply "break". Once we start putting the ODCB onto the SCB queue,
	** it becomes unsafe simply to "break" because we'd deallocate the
	** odcb while still leaving it on the scb queue, causing control block
	** corruptions.
	*/
	odcb->odcb_q_next = scb->scb_oq_next;
	odcb->odcb_q_prev = (DML_ODCB*) &scb->scb_oq_next;
	dmc->dmc_coldesc = odcb->odcb_dcb_ptr->dcb_collation;
	dmc->dmc_ucoldesc = odcb->odcb_dcb_ptr->dcb_ucollation;
        scb->scb_adf_cb->adf_collation = odcb->odcb_dcb_ptr->dcb_collation;
        scb->scb_adf_cb->adf_ucollation = odcb->odcb_dcb_ptr->dcb_ucollation;
	MEcopy(odcb->odcb_dcb_ptr->dcb_colname, sizeof dmc->dmc_collation,
			dmc->dmc_collation);
	MEcopy(odcb->odcb_dcb_ptr->dcb_ucolname, sizeof dmc->dmc_ucollation,
			dmc->dmc_ucollation);
	scb->scb_oq_next->odcb_q_prev = odcb;
	scb->scb_oq_next = odcb;
        scb->scb_o_ref_count++;

	scb->scb_loc_masks = lm;
	lm->locm_bits = size * BITSPERBYTE;
	lm->locm_w_allow = (i4 *)((char *)lm + sizeof(DML_LOC_MASKS));
	lm->locm_w_use   = 
		(i4 *)((char *)lm + sizeof(DML_LOC_MASKS) + size);
	lm->locm_d_allow = 
		(i4 *)((char *)lm + sizeof(DML_LOC_MASKS) + size + size);

	/* initialize location usage bit masks in scb */
	for (i=0; i < ext->ext_count; i++)
	{
	    if (ext->ext_entry[i].flags & DCB_WORK)
	    {
		BTset(i, (char *)lm->locm_w_allow);
		BTset(i, (char *)lm->locm_w_use);
	    }

	    if (ext->ext_entry[i].flags & DCB_AWORK)
		BTset(i, (char *)lm->locm_w_allow);
		
	    if (ext->ext_entry[i].flags & DCB_DATA)
		BTset(i, (char *)lm->locm_d_allow);
	}

        /*  
	**  If the following all hold:
	**
	**  	1) This is the iidbdb being opened,
	**  	2) The iidbdb is the target db (i.e. it's not being opened
	**  	   just for administrative purposes),
	**	3) Journaling is not enabled on the iidbdb,
	**  	4) The user is not $ingres nor $INGRES,
	**	5) The user is not the Security Audit Thread
	**
	**  then issue a warning that the iidbdb is not journaled.
	**
	**  Notes: 
	**
	**	1) The coding order hopes for short-circuiting on the fast
	**         operations.
	**	2) We must check for BOTH $ingres and $INGRES usernames.
	**	   For example, when the iidbdb is being created for a FIPS
	**	   installation, it is opened here with the username $INGRES,
	**	   not $ingres. Note that since a case-insensitive comparison
	**	   is used, mixed-case usernames such as $iNGres will be ok.
	*/

	STmove((PTR)DB_AUDIT_THREAD, ' ', sizeof(own), (PTR)&own);
	if (!(dcb->dcb_status & DCB_S_JOURNAL) &&
	    !(dmc->dmc_flags_mask & DMC_ADMIN_DB) &&
	    (MEcmp((PTR)dcb->dcb_name.db_db_name, 
			(PTR)DB_DBDB_NAME, DB_DB_MAXNAME) == 0) &&
	    (STbcompare(
		(char *)scb->scb_user.db_own_name, sizeof(DB_OWN_NAME),
		(char *)DB_INGRES_NAME, sizeof(DB_OWN_NAME), TRUE) 
		!= 0) &&
	    (MEcmp((PTR)scb->scb_user.db_own_name, (PTR)&own,
                sizeof(DB_OWN_NAME))
		!= 0) )
        {
            uleFormat(NULL, W_DM5422_IIDBDB_NOT_JOURNALED, NULL, ULE_LOG,
                NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
        }

	dmc->dmc_db_id = (char *)odcb;		/* dmf db id ptr cookie */
	dmc->dmc_udbid = dcb->dcb_id;		/* infodb db id for RDF etc */
	return (E_DB_OK);
    }

    if (db_opened)
    {
	local_status = dm2d_close_db(odcb->odcb_dcb_ptr, scb->scb_lock_list, 
						    (i4)0, &local_dberr);
	if (local_status != E_DB_OK)
	    uleFormat( &local_dberr, 0, NULL, ULE_LOG , NULL, 
			(char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
    }

    if (odcb)
    {
	dm0s_mrelease(&odcb->odcb_cq_mutex);
	dm0m_deallocate((DM_OBJECT **)&odcb);
    }
    if (lm)
	dm0m_deallocate((DM_OBJECT **)&lm);
    return (E_DB_ERROR);
}
