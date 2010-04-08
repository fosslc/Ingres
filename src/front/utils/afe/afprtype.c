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
#include	<afe.h>

/**
** Name:	afprtype.c -	Front-End Compare Function Instances and Types
**
** Description:
**	Contains a routine that determines which of two types is more
**	preferable.  This file should eventually be replaced by an ADF routine.
**	Defines:
**
**	afe_preftype()  compare two types to find the preferable one.
**
** History:
**	Revision 6.4  89/11  wong
**	Removed 'IIAFopCompare()' since 'afe_fdesc()' now does type resolution
**	using 'ade_resolve()'.  Also added DECIMAL support.
**	
**	Revision 6.0  88/02/20  wong
**	Initial revision (copied from "back/psf/pst.")
**	21-feb-1988 (Joe)
**		Put back afe_preftype since it is used by
**		report.
**	10/28/88 (dkh) - Performance changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

static i4	ty_rank();

/*{
** Name:	afe_preftype() -	Compare datatypes for preference.
**
** Description:
**	Determines whether one datatype is preferable to another for purposes
**	of datatype resolution.
**
**	NOTE:  This function should be replaced by an ADF routine.
**
** Inputs:
**      curtype	{ADI_DT_ID}  The type we are considering
**      bestype	{ADI_DT_ID}  The best type found so far
**
** Returns:
**	{bool}  TRUE	curtype is preferable to bestype
**			FALSE	bestype is preferable to curtype
**
** History:
**	23-apr-86 (jeff) written
**	31-mar-87 (daver) extracted out of affdesc.c
**	02/03/88 (jhw)  Reordered C, Text, Char, Varchar preferences
**						as per ADF change.
**	21-feb-1988 (Joe)
**		Put back in, but now uses ty_rank().
*/
bool
afe_preftype (DB_DT_ID curtype, DB_DT_ID bestype)
{
    return ty_rank(curtype) < ty_rank(bestype);
}

/*
** Name:	ty_rank() -    Rank Data Types to Determine Coercion Precedence.
**
** Description:
**	This function figures out the precedence ranking of a datatype for
**	coercion.  A datatype ID of DB_NODT will get a ranking of zero.  An
**	abstract type will be ranked 1, then float 2, and so forth.  Basically,
**	the lower the rank, the more preferable.  The order of preference is:
**
**		Most Preferable		Rank
**		^	Abstract	1
**	       /|\	Float		2
**		|	Decimal		3
**		|	Int		4
**		|	C		5
**		|	Text		6
**		|	Char		7
**		|	Varchar		8
**		|	LongText	9
**		Least Preferable
**
**	This function considers any type it doesn't know about to be abstract.
**	That is, if it's not float, int, c, text, char, varchar, or longtext,
**	it's abstract.
**
**	NOTE:  This function should be replaced by an ADF routine.
**
** Inputs:
**	dt	{DB_DT_ID}  The type for which we are considering preference.
**
** Returns:
**	{nat}  Precedence rank for input datatype.
**
** History:
**	18-feb-88 (thurston)
**		Initial creation, meant to be used instead of ty_better().
**	11/89 (jhw) -- Added DECIMAL support.
*/
static i4
ty_rank (dt)
DB_DT_ID	dt;
{
	switch (abs(dt))
	{
	  case DB_NODT:
		return 0;
	
	  case DB_LTXT_TYPE:
		return 9;
	
	  case DB_VCH_TYPE:
		return 8;
	
	  case DB_CHA_TYPE:
		return 7;
	
	  case DB_TXT_TYPE:
		return 6;
	
	  case DB_CHR_TYPE:
		return 5;
	
	  case DB_INT_TYPE:
		return 4;

	  case DB_DEC_TYPE:
		return 3;
	
	  case DB_FLT_TYPE:
		return 2;
	
	  default:		/* Must be abstract */
		return 1;
	} /* end switch */
	/*NOTREACHED*/
}
