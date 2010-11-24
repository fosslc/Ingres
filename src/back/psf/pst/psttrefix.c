/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
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
**  Name: PSTTREFIX.C - Functions for fixing a query tree after it's built.
**
**  Description:
**      This file contains the functions for fixing a query tree after it's
**	built.
**
**          pst_trfix - Fix a query tree
**
**
**  History:
**      02-apr-86 (jeff)    
**          written
**	17-apr-89 (jrb)
**	    Interface change for pst_node and prec cleanup for decimal project.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	04-aug-93 (shailaja)
**	    Fixed qac warnings.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 pst_trfix(
	PSS_SESBLK *sess_cb,
	PSF_MSTREAM *mstream,
	PST_QNODE *qtree,
	DB_ERROR *err_blk);

/*{
** Name: pst_trfix	- Fix a query tree
**
** Description:
**	After the grammar builds a query tree, it must have a PST_TREE node
**	put on the left side of the tree.  This function does that.
**
** Inputs:
**	sess_cb
**	mstream				Memory stream used to allocate the tree
**	qtree				Pointer to the root of the query tree
**					to fix.
**	err_blk				Pointer to error block.
**
** Outputs:
**      qtree                           PST_TREE node tacked on
**	err_blk				Filled in if an error happens.
**	Returns:
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
**	02-apr-86 (jeff)
**          written
*/
DB_STATUS
pst_trfix(
	PSS_SESBLK	   *sess_cb,
	PSF_MSTREAM	   *mstream,
	PST_QNODE          *qtree,
	DB_ERROR	   *err_blk)
{
    DB_STATUS           status;
    register PST_QNODE  *curnode;

    /*
    ** Assume that qtree actually points to something here.  I can't think
    ** of a reason it wouldn't.
    */

    /*
    ** Find the leftmost node in the tree.
    */
    for (curnode = qtree; curnode->pst_left != (PST_QNODE *) NULL;
	curnode = curnode->pst_left);
	;   /* NULL STATEMENT */

    /*
    ** Allocate the PST_TREE node.
    */
    if (curnode->pst_sym.pst_type != PST_TREE)
    {
	status = pst_node(sess_cb, mstream, (PST_QNODE *) NULL,
	    (PST_QNODE *) NULL, PST_TREE, (PTR) NULL, 0, DB_NODT, (i2) 0,
	    (i4) 0, (DB_ANYTYPE *) NULL, &curnode->pst_left, err_blk, (i4) 0);
	if (status != E_DB_OK)
	{
	    return (status);
	}
    }
    /*
    ** Map the range variable info into the root nodes.
    **
    ** We decided to do this in the optimizer, but I left this here just
    ** in case we reversed ourselves.
    ** NOTE: if this is done, check usage of this routine in pslsgram.
    ** qtree may not be pointing to a root node.
    */
/*    status = pst_rootmap(qtree, err_blk);
**    if (status != E_DB_OK)
**    {
**	return (status);
**    }
*/

    return (E_DB_OK);
}
