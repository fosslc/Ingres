
/*
** Copyright (c) 2004 Ingres Corporation
*/

#ifndef IISQLDA_H_INCLUDED
#define IISQLDA_H_INCLUDED

/**
+*  Name: iisqlda.h - Runtime defs for SQLDA handling in an embedded program.
**
**  Description:
**	This file contains our internal typedef definition of the SQLDA.
**
**  Defines:
**	IISQLDA
**	IISQLHDLR
**
**  Notes:
**	When LIBQ references the SQLDA, it relies on the typedef contained in
**	this file and a pointer passed in from the user program.  When an 
**	embedded program declares the use of the SQLDA, the preprocessor 
**	generates an include of a file 'eqsqlda.{h|def}' which contains
**	a similar typedef.  Obviously, the layout of data in both files is 
**	identical.
-*
**  History:
**	05-may-1987	- written (bjb)
**	26-oct-1992	- Added IISQLHDLR structure (kathryn).
**	19-Aug-2009 (kschendel) 121804
**	   Allow repeated inclusion (e.g. by runtime.h)
**/

/*
** Name: SQLDA - SQL Dynamic Area
** 
** Description:
**	The SQLDA structure is the structure that is filled and used to 
**	execute some of the dynamic SQL statements.  The user can define
**	this structure themselves or include it via the embedded INCLUDE
**	SQLDA command.
**	This structure contains the information to describe and process
**	dynamic queries.  The structure is used in three situations:
**	1. Data types are described into it.  This could be via the
**	   DESCRIBE statement, or via explicit filling of data types
**	   (and variable addresses) by the user.
**	2. The structure (once filled with variable addresses) can be
**	   used for input, as in the OPEN cursor_name USING statement.
**	   In this case the routine that is passed the SQLDA will call
**	   the appropriate input routine for all described variables.
**	3. The structure (once filled with variable addresses) can be
**	   used for output, as in the FETCH cursor_name USING statement.
**	   In this case the routine that is passed the SQLDA will call
**	   the appropriate output routine for all described variables.
**      The original definition of this structure (for PL/I) is documented
**      in the DB2 programming guide.
**
**	Note that variable addresses may include addresses of IISQLHDLR 
**	structures as described below.  When an SQLVAR has datatype of 
**	DB_HDLR_TYPE then sqldata will be expected to contain a pointer to an 
**	IISQLHDLR structure which contains a user-defined datahandler and 
**	optional argument.  The user-defined datahandler will be responsible 
**	for transmitting a data value to/from the database.
**
**  History:
**	05-may-1987	- written (bjb)
**	02-feb-1987	- increased length of sqlnamec to 34 (ncg)
**      17-Nov-2003 (hanal04) Bug 111309 INGEMB113
**          Changed sqllen to unsigned to prevent overflow when
**          using NVARCHARs.
*/

typedef struct {
    char	sqldaid[8];	/* Eye catcher initialized to "SQLDA " */
    i4		sqldabc;	/* Set to sizeof(sqldaid, sqldabc, sqln,
				** sqld) + sqln*sizeof(sqlvar) */
    i2		sqln;		/* Number of sqlvar's (set by user) */
    i2		sqld;		/* Number of result columns for SELECT
				** (set by LIBQ) and should be <= sqln
				** for executing a dynamic retrieval */
    struct sqvar {		/* Result column/Variable descriptor */
	i2	sqltype;	/* Type id of result column/variable */
	u_i2	sqllen;		/* Length of column/variable.  If type
				** is decimal then high byte is length
				** and low byte is scale */
	PTR	sqldata;	/* Address of variable for input/output 
				** Or pointer to IISQLHDLR structure for
				** user-defined datahandler.
				*/
	i2	*sqlind;	/* Address of null indicator */
	struct {		/* Varying length result column name */
	    i2	 sqlnamel;	/* Length of following column name */
# define IISQD_NAMELEN	34
	    char sqlnamec[IISQD_NAMELEN];	/* Result column name */
	} sqlname;
    } sqlvar[1];		/* Bogus array, allocated and initialized
				** by user */
} IISQLDA;


/*
** Name: IISQLHDLR - Sql DATAHANDLER Structure.
**
** Description:
**      The IISQLHDLR may be used to specify a user-defined datahandler for
**	transmitting data to/from the database. 
**
**	When an application sets the SQLTYPE field of an SQLVAR structure to
**	IISQ_HDLR_TYPE (internally DB_HDLR_TYPE) then the sqldata field is
**	expected to contain a pointer to an IISQLHDLR structure. The function
**	contained in the sqlhdlr field will be invoked with the optional 
**	argument contained in the sqlarg field.
**
**	This structure is provided in the EQSQLDA files of all host languages
**	except for MFcobol which does not support datahandlers.
**
** History:
**	19-oct-1992  (kathryn)
**		written.
*/
typedef struct {
	PTR	sqlarg;		/* optional argument to pass thru */
	int	(*sqlhdlr)();	/* user-defined function */
} IISQLHDLR;

#endif
