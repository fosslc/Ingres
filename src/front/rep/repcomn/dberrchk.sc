/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <generr.h>
# include <rpu.h>

/**
** Name:	dberrchk.sc - check for database error
**
** Description:
**	Defines
**		RPdb_error_check - check for database error
**
** History:
**	16-dec-96 (joea)
**		Created based on dberrchk.sc in replicator library.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	RPdb_error_check - check for database error
**
** Description:
**	Verifies the success of a database operation.  If there is no error
**	and the number of rows affected agrees with the expected result, it
**	returns OK.  If there is a deadlock error or the INGRES log file
**	becomes full, DBEC_DEADLOCK is returned.  For any other kind of error,
**	DBEC_ERROR is returned.
**
** Inputs:
**	flags	- integer combination of zero, one or more of the following
**		  constants, joined with bitwise-OR:
**
**	Constant		Meaning
**	------------	-------------------------------------------
**	SINGLE_ROW	The operation should affect only one row.
**
**	ZERO_ROWS_OK	The operation is considered successful even if it
**			does not affect any rows.
**
** Outputs:
**	errinfo	- if this pointer is not NULL, the following members are
**		  filled in:
**
**		errorno	 - generic INGRES error
**		rowcount - number of rows affected by the last query
**
** Returns:
**	OK		- database operation successful
**	DBEC_DEADLOCK	- serialization error
**	DBEC_ERROR	- other database error
**/
STATUS
RPdb_error_check(
i4		flags,
DBEC_ERR_INFO	*errinfo)
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	n;
	i4	err;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL INQUIRE_SQL (:n = ROWCOUNT, :err = ERRORNO);
	if (errinfo != (DBEC_ERR_INFO *)NULL)
	{
		errinfo->errorno = err;
		errinfo->rowcount = n;
	}
	if (err == GE_SERIALIZATION)
		return (DBEC_DEADLOCK);
	else if (err || (n != 1 && flags & DBEC_SINGLE_ROW) ||
			(n == 0 && !(flags & DBEC_ZERO_ROWS_OK)))
		return (DBEC_ERROR);
	return (OK);
}
