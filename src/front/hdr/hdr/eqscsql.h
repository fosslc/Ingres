/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	eqscsql.h	- Header for ESQL only scanner modules.
**
** Description:
**	This file contains prototypes for lanugage specific scanner 
**	functions used only by the ESQL preprocessor. 
**
** History:
**	13-feb-1993		Written (teresal)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Routines local to ESQL scanner. These routines are defined for each
** ESQL language in the language scanner file.
*/
FUNC_EXTERN bool sc_scanhost(i4 scan_term);		/* Scan host code */
FUNC_EXTERN bool sc_eatcomment(char **cp, bool eatall);	/* Eat host comment */
FUNC_EXTERN bool sc_inhostsc(void);			/* Comment or string?*/ 
