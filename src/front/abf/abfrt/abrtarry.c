/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>
#include	<abfcnsts.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	"rts.h"
#include	"keyparam.h"

/**
** Name:	abrtarry.c -   ABF Run-time System Array Method Routines.
**
** Description:
**	Contains the routines that implement the built-in array procedures that
**	are the "methods" for array objects.  Defines:
**
**	IIARaxClear()		Clear an Array.
**	IIARaxAllRows()		Return Total Array Size.
**	IIARaxLastRow()		Return Non-Deleted Array Size.
**	IIARaxFirstRow()	Return Index of First Record.
**	IIARaxInsertRow()	Insert Record into an Array.
**	IIARaxSetRowDeleted()	Delete a Record From an Array.
**	IIARaxRemoveRow()	Remove Record from an Array.
**
** History:
**	Revision 6.3/03/00  90/08  wong
**	Initial revision.
**
**	01-nov-93 (connie) Bug #40650
**		In getPrm, fix segmentation fault by testing fp for NULL
**		before any reference of *fp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static DB_DATA_VALUE	*getPrm();

/*{
** Name:	IIARaxClear() -	Clear an Array.
**
** Description:
**	Removes all the records in the array.
**
** Inputs:
**	prm	{ABRTSPRM *}  The parameter structure for the call.
**	name	{char *}  Procedure name
**	proc	{ABRTSPRO *}  The procedure object.
**
** Outputs:
**	prm	{ABRTSPRM *}
**			.pr_retvalue	{i4 * ==> DB_DATA_VALUE}
**						an integer STATUS value.
** History:
**	08/90 (jhw) -- Written.
*/
PTR
IIARaxClear ( prm, name, proc )
ABRTSPRM	*prm;
char		*name;
ABRTSPRO	*proc;
{
	STATUS		status = FAIL;
	DB_DATA_VALUE	*adbv;
	DB_DATA_VALUE	retvalue;

	if ( (adbv = getPrm(prm, name, (i4 *)NULL)) != NULL )
		status = iiarArClearArray(adbv);

	retvalue.db_data = (PTR)&status;
	retvalue.db_datatype = DB_INT_TYPE;
	retvalue.db_length = sizeof(status);
	IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );
	return NULL;
}

/*{
** Name:	IIARaxAllRows() -	Return Total Array Size.
**
** Description:
**	Returns a count of the total number of records in an array, both
**	deleted and otherwise.
**
** Inputs:
**	prm	{ABRTSPRM *}  The parameter structure for the call.
**	name	{char *}  Procedure name
**	proc	{ABRTSPRO *}  The procedure object.
**
** Outputs:
**	prm	{ABRTSPRM *}
**			.pr_retvalue	{i4 * ==> DB_DATA_VALUE *}
**						the integer count value.
** History:
**	08/90 (jhw) -- Written.
*/
PTR
IIARaxAllRows ( prm, name, proc )
ABRTSPRM	*prm;
char		*name;
ABRTSPRO	*proc;
{
	i4		count = 0;
	DB_DATA_VALUE	*adbv;
	DB_DATA_VALUE	retvalue;

	if ( (adbv = getPrm(prm, name, (i4 *)NULL)) != NULL )
	{
		i4	nondel;
		i4	del;

		if ( iiarArAllCount(adbv, &nondel, &del) == OK )
			count = nondel + del;
	}

	retvalue.db_data = (PTR)&count;
	retvalue.db_datatype = DB_INT_TYPE;
	retvalue.db_length = sizeof(count);
	IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );
	return NULL;
}

/*{
** Name:	IIARaxLastRow() -	Return Non-Deleted Array Size.
**
** Description:
**	Returns a count of the total number of non-deleted records in an array
**	not including the deleted rows.
**
** Inputs:
**	prm	{ABRTSPRM *}  The parameter structure for the call.
**	name	{char *}  Procedure name
**	proc	{ABRTSPRO *}  The procedure object.
**
** Outputs:
**	prm	{ABRTSPRM *}
**			.pr_retvalue	{i4 * ==> DB_DATA_VALUE *}
**						the integer count value.
** History:
**	08/90 (jhw) -- Written.
*/
PTR
IIARaxLastRow ( prm, name, proc )
ABRTSPRM	*prm;
char		*name;
ABRTSPRO	*proc;
{
	i4		count = 0;
	DB_DATA_VALUE	*adbv;
	DB_DATA_VALUE	retvalue;

	if ( (adbv = getPrm(prm, name, (i4 *)NULL)) != NULL )
		_VOID_ iiarArAllCount(adbv, &count, (i4 *)NULL);

	retvalue.db_data = (PTR)&count;
	retvalue.db_datatype = DB_INT_TYPE;
	retvalue.db_length = sizeof(count);
	IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );
	return NULL;
}

