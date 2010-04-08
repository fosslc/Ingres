/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <qu.h>
#include    <st.h>
#include    <sl.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefmain.h>
#include    <qefcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <sxf.h>
#include    <psyaudit.h>

/*
** Name: PSLLKMD.C:	This file contains functions used by both QUEL and SQL
**			grammars in parsing of SET LOCKMODE statement.
**
** Function Name Format:  (this applies to externally visible functions only)
** 
**	PSL_LM<number>{[S|Q]}_<production name>
**	
**	where <number> is a unique (within this file) number between 1 and 99
**	
**	if the function will be called only by the SQL grammar, <number> must be
**	followed by 'S';
**	
**	if the function will be called only by the QUEL grammar, <number> must
**	be followed by 'Q';
**
** Description:
**	This file contains functions used by both QUEL and SQL grammars in
**	parsing of SET LOCKMODE statement.
**
**
**	psl_lm1_setlockstmnt()	    - semantic action for SETLOCKSTMNT
**				      production
**	psl_lm2_setlockkey()	    - semantic action for SETLOCKKEY production
**	psl_lm3_setlockparm_name()  - semantic action for SETLOCKPARM production
**				      when the characteristic was specified as a
**				      string (NAME)
**	psl_lm4_setlockparm_num()   - semantic action for SETLOCKPARM production
**				      when the characteristic was specified as a
**				      number
**	psl_lm5_setlockmode_distr() - semantic action for SETLOCKMODE
**				      production (distributed thread actions)
**	psl_lm6_setlockscope_distr()- semantic action for SETLOCKSCOPE
**				      production (distributed thread actions)
**
** History:
**	7-mar-91 (andre)
**	    created
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	21-apr-92 (barbara)
**	    Updated for Sybil.  Added actions for distributed thread.
**	29-jun-93 (andre)
**	    #included CV.H
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	13-aug-93 (andre)
**	    corrected causes of compiler warnings
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	06-oct-93 (barbara)
**	    Added Star support for set lockmode session options in addition
**	    to timeout (which was already supported).  Star processing 
**	    consists of passing the set statement to QEF which in turn passes
**	    it on to all connected and yet-to-be connected sessions.  Fixes
**	    bug 53492.
**	25-oct-93 (vijay)
**	    cast rhs of dmc_id assigment to PTR.
**	9-nov-93 (robf)
**          Security audit setting lockmode (resource auditing)
**      04-apr-1995 (dilma04) 
**          Add support for READLOCK=READ_COMMITTED/REPEATABLE_READ.
**	22-nov-1996 (dilma04)
**	    Row-level locking project:
**          Add support for LEVEL=ROW.
**      24-jan-97 (dilma04)
**          Fix bug 80115: report error in psl_lm3_setlockparm_name(),
**          if setting row locking for system catalog or 2k page table.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Change readlock values according to new definitions.
**      14-oct-97 (stial01)
**          psl_lm2_setlockkey() Isolation levels are not valid readlock values.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SESBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	17-Jan-2001 (jenjo02)
**	    Added *PSS_SESBLK parm to psq_prt_tabname().
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**	20-Aug-2002 (jenjo02)
**	    SET LOCKMODE ... TIMEOUT = NOWAIT support added.
**      15-Apr-2003 (bonro01)
**          Added include <psyaudit.h> for prototype of psy_secaudit()
*/

/* external declarations */

/* Useful defines */
					/*
					** maximum number of SET LOCKMODE
					** characteristics
					*/
#define	    MAX_LOCKMODE_CHARS	    4
					/*
					** The following constants are used to
					** remember the type of characteristic
					** being set
					*/
