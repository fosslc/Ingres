/*
** Copyright (c) 2004 Ingres Corporation
*/





/*
** Name: aitproc.h - aitproc.c shared variables and functions header file
**
** Description:
**	API tester main processing function declarations.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	27-Mar-95 (gordy)
**	    Added tester control block.
**	31-Mar-95 (gordy)
**	    Added function pointer for messaging routine
**	    to handle output to standard output.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/





# ifndef __AITPROC_H__
# define __AITPROC_H__





typedef struct
{

    i4			state;		/* Tester execution state */

#define	AIT_DONE	0	/* Tester has finished */
#define AIT_PARSE	1	/* Reading/Parsing input file */
#define AIT_EXEC	2	/* Executing an API function */
#define	AIT_WAIT	3	/* Waiting for async API function completion */

    CALLTRACKLIST	*func;		/* Function info for AIT_EXEC */
    void		(*msg)();	/* Message display function */

} AITCB;





/*
** Global variables.
*/

GLOBALREF FILE		*inputf;
GLOBALREF FILE		*outputf;
GLOBALREF char    	*dbname;
GLOBALREF char    	*username;
GLOBALREF char    	*password;
GLOBALREF i4      	traceLevel;
GLOBALREF i4      	verboseLevel;





/*
** It would be better to make the control block
** local and pass it around as needed, but this
** will require a major amount of work.  Since
** we only have 1 control block, settle for
** global access.
*/

GLOBALREF AITCB		ait_cb;		/* AIT Control Block */





/*
** External Functions
*/

FUNC_EXTERN AITCB	*AITinit( int argc, char** argv, void (*msg)() );
FUNC_EXTERN bool	AITprocess( AITCB * );
FUNC_EXTERN VOID	AITterm( AITCB * );





# endif			/* __AITPROC_H__ */
