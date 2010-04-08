/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <dmf.h>
#include    <psfparse.h>
#include    <psttprpnd.h>

/**
**
**  Name: PSTTPRPND.C - Functions for prepending target lists.
**
**  Description:
**      This file contains the functions necessary to prepend one target
**	list onto another.
**
**          pst_tlprpnd - Prepend one target list onto another.
**
**
**  History:    $Log-for RCS$
**      28-mar-86 (jeff)    
**          Adapted from tl_prepend in version 3.0
**	19-jan-88 (stec)
**	    Include dmf.h.
**	08-feb-88 (stec)
**	    Improve pst_tlprpnd.
**	14-jan-92 (barbara)
**	    Included ddb.h, qefrcb.h for Star.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
[@history_template@]...
**/

/*{
** Name: pst_tlprpnd	- Prepend one target list onto another.
**
** Description:
**      This function attaches two target lists to each other.  Neither
**	component need be a single element.  The "from" component will be
**	attached at the extreme left of the "onto" component.
**
** Inputs:
**      from                            Target list to be attached
**      onto                            Target list to attach to
**
** Outputs:
**      NONE
**	Returns:
**	    Pointer to onto.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-mar-86 (jeff)
**          Adapted from tl_prepend in version 3.0
**	08-feb-88 (stec)
**	    Improve pst_tlprpnd so that PST_TREE nodes do not get lost.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/
PST_QNODE *
pst_tlprpnd(
	PST_QNODE          *from,
	PST_QNODE          *onto)
{
    register PST_QNODE  *q;
    PST_QNODE		*t;

    /* Scan to left end of onto */
    for (q = onto;
	((q->pst_left != (PST_QNODE *) NULL) && 
	    (q->pst_left->pst_sym.pst_type != PST_TREE));
	q = q->pst_left)
	    ; /* NO ACTION */

    /* Save pointer being replaced */
    t = q->pst_left;

    /* Attach "from" to the left end of "onto" */
    q->pst_left = from;

    /* If there was a tree node, and there is not one
    ** in the tree, attach it.
    */
    if (t != (PST_QNODE *) NULL)
    {
	/* Continue scanning to the left end. */
	for (; ((q->pst_left != (PST_QNODE *) NULL) && 
		(q->pst_sym.pst_type != PST_TREE));
	    q = q->pst_left)
		; /* NO ACTION */
	if (q->pst_sym.pst_type != PST_TREE)
	{
	    q->pst_left = t;
	}
    }

    return (onto);
}
