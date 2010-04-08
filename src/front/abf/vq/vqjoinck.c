
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include       "ervq.h"
# include       <er.h>
# include       <abclass.h>
# include       <metafrm.h>


/**
** Name:	vqjoinck.c - Check a join for validity
**
** Description:
**	This file defines:
**		IIVQjcJoinCheck  - check join for validity
**		IIVQjwJoinWarn  - issue warning messages for questionable joins
**
** History:
**	11/10/89 (tom)	- created
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name:	IIVQjcJoinCheck	- check join for validity
**
** Description:
**	This function takes 2 column arguments and tests them for 
**	a valid join.
**
** Inputs:
**	MFCOL *col1;	- column one to consider
**	MFCOL *col2;	- column two to consider
**
** Outputs:
**	Returns:
**		TRUE - the columns can be joined
**		FALSE - the columns do not join because of type problems
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	11/10/89 (tom) - created
*/
bool
IIVQjcJoinCheck(col1, col2)
MFCOL *col1;
MFCOL *col2;
{

	ADI_FI_ID   dummy = 0;
	i4 fmt1 = col1->type.db_datatype;
	i4 fmt2 = col2->type.db_datatype;
	DB_DATA_VALUE dbv;


	/* do length check to insure that the dbv is in good shape */
	if (  adc_lenchk(FEadfcb(), FALSE, &col1->type, &dbv) != OK
	   || adc_lenchk(FEadfcb(), FALSE, &col2->type, &dbv) != OK
	   )
	{
		return (FALSE);
	}

	/* make sure that the types can be coerced in both directions */
	if (  adi_ficoerce(FEadfcb(), fmt1, fmt2, &dummy) != OK
	   || adi_ficoerce(FEadfcb(), fmt2, fmt1, &dummy) != OK
	   )
	{ 
		return (FALSE);
	}

	return (TRUE);
}
/*{
** Name:	IIVQjwJoinWarn	- give warnings about possible join problems
**
** Description:
**	This function takes 2 column arguments, and table names and 
**	tests them for compatibility, and issues warnings for questionable
**	compatibility. (Note: that the joins are assumed to be valid)
**	If the tabname1 is NULL then we will just return false,
**	no errors (except internal ones) are reported.
**
** Inputs:
**	char *tabname1	- name of table for column 1
**				(if null.. don't report.. just return)
**	char *tabname2	- name of table for column 2 
**	MFCOL *col1;	- column one to consider
**	MFCOL *col2;	- column two to consider
**
** Outputs:
**	Returns:
**		TRUE - the columns are exact
**		FALSE - the columns have warnings associated
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	11/10/89 (tom) - created
*/
bool
IIVQjwJoinWarn(tabname1, tabname2, col1, col2)
char *tabname1;
char *tabname2;
MFCOL *col1;
MFCOL *col2;
{

	i4 fmt1 = col1->type.db_datatype;
	i4 fmt2 = col2->type.db_datatype;
	DB_DATA_VALUE dbv1;
	DB_DATA_VALUE dbv2;
	bool retval = TRUE;


	/* get a length that we can compare latter */
	if (  adc_lenchk(FEadfcb(), FALSE, &col1->type, &dbv1) != OK
	   || adc_lenchk(FEadfcb(), FALSE, &col2->type, &dbv2) != OK
	   )
	{
		FEafeerr(FEadfcb());	/* internal error */
		return (FALSE);
	}

	if (abs(fmt1) != abs(fmt2))
	{
		retval = FALSE;
		if (tabname1 != (char*)NULL)
		{
			IIVQer4(E_VQ0077_Mixed_Datatype_Join,
				tabname1, col1->name, tabname2, col2->name);
		}
	}

# if 0	/* !!! take this out if not needed */
	if (AFE_NULLABLE_MACRO(fmt1) != AFE_NULLABLE_MACRO(fmt2))
	{
		retval = FALSE;
		if (tabname1 != (char*)NULL)
		{
			IIVQer4(E_VQ0078_Mixed_Nullable_Join,
				tabname1, col1->name, tabname2, col2->name);
		}
	}
# endif

	if (dbv1.db_length != dbv2.db_length)
	{
		retval = FALSE;
		if (tabname1 != (char*)NULL)
		{
			IIVQer4(E_VQ0079_Mixed_Length_Join,
				tabname1, col1->name, tabname2, col2->name);
		}
	}

	return (retval);
}
