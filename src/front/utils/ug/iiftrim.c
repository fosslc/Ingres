/*
**	iiftrim.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include	<cm.h>

/**
** Name:	iiftrim.c	-	Do forms system char fld trimming
**
** Description:
**
**	Public (extern) routines defined:
**	Private (static) routines defined:
**
** History:
**	01-apr-1987	Created (drh)
**	10/14/88 (dkh) - Changed to handle double byte characters correctly.
**	10/25/88 (dkh) - Changed to reset nullable length correctly.
**	09/12/92 (dkh) - Added IIUGfnFeName() to create a valid FE name
**			 given a string.  Stolen from Vifred.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


FUNC_EXTERN	ADF_CB	*FEadfcb();
FUNC_EXTERN	STATUS	afe_clinstd();

/*{
** Name:	IIftrim		-	Trim a forms system character field
**
** Description:
**	The forms system always has trimmed trailing blanks
**	from character-type field values that are being returned to callers.
**	This is a unique forms system characteristic - an EQUEL program 
**	that retrieves a character-type value from the database will NOT
**	have it's trailing blanks trimmed.  Therefore, this routine
**	is called from all RUNTIME and TBACC routines that return FRS field
**	values to callers, to 'trim' trailing blanks.
**
**	Provided with an input and output DB_DATA_VALUE, the ouput
**	DB_DATA_VALUE will contain the trimmed data value for the
**	field.  As a side effect, if the input DBV was nullable, and
**	the actual value in it was not NULL, the output DBV will have
**	been converted to a non-nullable type, so CALLER BEWARE.
**
**	Method:  The input DB_DATA_VALUE is copied into the output one (NOTE
**	the DB_DATA_VALUE itself is copied, not the data it points to).
**	If the value is not NULL afe_dtrimlen is called to trim the value.
**	This is a change from the previous method is order to handle
**	double byte characters correctly.
**
** Inputs:
**	indbv		Ptr to a DB_DATA_VALUE to check, and perhaps, to trim.
**	outdbv		Ptr to a DB_DATA_VALUE to update with the trimmed
**				value.
**
** Outputs:
**	outdbv		Will be updated with a copy of the input DBV, trimmed
**				if necessary, and possibly made non-nullable.
**
** Returns:
**	STATUS		OK
**			FAIL	
**
** Exceptions:
**	none
**
** Side Effects:
**	If the dbv passed in was a nullable but pointed to a non-null value, 
**	then the dbv copy returned will be non-nullable.
**
** History:
**	01-apr-1987	Created (drh).
**	21-apr-1987	Modified input parameters to accept an output
**			DB_DATA_VALUE.
*/
STATUS
IIftrim( indbv, outdbv )
DB_DATA_VALUE	*indbv;
DB_DATA_VALUE	*outdbv;
{
	ADF_CB		*cb;
	DB_DATA_VALUE	result;
	DB_DATA_VALUE	*vals[3];
	AFE_OPERS	opers;
	ADI_FI_DESC	*fdesc;
	i4		isnull;
	i2		resval;
	bool		needtrim;
	bool		nullable;

	cb = FEadfcb();

	/*
	**  Copy the input DBV into the output one
	*/

	STRUCT_ASSIGN_MACRO( *indbv, *outdbv );

	/*
	**  If the dataype is nullable and the value is null, there is
	**  no need to trim.
	*/

	nullable = AFE_NULLABLE_MACRO( indbv->db_datatype );
	if ( nullable )
	{
		if ( ( adc_isnull( cb, indbv, &isnull ) ) != OK )
		{
			FEafeerr( cb );
			return( FAIL );
		}

		if ( isnull )
			return( OK );

		/*
		**  Reduce length so the last byte used for
		**  storing nullability is not looked at.
		*/
		outdbv->db_length--;

	}

	if (afe_dtrimlen( cb, outdbv) != OK)
	{
		if (nullable)
		{
			outdbv->db_length++;
		}
		FEafeerr( cb );
		return( FAIL );
	}

	if (nullable)
	{
		outdbv->db_length++;
	}

	return( OK );
}


/*{
** Name:	IIUGfnFeName - Create a valid FE Name given a string.
**
** Description:
**	This routine will scan through a string pulling out characters
**	that are valid for use as a valid FE name.  The resultant name
**	will be lower cased as well.  This routine will stop after pulling
**	out 32 bytes of characters.  Note that it is up to the caller to
**	check for an empty result string as well as providing an output
**	buffer that is big enough.
**
**	The rules for a FE name are:
**	- mono case (essentially lower case)
**	- up to 32 bytes in length.
**	- must start with an alpha or underscore character
**	- other valid characters are digits (0-9), #, @ and $
**	  the CM routines will actually enforced this for us.
**
** Inputs:
**	in		The input string to pull characters from
**
** Outputs:
**	out		Buffer for holding the constructed FE name.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/12/92 (dkh) - Initial version.
*/
void
IIUGfnFeName(char *in, char *out)
{
	i4	i;

	/*
	**  Clear output buffer.
	*/
	*out = EOS;

	/*
	**  Go through the string looking for a character that can
	**  be used as the starting character in a FE name.
	*/
	while (*in != EOS && !CMnmstart(in))
	{
		CMnext(in);
	}

	/*
	**  If there are no characters that can be used as the
	**  starting character in a FE name, just return.
	*/
	if (*in == EOS)
	{
		return;
	}

	/*
	**  Starting with the valid starting character found above,
	**  go through remainder of string looking for characters
	**  that can be used.
	*/
	for (i = 0; i < FE_MAXNAME && *in != EOS; CMnext(in))
	{
		if (CMnmchar(in))
		{
			CMbyteinc(i, in);

			/*
			**  If result is still less than the
			**  maximum allowed size of a FE name
			**  copy it to the output and lowercase
			**  it as well.
			*/
			if (i <= FE_MAXNAME)
			{
				CMtolower(in, out);
				CMnext(out);
			}
			else
			{
				/*
				**  We will overflow if
				**  we do the copy.  So just
				**  get out of the loop now.
				*/
				break;
			}
		}
	}

	/*
	**  Terminate the output string.
	*/
	*out = EOS;
}
