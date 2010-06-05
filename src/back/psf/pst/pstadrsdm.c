/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <me.h>
#include    <st.h>
#include    <qu.h>
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
#include    <usererror.h>
#include    <psftrmwh.h>
#include    <cui.h>

/**
**
**  Name: PSTADRSDM.C - Functions to add PST_RSDM_NODEs to query trees.
**
**  Description:
**      This file contains the functions for creating PST_RSDM_NODEs, and
**	hooking them up to query trees.
**
**          pst_adresdom - Add a PST_RSDM_NODE to a query tree.
**
**
**  History:
**      27-mar-86 (jeff)    
**          adapted from addresdom in 3.0 parser
**	07-jun-88 (stec)
**	    Added to initialization of the resdom node.
**	15-feb-89 (stec)
**	    Correct error processing for cursor columns.
**	06-apr-89 (fred)
**	    Added processing to support (or, more correctly, to avoid) the
**	    sending of datavalues of user defined datatypes through the system.
**	18-apr-89 (jrb)
**	    pst_node interface change and db_prec cleanup
**	05-jun-89 (neil)
**	    06-apr fix: Confirm use of adi_dtinfo before call.
**	25-sep-89 (fred)
**	    Altered pst_adresdom() to coerce non-exportable data only of resdom
**	    is at level 1 (i.e. to be sent to the user)
**	10-oct-89 (mikem)
**	    Disallow updates/inserts when specified with values for 
**	    system_maintained attributes.
**	08-jan-92 (andre)
**	    broke off code responsible for "verifying" data types into a
**	    separate function so that it can be invoked from psl_p_tlist() when
**	    we translate the prototype tree for [<corr_name>.]<col_name> target
**	    list elements
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	04-mar-93 (rblumer)
**	    change parameter to psf_adf_error to be psq_error, not psq_cb.
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_template@]...
**/


