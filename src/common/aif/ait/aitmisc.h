/*
** Copyright (c) 2004 Ingres Corporation
*/





/*
** Name: aitmisc.h - API tester miscellanous function header file
**
** Description:
**	This file contains miscellanous functions shared by API tester
**	submodules.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	27-Mar-95 (gordy)
**	    Updated variables/functions exported by aitmisc.c
**	29-Mar-95 (gordy)
**	    Remove ifdef's around tracing.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/





# ifndef __AITMISC_H__
# define __AITMISC_H__





/*
** API tester tracing
*/

# include <tr.h>

# define AIT_TRACE(x)		if ( ait_isTrace(x) ) TRdisplay

/*
** AIT tracing levels
*/

# define AIT_TR_ERROR		1
# define AIT_TR_TRACE		2
# define AIT_TR_WARN		3
# define AIT_TR_INFO		4





/*
** External Functions
*/

FUNC_EXTERN II_PTR 	ait_malloc( i4  size );
FUNC_EXTERN bool 	ait_openfile( char *filename, 
				      char *mode, 
				      FILE **fptrptr );

FUNC_EXTERN VOID 	ait_closefile( FILE *fptr );
FUNC_EXTERN VOID 	ait_exit();
FUNC_EXTERN VOID 	free_calltrackList();
FUNC_EXTERN VOID 	free_dscrpList();
FUNC_EXTERN VOID 	free_datavalList();
FUNC_EXTERN VOID	ait_initTrace( char* tracefile );
FUNC_EXTERN VOID	ait_termTrace( VOID );
FUNC_EXTERN bool	ait_isTrace( i4  currentTrace ); 





# endif				/* __AITMISC_H__ */