#define                 LOCKLEVEL       1
#define                 READLOCK        2
#define                 MAXLOCKS        3
#define                 TIMEOUT		4
/*
** Name psl_lm1_setlockstmnt() - perform semantic action for SETLOCKSTMNT
**			         production
**
** Description:
**	perform semantic action for SETLOCKSTMNT production in QUEL and SQL
**	grammars
**
** Input:
**	sess_cb		    PSF session CB
**	    pss_distrib	    DB_3_DDB_SESS if distributed thread
**	    pss_ostream	    stream to be opened for memory allocation
**	    pss_stmt_flags  PSS_SET_LOCKMODE_SESS if SET LOCKMODE SESSION
**	psq_cb		    PSF request CB
**
** Output:
**	sess_cb
**	    pss_ostream	    stream has been opened for memory allocation
**	    pss_object	    point to the root of a new QSF object
**			    (of type (DMC_CB *) or (QEF_RCB *)).
**	psq_cb
**	    psq_mode	    set to PSQ_SLOCKMODE
**	    psq_error	    filled in if an error occurred
**
** Returns:
**	E_DB_{OK, ERROR, SEVERE}
**
** Side effects:
**	Opens a memory stream and allocates memory
**
**  History:
**	07-mar-91 (andre)
**	    plagiarized from SETLOCKSTMNT production
**	17-apr-92 (barbara)
**	    Updated for Sybil.  Distributed thread allocates QEF_RCB and
**	    calls QEF directly.
**	07-oct-93 (swm)
**	    Bug #56437
**	    added PTR cast in assignment to dmc_cb->dmc_id.
**	09-oct-93 (swm)
**	    Bug #56437
**	    Put pss_sessid into new dmc_session_id rather than dmc_id.
**	26-Feb-2001 (jenjo02)
**	    Set session_id in QEF_RCB;
*/
DB_STATUS
psl_lm1_setlockstmnt(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb)
{
    DB_STATUS		status;
    i4		err_code;
    DMC_CB		*dmc_cb;
    DB_ERROR		err_blk;
    DB_STATUS	        tempstat;

    psq_cb->psq_mode = PSQ_SLOCKMODE;

    /* Verify the user has LOCKMODE permission */

    status = psy_ckdbpr(psq_cb, (u_i4) DBPR_LOCKMODE);
    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_error(6247L, 0L, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 0);
	/*
	** Audit failed  set lockmode
	*/
	if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
	{
	    (VOID)psy_secaudit(
		    FALSE,
		    sess_cb,
		    "LOCKMODE",
		    (DB_OWN_NAME *)0,
		    8,
		    SXF_E_RESOURCE,
		    I_SX2741_SET_LOCKMODE,
		    SXF_A_FAIL|SXF_A_LIMIT,
		    &err_blk);
	}
	return(status);
    }

    /* Create control block for DMC_ALTER or QEF_CALL for set lockmode */
    status = psf_mopen(sess_cb, QSO_QP_OBJ, &sess_cb->pss_ostream, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	/* Distributed thread calls QEF directly */
	QEF_RCB	*qef_rcb;

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QEF_RCB),
	    &sess_cb->pss_object, &psq_cb->psq_error);
	if (status != E_DB_OK)
	    return (status);

	status = psf_mroot(sess_cb, &sess_cb->pss_ostream, sess_cb->pss_object,
			   &psq_cb->psq_error);
	if (status != E_DB_OK)
	    return (status);

	/* Fill in the QEF control block */
	qef_rcb = (QEF_RCB *) sess_cb->pss_object;
	qef_rcb->qef_length = sizeof(QEF_RCB);
	qef_rcb->qef_type = QEFRCB_CB;
	qef_rcb->qef_owner = (PTR)DB_PSF_ID;
	qef_rcb->qef_ascii_id = QEFRCB_ASCII_ID;
	qef_rcb->qef_modifier = QEF_MSTRAN;
	qef_rcb->qef_flag = 0;
	qef_rcb->qef_cb = (QEF_CB *) NULL;
	qef_rcb->qef_sess_id = sess_cb->pss_sessid;
	return (E_DB_OK);
    }

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMC_CB), (PTR *) &dmc_cb,
	&psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    status = psf_mroot(sess_cb, &sess_cb->pss_ostream, (PTR) dmc_cb, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    sess_cb->pss_object		= (PTR) dmc_cb;
    dmc_cb->type		= DMC_CONTROL_CB;
    dmc_cb->length		= sizeof (DMC_CB);
    dmc_cb->dmc_op_type		= DMC_SESSION_OP;
    dmc_cb->dmc_session_id	= (PTR)sess_cb->pss_sessid;
    dmc_cb->dmc_flags_mask	= DMC_SETLOCKMODE;
    dmc_cb->dmc_db_id		= sess_cb->pss_dbid;
    dmc_cb->dmc_db_access_mode  =
    dmc_cb->dmc_lock_mode	= 0;

    /* need to allocate characteristics array with MAX_LOCKMODE_CHARS entries */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	sizeof(DMC_CHAR_ENTRY) * MAX_LOCKMODE_CHARS,
	(PTR *) &dmc_cb->dmc_char_array.data_address, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    dmc_cb->dmc_char_array.data_in_size = 0;

    /*
    ** Audit allowed to issue lockmode
    */
    if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
    {
	(VOID)psy_secaudit(
	    FALSE,
	    sess_cb,
	    "LOCKMODE",
	    (DB_OWN_NAME *)0,
	    8,
	    SXF_E_RESOURCE,
	    I_SX2741_SET_LOCKMODE,
	    SXF_A_SUCCESS|SXF_A_LIMIT,
	    &err_blk);
    }

    return(E_DB_OK);
}

