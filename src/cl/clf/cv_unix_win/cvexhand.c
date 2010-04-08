/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cs.h>
# include	<gl.h>
# include	<cv.h>
# include	<mh.h>
# include	<me.h>
# include	<clconfig.h>
# include	<meprivate.h>
# include	<ex.h>

/**
** Name: CVEXHAND.C - exception handler for cv module.
**
** Description:
**      This file contains the following external routines:
**    
**	CVexhandler()	   -  exception handler for cv module.
**
** History:
 * Revision 1.1  90/03/09  09:14:23  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:39:43  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:45:28  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:05:12  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:11:20  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:28:45  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/10  17:09:40  mikem
**      fix template comments
**      
**      Revision 1.2  87/11/10  12:37:13  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.3  87/07/27  11:23:27  mikem
**      Updated to conform to jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	08-jun-1995 (canor01)
**	    added thread-local storage for cv_errno in MCT server
**	03-jun-1996 (canor01)
**	    modified thread-local storage for used with operating-system
**	    threads
**      22-nov-1996 (canor01)
**          Changed names of CL-specific thread-local storage functions
**          from "MEtls..." to "ME_tls..." to avoid conflict with new
**          functions in GL.  Included <meprivate.h> for function prototypes.
**	19-feb-1999 (matbe01)
**	    Move include for ex.h to end of list. The check for "ifdef
**	    EXCONTINUE" does no good if sys/context.h (via meprivate.h) is
**	    included after ex.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-Jul-2004 (lakvi01)
**		SIR# 112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
# ifdef OS_THREADS_USED
GLOBALREF ME_TLS_KEY	cv_errno_key;
# endif /* OS_THREADS_USED */
GLOBALREF i4		cv_errno;

/* statics */


/*{
** Name: CVexhandler - Handler for overflow and underflow conditions.
**
** Description:
**
**    Will handle math underflow and overflow exceptions raised in CV routines.
**
** Inputs:
**      exargs				    exception info block. 
**					    exargs->exarg_num is the number of 
**					    the exception raised
**
** Output:
**	none.
**
**
**      Returns:
**	    Sets global variable cv_errno to CV_UNDERFLOW or CV_OVERFLOW.
**	    After setting cv_errno the handler will do a return EXDECLARE.
**
** Side Effects:
**          cv_errno is set to error status indicated by the exception.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
*/
i4
CVexhandler(exargs)
EX_ARGS		*exargs;
{
	i4	status;
	i4	*cv_errno_ptr = NULL;

# ifdef UNIX
# ifdef OS_THREADS_USED
	ME_tls_get( cv_errno_key, &cv_errno_ptr, &status );
	if ( status )
		cv_errno_ptr = &cv_errno;
	if ( cv_errno_ptr == NULL )
	{
		cv_errno_ptr = (i4 *) MEreqmem( 0, sizeof(i4), 0, NULL );
		ME_tls_set( cv_errno_key, cv_errno_ptr, &status );
	}
# else  /* OS_THREADS_USED */
	cv_errno_ptr = &cv_errno;
# endif /* OS_THREADS_USED */
# endif /* UNIX */


	switch (exargs->exarg_num)
	{
		case EXDECLARE:
			return (0);

		case EXFLTOVF:
		case EXINTOVF:	
		case EXFLOAT:	
			*cv_errno_ptr = CV_OVERFLOW;
			return (EXDECLARE);

		case EXFLTUND:
			*cv_errno_ptr = CV_UNDERFLOW;
			return (EXDECLARE);

		case MH_BADARG:
		case MH_MATHERR:
		case MH_PRECISION:
		case MH_INTERNERR:
			*cv_errno_ptr = FAIL;
			return (EXDECLARE);

		default:
			*cv_errno_ptr = FAIL;
			return (EXRESIGNAL);
	}
}
