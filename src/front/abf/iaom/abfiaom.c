/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/
#include	<compat.h>
#include	<er.h>
#include	<tm.h>
#include	<dmchecks.h>
#include	<abclass.h>

/*
**	Name:	abfiaom.c
**
**	Description: Contains routines to pass data values across boundaries.
**
**	In order to put the IAOM directory files in a Dynamic Link and/or
**	Shared Library, it is necessary not to have references from IAOM
**	to data in other ABF directories and not to have references from
**	other ABF directories to data in IAOM.  This is because DLLs and
**	ShareLibs can only access functions across boundaries at compile/link
**	time.  Therefore functions were created to pass data pointers across 
**	these boundaries at run time.  Two of these functions reside in this
**	module; the first gets the VISION flag, IIabVision from ABF and 
**	stores a copy of its value in IIab_Vision; the other passes to the 
**	abf/abmain.c routine in IIABF the address of a structure which 
**	contains addresses of most of the data that resides in IAOM that is 
**	needed in IIABF.  Other data, needed by multiple programs created from 
**	the ABF directories, are passed via functions that reside in 
**	iaom/abclass.c and iaom/iamsystm.c
**
**	History:
**
**	02-dec-91 (leighb) DeskTop Porting Change: Created.
*/

GLOBALREF OO_CLASS	IIamClass[];
GLOBALREF OO_CLASS	IIamProcCl[];
GLOBALREF OO_CLASS	IIamGlobCl[];
GLOBALREF OO_CLASS	IIamRecCl[];
GLOBALREF OO_CLASS	IIamRAttCl[];
GLOBALREF OO_CLASS	IIamConstCl[];

/* 
**		Same value as IIabVision, but for use by IAOM routines.
**
**	It must have a different name because when DLLs and ShareLibs are not
**	used, the values are passed nonetheless and it is not possible to have
**	2 GLOBALDEFs of the same name in the same program.  When DLLs or
**	ShareLibs are used it is necessary to have 2 GLOBALDEFs because the
**	value must appear in 2 separate programs/libs.
*/

GLOBALDEF bool		IIab_Vision;

/* Structure containing addresses to be passed back to ABF */

IIAMABFIAOM		IIAMabfiaom = {
					(PTR)IIamClass,
					(PTR)IIamProcCl,
					(PTR)IIamGlobCl,
					(PTR)IIamRecCl,
					(PTR)IIamRAttCl,
					(PTR)IIamConstCl,
					&IIAMzdcDelCheck,
					&IIAMzccCompChange,
					&IIAMzctCompTree,
					&IIAMzdcDateChange,
					&IIAMzraRecAttCheck
};

/*
**	Routine to pass value from ABF to IAOM.
*/

VOID
IIAMsivSetIIab_Vision(v)
bool	v;
{
	IIab_Vision = v;
}

/*
**	Routine to return values from IAOM to ABF.
*/

IIAMABFIAOM *
IIAMgaiGetIIAMabfiaom()
{
	return(&IIAMabfiaom);
}							 
