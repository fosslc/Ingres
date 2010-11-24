/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
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
**  Name: PSTCLRRNG.C - Functions for clearing range table
**
**  Description:
**      This file contains the functions for clearing the user range table.
**	This must be done after every parse.  It frees the memory in which
**	the table description information is stored.
**
**          pst_clrrng - Clear the user range table
**
**
**  History:    $Log-for RCS$
**      03-apr-86 (jeff)    
**          written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	08-jan-93 (rblumer)
**	    don't free rdf info for dummy set-input range variables.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 pst_clrrng(
	PSS_SESBLK *sess_cb,
	PSS_USRRANGE *rngtab,
	DB_ERROR *err_blk);

/*{
** Name: pst_clrrng	- Clear the user range table.
**
** Description:
**      This function clears the user range table.  It deallocates the memory
**	in which the table descriptions are stored, and it sets the memory
**	stream pointer for each range variable to NULL, so we can tell it's
**	no longer used.
**
** Inputs:
**      rngtab                          Pointer to the user range table
**      err_blk                         Place to put error info
**	sess_cb			 	Pointer to session control block
**
** Outputs:
**	rngtab				All mem. stream ptrs. set to NULL
**      err_blk                         Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Deallocates memory
**
** History:
**	03-apr-86 (jeff)
**          written
**	28-jul-86 (jeff)
**	    made it clear RDF info pointer for result range variable
**	1-sep-89 (andre)
**	    reset all bits inside the pss_outer_rel and pss_inner_rel.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**	21-may-90 (teg)
**	    init rdr_instr to RDF_NO_INSTR
**	15-jun-92 (barbara)
**	    Added sess_cb as a parameter.
**	03-aug-92 (barbara)
**	    Call pst_rdfcb_init to initialize RDF_CB before rdf_call()
**	08-jan-93 (rblumer)
**	    don't free rdf info for dummy set-input range variables.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    removed declaration of rdf_call() + cast the second arg as (PTR)
**	    to avoid acc warnings
**	25-jan-06 (dougi)
**	    Don't UNFIX derived table entries.
**	23-Jun-2006 (kschendel)
**	    Use new general unfixer.
*/
DB_STATUS
pst_clrrng(
	PSS_SESBLK	   *sess_cb,
	PSS_USRRANGE       *rngtab,
	DB_ERROR           *err_blk)
{
    DB_STATUS           status;
    register i4	i;
    register PSS_RNGTAB *rngvar;

    /* Reset the maximum range variable number */
    rngtab->pss_maxrng = -1;

    /* Close off all streams. */
    for (i = 0, rngvar = rngtab->pss_rngtab; i < PST_NUMVARS; i++, rngvar++)
    {
	/* Release cached description;
	** but don't release it if this is a dummy set-input range variable
	** and we are creating the procedure for the first time
	** or if it is a derived table (subselect in FROM clause).
	*/
	status = pst_rng_unfix(sess_cb, rngvar, err_blk);
	if (status != E_DB_OK)
	    return (status);

	/* Clear the range variable number */
	rngvar->pss_rgno = -1;

	/* clear the inner and the outer relation mask */
	MEfill(sizeof(PST_J_MASK), (u_char) 0, (PTR) &rngvar->pss_inner_rel);
	MEfill(sizeof(PST_J_MASK), (u_char) 0, (PTR) &rngvar->pss_outer_rel);
    }

    /* Release description for result range variable;
    ** but don't release it if this is a dummy set-input range variable
    ** and we are creating the procedure for the first time
    */
    status = pst_rng_unfix(sess_cb, &rngtab->pss_rsrng, err_blk);
    if (status != E_DB_OK)
	return (status);

    /* Clear the range variable number */
    rngtab->pss_rsrng.pss_rgno = -1;

    /* clear the inner and the outer relation mask */
    MEfill(sizeof(PST_J_MASK), (u_char) 0,
           (PTR) &rngtab->pss_rsrng.pss_inner_rel);
    MEfill(sizeof(PST_J_MASK), (u_char) 0,
           (PTR) &rngtab->pss_rsrng.pss_outer_rel);

    return (E_DB_OK);
}
