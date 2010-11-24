/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <bt.h>
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
**  Name: PSYMAPVAR.C - Functions for re-mapping variable numbers in trees
**
**  Description:
**      This file contains the functions for re-mapping the range variable
**	numbers in query trees.
**
**          psy_mapvars - Re-map a variable number in a query tree
**
**
**  History:    $Log-for RCS$
**      20-jun-86 (jeff)    
**          written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	08-dec-92 (anitap)
**	    Changed the order of #includes of header files for CREATE SCHEMA.
**	15-dec-92 (wolf) 
**	    ulf.h must be included before qsf.h
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psy_mapvars(
	PST_QNODE *tree,
	i4 map[],
	DB_ERROR *err_blk);

/*{
** Name: psy_mapvars	- Re-map the variable numbers in a query tree
**
** Description:
**	A tree is scanned for VAR nodes; when found, the
**	mapping defined by "map" is applied.  This is done so that
**	varno's as defined in trees in the 'tree' catalog will be
**	unique with respect to varno's in the user's query tree.  For
**	example, if the view definition uses variable 1 and the user's
**	query also uses variable 1, a new range variable must be assigned
**	to the view using pst_rgent(), and this routine must be called to
**	re-assign the range variable number.
**
** Inputs:
**      tree                            Root of tree to do mapping in
**	map				Array of range variable numbers;
**					the number in position n tells the
**					new number for range variable n.
**	err_blk				Filled in if an error happens
**
** Outputs:
**      tree                            All VAR nodes re-mapped
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
**	20-jun-86 (jeff)
**          Adapted from mapvars.c in 4.0
**	21-mar-87 (daved)
**	    change the from list
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	27-nov-02 (inkdo01)
**	    Range table expansion.
*/

DB_STATUS
psy_mapvars(
	PST_QNODE   *tree,
	i4	    map[],
	DB_ERROR    *err_blk)
{
    register PST_QNODE	*t;
    DB_STATUS		status;

    t = tree;

    while (t != (PST_QNODE *) NULL)
    {
	/* map right subtree */

	status = psy_mapvars(t->pst_right, map, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* check this node */

	if (t->pst_sym.pst_type == PST_VAR)
	{
	    t->pst_sym.pst_value.pst_s_var.pst_vno =
		map[t->pst_sym.pst_value.pst_s_var.pst_vno];
	}

	/* map unions */
	if (t->pst_sym.pst_type == PST_ROOT)
	{
	    status = 
		psy_mapvars(t->pst_sym.pst_value.pst_s_root.pst_union.pst_next, 
		map, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}	    
	/* map the from list */
	if (t->pst_sym.pst_type == PST_ROOT || 
	    t->pst_sym.pst_type == PST_SUBSEL ||
	    t->pst_sym.pst_type == PST_AGHEAD)
	{
	    PST_J_MASK	oldlist;
	    i4		i;

	    MEcopy((char *)&t->pst_sym.pst_value.pst_s_root.pst_tvrm,
			sizeof(PST_J_MASK), (char *)&oldlist);
	    MEfill(sizeof(PST_J_MASK), 0,
			(char *)&t->pst_sym.pst_value.pst_s_root.pst_tvrm);
	    for (i = -1; 
		(i = BTnext(i, (char*) &oldlist, BITS_IN(oldlist))) > -1;)
		BTset(map[i], (char*)&t->pst_sym.pst_value.pst_s_root.pst_tvrm);
	    
	}
	/* map left subtree (iteratively) */

	t = t->pst_left;
    }

    return (E_DB_OK);
}
