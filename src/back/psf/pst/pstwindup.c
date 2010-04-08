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
#include    <pstwindup.h>

/**
**
**  Name: PSTWINDUP.C - Functions for assigning result domain numbers to the
**			result domains of an aggregate function.
**
**  Description:
**      This file contains the functions necessary to assign the result domain
**	numbers to the result domains under an aggregate function.
**
**          pst_windup - Assign result domain numbers to an aggregate function
**
**
**  History:    $Log-for RCS$
**      01-may-86 (jeff)    
**          Adapted from windup() in old tree.c
**	19-jan-88 (stec)
**	    Include dmf.h.
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
** Name: pst_windup	- Assign result domain numbers to an aggregate function
**
** Description:
**      This function assigns result domain numbers to an aggregate function.
**	It takes two passes.  The first counts the result domains, and the
**	second assigns the numbers.  The numbers are simply assigned
**	sequentially from 1, with the highest number at the top.
**
**	This function assumes that the result domain list is well-formed.
**
** Inputs:
**      reslist                         The result domain list
**
** Outputs:
**      reslist                         Result domain numbers assigned
**	Returns:
**	    NONE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-may-86 (jeff)
**          adapted from windup() in old tree.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/
VOID
pst_windup(
	PST_QNODE          *reslist)
{
    register i4         tot;
    register i4	kk;
    register PST_QNODE	*t;

    /* Count the result domains of this target list */
    kk = 1;
    for (t = reslist; t != (PST_QNODE *)  NULL; t = t->pst_left)
	kk++;

    tot = 1;
    for (t = reslist; t != (PST_QNODE *) NULL; t = t->pst_left)
	t->pst_sym.pst_value.pst_s_rsdm.pst_rsno = kk - tot++;
}
