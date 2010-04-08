/*
** Copyright (c) 2004 Ingres Corporation
**
**  Name: pmutil.h -- header for non-GL PM functions 
**
**  Description:
**	The source for the following functions is contained in pm.c
**
**  History:
**	20-oct-92 (tyler)
**		Created.
**	24-may-93 (tyler)
**		Added argument to PMmInit().  Removed prototype for
**		PMexpToRegExp() which has moved to pm.h.  Added prototypes
**		for PMmRestrict() and PMrestrict().
**	21-jun-93 (tyler)
**		Changed the interfaces of multiple context PM functions
**		to use caller allocated control blocks rather than
**		memory tags and static arrays to maintain state.
**	23-jul-93 (tyler)
**		Modified prototype for PMmInit().  Moved prototypes
**		for published multiple context functions to
**		ingres!main!generic!gl!hdr!hdr!pm.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN char	*PMmExpandRequest( PM_CONTEXT *, char* );
FUNC_EXTERN char	*PMexpandRequest( char* );
FUNC_EXTERN char	*PMgetElem( i4, char * );
FUNC_EXTERN char	*PMmGetElem( PM_CONTEXT *, i4, char * );
FUNC_EXTERN void	PMlowerOn( void );
FUNC_EXTERN void	PMmLowerOn( PM_CONTEXT * );
FUNC_EXTERN i4		PMnumElem( char * );
FUNC_EXTERN i4		PMmNumElem( PM_CONTEXT *, char * );
FUNC_EXTERN i4		PMmRestrict( PM_CONTEXT *, char * );
FUNC_EXTERN i4		PMrestrict( char * );
FUNC_EXTERN STATUS	PMwrite( LOCATION * );
FUNC_EXTERN STATUS	PMmWrite( PM_CONTEXT *, LOCATION * );

