/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<fstm.h>
# include	<qr.h>
# include	<cm.h>
# include	<er.h>
# include	<ermf.h>
# include	<ug.h>
# include	<me.h>

GLOBALREF QRB	*FS_qrb;	/* global ptr to QRB, needed by exc. handler */

/*
**    FSrecbe  --  Get one single record from backend
**
**    FSrecbe is called only by FSmore when it wants one record of output
**    from the backend.
**
**    Returns: An integer representing the length of the record being
**	       returned. A length of '-1' means the request is complete
**	       and there is no further output data.
**
**	History:
**	08/25/87 (scl) Changed CMS to FT3270
**	11/12/87 (dkh) - FTdiag -> FSdiag.
**	11-aug-88 (bruceb)
**		Added ricks' kanji changes.
**	11-oct-88 (kathryn)  Bug# 2763
**		Removed local static variable qrptr and added it to qrb
**		structure so it can be used by FSrun().
**	13-jul-89 (teresa) Bug# 6595
**		Moved qrend to be reset to zero right after the "End of 
**		Request" message is generated - qrend used to rely on another
**		call to FSrecbe to reset it to zero.  This caused a problem: 
**		if the no. of records of output exactly matched the rows in
**		FSmore, this routine wouldn't be called again, and thus, the
**		next go block's output would not be displayed correctly.
**	14-aug-89 (sylviap)  
**		Created FSnxtrow for scrollable output.
**	12-oct-89 (sylviap)  
**		Changed calls to FSaddrec to IIUFadd_AddRec.
**	04/14/90 (dkh) - Added dummy second argument to qrrun() call
**			 since qrrun() interface has changed.
**	06-11-92 (rogerl) - Use EOS as string term, not NULL. Removed unused
**			parameter to FSrecbe()
**      03-oct-1996 (rodjo04)
**              Changed "ermf.h" to <ermf.h>
**	20-mar-2000 (kitch01)
**		Bug 99136. When used with large pagesizes this routine can 
**		overflow the buffer area passed in. Amend to reallocate the buffer
**		if the record length of the retrieved row is larger than the 
**		existing buffer passed in. Function params now include bfrlen to
**		indicate the existing buffer length.
**      08-Nov-2000 (horda03)
**              Added ME.H, required following above change for MEreqmem.
**      05-Dec-2000 (horda03)
**              Another change for Bug 99136. Need to initialise recsize
**              just in case it isn't initialised by qrrun().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler caught bad call to STlength.
*/
i4
FSrecbe(bcb, bfr, qrb, bfrlen)
BCB	*bcb;
char	**bfr;
QRB	*qrb;
i4	bfrlen;
{
	register i4	len;
	register char	*bfptr;
	static i4	qrend;
	char		*qrrun();
	i4		recsize = 0; /* renamed from 'dummy' to be more appropriate */
	char	*bufr;

	FSdiag(ERx("FSrecbe entered\n"));

	/*
	** don't want to do this because the error's been flushed already
	** if (len = FSermsg(bcb,bfr))
	**     return(len);
	*/

	if (bcb->req_complete)
	    FSdiag(ERx("FSrecbe: complete=%d!!!\n"), bcb->req_complete);

	if (bcb->req_complete)
	{
	    /* qrend = 0;	Bug 6595 */
	    return(-1);
	}

	len   = 0;


	/* check for pending output -- if none run QR */
	if (!qrb->qrptr)
	{
	    /*
	    ** if not at the end of the qry, get more output;
	    ** errors are flushed as they occur.
	    */
	    if (!qrend)
	    {
		qrb->qrptr = qrrun(qrb, &recsize);
		qrend = (qrb->qrptr == NULL);
	    }

		/* if new record size is larger than exisiting buffer we need 
		** to reallocate it to accomodate the incoming record
		*/
		if ( recsize > bfrlen )
		{
			if( (bufr = MEreqmem( 0, recsize, TRUE, NULL )) == NULL )
			{
			   FSdiag(ERx("FSrecbe memory alloc failed\n"));
	           iiufBrExit( bcb, E_UG0009_Bad_Memory_Allocation, 1, "FSrecbe");
			   return(-1);
			}
			MEfree((PTR)*bfr);
			*bfr = bufr;
		}

	    /* check for end of output */
	    if (qrend)
	    {
		/* end of output */
		qrrelease(qrb);

		/* re-init global FS_qrb, so that exception handler
		** (FStrap) knows about it.
		*/
		FS_qrb = NULL;

		bcb->req_complete = TRUE;
		STcopy(ERget(F_MF000C_End_of_Request), *bfr);
	    	qrend = 0;	/* Bug#6595 fix */

		if (qrb->error && qrb->severity)
		    STcat(*bfr, ERget(F_MF000D_Terminated_by_Errors));

		return(STlength(*bfr));
	    }
	}

	bfptr = *bfr;

	/* LINEDRAWING STUFF NEEDS A KANJI REPLACEMENT */
	for(;;)
	{
	    if (*qrb->qrptr == '\n' || *qrb->qrptr == '\0')
	    {
		*bfptr = EOS;
		if (!*qrb->qrptr || !*++qrb->qrptr)
		    qrb->qrptr = 0;
		return(len);
	    }
	    else if (*qrb->qrptr == '\t')
	    {
		i4 blanks;

		qrb->qrptr++;
		blanks = 8 - (len%8);

		while (blanks--)
		{
		    *bfptr++ = ' ';
		    len++;
		}
	    }
	    else
	    {
		CMbyteinc(len, qrb->qrptr);
		CMcpyinc(qrb->qrptr, bfptr);
	    }
	}
}


