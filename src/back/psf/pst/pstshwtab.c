/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <qu.h>
#include    <tm.h>
#include    <st.h>
#include    <iicommon.h>
#include    <usererror.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <cm.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <adf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <sxf.h>
#include    <psyaudit.h>

/**
**
**  Name: PSTSHWTAB.C - Functions for getting table and procedure
**			descriptions.
**
**  Description:
**      This file contains the functions for getting table and database
**	procedure definitions for the parser facility.
**	Table definitions consist of table ids, DMT_TBL_ENTRYs, and
**	DMT_ATTR_ENTRYs.  In addition, this function builds a hash table
**	for each range variable keyed on column name; this will
**	make for quick lookup of columns when parsing a query.
**	This file also contains a function to obtain info on LDB table.
**
**          pst_showtab    - Get a description of a table by name or table id.
**          pst_dbpshow    - Get a description of a database procedure by name.
**	    pst_ldbtabdesc - Get description of a LDB table.
**	    pst_rdfcb_init - initialize RDF_CB using contents of PSS_SESBLK
**
**  History:
**      20-mar-86 (jeff)    
**          written
**	18-may-88 (stec)
**	    Added pst_showdbp.
**	28-jul-88 (stec)
**	    Modify pst_dbpshow to save DB_PROCEDURE tuple.
**	03-aug-88 (stec)
**	    Improve error recovery in pst_dbpshow, unfix RDF cache.
**	19-aug-88 (andre)
**	    modify pst_dbpshow so that if a dbproc is new, we save its name and
**	    owner in dbpinfo
**	13-mar-89 (neil)
**	    modified pst_dbpshow to search in dba path as well - for rules.
**	29-nov-89 (andre)
**	    Change interface for pst_dbpshow(): allow lookup of a dbproc by the
**	    owner.
**	12-jan-1990 (andre)
**	    If running in FIPS mode, do not check for dbprocs owned by the DBA.
**	     Note that the user would ask to check for dbprocs owned BY_DBA only
**	     if he is looking for an existing procedure and doesn't know its
**	     owner.  In FIPS, if object owner name is not explicitly specified,
**	     default is to check user's objects only.
**	13-sep-90 (teresa)
**	    change pss_fips_mode from boolean to bitflag
**	27-sep-90 (ralph)
**          Call psy_dbpperm even if user own procedure, so that procedure
**          execution can be audited in one centralized place (bug 21243).
**	    Change interface to pst_dbpshow to include psq_cb.
**	23-sep-91 (andre)
**	    pst_dbpshow() will invoke psy_dbpperm() only if the caller requests
**	    it explicitly.
**	01-oct-91 (andre)
**	    added ret_flags to pst_dbpshow()'s interface.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	12-jun-92 (barbara)
**	    Added Star code for Sybil.
**	    Changes to pst_showtab, including passing in sess CB; add
**	    pst_ldbtab_desc(), a function to get a description of LDB table..
**	    Star history:
**	    23-sep-88 (andre)
**		Added pst_ldbtab_desc to obtain description of LDB table.
**	    28-mar-89 (andre)
**		modified pst_ldbtab_desc to check for errors in specifying
**		LDB site or DBMS type
**	    13-oct-89 (stec)
**		Modified pst_showtab to recognize PST_REMOVE flag for the
**		distributed case.
**	    15-aug-91 (kirke)
**		Removed MEcopy() declaration, added include of me.h.
**	    29-apr-92 (barbara)
**		Translate PSF flags regarding case of names into RDF flags.
**		(Part of fix for bug 42981).
**	04-dec-92 (tam)
**	    Changed pst_showtab to support registered dbproc in star.
**	03-apr-93 (ralph)
**	    DELIM_IDENT: Use pss_cat_owner instead of "$ingres"
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**      19-dec-94 (sarjo01) from 15-sep-94 (johna)
**                TRdisplay("Table %s\n",tabname);
**          added by markm while fixing bug (58681)
**      27-apr-95 (forky01)
**          Fix bug 68418, which came as a result of 19-dec-94 change.
**          Server would SEGV in TRdisplay when showtype was PST_SHWID
**          because tabname was NULL ptr.  Add check for tabname being
**          NULL, else use tabid
**	17-oct-95 (nick)
**	    DB_TAB_NAME being used as a char * in pst_showtab() ; this can 
**	    cause problems on some machines.  Add some casts to rdf_call()
**	    parameters to silence the compiler.
**	18-mar-96 (nick)
**	    Add RDR2_REFRESH if PSQ_MODIFY operation. #49396
**	19-nov-96 (kch)
**	    Added RDR2_REFRESH for PSQ_RANGE operation. This change fixes bug
**	    79156.
**	06-aug-1998 (nanpr01)
**	    Backed out the change for bug 79156 & 49396. Orlan's fix for RDF 
**	    cache invalidation fixes the problem. Also repeat query or define
**	    cursor need not refresh the cache.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout(), 
**	    pst_add_1indepobj(), pst_ldbtab_desc().
**      10-Oct-2002 (hanal04) Bug 108013 INGSRV 1805
**          Added RDR2_REFRESH for PSQ_COPY operation.. 
**      15-Apr-2003 (bonro01)
**          Added include <psyaudit.h> for prototype of psy_secaudit()
**	15-Feb-2004 (schka24)
**	    Don't allow partitions.
[@history_template@]...
**/

