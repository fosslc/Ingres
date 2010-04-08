/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<kst.h>
# include	<ft.h>
# include	<termdr.h>
# include	<tdkeys.h>
# include	<si.h>
# include	<st.h>
# include	<lo.h>
# include	<cm.h>
# include	<er.h>

/**
** Name:	locator.c -	Routines to support a locator.
**
** Description:
**	Includes routines to interpret the locator sequence sent by the
**	window manager (X's vt100 emulator and DECwindows) and various trace
**	routines to support development.
**
**	Routines defined are:
**		IITDplsParseLocString	Locator input processing routine.
**			IITDpdlParseDecLoc	DECterm specific routine.
**			IITDpxlParseXLoc	Xterm specific routine.
**		IITDlioLocInObj		Does a location overlap an object?
**		IITDltoLocTraceOn	Setup for locator debug tracing.
**		IITDpliPrtLocInfo	Print out a locator string.
**		IITDpciPrtCoordInfo	Print out coordinate info for an object.
**		IITDposPrtObjSelected	Print out trace info on object selected.
**		IITDptiPrtTraceInfo	Print out generic trace info.
**
**  History:
**	09-mar-90 (bruceb) - Initial version.
**	08/19/91 (dkh) - Integrated changes from PC group.
**	26-may-92 (leighb) DeskTop Porting Change:
**		Changed PMFE ifdefs to MSDOS ifdefs to ease OS/2 port.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	10/21/93 (dkh) - Fixed bug 52227.  Made sure that the
**			 variables for row and column in IITDpxlParseXLoc()
**			 are unsigned in case the encoding of the row/column
**			 positions causes the eighth bit to be set.  This
**			 is needed so that the values are not treated as
**			 negative values.  The eighth bit can be set when
**			 the row/column position is greater than 95.
**			 Note that the user must also make sure that the
**			 tty characteristics for the terminal do NOT
**			 strip off the eighth bit.  Otherwise, we get
**			 an incorrect location and things will work
**			 incorrectly in a different way.
**      14-Jun-95 (fanra01)
**              Added NT_GENERIC to MSDOS section.
**      23-apr-96 (chech02)
**              fixed compiler complaint for windows 3.1 port.
**      24-sep-96 (mcgem01)
**              Global data moved to termdrdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
** Indicates whether or not locator tracing will occur.  This is a compile
** time rather than runtime decision.  Setting IITDdp to FALSE here will
** ensure that tracing never occurs.  This is to avoid yet another runtime
** NMgtAt pass for a development parameter.  (This can be overridden by
** setting the value in the debugger.)
*/
GLOBALREF i4	IITDdpDebugPrint;


/* Portions of DECterm's locator sequence.  <ESC>[Pe;Pb;Pr;Pc;Pp&w  */
/*	and Xterm's sequence.  <ESC>[Mbxy */
# ifndef EBCDIC
# define	ESCAPE		'\033'
# else
# undef BELL
# define	ESCAPE		'\047'
# endif /* EBCDIC */

# define	STRT_STR	'['
# define	SEMI		';'
# define	END1_STR	'&'
# define	END2_STR	'w'

# if defined(MSDOS) || defined(NT_GENERIC)
# define	STRT2_STR	0x0D
# else
# define	STRT2_STR	'M'
# endif


/* Used for tracing locator sequences. */
static	FILE	*trfile = NULL;
static	bool	trace_on = FALSE;


FUNC_EXTERN	KEYSTRUCT	*FKnxtch();


GLOBALREF       bool    IITDlsLocSupport;
GLOBALREF       bool    IITDDT;
GLOBALREF       bool    IITDXT;


VOID		IITDltoLocTraceOn();
VOID		IITDpliPrtLocInfo();
KEYSTRUCT	*IITDpdlParseDecLoc();
KEYSTRUCT	*IITDpxlParseXLoc();



/*{
** Name:	IITDplsParseLocString - Locator input processing routine.
**
** Description:
**	Checks to see if the input stream contains a locator sequence.
**	Calls IITDpdl() or IITDpxl() to do the real work of obtaining a
**	DECterm or Xterm (respectively) locator sequence.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		ks	Pointer to KEYSTRUCT structure containing either the
**			<ESC> character or the function key KS_LOCATOR.
**
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	29-mar-90 (bruceb) - Initial version.
*/
KEYSTRUCT *
IITDplsParseLocString()
{
    KEYSTRUCT	*ks;

    if (IITDDT)
	ks = IITDpdlParseDecLoc();
    else
	ks = IITDpxlParseXLoc();

    return(ks);
}


