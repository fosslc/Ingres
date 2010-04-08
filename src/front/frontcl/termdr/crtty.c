/*
**  Terminal initialization routines.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  crtty.c
**
**  History:
**	03/19/87 (dkh) - Added VIGRAPH capabilities.
**	19-jun-87 (bruceb)	Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	10-sep-87 (bruceb)
**		Changed from use of ERR to FAIL (for DG.)
**	15-sep-87 (bruceb)
**		Add XF termcap capability initialization.
**	24-sep-87 (bruceb)
**		Add XL termcap capability initialization.
**	02-oct-87 (bruceb)
**		Changed TEopen call and subsequent code to potentially
**		change COLS, LINES, and IITDflu_firstlineused.
**	12/18/87 (dkh) - Fixed jup bug 1632.
**	12/24/87 (dkh) - Performance changes.
**	04/08/88 (dkh) - Added ability to determine window size on startup.
**	07/15/88 (dkh) - Modified window size string parsing routine.
**	07/25/88 (dkh) - Added XC to support color on Tektronix.
**	10/26/88 (dkh) - Performance changes.
**	17-nov-88 (bruceb)
**		Added dummy arg in call on TEget.  (arg used in timeout.)
**	12/21/88 (brett) - Added windex command string interfaces.
**	02/18/89 (dkh) - Yet more performance changes.
**	05/19/89 (dkh) - Fixed bug 6009.
**	08/04/89 (dkh) - Added support for 80/132 runtime switching.
**	09/22/89 (dkh) - Porting changes integration.
**	10/01/89 (dkh) - Code cleanup.
**	12/01/89 (dkh) - Added support for GOTOs.
**	01/04/90 (dkh) - Added WX capability for X113 xterm.
**	01/12/90 (dkh) - Put in checks to make sure window size is
**			 at least 5 rows by 25 columns.
**	12-mar-90 (bruceb)
**		Added DECterm interface.  (DT termcap entry and IITDDT boolean.)
**		Also defined IITDlsLocSupport as a more general request for
**		locator support.
**	29-mar-90 (bruceb)
**		Added Xterm support.  (XT termcap entry and IITDXT boolean.)
**	04/04/90 (dkh) - Integrated MACWS changes.
**	31-may-90 (rog)
**		Included <me.h> so MEfill() is defined correctly.
**	08/15/90 (dkh) - Fixed bug 21670.
**	12/13/90 (dkh) - Fixed bug 34915.
**	12/27/90 (dkh) - Fixed bug 35055.
**	03/24/91 (dkh) - Integrated changes from PC group.
**	08/19/91 (dkh) - Integrated changes from PC group.
**	26-may-92 (leighb) DeskTop Porting Change:
**		Changed PMFE ifdefs to MSDOS ifdefs to ease OS/2 port.
**	14-Mar-93 (fredb) hp9_mpe
**		Integrated 6.2/03p change:
**      		14-sep-89 (fredb) - Changed initialization of NONL
**					    in TDgettmode().
**	07/24/93 (dkh) - Fixing code to provide a more appropriate error
**			 message when no more memory can be allocated.
**      14-Jun-95 (fanra01)
**              Added NT_GENERIC define to MSDOS section to enable TEtermcap.
**              Previously Windows and MSDOS shared section.
**      24-sep-96 (mcgem01)
**              Global data moved to termdrdata.c
**	04-may-97 (mcgem01)
**		Now that all hard-coded references to TERM_INGRES have
**		been removed from the message files, make the appropriate 
**		system terminal type substitutions in the error messages.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      03-May-2001 (fanra01)
**          Sir 104700
**          Add js termcap flag to enable Java servlet implementation.
**	19-Sep-2003 (bonro01)
**	    NO_OPTIM required for i64_hpu to prevent SIGILL in 3gl COBOL tests.
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/
/*
NO_OPTIM = i64_hpu
*/


# include	<compat.h>
# include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<cm.h>
# include	<lo.h>
# include	<st.h>
# include	<ftdefs.h>
# include	<termdr.h>
# include	<tdkeys.h>
# include	<kst.h>
# include	<te.h>
# include	<ug.h>
# include	<er.h>
# include	"ertd.h"

