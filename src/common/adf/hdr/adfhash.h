/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ADFHASH.H -	Contains all of the constants for hash preparation
**			on "RTI known" datatypes.
**
** Description:
**        Contains all of the constants necessary for hash preparation
**	on "RTI known" datatypes.
**
** History: $Log-for RCS$
**      19-jun-86 (ericj)
**	    Initial creation.
[@history_line@]...
**/

/*
[@forward_type_references@]
[@forward_function_references@]
*/


/*
**  Defines of other constants.
*/

/*
**  Define constants for hashprep result value types.
*/
/*  Currently, we hash the last 10 bytes of a date value */
#define                 AD_DTE_HPLEN    10
/*  We hash the entire money data value as an f8. */
#define			AD_MNY_HPLEN	sizeof (f8)

/*
[@global_variable_references@]
[@type_definitions@]
*/
