/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <cm.h>
#include    <qu.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSTRSLVTBL.C - Function "resolving" table name.
**
**  Description:
**      This file contains a function used to "resolve" a table name.
**	"RESOLVING" table name involves making sure that a table exists and that
**	the user may be allowed some form access to the table.
**
**          pst_resolve_table	- make sure user is allowed some form of access
**				  access to the table whose name was passed
**
**  History:
**      23-jan-90 (andre)    
**          wrote pst_resolve_table.
**	13-sep-90 (teresa)
**	    changed pss_fips_mode from boolean to bitmask in pss_ses_flag
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	09-jul-92 (andre)
**	    changed pst_resolve_table()'s interface to accept two input
**	    parameters: owner name (could be of length 0 to indicate that none
**	    was specified) and table/view/index name.  Name(s) will be supplied
**	    in the correct case and no further processing of names will take
**	    place.
**
**	    If the object turns out to be a QUEL view, we will return it without
**	    attempting to unravel the view definition to determine whether the
**	    user posesses some privilege on every object used in its definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	30-mar-1998 (nanpr01)
**	    Even though the permission is granted to the base table, users
**	    other than owner is unable to retrieve the index information.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	    Changed psf_sesscb() prototype to return PSS_SESBLK* instead
**	    of DB_STATUS.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
i4 pst_resolve_table(
	DB_TEXT_STRING *obj_owner,
	DB_TEXT_STRING *obj_name,
	DB_TEXT_STRING *out);

