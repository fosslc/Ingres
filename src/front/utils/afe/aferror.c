/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include 	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>

/**
** Name:	aferror.c -	Sets up an ADF_CB's error block.
**
** Description:
**	This file contains and defines:
**
** 	afe_error - Sets up an ADF_CB's error block.
**
** History:
**	Revision 6.1  89/02  wong
**	Added parameter count argument to 'ERlookup()' call.
**
**	Revision 6.0  87/02/04  daver
**	Initial revision.
**	6/25/87	 changed pointer parameters' types from pointer to
**		i4 to generic PTR (danielt)
**
**	06/07/93 (dkh) - Removed ifdef TERMINATOR constructs since it
**			 is no longer needed and NO longer defined with
**			 the demise of dbms.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	afe_error() -	Sets up an ADF_CB's error block.
**
** Description:
**	Given an ADF_CB, afe/fmt error number, parameter count, and parameters,
**	afe_error will build an error message, and load it into the ADF_CB's
**	error block. It translates the error number to a DB_STATUS code to
**	be returned. 
**		Note, the return status in this procedure is overloaded.
**	It is intended to map an AFE error code to the appropriate DB_STATUS
**	error code.  However, if an error occurs in executing this routine,
**	it will be signaled by returning an E_DB_ERROR code.  This case
**	can be determined by looking at the contents of the ad_errcode field
**	in the ADF error block, adf_errcb.
**	  This routine will also format ADF warning messages.
**
**
**
** Inputs:
**	cb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.  If 0, no
**					error message will be formatted.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.  If
**					NULL, no error message will be
**					formatted.
**      errnum	                        The AFE error code to format a message
**					for and translate to a DB_STATUS code.
**	parcount			This is a count of the optional
**					parameters which should be formated
**					into the error message.  If no
**					parameters are necessary, this should
**					be zero.
**	[p1 ... p6]			Optional parameter pairs.  Optional
**					parameters should come in pairs where
**					the first member designates the second
**					values' length as specified by the
**					ERlookup() protocol and the second
**					member should be the address of the
**					value to be interpreted.
**
** Outputs:
**	cb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		AFE error code for the error.
**		.ad_errclass		Signifies the AFE error class.
**		.ad_usererr		If .ad_errclass is AFE_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an AFE error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**
** Returns:
**	{DB_STATUS}      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field cb.adf_errcb.ad_errcode to determine
**	    the AFE error code.  The following is a list of possible AFE error
**	    codes that can be returned by this routine:
**
**	    OK				If the AFE error was actually OK.
**
**	    E_AD1020_BAD_ERROR_LOOKUP	If ERlookup could not find the error
**					code that was passed to it in the
**					error message file.
**
**	    E_AF6014_BAD_ERROR_PARAMS	If the too many parameter arguments
**					are passed in or an odd number of
**					parameter arguments is passed in.
**
**
** History:
**	stolen from adu_error, 1/26/87 (dpr)
*/
/*VARARGS4*/
DB_STATUS
afe_error ( cb, errnum, parcount, p1, p2, p3, p4, p5, p6 )
ADF_CB		*cb;
ER_MSGID	errnum;
i4		parcount;
i4		p1;		/* grant wants a i4, so he gets a i4  */
PTR		p2;
i4		p3;
PTR		p4;
i4		p5;
PTR		p6;
{
	register ADF_ERROR	*err;
	DB_STATUS		ret_status = E_DB_OK;
	i4			final_msglen = 0;
	ER_MSGID		errtype;
	CL_SYS_ERR		sys_err;
	ER_ARGUMENT		loc_param;
	ER_ARGUMENT		params[3];	/* 3 is the max we can have */

	err = &cb->adf_errcb;
	err->ad_errcode = errnum;

	/* Check for bad parameters being passed to this routine */

	if ( (parcount % 2) == 1 || parcount > MAX_ERR_PARAMS )
	{
		err->ad_errclass = AFE_INTERNAL_ERROR;
		err->ad_errcode	= E_AF6014_BAD_ERROR_PARAMS;
		errnum = E_AF6014_BAD_ERROR_PARAMS;
	}

	if (parcount > 0)
	{
		params[0].er_size   = p1;
		params[0].er_value  = p2;
		if (parcount > 2)
		{
			params[1].er_size   = p3;
			params[1].er_value  = p4;
			if (parcount > 4)
			{
				params[2].er_size   = p5;
				params[2].er_value  = p6;
			}
		}
	}

	/* Set the DB_STATUS error code */

	errtype = errnum & AFE_FMT_MASK; /* make this facility independent*/
	if ( errtype < AFE_INTERNAL_BOUNDARY )
	{
		err->ad_errclass = AFE_USER_ERROR;
	}
	else 
	{
		err->ad_errclass = AFE_INTERNAL_ERROR;
		/* reduce error numbers to 0 - 3FFF range */
		errtype -= AFE_INTERNAL_BOUNDARY;
	}

	if ( errtype < AFE_WARN_BOUNDARY )
		ret_status = E_DB_OK;
	else if ( errtype < AFE_ERROR_BOUNDARY )
		ret_status = E_DB_WARN;
	else if ( errtype < AFE_FATAL_BOUNDARY )
		ret_status = E_DB_ERROR;
	else /* an unknown error is also considered fatal */
		ret_status = E_DB_FATAL;

	/* If no error message formating */
    
	if ( err->ad_errmsgp == NULL || err->ad_ebuflen == 0 )
		return ret_status;

	/* Format the error message. */

	if ( ERlookup( errnum, (CL_SYS_ERR *)NULL, 0, (i4 *)NULL,
				err->ad_errmsgp, err->ad_ebuflen,
				cb->adf_slang, &final_msglen, &sys_err,
				parcount / 2,
				params
		) != E_DB_OK )
	{
		/* Start processing an error for a bad 'ERlookup()' call rather
		** than the original error that was passed in to this routine.
		*/
		err->ad_errcode	= E_AD1020_BAD_ERROR_LOOKUP;
		err->ad_errclass = AFE_INTERNAL_ERROR;

		/* It is assumed that the following call will not produce an
		** error.  It had better be tested.
		*/
		loc_param.er_size   = 4;
		loc_param.er_value  = (PTR)&errnum;
		_VOID_ ERlookup( E_AD1020_BAD_ERROR_LOOKUP, (CL_SYS_ERR *)NULL,
			0, (i4 *)NULL, err->ad_errmsgp, err->ad_ebuflen,
			cb->adf_slang, &final_msglen, &sys_err,
			1,
			&loc_param
		);
		ret_status = E_DB_ERROR;
	}

	err->ad_emsglen = final_msglen;

	return ret_status;
}
