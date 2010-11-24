/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <me.h>
#include    <st.h>
#include    <tm.h>
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
#include    <sxf.h>
#include    <qefrcb.h>
#include    <qefmain.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>
#include    <psyaudit.h>
#include    <scf.h>
#include    <dudbms.h>

/**
**
**  Name: PSYGROUP.C - Functions used to manipulate the iiusergroup catalog.
**
**  Description:
**      This file contains functions necessary to create, alter and drop
**	groups:
**
**          psy_group  - Routes requests to manipulate iiusergroup
**          psy_cgroup - Create group identifiers and initial members
**          psy_agroup - Adds members to an existing group
**          psy_dgroup - Drops members from an existing group
**          psy_kgroup - Drops empty groups
**
**  History:
**	12-mar-89 (ralph)
**	    written
**	20-may-89 (ralph)
**	    Allow multiple groups to be specified.
**	06-jun-89 (ralph)
**	    Fixed unix portability problems
**      29-jul-89 (jennifer)
**          Added auditing of failure to perform operation.
**	30-oct-89 (ralph)
**	    Use pss_ustat for authorization check.
**      08-aug-90 (ralph)
**          Don't allow session to issue CREATE/ALTER/DROP GROUP
**	    if it only has UPDATE_SYSCAT privileges.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	03-aug-92 (barbara)
**	    Initialize RDF_CB through call to pst_rdfcb_init.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	04-dec-1992 (pholman)
**	    C2: replace obsolete qeu_audit call with psy_secaudit.
**	10-mar-1993 (markg)
**	    Fix bug in psy_group() where audit of failed group 
**	    operations was coded incorrectly.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**	16-aug-93 (stephenb)
**	    Changed all calls to psy_secaudit() to audit SXF_E_SECURITY instead
**	    of SXF_E_USER, group manipulation is a security event not a 
**	    user event.
**	3-sep-93 (stephenb)
**	    changed message parameter in call to psy_secaudit() from
**	    I_SX2022_GROUP_ALTER to I_SX2008_GROUP_MEM_CREATE, to make it
**	    clearer and to be consistent with other messages. Also made drop
**	    user from a group an SXF_A_ALTER action instead of SXF_A_DROP.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	18-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping.
**	10-Jan-2001 (jenjo02)
**	    Supply session id to SCF instead of DB_NOSESSION.
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psy_group(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_cgroup(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_agroup(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_dgroup(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_kgroup(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);

/*{
** Name: psy_group  - Dispatch group identifier qrymod routines
**
**  INTERNAL PSF call format: status = psy_group(&psy_cb, sess_cb);
**  EXTERNAL     call format: status = psy_call (PSY_GROUP,&psy_cb, sess_cb);
**
** Description:
**	This procedure checks the psy_grpflag field of the PSY_CB
**	to determine which qrymod processing routine to call:
**		PSY_CGROUP results in call to psy_cgroup()
**		PSY_AGROUP results in call to psy_agroup()
**		PSY_DGROUP results in call to psy_dgroup()
**		PSY_KGROUP results in call to psy_kgroup()
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_grpflag		Group identifier operation
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
**	    Allow multiple groups to be specified.
**      29-jul-89 (jennifer)
**          Added auditing of failure to perform operation.
**	30-oct-89 (ralph)
**	    Use pss_ustat for authorization check.
**	10-mar-93 (markg)
**	    Fix bug where audit of failed group operations
**	    was coded incorrectly.
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**	16-aug-93 (stephenb)
**	    Changed call to psy_secaudit() to audit SXF_E_SECURITY instead
**	    of SXF_E_USER, group manipulation is a security event not a 
**	    user event.
**	18-may-1993 (robf)
**	    Replaced SUPERUSER priv.
**	    Add security label to psy_secaudit() call
**	2-nov-93 (robf)
**          Enforce MAINTAIN_USERS priv to access user groups.
*/
DB_STATUS
psy_group(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status = E_DB_OK;
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
		DB_ERROR	    e_error;
		i4		    access = 0;    		        
		i4         msgid;
		PSY_TBL	    *psy_tbl = (PSY_TBL *)psy_cb->psy_tblq.q_next;

		/* Audit that operation rejected. */
		if (psy_cb->psy_grpflag == PSY_CGROUP)
		{	
		    access = SXF_A_FAIL | SXF_A_CREATE;
		    msgid = I_SX2007_GROUP_CREATE;
		}
		else if (psy_cb->psy_grpflag == PSY_AGROUP)
		{
		    access = SXF_A_FAIL | SXF_A_ALTER;
		    msgid = I_SX2008_GROUP_MEM_CREATE;
		}
		else if (psy_cb->psy_grpflag == PSY_DGROUP)
		{
		    access = SXF_A_FAIL | SXF_A_ALTER;
		    msgid = I_SX200A_GROUP_MEM_DROP;
		}
		else if (psy_cb->psy_grpflag == PSY_KGROUP)
		{
		    access = SXF_A_FAIL | SXF_A_DROP;
		    msgid = I_SX2009_GROUP_DROP;
		}
		else
		{
		    err_code = E_PS0D40_INVALID_GRPID_OP;
		    status = E_DB_SEVERE;
		    (VOID) psf_error(E_PS0D40_INVALID_GRPID_OP, 0L,
			PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
		    break;
		}
		
		status = psy_secaudit(FALSE, sess_cb,
		  (char *)&psy_tbl->psy_tabnm,
		  (DB_OWN_NAME *)NULL,
		   sizeof(DB_TAB_NAME), 
		   SXF_E_SECURITY,
		   msgid, access, 
		   &e_error);
	    }

            err_code = E_US18D3_6355_NOT_AUTH;
            (VOID) psf_error(E_US18D3_6355_NOT_AUTH, 0L,
                             PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
                             sizeof("CREATE/ALTER/DROP GROUP")-1,
                             "CREATE/ALTER/DROP GROUP");
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
                             sizeof("CREATE/ALTER/DROP GROUP")-1,
                             "CREATE/ALTER/DROP GROUP");
	    status   = E_DB_ERROR;
	    break;
	    

	}

	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);

    /* Select the proper routine to process this request */

    if (status == E_DB_OK)
    switch (psy_cb->psy_grpflag)
    {
	case PSY_CGROUP:
	    status = psy_cgroup(psy_cb, sess_cb);
	    break;

	case PSY_AGROUP:
	    status = psy_agroup(psy_cb, sess_cb);
	    break;

	case PSY_DGROUP:
	    status = psy_dgroup(psy_cb, sess_cb);
	    break;

	case PSY_KGROUP:
	    status = psy_kgroup(psy_cb, sess_cb);
	    break;

	default:
	    err_code = E_PS0D40_INVALID_GRPID_OP;
	    status = E_DB_SEVERE;
	    (VOID) psf_error(E_PS0D40_INVALID_GRPID_OP, 0L,
		    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    break;
    }

    return    (status);
} 

