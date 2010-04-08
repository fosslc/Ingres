/*
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
# include       <er.h>

/**
** Name:	afagfind.c -	Front-End ADF Find Aggregate Descriptor Module.
**
** Description:
**	Contains the routines used to set up an aggregate workspace structure
**	from the aggregate name or operator ID and initial value.  Defines:
**
** 	IIAFagFind()	set-up aggregate workspace and result.
** 	afe_agfind()	utility routine to find aggregrate desc.
**
** History:
**	Written 2/6/87 (dpr)
**	6/25/87	(danielt) cast NULL parameter to afe_fdesc() to proper type
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-oct-2002 (drivi01) Fixed assignment of workspace size from a 
**	    function descriptor to make sure that workspace of size 0
**	    doesn't get assigned to fdesc.adi_lenspec.adi_lncompute.
**	    Also changed the return value of IIAFagFind() from OK constant
**	    to rval, in case if afe_fdesc fails, and no function descriptor
**	    returned.
**	05-nov-2002 (drivi01) Corrected change #460135 to compile on unix.
**	    Changed code so that max function takes two parameters instead
**	    of three.
**/

DB_STATUS	afe_opid();
DB_STATUS	afe_fdesc();

/*{
** Name:	IIAFagFind() -	Set-up Aggregate Workspace and Result.
**
** Description:
**	Given an aggregate function operator ID and the initial value for the
**	aggregate, find the function instance descriptor and set-up the ADF
**	aggregate structure with it's function instance ID and required work-
**	space size.  Also, set the type and length of the result.
**
**	This information should be used to allocate a result area and a work-
**	space prior to computing an aggregate while the ADF aggregate structure,
**	itself, can be passed to 'adf_agbegin()', 'adf_agnext()' and
**	'adf_agend()' when computing the aggregate.
**
** Inputs:
**	cb	{ADF_CB *}  The ADF control block.
**	agg_opid {ADI_OP_ID}  The ADF operator ID for the aggregate function.
**	input	{DB_DATA_VALUE *}  The initial aggregate data value.
**	aggdesc	{ADF_AG_STRUCT *}  The ADF aggregate workspace structure.
**	result	{DB_DATA_VALUE *}  The aggregate result data value.
**
** Outputs:
**	aggdesc	{ADF_AG_STRUCT *}  The ADF aggregate workspace structure.
**			.adf_agfi		Aggregate funciton instance ID.
**			.adf_agwork.db_length	Aggregate workspace size.
**	result	{DB_DATA_VALUE *}  The aggregate result data value.
**		.db_datatype	Set to the datatype of the aggregate's result.
**		.db_length	Set to the length of the aggregate's result.
**
** Returns:
**	{DB_STATUS}  OK
**	    	     E_AF601A_NOT_AGG_FUNC	Operator ID is not an aggregate
**						function ID.
**		returns from: 'afe_fdesc()'
** History:
**	04-jun-1994 (jhw) Abstracted from 'afe_agfind()':
**		Written 2/6/87 (dpr)
**		6/25/87	(danielt) cast NULL parameter to afe_fdesc().
*/
DB_STATUS
IIAFagFind ( cb, agg_opid, input, aggdesc, result )
ADF_CB		*cb;
ADI_OP_ID	agg_opid;
DB_DATA_VALUE	*input;
ADF_AG_STRUCT	*aggdesc;
DB_DATA_VALUE	*result;
{
	STATUS		rval;
	ADI_FI_DESC	fdesc;
	AFE_OPERS	ops;
	DB_DATA_VALUE	*oparr[1];

	/*
	** Load the 'ops' structure with the aggregate's operand datatype.
	** Call 'afe_fdesc()' to get the function descriptors for this aggregate
	** 	using this datatype. Passing NULL is a "stub", as 'afe_fdesc()'
	**	will eventually tell whether one or more of the operands need
	**	to be coerced.
	** Load the function ID and workspace lengths from the function 
	**	descriptor, above, into our params to be passed back.
	** As a side-effect of 'afe_fdesc()', the aggregate's result datatype
	** and length will be set.
	*/
	ops.afe_opcnt = 1;
	ops.afe_ops = oparr;
	oparr[0] = input;
	rval=OK;
	if ( (rval = afe_fdesc(cb, agg_opid, &ops, (AFE_OPERS *)NULL,
					result, &fdesc)) == OK )
	{
		if ( fdesc.adi_fitype != ADI_AGG_FUNC )
		{ /* not aggregate function! */
			ADI_OP_NAME	aggname;

			_VOID_ adi_opname(cb, agg_opid, &aggname);
    			return afe_error( cb, E_AF601A_NOT_AGG_FUNC,
						1, aggname.adi_opname
			);
		}
		/* now that we have the function descriptor, get the function
		** instance ID and required workspace size from it.
		*/
		aggdesc->adf_agfi = fdesc.adi_finstid;
		if (fdesc.adi_lenspec.adi_lncompute!=ADI_FIXED ||
			fdesc.adi_lenspec.adi_lncompute!=ADI_LEN_INDIRECT)
		{
			aggdesc->adf_agwork.db_length = max(fdesc.adi_agwsdv_len, 
				max(input->db_length, result->db_length));
		}
		else
		{
			aggdesc->adf_agwork.db_length=max(fdesc.adi_agwsdv_len,
				fdesc.adi_lenspec.adi_fixedsize);
		}

	}

	return rval;
}