GLOBALREF bool	TDisopen;
GLOBALREF i4	IITDAllocSize;
GLOBALREF bool	IITDiswide;
GLOBALREF bool	IITDccsCanChgSize;
GLOBALREF i4	IITDcur_size;

FUNC_EXTERN	VOID	IITDwinsize();
FUNC_EXTERN	VOID	IITDbcBreakCapabilities();

/*
** WS is a boolean termcap that indicates the presence of the windex
** window environment.
**
** brett, Nov 21 1988
*/
GLOBALREF	bool     IITDWS;

/* DT is the boolean termcap entry that indicates support for DECterm. */
GLOBALREF	bool	IITDDT;
/* XT is the boolean termcap entry that indicates support for Xterm. */
GLOBALREF	bool	IITDXT;
GLOBALREF	bool	IITDlsLocSupport;

typedef struct tccap
{
	char	tc_cap[4];
	char	*pos;
	struct	tccap	*next;
} TCCAP;

GLOBALREF	char	*IItbuf;
GLOBALREF	char	*YP;
GLOBALREF	char	*YQ;
GLOBALREF	char	*YR;
GLOBALREF	char	*YS;
GLOBALREF	i4	YN;
GLOBALREF	i4	YO;
GLOBALREF	bool	IITDWX;

static	TCCAP	*IITDcaps = NULL;
static	TCCAP	**IITDarcap = NULL;
static	i4	IITDtag = 0;

static	i4	orig_size = 0;

static	bool	findfast = FALSE;

# define	MAX_TC_CAPS	250	/* maximum number of capabilities */

GLOBALREF	bool	*sflags[];

GLOBALREF char  *xPC;
GLOBALREF char  *xXL;
GLOBALREF char  **sstrs[];

FUNC_EXTERN	char	*TDtgoto();
FUNC_EXTERN	bool	TDtgetflag();
FUNC_EXTERN	i4	TDtgetnum();
FUNC_EXTERN	i4	TDtgetent();
FUNC_EXTERN	VOID	ITgetent();
FUNC_EXTERN	VOID	TDtcaploc();
FUNC_EXTERN	char	*TDtskip();

static	char	tspace[2048] = {0};	/* Space for capability strings */
static	char	*aoftspace = {0};	/* Address of tspace for relocation */

static	i4	destcol = 0;
static	i4	destline = 0;

/*
**  This routine does terminal type initialization routines, and
**  calculation of flags at entry.
**
**  History:
**	14-Mar-93 (fredb) hp9_mpe
**		Integrated 6.2/03p change:
**  		14-sep-89 (fredb) - mpe/xl's C runtime converts NL's to NL/CR
**                      	pairs which cause the horizontal cursor position
**                      	to be changed. Setting NONL to zero acknowledges
**                      	this 'feature'. Setting NONL to a non-zero value
**                      	assumes that the NL will move the cursor down
**                      	one line without changing the horiz. position.
*/

GLOBALREF i4	ospeed;

TDgettmode()
{
#ifdef hp9_mpe
	NONL = 0;				/* MPE/iX needs this unset */
#else
	NONL = 1;				/* NONL set on VMS */
#endif
}