/*{
** Name: pst_showtab	- Get a table description for a range variable.
**
** Description:
**      This function gets a table description for a range variable.  It
**	calls RDF to get it.  RDF caches this information, so it's not
**	necessary to allocate memory for it here.
**
**	Note that this function doesn't follow the standard QUEL table search
**	path of looking for the table owned by the current user, then by the
**	dba.  It is up to the caller to tell what owner to look for.
**
** Inputs:
**	sess_cb				Pointer to session control block
**      showtype                        Tells whether to look up by name, table
**					id, or either.
**	tabname				Pointer to table name, if looking up by
**					name.
**	tabown				Pointer to table owner, if looking up by
**					name.
**	tabid				Pointer to table id, if looking up by id
**	tabonly				TRUE iff we don't want attributes.
**	rngentry			Pointer to range table entry to fill in.
**	query_mode			Query mode.
**	err_blk				Pointer to error block.
**
** Outputs:
**      rngentry                        Filled in with table description
**	    pss_tabdesc			    Set to point to table description
**	    pss_colhsh			    Hash table set up for quick lookup.
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error.
**	Exceptions:
**	    none
**
** Side Effects:
**	    RDF may allocate memory.
**
** History:
**	24-mar-86 (jeff)
**          written
**      03-sep-86 (seputis)
**          changed to use ->DB_TAB_OWN needs to be fixed in dbms.h
**	25-apr-88 (stec)
**	    Change to conform to new RDF_RB.
**	03-oct-88 (andre)
**	    modified so that if dropping base table for which there is no file,
**	    we still proceed to drop.
**	06-oct-88 (andre)
**	    Put the above fix on hold until RDF and DMF start doing their part
**	    (see the FIX_ME_NOTE)
**	12-dec-89 (andre)
**	    if a table description has been obtained, initialize pss_var_mask
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**	21-may-90 (teg)
**	    init rdr_instr to RDF_NO_INSTR
**	12-jun-92 (barbara)
**	    Merged code for Sybil: changed parameters to pass in session CB,
**	    set various flags for RDF for Star, handle Star-specific error codes
**	    after RDF call, tell RDF to destroy existing cached object on 
**	    DROP/REMOVE table in case object is stale after possible rollback
**	    (RDR2_REFRESH).  Merged Star History:
**	    13-oct-88 (andre)
**		if (showtype & PST_CHECK_EXIST) (i.e. we are only
**		interested in existence of the STAR object), set
**		rdr_types_mask |= RDR_CHECK_EXIST.
**	    15-nov-88 (andre)
**		Make DROP [TABLE] more forgiving in STAR (i.e. if the LDB
**		table no longer exists, or if its structure has changed since
**		the time the STAR object was created, we still want to be
**		able to DROP the object.)
**	    17-nov-88 (andre)
**		if (showtype & PST_DTBL) (i.e. we are trying to DROP a table,
**		and so we need to know if the underlying LDB table is owned
**		by this user so that we may DROP it), set
**		rdr_types_mask |= RDR_USERTAB.
**	    8-apr-89 (andre)
**		Set RDR_REFRESH if askiung for info used for REGISTER
**		WITH REFRESH
**	    02-may-89 (andre)
**		Certain errors pertaining to unavailability/inaccessability
**		of the LDB object may be returned to the caller rather than
**		the user if user didn't specify table name.  This was done
**		so that the caller may decide on the appropriate message
**		to issue.
**	    08-jun-89 (andre)
**		If trying to DROP STAR object along with underlying LDB
**		object and the LDB site is inaccessible, try to obtain info
**		about the STAR object only and warn the user that the LDB
**		object would not be dropped.
**	    26-jun-89 (andre)
**		Change the way we handle some of RDF messages when trying to
**		DROP a STAR object.  If LDB object doesn't exist, we will
**		remove the STAR object w/o producing sny messages.  RDF
**		will no longer check for schema mismatch when an object is
**		being DROPPED.  If the LDB object is not owned by the
**		present user or a connection cannot be established with the
**		LDB site, nothing will be dropped, and the user will be
**		informed of the problem and advised to use REMOVE.
**	    29-jun-89 (andre)
**	    	If RDF returns one of RD026D, RD0254, or RD0277 when asked
**		for info needed to DROP tables, return code to the caller,
**		rather than produce an error message.  Caller will produce
**		the message, but it is important that this error doesn't
**		prevent other tables from being dropped.
**	    13-oct-89 (stec)
**	    	Modified pst_showtab to recognize PST_REMOVE flag for the 
**	    	distributed case. RDF need to know that in the case of
**		DROP LINK and REMOVE command it is not supposed to check for
**		existence for the underlying local database objects.
**	    07-may-90 (andre)
**		fix bug 21464: if caller was trying to obtain table
**		description by table id, return USER error E_PS0915 instead
**		of CALLER error E_PS0915.
**	    05-aug-92 (teresa)
**		fix bug where retry of repeat queries did not work because SCF
**		did not have an interface to tell PSF to clear the cache.  So,
**		always refresh on PSQ_DEFQRY to avoid the problem.
**	04-dec-92 (tam)
**	    Add support for registered dbproc in star.
**	05-jan-93 (rblumer)
**	    check if there is a set-input parameter for a dbproc, and see
**	    if table matches that set BEFORE looking for a regular table.
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**	28-sep-93 (robf)
**          Add MAC check for table existence. We do this here in case of
**	    cached info (otherwise might get DMF MAC rejection later, when
**	    harder to handle effectively)
**	28-oct-93 (robf)
**          Don't do MAC  check on set input temp tables, which don't
** 	    have labels.
**	24-nov-93 (robf)
**          Fire failure security alarms if MAC access denies access to
**	    the table.
**	14-feb-94 (robf)
**          Check for PST_SETINPUT should be  direct comparison,  not
**	    bit-wise.
**	21-mar-94 (andre)
**	    just as it is done for repeat queries (05-aug-92 (teresa)), we will 
**	    tell RDF to ensure that we are given up-to-date info when parsing 
**	    definition of a repeat cursor
**	28-mar-94 (teresa)
**	    for bug 60609 -- fix if (PST_REGPROC) ... else if (PST_FULLSEARCH)
**	    so that there is no else in front of the if.  We may want to set
**	    fullsearch for regprocs as well.  This fix causes us to get the
**	    correct and meaningful error message instead of an obscure one.
**	17-oct-95 (nick)
**	    Replace use of tabname by the string contained within. #71851
**	18-mar-96 (nick)
**	    Add RDR2_REFRESH if PSQ_MODIFY operation. #49396
**	19-nov-96 (kch)
**	    Added RDR2_REFRESH for PSQ_RANGE operation. This change fixes bug
**	    79156.
**	8-apr-98 (inkdo01)
**	    Added RDR_BLD_KEY for "alter table", in case it's
**	    "add constraint with index = base table structure".
**	06-aug-1998 (nanpr01)
**	    Backed out the change for bug 79156 & 49396. Orlan's fix for RDF 
**	    cache invalidation fixes the problem. Also repeat query or define
**	    cursor need not refresh the cache.
**      10-Oct-2002 (hanal04) Bug 108013 INGSRV 1805
**          Added RDR2_REFRESH for PSQ_COPY operation. Create followed by
**          copy on a gtt was failing because the partial information
**          retrieved caused a timestamp mismatch in qeu_b_copy().
**	16-Jan-2004 (schka24)
**	    Rename RDR_BLD_KEY flag to RDR_BLD_PHYS.
**	09-mar-04 (hayke02)
**	    Re-introduce the fix for 49396 for session temporary tables only.
**	    This change fixes problem INGSRV 2706, bug 111782.
**	11-Mar-2004 (schka24)
**	    Don't need RDF refresh on COPY, DMF invalidates session temps
**	    now when it kills them.
**	    (Keep refresh on modify for now, until timestamps checked.)
**	28-Apr-2004 (schka24)
**	    However we do need "build phys" on COPY, to get the partition
**	    def in case it turns out to be a partitioned COPY FROM.
**      02-sep-2004 (sheco02)
**          Back out X-integrate change 471464 per schka24's email which states
**	    that the problem was fixed in r3.
**	11-Aug-2005 (mutma03)
**	    fixed bug where retry of queries results in erroneous timeout by
**	    adding code to report LOCK_TIMER_EXPIRED msg and prevented retry.
**	15-Sep-2005 (thaju02) B109499
**	    Set RDR2_REFRESH for PSQ_INDEX and PSQ_ALTERTABLE.
**	28-Aug-2006 (kschendel) b122118
**	    Fix the debug trdisplay of table name when show fails.
**	11-Sep-2007 (kibro01) b119101
**	    If we are coming from PSQ_MODIFY, we have already checked whether it
**	    is legal to have a partition here.
*/
DB_STATUS
pst_showtab(
	PSS_SESBLK	    *sess_cb,
	i4		    showtype,
	DB_TAB_NAME	    *tabname,
	DB_TAB_OWN	    *tabown,
	DB_TAB_ID	    *tabid,
	bool		    tabonly,
	PSS_RNGTAB	    *rngentry,
	i4		    query_mode,
	DB_ERROR	    *err_blk)
{
    DB_STATUS           status;
    RDF_CB		rdf_cb;
    i4		err_code;
    PSS_RNGTAB		*dbp_rngvar = sess_cb->pss_dbprng;

    /* Make sure the range table entry doesn't have a description yet */
    if (rngentry->pss_rdrinfo != (RDR_INFO *) NULL)
    {
	psf_error(E_PS0901_REDESC_RNG, 0L, PSF_INTERR, &err_code, err_blk, 0);
	return (E_DB_SEVERE);
    }

    /* if we are processing a set-input dbproc,
    ** check to see if the table being referenced is actually the set-input
    ** parameter, and, if so, fill in the rngentry from there
    ** 
    ** NOTE: the following IF condition relies on the fact that the NAME of the
    ** set-input parameter is unique and cannot conflict with a user's table
    ** name (we can guarantee this because the only set-input procs in 6.5 are
    ** system-generated ones).
    ** Thus we don't look at the owner/schema name in the check below.
    ** When/if we allow users to create set-input dbprocs and choose their own
    ** name for set-input parameters, we will have to come up with some scheme
    ** for handling set names that conflict with table names.
    ** (One possibility is to generate a unique schema-id for set-input
    **  parameters, similar to the way we do for SESSION temporary tables,
    **  and require users to use SET.X when referring to a set-input param X.)
    */
    if ((showtype & PST_SHWNAME) 
	&& (sess_cb->pss_dbp_flags & PSS_SET_INPUT_PARAM) 
	&& (dbp_rngvar != (PSS_RNGTAB *) NULL)
	&& (MEcmp((PTR) tabname, 
		  (PTR) &dbp_rngvar->pss_tabname,  sizeof(DB_TAB_NAME)) == 0))
    {
	/* we are referencing the set-input parameter,
	** so copy info from it into returned range entry
	** (fill in same fields using dbp_rngvar
	**  as the later code does using RDF info)
	*/
	rngentry->pss_tabdesc = dbp_rngvar->pss_tabdesc;
	rngentry->pss_rdrinfo = dbp_rngvar->pss_rdrinfo;

	/* Store away the table and owner names */
	STRUCT_ASSIGN_MACRO(dbp_rngvar->pss_tabname, rngentry->pss_tabname);
	STRUCT_ASSIGN_MACRO(dbp_rngvar->pss_ownname,   rngentry->pss_ownname);

	/* Store away the table id */
	STRUCT_ASSIGN_MACRO(dbp_rngvar->pss_tabdesc->tbl_id,
			    rngentry->pss_tabid);

	/* Store pointer to column hash table, if caller wants it */
	if (tabonly)
	{
	    rngentry->pss_attdesc = (DMT_ATT_ENTRY **) NULL;
	    rngentry->pss_colhsh = (RDD_ATTHSH *) NULL;
	}
	else
	{
	    rngentry->pss_attdesc = dbp_rngvar->pss_attdesc;
	    rngentry->pss_colhsh  = dbp_rngvar->pss_colhsh;
	}

	rngentry->pss_var_mask = (i4) 0;

	/* flag that this is a set-input parameter, not a regular table
	 */
	rngentry->pss_rgtype = PST_SETINPUT;

	return(E_DB_OK);
	
    }  /* end if set-input param */

    /*
    ** else not set-input parameter, so get info from RDF as usual
    */
    
    /*
    ** Fill in RDF control block.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);

    if (showtype & PST_SHWID)
    {
	if (tabid->db_tab_index < 0)
	{
	    /* Shouldn't get here with a partition ID, but puke if we do. */
	    err_blk->err_code = E_PS0903_TAB_NOTFOUND;
	    err_blk->err_data = 0;
	    return (E_DB_ERROR);
	}
	STRUCT_ASSIGN_MACRO(*tabid, rdf_cb.rdf_rb.rdr_tabid);
    }
    else
    {
	STRUCT_ASSIGN_MACRO(*tabname, rdf_cb.rdf_rb.rdr_name.rdr_tabname);
	STRUCT_ASSIGN_MACRO(tabown->db_tab_own, rdf_cb.rdf_rb.rdr_owner);
    }

    /* set flag for registered procedure */
    if (showtype & PST_REGPROC)
    {
	rdf_cb.rdf_rb.rdr_instr = RDF_REGPROC;
    }
    if (showtype & PST_FULLSEARCH)
    {
	rdf_cb.rdf_rb.rdr_instr = RDF_FULLSEARCH;
    }

    rdf_cb.rdf_rb.rdr_types_mask = RDR_RELATION;
    if (!tabonly)
	rdf_cb.rdf_rb.rdr_types_mask |= RDR_ATTRIBUTES | RDR_HASH_ATT;
    if (showtype & PST_SHWNAME)
	rdf_cb.rdf_rb.rdr_types_mask |= RDR_BY_NAME;

    /* Things that might need the partition definition, or perhaps
    ** physical key info, at parse time:
    */
    if (query_mode == PSQ_ALTERTABLE || query_mode == PSQ_MODIFY
      || query_mode == PSQ_COPY)
	rdf_cb.rdf_rb.rdr_types_mask |= RDR_BLD_PHYS;

    /* PST_DTBL and PST_CHECK_EXIST are mutually exclusive */

    /*
    ** DROPping a table in Star.  Need info on DDB object as well as some
    ** info on LDB object owned by USER.
    **
    ** 8/8/90 (andre)
    ** if trying to drop a catalog (PST_DTBL | PST_IS_CATALOG), indicate to
    ** RDF to not check name of the LDB object owned against the current
    ** user since the LDB object may be owned by the LDB system catalog
    ** owner.  (there was an old FIXME asking for this)
    */
    if (showtype & PST_DTBL)
    {
	rdf_cb.rdf_rb.rdr_types_mask |= RDR_USERTAB;
	if (showtype & PST_IS_CATALOG)
	{
	    rdf_cb.rdf_rb.rdr_2types_mask |= RDR2_SYSCAT;
	}
    }
    /* Check for existence of a DDB object. */
    else if (showtype & PST_CHECK_EXIST)
	rdf_cb.rdf_rb.rdr_types_mask |= RDR_CHECK_EXIST;

    if (showtype & PST_REMOVE)
	rdf_cb.rdf_rb.rdr_types_mask |= RDR_REMOVE;

    /* some query types will require special masks */
    switch (query_mode)
    {
	case PSQ_SLOCKMODE:
	{
	    rdf_cb.rdf_rb.rdr_types_mask |= RDR_SET_LOCKMODE;
	    break;
	}
	case PSQ_REREGISTER:
	{
	    rdf_cb.rdf_rb.rdr_types_mask |= RDR_REFRESH;
	    break;
	}
	case PSQ_DESTROY:
	case PSQ_REMOVE:
        case PSQ_RANGE:
        case PSQ_COPY:
	case PSQ_MODIFY:
	case PSQ_INDEX:
	case PSQ_ALTERTABLE:
	{
	    rdf_cb.rdf_rb.rdr_2types_mask |= RDR2_REFRESH;
	    break;
	}
	default:
	{
	    break;
	}
    }

    /* Get the description */
    status = rdf_call(RDF_GETDESC, (PTR) &rdf_cb);

    /*
    ** In STAR, if underlying LDB table cannot be found and we are just
    ** trying to DROP, try to establish if the STAR object exists and DROP it.
    */
    if (DB_FAILURE_MACRO(status) && (showtype & PST_DTBL))
    {
	i4 err_num = 0L;

	switch (rdf_cb.rdf_error.err_code)
	{
	    case E_RD0268_LOCALTABLE_NOTFOUND:
	    {
		err_blk->err_code = W_PS0914_MISSING_LDB_TBL;

		/* don't need the LDB table info */
		rdf_cb.rdf_rb.rdr_types_mask &= ~RDR_USERTAB;

		/* just check for existence of the STAR object */
		rdf_cb.rdf_rb.rdr_types_mask |= RDR_CHECK_EXIST;
		rdf_cb.rdf_info_blk = (RDR_INFO *) NULL;

		/* Get the description */
		status = rdf_call(RDF_GETDESC, (PTR) &rdf_cb);
		break;
	    }
	    case E_RD026D_USER_NOT_OWNER:
	    {
		err_num = E_PS0916_USER_NOT_OWNER;  break;
	    }
	    case E_RD0277_CANNOT_GET_ASSOCIATION:
	    {
		err_num = E_PS091A_CANNOT_CONNECT;  break;
	    }
	    case E_RD0254_DD_NOTABLE:   /* catalog entry is inconsistent */
	    {
		err_num = E_PS0919_BAD_CATALOG_ENTRY;
	    }
	}
		
	/*
	** one of the errors that get special treatment when trying to DROP
	*/
	if (err_num != 0L)
	{
	    err_blk->err_code = err_num;
	    return(status);
	}
    }

    /* If the table exists, but is a partition, pretend it doesn't exist. */
    /* FIXME should we attempt to produce a better / different error???
    ** Might involve a lot of work in the grammar actions though?
    */
    /* If we are coming from PSQ_MODIFY, we have already checked whether it
    ** is legal to have a partition here.  (kibro01) b119101
    */
    if (DB_SUCCESS_MACRO(status)
      && (rdf_cb.rdf_info_blk->rdr_rel->tbl_2_status_mask & DMT_PARTITION)
      && (query_mode != PSQ_MODIFY))
    {
	/* Since we're pretending it didn't work, better let RDF know
	** we don't want its info block any more.
	*/
	status = rdf_call(RDF_UNFIX, &rdf_cb);
	if (DB_FAILURE_MACRO(status))
	    (void) psf_rdf_error(RDF_UNFIX, &rdf_cb.rdf_error, err_blk);
	err_blk->err_code = E_PS0903_TAB_NOTFOUND;
	err_blk->err_data = 0;
	return (E_DB_ERROR);
    }

    if (DB_FAILURE_MACRO(status))
    {
	/*
	** If table not found, treat it as a caller error.  In QUEL, we will
	** look for the table owned by the current user, then by the dba.  In
	** SQL, we will look for the table owned by the current owner or the
	** explicitly-named owner.  This is controlled by the individual
	** grammars.
	*/
	if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
	{
	    /* Report the user error */
	    (VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_CALLERR,
		&err_code, err_blk, 1,
		psf_trmwhite(sizeof(*tabname), (char *) tabname), tabname);
	    return (E_DB_ERROR);
	}
	else if (rdf_cb.rdf_error.err_code == E_RD0254_DD_NOTABLE)
	{
	    /* catalog entry is inconsistent */
	    (VOID) psf_error(E_PS0919_BAD_CATALOG_ENTRY, 0L,
	        (showtype & PST_SHWID) ? PSF_CALLERR : PSF_USERERR,
		&err_code, err_blk, 1,
		psf_trmwhite(sizeof(*tabname), (char *) tabname), tabname);
	    return (E_DB_ERROR);
	}
	else if (rdf_cb.rdf_error.err_code == E_RD0268_LOCALTABLE_NOTFOUND)
	{
	    if (showtype & PST_SHWID)
	    {
		(VOID) psf_error(E_PS0915_MISSING_LDB_TBL, 0L, PSF_USERERR,
		    &err_code, err_blk, 2,
		    sizeof(tabid->db_tab_base), &tabid->db_tab_base,
		    sizeof(tabid->db_tab_index), &tabid->db_tab_index);
	    }
	    else
	    {
		(VOID) psf_error(E_PS0912_MISSING_LDB_TBL, 0L, PSF_USERERR,
		    &err_code, err_blk, 1,
		    psf_trmwhite(sizeof(*tabname), (char *) tabname), tabname);
	    }
	    return (E_DB_ERROR);
	}
	else if (rdf_cb.rdf_error.err_code == E_RD0269_OBJ_DATEMISMATCH)
	{
	     (VOID) psf_error(E_PS0913_SCHEMA_MISMATCH, 0L,
		 (showtype & PST_SHWID) ? PSF_CALLERR : PSF_USERERR,
		 &err_code, err_blk, 1,
		 psf_trmwhite(sizeof(*tabname), (char *) tabname), tabname);
	      return (E_DB_ERROR);
	}
	else

	/*
	** FIX_ME_NOTE:	    The statement enclosed in this comment MUST be
	**		    uncommented as soon as RDF and DMF make
	**		    corresponding changes
	**	    
	**  if (!((query_mode == PSQ_DESTROY) &&
	**	  (rdf_cb.rdf_error.err_code == E_RD0021_FILE_NOT_FOUND)))
	*/
	
	{
            if (showtype & PST_SHWID)
		TRdisplay("Table ID [%d][%d]\n",
                    tabid->db_tab_base, tabid->db_tab_index);
	    else
		TRdisplay("Table %~t\n", sizeof(tabname->db_tab_name), tabname->db_tab_name);
	    /* Report this message and make it appear as if query is 
	       already in retry so that psq_call won't retry */
	    if ( rdf_cb.rdf_error.err_code == E_RD002B_LOCK_TIMER_EXPIRED)
	        sess_cb->pss_retry = PSS_REPORT_MSGS;
	    (VOID) psf_rdf_error(RDF_GETDESC, &rdf_cb.rdf_error, err_blk);
	    return (status);
	}
    }
    /* Success; set up pointer to table description */
    rngentry->pss_tabdesc = rdf_cb.rdf_info_blk->rdr_rel;

    /* Store away pointer to RDF information block */
    rngentry->pss_rdrinfo = rdf_cb.rdf_info_blk;

    /* Store away the table and owner names */
    STRUCT_ASSIGN_MACRO(rngentry->pss_tabdesc->tbl_name, rngentry->pss_tabname);
    STRUCT_ASSIGN_MACRO(rngentry->pss_tabdesc->tbl_owner,
			rngentry->pss_ownname);

    /* Store away the table id */
    STRUCT_ASSIGN_MACRO(rngentry->pss_tabdesc->tbl_id, rngentry->pss_tabid);


    /* Store pointer to column hash table, if caller wants it */
    if (tabonly)
    {
	rngentry->pss_attdesc = (DMT_ATT_ENTRY **) NULL;
	rngentry->pss_colhsh = (RDD_ATTHSH *) NULL;
    }
    else
    {
	rngentry->pss_attdesc = rdf_cb.rdf_info_blk->rdr_attr;
	rngentry->pss_colhsh = rdf_cb.rdf_info_blk->rdr_atthsh;
    }

    rngentry->pss_var_mask = (i4) 0;
    return (E_DB_OK);
}

