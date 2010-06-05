/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <bt.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSTRNGFCN.C - Range table functions
**
**  Description:
**      This file contains the functions for manipulating the range table.
**	The main operations are: initialize range table, enter a range
**	variable, look up a range variable, delete a range variable, and
**	check whether a range variable is updateable.
**
**          pst_rginit - Initialize range table
**          pst_rglook - Look up a range variable
**	    pst_rgent  - Enter a range variable in the range table
**	    pst_rgdel  - Delete a range variable
**	    pst_rgrent - Re-enter a range variable
**	    pst_sent   - SQL method of entering range variables
**	    pst_slook  - SQL method of finding range variables
**	    pst_snscope- move some range variables out of scope.
**	    pst_rescope- Mark range entries as in scope again
**	    pst_rgfind - Find a range variable for a given var. no.
**	    pst_sdent  - create range table entry for derived table or 
**			with list element
**	    pst_swelem - locate table amongst with list elements in range table
**
**
**  History:    $Log-for RCS$
**      25-mar-86 (jeff)    
**          written
**	01-dec-88 (stec)
**	    Change pst_sent.
**	18-jan-89 (stec)
**	    Change pst_slook to implement new processing
**	    scheme for table designators.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  15-aug-91 (kirke)
**	    Added pss_rgno check to pst_snscope().
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	12-jun-92 (barbara)
**	    Sybil merge.  Changed interface to pst_showtab which necessitated
**	    a change to the interface of pst_sent, pst_rglook, pst_rgent
**	    and pst_rgrent.
**	03-aug-92 (barbara)
**	    Wherever we call rdf_call, call pst_rdfcb_init to initialize
**	    RDF_CB.
**      23-mar-93 (smc)
**          Cast parameter of BTtest() to match prototype.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**      2-mar-95 (inkdo01)
**          Bug 67258 - fix array dimension in pst_slook.
**	22-may-96 (kch)
**	    Correct previous change in pst_slook() - increase the number of
**	    elements in sort_list_start[] by 1. This change fixes bug 76616
**	    (and 67258).
**	23-Jun-2006 (kschendel)
**	    Unfix wasn't testing for set-input DBP dummy formal, or for the
**	    new derived table thing.  Caused RD0003's unfixing nonsense.
**      30-mar-2009 (gefei01) bug 121870
**          Disallow DML operation in table procedure.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_template@]...
**/

static bool
pst_gb_search(
	PST_QNODE	*root);


/*{
** Name: pst_rginit	- Initialize the range table.
**
** Description:
**      This function initializes a range table for a session.  It links all of
**	the variables together into a QUEUE, and marks the variables as unused.
**	It also NULLs out the RDR_INFO pointerfor each entry, so we can tell
**	that they haven't been used during the current query.
**
** Inputs:
**      rangetab                        Pointer to range table
**
** Outputs:
**      rangetab                        Range table initialized
**        pss_qhead                       All range variables put in QUEUE.
**        pss_rngtab[]
**	    pss_used			    Set to FALSE.
**	    pss_rngque			    Linked into range variable QUEUE.
**	    pss_rdrinfo			    Set to NULL
**	    pss_rgno			    Set to -1 to indicate not assigned
**					    during current query
**
**	Returns:
**	    E_DB_OK			Always succeeds
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-mar-86 (jeff)
**          adapted from rnginit() in 3.0
**	14-apr-88 (stec)
**	    Initialize pss_rgtype field.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	21-oct-97 (inkdo01)
**	    Init the stupid outer join maps, too.
*/
DB_STATUS
pst_rginit(
	PSS_USRRANGE       *rangetab)
{
    register i4         i;

    /* Initialize QUEUE head */
    rangetab->pss_qhead.q_next = &rangetab->pss_qhead;
    rangetab->pss_qhead.q_prev = &rangetab->pss_qhead;

    /* Initialize max range variable number */
    rangetab->pss_maxrng = -1;

    /* Link all variables into QUEUE */
    for (i = 0; i < PST_NUMVARS; i++)
    {
	rangetab->pss_rngtab[i].pss_used = FALSE;
	rangetab->pss_rngtab[i].pss_rdrinfo = (RDR_INFO *) NULL;
	rangetab->pss_rngtab[i].pss_rgno = -1;
	rangetab->pss_rngtab[i].pss_rgtype = PST_UNUSED;
	MEfill(sizeof(PST_J_MASK), (u_char) 0, 
		(PTR) &rangetab->pss_rngtab[i].pss_outer_rel);
	MEfill(sizeof(PST_J_MASK), (u_char) 0, 
		(PTR) &rangetab->pss_rngtab[i].pss_inner_rel);
	QUinsert((QUEUE *) &rangetab->pss_rngtab[i],
	    (QUEUE *) &rangetab->pss_qhead.q_next);
    }
    rangetab->pss_rsrng.pss_used = FALSE;
    rangetab->pss_rsrng.pss_rdrinfo = (RDR_INFO *) NULL;
    rangetab->pss_rsrng.pss_rgno = -1;
    rangetab->pss_rsrng.pss_rgtype = PST_UNUSED;
    return (E_DB_OK);
}

/*{
** Name: pst_rglook	- Look up a range variable
**
** Description:
**      This function looks up a range variable by name.  If it finds the
**	variable, it puts the variable at the head of the QUEUE and returns
**	a pointer to it.  If it doesn't find it, it returns NULL.
**
**	Before Jupiter, this function could look up a range variable by range
**	variable name or by table name.  Now it will only look up by range
**	variable name.  The table name lookup was used for the result table of
**	an append; the result table for appends is now kept outside the range
**	table.
**
** Inputs:
**	sess_cb				Pointer to session control block
**      rngtable                        Pointer to the range table
**	scope				Range variable level. -1 means don't
**					care.
**      name                            Pointer to the range variable name
**	getdesc				TRUE means to get the table description
**	result				Pointer to place to put pointer to range
**					variable.
**	query_mode			query mode to pass to pst_showtab
**	err_blk				Place to put error information
**
** Outputs:
**	result				Filled in with pointer to range variable
**					or NULL if not found.
**	err_blk				Filled in with error information if an
**					error occurs.
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory is getdesc is TRUE.
**
** History:
**	25-mar-86 (jeff)
**          adapted from rnglook() in 3.0 parser
**	17-dec-86 (daved)
**	    if table not found, range variable doesn't exist
**	18-feb-87 (daved)
**	    add scope (for SQL) to the name space of range variables
**	03-oct-88 (andre)
**	    modified to receive query mode and pass it to pst_showtab
**	12-jun-92 (barbara)
**	    As part of Sybil merge, pass in sess_cb and remove sessid
**	    and user id parameters (for changed interface to pst_showtab).
*/
DB_STATUS
pst_rglook(
	PSS_SESBLK	   *sess_cb,
	PSS_USRRANGE       *rngtable,
	i4		   scope,
	char               *name,
	bool		   getdesc,
	PSS_RNGTAB	   **result,
	i4		   query_mode,
	DB_ERROR	   *err_blk)
{
    register PSS_RNGTAB *rptr = (PSS_RNGTAB *) rngtable->pss_qhead.q_next;
    char                namepad[DB_TAB_MAXNAME];
    DB_STATUS		status = E_DB_OK;
    i4		err_code;

    
    /* normalize the range variable name */
    STmove(name, ' ', DB_TAB_MAXNAME, namepad);

    /* Look for the range variable */
    while ((QUEUE *) rptr != &rngtable->pss_qhead)
    {
	if (rptr->pss_used && 
	    (rptr->pss_rgparent <= scope || scope == -1) &&
	    !MEcmp(namepad, rptr->pss_rgname, DB_TAB_MAXNAME))
	{
	    break;
	}

	rptr = (PSS_RNGTAB *) rptr->pss_rngque.q_next;
    }

    /* Return to beginning of QUEUE means not found */
    if ((QUEUE *) rptr == &rngtable->pss_qhead)
    {
	*result = rptr = (PSS_RNGTAB *) NULL;
	return (E_DB_OK);
    }

    /* If found, move to head of QUEUE */
    QUremove((QUEUE *) rptr);
    QUinsert((QUEUE *) rptr, (QUEUE *) &rngtable->pss_qhead.q_next);

    /*
    ** Get the table description if asked for and we don't already have it.
    */
    if (getdesc && rptr->pss_rdrinfo == (RDR_INFO *) NULL)
    {
	status = pst_showtab(sess_cb, PST_SHWID, (DB_TAB_NAME *) NULL,
	    (DB_TAB_OWN *) NULL, &rptr->pss_tabid, FALSE, rptr,
	    query_mode, err_blk);
	if (status != E_DB_OK)
	{
	    /*
	    ** On error, undeclare the range variable and move it to the
	    ** end of the QUEUE.  This strategy means that we won't keep
	    ** re-using bad range variables.
	    */
	    rptr->pss_used = FALSE;
	    QUremove((QUEUE *) rptr);
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);
	    *result = rptr = (PSS_RNGTAB *) NULL;
	    return (E_DB_OK);
	}
    }

    /* If this range variable does not have a number yet, give it one */
    if (rptr->pss_rgno == -1)
    {
	rptr->pss_rgno = ++rngtable->pss_maxrng;
	if (rptr->pss_rgno > PST_NUMVARS - 1)
	{
	    (VOID) psf_error(3000L, 0L, PSF_USERERR,
		&err_code, err_blk, 0);
	    *result = (PSS_RNGTAB *) NULL;
	    return (E_DB_ERROR);
	}
    }

    /* Set result pointer */
    *result = rptr;

    return (status);
}