/*{
** Name: pst_adresdom	- Add a PST_RSDM_NODE to the current query tree.
**
** Description:
**      This function creates a PST_RSDM_NODE, fills it in, and hooks it up 
**      to the current query tree.  It performs some checks; for instance, it
**	makes sure that the number of PST_RSDM_NODEs doesn't exceed
**	DB_MAX_COLS + 1 (one extra for the tid).
**
**	When creating a node for a target list, this function stores away the
**	address of the node.  This is used when we create the sort list; we'll
**	have a pointer to the target list available.
**
** Inputs:
**	attname				Name of result domain
**	left				Pointer to left sub-tree to hook up
**	right				Pointer to right sub-tree to hook up
**	cb				Pointer to session control block
**	    .pss_ostream		    For allocating new node
**	    .pss_tlist			    Place to put pointer to new target
**					    list
**	    .pss_usrrange.pss_rsrng	    The result range variable, for
**					    when we want a result domain to
**					    represent a column in a real table.
**	    .pss_rsdmno			    Used to count result columns
**	psq_cb				Pointer to caller control block
**	    .psq_error			    Place to put error info
**	    .psq_mode			    Query mode, to distinguish when we
**					    want the result domain to represent
**					    a column in a real table.
**	newnode				Place to put pointer to new node
**
** Outputs:
**      newnode                         Filled in with pointer to new node
**	cb
**	  .pss_tlist			Filled in with pointer to new node
**	  .pss_rsdmno			Might be incremented
**	psq_cb
**	  .psq_error			Might be filled in with error info
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	27-mar-86 (jeff)
**          Adapted from addresdom in 3.0 parser.
**	14-oct-86 (daved)
**	    use update colno for replace cursor instead of generating
**	    new resdom number. I don't understand why we would generate
**	    a new resdom number. This may fix or create a bug.
**	19-feb-87 (daved)
**	    init pst_print field
**	22-feb-87 (stec)
**	    Access appropriate range table depending on the query lang.
**	    Remove reference to pst_bydomorig.
**	06-nov-87 (stec)
**	    Attname in psf_error calls for 2100 must not be treated as string.
**	22-mar-88 (stec)
**	    Attname ia not a null-terminated string, but a normalized name.
**	07-jun-88 (stec)
**	    Added initialization for ntargno and ttargtype of the resdom node.
**	15-feb-89 (stec)
**	    Correct error processing for cursor columns. Remove call to
**	    psf_error with error code 2206. If cursor columns is not found,
**	    that means that it has not been declared for update, so issue 2207
**	    instead.
**	06-apr-89 (fred)
**	    Added support for user defined datatypes.  If an attempt is made to
**	    add a resdom for a datatype which is not transferable AND the query
**	    mode is retrieve or defcurs, then this routine will insert a
**	    coercion to the appropriate type between the resdom node and the
**	    `right' node.
**	05-jun-89 (neil)
**	    06-apr fix: Confirm use of adi_dtinfo before call.  Internally some
**	    nodes have their type resolved later (ie, aggs) and may enter here
**	    with DB_NODT as the type.
**	18-jul-89 (jrb)
**	    Filled in att_prec field properly.
**	25-sep-89 (fred)
**	    Altered to coerce non-exportable data only if resdom is at level 1,
**	    and therefore intended to be sent to the user.  This will be true
**	    when cb->pss_qualdepth == 1.
**	10-oct-89 (mikem)
**	    Disallow updates/inserts when specified with new values for 
**	    system_maintained attributes.
**	08-jan-92 (andre)
**	    broke off code responsible for "verifying" data types into a
**	    separate function so that it can be invoked from psl_p_tlist() when
**	    we translate the prototype tree for [<corr_name>.]<col_name> target
**	    list elements
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**	5-mar-2007 (dougi)
**	    Tidy up collation ID support.
**      29-Oct-2007 (hanal04) Bug 119368
**          If the front end does not have GCA i8 support then cast the
**          RESDOMs as i4s.
**      26-Nov-2009 (hanal04) Bug 122938
**          If the front end does not support the maximum precision
**          for decimals downgrade/convert to the supported maximum
**          precision level.
**       7-Dec-2009 (hanal04) Bug 123019, 122938
**          Adjust previous change for Bug 122938. Scale only needs to be
**          adjusted if it is greater than the new precision. This prevents
**          negative scale values seen in replicator testing.
**      25-Feb-2010 (hanal04) Bug 123291
**          Remove downgrade of decimal RESDOMs, old clients can display
**          precision in excess of 31, they just can't use them in ascii
**          copy.
*/
/*
** NOTE: in SQL grammar target_list of a subselect is processed BEFORE the
**	 from_list; consequently, data types of target list elements are not
**	 known when we build RESDOM nodes for the target list elements of form
**	 [<corr_name>.]<col_name>.  In psl_p_tlist(), we revisit the prototype
**	 tree and fill in the newly available information (type, length,
**	 precision, etc.)
**
**	 When making changes to pst_adresdom(), please take time to understand
**	 the effect these changes may have on the processing of prototype trees.
*/
DB_STATUS
pst_adresdom(
	char               *attname,
	PST_QNODE	   *left,
	PST_QNODE	   *right,
	PSS_SESBLK	   *cb,
	PSQ_CB		   *psq_cb,
	PST_QNODE	   **newnode)
{
    DB_STATUS           status;
    DMT_ATT_ENTRY	*coldesc;
    DMT_ATT_ENTRY	column;
    PSS_RNGTAB		*resrange;
    char		colname[sizeof(DB_ATT_NAME) + 1];   /* null term. */
    PST_RSDM_NODE	resdom;
    i4		err_code;
    PSC_RESCOL		*rescol;
    ADF_CB	*adf_scb;
    i2          null_adjust = 0;
    i4			temp_collID;

    /* Convert column name to a null-terminated string. */
    (VOID) MEcopy((PTR) attname, sizeof(DB_ATT_NAME), (PTR) colname);
    colname[sizeof(DB_ATT_NAME)] = '\0';
    (VOID) STtrmwhite(colname);

    /* For these operations, the result domain comes from the result table */
    if (psq_cb->psq_mode == PSQ_APPEND || psq_cb->psq_mode == PSQ_PROT)
    {
	/* Get the result range variable */
	if (psq_cb->psq_qlang == DB_SQL)
	{
	    resrange = &cb->pss_auxrng.pss_rsrng;
	}
	else
	{
	    resrange = &cb->pss_usrrange.pss_rsrng;
	}

	/* "tid" result column not allowed with these operations */
	if (!STcasecmp(((*cb->pss_dbxlate & CUI_ID_REG_U) ? "TID" : "tid"),
			colname ))
	{
	    psf_error(2100L, 0L, PSF_USERERR, &err_code, &psq_cb->psq_error, 4,
		(i4) sizeof(cb->pss_lineno), &cb->pss_lineno, 
		psf_trmwhite(sizeof(DB_TAB_NAME), 
		    (char *) &resrange->pss_tabname),
		&resrange->pss_tabname,
		psf_trmwhite(sizeof(DB_OWN_NAME), 
		    (char *) &resrange->pss_ownname),
		&resrange->pss_ownname,
		psf_trmwhite(sizeof(DB_ATT_NAME), attname), attname);
	    return (E_DB_ERROR);
	}

	/* Get the column description */
	coldesc = pst_coldesc(resrange, (DB_ATT_NAME *) attname);
	if (coldesc == (DMT_ATT_ENTRY *) NULL)
	{
	    psf_error(2100L, 0L, PSF_USERERR, &err_code, &psq_cb->psq_error, 4,
		(i4) sizeof(cb->pss_lineno), &cb->pss_lineno,
		psf_trmwhite(sizeof(DB_TAB_NAME), 
		    (char *) &resrange->pss_tabname),
		&resrange->pss_tabname,
		psf_trmwhite(sizeof(DB_OWN_NAME), 
		    (char *) &resrange->pss_ownname),
		&resrange->pss_ownname,
		psf_trmwhite(sizeof(DB_ATT_NAME), attname), attname);
	    return (E_DB_ERROR);
	}
	if (coldesc->att_flags & DMU_F_SYS_MAINTAINED)
	{
	    psf_error(E_US1900_6400_UPD_LOGKEY, 0L, PSF_USERERR, 
		&err_code, &psq_cb->psq_error, 4,
		(i4) sizeof(cb->pss_lineno), &cb->pss_lineno,
		psf_trmwhite(sizeof(DB_TAB_NAME), 
		    (char *) &resrange->pss_tabname),
		&resrange->pss_tabname,
		psf_trmwhite(sizeof(DB_OWN_NAME), 
		    (char *) &resrange->pss_ownname),
		&resrange->pss_ownname,
		psf_trmwhite(sizeof(DB_ATT_NAME), attname), attname);

	    return (E_DB_ERROR);
	}
    }
    else if (psq_cb->psq_mode == PSQ_REPLACE)
    {
	/*
	** For the "replace" command, use the result range variable that's
	** in the normal user range table, not the special slot that's
	** reserved for the result table in the append command.
	*/
	/* Get the result range variable */
	resrange = cb->pss_resrng;

	/* "tid" result column not allowed with these operations */
	if (!STcasecmp(((*cb->pss_dbxlate & CUI_ID_REG_U) ? "TID" : "tid"),
			colname))
	{
	    psf_error(2100L, 0L, PSF_USERERR, &err_code, &psq_cb->psq_error, 4,
		(i4) sizeof(cb->pss_lineno), &cb->pss_lineno,
		psf_trmwhite(sizeof(DB_TAB_NAME), 
		    (char *) &resrange->pss_tabname),
		&resrange->pss_tabname,
		psf_trmwhite(sizeof(DB_OWN_NAME), 
		    (char *) &resrange->pss_ownname),
		&resrange->pss_ownname,
		psf_trmwhite(sizeof(DB_ATT_NAME), attname), attname);
	    return (E_DB_ERROR);
	}

	/* Get the column description */
	coldesc = pst_coldesc(resrange, (DB_ATT_NAME *) attname);
	if (coldesc == (DMT_ATT_ENTRY *) NULL)
	{
	    psf_error(2100L, 0L, PSF_USERERR, &err_code, &psq_cb->psq_error, 4,
		(i4) sizeof(cb->pss_lineno), &cb->pss_lineno,
		psf_trmwhite(sizeof(DB_TAB_NAME), 
		    (char *) &resrange->pss_tabname),
		&resrange->pss_tabname,
		psf_trmwhite(sizeof(DB_OWN_NAME), 
		    (char *) &resrange->pss_ownname),
		&resrange->pss_ownname,
		psf_trmwhite(sizeof(DB_ATT_NAME), attname), attname);
	    return (E_DB_ERROR);
	}

	if (coldesc->att_flags & DMU_F_SYS_MAINTAINED)
	{
	    psf_error(E_US1900_6400_UPD_LOGKEY, 0L, PSF_USERERR, 
		&err_code, &psq_cb->psq_error, 4,
		(i4) sizeof(cb->pss_lineno), &cb->pss_lineno,
		psf_trmwhite(sizeof(DB_TAB_NAME), 
		    (char *) &resrange->pss_tabname),
		&resrange->pss_tabname,
		psf_trmwhite(sizeof(DB_OWN_NAME), 
		    (char *) &resrange->pss_ownname),
		&resrange->pss_ownname,
		psf_trmwhite(sizeof(DB_ATT_NAME), attname), attname);

	    return (E_DB_ERROR);
	}
    }
    else if (psq_cb->psq_mode == PSQ_REPCURS)
    {
	/*
	** For the "replace cursor" command, the info comes from the cursor 
	** control block. Cursor column list and update map should always 
	** specify same column set, so the second if statemnt (BTtest) could,
	** perhaps, be removed.
	*/
	rescol = psq_ccol(cb->pss_crsr, (DB_ATT_NAME *) attname);
	if (rescol == (PSC_RESCOL *) NULL)
	{
	    psf_error(2207L, 0L, PSF_USERERR, &err_code, &psq_cb->psq_error, 3,
		sizeof(cb->pss_lineno), &cb->pss_lineno,
		psf_trmwhite(DB_CURSOR_MAXNAME,
			cb->pss_crsr->psc_blkid.db_cur_name),
		cb->pss_crsr->psc_blkid.db_cur_name,
		psf_trmwhite(sizeof(DB_ATT_NAME), attname), attname);
	    return (E_DB_ERROR);
	}

	/* Make sure the column was declared "for update" */
	if (!BTtest((i4) rescol->psc_attid.db_att_id,
	    (char *) &cb->pss_crsr->psc_updmap))
	{
	    psf_error(2207L, 0L, PSF_USERERR, &err_code, &psq_cb->psq_error, 3,
		sizeof(cb->pss_lineno), &cb->pss_lineno,
		psf_trmwhite(DB_CURSOR_MAXNAME, 
			cb->pss_crsr->psc_blkid.db_cur_name),
		cb->pss_crsr->psc_blkid.db_cur_name,
		psf_trmwhite(sizeof(DB_ATT_NAME), attname), attname);
	    return (E_DB_ERROR);
	}

	/* Set up column descriptor */
	coldesc = &column;
	MEcopy((char *) attname, sizeof(DB_ATT_NAME),
	    (char *) &coldesc->att_name);

#ifdef NO
	/*
	** Count columns.  Give error if too many.  One extra for tid.
	*/
	cb->pss_rsdmno++;
	if (cb->pss_rsdmno > (DB_MAX_COLS + 1))
	{
	    psf_error(2130L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1, (i4) sizeof(cb->pss_lineno),
		&cb->pss_lineno);
	    return (E_DB_ERROR);
	}
	coldesc->att_number = cb->pss_rsdmno;
#endif

	coldesc->att_number	= rescol->psc_attid.db_att_id;
	coldesc->att_type	= rescol->psc_type;
	coldesc->att_width	= rescol->psc_len;
	coldesc->att_prec	= rescol->psc_prec;
	coldesc->att_collID	= -1;
	coldesc->att_geomtype = -1;
	coldesc->att_srid = -1;
    }
    else
    {
	/*
	** In all other cases, just take the datatype info
	** from the right child.
	*/
	coldesc = &column;
	MEcopy((char *) attname, sizeof(DB_ATT_NAME),
	    (char *) &coldesc->att_name);

	/*
	** Count columns.  Give error if too many.  One extra for tid.
	*/
	cb->pss_rsdmno++;
	if (cb->pss_rsdmno > (DB_MAX_COLS + 1))
	{
	    psf_error(2130L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1, (i4) sizeof(cb->pss_lineno),
		&cb->pss_lineno);
	    return (E_DB_ERROR);
	}
	coldesc->att_number = cb->pss_rsdmno;

	status = pst_rsdm_dt_resolve(right, coldesc, cb, psq_cb);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    /* Copy attribute information into PST_RSDM_NODE */
    resdom.pst_rsno = coldesc->att_number;
    /* The two fields below are initialized for a common case.
    ** They are context sensitive and in many cases may have to be
    ** modified by the caller of this routine.
    */
    resdom.pst_ntargno = resdom.pst_rsno;
    resdom.pst_ttargtype = PST_USER;
    resdom.pst_dmuflags  = 0;
    /* Don't bother with the conversion id for now */
    /* Not for update until we know otherwise */
    resdom.pst_rsupdt = FALSE;
    resdom.pst_rsflags = PST_RS_PRINT;
    MEcopy((char *) &coldesc->att_name, sizeof(DB_ATT_NAME),
	(char *) resdom.pst_rsname);
    temp_collID = coldesc->att_collID;

    /* If client can not handle i8 INTs downgrade to i4 */
    adf_scb = (ADF_CB *) cb->pss_adfcb;
    if ( !(adf_scb->adf_proto_level & AD_I8_PROTO) && (abs(coldesc->att_type) == DB_INT_TYPE) )
    {
        if(coldesc->att_type < 0)
        {
            null_adjust = 1;
        }
        if((coldesc->att_width - null_adjust) == sizeof(i8))
        {
            coldesc->att_width -= sizeof(i4);
        }
    }

    /* Now allocate the node */
    status = pst_node(cb, &cb->pss_ostream, left, right, PST_RESDOM,
	(char *) &resdom, sizeof(PST_RSDM_NODE), (DB_DT_ID) coldesc->att_type,
	(i2) coldesc->att_prec, (i4) coldesc->att_width, (DB_ANYTYPE *) NULL,
	newnode, &psq_cb->psq_error, (i4) 0);
    if (status != E_DB_OK)
    {
	return (status);
    }

    (*newnode)->pst_sym.pst_dataval.db_collID = temp_collID;
    /* Remember the last result domain produced */
    cb->pss_tlist = *newnode;
    return (E_DB_OK);
}

/*
** Name:    pst_rsdm_dt_resolve - determine type, length, precision to be stored
**				  in RESDOM node based on those found in its
**				  right child
**
** Description:
**	If the right child of RESDOM exists, copy the child's type, length, and
**	precision into the descriptor for the RESDOM node; if adi_dtinfo()
**	indicates that this type cannot be sent to FE, call adc_dbtoev() to
**	obtain an translate it into an "exportable" data type.
**	If the right child does not exist, then simply set type to DB_NODT and
**	length and precision to 0.
**
** Input:
**	right		    right child of the RESDOM node; NULL if none exists
**	cb		    PSF session CB
**	    pss_qualdepth   if set to 1, the resdom represents an element of a
**			    target list which will be returned to FE
**	psq_cb		    PSF request CB
**	    psq_mode	    mode of the query
**
** Output:
**	coldesc		    type/length,precision descriptor will be filled in
**	    att_type	    data type
**	    att_width	    length
**	    att_prec	    precision
**
** Side effects:
**	none
**
** Returns:
**	E_DB_{OK,ERROR}
**
** History:
**
**	08-jan-92 (andre)
**	    broke off from the body of pst_adresdom() to enable psl_p_tlist() to
**	    correctly process UDTs found in the target list
**	06-oct-92 (andre)
**	    we should not call adc_dbtoev() when processing INSERT ... SELECT.
**	    this can be determined by checking if 
**	    (!cb->pss_resrng || !cb->pss_resrng->pss_used ||
**	    cb->pss_resrng->pss_rgno < 0)
**	21-jun-93 (robf)
**	    we should be doing adc_dbtoev in QUEL retrieve (x.all), where
**	    pss_qualdepth is 0, so changed 
**		&& (cb->pss_qualdepth ==1)
**  	    to
**		&& (cb->pss_qualdepth <=1)
**	15-apr-05 (inkdo01)
**	    Add collation ID support.
**      29-Oct-2007 (hanal04) Bug 119368
**          If the front end does not have GCA i8 support then cast the
**          RESDOMs as i4s.
**      26-Nov-2009 (hanal04) Bug 122938
**          If the front end does not support the maximum precision
**          for decimals downgrade/convert to the supported maximum
**          precision level.
**       7-Dec-2009 (hanal04) Bug 123019, 122938
**          Adjust previous change for Bug 122938. Scale only needs to be
**          adjusted if it is greater than the new precision. This prevents
**          negative scale values seen in replicator testing.
**      25-Feb-2010 (hanal04) Bug 123291
**          Remove downgrade of decimal RESDOMs, old clients can display
**          precision in excess of 31, they just can't use them in ascii
**          copy.
*/
DB_STATUS
pst_rsdm_dt_resolve(
	PST_QNODE	*right,
	DMT_ATT_ENTRY	*coldesc,
	PSS_SESBLK	*cb,
	PSQ_CB		*psq_cb)
{
    DB_STATUS	    status;
    ADF_CB          *adf_scb;
    i2              null_adjust = 0;

    if (right != (PST_QNODE *) NULL)
    {
	coldesc->att_type = right->pst_sym.pst_dataval.db_datatype;
	coldesc->att_width = right->pst_sym.pst_dataval.db_length;
	coldesc->att_prec = right->pst_sym.pst_dataval.db_prec;
	coldesc->att_collID = right->pst_sym.pst_dataval.db_collID;
	/*
	** Internally some nodes have their type resolved later (ie, aggs)
	** and may enter here with DB_NODT as the type.  To avoid errors
	** from adi_dtinfo, we do not call if the type is DB_NODT.
	*/
	if (coldesc->att_type != DB_NODT)
	{
	    i4		dt_status;
	    ADF_CB	*adf_scb;

	    adf_scb = (ADF_CB *) cb->pss_adfcb;

	    status = adi_dtinfo(adf_scb, coldesc->att_type, &dt_status);
	    if (DB_FAILURE_MACRO(status))
	    {
		adf_scb->adf_errcb.ad_errclass = ADF_USER_ERROR;
		psf_adf_error(&adf_scb->adf_errcb, &psq_cb->psq_error, cb);
		return(status);
	    }

	    if (   (dt_status & (AD_NOEXPORT | AD_CONDEXPORT))
		&& ((psq_cb->psq_mode == PSQ_RETRIEVE) ||
		    (psq_cb->psq_mode == PSQ_DEFCURS))
		&& (cb->pss_qualdepth <=1)
		/* make sure we are not processing INSERT ... SELECT */
		&& (   !cb->pss_resrng	    
		    || !cb->pss_resrng->pss_used
		    || cb->pss_resrng->pss_rgno < 0
		   )
		)
	    {
		DB_DATA_VALUE		db_value;
		DB_DATA_VALUE		ev_value;

		db_value.db_datatype = coldesc->att_type;
		db_value.db_length = coldesc->att_width;
		db_value.db_prec = coldesc->att_prec;
		db_value.db_collID = coldesc->att_collID;
		status = adc_dbtoev(adf_scb, &db_value, &ev_value);
		if (DB_FAILURE_MACRO(status))
		{
		    adf_scb->adf_errcb.ad_errclass = ADF_USER_ERROR;
		    psf_adf_error(&adf_scb->adf_errcb, &psq_cb->psq_error, cb);
		    return(status);
		}
		
		coldesc->att_type = ev_value.db_datatype;
		coldesc->att_width = ev_value.db_length;
		coldesc->att_prec = ev_value.db_prec;
		coldesc->att_collID = ev_value.db_collID;
	    }
	} /* If valid data type */

        /* If client can not handle i8 INTs downgrade to i4 */
        if ( abs(coldesc->att_type) == DB_INT_TYPE) 
        {
            adf_scb = (ADF_CB *) cb->pss_adfcb;
            if ( !(adf_scb->adf_proto_level & AD_I8_PROTO))
            {
                if(coldesc->att_type < 0)
                {
                    null_adjust = 1;
                }
                if((coldesc->att_width - null_adjust) == sizeof(i8))
                {
                    coldesc->att_width -= sizeof(i4);
                }
            }
        }
    }
    else
    {
	coldesc->att_type = (DB_DT_ID) DB_NODT;
	coldesc->att_width = (i4) 0;
	coldesc->att_prec = (i4) 0;
	coldesc->att_collID = (i4) -1;
	coldesc->att_geomtype = (i2) -1;
	coldesc->att_srid = (i4) -1;
    }

    return(E_DB_OK);
}
