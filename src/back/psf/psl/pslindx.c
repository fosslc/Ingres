/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <bt.h>
#include    <er.h>
#include    <ci.h>
#include    <qu.h>
#include    <st.h>
#include    <cm.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <sxf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <psyaudit.h>
#include    <usererror.h>
#include    <cui.h>

/*
** Name: PSLINDX.C:	this file contains functions used in semantic actions
**			for INDEX (QUEL) and CREATE INDEX (SQL)
**
** Function Name Format:  (this applies to externally visible functions only)
**
**	PSL_CI<number>[S|Q]_<production name>
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
**	this file contains functions used in semantic actions for INDEX (QUEL)
**	and CREATE INDEX (SQL)
**
**	psl_ci1_create_index	- Semantic action for create_index: (SQL) and
**				  index: (QUEL) productions
**	psl_ci2_index_prefix	- Semantic action for index_prefix: (SQL) and
**				  indexq: (QUEL) productions
**	psl_ci3_indexrel	- Semantic action for indexrel: (SQL) and
**				  instmnt: (QUEL) productions
**	psl_ci4_indexcol	- Semantic actions for indexcol: (SQL) and
**				  indexcolname: (QUEL)
**	psl_ci5_indexlocname	- Semantic actions for indexlocname: production
**
** History:
**	5-mar-1991 (bryanp)
**	    Created as part of the Btree index compression project.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements: disallow CREATE INDEX on temp tables.
**	20-may-92 (schang)
**	    GW integration.
**	15-jun-1992 (barbara)
**	    Sybil merge.  In psl_ci2_index_prefix() disallow CREATE INDEX
**	    in distributed thread (using DB_3_DDB_SESS instead of DB_DSTYES).
**      28-sep-1992 (pholman)
**          C2: change qeu_audit statements to sxf_calls, add <sxf.h>, <tm.h>.
**	16-nov-1992 (pholman)
**	    C2: update sxf_call to be called via psy_secaudit wrapper
**      10-dec-1992 (rblumer)
**	    create special char entries for system-generated CREATE INDEX
**	    statements; handle new WITH PERSISTENCE and UNIQUE_SCOPE clauses.
**      04-mar-1993 (rblumer)
**	    add new error message.
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**	15-may-1993 (rmuth)
**	    Add support for WITH CONCURRENT_ACCESS.
**	28-jun-93 (robf)
**	    Propagate base table row security label/audit attributes to
**	    index. Added cond_add parameter to psl_ci4_indexcol to allow
**	    a conditional addition of an attribute.
**	    Pass security label to psy_secaudit()
**      12-Jul-1993 (fred)
**          Disallow indexes on extension tables.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rmuth)
**	    Allow psl_validate_options to check COPY options.
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	18-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping.
**	08-feb-95 (cohmi01)
**	    For RAW/IO, element can now contain raw extent name as well, place
**	    into new rawextnm array in dmu_cb.
**	23-feb-95 (cohmi01)
**	    For RAW I/O, allocate dmu_cb...rawextnm.data_address
**	28-apr-1995 (shero03)
**	    Support RTREE
**	15-may-1996 (shero03)
**	    Add HI compression
**	16-apr-1997 (shero03)
**	    Support NBR 3D
**      21-mar-1999 (nanpr01)
**          Support Raw location.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Apr-2001 (horda03) Bug 104402
**          Ensure that Foreign Key constraint indices are created with
**          PERSISTENCE. This is different to Primary Key constraints as
**          the index need not be UNIQUE nor have a UNIQUE_SCOPE.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**	01-Mar-2001 (jenjo02)
**	    Remove references to obsolete DB_RAWEXT_NAME.
**      02-jan-2004 (stial01)
**          Changes to expand number of WITH CLAUSE options
**	24-Jan-2004 (schka24)
**	    with compression=(key) can come thru for constraint now too;
**	    changes for partitioned tables project.
**      21-Nov-2008 (hanal04) Bug 121248
**          If we are invalidating a table referenced in a range variable
**          we should pass the existing pss_rdrinfo reference if there is one.
**          Furthermore, we need RDF to clear down our reference to the
**          rdf_info_blk if it is freed.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	13-Oct-2010 (kschendel) SIR 124544
**	    Many changes for centralized WITH-option handling.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
i4 psl_ci1_create_index(
	PSS_SESBLK *sess_cb,
	DB_ERROR *err_blk);
i4 psl_ci2_index_prefix(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	i4 unique);
i4 psl_ci3_indexrel(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PSS_OBJ_NAME *tbl_spec);
i4 psl_ci4_indexcol(
	PSS_SESBLK *sess_cb,
	char *colname,
	DB_ERROR *err_blk,
	bool cond_add);
i4 psl_ci5_indexlocname(
	PSS_SESBLK *sess_cb,
	char *loc_name,
	PSS_OBJ_NAME *index_spec,
	PSQ_CB *psq_cb);

/*
** Name: psl_ci1_create_index	- Semantic actions for CREATE INDEX
**
** Description:
**	This routine performs the semantic actions for create_index: (SQL) and
**	index: (QUEL) productions.
**
**	create_index:	index_prefix indexlocname ON indexrel
**			LPAREN indexcols RPAREN index_with
**
**	index:		instmnt LPAREN indexcols RPAREN index_with
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**
** Outputs:
**     err_blk		    will be filled in if an error was encountered
**
** Returns:
**	E_DB_{OK,ERROR}
**
** History:
**	5-mar-1991 (bryanp)
**	    Created.
**	03-aug-1992 (barbara)
**	    Invalidate the underlying table's infoblk from the RDF cache.
**	10-dec-1992 (rblumer)
**	    set up info to validate unique_scope and persistence
**	28-jun-93 (robf)
**	    Check for base table row security audit/label attributes and
**	    propagate to index.
**	10-aug-93 (andre)
**	    fixed cause of compiler warning
**	03-sep-1993 (rblumer)
**	    remove code to set up info to validate unique_scope & persistence;
**	    it is only used for MODIFY, so has been moved to pslmdfy.c.
**	22-oct-93 (robf)
**          When automatically adding the row audit attribute to the index,
**	    don't include it in the key, otherwise we may unexpectedly violate
**	    a UNIQUE constraint (FIPS).
**	 7-sep-95 (allst01)
**	    Don't add the security label to the key either.
**	    This may need expanding upon later if the field isn't hidden.
**	01-may-1995 (shero03)
**	    Support Rtree's range 
**	04-Feb-2004 (jenjo02)
**	    All indexes on partitioned tables are, for now, GLOBAL;
**	    add that DMU characteristic.
**	23-Nov-2005 (kschendel)
**	    Need to validate allocation= since scanning time can't.
**	19-Mar-2008 (kschendel) b122118
**	    Don't need default-structure validation, scf is on the job.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Rework to operate with dmu indicators.
*/
DB_STATUS
psl_ci1_create_index(
	PSS_SESBLK	*sess_cb,
	DB_ERROR	*err_blk)
{
    QEU_CB		    *qeu_cb;
    DMU_CB		    *dmu_cb;
    RDF_CB		    rdf_cb;
    DB_STATUS		    status;
    i4		    mask2=0;
    i4			    num_atts;
    i4			    i;
    DMT_ATT_ENTRY	    *att_entry;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    /* Make sure there is a location */
    if (!dmu_cb->dmu_location.data_in_size)
    {
	/* Default location */
	STmove("$default", ' ', sizeof(DB_LOC_NAME),
	    (char*) dmu_cb->dmu_location.data_address);
	dmu_cb->dmu_location.data_in_size = sizeof(DB_LOC_NAME);
    }

    /* Check for base table security row label/audit attributes and
    ** set the same for the index
    */
    mask2 = sess_cb->pss_resrng->pss_tabdesc->tbl_2_status_mask;

    if(mask2&DMT_ROW_SEC_AUDIT)
    {
	/*
	** Mark index as having row security audit
	*/
	BTset(DMU_ROW_SEC_AUDIT, dmu_cb->dmu_chars.dmu_indicators);
        num_atts = (i4) sess_cb->pss_resrng->pss_tabdesc->tbl_attr_count;
        for (i = 0; i < num_atts; i++)
	{
		att_entry = sess_cb->pss_resrng->pss_attdesc[i+1];
		if(att_entry->att_flags & DMU_F_SEC_KEY)
		{
			i4 num_att=dmu_cb->dmu_key_array.ptr_in_count;
			/*
			** Found the security key attribute, so add
			** it to the index.
			*/
			status=psl_ci4_indexcol(sess_cb,
				att_entry->att_nmstr,
				err_blk,
				TRUE);
			if(status!=E_DB_OK)
				return E_DB_ERROR;
			if(dmu_cb->dmu_key_array.ptr_in_count!=num_att)
			{
				/*
				** New column added, but we don't include
				** this in the key, so manufacture a key
				** clause if necessary.
				*/
				if(!dmu_cb->dmu_attr_array.ptr_in_count)
				    dmu_cb->dmu_attr_array.ptr_in_count=num_att;

			}
			break;
		}
	}
    }

    /* All indexes on partitioned tables are for now "GLOBAL" */
    if ( sess_cb->pss_resrng->pss_tabdesc->tbl_status_mask & DMT_IS_PARTITIONED )
    {
	BTset(DMU_GLOBAL_INDEX, dmu_cb->dmu_chars.dmu_indicators);
    }

    /*
    ** Invalidate base table information from RDF cache.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabid,
			rdf_cb.rdf_rb.rdr_tabid);
    rdf_cb.rdf_info_blk = sess_cb->pss_resrng->pss_rdrinfo;
    rdf_cb.caller_ref = &sess_cb->pss_resrng->pss_rdrinfo;

    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error, err_blk);

    return (status);
}

/*
** Name: psl_ci2_index_prefix	- Semantic actions for index_prefix: (SQL) and
**				  indexq: (QUEL) productions
**
** Description:
**	This routine performs the semantic actions for index_prefix: (SQL) and
**				  indexq: (QUEL) productions
**
**	index_prefix:       CREATE index_unique INDEX
**	indexq:		    INDEX index_unique
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	psq_cb		    ptr to the PSQ_CB control block.
**	unique		    TRUE if UNIQUE was specified; FALSE otherwise
**
** Outputs:
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	5-mar-1991 (bryanp)
**	    Created.
**	15-jun-1992 (barbara)
**	    Sybil merge.  Disallow CREATE INDEX in distributed thread (using
**	    DB_3_DDB_SESS instead of DB_DSTYES).
**	20-may-92 (schang)
**	    GW integration.
**	10-dec-92 (rblumer)
**	    Add code to create special char entries if PST_INFO is set
**          (in system-generated execute immediate actions for constraints);
**	    set PSS_WC_UNIQUE bit to be used in UNIQUE_SCOPE processing.
**	23-feb-95 (cohmi01)
**	    For RAW I/O, allocate dmu_cb...rawextnm.data_address
**	08-may-98 (nanpr01)
**	    Parallel Index creation.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Set indicator in DMU characteristics, not with-clauses.
*/
DB_STATUS
psl_ci2_index_prefix(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	i4		unique)
{
    i4         err_code;
    DB_STATUS	    status;
    QEU_CB	    *qeu_cb, *save_qeu_cb;
    DMU_CB	    *dmu_cb;
    DB_ERROR	    *err_blk = &psq_cb->psq_error;

    /* [CREATE] INDEX is not allowed in distributed yet */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	(VOID) psf_error(5311L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	return (E_DB_ERROR);
    }

    /* Start with resdom count = 0 */
    sess_cb->pss_rsdmno = 0;
    save_qeu_cb = (QEU_CB *) sess_cb->pss_object;

    /* allocate QEU_CB for [CREATE/REGISTER] INDEX and initialize its header */
    if (save_qeu_cb != (QEU_CB *) 0)
    {
	/* Fix up the previous one because qeu_d_op contains DMU_INDEX_TABLE */
        qeu_cb = (QEU_CB *) sess_cb->pss_object;
        qeu_cb->qeu_d_op    = DMU_PINDEX_TABLE;

	/* stream already opened - allocate the next one */
        status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QEU_CB),
            &sess_cb->pss_object, err_blk);
        if (status != E_DB_OK)
            return (status);

	/* Now initialize it */
        MEfill(sizeof(QEU_CB), (u_char) 0, sess_cb->pss_object);
 
        qeu_cb = (QEU_CB *) sess_cb->pss_object;
 
        /* Fill in the control block header */
        qeu_cb->qeu_length      = sizeof(QEU_CB);
        qeu_cb->qeu_type        = QEUCB_CB;
        qeu_cb->qeu_owner       = (PTR)DB_PSF_ID;
        qeu_cb->qeu_ascii_id    = QEUCB_ASCII_ID;
        qeu_cb->qeu_db_id       = sess_cb->pss_dbid;
        qeu_cb->qeu_d_id        = sess_cb->pss_sessid;
        qeu_cb->qeu_eflag       = QEF_EXTERNAL;
        qeu_cb->qeu_mask        = 0;
 
        /* Give QEF the opcode */
        qeu_cb->qeu_d_op    = DMU_PINDEX_TABLE;
    }
    else
        status = psl_qeucb(sess_cb, DMU_INDEX_TABLE, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    qeu_cb = (QEU_CB *) sess_cb->pss_object;

    /* Now link the saved que_cb with this and new is always the first one */
    if (save_qeu_cb != (QEU_CB *) 0)
    {
	qeu_cb->qef_next = save_qeu_cb->qef_next;
	save_qeu_cb->qef_next = qeu_cb;
	qeu_cb->qeu_prev = save_qeu_cb;
    }
    else 
    {
	qeu_cb->qef_next = (QEU_CB *)&qeu_cb->qef_next;
	qeu_cb->qeu_prev = (QEU_CB *)&qeu_cb->qef_next;
	sess_cb->pss_save_qeucb = (PTR) qeu_cb;
    }

    /* Allocate a DMU control block */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMU_CB),
	(PTR *) &dmu_cb, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    qeu_cb->qeu_d_cb = (PTR) dmu_cb;

    /* Clear DMU_CB and everything in it */
    MEfill(sizeof(DMU_CB), 0, dmu_cb);

    /* Fill in the DMU control block header */
    dmu_cb->type = DMU_UTILITY_CB;
    dmu_cb->length = sizeof(DMU_CB);
    dmu_cb->dmu_flags_mask = 0;
    dmu_cb->dmu_db_id = (char *) sess_cb->pss_dbid;
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, dmu_cb->dmu_owner);
    dmu_cb->dmu_gw_id = DMGW_NONE;

    /*
    ** Allocate the index entries.  Allocate enough space for DB_MAXKEYS
    ** (the maximum number of keys in an index), although it's probably
    ** fewer.  This is because we don't know how many columns we have at
    ** this point, and it would be a big pain to allocate less and then
    ** run into problems.
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			DB_MAXKEYS * sizeof(DMU_KEY_ENTRY *),
			(PTR *) &dmu_cb->dmu_key_array.ptr_address, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    dmu_cb->dmu_key_array.ptr_size      = sizeof(DMU_KEY_ENTRY);
    dmu_cb->dmu_key_array.ptr_in_count  = 0;    /* Start with 0 attributes*/

    /*
    ** Assume that KEYS clause is nonexistent; count below is set to 0
    ** which means that all attributes described in dmu_key_array are to
    ** be treated as keys.
    */
    dmu_cb->dmu_attr_array.ptr_address  = dmu_cb->dmu_key_array.ptr_address;
    dmu_cb->dmu_attr_array.ptr_size	= sizeof(DMU_KEY_ENTRY);
    dmu_cb->dmu_attr_array.ptr_in_count = 0;

    /* schang : find out if gateway or not by testing psq_cb-> */

    if (psq_cb->psq_mode == PSQ_REG_INDEX)
	BTset(DMU_GATEWAY, dmu_cb->dmu_chars.dmu_indicators);

    /* if PST_INFO flags are set, set special attributes.
    **
    ** (For CONSTRAINTS, QEF will build an Execute Immediate CREATE INDEX
    **  statement, and set bits in the PST_INFO structure to tell us
    **  special things about the index.)
    */
    if (psq_cb->psq_info != (PST_INFO *) NULL)
    {
	if (psq_cb->psq_info->pst_execflags & PST_SYSTEM_GENERATED)
	    BTset(DMU_SYSTEM_GENERATED, dmu_cb->dmu_chars.dmu_indicators);

	if (psq_cb->psq_info->pst_execflags & PST_NOT_DROPPABLE)
	    BTset(DMU_NOT_DROPPABLE, dmu_cb->dmu_chars.dmu_indicators);

	if (psq_cb->psq_info->pst_execflags & PST_SUPPORTS_CONSTRAINT)
	    BTset(DMU_SUPPORTS_CONSTRAINT, dmu_cb->dmu_chars.dmu_indicators);

	if (psq_cb->psq_info->pst_execflags & PST_NOT_UNIQUE)
	    BTset(DMU_NOT_UNIQUE, dmu_cb->dmu_chars.dmu_indicators);
    } /* end if psq_info != NULL */

    /* If we know it's unique, remember that now.  Some forms of syntax
    ** may set UNIQUE later on (but still before the WITH-clause is
    ** parsed.
    */
    if (unique)
	BTset(DMU_UNIQUE, dmu_cb->dmu_chars.dmu_indicators);

    /*
    ** Allocate the location entries.  Assume DM_LOC_MAX, although it's
    ** probably fewer.  This is because we don't know how many locations
    ** we have at this point, and it would be a big pain to allocate
    ** less and then run into problems.
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	    DM_LOC_MAX * sizeof(DB_LOC_NAME),
	    (PTR *) &dmu_cb->dmu_location.data_address, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    dmu_cb->dmu_location.data_in_size = 0;    /* Start with 0 locations */

    if (psq_cb->psq_mode == PSQ_REG_INDEX)
    {
	/*  schang : GW sure is different here
	** Allocate the OLOCATION entries to save the from clause for gateway
	*/

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMU_FROM_PATH_ENTRY),
	    (PTR *) &dmu_cb->dmu_olocation.data_address, err_blk);
        if (DB_FAILURE_MACRO(status))
	    return (status);

	dmu_cb->dmu_olocation.data_in_size = sizeof(DMU_FROM_PATH_ENTRY);
    }
    return (E_DB_OK);
}

