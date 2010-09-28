/*
**Copyright (c) 2004 Ingres Corporation
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
**	psl_lst_elem		- Semantic action for index_lst_elem: and
**				  modopt_lst_elem: productions
**	psl_ci7_index_woword	- semantic action for index_woword production
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
**      10-Mar-2010 (thich01)
**          Allow GEOM family an exception for rtree indexing, but only for
**          rtrees.  Still disallow other index types.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      19-Jul-2010 (thich01)
**          Slight change to where dt family is checked for GEOM rtree.  This
**          seems to be a more reliable place to check, as it was failing
**          when it was in the if.
*/
		
static DB_STATUS psl_validate_rtree(
	PSS_SESBLK	*sess_cb,
	i4		qmode,
	PSS_WITH_CLAUSE *options,
	char		*qry,
	i4		qry_len,
	DB_ERROR	*err_blk);


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
**	with_clauses	    map of options specified with this statement
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
*/
DB_STATUS
psl_ci1_create_index(
	PSS_SESBLK	*sess_cb,
	PSS_WITH_CLAUSE *with_clauses,
	i4		unique,
	DB_ERROR	*err_blk)
{
    QEU_CB		    *qeu_cb;
    DMU_CB		    *dmu_cb;
    i4		    err_code;
    DMU_CHAR_ENTRY	    *chr, *chr_lim;
    DMU_CHAR_ENTRY	    *dcomp = (DMU_CHAR_ENTRY *) NULL;
    DMU_CHAR_ENTRY	    *icomp = (DMU_CHAR_ENTRY *) NULL;
    RDF_CB		    rdf_cb;
    i4			    minp = 0;
    i4			    maxp = 0;
    i4			    alloc = 0;
    i4		    sstruct = 0L;
    i4                      dim = 2;		/* default */
    DB_STATUS		    status;
    i4		    mask2=0;
    i4			    num_atts;
    i4			    i;
    DMT_ATT_ENTRY	    *att_entry;
    i4			    dcomptype = (i4)FALSE;
    DMU_KEY_ENTRY **key;
    DB_ATT_NAME attrname;

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

    chr = (DMU_CHAR_ENTRY *) dmu_cb->dmu_char_array.data_address;
    chr_lim =
	(DMU_CHAR_ENTRY *) ((char *) chr + dmu_cb->dmu_char_array.data_in_size);

    /*
    ** walk the characteristics array looking for entries whose validity must be
    ** verified; we are interested in minpages, maxpages, structure, data and
    ** index compression
    */

    for (; chr < chr_lim; chr++)
    {
	switch (chr->char_id)
	{
	    case DMU_ALLOCATION:
		alloc = chr->char_value;
		break;
	    case DMU_MINPAGES:
		minp = chr->char_value;
		break;
	    case DMU_MAXPAGES:
		maxp = chr->char_value;
		break;
	    case DMU_COMPRESSED:
		dcomp = chr;
		break;
	    case DMU_INDEX_COMP:
		icomp = chr;
		break;
	    case DMU_STRUCTURE:
		sstruct = chr->char_value;
		break;
	    case DMU_DIMENSION:
		dim = chr->char_value;
		break;
	    case DMU_UNIQUE:
		/* fix it up now if not set previously */
		chr->char_value = unique ? DMU_C_ON : DMU_C_OFF;
	    default:
		break;
	}
    }

    if (!sstruct && !dcomp && !icomp)
    {
	i4		compressed;

	/* Two characteristics needed */
	if (dmu_cb->dmu_char_array.data_in_size >=
	    (PSS_MAX_INDEX_CHARS - 1) * sizeof (DMU_CHAR_ENTRY))
	{
	    /* Invalid with clause - too many options */
	    _VOID_ psf_error(5327L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);
	}

	compressed = (sess_cb->pss_idxstruct < 0);

	/*
	** chr already points to where the next characteristic entry will go
	*/

	chr->char_id = DMU_COMPRESSED;
	chr->char_value = compressed ? DMU_C_ON : DMU_C_OFF;
	chr++;

	chr->char_id = DMU_STRUCTURE;
	sstruct = chr->char_value = compressed ? -sess_cb->pss_idxstruct
					       : sess_cb->pss_idxstruct;

	/*
	** At present (9.x), pss_idxstruct is only set by a possibly-unused
	** session modifier from the session connect message in SCF.
	** SCF validates that it's not heap or heapsort.  If any new
	** "set index_structure" statement is invented, it should check too.
	** Don't bother checking here.
	*/

	dmu_cb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY) * 2;
    }

    /*
    ** if knowledge of structure type was required to determine whether the data
    ** compression will take place, determine it now
    */
    if (dcomp && sstruct && dcomp->char_value == DMU_C_NOT_SET)
    {
	dcomp->char_value =
	    (sstruct == DB_HASH_STORE || sstruct == DB_ISAM_STORE)
		? DMU_C_ON : DMU_C_OFF;
    }

    /*
    ** if knowledge of structure type was required to determine whether the key
    ** compression will take place, determine it now
    */
    if (icomp && sstruct && icomp->char_value == DMU_C_NOT_SET)
    {
	icomp->char_value = (sstruct == DB_BTRE_STORE) ? DMU_C_ON : DMU_C_OFF;
    }

    if (dcomp)
    {
      switch (dcomp->char_value)
      {
        case DMU_C_ON: 
	  dcomptype = (i4)TRUE;
	  break;
        case DMU_C_HIGH:
	  dcomptype = (i4)DMU_C_HIGH;
	  break;
       /* dcomptype is initially set to false */
      }
    }

    /* verify that the specified combination of options was legal */
    /* Note that indexes can't be partitioned (yet) */
    /* Check for GEOM family and only allow rtrees */
    key = (DMU_KEY_ENTRY **)dmu_cb->dmu_key_array.ptr_address;
    STmove((char *) &(*key)->key_attr_name, ' ', sizeof(DB_ATT_NAME),
           (char *)&attrname);
    att_entry = pst_coldesc(sess_cb->pss_resrng, &attrname);
    if(adi_dtfamily_retrieve(att_entry->att_type) == DB_GEOM_TYPE && 
       sstruct != DB_RTRE_STORE )
    {
        //GEOM types can only be rtrees.
        _VOID_ psf_error(2180L, 0L, PSF_USERERR, &err_code, err_blk, 2,
                sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
                psf_trmwhite(DB_ATT_MAXNAME, (char *) &attrname), &attrname);
        return E_DB_ERROR;
    }
    status = psl_validate_options(sess_cb, PSQ_INDEX, with_clauses, sstruct,
	minp, maxp,
	dcomptype,
	(i4) ((icomp) ? (icomp->char_value == DMU_C_ON) : FALSE),
	alloc,
	err_blk);

    if (status != E_DB_OK)
	return(status);

    /* Check for base table security row label/audit attributes and
    ** set the same for the index
    */
    mask2 = sess_cb->pss_resrng->pss_tabdesc->tbl_2_status_mask;

    if(mask2&DMT_ROW_SEC_AUDIT)
    {
	/*
	** Mark index as having row security audit
	*/
	if (dmu_cb->dmu_char_array.data_in_size >=
	 (PSS_MAX_INDEX_CHARS - 1) * sizeof (DMU_CHAR_ENTRY))
	{
		    /* Invalid with clause - too many options */
		    _VOID_ psf_error(5327L, 0L, PSF_USERERR, &err_code, err_blk, 0);
		    return (E_DB_ERROR);
	}
	chr = (DMU_CHAR_ENTRY *) ((char *) dmu_cb->dmu_char_array.data_address
				   + dmu_cb->dmu_char_array.data_in_size);
	chr->char_id = DMU_ROW_SEC_AUDIT;
	chr->char_value = DMU_C_ON;
	chr++;
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
				att_entry->att_name.db_att_name,
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
	dmu_cb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
    }

    /* All indexes on partitioned tables are for now "GLOBAL" */
    if ( sess_cb->pss_resrng->pss_tabdesc->tbl_status_mask & DMT_IS_PARTITIONED )
    {
	if (dmu_cb->dmu_char_array.data_in_size >=
		    (PSS_MAX_INDEX_CHARS - 1) * sizeof (DMU_CHAR_ENTRY))
	{
		    /* Invalid with clause - too many options */
		    _VOID_ psf_error(5327L, 0L, PSF_USERERR, &err_code, err_blk, 0);
		    return (E_DB_ERROR);
	}
	chr = (DMU_CHAR_ENTRY *) ((char *) dmu_cb->dmu_char_array.data_address
				   + dmu_cb->dmu_char_array.data_in_size);
	chr->char_id = DMU_GLOBAL_INDEX;
	chr->char_value = DMU_C_ON;
	chr++;
	dmu_cb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
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
**	with_clauses	    Set to 0 to indicate no with clauses yet.
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
*/
DB_STATUS
psl_ci2_index_prefix(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_WITH_CLAUSE *with_clauses,
	i4		unique)
{
    i4         err_code;
    DB_STATUS	    status;
    QEU_CB	    *qeu_cb, *save_qeu_cb;
    DMU_CB	    *dmu_cb;
    DMU_CHAR_ENTRY  *chr;
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

    /* Fill in the DMU control block header */
    dmu_cb->q_next = (PTR) NULL;
    dmu_cb->q_prev = (PTR) NULL;
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

    /*
    ** Allocate the characteristics array for unique, structure, compressed.
    ** fillfactor, minpages, maxpages, indexfill, nonleaffill,
    ** index_compressed. Actually a few more entries will be allocated for
    ** safety. Indicate that no with clauses have yet been parsed by
    ** setting with_clauses to 0.
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	PSS_MAX_INDEX_CHARS * sizeof(DMU_CHAR_ENTRY),
	(PTR *) &dmu_cb->dmu_char_array.data_address, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    MEfill(sizeof(PSS_WITH_CLAUSE), 0, with_clauses);

    dmu_cb->dmu_range_cnt = 0;		/* Clear RTree items */
    dmu_cb->dmu_range = NULL;

    dmu_cb->dmu_char_array.data_in_size = sizeof(DMU_CHAR_ENTRY);
    chr = (DMU_CHAR_ENTRY *) dmu_cb->dmu_char_array.data_address;

    /* schang : find out if gateway or not by testing psq_cb-> */

    if (psq_cb->psq_mode == PSQ_REG_INDEX)
    {
	chr->char_id = DMU_GATEWAY;
        chr->char_value = DMU_C_ON;
        chr++;
	dmu_cb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
    }

    /* if PST_INFO flags are set, build special char entries.
    **
    ** (For CONSTRAINTS, QEF will build an Execute Immediate CREATE INDEX
    **  statement, and set bits in the PST_INFO structure to tell us
    **  special things about the index.)
    */
    if (psq_cb->psq_info != (PST_INFO *) NULL)
    {
	if (psq_cb->psq_info->pst_execflags & PST_SYSTEM_GENERATED)
	{
	    chr->char_id = DMU_SYSTEM_GENERATED;
	    chr->char_value = DMU_C_ON;
	    chr++;
	    dmu_cb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
	}

	if (psq_cb->psq_info->pst_execflags & PST_NOT_DROPPABLE)
	{
	    chr->char_id = DMU_NOT_DROPPABLE;
	    chr->char_value = DMU_C_ON;
	    chr++;
	    dmu_cb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
	}

	if (psq_cb->psq_info->pst_execflags & PST_SUPPORTS_CONSTRAINT)
	{
	    chr->char_id = DMU_SUPPORTS_CONSTRAINT;
	    chr->char_value = DMU_C_ON;
	    chr++;
	    dmu_cb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
	}

	if (psq_cb->psq_info->pst_execflags & PST_NOT_UNIQUE)
	{
	    chr->char_id = DMU_NOT_UNIQUE;
	    chr->char_value = DMU_C_ON;
	    chr++;
	    dmu_cb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
	}
    } /* end if psq_info != NULL */

    chr->char_id = DMU_UNIQUE;

    /* Fill in the UNIQUE info */
    chr->char_value = unique ? DMU_C_ON : DMU_C_OFF;

    /* store UNIQUE in with clause; used later for UNIQUE_SCOPE processing
     */
    if (unique)
	PSS_WC_SET_MACRO(PSS_WC_UNIQUE, with_clauses);

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
**	tbl_spec	    structure describing the relation name as it was
**			    entered by the user
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
	PSS_OBJ_NAME	*tbl_spec,
	DB_ERROR	*err_blk,
	i4		qmode)
{
    PSS_RNGTAB		*rngtab;
    i4		err_code;
    DB_STATUS		status;
    i4			rngvar_info;
    DMU_CB		*dmu_cb;
    i4		mask, err_num = 0L;
    i4             mask2;
    bool		must_audit = FALSE;
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
    DB_ATT_NAME		attname;
    DMT_ATT_ENTRY	*attribute;
    DMU_KEY_ENTRY	**key;
    i4			i;
    char		tid[DB_ATT_MAXNAME];

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
    STmove(colname, ' ', sizeof(DB_ATT_NAME), (char *) &attname);
    attribute = pst_coldesc(sess_cb->pss_resrng, &attname);
    if (attribute == (DMT_ATT_ENTRY *) NULL)
    {
	_VOID_ psf_error(5302L, 0L, PSF_USERERR, &err_code,
	    err_blk, 1, STtrmwhite(colname), colname);
	return (E_DB_ERROR);
    }

    /*
    ** Check for duplicates and `tid' column names.
    */

    /* Normalize the `tid' attribute name */
    STmove(((*sess_cb->pss_dbxlate & CUI_ID_REG_U) ? "TID" : "tid"),
	   ' ', DB_ATT_MAXNAME, tid);

    /* Is the name equal to "tid" */
    if (!MEcmp(attname.db_att_name, tid, DB_ATT_MAXNAME))
    {
	_VOID_ psf_error(2105L, 0L, PSF_USERERR, &err_code, err_blk, 2,
	    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
	    psf_trmwhite(DB_ATT_MAXNAME, (char *) &attname), &attname);
	return (E_DB_ERROR);
    }

    key = (DMU_KEY_ENTRY **) dmu_cb->dmu_key_array.ptr_address;
    for (i = dmu_cb->dmu_key_array.ptr_in_count; i > 0; i--, key++)
    {
	if (!MEcmp((char *) &attname, (char *) &(*key)->key_attr_name,
		sizeof(DB_ATT_NAME)))
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
        /* Allow GEOM family types to get past the check key as an exception
         * for rtree indexing. */
        DB_DT_ID dtfam = adi_dtfamily_retrieve(attribute->att_type);
        if (dtfam != DB_GEOM_TYPE)
        {
	    _VOID_ psf_error(2180L, 0L, PSF_USERERR, &err_code, err_blk, 2,
	        sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
	        psf_trmwhite(DB_ATT_MAXNAME, (char *) &attname), &attname);
	    return (status);
        }
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

    STRUCT_ASSIGN_MACRO(attname, (*key)->key_attr_name);
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
**	with_clauses	    PSS_WC_LOCATION will be set if location was
**			    specified
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
*/
DB_STATUS
psl_ci5_indexlocname(
	PSS_SESBLK	*sess_cb,
	char		*loc_name,
	PSS_OBJ_NAME	*index_spec,
	PSS_WITH_CLAUSE *with_clauses,
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
	PSS_WC_SET_MACRO(PSS_WC_LOCATION, with_clauses);
    }

    /* give DMF the table name */
    STRUCT_ASSIGN_MACRO(index_spec->pss_obj_name, dmu_cb->dmu_index_name);

    /* for error handling */
    STRUCT_ASSIGN_MACRO(index_spec->pss_obj_name, dmu_cb->dmu_table_name);

    return (E_DB_OK);
}

/*
** Name: psl_lst_elem	- semantic action for "table-like with clause list
**			  element" (twith_list_clist_elem) when not in a
**			  create-table-like context.  (i.e. create index,
**			  modify, ansi constraint with-clause.)
**
** Description:
**	This routine performs the semantic actions for index_lst_elem: and
**	modopt_lst_elem: productions
**
**	index_lst_elem:	    NAME
**	modopt_lst_elem:    NAME
**
** Inputs:
**	sess_cb			ptr to a PSF session CB
**	    pss_lang		query language
**	list_clause		Which type of list clause we're parsing.
**	    PSS_KEY_CLAUSE	WITH KEY = (keys) ([CREATE] INDEX only)
**	    PSS_NLOC_CLAUSE	WITH NEWLOCATION = (locs) (MODIFY only)
**				WITH LOCATION = (locs) (MODIFY, [CREATE] INDEX,
**				ANSI constraint definition)
**	    PSS_OLOC_CLAUSE	WITH OLDLOCATION = (locs)   (MODIFY only)
**	    PSS_COMP_CLAUSE	WITH COMPRESSION = ([no]key, [no|hi]data)
**				(MODIFY, [CREATE] INDEX)
**	element			The word which gives the with option.
**	qmode			query mode
**	    PSQ_INDEX		[CREATE] INDEX
**	    PSQ_CONS		ANSI constraint definition
**	    PSQ_MODIFY		MODIFY
**
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
**	08-feb-1995 (cohmi01)
**	For RAW/IO, element can now contain raw extent name as well, place
**	    into new rawextnm array in dmu_cb.
**	28-Aug-1997 (shero03)
**	    B85015: SEGV when range=((,),(,),page_size=... is specified
**	    Code incorrectly assumes it is processing a location clause.
**	30-mar-98 (inkdo01)
**	    Added WITH LOCATION for ANSI constraint definitions.
**	18-may-00 (inkdo01)
**	    Changes to fix 100756 (alter table add constraint).
**	28-Jan-2004 (schka24)
**	    Partition with-clause keeps its location data in yet a different
**	    place.
**	28-May-2009 (kschendel) b122118
**	    Simplify a bit.
**	17-Nov-2009 (kschendel) SIR 122890
**	    Revise call to pass yyvarsp directly, similar to the create-table
**	    variant (psl_ct8_cr_lst_elem).
*/
DB_STATUS
psl_lst_elem(
	PSS_SESBLK	*sess_cb,
	PSS_YYVARS	*yyvarsp,
	char		*element,
	i4		qmode,
	DB_ERROR	*err_blk)
{
    QEU_CB		*qeu_cb;
    DMU_CB		*dmu_cb;
    char	command[PSL_MAX_COMM_STRING];
    i4		err_code;
    i4		length;
    i4		list_clause;

    psl_command_string(qmode,sess_cb->pss_lang,command,&length);
    list_clause = yyvarsp->list_clause;
    if (qmode != PSQ_CONS)
    {
	qeu_cb = (QEU_CB *) sess_cb->pss_object;
	dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
    }
    else
    {
	qeu_cb = (QEU_CB *) NULL;
	dmu_cb = (DMU_CB *) NULL;
    }

    /* All the current list clauses that get here expect a name style
    ** element, not a quoted-string style element.
    */
    if (yyvarsp->id_type == PSS_ID_SCONST)
    {
	(void) psf_error(E_US1942_6460_EXPECTED_BUT, 0L, PSF_USERERR,
		&err_code, err_blk, 3,
		length, command,
		0, "A name",
		0, "a string constant");
	return (E_DB_ERROR);
    }

    if (list_clause == PSS_KEY_CLAUSE)
    {
	DB_ATT_NAME	    attname;
	DMU_KEY_ENTRY	    **key;
	i4		    colno;

	colno = dmu_cb->dmu_attr_array.ptr_in_count;

	/*
	** Number of keys must be <= number of columns.
	*/
	if ((colno + 1) > dmu_cb->dmu_key_array.ptr_in_count)
	{
	    (VOID) psf_error(5331L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);
	}

	key = (DMU_KEY_ENTRY **) dmu_cb->dmu_key_array.ptr_address;
	/*
	** Make sure the column exists in the column list in the right
	** order.
	*/
	STmove(element, ' ', sizeof(DB_ATT_NAME), (char *) &attname);

	if (MEcmp((PTR) &attname, (PTR) &(key[colno])->key_attr_name,
	    sizeof (DB_ATT_NAME)))
	{
	    (VOID) psf_error(5329L, 0L, PSF_USERERR, &err_code, err_blk, 2,
		psf_trmwhite((u_i4) sizeof(DB_TAB_NAME),
		(char *) &dmu_cb->dmu_index_name),
		(PTR) &dmu_cb->dmu_index_name,
		(i4) STlength(element), (PTR) element);
	    return (E_DB_ERROR);
	}
	dmu_cb->dmu_attr_array.ptr_in_count++;
    }
    else if (list_clause == PSS_COMP_CLAUSE)
    {
	DMU_CHAR_ENTRY	*chr, *chr_lim;
	bool		dcomp_chr = FALSE;
	bool		kcomp_chr = FALSE;

	/*
	** WITH COMPRESSION=([NO]KEY,[NO|HI]DATA)
	*/

	if (dmu_cb->dmu_char_array.data_in_size >=
	    PSS_MAX_INDEX_CHARS * sizeof (DMU_CHAR_ENTRY))
	{
	    /* Invalid with clause - too many options */
	    _VOID_ psf_error(5344L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		length, command);
	    return (E_DB_ERROR);
	}

	/*
	** Determine whether there are already any DMU_INDEX_COMP or
	** DMU_COMPRESSED characteristics built.
	*/
	chr = (DMU_CHAR_ENTRY *) dmu_cb->dmu_char_array.data_address;
	chr_lim = (DMU_CHAR_ENTRY *)
	    ((char *) chr + dmu_cb->dmu_char_array.data_in_size);

	for (; chr < chr_lim; chr++)
	{
	    if (chr->char_id == DMU_COMPRESSED)
	    {
		dcomp_chr = TRUE;
		if (kcomp_chr)
		    break;
	    }
	    else if (chr->char_id == DMU_INDEX_COMP)
	    {
		kcomp_chr = TRUE;
		if (dcomp_chr)
		    break;
	    }
	}

	/*
	** Validate the keyword, and add a new characteristic if valid.
	*/
	chr = chr_lim;

	if (!STcasecmp(element, "key"))
	{
	    chr->char_id = DMU_INDEX_COMP;
	    chr->char_value = DMU_C_ON;
	}
	else if (STcasecmp(element, "nokey") == 0)
	{
	    chr->char_id = DMU_INDEX_COMP;
	    chr->char_value = DMU_C_OFF;
	}
	else if (STcasecmp(element, "data") == 0)
	{
	    chr->char_id = DMU_COMPRESSED;
	    chr->char_value = DMU_C_ON;
	}
	else if (STcasecmp(element, "hidata") == 0)
	{
	    chr->char_id = DMU_COMPRESSED;
	    chr->char_value = DMU_C_HIGH;
	}
	else if (STcasecmp(element, "nodata") == 0)
	{
	    chr->char_id = DMU_COMPRESSED;
	    chr->char_value = DMU_C_OFF;
	}
	else
	{
	    (VOID) psf_error(E_PS0BC5_KEY_OR_DATA_ONLY, 0L, PSF_USERERR,
			&err_code, err_blk, 3,
			length, command,
			sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
			STlength(element), element);
	    return (E_DB_ERROR);
	}

	if (   (kcomp_chr && chr->char_id == DMU_INDEX_COMP)
	    || (dcomp_chr && chr->char_id == DMU_COMPRESSED))
	{
	    _VOID_ psf_error(E_PS0BC4_COMPRESSION_TWICE,
		    0L, PSF_USERERR, &err_code, err_blk, 2,
		    length, command,
		    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno);
	    return (E_DB_ERROR);
	}

	dmu_cb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
    }
    else if ((list_clause == PSS_NLOC_CLAUSE) ||
    	     (list_clause == PSS_OLOC_CLAUSE))
    {
	register DB_LOC_NAME	*loc, *lim;
	DM_DATA                 *locdesc;

	if (sess_cb->pss_stmt_flags & PSS_PARTITION_DEF)
	    locdesc = &yyvarsp->part_locs;
	else if (qmode == PSQ_CONS) 
	    locdesc = &sess_cb->pss_curcons->pss_restab.pst_resloc;
	else
	    locdesc = (list_clause == PSS_NLOC_CLAUSE) ? &dmu_cb->dmu_location
						   : &dmu_cb->dmu_olocation;
	/* See if there is space for 1 more */
	if (locdesc->data_in_size/sizeof(DB_LOC_NAME) >= DM_LOC_MAX)
	{
	    /* Too many locations */
	    (VOID) psf_error(2115L, 0L, PSF_USERERR,
		&err_code, err_blk, 1, sizeof(sess_cb->pss_lineno),
		&sess_cb->pss_lineno);
	    return (E_DB_ERROR);
	}

	lim = (DB_LOC_NAME *) (locdesc->data_address + locdesc->data_in_size);

	STmove(element, ' ', (u_i4) DB_LOC_MAXNAME, (char *) lim);
	locdesc->data_in_size += sizeof(DB_LOC_NAME);

	/* See if not a duplicate */
	for (loc = (DB_LOC_NAME *) locdesc->data_address; loc < lim; loc++)
	{
	    if (!MEcmp((char *) loc, (char *) lim, sizeof(DB_LOC_NAME)))
	    {
		/* A duplicate was found */
		(VOID) psf_error(2116L, 0L, PSF_USERERR, &err_code, err_blk, 2,
		    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
		    psf_trmwhite(DB_LOC_MAXNAME, element), element);
		return (E_DB_ERROR);
	    }
	}
    }
    else 
    {
       /* An invalid option found (B85015) */
       (VOID) psf_error(5342L, 0L, PSF_USERERR, &err_code, err_blk, 2,
		    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
		    psf_trmwhite(DB_MAXNAME, element), element);
       return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*
** Name: psl_lst_relem	- semantic action for index_relem: SQL production
**
** Description:
**	This routine performs the semantic actions for index_relem:
**	productions
**
**	index_relem:   	    signop I2CONST | signop I4CONST | signop DECCONST
**	                    signop F4CONST | signop F8CONST
**
** Inputs:
**	sess_cb			ptr to a PSF session CB
**	    pss_lang		query language
**	list_clause		Which type of list clause we're parsing.
**	    PSS_RANGE_CLAUSE	WITH RANGE = ((#,#),(#,#))
**	element			The word which gives the with option.
**	qmode			query mode
**	    PSQ_INDEX		[CREATE] INDEX
**
**
** Outputs:
**     err_blk		    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	05-may-1995 (shero03)
**	    Created.
*/
DB_STATUS
psl_lst_relem(
	PSS_SESBLK	*sess_cb,
	i4		list_clause,
	f8  		*element,
	i4		qmode,
	DB_ERROR	*err_blk)
{
    QEU_CB		*qeu_cb;
    DMU_CB		*dmu_cb;
    i4		err_code;
    DB_STATUS		status;
    char		*qry;
    i4		qry_len;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    qry	= "CREATE INDEX";
    qry_len = sizeof("CREATE INDEX") - 1;

    if (list_clause == PSS_RANGE_CLAUSE)
    {
	i4		rangeno;
	f8		tmp;

	rangeno = dmu_cb->dmu_range_cnt;

	/*
	** Number of ranges must be <= 2 * number of dimensions.
	*/
	if ((rangeno + 1) > 2 * DB_MAXRTREE_DIM)
	{
	    /* FIX ME put in a correct RTREE message */

	    (VOID) psf_error(5331L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);
	}

	F8ASSIGN_MACRO(*element, tmp);

	if (dmu_cb->dmu_range == NULL)
	{

    	    /*
    	    ** Allocate the range entries.  Allocate enough space for
    	    ** DB_MAXRTREE_DIM * 2, although it's probably
    	    ** fewer.  This is because we don't know how many dimensions we
    	    ** have at this point, and it would be a big pain to allocate less
    	    ** and then run into problems.
    	    */
    	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
				DB_MAXRTREE_DIM * 2 * sizeof(f8),
				(PTR *) &dmu_cb->dmu_range, err_blk);
    	    if (DB_FAILURE_MACRO(status))
		return (status);

	    rangeno = 0;

	}

	dmu_cb->dmu_range[rangeno] = tmp;
	dmu_cb->dmu_range_cnt++;
    }
    else
    {
	/* FIX ME put in a more correct error message */

	/* Unknown parameter */
	_VOID_ psf_error(E_PS0BC6_BAD_COMP_CLAUSE,
			0L, PSF_USERERR, &err_code, err_blk, 2,
			qry_len, qry,
			sizeof (sess_cb->pss_lineno), &sess_cb->pss_lineno);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*
** Name: psl_ci7_index_woword 	- semantic action for index_woword production
**
** Description:
**	This routine performs the semantic actions for index_woword production
**
**	index_woword:	    NAME
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	word		    The word which gives the with option.
**	with_clauses	    The current set of already parsed WITH clauses
**
** Outputs:
**	with_clauses	    Updated to reflect the WITH clauses specified.
**     err_blk		    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	5-mar-1991 (bryanp)
**	    Created.
**	10-dec-92 (rblumer)
**	    handle new WITH PERSISTENCE clause
**	15-may-1993 (rmuth)
**	    Add support for WITH CONCURRENT_ACCESS.
*/
DB_STATUS
psl_ci7_index_woword(
	PSS_SESBLK	*sess_cb,
	char		*word,
	PSS_WITH_CLAUSE *with_clauses,
	DB_ERROR	*err_blk)
{
    i4			err_code;
    QEU_CB		        *qeu_cb;
    DMU_CB		        *dmu_cb;
    DMU_CHAR_ENTRY		*kchr, *dchr;
    char			*qry;
    i4			qry_len;

    /* Note: no need to check for "in partition definition", done in the
    ** grammar actions since there are lots of places that single-word
    ** with keywords can end up.
    */

    if (sess_cb->pss_lang == DB_SQL)
    {
	qry	= "CREATE INDEX";
	qry_len = sizeof("CREATE INDEX") - 1;
    }
    else
    {
	qry	= "INDEX";
	qry_len = sizeof("INDEX") - 1;
    }

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    /*
    ** Handle WITH [NO]COMPRESSION
    */
    if (   (STcasecmp(word, "compression") == 0)
	|| (STcasecmp(word, "nocompression") == 0))
    {
	if (PSS_WC_TST_MACRO(PSS_WC_COMPRESSION, with_clauses))
	{
	    _VOID_ psf_error(E_PS0BC9_DUPLICATE_WITH_CLAUSE,
			     0L, PSF_USERERR, &err_code, err_blk, 3,
			     qry_len, qry,
			     sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
			     sizeof("COMPRESSION")-1, "COMPRESSION");
	    return (E_DB_ERROR);
	}

	PSS_WC_SET_MACRO(PSS_WC_COMPRESSION, with_clauses);

	/* check for characteristic overflow
	 */
	if (dmu_cb->dmu_char_array.data_in_size >=
	    (PSS_MAX_INDEX_CHARS - 1) * sizeof (DMU_CHAR_ENTRY))
	{
	    (VOID) psf_error(5327L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);
	}

	dchr = (DMU_CHAR_ENTRY *) ((char *) dmu_cb->dmu_char_array.data_address
				   + dmu_cb->dmu_char_array.data_in_size);
	kchr = dchr + 1;

	dchr->char_id = DMU_COMPRESSED;
	kchr->char_id = DMU_INDEX_COMP;

	if (!CMcmpcase(word, "c"))
	{
	    /*
	    ** Defer determining the semantics of the WITH COMPRESSION until the
	    ** end of the WITH-clause processing
	    */
	    dchr->char_value = kchr->char_value = DMU_C_NOT_SET;
	}
	else
	{
	    dchr->char_value = kchr->char_value = DMU_C_OFF;
	}

	dmu_cb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY) * 2L;

    } /* end if compression */
    else
	if (   (STcasecmp(word, "persistence")   == 0)
	    || (STcasecmp(word, "nopersistence") == 0))
    {
	/*
	** Handle WITH [NO]PERSISTENCE
	*/
	if (PSS_WC_TST_MACRO(PSS_WC_PERSISTENCE, with_clauses))
	{
	    _VOID_ psf_error(E_PS0BC9_DUPLICATE_WITH_CLAUSE,
			     0L, PSF_USERERR, &err_code, err_blk, 3,
			     qry_len, qry,
			     sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
			     sizeof(ERx("PERSISTENCE"))-1, ERx("PERSISTENCE"));
	    return (E_DB_ERROR);
	}

	PSS_WC_SET_MACRO(PSS_WC_PERSISTENCE, with_clauses);

	/* check for characteristic overflow
	 */
	if (dmu_cb->dmu_char_array.data_in_size >=
	    (PSS_MAX_INDEX_CHARS - 1) * sizeof (DMU_CHAR_ENTRY))
	{
	    (VOID) psf_error(5327L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);
	}

	dchr = (DMU_CHAR_ENTRY *) ((char *) dmu_cb->dmu_char_array.data_address
				   + dmu_cb->dmu_char_array.data_in_size);

	dchr->char_id = DMU_PERSISTS_OVER_MODIFIES;

	if (CMcmpcase(word, "p") == 0)
	{
	    dchr->char_value = DMU_C_ON;
	}
	else
	{
	    dchr->char_value = DMU_C_OFF;
	}

	dmu_cb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);

    } /* end persistence */
    else
	if ((STcasecmp(word, "concurrent_access")   == 0)
				||
	    (STcasecmp(word, "noconcurrent_access") == 0))
    {
	if (PSS_WC_TST_MACRO(PSS_WC_CONCURRENT_ACCESS, with_clauses))
	{
	    _VOID_ psf_error(E_PS0BC9_DUPLICATE_WITH_CLAUSE,
			     0L, PSF_USERERR, &err_code, err_blk, 3,
			     qry_len, qry,
			     sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
			     sizeof(ERx("CONCURRENT_ACCESS"))-1,
			     ERx("CONCURRENT_ACCESS"));
	    return (E_DB_ERROR);
	}

	PSS_WC_SET_MACRO(PSS_WC_CONCURRENT_ACCESS, with_clauses);

	dchr = (DMU_CHAR_ENTRY *) ((char *) dmu_cb->dmu_char_array.data_address
				   + dmu_cb->dmu_char_array.data_in_size);

	dchr->char_id = DMU_CONCURRENT_ACCESS;

	if (CMcmpcase(word, "c") == 0)
	{
	    dchr->char_value = DMU_C_ON;
	}
	else
	{
	    dchr->char_value = DMU_C_OFF;
	}

	dmu_cb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);

    } /* end concurrent_access */
    else
    {
	/* Unknown parameter */
	_VOID_ psf_error(E_PS0BC6_BAD_COMP_CLAUSE,
			0L, PSF_USERERR, &err_code, err_blk, 3,
			qry_len, qry,
			sizeof (sess_cb->pss_lineno), &sess_cb->pss_lineno,
			(i4) STtrmwhite(word), word);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*
** Name: psl_validate_options - verify that the specified combination of options
**				is legal
**
** Description:
**	examine options specified with [CREATE] INDEX or CREATE TABLE AS SELECT
**	statement to determine if they form a legal combination
**
** Inputs:
**	sess_cb			PSF session CB
**	qmode			query mode of the statement being parsed;
**				currently we are prepared to handle
**	    PSQ_INDEX		[CREATE] INDEX
**	    PSQ_RETINTO		CREATE TABLE AS SELECT
**	    PSQ_DGTT_AS_SELECT	DECLARE GLOBAL TEMPORARY TABLE AS SELECT
**	    PSQ_MODIFY		MODIFY
**	    PSQ_COPY		COPY
**	options			map of options specified; those of interest
**				include:
**	    PSS_WC_FILLFACTOR	DATAFILL was specified
**	    PSS_WC_LEAFFILL	LEAFFILL was specified
**	    PSS_WC_NONLEAFFILL	NONLEAFFILL was specified
**	    PSS_WC_UNIQUE	UNIQUE was specified
**	    PSS_WC_UNIQUE_SCOPE	statement-level UNIQUE_SCOPE was specified
**	    PSS_WC_PERSISTENCE	PERSISTENCE was turned on
**	sstruct			structure type
**	    0			structure was not specified (can happen for
**				CREATE TABLE AS SELECT or [CREATE] INDEX if the
**				structure was not specified but compression was)
**	    DB_ISAM_STORE	[c]isam
**	    DB_HASH_STORE	[c]hash
**	    DB_BTRE_STORE	[c]btree
**	    DB_RTRE_STORE	rtree
**	minp			minpages (0 if none was specified)
**	maxp			maxpages (0 if none was specified)
**	dcomp			TRUE if data compression was specified; FALSE
**				DMU_C_HIGH if high data compression specified
**				otherwise
**	icomp			TRUE if key compression was specified; FALSE
**				otherwise
**	sliced_alloc		ALLOCATION= value divided by # of partitions
**
** Output:
**	err_blk			filled in if an illegal combnination of options
**				is discovered
**
** Side effects:
**	none
**
** Returns:
**	E_DB_{OK,ERROR}
**
** History
**
**	14-nov-91 (andre)
**	    written
**	18-nov-91 (andre)
**	    adapted for use with MODIFY statement
**	10-dec-1992 (rblumer)
**	    don't allow changes to unique_scope and persistence
**	    if the index supports a constraint
**	26-jul-1993 (rmuth)
**	    Allow psl_validate_options to check COPY options.
**	14-may-1996 (shero03)
**	    Support HI compression
**	28-jun-1996 (shero03)
**	    Separate RTree validation into a separate routine
**	18-aug-1997 (nanpr01)
**	    Prevent segv while getting the table name.
**	29-Jan-2004 (schka24)
**	    Use psl-command-string instead of big if-then-else.
**	23-Nov-2005 (schka24)
**	    Validate allocation= against number of partitions.
**	03-Oct-2006 (jonj)
**	    Deferred validation of allocation against number of partitions for
**	    those qmodes that are partitionable; all others were checked
**	    in psl_nm_eq_no().
*/
DB_STATUS
psl_validate_options(
	PSS_SESBLK	*sess_cb,
	i4		qmode,
	PSS_WITH_CLAUSE *options,
	i4		sstruct,
	i4		minp,
	i4		maxp,
	i4		dcomp,
	i4		icomp,
	i4		sliced_alloc,
	DB_ERROR	*err_blk)
{
    i4	err_code;
    char qry[PSL_MAX_COMM_STRING];
    i4	qry_len;
    DB_STATUS	status;

    psl_command_string(qmode,sess_cb->pss_lang,qry,&qry_len);

    /* compression should not be specified if the structure was not */
    if (!sstruct && PSS_WC_TST_MACRO(PSS_WC_COMPRESSION, (char *)options))
    {
	_VOID_ psf_error(E_PS0BC8_COMP_NO_STRUCTURE, 0L, PSF_USERERR,
	    &err_code, err_blk, 2,
	    qry_len, qry,
	    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno);
	return (E_DB_ERROR);
    }

    /*
    ** Min or max pages requires hashed relation.
    ** Data fillfactor must use [c]isam, [c]hash, [c]btree (i.e. not for
    ** [c]heap)
    ** Key may not be specified for [c]heap (this may only happen with CREATE
    ** TABLE AS SELECT since [CREATE] INDEX does not allow one to specify heap
    ** structure)
    ** Leaffill and indexfill require btree.
    **
    ** compression option semantics depend on the storage structure.
    ** some storage structures don't support some compression options.
    */

    if (sstruct != DB_HASH_STORE)
    {
    	if (minp)
	{
	    _VOID_ psf_error(5333L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		qry_len, qry);
	    return (E_DB_ERROR);
	}

	if (maxp)
	{
	    _VOID_ psf_error(5334L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		qry_len, qry);
	    return (E_DB_ERROR);
	}
    }

    /*
    ** in CREATE TABLE AS SELECT if no structure was specified, default is HEAP
    */
    if (!sstruct || sstruct == DB_HEAP_STORE)
    {
	if (PSS_WC_TST_MACRO(PSS_WC_FILLFACTOR, (char *)options))
	{
	    _VOID_ psf_error(5337L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		qry_len, qry);
	    return (E_DB_ERROR);
	}

	if (PSS_WC_TST_MACRO(PSS_WC_KEY, (char *)options))
	{
	    _VOID_ psf_error(5339L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		qry_len, qry);
	    return (E_DB_ERROR);
	}

	/*
	** COPY into a heap table never re-builds the table so check
	** user has not specified options which are only used during
	** this process. May add more in the future so code accodingly.
	*/
	if ( qmode == PSQ_COPY )
	{
	    char	*clause;
	    i4	clause_len;
	    char	*table_type;
	    i4 table_type_len;

	    table_type     = "HEAP";
	    table_type_len = sizeof("HEAP") - 1;

	    if ( PSS_WC_TST_MACRO(PSS_WC_ALLOCATION, (char *)options) )
	    {
		clause     = "ALLOCATION";
		clause_len = sizeof("ALLOCATION") - 1;

		_VOID_ psf_error( 5347L, 0L, PSF_USERERR, &err_code, err_blk, 3,
				  clause_len, clause,
				  qry_len, qry,
				  table_type_len, table_type );
		return (E_DB_ERROR);

	    }
	}
    }

    /* Check allocation sliced by partitioning, if specified */
    if ( PSS_WC_TST_MACRO(PSS_WC_ALLOCATION, (char *)options) )
    {
	switch ( qmode )
	{
	    /* Things that are partitionable: */
	    case PSQ_RETINTO:
	    case PSQ_DGTT_AS_SELECT:
	    case PSQ_MODIFY:
	        {
		    i4 lo = 4, hi = DB_MAX_PAGENO;

		    if ( sliced_alloc < lo || sliced_alloc > hi )
		    {
			(void) psf_error(5341, 0, PSF_USERERR, &err_code, err_blk, 5,
				qry_len, qry, 0, "ALLOCATION",
				sizeof(i4), &sliced_alloc,
				sizeof(i4), &lo, sizeof(i4), &hi);
			return (E_DB_ERROR);
		    }
		    break;
		}

	    /* ...the others we already checked in psl_nm_eq_no() */
	    default:
		break;
	}
    }

    if ((sstruct != DB_BTRE_STORE) &&
	(sstruct != DB_RTRE_STORE) )
    {
	if (PSS_WC_TST_MACRO(PSS_WC_NONLEAFFILL, (char *)options))
	{
	    _VOID_ psf_error(5335L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		qry_len, qry);
	    return (E_DB_ERROR);
	}

	if (PSS_WC_TST_MACRO(PSS_WC_LEAFFILL, (char *)options))
	{
	    _VOID_ psf_error(5336L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		qry_len, qry);
	    return (E_DB_ERROR);
	}

    }

    /*
    ** with [CREATE] INDEX data compression may not be specified for btree or
    ** heap (of course, one cannot specify heap for index anyway)
    **
    ** with MODIFY, data compression may not be specified for btree secondary
    ** index (since neither heap nor sort may be specified for secondary index,
    ** this leaves only isam and hash)
    */
    if (   (qmode == PSQ_INDEX ||
            (qmode == PSQ_MODIFY &&
	     sess_cb->pss_resrng->pss_tabdesc->tbl_status_mask & DMT_IDX)
	   )
	&& dcomp && sstruct != DB_HASH_STORE && sstruct != DB_ISAM_STORE)
    {
	QEU_CB		*qeu_cb;
	DMU_CB		*dmu_cb;
	char		*tabname;
	i4		local_err;

	qeu_cb = (QEU_CB *) sess_cb->pss_object;
	dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
	tabname = (char *) &dmu_cb->dmu_table_name;

	if (sstruct == DB_BTRE_STORE)
	  local_err = E_PS0BC0_BTREE_INDEX_NO_DCOMP;
	else
	  local_err = E_PS0BCA_RTREE_INDEX_NO_COMP;
	_VOID_ psf_error(local_err,
	    0L, PSF_USERERR, &err_code, err_blk, 2,
	    qry_len, qry,
	    psf_trmwhite(sizeof(DB_TAB_NAME), tabname), tabname);
	return (E_DB_ERROR);
    }

    if (icomp && sstruct != DB_BTRE_STORE)
    {
	char		*tabname;
	QEU_CB		*qeu_cb;
	DMU_CB		*dmu_cb;
 
	qeu_cb = (QEU_CB *) sess_cb->pss_object;
	dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
	tabname = (char *)  dmu_cb->dmu_table_name.db_tab_name;

	_VOID_ psf_error(E_PS0BC1_KCOMP_BTREE_ONLY,
	    0L, PSF_USERERR, &err_code, err_blk, 2,
	    psf_trmwhite(sizeof(DB_TAB_NAME), tabname), tabname,
	    (i4) sizeof("COMPRESSION") - 1, "COMPRESSION");
	return (E_DB_ERROR);
    }

    if (minp && maxp && minp > maxp)
    {
	_VOID_ psf_error(5338L, 0L, PSF_USERERR, &err_code, err_blk, 2,
	    qry_len, qry, (i4) sizeof(minp), &minp);
	return (E_DB_ERROR);
    }
    
     if (sstruct == DB_RTRE_STORE)
     {

       status = psl_validate_rtree(sess_cb, qmode, options, 
       			qry, qry_len, err_blk);
       if (DB_FAILURE_MACRO(status))
       	 return (E_DB_ERROR);
     } 

    /* if an index/table supports a UNIQUE constraint, it must be
    ** specified with PERSISTENCE, and UNIQUE with UNIQUE_SCOPE = statement
    */
    if ((qmode == PSQ_MODIFY)
	&& (sess_cb->pss_resrng->pss_tabdesc->tbl_2_status_mask
	    & DMT_SUPPORTS_CONSTRAINT))
    {
	u_i4 local_err = 0;

	if (sess_cb->pss_resrng->pss_tabdesc->tbl_2_status_mask &
	     DMT_NOT_UNIQUE)
        {
	   if ( !(PSS_WC_TST_MACRO(PSS_WC_PERSISTENCE, (char *)options)))
	   {
	      local_err = E_PS0BB6_PERSISTENT_CONSTRAINT;
           }
        }
	else
	{
	   /*
	   ** You must specify WITH options UNIQUE, UNIQUE_SCOPE and PERSISTENCE
	   ** for an index that supports a unique constraint
	   */
	   if (!PSS_WC_TST_MACRO(PSS_WC_UNIQUE, (char *)options) || 
		!PSS_WC_TST_MACRO(PSS_WC_UNIQUE_SCOPE, (char *)options) ||
		!PSS_WC_TST_MACRO(PSS_WC_PERSISTENCE, (char *)options))
	   {
	      local_err = E_PS0BB4_CANT_CHANGE_UNIQUE_CONS;
           }
        }

	if (local_err)
	{
	    QEU_CB	*qeu_cb;
	    DMU_CB	*dmu_cb;
	    char	*tabname, *typename;

	    qeu_cb = (QEU_CB *) sess_cb->pss_object;
	    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
	    tabname = (char *)  dmu_cb->dmu_table_name.db_tab_name;

	    if (sess_cb->pss_resrng->pss_tabdesc->tbl_status_mask & DMT_IDX)
		typename = ERx("index");
	    else
		typename = ERx("table");

	    (void) psf_error(local_err, 0L, PSF_USERERR,
			     &err_code, err_blk, 3,
			     qry_len, qry,
			     5L, typename,
			     psf_trmwhite(DB_TAB_MAXNAME, tabname), tabname);
	    return (E_DB_ERROR);
	}

    }

    return (E_DB_OK);

}  /* end psl_validate_options */

/*
** Name: psl_validate_rtree - verify that the specified combination of options
**				is legal for an RTREE structure
**
** Description:
**	examine options specified with [CREATE] INDEX or CREATE TABLE AS SELECT
**	statement to determine if they form a legal combination
**
** Inputs:
**	sess_cb			PSF session CB
**	qmode			query mode of the statement being parsed;
**				currently we are prepared to handle
**	    PSQ_INDEX		[CREATE] INDEX
**	    PSQ_RETINTO		CREATE TABLE AS SELECT
**	    PSQ_DGTT_AS_SELECT	DECLARE GLOBAL TEMPORARY TABLE AS SELECT
**	    PSQ_MODIFY		MODIFY
**	    PSQ_COPY		COPY
**	options			map of options specified; those of interest
**				include:
**	    PSS_WC_UNIQUE	UNIQUE was specified
**	    PSS_WC_PARTITION	PARTITION was specified
**	qry			The name of the statement being parsed
**	qry_len			Length of qry
**
** Output:
**	err_blk			filled in if an illegal combnination of options
**				is discovered
**
** Side effects:
**	none
**
** Returns:
**	E_DB_{OK,ERROR}
**
** History
**
**	28-jun-1996 (shero03)
**	    written
**	16-apr-1997 (shero03)
**	    Use adi_dt_rtree to verify that all the proper functions exist
**	26-sep-1997 (shero03)
**	    B85904: Validate the range with the data type.  
**	29-Jan-2004 (schka24)
**	    No partitioned RTREE's.
*/
static DB_STATUS
psl_validate_rtree(
	PSS_SESBLK	*sess_cb,
	i4		qmode,
	PSS_WITH_CLAUSE *options,
	char		*qry,
	i4		qry_len,
	DB_ERROR	*err_blk)
{
    QEU_CB	    	*qeu_cb;
    DMU_CB	    	*dmu_cb;
    ADF_CB          	*adf_scb;
    char	    	*tabname;
    DMT_ATT_ENTRY   	*attr;
    DB_ATT_NAME		attrname;
    DMU_KEY_ENTRY	**key;
    ADI_DT_RTREE	rtree_blk;
    i4		err_code;
    i4			i, j;
    DB_STATUS		status;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
    adf_scb = (ADF_CB*) sess_cb->pss_adfcb;
    tabname = (char *) &dmu_cb->dmu_table_name;
	
    if ( (sess_cb->pss_rsdmno == 0) ||
         (sess_cb->pss_rsdmno > 1) )
    {
       (VOID) psf_error(E_PS0BCC_RTREE_INDEX_ONE_KEY,
		0L, PSF_USERERR, &err_code, err_blk, 2,
		qry_len, qry,
		psf_trmwhite(sizeof(DB_TAB_NAME),
		(char *) &dmu_cb->dmu_table_name),
		&dmu_cb->dmu_table_name);
       return (E_DB_ERROR);
    }

    /* Check for PARTITION, but allow NOPARTITION. */
    if (PSS_WC_TST_MACRO(PSS_WC_PARTITION, options)
      && dmu_cb->dmu_part_def != NULL)
    {
	(void) psf_error(E_US1932_6450_PARTITION_NOTALLOWED,
		0, PSF_USERERR, &err_code, err_blk, 2,
		qry_len, qry, sizeof("the RTREE structure")-1,"the RTREE structure");
	return (E_DB_ERROR);
    }
	
    if (PSS_WC_TST_MACRO(PSS_WC_UNIQUE, (char *)options))
    {
       (VOID) psf_error(E_PS0BCB_RTREE_INDEX_NO_UNIQUE,
		0L, PSF_USERERR, &err_code, err_blk, 2,
		qry_len, qry,
		psf_trmwhite(sizeof(DB_TAB_NAME),
		(char *) &dmu_cb->dmu_table_name),
		&dmu_cb->dmu_table_name);
       return (E_DB_ERROR);
    }
	
    if (!(PSS_WC_TST_MACRO(PSS_WC_RANGE, (char *)options)))
    {
       (VOID) psf_error(E_PS0BCD_RTREE_INDEX_NEED_RANGE,
		0L, PSF_USERERR, &err_code, err_blk, 2,
		qry_len, qry,
		psf_trmwhite(sizeof(DB_TAB_NAME),
		(char *) &dmu_cb->dmu_table_name),
		&dmu_cb->dmu_table_name);
       return (E_DB_ERROR);
    }
	
    /* Ensure that the key's data type has all appropiate functions */
    /* First, find the key's attribute by locating its name */
    key = (DMU_KEY_ENTRY **)dmu_cb->dmu_key_array.ptr_address;
    STmove((char *) &(*key)->key_attr_name, ' ',
           sizeof(DB_ATT_NAME), (char *)&attrname);
    attr = pst_coldesc(sess_cb->pss_resrng, &attrname); 
    status = adi_dt_rtree(adf_scb, attr->att_type, &rtree_blk);
    if (DB_FAILURE_MACRO(status))
    {
       	(VOID) psf_error(E_PS0BCE_RTREE_INDEX_BAD_KEY,
			0L, PSF_USERERR, &err_code, err_blk, 2,
			qry_len, qry,
			psf_trmwhite(sizeof(DB_TAB_NAME),
			(char *) &dmu_cb->dmu_table_name),
			&dmu_cb->dmu_table_name);
       return (E_DB_ERROR);
    }

    if (dmu_cb->dmu_range_cnt != rtree_blk.adi_dimension * 2)
    {
       	(VOID) psf_error(E_PS0BCF_RTREE_INDEX_BAD_RANGE,
			0L, PSF_USERERR, &err_code, err_blk, 2,
			qry_len, qry,
			psf_trmwhite(sizeof(DB_TAB_NAME),
			(char *) &dmu_cb->dmu_table_name),
			&dmu_cb->dmu_table_name);
       return (E_DB_ERROR);
    }

    for (i = 0, j = rtree_blk.adi_dimension; i < rtree_blk.adi_dimension; i++, j++)
    {
    	if (dmu_cb->dmu_range[i] >= dmu_cb->dmu_range[j])
        {
       	   (VOID) psf_error(E_PS0BCF_RTREE_INDEX_BAD_RANGE,
			0L, PSF_USERERR, &err_code, err_blk, 2,
			qry_len, qry,
			psf_trmwhite(sizeof(DB_TAB_NAME),
			(char *) &dmu_cb->dmu_table_name),
			&dmu_cb->dmu_table_name);
          return (E_DB_ERROR);
        }
    }

    return (E_DB_OK);

}  /* end psl_validate_rtree */