/*{
** Name: psy_cgroup - Create group
**
**  INTERNAL PSF call format: status = psy_cgroup(&psy_cb, sess_cb);
**
** Description:
**	This procedures create a group identifier, with or without
**	members.  If the group already exists, the statement is aborted.
**	If the group does not exist, the group and all specified members
**	are added to iiusergroup.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_tblq			head of group  queue
**	    .psy_usrq			head of member queue
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
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores tuples in iiusergroup.
**
** History:
**	12-mar-89 (ralph)
**          written
**	20-may-89 (ralph)
**	    Allow multiple groups to be specified.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
*/
DB_STATUS
psy_cgroup(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_USERGROUP	ugtuple;
    register DB_USERGROUP *ugtup = &ugtuple;
    PSY_TBL		*psy_tbl;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_APPEND;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_GROUP;
    rdf_rb->rdr_qrytuple    = (PTR) ugtup;
    rdf_rb->rdr_qtuple_count = 1;

    MEfill(sizeof(DB_OWN_NAME), (u_char)' ', (PTR)&ugtup->dbug_member);
    MEfill(sizeof(ugtup->dbug_reserve),
	   (u_char)' ',
	   (PTR)ugtup->dbug_reserve);

    status = E_DB_OK;

    for (psy_tbl  = (PSY_TBL *)  psy_cb->psy_tblq.q_next;
	 psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_tbl  = (PSY_TBL *)  psy_tbl->queue.q_next
	)
    {
	/* STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm,
			       ugtup->dbug_group); */
	MEcopy((PTR)&psy_tbl->psy_tabnm,
	       sizeof(ugtup->dbug_group),
	       (PTR)&ugtup->dbug_group);

	stat   = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;

	if (stat > E_DB_INFO)				    /* Mark group bad */
	    MEfill(sizeof(psy_tbl->psy_tabnm),
		   (u_char)' ',
		   (PTR)&psy_tbl->psy_tabnm);
    }

    if (DB_FAILURE_MACRO(status))
        (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			     &psy_cb->psy_error);
    else if (psy_cb->psy_usrq.q_next != &psy_cb->psy_usrq)
	    status = psy_agroup(psy_cb, sess_cb);	/* Process members */

    return    (status);
}

