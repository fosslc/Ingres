/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include    	<cm.h>
# include       <me.h>
# include	<sl.h>
# include	<st.h>
# include	<bt.h>      
# include	<iicommon.h>
# include	<adf.h>
# include	<ulf.h>      
# include	<adfint.h>      
# include       <aduint.h>

/**
** Name:	adubitwise.c - Logical routines manipulating byte type(s)
**
** Description:
**	This file contains the logical routines which operate on the byte datatypes.
**
** This file defines:
**
**	adu_bit_add()	Provide a bitwise ADD on the two byte operands.
**	adu_bit_and()	Provide a bitwise AND on the two byte operands.
**	adu_bit_not()	Provide a bitwise NOT on the byte operand.
**	adu_bit_or()	Provide a bitwise OR on the two byte operands.
**	adu_bit_xor()	Provide a bitwise XOR on the two byte operands.
**	adu_ipaddr()	Convert an IP address string to a Int4
**	adu_intextract() Provide an integer from the selected byte
**
** History:
**	05-oct-1998 (shero03)
**	    Created.
**	04-feb-2000 (gupsh01)
**	    Added format checking for the ipaddress in adu_ipaddr. returns 
**	    Bad_IP_ADDRESS error if illegal format is found. (bug 100265)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      31-May-2001 (hweho01)
**          Changed tptr and lptr from char pointer to uchar pointer in
**          adu_bit_add(), fix Bug #104698. 
**      24-sep-2007 (lunbr01)  bug 119125
**	    Add support for IPv6 addresses, which are in coloned-hex format,
**	    to ii_ipaddr() scalar function (C fn adu_ipaddr).
**	    Previously only handled IPv4 dotted-decimal format.
**      10-dec-2007 (lunbr01)  bug 119125
**	    Improve backward compatibility of prior IPv6 fix to ii_ipaddr()
**	    by returning varbyte (len = 4 for IPv4, 16 for IPv6) instead
**	    of fixed len=16 byte.  Also support prior "cloaked" function
**	    definition which only expects to return fixed length=4 BYTE result.
**/

/* Internal Routines */
		
/*{
** Name:	adu_bit_prepop - Prepare Byte Operands
**
** Description:
**	This routine determines which operands is shorter.
**	Then moves the shorter to the result field padded with nulls on the right
**
** Inputs:
**	dv1		Pointer to DB_DATA_VALUE describing an existing byte type
**	dv2		Pointer to DB_DATA_VALUE describing an existing byte type
**	rdv		Pointer to DB_DATA_VALUE describing memory into which
**				the resultant byte string should be put.
**
** Outputs:
**	rdv		Contains the shorter
**
** Returns:
**	The address of the longer operand.
**
** History:
**	06-oct-1998 (shero03)
**	    Created.
*/
static char *
adu_bit_prepop(
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
DB_DATA_VALUE	*rdv)
{
    char	*sptr, *lptr, *tptr;
    i4		slen, llen, tlen;
    DB_STATUS	status = E_DB_OK;

    tlen = rdv->db_length;
    tptr = rdv->db_data;

    /* Find out the shorter operand and set sptr to it, lptr to other */

    if (dv1->db_length > dv2->db_length)
    {
        slen = dv2->db_length;
        sptr = (char*)dv2->db_data;
        llen = dv1->db_length;
        lptr = (char*)dv1->db_data;
    }
    else
    {
        slen = dv1->db_length;
        sptr = (char*)dv1->db_data;
        llen = dv2->db_length;
        lptr = (char*)dv2->db_data;
    }
	
    if (tlen == llen)  /* the target is always the size of the larger operand */
    {
    	if (slen == llen)
	    MEcopy(sptr, tlen, tptr);
	else
	{
	    /* Copy the operand to the right and NULL pad */
	    i4  diflen = llen - slen;
	    MEfill(diflen, 0x00, tptr);
            MEcopy(sptr, tlen - diflen, tptr + diflen);
	}
    }
    else
    	lptr = NULL;

    return(lptr);
}