/*{
** Name:	IIARaxFirstRow() -	Return Index of First Record.
**
** Description:
**	Returns the index of the first record in an array, which may be a
**	deleted record.  Thus, this will either be one or a non-positive
**	number.
**
** Inputs:
**	prm	{ABRTSPRM *}  The parameter structure for the call.
**	name	{char *}  Procedure name
**	proc	{ABRTSPRO *}  The procedure object.
**
** Outputs:
**	prm	{ABRTSPRM *}
**			.pr_retvalue	{i4 * ==> DB_DATA_VALUE *}
**						the integer index value.
** History:
**	08/90 (jhw) -- Written.
*/
PTR
IIARaxFirstRow ( prm, name, proc )
ABRTSPRM	*prm;
char		*name;
ABRTSPRO	*proc;
{
	i4		first = 1;
	DB_DATA_VALUE	*adbv;
	DB_DATA_VALUE	retvalue;

	if ( (adbv = getPrm(prm, name, (i4 *)NULL)) != NULL )
	{
		_VOID_ iiarArAllCount(adbv, (i4 *)NULL, &first);
		first = 1 - first;
	}

	retvalue.db_data = (PTR)&first;
	retvalue.db_datatype = DB_INT_TYPE;
	retvalue.db_length = sizeof(first);
	IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );
	return NULL;
}

/*{
** Name:	IIARaxInsertRow() -	Insert Record into an Array.
**
** Description:
**	Inserts an empty record into an array at the given index position.
**	This is different than the INSERTROW statement applied to an array
**	since the new record is inserted at the index position not after it.
**	Required parameter:
**
**		rownumber	{integer}  The record number.
**
** Inputs:
**	prm	{ABRTSPRM *}  The parameter structure for the call.
**	name	{char *}  Procedure name
**	proc	{ABRTSPRO *}  The procedure object.
**
** Outputs:
**	prm	{ABRTSPRM *}
**			.pr_retvalue	{i4 * ==> DB_DATA_VALUE}
**						an integer STATUS value.
** History:
**	08/90 (jhw) -- Written.
*/
PTR
IIARaxInsertRow ( prm, name, proc )
ABRTSPRM	*prm;
char		*name;
ABRTSPRO	*proc;
{
	STATUS		status = FAIL;
	i4		indx = 0;
	DB_DATA_VALUE	*adbv;
	DB_DATA_VALUE	retvalue;

	if ( (adbv = getPrm(prm, name, &indx)) != NULL )
		status = IIARariArrayInsert(adbv, indx - 1);

	retvalue.db_data = (PTR)&status;
	retvalue.db_datatype = DB_INT_TYPE;
	retvalue.db_length = sizeof(status);
	IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );
	return NULL;
}

/*{
** Name:	IIARaxSetRowDeleted() -	Delete a Record From an Array.
**
** Description:
**	Sets a positively indexed record in the array to be deleted, which
**	moves it to a non-positive index in the array and sets its state to
**	deleted.  Required parameter:
**
**		rownumber	{integer}  The record number.
**
** Inputs:
**	prm	{ABRTSPRM *}  The parameter structure for the call.
**	name	{char *}  Procedure name
**	proc	{ABRTSPRO *}  The procedure object.
**
** Outputs:
**	prm	{ABRTSPRM *}
**			.pr_retvalue	{i4 * ==> DB_DATA_VALUE}
**						an integer STATUS value.
** History:
**	08/90 (jhw) -- Written.
*/
PTR
IIARaxSetRowDeleted ( prm, name, proc )
ABRTSPRM	*prm;
char		*name;
ABRTSPRO	*proc;
{
	STATUS		status = FAIL;
	i4		indx = 0;
	DB_DATA_VALUE	*adbv;
	DB_DATA_VALUE	retvalue;

	if ( (adbv = getPrm(prm, name, &indx)) != NULL )
		status = IIARardArrayDelete(adbv, indx);

	retvalue.db_data = (PTR)&status;
	retvalue.db_datatype = DB_INT_TYPE;
	retvalue.db_length = sizeof(status);
	IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );
	return NULL;
}