/*{
** Name: psy_agroup - Add members to group
**
**  INTERNAL PSF call format: status = psy_agroup(&psy_cb, sess_cb);
**
** Description:
**	This procedures adds members to an existing group identifier.
**	If the group does not exist, the statement is aborted.
**	If the group does exist, all specified members are added to
**	iiusergroup.  If a member already exists, a warning is issued,
**	but the statement is not aborted.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_tblq			head of group  queue
**	    .psy_usrq			head of member queue
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_INFO			One or more members were rejected.
**	    E_DB_WARN			One or more groups  were rejected.
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores tuples in iiusergroup.
**
** History:
**	12-mar-89 (ralph)
**          written
**	20-may-89 (ralph)
**	    Allow multiple groups to be specified.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
*/
DB_STATUS
psy_agroup(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_USERGROUP	ugtuple;
    register DB_USERGROUP *ugtup = &ugtuple;
    PSY_TBL		*psy_tbl;
    PSY_USR		*psy_usr;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_APPEND;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_GROUP;
    rdf_rb->rdr_qrytuple    = (PTR) ugtup;
    rdf_rb->rdr_qtuple_count = 1;

    MEfill(sizeof(ugtup->dbug_reserve),
	   (u_char)' ',
	   (PTR)ugtup->dbug_reserve);

    status = E_DB_OK;

    for (psy_tbl  = (PSY_TBL *)  psy_cb->psy_tblq.q_next;
	 psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_tbl  = (PSY_TBL *)  psy_tbl->queue.q_next
	)
    {
	if (STskipblank((char *)&psy_tbl->psy_tabnm,
			(i4)sizeof(psy_tbl->psy_tabnm)) == NULL)
	    continue;

	/* STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm,
			       ugtup->dbug_group); */
	MEcopy((PTR)&psy_tbl->psy_tabnm,
	       sizeof(ugtup->dbug_group),
	       (PTR)&ugtup->dbug_group);

	stat = E_DB_OK;

	for (psy_usr  = (PSY_USR *)  psy_cb->psy_usrq.q_next;	/* Each user  */
	     psy_usr != (PSY_USR *) &psy_cb->psy_usrq;
	     psy_usr  = (PSY_USR *)  psy_usr->queue.q_next
	    )
	{
	    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, ugtup->dbug_member);

	    stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	    status = (stat > status) ? stat : status;

	    if (stat > E_DB_INFO)
		break;
	}

	if (DB_FAILURE_MACRO(status))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
			     &psy_cb->psy_error);

    return    (status);
}
 

