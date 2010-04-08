/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <me.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSLRNGFCN.C - Functions for managing range variables
**
**  Description:
**      QUEL, unlike SQL, first looks for a table name owned by the current
**	user, then by the dba.  This file contains the functions that
**	implement the search path.
**
**          psl_rngent - Enter a range variable, looking first for a table
**			 owned by the current user, then for one owned by the
**			 dba, then for one owned by $ingres (system catalog).
**
**	    psl0_rngent - like above but doesn't report table not found errors.
**
**          psl_orngent - Enter a range variable, looking for a table owned by
**			  specified user.
**
**	    psl0_orngent - like above but doesn't report table not found errors.
**
**  	Static functions:
**	    psl_syn_id	- determine id of a synonym, given its name and owner
**
**  History:
**      18-may-86 (jeff)    
**          written
**	10-may-88 (stec)
**	    Added psl_orngent.
**	12-aug-88 (stec)
**	    Modified psl0_rngent to make sure the owner is the current user
**	    or the DBA or $ingres.
**	03-oct-88 (andre)
**	    modified psl_rngent to receive and pass query mode to psl0_rngent
**	12-jan-90 (andre)
**	    modiifed psl0_rngent() to not automatiacally check tables owned by
**	    user, then dba, then $ingres.  Instead user needs to specify whether
**	    tables owned by the user (PSS_USRTBL) and/or dba (PSS_DBATBL)
**	    and/or $ingres (PSS_INGTBL) are to be checked.
**	09-apr-90 (andre)
**	    psl_rngent(), psl_orngent(), psl0_rngent() and psl0_orngent() will
**	    notify the caller if the supplied name was that of a synonym.
**	    Additionally, psl[0]_rngent() will also indicate if the object or
**	    synonym retrieved is owned by the user, DBA, or $INGRES.
**	12-sep-90 (teresa)
**	    make the following booleans become bitflags in pss_ses_flag: 
**	    pss_fips_mode
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	21-apr-92 (barbara)
**	    Updated for Sybil.  Pass in mask to psl0_rngent for distributed
**	    thread.  Values will be mapped to RDF masks.
**	18-nov-92 (barbara)
**	    Changed interface to psl0_orngent to pass in Star-specific flags
**	    (same as psl0_rngent).
**	03-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use pss_cat_owner instead of "$ingres"
**      23-mar-93 (smc)
**          Fixed up prototyped function pointer declarations.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-aug-93 (andre)
**	    fixed causes of ACC warnings
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	21-oct-93 (andre)
**	    added psl_syn_id() and made changes to psl0_[o]rngent() to invoke it
**	    if a synonym reference has been encountered in the course of parsing
**	    a user-generated dbproc
**      02-mar-95 (harpa06)
**          Bug #66070 - Changed error returned to user from E_US0847_2119 to
**          E_US0845_2117 since this error message was confusing people when 
**          non-existent temporary tables were being referenced.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* static function declarations */
static DB_STATUS
psl_syn_id(
	PSS_SESBLK	*sess_cb,
	DB_OWN_NAME	*syn_owner,
	DB_TAB_NAME	*syn_name,
	DB_TAB_ID	*syn_id,
	DB_ERROR	*err_blk);