/*{
** Name: pst_rgent	- Enter a range variable in the range table.
**
** Description:
**      This function enters a range variable in the range table.  If a
**	range variable by the same name already exists, it re-defines it.
**	It describes the table entry by using the pst_showtab function. 
**
**	One can enter a range variable by table name or by table id.
**	Entering a range variable by name forces DMF to do a catalog
**	read; doing it by table id allows it to use its internal cache of
**	table descriptions.
**
**	Optionally, one can tell this function not to bother with column
**	descriptions.  This is useful for the "range of..." statement; this
**	statement doesn't need a the column descriptions, and we throw out
**	the table description at the end of the parse, so what would be the
**	point of getting the column descriptions?
**
** Inputs:
**	sess_cb				Pointer to session control block
**      rngtable                        Pointer to the user range table
**	scope				scope that range variable belongs in
**					-1 means don't care.
**	varname				Name of the range variable
**	showtype			Method to get table description:
**					  PST_SHWID - By table id
**					  PST_SHWNAME - By table name
**					  PST_SHWEITHER - By either
**	tabname				Pointer to table name, if showing by
**					name
**	tabown				Pointer to table owner name, if showing
**					by name
**	tabid				Pointer to table id, if showing by
**					table id
**	tabonly				TRUE means don't get attribute info
**	rngvar				Place to put pointer to new range
**					variable
**	query_mode			query mode to pass to pst_showtab
**	err_blk				Place to put error information
**
** Outputs:
**      rngvar                          Set to point to the range variable
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**
** History:
**	24-mar-86 (jeff)
**          Adapted from rngent in 3.0 parser.
**	18-feb-87 (daved)
**	    add scope for SQL.
**	03-oct-88 (andre)
**	    modified to receive and pass query mode to pst_showtab
**	01-dec-88 (stec)
**	    Change code to execute QUinsert only when rptr not
**	    equal to &rngtable->pss_rsrng.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**	21-may-90 (teg)
**	    init rdr_instr to RDF_NO_INSTR
**	26-jun-90 (andre)
**	    if caller wants he result placed in pss_rsrng, there is no reason to
**	    call pst_rglook();
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    fix bug 38275: do not look for a range entry with the same name if
**	    the name is set to "".
**	    The change can be justified on the grounds of efficiency since the
**	    entry (if any) found by pst_rglook() will be discarded if
**	    varname=="".
**	    The bug itself was related to the change in the query tree header
**	    structure which replaced a fixed size array of range entries with a
**	    variable size array of pointers and added a correlation name to a
**	    range entry.  For views created before this change, correlation name
**	    is set to blanks, and the code in psy_view() copies the correlation
**	    name into pss_rgname when merging views into the query tree.  As a
**	    result we may end up having range entries in pss_auxrng with
**	    pss_rgname set to all blanks.  When psy_dinteg() calls pst_rgent()
**	    to look up a table description, it sets varname to "".  Then
**	    pst_rglook() "finds" a range entry with a matching name, and
**	    increments pss_maxrng (to 0) since pss_rgno is -1.  Back in
**	    pst_rgent(), the entry found by pst_rglook() is discarded, and
**	    another entry is selected.  The new entry's pss_rgno is also -1, so
**	    pss_maxrng is incremented (to 1) and the value is stored in
**	    pss_rgno.  Unfortunately, the query tree header has an entry
**	    corresponding to range number of 0 and not to 1, psy_dinteg() still
**	    behaves as if the range variable has pss_rgno of 0, and all hell
**	    breaks loose when OPF tries to find an entry[1] in the query tree
**	    header.
**	20-nov-91 (rog)
**	    Added a missing query_mode argument to a pst_rglook() call.
**	12-jun-92 (barbara)
**	    As part of Sybil merge, pass in sess_cb and remove sessid
**	    and user id parameters; also change interface to pst_showtab.
**	10-jul-93 (andre)
**	    cast the second arg to QUinsert() as (QUEUE *) to agree with the
**	    prototype declaration
**	10-aug-93 (andre)
**	    removed cause of a compiler warning
*/
DB_STATUS
pst_rgent(
	PSS_SESBLK	   *sess_cb,
	PSS_USRRANGE	   *rngtable,
	i4		    scope,
	char               *varname,
	i4		   showtype,
	DB_TAB_NAME	   *tabname,
	DB_TAB_OWN	   *tabown,
	DB_TAB_ID	   *tabid,
	bool               tabonly,
	PSS_RNGTAB	   **rngvar,
	i4		   query_mode,
	DB_ERROR	   *err_blk)
{
    register PSS_RNGTAB *rptr;
    PSS_RNGTAB		*rp;
    DB_STATUS           status;
    i4		err_code;

    /* Variable name starting with '!' means don't try to replace */
    if (*varname == '!')
    {
	rptr = (PSS_RNGTAB *) NULL;
    }
    else if (*varname != EOS)
    {
	/* Look for range variable of same name */
	status = pst_rglook(sess_cb, rngtable, scope, varname, FALSE, 
	    &rp, query_mode, err_blk);
	if (status != E_DB_OK)
	{
	    *rngvar = (PSS_RNGTAB *) NULL;
	    return (status);
	}
	rptr = rp;
    }

    /* Blank range variable name means result variable */
    if (*varname == EOS)
    {
	rptr = &rngtable->pss_rsrng;
    }
    else
    {
	if (rptr == (PSS_RNGTAB *) NULL)
	{
	    /* Not in range table, replace variables on LRU basis */
	    rptr = (PSS_RNGTAB *) QUremove(rngtable->pss_qhead.q_prev);
	    /* Because the aux range table has the used field cleared
	    ** prior to getting the new variables, if
	    ** the last LRU range entry is used, we are using too many
	    ** variables.
	    */
	    if (rptr->pss_used == TRUE && rptr->pss_rgno > -1)
	    {
		QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);
		(VOID) psf_error(3100L, 0L, PSF_USERERR,
		    &err_code, err_blk, 0);
		*rngvar = (PSS_RNGTAB *) NULL;
		return (E_DB_ERROR);
	    }
	}
	else
	{
	    /* Found in range table; re-declare with new table */
	    QUremove((QUEUE *) rptr);
	}
    }

    /* Unfix existing definition */

    status = pst_rng_unfix(sess_cb, rptr, err_blk);
    if (DB_FAILURE_MACRO(status))
    {
	/* Put the range variable at the head of the queue */
	if (rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);
	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }
    rptr->pss_rgtype  = PST_TABLE;
    rptr->pss_rgparent= scope;
    rptr->pss_qtree   = 0;

    /* Get the table description */
    status = pst_showtab(sess_cb, showtype, tabname, tabown, tabid, tabonly,
	rptr, query_mode, err_blk);
    if (status != E_DB_OK)
    {
	/* Put the range variable at the head of the queue */
	if (rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }

    /* Mark the range variable as used */
    rptr->pss_used = TRUE;

    /* Store away the range variable name */
    STmove(varname, ' ', sizeof(rptr->pss_rgname), (char *) rptr->pss_rgname);

    /* Put the range variable at the head of the queue */
    if (rptr != &rngtable->pss_rsrng)
	QUinsert((QUEUE *) rptr, (QUEUE *) &rngtable->pss_qhead.q_next);

    /* If this range variable doesn't have a number yet, give it one */
    if (rptr->pss_rgno == -1)
    {
	rptr->pss_rgno = ++rngtable->pss_maxrng;
	if (rptr->pss_rgno > PST_NUMVARS - 1)
	{
	    (VOID) psf_error(3100L, 0L, PSF_USERERR,
		&err_code, err_blk, 0);
	    *rngvar = (PSS_RNGTAB *) NULL;
	    return (E_DB_ERROR);
	}
    }

    /* Give the range variable back to the caller */
    *rngvar = rptr;

    return (E_DB_OK);
}

/*{
** Name: pst_rgdel	- Delete all references in a range table to a given
**			    table name.
**
** Description:
**      This function deletes all references in the range table to a table of
**	a given name.  This is necessary when doing a create, for example; if
**	a user creates his or her own table, we don't want to keep references
**	to the DBA's table around, because that should become invisible.
**
**	NOTE: we don't have to worry about the result range variable here,
**	because it is deallocated after each parse.
**
** Inputs:
**	sess_cb				Pointer to session control block
**      tabname                         The table name to expunge
**	rngtable			Pointer to the range table
**	memleft				Pointer to amount of memory left
**	err_blk				Place to put error info
**
** Outputs:
**      err_blk                         Filled in with error info if error
**					occurs
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can deallocate memory
**
** History:
**	25-mar-86 (jeff)
**          adapted from rngdel in 3.0 parser
**	22-jun-89 (andre)
**	    set status to E_DB_OK in case there is no table description to
**	    deallocate
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**	21-may-90 (teg)
**	    init rdr_instr to RDF_NO_INSTR
**	12-jun-92 (barbara)
**	    As part of Sybil merge, pass in sess_cb.
**	10-aug-93 (andre)
**	    removed cause of a compiler warning
*/
DB_STATUS
pst_rgdel(
	PSS_SESBLK	   *sess_cb,
	DB_TAB_NAME        *tabname,
	PSS_USRRANGE       *rngtable,
	DB_ERROR           *err_blk)
{
    register PSS_RNGTAB  *rptr;
    i4			 i;
    DB_STATUS            status = E_DB_OK;

    /* Run through the whole range table */
    for (rptr = &rngtable->pss_rngtab[0], i = 0;
         i < PST_NUMVARS; i++, rptr++)
    {
	/* Only delete those that are in use and have the right table name */
	if (rptr->pss_used &&
	!MEcmp((char *) tabname, rptr->pss_tabname.db_tab_name,
	    sizeof(DB_TAB_NAME)))
	{
	    status = pst_rng_unfix(sess_cb, rptr, err_blk);

	    /* Mark entry unused */
	    rptr->pss_used = FALSE;

	    /* Put it at the end of the QUEUE */
	    QUremove((QUEUE *) rptr);
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	    /* If rdf_call failed, return an error */
	    if (status != E_DB_OK)
	    {
		return (status);
	    }
	}
    }

    return (E_DB_OK);
}

