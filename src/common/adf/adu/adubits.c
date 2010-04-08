/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include       <me.h>
# include	<iicommon.h>
# include	<adf.h>
# include	<ulf.h>
# include	<cm.h>
# include	<adfint.h>
# include       <aduint.h>

/**
** Name:	adubits.c - Routines to manipulate bit type(s)
**
** Description:
**	This file contains the routines which operate on the bit datatypes.
**	Externally, these are known as BIT, BIT VARYING, and LONG BIT, which
**	are DB_BIT_TYPE, DB_VBIT_TYPE, and DB_LBIT_TYPE respectively.
**
** This file defines:
**
**	adu_str2bit()	Convert character string type to bit type
**	adu_bit2str()	Convert bit type to character string
**	adu_bitlength()	Provide the length (in bits) of a string.
**	adu_mb_move_bits() Move bit types from one place/type/length to another.
**			 (Note that the name is ugly to 1) comply with the
**					8 character linker distinct rule,
**					and because of 1) to
**					2) avoid conflict with adu_movestring()
**      adu_bitsize()   Provide declared size for bit types.
**      adu_bitconcat() Concatenate two bit strings.
** Internal (static) routines:
**      adu__bit_find() Get bit length and locations.
**      adu__char_find() Get char length and locations.
** History:
**	23-sep-1992 (fred)
**	    Created.
**	22-Oct-1992 (fred)
**	   Altered bit pattern so that it matched ANSI requirements
**      31-dec-1992 (stevet)
**         Added function prototypes.
**      08-Jan-1993 (fred)
**         Added adu_bitsize() routine.
**      26-Mar-1993 (fred)
**         Added bit concatenation routine.  Moved a bunch of common
**         code into internal static routines to determine where the
**         number and location of bits and/or characters.
**      20-apr-1993 (stevet)
**         Added missing include file <me.h>.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      26-Jul-1993 (fred)
**	   Fixed bug in calculating destination string length.  Must use the
**         maximum possible length rather than the [unset] internal length.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* Internal routines */

/*{
** Name:	adu__bit_find - Find content of DB_DATA_VALUE
**
** Description:
**      This routine simply finds the appropriate bytes in a string of
**      bits.  For datatypes corresponding to bit types, this routine
**      returns the internal values which describe the location of and
**      number of bits in the db_data_value passed in.  Either of
**      these can be suppressed by passing 0 as the pointer into which
**      to put the answer.
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	dv		Pointer to DB_DATA_VALUE describing an existing bit
**				string;
**      bp              Pointer to uchar* into which to place the
**          	    	location of the bytes containing the bits.  If
**          	    	this is zero, do not supply this information.
**      blenp           Pointer to i4  to get the bit count.  If zero,
**          	    	do not return this information.
**
** Outputs:
**	*bp             Points to bits (if bp != 0)
**      *blenp          Number of valid bits (if != 0)
**
** Returns:
**	DB_STATUS
**
**		Can generate E_AD1070_BIT_TO_STR_OVFL if there are
**			more bits than character spaces; or
**			E_AD9999_INTERNAL_ERROR in unexpected cases.
**		These are returned in the scb, as usual.
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	05-Apr-1993 (fred)
**	    Created.
*/
static DB_STATUS
adu__bit_find(ADF_CB 	    *scb,
	      DB_DATA_VALUE *dv,
	      u_char        **bp,
	      i4            *blenp)
{
    DB_STATUS	    	status = E_DB_OK;
    u_char  	    	*bits;
    i4	    	    	blen;

    switch (dv->db_datatype)
    {
    case DB_BIT_TYPE:
	bits = (u_char *) dv->db_data;
	blen = ((dv->db_length- 1) * BITSPERBYTE) + dv->db_prec;
	break;
	
    case DB_VBIT_TYPE:
	bits = (u_char *) ((DB_BIT_STRING *) dv->db_data)->db_b_bits;
	blen = ((DB_BIT_STRING *) dv->db_data)->db_b_count;
	break;
	
    default:
	status = adu_error(scb, E_AD9999_INTERNAL_ERROR, 0);
	bits = NULL;
	blen = 0;
	break;
    }

    if (bp)
	*bp = bits;
    if (blenp)
	*blenp = blen;
    return(status);
}

