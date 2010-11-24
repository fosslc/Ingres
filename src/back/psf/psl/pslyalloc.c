/*
**Copyright (c) 2004, 2010 Ingres Corporation
**
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
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    "pslgram.h"
#include    <yacc.h>

/**
**
**  Name: PSLYALLOC.C - Functions for allocation yacc control block
**
**  Description:
**      This file contains the functions for allocating yacc control blocks 
**
**          psl_yalloc - Allocate a yacc control block 
**
**
**  History:    $Log-for RCS$
**      07-jan-86 (jeff)    
**          written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE for memory pool > 2Gig.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psl_yalloc(
	PTR stream,
	SIZE_TYPE *memleft,
	PTR *yacc_cb,
	DB_ERROR *err_blk);
void psl_yinit(
	PSS_SESBLK *sess_cb);

/*{
** Name: psl_yalloc	- Allocate a yacc control block
**
** Description:
**      This function allocates a yacc control block.  It should be called 
**      when starting up a session.
**	In addition to allocating the yacc control block, this function fills
**	in the control block with pointers to the scanner and yacc error
**	functions.
** 
** Inputs:
**      stream                          Pointer to the memory stream to use
**      yacc_cb                         Pointer to place to put cb pointer
**      err_blk                         Pointer to error block
**
** Outputs:
**      yacc_cb                         Filled in with control block pointer
**	    .yyerror			    Set to point to yacc error function.
**	    .yylex			    Set to point to QUEL scanner.
**      err_blk                         Error info (if any) put here
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**					error status in psq_cb.psq_error chain
**					of error blocks.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in psq_cb.psq_error chain
**					of error blocks.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					psq_cb.psq_error chain of error blocks.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	07-jan-86 (jeff)
**          written
**	27-jan-87 (daved)
**	    allocate sql control block
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/
DB_STATUS
psl_yalloc(
	PTR		stream,
	SIZE_TYPE	*memleft,
	PTR    		*yacc_cb,
	DB_ERROR        *err_blk)
{
    i4		  err_code;
    ULM_RCB		  ulm_rcb;
    DB_STATUS		  status;
    YACC_CB		  *yacc;
    
    ulm_rcb.ulm_facility = DB_PSF_ID;
    ulm_rcb.ulm_streamid_p = &stream;
    ulm_rcb.ulm_psize = sizeof(YACC_CB);
    ulm_rcb.ulm_memleft = memleft;
    if (ulm_palloc(&ulm_rcb) != E_DB_OK)
    {
	if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, err_blk, 0);
	}
	else
	    psf_error(E_PS0207_YACC_ALLOC, ulm_rcb.ulm_error.err_code,
		PSF_INTERR, &err_code, err_blk, 0);

	if (ulm_rcb.ulm_error.err_code == E_UL0004_CORRUPT)
	{
	    status = E_DB_FATAL;
	}
	else
	{
	    status = E_DB_ERROR;
	}

	return (status);
    }
    yacc	    = (YACC_CB*) ulm_rcb.ulm_pptr;
    yacc->yyr_push = &yacc->yyr;
    yacc->yyr_pop  = &yacc->yyr;
    yacc->yyr.yy_next = (YYTOK_LIST*) NULL;

    /* expect lint error */
    yacc->yyerror   = psl_yerror;
    yacc->yydebug   = FALSE;

    *yacc_cb	    = (PTR) yacc;
    return (E_DB_OK);
}

/*{
** Name: psl_yinit	- Init the yacc control block
**
** Description:
**      This function initializes the yacc control block, usually on
**	behalf of psq_parseqry, for each query.
** 
** Inputs:
**	sess_cb				session control block
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
**	10-feb-87 (daved)
*/
VOID
psl_yinit(
	PSS_SESBLK	*sess_cb)
{
    YACC_CB		    *yacc_cb = (YACC_CB*)sess_cb->pss_yacc;

    yacc_cb->yyr_save	= 0;
    yacc_cb->yyr_push	= yacc_cb->yyr_pop = &yacc_cb->yyr;
    yacc_cb->yyr_top	= 0;
    yacc_cb->yyr_cur	= 0;
    if (sess_cb->pss_lang == DB_QUEL)
	yacc_cb->yylex	    = psl_scan;
    else
	yacc_cb->yylex	    = psl_sscan;
}
