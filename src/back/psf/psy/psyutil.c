/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <st.h>
#include    <me.h>
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

/**
**
**  Name: PSYUTIL.C - QRYMOD utilities
**
**  Description:
**      This file contains several QRYMOD utilities
**
**          psy_trmqlnd - Trim the QLEND node off a tree
**          psy_apql - Append a qualification to a tree
**          psy_mrgvar - Merge variable numbers to link protections and
**			integrities
**          psy_error - Report a QRYMOD error
**
**
**  History:
**      19-jun-86 (jeff)    
**          written
**	03-dec-87 (stec)
**	    Change psy_apql interface.
**	29-jan-88 (stec)
**	    Change psy_apql.
**	07-jun-88 (stec)
**	    Make changes for DB procs.
**	17-apr-89 (jrb)
**	    Interface change for pst_node and prec cleanup for decimal project.
**	08-dec-92 (anitap)
**	    Changed the order of #includes of header files for CREATE SCHEMA.
**	15-dec-92 (wolf) 
**	    ulf.h must be included before qsf.h
**	25-jan-93 (rblumer) 
**	    remove psy_mkempty function;
**	    it has been replaced by psl_make_default_node.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	    Changed psf_sesscb() prototype to return PSS_SESBLK* instead
**	    of DB_STATUS.
[@history_template@]...
**/

/*{
** Name: psy_trmqlnd	- Trim QLEND node off of qualification
**
** Description:
**	The QLEND node, and possible the AND node preceeding it,
**	are trimmed off.  The result of this routine should be
**	a very ordinary tree like you might see in some textbook.
**
**	A fast note on the algorithm: the pointer 't' points to the
**	current node (the one which we are checking for a QLEND).
**	's' points to 't's parent, and 'r' points to 's's parent;
**	'r' is NULL at the top of the tree.
**
**	This routine works correctly on trees with no QLEND in
**	the first place, returning the original tree.
**
**	If there is a QLEND, it must be on the far right branch
**	of the tree, that is, the tree must be INGRES-canonical.
**
** Inputs:
**      qual                            Pointer to root of qualification
**
** Outputs:
**      qual                            QLEND node is trimmed off
**	Returns:
**	    Pointer to trimmed tree
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-jun-86 (jeff)
**          written
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
*/

PST_QNODE *
psy_trmqlnd(
	PST_QNODE *qual)
{
    register PST_QNODE	*t;
    register PST_QNODE	*s;
    register PST_QNODE	*r;

    t = qual;

    /* check for the simple null qualification case */
    if (t == (PST_QNODE *) NULL || t->pst_sym.pst_type == PST_QLEND)
	return (NULL);
	
    /* scan tree for QLEND node */
    for (r = (PST_QNODE *) NULL, s = t;
	(t = t->pst_right) != (PST_QNODE *) NULL; r = s, s = t)
    {
	if (t->pst_sym.pst_type == PST_QLEND)
	{
	    /* trim of QLEND and AND node */
	    if (r == (PST_QNODE *) NULL)
	    {
		/* only one AND -- return its operand */
		return (s->pst_left);
	    }

	    /*
	    ** If we are dealing with a permit created in SQL, S is not
	    ** necessarily a PST_AND, e.g. for a permit created as
	    ** "create permit select on t1 to U where i in (select * from t2)"
	    ** S would be pointing to a PST_SUBSEL, and getting rid of it would
	    ** produce problems.
	    */
	    if (s->pst_sym.pst_type == PST_AND)
	    {
		r->pst_right = s->pst_left;
	    }
	    else
	    {
		/* Just get rid of PST_QLEND node */
		s->pst_right = (PST_QNODE *) NULL;
	    }
	    break;
	}
    }

    /* return tree with final AND node and QLEND node pruned */
    return (qual);
}

/*{
** Name: psy_apql	- Append a qualification to a query tree
**
** Description:
**      The qualification is conjoined to the qualification of the tree
**	which is passed in.
**
** Inputs:
**	cb				session control block
**	mstream				QSF memory stream to allocate from
**      qual                            Root of qual to be appended
**	root				Root of tree to append to
**	err_blk				Filled in if an error happens
**
** Outputs:
**      root                            Qualification append to tree
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	Both 'qual' ad 'root' may be modified.  Note that
**	    'qual' is linked into 'root', and must be
**	    retained.
**	Memory may be allocated.
**
** History:
**	19-jun-86 (jeff)
**          written
**	03-dec-87 (stec)
**	    Change psy_apql interface and algorithm.
**	29-jan-88 (stec)
**	    Check for null quals and QLEND nodes.
**	22-sep-90 (teresa)
**	    if there is a non-QLEND qual in either qual or root but the other 
**	    has a QLEND, then just add the qualification to the root without
**	    creating a new AND node.
**	25-Mar-2010 (kiria01) b123535
**	    Be a little more forgiving if the root->pst_right has not been
**	    assigned to and is about to be.
*/

