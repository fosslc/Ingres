/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cm.h>
#include	<st.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include        <er.h>
# include        <fe.h>
#include        <ug.h>
#include        <lo.h>

/**
** Name:	feclinit.c -	Front-End CL initialization routine
**
** Description:
**	Contains the routines needed to initialize all FE executables,
**	particularly the character set attribute table definition.  Defines:
**
**	IIUGinit()	front-end initialization routine.
**	IIUGcmInit()	initialize the character set attribute table.
**
** History:
**	Revision 6.4  90/9/10 stevet
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static	bool	charset_loaded = FALSE;

/*{
** Name:	IIUGinit() -	Front-End Initialization.
**
**	This routine is called by all front end executables.
** History:
**	9/90 (stevet) -- Created.
**      9/90 (stevet) -- Add ifdef to get around linking problem using old CL.
**                       ifdef should be removed when the ingres63p CL is 
**                       ready.
**     11/90 (stevet) -- Removed #ifdef CM_NOCHARSET.  Added error handling
**                       code.  
**
**	15-nov-91 (leighb) DeskTop Porting Change: Include <nm.h> & <st.h>.
**	03/21/93 (dkh) - Added check to load the character set only once.
**	6-oct-1993 (rdrane)
**		Add #include of fe.h, as ug.h now has a dependency with fe.h.
*/

STATUS
IIUGinit ()
{

/* Initialize Character Set Attribute Table */

	return IIUGcmInit();
}

/*{
** Name:	IIUGcmInit() -	Installing character set attribute table.
**
**History:
**	9/90 (stevet) -- Created.
**	7/91 (bobm) -- Changed to use II_CHARSET logical.
**	17-aug-91 (leighb) DeskTop Porting Change: 
**		Must pass address of struct clerr when calling ERlookup().
**	14-Jun-2004 (schka24)
**	    Use canned (safe) routine for charset setting.
*/

STATUS
IIUGcmInit ()
{
	CL_ERR_DESC	clerr;
        CL_ERR_DESC     sys_err;                /* Used to pass to ERlookup */
	STATUS		status;
        char            msg_buf[ER_MAX_LEN];
        i4              msg_len = ER_MAX_LEN;

	/*
	**  If this is the first time that we've been called, set
	**  boolean so that we don't do this more than once.
	**  If this is not the first call, just return OK even
	**  though the first call may have failed in some way.
	**  We do this so that the character set will not change
	**  once processing has begun.
	*/
	if (charset_loaded == FALSE)
	{
		charset_loaded = TRUE;
	}
	else
	{
		return(OK);
	}

	/* Set CM character set stuff */

	status = CMset_charset(&clerr);
        if ( status != OK)
	{
           if ( ERlookup(0, &clerr, ER_TIMESTAMP, NULL, msg_buf, msg_len,
             iiuglcd_langcode(), &msg_len, &sys_err, 0, NULL) != OK)
	   {
               /* No error message is found. */
               IIUGerr(E_UG0024_CharInitError, UG_ERR_ERROR, 1, "");
	   }
           else
	   {
               IIUGerr(E_UG0024_CharInitError, UG_ERR_ERROR, 1, msg_buf);
	   }

           return status;
	}

	return OK;

}