/*{
** Name:	IITDpdlParseDecLoc - DECterm input processing routine.
**
** Description:
**	Checks to see if the input stream contains a locator sequence
**	from DECterm.  A locator sequence looks like this:
**
**		<ESC>[Pe;Pb;Pr;Pc&w
**
**		(  <ESC>[Pe;Pb;Pr;Pc;1&w  in DECterm V1.0   )
**
**	where Pe, Pb and Pp are numbers whose value are ignored, and Pr and Pc
**	are the locator's row and column within the terminal emulator window.
**
**	Any string that does not match the locator sequence will be pushed
**	back into the input stream for processing elsewhere.  Typically this
**	processing will involve stripping off the ESC (in FKgetinput) and
**	treating the rest as normal characters.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		ks	Pointer to KEYSTRUCT structure containing either the
**			<ESC> character or the function key KS_LOCATOR.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	If tracing is on, dump the fully parsed locator sequence to the trace
**	file.
**
** History:
**	09-mar-90 (bruceb) - Initial version.
*/
KEYSTRUCT *
IITDpdlParseDecLoc()
{
    bool	found_num;
    bool	badstring = FALSE;
    bool	v2 = TRUE;	/* Version 2.0+ of DECterm. */
    i4		row;
    i4		num;
    i4		i;
    KEYSTRUCT	*ks;
    u_char	tbuf[KBUFSIZE];
    u_char	numbuf[KBUFSIZE];
    u_char	*tptr = tbuf;
    u_char	*np = numbuf;

    /* Store away the original ESC. */
    *tptr++ = ESCAPE;

    ks = FKnxtch();

    if (ks->ks_ch[0] != STRT_STR)
    {
	badstring = TRUE;
    }
    else
    {
	CMcpychar(ks->ks_ch, tptr);	/* Store the STRT_STR. */
	CMnext(tptr);

	/* Now look for Pe and Pb (ignore values for now). */
	for (i = 0; i < 2; i++)
	{
	    found_num = FALSE;
	    ks = FKnxtch();
	    while (CMdigit(ks->ks_ch))
	    {
		found_num = TRUE;
		CMcpychar(ks->ks_ch, tptr);
		CMnext(tptr);
		ks = FKnxtch();
	    }
	    if (!found_num || ks->ks_ch[0] != SEMI)
	    {
		badstring = TRUE;
		break;
	    }

	    CMcpychar(ks->ks_ch, tptr);	/* Store the SEMI. */
	    CMnext(tptr);
	}

	if (!badstring)
	{
	    /* Now look for the row (Pr) and column (Pc). */
	    for (i = 0; i < 2; i++)
	    {
		np = numbuf;
		*np = EOS;
		ks = FKnxtch();
		while (CMdigit(ks->ks_ch))
		{
		    CMcpychar(ks->ks_ch, np);
		    CMnext(np);
		    CMcpychar(ks->ks_ch, tptr);
		    CMnext(tptr);
		    ks = FKnxtch();
		}
		*np = EOS;
		if (np == numbuf || CVan(numbuf, &num) != OK)
		{
		    badstring = TRUE;
		    break;
		}

		if (i == 0)
		{
		    row = num;	/* Pr */
		    if (ks->ks_ch[0] != SEMI)
		    {
			badstring = TRUE;
			break;
		    }
		}
		else
		{
		    if (ks->ks_ch[0] == SEMI)
		    {
			v2 = FALSE;
			break;
		    }
		    else if (ks->ks_ch[0] != END1_STR)
		    {
			badstring = TRUE;
			break;
		    }
		}

		CMcpychar(ks->ks_ch, tptr); /* Store the SEMI or END1_STR. */
		CMnext(tptr);
	    }

	    if (!badstring)
	    {
		if (!v2)
		{
		    /* Now look for the optional Pp. */
		    ks = FKnxtch();
		    while (CMdigit(ks->ks_ch))
		    {
			CMcpychar(ks->ks_ch, tptr);
			CMnext(tptr);
			ks = FKnxtch();
		    }
		    if (ks->ks_ch[0] == END1_STR)
		    {
			/* Store the END1_STR. */
			CMcpychar(ks->ks_ch, tptr);
			CMnext(tptr);
		    }
		    else
		    {
			badstring = TRUE;
		    }
		}

		ks = FKnxtch();
		if (ks->ks_ch[0] != END2_STR)
		{
		    badstring = TRUE;
		}

		CMcpychar(ks->ks_ch, tptr);	/* Store the END2_STR. */
		CMnext(tptr);
		*tptr = EOS;
	    }
	}
    }

    if (badstring)
    {
	CMcpychar(ks->ks_ch, tptr);
	CMnext(tptr);
	*tptr = EOS;
	FKungetch(tbuf, (i4) STlength((char *) tbuf));
	return(FKnxtch());
    }

    IITDpliPrtLocInfo(tbuf);	/* Dump out the locator string. */

    /* Set up return info for a locator command. */
    ks->ks_fk = KS_LOCATOR;
    /* DECterm locations are 1's based; FRS locations are 0's based. */
    ks->ks_p1 = row - 1;
    ks->ks_p2 = num - 1;

    return(ks);
}


