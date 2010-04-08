/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>

/**
+*  Name: iidml.c - No op.
**
**  Defines:
**	IIdml - No op.
**
**  Description
**	IIdml used to set the dml language.  However, the correct way to
**	set the dml language to DB_SQL is by calling IIsqInit() at the
**	commencement of an ESQL statement.  The dml is by default set
**	to DB_QUEL, and is reset to DB_QUEL at the end of any statement
**	through IIqry_end().  IIdml() is going away because its use can
**	cause a problem in the following circumstances:
**	    1. 	An ESQL statement is started.  LIBQ sets ii_lq_dml to
**		DB_SQL and sets ii_lq_sqlca to point at user's SQLCA.
**	    2.  The ESQL statement completes.  LIBQ sets ii_lq_dml to
**		DB_QUEL but leaves ii_lq_sqlca alone.  (This is because
**		holding on to the SQLCA is useful in nested statements.)
**	    3.  A (internal) program sets the dml to DB_SQL via IIdml().
**		In this case LIBQ might have an old and invalid pointer to 
**		an SQLCA.
**	Solution:
**	    Internal programs should set ii_lq_ dml to DB_SQL through IIsqInit
**	    which will also set ii_lq_sqlca to be the current SQLCA or
**	    to null.
-*
**  History:
**	dd-mmm-yyyy	- text (initials)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

i4
IIdml(lang)
i4	lang;
{
    return 0;
}