/*
** Name: psl_ci3_indexrel   - semantic action for indexrel: (SQL) and
**			      instmnt: (QUEL) productions
**
** Description:
**	This routine performs the semantic actions for indexrel: (SQL) and
**	instmnt: (QUEL) productions
**
**	indexrel:           NAME
**	instmnt:	    indexq ON NAME index_is indexlocname
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	psq_cb		    Query parse control block
**	tbl_spec	    structure describing the relation name as it was
**			    entered by the user
**
** Outputs:
**	psq_cb.err_blk	    Will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	5-mar-1991 (bryanp)
**	    Created.
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements: disallow CREATE INDEX on temp tables.
**	20-may-92 (schang)
**	    GW integration.
**	15-jun-1992 (barbara)
**	    Added mask argument (value 0) to psl0_rngent because interface
**	    to that function has changed for Sybil.
**      28-sep-1992 (pholman)
**          C2: change qeu_audit statements to sxf_calls.
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**      12-Jul-1993 (fred)
**          Disallow indexes on extension tables.
**	12-aug-93 (andre)
**	    replace TABLES_TO_CHECK() with PSS_USRTBL|PSS_DBATBL|PSS_INGTBL.
**	    TABLES_TO_CHECK() was being used because long time ago we made an
**	    assumption that 3-tier name space will not be used when processing
**	    SQL queries while running in FIPS mode.  That assumption proved to
**	    be incorrect and there is no longer a need to use this macro
**	17-aug-93 (andre)
**	    PSS_OBJ_NAME.pss_qualified has been replaced with
**	    PSS_OBJSPEC_EXPL_SCHEMA defined over PSS_OBJ_NAME.pss_objspec_flags
**	    PSS_OBJ_NAME.pss_sess_table has been replaced with
**	    PSS_OBJSPEC_SESS_SCHEMA defined over PSS_OBJ_NAME.pss_objspec_flags
**	26-Feb-2004 (schka24)
**	    Set nparts in the dmucb if needed.
**	15-Aug-2006 (jonj)
**	    Indexes on Temporary Base tables are now ok.
**	31-aug-06 (toumi01)
**	    Allow "session." to be optional when creating index for GTT.
**	19-Mar-2007 (kibro01) b117391
**	    Don't allow creation of indices on IMA GW tables
**	19-Jun-2010 (kiria01) b123951
**	    Add extra parameter to psl0_rngent for WITH support.
*/
DB_STATUS
psl_ci3_indexrel(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_OBJ_NAME	*tbl_spec)
{
    DB_ERROR	*err_blk = &psq_cb->psq_error;
    PSS_RNGTAB	*rngtab;
    i4		err_code;
    i4		qmode = psq_cb->psq_mode;
    DB_STATUS	status;
    i4		rngvar_info;
    DMU_CB	*dmu_cb;
    i4		mask, err_num = 0L;
    i4		mask2;
    bool	must_audit = FALSE;
    i4		tbls_to_lookup = (PSS_USRTBL | PSS_DBATBL | PSS_INGTBL);

    /*
    ** Get description for base table;
    */

    if (tbl_spec->pss_objspec_flags & PSS_OBJSPEC_EXPL_SCHEMA)
    {
	status = psl_orngent(&sess_cb->pss_auxrng, -1, "", &tbl_spec->pss_owner,
	    &tbl_spec->pss_obj_name, sess_cb, FALSE, &rngtab, PSQ_INDEX,
	    err_blk, &rngvar_info);
    }
    else
    {
	/* allow "session." to be optional when creating index for GTT */
	if (sess_cb->pss_ses_flag & PSS_GTT_SYNTAX_SHORTCUT)
	    tbls_to_lookup |= PSS_SESTBL;
	status = psl0_rngent(&sess_cb->pss_auxrng, -1, "",
	    &tbl_spec->pss_obj_name, sess_cb, FALSE, &rngtab, PSQ_INDEX,
	    err_blk, tbls_to_lookup, &rngvar_info, 0, NULL);

	if (DB_SUCCESS_MACRO(status) && !rngtab)
	{
            if (qmode == PSQ_REG_INDEX)
	        (VOID) psf_error(E_PS1106_IDX_NOPTBL, 0L, PSF_USERERR,
                            &err_code, err_blk, 1,
                            psf_trmwhite(sizeof(DB_TAB_NAME),
                                (char *) &tbl_spec->pss_obj_name),
			    &tbl_spec->pss_obj_name);
            else
		(VOID) psf_error(5300L, 0L, PSF_USERERR, &err_code,
		    	    err_blk, 1,
		    	    psf_trmwhite(sizeof(DB_TAB_NAME),
				(char *) &tbl_spec->pss_obj_name),
			    &tbl_spec->pss_obj_name);

	    status = E_DB_ERROR;
	}
    }

    if (DB_FAILURE_MACRO(status))
	return (status);

    sess_cb->pss_resrng = rngtab;

    mask = rngtab->pss_tabdesc->tbl_status_mask;
    mask2 = rngtab->pss_tabdesc->tbl_2_status_mask;

    /*
    ** user may not create an index on a table (not a catalog) owned by
    ** someone else, unless it's a Temp table, which is owned by
    ** the session (pss_sess_owner). The Temp table Index will also
    ** be owned by the session.
    */
    if ( !(rngtab->pss_tabdesc->tbl_temporary) &&
	    ~mask & DMT_CATALOG &&
	    MEcmp((PTR) &rngtab->pss_ownname, (PTR) &sess_cb->pss_user,
		   sizeof(DB_OWN_NAME)))
    {
	err_num = (sess_cb->pss_lang == DB_SQL) ? E_PS0424_ILLEGAL_TBL_REF
					       : 5303L;
	must_audit = TRUE;
    }

    /*
    ** We will prevent the user from creating an index if one of the
    ** following holds:
    **
    ** - catalog marked as S_CONCUR (core DBMS catalog)
    ** - table is a catalog, and the user has no permission to
    **   update catalogs (extended catalogs excluded).
    */
    else if (mask & DMT_CONCURRENCY
	     ||
	     ((mask & DMT_CATALOG || mask2 & DMT_TEXTENSION)
	      && ~mask & DMT_EXTENDED_CAT
	      && !(sess_cb->pss_ses_flag & PSS_CATUPD))
       )
    {
	err_num = E_PS0423_CRTIND_INSUF_PRIV;
	must_audit = TRUE;
    }
    /* Make sure base table isn't an index already */
    else if (mask & DMT_IDX)
    {
	err_num = 5304L;
    }
    /* Make sure base table isn't a view */
    else if (mask & DMT_VIEW)
    {
	err_num = 5306L;
    }
    /* Make sure base table isn't an IMA gateway (kibro01) b117391 */
    else if (rngtab->pss_tabdesc->tbl_relgwid == DMGW_IMA)
    {
	err_num = 9358L;
    }

    if (err_num)
    {
	status = E_DB_ERROR;

	if ( must_audit && Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
	{
	    /* Must audit INDEX failure. */
	    DB_STATUS	local_status;
	    DB_ERROR    e_error;

	    local_status = psy_secaudit(FALSE, sess_cb,
	    		(char *)&rngtab->pss_tabdesc->tbl_name,
			&rngtab->pss_ownname,
	    		sizeof(DB_TAB_NAME), SXF_E_TABLE,
	      		I_SX2026_TABLE_ACCESS, SXF_A_FAIL | SXF_A_INDEX,
	      		&e_error);

	    if (DB_FAILURE_MACRO(local_status) && local_status > status)
		status = local_status;
	}

	/*
	** let user know if name supplied by the user was resolved to a
	** synonym
	*/
	if (rngvar_info & PSS_BY_SYNONYM)
	{
	    psl_syn_info_msg(sess_cb, rngtab, tbl_spec, rngvar_info,
		sizeof("CREATE INDEX") - 1, "CREATE INDEX", err_blk);
	}

	if (err_num == E_PS0424_ILLEGAL_TBL_REF)
	{
	    _VOID_ psf_error(E_PS0424_ILLEGAL_TBL_REF, 0L, PSF_USERERR,
		&err_code, err_blk, 3,
		sizeof("CREATE/REGISTER INDEX") - 1, "CREATE/REGISTER INDEX",
		psf_trmwhite(sizeof(DB_TAB_NAME),
		    (char *) &rngtab->pss_tabname),
		&rngtab->pss_tabname,
		psf_trmwhite(sizeof(DB_OWN_NAME),
		    (char *) &rngtab->pss_ownname),
		&rngtab->pss_ownname);
	}
	else
	{
	    _VOID_ psf_error(err_num, 0L, PSF_USERERR, &err_code, err_blk, 1,
		psf_trmwhite(sizeof(DB_TAB_NAME),
		    (char *) &rngtab->pss_tabname),
		&rngtab->pss_tabname);
	}

	return(status);
    }

    dmu_cb = (DMU_CB *) ((QEU_CB *) sess_cb->pss_object)->qeu_d_cb;

    STRUCT_ASSIGN_MACRO(rngtab->pss_tabid, dmu_cb->dmu_tbl_id);

    /* it's unlikely that anyone will look at this for create index, but
    ** let's keep the control block as filled in as possible
    */
    dmu_cb->dmu_nphys_parts = rngtab->pss_tabdesc->tbl_nparts;

    /* schang : only code difference */
    /* Get the gateway id from base table. */
    if (qmode == PSQ_REG_INDEX)
	dmu_cb->dmu_gw_id = rngtab->pss_tabdesc->tbl_relgwid;
    return(E_DB_OK);
}

/*
** Name: psl_ci4_indexcol - semantic actions for indexcol: (SQL) and
**			    indexcolname: (QUEL)
**
** Description:
**	This routine performs the semantic actions for indexcol: (SQL) and
**	indexcolname: (QUEL)
**
**	indexcol:           NAME asc_desc
**	indexcolname:       NAME
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	colname		    Column name.
**
** Outputs:
**     err_blk		    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	5-mar-1991 (bryanp)
**	    Created.
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**	28-jun-93 (robf)
**	    Add option cond_add to only add a column if not there already.
**	    This is used to conditionally add a column internally.
*/
DB_STATUS
psl_ci4_indexcol(
	PSS_SESBLK	*sess_cb,
	char		*colname,
	DB_ERROR	*err_blk,
	bool		cond_add)
{
    DB_STATUS		status;
    i4             err_code;
    QEU_CB		*qeu_cb;
    DMU_CB		*dmu_cb;
    DMT_ATT_ENTRY	*attribute;
    DMU_KEY_ENTRY	**key;
    i4			i;
    i4			col_nmlen;

    /* Count index columns and give error if too many */
    if (sess_cb->pss_rsdmno == DB_MAXKEYS)
    {
	if (sess_cb->pss_lang == DB_QUEL)
	{
	    (VOID) psf_error(2111L, 0L, PSF_USERERR, &err_code,
		err_blk, 1, sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno);
	}
	else
	{
	    (VOID) psf_error(5328L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	}
	return (E_DB_ERROR);
    }

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    /* Make sure the column exists */
    col_nmlen = STlength(colname);
    attribute = pst_coldesc(sess_cb->pss_resrng, colname, col_nmlen);
    if (attribute == (DMT_ATT_ENTRY *) NULL)
    {
	_VOID_ psf_error(5302L, 0L, PSF_USERERR, &err_code,
	    err_blk, 1, STtrmwhite(colname), colname);
	return (E_DB_ERROR);
    }

    /*
    ** Check for duplicates and `tid' column names.
    */

    /* Is the name equal to "tid" */
    if (cui_compare(col_nmlen, colname, 
        3, ((*sess_cb->pss_dbxlate & CUI_ID_REG_U) ? "TID" : "tid")) == 0)
    {
	_VOID_ psf_error(2105L, 0L, PSF_USERERR, &err_code, err_blk, 2,
	    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
	    col_nmlen, colname);
	return (E_DB_ERROR);
    }

    key = (DMU_KEY_ENTRY **) dmu_cb->dmu_key_array.ptr_address;
    for (i = dmu_cb->dmu_key_array.ptr_in_count; i > 0; i--, key++)
    {
	if (cui_compare(col_nmlen, colname, 
		DB_ATT_MAXNAME, (*key)->key_attr_name.db_att_name) == 0)
	{
	    /*
	    ** Found already. Check "cond_add" option to determine
	    ** whether to error, or silently accept and ignore.
	    */
	    if(cond_add)
		return E_DB_OK;
	    else
	    {
	        _VOID_ psf_error(5307L, 0L, PSF_USERERR, &err_code,
	    	    err_blk, 1, STlength(colname), colname);
	        return (E_DB_ERROR);
	    }
	}
    }

    status = psl_check_key(sess_cb, err_blk, (DB_DT_ID) attribute->att_type);
    if (DB_FAILURE_MACRO(status))
    {
	_VOID_ psf_error(2180L, 0L, PSF_USERERR, &err_code, err_blk, 2,
	    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
	    col_nmlen, colname);
	return (status);
    }

    /* Store the column name in the DMU_CB key entry array */

    /*
    ** "key" already points to the address of the next entry, i.e.
    ** (DMU_KEY_ENTRY **) dmu_cb->dmu_key_array.ptr_address +
    **	dmu_cb->dmu_key_array.ptr_in_count
    */

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMU_KEY_ENTRY),
	(PTR *) key, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    cui_move(col_nmlen, colname, ' ', 
		DB_ATT_MAXNAME, (*key)->key_attr_name.db_att_name);
    (*key)->key_order = DMU_ASCENDING;

    /* SQL allows one to specify ASCENDING/DESCENDING but it is ignored in R6 */

    dmu_cb->dmu_key_array.ptr_in_count++;
    sess_cb->pss_rsdmno++;

    return (E_DB_OK);
}

/*
** Name: psl_ci5_indexlocname 	- semantic action for indexlocname: production
**
** Description:
**	This routine performs the semantic actions for indexlocname: production
**
**	indexlocname:       NAME COLON obj_spec
**		    |	    obj_spec			    (SQL)
**
**	indexlocname:       NAME COLON NAME
**		    |	    NAME			    (QUEL)
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	loc_name	    The location name (NULL if location was not
**			    specified)
**	index_spec	    structure describing index name as it was specified
**			    by the user
**
** Outputs:
**	err_blk		    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	5-mar-1991 (bryanp)
**	    Created.
**	20-may-92 (schang)
**	    GW integration.
**	17-aug-93 (andre)
**	    PSS_OBJ_NAME.pss_qualified has been replaced with
**	    PSS_OBJSPEC_EXPL_SCHEMA defined over PSS_OBJ_NAME.pss_objspec_flags
**	    PSS_OBJ_NAME.pss_sess_table has been replaced with
**	    PSS_OBJSPEC_SESS_SCHEMA defined over PSS_OBJ_NAME.pss_objspec_flags
**	23-jun-97 (nanpr01)
**	    Fixup the error parameters.
**	15-Aug-2006 (jonj)
**	    Indexes on TempTables are ok now.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Set indicator in DMU characteristics, not with-clauses.
*/
DB_STATUS
psl_ci5_indexlocname(
	PSS_SESBLK	*sess_cb,
	char		*loc_name,
	PSS_OBJ_NAME	*index_spec,
	PSQ_CB		*psq_cb)
{
    QEU_CB	    *qeu_cb;
    DMU_CB	    *dmu_cb;
    i4	    err_code;
    char	    *ch1 = index_spec->pss_obj_name.db_tab_name;
    char	    *ch2 = ch1 + CMbytecnt(ch1);
    char	    *qry;
    i4	    qry_len;
    DB_ERROR	    *err_blk = &psq_cb->psq_error;

    if (sess_cb->pss_lang == DB_SQL)
    {
	if (psq_cb->psq_mode == PSQ_REG_INDEX)
	{
	    qry = "REGISTER INDEX";
	    qry_len = sizeof("REGISTER INDEX") - 1;
	}
	{
	    qry	= "CREATE INDEX";
	    qry_len = sizeof("CREATE INDEX") - 1;
	}
    }
    else
    {
	qry	= "INDEX";
	qry_len = sizeof("INDEX") - 1;
    }

    /*
    ** can only create tables beginning with 'II' if running with
    ** system catalog update privilege.
    */
    if (!CMcmpcase(ch1, "i") && !CMcmpcase(ch2, "i") &&
	!(sess_cb->pss_ses_flag & PSS_CATUPD))
    {
	_VOID_ psf_error(5116L, 0L, PSF_USERERR, &err_code, err_blk, 3,
	    qry_len, qry,
	    STlength(index_spec->pss_orig_obj_name),
	    index_spec->pss_orig_obj_name, 
	    STlength(SystemCatPrefix), SystemCatPrefix);

	return (E_DB_ERROR);
    }

    /* If name was qualified, it'd better be the same as the user */
    if (index_spec->pss_objspec_flags & PSS_OBJSPEC_EXPL_SCHEMA &&
	MEcmp((PTR) &index_spec->pss_owner, (PTR) &sess_cb->pss_user,
	      sizeof(DB_OWN_NAME)))
    {
	_VOID_ psf_error(E_PS0421_CRTOBJ_NOT_OBJ_OWNER, 0L, PSF_USERERR,
	    &err_code, err_blk, 2,
	    qry_len, qry,
	    psf_trmwhite(sizeof(DB_OWN_NAME), (char *) &index_spec->pss_owner),
	    &index_spec->pss_owner);
	return(E_DB_ERROR);
    }

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    /* Give DMF location if it was specified */

    if (loc_name)
    {
	STmove(loc_name, ' ', sizeof(DB_LOC_NAME),
	    (char *) dmu_cb->dmu_location.data_address);
	dmu_cb->dmu_location.data_in_size = sizeof(DB_LOC_NAME);

	/* remember that location was specified */
	BTset(PSS_WC_LOCATION, dmu_cb->dmu_chars.dmu_indicators);
    }

    /* give DMF the table name */
    STRUCT_ASSIGN_MACRO(index_spec->pss_obj_name, dmu_cb->dmu_index_name);

    /* for error handling */
    STRUCT_ASSIGN_MACRO(index_spec->pss_obj_name, dmu_cb->dmu_table_name);

    return (E_DB_OK);
}