/*{
** Name:	FSouterr - print an error message for FSTM.
**
** Description:
**	Added for multi error-message support. This routine is called
**	from qrerror. It is the flush function found in the QRB. Upon
**	finding an error from LIBQ, qrerror checks the severity, and
**	calls FSouterr (via the flushfunc field) to send the message to
**	the screen. Upon exit, FSouterr returns control to qrerror, which
**	in turn tells LIBQ all is dealt with, which will return control
**	back to our program.
**
**	FSouterr requires enough information to call FSaddrec, which
**	sends the message to the proper place.
**
** Inputs:
**	bcbptr		A PTR which is really the current bcb.
**
**	msg		The error message in question.
**
** Outputs:
**	none.
**
**	Returns:
**		none.
**
** Side Effects:
**	Sends the message to FSaddrec for printing or safekeeping.
**
** History:
**	15-may-87 (daver) First Written.
**	08-nov-88 (kathryn) Bug# 3359
**		Check for messages which are terminated with EOS
**		but have no "\n" character.
*/
VOID
FSouterr(bcbptr, msg)
PTR	bcbptr;
char	*msg;
{
	BCB		*bcb;
	register char	*err;
	register i4	blanks,len;
	char		errmsg[4096];

	bcb = (BCB *)bcbptr;

	for (err=errmsg, len=0 ; *msg; CMnext(msg))
	{
		switch (*msg)
		{
		    case '\t':
			blanks = 8 - (len%8);
			while (blanks--)
			{
				*err++ = ' ';
				len++;
			}
			break;
# ifdef	FT3270
		    case '[':
			*err++ = '<';
			len++;
			break;

		    case ']':
			*err++ = '>';
			len++;
			break;
# endif
		    case '\n':
			len = 0;	/* reset tab stops on newline */
			*err++ = EOS;
			IIUFadd_AddRec(bcb, errmsg, TRUE);
			err = errmsg;	/* reset error string on newline */
			break;

		    default:
			CMcpychar(msg, err);
			CMbyteinc(len, err);
			CMnext(err);
			break;

		}	/* end switch */
	} /* end for */

	if (len > 0)			/* BUG 3359 */
	{
		*err = EOS;
		IIUFadd_AddRec(bcb,errmsg, TRUE);
	}
}

/*
**    FSnxtrow --  Gets one single record and adds it to the output buffer
**
**    FSnxtrow is called only by FSmore when it wants one record of output
**    from the backend.  FSnxtrow gets the row from the backend, then
**    adds it to the output file/buffer.
**
**    Returns: An integer representing the length of the record being
**	       returned. A length of '-1' means the request is complete
**	       and there is no further output data.
**
**	History:
**	03-aug-89 (sylviap)  
**		Initial version.  Created because ReportWriter now calls the
**		FS routines to use the scrollable output.  Some of the
**		interfaces needed to be changed to remain compatible.
**		Needed another parameter, rows_added.
**	29-jan-89 (teresal)
**		FSrecbe returns a -1 for length when a request is completed.
**		Modified so rows_added is always set to 1 even if FSrecbe 
**		returns -1. This fixes a problem where ^J wasn't displaying 
**		the bottom section of the output screen.
**	20-mar-2000 (kitch01)
**		Bug 99136. Amended call to FSrecbe to include passed in buffer
**		size.
*/
i4
FSnxtrow(bcb, bfr, bfrlen, qrb, rows_added)
BCB	*bcb;
char	**bfr;
i4	bfrlen;
QRB	*qrb;
i4	*rows_added;
{
	i4	len;	/* length of the row */

	*rows_added = 1;
	if ((len = FSrecbe(bcb, bfr, qrb, bfrlen)) < 0)
	{
		return (-1);
	}
	IIUFadd_AddRec(bcb,*bfr,TRUE);
	return (len);
}