/*{
** Name:	IIARaxRemoveRow() -	Remove Record from an Array.
**
** Description:
**	Removes a record from the dataset for an array.  Required parameter:
**
**		rownumber	{integer}  The record number.
**
**	This should remove deleted records as well if specified.
**
** Inputs:
**	prm	{ABRTSPRM *}  The parameter structure for the call.
**	name	{char *}  Procedure name
**	proc	{ABRTSPRO *}  The procedure object.
**
** Outputs:
**	prm	{ABRTSPRM *}
**			.pr_retvalue	{i4 * ==> DB_DATA_VALUE}
**						an integer STATUS value.
** History:
**	08/90 (jhw) -- Written.
*/
PTR
IIARaxRemoveRow ( prm, name, proc )
ABRTSPRM	*prm;
char		*name;
ABRTSPRO	*proc;
{
	STATUS		status = FAIL;
	i4		indx = 0;
	DB_DATA_VALUE	*adbv;
	DB_DATA_VALUE	retvalue;

	if ( (adbv = getPrm(prm, name, &indx)) != NULL )
		status = iiarArxRemoveRow(adbv, indx);

	retvalue.db_data = (PTR)&status;
	retvalue.db_datatype = DB_INT_TYPE;
	retvalue.db_length = sizeof(status);
	IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );
	return NULL;
}

/*
** Name:	getPrm() -	Get Array Method Parameters.
**
** Description:
**	Gets the parameters for the array `method' built-ins.  Required is an
**	array DB_DATA_VALUE place holder in the first position.  Optional is
**	the record number in the second position possibly specified with the
**	"rownumber" keyword, which is only accepted if the caller specifies
**	the 'indx' parameter.
**
** Inputs:
**	prm	{ABRTSPRM *}  The parameter structure for the call.
**	name	{char *}  Procedure name.
**
** Output:
**	indx	{i4 *}  The record number, which must be specified if
**				this address is not NULL.
** History:
**	08/90 (jhw) -- Written.
*/
static DB_DATA_VALUE *
getPrm ( prm, name, indx )
ABRTSPRM	*prm;
char		*name;
i4		*indx;
{
	register char  		**fp;		/* pointer to formal name */
	register ABRTSPV	*ap;		/* pointer to actuals */
	register i4		cnt;
	DB_DATA_VALUE		*adbv;

	if ( prm == NULL || prm->pr_argcnt == 0 )
	{ /* error!  no arguments */
		return NULL;
	}
	else
	{
		/* assert((prm->pr_version > 1);	calling frame >= 6.0 */

		ap = prm->pr_actuals;
		cnt = prm->pr_argcnt - 1;

		adbv = (DB_DATA_VALUE *)ap->abpvvalue;
		++ap;

		if ( adbv->db_datatype != DB_DMY_TYPE || adbv->db_length != 1 )
		{
			AB_TYPENAME	otype;

			iiarUsrErr( NOTARRAY, 1, iiarTypeName(adbv, otype) );
			return NULL;
		}
		else if ( (fp = prm->pr_formals) != NULL || cnt > 0 )
		{
			KEYWORD_PRM	prm_desc;

			prm_desc.parent = name;
			prm_desc.pclass = F_AR0006_procedure;

			if ( fp == NULL || *fp != NULL )
				{
				iiarUsrErr( _UnknownPrm,
					3, 
					(fp == NULL || *fp == NULL) ? "" : *fp,
					name,
					ERget(F_AR0006_procedure)
					);
				return NULL;
				}
			/* . . . will skip positional array parameter below */
			while ( --cnt >= 0 )
			{
				char	prm_name[FE_MAXNAME+1];

				prm_desc.name = *++fp;
				prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
				++ap;
				if ( *fp == NULL )
					prm_name[0] = EOS;
				else
				{
					STlcopy( *fp, prm_name,
						(u_i4)sizeof(prm_name)-1
					);
					CVlower(prm_name);
				}
				if ( indx != NULL
				    && ( STequal(prm_name, ERx("rownumber"))
				    	|| ( *fp == NULL
						&& cnt == prm->pr_argcnt - 2
						&& AFE_DATATYPE(prm_desc.value)
							== DB_INT_TYPE ) ) )
				{
					iiarGetValue( &prm_desc, (PTR)indx,
						DB_INT_TYPE, sizeof(*indx)
					);
				}
				else
				{
					iiarUsrErr( _UnknownPrm,
						3, *fp == NULL ? "" : *fp, name,
						    ERget(F_AR0006_procedure)
					);
					return NULL;
				}
			} /* end for */
		}

		return adbv;
	}
}