/*
** Name psl_lm2_setlockkey() - perform semantic action for SETLOCKKEY production
**
** Description:
**	perform semantic action for SETLOCKKEY production in QUEL and SQL
**	grammars
**
** Input:
**	sess_cb		    PSF session CB
**	    pss_distrib	    DB_3_DDB_SESS if distributed thread
**	    pss_stmt_flags  PSS_SET_LOCKMODE_SESS if SET LOCKMODE SESSION
**			    (distributed thread only)
**	char_name	    name of a characteristic
**
** Output:
**	char_type	    a number corresponding to this characteristic type
**	err_blk		    filled in if an error occurred
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	none
**
**  History:
**	07-mar-91 (andre)
**	    plagiarized from SETLOCKKEY production
**	20-apr-92 (barbara)
**	    Updated for Sybil.  Added session cb to interface.  For the
**	    distributed thread, TIMEOUT is the only option supported on a
**	    SESSION basis.
**	06-oct-93 (barbara)
**	    Fixed bug 53492.  Star now supports all set lockmode session
**	    statements.
**      04-apr-1995 (dilma04) 
**          Add support for READLOCK=READ_COMMITTED/REPEATABLE_READ.
**      14-oct-97 (stial01)
**          psl_lm2_setlockkey() Isolation levels are not valid readlock values.
**	    
*/
DB_STATUS
psl_lm2_setlockkey(
	PSS_SESBLK  *sess_cb,
	char	    *char_name,
	i4	    *char_type,
	DB_ERROR    *err_blk)
{
    i4                err_code;

    /* Decode "set lockmode" parameter.  Error if unknown. */
    if (!STcompare(char_name, "level"))
    {
	*char_type = LOCKLEVEL;
    }
    else if (!STcompare(char_name, "readlock"))
    {
	*char_type = READLOCK;
    }
    else if (!STcompare(char_name, "maxlocks"))
    {
	*char_type = MAXLOCKS;
    }
    else if (!STcompare(char_name, "timeout"))
    {
    	*char_type = TIMEOUT;
    }
    else
    {
	(VOID) psf_error(5928L, 0L, PSF_USERERR, &err_code,
	    err_blk, 1, (i4) STlength(char_name), char_name);
	return (E_DB_ERROR);    /* non-zero return means error */
    }

    return(E_DB_OK);
}