/*{
** Name: psy_dgroup - Removes members from a group
**
**  INTERNAL PSF call format: status = psy_dgroup(&psy_cb, sess_cb);
**
** Description:
**	This procedure removes memvers from an existing group identifier.
**	All specified members are removed from iiusergroup.
**	If a member does not exist, a warning is issued, but the
**	statement is not aborted.
**	If the user queue is null, all members are removed from
**	the group.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_tblq			head of group  queue
**	    .psy_usrq			head of member queue
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_INFO			One or more members were rejected.
**	    E_DB_WARN			One or more groups  were rejected.
**	    E_DB_ERROR			Function failed; non-catastrophic error.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Removed tuples from iiusergroup.
**
** History:
**	13-mar-89 (ralph)
**          written
**	20-may-89 (ralph)
**	    Allow multiple groups to be specified.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
*/
DB_STATUS
psy_dgroup(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_USERGROUP	ugtuple;
    register DB_USERGROUP *ugtup = &ugtuple;
    PSY_TBL		*psy_tbl;
    PSY_USR		*psy_usr;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_DELETE;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_GROUP;
    rdf_rb->rdr_qrytuple    = (PTR) ugtup;
    rdf_rb->rdr_qtuple_count = 1;

    MEfill(sizeof(ugtup->dbug_reserve),
	   (u_char)' ',
	   (PTR)ugtup->dbug_reserve);

    status = E_DB_OK;

    for (psy_tbl  = (PSY_TBL *)  psy_cb->psy_tblq.q_next;
	 psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_tbl  = (PSY_TBL *)  psy_tbl->queue.q_next
	)
    {
	/* STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm,
			       ugtup->dbug_group); */
	MEcopy((PTR)&psy_tbl->psy_tabnm,
	       sizeof(ugtup->dbug_group),
	       (PTR)&ugtup->dbug_group);

	stat = E_DB_OK;

	if ((PSY_USR *) psy_cb->psy_usrq.q_next		/* DROP ALL specified */
			== (PSY_USR *) &psy_cb->psy_usrq)
	{
	    MEfill(sizeof(DB_OWN_NAME), (u_char)' ', 
		   (PTR)&ugtup->dbug_member);
	    rdf_rb->rdr_update_op   = RDR_PURGE;
	    stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	    status = (stat > status) ? stat : status;
	}
	else
	for (psy_usr  = (PSY_USR *)  psy_cb->psy_usrq.q_next;
	     psy_usr != (PSY_USR *) &psy_cb->psy_usrq;
	     psy_usr  = (PSY_USR *)  psy_usr->queue.q_next
	    )
	{
	    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, ugtup->dbug_member);

	    stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	    status = (stat > status) ? stat : status;

	    if (stat > E_DB_INFO)
		break;
	}

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			     &psy_cb->psy_error);

    return    (status);
} 

/*{
** Name: psy_kgroup - Destroy group
**
**  INTERNAL PSF call format: status = psy_kgroup(&psy_cb, sess_cb);
**
** Description:
**	This procedure deletes group identifiers.  No members can be
**	in the group to be deleted.  If members exist in a group,
**	the group is not deleted.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_tblq			head of group  queue
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_INFO			Function completed with message(s).
**	    E_DB_WARN			One or more groups were rejected.
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Removed tuples from iiusergroup.
**
** History:
**	13-mar-89 (ralph)
**          written
**	20-may-89 (ralph)
**	    Allow multiple groups to be specified.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
*/
DB_STATUS
psy_kgroup(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_USERGROUP	ugtuple;
    register DB_USERGROUP *ugtup = &ugtuple;
    PSY_TBL		*psy_tbl;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_DELETE;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_GROUP;
    rdf_rb->rdr_qrytuple    = (PTR) ugtup;
    rdf_rb->rdr_qtuple_count = 1;

    MEfill(sizeof(DB_OWN_NAME), (u_char)' ', (PTR)&ugtup->dbug_member);
    MEfill(sizeof(ugtup->dbug_reserve),
	   (u_char)' ',
	   (PTR)ugtup->dbug_reserve);

    status = E_DB_OK;

    for (psy_tbl  = (PSY_TBL *)  psy_cb->psy_tblq.q_next;
	 psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_tbl  = (PSY_TBL *)  psy_tbl->queue.q_next
	)
    {
	/* STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm,
			       ugtup->dbug_group); */
	MEcopy((PTR)&psy_tbl->psy_tabnm,
	       sizeof(ugtup->dbug_group),
	       (PTR)&ugtup->dbug_group);

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
