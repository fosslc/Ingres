/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>	/* needed for psfparse.h */
#include    <adf.h>	/* needed for psfparse.h */
#include    <ddb.h>	/* needed for psfparse.h */
#include    <ulf.h>	/* needed for qsf.h, rdf.h and pshparse.h */
#include    <qsf.h>	/* needed for psfparse.h and qefrcb.h */
#include    <qefrcb.h>	/* needed for psfparse.h */
#include    <psfparse.h>
#include    <dmtcb.h>	/* needed for rdf.h and pshparse.h */
#include    <qu.h>	    /* needed for pshparse.h */
#include    <rdf.h>	    /* needed for pshparse.h */
#include    <psfindep.h>    /* needed for pshparse.h */
#include    <pshparse.h>
#include    <er.h>
#include    <psftrmwh.h>
#include    <cui.h>

/*
** Name: PSLCDBP.C:	this file contains functions used in semantic actions
**			for the CREATE PROCEDURE (SQL) statement
**
** Description:
**	this file contains functions used by the SQL grammar in
**	parsing and subsequent processing of the ALTER TABLE statement
**
**	Routines:
**	    psl_cdbp_build_setrng  - create a dummy range variable to
**	                            represent a set-input parameter
**	    psl_fill_proc_params   - puts procedure parmeters into a form
** 			             that we can pass to RDF/QEF (via PSY).
**
**  History:
**      06-jan-93 (rblumer)    
**          created.
**	09-mar-93 (walt)
**	    Removed external declaration of MEmove().  It's declared in
**	    me.h and this declaration causes a conflict if it's prototyped
**	    in me.h and not here.
**	09-mar-93 (rblumer)
**	    Added psl_fill_proc_params to fill in parameters for set-input
**	    dbprocs; these parameters are passed directly to RDF in PSY.
**	    renamed DB_DEF_ID_OBJKEY to DB_DEF_ID_LOGKEY.
**	19-mar-93 (rblumer)
**	    when recreating a dbproc, need to copy the set-parameter name into
**	    the rngvar, as otherwise the table name is "<temporary table>".
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	19-jul-93 (rblumer)
**	    in psl_init_attrs, fix typo when setting up tid column;
**	    was using subscript of 'i' instead of DB_IMTID.
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	22-oct-93 (rblumer)
**	    updated psl_fill_proc_params to be called for either set-input
**	    procedures or normal single-valued procedures.
**	    Also, pss_setparm* variables have moved from the sess_cb to the
**	    dbpinfo block (affects several routines).
**	29-nov-93 (rblumer)
**	    In psl_fill_proc_params, we will use the text stream (pss_tstream)
**	    instead of the ostream to allocate the procedure parameter tuples.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout().
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
*/


/* external functions that don't have prototypes
 */


/* function prototypes for static functions
 */
static DB_STATUS    psl_init_attrs(
				   PSS_SESBLK	 *sess_cb,
				   PSS_DBPINFO	 *dbpinfo,
				   DMT_ATT_ENTRY ***attsp,
				   i4	 *num_atts,
				   i4	 *tot_length,
				   DB_ERROR	 *err_blk);
static DB_STATUS    psl_init_rdrinfo(
				     PSS_SESBLK	  *sess_cb,
				     PSS_DBPINFO  *dbpinfo,
				     RDR_INFO	  **rdr_infop,
				     DB_ERROR	  *err_blk);

static DB_STATUS    psl_build_atthsh(
				     PSS_SESBLK	*sess_cb,
				     i4	num_atts,
				     RDR_INFO	*rdr_info,
				     DB_ERROR	*err_blk);
static i4	    psl_rdf_hash(
				 DB_ATT_NAME *colname);