/*{
** Name: pst_resolve_table()	- make sure user is allowed some form of access
**				  to the table whose name was passed.
**
** Description:
**	If the object with specified name could not be found or if the object
**	is a base table or an SQL view owned by another user and the current
**	session does not posess any privileges on it, output will consist of
**	blanks; 
**	otherwise output will consist of object owner name left-justified within
**	the first DB_GW1_MAXNAME characters (blank padded) and the object name
**	in the second DB_GW1_MAXNAME characters (blank padded).
**
** Input:
**	obj_owner	    - object owner name (if any)
**	obj_name	    - object name
**
** Output:
**	out		    - see description
**
** Returns:
**	E_DB_OK	unless some unexpected error took place.
**
** History:
**	23-jan-90 (andre)
**	    written.
**	18-apr-90 (andre)
**	    interface to psl0_[o]rngent() has changed to facilitate handling of
**	    synonyms.
**	15-jun-92 (barbara)
**	    For Sybil merge change interface to pst_sent and pst_clrrng to
**	    pass in session control block.  Set rdr_r1_distrib field.
**	09-jul-92 (andre)
**	    changed interface of pst_resolve_table() to receive owner name and
**	    object name as seperate input parameters.  Input parameters are
**	    expected to be specified in correct case and ready to be used in a
**	    search for the object.
**
**	    If the object turns out to be a QUEL view, we will return its name
**	    without trying to unravel its definition and establish that the
**	    current session posesses some privilege on every object used in its
**	    definition.
**	11-sep-92 (andre)
**	    removed code to begin and end the transaction - this will be taken
**	    care of in qeq_query()
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
**	18-nov-92 (barbara)
**	    Make call to psl0_orngent consistent with new interface.
**	10-feb-92 (teresa)
**	    Changed RDF I/F to call RDF_READTUPLES instead of RDF_GETINFO.
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**	12-aug-93 (andre)
**	    replace TABLES_TO_CHECK() with PSS_USRTBL|PSS_DBATBL|PSS_INGTBL.
**	    TABLES_TO_CHECK() was being used because long time ago we made an
**	    assumption that 3-tier name space will not be used when processing 
**	    SQL queries while running in FIPS mode.  That assumption proved to 
**	    be incorrect and there is no longer a need to use this macro
**	23-jul-93 (stephenb)
**	    Added check that permit tuple is not a security alarm, security
**	    alarms do not give a user access to a table and should not set
**	    perm_found = TRUE.
**	11-oct-93 (stephenb)
**	    Corrected the above check, I was checking protup->dbp_flags instead
**	    of protup->dbp_popset.
**	19-nov-93 (andre)
**	    the check mentioned above was looking for a right bit in the wrong 
**	    place - instead of dbp_flags we must check dbp_popset
**	30-mar-1998 (nanpr01)
**	    Even though the permission is granted to the base table, users
**	    other than owner is unable to retrieve the index information.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	19-Jun-2010 (kiria01) b123951
**	    Add extra parameter to psl0_rngent for WITH support.
**      30-Nov-2010 (hanal04) Bug 124758
**          If the session is from an old client that does not have long (256)
**          object names the client will expect 64 characters where the first 
**          32 are for the owner and the second 32 are for the table name.
**          Use the length established in the ADF control block when
**          establishing the result and performing the initial error check.
*/
DB_STATUS
pst_resolve_table(
	DB_TEXT_STRING	*obj_owner,
	DB_TEXT_STRING  *obj_name,
	DB_TEXT_STRING	*out)
{
    RDF_CB		    rdf_cntrl_block, *rdf_cb = &rdf_cntrl_block;
    RDR_RB		    *rdf_rb = &rdf_cb->rdf_rb;
    DB_TAB_NAME		    tab_name;
    DB_OWN_NAME		    own_name;
    DB_ERROR		    err_blk;
    i4		    err_code;
    PSS_RNGTAB              *cur_var;
    DB_STATUS		    status, temp_status;
    i4			    rngvar_info;
    PSS_SESBLK		    *sess_cb = (PSS_SESBLK *) NULL;
    DB_OWN_NAME		    *user;
    DB_TAB_OWN		    *group, *applid;
    DB_LANG		    save_lang;
    i4		    mask;
    bool		    perm_found;

    i4			    outcome;
#define		NAME_RESOLVED		1
#define		NAME_NOT_RESOLVED	2
#define		ABNORMAL_ERROR		3
#define		BAD_PARAMETERS		4
    i4                      obj_maxname;
    
    /* first we must obtain the session control block */

    sess_cb = psf_sesscb();
    obj_maxname = ((ADF_CB *)sess_cb->pss_adfcb)->adf_max_namelen;

    /*
    ** if obj_owner or obj_name or out is NULL, or
    **    length of object name not in [1, DB_TAB_MAXNAME], or
    **	  length of owner name not in [0, DB_OWN_MAXNAME],
    **	just return error
    **  Don't need to test count < 0, it's unsigned.
    */
    if (   obj_owner		    == (DB_TEXT_STRING *) NULL
        || obj_name		    == (DB_TEXT_STRING *) NULL
	|| out			    == (DB_TEXT_STRING *) NULL
	|| obj_owner->db_t_count    >  obj_maxname
	|| obj_name->db_t_count     <  1
	|| obj_name->db_t_count     >  obj_maxname
       )
    {
	status = E_DB_ERROR;
	outcome = BAD_PARAMETERS;
	goto wrapup;
    }

    /*
    ** Now we do some real checking:
    ** - make sure table exists
    ** - make sure user is allowed some form of access to it (unless the object
    **   is a QUEL view, in which case we will not bother unraveling the view
    **   definition but simply return the owner name and object name as
    **	 described in the prolog
    */

    sess_cb->pss_auxrng.pss_maxrng = -1;

    /* save ptrs to names of user, group, and application */
    user    = &sess_cb->pss_user;
    group   = &sess_cb->pss_group;
    applid  = &sess_cb->pss_aplid;

    /* we set language to SQL and will reset it before we exit */
    save_lang = sess_cb->pss_lang;
    sess_cb->pss_lang = DB_SQL;
        
    /*
    ** get description of the table, if it exists
    */
    MEmove(obj_name->db_t_count, (PTR) obj_name->db_t_text, ' ',
	sizeof(DB_TAB_NAME), (PTR) &tab_name);

    if (obj_owner->db_t_count == 0)
    {
	/* user specified unqualified table name */

	status = psl0_rngent(&sess_cb->pss_auxrng, -1, "", &tab_name, sess_cb,
	    TRUE, &cur_var, 0, &err_blk, PSS_USRTBL | PSS_DBATBL | PSS_INGTBL,
	    &rngvar_info, 0, NULL);	
    }
    else
    {
	MEmove(obj_owner->db_t_count, (PTR) obj_owner->db_t_text, ' ',
	       sizeof(DB_OWN_NAME), (PTR) &own_name);

	status = psl0_orngent(&sess_cb->pss_auxrng, -1, "", &own_name,
	    &tab_name, sess_cb, TRUE, &cur_var, 0, &err_blk, &rngvar_info, 0);
    }

    if (DB_FAILURE_MACRO(status))
    {
	outcome = ABNORMAL_ERROR;
	goto wrapup;
    }
    else if (cur_var == (PSS_RNGTAB *) NULL)
    {
	/* Table doesn't exist */
	outcome = NAME_NOT_RESOLVED;
	goto wrapup;
    }
    
    /*
    ** save name of the owner and the table; if name specified by the user was a
    ** synonym, names passed to psl0_[o]rngent() are likely to have nothing to
    ** do with the name of the real table
    */
    if (rngvar_info & PSS_BY_SYNONYM)
    {
	STRUCT_ASSIGN_MACRO(cur_var->pss_ownname, own_name);
	STRUCT_ASSIGN_MACRO(cur_var->pss_tabname, tab_name);
    }
    else if (obj_owner->db_t_count == 0)
    {
	/*
	** if user supplied name of a table but not of its owner,
	** save the owner name
	*/
	STRUCT_ASSIGN_MACRO(cur_var->pss_ownname, own_name);
    }
    
    /* if object is owned by the current user, we are done */
    if (!MEcmp((PTR) &cur_var->pss_ownname, (PTR) &sess_cb->pss_user,
	      sizeof(DB_OWN_NAME)))
    {
	outcome = NAME_RESOLVED;
	goto wrapup;
    }

    mask = cur_var->pss_tabdesc->tbl_status_mask;

    /*
    ** if ALL/RETRIEVE has been granted to PUBLIC, we are done
    ** recall that all/retrieve to all is recorded using negative logic
    */
    if (~mask & (DMT_ALL_PROT | DMT_RETRIEVE_PRO))
    {
	outcome = NAME_RESOLVED;
	goto wrapup;
    }

    pst_rdfcb_init(rdf_cb, sess_cb);

    STRUCT_ASSIGN_MACRO(cur_var->pss_tabid, rdf_rb->rdr_tabid);
    /* if table is not base table, get the protections from the base table */ 
    if (rdf_rb->rdr_tabid.db_tab_index > 0)
      rdf_rb->rdr_tabid.db_tab_index = 0;
    rdf_cb->rdf_info_blk	= cur_var->pss_rdrinfo;

    /*
    ** at this point we know that the object is owned by another user.
    ** if the object is a view, obtain the view tree to determine whether this
    ** is a QUEL view, in which case we will return its name without checking
    ** privileges (of which there won't be any)
    */
    if (mask & DMT_VIEW)
    {
	PST_QTREE	    *vtree;

	rdf_rb->rdr_tree_id.db_tre_high_time	= 0;
	rdf_rb->rdr_tree_id.db_tre_low_time	= 0;
	rdf_rb->rdr_types_mask			= RDR_VIEW | RDR_QTREE;
	rdf_rb->rdr_update_op			= 0;
	rdf_rb->rdr_qtuple_count		= 1;
	rdf_rb->rdr_rec_access_id		= NULL;

	status = rdf_call(RDF_GETINFO, (PTR) rdf_cb);

        /*
        ** RDF may choose to allocate a new info block and return its address in
        ** rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid memory
        ** leak and other assorted unpleasantries
        */
	cur_var->pss_rdrinfo = rdf_cb->rdf_info_blk;

	if (DB_FAILURE_MACRO(status))
	{
	    outcome = ABNORMAL_ERROR;
	    goto wrapup;
	}

	/* Make vtree point to the view tree retrieved */
	vtree =
	    ((PST_PROCEDURE *) rdf_cb->rdf_info_blk->rdr_view->qry_root_node)->
		pst_stmts->pst_specific.pst_tree;
	   
	if (vtree->pst_qtree->pst_sym.pst_value.pst_s_root.pst_qlang != DB_SQL)
	{
	    /* this is a QUEL view - we are done */
	    outcome = NAME_RESOLVED;
	    goto wrapup;
	}
    }

    /*
    ** object is a table or an SQL view owned by another user - will check
    ** whether the current session possesses any privilege on the object
    */

    rdf_rb->rdr_types_mask	= RDR_PROTECT;
    rdf_rb->rdr_update_op	= RDR_OPEN;
    rdf_rb->rdr_rec_access_id	= NULL;
					/* Get 20 permits at a time */
    rdf_rb->rdr_qtuple_count	= 20;
					/* Get all permit tuples */
    rdf_rb->rdr_qrymod_id	= 0; 
    rdf_cb->rdf_error.err_code	= 0;

    perm_found = FALSE;

    /*
    ** check IIPROTECT for some tuple involving this object and naming one of
    ** the current DBMS session identifiers (user, group, or role) or PUBLIC
    ** as grantee; if no such tuples are found, we return blank string
    */

    /* For each group of 20 permits */
    while (!perm_found && rdf_cb->rdf_error.err_code == 0)
    {
	register DB_PROTECTION	    *protup;
	i4			    tupcount;
	register QEF_DATA	    *qp;

	status = rdf_call(RDF_READTUPLES, (PTR) rdf_cb);

        /*
        ** RDF may choose to allocate a new info block and return its address in
        ** rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid memory
        ** leak and other assorted unpleasantries
        */
	cur_var->pss_rdrinfo = rdf_cb->rdf_info_blk;

	rdf_rb->rdr_update_op = RDR_GETNEXT;
	if (status != E_DB_OK)
	{
	    i4	rdf_err = rdf_cb->rdf_error.err_code;
	    
	    if (rdf_err == E_RD0002_UNKNOWN_TBL)
	    {
		/* this had better not happen */

		(VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR,
		    &err_code, &err_blk, 1,
		    psf_trmwhite(sizeof(DB_TAB_NAME), 
			(char *) &cur_var->pss_tabname),
		    &cur_var->pss_tabname);
		continue;
	    }
	    else if (rdf_err == E_RD0011_NO_MORE_ROWS)
	    {
		/* found fewer than 20 tuples - process them */
		status = E_DB_OK;
	    }
	    else if (rdf_err == E_RD0013_NO_TUPLE_FOUND)
	    {
		/* found no tuples at all */
		status = E_DB_OK;
		continue;
	    }
	    else
	    {
		(VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb->rdf_error,
		    &err_blk);
		continue;
	    }
	}

	/* For each permit tuple */
	for (qp = rdf_cb->rdf_info_blk->rdr_ptuples->qrym_data,
	     tupcount = 0;
	     tupcount < rdf_cb->rdf_info_blk->rdr_ptuples->qrym_cnt;
	     qp = qp->dt_next, tupcount++
	    )
	{
	    /* Set protup pointing to current permit tuple */
	    protup = (DB_PROTECTION *) qp->dt_data;

	    /*
	    ** we only need to make sure that the present user, group, role, or
	    ** the public were given some permission
	    */
	    /*
	    ** we also need to check that the permit tuple is not a security
	    ** alarm, these are bogus permits and do not realy give a user
	    ** access to the table
	    */
	    if (!(protup->dbp_popset & DB_ALARM)
		&&
		(protup->dbp_gtype == DBGR_PUBLIC
		||
		protup->dbp_gtype == DBGR_USER &&
		!MEcmp((PTR) &protup->dbp_owner, (PTR) user, sizeof(*user))
		||
		protup->dbp_gtype == DBGR_GROUP &&
		!MEcmp((PTR) &protup->dbp_owner, (PTR) group, sizeof(*group))
		||
		protup->dbp_gtype == DBGR_APLID &&
		!MEcmp((PTR) &protup->dbp_owner, (PTR) applid, sizeof(*applid))
	       ))
	    {
		/*
		** user is allowed some form of access to this
		** object
		*/
		perm_found = TRUE;
		break;
	    }
	}	/* for all retrieved tules */
    }   /* while no errors and no interesting tuple was found */

    if (rdf_rb->rdr_rec_access_id != NULL)
    {
	/*
	** close RDF tuple stream, to release lock on
	** resource
	*/
	rdf_rb->rdr_update_op = RDR_CLOSE;
	temp_status = rdf_call(RDF_READTUPLES, (PTR) rdf_cb);
	if (DB_FAILURE_MACRO(temp_status))
	{
	    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb->rdf_error, &err_blk);
	    if (temp_status > status)
	    {
		status = temp_status;
	    }
	}
    }
    if (DB_FAILURE_MACRO(status))
    {
	outcome = ABNORMAL_ERROR;
    }
    else if (perm_found)
    {
	outcome = NAME_RESOLVED;
    }
    else
    {
	outcome = NAME_NOT_RESOLVED;
    }

wrapup:

    /* we need to clean up after ourselves and return gracefully */

    /* restore session language */
    if (sess_cb != (PSS_SESBLK *) NULL)
    {
	sess_cb->pss_lang = save_lang;

	temp_status = pst_clrrng(sess_cb, &sess_cb->pss_auxrng, &err_blk);
	if (DB_FAILURE_MACRO(temp_status) && temp_status > status)
	{
	    status = temp_status;
	    outcome = ABNORMAL_ERROR;
	}
    }
    
    switch (outcome)
    {
	case NAME_RESOLVED:
	{
	    MEfill(2 * obj_maxname, (u_char) ' ',
		(PTR) out->db_t_text);
	    out->db_t_count = 2 * obj_maxname;

	    MEcopy((PTR) &own_name, sizeof(own_name), (PTR) out->db_t_text);
	    MEcopy((PTR) &tab_name, sizeof(tab_name),
		(PTR) (out->db_t_text + obj_maxname));
	    return(E_DB_OK);
	}
	case NAME_NOT_RESOLVED:	
	{
	    out->db_t_count = (u_i2) 0;
	    return(E_DB_OK);
	}
	case BAD_PARAMETERS:
	case ABNORMAL_ERROR:
	default:
	{
	    return(status);
	}
    }
}