/*{
** Name:	afe_agfind	-	Utility routine to find aggregrate desc.
**
** Description:
**	Given an aggregate name and an argument for the aggregate,
**	this routine finds the aggregate's function instance id, and
**	returns the type and length of the result, plus the size of the
**	aggregate workspace needed.
**
**	This information can be used to allocate a result area and
**	a workspace prior to computing an aggregate.
**
** Inputs:
**	cb			A pointer to the current ADF_CB 
**				control block.
**
**	agname			A NULL terminated string containing the
**				aggregate name.
**
**	dbv
**		.db_datatype	The datatype of the argument to the aggregate.
**
**		.db_length	The length of the argument.
**
**	finstance		Must point to an allocated ADI_FI_ID.
**
**	result			Must point to an allocated DB_DATA_VALUE.
**
**	workspace		Must point to a nat.
**
** Outputs:
**	finstance		Set to the function instance id of the 
**				aggregate. This will be used in later calls to:
**
**				afe_agbegin
**				afe_agnext
**				afe_agend
**
**	result
**		.db_datatype	Set to the datatype of the aggregate's result.
**
**		.db_length	Set to the length of the aggregate's result.
**
**	workspace		Set to the size of the workspace needed to
**				compute this aggregate.
** Returns:
**	{DB_STATUS}  OK
**			returns from:
**				afe_opid()
**				IIAFagFind()
** History:
**	Written 2/6/87 (dpr)
**	6/25/87	(danielt) cast NULL parameter to afe_fdesc() to proper type
**	04-jun-1991 (jhw) Abstracted main functionality into 'IIAFagFind()'.
*/
DB_STATUS
afe_agfind ( cb, agname, dbv, finstance, result, workspace )
ADF_CB		*cb;
char		*agname;
DB_DATA_VALUE	*dbv;
ADI_FI_ID	*finstance;
DB_DATA_VALUE	*result;
i4		*workspace;
{
	ADI_OP_ID	opid;
	DB_STATUS	rval;
	ADF_AG_STRUCT	agg;

	/*
	** First, get the opid corresponding to the aggregate name passed in.
	** Set-up an aggregate workspace using the operator ID.
	** Load the function ID and workspace lengths from the aggregate
	**	workspace, above, into our params to be passed back.
	** As a side-effect of 'IIAFagFind()', the aggregate's result datatype
	** and length will be set.
	*/
	if ( (rval = afe_opid(cb, agname, &opid)) != OK ||
			(rval = IIAFagFind(cb, opid, dbv, &agg, result)) != OK )
	{
		return rval;
	}
	/* now that we have the aggregate workspace,
	** get the finstid and workspace from it
	*/
	*finstance = agg.adf_agfi;
	*workspace = agg.adf_agwork.db_length;

	return OK;
}