/*{
** Name:	adu__char_find - Find content of DB_DATA_VALUE
**
** Description:
**      This routine simply finds the appropriate bytes in a string of
**      characters.  For datatypes corresponding to char types, this routine
**      returns the internal values which describe the location of and
**      number of chars in the db_data_value passed in.  Either of
**      these can be suppressed by passing 0 as the pointer into which
**      to put the answer.
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	dv		Pointer to DB_DATA_VALUE describing an existing
**				string;
**      cp              Pointer to uchar* into which to place the
**          	    	location of the bytes containing the chars.  If
**          	    	this is zero, do not supply this information.
**      clenp           Pointer to i4  to get the char count.  If zero,
**          	    	do not return this information.
**
** Outputs:
**	*cp             Points to chars (if bp != 0)
**      *clenp          Number of valid chars (if != 0)
**
** Returns:
**	DB_STATUS
**
**		Can generate E_AD1070_BIT_TO_STR_OVFL if there are
**			more bits than character spaces; or
**			E_AD9999_INTERNAL_ERROR in unexpected cases.
**		These are returned in the scb, as usual.
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	05-Apr-1993 (fred)
**	    Created.
*/
static DB_STATUS
adu__char_find(ADF_CB 	     *scb,
	       DB_DATA_VALUE *dv,
	       char          **cp,
	       i4            *clenp)
{
    DB_STATUS	    status = E_DB_OK;
    char            *c;
    i4              clen;

    switch (dv->db_datatype)
    {
    case DB_CHR_TYPE:
    case DB_CHA_TYPE:
	clen = dv->db_length;
	c = (char *) dv->db_data;
	break;
	
    case DB_VCH_TYPE:
    case DB_LTXT_TYPE:
    case DB_TXT_TYPE:
	clen = ((DB_TEXT_STRING *) dv->db_data)->db_t_count;
	c = (char *) ((DB_TEXT_STRING *) dv->db_data)->db_t_text;
	break;
	
    default:
	status = adu_error(scb, E_AD9999_INTERNAL_ERROR, 0);
	clen = 0;
	c = NULL;
	break;
    }
    if (cp)
	*cp = c;
    if (clenp)
	*clenp = clen;
    return(status);
}

