/*
** Copyright (c) 1993, 2008 Ingres Corporation
*/

EXEC SQL INCLUDE SQLDA;

#include	<compat.h>
#include	<er.h>
#include	<st.h>
#include	<me.h>
#include	<dbms.h>
#include	<fe.h>
#include	<ug.h>
#include	<ui.h>
#include	<erui.h>

/*
** Name:	uiseqval.sc - 4gl Sequence Value Function.
**
** Description:
**	Contains the code which does the actual work of the W4gl
**	SequenceValue() method, Vision/4gl Sequence_value() built in
**	function.
**
**	IIUIsequence_value() - return sequence value
**
** History:
**	28-jun-93 (blaise)
**		Initial version, based on code formerly in abfrt!abrtseqv.sc.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

# define HASH_SIZE	17	/* size of hash table */

typedef struct
{
	char string[2*DB_GW1_MAXNAME + 2];
	char owner[DB_GW1_MAXNAME + 1];
	char table[DB_GW1_MAXNAME + 1];
} TABLE_NAMES;

FUNC_EXTERN STATUS	IIUGhfHtabFind();
FUNC_EXTERN STATUS	IIUGhiHtabInit();
FUNC_EXTERN STATUS	IIUGheHtabEnter();

static PTR	hashtab = NULL;
static STATUS	get_max();
static STATUS	resolve_table();

/* Argument names */
static const char ARGincrement[] = ERx("increment");
static const char ARGstart_value[] = ERx("start_value");

/* {
** Name:	IIUIsequence_value() - Return sequence value for key.
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
** Inputs:
**	(these correspond to the parameters passed to the function/method by
**	the user's code)
**	table_string {char *}	table name parameter
**	column_name {char *}	column name parameter
**	increment {nat}		increment parameter (optional, defaults to 1)
**	start_value {nat}	start value parameter (optional, defaults to 1)
**
** Returns:
**	value {nat}		the new sequence value
**
** Side Effects:
**	Updates the ii_sequence_values system catalogue.
**
** History:
**	28-jun-93 (blaise)
**		Initial Version, based on code formerly in abrtseqv.sc.
**	21-sep-93 (donc) Bug 54879
**		modified resolve_table. "owner.table" wasn't being
**		handled properly because "dot" variable was never set
**		to the pointer returned by STindex.
**	13-jun-1994 (mgw) Bug #64207
**		Send proper parameters down to IIUGerr()
*/

i4
IIUIsequence_value(table_string, column_name, increment, start_value)
exec sql begin declare section;
char	*table_string;
char	*column_name;
i4	increment;
i4	start_value;
exec sql end declare section;
{
	exec sql begin declare section;
	char	*owner;
	char	*table_name;
	i4	value = 0;
	exec sql end declare section;

	bool	db_err = FALSE;

	/* Check that the increment and start_value parameters are positive */
	if (increment <= 0)
	{
		i4	linc = increment;

		IIUGerr(E_UI003A_BadSqvNumber, UG_ERR_ERROR, 2, ARGincrement,
		        (PTR)&linc);
		goto ret;
	}
	if (start_value <= 0)
	{
		i4	lstart = start_value;

		IIUGerr(E_UI003A_BadSqvNumber, UG_ERR_ERROR, 2, ARGstart_value,
		        (PTR)&lstart);
		goto ret;
	}

	/* Now parse the table_string */
	if (resolve_table(table_string, &owner, &table_name) != OK)
	{
		IIUGerr(E_UI003B_NoSuchTable, UG_ERR_ERROR, 1, table_string);
		goto ret;
	}

	/* Turn on catalogue writing capability */
	iiuicw1_CatWriteOn();

	/* update the system catalogue */
	exec sql repeated
	update ii_sequence_values
		set sequence_value = sequence_value + :increment
		where sequence_owner = :owner and
			sequence_table = :table_name and
			sequence_column = :column_name;
	if ( FEinqerr() != OK )
	{
		db_err = TRUE;
		goto ret;
	}
	else if ( FEinqrows() != 0 )
	{
		/* read the data back */
		exec sql select sequence_value
			into :value from ii_sequence_values
			where sequence_owner = :owner and
			      	sequence_table = :table_name and
				sequence_column = :column_name;
		if ( FEinqerr() != OK || FEinqrows() == 0 )
		{
			/* strange error */
			db_err = TRUE;
			goto ret;
		}
	}
	else
	{
		/* Add row for table. First figure out what value */
		if (get_max(table_name, column_name, start_value, &value) != OK)
		{
			value = 0;
			goto ret;
		}
		value += increment;
		exec sql insert into ii_sequence_values
			(sequence_owner, sequence_table, sequence_column,
				sequence_value)
			values ( :owner, :table_name, :column_name, :value);
		if ( FEinqerr() != OK || FEinqrows() == 0 )
		{
			db_err = TRUE;
			goto ret;
		}
	}
ret:
	if (db_err)
	{
		/* a database error occurred */
		IIUGerr(E_UI003C_Sqv_DBErr, UG_ERR_ERROR, 0);
		value = 0;
	}
	iiuicw0_CatWriteOff();

	return value;
}


