/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <st.h>
#include    <qu.h>
#include    <me.h>
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

/**
**
**  Name: PSTSORT.C - Functions for handling sort nodes.
**
**  Description:
**      This file contains the functions for generating sort nodes, modifying
**	them, and adding them to sort lists.
**
**          pst_adsort - Add a sort node to a sort list
**          pst_sdir - Change the sorting direction of a sort node
**          pst_varsort - Generate a sort node by range variable and column
**	    pst_expsort - Generate a sort node by expression name
**	    pst_numsort - Generate a sort node by expression name
**	    pst_sqlsort - Generate a sort node for any SQL order by element
**
**
**  History:
**      02-may-86 (jeff)    
**          written
**	23-feb-87 (daved)
**	    add pst_numsort
**	17-apr-89 (jrb)
**	    Interface change for pst_node and prec cleanup for decimal project.
**	30-oct-89 (fred)
**	    Added support (== generate error) for unsortable datatypes.
**	29-may-90 (andre)
**	    changed interface for pst_adsort slightly.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	04-mar-93 (rblumer)
**	    change parameter to psf_adf_error to be psq_error, not psq_cb.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	10-may-1999 (somsa01)
**	    In pst_sqlsort_resdom, fixed typo where we would return nothing.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to pst_treedup(), pst_sqlsort_resdom().
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**      04-nov-08 (stial01)
**          Fixed name buffer size
**	03-Dec-2009 (kiria01) SIR 121883
**	    Changes for subselect order bys.
[@history_template@]...
**/

/* static function declarations */
static DB_STATUS
pst_sqlsort_resdom(
	PSS_SESBLK	*sess_cb,
	PSF_MSTREAM	*stream,
	DB_ERROR	*err_blk,
	PST_QNODE	**nodep);

static DB_STATUS
pst_adsort(
	PSS_SESBLK	    *sess_cb,
	PST_QNODE	    *sort_attr,
	PST_QNODE	    **list,
	PSF_MSTREAM	    *stream,
	PST_QNODE	    **sort_node,
	DB_ERROR	    *err_blk);
/*{
** Name: pst_adsort	- Add a sort node to a sort list
**
** Description:
**      This function will try to add a sort node to a sort list.
**	First we will check if this attribute has alrteady been placed on the
**	sort list.  If so, we just return; otherwise a new sort node will be
**	created and appended to the list.
**
** Inputs:
**	sess_cb				session CB
**      sort_attr			Node representing the sort attribute
**	list				addr of the ptr to the first node in
**					the sort list (*list may be NULL if the
**					list is empty)
**	stream				memory stream for allocating a node
**	err_blk				Filled in if an error happens
**
** Outputs:
**      list                            node may have been appended to the list
**	sort_node			will contain address of the newly
**					created sort node (if any)
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-may-86 (jeff)
**          Adapted from sr_addkey() in old sortstuff.c
**	29-may-90 (andre)
**	    Will check for duplicate occurences of sort attributes.  If there
**	    are no occurrences of this attribute in the sort list, create a sort
**	    node and add it to the list.
**      08-Jun-2009 (coomi01) b122164
**          Do not admit a PST_SEQOP into a sort list.
*/
static DB_STATUS
pst_adsort(
	PSS_SESBLK	    *sess_cb,
	PST_QNODE	    *sort_attr,
	PST_QNODE	    **list,
	PSF_MSTREAM	    *stream,
	PST_QNODE	    **sort_node,
	DB_ERROR	    *err_blk)
{
    PST_SRT_NODE	sortnode;
    PST_QNODE		*tail;
    DB_STATUS		status;

    if ( sort_attr && sort_attr->pst_right && sort_attr->pst_right->pst_sym.pst_type == PST_SEQOP )
    {
        *sort_node = (PST_QNODE *)0;
        return(E_DB_OK);
    }

    if (*list != (PST_QNODE *) NULL)
    {
	register PST_QNODE  *sort_list_elem;
	register i4	    attr_no =
			      sort_attr->pst_sym.pst_value.pst_s_sort.pst_srvno;

	for (sort_list_elem = *list;
	     sort_list_elem != (PST_QNODE *) NULL;
	     tail = sort_list_elem, sort_list_elem = sort_list_elem->pst_right)
	{
	    if (sort_list_elem->pst_sym.pst_value.pst_s_sort.pst_srvno ==
		attr_no)
	    {
		/*
		** if an attribute appears more than once in the ORDER BY clause
		** all but the first occurrence will be discarded
		*/
		*sort_node = (PST_QNODE *) NULL;
		return(E_DB_OK);
	    }
	}
    }

    /* first create a sort node */
    sortnode.pst_srvno = sort_attr->pst_sym.pst_value.pst_s_sort.pst_srvno;
    sortnode.pst_srasc = TRUE;	    /* Assume ascending sort */
    status = pst_node(sess_cb, stream, (PST_QNODE *) NULL, (PST_QNODE *) NULL,
	PST_SORT, (char *) &sortnode, sizeof(sortnode),
	sort_attr->pst_sym.pst_dataval.db_datatype, 
	sort_attr->pst_sym.pst_dataval.db_prec,
	sort_attr->pst_sym.pst_dataval.db_length,
	(DB_ANYTYPE *) NULL, sort_node, err_blk, (i4) 0);

    /* if node has been created successfully, append it to the list */
    if (DB_SUCCESS_MACRO(status))
    {
	if (*list != (PST_QNODE *) NULL)
	{
	    tail->pst_right = *sort_node;
	}
	else
	{
	    /* newly created node is the first node in the list */
	    *list = *sort_node;
	}
    }

    return(status);
}

