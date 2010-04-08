/*
**	iiprompt.c
*/

/*
**	IIprompt - Prompt the user at the bottom of the screen.
**
**	These routines prompt the user to fill in the buffer at the
**	bottom of the screen.
**
**	Copyright (c) 2004 Ingres Corporation
**
**  History:
**	xx/xx/83 - JEN - Original version
**	09/03/85 - DKH - Added IIneprompt to do no-echo prompting.
**	12/24/86 (dkh) - Added support for new activations.
**	01/14/87 (dkh) - Added better documentation for IInprompt.
**	02/26/87 (drh) - Modified for ADTs and NULLs.
**	03/30/87 (drh) - Added new parameters to fmt_cvt call.
**	19-jun-87 (bruceb)	Code cleanup.
**	13-jul-87 (bruceb)	Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	09/29/87 (dkh) - Changed iiugbma* to IIUGbma*.
**	09/30/87 (dkh) - Added procname as param to IIUGbma*.
**	07/21/88 (dkh) - Fixed to set length of returned value
**			 for popup prompt.
**	06-dec-88 (bruceb)
**		Initialize event to fdNOCODE before calling FT prompt
**		routines.  On return, code should be either fdNOCODE or
**		fdopTMOUT.
**	10-mar-89 (bruceb)
**		Splitting up 'event' and 'last command'.  Now, event doesn't
**		change here.  It's the last command that gets set to fdNOCODE
**		or fdopTMOUT.
**	01/11/90 (dkh) - Before doing prompt, check to see if we need to
**			 do a redisplay.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	08/15/91 (dkh) - Put in changes to not redraw entire screen when
**			 executing a prompt in the (runtime) context
**			 of an initialize block.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	<frserrno.h>
# include	<afe.h>
# include	<er.h>
# include	<fsicnsts.h>
# include	<erfi.h>
# include	<lqgdata.h>
# include	"iigpdef.h"
# include	<rtvars.h>


FUNC_EXTERN	STATUS	adh_dbcvtev();

# define	MAXPROMPT	200

/*{
** Name:	IIprmptio	- Interface for forms ## prompt statement.
**
** Description:
**	This routine is the new interface for the EQUEL/ESQL/FORMS
**	"prompt" [noecho] statement in INGRES version 6.0.
**	Advantages of this interface are:
**	  1) it will allow us to support putting results into
**	     DB_DATA_VALUES, (eventually) VARCHAR as well as
**	     normal character data types.
**	  2) it allows a NULL response to a prompt
**	  3) collapses two separate calls (normal and noecho prompt)
**	     into one.
**
**	The current code really relies on IIprmthdlr() to do the real work
**	and then converts the result to the appropriate data type, if
**	necessary.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	noecho		0 for echoing input, 1 to prevent echoing of input.
**	pstring		The prompt string to display
**	nullind		Ptr to a null indicator, if any
**	isvar		Is result buffer a real variable, an error if not 1
**	type		The data type of the result variable. Should be
**			one of the character types, or a DBV.
**	length		Length of result variable.  Currently is always 0
**			but will be non-zero for DB_VCH_TYPE.
**
** Outputs:
**	data		Pointer (address) to result variable where user
**			input will be placed.
**	Returns:
**		Nothing.
**	Exceptions:
**		None.
**
** Example and Code Generation:
**	## DB_DATA_VALUE 	dbv;
**	## prompt noecho (message_buf, dbv);
**
**	IIprmptio(NOECHO, message_buf, 0, ISREF, DB_DBV_TYPE, 0, &dbv);
**
** Side Effects:
**	None.
**
** History:
**	01/10/87 (dkh) - Initial version.
**	24-feb-1987	Added null indicator parameter, and real ADT/NULL
**			support.  Renamed from IInprompt (drh)
**	02/15/91 (tom) - increased the prompt size to 2000 bytes to better
**			accomondate larger popup prompts.
*/
VOID
IIprmptio(i4 noecho, char *prstring, i2 *nullind, i4 isvar, i4 type, i4 length, PTR data)
{

	DB_EMBEDDED_DATA	edv;
	i4			echo = TRUE;

	if (noecho)
	{
		echo = FALSE;
	}

	/*  
	**  Build an EDV from the user's data
	*/

	edv.ed_type = type;
	edv.ed_length = length;
	edv.ed_data = (PTR) data;
	edv.ed_null = nullind;

	/*
	**  Issue the prompt and get a response
	*/

	IIprmthdlr( prstring, &edv, echo );

}