/*{
** Name: pst_rgrent	- Re-enter a range variable description
**
** Description:
**      Given a range table slot and a table id, this function gets rid of the
**	old description and creates a new one.  The new table id does not have
**	to be the same as the old one.  In fact, it will usually be different.
**
** Inputs:
**	sess_cb				Pointer to session control block
**      rngvar                          Pointer to range table slot to re-enter
**	tabid				Pointer to new table id
**	query_mode			Query mode
**	err_blk				Filled in if an error happens
**
** Outputs:
**      rngvar                          Filled in with new table description
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Frees and gets RDF descriptions.
**
** History:
**	20-jun-86 (jeff)
**          written
**	03-oct-88 (andre)
**	    modified to receive query mode and pass it to pst_showtab
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**	21-may-90 (teg)
**	    init rdr_instr to RDF_NO_INSTR
**	12-jun-92 (barbara)
**	    As part of Sybil merge, pass in sess_cb and remove sessid
**	    and user id parameters and change interface to pst_showtab.
**	10-aug-93 (andre)
**	    removed cause of a compiler warning
*/
DB_STATUS
pst_rgrent(
	PSS_SESBLK	   *sess_cb,
	PSS_RNGTAB         *rngvar,
	DB_TAB_ID	   *tabid,
	i4		   query_mode,
	DB_ERROR	   *err_blk)
{
    DB_STATUS           status;

    /* Unfix the cached description.
    ** The few callers of this routine apparently always pass a real
    ** object (eg a result table), not a dummy, but there is no harm
    ** in using the regular routine for this anyway.
    */

    status = pst_rng_unfix(sess_cb, rngvar, err_blk);
    if (status != E_DB_OK)
	return (status);

    /* Get the new description */
    status = pst_showtab(sess_cb, PST_SHWID, NULL, NULL, tabid, FALSE,
	rngvar, query_mode, err_blk);
    if (status != E_DB_OK)
    {
	return (status);
    }

    return (E_DB_OK);
}

/*{
** Name: pst_rgcopy	- Copy one range table to another
**
** Description:
**      This function copies one range table to another, including any
**	table descriptions therein.  It is assumed that the "from" table
**	will no longer be used for the duration of this parse.  This makes
**	it safe to simply copy the table info from the "from" range table
**	to the "to" range table, and NULL out any pointers in the "from"
**	table.  Therefore, we don't have to tell RDF that we're getting an
**	extra copy of each description (since there will be only one copy
**	of each one).
**
**	This function makes no effort to preserve the LRU ordering of the
**	"from" table.  LRU ordering is only necessary for the user range
**	table, which will usually be the "from" table.
**
** Inputs:
**      fromtab                         The "from" range table
**	totab				The "to" range table
**	resrng				result range pointer
**	err_blk				Filled in if an error happens
**
** Outputs:
**      fromtab                         RDF pointers NULLed out
**	totab				Range variables copied to here
**	resrng				new result range pointer
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      09-jul-86 (jeff)
**          written
**      29-aug_86 (seputis)
**          fixed MEcopy parameters
**	23-dec-86 (daved)
**	    set result range pointer
**	10-jan-93 (andre)
**	    changed to use STRUCT_ASSIGN_MACRO to copy the whole structure - as
**	    a result, we will not need to worry about individual fields as they
**	    get added.  Note that by using STRUCT_ASSIGN_MACRO we clobber
**	    pss_rngque, but that should not cause any problems since the entry
**	    has been removed from the list and will be reinserted after a copy
**	    has been made
**	10-jul-93 (andre)
**	    cast the second arg to QUinsert() as (QUEUE *) to agree with the
**	    prototype declaration
*/
DB_STATUS
pst_rgcopy(
	PSS_USRRANGE	    *fromtab,
	PSS_USRRANGE	    *totab,
	PSS_RNGTAB	    **resrng,
	DB_ERROR	    *err_blk)
{
    register PSS_RNGTAB *fromrng;
    register PSS_RNGTAB *torng;
    register i4         i;

    /* for safety, this value is used in rgent in during qrymod to catch
    ** too many vars for query.
    */
    for (i = 0; i < PST_NUMVARS; i++)
	totab->pss_rngtab[i].pss_used = FALSE;

    for (i = 0; i < PST_NUMVARS; i++)
    {
	fromrng = &fromtab->pss_rngtab[i];
	if (fromrng->pss_used && fromrng->pss_rgno > -1)
	{
	    /* get vars in LRU order so the won't get replaced in qrymod */
	    torng = (PSS_RNGTAB*) QUremove(totab->pss_qhead.q_prev);
	    if (*resrng == fromrng)
		*resrng = torng;

	    /*
	    ** - make a copy of the *fromrng
	    ** - then set pointers to table/attribute descriptions in *fromrng
	    **   to NULL to avoid attempts to unfix them from both the original
	    **   range entry and from the copy
	    ** - finally insert the copied range entry into the queue of range
	    **   entries
	    */
	    STRUCT_ASSIGN_MACRO((*fromrng), (*torng));

	    fromrng->pss_colhsh = (RDD_ATTHSH *) NULL;
	    fromrng->pss_tabdesc = (DMT_TBL_ENTRY *) NULL;
	    fromrng->pss_attdesc = (DMT_ATT_ENTRY **) NULL;
	    fromrng->pss_rdrinfo = (RDR_INFO *) NULL;

	    QUinsert((QUEUE *) torng, (QUEUE *) &totab->pss_qhead.q_next);
	}
    }

    fromrng = &fromtab->pss_rsrng;
    torng = &totab->pss_rsrng;

    torng->pss_used = fromrng->pss_used;
    if (fromrng->pss_used && fromrng->pss_rgno > -1)
    {
	if (*resrng == fromrng)
	    *resrng = torng;
        torng->pss_rgno = fromrng->pss_rgno;
        MEcopy((PTR)fromrng->pss_rgname, DB_TAB_MAXNAME, (PTR)torng->pss_rgname);
        STRUCT_ASSIGN_MACRO(fromrng->pss_tabid, torng->pss_tabid);
        STRUCT_ASSIGN_MACRO(fromrng->pss_tabname, torng->pss_tabname);
        STRUCT_ASSIGN_MACRO(fromrng->pss_ownname, torng->pss_ownname);
        torng->pss_colhsh = fromrng->pss_colhsh;
        fromrng->pss_colhsh = (RDD_ATTHSH *) NULL;
        torng->pss_tabdesc = fromrng->pss_tabdesc;
        fromrng->pss_tabdesc = (DMT_TBL_ENTRY *) NULL;
        torng->pss_attdesc = fromrng->pss_attdesc;
        fromrng->pss_attdesc = (DMT_ATT_ENTRY **) NULL;
        torng->pss_rdrinfo = fromrng->pss_rdrinfo;
        fromrng->pss_rdrinfo = (RDR_INFO *) NULL;
	torng->pss_permit =  fromrng->pss_permit;
	torng->pss_rgtype =  fromrng->pss_rgtype;
	torng->pss_rgparent = fromrng->pss_rgparent;
	torng->pss_qtree = fromrng->pss_qtree;
    }
    totab->pss_maxrng = fromtab->pss_maxrng;

    return (E_DB_OK);
}

/*{
** Name: pst_snscope 	- Mark range entries as out of scope
**
** Description:
**	This function takes the variables in a from list and marks them
**	as inaccessable.
**
** Inputs:
**      rngtable                        Pointer to the range table
**	from_list			range variables to mark.
**	offset				amount of adjustment to use
**					to make inaccessible.
**					Usually MAXI1 or n*MAXI1
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	4-mar-87 (daved)
**	    written
**	17-aug-87 (stec)
**	    Place out of scope by adding MAXI1 rather than setting it to MAXI2.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  15-aug-91 (kirke)
**	    Added check for pss_rgno > -1.  Unknown behavior will result
**	    (including a possible core dump) if we call BTtest() with -1.
**	26-nov-02 (inkdo01)
**	    Range table expansion (from_list is PST_J_MASK).
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects - allow different offsets for putting out of scope
**	    so that some could be brought back into scope by pst_rescope.
**	    This arises in union contexts where the primary union's scope is needed
**	    in order by contexts.
*/
DB_STATUS
pst_snscope(
	PSS_USRRANGE       *rngtable,
	PST_J_MASK	   *from_list,
	i4		    offset)
{
    register PSS_RNGTAB *rptr = (PSS_RNGTAB *) rngtable->pss_qhead.q_next;

    /* Look for the range variable */
    while ((QUEUE *) rptr != &rngtable->pss_qhead)
    {
	if (rptr->pss_used && rptr->pss_rgno > -1 &&
	    BTtest(rptr->pss_rgno, (char *)from_list))
	    rptr->pss_rgparent += offset;
	rptr = (PSS_RNGTAB *) rptr->pss_rngque.q_next;
    }
}

/*{
** Name: pst_rescope 	- Mark range entries as in scope again
**
** Description:
**	This function takes the variables in a from list and marks them
**	as accessable if they were suitably marked.
**
** Inputs:
**      rngtable                        Pointer to the range table
**	from_list			range variables to mark.
**	offset				amount off adjustment used to
**					previously make inaccessible
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	26-nov-02 (kiria01)
**	    Created.
*/
DB_STATUS
pst_rescope(
	PSS_USRRANGE       *rngtable,
	PST_J_MASK	   *from_list,
	i4		    offset)
{
    register PSS_RNGTAB *rptr = (PSS_RNGTAB *) rngtable->pss_qhead.q_next;

    /* Look for the range variable */
    while ((QUEUE *) rptr != &rngtable->pss_qhead)
    {
	if (rptr->pss_used && rptr->pss_rgno > -1 &&
		rptr->pss_rgparent >= offset)
	    rptr->pss_rgparent -= offset;
	rptr = (PSS_RNGTAB *) rptr->pss_rngque.q_next;
    }
}