TDsetterm(type)
char	*type;
{
	char		genbuf[2048];
	TEENV_INFO	envinfo;
	i4		rows;
	i4		columns;
	LOCATION	tloc;
	TCCAP		tc_cap[MAX_TC_CAPS];
	TCCAP		*ar_cap[MAX_TC_CAPS];
	char		locbuf[MAX_LOC + 1];
	i4		getsize = TRUE;
	i4		errcols;
	i4		errrows;

	if (type[0] == '\0')
		type = ERx("xx");

	/*
	**  Get location of termcap file.
	**  (or use global buffer setup by another TDtgetent caller, who
	**  gave it to use through TDglobtc().
	*/
	TDtcaploc(&tloc, locbuf);

	if (TDtgetent(genbuf, type, &tloc) != 1)
	{
# ifdef	DGC_AOS
		/*
		**  Users are not being told about/made
		**  aware of the termcap file
		*/
		IIUGerr(E_TD0021_UNKNOWN, UG_ERR_ERROR, 2, type,
				SystemTermType);
		return(FAIL);
# else
		IIUGerr(E_TD001D_UNKNOWN, UG_ERR_ERROR, 2, type, 
				SystemTermType);
		return(FAIL);
# endif
	}

	IITDcaps = tc_cap;
	IITDarcap = ar_cap;

	MEfill((sizeof(TCCAP *) * MAX_TC_CAPS),  EOS, ar_cap);

	IITDbcBreakCapabilities();

	if (LINES == 0)
		LINES = TDtgetnum(ERx("li"));
	if (LINES > MAX_TERM_SIZE)
		LINES = MAX_TERM_SIZE;

	if (COLS == 0)
		COLS = TDtgetnum(ERx("co"));
	if (COLS > MAX_TERM_SIZE)
		COLS = MAX_TERM_SIZE;

	YN = TDtgetnum(ERx("yn"));
	YO = TDtgetnum(ERx("yo"));

	/*
	**  Additional capabilities for VIGRAPH.
	*/
	TDGC = TDtgetnum(ERx("GC"));
	TDGR = TDtgetnum(ERx("GR"));
	TDGA = TDtgetnum(ERx("Ga"));
	TD_GC = TDtgetnum(ERx("Gc"));
	TD_GR = TDtgetnum(ERx("Gr"));
	TDGX = TDtgetnum(ERx("Gx"));
	TDGY = TDtgetnum(ERx("Gy"));

	aoftspace = tspace;
	TDzap();		/* get terminal description */

	/*
	**  Check for ability to change size of physical terminal.
	*/
	IITDccsCanChgSize = FALSE;
	if (YN && YO && (YN == COLS || YO == COLS))
	{
		IITDccsCanChgSize = TRUE;
		orig_size = COLS;
		IITDcur_size = COLS;
		IITDAllocSize = max(YN, YO);
		if (YN == COLS)
		{
			IITDiswide = FALSE;
		}
		else
		{
			IITDiswide = TRUE;
		}
	}

	/*
	**  Get information for an InTernational support stuff.
	*/
	ITgetent();

	if (TDtgoto(CM, destcol, destline)[0] == 'O')
		CA = FALSE, CM = 0;
	else
		CA = TRUE;
	PC = xPC ? xPC[0] : FALSE;
	XL = xXL ? xXL[0] : FALSE;
	aoftspace = tspace;
	/*
	** use user specified name rather than termcap comment-entry.
	** bug 10315 (bruceb)
	*/
	STcopy(type, ttytype);

	if (TEopen(&envinfo) != OK)
	{
		return(TDerrmsg(E_TD001E_NOTTERM, 1, type));
	}
	ospeed = envinfo.delay_constant;
	rows = envinfo.rows;
	columns = envinfo.cols;
	if (rows > 0 && IITDWX)
	{
		getsize = FALSE;
		LINES = rows;
	}
	else if (rows < 0)
	{
		/* returned value indicates number rows unavailable to frs */
		IITDflu_firstlineused = -rows;
	}
	if (columns > 0 && IITDWX)
	{
		getsize = FALSE;
		IITDcur_size = COLS = columns;
	}
	/*
	** else if (columns < 0), indicate num cols unavailable
	** to frs.  not currently implemented.
	*/

	/*
	**  Get window size in case we are running from a
	**  window manager terminal emulator.  This only
	**  works on startup.  Don't do this if TEopen()
	**  returns positive sizes since that should
	**  return the same info.
	*/
	if (getsize)
	{
		IITDwinsize();
	}

	/*
	**  Double check to see if YN and YO equals the column size
	**  from TEopen().  If not, we disable IITDccsCanChgSize.
	**  This can happen if user is on a terminal emulator and
	**  has a window size that doesn't match what termcap is telling us.
	*/
	if (YN != COLS && YO != COLS)
	{
		IITDccsCanChgSize = FALSE;
		IITDAllocSize = 0;
	}

	/*
	**  Finally, check that we are dealing with a terminal size
	**  that is at least 5 rows by 25 columns.
	*/
	if (COLS < 25 || LINES < 5)
	{
		TEclose();
		errcols = COLS;
		errrows = LINES;
		IIUGerr(E_TD0022_too_small, UG_ERR_ERROR,3, &errrows, 
			&errcols, SystemTermType);
		return(FAIL);
	}

	IITDAllocSize = max(IITDAllocSize, COLS);

	TDisopen = TRUE;
	GT = 0;		/* bug fix 1886 (dkh) */
	_rawmode = TRUE;
	_pfast = TRUE;
	_echoit = FALSE;

	/*
	**  Reset "findfast" so gt can work.
	*/
	findfast = FALSE;

	if (IITDtag)
	{
		FEfree(IITDtag);
	}

	/*
	** brett, set windex flag accordingly.  Then append the VS
	** and VE termcaps with the windex command strings.
	*/
	if (IITDWS)
		IITDfixvsve(&VS, &VE);

	/*
	** If DECterm is on, then locator support is requested.
	*/
	if (IITDDT || IITDXT)
		IITDlsLocSupport = TRUE;

# if defined(MSDOS) || defined (NT_GENERIC)
	/*
	**  Pass some of the termcap strings to TE
	**  so that TE can do direct video RAM writes.
	**  Dave - PC group should really get this into the CL spec.
	*/
	TEtermcap(LINES, COLS, EA, BO, US, ZB, BL, ZC, ZE, ZH, RV, ZD, ZF,
		ZK, ZG, ZJ, ZI, ZA, YA, YB, YC, YD, YE, YF, YG, YH);
# endif /* MSDOS || NT_GENERIC */

	return(OK);
}