DB_STATUS
psy_apql(
	PSS_SESBLK  *cb,
	PSF_MSTREAM *mstream,
	PST_QNODE   *qual,
	PST_QNODE   *root,
	DB_ERROR    *err_blk)
{
    DB_STATUS	status;
    PST_QNODE	*newnode;

    /*
    ** The new algorithm assumes that trees are not normalized,
    ** therefore an AND node will be added at the top and both
    ** qualification trees will be ANDed.
    ** Root node will be made to point to the new AND node.
    ** No action is taken when qualification pointer is NULL
    ** or points to a QLEND node.
    */

    if ((qual == (PST_QNODE *) NULL) ||
	(qual->pst_sym.pst_type == PST_QLEND)
       )
    {
	return (E_DB_OK);
    }
    else if ( (qual != (PST_QNODE *)NULL) &&
		(!root->pst_right ||
		root->pst_right->pst_sym.pst_type == PST_QLEND) )
    {
	/* There is a qualification from the view that we need to attach. 
	** However, the root's pst_right just has a QLEND.  In that case, 
	** just assign the qual to the root's right child, replacing the QLEND 
	** with that qual.  DO NOT MAKE A NEW PST_AND NODE TO HANG IF OFF OF.
	** (teresa)
	*/

	root->pst_right = qual;
	return (E_DB_OK);
    }

    /* here we have a qualification in the tree and also have a
    ** qualification from the view that we need to attach.  So, make
    ** a PST_AND node as the root's right child, and then hang both 
    ** qualificaitons off of that and node.
    */
    status = pst_node(cb, mstream, qual, root->pst_right, PST_AND,
	(PTR) NULL, sizeof(PST_OP_NODE), DB_NODT, (i2) 0, (i4) 0,
	(DB_ANYTYPE *) NULL, &newnode, err_blk, (i4) 0);

    root->pst_right = newnode;
    return (status);
}

/*{
** Name: psy_mrgvar	- Merge variable numbers to link terms
**
** Description:
**	One specified variable gets mapped into another, effectively
**	merging those two variables.  This is used for protection
**	and integrity, since the constraint read from the tree
**	must coincide with one of the variables in the query tree.
**
** Inputs:
**      rngtab                          Pointer to range table
**      a                               Number of the variable that will
**					disappear.
**	b				The variable which 'a' gets mapped into
**	root				The root of the tree to map
**	err_blk				Filled in if an error happens
**
** Outputs:
**      rngtab                          Range variable a gets mapped out
**	root				All 'a' VAR nodes get changed to b
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
**	19-jun-86 (jeff)
**          written
*/

