# include	<compat.h>
# include	<me.h>		 
# include	<st.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<itline.h>
# include	<er.h>
# include	<qr.h>
# include	<si.h>
# include	"monitor.h"

#ifdef SEP

# include <tc.h>

GLOBALREF   TCFILE *IITMincomm;
GLOBALREF   TCFILE *IITMoutcomm;

#endif /* SEP */

/*
** Copyright (c) 2004 Ingres Corporation
**
**  GET CHARACTER
**
**	This routine is just a getchar, except it allows a single
**	backup character ("Peekch").  If Peekch is nonzero it is
**	returned, otherwise a getchar is returned.
**
**	This routine also provides the interface to the international
**	character translation routines (ITxin).
**
**  HISTORY:
**	8/17/85 (peb) -- Modified to support international character
**			 translations.
**	16-feb-1988 (neil) -- Added putresults, so that all other putprintf
**			      need not go through IT layers.
**	29-jul-88 (bruceb)
**		In putresults, buffers now dynamically allocated to
**		prevent overflow on wide or long output.  At present
**		these buffers can still grow large, however, so further
**		work is required to decrease the size required.
**		putresults() now takes only one argument, the result
**		buffer, thus simplifying calculations on necessary
**		buffer size.
**	08-aug-88 (bruceb)
**		Placed %s's back into the printf's in putresult(),
**		since data string might contain % characters.
**	11-aug-88 (bruceb)
**		Declared putresults() a VOID function.
**		Added IIMOpfh_PutFlush routine.
**	28-nov-1988 (kathryn) bug# 2997
**		Change call from ITlndraw to ITlnprint so -v flag works.
**	28-nov-88 (bruceb)
**		Changed prior fix so that ITlndraw is called unless the
**		the -v flag is set, in which case ITlnprint is used.
**		This allows the default to be use of the line drawing
**		box characters.
**	10-may-89 (Eduardo)
**		Added interface to SEP tool
**	02/10/90 (dkh) - TM tuning changes - basically eliminating
**			 unnecessary buffer copies, calls to STlength()
**			 and using SIwrite() to put the result out
**			 rather than SIprintf().
**	26-apr-90 (teresal)
**		Fix for TM tuning - calculate length from start of output line 
**		(qrb->outlin) rather than start of buffer.
**	7-oct-92 (rogerl)
**		Delete IIMObf2_Buffer2.
**	10-oct-92 (leighb) DeskTop Porting Change:
**		Added '\n' to string gotten from SIgetrec for MS-WIN version.
**	18-jan-93 (leighb)
**		Removed previous change (10-oct-92).
**	15-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Changed type of args p1...p9 in putprintf from i4 to PTR for
**		pointer portability (axp_osf port).
**	13-dec-1996 (thoda04)
**		After IIMOpfh_PutFlush flushes the line out, reset line pntrs.
**	02-oct-1997 (walro03)
**		Corrected function prototype of ITxin to fix compile errors.
**      31-Jul-2002 (fanra01)
**          Need to cast char to u_char before converting to an int.
**          FF is actually 255 but was being cast to -1 in the convert.
**	11-Feb-2003 (hanje04)
**	     Bug 109613
**	     Increased size of 'buf' in putprintf() by DB_MAXNAME to 
**	     accommodate long error messages which reference potentially long 
**	     table names. (e.g E_US100D) 
**	31-mar-2004 (kodse01)
**		SIR 112068
**		Added call to SIhistgetrec() instead of SIgetrec() for Linux OS.
**		This call will support history recall in terminal monitor.
**	29-apr-2004 (kodse01)
**		SIhistgetrec() is called if HistoryRecall is TRUE.
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	20-Apr-2010 (hanje04)
**	    SIR 123622
**	    Enable history recall for all UNIXES
*/

GLOBALREF	bool	IIMOupcUsePrintChars;
GLOBALREF	bool	HistoryRecall;

FUNC_EXTERN	i4	ITlndraw();
FUNC_EXTERN	i4	ITlnprt();
#ifndef FT3270
FUNC_EXTERN	void	ITxin(char *, char *);
#endif

/*
**  Buffers for international character translation
*/

# define	MAX_T_LINE	256	/* Maximum length terminal line */

static char	Inbufraw[MAX_T_LINE+1] ZERO_FILL;
static char	Inbufxlate[MAX_T_LINE+1] ZERO_FILL;
static char	*Inbufptr = &Inbufxlate[0];

static char	Outbufraw[MAX_T_LINE+1] ZERO_FILL;
static char	Outbufxlate[MAX_T_LINE+1] ZERO_FILL;
static char	*Outbufptr = &Outbufraw[0];

/* putresults() buffer size and buffers */
static i4	IIMObfl_BufLen = 0;
static char	*IIMObf1_Buffer1 = NULL;