/*{
** Name: pst_dbpshow	- Get a database procedure description.
**
** Description:
**      This procedure gets a descriptor for the database procedure. The
**	descriptor is allocated in QSF memory (for now). If the procedure
**	exists the descriptor will be, in most case, filled with RDF info
**	describing the procedure.
**	Return status indicates success unless things failed in RDF or 
**	when allocating memory. Value of the ptr to PSS_DBPINFO indicates
**	whether a descriptor could be created.
**	The code labelled "info" is executed for existing and non-existent
**	procedures (depends on (dbp_mask & PSS_NEWDBP).)  For an existing
**	procedure it will contain ptrs to catalog info, for a new procedure
**	the catalog info will not be filled in, a descriptor is needed,
**	however, because of other fields in PSS_DBPINFO which are used when
**	constructing the procedure tree.
**
** Inputs:
**	sess_cb			PSF session control block
**	dbpname			ptr to procedure name
**	dbpinfop		pointer to location that will hold
**				the address of allocated PSS_DBPINFO
**				structure, which holds procedure
**				related info.
**	dbp_owner		owner of dbproc if (dbp_mask & PSS_DBP_BY_OWNER)
**	dbp_id			id of dbproc if (dbp_mask & PSS_DBP_BY_ID)
**	dbp_mask		flag field
**	    PSS_USRDBP		search for dbprocs owned by the current user
**	    PSS_DBADBP		search for dbprocs owned by the DBA
**				(must not be set unless (dbp_mask & PSS_USRDBP))
**	    PSS_INGDBP		search for dbprocs owned by $ingres
**				(must not be set unless (dbp_mask & PSS_USRDBP))
**	    PSS_DBP_BY_OWNER	search for dbproc owned by specific user
**	    PSS_NEWDBP		dbproc is NOT expected to exist
**	    PSS_DBP_PERM_AND_AUDIT call psy_dbpperm() to verify that the user
**	                           posesses privileges required to execute the
**				   statement + audit the statement if necessary
**	    PSS_DBP_BY_ID	search for dbproc by specified id
**		NOTE:		PSS_USRDBP <==> !PSS_DBP_BY_OWNER, 
**				PSS_NEWDBP ==> !PSS_DBADBP,
**				PSS_NEWDBP ==> !PSS_INGDBP,
**				PSS_DBP_BY_ID ==> !PSS_USRDBP,
**				PSS_DBP_BY_ID ==> !PSS_DBADBP,
**				PSS_DBP_BY_ID ==> !PSS_INGDBP,
**				PSS_DBP_BY_ID ==> !PSS_DBP_BY_OWNER,
**				PSS_DBP_BY_ID ==> !PSS_NEWDBP
**		
**	psq_cb			ptr to psq_cb
**
** Outputs:
**	dbpinfop		holds the adress of allocated PSS_DBPINFO
**				structure filled in with procedure related
**				info, else NULL
**	psq_cb->
**		err_blk		error block filled in with error info
**	ret_flags		may have bits set to indicate certain expected
**				conditions
**	    PSS_INSUF_DBP_PRIVS	user lacks required privileges
**	    PSS_DUPL_DBPNAME	current user already owns a dbproc with the
**				specified name
**	    PSS_MISSING_DBPROC	dbproc could not be found
**
**	Returns:
**	    E_DB_OK		Success
**	    E_DB_ERROR		Non-catastrophic error
**	    E_DB_FATAL		Catastrophic error.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    RDF may allocate memory.
**	    PSF allocates memory.
**
** History:
**	18-may-88 (stec)
**          written
**	28-jul-88 (stec)
**	    Modify pst_dbpshow store DB_PROCEDURE tuple.
**	03-aug-88 (stec)
**	    Improve error recovery in pst_dbpshow, unfix RDF cache.
**	13-mar-89 (neil)
**	    Added "anyproc" argument to search in dba path too.  If the
**	    procedure is not owned by the current user then verify that
**	    they have EXECUTE permission.
**	29-nov-89 (andre)
**	    Change interface for pst_dbpshow(): allow lookup of a dbproc by the
**	    owner.
**	12-jan-1990 (andre)
**	    If running in FIPS mode, do not check for dbprocs owned by the DBA.
**	     Note that the user would ask to check for dbprocs owned BY_DBA only
**	     if he is looking for an existing procedure and doesn't know its
**	     owner.  In FIPS, if object owner name is not explicitly specified,
**	     default is to check user's objects only.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**	21-may-90 (teg)
**	    init rdr_instr to RDF_NO_INSTR
**	13-sep-90 (teresa)
**	    change pss_fips_mode from boolean to bitflag.
**	27-sep-90 (ralph)
**          Call psy_dbpperm even if user own procedure, so that procedure
**          execution can be audited in one centralized place (bug 21243).
**	    Change interface to pst_dbpshow to include psq_cb.
**	20-feb-91 (andre)
**	    interface to psy_dbpperm() has changed
**	20-sep-91 (andre)
**	    if a given dbproc is not owned by either the current user or the dba
**	    and (dbp_mask & PSS_INGDBP), look at dbprocs owned by $ingres
**	23-sep-91 (andre)
**	    pst_dbpshow() will invoke psy_dbpperm() only if the caller requests
**	    it explicitly.
**	23-sep-91 (andre)
**	    pst_dbpshow() will produce error msgs 2400/2405 if it comes across
**	    duplicate/missing dbproc
**	26-sep-91 (andre)
**	    if parsing a dbproc and the dbproc being looked up is owned by the
**	    current user, add it to the list of independent objects for the
**	    dbproc being parsed (duplicate entries will be eliminated.)
**	30-sep-91 (andre)
**	    use sess_cb->pss_dependencies_stream to allocate memory for the
**	    elements of independent objects
**	01-oct-91 (andre)
**	    added ret_flags to pst_dbpshow()'s interface.
**	04-oct-91 (andre)
**	    If we are parsing a dbproc (P) and the dbproc which has just been
**	    looked up (P') is owned by the current user, scan the sublist of
**	    independent objects associated with P looking for a descriptor for
**	    P'; if not found, insert a descriptor for P' into the sublist.
**	12-feb-92 (andre)
**	    psy_dbpperm() will no longer accept rdf_cb as one of its parameters;
**	    instead it will expect the caller to implicitly pass dbp name,
**	    owner, and id; at the same time, rather than passing a flag field to
**	    indicate whether the user posessed required privilege, we will pass
**	    an address of a map describing privilege - if upon return it is
**	    non-zero, user lacked required privilege
**	08-apr-92 (andre)
**	    if we fail to find a table/view and we are parsing a dbproc, set
**	    PSS_MISSING_OBJ in sess_cb->pss_dbp_flags
**	30-jun-92 (andre)
**	    added dbpid to the interface + added code to allow retrieval by
**	    dbproc id
**	12-apr-93 (andre)
**	    do not add dbprocs to the independent object list if parsing a
**	    system-generated dbproc
**	10-jul-93 (andre)
**	    cast func args to match prototype declarations
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**	03-nov-93 (andre)
**	    if a user specified name of the schema and the dbproc was not found,
**	    return a new "vague" message - E_US0890_2192_INACCESSIBLE_DBP - 
**	    saying that either the dbproc did not exist or the user lacked 
**	    required privileges on it - this is done to prevent an unauthorized
**	    user from deducing any information about the database procedure on 
**	    which he possesses no privileges
**	29-dec-93 (robf)
**          Audit procedure owner on MAC failure.
*/
DB_STATUS
pst_dbpshow(
	PSS_SESBLK	*sess_cb,
	DB_DBP_NAME	*dbpname,
	PSS_DBPINFO	**dbpinfop,
	DB_OWN_NAME	*dbp_owner,
	DB_TAB_ID	*dbp_id,
	i4		dbp_mask,
	PSQ_CB		*psq_cb,
	i4		*ret_flags)
{
    DB_STATUS           status, stat;
    DB_STATUS	        local_status;
    RDF_CB		rdf_cb;
    PSS_DBPINFO		*dbpinfo;
    DB_ERROR		*err_blk = &psq_cb->psq_error;
    i4		err_code;
    bool		leave_loop = TRUE;
    DB_PROCEDURE	*dbp;
    i4		msgid;
    i4		access;

    *dbpinfop = (PSS_DBPINFO *) NULL;
    *ret_flags = 0;

    /*
    ** Fill in RDF control block.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);

    if (dbp_mask & PSS_DBP_BY_ID)
    {
	STRUCT_ASSIGN_MACRO((*dbp_id), rdf_cb.rdf_rb.rdr_tabid);
	rdf_cb.rdf_rb.rdr_types_mask = RDR_PROCEDURE;	/* retrieve by id */
    }
    else
    {
	STRUCT_ASSIGN_MACRO(*dbpname, rdf_cb.rdf_rb.rdr_name.rdr_prcname);

	if (dbp_mask & PSS_USRDBP)
	{
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, rdf_cb.rdf_rb.rdr_owner);
	}
	else	/* must be PSS_DBP_BY_OWNER */
	{
	    STRUCT_ASSIGN_MACRO((*dbp_owner), rdf_cb.rdf_rb.rdr_owner);
	}

	rdf_cb.rdf_rb.rdr_types_mask = RDR_PROCEDURE | RDR_BY_NAME;
    }

    do      		/* something to break out of */
    {
	/* Get the description */
	status = rdf_call(RDF_GETINFO, (PTR) &rdf_cb);

	/*
	** if caller specified dbproc id, or
	**    caller specified dbproc owner name, or
	**    the dbproc was found, or
	**    the error other than "dbproc not found" was encountered,
	**  bail out
	*/
	if (   dbp_mask & (PSS_DBP_BY_ID | PSS_DBP_BY_OWNER)
	    || DB_SUCCESS_MACRO(status)
	    || rdf_cb.rdf_error.err_code != E_RD0201_PROC_NOT_FOUND
	   )
	    break;

	/*
	** if
	**	    - dbproc was not found, and
	**	    - caller requested that DBA's dbprocs be considered, and
	**	    - user is not the DBA,
	** check if the dbproc is owned by the DBA
	*/
	if (   dbp_mask & PSS_DBADBP
	    && MEcmp((PTR)&sess_cb->pss_dba.db_tab_own,
		     (PTR)&sess_cb->pss_user, sizeof(DB_OWN_NAME))
	   )
	{
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_dba.db_tab_own,
				rdf_cb.rdf_rb.rdr_owner);
	    status = rdf_call(RDF_GETINFO, (PTR) &rdf_cb);
	}

	/*
	** if
	**	    - still not found and
	**	    - caller requested that $INGRES' dbprocs be looked at and
	**	    - user is not $ingres and
	**	    - DBA is not $ingres,
	** look at dbprocs owned by $ingres
	*/
	if (   DB_FAILURE_MACRO(status)
	    && rdf_cb.rdf_error.err_code == E_RD0201_PROC_NOT_FOUND
	    && dbp_mask & PSS_INGDBP)
	{
	    if (   MEcmp((PTR) &sess_cb->pss_user,
	                 (PTR) sess_cb->pss_cat_owner,
			 sizeof(DB_OWN_NAME))
		&& MEcmp((PTR) &sess_cb->pss_dba.db_tab_own,
			 (PTR) sess_cb->pss_cat_owner,
			 sizeof(DB_OWN_NAME)))
	    {
		STRUCT_ASSIGN_MACRO((*sess_cb->pss_cat_owner),
				    rdf_cb.rdf_rb.rdr_owner);
		status = rdf_call(RDF_GETINFO, (PTR) &rdf_cb);
	    }
	}

	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);
    
    if (DB_FAILURE_MACRO(status))
    {
	switch (rdf_cb.rdf_error.err_code)
	{
	    case E_RD0201_PROC_NOT_FOUND:
	    {
		/*
		** If the dbproc was expected to exist but was not found,
		** *ret_flags will provide an indication of the problem - status
		** will be set to E_DB_OK. (caller no longer needs to check if
		** dbpinfop==NULL)
		*/
		status = E_DB_OK;
		if (dbp_mask & PSS_NEWDBP)
		{
		    goto info;
		}
		else
		{
		    if (sess_cb->pss_dbp_flags & PSS_DBPROC)
			sess_cb->pss_dbp_flags |= PSS_MISSING_OBJ;

		    *ret_flags |= PSS_MISSING_DBPROC;

		    if (dbp_mask & PSS_DBP_BY_ID)
		    {
			(VOID) psf_error(E_PS055F_DBP_NOT_FOUND,  0L,
			    PSF_USERERR, &err_code, err_blk, 2,
			    (i4) sizeof(dbp_id->db_tab_base),
			    (PTR) &dbp_id->db_tab_base,
			    (i4) sizeof(dbp_id->db_tab_index),
			    (PTR) &dbp_id->db_tab_index);
		    }
		    else if (dbp_mask & PSS_DBP_BY_OWNER)
		    {
			char        command[PSL_MAX_COMM_STRING];
			i4     length;

			/*
			** if the user specified name of the schema in which 
			** the dbproc was supposed to exist, issue a vague 
			** message designed to prevent an unauthorized user 
			** from deducing any information about a dbproc on 
			** which he possesses no privileges
			*/

			psl_command_string(psq_cb->psq_mode, sess_cb->pss_lang,
			    command, &length);

			_VOID_ psf_error(E_US0890_2192_INACCESSIBLE_DBP,
			    0L, PSF_USERERR, &err_code, err_blk, 3,
			    length, command,
			    psf_trmwhite(sizeof(*dbp_owner), 
				(char *) dbp_owner),
			    (PTR) dbp_owner,
			    psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbpname),
			    (PTR) dbpname);
		    }
		    else
		    {
			_VOID_ psf_error(2405L, 0L, PSF_USERERR, &err_code,
			    err_blk, 1,
			    psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbpname),
			    (char *) dbpname);
		    }
		    return(status);
		}
	    }
	    default:
	    {
		(VOID) psf_rdf_error(RDF_GETINFO, &rdf_cb.rdf_error, err_blk);
		return (status);
	    }
	}
    }

    /* Procedure exists if we got to this point. */
    dbpname = &rdf_cb.rdf_rb.rdr_name.rdr_prcname;

    if (dbp_mask & PSS_NEWDBP)
    {
	/*
	** Procedure does exist, but this was not expected,
	** just exit - *ret_flags contains a desription of the problem
	** (caller no longer needs to check if dbpinfop==NULL).
	*/

	*ret_flags |= PSS_DUPL_DBPNAME;
	_VOID_ psf_error(2400L, 0L, PSF_USERERR, &err_code, err_blk, 1,
	    psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbpname),
	    (char *) dbpname);

	status = E_DB_OK;
	goto cleanup;
    }
    /*
    ** At this point have found the procedure, so check if we are allowed
    ** MAC access to this, we check early so failure is treated as a 
    ** non-existent procedure
    */
    dbp=rdf_cb.rdf_info_blk->rdr_dbp;

    /*
    ** if we reached this point,
    ** either (dbp_mask & PSS_NEWDBP) && no RDF data was found or
    ** (!(dbp_mask & PSS_NEWDBP)) and RDF data was found.
    */ 