/*
** Name: psl_lm3_setlockparm_name() - perform semantic action for SETLOCKPARM
**				      production when the characteristic value
**				      has been specified as a string
**
** Input: 
**	sess_cb		    PSF session CB
**	    pss_distrib	    DB_3_DDB_SESS if distributed thread
**	    pss_stmt_flags  PSS_SET_LOCKMODE_SESS if SET LOCKMODE SESSION
**			    (distributed thread only)
**	    pss_object	    pointer to the root of a QSF object
**			    (of type (DMC_CB *) or (QEF_RCB *)).
**	char_type	    characteristic type
**	char_val	    value as specified by the user
**
** Output:
**	err_blk		    filled in if an error occurred
**	sess_cb		    PSF session CB
**	    pss_object	    characteristics of SETLOCKPARM filled in.
**
** Returns:
**	E_DB_{OK, ERROR, SEVERE}
**
** Side effects:
**	None
**
**  History:
**	07-mar-91 (andre)
**	    plagiarized from SETLOCKPARM production
**	21-apr-92 (barbara)
**	    Added support for Sybil.  For distributed thread, on
**	    SET LOCKMODE SESSION .. TIMEOUT we set timeout value
**	    specifically for QEF; all other options are passed to QEF
**	    in textual representation (see psl_lm5_setlockmode_distr) and
**	    are ignored in this function.
**      22-nov-1996 (dilma04)
**          Row-level locking project:
**          Add support for LEVEL=ROW.
**      24-jan-97 (dilma04)
**          Fix bug 80115: report an error if trying to specify row locking
**          for a system catalog or a table with 2k page size. 
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Change readlock values according to new definitions.
**	25-feb-99 (hayke02)
**	    When setting row level locking in STAR, we now do not check
**	    if the lockmode scope is table. This was causing a SIGSEGV
**	    because dmc_cb had not been set. This change fixes bug 95490.
**	31-May-2006 (kschendel)
**	    Allow level=row for catalogs, no reason to exclude it.
**	15-Jan-2010 (stial01)
**	    SIR 121619 MVCC: Add lock level MVCC
*/
DB_STATUS
psl_lm3_setlockparm_name(
	i4	    char_type,
	char	    *char_val,
	PSS_SESBLK  *sess_cb,
	DB_ERROR    *err_blk)
{
    i4		err_code, err_no = 0L;
    DMC_CHAR_ENTRY	*chr;
    DM_DATA	    	*char_array;
    DMC_CB		*dmc_cb; 
    bool		not_distr = ~sess_cb->pss_distrib & DB_3_DDB_SESS;
    i4		*val, dummy;

    if (not_distr)
    {
    	char_array = &((DMC_CB *)sess_cb->pss_object)->dmc_char_array;
        dmc_cb = (DMC_CB *) sess_cb->pss_object;

    	if (char_array->data_in_size / sizeof (DMC_CHAR_ENTRY) ==
	    MAX_LOCKMODE_CHARS)
    	{
	    (VOID) psf_error(5931L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);	/* non-zero return means error */
    	}   

    	chr = (DMC_CHAR_ENTRY *)
	    ((char *) char_array->data_address + char_array->data_in_size);
	val = &chr->char_value;
    }
    else
    {
	val = &dummy;	
    }

    switch (char_type)
    {
	case LOCKLEVEL:
	    /*
	    ** Acceptable values for locklevel are "row", "page", "table",
	    ** "session", "system", "mvcc".
	    */
	    if (not_distr)
		chr->char_id = DMC_C_LLEVEL;

	    if (!STcompare(char_val, "page"))
	    {
		*val = DMC_C_PAGE;
	    }
	    else if (!STcompare(char_val, "session"))
	    {
		*val = DMC_C_SESSION;
	    }
	    else if (!STcompare(char_val, "system"))
	    {
		*val = DMC_C_SYSTEM;
	    }
	    else if (!STcompare(char_val, "table"))
	    {
		*val = DMC_C_TABLE;
	    }
            else if (!STcompare(char_val, "row")
		    || !STcompare(char_val, "mvcc"))
            {
                if (not_distr && (dmc_cb->dmc_sl_scope == DMC_SL_TABLE))
                {
                    DB_TAB_ID       *tabid = &dmc_cb->dmc_table_id;
                    PSS_RNGTAB      *tbl = &sess_cb->pss_auxrng.pss_rsrng;
 
		    if (tbl->pss_tabdesc->tbl_pg_type == DB_PG_V1)
                    {
                        (VOID) psf_error(E_PS03B0_ILLEGAL_ROW_LOCKING, 0L,
                            PSF_USERERR, &err_code, err_blk, 2,
			    STlength(char_val), char_val,
                            psf_trmwhite(sizeof(tbl->pss_tabname),
                                     (char *) &tbl->pss_tabname),
                            &tbl->pss_tabname);
                        return (E_DB_ERROR);
                    }
                }
  
		if (*char_val == 'r')
		    *val = DMC_C_ROW;
		else
		    *val = DMC_C_MVCC;
            }
	    else
	    {
		err_no = 5924L;
	    }
	    break;

	case READLOCK:
	    /*
	    ** Acceptable values for readlock are "nolock", "shared",
            ** "exclusive", "session", "system".
	    */
	    if (not_distr)
		chr->char_id = DMC_C_LREADLOCK;

	    if (!STcompare(char_val, "nolock"))
	    {
		*val = DMC_C_READ_NOLOCK; 
	    }
	    else if (!STcompare(char_val, "shared"))
	    {
                *val = DMC_C_READ_SHARE;
	    }
	    else if (!STcompare(char_val, "exclusive"))
	    {
		*val = DMC_C_READ_EXCLUSIVE; 
	    }
	    else if (!STcompare(char_val, "session"))
	    {
		*val = DMC_C_SESSION;
	    }
	    else if (!STcompare(char_val, "system"))
	    {
		*val = DMC_C_SYSTEM;
	    }
	    else
	    {
		err_no = 5925L;
	    }
	    break;

	case MAXLOCKS:
	    /*
	    ** Acceptable values for maxlocks are "session", "system".
	    */
	    if (not_distr)
		chr->char_id = DMC_C_LMAXLOCKS;

	    if (!STcompare(char_val, "session"))
	    {
		*val = DMC_C_SESSION;
	    }
	    else if (!STcompare(char_val, "system"))
	    {
		*val = DMC_C_SYSTEM;
	    }
	    else
	    {
		err_no = 5926L;
	    }
	    break;

	case TIMEOUT:
	    /*
	    ** Acceptable values for timeout are "session" , "system",
	    ** and "nowait".
	    */
	    if (not_distr)
	    {
		chr->char_id = DMC_C_LTIMEOUT;
	    }
	    else if (sess_cb->pss_stmt_flags & PSS_SET_LOCKMODE_SESS)
	    {
		/*
		** The distributed server is interested in the value from 
		** the SETLOCKKEY production when the SETLOCKSCOPE value
		** is session.
		*/
		val =
	&((QEF_RCB *)sess_cb->pss_object)->qef_r3_ddb_req.qer_d14_ses_timeout;
	    }

	    if (!STcompare(char_val, "session"))
	    {
		*val = DMC_C_SESSION;
	    }
	    else if (!STcompare(char_val, "system"))
	    {
		*val = DMC_C_SYSTEM;
	    }
	    else if (!STcompare(char_val, "nowait"))
	    {
		*val = DMC_C_NOWAIT;
	    }
	    else
	    {
		err_no = 5927L;
	    }
	    break;

	default:
	    /* Unknown "set lockmode" parameter */
	    (VOID) psf_error(E_PS0351_UNKNOWN_LOCKPARM, 0L, PSF_INTERR, 
		&err_code, err_blk, 1,
		(i4) sizeof(char_type), &char_type);
	    return (E_DB_SEVERE);    /* non-zero return means error */
    }

    if (err_no != 0L)
    {
	(VOID) psf_error(err_no, 0L, PSF_USERERR, &err_code, err_blk, 1,
	    (i4) STlength(char_val), char_val);
	return (E_DB_ERROR);	/* non-zero return means error */
    }
    else
    {
	if (not_distr)
	    char_array->data_in_size += sizeof (DMC_CHAR_ENTRY);
    }

    return(E_DB_OK);
}