/*{
** Name: psl_rngent	- Enter a range variable
**
** Description:
**      This function enters a range variable into the range table, first
**	looking for declared global session temporary tables, then
**	looking for tables owned by the current user, then for tables owned
**	by the dba, then by $ingres. For compability reasons the session
**	temporary table search is only done if the gtt_syntax_shortcut
**	switch in config.dat is set to ON.
**
** Inputs:
**      rngtable                        Pointer to the range table
**	scope				scope that range variable can be
**					found in.
**	varname				Name of range variable
**	tabname				Name of table to look up
**	cb				session control block
**	    pss_user			Current user name
**	    pss_dba			Name of DBA
**	    pss_sessid			Session id
**	    pss_dbid			Database id
**	    pss_lang			query language
**	tabonly				TRUE means don't get info about columns
**	rngvar				Place to put pointer to range var
**	err_blk				Filled in if an error happens
**
** Outputs:
**	rngvar				Filled in with pointer to range variable
**	err_blk				Filled in if an error happened
**	caller_info			Filled in with info which may be of
**					interest to the caller
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_INFO			Success but session table found
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can cause RDF to allocate memory
**
** History:
**	18-may-86 (jeff)
**          written
**	4-feb-87 (daved)
**	    call psl0_rngent
**	18-feb-87 (daved)
**	    add scope for SQL.
**	03-oct-88 (andre)
**	    modified to receive and pass query mode to psl0_rngent 
**	12-jan-90 (andre)
**	    modiifed psl0_rngent() to not automatiacally check tables owned by
**	    user, then dba, then $ingres.  Instead user needs to specify whether
**	    tables owned by the user (PSS_USRTBL) and/or dba (PSS_DBATBL)
**	    and/or $ingres (PSS_INGTBL) are to be checked.
**	09-apr-90 (andre)
**	    Added a new field, caller_info, to pass info back to the caller
**	12-sep-90 (teresa)
**	    pss_fips_mode becomes bitflag in pss_ses_flag instead of being
**	    a boolean.
**	21-apr-92 (barbara)
**	    For Sybil, new interface to psl0_rngent requires a mask.
**	    Pass in default of 0.
**	12-aug-93 (andre)
**	    replace TABLES_TO_CHECK() with PSS_USRTBL|PSS_DBATBL|PSS_INGTBL.
**	    TABLES_TO_CHECK() was being used because long time ago we made an
**	    assumption that 3-tier name space will not be used when processing 
**	    SQL queries while running in FIPS mode.  That assumption proved to 
**	    be incorrect and there is no longer a need to use this macro
**	23-may-06 (toumi01)
**	    Allow session temporary tables to be referenced without the
**	    "session." in DML. This eliminates the need for this Ingres-
**	    specific extension, which is a hinderance to app portability.
**	24-aug-06 (toumi01)
**	    Rewrite "session." optional change to clean up from Karl's
**	    changes, to allow perm and temp tables to have the same name, to
**	    make the feature conditional on a config.dat parameter to handle
**	    backward compatibility issues, and to minimize extra lookups.
**	31-aug-06 (toumi01)
**	    GTT syntax shortcut is now implicitly turned on when a dgtt
*	    command is parsed without "session." specified for the name.
**	    Thus, "old style" and "new style" applications can coexist
**	    in one installation with no need for a config.dat option.
*/
DB_STATUS
psl_rngent(
	PSS_USRRANGE       *rngtable,
	i4		    scope,
	char		   *varname,
	DB_TAB_NAME	   *tabname,
	PSS_SESBLK	   *cb,
	bool		   tabonly,
	PSS_RNGTAB	   **rngvar,
	i4		   query_mode,
	DB_ERROR	   *err_blk,
	i4                *caller_info)
{
    DB_STATUS           status;
    i4		err_code;
    i4		tbls_to_lookup = (PSS_USRTBL | PSS_DBATBL | PSS_INGTBL);

    /* if SQL and
    **    not a dbproc variable and
    **    PSS_GTT_SYNTAX_SHORTCUT is ON for the session
    ** search for session tables as well as user / dba / ingres
    */
    if (cb->pss_lang == DB_SQL &&
	(cb->pss_dbp_flags & PSS_SET_INPUT_PARAM) == 0 &&
	cb->pss_ses_flag & PSS_GTT_SYNTAX_SHORTCUT)
	tbls_to_lookup |= PSS_SESTBL;

    status = psl0_rngent(rngtable, scope, varname, tabname, cb, tabonly, rngvar,
	query_mode, err_blk, tbls_to_lookup,
	caller_info, 0);

    /*
    ** If table wasn't found, report it to user here.  Couldn't
    ** report it in pst_rgent because that function doesn't know
    ** if it's being used for SQL or QUEL.
    */
    if (DB_SUCCESS_MACRO(status) && !*rngvar)
    {
	(VOID) psf_error(2117L, 0L, PSF_USERERR, &err_code, err_blk, 1,
	    psf_trmwhite(sizeof(DB_TAB_NAME), (char *) tabname), tabname);
	status = E_DB_ERROR;
    }
    return (status);
}