/*{
** Name: pst_sent	- Enter a range variable in the range table for SQL
**
** Description:
**      This function enters a range variable in the range table. 
**	It describes the table entry by using the pst_showtab function. 
**
**	One can enter a range variable by table name or by table id.
**	Entering a range variable by name forces DMF to do a catalog
**	read; doing it by table id allows it to use its internal cache of
**	table descriptions.
**
**	Normally a range variable is taken from the free queue.  If
**	the caller doesn't supply a range table header at all, the
**	range variable to use is passed in via *rngvar.  (probably a
**	local temp of some kind, and the caller is responsible for
**	Doing All The Right Things.)  If the variable name is the
**	empty string, it's the result range variable and the pss_rsrng
**	member of the range table header is used.
**
** Inputs:
**	sess_cb				Pointer to session control block
**      rngtable                        Pointer to the user range table
**	scope				scope that range variable belongs in
**					-1 means don't care.
**	varname				Name of the range variable
**	showtype			Method to get table description:
**					  PST_SHWID - By table id
**					  PST_SHWNAME - By table name
**					  PST_SHWEITHER - By either
**	tabname				Pointer to table name, if showing by
**					name
**	tabown				Pointer to table owner name, if showing
**					by name
**	tabid				Pointer to table id, if showing by
**					table id
**	tabonly				TRUE means don't get attribute info
**	rngvar				Place to put pointer to new range
**					variable
**	query_mode			Query mode.
**	err_blk				Place to put error information
**
** Outputs:
**      rngvar                          Set to point to the range variable
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**
** History:
**	4-mar-87 (daved)
**	    written
**	03-oct-88 (andre)
**	    modified to receive query mode and pass it to pst_showtab
**	01-dec-88 (stec)
**	    Check if rptr->pss_rdrinfo is null, if not - call RDF to
**	    unfix.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**	21-may-90 (teg)
**	    init rdr_instr to RDF_NO_INSTR
**	12-jun-92 (barbara)
**	    As part of Sybil merge, pass in sess_cb and remove sessid
**	    and user id parameters and change interface to pst_showtab.
**	10-jul-93 (andre)
**	    cast the second arg to QUinsert() as (QUEUE *) to agree with the
**	    prototype declaration
**	10-aug-93 (andre)
**	    removed cause of a compiler warning
**	23-Jun-2006 (kschendel)
**	    Add special case where caller can supply range variable.
*/
DB_STATUS
pst_sent(
	PSS_SESBLK	   *sess_cb,
	PSS_USRRANGE	   *rngtable,
	i4		    scope,
	char               *varname,
	i4		   showtype,
	DB_TAB_NAME	   *tabname,
	DB_TAB_OWN	   *tabown,
	DB_TAB_ID	   *tabid,
	bool               tabonly,
	PSS_RNGTAB	   **rngvar,
	i4		   query_mode,
	DB_ERROR	   *err_blk)
{
    register PSS_RNGTAB *rptr;
    PSS_RNGTAB		*rp;
    DB_STATUS           status;
    i4		err_code;

    rptr = (PSS_RNGTAB *) NULL;

    /* Blank range variable name means result variable.
    ** Null range table ptr means use caller supplied range variable.
    */
    if (rngtable == NULL)
	rptr = *rngvar;
    else if (*varname == '\0')
	rptr = &rngtable->pss_rsrng;
    else
	rptr = (PSS_RNGTAB *) QUremove(rngtable->pss_qhead.q_prev);


    /* Free up table description if it exists, but not if
    ** it's a dummy set-input range variable, or a dummy derived table
    ** range variable.
    */
    status = pst_rng_unfix(sess_cb, rptr, err_blk);
    if (DB_FAILURE_MACRO(status))
    {
	/* Put the range variable at the head of the queue */
	if (rngtable != NULL && rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }

    rptr->pss_rgtype  = PST_TABLE;
    rptr->pss_rgparent= scope;
    rptr->pss_qtree   = 0;

    /* Get the table description */
    status = pst_showtab(sess_cb, showtype, tabname, tabown, tabid, tabonly, 
	rptr, query_mode, err_blk);
    if (status != E_DB_OK)
    {
	/* Put the range variable at the head of the queue */
	if (rngtable != NULL && rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }

    /* Mark the range variable as used */
    rptr->pss_used = TRUE;

    /* Store away the range variable name */
    STmove(varname, ' ', sizeof(rptr->pss_rgname), (char *) rptr->pss_rgname);

    /* Put the range variable at the head of the queue */
    rptr->pss_rgno = 0;
    if (rngtable != NULL)
    {
	if (rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, (QUEUE *) &rngtable->pss_qhead.q_next);
	rptr->pss_rgno = ++rngtable->pss_maxrng;
    }
    if (rptr->pss_rgno > PST_NUMVARS - 1)
    {
	(VOID) psf_error(3100L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	*rngvar = (PSS_RNGTAB *) NULL;
	return (E_DB_ERROR);
    }

    /* Give the range variable back to the caller */
    *rngvar = rptr;

    return (E_DB_OK);
}

/*{
** Name: pst_slook	- Look up a range variable SQL style.
**
** Description:
**	This procedure implements processing scheme for SQL table
**	designators. A table designator is a table name, in case when
**	correlation name is not present, or a correlation name itself,
**	if it has been specified (table name is not exposed in this case). 
**	The way range table entries are filled out is such that pss_rgname
**	value is effectively a table designator.
**	The range var must be at the current scope or higher.
**	Each range entry is checked for matching table designator (pss_rgname).
**	If two such range entries are found at the same scope, an ambiguous FROM
**	list error is generated.
**
**	If it finds the variable, it puts the variable at the head of the 
**	QUEUE and returns a pointer to it, else NULL is returned.
**
**	
**
** Inputs:
**      rngtable                        Pointer to the range table
**	cb				session control block
**	    pss_qualdepth		scope
**      name                            Pointer to the range variable name
**	result				Pointer to place to put pointer to range
**					variable.
**	err_blk				Place to put error information
**	scope_flag			boolean flag indicating whether range vars
**					at current scope only should be checked
**					(required for target list processing)
**					If TRUE only current scope is to be checked.
** Outputs:
**	result				Filled in with pointer to range variable
**					or NULL if not found.
**	err_blk				Filled in with error information if an
**					error occurs.
**		2921			ambiguous from list
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory is getdesc is TRUE.
**
** History:
**	4-mar-87 (daved)
**	    written
**	18-jan-89 (stec)
**	    Change pst_slook to implement new processing
**	    scheme for table designators. Make sure that
**	    for target lists only current scope is checked.
**	25-sep-92 (andre)
**	    add ability to resolve <table reference>s of form <schema>.<table>.
**	    
**	    We will search for a range variable based on the specification
**	    provided by the caller proceeding from the current scope outward (as
**	    opposed to simply scanning the entire list.)  This will ensure that
**	    queries that worked before will continue to work, but some queries
**	    which may have failed before, will now work.  The main reason for
**	    choosing this order of traversal is to ensure that queries work or
**	    fail consistently, regardless of the order in which their
**	    syntactical elements were specified.
**	    
**	    If a <table name> without a <schema name> is specified, at first we
**	    will apply INGRES algorithm whereby T refers to a table named T or
**	    to a table with explicit correlation name equal to T.  If at any
**	    point this results in an "ambiguous reference" error, we will apply
**	    the SQL92-compliant method whereby T refers to tables with explicit
**	    correlation name equal to T or to table T in the default schema of
**	    the current SQL-session user.  If both the backward-compatible and
**	    SQL92-compliant methods fail to resolve the ambiguity, then so be
**	    it.
**
**	    If <schema name>.<table name> was specified, we will search for a
**	    range variable with matching schema and table name for which no
**	    correlation name was specified.
**      2-mar-95 (inkdo01)
**          Bug 67258 - fix array dimension of sort_list_start to be 
**          consistent with funny qualdepth convention used in psl_tbl_ref
**          (in pslsgram).
**	22-may-96 (kch)
**	    Correct previous change to this function - increase the number of
**	    elements in sort_list_start[] by 1. This change fixes bug 76616
**	    (and 67258).
*/
DB_STATUS
pst_slook(
	PSS_USRRANGE       *rngtable,
	PSS_SESBLK	   *cb,
	DB_OWN_NAME	   *schema_name,
	char               *name,
	PSS_RNGTAB	   **result,
	DB_ERROR	   *err_blk,
	bool		   scope_flag)
{
    register PSS_RNGTAB *rptr = (PSS_RNGTAB *) rngtable->pss_qhead.q_next;
    PSS_RNGTAB		*first = (PSS_RNGTAB *) NULL;
    char                namepad[DB_TAB_MAXNAME];
    i4		err_code;
    i4		err_num = 0L;
    i4			scope = cb->pss_qualdepth;
    i4			cur_scope = -1;
    i4			i;
    struct PST_SLOOK_SORT_
    {
	struct PST_SLOOK_SORT_	*pst_next;
	PSS_RNGTAB		*pst_elem;
    } sort_arr[PST_NUMVARS];

	/*
	** sort_list_start[0] will point to the beginning of the list of range
	** vars at specified scope, sort_list_start[1] will point to the
	** beginning of the list of range vars at the necxt surrounding scope,
	** etc.
	*/
    struct PST_SLOOK_SORT_  *sort_list_start[MAXI1 + 2];
    struct PST_SLOOK_SORT_  *list_elem;

    /*
    ** first we need to sort elements of the range table according to scope.
    ** If scope_flag is set, we are only interested in elements with
    ** scope == cb->pss_qualdepth; otherwise we are interested in elements with
    ** scope <= cb->pss_qualdepth
    */
    for (i = 0; i <= (MAXI1 + 1); i++)
	sort_list_start[i] = (struct PST_SLOOK_SORT_ *) NULL;

    /*
    ** here we rely on the fact that all range vars used in this query will be
    ** clumped together in the front of the queue
    */
    for (i = 0, rptr = (PSS_RNGTAB *) rngtable->pss_qhead.q_next;
         (i < PST_NUMVARS && rptr->pss_used && rptr->pss_rgno >= 0);
	 i++, rptr = (PSS_RNGTAB *) rptr->pss_rngque.q_next)
    {
	if (   rptr->pss_rgparent > scope
	    || scope_flag && rptr->pss_rgparent != scope)
	{
	    continue;
	}

	sort_arr[i].pst_next = sort_list_start[scope - rptr->pss_rgparent];
	sort_arr[i].pst_elem = rptr;
	sort_list_start[scope - rptr->pss_rgparent] = sort_arr + i;
    }
    
    /* normalize the range variable name */
    STmove(name, ' ', DB_TAB_MAXNAME, namepad);

    if (!schema_name)
    {
	i4	    sql92_rules = FALSE;
	
	for (i = 0; i <= scope; i++)
	{
	    for (list_elem = sort_list_start[i];
		 list_elem;
		 list_elem = list_elem->pst_next)
	    {
		rptr = list_elem->pst_elem;

		/*
		** if the (explicit or implicit) correlation name does not
		** match that passed by the caller, this entry is of no interest
		*/
		if (MEcmp(namepad, rptr->pss_rgname, DB_TAB_MAXNAME))
		    continue;

		/*
		** if (this is the first element with satisfying the rules being
		**     followed (see below))
		** {
		**   remember it;
		** }
		** else if (we were applying backward-compatible rules)
		** {
		**   switch to SQL92-compliant rules;
		** }
		** else if (the reference is ambiguous by SQL92-compliant rules)
		** {
		**   report an error.
		** }
		**
		** Backward-compatible rules:
		**	T applies to a range entry for a table T (regardless of
		**	schema) for which no correlation name was specified
		**	or to a range entry for a table with explicit
		**	correlation name equal to T.  If at any scope we find
		**	more than one range var satisfying this description,
		**	the reference is considered ambiguous.
		**
		** SQL92-compliant rules:
		**	T applies to a range entry for a table T in the default
		**	schema of the current SQL session user or to a range
		**	enrty for a table with explicit correlation name equal
		**	to T.  If at any scope we find more than one range var
		**	satisfying this description, the reference is considered
		**	ambiguous.
		*/
		if (first)
		{
		    if (sql92_rules)
		    {
			if (   rptr->pss_var_mask & PSS_EXPLICIT_CORR_NAME
			    || !MEcmp((PTR) &cb->pss_user,
				      (PTR) &rptr->pss_ownname,
				      sizeof(cb->pss_user))
			   )
			{
			    /* ambiguous reference - report an error */
			    (VOID) psf_error(2921L, 0L, PSF_USERERR,
				&err_code, err_blk, 2,
				(i4) sizeof (cb->pss_lineno),
				(PTR) &cb->pss_lineno,
				(i4) STlength(name), (PTR) name);
			    return (E_DB_ERROR);
			}
		    }
		    else
		    {
			/*
			** this qualifies as ambiguous reference under
			** backward-compatible rules; if it is also ambiguous
			** according to SQL92-compliant rules, report an error,
			** otherwise, switch to SQL92-compliant rules
			*/
			i4 	    first_in_default_schema,
				    cur_in_default_schema;

			first_in_default_schema =
			    !MEcmp((PTR) &cb->pss_user,
				(PTR) &first->pss_ownname,
				sizeof(cb->pss_user));
			cur_in_default_schema =
			    !MEcmp((PTR) &cb->pss_user,
			        (PTR) &rptr->pss_ownname,
				sizeof(cb->pss_user));
			
			if (   rptr->pss_var_mask & PSS_EXPLICIT_CORR_NAME
			    || first->pss_var_mask & PSS_EXPLICIT_CORR_NAME
			    || (   first_in_default_schema
				&& cur_in_default_schema)
			   )
			{
			    (VOID) psf_error(2921L, 0L, PSF_USERERR,
				&err_code, err_blk, 2,
				(i4) sizeof(cb->pss_lineno),
				(PTR) &cb->pss_lineno,
				(i4) STlength(name), (PTR) name);
			    return (E_DB_ERROR);
			}
			else
			{
			    /*
			    ** we will try to resolve the reference using
			    ** SQL92-compliant rules; if either the "first" or
			    ** the current entry corresponds to a table in the
			    ** current user's default schema, remember it;
			    */

			    sql92_rules = TRUE;
			    
			    if (cur_in_default_schema)
				first = rptr;
			    else if (!first_in_default_schema)
				first = (PSS_RNGTAB *) NULL;
			}
		    }
		}
		else
		{
		    if (sql92_rules)
		    {
			/*
			** if the correlation name was explicit or if the table
			** is in the current user's default schema, remember it
			*/
			if (   rptr->pss_var_mask & PSS_EXPLICIT_CORR_NAME
			    || !MEcmp((PTR) &cb->pss_user,
			              (PTR) &rptr->pss_ownname,
				      sizeof(cb->pss_user))
			   )
			{
			    first = rptr;
			}
		    }
		    else
		    {
			first = rptr;
		    }
		}
	    }

	    /*
	    ** if first is set, the search has been successful; if scope_flag is
	    ** set, there is no reason to proceed since we are only interested
	    ** in vars at current scope
	    */
	    if (first || scope_flag)
		break;
	}

	/*
	** if first is not set and we were operating under SQL92 rules, we must
	** have encountered an ambiguous reference under backward-compatible
	** rules - so report the "ambiguous reference" error now
	*/
	if (!first && sql92_rules)
	{
	    (VOID) psf_error(2921L, 0L, PSF_USERERR, &err_code, err_blk, 2,
		(i4) sizeof(cb->pss_lineno), (PTR) &cb->pss_lineno,
		(i4) STlength(name), (PTR) name);
	    return (E_DB_ERROR);
	}
    }
    else
    {
	/*
	** <table reference> of form <schema>.<table> - use SQL92-compliant
	** rules, i.e. look for range vars with matching schema and table name
	** and without explicit correlation name
	*/
	for (i = 0; i <= scope; i++)
	{
	    for (list_elem = sort_list_start[i];
		 list_elem;
		 list_elem = list_elem->pst_next)
	    {
		rptr = list_elem->pst_elem;

		if (   rptr->pss_var_mask & PSS_EXPLICIT_CORR_NAME
		    || MEcmp(namepad, rptr->pss_rgname, DB_TAB_MAXNAME)
		    || MEcmp((PTR) schema_name, (PTR) &rptr->pss_ownname,
			     sizeof(cb->pss_user))
		   )
		{
		    continue;
		}

		if (first)
		{
		    /* ambiguous reference - report an error */
		    (VOID) psf_error(2921L, 0L, PSF_USERERR,
			&err_code, err_blk, 2,
			(i4) sizeof (cb->pss_lineno),
			(PTR) &cb->pss_lineno,
			(i4) STlength(name), (PTR) name);
		    return (E_DB_ERROR);
		}
		else
		{
		    first = rptr;
		}
	    }

	    /*
	    ** if first is set, the search has been successful; if scope_flag is
	    ** set, there is no reason to proceed since we are only interested
	    ** in vars at current scope
	    */
	    if (first || scope_flag)
		break;
	}
    }

    /* if we found one, move it to head of queue */
    if (first)
    {
	QUremove((QUEUE *) first);
	QUinsert((QUEUE *) first, (QUEUE *) &rngtable->pss_qhead.q_next);
    }

    /* Set result pointer */
    *result = first;

    return ((DB_STATUS)E_DB_OK);
}

/*{
** Name: pst_sdent	- Create a range table entry for an SQL derived table
**
** Description:
**      This function allocates a range table entry for a derived table
**	(subselect in the FROM clause). It then allocates and formats the
**	RDF structures usually associated with a table (actually, more like
**	a view), including the attribute list which it constructs from the
**	RESDOM list of the parse tree for the derived table.
**
** Inputs:
**	sess_cb				Pointer to session control block
**      rngtable                        Pointer to the user range table
**	scope				scope that range variable belongs in
**					-1 means don't care.
**	corrname			Correlation name of derived table
**	rngvar				Place to put pointer to new range
**					variable
**	root				Ptr to derived table parse tree
**	type				Type code for range table entry
**	err_blk				Place to put error information
**
** Outputs:
**      rngvar                          Set to point to the range variable
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**
** History:
**	24-jan-06 (dougi)
**	    Written for derived tables (subselects in the FROM clause).
**	28-dec-2006 (dougi)
**	    Add "type" parm to support both derived tables and with clause
**	    elements.
*/
DB_STATUS
pst_sdent(
	PSS_USRRANGE	   *rngtable,
	i4		    scope,
	char               *corrname,
	PSS_SESBLK	   *sess_cb,
	PSS_RNGTAB	   **rngvar,
	PST_QNODE	   *root,
	i4		    type,
	DB_ERROR	   *err_blk)
{
    PSS_RNGTAB 		*rptr;
    DMT_ATT_ENTRY	**attarray, *attrp;
    DMT_TBL_ENTRY	*tblptr;
    RDR_INFO		*rdrinfop;
    PST_QNODE		*nodep;

    i4			i, j, totlen, offset;
    DB_STATUS           status;
    i4		err_code;

    rptr = (PSS_RNGTAB *) NULL;

    rptr = (PSS_RNGTAB *) QUremove(rngtable->pss_qhead.q_prev);

    /* Free up table description if it exists */
    status = pst_rng_unfix(sess_cb, rptr, err_blk);
    if (status != E_DB_OK)
    {
	/* Put the range variable at the head of the queue */
	if (rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }

    /* Count up the RESDOMs and allocate attr array. */
    for (i = 0, totlen = 0, nodep=root->pst_left; nodep && 
	nodep->pst_sym.pst_type == PST_RESDOM; i++, nodep = nodep->pst_left)
	totlen += nodep->pst_sym.pst_dataval.db_length;

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, (sizeof(PTR) +
		sizeof(DMT_ATT_ENTRY)) * i + sizeof(PTR), &attarray, err_blk);
				/* 1 extra ptr because array is 1-origin */

    if (status != E_DB_OK)
    {
	/* Put the range variable at the head of the queue */
	if (rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }

    /* Allocate table descriptor. */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(DMT_TBL_ENTRY), &tblptr, err_blk);

    if (status != E_DB_OK)
    {
	/* Put the range variable at the head of the queue */
	if (rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }

    /* Allocate RDR_INFO block. */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(RDR_INFO) + sizeof(PTR), &rdrinfop, err_blk);

    if (status != E_DB_OK)
    {
	/* Put the range variable at the head of the queue */
	if (rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }

    /* Now format them all. */
    rptr->pss_rgtype  = type;
    rptr->pss_permit  = 1;
    rptr->pss_rgparent= scope;
    rptr->pss_qtree   = root;
    rptr->pss_tabid.db_tab_base = 0;
    rptr->pss_tabid.db_tab_index = 0;
    rptr->pss_var_mask = 0;
    STRUCT_ASSIGN_MACRO(rptr->pss_tabid, rptr->pss_syn_id);
    STmove(corrname, ' ', sizeof(rptr->pss_rgname), (char *) rptr->pss_rgname);
    STmove(corrname, ' ', sizeof(DB_TAB_NAME), (char *)&rptr->pss_tabname);
    MEfill(sizeof(DB_OWN_NAME), (u_char)' ', (char *)&rptr->pss_ownname);
    MEfill(sizeof(PST_J_MASK), (u_char) 0, (PTR) &rptr->pss_outer_rel);
    MEfill(sizeof(PST_J_MASK), (u_char) 0, (PTR) &rptr->pss_inner_rel);

    rptr->pss_colhsh = (RDD_ATTHSH *) NULL;
    rptr->pss_tabdesc = tblptr;
    rptr->pss_attdesc = attarray;
    rptr->pss_rdrinfo = rdrinfop;

    MEfill(sizeof(DMT_TBL_ENTRY), (u_char) 0, tblptr);
    STRUCT_ASSIGN_MACRO(rptr->pss_tabname, tblptr->tbl_name);
    STRUCT_ASSIGN_MACRO(rptr->pss_ownname, tblptr->tbl_owner);
    MEfill(sizeof(DB_LOC_NAME), (u_char)' ', (char *)&tblptr->tbl_location);
    STRUCT_ASSIGN_MACRO(tblptr->tbl_location, tblptr->tbl_filename);
    tblptr->tbl_attr_count = i;
    tblptr->tbl_width = totlen;
    /* All the other DMT_TBL_ENTRY fields are being left 0 until 
    ** something happens that suggests other values. */

    /* Loop over attrs, filling attr ptr array and filling in
    ** DMT_ATT_ENTRY entries. */
    attrp = (DMT_ATT_ENTRY *)&attarray[i+1];
    attarray[0] = (DMT_ATT_ENTRY *) NULL;
    for (j = 0, offset = 0, nodep = root->pst_left; j < i; 
		j++, attrp = &attrp[1], nodep = nodep->pst_left)
    {
	/* Stupid RESDOM list is inside out, so array is populated
	** back to front. */
	attarray[i-j] = attrp;
	MEfill(sizeof(DMT_ATT_ENTRY), (u_char) 0, (char *)attrp);
	MEcopy((char *)&nodep->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		sizeof(DB_ATT_NAME), (char *)&attrp->att_name);
	attrp->att_number = i-j;
	attrp->att_type = nodep->pst_sym.pst_dataval.db_datatype;
	attrp->att_width = nodep->pst_sym.pst_dataval.db_length;
	attrp->att_prec = nodep->pst_sym.pst_dataval.db_prec;
    }

    /* Now loop again to compute offsets. */
    for (j = 0, offset = 0; j < i; j++)
    {
	attarray[j+1]->att_offset = offset;
	offset += attarray[j+1]->att_width;
    }

    /* Finally fill in the RDR_INFO structure. */
    MEfill(sizeof(RDR_INFO), (u_char) 0, rdrinfop);
    rdrinfop->rdr_rel = tblptr;
    rdrinfop->rdr_attr = attarray;
    rdrinfop->rdr_no_attr = i;
    rdrinfop->rdr_view = (RDD_VIEW *)&rdrinfop[1];
				/* address word following the RDR_INFO */
    rdrinfop->rdr_view->qry_root_node = (PTR)root;

    /* Mark the range variable as used */
    rptr->pss_used = TRUE;


    /* Put the range variable at the head of the queue */
    QUinsert((QUEUE *) rptr, (QUEUE *) &rngtable->pss_qhead.q_next);

    rptr->pss_rgno = ++rngtable->pss_maxrng;
    if (rptr->pss_rgno > PST_NUMVARS - 1)
    {
	(VOID) psf_error(3100L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	*rngvar = (PSS_RNGTAB *) NULL;
	return (E_DB_ERROR);
    }

    /* Call pst_gb_search() to possibly set PST_3GROUP_BY flags. */
    pst_gb_search(root);

    /* Give the range variable back to the caller */
    *rngvar = rptr;

    return (E_DB_OK);
}

/*{
** Name: pst_swelem	- search range table for with list element of
**	supplied name
**
** Description:
**      This function searches the range table with list elements for an
**	entry with the supplied name. This function is called first to
**	resolve from list items, then it successively calls pst_sent to
**	locate table amongst temp tables, then finally persistent tables
**	and views.
**
** Inputs:
**	sess_cb				Pointer to session control block
**	scope				scope that range variable belongs in
**					-1 means don't care.
**      rngtable                        Pointer to the user range table
**	tabname				Table name of desired entry
**	rngvar				Place to put pointer to new range
**					variable
**	err_blk				Place to put error information
**
** Outputs:
**      rngvar                          Set to point to the range variable
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	None
**
** History:
**	29-dec-2006 (dougi)
**	    Written for support of with list elements.
*/
DB_STATUS
pst_swelem(
	PSS_SESBLK	   *sess_cb,
	i4		    scope,
	PSS_USRRANGE	   *rngtable,
	DB_TAB_NAME	   *tabname,
	PSS_RNGTAB	   **rngvar,
	DB_ERROR	   *err_blk)

{
    PSS_RNGTAB	*rptr;
    PST_QNODE	*duptree;
    PSS_DUPRB	dup_rb;
    DB_STATUS	status;
    i4		i, err_code;
    bool	found;


    /* Search range table for with list elements whose name matches
    ** the supplied table name. */
    for (i = 0, found = FALSE, rptr = (PSS_RNGTAB *)rngtable->pss_qhead.q_next;
	!found && i < PST_NUMVARS && rptr && 
				rptr->pss_used && rptr->pss_rgno > 0;
	i++, rptr = (PSS_RNGTAB *)rptr->pss_rngque.q_next)
    {
	if (rptr->pss_rgtype == PST_WETREE &&
		MEcmp(tabname, &rptr->pss_tabname, DB_TAB_MAXNAME) == 0)
	{
	    found = TRUE;
	    *rngvar = rptr;
	    if (scope != -1)
		rptr->pss_rgparent = scope;
	}
    }

    if (!found)
    {
	err_blk->err_code = E_PS0903_TAB_NOTFOUND;
	return(E_DB_ERROR);
    }

    if ((rptr->pss_var_mask & PSS_WE_REFED) == 0)
    {
	/* First reference - no need to make a copy yet. */
	rptr->pss_var_mask |= PSS_WE_REFED;
	return(E_DB_OK);
    }

    /* Get entry to copy element into. */
    rptr = (PSS_RNGTAB *) QUremove(rngtable->pss_qhead.q_prev);

    /* Free up table description if it exists */
    status = pst_rng_unfix(sess_cb, rptr, err_blk);
    if (status != E_DB_OK)
    {
	/* Put the range variable at the head of the queue */
	if (rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }

    STRUCT_ASSIGN_MACRO(*(*rngvar), *rptr);
    *rngvar = rptr;

    /* Put the range variable at the head of the queue */
    QUinsert((QUEUE *) rptr, (QUEUE *) &rngtable->pss_qhead.q_next);

    rptr->pss_rgno = ++rngtable->pss_maxrng;
    if (rptr->pss_rgno > PST_NUMVARS - 1)
    {
	(VOID) psf_error(3100L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	*rngvar = (PSS_RNGTAB *) NULL;
	return (E_DB_ERROR);
    }

    /* Replicate the element's parse tree. */
    dup_rb.pss_op_mask = 0;
    dup_rb.pss_tree_info = NULL;
    dup_rb.pss_mstream = &sess_cb->pss_ostream;
    dup_rb.pss_err_blk = err_blk;
    dup_rb.pss_1ptr = NULL;
    dup_rb.pss_num_joins = 0;
    dup_rb.pss_tree = rptr->pss_qtree;
    dup_rb.pss_dup = &duptree;
    status = pst_treedup(sess_cb, &dup_rb);
    if (DB_FAILURE_MACRO(status))
	return(status);

    rptr->pss_qtree = duptree;

    /* Found a matching with element - return success. */
    return(E_DB_OK);
}

/*{
** Name: pst_stproc	- Create a range table entry for a table procedure
**
** Description:
**      This function allocates a range table entry for a table procedure
**	(proc invocation in the FROM clause). It then allocates and formats 
**	faked RDF structures usually associated with a table, including the 
**	attribute list which it constructs from the result column descriptors
**	in the iiprocedure_parameters catalog.
**
** Inputs:
**	sess_cb				Pointer to session control block
**      rngtable                        Pointer to the user range table
**	scope				scope that range variable belongs in
**					-1 means don't care.
**	corrname			Correlation name of table procedure
**	dbpid				Internal ID of database procedure
**	rngvar				Place to put pointer to new range
**					variable
**	root				Ptr to RESDOM list of parameter specs
**	err_blk				Place to put error information
**
** Outputs:
**      rngvar                          Set to point to the range variable
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**
** History:
**	3-april-2008 (dougi)
**	    Written for table procedures.
**	30-march-2009 (dougi) bug 121651
**	    Trap "proc not found".
**      30-march-2009 (gefei01) bug 121870
**          Disallow DML operation in table procedure.
*/
DB_STATUS
pst_stproc(
	PSS_USRRANGE	   *rngtable,
	i4		    scope,
	char               *corrname,
	i4		    dbpid,
	PSS_SESBLK	   *sess_cb,
	PSS_RNGTAB	   **rngvar,
	PST_QNODE	   *root,
	DB_ERROR	   *err_blk)
{
    PSS_RNGTAB 		*rptr;
    DMT_ATT_ENTRY	**attarray, *attrp;
    DMT_ATT_ENTRY	**parmarray, **rescarray;
    DMT_TBL_ENTRY	*tblptr;
    DB_PROCEDURE	*dbptupp;
    RDR_INFO		*rdrinfop;
    PST_QNODE		*nodep;
    QEF_DATA		*qp;
    RDF_CB		rdf_cb;
    RDR_RB		*rdf_rb;

    i4			i, j, totlen, offset;
    DB_STATUS           status;
    i4		err_code;

    rptr = (PSS_RNGTAB *) NULL;

    rptr = (PSS_RNGTAB *) QUremove(rngtable->pss_qhead.q_prev);

    /* Free up table description if it exists */
    status = pst_rng_unfix(sess_cb, rptr, err_blk);
    if (status != E_DB_OK)
    {
	/* Put the range variable at the head of the queue */
	if (rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }

    /* Position on procedure (and return procedure descriptor for 
    ** psyview calls). */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb = &rdf_cb.rdf_rb;

    rdf_rb->rdr_tabid.db_tab_base = dbpid;
    rdf_rb->rdr_tabid.db_tab_index = 0;
    rdf_rb->rdr_types_mask  = RDR_PROCEDURE; 
    rdf_rb->rdr_2types_mask = 0;
    rdf_rb->rdr_instr       = RDF_NO_INSTR;

    /*
    ** need to set rdf_info_blk to NULL for otherwise RDF assumes that we
    ** already have the info_block
    */
    rdf_cb.rdf_info_blk = (RDR_INFO *) NULL;

    status = rdf_call(RDF_GETINFO, &rdf_cb);
    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb.rdf_error.err_code == E_RD0201_PROC_NOT_FOUND)
	    *rngvar = (PSS_RNGTAB *)NULL;
	else (VOID) psf_rdf_error(RDF_GETINFO, &rdf_cb.rdf_error, err_blk);
	return(E_DB_ERROR);
    }

    dbptupp = rdf_cb.rdf_info_blk->rdr_dbp;

    /* Disallow DML in table procedures */
    if (dbptupp->db_mask[0] & DBP_DATA_CHANGE)
    {
        i4  err_code;

        (VOID)psf_error(2449L, 0L, PSF_USERERR, &err_code, err_blk, 1,
                        sizeof(dbptupp->db_dbpname.db_dbp_name),
                        dbptupp->db_dbpname.db_dbp_name);
        return E_DB_ERROR;
    }

    /* Load iiprocedure_parameter rows for both parms and result cols. */
    i = dbptupp->db_parameterCount + dbptupp->db_rescolCount;

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, (sizeof(PTR) +
		sizeof(DMT_ATT_ENTRY)) * i + sizeof(PTR), &attarray, err_blk);
				/* 1 extra ptr because array is 1-origin */

    /* Set up attr pointer arrays for both parms and result columns. */
    for (j = 1, attrp = (DMT_ATT_ENTRY *)&attarray[i+1],
	attarray[0] = (DMT_ATT_ENTRY *)NULL; j <= i; j++, attrp = &attrp[1])
    {
	attarray[j] = attrp;
	MEfill(sizeof(DMT_ATT_ENTRY), (u_char)0, (char *)attrp);
    }

    rescarray = attarray;
    parmarray = &attarray[dbptupp->db_rescolCount+1];

    if (status != E_DB_OK)
    {
	/* Put the range variable at the head of the queue */
	if (rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }

    /* Must retrieve iiprocedure_parameters tuples - first init rdf_cb
    ** and position on iiprocedure tuple. */
    rdf_rb->rdr_types_mask     = 0;
    rdf_rb->rdr_2types_mask    = RDR2_PROCEDURE_PARAMETERS;
    rdf_rb->rdr_instr          = RDF_NO_INSTR;

    rdf_rb->rdr_update_op      = RDR_OPEN;
    rdf_rb->rdr_qrymod_id      = 0;	/* get all tuples */
    rdf_rb->rdr_qtuple_count   = 20;	/* get 20 at a time */
    rdf_cb.rdf_error.err_code = 0;

    /*
    ** must set rdr_rec_access_id since otherwise RDF will barf when we
    ** try to RDR_OPEN
    */
    rdf_rb->rdr_rec_access_id  = NULL;

    while (rdf_cb.rdf_error.err_code == 0)
    {
	status = rdf_call(RDF_READTUPLES, &rdf_cb);
	rdf_rb->rdr_update_op = RDR_GETNEXT;

	/* Must not use DB_FAILURE_MACRO because E_RD0011 returns
	** E_DB_WARN that would be missed.
	*/
	if (status != E_DB_OK)
	{
	    switch(rdf_cb.rdf_error.err_code)
	    {
		case E_RD0011_NO_MORE_ROWS:
		    status = E_DB_OK;
		    break;

		case E_RD0013_NO_TUPLE_FOUND:
		    status = E_DB_OK;
		    continue;

		default:
		    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error,
                                err_blk);
		    return(E_DB_ERROR);
	    }	    /* switch */
	}	/* if status != E_DB_OK */

	/* For each dbproc parameter tuple */
	for (qp = rdf_cb.rdf_info_blk->rdr_pptuples->qrym_data, j = 0;
	    j < rdf_cb.rdf_info_blk->rdr_pptuples->qrym_cnt;
	    qp = qp->dt_next, j++)
	{
	    DB_PROCEDURE_PARAMETER *param_tup =
		(DB_PROCEDURE_PARAMETER *) qp->dt_data;
	    if (i-- == 0)
	    {
		(VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error,
                                err_blk);
		return(E_DB_ERROR);
	    }
	    if (param_tup->dbpp_flags & DBPP_RESULT_COL)
	    {
		attrp = rescarray[param_tup->dbpp_number];
		attrp->att_number = param_tup->dbpp_number;
	    }
	    else 
	    {
		attrp = parmarray[param_tup->dbpp_number-1];
		attrp->att_flags = DMT_F_TPROCPARM;
		attrp->att_number = param_tup->dbpp_number +
					dbptupp->db_rescolCount;
	    }

	    STRUCT_ASSIGN_MACRO(param_tup->dbpp_name, attrp->att_name);
	    attrp->att_type = param_tup->dbpp_datatype;
	    attrp->att_width = param_tup->dbpp_length;
	    attrp->att_prec = param_tup->dbpp_precision;
	    attrp->att_offset = param_tup->dbpp_offset;
	}
    }

    if (rdf_rb->rdr_rec_access_id != NULL)
    {
	rdf_rb->rdr_update_op = RDR_CLOSE;
	status = rdf_call(RDF_READTUPLES, &rdf_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, err_blk);
	    return(E_DB_ERROR);
	}
    }

    /* now unfix the dbproc description */
    rdf_rb->rdr_types_mask  = RDR_PROCEDURE | RDR_BY_NAME;
    rdf_rb->rdr_2types_mask = 0;
    rdf_rb->rdr_instr       = RDF_NO_INSTR;

    status = rdf_call(RDF_UNFIX, &rdf_cb);
    if (DB_FAILURE_MACRO(status))
    {
        (VOID) psf_rdf_error(RDF_UNFIX, &rdf_cb.rdf_error, err_blk);
        return(E_DB_ERROR);
    }

    /* Allocate table descriptor. */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(DMT_TBL_ENTRY), &tblptr, err_blk);

    if (status != E_DB_OK)
    {
	/* Put the range variable at the head of the queue */
	if (rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }

    /* Allocate RDR_INFO block. */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(RDR_INFO) + sizeof(PTR), &rdrinfop, err_blk);

    if (status != E_DB_OK)
    {
	/* Put the range variable at the head of the queue */
	if (rptr != &rngtable->pss_rsrng)
	    QUinsert((QUEUE *) rptr, rngtable->pss_qhead.q_prev);

	*rngvar = (PSS_RNGTAB *) NULL;
	return (status);
    }

    /* Now format them all. */
    rptr->pss_rgtype  = PST_TPROC;
    rptr->pss_permit  = 1;
    rptr->pss_rgparent= scope;
    rptr->pss_qtree   = root;
    rptr->pss_var_mask = 0;
    STRUCT_ASSIGN_MACRO(dbptupp->db_procid, rptr->pss_tabid);
    STRUCT_ASSIGN_MACRO(rptr->pss_tabid, rptr->pss_syn_id);
    if (corrname)
	STmove(corrname, ' ', sizeof(rptr->pss_rgname), 
					(char *) rptr->pss_rgname);
    else STmove ((char *)&dbptupp->db_dbpname, ' ', 
			sizeof(rptr->pss_rgname), (char *) rptr->pss_rgname);
    STmove(rptr->pss_rgname, ' ', sizeof(rptr->pss_rgname), 
					(char *)&rptr->pss_tabname);
    MEfill(sizeof(DB_OWN_NAME), (u_char)' ', (char *)&rptr->pss_ownname);
    MEfill(sizeof(PST_J_MASK), (u_char) 0, (PTR) &rptr->pss_outer_rel);
    MEfill(sizeof(PST_J_MASK), (u_char) 0, (PTR) &rptr->pss_inner_rel);

    rptr->pss_colhsh = (RDD_ATTHSH *) NULL;
    rptr->pss_tabdesc = tblptr;
    rptr->pss_attdesc = rescarray;
    rptr->pss_rdrinfo = rdrinfop;

    MEfill(sizeof(DMT_TBL_ENTRY), (u_char) 0, tblptr);
    STRUCT_ASSIGN_MACRO(rptr->pss_tabname, tblptr->tbl_name);
    STRUCT_ASSIGN_MACRO(rptr->pss_ownname, tblptr->tbl_owner);
    MEfill(sizeof(DB_LOC_NAME), (u_char)' ', (char *)&tblptr->tbl_location);
    STRUCT_ASSIGN_MACRO(tblptr->tbl_location, tblptr->tbl_filename);
    tblptr->tbl_attr_count = dbptupp->db_rescolCount +
					dbptupp->db_parameterCount;
    tblptr->tbl_width = dbptupp->db_resrowWidth;
    tblptr->tbl_date_modified.db_tab_high_time = dbptupp->db_created;
    /* All the other DMT_TBL_ENTRY fields are being left 0 until 
    ** something happens that suggests other values. */

    /* Finally fill in the RDR_INFO structure. */
    MEfill(sizeof(RDR_INFO), (u_char) 0, rdrinfop);
    rdrinfop->rdr_rel = tblptr;
    rdrinfop->rdr_attr = rescarray;
    rdrinfop->rdr_no_attr = tblptr->tbl_attr_count;
    rdrinfop->rdr_view = (RDD_VIEW *)&rdrinfop[1];
				/* address word following the RDR_INFO */
    rdrinfop->rdr_view->qry_root_node = (PTR)root;

    /* Mark the range variable as used */
    rptr->pss_used = TRUE;


    /* Put the range variable at the head of the queue */
    QUinsert((QUEUE *) rptr, (QUEUE *) &rngtable->pss_qhead.q_next);

    rptr->pss_rgno = ++rngtable->pss_maxrng;
    if (rptr->pss_rgno > PST_NUMVARS - 1)
    {
	(VOID) psf_error(3100L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	*rngvar = (PSS_RNGTAB *) NULL;
	return (E_DB_ERROR);
    }

    /* Give the range variable back to the caller */
    *rngvar = rptr;

    return (E_DB_OK);
}

/*{
** Name: pst_gb_search - search parse tree for GROUP BY
**
** Description:
**      This function recursively analyzes a parse tree built for a 
**	derived table, looking for an AGHEAD with a contained BYHEAD
**	signifying a GROUP BY in the derived table. A flag is then set
**	in the root node of the containing tree (either a PST_ROOT or
**	PST_SUBSEL) to guide OPF appropriately. This logic comes from 
**	pst_treedup() which does essentially the same thing when copying
**	parse trees for views referenced inside queries. That function 
**	seems to set a few other fields that may be needed for derived 
**	tables, and when they're found, they might also be implemented here.
**
** Inputs:
**	root				Ptr to derived table parse tree
**
** Outputs:
**      root				Ptr to possibly updated parse tree
**
** History:
**	30-jan-06 (dougi)
**	    Written for derived tables (subselects in the FROM clause).
*/
static bool
pst_gb_search(
	PST_QNODE	*root)
{
    bool	leftbool = FALSE;
    bool	rightbool = FALSE;
    bool	retbool = TRUE;

    /* Recurse on left and right - if they come back TRUE and this is
    ** ROOT or SUBSEL, set pst_mask1 accordingly. */

    if (root->pst_left)
	leftbool = pst_gb_search(root->pst_left);
    if (root->pst_right)
	rightbool = pst_gb_search(root->pst_right);

    switch (root->pst_sym.pst_type) {

      case PST_SUBSEL:
	retbool = FALSE;
      case PST_ROOT:
	if (leftbool || rightbool)
	    root->pst_sym.pst_value.pst_s_root.pst_mask1 |= PST_3GROUP_BY;
	else root->pst_sym.pst_value.pst_s_root.pst_mask1 &= ~PST_3GROUP_BY;
	break;

      case PST_AGHEAD:
	if (root->pst_left->pst_sym.pst_type != PST_BYHEAD)
	    retbool = (leftbool || rightbool);
	break;

      default:
	retbool = (leftbool || rightbool);
    }

    return(retbool);
}

/*{
** Name: pst_rgfind - Find a range variable with a given variable number.
**
** Description:
**      This function looks up a range variable by variable number.
**	If it finds it, a pointer to the entry in the range table is returned.
**
** Inputs:
**      rngtable                        Pointer to the range table
**	result				Pointer to place to put pointer to range
**					variable.
**	err_blk				Place to put error information.
**
** Outputs:
**	result				Filled in with pointer to range variable
**					or NULL if not found.
**	err_blk				Filled in with error information if an
**					error occurs.
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	09-apr-87 (stec)
**          Written.
*/
DB_STATUS
pst_rgfind(
	PSS_USRRANGE       *rngtable,
	PSS_RNGTAB	   **result,
	i4		   vno,
	DB_ERROR	   *err_blk)
{
    register PSS_RNGTAB *rptr = (PSS_RNGTAB *) rngtable->pss_qhead.q_next;
    DB_STATUS		status = E_DB_OK;

    /* initialize error structure */
    err_blk->err_code = E_DB_OK;
    err_blk->err_data = 0;

    /* Look for the range variable */
    while ((QUEUE *) rptr != &rngtable->pss_qhead)
    {
	if (rptr->pss_used && (vno == rptr->pss_rgno))
	{
	    break;  /* found it */
	}
	rptr = (PSS_RNGTAB *) rptr->pss_rngque.q_next;
    }

    /* Return to beginning of QUEUE means not found */
    if ((QUEUE *) rptr == &rngtable->pss_qhead)
    {
	rptr = (PSS_RNGTAB *) NULL;
	status = E_DB_ERROR;
	err_blk->err_code = E_DB_ERROR;
    }

    /* Set result pointer */
    *result = rptr;

    return (status);
}

/*
** Name: pst_rng_unfix -- Unfix a range table entry
**
** Description:
**	This routine unfixes a range table entry from RDF.
**	There are a few special cases where the range table entry has
**	no valid RDF entry, even though it looks like it:  namely,
**	derived-table entries (subquery in FROM clauses);  and set-
**	input DBP parameter dummies, unless we're re-parsing the
**	set input DBP for QEF in which case there really is RDF
**	stuff there.  I guess.
**
**	Unfixing happens often enough that testing the special cases
**	and getting it right is a PITA, so it's been extracted into
**	this routine.  If an RDF unfix error occurs, we issue the
**	message (with psf-rdf-error) and return the error status.
**
**	If the unfixing works, pss_rdrinfo is cleared, but nothing
**	else in the range table entry is affected.
**
** Inputs:
**	sess_cb		PSS_SESBLK * pointer to parser session CB
**	rng_ptr		Pointer to range table entry to possibly unfix
**	errblk		Pointer to caller's error info block
**
** Outputs:
**	None (unless an error occurs)
**	Returns E_DB_xxx ok or error status
**
** History:
**	23-Jun-2006 (kschendel)
**	    Extract from various places that do this.
**    20-Nov-08 (wanfr01)
**          Bug 121252
**          Grant statements also fake the set-input DBP.  Do not unfix it.
*/

DB_STATUS
pst_rng_unfix(PSS_SESBLK *sess_cb, PSS_RNGTAB *rng_ptr,
	DB_ERROR *errblk)

{

    DB_STATUS status;
    RDF_CB unfix_cb;			/* Our unfixing request block */


    /* Unfix if RDF info there, and not something faked up:
    ** derived table entry, set-input DBP formal during DBP creation.
    */
    if (rng_ptr->pss_rdrinfo != (RDR_INFO *) NULL
      && rng_ptr->pss_rgtype != PST_DRTREE
      && rng_ptr->pss_rgtype != PST_WETREE
      && rng_ptr->pss_rgtype != PST_TPROC
      && (rng_ptr->pss_rgtype != PST_SETINPUT
          ||((sess_cb->pss_dbp_flags & PSS_RECREATE) &&
             !(sess_cb->pss_dbp_flags & PSS_PARSING_GRANT))))
    {
	pst_rdfcb_init(&unfix_cb, sess_cb);
	unfix_cb.rdf_info_blk = rng_ptr->pss_rdrinfo;
	status = rdf_call(RDF_UNFIX, &unfix_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    (void) psf_rdf_error(RDF_UNFIX, &unfix_cb.rdf_error, errblk);
	    return (status);
	}
    }
    rng_ptr->pss_rdrinfo = NULL;
    return (E_DB_OK);
} /* pst_rng_unfix */