/*
** Name: psl_lm4_setlockparm_num()  - perform semantic action for SETLOCKPARM
**				      production when the characteristic value
**				      has been specified as a number or a string
**				      constant containing a number
**
** Input:
**	char_type		    characteristic type
**	char_val		    value as specified by the user
**	sess_cb			    PSF session CB
**	    pss_distrib	    	    DB_3_DDB_SESS if distributed thread
**	    pss_stmt_flags  	    PSS_SET_LOCKMODE_SESS if SET LOCKMODE
**				    SESSION (distributed thread only)
**	    pss_object		    points to DMC_CB (or QEF_RCB) structure
**		dmc_char_array	    characteristics array
**		dmc_sl_scope	    scope of SET LOCKMODE
**	    pss_auxrng
**		pss_rerng	    if setting lockmode on a table, table's
**				    description can be found here
**
** Output:
**	err_blk		    filled in if an error occurred
**
** Returns:
**	E_DB_{OK, ERROR, SEVERE}
**
** Side effects:
**	None
**
**  History:
**	07-mar-91 (andre)
**	    plagiarized from SETLOCKPARM production
**	21-apr-92 (barbara)
**	    Added support for Sybil.  For distributed thread, on
**	    SET LOCKMODE SESSION .. TIMEOUT we set timeout value
**	    specifically for QEF.
**	19-oct-92 (barbara)
**	    Test for non_distrib before testing dmc_cb fields (because
**	    Star doesn't have valid dmc_cb).
*/
DB_STATUS
psl_lm4_setlockparm_num(
	i4	    char_type,
	i4	    char_val,
	PSS_SESBLK  *sess_cb,
	DB_ERROR    *err_blk)
{
    i4		err_code, err_no = 0L;
    DMC_CHAR_ENTRY	*chr;
    DMC_CB		*dmc_cb;
    bool		not_distr = ~sess_cb->pss_distrib & DB_3_DDB_SESS;


    if (not_distr)
    {
    	dmc_cb = (DMC_CB *) sess_cb->pss_object;

    	if (dmc_cb->dmc_char_array.data_in_size / sizeof (DMC_CHAR_ENTRY) ==
	    MAX_LOCKMODE_CHARS)
    	{
	    (VOID) psf_error(5931L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);	/* non-zero return means error */
    	}   

        chr = (DMC_CHAR_ENTRY *)
	    ((char *) dmc_cb->dmc_char_array.data_address +
	     dmc_cb->dmc_char_array.data_in_size);
    }

    switch (char_type)
    {
	case LOCKLEVEL:
	    /* Can't set locklevel to a number */
	    err_no = 5924L;
	    break;

	case READLOCK:
	    /* Can't set readlocks to a number */
	    err_no = 5925L;
	    break;

	case MAXLOCKS:
	    if (not_distr)	
	    {
	    	chr->char_id = DMC_C_LMAXLOCKS;
	    }
	    break;

	case TIMEOUT:
	{
	    extern PSF_SERVBLK  *Psf_srvblk;

	    /*
	    ** if server was started with OPF flag which may result in
	    ** deadlock when accesing SCONSUR catalogs, iihistogram, and
	    ** iistatistics, we have to prevent user from specifying
	    ** TIMEOUT=0 for any of these catalogs
	    */

	    if (not_distr &&
		Psf_srvblk->psf_flags & PSF_NO_ZERO_TIMEOUT &&
		dmc_cb->dmc_sl_scope == DMC_SL_TABLE) 
	    {
		DB_TAB_ID	*tabid = &dmc_cb->dmc_table_id;
		PSS_RNGTAB	*tbl = &sess_cb->pss_auxrng.pss_rsrng;
		i4		mask = tbl->pss_tabdesc->tbl_status_mask;

		if (mask & DMT_CATALOG
		    &&
		    char_val == 0L
		    &&
		    (mask & DMT_CONCURRENCY
		     ||
		     tabid->db_tab_base == DM_B_STATISTICS_TAB_ID &&
		     tabid->db_tab_index == DM_I_STATISTICS_TAB_ID
		     ||
		     tabid->db_tab_base == DM_B_HISTOGRAM_TAB_ID  &&
		     tabid->db_tab_index == DM_I_HISTOGRAM_TAB_ID
		    )
		   )
		{
		    (VOID) psf_error(E_PS0352_ILLEGAL_0_TIMEOUT, 0L,
			PSF_USERERR, &err_code, err_blk, 1,
			psf_trmwhite(sizeof(tbl->pss_tabname),
				     (char *) &tbl->pss_tabname),
			&tbl->pss_tabname);
		    return (E_DB_ERROR);
		}
	    }
	    
	    if (not_distr)
	    {
	    	chr->char_id = DMC_C_LTIMEOUT;
	    }
	    else if (sess_cb->pss_stmt_flags & PSS_SET_LOCKMODE_SESS)
	    {
	    	/*
	    	** The distributed server is interested in the value from 
	    	** the SETLOCKKEY production when the SETLOCKSCOPE value
	    	** was session; otherwise the set statement is just
	    	** pased on to the LDBs.
	    	*/
		((QEF_RCB*)sess_cb->pss_object)->qef_r3_ddb_req.
			qer_d14_ses_timeout = char_val;
	    }
	    break;
	}

	default:
	    /* Unknown "set lockmode" parameter */
	    (VOID) psf_error(E_PS0351_UNKNOWN_LOCKPARM, 0L, PSF_INTERR, 
		&err_code, err_blk, 1,
		(i4) sizeof(char_type), &char_type);
	    return (E_DB_SEVERE);    /* non-zero return means error */
    }

    if (err_no != 0L)
    {
	char	num_buf[30];

	CVla((i4) char_val, num_buf);
	(VOID) psf_error(err_no, 0L, PSF_USERERR, &err_code, err_blk, 1,
	    (i4) STlength(num_buf), num_buf);
	return (E_DB_ERROR);    /* non-zero return means error */
    }
    else if (not_distr)
    {
	chr->char_value = char_val;
	dmc_cb->dmc_char_array.data_in_size += sizeof (DMC_CHAR_ENTRY);
    }

    return(E_DB_OK);
}

