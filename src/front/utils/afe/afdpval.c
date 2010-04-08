/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include       <er.h>

/**
** Name:	afdpvalue	-	Compute the display value for a type
**
** Description:
**	Contains and defines the routine(s):
**
**	afdpvalue	Computes the display value for a type.
**
** History:
**	Written 2/4/87	(dpr)
**
**	06/27/92 (dkh) - Updated code to handle changed interface for
**			 afe_ficoerce().  Change was needed to handle
**			 decimal datatypes for 6.5.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN    DB_STATUS	afe_ficoerce();
FUNC_EXTERN    STATUS		afe_clinstd();

/*{
** Name:	afe_dpvalue	-	Compute the display value for a type
**
** Description:
**	Given a ADT value, this routine computes the display value for
**	that type and places it in the passed output value.  The
**	output value must be a LONGTEXT.   If it is not big enough
**	to hold the resulting value, then the result will be truncated,
**	and the return value will be VALUE_TRUNCATED.
**
** Inputs:
**	cb			A pointer to the current ADF_CB 
**				control block.
**
**	value			A DB_DATA_VALUE for the value to
**				convert to a display value.  All fields
**				of the value must be valid.
**
**	display
**		.db_datatype	Must be DB_LTXT_TYPE.
**
**		.db_length	The length of the DB_TEXT_STRING to place
**				the display value in.
**
**		.db_data	A pointer to the DB_TEXT_STRING to place the
**				value in.
** Outputs:
**	display
**		.db_data	The DB_TEXT_STRING will be filled in with
**				the display value for `value'.
**
**		.db_length	The number of characters placed
**				in db_t_text.
**
**	Returns:
**		OK				If the display value was 
**						successfully calculated
**						and placed in display.
**
**		E_DB_WARN			The display value was
**						sucessfully calculated if
**						the error code in 
**						cb.adf_errcb.ad_errcode is
**						E_AF5000_VALUE_TRUNCATED.
**						Otherwise, some other ADF/AFE
**						warning occured.
**
**		E_DB_ERROR			Some error occured.
**
**		If OK is not returned, the error code in cb.adf_errcb.ad_errcode
**		can be checked. Possible values and their meanings are:
**
**		E_AF5000_VALUE_TRUNCATED	The display value that was
**						placed in `display' was 
**						truncated because the size
**						of the display DB_DATA_VALUE
**						was not big enough to hold the
**						value.
**
**		E_AF6004_NOT_LONGTEXT		The value's type isn't longtext.
**
**		Possible returns from:
**			afe_ficoerce
**			afe_clinstd
*/
DB_STATUS
afe_dpvalue(cb, value, display)
ADF_CB		*cb;
DB_DATA_VALUE	*value;
DB_DATA_VALUE	*display;
{
    DB_STATUS		rval;
    i4			itsnull;
    ADI_FI_ID		fid;
    i4			len;
    AFE_OPERS		ops;
    DB_DATA_VALUE	*oparr[1];
    DB_DATA_VALUE	tdbv;


    if (abs(display->db_datatype) != DB_LTXT_TYPE)
    {
    	return afe_error(cb, E_AF6004_NOT_LONGTEXT, 0);
    }

    if ((rval = adc_isnull(cb, value, &itsnull)) != OK)
    {
    	return rval;
    }
    	
    if (itsnull)
    {
    	DB_TEXT_STRING	*nlstr;

	/* get the length from the control block. */
	len = cb->adf_nullstr.nlst_length;
	nlstr = (DB_TEXT_STRING *)(display->db_data);
	nlstr->db_t_count = len;
	/*
	** copy the null string in the control block into the DB_DATA_VALUE.
	** only copy those bytes that will fit. we have to subtract the size
	** of the db_t_count field to get the number of byte allocated for the
	** null string itself. copy at least this number of byte, but set
	** the VALUE_TRUNCATED warning later.
	*/
	MEcopy( (PTR)cb->adf_nullstr.nlst_string, 
		(u_i2)(min(len,(display->db_length - sizeof(nlstr->db_t_count)))),
		(PTR)(nlstr->db_t_text) );

	len = len + sizeof(nlstr->db_t_count);
    }
    else
    {
	tdbv.db_datatype = display->db_datatype;
	tdbv.db_length = 0;
	tdbv.db_prec = 0;
	if ((rval = afe_ficoerce(cb, value, &tdbv, &fid)) != OK)
	{
	    return rval;
	}
	len = tdbv.db_length;

	ops.afe_opcnt = 1;
	oparr[0] = value;
	ops.afe_ops = oparr;

	/* go ahead and coerce the value to the display type */
	if ((rval = afe_clinstd(cb, fid, &ops, display)) != OK) 
	{
	    return rval;
	}
    }

    /*
    ** now that the value's been coerced, or loaded, check the length of the 
    ** space in display. if isn't big enough, the backend will 
    ** silently truncate the value. we'll be load about it, 
    ** and return a warning to notify our clients about the truncation.
    */
    if (display->db_length < len)
    {
        return afe_error(cb, E_AF5000_VALUE_TRUNCATED, 0);
    }


    return OK;
}