/*{
** Name: psl0_rngent	- Enter a range variable
**
** Description:
**      This function enters a range variable into the range table, first
**	looking for tables owned by the current user, then for tables owned
**	by the dba, then by $ingres.
**
** Inputs:
**      rngtable                        Pointer to the range table
**	scope				scope that range variable can be
**					found in.
**	varname				Name of range variable
**	tabname				Name of table to look up
**	cb				session control block
**	    pss_user			Current user name
**	    pss_dba			Name of DBA
**	    pss_sessid			Session id
**	    pss_dbid			Database id
**	    pss_lang			query language
**	tabonly				TRUE means don't get info about columns
**	rngvar				Place to put pointer to range var
**	err_blk				Filled in if an error happens
**	tbls_to_lookup			mask containing indication of whose
**					tables are to be looked for
**	    PSS_USRTBL			look for tables owned by the user
**	    PSS_DBATBL			look for tables owned by the DBA
**	    PSS_INGTBL			look for tables owned by $INGRES
**	lookup_mask			mask to be mapped to RDF mask to save
**					on RDF's queries to CDB catalogs
**
** Outputs:
**	rngvar				Filled in with pointer to range variable
**	    pss_var_mask
**		PSS_REFERENCED_THRU_SYN_IN_DBP
**					object was referenced through a synonym
**					inside a user-defined dbproc
**	err_blk				Filled in if an error happened
**	caller_info			Filled in with info which may be of
**					interest to the caller
**	    PSS_BY_SYNONYM		name supplied by the caller was that of
**					a synonym
**	    PSS_USR_OBJ			info was retrieved on an object or
**					synonym owned by the current user
**	    PSS_DBA_OBJ			info was retrieved on an object or
**					synonym owned by the DBA
**	    PSS_SYS_OBJ			info was retrieved on an object or
**					synonym owned by system ($INGRES)
**		
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_INFO			Success but session table found when
**					searching for session tables and
**					any of ( user | dba | ingres ) tables
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can cause RDF to allocate memory
**
** History:
**	4-feb-1987 (daved)
**          written
**	18-feb-87 (daved)
**	    add scope for SQL
**	12-jan-90 (andre)
**	    let user specify whether tables owned by the user and/or dba and/or
**	    $ingres are to be looked for.
**	11-apr-90 (andre)
**	    Added a new field, caller_info, to pass info back to the caller
**	    it will be used to indicate if the name
**	    supplied by the user was that of a synonym + indicate if the
**	    object or synonym whose name was specified by the caller was owned
**	    by the user, DBA, or $INGRES
**	26-jun-90 (andre)
**	    if a synonym is encountered in a QUEL query, we will act as if it
**	    was never seen unless we are checking for duplicate object names in
**	    CREATE/DEFINE VIEW/RETRIEVE INTO.
**	21-apr-92 (barbara)
**	    For Sybil, pass in mask of values to be mapped into RDF values.
**	    Also, pass in session control block to pst_sent and pst_rgent
**	    and eliminate session and user id parameters.
**	11-may-92 (andre)
**	    if a table could not be looked up in the course of parsing a dbproc
**	    definition, set PSS_MISSING_OBJ in cb->pss_dbp_flags
**	18-nov-92 (barbara)
**	    Reset err_code before looking up table; otherwise old error code
**	    stays around and may be used erroneously by caller.
**	21-oct-93 (andre)
**	    If the name specified by the caller was that of a synonym and we 
**	    are parsing a definition of a non-system generated dbproc, we will 
**	    invoke psl_syn_id() to determine id of the synonym - this id will 
**	    be stored in the dbproc's independent object list so that whenever 
**	    the synonym is dropped, the dbproc will be marked dormant
**	11-feb-94 (andre)
**	    fix for bug 59707:
**	    when checking whether the object was referenced through a synonym, 
**	    we must avoid mistaking a reference to the input set of a set-input
**	    dbproc for a reference to a synonym; we can detect an input set 
**	    reference by checking whether pss_rgtype is PST_SETINPUT
**	08-apr-94 (andre)
**	    the code in this function was making an (incorrect) assumption that
**	    caller would not ask us to look for $ingres' table without asking us
**	    to look for a table owned by the current use or by the DBA.  
**	    This assumption is violated when in the course of processing 
**	    CREATE TABLE we attempt to verify that a table with a given name 
**	    is not already owned by the current user or by $ingres 
**	    (WITHOUT CHECKING WHETHER IT IS OWNED BY THE DBA.)  As a result, 
**	    an attempt by to create a table iirelation by $ingres on a 
**	    database of which $ingres is a DBA (e.g. iidbdb) does get caught 
**	    in PSF and is instead caught in DMF.  
**	   
**	    To correct the problem, we will skip checking for existence of a 
**	    table owned by DBA if the current user is the DBA AND we have 
**	    already checked for existence of the table owned by the current 
**	    user.  Similarly, we will skip checking for existence of the table 
**	    owned by $ingres if the current user is $ingres AND we checked for 
**	    existence of the table owned by the current user or the DBA is $
**	    ingres AND we checked for existence of the table owned by the DBA
**	30-apr-02 (inkdo01)
**	    Added support for PSS_SESTBL flag to request access to SQL-generated
**	    global temporary tables.
**	24-aug-06 (toumi01)
**	    For "session." optional change to handle getting all four search
**	    flags set in one go, and return E_DB_INFO if we end up finding
**	    a session temporary table.
**	31-aug-06 (toumi01)
**	    For SIR 116264 fix E_DB_WARN s.b. E_DB_INFO typo.
**	26-sep-06 (toumi01)
**	    For GTT lookup changes fix init of err_code for USRTBL search.
**	29-dec-2006 (dougi)
**	    Call to new function pst_swelem() to potentially resolve table 
**	    references against with list elements.
*/
DB_STATUS
psl0_rngent(
	PSS_USRRANGE    *rngtable,
	i4		scope,
	char		*varname,
	DB_TAB_NAME	*tabname,
	PSS_SESBLK	*cb,
	bool		tabonly,
	PSS_RNGTAB	**rngvar,
	i4		query_mode,
	DB_ERROR	*err_blk,
	i4		tbls_to_lookup,
	i4		*caller_info,
	i4		lookup_mask)
{
    DB_STATUS           status = E_DB_ERROR;
    i4		err_code;
    i4		fishing = (PSS_SESTBL & tbls_to_lookup) &&
			(PSS_USRTBL|PSS_DBATBL|PSS_INGTBL) & tbls_to_lookup;
    DB_TAB_OWN		*owner;
    bool		withelem = FALSE;
    DB_STATUS		(*rng_ent)(PSS_SESBLK         *sess_cb,
				   PSS_USRRANGE       *rngtable,
				   i4                  scope,
				   char               *varname,
				   i4                 showtype,
				   DB_TAB_NAME        *tabname,
				   DB_TAB_OWN         *tabown,
				   DB_TAB_ID          *tabid,
				   bool               tabonly,
				   PSS_RNGTAB         **rngvar,
				   i4                 query_mode,
				   DB_ERROR           *err_blk);

    rng_ent = (cb->pss_lang == DB_QUEL) ? pst_rgent : pst_sent;

    if (cb->pss_ses_flag & PSS_WITHELEM_INQ)
    {
	/* There is a with clause in this query. First try to find table 
	** amongst with list elements. */
	*caller_info = PSS_WITH_ELEM;
	status = pst_swelem(cb, scope, rngtable, tabname, rngvar, err_blk);

	if (status != E_DB_OK)
	{
	    status = E_DB_ERROR;
	    err_blk->err_code = E_PS0903_TAB_NOTFOUND;
	}
	else withelem = TRUE;
    }

    /* if caller requested that SQL "session." temp tables be looked at */
    if (status != E_DB_OK && tbls_to_lookup & PSS_SESTBL)
    {
	/* First try it as a session GTT */
	*caller_info = PSS_USR_OBJ;

	status = (*rng_ent)(cb, rngtable, scope, varname,
	    lookup_mask | PST_SHWNAME, tabname,
	    (owner = (DB_TAB_OWN *) &cb->pss_sess_owner), (DB_TAB_ID *) NULL,
	    tabonly, rngvar, query_mode, err_blk);

	/*
	** if a synonym is encountered in a QUEL query, we will act as if it
	** was never seen unless we are checking for duplicate object names in
	** CREATE/DEFINE VIEW/RETRIEVE INTO.
	*/
	if (   DB_SUCCESS_MACRO(status)
	    && cb->pss_lang == DB_QUEL
	    && (   *rngvar != &rngtable->pss_rsrng ||
		   query_mode != PSQ_RETINTO
		&& query_mode != PSQ_VIEW
		&& query_mode != PSQ_CREATE
	       )
	    && (MEcmp((PTR) &owner->db_tab_own, (PTR) &(*rngvar)->pss_ownname,
		  sizeof(DB_TAB_OWN))
	        ||
	        MEcmp((PTR) tabname, (PTR) &(*rngvar)->pss_tabname, 
		  sizeof(DB_TAB_NAME))
	       )
	   )
	{
	    (*rngvar)->pss_used = FALSE;

	    if (*rngvar != &rngtable->pss_rsrng)
	    {
		/* Put the range variable at the head of the queue */
		QUinsert(QUremove((QUEUE *) *rngvar), 
		    rngtable->pss_qhead.q_prev);
	    }
	    
	    /*
	    ** make it look as if we looked for user's table, but it didn't
	    ** exist
	    */
	    status = E_DB_ERROR;
	    err_blk->err_code = E_PS0903_TAB_NOTFOUND;
	}
	if (status == E_DB_OK && fishing)
	{
	    /*
	    ** tell callers we found a session table if they were looking for
	    ** temporary session tables AND any sort of permanent tables
	    */
	    status = E_DB_INFO;
	}
    }
    else if (status != E_DB_OK)
    {
	/* make it look as if we looked for GTT, but it didn't exist */
	status = E_DB_ERROR;
	err_blk->err_code = E_PS0903_TAB_NOTFOUND;
    }

    /*
    ** if
    **	    - not found and
    **	    - caller requested that user tables be looked at
    ** look at user tables
    */
    if (status != E_DB_OK && status != E_DB_INFO
	&& err_blk->err_code == E_PS0903_TAB_NOTFOUND
	&& tbls_to_lookup & PSS_USRTBL)
    {
	/* First try it by user */
	*caller_info = PSS_USR_OBJ;

	err_blk->err_code = 0;
	status = (*rng_ent)(cb, rngtable, scope, varname,
	    lookup_mask | PST_SHWNAME, tabname,
	    (owner = (DB_TAB_OWN *) &cb->pss_user), (DB_TAB_ID *) NULL,
	    tabonly, rngvar, query_mode, err_blk);

	/*
	** if a synonym is encountered in a QUEL query, we will act as if it
	** was never seen unless we are checking for duplicate object names in
	** CREATE/DEFINE VIEW/RETRIEVE INTO.
	*/
	if (   DB_SUCCESS_MACRO(status)
	    && cb->pss_lang == DB_QUEL
	    && (   *rngvar != &rngtable->pss_rsrng ||
		   query_mode != PSQ_RETINTO
		&& query_mode != PSQ_VIEW
		&& query_mode != PSQ_CREATE
	       )
	    && (MEcmp((PTR) &owner->db_tab_own, (PTR) &(*rngvar)->pss_ownname,
		  sizeof(DB_TAB_OWN))
	        ||
	        MEcmp((PTR) tabname, (PTR) &(*rngvar)->pss_tabname, 
		  sizeof(DB_TAB_NAME))
	       )
	   )
	{
	    (*rngvar)->pss_used = FALSE;

	    if (*rngvar != &rngtable->pss_rsrng)
	    {
		/* Put the range variable at the head of the queue */
		QUinsert(QUremove((QUEUE *) *rngvar), 
		    rngtable->pss_qhead.q_prev);
	    }
	    
	    /*
	    ** make it look as if we looked for user's table, but it didn't
	    ** exist
	    */
	    status = E_DB_ERROR;
	    err_blk->err_code = E_PS0903_TAB_NOTFOUND;
	}
    }

    /*
    ** if
    **	    - not found and
    **	    - caller requested that DBA's tables be looked at and
    **	    - we have not checked for existence of table owned by the current 
    **	      user OR the current user is not the dba,
    ** look at dba's tables
    */
    if (status != E_DB_OK && status != E_DB_INFO
	&& err_blk->err_code == E_PS0903_TAB_NOTFOUND
	&& tbls_to_lookup & PSS_DBATBL
	&& (   ~tbls_to_lookup & PSS_USRTBL
	    || MEcmp((PTR) &cb->pss_user, (PTR) &cb->pss_dba.db_tab_own, 
	       sizeof(DB_OWN_NAME))))
    {
	*caller_info = PSS_DBA_OBJ;

	err_blk->err_code = 0;
	status = (*rng_ent)(cb, rngtable, scope, varname,
	    lookup_mask | PST_SHWNAME, tabname, (owner = &cb->pss_dba),
	    (DB_TAB_ID *) NULL, tabonly, rngvar, query_mode, err_blk);

	/*
	** if a synonym is encountered in a QUEL query, we will act as if it
	** was never seen unless we are checking for duplicate object names in
	** CREATE/DEFINE VIEW/RETRIEVE INTO.
	*/
	if (   DB_SUCCESS_MACRO(status)
	    && cb->pss_lang == DB_QUEL
	    && (   *rngvar != &rngtable->pss_rsrng ||
		   query_mode != PSQ_RETINTO
		&& query_mode != PSQ_VIEW
		&& query_mode != PSQ_CREATE
	       )
	    && (MEcmp((PTR) &owner->db_tab_own, (PTR) &(*rngvar)->pss_ownname,
		  sizeof(DB_TAB_OWN))
	        ||
	        MEcmp((PTR) tabname, (PTR) &(*rngvar)->pss_tabname, 
		  sizeof(DB_TAB_NAME))
	       )
	   )
	{
	    (*rngvar)->pss_used = FALSE;

	    if (*rngvar != &rngtable->pss_rsrng)
	    {
		/* Put the range variable at the head of the queue */
		QUinsert(QUremove((QUEUE *) *rngvar), 
		    rngtable->pss_qhead.q_prev);
	    }
	    
	    /*
	    ** make it look as if we looked for DBA's table, but it didn't
	    ** exist
	    */
	    status = E_DB_ERROR;
	    err_blk->err_code = E_PS0903_TAB_NOTFOUND;
	}
    }

    /*
    ** if
    **	    - still not found and
    **	    - caller requested that $INGRES' tables be looked at and
    **	    - we have not checked for existence of table owned by the current 
    **	      user OR the current user is not $ingres and
    **	    - we have not checked for existence of table owned by the DBA 
    **	      OR the DBA is not $ingres,
    ** look at tables owned by $ingres
    */
    if (status != E_DB_OK && status != E_DB_INFO
	&& err_blk->err_code == E_PS0903_TAB_NOTFOUND
	&& tbls_to_lookup & PSS_INGTBL)
    {
	if (   (   ~tbls_to_lookup & PSS_USRTBL
		|| MEcmp((PTR) &cb->pss_user, (PTR) cb->pss_cat_owner, 
	               sizeof(DB_OWN_NAME)))
	    && (   ~tbls_to_lookup & PSS_DBATBL
		|| MEcmp((PTR) &cb->pss_dba.db_tab_own, (PTR) cb->pss_cat_owner,
		       sizeof(DB_OWN_NAME))))
	{
	    *caller_info = PSS_SYS_OBJ;
	    
	    err_blk->err_code = 0;
	    status = (*rng_ent)(cb, rngtable, scope, varname,
		lookup_mask | PST_SHWNAME, tabname, 
		(owner = (DB_TAB_OWN *) cb->pss_cat_owner),
		(DB_TAB_ID *) NULL, tabonly, rngvar, query_mode, err_blk);

	    /*
	    ** if a synonym is encountered in a QUEL query, we will act as if it
	    ** was never seen unless we are checking for duplicate object names
	    ** in CREATE/DEFINE VIEW/RETRIEVE INTO.
	    */
	    if (   DB_SUCCESS_MACRO(status)
		&& cb->pss_lang == DB_QUEL
		&& (   *rngvar != &rngtable->pss_rsrng ||
		       query_mode != PSQ_RETINTO
		    && query_mode != PSQ_VIEW
		    && query_mode != PSQ_CREATE
		   )
		&& (    MEcmp((PTR) &owner->db_tab_own, 
			    (PTR) &(*rngvar)->pss_ownname,
		            sizeof(DB_TAB_OWN))
		    || MEcmp((PTR) tabname, (PTR) &(*rngvar)->pss_tabname, 
			   sizeof(DB_TAB_NAME))
		   )
	       )
	    {
		(*rngvar)->pss_used = FALSE;

		if (*rngvar != &rngtable->pss_rsrng)
		{
		    /* Put the range variable at the head of the queue */
		    QUinsert(QUremove((QUEUE *) *rngvar), 
			rngtable->pss_qhead.q_prev);
		}
		
		/*
		** make it look as if we looked for $ingres' table, but it
		** didn't exist
		*/
		status = E_DB_ERROR;
		err_blk->err_code = E_PS0903_TAB_NOTFOUND;
	    }
	}
    }

    /*
    ** If table wasn't found, clear the range table ptr. Return OK.
    */
    if (DB_SUCCESS_MACRO(status))
    {
	/*
	** let caller know if the name supplied by him was resolved to a synonym
	**
	** we have to be careful to not mistake a reference to the input set of 
	** a set-input dbproc for a reference to the synonym
	*/
	if (   (*rngvar)->pss_rgtype != PST_SETINPUT
	    && !withelem
	    && (   MEcmp((PTR) &owner->db_tab_own, 
		       (PTR) &(*rngvar)->pss_ownname, sizeof(DB_TAB_OWN))
	        || MEcmp((PTR) tabname, (PTR) &(*rngvar)->pss_tabname, 
		       sizeof(DB_TAB_NAME)))
	   )
	{
	    *caller_info |= PSS_BY_SYNONYM;

	    /*
	    ** if a synonym was referenced inside a user-defined dbproc, 
	    ** determine synonym's id so that later on we can add a tuple to 
	    ** IIDBDEPENDS describing dependence of a dbproc on this synonym
	    */
	    if (   cb->pss_dbp_flags & PSS_DBPROC
		&& ~cb->pss_dbp_flags & PSS_SYSTEM_GENERATED)
	    {
		(*rngvar)->pss_var_mask |= PSS_REFERENCED_THRU_SYN_IN_DBP;
		status = psl_syn_id(cb, &owner->db_tab_own, tabname, 
		    &(*rngvar)->pss_syn_id, err_blk);
		
		if (status == E_DB_OK && (*rngvar)->pss_syn_id.db_tab_base == 0)
		{
		    /* 
		    ** synonym was not found - this may happen if the RDF
		    ** cache entry is stale; set status and err_blk->err_code
		    ** as they would be set if the entry was not found by 
		    ** pst_sent() or pst_rgent()
		    */
		    status = E_DB_ERROR;
		    err_blk->err_code = E_PS0903_TAB_NOTFOUND;
		}
	    }
	}
    }

    if (DB_FAILURE_MACRO(status) && err_blk->err_code == E_PS0903_TAB_NOTFOUND)
    {
	/*
	** if parsing a dbproc definition, remember that some object referenced
	** in the definition could not be found
	*/
	if (cb->pss_dbp_flags & PSS_DBPROC)
	    cb->pss_dbp_flags |= PSS_MISSING_OBJ;

	*rngvar = NULL;
	status = E_DB_OK;
    }
    return (status);
}

