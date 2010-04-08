/*
** Copyright (c) 1989, 2008 Ingres Corporation
**	All rights reserved.
*/

EXEC SQL INCLUDE SQLDA;

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<er.h>
#include	<st.h>
#include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<ui.h>
#include        <fdesc.h>
#include        <abfrts.h>
#include	<erar.h>
#include	"keyparam.h"

/**
** Name:	abrtseqv.sc -	ABF Run-Time Sequence Value Function.
**
** Description:
**	Contains the routine used to implement the sequence value system
**	function.  This function returns a sequence value.  Define:
**
**	IIARsequence_value()	return sequence value for key.
**
** History:
**	Revision 6.3/03/00  89/06  wong
**	Initial revision.
**
**	8/90 (Mike S)
**	New parameters; new functionality.
**
**	Revision 6.4/02
**	26-jul-92 (davel)
**		Fixed bug 44305 (same as 44982) - strings and data passed to
**		IIUGheHtabEnter() must point to saved copies of the key and 
**		data. Fix made in static resolve_table().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

# define HASH_SIZE	17	/* Size of hash table */

typedef struct
{
	char string[2*DB_GW1_MAXNAME + 2];
	char owner[DB_GW1_MAXNAME + 1];
	char table[DB_GW1_MAXNAME + 1];
} TABLE_NAMES;

FUNC_EXTERN STATUS	IIUGhfHtabFind();
FUNC_EXTERN STATUS	IIUGhiHtabInit();
FUNC_EXTERN STATUS	IIUGheHtabEnter();

static PTR hashtab = NULL;
static STATUS get_max();
static STATUS resolve_table();

/* Argument names */
static const char ARGtable_name[] = ERx("table_name");
static const char ARGcolumn_name[] = ERx("column_name");
static const char ARGincrement[] = ERx("increment");
static const char ARGstart_value[] = ERx("start_value");

/*{
** Name:	IIARsequence_value() -	Return Sequence Value for Key.
**
** Description:
**	Returns a sequence value (or range) for the input key given a range
**	increment.  This value is the highest sequential integer in the
**	allocated range between 1 and 2^31 - 1.  A zero sequence value is an
**	error and is returned then or when no more values can be allocated.
**	A starting value can also be specified; it will be used if there
**	is no corresponding row in the ii_sequence_values table, and the
**	database table has no non-NULL rows for the specified column.
**
**	The UPDATE detects that the row exists and that another value (or range)
**	can be allocated.  If this fails, then either the row does not exist or
**	no more sequence values can be allocated.  If the row for the key does
**	not exist, then the row will be added to the sequence value table. It
**	will be initialzed at 1 greater then the current maximum value of
**	the key.
**
**	This routine must run in a transaction.  We won't attempt to guarantee
**	that here, but documentation should stress it.
**
** Input:
**      prm     {ABRTSPRM *}  The parameter structure for the call.
**      name    {char *}  Procedure name
**      proc    {ABRTSPRO *}  The procedure object.
**
** Returns:
**
** History:
**	06/89 (jhw) -- Written.
**	8/90 (Mike S) -- Added logic to search base table, resolve table names,
**			parse named parameters.
**	10-sep-93 (blaise)
**		Most of the code for this function has been moved to utils!ui,
**		to allow W4GL to use the sequence_values function. Replaced
**		a chunk of code with a call to IIUIsequence_value() - this
**		function only does parameter parsing now.
*/

PTR
IIARsequence_value ( prm, name, proc)
ABRTSPRM        *prm;
char            *name;
ABRTSPRO        *proc;
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	*owner;
	char	*table_name;
	char	column_name[FE_MAXNAME+1];
	i4	inc;
	i4	start_value;
	i4	value;
	EXEC SQL END DECLARE SECTION;

	bool		db_err = FALSE;
	KEYWORD_PRM     prm_desc;
	char	table_string[2*FE_MAXNAME+2];
	DB_DATA_VALUE   retvalue;

	register char           **fp;           /* pointer to formal name */
	register ABRTSPV        *ap;            /* pointer to actuals */
	register i4             cnt;

	/* Initialize parameters */
	table_string[0] = EOS;
	column_name[0] = EOS;
	inc = 1;
	start_value = 1;
	value = 0;

	/* Now, let's get our arguments */
	if (prm != NULL)
	{
		prm_desc.parent = name;
		prm_desc.pclass = F_AR0006_procedure;
		for (fp = prm->pr_formals, ap = prm->pr_actuals, 
			cnt = prm->pr_argcnt;
		     fp != NULL && --cnt >= 0;
		     ++fp, ++ap
		)
		{
			char    prm_name[FE_MAXNAME+1];

			if ( *fp == NULL )
			{
				iiarUsrErr( E_AR0004_POSTOOSL, 1,
					   ERget(F_AR0005_frame) );
				goto done;
			}
			prm_desc.name = *fp;
			prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
			STlcopy(*fp, prm_name, (u_i4)sizeof(prm_name) - 1);
			CVlower(prm_name);
			if ( STequal(prm_name, ARGtable_name))
			{
				iiarGetValue(&prm_desc, (PTR)table_string,
					     DB_CHR_TYPE, 
					     sizeof(table_string)-1);	
				_VOID_ STtrmwhite(table_string);
			}
			else if (STequal(prm_name, ARGcolumn_name))
			{
				iiarGetValue(&prm_desc, (PTR)column_name,
					     DB_CHR_TYPE,
					     sizeof(column_name)-1);
				_VOID_ STtrmwhite(column_name);
			}
			else if (STequal(prm_name, ARGincrement))
			{
				iiarGetValue(&prm_desc, (PTR)&inc,
						DB_INT_TYPE, 
						sizeof(inc));
			}
			else if (STequal(prm_name, ARGstart_value))
			{
				iiarGetValue(&prm_desc, (PTR)&start_value,
						DB_INT_TYPE, 
						sizeof(start_value));
			}
			else
			{
				iiarUsrErr( _UnknownPrm, 3, *fp, name, 
					    ERget(F_AR0006_procedure));
				goto done;
			}
		}
	}
	
	/* Let's be sure all needed parameters were entered */
	if (table_string[0] == EOS)
	{
		iiarUsrErr(E_AR0072_NoSqv_Parameter, 1, ARGtable_name);
		goto done;
	}
	if (column_name[0] == EOS)
	{
		iiarUsrErr(E_AR0072_NoSqv_Parameter, 1, ARGcolumn_name);
		goto done;
	}

	value = IIUIsequence_value(table_string, column_name, inc, start_value);

done:
	retvalue.db_data = (PTR)&value;
	retvalue.db_datatype = DB_INT_TYPE;
	retvalue.db_length = sizeof(value);
	IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );
	return NULL;
}