info:		    
    /* Allocate structure holding DBP info */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, (i4) (sizeof(PSS_DBPINFO)), 
	(PTR *) &dbpinfo, err_blk);
    if (DB_FAILURE_MACRO(status))
    {
	if (dbp_mask & PSS_NEWDBP)	
	{
	    return(status);	/* for new dbproc no RDF data was fixed */
	}
	else
	{
	    goto cleanup;
	}
    }
    	
    *dbpinfop = dbpinfo;

    /*
    ** if procedure didn't exist, no RDF data was fixed, so save procedure
    ** name and owner in dbpinfo and return;
    ** otherwise save the DB_PROCEDURE tuple and fall through to unfix
    ** RDF entry
    */
    if (dbp_mask & PSS_NEWDBP)
    {
	STRUCT_ASSIGN_MACRO(*dbpname, dbpinfo->pss_ptuple.db_dbpname);
	/*
	** Note that we use rdr_owner, so if PSS_NEWDBP && PSS_DBP_BY_OWNER,
	** name stored in db_owner will be that from *dbp_owne
	*/
	STRUCT_ASSIGN_MACRO(rdf_cb.rdf_rb.rdr_owner,
			    dbpinfo->pss_ptuple.db_owner);
	return(status);
    }
    else 
    {
	(VOID) MEcopy((PTR) rdf_cb.rdf_info_blk->rdr_dbp,
		      sizeof (DB_PROCEDURE),
		      (PTR) &dbpinfo->pss_ptuple);
    }

    /*
    ** If we are parsing a dbproc let P refer to the dbproc being parsed, and P'
    ** refer to the dbproc which we have just looked up.  If P' is owned by the
    ** owner of P (same as sess_cb->pss_user), we will scan the independent
    ** object sublist associated with P and if an entry for P' is not found, one
    ** will be inserted.  Note that only dbprocs owned by the current user will
    ** be included into the list of independent objects.
    **
    ** NOTE: we do not build independent object list for system-generated
    **	     dbprocs
    */
    if (   sess_cb->pss_dbp_flags & PSS_DBPROC
	&& ~sess_cb->pss_dbp_flags & PSS_SYSTEM_GENERATED
	&& psq_cb->psq_mode == PSQ_CALLPROC
        && !MEcmp((PTR) &sess_cb->pss_user, (PTR) &dbpinfo->pss_ptuple.db_owner,
	       sizeof(sess_cb->pss_user))
       )
    {
	i4			    obj_type = PSQ_OBJTYPE_IS_DBPROC;
	
	if (dbpinfo->pss_ptuple.db_mask[0] & DB_DBPGRANT_OK)
	{
	    /*
	    ** if the dbproc we just looked up was marked as grantable in
	    ** IIPROCEDURE, store an indicator of its grantability in the
	    ** independent object list; this is especially valuable when
	    ** processing a dbproc to determine if it is grantable since
	    ** psy_dbp_stat() will use it to decide to not call
	    ** psy_dbp_priv_check() on this dbproc
	    */
	    obj_type |= PSQ_GRANTABLE_DBPROC;
	}
	
	if (dbpinfo->pss_ptuple.db_mask[0] & DB_ACTIVE_DBP)
	{
	    /*
	    ** if the dbproc we just looked up was marked as active in
	    ** IIPROCEDURE, store an indicator that it is active in the
	    ** independent object list; this is especially valuable when
	    ** processing a dbproc to determine if it is at least active (as
	    ** would happen when we try to verify that the dbproc being
	    ** [re]created is not dormant) since psy_dbp_stat() will use it to
	    ** decide to not call psy_dbp_priv_check() on this dbproc
	    */
	    obj_type |= PSQ_ACTIVE_DBPROC;
	}

	status = pst_add_1indepobj(sess_cb, &dbpinfo->pss_ptuple.db_procid, obj_type,
	    dbpname, &sess_cb->pss_indep_objs.psq_objs,
	    sess_cb->pss_dependencies_stream, err_blk);

	if (DB_FAILURE_MACRO(status))
	{
	    goto cleanup;
	}
    }
	
    /* 
    ** If user requested that we check permissions and/or audit this statement,
    ** do it now.
    **
    ** In case of error or if the required privileges could not be found
    ** psy_dbpperm() will clean up iiprotect before returning.
    */
    if (dbp_mask & PSS_DBP_PERM_AND_AUDIT)
    {
	i4                  privs;

	/* psy_dbpperm() expects caller to set the required privilege */
	privs = (psq_cb->psq_mode == PSQ_GRANT) ? DB_EXECUTE | DB_GRANT_OPTION
						: DB_EXECUTE;

	status = psy_dbpperm(sess_cb, &rdf_cb, psq_cb, &privs);

	if (DB_SUCCESS_MACRO(status) && privs)
	{
	    *ret_flags |= PSS_INSUF_DBP_PRIVS;
	}
    }

    rdf_cb.rdf_rb.rdr_types_mask = RDR_PROCEDURE | RDR_BY_NAME;

