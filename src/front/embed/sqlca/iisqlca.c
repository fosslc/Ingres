/*
** Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include 	<iisqlca.h>

/**
+*  Name: iisqlca.c - Runtime declaration of the SQLCA 
**
**  Description:
**	The global declaration of the SQLCA is contained in this file.  
**	The file constitutes the iisqlca library, with which all embedded 
**	programs must link.  The SQLCA is global so that (on most machines) 
**	the SQLCA pointer being passed to the SQL run-time system is the same
**	across different files and even different languages.
**
**	VMS Note: Even on VMS where GLOBALDEF is the rule for declarations,
**		  we do not use the GLOBALDEF storage class, as we must make
**	this visible to user's "extern" reference (who do not go through the
**	compat.h which redefines that class to globalref).  In fact, the 
**	ESQL/C preprocessors on VMS, running under the compatlib (with the -c
**	flag) make sure that the SQLCA is extern and not globalref.
**
**	When an embedded program declares the use of the SQLCA, the
**	preprocessor generates an external (or common) declaration of
**	the global SQLCA.  Embedded programs use the definition (or typedef)
**	of the SQLCA contained in the "eqsqlca.{h,def}" file.
**
**	When LIBQ references the SQLCA, it relies on a typedef contained
**	in the file iisqlca.h and a pointer passed in from the user program.
**
**	The SQLCA structure is the status area in which SQL errors,
**	warnings and return codes are stored.  The global definition
**	This structure contains the status information that is returned
**	to the embedded program after each executable SQL statement.  The
**	SQLCA is updated during the execution of every database-related
**	SQL statement.  By inspecting the SQLCA the program can take
**	appropriate action in response to errors or exceptional conditions
**	that occurred in the most recently executed SQL statement.
**	The original definition of this structure (for PL/I) is described
**	in the DB2 programming guide.
**
LIBOBJECT = iisqlca.c
**
**
-*
**  History:
**	13-apr-1987	- written (bjb)
**      12-jun-1996 (sarjo01)
**              Bug 76373: Use GLOBALDEF for NT to assure correct
**              linkage using DLL.
**	29-Sep-2004 (drivi01)
**	    Added LIBOBJECT hint which creates a rule
**	    that copies specified object file to II_SYSTEM/ingres/lib
**/
#ifdef NT_GENERIC
    GLOBALDEF IISQLCA	sqlca = { {'S', 'Q', 'L', 'C', 'A', ' ', ' ', ' '},
			  sizeof(IISQLCA) };
#else
/* NOT GLOBALDEF (see note above) */
    IISQLCA	sqlca = { {'S', 'Q', 'L', 'C', 'A', ' ', ' ', ' '},
			  sizeof(IISQLCA) };
#endif