/*
**  This routine gets all the terminal flags from the termcap database.
*/

TDzap()
{

	reg	bool	**fp;
	reg	char	*namp;
	reg	char	***sp;
	reg	u_char	*strp;
	reg	i4	inx;
	FUNC_EXTERN char	*TDtgetstr();
	GLOBALREF char		*KH;

	/*
	** get boolean flags
	*/

	inx = 0;
	MAXFKEYS = 0;

	namp = ERx("ambsmsnckypfxdxfxnGhGiGlGoGsGwWSmwxcwxxsDTXTjs\0\0");

	fp = sflags;
	do
	{
		**fp = TDtgetflag(namp);
		fp++;
		namp += 2;
	} while (*namp);

	/*
	** get string values
	*/

	namp = ERx("bcbtceclcmhoisllndpcsesosrtatetiucueupusvsvexlcsGEGLGeGpdcypyqyrys");
	sp = sstrs;
	do
	{
		**sp = TDtgetstr(namp, &aoftspace);
		sp++;
		namp += 2;
	} while (*namp);
	namp = ERx("ldlelsqaqbqcqdqeqfqgqhqiqjqkbeblrerveazazbzczdzezfzgzhzizjzkboebkskeyaybycydyeyfygyhmfeswgwr");
	do
	{
		**sp = TDtgetstr(namp, &aoftspace);
		sp++;
		namp += 2;
	} while (*namp);

	if (KY)
	{
		namp = ERx("kukdkrklk0k1k2k3k4k5k6k7k8k9kAkBkCkDkEkFkGkHkIkJkKkLkMkNkOkPkQkRkSkTkUkVkWkXkYkZKAKBKCKD");
		do
		{
			strp = (u_char *) TDtgetstr(namp, &aoftspace);
			if (strp)
			{
				KEYSEQ[inx++] = strp;
			}
			else
			{
				KEYSEQ[inx++] = KEYNULLDEF;
			}
			MAXFKEYS++;
			**sp = (char *) strp;
			sp++;
			namp += 2;
		} while (*namp);

		in_func = FKgetinput;

		MENUCHAR = F0KEY;
	}
	KEYNUMS = TDtgetnum(ERx("kn"));
	FKEYS = KEYNUMS;
	MAXFKEYS = FKEYS + 4;
	if (!SO && US)
	{
		SO = US;
		SE = UE;
	}
}