DB_STATUS
psy_mrgvar(
	PSS_USRRANGE	*rngtab,
	register i4	a,
	register i4	b,
	PST_QNODE	*root,
	DB_ERROR	*err_blk)
{
    i4		err_code;
    DB_STATUS		status;
    i4			map[PST_NUMVARS];   /* Map for reassigning varnos */
    register i4	i;

    /*
    **  Insure that 'a' and 'b' are consistent, that is,
    **  that they both are in range, are defined, and range over
    **  the same relation.
    */

    if (a < 0 || b < 0 || a > PST_NUMVARS || b > PST_NUMVARS)
    {
	(VOID) psf_error(E_PS0D07_VARNO_OUT_OF_RANGE, 0L, PSF_INTERR, &err_code,
	    err_blk, 0);
	return (E_DB_SEVERE);
    }

    /*
    ** The range variables had better both exist.
    */
    if (!rngtab->pss_rngtab[a].pss_used || !rngtab->pss_rngtab[b].pss_used)
    {
	(VOID) psf_error(E_PS0D08_RNGVAR_UNUSED, 0L, PSF_INTERR, &err_code,
	    err_blk, 0);
	return (E_DB_SEVERE);
    }

    /*
    ** The range variables had better both point to the same table.
    */
    if (MEcmp((PTR) &rngtab->pss_rngtab[a].pss_tabid,
	(PTR) &rngtab->pss_rngtab[b].pss_tabid, sizeof(DB_TAB_ID)))
    {
	(VOID) psf_error(E_PS0D09_MRG_DIFF_TABS, 0L, PSF_INTERR, &err_code,
	    err_blk, 0);
	return (E_DB_SEVERE);
    }
	
    /*
    ** Do the actual mapping by reassigning range variable numbers.  Set up
    ** the map to map everything to itself, then change the one slot in the
    ** map that we care about to do the actual mapping.
    */

    for (i = 0; i < PST_NUMVARS; i++)
	map[i] = i;
    map[a] = b;
    status = psy_mapvars(root, map, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* delete a from the range table */
    rngtab->pss_rngtab[a].pss_used = FALSE;

    /* Should probably do something here to free the description */

    return (E_DB_OK);
}

/*{
** Name: PSY_ERROR	- Report a qrymod user error
**
** Description:
**      This function reports a qrymod user error.  It is used almost like
**	psf_error(), except that it takes a query mode, a variable number,
**	and a range table.  It inserts the table name and operation name into
**	the text of the error message.
**
** Inputs:
**      errorno                         The user error number
**	qmode				The query mode.  Negative means none
**					or don't care.
**	rngvar				The range variable entry
**	err_blk				Pointer to standard error block
**	p1				First parameter
**	p2				Second parameter
**	p3				Third parameter
**	p4				Fourth parameter
**	p5				Fifth parameter
**	p6				Sixth parameter
**
** Outputs:
**      err_blk                         Filled in with the error number
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-may-86 (jeff)
**          written
**	07-aug-91 (andre)
**	    qmode may be set to PSQ_VIEW
**	6-apr-1992 (bryanp)
**	    Added support for PSQ_DGTT_AS_SELECT
**	28-nov-2007 (dougi)
**	    Added PSQ_REPDYN to PSQ_DEFCURS for cached dynamic qs.
*/
/*VARARGS4*/
DB_STATUS
psy_error(errorno, qmode, rngvar, err_blk, p1, p2, p3, p4, p5, p6)
i4            errorno;
i4		   qmode;
PSS_RNGTAB	   *rngvar;
DB_ERROR	   *err_blk;
i4		   p1;
PTR		   p2;
i4		   p3;
PTR		   p4;
i4		   p5;
PTR		   p6;
/*
psy_error(
	i4            errorno,
	i4		   qmode,
	PSS_RNGTAB	   *rngvar,
	DB_ERROR	   *err_blk,
	i4		   p1,
	PTR		   p2,
	i4		   p3,
	PTR		   p4,
	i4		   p5,
	PTR		   p6)
*/
{
    char                tabname[DB_MAXNAME + 1];
    char		ownname[DB_MAXNAME + 1];
    char		*modestr;
    i4			err_code;
    DB_STATUS		status;
    PSS_SESBLK		*sess_cb;

    sess_cb = psf_sesscb();

    switch (qmode)
    {
	case PSQ_RETRIEVE:
	case PSQ_RETINTO:
	case PSQ_DEFCURS:
	case PSQ_REPDYN:
	case PSQ_DGTT_AS_SELECT:
	{
	    modestr = (sess_cb->pss_lang == DB_SQL) ? "SELECT" : "RETRIEVE";
	    break;
	}
	case PSQ_APPEND:
	{
	    modestr = (sess_cb->pss_lang == DB_SQL) ? "INSERT" : "APPEND";
	    break;
	}
	case PSQ_REPLACE:
	{
	    modestr = (sess_cb->pss_lang == DB_SQL) ? "UPDATE" : "REPLACE";
	    break;
	}
	case PSQ_DELETE:
	{
	    modestr = "DELETE";
	    break;
	}
	case PSQ_VIEW:
	{
	    modestr = (sess_cb->pss_lang == DB_SQL) ? "CREATE VIEW"
						    : "DEFINE VIEW";
	    break;
	}
	default:
	{
	    modestr = "UNKNOWN";
	    break;
	}
    }

    /* Get the table name. */
    STncpy(tabname, rngvar->pss_tabname.db_tab_name, DB_MAXNAME);
    tabname[ DB_MAXNAME ] = '\0';
    STncpy(ownname, rngvar->pss_ownname.db_own_name, DB_MAXNAME);
    ownname[ DB_MAXNAME ] = '\0';

    if (qmode >= 0)
    {
	(VOID) psf_error(errorno, 0L, PSF_USERERR, &err_code, err_blk, 5,
	    (i4) STtrmwhite(modestr), modestr,
	    (i4) STtrmwhite(tabname), tabname,
	    (i4) STtrmwhite(ownname), ownname,
	    p1, p2, p3, p4);
    }
    else
    {
	(VOID) psf_error(errorno, 0L, PSF_USERERR, &err_code, err_blk, 4,
	    (i4) STtrmwhite(tabname), tabname,  
	    (i4) STtrmwhite(ownname), ownname,
	    p1, p2, p3, p4);
    }

    return (E_DB_OK);
}