/*{
** Name:	adu_bit_add - Logically ADD two byte types
**
** Description:
**	This routine produces the logical ADD of the two byte operands.
**	If the operands are different lengths, then the shorter is padded
**	with nulls on the left.
**
** Note:  There are additional improvements that could be made later,
**	by using integer addition in i4  sized pieces.  However, using integers
**	creates Big and Little Edian concerns.  This technique is simple, 
**	straight-forward, explainable, and slow.
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	dv1		Pointer to DB_DATA_VALUE describing an existing byte type
**	dv2		Pointer to DB_DATA_VALUE describing an existing byte type
**	rdv		Pointer to DB_DATA_VALUE describing memory into which
**				the resultant byte string should be put.
**
** Outputs:
**	rdv		Byte string result is supplied.
**
** Returns:
**	DB_STATUS
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	06-oct-1998 (shero03)
**	    Created.
*/
DB_STATUS
adu_bit_add(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
DB_DATA_VALUE	*rdv)
{
    uchar *lptr, *tptr;
    i4		tlen;
    i4		i, carry;
    i4		i3;
    DB_STATUS	status = E_DB_OK;

    for (;;)
    {
	if ((dv1->db_datatype == DB_BYTE_TYPE) &&
	    (dv2->db_datatype == DB_BYTE_TYPE) &&
	    (rdv->db_datatype == DB_BYTE_TYPE))
	{}
	else
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	
        lptr = adu_bit_prepop(dv1, dv2, rdv);
	if (lptr == NULL)
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	
	tlen = rdv->db_length;
	tptr = rdv->db_data;

	/* point to the rightmost byte */
	tptr += tlen - 1;
	lptr += tlen - 1;
	for (i = tlen, carry = 0; i--; tptr--, lptr--)
	{
	    i3 = *tptr + *lptr + carry;
	    carry = (i3 > 255) ? 1 : 0;
	    *tptr = (char)i3;
	}		      

	break;
    }	/* LOOP */

    return(status);
}

/*{
** Name:	adu_bit_and - Logically AND two byte types
**
** Description:
**	This routine produces the logical AND of the two byte operands.
**	If the operands are a different length, then the shorter is padded
**	with nulls on the right.
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	dv1		Pointer to DB_DATA_VALUE describing an existing byte type
**	dv2		Pointer to DB_DATA_VALUE describing an existing byte type
**	rdv		Pointer to DB_DATA_VALUE describing memory into which
**				the resultant byte string should be put.
**
** Outputs:
**	rdv		Byte string result is supplied.
**
** Returns:
**	DB_STATUS
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	05-oct-1998 (shero03)
**	    Created.
*/
DB_STATUS
adu_bit_and(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
DB_DATA_VALUE	*rdv)
{
    char	*lptr, *tptr;
    i4		tlen;
    DB_STATUS	status = E_DB_OK;

    for (;;)
    {
	if ((dv1->db_datatype == DB_BYTE_TYPE) &&
	    (dv2->db_datatype == DB_BYTE_TYPE) &&
	    (rdv->db_datatype == DB_BYTE_TYPE))
	{}
	else
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	
        lptr = adu_bit_prepop(dv1, dv2, rdv);
	if (lptr == NULL)
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	
	tlen = rdv->db_length;
	tptr = rdv->db_data;

	BTand(tlen * BITSPERBYTE, lptr, tptr); 
	break;
    }	/* LOOP */

    return(status);
}