/*{
** Name:	IIneprompt	-	Issue noecho prompt
**
** Description:
**	This is a compatability cover for pre-6.0 EQUEL.
**
** Inputs:
**	promptstr	Prompt string
**	retbuf		Buffer to hold the response
**
** Outputs:
**	Retbuf will be updated with the prompt response
**
** Returns:
**	VOID
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	26-feb-1987	Converted to a cover routine (drh)
*/
VOID
IIneprompt(promptstr, retbuf)
char	*promptstr;
char	*retbuf;
{
	 IIprmptio( (i4) TRUE, promptstr, (i2 *) NULL, (i4) 1,
		(i4) DB_CHR_TYPE, (i4) 0, retbuf );
}

/*{
** Name:	IIdoprompt	-	Issue prompt
**
** Description:
**	This is a compatability cover for pre-6.0 EQUEL.
**
** Inputs:
**	promptstr	Prompt string
**	retbuf		Buffer to hold the response
**
** Outputs:
**	Retbuf will be updated with the prompt response
**
** Returns:
**	VOID
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	26-feb-1987	Converted to a cover routine (drh)
*/

VOID
IIdoprompt(promptstr, retbuf)
char	*promptstr;
char	*retbuf;
{
	 IIprmptio( (i4) FALSE, promptstr, (i2 *) NULL, (i4) 1,
		(i4) DB_CHR_TYPE, (i4) 0, retbuf );
}

/*{
** Name:	IIprmthdlr	-	Issue prompt, get response
**
** Description:
**	This is the routine that actually 'does the work' of issuing
**	a prompt and converting the response into the variable(s) provided
**	by the caller.
**
**	It copies the prompt string, and builds a db-data-value to hold
**	the prompt response (as a long text).  Then, it calls the appropriate
**	FT routine to issue an echo or no-echo prompt.  
**
**	The prompt response is converted to a 'c' type using a default format,
**	to capture information about whether the response was the 'null' string,
**	if the embedded-data-value provided by the caller included a null
**	indicator.  Finally, the response is converted to the embedded-
**	data-value.
**
** Inputs:
**	promptstr	The prompt string
**	edvptr		Ptr to embedded-data-value to update with response
**	echo		If TRUE, echo the prompt response
**
** Outputs:
**	The embedded-data-value pointed to by edvptr will be updated with
**	the prompt response, including the null indicator if one was
**	provided.
**
** Returns:
**	i4		OK    Prompt was issued successfully, response returned
**			FAIL  An error occurred in issuing the prompt or in
**			      converting the response.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	26-feb-1987	Rewritten for NULL and ADT support (drh)
**	02-apr-1987	Added IIftrim call to trim trailing blanks (drh)
*/