cleanup:

    /* We can unfix the cache entry now */
    stat = rdf_call(RDF_UNFIX, (PTR) &rdf_cb);

    if (DB_FAILURE_MACRO(stat))
    {
	(VOID) psf_rdf_error(RDF_UNFIX, &rdf_cb.rdf_error, err_blk);
	if (stat > status)
	    status = stat; 
    }

    return (status);
}

/*
** Name:    pst_add_1indepobj - add an element describing a single object to the
**			        independent object list unless one has already
**			        been added
**
** Description:
**	If the current sublist does not contain an element describing this
**	object, generate one.  This function will be called for database
**	procedures and dbevents.
**
**	We could not use the function used for independent tables/views because
**	an independent object list element describing tables and views contains
**	IDs of ALL objects found in one statement and is constructed differently
**	from independent object elements for dbprocs and dbevents.
**
** Input:
**
**	obj_id				    object id
**	obj_type			    object type
**	    PSQ_OBJTYPE_IS_DBPROC	    object is a dbproc
**	    PSQ_GRANTABLE_DBPROC	    dbproc is marked as grantable
**	    PSQ_ACTIVE_DBPROC		    dbproc is marked as active
**	    PSQ_OBJTYPE_IS_DBEVENT	    object is a dbevent
**	dbpname				    name of database procedure to be
**					    included in the new element if
**					    (obj_type & PSQ_OBJTYPE_IS_DBPROC);
**					    NULL otherwise
**	obj_list			    address of ptr to the first element
**					    of the independent object list
**	mstream				    stream to be used if a new element
**					    must be allocated
**
** Output:
**	obj_list			    will point to the new element if one
**					    was added
**	err_blk				    will be filed in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR)
**
** Side effects:
**	may allocate memory
**
** History:
**
**	11-feb-92 (andre)
**	    written
*/
DB_STATUS
pst_add_1indepobj(
	PSS_SESBLK	*sess_cb,
	DB_TAB_ID	*obj_id,
	i4		obj_type,
	DB_DBP_NAME     *dbpname,
	PSQ_OBJ		**obj_list,
	PSF_MSTREAM	*mstream,
	DB_ERROR	*err_blk)
{
    /*
    ** check the sublist associated with the dbproc being parsed for duplicates
    */

    PSQ_OBJ	    *elem;
    bool	    found = FALSE;
    i4		    end_of_sublist = PSQ_INFOLIST_HEADER | PSQ_DBPLIST_HEADER;
    DB_STATUS	    status;
    i4		    real_obj_type =
			obj_type & ~(PSQ_GRANTABLE_DBPROC | PSQ_ACTIVE_DBPROC);

    for (elem = *obj_list;
	 (elem && !(elem->psq_objtype & end_of_sublist));
	 elem = elem->psq_next
	)
    {
	/*
	** only need to consider elements containing objects of specified type
	** (dbproc or dbevent); such elements contain exactly one object id
	*/
	if (   elem->psq_objtype & real_obj_type
	    && elem->psq_objid->db_tab_base == obj_id->db_tab_base
	    && elem->psq_objid->db_tab_index == obj_id->db_tab_index
	   )
	{
	    found = TRUE;
	    break;
	}
    }

    if (!found)
    {
	/*
	** allocate space for DB_DBP_NAME only if the creating a descriptor for
	** a database procedure
	*/

	i4	size = (obj_type & PSQ_OBJTYPE_IS_DBPROC)
	    ? sizeof(PSQ_OBJ) : sizeof(PSQ_OBJ) - sizeof(DB_DBP_NAME);
							
	status = psf_malloc(sess_cb, mstream, size, (PTR *) &elem, err_blk);

	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}

	/* populate the entry */
	elem->psq_objtype = obj_type;
	elem->psq_num_ids = 1;
	elem->psq_objid->db_tab_base = obj_id->db_tab_base;
	elem->psq_objid->db_tab_index = obj_id->db_tab_index;
	if (obj_type & PSQ_OBJTYPE_IS_DBPROC)
	    STRUCT_ASSIGN_MACRO((*dbpname), elem->psq_dbp_name);

	/* and insert it into the list */
	elem->psq_next = *obj_list;
	*obj_list = elem;
    }

    return(E_DB_OK);
}

