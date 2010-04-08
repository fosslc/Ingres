/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cm.h>
#include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<adf.h>
#include	<afe.h>

/**
** Name:	aftxtrns.c -	Translate String with respect to
**					Text String Semantics.
** Description:
**	Contains routines used to translate strings that contain text string
**	escape sequences and QUEL pattern-matching characters.  Defines:
**
**	IIAFpmEncode()	encode QUEL p.m. and text escape sequences into string.
**
** History:
**	Revision 8.0  89/08  wong
**	Replaced 'afe_txtconst()' and 'IIAFtxTrans()' with 'IIAFpmEncode()'.
**
**	Revision 6.2  89/07/22  dave
**	Added 'IIAFfpcFindPatternChars()' for JupBug #6803.
**
**	Revision 6.0  87/02/06  daver
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/*{
** Name:	IIAFpmEncode() -	Encode QUEL Pattern-Match Characters
**						and Text Escape Sequences
**						in String Data Value.
** Description:
**	Encodes QUEL pattern-match characters and text escape sequences in an
**	input string data value.  This calls 'adi_pm_encode()' to do the work.
**
** Inputs:
**	value	{DB_DATA_VALUE *}  The string value to encode.
**	pmatch	{bool}  Whether to encode pattern-match characters.
**	pm	{nat *}  The pattern-match encode result. 
**
** Outputs:
**	value	{DB_DATA_VALUE *}
**			.db_data	{PTR}  The encoded string.
**	pm	{nat *}  AFE_PM_FOUND if pattern-match characters were encoded;
**			 AFE_PM_NOT_FOUND otherwise.
**
** Returns:
**	{DB_STATUS}  OK if no errors; otherwise status from 'adi_pm_encode()'.
**
** History:
**	08/89 (jhw) -- Written.
*/
DB_STATUS
IIAFpmEncode ( value, pmatch, pm )
DB_DATA_VALUE	*value;
bool		pmatch;
i4		*pm;
{
	bool		pm_found;
	i4		lines;
	DB_STATUS	stat;

	ADF_CB	*FEadfcb();

	*pm = AFE_PM_NOT_FOUND;
	if ( (stat = adi_pm_encode( FEadfcb(),
				    (pmatch ? ADI_DO_PM : 0) | ADI_DO_BACKSLASH,
				    value, &lines, &pm_found)) != OK )
	{
		return stat;
	}

	if ( pm_found )
		*pm = AFE_PM_FOUND;

	return OK;
}

/*{
** Name:	IIAFfpcFindPatternChars - Find a pattern match character.
**
** Description:
**	This routine is called to check if a string contains a
**	pattern match character.
**
** Inputs:
**	dbv		Pointer to DB_DATA_VALUE containing string to search.
**
** Returns:
**	{bool}	TRUE	If pattern match characters were found.
**		FALSE	If no pattern match characters were found.
**
** History:
**	07/22/89 (dkh) - Initial version (for bug fix 6803).
*/
bool
IIAFfpcFindPatternChars ( DB_DATA_VALUE *dbv )
{
	i4		length;
	register u_char	*chr;
	register i4	i;

	if ( AFE_ISNULL(dbv) )
		return FALSE;

	switch ( AFE_DATATYPE(dbv) )
	{
		case DB_TXT_TYPE:
		case DB_VCH_TYPE:
		case DB_LTXT_TYPE:
		{
			DB_TEXT_STRING	*txtptr = (DB_TEXT_STRING*)dbv->db_data;

			length = txtptr->db_t_count;
			chr = txtptr->db_t_text;
			break;
		}

		case DB_CHA_TYPE:
		case DB_CHR_TYPE:
			length = dbv->db_length;
			if ( AFE_NULLABLE(dbv->db_datatype) )
				--length;
			chr = (u_char *) dbv->db_data;
			break;

		default:
			return FALSE;
			break;
	}
	/*
	**  Since the special pattern match characters don't
	**  overlap with any known character sets (including
	**  double byte ones), we just step down the string
	**  byte by byte.
	*/
	for ( i = length ; --i >= 0 ; ++chr )
	{
		if (*chr == DB_PAT_ANY ||
			*chr == DB_PAT_ONE ||
			*chr == DB_PAT_LBRAC ||
			*chr == DB_PAT_RBRAC)
		{
			return TRUE;
		}
	}

	return FALSE;
}
