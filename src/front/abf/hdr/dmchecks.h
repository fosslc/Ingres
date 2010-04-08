/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	dmchecks.h - Definitions needed by Dependency Manager callers
**
** Description:
**	Define structures and "magic cookies" needed to call the dependency
**	manager.
**
**	Includers of this file must include <tm.h>
**
** History:
**	7/90 (Mike S) Initial version
**	02-dec-91 (leighb) DeskTop Porting Change: created global variables
**	6-Feb-1992 (blaise) Moved global variables created in previous 
**		change to abf!abfglobs.h.
**	6-Feb-92 (blaise) Moved definition of IIAMABFIAOM back from abfglobs.h
**/

/* 
** Use this before deleting a component, to see if it's OK.
** The argument is of the type bool 
*/
GLOBALCONSTREF PTR IIAMzdcDelCheck;

/* 
** Use this when a component changes, to update dependencies. 
** No argument required.
*/
GLOBALCONSTREF PTR IIAMzccCompChange;

/* 
** Use this to see if a frame must be recompiled.  
** The argument is of type DM_DTCHECK_ARG
*/
GLOBALCONSTREF PTR IIAMzdcDateChange;

typedef struct
{
	SYSTIME *frame_time;	/* Frame's IL date */
	bool	recompile;	/* set to TRUE if it must be recompiled */
	ER_MSGID msg;		/* Message to issue */
	char * name;		/* Name of object which forces recompilation */
} DM_DTCHECK_ARG;

/* 
** Use this to see if adding a record attribute would lead to self-inclusion.
** The argument is of type DM_RACHECK_ARG
*/
GLOBALCONSTREF PTR IIAMzraRecAttCheck;

typedef struct
{
	char *attr_type;	/* Attribute type */
	bool self;		/* Will be set TRUE for self-inclusion */
} DM_RACHECK_ARG;

/*
** Compile the tree down from a starting frame.
** The argument is of type DM_COMPTREE_ARG
*/
GLOBALCONSTREF PTR IIAMzctCompTree;

typedef struct
{
	PTR	tstlnk;		/* Test Link structure (really an ABLNKTST *) */
	STATUS	rval;		/* Status of all compiles */
	STATUS	(*compile)();	/* Compilation routine */
} DM_COMPTREE_ARG;

/*
**	Structure to hold addresses to be passed from IAOM back to ABF.
**	
**	History:
**
**	02-dec-91 (leighb) DeskTop Porting Change: Created
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes, add const modifiers..
*/

typedef struct IIAMABFIAOM	{
	PTR	IIamClass;
	PTR	IIamProcCl;
	PTR	IIamGlobCl;
	PTR	IIamRecCl;
	PTR	IIamRAttCl;
	PTR	IIamConstCl;
	const PTR *	IIAMzdcDelCheck;
	const PTR *	IIAMzccCompChange;
	const PTR *	IIAMzctCompTree;
	const PTR *	IIAMzdcDateChange;
	const PTR *	IIAMzraRecAttCheck;
}	IIAMABFIAOM;
