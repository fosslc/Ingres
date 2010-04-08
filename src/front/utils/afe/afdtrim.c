/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>

/**
** Name:	afdtrim.c  -	Contains afe_dtrimlen
**
** Description:
**	This file contains the routine afe_dtrimlen.
**
** History:
**	30-mar-1987 (Joe)
**		Written
**	25-jun-1987 (danielt)
**		cast pointer argument to afe_error to PTR
**	10/14/88 (dkh) - Changed to handle double byte characters correctly.
**	02-may-1989 (sylviap)
**		Added absolute value for nullable fields.
**	09/90 (jhw) -- Moved 'AFE_TRIMBLANKS()' into <afe.h>.
**	14-1-92 (seg)  Can't do arithmetic on PTR variables.  Need to cast.
**/

/*{
** Name:	afe_dtrimlen() -	Determine if datatype needs trimming
**
** Description:
**	Given a datatype, trim the passed in value using the above
**	macro.  This is only done for DB_CHR_TYPE and DB_CHA_TYPE.
**	The above macro is used since none of the functions in ADF
**	currently has the desired semantics.
**
**	This function will be used by the forms system to ensure that it
**	always returns trimmed data values to it's callers.
**
** Inputs:
**	cb			A ADF control block.
**
**	value			A DB_DATA_VALUE to be checked.
**
**		.db_datatype	The type id for the data value.
**
**
** Outputs:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	23-mar-1987 (Robin)
**		Designed
**	30-mar-1987 (Joe)
**		Written
**	14-1-92 (seg)  Can't do arithmetic on PTR variables.  Need to cast.
*/
STATUS
afe_dtrimlen(cb, value)
ADF_CB		*cb;
DB_DATA_VALUE	*value;
{
    switch (abs(value->db_datatype))
    {
	case DB_CHR_TYPE:
	case DB_CHA_TYPE:
	    AFE_TRIMBLANKS((char *)value->db_data, value->db_length);
	    break;
	
	default:
	    break;
    }
    return OK;
}
