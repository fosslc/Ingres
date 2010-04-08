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
** Name:	afficoerce.c -	Get function instance ID and result length
**					for a coercion.
**
** Description:
**	This file contains and defines:
**
** 	afe_ficoerce - Get function instance ID and result length 
**				for a coercion.
**
** History:
**	Revision 6.0  87/01/27  daver
**	Initial revision.
**	22-aug-1990	Modified to call adi_0calclen routine rather than
**			obsolete adi_calclen. (teresal)
**	06/27/92 (dkh) - Changed the interface to pass the db_length and
**			 db_prec in the to_type DBV parameter.  This is
**			 needed to pick up precision/scale info for
**			 decimal datatypes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

DB_STATUS    adi_ficoerce();
DB_STATUS    adi_fidesc();
DB_STATUS    adi_0calclen();

/*{
** Name:	afe_ficoerce() -	Get Function Instance ID and
**						Result Length for a Coercion.
** Description:
**	Given two types to coerce, afe_ficoerce returns the function 
**	instance ID of the desired coercion function. It also returns the
**	length of the resulting (destination) datatype. With this information,
**	frontend programs can call afe_clinstd (which takes a function
**	instance ID and result DB_DATA_VALUE) to perform the coercion.
**
** Inputs:
**	cb		pointer to an ADF_CB.
**	
**	fromtype	the DB_DATA_VALUE to be coerced.
**
**	totype		the DB_DATA_VALUE fromtype is to be coerced into.
**			
**	fid		pointer to an ADI_FI_ID.
**
**	length		pointer to a i4 to hold the resulting length.
**
** Outputs:
**	fid		the function instance ID of the desired coercion.
**
**	totype
**	  .db_length	length of the resulting DB_DATA_VALUE totype.
**	  .db_prec	precision/scale of the result DB_DATA_VALUE totype.
**
**	Returns:
**		OK
**		returns from:
**    		    adi_ficoerce
**    		    adi_fidesc
**    		    adi_0calclen
**
** History:
**	Written 1/27/87 (dpr)
**	06/07/1999 (shero03)
**	    Support new parmater list for adi_0calclen.
**	6-Dec-2007 (kibro01) b119574
**	    Copy entire DB_DATA_VALUE structure so we don't end up in 
**	    adi_0calclen using the uninitialised db_length value of tdbv
**	    (which happens when the lenspec value is ADI_RESLEN).  When
**	    uninitialised the value can either be -1 (ADE_UNKNOWN_LEN) in
**	    which case you get E_AD2022 back, or it could be a valid length
**	    but too short, in which case you can get data truncation.
*/
DB_STATUS
afe_ficoerce ( cb, fromtype, totype, fid )
ADF_CB	    	*cb;
DB_DATA_VALUE 	*fromtype;
DB_DATA_VALUE 	*totype;
ADI_FI_ID   	*fid;   
{
	DB_STATUS	rval;
	ADI_FI_DESC	*fptr;
	ADI_FI_DESC	fdesc;
	DB_DATA_VALUE	tdbv;
	DB_DATA_VALUE	*opptrs[ADI_MAX_OPERANDS];

	fptr = &fdesc;
	tdbv = *totype;
	opptrs[0] = fromtype;
	if ( (rval = adi_ficoerce(cb, fromtype->db_datatype,
				totype->db_datatype, fid)) != OK  ||
		(rval = adi_fidesc(cb, *fid, &fptr)) != OK  ||
			(rval = adi_0calclen( cb, &(fptr->adi_lenspec),
				1, opptrs, &tdbv)) != OK )
	{
		return rval;
	}
	totype->db_length = tdbv.db_length;
	totype->db_prec = tdbv.db_prec;
	return OK;
}