/*{
** Name:	adu_bit_or - Logically OR two byte types
**
** Description:
**	This routine produces the logical OR of the two byte operands.
**	If the operands are a different length, then the shorter is padded
**	with nulls on the right.
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	dv1		Pointer to DB_DATA_VALUE describing an existing byte type
**	dv2		Pointer to DB_DATA_VALUE describing an existing byte type
**	rdv		Pointer to DB_DATA_VALUE describing memory into which
**				the resultant byte string should be put.
**
** Outputs:
**	rdv		Byte string result is supplied.
**
** Returns:
**	DB_STATUS
**
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	05-oct-1998 (shero03)
**	    Created.
*/
DB_STATUS
adu_bit_or(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
DB_DATA_VALUE	*rdv)
{
    char	*lptr, *tptr;
    i4		tlen;
    DB_STATUS	status = E_DB_OK;

    for (;;)
    {
	if ((dv1->db_datatype == DB_BYTE_TYPE) &&
	    (dv2->db_datatype == DB_BYTE_TYPE) &&
	    (rdv->db_datatype == DB_BYTE_TYPE))
	{}
	else
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	
        lptr = adu_bit_prepop(dv1, dv2, rdv);
	if (lptr == NULL)
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	
	tlen = rdv->db_length;
	tptr = rdv->db_data;

	BTor(tlen * BITSPERBYTE, lptr, tptr); 
	break;
    }	/* LOOP */

    return(status);
}

/*{
** Name:	adu_bit_xor - Logically XOR two byte types
**
** Description:
**	This routine produces the logical XOR of the two byte operands.
**	If the operands are a different length, then the shorter is padded
**	with nulls on the right.
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	dv1		Pointer to DB_DATA_VALUE describing an existing byte type
**	dv2		Pointer to DB_DATA_VALUE describing an existing byte type
**	rdv		Pointer to DB_DATA_VALUE describing memory into which
**				the resultant byte string should be put.
**
** Outputs:
**	rdv		Byte string result is supplied.
**
** Returns:
**	DB_STATUS
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	05-oct-1998 (shero03)
**	    Created.
*/
DB_STATUS
adu_bit_xor(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
DB_DATA_VALUE	*rdv)
{
    char	*lptr, *tptr;
    i4		tlen;
    DB_STATUS	status = E_DB_OK;

    for (;;)
    {
	if ((dv1->db_datatype == DB_BYTE_TYPE) &&
	    (dv2->db_datatype == DB_BYTE_TYPE) &&
	    (rdv->db_datatype == DB_BYTE_TYPE))
	{}
	else
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	
        lptr = adu_bit_prepop(dv1, dv2, rdv);
	if (lptr == NULL)
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	
	tlen = rdv->db_length;
	tptr = rdv->db_data;

	BTxor(tlen * BITSPERBYTE, lptr, tptr); 
	break;
    }	/* LOOP */

    return(status);
}

/*{
** Name:	adu_bit_not - Logically NOT one byte type
**
** Description:
**	This routine produces the logical NOT of the byte operand.
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	dv1		Pointer to DB_DATA_VALUE describing an existing byte type
**	rdv		Pointer to DB_DATA_VALUE describing memory into which
**				the resultant byte string should be put.
**
** Outputs:
**	rdv		Byte string result is supplied.
**
** Returns:
**	DB_STATUS
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	05-oct-1998 (shero03)
**	    Created.
*/
DB_STATUS
adu_bit_not(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv,
DB_DATA_VALUE	*rdv)
{
    char	*lptr, *tptr;
    i4		llen, tlen;
    DB_STATUS	status = E_DB_OK;

    for (;;)
    {
	if ((dv->db_datatype == DB_BYTE_TYPE) &&
	    (rdv->db_datatype == DB_BYTE_TYPE))
	{}
	else
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	

	llen = dv->db_length;
	lptr = (char*)dv->db_data;
	tlen = rdv->db_length;
	tptr = rdv->db_data;

	if (tlen != llen)
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}

	MEcopy(lptr, llen, tptr);
	BTnot(tlen * BITSPERBYTE, tptr); 
		    	      
	break;
    }	/* LOOP */

    return(status);
}