/*
** Name: psl_cdbp_build_setrng  - make a (possible dummy) range table 
**                                representing a set-input parameter
**
** Description:	    
**	If PSS_RECREATE is set, the temp table implementing the set
**	parameter has already been created by QEF.  In that case, we call
**	pst_showtab with the tab_id (just like other places in PSF) to get
**	the range entry.  This query WILL eventually go to OPF.
**
**	If PSS_RECREATE is not set, the query will NOT go to OPF, but just
**	put the procedure description in the system catalogs.  In that
**	case, we only need to create a dummy range table for PSF to use.
**	(This range table is needed because the set-parameter can be
**	accessed in SELECT, UPDATE, etc statements inside the procedure).
**	PSF is mainly interested in the attribute names and types, so all
**	we set up is the table descriptor, the attribute descriptor, and
**	the attribute hash table.  Plus a few other minor fields.
**
** Inputs:
**	sess_cb		    ptr to PSF session CB (passed to other routines)
**	    pss_ostream	           ptr to mem stream for allocating structures
**	    pss_dbp_flags          check if PSS_RECREATE is set
**	             (the following are only used if not RECREATEing procedure)
**	psq_cb		    ptr to parser control block
**	    qmode		   query mode
**	    psq_set_input_tabid    id of temp table representing set parameter
**	dbpinfo		    ptr to YACC's dbproc info block
**			    (passed to other routines)
**	    pss_setparmname	name of set-input parameter
**	    pss_setparmno	number of columns in set parameter
**	    pss_setparmq	queue of column descriptions for set parameter
**
** Outputs:
**	dbprngp		    filled in with RNGTAB if
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	05-jan-93 (rblumer)
**	    written
**	19-mar-93 (rblumer)
**	    when recreating a dbproc, need to copy the set-paramter name into
**	    the rngvar, as otherwise the table name is "<temporary table>".
**	22-oct-93 (rblumer)
**	    now get setparmname from dbpinfo instead of sess_cb.
**    20-Nov-08 (wanfr01)
**        Bug 121252
**        Do not attempt to reuse the set-input table parameter for grant
**        statements because it doesn't exist.
**
*/
DB_STATUS
psl_cdbp_build_setrng(
		      PSS_SESBLK    *sess_cb,
		      PSQ_CB	    *psq_cb,
		      PSS_DBPINFO   *dbpinfo, 
		      PSS_RNGTAB    **dbprngp)
{
    PSS_RNGTAB	*rngvar;
    DB_TAB_ID	set_tabid;
    DB_STATUS	status;
    DB_ERROR	*err_blk = &psq_cb->psq_error;

    *dbprngp = (PSS_RNGTAB *) NULL;

    /* allocate space for range table
     */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(PSS_RNGTAB), (PTR *) dbprngp, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    MEfill(sizeof(PSS_RNGTAB), (u_char) 0, (PTR) *dbprngp);

    rngvar = *dbprngp;
    
    /* initialize non-zero fields
     */
    QUinit(&rngvar->pss_rngque);
    rngvar->pss_rgtype = PST_SETINPUT;


    /* If are recreating the procedure (for QEF), the query tree will go
    ** to OPF to be optimized.  Since the temp table representing the set-input
    ** parameter already exists at this point, we can get accurate info about
    ** it from RDF via pst_showtab.
    **
    ** (PSS_RECREATE | PSS_PARSING_GRANT) indicates the procedure is NOT
    ** being recreated - it is just being parsed for grants.
    */
    if ((sess_cb->pss_dbp_flags & PSS_RECREATE) &&
      (!(sess_cb->pss_dbp_flags & PSS_PARSING_GRANT)))
    {
	/* get description using temp table id passed from QEF
	 */ 
	STRUCT_ASSIGN_MACRO(psq_cb->psq_set_input_tabid, set_tabid);
	
	status = pst_showtab(sess_cb, PST_SHWID, 
			     (DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL, 
			     &set_tabid, FALSE, 
			     rngvar, psq_cb->psq_mode, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	
	/* copy the set name as the table name,
	** since RDF/DMF returns the name "<Temporary Table>" instead.
	*/
	MEcopy((PTR) &dbpinfo->pss_setparmname->pss_varname,
	       sizeof(DB_TAB_NAME), (PTR) &rngvar->pss_tabname);

	return(E_DB_OK);
    }

    /*
    ** Else we are NOT recreating the procedure, so it is being created for
    ** the first time and will not be sent to OPF.  In that case, we can just
    ** create a dummy range table entry with table and attribute information
    ** (using information about the set parameter stored in dbpinfo).
    ** Only need to fill in fields describing the table and its attributes,
    ** as that is all PSF needs to look at.
    */
    
    /* copy the set name as the table name
    ** 
    **  NOTE: we are leaving the owner/schema name NULL for now, as the
    **        set-input table doesn't really belong to any schema.
    **        When we allow users to create set-input dbprocs, we will have to
    **        come up with a different scheme in order to resolve ambiguities
    **        that could arise if the user has a regular table of the same name
    **        as the set-input parameter.
    */
    MEcopy((PTR) &dbpinfo->pss_setparmname->pss_varname,
	   sizeof(DB_TAB_NAME), (PTR) &rngvar->pss_tabname);

    /* 
    ** allocate and initialize the rdr_info block
    */
    status = psl_init_rdrinfo(sess_cb, dbpinfo, 
			      &rngvar->pss_rdrinfo, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* set up table descriptor, attribute descriptors,
    ** and hash table from rdr_info
    */
    rngvar->pss_tabdesc = rngvar->pss_rdrinfo->rdr_rel;
    rngvar->pss_attdesc = rngvar->pss_rdrinfo->rdr_attr;
    rngvar->pss_colhsh  = rngvar->pss_rdrinfo->rdr_atthsh;

    return(E_DB_OK);

}  /* end psl_cdbp_build_setrng */


/*
** Name: psl_fill_proc_params -puts procedure parmeters into the structure used
** 			       to pass the parameters to RDF/QEF (via PSY).
**
** Description:
**	Converts a list of PSS_DECVARS into a QEF_DATA list consisting of
**	DB_PROCEDURE_PARAMETER tuples.
**	
**	Allocates arrays of DB_PROCEDURE_PARAMETER tuples and QEF_DATA
**	structures in which to store the parameter descriptions.  The tuples
**	are put into a linked list of QEF_DATA pointers, which RDF will pass
**	directly to QEF (so that QEF can append the tuples to the procedure
**	parameter catalog).
**
**	The memory stream pss_tstream is used to store these tuples, since this
**	stream is already passed to PSY, and so will be available when the
**	tuples are copied to the RDF control block.  We assume that pss_tstream
**	is already OPEN but unlocked, and so we LOCK it before using it and
**	UNLOCK it before returning.
**	(Previously, pss_ostream was being used for these tuples. That worked
**	for set-input procedures, since they don't go through OPF on creation;
**	but for normal procedures [which go through OPF], pss_ostream is freed
**	before PSY is called and the tuples can get overwritten before PSY uses
**	them.)
**
** Input:
**	sess_cb		    ptr to PSF session CB
**	    pss_tstream		ptr to mem stream for allocating QEF_DATA nodes
**	num_params	    number of parameters to procedure
**	parmq		    header to queue of PSS_DECVARs describing params
**	num_rescols	    number of result row columns in procedure
**	rescolq		    header to queue of PSS_DECVARS describing result row
**				columns
**	set_input_proc	    TRUE if the procedure is a set-input procedure
**
** Output:
**	sess_cb		    ptr to PSF session CB
**	    pss_parmlist	QEF_DATA list of DB_PROCEDURE_PARAMETER tuples
**	err_blk		filled in if an error is encountered
**
** Returns:
**	E_DB_{OK,ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**	09-mar-93 (rblumer)
**	    written
**	22-oct-93 (rblumer)
**	    updated to be called for either set-input procedures or normal
**	    single-valued procedures.  This included adding num_params, parmq,
**	    and set_input_proc parameters, and changing code to use them rather
**	    than the hard-coded setparm variables.
**	29-nov-93 (rblumer)
**	    since normal procedures go through OPF which frees pss_ostream, the
**	    memory pointed to by pss_ostream will be freed by the time PSY gets
**	    called to pass the parameters to RDF.   Thus PSY will be passing
**	    freed memory to QEF, which could get bad data if someone else has
**	    allocated and overwritten that memory.
**	    So we will use the text stream (pss_tstream) instead of pss_ostream
**	    to allocate the procedure parameter tuples.
**	7-june-06 (dougi)
**	    Set dbpp_flags with IN/OUT/INOUT.
**	03-aug-2006 (gupsh01)
**	    Added support for ANSI date/time types.
**	26-march-2008 (dougi)
**	    Add support for result row columns.
**	27-july-2008 (gupsh01)
**	    Fix the correct default values for Ansi
**	    Date/time types. The default value for
**	    these types should be current date/time.
*/
DB_STATUS
psl_fill_proc_params(
		     PSS_SESBLK *sess_cb, 
		     i4	num_params,
		     QUEUE	*parmq,
		     i4 num_rescols,
		     QUEUE	*rescolq,
		     i4	set_input_proc,
		     DB_ERROR	*err_blk)
{
    DB_PROCEDURE_PARAMETER  *pptuple_arr;
    QEF_DATA		    *qef_data_arr;
    DB_PROCEDURE_PARAMETER  *pptuple;
    QEF_DATA		    *qef_dptr;
    PSS_DECVAR	*parm;
    DB_STATUS	status;
    i4	err_code;
    i4		i, offset;
    i4		totrows = num_params + num_rescols;
    
    if (totrows == 0)
    {
	sess_cb->pss_procparmlist = (QEF_DATA *) NULL;
	return(E_DB_OK);
    }

    /* lock the memory stream so others can't conflict with us
     */
    status = psf_mlock(sess_cb, &sess_cb->pss_tstream, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    /* allocate space for the proc parameter tuples
     */
    status = psf_malloc(sess_cb, &sess_cb->pss_tstream, 
			totrows * sizeof(DB_PROCEDURE_PARAMETER), 
			(PTR *) &pptuple_arr, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    MEfill(totrows * sizeof(DB_PROCEDURE_PARAMETER), 
	   (u_char) 0, (PTR) pptuple_arr);

    /* allocate space for the QEF_DATA pointers
     */
    status = psf_malloc(sess_cb, &sess_cb->pss_tstream, 
			totrows * sizeof(QEF_DATA), 
			(PTR *) &qef_data_arr, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    MEfill(totrows * sizeof(QEF_DATA), (u_char) 0, (PTR) qef_data_arr);

    parm = (PSS_DECVAR *) parmq->q_prev;
    if (num_params <= 0 && num_rescols > 0)
	parm = (PSS_DECVAR *)rescolq->q_prev;

    /* if not a set-input parameter, need to skip the first two vars in the
    ** pararameter queue, as these are the builtin 'ii' local variables.
    ** In either case, we have to ignore the dummy queue header.
    */
    if (!set_input_proc && num_params > 0)
    {
	for (i = 0;  i <= PST_DBPBUILTIN; i++)
	    parm = (PSS_DECVAR *) parm->pss_queue.q_prev;
    }
    
    /* for each parameter, set up an iiprocedure_parameter tuple,
    ** stick it into a QEF_DATA structure, link that into the list of tuples
    ** (note that parameters are linked in reverse order on the setparmq)
    */
    offset = 0;
    
    for (i = 1;
	 i <= totrows;
	 i++,   parm = (PSS_DECVAR *) parm->pss_queue.q_prev)
    {
	if (parm == (PSS_DECVAR *) parmq) /* || parm == (PSS_DECVAR *) rescolq) */
	{
	    /* should never happen */
	    (VOID) psf_error(E_PS0002_INTERNAL_ERROR, 0L, 
			     PSF_INTERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);
	}	    

	/* (This should never happen, but ... just in case)
	** vars with scope greater than 0 are local variables,
	** not parameters.  And when we hit the first local variable,
	** we know there are no more parameters.
	*/
	if (parm->pss_scope > 0)
	{
	    num_params = i-1;
	    qef_dptr->dt_next = (QEF_DATA *) NULL;
	    break;
	}

	pptuple = &pptuple_arr[i-1];
	
	MEcopy((PTR) &parm->pss_varname,
	       sizeof(DB_ATT_NAME), (PTR) &pptuple->dbpp_name);

	pptuple->dbpp_number    = i;
	pptuple->dbpp_datatype  = parm->pss_dbdata.db_datatype;
	pptuple->dbpp_length    = parm->pss_dbdata.db_length;
	pptuple->dbpp_precision = parm->pss_dbdata.db_prec;
	pptuple->dbpp_offset    = offset;
	pptuple->dbpp_flags     = parm->pss_default ? 0 : DBPP_NDEFAULT;

	if (pptuple->dbpp_datatype > 0)
	    pptuple->dbpp_flags |= DBPP_NOT_NULLABLE;

	if (i > num_params)
	{
	    pptuple->dbpp_flags |= DBPP_RESULT_COL;
	    pptuple->dbpp_number -= num_params;		/* reset rescol no */
	}

	if (parm->pss_flags & PSS_PPOUT)
	    pptuple->dbpp_flags |= DBPP_OUT;
	else if (parm->pss_flags & PSS_PPINOUT)
	    pptuple->dbpp_flags |= DBPP_INOUT;
	else pptuple->dbpp_flags |= DBPP_IN;

	if (!parm->pss_default)
	{
	    /* not default */
	    SET_CANON_DEF_ID(pptuple->dbpp_defaultID, DB_DEF_NOT_DEFAULT);
	}
	else
	{
	    /* param has an INGRES default,
	    ** so have to figure out which canon defid to use
	    **
	    ** NOTE: user-defined defaults for dbprocs aren't implemented in 6.5
	    */
	    if (pptuple->dbpp_datatype < 0)
	    {
		/* have a nullable datatype, so default is null
		 */
		SET_CANON_DEF_ID(pptuple->dbpp_defaultID, DB_DEF_NOT_SPECIFIED);
	    }
	    else
	    {
		/* non-nullable datatype, so default is either 0 or blank
		 */
		switch (pptuple->dbpp_datatype)
		{
		case DB_INT_TYPE:
		case DB_FLT_TYPE:
		case DB_MNY_TYPE:
		case DB_DEC_TYPE:
		    SET_CANON_DEF_ID(pptuple->dbpp_defaultID, DB_DEF_ID_0);
		    break;
		    
		case DB_CHR_TYPE:
		case DB_TXT_TYPE:
		case DB_DTE_TYPE:
		case DB_INYM_TYPE:
		case DB_INDS_TYPE:
		case DB_CHA_TYPE:
		case DB_VCH_TYPE:
		case DB_LVCH_TYPE:
		    SET_CANON_DEF_ID(pptuple->dbpp_defaultID, DB_DEF_ID_BLANK);
		    break;

		case DB_ADTE_TYPE:
		    SET_CANON_DEF_ID(pptuple->dbpp_defaultID, DB_DEF_ID_CURRENT_DATE);
		    break;

		case DB_TMWO_TYPE:
		case DB_TMW_TYPE:
		case DB_TME_TYPE:
		    SET_CANON_DEF_ID(pptuple->dbpp_defaultID, DB_DEF_ID_CURRENT_TIME);
		    break;

		case DB_TSWO_TYPE:
		case DB_TSW_TYPE:
		case DB_TSTMP_TYPE:
		    SET_CANON_DEF_ID(pptuple->dbpp_defaultID, DB_DEF_ID_CURRENT_TIMESTAMP);
		    break;
		    
		case DB_LOGKEY_TYPE:
		    SET_CANON_DEF_ID(pptuple->dbpp_defaultID, DB_DEF_ID_LOGKEY);
		    break;
		    
		case DB_TABKEY_TYPE:
		    SET_CANON_DEF_ID(pptuple->dbpp_defaultID, DB_DEF_ID_TABKEY);
		    break;
		    
		default:
		    SET_CANON_DEF_ID(pptuple->dbpp_defaultID, DB_DEF_UNKNOWN);
		    break;
		}
	    } /* end else non-nullable */
	}  /* end else has a default */
	
	offset += pptuple->dbpp_length;

	/* put tuple into QEF-style linked list
	 */
	qef_dptr = &qef_data_arr[i-1];

	qef_dptr->dt_size = sizeof(DB_PROCEDURE_PARAMETER);
	qef_dptr->dt_data = (PTR) pptuple;

	/* check if this is the last parameter
	** before linking to the 'next' pointer or 
	** jumping to result column queue
	*/
	if (i == num_params)
	{
	    if (num_rescols > 0)
	    {
		parm = (PSS_DECVAR *)rescolq;
		qef_dptr->dt_next = &qef_data_arr[i];
	    }
	    else qef_dptr->dt_next = (QEF_DATA *) NULL;
	}
	else if (i == totrows)
	    qef_dptr->dt_next = (QEF_DATA *) NULL;
	else
	    qef_dptr->dt_next = &qef_data_arr[i];

    }  /* end for i < totrows */

    sess_cb->pss_procparmlist = &qef_data_arr[0];

    /* unlock the memory stream so others can't conflict with us
     */
    status = psf_munlock(sess_cb, &sess_cb->pss_tstream, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    return(E_DB_OK);

}  /* end psl_fill_proc_params */




/*
** Name: psl_init_attrs  - allocate and initialize attribute descriptors
**                         representing a set-input parameter
**
** Description:	    
**	Creates an array of DMT_ATT_ENTRYs from information about the
**	set-input parameter in the DBPINFO block, in order to emulate how
**	RDF returns information about a table's attributes.
**	
**	Allocates an array of DMT_ATT_ENTRYs (including one for the TID
**	column), and copies information about the columns in the
**	set-parameter into the array.
**	
**	Returns the number of attributes in the set and the total length of
**	a tuple in the set. 
**
** Inputs:
**	sess_cb		    ptr to PSF session CB (passed to other routines)
**	    pss_ostream		ptr to mem stream for allocating structures
**	    pss_dbxlate		translation semantics for current database
**	dbpinfo		    ptr to YACC's dbproc information block
**	    pss_setparmno	number of columns in set parameter
**	    pss_setparmq	queue of column descriptions for set parameter
**
** Outputs:
**	attsp		    returns array of DMT_ATT_ENTRYs, representing
**			        the columns of the set parameter
**	num_atts	    returns number of attributes in the set
**	tot_length	    returns total length of a tuple in the set
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	05-jan-93 (rblumer)
**	    written
**	19-jul-93 (rblumer)
**	    fix typo when setting up tid column; was using subscript of 'i'
**	    instead of DB_IMTID; this could cause segv.
**	22-oct-93 (rblumer)
**	    now get setparmq and setparmno from dbpinfo instead of sess_cb.
**	27-apr-2009 (wanfr01)
**	    Bug 121586
**	    Do not copy the null termination of 'tid'.  Columns must be
**          'space terminated' here like they are everywhere else for hashing
*/
static 
DB_STATUS
psl_init_attrs(
	       PSS_SESBLK	*sess_cb,
	       PSS_DBPINFO	*dbpinfo,
	       DMT_ATT_ENTRY	***attsp,
	       i4		*num_atts,
	       i4		*tot_length,
	       DB_ERROR		*err_blk)
{
    DMT_ATT_ENTRY   *att_array;
    DMT_ATT_ENTRY   **atts;
    DB_STATUS	    status;
    PSS_DECVAR	    *parm;
    i4	    array_size, i;
    i4	    offset;
    i4	    err_code;
    
    *num_atts  = dbpinfo->pss_setparmno;
    
    /* allocate array of pointers
    ** 
    **  NOTE: allocate an extra pointer for a "tid" column, to emulate RDF
    */
    array_size = *num_atts +1;
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			array_size * sizeof(DMT_ATT_ENTRY *), 
			(PTR *) attsp, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    /* allocate array of attribute descriptors
     */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			array_size * sizeof(DMT_ATT_ENTRY), 
			(PTR *) &att_array, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    MEfill(array_size * sizeof(DMT_ATT_ENTRY), (u_char) 0, (PTR) att_array);

    /* set each pointer in the array of pointers
    ** to point to a descriptor in the array of descriptors
    */
    atts = *attsp;
    for (i = 0; i < array_size; i++)
    {
	atts[i] = &att_array[i];
    }
    
    /* copy info from set-param queue to attribute descriptors
    ** 
    **  NOTE 1: that the queue is reverse order, 
    **	        so we start with last parm and work towards beginning of list.
    **  NOTE 2: RDF uses attrs[DB_IMTID] for the "tid" column,
    **  	so initialize it like in rdf_hash_att_bld
    */
    offset = 0;
    
    for (i = 1, parm = (PSS_DECVAR *) dbpinfo->pss_setparmq.q_prev;
	 i < array_size;
	 i++,   parm = (PSS_DECVAR *) parm->pss_queue.q_prev)
    {
	if (parm == (PSS_DECVAR *) &dbpinfo->pss_setparmq)
	{
	    /* should never happen */
	    (VOID) psf_error(E_PS0002_INTERNAL_ERROR, 0L, 
			     PSF_INTERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);
	}	    
	
	MEcopy((PTR) &parm->pss_varname,
	       sizeof(DB_ATT_NAME), (PTR) &att_array[i].att_name);

	att_array[i].att_number = i;
	att_array[i].att_type   = parm->pss_dbdata.db_datatype;
	att_array[i].att_width  = parm->pss_dbdata.db_length;
	att_array[i].att_prec   = parm->pss_dbdata.db_prec;
	att_array[i].att_flags  = parm->pss_default ? 0 : DMU_F_NDEFAULT;
	att_array[i].att_offset = offset;

	SET_CANON_DEF_ID(att_array[i].att_defaultID, DB_DEF_NOT_SPECIFIED);

	offset += att_array[i].att_width;

    }  /* end for i < array_size */
    
    if (parm != (PSS_DECVAR *) &dbpinfo->pss_setparmq)
    {
	/* should never happen */
	(VOID) psf_error(E_PS0002_INTERNAL_ERROR, 0L, 
			 PSF_INTERR, &err_code, err_blk, 0);
	return (E_DB_ERROR);
    }	    
    
    *tot_length = offset;
    

    /* for completeness, setup "tid" column (just like RDF does)
     */
    MEmove(3,
	   ((*sess_cb->pss_dbxlate & CUI_ID_REG_U) ? ERx("TID") : ERx("tid")),
	   ' ', sizeof(DB_ATT_NAME), 
	   (PTR) &att_array[DB_IMTID].att_name);

    att_array[DB_IMTID].att_type   = DB_INT_TYPE;
    att_array[DB_IMTID].att_width  = 4;

    SET_CANON_DEF_ID(att_array[DB_IMTID].att_defaultID, DB_DEF_NOT_SPECIFIED);

    return(E_DB_OK);

}  /* end psl_init_attrs */



/*
** Name: psl_rdf_hash  - hash function for column names (a la RDF);
**                       taken from pst_coldesc function
**
** Description:	    
**	Calculates a bucket value given a column name (DB_ATT_NAME).
**
** Inputs:
**	colname		    ptr to column name
**
** Outputs:
**	none
**
** Returns:
**	Value of hash function applied to column name.
**	(Taken from pst_coldesc function.)
**
** Side effects:
**	none
**
** History:
**
**	05-jan-93 (rblumer)
**	    written
*/
static
i4
psl_rdf_hash(DB_ATT_NAME *colname)
{
    register i4	bucket;
    register u_char	*p;
    register i4	i;

    /* First, find the right hash bucket */
    bucket = 0;
    p = (u_char *) colname->db_att_name;
    for (i = 0; i < sizeof(DB_ATT_NAME); i++)
    {
	bucket += *p;
	p++;
    }
    bucket %= RDD_SIZ_ATTHSH;

    return(bucket);
    
}  /* end psl_rdf_hash */



/*
** Name: psl_build_atthsh - build a hash table of attributes (a la RDF)
**
** Description:
**	Given an array of attribute descriptors (DMT_ATT_ENTRYs), builds a hash
**	table of those attribute descriptors in order to emulate how
**	RDF returns information about a table's attributes.
**	
**	Allocates an RDD_ATTHSH structure to hold the hash table, then
**	allocates a hash entry for each attribute and hashes it into the table.
**
** Input:
**	sess_cb		    ptr to PSF session CB
**	    pss_ostream		ptr to mem stream for allocating memory
**	num_atts	    number of attributes in rdr_attr
**	rdr_info	    ptr to RDF info block 
**	    rdr_attr		array of attribute descriptors
**
** Output:
**	rdr_info	    ptr to RDF info block 
**	    rdr_atthsh		hash table of attribute descriptors
**	err_blk		filled in if an error is encountered
**
** Returns:
**	E_DB_{OK,ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	05-jan-93 (rblumer)
**	    written
*/
static
DB_STATUS
psl_build_atthsh(
		 PSS_SESBLK	*sess_cb,
		 i4	num_atts,
		 RDR_INFO	*rdr_info,
		 DB_ERROR	*err_blk)
{
    DMT_ATT_ENTRY   **atts;
    RDD_BUCKET_ATT  **hsh_tbl;
    RDD_BUCKET_ATT  *hsh_entry;
    i4  	    bucket;
    DB_STATUS	    status;
    i4	    i;
    
    /* allocate array of hash buckets
     */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			sizeof(RDD_ATTHSH), 
			(PTR *) &rdr_info->rdr_atthsh, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    MEfill(sizeof(RDD_ATTHSH), (u_char) 0, (PTR) rdr_info->rdr_atthsh);

    /* for each attribute,
    ** find its bucket and put it into the hash table
    */
    atts    = rdr_info->rdr_attr;
    hsh_tbl = rdr_info->rdr_atthsh->rdd_atthsh_tbl;
    
    /* 
    **  NOTE: to emulate RDF, there is one extra attribute descriptor
    **        for a "tid" column,  so process 0 thru 'num_atts' atts
    */
    for (i = 0; i <= num_atts; i++)
    {
	/* allocate a new hash entry and fill it in
	 */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			    sizeof(RDD_BUCKET_ATT),
			    (PTR *) &hsh_entry, err_blk);

	hsh_entry->attr = atts[i];

	/* insert hash entry at beginning of linked list
	 */
	bucket = psl_rdf_hash(&(atts[i]->att_name));

	hsh_entry->next_attr = hsh_tbl[bucket];
	hsh_tbl[bucket]      = hsh_entry;
    }
    
    return(E_DB_OK);

}  /* end psl_build_atthsh */




/*
** Name: psl_init_rdrinfo  - creates an rdr_info block describing a set-input
** 			     procedure parameter and its columns.
** 				  
** Description:	    
**	Allocates the rdr_info block, sets up the attribute descriptors,
**	builds a hash table of the attributes, and initializes the table
**	description, all using information on the set parameter from the
**	DBPINFO block.
**	
** Inputs:
**	sess_cb		    ptr to PSF session CB (passed to other routines)
**	    pss_ostream	           ptr to mem stream for allocating structures
**	dbpinfo		    ptr to YACC's dbproc info block
**			    (passed to other routines)
**	    pss_setparmname	name of set-input parameter
**
** Outputs:
**	rdr_infop	    newly allocated and initialized rdr_info block
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	05-jan-93 (rblumer)
**	    written
**	22-oct-93 (rblumer)
**	    now get setparmname from dbpinfo instead of sess_cb.
*/
static 
DB_STATUS
psl_init_rdrinfo(
		 PSS_SESBLK	*sess_cb,
		 PSS_DBPINFO	*dbpinfo,
		 RDR_INFO	**rdr_infop,
		 DB_ERROR	*err_blk)
{
    DB_STATUS	status;
    RDR_INFO	*rdr_info;
    i4	num_atts, tot_length;


    /* allocate and initialize the rdrinfo block
     */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(RDR_INFO), 
			(PTR *) rdr_infop, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    rdr_info = *rdr_infop;

    MEfill(sizeof(RDR_INFO), (u_char) 0, (PTR) rdr_info);

    rdr_info->stream_id = (PTR) &sess_cb->pss_ostream;

    /* 
    ** allocate and initialize the attribute descriptors
    */
    status = psl_init_attrs(sess_cb, dbpinfo, &rdr_info->rdr_attr,
			    &num_atts, &tot_length, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* 
    ** allocate and build the hash table of attributes
    */
    status = psl_build_atthsh(sess_cb, num_atts, rdr_info, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);


    /*
    ** allocate and initialize the table description
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(DMT_TBL_ENTRY), 
			(PTR *) &rdr_info->rdr_rel, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    MEfill(sizeof(DMT_TBL_ENTRY), (u_char) 0, (PTR) rdr_info->rdr_rel);

    rdr_info->rdr_rel->tbl_attr_count   = num_atts;
    rdr_info->rdr_rel->tbl_width        = tot_length;
    rdr_info->rdr_rel->tbl_storage_type = DMT_HEAP_TYPE;

    MEcopy((PTR) &dbpinfo->pss_setparmname->pss_varname,
	   sizeof(DB_TAB_NAME), (PTR) &rdr_info->rdr_rel->tbl_name);
    
    rdr_info->rdr_no_attr = num_atts;

    return(E_DB_OK);

}  /* end psl_init_rdrinfo */
