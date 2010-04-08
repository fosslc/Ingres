/*
**  ftprompt.c
**
**  Borrowed from fdprompt (dkh)
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	10/xx/84 (dkh) - Original version.
**	12/24/86 (dkh) - Added support for new activations.
**	03/05/87 (dkh) - Added support for ADTs.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	08/14/87 (dkh) - ER changes.
**	09-nov-88 (bruceb)
**		FRS_EVCB event is set by FTuserinp().  No need to touch it
**		in these routines.
**	13-mar-89 (bruceb)
**		Set timeout value of the evcb if the control block being
**		passed in is NULL.  (For all three routines.)
**	30-apr-90 (bruceb)
**		Added locator support for prompts.
**	10-dec-92 (leighb) DeskTop Porting Change:
**		Use Windows DialogBox for Prompting if DLGBOXPRMPT defined.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



# include	<compat.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<frsctblk.h>
# include	<er.h>
# ifdef	DLGBOXPRMPT
# include	<wn.h>
# endif

FUNC_EXTERN	u_char	*FTuserinp();
FUNC_EXTERN	VOID	IIFTsplSetPromptLoc();

i4	FTprmthdlr();

GLOBALREF	FRS_EVCB	*IIFTevcb;


FTneprompt(prompt, resp, frscb)
char		*prompt;
DB_DATA_VALUE	*resp;
FRS_CB		*frscb;
{
	FRS_EVCB	evcb;
	FRS_EVCB	*evcbp;
	u_char		buf[256];
	u_char		*cp;
	u_char		*txtbuf;
	i4		retval;
	i4		respsize;
	i4		count = 0;
	DB_TEXT_STRING	*text;

	if (frscb == NULL)
	{
		evcbp = &evcb;
		if (IIFTevcb != NULL)
			evcbp->timeout = IIFTevcb->timeout;
		else
			evcbp->timeout = 0;
	}
	else
	{
		evcbp = frscb->frs_event;
	}
	retval = FTprmthdlr(prompt, buf, FALSE, evcbp);

	/*
	**  For now, just assume that the DB_DATA_VALUE passed
	**  down is of type LONG_TEXT.  This should be fairly
	**  safe.
	*/
	text = (DB_TEXT_STRING *) resp->db_data;
	txtbuf = text->db_t_text;
	respsize = resp->db_length - sizeof(u_i2);
	cp = buf;
	while (count < respsize && *cp)
	{
		*txtbuf++ = *cp++;
		count++;
	}
	text->db_t_count = count;
	return(retval);
}



FTprompt(prompt, resp, frscb)		/* PROMPT: */
char		*prompt;		/* user prompt */
DB_DATA_VALUE	*resp;			/* response buffer */
FRS_CB		*frscb;
{
	FRS_EVCB	evcb;
	FRS_EVCB	*evcbp;
	u_char		buf[256];
	u_char		*cp;
	u_char		*txtbuf;
	i4		retval;
	i4		respsize;
	i4		count = 0;
	DB_TEXT_STRING	*text;

	if (frscb == NULL)
	{
		evcbp = &evcb;
		if (IIFTevcb != NULL)
			evcbp->timeout = IIFTevcb->timeout;
		else
			evcbp->timeout = 0;
	}
	else
	{
		evcbp = frscb->frs_event;
	}
	retval = FTprmthdlr(prompt, buf, TRUE, evcbp);

	/*
	**  For now, just assume that the DB_DATA_VALUE passed
	**  down is of type LONG_TEXT.  This should be fairly
	**  safe.
	*/
	text = (DB_TEXT_STRING *) resp->db_data;
	txtbuf = text->db_t_text;
	respsize = resp->db_length - sizeof(u_i2);
	cp = buf;
	while (count < respsize && *cp)
	{
		*txtbuf++ = *cp++;
		count++;
	}
	text->db_t_count = count;
	return(retval);
}


i4
FTprmthdlr(prompt, buf, echo, evcb)
char	*prompt;
u_char	*buf;
i4	echo;
FRS_EVCB *evcb;
{
	FRS_EVCB	aevcb;
	reg u_char	*tp;	/* temporary bufr ptr */

	if (evcb == NULL)
	{
		evcb = &aevcb;
		if (IIFTevcb != NULL)
			evcb->timeout = IIFTevcb->timeout;
		else
			evcb->timeout = 0;
	}
# ifdef	DLGBOXPRMPT
	if (WNDlgBoxPrompt(prompt, buf, echo, evcb->timeout) != OK)
		evcb->event = fdopTMOUT;
# else
	IIFTsplSetPromptLoc(stdmsg);
	tp = FTuserinp(stdmsg, prompt, FT_PROMPTIN, echo, evcb);
	STcopy(tp, buf);
#endif	/* DLGBOXPRMPT */

	(*FTdmpmsg)(ERx("PROMPT: %s\n"), prompt );
	(*FTdmpmsg)(ERx("PROMPT RESPONSE: %s\n"), buf );

	return(STcompare((char *) buf, ERx("")) != 0);
}
