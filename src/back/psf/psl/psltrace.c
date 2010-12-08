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
#include    "pslgram.h"
#include    <yacc.h>

/**
**
**  Name: PSLTRACE.C - Set and clear yacc tracing
**
**  Description:
**	This file contains the functions for setting and clearing tracing for
**	the QUEL grammar.  It requires a separate function within the QUEL
**	directory because this is the only place where there is knowledge of
**	the yacc control block.  Alternatively, one could make yaccpar know
**	how to test trace vectors, but that would force whoever was using yacc
**	to use a certain style of trace vector, which might not be appropriate.
**
**          psl_trace - Set or clear the yacc trace flag w/i the yacc cb
**
**
**  History:    $Log-for RCS$
**      27-may-86 (jeff)    
**          written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void psl_trace(
	PTR yacc_cb,
	i4 onoroff);

/*{
** Name: psl_trace	- Set or clear the yacc trace flag
**
** Description:
**      This function sets or clears the yacc trace flag used by the grammar
**	for debug output.
**
** Inputs:
**      yacc_cb                         Pointer to the yacc control block
**	onoroff				TRUE means turn it on; FALSE means off
**
** Outputs:
**	yacc_cb
**	    yydebug			Turned on or off as appropriate
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-may-86 (jeff)
**          written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/
VOID
psl_trace(
	PTR                yacc_cb,
	i4		   onoroff)
{
    YACC_CB	*yacc = (YACC_CB *) yacc_cb;

    yacc->yydebug = onoroff;
}