/*{
** Name:	IITDpxlParseXLoc - Xterm input processing routine.
**
** Description:
**	Checks to see if the input stream contains a locator sequence
**	from Xterm.  A locator sequence looks like this:
**
**		<ESC>[Mbxy
**
**	where b, x and y are characters representing the button/event and the
**	locator's column and row within the terminal emulator window.  B will
**	be ignored; column and row values are obtained by subtracting the space
**	character from x and y, and then subtracting 1 for 0's based indexing.
**
**	Note:  subtracting ' ' from a character to obtain a coordinate will
**	possibly/probably not work on an EBCDIC machine.  Will need
**	investigation for such a machine.
**
**	Any string that does not match the locator sequence will be pushed
**	back into the input stream for processing elsewhere.  Typically this
**	processing will involve stripping off the ESC (in FKgetinput) and
**	treating the rest as normal characters.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		ks	Pointer to KEYSTRUCT structure containing either the
**			<ESC> character or the function key KS_LOCATOR.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	If tracing is on, dump the fully parsed locator sequence to the trace
**	file.
**
** History:
**	09-mar-90 (bruceb) - Initial version.
*/
KEYSTRUCT *
IITDpxlParseXLoc()
{
    u_char	row;
    u_char	col;
    KEYSTRUCT	*ks;
    u_char	tbuf[KBUFSIZE];
    u_char	numbuf[KBUFSIZE];
    u_char	*tptr = tbuf;

    /* Store away the original ESC. */
    *tptr++ = ESCAPE;

    ks = FKnxtch();

    if (ks->ks_ch[0] != STRT_STR)
    {
	CMcpychar(ks->ks_ch, tptr);
	CMnext(tptr);
	*tptr = EOS;
	FKungetch(tbuf, (i4) STlength((char *) tbuf));
	return(FKnxtch());
    }

    /* Store the STRT_STR. */
    CMcpychar(ks->ks_ch, tptr);
    CMnext(tptr);

    ks = FKnxtch();

    if (ks->ks_ch[0] != STRT2_STR)
    {
	CMcpychar(ks->ks_ch, tptr);
	CMnext(tptr);
	*tptr = EOS;
	FKungetch(tbuf, (i4) STlength((char *) tbuf));
	return(FKnxtch());
    }

    /* Store the STRT2_STR. */
    CMcpychar(ks->ks_ch, tptr);
    CMnext(tptr);

    /* Now look for b (ignore value for now). */
    ks = FKnxtch();
    CMcpychar(ks->ks_ch, tptr);
    CMnext(tptr);

    /* Obtain column. */
    ks = FKnxtch();
    CMcpychar(ks->ks_ch, tptr);
    CMnext(tptr);
    col = ks->ks_ch[0];

    /* Obtain row. */
    ks = FKnxtch();
    CMcpychar(ks->ks_ch, tptr);
    CMnext(tptr);
    row = ks->ks_ch[0];
    *tptr = EOS;


    IITDpliPrtLocInfo(tbuf);	/* Dump out the locator string. */

    /* Set up return info for a locator command. */
    ks->ks_fk = KS_LOCATOR;
    /* Xterm locations are 1's based; FRS locations are 0's based. */
    ks->ks_p1 = row - ' ' - 1;
    ks->ks_p2 = col - ' ' - 1;

    return(ks);
}


/*{
** Name:	IITDlioLocInObj - Determine if a location is in an object.
**
** Description:
**	Determine whether or not a location (x,y coordinates) overlaps a
**	specified object.
**
** Inputs:
**	obj	Coordinates of an object.
**	row	Row coordinate of the locator.
**	col	Column coordinate of the locator.
**
** Outputs:
**
**	Returns:
**		TRUE/FALSE that the object and locator overlap.
**
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	13-mar-90 (bruceb)	Written.
*/
i4
IITDlioLocInObj(obj,row,col)
IICOORD	*obj;
i4	row;
i4	col;
{
    if ((obj->begy <= row) && (row <= obj->endy)
	&& (obj->begx <= col) && (col <= obj->endx))
    {
	return(TRUE);
    }
    else
    {
	return(FALSE);
    }
}



