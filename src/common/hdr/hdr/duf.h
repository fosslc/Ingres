/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DUF.H -    Definitions and typedefs that are used by most database
**		    utilities.
**
** Description:
**        The definitions and typedefs in this header are used by most
**	of the database utilities.
**
**	Dependencies:
**	    dbms.h
**
** History: $Log-for RCS$
**      16-Oct-86 (ericj)
**          Initial creation.
[@history_template@]...
**/
/*
[@forward_type_references@]
[@forward_function_references@]
*/


/*
**  Defines of other constants.
*/

/*
**	Define constants related to frontend IO.
*/
#define                 DU_MAXLINE      256

/*
**  Define a synonym for the C construct "static".  When the code is being
**  developed, this will be set to the null string, so the routine will be
**  visible through the debugger.  In production, it should be defined to
**  be "static".
*/
#define                 STATIC

/*
**      Define the sizes for various object types.
*/
#define                 DU_LOC_SIZE     sizeof (DB_LOC_NAME)
#define			DU_USR_SIZE	sizeof (DB_OWN_NAME)

/*
[@group_of_defined_constants@]...
*/

/*
[@group_of_defined_constants@]...
*/

/*
[@global_variable_references@]
*/


/*
[@type_definitions@]
*/