/*{
** Name: psl_drngent	- Enter a range variable for a derived table
**
** Description:
**      This function enters a range variable into the range table for a
**	derived table (subselect in the FROM clause). The appropriate RDF 
**	table descriptors are fabricated and attribute info is filled in
**	from the parse tree.
**
** Inputs:
**      rngtable                        Pointer to the range table
**	scope				scope that range variable can be
**					found in.
**	varname				Name of range variable
**	cb				session control block
**	    pss_user			Current user name
**	    pss_dba			Name of DBA
**	    pss_sessid			Session id
**	    pss_dbid			Database id
**	    pss_lang			query language
**	rngvar				Place to put pointer to range var
**	root				Ptr to parse tree of derived table
**	type				Type of entry (derived table or 
**					with clause element)
**	err_blk				Filled in if an error happens
**
** Outputs:
**	rngvar				Filled in with pointer to range variable
**	err_blk				Filled in if an error happened
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Will result in allocation of persistent memory
**
** History:
**	24-jan-06 (dougi)
**	    Written for subselects in FROM clause.
**	28-dec-2006 (dougi)
**	    Added type parm to support with clauses, too.
*/
DB_STATUS
psl_drngent(
	PSS_USRRANGE       *rngtable,
	i4		    scope,
	char		   *varname,
	PSS_SESBLK	   *cb,
	PSS_RNGTAB	   **rngvar,
	PST_QNODE	   *root,
	i4		    type,
	DB_ERROR	   *err_blk)
{
    DB_STATUS           status;
    i4		err_code;

    /* The work is actually performed by a pst function - so send it all on. */
    status = pst_sdent(rngtable, scope, varname, cb, rngvar, 
						root, type, err_blk);

    return (status);
}