i4
#ifdef CMS	  /* CMS Whitesmith function name conflict */
tmgetch()
#else
getch()
#endif
{
	register i4	c;
	STATUS	status;

	c = Peekch;
	if (c)
	{
		Peekch = '\0';
	}
	else
	{
		if (Input == stdin && Inisterm == TRUE)
		{
			/* Place characters in international xlate buffer */
			if (*Inbufptr == '\0')
			{
/* Ony want command recall for Unixes */
# ifdef UNIX
				if (HistoryRecall)
					status = SIhistgetrec(Inbufraw, MAX_T_LINE, Input);
				else
					status = SIgetrec(Inbufraw, MAX_T_LINE, Input);
				if (status == ENDFILE)
# else
				if (SIgetrec(Inbufraw, MAX_T_LINE, Input) == ENDFILE)
# endif
				{
					c = EOF;
				}
				else
				{
#ifndef FT3270
					ITxin(Inbufraw, Inbufxlate);
#else
					STcopy(Inbufraw, Inbufxlate);
#endif
					Inbufptr = &Inbufxlate[0];
					while (c == '\0')
						c = (u_char)*Inbufptr++;
				}
			}
			else
			{
				while (c == '\0')
					c = (u_char)*Inbufptr++;
			}
		}
		else
		{
#ifdef SEP
		    if (Input == stdin && IITMincomm != NULL)
		    {
			for (;;)
			{
			    c = TCgetc(IITMincomm,0);
			    if (c == '\0')
				continue;
			    if (c == TC_EOQ)
			    {
				TCputc(TC_EQR,IITMoutcomm);
				TCflush(IITMoutcomm);
				continue;
			    }
			    if (c == TC_EOF)
				c = EOF;
			    break;
			}
		    }
		    else
		    {
#endif /* SEP */
			while (c == '\0')
				c = SIgetc(Input);
#ifdef SEP
		    }
#endif /* SEP */
		}
	}
	if (c == EOF)
		c = '\0';

	/* deliver EOF if newline in Oneline mode */
	if (c == '\n' && Oneline)
	{
		Peekch = c;
		c = '\0';
	}

	return (c);
}



/*
**  PUTCH -- SIputc for the terminal monitor
**
**  HISTORY:
**	8/17/85 (peb)	First written
**	10/22/87 (peterk) - deleted; see putprintf() below.
*/


/*
**  PUTPRINTF -- terminal monitors simple version of SIprintf.
**
**	putprintf() allows the terminal monitor to write to stdout
**	and the script file for its own internal messages.
**
**	NOTE:
**		Allows a maximum of nine formatting arguments.
**
**  HISTORY:
**	16-feb-1988 (neil) - Written
**      10-may-89 (Eduardo)
**              Added interface to SEP tool
**	15-aug-1990 (kathryn)  Bug 31965
**		Increase size of "buf" to ER_MAX_LEN (Maximum Error message
**		length).  To accomodate all Error Messages/QEP output sent.
**	11-Feb-2003 (hanje04)
**		Bug 109613
**		Increased size of 'buf' in putprintf() by DB_MAXNAME to 
**		accommodate long error messages which reference potentially long
**		table names. (e.g E_US100D) 
*/

void
putprintf(fmt, p1, p2, p3, p4, p5, p6, p7, p8, p9)
char	*fmt;
PTR	p1, p2, p3, p4 ,p5, p6, p7, p8, p9;
{
	char	buf[ER_MAX_LEN + DB_MAXNAME];
#ifdef SEP
	char	*cptr;
#endif
	STprintf(buf, fmt, p1, p2, p3, p4, p5, p6, p7, p8, p9);

#ifdef SEP
	if (IITMoutcomm != NULL)
	{
	    cptr = buf;
	    while (*cptr != '\0')
	    {
		TCputc(*cptr,IITMoutcomm);
		cptr++;
	    }
	}
	else
#endif /* SEP */
	    SIprintf(ERx("%s"), buf);

	if (Script != NULL)
	    SIfprintf(Script, ERx("%s"), buf);
}

/*
**  PUTRESULTS -- terminal monitors version of SIprintf.
**
**	putresults() allows the terminal monitor to do mutlinational
**	character set translations.
**
**	NOTE:
**		Allows a maximum of nine formatting arguments.
**
**  HISTORY:
**	8/17/85 (peb)	First written
**	10/22/87 (peterk) - Eliminated putch() and moved output to Script
**		file here instead.
**	10/22/87 (peterk) - Call Kanji IT line-drawing routines here.
**	16-feb-1988 (neil) - Extracted into putresults
**      10-may-89 (Eduardo)
**              Added interface to SEP tool
**      7-oct-92 (rogerl)
**		Delete IIMObf2_Buffer2 alloc and free, since it isn't used.
**	11-nov-2009 (wanfr01) b122875
**		Do not run ITlnprint if Outisterm = FALSE.  Character
**		substitution was already done in qrputc().  Just output
**	 	the string.  Also don't run IIlnprint twice in a row if
**		you are using \script.
*/