/*
** Name: psl_lm5_setlockmode_distr()- perform semantic actions for SETLOCKMODE
**				      production.
**
** Description:
**	perform semantic action for SETLOCKMODE production in SQL
**	grammar for distributed thread.  SET LOCKMODE SESSION ... TIMEOUT
**	is handled by QEF in the distributed server by timing out on
**	queries to LDB; all other SET LOCKMODE ON TABLE options are sent
**	to QEF to pass on to LDB as text.
**
** Inputs:
**      sess_cb             ptr to a PSF session CB
**	    pss_distrib	    DB_3_DDB_SESS if distributed thread
**	    pss_stmt_flags  PSS_SET_LOCKMODE_SESS if SET LOCKMODE SESSION
**			    (distributed thread only)
**	scanbuf_ptr	    ptr into scanner buffer containing text of
**			    SET LOCKMODE statement
**      xlated_qry          For query text buffering
**
** Outputs:
**      err_blk             will be filled in if an error occurs
**
** Returns:
**      E_DB_{OK, ERROR}
**
** History:
**      5-mar-1991 (barbara)
**	    Extracted from 6.4 Star grammar for Sybil.
** 	4-dec-92 (barbara)
**	    If call to QEF fails, error has already been reported.  PSF must
**	    set psq_error (or SCF will complain); however, we want to avoid
**	    PSF retry so we also set PSQ_REDO to pretend we are already in
**	    server retry.
**	06-oct-93 (barbara)
**	    Fix for bug 53492.  SET LOCKMODE SESSION ... is now supported for
**	    options other than TIMEOUT.
*/
DB_STATUS
psl_lm5_setlockmode_distr(
	PSS_SESBLK  	*sess_cb,
	char		*scanbuf_ptr,
	PSS_Q_XLATE     *xlated_qry,
	PSQ_CB    	*psq_cb)
{
    QEF_RCB	*qef_rb;
    DB_STATUS	status;
    DB_ERROR	*err_blk = &psq_cb->psq_error;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	qef_rb = (QEF_RCB *) sess_cb->pss_object;

	if (   sess_cb->pss_stmt_flags & PSS_SET_LOCKMODE_SESS
	    && qef_rb->qef_r3_ddb_req.qer_d14_ses_timeout != DMC_C_UNKNOWN)
	{
	    status = qef_call(QED_SES_TIMEOUT, ( PTR ) qef_rb);

	    if (DB_FAILURE_MACRO(status))
	    {
		err_blk->err_code = E_PS0001_USER_ERROR;
		/*
		** Make it appear as if query is already in retry so that
		** psq_call won't retry this failed statement.
		*/
		psq_cb->psq_flag |= PSQ_REDO;
		return(status);
	    }
	}

	else
	{
	    status = psq_x_add(sess_cb, (char *)scanbuf_ptr, &sess_cb->pss_ostream,
			 xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
			 (i4) ((char *) sess_cb->pss_endbuf - scanbuf_ptr),
			 " ", (char *) sess_cb->pss_endbuf, (char *) NULL,
			 err_blk);
		
	    if (DB_FAILURE_MACRO(status))
	    {
		return(status);
	    }

	    qef_rb->qef_r3_ddb_req.qer_d4_qry_info.qed_q4_pkt_p =
					    xlated_qry->pss_q_list.pss_head;
					    
	    if (~sess_cb->pss_stmt_flags & PSS_SET_LOCKMODE_SESS)
	    {
		status = qef_call(QED_QRY_FOR_LDB, ( PTR ) qef_rb);
	    }
	    else
	    {
		status = qef_call(QED_SET_FOR_RQF, ( PTR ) qef_rb);
	    }
	    if (DB_FAILURE_MACRO(status))
	    { 
		err_blk->err_code = E_PS0001_USER_ERROR;
		/*
		** Make it appear as if query is already in retry so that
		** psq_call won't retry this failed statement.
		*/
		psq_cb->psq_flag |= PSQ_REDO;
		return(status);
	    }
    	}

	/*
	** we are done; deallocate the memory as it will not be used by
	** anyone else
	*/
	status = psf_mclose(sess_cb, &sess_cb->pss_ostream, err_blk);

	if (DB_FAILURE_MACRO(status))
		return (status);

	/*
	** make sure psq_parseqry() realizes that the stream has been closed
	*/
	sess_cb->pss_ostream.psf_mstream.qso_handle = (PTR) NULL;
    }
    return (E_DB_OK);
}