/*{
** Name: psl_tprngent	- Enter a range variable for a table procedure
**
** Description:
**      This function enters a range variable into the range table for a
**	table procedure (row producing procedure invocation in the FROM
**	clause). RDF "table" descriptors are fabricated and attribute info 
**	is filled in from the result column descriptors in the iiprocedure_
**	parameter catalog.
**
** Inputs:
**      rngtable                        Pointer to the range table
**	scope				scope that range variable can be
**					found in.
**	varname				Name of range variable
**	dbp				Ptr to PSF procedure descriptor
**	cb				session control block
**	    pss_user			Current user name
**	    pss_dba			Name of DBA
**	    pss_sessid			Session id
**	    pss_dbid			Database id
**	    pss_lang			query language
**	rngvar				Place to put pointer to range var
**	root				Ptr to parse tree containing RESDOM
**					list of parameter specifications
**	err_blk				Filled in if an error happens
**
** Outputs:
**	rngvar				Filled in with pointer to range variable
**	err_blk				Filled in if an error happened
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Will result in allocation of persistent memory
**
** History:
**	3-april-2008 (dougi)
**	    Written for table procedures in FROM clause.
**	29-march-2009 (dougi) bug 121651
**	    Trap "no proc found" error.
*/
DB_STATUS
psl_tprngent(
	PSS_USRRANGE       *rngtable,
	i4		    scope,
	char		   *varname,
	i4		    dbpid,
	PSS_SESBLK	   *cb,
	PSS_RNGTAB	   **rngvar,
	PST_QNODE	   *root,
	DB_ERROR	   *err_blk)
{
    DB_STATUS           status;
    i4		err_code;

    /* The work is actually performed by a pst function - so send it all on. */
    status = pst_stproc(rngtable, scope, varname, dbpid, cb, 
						rngvar, root, err_blk);
    if (!*rngvar)
    {
	(VOID) psf_error(2405L, 0L, PSF_USERERR, &err_code, err_blk, 1,
	    psf_trmwhite(sizeof(DB_TAB_NAME), (char *) varname), varname);
	return (E_DB_ERROR);
    }

    return (status);
}