VOID
putresults(str, len)
char	*str;
i4	len;
{
    bool	new = FALSE;
#ifdef SEP
    char	*cptr;
#endif
    i4	dummy;
    i4	outlen;

    /*
    ** Make sure that the buffers are big enough to hold the result
    ** string.  Include space for multinational character expansion
    ** and the NULL terminator.  Allocate an initial minimum of
    ** DB_MAXSTRING to prevent lots of allocations for normal work.
    ** (NOTE:  Still using DB_MAXSTRING rather than DB_GW4_MAXSTRING
    ** since this is simply an arbitrary largish buffer.)
    */
    if (IIMObf1_Buffer1 == NULL)
    {
	IIMObfl_BufLen = max(DB_MAXSTRING, len);
	new = TRUE;
    }
    else if (len > IIMObfl_BufLen)
    {
	MEfree(IIMObf1_Buffer1);
	IIMObfl_BufLen = len;
	new = TRUE;
    }

    if (new)
    {
	/*
	** Amount of memory allocated takes into account an inflation
	** factor of 2 for multi-national characters and EOS.
	**
	** If these buffers are of serious concern to FT3270, then
	** perhaps should only allocate buf1 if 'Outisterm == TRUE',
	** and otherwise ITlnprint() directly from the 'str' parameter.
	*/
	if ((IIMObf1_Buffer1 = (char *)MEreqmem((u_i2)0,
		(u_i4)(  2 * IIMObfl_BufLen + 1), TRUE,
		(STATUS *)NULL)) == NULL)
	{
	    IIUGbmaBadMemoryAllocation(ERx("putresults"));
	}
    }

# ifndef	FT3270
    if (Outisterm == TRUE)
    {
	if (IIMOupcUsePrintChars)
	{
	    /* use characters such as +, -, | */
	    outlen = (i4) ITlnprint(str, IIMObf1_Buffer1, len);
	}
	else
	{
	    /* use line drawing/box characters */
	    outlen = (i4) ITlndraw(str, IIMObf1_Buffer1, len);
	}
	SIwrite(outlen, IIMObf1_Buffer1, &dummy, stdout);
	if (Script != NULL)
	{
	    if (!IIMOupcUsePrintChars)
	        outlen = (i4) ITlnprint(str, IIMObf1_Buffer1, len);
	    SIwrite(outlen, IIMObf1_Buffer1, &dummy, Script);
	}
    }
    else
# endif
    {
	IIMObf1_Buffer1 = str;
#ifdef SEP
	if (IITMoutcomm != NULL)
	{
	    outlen = 0;
	    cptr = IIMObf1_Buffer1;
	    while (*cptr != EOS && outlen < len)
	    {
		TCputc(*cptr,IITMoutcomm);
		cptr++;
		outlen++;
	    }
	}
	else
#endif /* SEP */
	{
	    SIwrite(len, IIMObf1_Buffer1, &dummy, stdout);
	}

	if (Script != NULL)
	{
	    SIwrite(len, IIMObf1_Buffer1, &dummy, Script);
	}
    }
}


/*
** Name: IIMOpfh_PutFlush	- write out the standard output
**
** Description:
**	Used to flush out the query results.  Passed to qrinit, assigned
**	to qrb->putfunc, and used in the help modules to flush output
**	without having to return up the call stack, and to avoid creation
**	of huge dynamic buffers to store long query results.
**
** Inputs:
**	qrb	The current query block.  Contains query results.
**
** Outputs:
**
**	Returns:
**		VOID
**
** Side Effects:
**	Generates output and updates the qrb.
*/


VOID
IIMOpfh_PutFlush(qrb)
QRB	*qrb;
{
    char	*ret;
    i4		outlen;

    outlen = qrb->outp - qrb->outlin;	
    ret = qrb->outlin;
    qrb->outlin = &ret[outlen];
/* if    outlin = &ret[outlen]
   then  outlin =  ret+outlen;
   then  outlin =  ret+(outp - outlin)
   then  outlin =  outlin+(outp - outlin)
   ergo  outlin always =  outp.
   Then why is next test needed?  Test left in in case I missed something.
*/
    if (qrb->outlin != qrb->outp)
	qrb->outlin++;
    
    putresults(ret, outlen);
    
    qrb->outp   = ret;     /* reset current output line pointers */
    qrb->outlin = ret;     /* back to start of line              */
}