/*{
** Name:	IITDwinsize - Get window size on startup.
**
** Description:
**	Get window size at startup time in case we are running
**	from a window manager terminal emulator window.  Note
**	that we are only supporting startups and not dynamic
**	resizing.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
**	Returns:
**		VOID
**	Exceptions:
**		None.
**
** Side Effects:
**	Variables COLS and LINES may be changed.
**
** History:
**	04/08/88 (dkh) - Initial version.
*/
VOID
IITDwinsize()
{
	char	buf[100];
	char	str[3];
	char	int1[10];
	char	int2[10];
	char	*intptr;
	char	*cp;
	char	*wrptr;
	i4	ln;
	i4	co;
	i4	onlydigits = FALSE;
	i4	digitcount = 0;

	/*
	**  Only check if both WG and WR are present.
	*/
	if (WG && WR && *WG != EOS && *WR != EOS)
	{
		/*
		**  Flush any junk in input buffers.
		*/
		TEinflush();
		cp = WG;
		while (*cp)
		{
			TEput(*cp++);
		}
		TEflush();
		str[1] = EOS;
		cp = buf;
		wrptr = WR;
		intptr = int1;
		str[1] = int1[0] = int2[0] = EOS;

		/*
		**  The algorithm below assumes that there is a
		**  non-digit terminating character for the WR
		**  string.
		*/
		for (;;)
		{
			if (*wrptr == '%' && *(wrptr + 1) == 'd')
			{
				wrptr += 2;
				onlydigits = TRUE;
				if (++digitcount > 2)
				{
					/*
					**  More than two numbers in
					**  input string, just get out.
					*/
					return;
				}
			}
			*str = TEget((i4)0);
			if (onlydigits)
			{
				if (!CMdigit(str))
				{
					*intptr = EOS;
					onlydigits = FALSE;
					intptr = int2;
					if (*str != *wrptr)
					{
						/*
						**  Out of sync with
						**  WR string, get out.
						*/
						return;
					}
				}
				else
				{
					*intptr++ = *str;
					continue;
				}
			}
			else
			{
				if (*str != *wrptr)
				{
					/*
					**  Out of sync with
					**  WR string, get out.
					*/
					return;
				}
			}

			/*
			**  Break out if at end of string.
			*/
			if (*++wrptr == EOS)
			{
				break;
			}
		}

		/*
		**  Only convert value if there is something to
		**  convert.  Both values must be present to work.
		*/
		if (STlength(int1) == 0 || CVan(int1, &ln) != OK)
		{
			return;
		}
		if (STlength(int2) == 0 || CVan(int2, &co) != OK)
		{
			return;
		}
		LINES = ln;
		IITDcur_size = COLS = co;
	}
}



/*{
** Name:	IITDbcBreakCapabilities - Break termcap capabilities.
**
** Description:
**	All this routine does is to break up the capabilities for an
**	entry in the termcap file.  This makes it faster to grab
**	capabilities instead of scanning the entire capability
**	string each time.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	TCCAP structures are filled in.
**
** History:
**	10/28/88 (dkh) - Initial version.
*/
VOID
IITDbcBreakCapabilities()
{
	i4	count = 0;
	i4	index;
	char	*bp = IItbuf;
	char	c1;
	char	c2;
	TCCAP	*cap;
	TCCAP	*acap;
	TCCAP	*tcap;
	i4	found;

	findfast = TRUE;

	for (;;)
	{
		while (*bp && *bp != ':')
		{
			bp++;
		}
		if (*bp == ':')
		{
			c1 = *++bp;

			/*
			**  If at end of string, return.
			*/
			if (!c1)
			{
				return;
			}

			/*
			**  Just continue if we have back to back
			**  colons (:).
			*/
			if (c1 == ':')
			{
				continue;
			}
			if ((c2 = *++bp) == ':')
			{
				continue;
			}

			/*
			**  Have two characters that are not
			**  colons.  Must be a capabilitly, we hope.
			*/
			index = (c1 + c2) % MAX_TC_CAPS;
			if (count < MAX_TC_CAPS)
			{
				cap = &IITDcaps[count++];
			}
			else
			{
				if (!IITDtag)
				{
					IITDtag = FEgettag();
				}
				if ((cap = (TCCAP *) FEreqmem(IITDtag,
					sizeof(TCCAP), TRUE,
					(STATUS *) NULL)) == NULL)
				{
					IIUGbmaBadMemoryAllocation(ERx("IITDbc"));
				}
			}
			cap->tc_cap[0] = c1;
			cap->tc_cap[1] = c2;
			cap->pos = bp-1;
			if ((acap = IITDarcap[index]) == NULL)
			{
				cap->next = NULL;
				IITDarcap[index] = cap;
			}
			else
			{
				/*
				**  Check for previous existence of the
				**  capability.  If found, it has
				**  precedence.  This keeps the tc
				**  capability from being broken.
				*/
				found = FALSE;
				for (tcap = acap; tcap; tcap = tcap->next)
				{
					if (tcap->tc_cap[0] == c1 &&
						tcap->tc_cap[1] == c2)
					{
						/*
						**  Found a match, skip
						**  this one.
						*/
						found = TRUE;
						break;
					}
				}
				if (found)
				{
					continue;
				}

				/*
				**  Didn't find a match, can save it.
				*/
				cap->next = acap;
				IITDarcap[index] = cap;
			}
			continue;

		}
		return;
	}
}



