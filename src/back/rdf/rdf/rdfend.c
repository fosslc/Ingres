/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<ddb.h>
#include	<ulf.h>
#include	<ulm.h>
#include        <cs.h>
#include	<scf.h>
#include	<ulh.h>
#include	<adf.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qefmain.h>
#include        <qsf.h>
#include	<qefrcb.h>
#include        <qefcb.h>
#include	<qefqeu.h>
#include        <rqf.h>
#include	<rdf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>
/**
**
**  Name: RDFEND.C - End a RDF session
**
**  Description:
**      This file contains the rdf_end_session function which is called
**	by SCF to end an RDF session.
**
**          rdf_end_session - end an RDF session.
**
**
**  History:
**      04-apr-89 (mings)
**          initial version.
**	20-dec-91 (teresa)
**	    removed include of qefddb.h since that is now part of qefrcb.h
**	10-feb-92 (teresa)
**	    removed globalref to rdf_svcb.
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA. 
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.
**/

/*{
** Name: rdf_end_session - end an RDF session.
**
**	External call format:	status = rdf_call(RDF_END_SESSION, &rdf_ccb)
**
** Description:
**	This routine set RDF server control block pointer in rdf_ccb.rdf_server
** Inputs:
**      rdf_ccb
**	        .rdf_server		ptr to server control block.
**
** Outputs:
**      rdf_ccb                          
**					
**		.rdf_error              Filled with error if encountered.
**
**			.err_code	One of the following error numbers.
**				E_RD0000_OK
**					Operation succeed.
**				E_RD0003_BAD_PARAMETER
**					Bad pointer to RDF server block.
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_ccb.rdf_error.
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	04-apr-89 (mings)
**	    initial version.
**	17-jul-92 (teresa)
**	    Prototypes
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value RDF_CCB's rdf_error.
*/
DB_STATUS
rdf_end_session(    RDF_GLOBAL	*global,
		    RDF_CCB	*rdf_ccb)
{
	global->rdfccb = rdf_ccb;
	SETDBERR(&rdf_ccb->rdf_error, 0, E_RD0000_OK);
	return (E_DB_OK);
}