/*
** Name: psl_lm6_setlockscope_distr()- perform semantic actions for SETLOCKSCOPE
**				      production.
**
** Description:
**	Perform semantic action for the distributed part of the setlockscope
**	production in SQL grammar when the rule is of the form "ON obj_spec",
**	e.g.
**
**	setlockscope:	NAME
**		|	ON obj_spec	-- psl_lm6_setlockscope_distr()
**
**	The SET LOCKMODE query thus far parsed is buffered, e.g.
**	"set lockmode on my.tab".  The remainder of the lockmode statement
**	gets buffered in the highest level rule for this statement,
**	setlockmode.  See psl_lm5_setlockmode_distr().
**
** Inputs:
**      sess_cb             ptr to a PSF session CB
**	    pss_distrib	    DB_3_DDB_SESS if distributed thread
**	    pss_object	    pointer to QEF_RCB
**	rngvar		    pointer to range table entry
**      xlated_qry          For query text buffering
**
** Outputs:
**	sess_cb
**	    pss_object	    QEF_RCB initialized for qef_call.
**      err_blk             will be filled in if an error occurs
**
** Returns:
**      E_DB_{OK, ERROR}
**
** History:
**      29-may-1992 (barbara)
**	    Extracted from 6.4 Star grammar for Sybil.
**	06-oct-93 (barbara)
**	    Fix for bug 53492.  SET LOCKMODE SESSION ... is now supported for
**	    options other than TIMEOUT.
*/
DB_STATUS
psl_lm6_setlockscope_distr(
	PSS_SESBLK  	*sess_cb,
	PSS_RNGTAB	*rngvar,
	PSS_Q_XLATE     *xlated_qry,
	DB_ERROR    	*err_blk)
{
    QEF_RCB		*qef_rb = (QEF_RCB *) sess_cb->pss_object;
    i4		err_code;
    DB_STATUS		status = E_DB_OK; 

    if (~sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	return (E_DB_OK);
    }

    if (rngvar == (PSS_RNGTAB *)NULL)
    {
	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream, xlated_qry->pss_buf_size,
			&xlated_qry->pss_q_list, -1, (char *) NULL,
			(char *) NULL, "set lockmode session ", err_blk);

	qef_rb->qef_r3_ddb_req.qer_d2_ldb_info_p = (DD_1LDB_INFO *) NULL;
    }
    else
    {
    	DD_OBJ_DESC		*obj_desc = rngvar->pss_rdrinfo->rdr_obj_desc;
    	DD_2LDB_TAB_INFO	*ldb_tab_info = &obj_desc->dd_o9_tab_info;

        status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream, xlated_qry->pss_buf_size,
			&xlated_qry->pss_q_list, -1, (char *) NULL,
			(char *) NULL, "set lockmode on ", err_blk);

    	if (DB_FAILURE_MACRO(status))
	    return(status);
    	/*
    	** NOTE that since the text of SET LOCKMODE ON table_name ... will
    	** be sent to LDB, we choose to not support it on distributed views
    	** (yet?)
    	*/
    	if (obj_desc->dd_o6_objtype == DD_3OBJ_VIEW)
    	{
	    (VOID) psf_error(5986L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);
    	}

	qef_rb->qef_r3_ddb_req.qer_d2_ldb_info_p = ldb_tab_info->dd_t9_ldb_p;

    	/* Generate the table name */

    	status = psq_prt_tabname(sess_cb, xlated_qry, &sess_cb->pss_ostream, rngvar,
			PSQ_SLOCKMODE, err_blk);
    }

    if (DB_FAILURE_MACRO(status))
    	return(status);

    qef_rb->qef_eflag = QEF_EXTERNAL;
    qef_rb->qef_r3_ddb_req.qer_d1_ddb_p = (DD_DDB_DESC *) NULL;
    qef_rb->qef_r3_ddb_req.qer_d8_usr_p = (DD_USR_DESC *) NULL;
    qef_rb->qef_r3_ddb_req.qer_d4_qry_info.qed_q1_timeout = 0;
    qef_rb->qef_r3_ddb_req.qer_d4_qry_info.qed_q2_lang = DB_SQL;
    qef_rb->qef_r3_ddb_req.qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    qef_rb->qef_r3_ddb_req.qer_d14_ses_timeout=DMC_C_UNKNOWN;

    return (E_DB_OK);
}
