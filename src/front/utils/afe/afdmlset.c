/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>

/**
** Name:    afdmlset.c -	Set DML For ADF Control Structure.
**
** Description:
**	Contains the routine 'IIAFdsDmlSet()' that sets the query language
**	in an ADF control structure.
**
** History:
**	Revision 6.0  87/20/11  dave
**	Initial version taken from "abfrt!abrtcall.qc."
**/

ADF_CB	*FEadfcb();

/*{
** Name:    IIAFdsDmlSet() -	Set the DML for an ADF_CB struct.
**
** Description:
**	Given a DB_LANG	value, this routine will set the adf_qlang
**	member of the ADF_CB struct returned by call to FEadfcb()
**	to this value and return the old one.
**
** Inputs:
**	dml	{DB_LANG}  New DML to set in the control structure.
**
** Returns:
**	{DB_LANG}	DML value that was in the control structure.
**
** Side Effects:
**	Changes the DML value in the ADF control structure.
**
** History:
**	11/20/87 (dkh) - Initial version taken from "abfrt!abrtcall.qc"
*/
DB_LANG
IIAFdsDmlSet (dml)
DB_LANG	dml;
{
    DB_LANG		olddml;
    register ADF_CB	*cb;

    cb = FEadfcb();
    olddml = cb->adf_qlang;
    cb->adf_qlang = dml;

    return olddml;
}