/*{
** Name:	adu_bit2str - Convert bits to string type
**
** Description:
**	This routine convert bit types to strings of whatever variety
**	is desired.  For each bit encountered, it simply adds a '0' or '1'
**	to the output stream, as one would expect.
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	src_dv		Pointer to DB_DATA_VALUE describing an existing bit
**				string;
**	dst_dv		Pointer to DB_DATA_VALUE describing memory into which
**				some character string equivalent should be put.
**
** Outputs:
**	dst_dv		Character string equivalent is supplied.
**
** Returns:
**	DB_STATUS
**
**		Can generate E_AD1070_BIT_TO_STR_OVFL if there are
**			more bits than character spaces; or
**			E_AD9999_INTERNAL_ERROR in unexpected cases.
**		These are returned in the scb, as usual.
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	23-sep-1992 (fred)
**	    Created.
**	22-Oct-1992 (fred)
**     	   Altered bit pattern to match ANSI semantics.  Bits are now packed
**	   the high order end down.
**      26-Jul-1993 (fred)
**	   Fixed bug in calculating destination string length.  Must use the
**         maximum possible length rather than the [unset] internal length.
*/
DB_STATUS
adu_bit2str(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*src_dv,
DB_DATA_VALUE	*dst_dv)
{
    char		*bits;
    char		*c;
    char		*starting_c;
    DB_BIT_STRING	*bs;
    i4			clen;
    i4		blen;
    i4		bits_left;
    i4		byte_count;
    i4		i;
    i4		j;
    u_char		mask;
    DB_STATUS		status = E_DB_OK;

    for (;;)
    {
	switch (dst_dv->db_datatype)
	{
	  case DB_CHR_TYPE:
	  case DB_CHA_TYPE:
	    clen = dst_dv->db_length;
	    c = (char *) dst_dv->db_data;
	    break;
	    
	  case DB_VCH_TYPE:
	  case DB_LTXT_TYPE:
	  case DB_TXT_TYPE:
	    clen = dst_dv->db_length - DB_CNTSIZE;
	    c = (char *) ((DB_TEXT_STRING *) dst_dv->db_data)->db_t_text;
	    starting_c = c;
	    break;
	    
	  default:
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	
	switch (src_dv->db_datatype)
	{
	  case DB_BIT_TYPE:
	    bits = (char *) src_dv->db_data;
	    blen = ((src_dv->db_length- 1) * BITSPERBYTE) + src_dv->db_prec;
	    break;
	    
	  case DB_VBIT_TYPE:
	    bits = (char *) ((DB_BIT_STRING *) src_dv->db_data)->db_b_bits;
	    blen = ((DB_BIT_STRING *) src_dv->db_data)->db_b_count;
	    break;
	    
	  default:
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	if (status)
	    break;
	
	if (clen < blen)
	{
	    status = adu_error(adf_scb, E_AD1070_BIT_TO_STR_OVFL, 0);
	    break;
	}
	
	byte_count = (blen / BITSPERBYTE) + ((blen % BITSPERBYTE) ? 1 : 0);
	bits_left = blen;
	
	for (i = 0; i < byte_count && bits_left; i++)
	{
	    mask = 1 << (BITSPERBYTE - 1);

	    for (/* Do nothing */;
		 mask && bits_left;
		 mask >>= 1, bits_left -= 1, clen--)
	    {
		if (bits[i] & mask)
		    *c++ = '1';
		else
		    *c++ = '0';
	    }
	}

	switch (dst_dv->db_datatype)
	{
	  case DB_CHA_TYPE:
	  case DB_CHR_TYPE:
	    for ( /* Do nothing */; clen; c++, clen--)
		*c = ' ';
	    break;

	  case DB_TXT_TYPE:
	  case DB_VCH_TYPE:
	  case DB_LTXT_TYPE:
	    ((DB_TEXT_STRING *)dst_dv->db_data)->db_t_count = blen;
	    break;
	}
	break;
    }
    return(status);
}

/*{
** Name:	adu_str2bit - Convert string types to bit types
**
** Description:
**	This routine convert string types to bit strings of whatever variety
**	is desired.  For each '0' or '1' encountered, the appropriate bit is
**	added to the bit string.  Any character other than '0' or '1'
**	is an error;  white space is the exception -- it is ignored.
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	src_dv		Pointer to DB_DATA_VALUE describing an existing
**				character string;
**	dst_dv		Pointer to DB_DATA_VALUE describing memory into which
**				some bit string equivalent should be put.
**
** Outputs:
**	dst_dv		Bit string equivalent is supplied.
**
** Returns:
**	DB_STATUS
**
**		Can generate E_AD1071_NOT_BIT if character other than
**			'0','1', or white space is supplied.
**			E_AD1072_STR_TO_BIT_OVFL if there are more 0's and 1's
**				than space for bits, or
**			E_AD9999_INTERNAL_ERROR in unexpected cases.
**		These are returned in the scb, as usual.
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	23-sep-1992 (fred)
**	    Created.
**	22-Oct-1992 (fred)
**     	   Altered bit pattern to match ANSI semantics.  Bits are now packed
**	   the high order end down.
*/
DB_STATUS
adu_str2bit(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*src_dv,
DB_DATA_VALUE	*dst_dv)
{
    char		*bits;
    char		*c;
    char		*starting_c;
    DB_BIT_STRING	*bs;
    i4			clen;
    i4			starting_len;
    i4		blen;
    i4		input_bit_count;
    i4		bits_left;
    i4		byte_count;
    i4		i;
    i4		j;
    u_char		mask;
    DB_STATUS		status = E_DB_OK;

    for (;;)
    {
	switch (src_dv->db_datatype)
	{
	  case DB_CHR_TYPE:
	  case DB_CHA_TYPE:
	    clen = src_dv->db_length;
	    c = (char *) src_dv->db_data;
	    break;
	    
	  case DB_VCH_TYPE:
	  case DB_LTXT_TYPE:
	  case DB_TXT_TYPE:
	    clen = ((DB_TEXT_STRING *) src_dv->db_data)->db_t_count;
	    c = (char *) ((DB_TEXT_STRING *) src_dv->db_data)->db_t_text;
	    break;
	    
	  default:
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}

	starting_c = c;
	starting_len = clen;
	
	switch (dst_dv->db_datatype)
	{
	  case DB_BIT_TYPE:
	    bits = (char *) dst_dv->db_data;
	    byte_count = dst_dv->db_length;
	    break;
	    
	  case DB_VBIT_TYPE:
	    bits = (char *) ((DB_BIT_STRING *) dst_dv->db_data)->db_b_bits;
	    byte_count = dst_dv->db_length - DB_BCNTSIZE;
	    break;
	    
	  default:
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	if (status)
	    break;

        input_bit_count = ((byte_count - 1) * BITSPERBYTE) + dst_dv->db_prec;
	blen = 0;
	for (i = 0; i < byte_count && clen; i++)
	{
	    bits[i] = 0;  /* Start things off right */

	    for (mask = 1 << (BITSPERBYTE - 1);
		 mask && clen;
		 clen--)
	    {
		if (!CMwhite(c))
		{
		    if (*c == '1')
		    {
			bits[i] |= mask;
		    }
		    else if (*c != '0')
		    {
			status = adu_error(adf_scb, E_AD1071_NOT_BIT, 2,
				    starting_len, starting_c);
			break;
		    }
		    mask >>= 1;
		    blen += 1;
		}
		CMnext(c);
	    }
	    if (status)
		break;
	}
	if (status)
	    break;

	if (clen || (blen > input_bit_count))
	{
	    status = adu_error(adf_scb, E_AD1072_STR_TO_BIT_OVFL, 0);
	    break;
	}

	switch (dst_dv->db_datatype)
	{
	  case DB_BIT_TYPE:
	    for ( /* Do nothing */; i < byte_count; i++)
		bits[i] = 0;
	    break;

	  case DB_VBIT_TYPE:
	    ((DB_BIT_STRING *)dst_dv->db_data)->db_b_count = blen;
	    break;
	}
	break;
    }
    return(status);
}

/*{
** Name:	adu_bitlength - Provide bit_length information about bit types.
**
** Description:
**	This routine returns the number of bits in a given string.
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	src_dv		Pointer to DB_DATA_VALUE describing an existing
**				bit string;
**	dst_dv		Pointer to DB_DATA_VALUE describing memory into which
**				an integer bit count should be placed.
**
** Outputs:
**	dst_dv		Bit count is supplied.
**
** Returns:
**	DB_STATUS
**
**		Can generate E_AD1071_NOT_BIT if character other than
**			'0','1', or white space is supplied.
**			E_AD1072_STR_TO_BIT_OVFL if there are more 0's and 1's
**				than space for bits, or
**			E_AD9999_INTERNAL_ERROR in unexpected cases.
**		These are returned in the scb, as usual.
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	23-sep-1992 (fred)
**	    Created.
*/
DB_STATUS
adu_bitlength(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*src_dv,
DB_DATA_VALUE	*dst_dv)
{
    i4	blen;
    DB_STATUS	status = E_DB_OK;
    i4		overflow;

    for (;;)
    {
	switch (src_dv->db_datatype)
	{
	  case DB_BIT_TYPE:
	    blen = ((src_dv->db_length - 1) * BITSPERBYTE) + src_dv->db_prec;
	    break;
	    
	  case DB_VBIT_TYPE:
	    blen = ((DB_BIT_STRING *) src_dv->db_data)->db_b_count;
	    break;
	    
	  default:
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	if (status)
	    break;

	if (dst_dv->db_datatype != DB_INT_TYPE)
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}

	overflow = TRUE;
	switch (dst_dv->db_length)
	{
	  case 2:
	    if ((MINI2 <= blen) && (blen <= MAXI2))
	    {
		i2	v = blen;
		
		I2ASSIGN_MACRO(v, *dst_dv->db_data);
		overflow = FALSE;
	    }
	    break;

	  case 4:
	    if ((MINI4 <= blen) && (blen <= MAXI4))
	    {
		i4	v = blen;
		
		I4ASSIGN_MACRO(v, *dst_dv->db_data);
		overflow = FALSE;
	    }
	    break;

	  default:
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	if (status)
	    break;

	if (overflow)
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	break;
    }
    return(status);
}

/*{
** Name:	adu_bitsize - Provide size(bit types) function.
**
** Description:
**	This routine returns the number of bits declared for a  string.
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	src_dv		Pointer to DB_DATA_VALUE describing an existing
**				bit string;
**	dst_dv		Pointer to DB_DATA_VALUE describing memory into which
**				an integer bit count should be placed.
**
** Outputs:
**	dst_dv		Declared bit count is supplied.
**
** Returns:
**	DB_STATUS
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	08-Jan-1993 (fred)
**	    Created.
*/
DB_STATUS
adu_bitsize(ADF_CB              *adf_scb,
	    DB_DATA_VALUE	*src_dv,
	    DB_DATA_VALUE	*dst_dv)
{
    i4	blen;
    DB_STATUS	status = E_DB_OK;
    i4		overflow;

    for (;;)
    {
	switch (src_dv->db_datatype)
	{
	case DB_BIT_TYPE:
	    blen = ((src_dv->db_length - 1) * BITSPERBYTE) + src_dv->db_prec;
	    break;

	case DB_VBIT_TYPE:
	    blen = ((src_dv->db_length - 1 - DB_BCNTSIZE)
		                              * BITSPERBYTE)
		         + src_dv->db_prec;
	    break;
	    
	  default:
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	if (status)
	    break;

	if (dst_dv->db_datatype != DB_INT_TYPE)
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}

	overflow = TRUE;
	switch (dst_dv->db_length)
	{
	  case 2:
	    if ((MINI2 <= blen) && (blen <= MAXI2))
	    {
		i2	v = blen;
		
		I2ASSIGN_MACRO(v, *dst_dv->db_data);
		overflow = FALSE;
	    }
	    break;

	  case 4:
	    if ((MINI4 <= blen) && (blen <= MAXI4))
	    {
		i4	v = blen;
		
		I4ASSIGN_MACRO(v, *dst_dv->db_data);
		overflow = FALSE;
	    }
	    break;

	  default:
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	if (status)
	    break;

	if (overflow)
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	break;
    }
    return(status);
}

/*{
** Name: adu_mb_move_bits	- Move a bit string around
**
** Description:
**      This routine moves a bit string datatype (BIT or BIT VARYING) from
**	one place to another.  If necessary, the type and or length is
**	converted.
**
**	At the moment, we observe Ingres-ish semantics.  Moving a bit string
**	from a longer to a shorter will truncate the result.  At some point,
**	it may be necessary to move to ANSI SQL semantics.  These state
**	that
**	    if doing a retrieval operation,
**		if truncation would be necessary, do it but raise a warning.
**		otherwise, move and pad w/0's if necesary.
**
**	    if doing a store operation, 
**		if truncation is necessary, raise an exception (error).
**		if padding necessary, raise an exception (error).
**		otherwise, move.
**
**	In other words, for putting data into the database, all lengths must
**	match exactly.  Somewhat less than friendly, but ...
**
**	At present, we will silently pad or truncation.
**
** Inputs:
**      adf_scb                         Adf session control block
**      from_dv                         DB_DATA_VALUE of source
**      to_dv                           DB_DATA_VALUE of destination
**
** Outputs:
**      *to_dv                          Filled with requested data.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    None
**
** History:
**      29-sep-1992 (fred)
**          Created.
**	22-Oct-1992 (fred)
**     	    Altered bit pattern to match ANSI semantics.  Simplified copying
**	    of final byte substantially.
[@history_template@]...
*/
DB_STATUS
adu_mb_move_bits(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *from_dv,
DB_DATA_VALUE	    *to_dv)
{
    DB_STATUS		status = E_DB_OK;
    u_char		*from_bits;
    u_char		*to_bits;
    i4		from_count;
    i4		to_count;
    i4			move_bytes;
    i4			move_bits;

    for (;;)
    {
	switch (from_dv->db_datatype)
	{
	case DB_BIT_TYPE:
	    from_bits = (u_char *) from_dv->db_data;
	    from_count = ((from_dv->db_length - 1) * BITSPERBYTE)
				+ from_dv->db_prec;
	    break;

	case DB_VBIT_TYPE:
	    from_bits = (u_char *)
		    ((DB_BIT_STRING *) from_dv->db_data)->db_b_bits;
	    from_count = ((DB_BIT_STRING *) from_dv->db_data)->db_b_count;
	    break;
	    
	default:
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	if (status)
	    break;
	    
	switch (to_dv->db_datatype)
	{
	case DB_BIT_TYPE:
	    to_bits = (u_char *) to_dv->db_data;
	    to_count = ((to_dv->db_length - 1) * BITSPERBYTE)
				+ to_dv->db_prec;
	    break;

	case DB_VBIT_TYPE:
	    to_bits = (u_char *)
		    ((DB_BIT_STRING *) to_dv->db_data)->db_b_bits;
	    /*
	    ** Since it is only max # bits we care about here, the number
	    ** of bits is that specified in the DB_DATA_VALUE, not in the
	    ** value itself.
	    */
	    
	    to_count = ((to_dv->db_length - 1 - DB_BCNTSIZE) * BITSPERBYTE)
				+ to_dv->db_prec;
	    break;
	    
	default:
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	if (status)
	    break;

	if (to_count >= from_count)
	{
	    move_bytes = from_count / BITSPERBYTE;
	    move_bits = from_count % BITSPERBYTE;
	}
	else /* To area smaller than from ==> Truncate */
	{
	    move_bytes = to_count / BITSPERBYTE;
	    move_bits = to_count % BITSPERBYTE;
	}

	if (move_bytes)
	    MEcopy((PTR) from_bits, move_bytes, (PTR) to_bits);
	    
	if (move_bits)
	{
	    /*
	    ** In this case, then there are a few bits left to move
	    ** to the to pointer.  So far, we have moved move_bytes
	    ** bytes from from_bits to to_bits.  Thus, now, we need
	    ** to move move_bits bits from from_bits[move_bytes] to
	    ** to_bits[move_bytes].
	    **
	    ** The bits we want are the most significant move_bits bits
	    ** of from_bits[move_bytes].  Therefore, we build a mask by
	    ** creating a mask of all 1's (~0), shifting it left by the number
	    ** of bits we are interested in (thus putting 0's in those
	    ** positions), then complementing the whole whole mask.  Thus,
	    ** if we need 3 bits (our example is 8 bits per byte), then our
	    ** mask is ~(~0 >> 3), or ~(0xff >> 3), or ~0x1f, or 0xE0.
	    ** 
	    */

	    to_bits[move_bytes] = from_bits[move_bytes] & ~(~0 >> move_bits);
	}

	switch (to_dv->db_datatype)
	{
	case DB_BIT_TYPE:
	    if (to_count > from_count)
	    {
		/* Then we may need to pad some bytes with 0 */

		if (move_bits)
		{
		    move_bytes += 1;
		}
		    
		for ( /* No init */;
			    move_bytes < to_dv->db_length;
			    move_bytes++)
		{
		    to_bits[move_bytes] = '\0';
		}
	    }
	    break;

	case DB_VBIT_TYPE:
	    ((DB_BIT_STRING *) to_dv->db_data)->db_b_count =
				    (move_bytes * BITSPERBYTE) + move_bits;
	    break;
	}
	break;
    }
    return(status);
}
