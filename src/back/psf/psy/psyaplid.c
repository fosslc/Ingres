/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <tm.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmacb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>
#include    <scf.h>
#include    <sxf.h>
#include    <dudbms.h>
#include    <psyaudit.h>

/**
**
**  Name: PSYAPLID.C - Functions to manipulate the iirole catalog.
**
**  Description:
**      This file contains functions necessary to create, alter and drop
**	roles:
**
**          psy_aplid  - Routes requests to manipulate iirole
**          psy_caplid - Creates roles
**          psy_aaplid - Changes role passwords
**          psy_kaplid - Drops roles
**
**
**  History:
**	12-mar-89 (ralph)
**	    written
**	20-may-89 (ralph)
**	    Allow multiple roles to be specified.
**	06-jun-89 (ralph)
**	    Corrected unix portability problems
**	30-oct-89 (ralph)
**	    Use pss_ustat for authorization check.
**	08-aug-90 (ralph)
**	    Don't allow session to issue CREATE/ALTER/DROP ROLE
**	    if it only has UPDATE_SYSCAT privileges.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	03-aug-92 (barbara)
**	    Call pst_rdfcb_init() to initalize RDF_CB before call to rdf_call()
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	16-nov-1992 (pholman)
**	    C2: replace obsolete qeu_audit with psy_secaudit.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed cause of a compiler warning
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	18-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping.
**	10-Jan-2001 (jenjo02)
**	    Supply session id to SCF instead of DB_NOSESSION.
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**	19-July-2005 (toumi01)
**	    Support WITH PASSWORD=X'<encrypted>'. 
**      20-nov-2008 (stial01)
**          psy_kaplid is using wrong size when initializing dbap_aplid
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psy_aplid(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_caplid(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_aaplid(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_kaplid(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);

/*{
** Name: psy_aplid - Dispatch role qrymod routines
**
**  INTERNAL PSF call format: status = psy_aplid(&psy_cb, sess_cb);
**  EXTERNAL     call format: status = psy_call (PSY_APLID,&psy_cb, sess_cb);
**
** Description:
**	This procedure checks the psy_aplflag field of the PSY_CB
**	to determine which qrymod processing routine to call:
**		PSY_CAPLID results in call tp psy_caplid()
**		PSY_AAPLIDresults in call tp psy_aaplid()
**		PSY_KAPLID results in call tp psy_kaplid()
**
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_aplflag		role operation
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	12-mar-89 (ralph)
**          written
**	20-may-89 (ralph)
**	    Allow multiple roles to be specified.
**      29-jul-89 (jennifer)
**          Audit failure to perform operation.
**	30-oct-89 (ralph)
**	    Use pss_ustat for authorization check.
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**	18-may-1993 (robf)
**	    Replaced SUPERUSER priv.
**	    Pass security label in psy_secaudit()
**	    Check if roles are allowed, and if they need passwords.
**	8-mar-94 (robf)
**          Major enhancements to roles to support privileges/auditing
**	    similar to users.
*/
DB_STATUS
psy_aplid(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status;
    i4		err_code;
    char		dbname[DB_DB_MAXNAME];
    SCF_CB		scf_cb;
    SCF_SCI		sci_list[1];
    bool		leave_loop = TRUE;

    /* This code is called for SQL only */

    /* Make sure user is authorized and connected to iidbdb */

    do
    {
	if (!(sess_cb->pss_ustat & DU_UMAINTAIN_USER))
	{
	    if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
	    {
		DB_STATUS	    local_status;
		DB_ERROR	    e_error;
		i4              accessmask;
		i4         msg_id;
		PSY_TBL	    *psy_tbl = (PSY_TBL *)psy_cb->psy_tblq.q_next;

		/* Audit that operation rejected. */

		if (psy_cb->psy_aplflag == PSY_CAPLID)
		{	
		    accessmask = (SXF_A_FAIL | SXF_A_CREATE);
		    msg_id = I_SX2001_ROLE_CREATE;
		}
		else if (psy_cb->psy_aplflag == PSY_AAPLID)
		{
		    accessmask = (SXF_A_FAIL | SXF_A_ALTER);
		    msg_id     = I_SX2021_ROLE_ALTER;
		}
		else if (psy_cb->psy_aplflag == PSY_KAPLID)
		{
		    accessmask = (SXF_A_FAIL | SXF_A_DROP);
		    msg_id = I_SX2002_ROLE_DROP;
		}	    
		
		local_status = psy_secaudit(FALSE, sess_cb,
			    (char *)&psy_tbl->psy_tabnm,
			    (DB_OWN_NAME *)NULL,
			    sizeof(DB_TAB_NAME), SXF_E_SECURITY,
			    msg_id, accessmask,
			    &e_error);
	    }

            err_code = E_US18D3_6355_NOT_AUTH;
            (VOID) psf_error(E_US18D3_6355_NOT_AUTH, 0L,
                             PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
                             sizeof("CREATE/ALTER/DROP ROLE")-1,
                             "CREATE/ALTER/DROP ROLE");
	    status   = E_DB_ERROR;
	    break;
	}
	scf_cb.scf_length	= sizeof (SCF_CB);
	scf_cb.scf_type	= SCF_CB_TYPE;
	scf_cb.scf_facility	= DB_PSF_ID;
        scf_cb.scf_session	= sess_cb->pss_sessid;
        scf_cb.scf_ptr_union.scf_sci     = (SCI_LIST *) sci_list;
        scf_cb.scf_len_union.scf_ilength = 1;
        sci_list[0].sci_length  = sizeof(dbname);
        sci_list[0].sci_code    = SCI_DBNAME;
        sci_list[0].sci_aresult = (char *) dbname;
        sci_list[0].sci_rlength = NULL;

        status = scf_call(SCU_INFORMATION, &scf_cb);
        if (status != E_DB_OK)
        {
	    err_code = scf_cb.scf_error.err_code;
	    (VOID) psf_error(E_PS0D13_SCU_INFO_ERR, 0L,
			     PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
	    break;
        }

	if (MEcmp((PTR)dbname, (PTR)DB_DBDB_NAME, sizeof(dbname)))
	{
	    /* Session not connected to iidbdb */
            err_code = E_US18D4_6356_NOT_DBDB;
            (VOID) psf_error(E_US18D4_6356_NOT_DBDB, 0L,
                             PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
                             sizeof("CREATE/ALTER/DROP ROLE")-1,
                             "CREATE/ALTER/DROP ROLE");
	    status   = E_DB_ERROR;
	    break;
	    

	}
	/*
	** Check if roles are allowed
	*/
	if(sess_cb->pss_ses_flag & PSS_ROLE_NONE)
	{
	    err_code=E_US18E7_6375;
            (VOID) psf_error(E_US18E7_6375, 0L,
                             PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
                             sizeof("CREATE/ALTER/DROP ROLE")-1,
                             "CREATE/ALTER/DROP ROLE");
	    status   = E_DB_ERROR;
	    break;
	}
	/*
	** Check if role passwords are required.
	*/
	if((sess_cb->pss_ses_flag & PSS_ROLE_NEED_PW) &&
	    ( psy_cb->psy_aplflag== PSY_CAPLID  ||
	     (psy_cb->psy_aplflag==PSY_AAPLID && 
	      (psy_cb->psy_usflag & PSY_USRPASS))) && 
	      !(psy_cb->psy_usflag & PSY_USREXTPASS) &&
    	    STskipblank((char *)&psy_cb->psy_apass,
			   (i4)sizeof(psy_cb->psy_apass)) == NULL)
	{
	    err_code=E_US18E7_6375;
            (VOID) psf_error(E_US18E7_6375, 0L,
                             PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
                             sizeof("CREATE/ALTER ROLE WITH NOPASSWORD")-1,
                             "CREATE/ALTER ROLE WITH NOPASSWORD");
	    status   = E_DB_ERROR;
	    break;
	}
	break;
	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);

    /* Select the proper routine to process this request */

    if (status == E_DB_OK)
    switch (psy_cb->psy_aplflag)
    {
	case PSY_CAPLID:
	    status = psy_caplid(psy_cb, sess_cb);
	    break;

	case PSY_AAPLID:
	    status = psy_aaplid(psy_cb, sess_cb);
	    break;

	case PSY_KAPLID:
	    status = psy_kaplid(psy_cb, sess_cb);
	    break;

	default:
	    status = E_DB_SEVERE;
	    err_code = E_PS0D41_INVALID_APLID_OP;
	    (VOID) psf_error(E_PS0D41_INVALID_APLID_OP, 0L,
		    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    break;
    }

    return    (status);
}

/*{
** Name: psy_caplid - Create application_id
**
**  INTERNAL PSF call format: status = psy_caplid(&psy_cb, sess_cb);
**
** Description:
**	This procedure creates an role.  If the
**	role already exists, the statement is aborted.
**	If the role does not exist, it is added to
**	iirole.  The specified password is encrypted before
**	it is put in the catalog.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_tblq			role list
**	    .psy_apass			applicaiton id password
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores tuples in iirole.
**
** History:
**	13-mar-89 (ralph)
**          written
**	20-may-89 (ralph)
**	    Allow multiple roles to be specified.
**	    Changed interface to scu_xencode.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	8-mar-94 (robf)
**          Save privilege and security audit masks.
**	23-mar-94 (robf)
**          Initialize auditmask to 0, otherwise could get bogus attributes
**	    set. 
*/
DB_STATUS
psy_caplid(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    i4		err_code;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_APPLICATION_ID	aptuple;
    register DB_APPLICATION_ID *aptup = &aptuple;
    SCF_CB		scf_cb;
    PSY_TBL		*psy_tbl;
    bool		encrypt;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_APPEND;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_APLID;
    rdf_rb->rdr_qrytuple    = (PTR) aptup;
    rdf_rb->rdr_qtuple_count = 1;

    aptup->dbap_flagsmask= 0;

    MEfill(sizeof(aptup->dbap_reserve),
	   (u_char)' ',
	   (PTR)aptup->dbap_reserve);

    if (psy_cb->psy_usflag & PSY_USRPRIVS)
	aptup->dbap_status = (i4) psy_cb->psy_usprivs;
    else
	aptup->dbap_status = 0;
    /*
    ** Save security audit options
    */
    if (psy_cb->psy_usflag & PSY_USRSECAUDIT)
    {
	if(psy_cb->psy_ussecaudit & PSY_USAU_ALL_EVENTS)
		aptup->dbap_status |= DU_UAUDIT;
	if (psy_cb->psy_ussecaudit & PSY_USAU_QRYTEXT)
		aptup->dbap_status |= DU_UAUDIT_QRYTEXT;
    }
    if (psy_cb->psy_usflag & PSY_USREXTPASS)
	    aptup->dbap_flagsmask |= DU_UEXTPASS;

    encrypt = (STskipblank((char *)&psy_cb->psy_apass,
			   (i4)sizeof(psy_cb->psy_apass))
			    != NULL);
    if (encrypt)
    {
	scf_cb.scf_length	= sizeof (SCF_CB);
	scf_cb.scf_type	= SCF_CB_TYPE;
	scf_cb.scf_facility	= DB_PSF_ID;
	scf_cb.scf_session	= sess_cb->pss_sessid;
	scf_cb.scf_nbr_union.scf_xpasskey =
	    (PTR)&aptup->dbap_aplid;
	scf_cb.scf_ptr_union.scf_xpassword =
	    (PTR)&aptup->dbap_apass;
	scf_cb.scf_len_union.scf_xpwdlen =
	     sizeof(aptup->dbap_apass);
    }
    else
	STRUCT_ASSIGN_MACRO(psy_cb->psy_apass, aptup->dbap_apass);

    status = E_DB_OK;

    for (psy_tbl  = (PSY_TBL *)  psy_cb->psy_tblq.q_next;
	 psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_tbl  = (PSY_TBL *)  psy_tbl->queue.q_next
	)
    {
	/* STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm,
			       aptup->dbap_aplid); */
	MEcopy((PTR)&psy_tbl->psy_tabnm,
	       sizeof(aptup->dbap_aplid),
	       (PTR)&aptup->dbap_aplid);

	/* Encrypt the password if nonblank and not already hex value */

	if (encrypt)
	{
	    STRUCT_ASSIGN_MACRO(psy_cb->psy_apass, aptup->dbap_apass);
	    if(!(psy_cb->psy_usflag & PSY_HEXPASS))
	    {
		status = scf_call(SCU_XENCODE, &scf_cb);
		if (status != E_DB_OK)
		{
		    err_code = scf_cb.scf_error.err_code;
		    (VOID) psf_error(E_PS0D43_XENCODE_ERROR, 0L,
			PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
		    status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
		    return(status);
		}
	    }
	}

	stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
	    &psy_cb->psy_error);

    return    (status);
}

/*{
** Name: psy_aaplid - Alter role
**
**  INTERNAL PSF call format: status = psy_aaplid(&psy_cb, sess_cb);
**
** Description:
**	This procedure alters an role tuple.  If the
**	role does not exist, the statement is aborted.
**	If the application identifer does exist, the associated
**	iirole tuple is replaced.  The specified password
**	is encrypted before it is put in the catalog.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_tblq			role list
**	    .psy_apass			application id password
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Replaces tuples in iirole.
**
** History:
**	13-mar-89 (ralph)
**          written
**	20-may-89 (ralph)
**	    Allow multiple roles to be specified.
**	    Changed interface to scu_xencode.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	8-mar-94 (robf)
**          save privileges/audit masks
*/
DB_STATUS
psy_aaplid(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    i4		err_code;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_APPLICATION_ID	aptuple;
    register DB_APPLICATION_ID *aptup = &aptuple;
    SCF_CB		scf_cb;
    PSY_TBL		*psy_tbl;
    bool		encrypt;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_REPLACE;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_APLID;
    rdf_rb->rdr_qrytuple    = (PTR) aptup;
    rdf_rb->rdr_qtuple_count = 1;

    status = E_DB_OK;

    MEfill(sizeof(aptup->dbap_reserve),
	   (u_char)' ',
	   (PTR)aptup->dbap_reserve);

    encrypt = (STskipblank((char *)&psy_cb->psy_apass,
			   (i4)sizeof(psy_cb->psy_apass))
			    != NULL);
    aptup->dbap_flagsmask =0;

    aptup->dbap_status = (i4) psy_cb->psy_usprivs;
    if (psy_cb->psy_usflag & PSY_USRAPRIVS)
	aptup->dbap_flagsmask |=  DU_UAPRIV;
    else if (psy_cb->psy_usflag & PSY_USRDPRIVS)
	aptup->dbap_flagsmask |=  DU_UDPRIV;
    else if (psy_cb->psy_usflag & PSY_USRPRIVS)
	aptup->dbap_flagsmask |=  DU_UPRIV;
    else
	aptup->dbap_status = 0;

    if (psy_cb->psy_usflag & PSY_USRPASS)
    {
	aptup->dbap_flagsmask|= DU_UPASS;

        if (psy_cb->psy_usflag & PSY_USREXTPASS)
	    aptup->dbap_flagsmask |= DU_UEXTPASS;
    }
    /*
    ** Check if updating security audit options
    */
    if (psy_cb->psy_usflag & PSY_USRSECAUDIT)
    {
	if(psy_cb->psy_ussecaudit & PSY_USAU_ALL_EVENTS)
		aptup->dbap_flagsmask |= DU_UALLEVENTS;
	else
		aptup->dbap_flagsmask |= DU_UDEFEVENTS;

	if (psy_cb->psy_ussecaudit & PSY_USAU_QRYTEXT)
		aptup->dbap_flagsmask |= DU_UQRYTEXT;
	else
		aptup->dbap_flagsmask |= DU_UNOQRYTEXT;
    }

    if (encrypt)
    {
	scf_cb.scf_length	= sizeof (SCF_CB);
	scf_cb.scf_type	= SCF_CB_TYPE;
	scf_cb.scf_facility	= DB_PSF_ID;
	scf_cb.scf_session	= sess_cb->pss_sessid;
	scf_cb.scf_nbr_union.scf_xpasskey =
	    (PTR)&aptup->dbap_aplid;
	scf_cb.scf_ptr_union.scf_xpassword =
	    (PTR)&aptup->dbap_apass;
	scf_cb.scf_len_union.scf_xpwdlen =
	     sizeof(aptup->dbap_apass);
    }
    else
	STRUCT_ASSIGN_MACRO(psy_cb->psy_apass, aptup->dbap_apass);

    status = E_DB_OK;

    for (psy_tbl  = (PSY_TBL *)  psy_cb->psy_tblq.q_next;
	 psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_tbl  = (PSY_TBL *)  psy_tbl->queue.q_next
	)
    {
	/* STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm,
			       aptup->dbap_aplid); */
	MEcopy((PTR)&psy_tbl->psy_tabnm,
	       sizeof(aptup->dbap_aplid),
	       (PTR)&aptup->dbap_aplid);

	/* Encrypt the password if nonblank and not already hex value */

	if (encrypt)
	{
	    STRUCT_ASSIGN_MACRO(psy_cb->psy_apass, aptup->dbap_apass);
	    if(!(psy_cb->psy_usflag & PSY_HEXPASS))
	    {
		status = scf_call(SCU_XENCODE, &scf_cb);
		if (status != E_DB_OK)
		{
		    err_code = scf_cb.scf_error.err_code;
		    (VOID) psf_error(E_PS0D43_XENCODE_ERROR, 0L,
			PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
		    status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
		    return(status);
		}
	    }
	}

	stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
	    &psy_cb->psy_error);

    return    (status);
} 

/*{
** Name: psy_kaplid - Destroy role
**
**  INTERNAL PSF call format: status = psy_kaplid(&psy_cb, sess_cb);
**
** Description:
**	This procedure deletes an role.  If an
**	role does not exist, the statement is aborted.
**	If the role does exist, the associated
**	iirole tuple is deleted.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_tblq			role list
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_INFO			Function completed with warning(s).
**	    E_DB_WARN			One ore more roles were rejected.
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Removed tuples from iirole
**
** History:
**	13-mar-89 (ralph)
**          written
**	20-may-89 (ralph)
**	    Allow multiple roles to be specified.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
*/
DB_STATUS
psy_kaplid(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_APPLICATION_ID	aptuple;
    register DB_APPLICATION_ID *aptup = &aptuple;
    PSY_TBL		*psy_tbl;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_DELETE;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_APLID;
    rdf_rb->rdr_qrytuple    = (PTR) aptup;
    rdf_rb->rdr_qtuple_count = 1;

    MEfill(sizeof(aptup->dbap_reserve),
	   (u_char)' ',
	   (PTR)aptup->dbap_reserve);

    MEfill(sizeof(aptup->dbap_apass), (u_char)' ', (PTR)&aptup->dbap_apass);

    status = E_DB_OK;

    for (psy_tbl  = (PSY_TBL *)  psy_cb->psy_tblq.q_next;
	 psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_tbl  = (PSY_TBL *)  psy_tbl->queue.q_next
	)
    {
	/* STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm,
			       aptup->dbap_aplid); */
	MEcopy((PTR)&psy_tbl->psy_tabnm,
	       sizeof(aptup->dbap_aplid),
	       (PTR)&aptup->dbap_aplid);

	stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			     &psy_cb->psy_error);

    return    (status);
} 