/*{
** Name: psl_orngent	- Enter a range variable for a table owned by
**			  the specified user.
**
** Description:
**      This function enters a range variable into the range table, 
**	looking for a table owned by the specified user.
**
** Inputs:
**      rngtable                        Pointer to the range table
**	scope				scope that range variable can be
**					found in.
**	varname				Name of range variable
**	ownname				Name of table owner.
**	tabname				Name of table to look up
**	cb				session control block
**	    pss_user			Current user name
**	    pss_dba			Name of DBA
**	    pss_sessid			Session id
**	    pss_dbid			Database id
**	    pss_lang			query language
**	tabonly				TRUE means don't get info about columns
**	rngvar				Place to put pointer to range var
**	query_mode			query mode
**	err_blk				Filled in if an error happens
**
** Outputs:
**	rngvar				Filled in with pointer to range variable
**	err_blk				Filled in if an error happened
**	caller_info			Filled in with info which may be of
**					interest to the caller
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can cause RDF to allocate memory.
**
** History:
**	10-may-88 (stec)
**          written.
**	03-oct-88 (andre)
**	    modified to receive and pass query mode to psl0_orngent.
**	09-apr-90 (andre)
**	    Added a new field, caller_info, to pass info back to the caller
**	    In particular, user will be notified if the name specified by the
**	    user was that of a synonym.
**	18-nov-92 (barbara)
**	    Made call to psl0_orngent consistent with new interface.
**	02-mar-1995 (harpa06)
**          Bug #66070 - Changed error returned to user from E_US0847_2119 to
**	    E_US0845_2117 since this error message was confusing people when 
**	    non-existent temporary tables were being referenced.
*/
DB_STATUS
psl_orngent(
	PSS_USRRANGE    *rngtable,
	i4		scope,
	char		*varname,
	DB_OWN_NAME	*ownname,
	DB_TAB_NAME	*tabname,
	PSS_SESBLK	*cb,
	bool		tabonly,
	PSS_RNGTAB	**rngvar,
	i4		query_mode,
	DB_ERROR	*err_blk,
	i4             *caller_info)
{
    DB_STATUS           status;
    i4		err_code;

    status = psl0_orngent(rngtable, scope, varname, ownname,
	    tabname, cb, tabonly, rngvar, query_mode, err_blk, caller_info, 0);
    if (status != E_DB_OK)
	return (status);
    /*
    ** If table wasn't found, report it to user here.  Couldn't
    ** report it in pst_rgent because that function doesn't know
    ** if it's being used for SQL or QUEL.
    */
    if (!*rngvar)
    {
	(VOID) psf_error(2117L, 0L, PSF_USERERR, &err_code, err_blk, 1,
	    psf_trmwhite(sizeof(DB_TAB_NAME), (char *) tabname), tabname);
	return (E_DB_ERROR);
    }
    return (status);
}