i4
IIprmthdlr(char *promptstr, DB_EMBEDDED_DATA *edvptr, i4 echo)
{
	AFE_DCL_TXT_MACRO( MAXPROMPT  )	resbuf;
	DB_TEXT_STRING		*resp;
	char			fmtstr[ MAX_FMTSTR ];
	i4			areasize;
	FMT			*fmtptr;
	char			pbuf[2005];
	char			*fstr;
	register char		*pstr;
	DB_DATA_VALUE		respdbv;
	DB_DATA_VALUE		cnvtdbv;
	DB_DATA_VALUE		dbvcop;
	DB_DATA_VALUE		*dbvptr;
	PTR			fmtarea;
	ADF_CB			*cb;
	i2			tag;
	i4			retval;
	POPINFO			pop;
	i4			style;
	i4			event;

	if (promptstr == NULL )
		return (FAIL);

	if (!IILQgdata()->form_on)
	{
		IIFDerror(RTNOFRM, 0, NULL);
		return (FAIL);
	}

	style = GP_INTEGER(FSP_STYLE);
	pop.begy = GP_INTEGER(FSP_SROW);
	pop.begx = GP_INTEGER(FSP_SCOL);
	pop.maxy = GP_INTEGER(FSP_ROWS);
	pop.maxx = GP_INTEGER(FSP_COLS);

	IIFRgpcontrol(FSPS_RESET,0);

	cb = FEadfcb();

	respdbv.db_datatype = DB_LTXT_TYPE;
	respdbv.db_length = sizeof( resbuf );
	respdbv.db_prec = 0;
	resp = (DB_TEXT_STRING *) &resbuf;
	respdbv.db_data = (PTR) resp;
	dbvptr = &respdbv;

	pstr = IIstrconv(II_TRIM, promptstr, pbuf, (i4)2000);
	STcat(pstr, ERx(" "));


	event = IIfrscb->frs_event->event;
	IIfrscb->frs_event->event = fdNOCODE;

	/*
	**  Check to see if we need to do a redisplay before
	**  doing the prompt.
	*/
	if (IIstkfrm && !(IIstkfrm->rfrmflags & RTACTNORUN) &&
		IIstkfrm->fdrunfrm->frmflags & fdDISPRBLD)
	{
		IIredisp();
	}

	if (style == FSSTY_POP)
	{
		if (IIFRbserror(&pop, MSG_MINROW, MSG_MINCOL,
			ERget(F_FI0002_Prompt)))
		{
			/* Restore event and set 'last command' to fdNOCODE. */
			IIfrscb->frs_event->lastcmd = fdNOCODE;
			IIfrscb->frs_event->event = event;
			return (FAIL);
		}
		IIFTpprompt(pstr, echo, &pop, IIfrscb, resbuf.text);
		resbuf.count = STlength((char *) resbuf.text);
	}
	else
	{
		if (echo)
		{
			FTprompt(pstr, &respdbv, IIfrscb);
		}
		else
		{
			FTneprompt(pstr, &respdbv, IIfrscb);
		}
		FTmessage(ERx(" "), FALSE, FALSE);
	}

	/* Restore event and set the 'last command' (fdNOCODE or fdopTMOUT). */
	IIfrscb->frs_event->lastcmd = IIfrscb->frs_event->event;
	IIfrscb->frs_event->event = event;

	/*
	**  If the caller provided a null indicator, take extra steps to
	**  check whether the prompt response was the 'null string'.
	*/

	tag = FEgettag();
	retval = OK;

	if ( edvptr->ed_null != NULL )
	{
		/*
		**  Build a DB_DATA_VALUE to hold the prompt response converted
		**  from a longtext to a 'c' datatype.  We make it a 'c' type 
		**  because prompts only allow printing characters as a 
		**  response. The conversion is necessary because the 
		**  response may have been the 'null' string.
		*/

		dbvptr = &cnvtdbv;
		cnvtdbv.db_datatype = DB_CHR_TYPE;
		cnvtdbv.db_length = MAXPROMPT;
		cnvtdbv.db_prec = 0;

		AFE_MKNULL_MACRO( &cnvtdbv );
	
		if ((cnvtdbv.db_data = FEreqmem((u_i4)tag,
		    (u_i4)cnvtdbv.db_length, TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIprmpthdlr"));
		}

		/*  Get a default format string */

		fstr = fmtstr;

		if ( retval == OK)
		{
			retval = fmt_deffmt( cb, dbvptr, (i4) 
				cnvtdbv.db_length, (bool) FALSE, fstr );
		}

		/*
		**  Get the size requirements of a format structure for
		**  the default format, and allocate an area for it.
		*/

		if ( retval == OK )
		{
			retval = fmt_areasize( cb, fstr, &areasize );
		}

		if ( retval == OK )
		{
			if ((fmtarea = FEreqmem((u_i4)tag, (u_i4)areasize,
			    TRUE, (STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(ERx("IIprmpthdlr"));	
			}
		}

		/*
		**  Build a format structure from the default format string
		*/

		if ( retval == OK )
		{
			retval = fmt_setfmt( cb, fstr, fmtarea, &fmtptr,
				(i4 *) NULL );
		}

		/*
		**  Convert the prompt response (which is a longtext) into a
		**  nullable 'c' type.  This conversion takes care of any
		**  recognition of the null string that is required.
		*/

		if ( retval == OK )
		{
			retval = fmt_cvt( cb, fmtptr, &respdbv, dbvptr,
				(bool) FALSE, (i4) 0);
		}
	}

	/*
	**  Trim trailing blanks from the prompt resonse, if necessary.
	*/

	if ( retval == OK )
	{
		if ( ( IIftrim( dbvptr, &dbvcop ) ) != OK )
		{
			retval = FAIL;
		}
	}

	/*
	**  Convert either the longtext or c prompt response to the 
	**  embedded-data-value that the caller provided.
	*/

	if ( retval == OK )
	{
		retval = adh_dbcvtev( cb, &dbvcop, edvptr );
	}

	if ( retval != OK )
	{
		IIFDerror(RTPMERR, 0);
	}

	FEfree(tag);
	return( retval );
}
