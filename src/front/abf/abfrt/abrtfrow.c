/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<st.h>
#include	<me.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<uf.h>
#include	<adf.h>
#include	<afe.h>
#include	<abfcnsts.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	"keyparam.h"

/**
** Name:	abrtfrow.c -   ABF Run-time System Find Record System Procedure.
**
** Description:
**	Contains the routine that is the 4GL find record system procedure.
**	Defines:
**
**	IIARfnd_row()		find table field record with column value.
**
** History:
**	Revision 6.3/03/00  89/07  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for find_record() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**    30-May-2006 (hweho01)
**        Changed the old style parameter list in find_record(),  
**        prevent compiler error on AIX platform.
*/

GLOBALREF ABRTSSTK	*IIarRtsStack;

/* local prototypes */

static i4 find_record(
		FLD_VALUE	*fld,
		register DB_DATA_VALUE	*value,
		bool		noscroll);

/*{
** Name:	IIARfnd_row() -	Find Table Field Record with Column Value.
**
** Description:
**	Find the record of a table field that contains the specified value in a
**	column on a form.  All parameters are optional keywords and are as
**	follows:
**
**		form		Any string type.  The form name.
**		table_field	Any string type.  The table field name.
**		column		Any string type.  The column name.
**		value		Any type.	  The value for which to search.
**		noscroll	Integer		  Whether to scroll to record.
**
** Inputs:
**	prm	{ABRTSPRM *}  The parameter structure for the call.
**	name	{char *}  Procedure name
**	proc	{ABRTSPRO *}  The procedure object.
**
** History:
**	07/89 (jhw) -- Written.
**	06/90 (jhw) -- Added 'noscroll'.
*/
PTR
IIARfnd_row ( prm, name, proc )
ABRTSPRM	*prm;
char		*name;
ABRTSPRO	*proc;
{
	register char  		**fp;		/* pointer to formal name */
	register ABRTSPV	*ap;		/* pointer to actuals */
	register i4		cnt;

	bool		noscroll;
	DB_DATA_VALUE	*value;
	i4		record;
	KEYWORD_PRM	prm_desc;
	FLD_VALUE	fld_value;
	char		form_name[FE_MAXNAME+1];
	char		tblfld[FE_MAXNAME+1];
	char		column[FE_MAXNAME+1];
	DB_DATA_VALUE	retvalue;

	if (prm == NULL)
	{
		record = IIFRfind_row( (FLD_VALUE *)NULL, (DB_DATA_VALUE *)NULL,
					FALSE
		);
	}
	else
	{
		/* assert((prm->pr_version > 1);	calling frame >= 6.0 */

		form_name[0] = EOS;
		tblfld[0] = EOS;
		column[0] = EOS;

		fld_value._form = form_name;
		fld_value._field = tblfld;
		fld_value._column = column;
		value = NULL;
		noscroll = FALSE;

		prm_desc.parent = name;
		prm_desc.pclass = F_AR0006_procedure;
		for ( fp = prm->pr_formals, ap = prm->pr_actuals,
						cnt = prm->pr_argcnt ;
				fp != NULL && --cnt >= 0 ; ++fp, ++ap )
		{
			char	prm_name[FE_MAXNAME+1];

			if ( *fp == NULL )
				iiarUsrErr( E_AR0004_POSTOOSL,
						1, ERget(F_AR0005_frame)
				);
			prm_desc.name = *fp;
			prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
			STlcopy(*fp, prm_name, (u_i4)sizeof(prm_name) - 1);
			CVlower(prm_name);
			if ( STequal(prm_name, ERx("form")) )
			{
				iiarGetValue( &prm_desc, (PTR)form_name,
						DB_CHR_TYPE, sizeof(form_name)-1
				);
				_VOID_ STtrmwhite(form_name);
			}
			else if ( STequal(prm_name, ERx("table_field")) )
			{
				iiarGetValue( &prm_desc, (PTR)tblfld,
						DB_CHR_TYPE, sizeof(tblfld) - 1
				);
				_VOID_ STtrmwhite(tblfld);
			}
			else if ( STequal(prm_name, ERx("column")) )
			{
				iiarGetValue( &prm_desc, (PTR)column,
						DB_CHR_TYPE, sizeof(column) - 1
				);
				_VOID_ STtrmwhite(column);
			}
			else if ( STequal(prm_name, ERx("value")) )
			{
				value = prm_desc.value;
			}
			else if ( STequal(prm_name, ERx("noscroll")) )
			{
				iiarGetValue( &prm_desc, (PTR)&noscroll,
						DB_INT_TYPE, sizeof(noscroll)
				);
			}
			else
			{
				iiarUsrErr( _UnknownPrm,
					3, *fp, name, ERget(F_AR0006_procedure)
				);
			}
		} /* end for */

		record = find_record(&fld_value, value, noscroll);
	}

	retvalue.db_data = (PTR)&record;
	retvalue.db_datatype = DB_INT_TYPE;
	retvalue.db_length = sizeof(record);
	IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );
	return NULL;
}