/*{
** Name: pst_sdir	- Change the sorting direction in a sort node
**
** Description:
**      This function changes the sorting direction in a sort node.  The sort
**	direction is either ascending or descending.  Ascending can be
**	specified as "a", "asc", or "ascending".  Descending can be specified
**	as "d", "desc", or "descending".
**
** Inputs:
**      node                            The node in which to change the sort
**					order
**	direction			String specifying the sort order
**	err_blk				Filled in if an error happens
**
** Outputs:
**      node                            Sort order filled in
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-may-86 (jeff)
**          adapted from sr_dir(0 in old sortstuff.c
*/
DB_STATUS
pst_sdir(
	PST_QNODE          *node,
	char		   *direction,
	DB_ERROR	   *err_blk)
{
    i4             err_code;

    if (node == (PST_QNODE *) NULL)
    {
	psf_error(2161L, 0L, PSF_USERERR, &err_code, err_blk, 1,
	    (i4) (sizeof("(null)") - 1), "(null)");
	return (E_DB_ERROR);
    }

    if (!STcompare(direction, "a") || !STcompare(direction, "asc") ||
	!STcompare(direction, "ascending"))
    {
	node->pst_sym.pst_value.pst_s_sort.pst_srasc = TRUE;
    }
    else if (!STcompare(direction, "d") || !STcompare(direction, "desc") ||
	!STcompare(direction, "descending"))
    {
	node->pst_sym.pst_value.pst_s_sort.pst_srasc = FALSE;
    }
    else
    {
	(VOID) psf_error(2163L, 0L, PSF_USERERR, &err_code,
	    err_blk, 1, (i4) STtrmwhite(direction), direction);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: pst_expsort	- Create a sort node by expression name
**
** Description:
**      This function creates a sort node by the name of an expression and
**	appends it to the sort list.  In other words, if a user says
**	"sort by xyz", this function will look for the column named "xyz" in the
**	target list, and make a sort node corresponding to it. The actual
**	creation and appending is done by pst_adsort().
**
** Inputs:
**	sess_cb
**      stream                          The memory stream for allocating the
**					node
**	tlist				The target list for the query
**	sort_list			Address  of the pointer to the first
**					node in the sort list
**	expname				The column name to sort on
**	newnode				Place to put pointer to new node
**	psq_cb				Caller's request control block
**
** Outputs:
**      newnode                         Filled in with pointer to sort node (may
**					be set to NULL if this was not the first
**					occurrence of the sort attribute in the
**					sort list)
**	sort_list			a new sort node may have been appended
**					to the sort list 
**      psq_cb
**		->psq_error		Filled in if an error happens
**
** Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	02-may-86 (jeff)
**          adapted from sr_explook() in old sortstuff.c
**	23-jun-87 (daved)
**	    use column named first in target list if duplicate names:
**		"retrieve (a.a,b.a) sort by a"
**	    should sort by a.a
**	30-oct-89 (fred)
**	    Added support for unsortable datatypes.  Also changed interface to
**	    accept psq_cb instead of err_blk.
**	29-may-90 (andre)
**	    will accept an address of the ptr to the head of the sort list as
**	    one of the arguments.
*/
DB_STATUS
pst_expsort(
	PSS_SESBLK	    *sess_cb,
	PSF_MSTREAM	    *stream,
	PST_QNODE	    *tlist,
	PST_QNODE	    **sort_list,
	char		    *expname,
	PST_QNODE	    **newnode,
	PSQ_CB		    *psq_cb)
{
    register PST_QNODE	*attr;
    PST_QNODE		*save_attr = (PST_QNODE *) NULL;
    char                resname[DB_MAXNAME];
    DB_STATUS		status;
    i4		err_code;

    /* Normalize the result column name */
    STmove(expname, ' ', DB_MAXNAME, resname);

    /* Look through the target list for a column of that name */
    for (attr = tlist;
         attr != NULL && attr->pst_sym.pst_type == PST_RESDOM;
	 attr = attr->pst_left)
    {
	if (!MEcmp(resname, attr->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
	    DB_MAXNAME))
	{
	    save_attr = attr;
	}
    }

    /*
    ** If the column was not found, complain to the user and return an error.
    */
    if (!save_attr)
    {
	(VOID) psf_error(2161L, 0L, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 1, (i4) STtrmwhite(expname), expname);
	return (E_DB_ERROR);
    }

    if (save_attr->pst_sym.pst_dataval.db_datatype != DB_NODT)
    {
	i4	    dt_status;
	ADF_CB	    *adf_scb;

	adf_scb = (ADF_CB *)sess_cb->pss_adfcb;

	status = adi_dtinfo(adf_scb, save_attr->pst_sym.pst_dataval.db_datatype,
			    &dt_status);
	if (status)
	{
	    adf_scb->adf_errcb.ad_errclass = ADF_USER_ERROR;
	    psf_adf_error(&adf_scb->adf_errcb, &psq_cb->psq_error, sess_cb);
	    return(status);
	}
	else if (dt_status & AD_NOSORT)
	{
	    (VOID) psf_error(2181L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1,(i4) STtrmwhite(expname), expname);
	    return (E_DB_ERROR);
	}
    }

    /* allocate the sort node and attach it to the sort list */
    status = pst_adsort(sess_cb, save_attr, sort_list, stream, newnode,
			&psq_cb->psq_error);

    return (status);
}

/*{
** Name: pst_varsort	- Create a sort node by variable and column
**
** Description:
**      This function creates a sort node by variable and column.  In other
**	words, if the user enters "sort by x.y", this function will try to
**	find x.y in the target list and create a sort node from it.
**	pst_adsort() is responsible for creating a sort node and appending it to
**	the list, if necessary (see pst_adsort() for more details.)
**
** Inputs:
**	sess_cb
**      stream                          Memory stream to create the sort node
**					from
**	tlist				The target list for this query
**	sort_list			Address  of the pointer to the first
**					node in the sort list
**	rngvar				Pointer to the range variable to look
**					for
**	colname				Name of the column within the range
**					variable
**	newnode				Place to put pointer to the sort node
**	psq_cb				Callers request control block -- used
**					for error processing
**
** Outputs:
**      newnode                         Filled in with pointer to sort node (may
**					be set to NULL if this was not the first
**					occurrence of the sort attribute in the
**					sort list)
**	sort_list			a new sort node may have been appended
**					to the sort list 
**	psq_cb
**		->psq_error		Filled in if an error happens
**
** Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	02-may-86 (jeff)
**          adapted from sr_varlook() in old sortstuff.c
**	30-oct-89 (fred)
**	    Added support for unsortable datatypes.  Also changed interface to
**	    accept psq_cb instead of a err_blk.
**	29-may-90 (andre)
**	    will accept an address of the ptr to the head of the sort list as
**	    one of the arguments.
*/
DB_STATUS
pst_varsort(
	PSS_SESBLK		*sess_cb,
	PSF_MSTREAM		*stream,
	PST_QNODE		*tlist,
	PST_QNODE		**sort_list,
	register PSS_RNGTAB	*rngvar,
	char			*colname,
	PST_QNODE		**newnode,
	PSQ_CB			*psq_cb)
{
    DB_ATT_NAME         column;
    register PST_QNODE  *attr;
    i4			found;
    DB_STATUS		status;
    i4		err_code;

    /* Normalize the column name */
    STmove(colname, ' ', sizeof(DB_ATT_NAME), (char *) &column);

    /*
    ** Look through the target list for a result domain with a var node
    ** under it with the right variable number and column name.
    */

    for (attr = tlist, found = FALSE;
         attr != NULL && attr->pst_sym.pst_type == PST_RESDOM;
	 attr = attr->pst_left)
    {
	if (attr->pst_right->pst_sym.pst_type == PST_VAR	    &&
	    attr->pst_right->pst_sym.pst_value.pst_s_var.pst_vno ==
	        rngvar->pss_rgno				    &&
	    !MEcmp((char *) &column,
	      (char *) &attr->pst_right->pst_sym.pst_value.pst_s_var.pst_atname,
	      sizeof(DB_ATT_NAME)))
	{
	    found = TRUE;
	    break;
	}
    }

    /*
    ** If the column was not found, complain to the user and return an error.
    */
    if (!found)
    {
	(VOID) psf_error(2161L, 0L, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 1, (i4) STtrmwhite(colname), colname);
	return (E_DB_ERROR);
    }

    if (attr->pst_sym.pst_dataval.db_datatype != DB_NODT)
    {
	i4	    dt_status;
	ADF_CB	    *adf_scb;

	adf_scb = (ADF_CB *)sess_cb->pss_adfcb;

	status = adi_dtinfo(adf_scb, attr->pst_sym.pst_dataval.db_datatype,
			    &dt_status);
	if (status)
	{
	    adf_scb->adf_errcb.ad_errclass = ADF_USER_ERROR;

	    psf_adf_error(&adf_scb->adf_errcb, &psq_cb->psq_error, sess_cb);
	    return(status);
	}
	else if (dt_status & AD_NOSORT)
	{
	    (VOID) psf_error(2181L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1, (i4) STtrmwhite(colname), colname);
	    return (E_DB_ERROR);
	}
    }

    /* Allocate the sort node */
    status = pst_adsort(sess_cb, attr, sort_list, stream, newnode,
			&psq_cb->psq_error);

    return (status);
}

/*{
** Name: pst_numsort	- Create a sort node by expression number
**
** Description:
**      This function creates a sort node by the number of an expression.
**	In other words, if a user says "sort by 3", this function will
**	look for the column numbered "3" in the target list, and make a
**	sort node corresponding to it.  pst_adsort() is responsible for creating
**	and appending of the sort node to the sort list, if necessary (for more
**	details, see pst_adsort().)
**
** Inputs:
**	sess_cb
**      stream                          The memory stream for allocating the
**					node
**	tlist				The target list for the query
**	sort_list			Address  of the pointer to the first
**					node in the sort list
**	expnum				The column number to sort on
**	newnode				Place to put pointer to new node
**	psq_cb				Callers request control block
**
** Outputs:
**      newnode                         Filled in with pointer to sort node (may
**					be set to NULL if this was not the first
**					occurrence of the sort attribute in the
**					sort list)
**	sort_list			a new sort node may have been appended
**					to the sort list 
**      psq_cb
**		->psq_error		Filled in if an error happens
**
** Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	23-feb-87 (daved)
**	    written
**	30-oct-89 (fred)
**	    Added support for unsortable datatypes
**	29-may-90 (andre)
**	    will accept an address of the ptr to the head of the sort list as
**	    one of the arguments.
*/
DB_STATUS
pst_numsort(
	PSS_SESBLK	    *sess_cb,
	PSF_MSTREAM	    *stream,
	PST_QNODE	    *tlist,
	PST_QNODE	    **sort_list,
	i2		    *expnum,
	PST_QNODE	    **newnode,
	PSQ_CB		    *psq_cb)
{
    register PST_QNODE	*attr;
    i4                  found;
    DB_STATUS		status;
    i4		err_code;

    /* Look through the target list for a column of that name */
    for (found = FALSE, attr = tlist;
         attr != NULL && attr->pst_sym.pst_type == PST_RESDOM;
	 attr = attr->pst_left)
    {
	if (attr->pst_sym.pst_value.pst_s_rsdm.pst_rsno == *expnum)
	{
	    found = TRUE;
	    break;
	}
    }

    /*
    ** If the column was not found, complain to the user and return an error.
    */
    if (!found)
    {
	(VOID) psf_error(2162L, 0L, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 0);
	return (E_DB_ERROR);
    }

    if (attr->pst_sym.pst_dataval.db_datatype != DB_NODT)
    {
	i4	    dt_status;
	ADF_CB	    *adf_scb;

	adf_scb = (ADF_CB *)sess_cb->pss_adfcb;

	status = adi_dtinfo(adf_scb, attr->pst_sym.pst_dataval.db_datatype,
			    &dt_status);
	if (status)
	{
	    adf_scb->adf_errcb.ad_errclass = ADF_USER_ERROR;
	    psf_adf_error(&adf_scb->adf_errcb, &psq_cb->psq_error, sess_cb);
	    return(status);
	}
	else if (dt_status & AD_NOSORT)
	{
	    (VOID) psf_error(2182L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1, (i4) sizeof(*expnum), expnum);
	    return (E_DB_ERROR);
	}
    }

    /* allocate a new sort node and append it to the sort list */
    status = pst_adsort(sess_cb, attr, sort_list, stream, newnode,
			&psq_cb->psq_error);

    return (status);
}

/*{
** Name: pst_sqlsort	- Create a sort node by any of the SQL-acceptable
**	methods (column, qualified column, ordinal select list number, 
**	column not in select list, expression).
**
** Description:
**      This function creates a sort node defined by any of the acceptable
**	SQL sort constructs. For columns already in the select list and for
**	ordinal select list numbers, there will already be a resdom for the
**	sort node in the target list. For columns not in the select list and
**	for expressions, a non-printing resdom must be added to the target list.
**	The sort node itself is then made by pst_adsort().
**
** Inputs:
**	sess_cb
**      stream                          The memory stream for allocating the
**					node
**	tlist				The target list for the query
**	intolist			The target INTO list if this is a
**					database procedure with an INTO clause,
**					else NULL
**	sort_list			Address  of the pointer to the first
**					node in the sort list
**	expnode				Address of the parse tree fragment (or
**					VAR or CONST node) defining order by
**					element
**	newnode				Place to put pointer to new node
**	psq_cb				Caller's request control block
**
** Outputs:
**      newnode                         Filled in with pointer to sort node (may
**					be set to NULL if this was not the first
**					occurrence of the sort attribute in the
**					sort list)
**	tlist				a new RESDOM may be added if query has
**					an ORDER BY for an unselected column
**	intolist			a new RESDOM may be added if query has
**					an ORDER BY for an unselected column
**					and the query is a database procedure
**					with an INTO clause
**	sort_list			a new sort node may have been appended
**					to the sort list 
**      psq_cb
**		->psq_error		Filled in if an error happens
**
** Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	1-dec-98 (inkdo01)
**	    Cloned from pst_expsort.
**	27-jan-99 (inkdo01)
**	    When column names are same and order by was done on the same col
**	    name from different tables, the code ignored the second column
**	    thinking they are the same.
**	13-apr-99 (inkdo01)
**	    Turns out that an order by expression referencing a derived column 
**	    from the select list gets a RESDOM node built into it (instead
**	    of the resdom's pst_right expression). This causes OPF grief. 
**	    The fix is to hunt out resdom's in the expression subtree and 
**	    replace them with their respective pst_right's. 
**	22-oct-99 (inkdo01)
**	    The exception to the above rule is when the derived expression is
**	    an aggregate, which cannot then be part of an order by expression.
**	6-jul-00 (inkdo01)
**	    Set up expname local for error messages (e.g., ordering on BLOB
**	    column). Fixes 102025.
**	1-may-2002 (toumi01)
**	    issue 11873547 bug 107626 dbproc order by unselected cols
**	    When processing a dbproc and adding RESDOMs for sort fields that
**	    are not in the select list, add the nodes to the INTO list tree.
**	    We will be substituting the INTO tree for the current LH branch
**	    when the "query:" rule fires.
**	2-may-2003 (inkdo01)
**	    The above change needs to check for duplicates (in the PST_VAR 
**	    logic) in the intolist, for database procdures. This fixes bug
**	    110170.
**	30-may-03 (inkdo01)
**	    An adjustment to the previous change to have common logic for 
**	    RESDOM list traversal - using intolist if not empty, otherwise
**	    using tlist. 
**	14-May-2004 (schka24)
**	    Support i8 - kind of dumb, but it should be allowed.
**      15-jul-2004 (huazh01)
**          For aggregate query, do not create sort node for columns not 
**          in the select list. 
**          This fixes INGSRV2818, b112218.  
**	23-dec-04 (inkdo01)
**	    Pass collation ID into RESDOM.
**	18-apr-05 (inkdo01)
**	    Dumb little fix to support order by on base table column name,
**	    as well as result set column name in presence of group by (SIR
**	    114332).
**      27-feb-2006 (huazh01)
**          Extend the fix of b112218 so that it covers the case of the 
**          aggregate query is located under a PST_BOP node. An example of
**          the query is: select ... ifnull(min(a), 0) .. order by a 
**          This fixes b115798. 
**      24-mar-2006 (huazh01)
**          fix a side effect of the fix to b112218 by allowing parser to
**          to create sort node for columns that have been renamed to
**          to another column. i.e.,  "select c1 as col1 .. order by c1".
**          b115859. 
**      08-Jun-2009 (coomi01) b122164
**          Order by sequence opperator is not allowed, generate an error
**          message when this occurs.
*/
DB_STATUS
pst_sqlsort(
	PSS_SESBLK	    *sess_cb,
	PSF_MSTREAM	    *stream,
	PST_QNODE	    *tlist,
	PST_QNODE	    *intolist,
	PST_QNODE	    **sort_list,
	PST_QNODE	    *expnode,
	PST_QNODE	    **newnode,
	PSQ_CB		    *psq_cb,
	PSS_RNGTAB	    **rng_vars)
{
    register PST_QNODE	*attr;
    PST_QNODE		*save_attr = (PST_QNODE *) NULL;
    PST_QNODE		*rlist = (intolist) ? intolist : tlist;
				/* use intolist if not empty, else tlist */
    DB_STATUS		status;
    i4		err_code;
    i4			maxrsno;
    char		expname[DB_MAXNAME + 1];
    DB_ERROR		err_blk;
    bool                aggQry = FALSE; 

    /* Start by searching for resdom list entry no (if there) based on
    ** expression type. But first, seek RESDOM nodes buried in the 
    ** expression and replace them by the pst_right of the resdom. */

    expname[0] = expname[DB_MAXNAME] = 0x0;

    switch (expnode->pst_sym.pst_type) {

      case PST_VAR:
	/* Copy column name first, in case of error. */
	MEcopy((PTR)&expnode->pst_sym.pst_value.pst_s_var.pst_atname,
		DB_MAXNAME, (PTR)&expname);
	/* Search resdom list for matching entry, else leave with NULL */
	for (attr = rlist;
         attr != NULL && attr->pst_sym.pst_type == PST_RESDOM;
	 attr = attr->pst_left)
	{
            if ((attr->pst_right->pst_sym.pst_type == PST_AGHEAD) ||
                (attr->pst_right->pst_sym.pst_type == PST_BOP &&
                 attr->pst_right->pst_left->pst_sym.pst_type == PST_AGHEAD))
            {
               aggQry = TRUE; 
            }

	    if (attr->pst_right->pst_sym.pst_type == PST_VAR &&
		expnode->pst_sym.pst_value.pst_s_var.pst_vno ==
		 attr->pst_right->pst_sym.pst_value.pst_s_var.pst_vno &&
		(!MEcmp((PTR)&expnode->pst_sym.pst_value.pst_s_var.pst_atname,
		(PTR)&attr->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
	    	DB_MAXNAME) ||
		expnode->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		 attr->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id))
	    {
		save_attr = attr;
		MEcopy((PTR)&expnode->pst_sym.pst_value.pst_s_var.pst_atname,
			DB_MAXNAME, (PTR)&expname);
	    }
	}

        /* INGSRV2818, b112218 */
        if (aggQry && !save_attr)
        {
           (VOID) psf_error(2161L, 0L, PSF_USERERR, &err_code,
                            &psq_cb->psq_error, 1, 
                            (i4) STtrmwhite(expname), expname);
           return (E_DB_ERROR);
        }
	break;

      case PST_CONST:
	/* Ordinal select list entry - must be integer constant. */
	if (expnode->pst_sym.pst_dataval.db_datatype == DB_INT_TYPE &&
		expnode->pst_sym.pst_dataval.db_data)
	{
	    i8		expnum;
	    switch (expnode->pst_sym.pst_dataval.db_length) {
	     case 1:
		expnum = *((i1 *)expnode->pst_sym.pst_dataval.db_data);
		break;

	     case 2:
		expnum = *((i2 *)expnode->pst_sym.pst_dataval.db_data);
		break;

	     case 4:
		expnum = *((i4 *)expnode->pst_sym.pst_dataval.db_data);
		break;

	     case 8:
		expnum = *((i8 *)expnode->pst_sym.pst_dataval.db_data);
		break;
	    }

	    for (attr = rlist; 
	     attr != NULL && attr->pst_sym.pst_type == PST_RESDOM;
	     attr = attr->pst_left)
	      if (attr->pst_sym.pst_value.pst_s_rsdm.pst_rsno == expnum &&
		attr->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
	      {
		save_attr = attr;
		MEcopy((PTR)&attr->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
			DB_MAXNAME, (PTR)&expname);
		break;
	      }
	    if (save_attr != NULL) break;		/* got it */
	}

	/* If we get here, constant was bad. */
	(VOID) psf_error(2162L, 0L, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 0);
	return (E_DB_ERROR);
	break;

      case PST_RESDOM:
	/* Named (but not qualified) column in target list. This also
	** handles renamed columns/expressions in select-list. 
	** NOTE: aggregate expressions under resdoms don't need this stuff. */
	if (expnode->pst_right && 
		expnode->pst_right->pst_sym.pst_type != PST_AGHEAD)
	{
	    status = pst_sqlsort_resdom(sess_cb, stream, &err_blk, &expnode->pst_right);
	    if (status != E_DB_OK) return(status);
	}
	save_attr = expnode;
	MEcopy((PTR)&expnode->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		DB_MAXNAME, (PTR)&expname);
	break;

      default:
	status = pst_sqlsort_resdom(sess_cb, stream, &err_blk, &expnode);
	if (status != E_DB_OK) return(status);
	save_attr = NULL;
	{
	    PST_QNODE	*nodep;
	    PST_STK_CMP cmp_ctx;

	    PST_STK_CMP_INIT(cmp_ctx, sess_cb, rng_vars);

	    /* Normalise for compares. The target list has been
	    ** treated similarly. */
	    if (status = pst_qtree_norm(&cmp_ctx.stk,
			&expnode, psq_cb))
		return status; 
	    /* Find next available pst_rsno too. */
	    for (nodep = rlist, maxrsno = -1; nodep && 
		nodep->pst_sym.pst_type == PST_RESDOM; nodep = nodep->pst_left)
	    {
		pst_qtree_compare(&cmp_ctx, &nodep->pst_right, &expnode, FALSE);
		if (cmp_ctx.same)
		{
		    /* We found a match though it might need a fix up */
		    if (cmp_ctx.n_covno && !cmp_ctx.inconsistant)
		    {
			/* Redo this time fixing up */
			pst_qtree_compare(&cmp_ctx, &nodep->pst_right, &expnode, TRUE);
			cmp_ctx.n_covno = 0;
		    }
		    if (cmp_ctx.same)
		    {
			save_attr = nodep;
			break;
		    }
		}
	    }
	}
	break;
    }	/* end of switch */

    /* If there is no corresponding RESDOM, make one now to attach the
    ** expression. */
    if (!save_attr)
    {
	PST_QNODE	*nodep;
	PST_RSDM_NODE	resdom;

	/* First, find next available pst_rsno. */
	for (nodep = rlist, maxrsno = -1; nodep && 
	    nodep->pst_sym.pst_type == PST_RESDOM; nodep = nodep->pst_left)
	 if (maxrsno < nodep->pst_sym.pst_value.pst_s_rsdm.pst_rsno)
		maxrsno = nodep->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	/* Then fill in resdom and allocate it. */
	resdom.pst_rsflags = 0; /* no PST_RS_PRINT - make it non-printing */
	resdom.pst_rsupdt = 0;
	resdom.pst_ntargno = 0;
	resdom.pst_ttargtype = PST_USER;
	if (expnode->pst_sym.pst_type == PST_VAR)
	{
	    MEcopy((PTR)&expnode->pst_sym.pst_value.pst_s_var.pst_atname,
		DB_MAXNAME, (PTR)&resdom.pst_rsname);
	    MEcopy((PTR)&expnode->pst_sym.pst_value.pst_s_var.pst_atname,
		DB_MAXNAME, (PTR)&expname);
	}
	resdom.pst_rsno = maxrsno + 1;

	status = pst_node(sess_cb, stream, (PST_QNODE *) NULL, (PST_QNODE *) NULL,
	    PST_RESDOM, (char *) &resdom, sizeof(resdom),
	    expnode->pst_sym.pst_dataval.db_datatype, 
	    expnode->pst_sym.pst_dataval.db_prec,
	    expnode->pst_sym.pst_dataval.db_length,
	    (DB_ANYTYPE *) NULL, &save_attr, &err_blk, (i4) 0);
	if (status != E_DB_OK) return(status);

	/* Link new resdom into target list. */
	save_attr->pst_left = rlist->pst_left;
	rlist->pst_left = save_attr;
	save_attr->pst_right = expnode;
	save_attr->pst_sym.pst_dataval.db_collID =
			expnode->pst_sym.pst_dataval.db_collID;
    }

    if (save_attr->pst_sym.pst_dataval.db_datatype != DB_NODT)
    {
	i4	    dt_status;
	ADF_CB	    *adf_scb;

	adf_scb = (ADF_CB *)sess_cb->pss_adfcb;

	status = adi_dtinfo(adf_scb, save_attr->pst_sym.pst_dataval.db_datatype,
			    &dt_status);
	if (status)
	{
	    adf_scb->adf_errcb.ad_errclass = ADF_USER_ERROR;
	    psf_adf_error(&adf_scb->adf_errcb, &psq_cb->psq_error, sess_cb);
	    return(status);
	}
	else if (dt_status & AD_NOSORT || 
		(save_attr->pst_right && save_attr->pst_right->pst_sym.pst_type == PST_SEQOP))
	{
	    (VOID) psf_error(2181L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1,(i4) STtrmwhite(expname), expname);
	    return (E_DB_ERROR);
	}
    }

    /* allocate the sort node and attach it to the sort list */
    status = pst_adsort(sess_cb, save_attr, sort_list, stream, newnode,
			&psq_cb->psq_error);

    return (status);
}


/*{
** Name: pst_sqlsort_resdom - eliminates RESDOM nodes from sort expression 
**	parse trees.
**
** Description:
**      This function recursively analyzes the parse tree fragment for an
**	order by expression looking for RESDOM nodes. They appear when the
**	order by expression includes a derived column from the select list;
**	e.g. select a+b as sumab from x order by sumab*2;
**	The RESDOMs must be replaced by their right subtrees.
**
** Inputs:
**	nodep				Pointer to PST_NODE pointer addressing
**					sort expression subtree
**
** Outputs:
**      none
**
** Returns:
**	nothing
**
** Side Effects:
**	none
**
** History:
**	13-apr-99 (inkdo01)
**	    Written.
**	10-may-1999 (somsa01)
**	    Fixed typo where we would return nothing.
**      23-apr-2001 (zhahu02)
**          To allow referencing a derived expression as an aggregate in
**          an order by clause.
**          bug 104520, problem INGSRV1436 
**      
*/

static DB_STATUS
pst_sqlsort_resdom(
	PSS_SESBLK	*sess_cb,
	PSF_MSTREAM	*stream,
	DB_ERROR	*err_blk,
	PST_QNODE	**nodep)

{
    DB_STATUS	status = E_DB_OK;

    /* Recursively descend pst_left side, and iterate pst_right.
    ** If a RESDOM is found, replace it with its own pst_right. */

    while (TRUE)
    switch ((*nodep)->pst_sym.pst_type)
    {
      case PST_RESDOM:
	{
	    PSS_DUPRB		dup_rb;

	    /* initialize fields in dup_rb */
	    dup_rb.pss_op_mask = 0;
	    dup_rb.pss_num_joins = PST_NOJOIN;
	    dup_rb.pss_tree_info = (i4 *) NULL;
	    dup_rb.pss_mstream = stream;
	    dup_rb.pss_err_blk = err_blk;

	    /*
	    ** Make copy of right subtree (to avoid double processing of
	    ** VAR nodes in OPF) and replace RESDOM with it.
	    */
	    dup_rb.pss_tree = (*nodep)->pst_right;
	    dup_rb.pss_dup  = nodep;
	    dup_rb.pss_1ptr = NULL;
	    status = pst_treedup(sess_cb, &dup_rb);
	    
	    if (status != E_DB_OK)
		return (status);
	}

	continue;

      case PST_TREE:
      case PST_QLEND:
	return (E_DB_OK);

      case PST_AGHEAD:
      case PST_SUBSEL:
        /* Aggregate or subsel inside some other expression. */
        return(status);

      default:	/* everything but RESDOM, TRE and QLEND */
	if ((*nodep)->pst_left != NULL)
		pst_sqlsort_resdom(sess_cb, stream, err_blk, &(*nodep)->pst_left);
	if ((*nodep)->pst_right == NULL) return (E_DB_OK);
	nodep = &(*nodep)->pst_right;
    }	/* end of switch */

    return(status);
}