/* Static places for table data */
static char table_name[DB_GW1_MAXNAME+1];
static char owner_name[DB_GW1_MAXNAME+1];

/*
**	Translate table name to owner and table.  This is a poor thing 
**	compared to the DBMS resolve_table function we're expecting in 6.5.
**	It hashes names we've already resolved.
*/
static STATUS
resolve_table(string, owner, table)
char	*string;
char	**owner;
char	**table;
{
	char	*dot;	
	PTR	ptr;
	TABLE_NAMES	*tbl;
	FE_REL_INFO relinfo;
	STATUS status;

	/* See if the "." is present */
	if ( (dot = STindex(string, ERx("."), 0)) != NULL)
	{
		/* It is; we've got "owner.table" already */
		STlcopy(string, owner_name, min(dot - string, DB_GW1_MAXNAME));
		STlcopy(dot+1, table_name, DB_GW1_MAXNAME);
		*owner = owner_name;
		*table = table_name;
		return OK;
	}

	/* We have an unqualified table name; is it in the hash table? */
	if (hashtab != NULL)
	{
		if (IIUGhfHtabFind(hashtab, string, &ptr) == OK)
		{
			/* Found it */
			tbl = (TABLE_NAMES *)ptr;
			*owner = tbl->owner;
			*table = tbl->table;
			return OK;
		}
	}
	else
	{
		/* Initialize hash table */
		_VOID_ IIUGhiHtabInit(HASH_SIZE, 
				(VOID (*)())NULL, (i4 (*)())NULL,
				(i4 (*)())NULL, &hashtab);
	}

	/* We have an unqualified table name; let's ask the database */
	status = FErel_ffetch(string, FALSE, &relinfo);
	if (status == OK)
	{
		/* Let's hash it for later use */
		tbl = (TABLE_NAMES *)
			MEreqmem((u_i4)0, (u_i4)sizeof(TABLE_NAMES), 
				  FALSE, NULL);
		STcopy(string, tbl->string);	
		STcopy(relinfo.owner, tbl->owner);
		STcopy(relinfo.name, tbl->table);
		IIUGheHtabEnter(hashtab, tbl->string, (PTR)tbl);

		*owner = tbl->owner;
		*table = tbl->table;
	}
	return status;
}

/*
**	We don't have a row in the sequenced_value table which describes the
**	current table; let's try to find the maximum current value.
*/
static STATUS
get_max(table, column, start_value, maxval)
char	*table;
char	*column;
i4	start_value;
i4	*maxval;	/* Output */
{
	/*
	** Declare a small SQL data area, since we're only retrieving one value.
	*/
	IISQLDA_TYPE(sqldatag, sqlda, 1);
	struct sqldatag  *p_sqlda = &sqlda;

	exec sql begin declare section;
	char qtext[256];	/* Query text */
	i2 ind;			/* Nullablility indicator */
	i4 data;		/* Data to retrieve (column maximium) */
	exec sql end declare section;

	sqlda.sqln = 1;
	STprintf(qtext, ERx("SELECT MAX(%s) FROM %s"), column, table);
	exec sql prepare s1 from :qtext;
	if (FEinqerr() != OK)
	{
		goto error;
	}
	exec sql describe s1 into :p_sqlda;
	if (FEinqerr() != OK)
	{
		goto error;
	}

	/* 
	** At this point, we've looked at the table and retrieved the column
	** definitions.  We'll complain if it's not an i4.
	*/
	if (abs(sqlda.sqlvar[0].sqltype) != DB_INT_TYPE || 
	    sqlda.sqlvar[0].sqllen != 4)
	{
		IIUGerr(E_UI003D_NotI4column, UG_ERR_ERROR, 2, column, table);
		return FAIL;
	}

	/* Set up data pointers */
	sqlda.sqlvar[0].sqldata = (char *)&data;
	sqlda.sqlvar[0].sqlind = (short *)&ind;

	exec sql execute immediate :qtext USING :p_sqlda;
	if (FEinqerr() != OK)
	{
		goto error;
	}
	
	if (ind < 0)
	{
		data = start_value - 1;	/* No rows have a non-NULL value */
	}
	*maxval = abs(data);		/* Shouldn't be negative */
	return OK;

/* SQL error handling */
error:
	IIUGerr(E_UI003E_BadMaxGet, UG_ERR_ERROR, 2, column, table);
				/* Explain what it means for the user */
	return FAIL;
}