/*
** Name: pst_rdfcb_init	- common code to initialize RDF_CB
**
** Description:
**	this function will be called to zero out RDF_CB structure and initialize
**	common elements of RDF_CB.  caller will still be responsible for
**	customizing RDF_CB to its specific needs.
**
** Input:
**	rdf_cb				ptr to RDF_CB to be initialized
**	sess_cb				PSF session CB
**	    pss_dbid			database id
**	    pss_udbid			unique db id
**	    pss_sessid			Pointer to session id
**	    pss_distrib			Flag indicating whether we are running
**					in distributed session
**	    pss_retry
**		PSS_REFRESH_CACHE	Redo mode
**
** Output:
**	rdf_cb				initialized RDF_CB
**	    rdf_rb			request block
**		rdr_db_id		    = pss_dbid
**		rdr_unique_dbid		    = pss_udbid
**		rdr_session_id		    = pss_sessid
**		rdr_r1_distrib		    = pss_distrib
**		rdr_fcb			    = Psf_srvblk->psf_rdfid
**		rdr_2types_mask		    |= RDR2_REFRESH
**
** History
**
**	27-jul-92 (andre)
**	    written
**	6-Jul-2006 (kschendel)
**	    Update unique dbid type (is i4).
**      21-Nov-2008 (hanal04) Bug 121248
**          Initialise new rdf_cb->caller_ref to NULL.
*/
VOID
pst_rdfcb_init(
	RDF_CB	    *rdf_cb,
	PSS_SESBLK  *sess_cb)
{
    RDR_RB	        *rdf_rb = &rdf_cb->rdf_rb;
    extern PSF_SERVBLK  *Psf_srvblk;

    MEfill(sizeof(RDF_CB), (u_char) 0, (PTR) rdf_cb);
    rdf_rb->rdr_db_id		= sess_cb->pss_dbid;
    rdf_rb->rdr_unique_dbid	= sess_cb->pss_udbid;
    rdf_rb->rdr_session_id	= sess_cb->pss_sessid;
    rdf_rb->rdr_r1_distrib	= sess_cb->pss_distrib;
    rdf_rb->rdr_fcb		= Psf_srvblk->psf_rdfid;
    rdf_cb->caller_ref          = NULL;
    if (sess_cb->pss_retry & PSS_REFRESH_CACHE)
	rdf_rb->rdr_2types_mask |= RDR2_REFRESH;

    return;
}

