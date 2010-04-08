/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
+*  Name: iisqlca.h - Runtime defs for SQLCA handling in an embedded program.
**
**  Description:
**	This file contains our internal typedef definition of the SQLCA as
**	well as some constants and a macro used by LIBQ in assigning status
**	information to the SQLCA.  
**
**  Defines:
**	IISQLCA
**
**  Notes:
**	When LIBQ references the SQLCA, it relies on the typedef contained in
**	this file and a pointer passed in from the user program.  When an 
**	embedded program declares the use of the SQLCA, the preprocessor 
**	generates an external (or common) declaration of the SQLCA based on 
**	the SQLCA definition contained in the file 'eqsqlca.{h|def}'.  
**	Obviously, the layout of data in both definitions is identical.
-*
**  History:
**	13-apr-1987	- written (bjb)
**	06-dec-1989	- Updated for Phoenix/Alerters. (bjb)
**/

/*
** Name: SQLCA - SQL Communications Area
** 
** Description:
**	The SQLCA structure is the status area in which SQL errors,
**	warnings and return codes are stored.  The global definition
**	of this is in a library called iisqlca, with which all embedded
**	programs must link.  It is global so that no matter what
**	language or module the program is currently in, the structure
**	being passed to the SQL run-time system is the same.
**
**	This structure contains the status information that is returned
**	to the embedded program after each executable SQL statement.  The
**	SQLCA is updated during the execution of every database-related
**	SQL statement.  By inspecting the SQLCA the program can take
**	appropriate action in response to errors or exceptional conditions
**	that occurred in the most recently executed SQL statement.
**	The original definition of this structure (for PL/I) is described
**	in the DB2 programming guide.
**
**	The SQLCA is passed to LIBQ before the execution of each query.
**	Internally, LIBQ will update the different fields through the different
**	phases of the query.  It is up to LIBQ to map the internal errors,
**	status bits, and data results to the different fileds in the SQLCA.
**
**  History:
**	28-apr-1987	- written (bjb)
**	11-may-1988	- added definition of SQL messages (ncg)
**	06-dec-1989	- added definition of SQL events. (bjb)
**	23-feb-1993 - added tags to structure for making correct prototypes (mohan).
**	11-mar-93 (fraser)
**	    Changed structure tag name--shouldn't begin with underscore.
*/

typedef struct s_IISQLCA {
    char	sqlcaid[8];	/* Eye catcher initialized to "SQLCA " */
    i4		sqlcabc;	/* Set to sizeof(IISQLCA) */
    i4		sqlcode; 	/* SQL return code - 
				** 0   - Executed successfully
				** > 0 - Successful but with an exception.
				**       100 = No rows satisfy a query
				**             qualification.
				**	 700 = User message returned from
				**	       database procedure.
				**	 710 = Unclaimed event info queued.
				** < 0 - Is the negative value of an
				**       error number.  */
# define	IISQ_NOTFOUND	(i4)100
# define	IISQ_MESSAGE	(i4)700
# define	IISQ_EVENT	(i4)710
    struct {			/* Error message when sqlcode < 0 */
	i2	sqlerrml;	/* Length of following error message */
				/* Size of sqlerrmc */
# define	IISQ_ERRMLEN	70
				/* As much of error message (if any) */
	char	sqlerrmc[IISQ_ERRMLEN];
    } sqlerrm;
    char	sqlerrp[8];	/* Internal module name (?) */
    i4		sqlerrd[6];	/* Diagnostic values - the third
				** element (sqlerrd[2]) indicates how
				** how many rows were physically updated
				** (or returned) for the last query.  */
				/* Warning indicators - when a warning is
				** set then the field is set to 'W'
				** otherwise the field is blank. */
    struct {
	char	sqlwarn0;	/* Set to W when another warning is set */
	char	sqlwarn1;	/* Truncation of string into variable */
	char	sqlwarn2;	/* Elimination of null values in aggregate */
	char	sqlwarn3;	/* # result variables != # result columns */
	char	sqlwarn4;	/* PREPAREd UPDATE or DELETE without WHERE */
	char	sqlwarn5;	/* Unused */
	char	sqlwarn6;	/* Unused */
	char	sqlwarn7;	/* Unused */
    } sqlwarn;
    char	sqlext[8];	/* Unused */
} IISQLCA;

/* 
** Macro used by LIBQ in referring to SQLCA
*/
# define IISQ_SQLCA_MACRO(lb)	(lb->ii_lq_sqlca && lb->ii_lq_dml == DB_SQL)