/*
** Name:	find_record() -	Find Table Field Record with Column Value.
**
** Description:
**	Allocates internal memory for the column value if it is a string type.
**	This is because the find record utility will overwrite the value if it
**	contains any pattern match characters.  At the same time, it translates
**	any string value passed from an SQL frame or procedure into a string
**	that contains QUEL pattern match characters.
**
** History:
**	08/89 (jhw) -- Written.
**	31-aug-1993 (mgw)
**		Casted MEcopy args properly for new IIMEcopy prototype
**		compatability.
*/
static i4
find_record ( FLD_VALUE *fld, register DB_DATA_VALUE *value, bool noscroll )
{
	i4			record;
	DB_LANG			dml = IIarRtsStack->abrfsnext->abrfsdml;
	DB_DATA_VALUE		buffer;
	AFE_DCL_TXT_MACRO(128)	_buffer;

	if ( value != NULL
		&& ( !AFE_NULLABLE(value->db_datatype) || !AFE_ISNULL(value) ) )
	{ /* if non-Null value */
		i4 len;

		buffer.db_datatype = AFE_DATATYPE(value);
		buffer.db_length = value->db_length;
		if ( AFE_NULLABLE(value->db_datatype) )
			--buffer.db_length;

		switch ( buffer.db_datatype )
		{
		  default:
			/* Not a string type; do nothing.  Note that
			** "value != &buffer", which skips any possible
			** translation below.
			*/
			break;

		  case DB_CHR_TYPE:
		  case DB_CHA_TYPE:
			len = buffer.db_length;
			buffer.db_data =
				( len > sizeof(_buffer.text) )
					? MEreqmem(0, len, FALSE, (STATUS*)NULL)
					: (PTR)_buffer.text;
			MEcopy(value->db_data, (u_i2)len, buffer.db_data);
			value = &buffer;
			break;

		  case DB_TXT_TYPE:
		  case DB_VCH_TYPE:
		  case DB_LTXT_TYPE:
			len = ((DB_TEXT_STRING*)value->db_data)->db_t_count;

			buffer.db_data =
				( len > sizeof(_buffer.text) )
					? MEreqmem(0, len+sizeof(_buffer.count),
							FALSE, (STATUS*)NULL)
					: (PTR)&_buffer;
			MEcopy( (PTR)
				((DB_TEXT_STRING*)value->db_data)->db_t_text,
				(u_i2) len,
				(PTR)
				((DB_TEXT_STRING*)buffer.db_data)->db_t_text
			);
			((DB_TEXT_STRING *)buffer.db_data)->db_t_count = len;

			value = &buffer;
			break;
		} /* end switch */

		if ( value == &buffer && dml == DB_SQL )
		{ /* translate possible SQL pattern to QUEL */
			i4	pm_result;

			_VOID_ IIAFcvWildCard ( value, DB_QUEL,
						AFE_PM_SQL_CHECK, &pm_result
			);
		}
	}

	record = IIFRfind_row(fld, value, noscroll);

	/* Free any allocated data */
	if ( value == &buffer && buffer.db_data != (PTR)&_buffer
			&& buffer.db_data != (PTR)_buffer.text )
		MEfree(buffer.db_data);

	return record;
}
