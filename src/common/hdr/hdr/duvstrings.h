/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DUVSTRINGS.H - string definitions that are used in error messages.
**
** Description:
**        This header contains string definitions that are used in
**	the 5.0 phase of CONVTO60 error messages.  This strings have
**	been placed here so that they can easily be redefined for
**	internationalization of error messages.
**
** History: $Log-for RCS$
**      30-Jun-87 (ericj)
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
**      Names of objects used in error messages to specify that an object
**	is owned by a non-existent Ingres user.
*/
#define                 DUV_TABLE	    "table"
#define                 DUV_FORM 	    "form"
#define                 DUV_REPORT	    "report"
#define                 DUV_QBFNAME	    "qbfname"
#define                 DUV_JOINDEF	    "joindef"
#define                 DUV_GRAPH	    "GBF graph"
#define                 DUV_VIGRAPH_OBJ	    "VIGRAPH object"

/*
**      The following is a list of CL routine names.  These are used
**	in the general CL failure error messages, but should not be
**	changed for internationalization.
*/
#define                 DUV_CKRESTORE   "CKrestore()"
#define                 DUV_CKSAVE	"CKsave()"
#define                 DUV_LOCOPY	"LOcopy()"
#define			DUV_LOCREATE	"LOcreate()"
#define                 DUV_LODELETE    "LOdelete()"
#define                 DUV_LOFADDPATH	"LOfaddpath()"
#define                 DUV_LOFROMS	"LOfroms()"
#define                 DUV_LOFSTFILE	"LOfstfile()"
#define                 DUV_LOINGPATH	"LOingpath()"
#define			DUV_SICLOSE	"SIclose()"
#define			DUV_SIFLUSH	"SIflush()"
#define			DUV_SIFOPEN	"SIfopen()"
#define			DUV_SIFSEEK	"SIfseek()"
#define			DUV_SIOPEN	"SIopen()"
#define			DUV_SIREAD	"SIread()"
#define			DUV_SIWRITE	"SIwrite()"


/*
[@group_of_defined_constants@]...
*/
#define			DUV_RESUMING	"Resuming"
#define			DUV_STARTING	"Starting"
#define			DUV_CONVERSION	"Conversion"
/*
[@global_variable_references@]
[@type_definitions@]
*/