/*{
** Name: psl0_orngent	- Enter a range variable
**
** Description:
**      This function enters a range variable into the range table, 
**	looking for tables owned by the specified user. The user should
**	be the current user, or the DBA. This requirement is enforced to
**	make other users' tables invisible, which is compatible with the 
**	existing system.
**
** Inputs:
**      rngtable                        Pointer to the range table
**	scope				scope that range variable can be
**					found in.
**	varname				Name of range variable
**	ownname				Name of table owner.
**	tabname				Name of table to look up
**	cb				session control block
**	    pss_user			Current user name
**	    pss_dba			Name of DBA
**	    pss_sessid			Session id
**	    pss_dbid			Database id
**	    pss_lang			query language
**	tabonly				TRUE means don't get info about columns
**	rngvar				Place to put pointer to range var
**	query_mode			Query mode
**	err_blk				Filled in if an error happens
**
** Outputs:
**	rngvar				Filled in with pointer to range variable
**	    pss_var_mask		all sorts of useful info
**		PSS_EXPLICIT_QUAL	object name was explicitly qualified
**					with the owner name (this bit will get
**					set whenever we successfully obtain an
**					object description)
**		PSS_REFERENCED_THRU_SYN_IN_DBP
**					object was referenced through a synonym
**					inside a user-defined dbproc
**	err_blk				Filled in if an error happened
**	caller_info			Filled in with info which may be of
**					interest to the caller
**	    PSS_BY_SYNONYM		name supplied by the caller was that of
**					a synonym
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can cause RDF to allocate memory.
**
** History:
**	10-may-88 (stec)
**          written.
**	12-aug-88 (stec)
**	    Make sure the owner is the current user or the DBA
**	    or $ingres.
**	10-mar-89 (andre)
**	    DBA may search for anyone's table providing he cb->pss_dba_drop_all
**	    is set.
**	22-may-89 (andre)
**	    Further modify the above change: DBA or $ingres may search for
**	    a table not in their name space providing query mode is PSQ_DESTROY
**	    and dba_drop_all is set.
**	01-nov-89 (andre)
**	    As a part of work to bring name space up to FIPS requirements, we
**	    will no longer insist that owner of the table is present user, DBA,
**	    or $INGRES.  As a result, we will no longer be claiming that table
**	    was not found without looking for it: anyone can access anyone's
**	    table.  For DROP user.tbl, drop_obj_spec: production has code to
**	    make sure that if the table i not owned by the user, than
**	    dba_drop-all must be set, and user must be DBA or $INGRES.
**	09-apr-90 (andre)
**	    Added a new field, caller_info, to pass info back to the caller
**	    it will be used to indicate if the name supplied by the user was
**	    that of a synonym.
**	    Also set (*rngvar)->pss_var_mask to PSS_EXPLICIT_QUAL to indicate
**	    that object name was explicitly qualified by the name of its owner.
**	21-apr-92 (barbara)
**	    Pass session control block to pst_sent/pst_rgent and eliminate
**	    session and user id parameters.
**	11-may-92 (andre)
**	    if a table could not be looked up in the course of parsing a dbproc
**	    definition, set PSS_MISSING_OBJ in cb->pss_dbp_flags
**	18-nov-92 (barbara)
**	    Changed interface to psl0_orngent to pass in Star-specific flags
**	    (same as psl0_rngent).
**	21-oct-93 (andre)
**	    If the name specified by the caller was that of a synonym and we 
**	    are parsing a definition of a non-system generated dbproc, we will 
**	    invoke psl_syn_id() to determine id of the synonym - this id will 
**	    be stored in the dbproc's independent object list so that whenever 
**	    the synonym is dropped, the dbproc will be marked dormant
*/
DB_STATUS
psl0_orngent(
	PSS_USRRANGE    *rngtable,
	i4		scope,
	char		*varname,
	DB_OWN_NAME	*ownname,
	DB_TAB_NAME	*tabname,
	PSS_SESBLK	*cb,
	bool		tabonly,
	PSS_RNGTAB	**rngvar,
	i4		query_mode,
	DB_ERROR	*err_blk,
	i4             *caller_info,
	i4		lookup_mask)
{
    DB_STATUS           status;
    DB_STATUS		(*rng_ent)(PSS_SESBLK         *sess_cb,
				   PSS_USRRANGE       *rngtable,
				   i4                  scope,
				   char               *varname,
				   i4                 showtype,
				   DB_TAB_NAME        *tabname,
				   DB_TAB_OWN         *tabown,
				   DB_TAB_ID          *tabid,
				   bool               tabonly,
				   PSS_RNGTAB         **rngvar,
				   i4                 query_mode,
				   DB_ERROR           *err_blk);


    *caller_info = 0;

    if (cb->pss_lang == DB_QUEL)
	rng_ent = pst_rgent;
    else
	rng_ent = pst_sent;

    status = (*rng_ent)(cb, rngtable, scope, varname, lookup_mask|PST_SHWNAME,
			tabname, (DB_TAB_OWN *)ownname, (DB_TAB_ID *) NULL, 
			tabonly, rngvar, query_mode, err_blk);
    if (status == E_DB_OK)
    {
	/*
	** let caller know if the name supplied by him was resolved to a synonym
	*/
	if (   MEcmp((PTR) ownname, (PTR) &(*rngvar)->pss_ownname, 
		   sizeof(DB_OWN_NAME))
	    || MEcmp((PTR) tabname, (PTR) &(*rngvar)->pss_tabname, 
		   sizeof(DB_TAB_NAME)))
	{
	    *caller_info |= PSS_BY_SYNONYM;

	    /*
	    ** if a synonym was referenced inside a user-defined dbproc, 
	    ** determine synonym's id so that later on we can add a tuple to 
	    ** IIDBDEPENDS describing dependence of a dbproc on this synonym
	    */
	    if (   cb->pss_dbp_flags & PSS_DBPROC
		&& ~cb->pss_dbp_flags & PSS_SYSTEM_GENERATED)
	    {
		(*rngvar)->pss_var_mask |= PSS_REFERENCED_THRU_SYN_IN_DBP;
		status = psl_syn_id(cb, ownname, tabname, 
		    &(*rngvar)->pss_syn_id, err_blk);
		
		if (status == E_DB_OK && (*rngvar)->pss_syn_id.db_tab_base == 0)
		{
		    /* 
		    ** synonym was not found - this may happen if the RDF
		    ** cache entry is stale; set status and err_blk->err_code
		    ** as they would be set if the entry was not found by 
		    ** pst_sent() or pst_rgent()
		    */
		    status = E_DB_ERROR;
		    err_blk->err_code = E_PS0903_TAB_NOTFOUND;
		}
	    }
	}
	(*rngvar)->pss_var_mask |= PSS_EXPLICIT_QUAL;
    }

    /*
    ** If table wasn't found, clear the range table ptr. Return OK.
    */
    if (DB_FAILURE_MACRO(status) && err_blk->err_code == E_PS0903_TAB_NOTFOUND)
    {
	/*
	** if parsing a dbproc definition, remember that some object referenced
	** in the definition could not be found
	*/
	if (cb->pss_dbp_flags & PSS_DBPROC)
	    cb->pss_dbp_flags |= PSS_MISSING_OBJ;

	*rngvar = NULL;
	status = E_DB_OK;
    }

    return (status);
}