/*{
** Name: pst_ldbtab_desc	- Get a description of a LDB table.
**
** Description:
**      This function gets a LDB table description.  It
**	calls RDF to get it.
**
**
** Inputs:
**	ddl_info			ptr to a structure descibing STAR object
**					and the corresponding LDB object
**					
**	    qed_d6_tab_info_p		pointer to LDB table info structure
**					containing LDB table name and, possibly,
**					number of columns and their names
**					
**		dd_t1_tab_name		LDB table name
**		dd_t6_mapped_b		TRUE if user provided column mapping
**		dd_t7_col_cnt		expected number of columns (if user
**					provided mapping)
**		dd_t8_cols_pp		ptr to array of column descriptor
**					pointers; will be filled in.
**					NOTE that if the user requested mapping,
**					column descriptor ptrs have not been
**					allocated yet.
**
**		dd_t9_ldb_p		pointer to a structure containing LDB
**					descriptor (NODE, LDB, DBMS) and DBA
**					information and LDB capabilities
**					
**		    dd_i1_ldb_desc	LDB descriptor
**		    dd_i2_ldb_plus	structure containing DBA info and LDB
**					capabilities.
**
**	mem_stream			Ptr to memory stream descriptor to be
**					used for memory allocation.
**
**	flag				Flag that may indicate special
**					conditions.
**	
**	err_blk				Pointer to error block.
**
** Outputs:
**      ddl_info
**	    qed_d6_tab_info_p		pointer to LDB table info structure
**					containing all available info about
**					LDB table if table is found.
**
**		dd_t1_tab_name		LDB table name may be uppercased if the
**					user has not quoted it, and the LDB
**					supports upper case.
**		dd_t6_mapped_b		may be reset to TRUE if the LDB supports
**					mixed case and the user did not provide
**					mapping.
**		dd_t7_col_cnt		number of columns in LDB table
**		dd_t8_cols_pp		will point to an array of column
**					descriptor ptrs pointing to LDB
**					table column descriptors.
**					NOTE that if LDB supports upper case and
**					the user provided no mapping, the LDB
**					table column names will be all
**					lowercased (this has to do with the fact
**					that when no mapping is specified, ptr
**					to array of link column descriptors is
**					set to the same value as that pointing
**					to the array of LDB table column
**					descriptors.) 
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error.
**	Exceptions:
**	    none
**
** Side Effects:
**	    May allocate memory
**
** History:
**	23-sep-88 (andre)
**          written
**	02-nov-88 (andre)
**	    changed so that in the absence of mapping, we first ask RDF for the
**	    number of columns, and then ask RDF to fill in their names.
**	07-nov-88 (andre)
**	    Modified to check for existence of an LDB table owned by a
**	    user.
**	09-nov-88 (andre)
**	    Modified to handle "bad LDB user" error from RDF.
**	03-jan-89 (andre)
**	    Modified to propvide RDF with indication of when the case of table
**	    name may be changed.
**	04-jan-89 (andre)
**	    if LDB dbms_type is ingres, do not check if the LDB column names
**	    are valid INGRES names.
**	01-feb-89 (andre)
**	    Will accept (QED_DD:_INFO *) instead of (DD_2LDB_TAB_INFO *) and
**	    (DD_1LDB_INFO *).
**	01-feb-89 (andre)
**	    If user specified no mapping, AND the LDB supports mixed case names,
**	    AND not all column names are lower case, lowercase the LDB column
**	    names and use them as link column names as if the mapping was
**	    provided by the user.
**	28-mar-89 (andre)
**	    check for errors in specifying LDB site or DBMS type.
**	07-apr-89 (andre)
**	    Remove code checking correctness of specified DBMS-type (also known
**	    as "server class".)  Check will be performed by RQF.
**      03-mar-90 (teg)
**          initialized rdr_2types_mask.
**	08-aug-90 (andre)
**	    defined a new bit - PSS_DUP_CAT
**	29-apr-92 (barbara)
**	    Translate PSF flags regarding case of names into RDF flags.
**	    (Part of fix for bug 42981).
**	16-now-92 (teresa)
**	    support procedures.
**	07-jan-93 (tam)
**	    add error reporting for registered procedure.
**	15-jan-93 (tam)
**	    initialize rdf_cb.
**	01-mar-93 (barbara)
**	    Delimited id support.  Check case mapping incompatibilities
**	    between DDB and LDB after looking table up.
**	20-may-93 (barbara)
**	    Remove "else" when setting flags RDF_DELIM_TBL and RDF_TBL_CASE.
**	    These flags are not mutually exclusive.  Same for RDF_DELIM_OWN
**	    and RDF_OWN_CASE.
**	01-jun-93 (barbara)
**	    Remove code that massaged case of column names for non-ingres
**	    tables.  The delimited id work provides for that mapping
**	    (for ingres tables as well) in QEF.
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**	02-sep-93 (barbara)
**	    Re delimited id support: exclude tables that are being CREATEd
**	    when checking case mapping problems between DDB and LDB.  We
**	    allow a table to be created on a case sensitive LDB when the DDB
**	    is case sensitive; we just don't allow registrations.
**	19-oct-93 (barbara)
**	    Removed unnecessary arguments from call to psf_error for
**	    E_PS091E_INCOMPAT_CASE_MAPPING. (bug 56674)
**	1-nov-93 (barbara)
**	    Registered procedure code was executing a return from the main
**	    "do" loop; however it by-passed code that checks for incompatible
**	    case translation semantics.  Changed it to do a "break" instead.
**	    (bug 56676)
**	26-Feb-2001 (jenjo02)
**	    Provide session id to RDF.
*/
DB_STATUS
pst_ldbtab_desc(
	PSS_SESBLK		*sess_cb,
	QED_DDL_INFO		*ddl_info,
	PSF_MSTREAM		*mem_stream,
	i4			flag,
	DB_ERROR		*err_blk)
{
    RDF_CB		    rdf_cb;
    i4		    err_code;
    DD_2LDB_TAB_INFO	    *ldb_tab_info = ddl_info->qed_d6_tab_info_p;
    RDR_RB		    *rdf_rb  = &rdf_cb.rdf_rb;
    RDR_DDB_REQ		    *rdr_req = &rdf_rb->rdr_r2_ddb_req;
    DB_STATUS		    status;
    DD_LDB_DESC		    *ldb_desc =
				     &ldb_tab_info->dd_t9_ldb_p->dd_i1_ldb_desc;
    bool		    leave_loop = TRUE;
				     
    register i4	    col_cnt;
    register DD_COLUMN_DESC **col_names;

    i4			    *name_case =
      &ldb_tab_info->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c6_name_case;
    bool		    duplicate_table = FALSE;
      
    /* populate the RDF request block */

    MEfill(sizeof(RDF_CB), (u_char) 0, (PTR) &rdf_cb);

    rdf_rb->rdr_types_mask = RDR_RELATION;	/* get table description */
    rdf_rb->rdr_2types_mask = 0;
    rdf_rb->rdr_r1_distrib = DB_3_DDB_SESS;
    rdf_rb->rdr_session_id = sess_cb->pss_sessid;

    /* get LDB table info */
    rdr_req->rdr_d1_ddb_masks = RDR_DD2_TABINFO;

    rdr_req->rdr_d2_ldb_info_p = ldb_tab_info->dd_t9_ldb_p;
    
    /* provide structure to contain LDB table description */
    rdr_req->rdr_d5_tab_info_p = ldb_tab_info;

    if (flag & PSS_DELIM_OWN)
    {
	rdf_cb.rdf_rb.rdr_instr |= RDF_DELIM_OWN;
    }
    if (flag & PSS_GET_OWNCASE)
    {
	rdf_cb.rdf_rb.rdr_instr |= RDF_OWN_CASE;
    }
    if (flag & PSS_DELIM_TBL)
    {
	rdf_cb.rdf_rb.rdr_instr |= RDF_DELIM_TBL;
    }
    if (flag & PSS_GET_TBLCASE)
    {
	rdf_cb.rdf_rb.rdr_instr |= RDF_TBL_CASE;
    }

    /* BB_FIXME: Do I need this? */
    /* if (flag & PSS_DDB_MIXED)
    {
	rdf_cb.rdf_rb.rdr_instr |= RDF_DDB_MIXED;
    }
    */
    if (flag & PSS_REGPROC)
    {
	rdf_cb.rdf_rb.rdr_instr |= RDF_REGPROC;
    }
    
    do      	/* something to break out of */
    {    
	if (flag & (PSS_DUP_TBL | PSS_DUP_CAT))
	{
	    if (flag & PSS_DUP_TBL)
	    {
		/* check if table by user already exists */
		rdr_req->rdr_d1_ddb_masks |= RDR_DD7_USERTAB;
	    }
	    else
	    {
		/*
		** check if a catalog owned by the LDB system catalog owner
		** already exists
		*/
		rdr_req->rdr_d1_ddb_masks |= RDR_DD11_SYSOWNER;
	    }
	    
	    status = rdf_call(RDF_DDB, (PTR) &rdf_cb);

	    if (DB_SUCCESS_MACRO(status))
	    {
		duplicate_table = TRUE;
	    }
	    break;
	}
	else if (!ldb_tab_info->dd_t6_mapped_b)
	{
	    /*
	    ** if column name mapping was not required, (i.e. we don't know
	    ** how many columns the table will have), ask RDF
	    */

	    status = rdf_call(RDF_DDB, (PTR) &rdf_cb);

	    if (DB_FAILURE_MACRO(status))
	    {
		break;
	    }
	    
	    col_cnt = ldb_tab_info->dd_t7_col_cnt;

	    /* we have already obtained table info */
	    rdf_rb->rdr_types_mask &= ~RDR_RELATION;
	}
	else
	{
	    if (flag & PSS_REGPROC)
	    {
		/* There shouild never be mapped columns for registered
		** procedures, so error out with a consistency check
		*/
		psf_error(E_PS091D_PROC_MAPPED_COLS, 0L, PSF_USERERR, &err_code,
		    err_blk, 3,
		    psf_trmwhite(sizeof(DD_NAME), ldb_tab_info->dd_t1_tab_name),
		    ldb_tab_info->dd_t1_tab_name,
		    psf_trmwhite(sizeof(DD_NAME), ldb_desc->dd_l2_node_name),
		    ldb_desc->dd_l2_node_name,
		    psf_trmwhite(sizeof(DD_256C), ldb_desc->dd_l3_ldb_name),
		    ldb_desc->dd_l3_ldb_name);
		return (E_DB_ERROR);
	    }

	    /* expected # of columns in LDB table */
	    col_cnt = ldb_tab_info->dd_t7_col_cnt;
	}
	

	/* all subsequent processing deals with column descriptions.
	** however, if this is a registered procedure, it does not have any,
	** do exit now.
	*/
	if (flag & PSS_REGPROC)
	{
	    col_cnt = 0;
	    break;
	}

	    /* Allocate pointers to column descriptors of the LDB table. */
	status = psf_malloc(sess_cb, mem_stream,
		    (col_cnt + 1) * sizeof(DD_COLUMN_DESC *),
		    (PTR *) &ldb_tab_info->dd_t8_cols_pp, err_blk);

	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}

	/* allocate space for LDB table column descriptors */ 

	for (col_names = &ldb_tab_info->dd_t8_cols_pp[1];
	     col_cnt > 0;
	     col_names++, col_cnt--)
	{
	    status = psf_malloc(sess_cb, mem_stream, sizeof(DD_COLUMN_DESC),
				(PTR *)col_names, err_blk);

	    if (DB_FAILURE_MACRO(status))
	    {
		return(status);
	    }
	}

	rdf_rb->rdr_types_mask |= RDR_ATTRIBUTES;   /* need attrib. info */
	
	/* Get the description */
	status = rdf_call(RDF_DDB, (PTR) &rdf_cb);

	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);

    if (   DB_FAILURE_MACRO(status)
	&& rdf_cb.rdf_error.err_code == E_RD0277_CANNOT_GET_ASSOCIATION
       )
    {
	psf_error(E_PS0917_CANNOT_CONNECT, 0L, PSF_USERERR, &err_code, err_blk,
		3, psf_trmwhite(sizeof(DD_NAME), ldb_desc->dd_l2_node_name),
    		ldb_desc->dd_l2_node_name,
    		psf_trmwhite(sizeof(DD_256C), ldb_desc->dd_l3_ldb_name),
    		ldb_desc->dd_l3_ldb_name,
    		psf_trmwhite(sizeof(DD_NAME), ldb_desc->dd_l4_dbms_name),
    		ldb_desc->dd_l4_dbms_name);
	return(status);
    }		

    /*
    ** Check for incompatibility between DDB's and LDB's case translation
    ** semantics.  This is an issue if: 
    ** 1. the user delimited the LDB object name and the local DB supports
    **	  mixed case delimited ids;
    **    or the user delimited the LDB user name the local DB supports
    **	  mixed case real user names; and
    ** 2. This is a REGISTER (PSS_DUP_TBL will be set if processing a CREATE).
    */
    if (   !(flag & (PSS_DDB_MIXED|PSS_DUP_TBL|PSS_DUP_CAT))
	&& (   (   flag & PSS_DELIM_TBL
		&& rdf_rb->rdr_r2_ddb_req.rdr_d2_ldb_info_p->dd_i2_ldb_plus.
	   	   dd_p3_ldb_caps.dd_c10_delim_name_case == DD_1CASE_MIXED)
	    || (   flag & PSS_DELIM_OWN
		&& rdf_rb->rdr_r2_ddb_req.rdr_d2_ldb_info_p->dd_i2_ldb_plus.
	   	   dd_p3_ldb_caps.dd_c11_real_user_case == DD_1CASE_MIXED)
	    )
       )
    {
	psf_error(E_PS091E_INCOMPAT_CASE_MAPPING, 0L, PSF_USERERR, &err_code,
		err_blk, 1, 
		psf_trmwhite(sizeof(DD_NAME), ldb_tab_info->dd_t1_tab_name),
		ldb_tab_info->dd_t1_tab_name);
	return (E_DB_ERROR);
    }

    if (DB_FAILURE_MACRO(status))
    {
	switch (rdf_cb.rdf_error.err_code)
	{
	    case E_RD0002_UNKNOWN_TBL:
	    {
		/*
		** if checking for duplicates, not finding the table is
		** good news
		*/
		if (flag & (PSS_DUP_TBL | PSS_DUP_CAT))
		{
		    return(E_DB_OK);
		}
		
		/* If table not found, treat it as a user error. */

		if (flag & PSS_REGPROC)
		{
		    /* no ldb procedure */
		    psf_error(E_PS1203_NO_LDB_PROC, 0L, PSF_USERERR, &err_code,
		    	err_blk, 4,
		    	psf_trmwhite(sizeof(DD_NAME), ldb_tab_info->dd_t1_tab_name),
		    	ldb_tab_info->dd_t1_tab_name,
		    	psf_trmwhite(sizeof(DD_NAME), ldb_desc->dd_l2_node_name),
		    	ldb_desc->dd_l2_node_name,
		    	psf_trmwhite(sizeof(DD_256C), ldb_desc->dd_l3_ldb_name),
		    	ldb_desc->dd_l3_ldb_name,
		    	psf_trmwhite(sizeof(DD_NAME), ldb_desc->dd_l4_dbms_name),
		    	ldb_desc->dd_l4_dbms_name);
		}
		else
		{
		    psf_error(E_PS090D_NO_LDB_TABLE, 0L, PSF_USERERR, &err_code,
		    	err_blk, 4,
		    	psf_trmwhite(sizeof(DD_NAME), ldb_tab_info->dd_t1_tab_name),
		    	ldb_tab_info->dd_t1_tab_name,
		    	psf_trmwhite(sizeof(DD_NAME), ldb_desc->dd_l2_node_name),
		    	ldb_desc->dd_l2_node_name,
		    	psf_trmwhite(sizeof(DD_256C), ldb_desc->dd_l3_ldb_name),
		    	ldb_desc->dd_l3_ldb_name,
		    	psf_trmwhite(sizeof(DD_NAME), ldb_desc->dd_l4_dbms_name),
		    	ldb_desc->dd_l4_dbms_name);
		}

		break;
	    }
	    
	    case E_RD0256_DD_COLMISMATCH:
	    {
		/*
		** number of columns specified by the user is different
		** from that in the LDB table
		*/
		psf_error(E_PS090E_COL_MISMATCH, 0L, PSF_USERERR, &err_code,
		    err_blk,0);
		break;
	    }
	    
	    case E_RD026C_UNKNOWN_USER:
	    {
		psf_error(E_PS0911_BAD_LDB_USER, 0L, PSF_USERERR, &err_code,
		    err_blk,2,
		    psf_trmwhite(sizeof(DD_NAME), ldb_desc->dd_l2_node_name),
		    ldb_desc->dd_l2_node_name,
		    psf_trmwhite(sizeof(DD_256C), ldb_desc->dd_l3_ldb_name),
		    ldb_desc->dd_l3_ldb_name);
		
		break;
	    }
	    default:
	    {
		psf_rdf_error(RDF_GETDESC, &rdf_cb.rdf_error, err_blk);
		break;
	    }
	}
	return(status);
    }

    if (duplicate_table)
    {
	psf_error(E_PS0910_DUP_LDB_TABLE, 0L, PSF_USERERR, &err_code,
		err_blk, 3,
	    	psf_trmwhite(sizeof(DD_NAME), ldb_tab_info->dd_t1_tab_name),
	    	ldb_tab_info->dd_t1_tab_name,
	    	psf_trmwhite(sizeof(DD_NAME), ldb_desc->dd_l2_node_name),
	    	ldb_desc->dd_l2_node_name,
	    	psf_trmwhite(sizeof(DD_256C), ldb_desc->dd_l3_ldb_name),
	    	ldb_desc->dd_l3_ldb_name);

	return (E_DB_ERROR);
    }
    return(E_DB_OK);
}