/*{
** Name:	IITDtskip - Fast version to get to start of a capability.
**
** Description:
**	This routine quickly finds the start of a capability depending
**	on IITDbcBreakCapabilities being previously called.  If we
**	are not searching in a fast way, do it the slow way.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		cp	Pointer to start of capability.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	10/28/88 (dkh) - Initial version.
*/
char *
IITDtskip(buf, id)
char	*buf;
char	*id;
{
	i4	index;
	TCCAP	*cap;
	char	*cp = ERx("");
	char	id1;
	char	id2;

	if (!findfast)
	{
		return(TDtskip(buf, id));
	}

	id1 = *id++;
	id2 = *id;
	index = (id1 + id2) % MAX_TC_CAPS;
	for (cap = IITDarcap[index]; cap; cap = cap->next)
	{
		if (cap->tc_cap[0] == id1 && cap->tc_cap[1] == id2)
		{
			cp = cap->pos;
			break;
		}
	}
	return(cp);
}


/*{
** Name:	IITDscSizeChg - Change width of terminal screen.
**
** Description:
**	Change the physical size (width) of the terminal screen.  The
**	(logical) width to set is passed in.
**
** Inputs:
**	iswide		Set terminal to logical wide width if TRUE is passed
**			in.  If FALSE, set to logical narrow width.
**
** Outputs:
**
**	Returns:
**		TRUE	If terminal width is changeable.
**		FALSE	If terminal width is NOT changeable.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/04/89 (dkh) - Initial version.
*/
i4
IITDscSizeChg(iswide)
i4	iswide;
{
	char	*cp;

	/*
	**  If terminal description does not support runtime size
	**  changing, ignore the request.
	*/
	if (!IITDccsCanChgSize)
	{
		return(FALSE);
	}
	if (iswide)
	{
		if (IITDcur_size == YO)
		{
			return(FALSE);
		}
		COLS = curscr->_maxx = IITDcur_size = YO;
		CM = YQ;
		cp = YS;
# ifdef PMFEDOS
		stdmsg->_maxx = YO;
# else
		stdmsg->_maxx = YO - 1;
# endif
		IITDiswide = TRUE;
	}
	else
	{
		if (IITDcur_size == YN)
		{
			return(FALSE);
		}
		COLS = curscr->_maxx = IITDcur_size = YN;
		CM = YP;
		cp = YR;
# ifdef PMFEDOS
		stdmsg->_maxx = YN;
# else
		stdmsg->_maxx = YN - 1;
# endif
		IITDiswide = FALSE;
	}

	/*
	**  Send out command to switch terminal size.
	*/
	while (*cp)
	{
		TEput(*cp++);
	}
	TEflush();

	/*
	**  Now sync up display with a clear screen.
	*/
	TDclear(curscr);

	return(TRUE);
}


/*{
** Name:	IITDrstsize - Reset terminal size.
**
** Description:
**	Routine resets the terminal to the same width at the time the
**	forms system started up.  This, of course, assumes that the
**	physical size is the same as that defined in the termcap entry.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/04/89 - (dkh) Initial version.
*/
VOID
IITDrstsize()
{
	/*
	**  Reset terminal if size has changed from startup.
	**  But only if we are currently in one of the
	**  standard sizes.  Non-standard size indicates
	**  that we are probably running from a terminal
	**  emulator and that no size switching has occurred.
	*/
	if (IITDcur_size == YO || IITDcur_size == YN)
	{
		if (IITDcur_size != orig_size)
		{
			if (orig_size == YN)
			{
				_puts(YR);
			}
			else
			{
				_puts(YS);
			}
		}
	}
}
