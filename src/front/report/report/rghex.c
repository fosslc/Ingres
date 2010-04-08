/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cv.h>
# include	<cm.h>
# include	<st.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<adf.h>
# include	<adfops.h>
# include	<fmt.h>
# include	<rtype.h> 
# include	<rglob.h> 

/*
** R_G_HEXLIT -	Retrieve and verify a hexidecimal literal from the current
**		line.  The routine assumes that Tokchar points to a string
**		of the form
**			'010203...FEFF'
**		Note, then, that it assumes that the flagging "x" or "X"
**		character has already been processed.
**
**		The string is optionally copied to permanent storage allocated
**		by this routine as its binary value.  This value should be
**		treated as a VARCHAR - a length is returned, the initial bytes
**		reflect the length, and the data is not NULL terminated.
**
** Inputs:
**	hex_result -	Pointer to a DB_TEXT_STRING pointer in which to place
**			the address of the allocated hex string.  This may be
**			NULL.
**
** Outputs:
**	hex_result -	Permanent storage for the "converted" hexadecimal string
**			is allocated, and its address stored in the
**			DB_TEXT_STRING pointer pointed to by hex_result,
**			providing hex_result is specified as non-NULL.  SREPORT
**			is expected to pass NULL; REPORT is expected to pass
**			non-NULL.
**
**			In case of error (invalid string - odd number of hex
**			digits, characters other than 0-9,A-F), *hex_result
**			will reflect NULL and no storage is allocated.
**
** Returns:
**	digit_cnt -	Count of hex digits represented by the hex literal
**			string.  This will be the actual valid length of the
**			allocated hex_result character.  A value of zero
**			indicates that the string was invalid.
**
** History:
**
**	17-may-1993 (rdrane)
**		Created.
**	6-oct-1993 (rdrane)
**		Re-order #include's of fe.h and ug.h, as ug.h now
**		has a dependency with fe.h.  Sigh.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/


static	bool	rgh_cnv_hex_digit();


i4
r_g_hexlit(hex_result)
	DB_TEXT_STRING	**hex_result;
{
	i4	digit_cnt;
	i4	hex_digit;
	char	*hex_ptr;
	bool	first_digit;
	char	hex_char;
	char	hex_buf[MAXRTEXT];
		
	
	digit_cnt = 0;
	first_digit = TRUE;	
	if  (hex_result != (DB_TEXT_STRING **)NULL)
	{
		*hex_result = (DB_TEXT_STRING *)NULL;
	}
	hex_ptr = &hex_buf[0];
	CMnext(Tokchar);

	while ((digit_cnt < MAXRTEXT) &&
	       (*Tokchar != 0x00) && (*Tokchar != '\''))
	{
		if  (!rgh_cnv_hex_digit(&hex_digit))
		{
			/*
			** Invalid character found!
			*/
			return(0);
		}
		CMnext(Tokchar);
		if  (first_digit)
		{
			*hex_ptr = hex_digit * 16;
		}
		else
		{
			*hex_ptr += hex_digit;
			hex_ptr++;
			digit_cnt++;
		}
		first_digit ^= TRUE;
	}
		
	hex_char = *Tokchar;
	CMnext(Tokchar);

	if  (hex_char != '\'')
	{
		/*
		** No termination of value string, or
		** string exceeded max allowed hex digits.
		*/
		return(0);
	}

	if  (!first_digit)
	{
		/*
		** Uneven number of hex digits
		*/
		return(0);
	}
	
	if  (hex_result != (DB_TEXT_STRING **)NULL)
	{
		*hex_result = (DB_TEXT_STRING *)MEreqmem((u_i4)0,
			(u_i4)(digit_cnt +
			 sizeof((*hex_result)->db_t_count)),
			TRUE,(STATUS *)NULL);
		if  (*hex_result == (DB_TEXT_STRING *)NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("r_g_hex - hex_result"));
		}
		(*hex_result)->db_t_count = digit_cnt;
		MEcopy((PTR)&hex_buf[0],(u_i2)digit_cnt,
		       (PTR)(&(*hex_result)->db_t_text[0]));
	}
	return(digit_cnt);
}



static
bool
rgh_cnv_hex_digit(hex_digit)
	int	*hex_digit;
{


	/*
	** It's not really redundant ... we do this to avoid
	** being confused by double-byte characters.  Note that
	** we asume that 0-9, A-F, and a-f will all exist as
	** single byte characters.
	*/
	if  ((!CMhex(Tokchar)))
	{
		return(FALSE);
	}
		
	switch(*Tokchar)
	{
	case '0':
		*hex_digit = 0;
		break;
	case '1':
		*hex_digit = 1;
		break;
	case '2':
		*hex_digit = 2;
		break;
	case '3':
		*hex_digit = 3;
		break;
	case '4':
		*hex_digit = 4;
		break;
	case '5':
		*hex_digit = 5;
		break;
	case '6':
		*hex_digit = 6;
		break;
	case '7':
		*hex_digit = 7;
		break;
	case '8':
		*hex_digit = 8;
		break;
	case '9':
		*hex_digit = 9;
		break;
	case 'A':
	case 'a':
		*hex_digit = 10;
		break;
	case 'B':
	case 'b':
		*hex_digit = 11;
		break;
	case 'C':
	case 'c':
		*hex_digit = 12;
		break;
	case 'D':
	case 'd':
		*hex_digit = 13;
		break;
	case 'E':
	case 'e':
		*hex_digit = 14;
		break;
	case 'F':
	case 'f':
		*hex_digit = 15;
		break;
	default:
		return(FALSE);
		break;
	}
		
	return(TRUE);
}