/*{
** Name:	IITDltoLocTraceOn - Open a file for locator debug tracing.
**
** Description:
**	If tracing the locator information (locator sequence and FRS
**	structures), open the trace file.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		None.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	If tracing is on, open up the trace file.
**
** History:
**	09-mar-90 (bruceb) - Initial version.
*/
VOID
IITDltoLocTraceOn()
{
    LOCATION	loc;
    char	buf[MAX_LOC + 1];

    if (!trace_on)
    {
	if (!IITDlsLocSupport)
	{
	    /* Currently, only tracing if DECterm is being used. */
	    IITDdpDebugPrint = FALSE;
	}
	else
	{
	    STcopy(ERx("iiprtloc.log"), buf);
	    /*
	    **  If we can't open dump file, then just turn off tracing.
	    */
	    if ((LOfroms(FILENAME, buf, &loc) != OK)
		|| (SIopen(&loc, ERx("w"), &trfile) != OK))
	    {
		IITDdpDebugPrint = FALSE;
		return;
	    }
	    trace_on = TRUE;
	}
    }
}


/*{
** Name:	IITDpliPrtLocInfo - Print out a locator string.
**
** Description:
**	If tracing the locator information, print out the parsed locator
**	string to the trace file.
**
**	This routine understands that a locator string has an ESC as the
**	first character, and that the remaining characters are printable.
**	Note that this is true for the DECterm and Xterm locators; if not
**	true for other locators being traced, need to change this routine.
**
** Inputs:
**	locstr		The parsed locator string.
**
** Outputs:
**
**	Returns:
**		None.
**
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	09-mar-90 (bruceb) - Initial version.
*/
VOID
IITDpliPrtLocInfo(locstr)
char	*locstr;
{
    if (!IITDdpDebugPrint)
    {
	return;
    }

    if (!trace_on)
    {
	IITDltoLocTraceOn();
	if (!IITDdpDebugPrint)
	{
	    return;
	}
    }

    locstr++;	/* Skip the ESC. */
    SIfprintf(trfile, ERx("Locator string:  <ESC>%s\n"), locstr);
    SIflush(trfile);
}


/*{
** Name:	IITDpciPrtCoordInfo - Print out coordinate info for an object.
**
** Description:
**	If tracing the locator information, print out the coordinate
**	information passed in.
**
** Inputs:
**	routine		Calling routine.
**	objname		Object name.
**	obj		Coordinates of the object.
**
** Outputs:
**
**	Returns:
**		None.
**
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	12-mar-90 (bruceb) - Initial version.
*/
VOID
IITDpciPrtCoordInfo(routine, objname, obj)
char	*routine;
char	*objname;
IICOORD	*obj;
{
    char	rname[50];

    if (!IITDdpDebugPrint)
    {
	return;
    }

    if (!trace_on)
    {
	IITDltoLocTraceOn();
	if (!IITDdpDebugPrint)
	{
	    return;
	}
    }

    if (routine)
	STprintf(rname, ERx("%s():"), routine);
    else
	rname[0] = EOS;

    SIfprintf(trfile, ERx("%s  '%s' coords are:  (%d,%d) x (%d,%d)\n"),
	rname, objname, obj->begy, obj->begx, obj->endy, obj->endx);
    SIflush(trfile);
}


/*{
** Name:	IITDposPrtObjSelected - Print out trace info on object selected.
**
** Description:
**	If tracing the locator information, print out the name of the object
**	the locator selected.
**
** Inputs:
**	objname		Object name.
**
** Outputs:
**
**	Returns:
**		None.
**
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	13-mar-90 (bruceb) - Initial version.
*/
VOID
IITDposPrtObjSelected(objname)
char	*objname;
{
    if (!IITDdpDebugPrint)
    {
	return;
    }

    if (!trace_on)
    {
	IITDltoLocTraceOn();
	if (!IITDdpDebugPrint)
	{
	    return;
	}
    }

    SIfprintf(trfile, ERx("Object selected is '%s'\n"), objname);
    SIflush(trfile);
}

/*{
** Name:	IITDptiPrtTraceInfo	- Print out generic trace info.
**
** Description:
**	If tracing the locator information, print out the passed-in buffer.
**
** Inputs:
**	buf		Trace information.
**
** Outputs:
**
**	Returns:
**		None.
**
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-mar-90 (bruceb) - Initial version.
*/
VOID
IITDptiPrtTraceInfo(buf)
char	*buf;
{
    if (!IITDdpDebugPrint)
    {
	return;
    }

    if (!trace_on)
    {
	IITDltoLocTraceOn();
	if (!IITDdpDebugPrint)
	{
	    return;
	}
    }

    SIfprintf(trfile, ERx("%s\n"), buf);
    SIflush(trfile);
}
