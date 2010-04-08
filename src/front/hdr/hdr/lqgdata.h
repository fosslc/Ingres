
/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	lqgdata.h  - declare global data defined in embed!libq
**
** Description:
**	The data defined in embed!libq, and needed outside the LIBQLIB 
**	shared image, is declared here.  It's collected into one structure
**	to reduce the number of transfer vectors needed in VMS.
**
** History:
**	11/30/89 (Mike S)	Initial Version.
**	11/02/90 (dkh) - Changed IILIBQgdata to IILQgdata().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*}
** Name:	LIBQGDATA - global data defined in embed!libq
**
** Description:
**	All the global data defined in LIBQ and known by the forms system and 
**	other front-ends is collected into this structure.
**
** form_on - Form system is being used
**
** os_errno - Copy of Operating System error when starting the back end.
**
** win_err - Function pointer to be called indirectly to display error
**             messages in a screen-interactive mode.
** Inputs:
**      buffer  - char * - Error message.
**
** f_end - Function pointer to be called indirectly to set or reset the
**           screen mode in the form system.  This is use when LIBQ needs to
**           print things in a normal mode (ie, trace data).
** Inputs:
**      flag    - i4    - TRUE  - End the screen mode,
**                        FALSE - Restore it.
**
** excep - Function pointer to be called indirectly in error processing.
** Inputs:
**      None
**
**
** History:
**	11/30/89 (Mike S)	Initial Version.
**	19-feb-92 (leighb) DeskTop Porting Change:
**		added _LIBQGDATA as tag for LIBQGDATA struct.
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/
typedef struct _LIBQGDATA		 
{
	i4     form_on;    	/* Forms flag */
	i4     os_errno;       /* Remembers OS errors */
	VOID    (*f_end)(bool);    	/* Set/Reset forms mode */
	void     (*win_err)(char *);  	/* FRS error function */
	i4     (*excep)(void);    	/* Error exception routine */
} LIBQGDATA;

FUNC_EXTERN	LIBQGDATA *IILQgdata();