/*{
** Name:	adu_ipaddr - Convert an IP address (string) to a byte 
**
** Description:
**	This routine converts a character IP address to a binary form
**      The input format is dependent on the IP address type: IPv4 is
**      in dotted-decimal format while IPv6 is in coloned-hex format
**      and is 4 times as long (16 versus 4 bytes).
**
**      Each set of character digits is converted to byte data based on 
**      the IP address type (4 or 6) as follows:
**         IP@                               OUTBYTES        OUTBYTES
**        TYPE   SEPARATOR     DIGIT TYPE    PER SET  #SETS   TOTAL
**        IPv4   period(.)       decimal        1       4       4
**        IPv6   colon(:)      hexadecimal      2       8      16
**
**      The result is returned as VBYTE with the length=4 for IPv4 addresses
**      and the length=16 for IPv6 addresses.  NOTE that prior to IPv6, this
**      routine returned only 4 bytes of type BYTE (not VBYTE); this is still
**      supported for "cloaked" fi_defn for backward compatibility (old format
**      doesn't support IPv6 addresses and will generate an error for them).
**      
**      EXAMPLES - 
**        IPv4 addresses:
**         "141.202.36.10"              => x'8DCA240A'
**         "127.0.0.1"                  => x'7F000001'
**         "2.1"                        => x'02000001'
**        IPv6 addresses:
**         "fe80::208:74ff:fef0:42b3"   => x'FE80000000000000020874FFFEF042B3'
**         "fe80::208:74ff:fef0:42b3%4" => x'FE80000000000000020874FFFEF042B3'
**         "::1"                        => x'00000000000000000000000000000001'
**        IPv4-mapped IPv6 address:
**         "::FFFF:141.202.36.10"       => x'00000000000000000000FFFF8DCA240A'
**
**      If the input IP address is invalid, an error status will be
**      returned.  For example, IPv4 addresses with any number > 255
**      would be invalid.
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	dv 		Pointer to DB_DATA_VALUE describing an IP address
**	rdv		Pointer to DB_DATA_VALUE describing memory into which
**				the resultant byte string should be put.
**
** Outputs:
**	rdv		Byte string result is supplied. For new fi_defn,
**				return VBYTE (len=4 for IPv4 and len=16
**				for IPv6). For old fi_defn, return BYTE
**				(len=4, IPv6 addresses not supported).
**
** Returns:
**	DB_STATUS
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	07-oct-1998 (shero03)
**	    Created.
**      04-feb-2000 (gupsh01)
**          Added format checking for the ipaddress in adu_ipaddr. returns
**          Bad_IP_ADDRESS error if illegal format is found. (bug 100265)
**      24-sep-2007 (lunbr01)  bug 119125
**          Add support for IPv6 addresses, which are in coloned-hex format.
**          Previously only handled IPv4 dotted-decimal format.
**      06-dec-2007 (lunbr01)  bug 119125
**	    Improve backward compatibility of prior IPv6 fix to ii_ipaddr()
**	    by returning varbyte (len = 4 for IPv4, 16 for IPv6) instead
**	    of fixed len=16 byte (but ck for latter due to old apps).
**	    Also support old "cloaked" defn that returns fixed-len(4) BYTE
**	    and hence only supports IPv4 addresses.
*/
DB_STATUS
adu_ipaddr(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv,
DB_DATA_VALUE	*rdv)
{
    DB_STATUS           db_stat;
    i4			i;
    i4			i_start_ipv4_addr = 0;
    i4			lensrc;
    unsigned char	ipbyte;
    u_i2		ipnum;
    char		*srcptr, *destptr, *endsrc;
    char		*sepptr;	/* ptr to IP@ separator (. or :) */
    char		*srcptr_last_colon = NULL;
    char		*dbl_colon_destptr = NULL;
    char		iptype, iptype_mapped;	/* IP address type */
# define ADU_IPV4_TYPE_ADDR '4'
# define ADU_IPV6_TYPE_ADDR '6'
    i4  		lendest;	/* Length of byte output expected */
    i4  	  	shift_right;	/* #bytes to shift right for dbl colon*/
    u_i1		numbase;	/* Base (10 or 16) of digits */
    u_i2		nummax;		/* Max value of digit "set" */
    u_i2		numlen;		/* Length of digit "set" out byte(s) */
    u_i2		num_colons = 0; /* Number of colons found */
    char		sepchar;	/* Separator (. or :) for IP@ */
    bool		previous_char_was_colon = FALSE;

    if (((rdv->db_datatype == DB_BYTE_TYPE) &&
    	 (rdv->db_length == AD_LEN_IPV4ADDR)) ||
        ((rdv->db_datatype == DB_VBYTE_TYPE) &&
    	 (rdv->db_length == AD_LEN_IPADDR + DB_CNTSIZE)) )
    {}
    else
        return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    
    if ((dv->db_datatype == DB_CHR_TYPE) ||
    	(dv->db_datatype == DB_CHA_TYPE) ||
	(dv->db_datatype == DB_TXT_TYPE) ||
	(dv->db_datatype == DB_VCH_TYPE))
    {}
    else
        return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    	
    if (db_stat = adu_lenaddr(adf_scb, dv, &lensrc, &srcptr))
	return (db_stat);
    if (db_stat = adu_lenaddr(adf_scb, rdv, &lendest, &destptr))
	return (db_stat);

    endsrc = srcptr + lensrc;

    /*
    ** Check if IPv6 format (coloned-hex, such as "fe80::208:74ff:fef0:42b3")
    **    or if IPv4 format (dotted-decimal, such as "141.202.36.10")
    ** Since IPv6 addresses might contain IPv4 @ mapped within, check for
    ** colon, not period, to determine type.
    */
    if ((sepptr = STindex (srcptr, ":", lensrc)) == NULL)
    {
        iptype  = ADU_IPV4_TYPE_ADDR;
        numbase = 10;
        nummax  = 255;
        numlen  = 1;
        sepchar = '.';
        lendest = AD_LEN_IPV4ADDR;
    }
    else
    {
	if (rdv->db_length < AD_LEN_IPV6ADDR) /* ck for old IPv4-only fi_defn*/
            return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
        iptype  = ADU_IPV6_TYPE_ADDR;
        numbase = 16;
        nummax  = 0xffff;
        numlen  = 2;
        sepchar = ':';
        lendest = AD_LEN_IPV6ADDR;
    }
    iptype_mapped = iptype;

    MEfill (lendest, 0, destptr);  /* init with zeroes */

    if (rdv->db_datatype == DB_VBYTE_TYPE)  /* update vbyte len prefix */
        ((DB_TEXT_STRING *)rdv->db_data)->db_t_count = lendest;
    
    for (i = ipnum = 0; (srcptr < endsrc) && (i < AD_LEN_IPADDR); CMnext(srcptr))
    {
    	if (CMhex(srcptr)) 
	{	
	    previous_char_was_colon = FALSE;
    	    if (CMdigit(srcptr)) 
	        ipbyte = *srcptr - '0';
	    else
	    {
	        if (iptype_mapped == ADU_IPV4_TYPE_ADDR)
		    return (adu_error(adf_scb, E_AD5008_BAD_IP_ADDRESS, 0)); 
	        CMtolower(srcptr, srcptr);
	        ipbyte = *srcptr - 'a' + 10;
	    }

	    if (ipnum == 0)
	    {
	        ipnum = ipbyte;
	    }
	    else
	    {   
		if (((ipnum * numbase) + ipbyte) > nummax )
		    return (adu_error(adf_scb, E_AD5008_BAD_IP_ADDRESS, 0)); 
	        ipnum = (ipnum * numbase) + ipbyte;
	    }
	}
	else if (CMoper(srcptr))
	{	
	    if (*srcptr == sepchar)
	    {	
		if (sepchar == ':') 
		{
		    num_colons++;
		    srcptr_last_colon = srcptr;
		    if (previous_char_was_colon)
		        if (dbl_colon_destptr == NULL)
		            dbl_colon_destptr = destptr;
		        else   /* Only one occurrence of double colon allowed */
		            return (adu_error(adf_scb, E_AD5008_BAD_IP_ADDRESS, 0));
		    previous_char_was_colon = TRUE;
		    *destptr = ipnum / 256;
	            destptr++; i++;
		    if (i >= AD_LEN_IPADDR)
		        return (adu_error(adf_scb, E_AD5008_BAD_IP_ADDRESS, 0));
		}
		else  /* It must be a period (.) */
		{
		    previous_char_was_colon = FALSE;
		}
		*destptr = ipnum % 256;
	        destptr++; i++;
		ipnum = 0;
	    }
	    else  /* It wasn't the expected separator character */
	    {
		/*
		** Handle Special Cases
		*/
		if ((iptype == ADU_IPV6_TYPE_ADDR) && (num_colons > 1))
		{
		    if (*srcptr == '.')
		    {
		        /*
		        ** This is an IPv4-mapped IPv6 address, such as:
		        **   "::FFFF:141.202.36.10"
		        ** So now switch to IPv4 mode!!
		        ** This involves backing up in srcptr to reprocess
		        ** last digit set as decimal rather than hex.
		        */ 
		        iptype_mapped = ADU_IPV4_TYPE_ADDR;
        	        numbase = 10;
        	        nummax  = 255;
		        sepchar = '.'; 
		        srcptr = srcptr_last_colon;
		        ipnum = 0;
		        i_start_ipv4_addr = i; 
		        continue;
		    }
		    else
		    if (*srcptr == '%')
		    {
		        /*
		        ** This may be an interface identifier suffix for 
		        ** link-local and site-local IPv6 addresses.  E.g.,
		        **   "fe80::208:74ff:fef1:42c4%4"
		        ** This suffix can be ignored; it can only be followed
		        ** by decimal digits (usually 1-9).
		        */ 
		        for (CMnext(srcptr); srcptr < endsrc ; CMnext(srcptr))
		        {
		            if (!CMdigit(srcptr))
		                return (adu_error(adf_scb, E_AD5008_BAD_IP_ADDRESS, 0));
		        }
		        break;
		    }
		    return (adu_error(adf_scb, E_AD5008_BAD_IP_ADDRESS, 0));
		} /* end if IPv6 special cases */

		else /* not a special case, so it's an error */
		    return (adu_error(adf_scb, E_AD5008_BAD_IP_ADDRESS, 0));
	    } /* end else wasn't expected separator character */
        }
	else
	    return (adu_error(adf_scb, E_AD5008_BAD_IP_ADDRESS, 0)); 
    }  /* end for */

    /*
    ** If we exited the loop due to reaching end of the source,
    ** then there is potentially digits processed that haven't
    ** yet been moved to "dest".  Move them over.
    */
    if (i < AD_LEN_IPADDR)
    {
	if (iptype_mapped == ADU_IPV6_TYPE_ADDR)
	{
	    *destptr = ipnum / 256;
	    destptr++; i++;
	    if (i >= AD_LEN_IPADDR)
	 	return (adu_error(adf_scb, E_AD5008_BAD_IP_ADDRESS, 0));    
	}
	*destptr = ipnum % 256;
	destptr++; i++;
    }

    /*
    ** For IPv4 addresses, if fewer than 4 sets of decimal digits
    ** separated by periods were found in the input, then the last
    ** period acts as a zero-value insertion wildcard.  For instance,
    **    2.1 ==> 2.0.0.1
    **    2.1.5 ==> 2.1.0.5
    ** Because the output area (destptr) was zero-filled, all we need
    ** to do at this point if we encounter one of these "short" IPv4
    ** addresses, is move the last digit processed to the end and 
    ** replace it with zero.
    ** NOTE: Short IPv4 @ cannot be at the end of an IPv6 @ ... ie, an 
    ** IPv4-mapped IPv6 address, such as:
    **     ::FFFF:141.10
    ** IPv6 considers this an invalid address.
    */
    if ((iptype_mapped == ADU_IPV4_TYPE_ADDR) &&
        ((i - i_start_ipv4_addr) < AD_LEN_IPV4ADDR))
    {
	if (iptype == ADU_IPV6_TYPE_ADDR)
	    return (adu_error(adf_scb, E_AD5008_BAD_IP_ADDRESS, 0));    
	shift_right = (AD_LEN_IPV4ADDR - i);
	destptr--;   /* Back up to last digit */
	*(destptr + shift_right) = *destptr;
	*destptr = 0;
	i       += shift_right;
	destptr += shift_right;
    }

    /*
    ** For IPv6 addresses, if a double-colon that represented more
    ** than 1 consecutive digit sets of 0, then shift dest to the 
    ** right starting at the position corresponding to the double
    ** colon.  Start at back and work forward to avoid overlap problems
    ** until the end of the double colon position (dbl_colon_destptr + 2)
    ** is reached (no need to move the x'0000' that represented the
    ** double colon).
    */
    if ((iptype == ADU_IPV6_TYPE_ADDR) &&
        (dbl_colon_destptr != NULL) &&
        (i < AD_LEN_IPV6ADDR))
    {
	destptr--;   /* Back up to last digit */
	for (shift_right = (AD_LEN_IPV6ADDR - i); destptr >= (dbl_colon_destptr + 2); *destptr--)
	{
	    *(destptr + shift_right) = *destptr;
	    *destptr = 0;
	}
	i       += shift_right;
	destptr += shift_right;
    }

    /*
    ** Check to make sure our output lengths came out correctly.
    ** Also make sure we processed all the input source...if not,
    ** that's an error too because there was a longer IP@ than allowed.
    */
    if ((i != lendest) || (srcptr < endsrc))
	 return (adu_error(adf_scb, E_AD5008_BAD_IP_ADDRESS, 0));    
    return (E_DB_OK);
}