/*
** Name:	psl_syn_id - determine id of a synonym
**
** Description:
**	Given name and owner of a synonym, invoke rdf_call() to determine its 
**	id
**
** Input:
**	sess_cb			PSF session cb
**	syn_owner		name of synonym's owner
**	syn_name		name of the synonym
**
** Output:
**	syn_id			id of the synonym; will be zeroed out if one 
**				was not found 
**	err_blk			filled in if an error is encountered
**
** Returns:
**	E_DB_{OK,ERROR}
**
** History:
**	21-oct-93 (andre)
**	    written
*/
static DB_STATUS
psl_syn_id(
	PSS_SESBLK	*sess_cb,
	DB_OWN_NAME	*syn_owner,
	DB_TAB_NAME	*syn_name,
	DB_TAB_ID	*syn_id,
	DB_ERROR	*err_blk)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    DB_IISYNONYM	syn_tuple;
    RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;

    pst_rdfcb_init(&rdf_cb, sess_cb);

    STRUCT_ASSIGN_MACRO((*syn_owner), rdf_rb->rdr_owner);
    MEcopy((PTR) syn_name, sizeof(DB_TAB_NAME), 
	(PTR) &rdf_rb->rdr_name.rdr_synname);

    rdf_rb->rdr_2types_mask |= RDR2_SYNONYM;

    /* 
    ** there should be no more than one synonym for a given (owner, name) pair 
    */
    rdf_rb->rdr_qtuple_count = 1;

    rdf_rb->rdr_update_op = RDR_OPEN;
    rdf_rb->rdr_qrytuple  = (PTR) &syn_tuple;

    status = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);
    if (status != E_DB_OK)
    {
	if (   rdf_cb.rdf_error.err_code == E_RD0011_NO_MORE_ROWS
	    || rdf_cb.rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND)
	{
	    /*
	    ** zero out *syn_id to let caller know that the synonym was not 
	    ** found
	    */
	    status = E_DB_OK;
	    syn_id->db_tab_base = syn_id->db_tab_index = 0;
	}
	else
	{
	    _VOID_ psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, err_blk);
	}
    }
    else
    {
	STRUCT_ASSIGN_MACRO(syn_tuple.db_syn_id, (*syn_id));
    }

    if (rdf_rb->rdr_rec_access_id != NULL)
    {
        rdf_rb->rdr_update_op = RDR_CLOSE;

        stat = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    _VOID_ psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, err_blk);
	    status = (status > stat) ? status : stat;
	}
    }
    
    return(status);
}