/*{
** Name:	adu_intextract - Extract an integer from a byte string 
**
** Description:
**	This routine picks a specified byte from a byte string 
**	and returns it as an integer.  If the index is out of range, then 
**	a 0 is returned.  This routine returns a 2 byte integer. 
**
** Re-entrancy:
**      This function is re-entrant.
**
** Inputs:
**	adf_scb		Pointer to ADF session control block
**	dv1 		Pointer to DB_DATA_VALUE describing an IP address
**	dv2 		Pointer to DB_DATA_VALUE describing an IP address
**	rdv		Pointer to DB_DATA_VALUE describing memory into which
**				the resultant integer should be put.
**
** Outputs:
**	rdv		Byte string result is supplied.
**
** Returns:
**	DB_STATUS
**
** Exceptions: None.
**
** Side Effects: None.
**
** History:
**	09-oct-1998 (shero03)
**	    Created.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
**	04-Jun-2010 (kiria01) b123879 i144946
**	    Don't return unnecessary errors for i8 values that are outside of
**	    i4 range when later logic would have adjusted the parameter anyway.
*/
DB_STATUS
adu_intextract(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dvbyte,
DB_DATA_VALUE	*dvint,
DB_DATA_VALUE	*rdv)
{
    DB_STATUS           db_stat;
    unsigned char	*s;
    i4		    	slen, n, r;
    	
    if ((db_stat = adu_3straddr(adf_scb, dvbyte, (char**)&s)) != E_DB_OK)
	return (db_stat);
    if ((db_stat = adu_5strcount(adf_scb, dvbyte, &slen)) != E_DB_OK)
	return (db_stat);

    switch (dvint->db_length)
    {
    	case 1:
    	  n = I1_CHECK_MACRO(*(i1*)dvint->db_data);
	  break;
    	case 2:
    	  n = *(i2*)dvint->db_data;
	  break;
 	case 4:
    	  n = *(i4*)dvint->db_data;
	  break;
    	case 8:
	{
	  i8 i8n = *(i8 *)dvint->db_data;

	  /* limit to i4 values - don't return error though as outrange should return
	  ** 0 anyway regardless. */
	  if (i8n > MAXI4)
		i8n = MAXI4;
	  else if (i8n < MINI4LL)
		i8n = MINI4LL;	
	  n = (i4) i8n;
	  break;
	}
	default:
	  return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "intextract count length"));
    }

    if (n < 0 || n > slen)
    	r = 0;	
    else
    	r = s[n-1];

    *(i2*)rdv->db_data = r;

    return (E_DB_OK);
}
