/*
**  TERMCAP.C - 5/27/81
**
**  Copyright (c) 2004 Ingres Corporation
**  All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<st.h>
# include	<lo.h>
# include	<nm.h>
# include	<cm.h>
# include	<er.h>
# include	"ertd.h"
# include	<termdr.h>

# define	TBUFSIZ		2048

# define	MAXHOP	32	/* max number of tc= indirections */

# ifdef VMS
#	define	TERMFILE	ERx("termcap.")
# else
# ifndef CMS
#	define	TERMFILE	ERx("termcap")
# else
#	define	TERMFILE	ERx("termcap.zz")
# endif
# endif
# define	TFILENAM	ERx("II_TERMCAP_FILE")



/*
**  termcap - routines for dealing with the terminal capability data base
**
**  BUG:	Should use a "last" pointer in IItbuf, so that searching
**		for capabilities alphabetically would not be a n**2/2
**		process when large numbers of capabilities are given.
**  Note:	If we add a last pointer now we will screw up the
**		tc capability. We really should compile termcap.
**
**  Essentially all the work here is scanning and decoding escapes
**  in string capabilities.  We don't use stdio because the editor
**  doesn't, and because living w/o it is not hard.
**
**  History:
**	6/18/85 (prs)	Add ref to II_TERMCAP_FILE name.
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	08/14/87 (dkh) - ER changes.
**	12/23/87 (dkh) - Performance changes.
**	04-06-87 (bab)	Fix for bug 11440
**		TDtnamatch() will now compare terminal names
**		without case-sensitivity.  This enables termcap
**		terminal names to contain upper case chars, and
**		for users on systems such as VMS to specify
**		values for TERM_INGRES without placing the
**		terminal name in quotes (VMS uppercases the
**		value of the logical.)
**	11/01/88 (dkh) - Performance changes.
**	02/18/89 (dkh) - Yet more performance changes.
**	4/15/89  (Mike S) 
**		In TDtgetent, invalidate saved terminal name after a bad
**		name wipes out the saved buffer.
**	09/23/89 (dkh) - Porting changes integration.
**	09/26/89 (dkh) - More porting changes.
**	08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**	10-mar-1994 (mgw) Bug #60483
**		Don't use floc in an LOcopy() if the preceding NMloc()
**		failed (e.g. because II_SYSTEM isn't set) in TDtcaploc().
**		Also don't call SIopen() for a tcaploc that doesn't exist
**		in TDitgetent().
**      24-sep-96 (mcgem01)
**              Global data moved to termdrdata.c
**	17-apr-1997 (canor01)
**		In TDitgetent, use returned byte count to determine
**		success of the read.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      13-feb-03 (chash01) x-integrate change#461908
**        Initialize readstatus in if(); compiler copmplains about 
**        uninitialized variable.
**	17-Jun-2004 (schka24)
**	    Safe env var handling.
**	06-May-2005 (hanje04)
**	   Rename devname to iidevname, it clashes with the devname() function.
*/

GLOBALREF char	*IItbuf;
GLOBALREF i4	IIhopcount;	/* detect infinite loops in termcap, init 0 */

FUNC_EXTERN	char	*TDtskip();
char		*TDtgetstr();
static char	*TDtdecode();

FUNC_EXTERN	char	*IITDtskip();

static	char	iidevname[80] = {0};
static	char	sgenbuf[TBUFSIZ] = {0};

/*
**  Create LOCATION of termcap file.
*/
VOID
TDtcaploc(loc, locbuf)
LOCATION	*loc;
char		*locbuf;
{
	char		*cp;
	LOCATION	floc;

	NMgtAt(TFILENAM, &cp);
	if (cp != NULL && *cp != '\0')
	{
		STlcopy(cp, locbuf, MAX_LOC-1);
		LOfroms(FILENAME, locbuf, loc);
	}
	else
	{
		if (NMloc(FILES, FILENAME, TERMFILE, &floc) == OK) /* 60483 */
			LOcopy(&floc, locbuf, loc);
	}
}

i4
TDtgetent(bp, name, tcaploc)
reg char	*bp;
char		*name;
LOCATION	*tcaploc;
{
	LOCATION	loc;
	char		locbuf[MAX_LOC + 1];
	LOCATION	*locp;
	char		tname[80];
	i4		retval;

	STcopy(name, tname);
	CVlower(tname);
	if (STcompare(tname, iidevname) == 0)
	{
		IItbuf = sgenbuf;
		return(1);
	}
	locp = tcaploc;
	if (tcaploc == NULL)
	{
		TDtcaploc(&loc, locbuf);
		locp = &loc;
	}
	if ((retval = TDitgetent(sgenbuf, tname, locp)) == 1)
	{
		STcopy(tname, iidevname);
	}
	else
	{
		iidevname[0] = EOS;
	}
	return(retval);
}


/*
** Get an entry for terminal name in buffer bp,
** from the termcap file.  Parse is very rudimentary;
** we just notice escaped newlines.
**
*/

i4
TDitgetent(bp, name, tcaploc)
reg char	*bp;
char		*name;
LOCATION	*tcaploc;
{
	reg	char	*cp;
	reg	char	c;
	reg	i4	i = 0;
		i4	cnt = 0;
		char	ibuf[TBUFSIZ];
		FILE	*tf;
	STATUS		readstatus;
		char	*fname;
	LOINFORMATION	locinf;
		i4	locflag = LO_I_PERMS;

	IItbuf = bp;
	if ((readstatus=LOinfo(tcaploc, &locflag, &locinf)) != OK) /* Bug 60483 */
	{
		LOtos(tcaploc, &fname);
		SIfprintf(stderr, ERget(E_TD0003_error_opening),
			readstatus, ERx(""));
		SIflush(stderr);
		return (-1);
	}
	if ((readstatus = SIopen(tcaploc, ERx("r"), &tf)) != OK)
	{
		LOtos(tcaploc, &fname);
		SIfprintf(stderr, ERget(E_TD0003_error_opening),
			readstatus, fname);
		SIflush(stderr);
		return(-1);
	}
	for (;;)
	{
		cp = bp;
		for (;;)
		{
			if (i == cnt)
			{
				readstatus = SIread(tf, TBUFSIZ, &cnt, ibuf);
				if (cnt <= 0)
				{
					SIclose(tf);
					return(0);
				}
				i = 0;
			}
			c = ibuf[i++];
			if (c == '\n')
			{
				if (cp > bp && cp[-1] == '\\')
				{
					cp--;
					continue;
				}
				break;
			}
			if (cp >= bp+TBUFSIZ)
			{
				SIfprintf(stderr,
					ERget(E_TD0004_Termcap_entry_long));
				SIflush(stderr);
				break;
			}
			else
				*cp++ = c;
		}
		*cp = 0;

		/*
		** The real work for the match.
		*/

		if (TDtnamatch(name))
		{
			SIclose(tf);
			i = TDtnchktc(tcaploc);
			return(i);
		}
	}
}

/*
** TDtnchktc: check the last entry, see if it's tc=xxx. If so,
** recursively find xxx and append that entry (minus the names)
** to take the place of the tc=xxx entry. This allows termcap
** entries to say "like an HP2621 but doesn't turn on the labels".
** Note that this works because of the left to right scan.
*/

TDtnchktc(tcaploc)
LOCATION	*tcaploc;
{
	reg	char	*p;
	reg	char	*q;
	char	tcname[16];	/* name of similar terminal */
	char	tcbuf[TBUFSIZ];
	char	*holdtbuf = IItbuf;
	i4	l;

	p = IItbuf + STlength(IItbuf) - 2;	/* before the last colon */
	while (*--p != ':')
		if (p<IItbuf)
		{
			SIfprintf(stderr, ERget(E_TD0005_Bad_termcap_entry));
			SIflush(stderr);
			return(0);
		}
	p++;

	/* p now points to beginning of last field */

	if (p[0] != 't' || p[1] != 'c')
		return(1);
	STcopy(p+3, tcname);
	q = tcname;
	while (q && *q != ':')
		q++;
	*q = 0;
	if (++IIhopcount > MAXHOP)
	{
		SIfprintf(stderr, ERget(E_TD0006_Infinite_tc_loop));
		SIflush(stderr);
		return(0);
	}
	if (TDitgetent(tcbuf, tcname, tcaploc) != 1)
		return(0);
	for (q=tcbuf; *q != ':'; q++)
		;
	l = p - holdtbuf + STlength(q);
	if (l > TBUFSIZ)
	{
		SIfprintf(stderr, ERget(E_TD0004_Termcap_entry_long));
		SIflush(stderr);
		q[TBUFSIZ - (p-IItbuf)] = 0;
	}
	STcopy(q+1, p);
	IItbuf = holdtbuf;
	return(1);
}

/*
** TDtnamatch deals with name matching.	 The first field of the termcap
** entry is a sequence of names separated by |'s, so we compare
** against each such name.  The normal : terminator after the last
** name (before the first field) stops us.
**
** History:
**	04-06-87 (bab)
**		Name compares are now case-insensitive.
*/

TDtnamatch(np)
char	*np;
{
	reg	char	*Np;
	reg	char	*Bp;
		char	*nm;
		char	nmbuf[100];

	Bp = IItbuf;
	if (*Bp == '#')
		return(0);

	STcopy(np, nmbuf);
	nm = nmbuf;
	/* lowercase the name once, rather that once everytime through loop */
	CVlower(nm);

	for (;;)
	{
		for (Np = nm; *Np; Bp++, Np++)
		{
			CMtolower(Bp, Bp);
			if (*Bp != *Np)
			{
				break;
			}
			continue;
		}
		if (*Np == 0 && (*Bp == '|' || *Bp == ':' || *Bp == 0))
		{
			return(1);
		}
		while (*Bp && *Bp != ':' && *Bp != '|')
			Bp++;
		if (*Bp == 0 || *Bp == ':')
		{
			return(0);
		}
		Bp++;
	}
}

/*
** Skip to the next field.  Notice that this is very dumb, not
** knowing about \: escapes or any such.  If necessary, :'s can be put
** into the termcap file in octal.
*/

char *
TDtskip(abp, id)
char	*abp;
char	*id;
{
	reg	char	*bp = abp;
	reg	char	id0 = *id++;
	reg	char	id1 = *id;

	for (;;)
	{
		while (*bp && *bp != ':')
			bp++;
		if (*bp == ':')
		{
			if (*++bp != id0 || *(bp + 1) != id1)
			{
				continue;
			}
		}
		return(bp);
	}
}

/*
** Return the (numeric) option id.
** Numeric options look like
**	li#80
** i.e. the option string is separated from the numeric value by
** a # character.  If the option is not found we return -1.
** Note that we handle octal numbers beginning with 0.
*/

i4
TDtgetnum(id)
char	*id;
{
	reg	i4	i;
	reg	i4	base;
	reg	char	*bp = IItbuf;

	for (;;)
	{
	/*
		bp = TDtskip(bp, id);
	*/
		bp = IITDtskip(bp, id);
		if (*bp == 0)
			return(-1);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return(-1);
		if (*bp != '#')
			continue;
		bp++;
		base = 10;
		if (*bp == '0')
			base = 8;
		i = 0;
		while (CMdigit(bp))
			i *= base, i += *bp++ - '0';
		return(i);
	}
}

/*
** Handle a flag option.
** Flag options are given "naked", i.e. followed by a : or the end
** of the buffer.  Return 1 if we find the option, or 0 if it is
** not given.
*/
bool
TDtgetflag(id)
char	*id;
{
	reg	char	*bp = IItbuf;

	for (;;)
	{
	/*
		bp = TDtskip(bp, id);
	*/
		bp = IITDtskip(bp, id);
		if (!*bp)
			return((bool) 0);
		if (*bp++ == id[0] && *bp != 0 && *bp++ == id[1])
		{
			if (!*bp || *bp == ':')
				return((bool) 1);
			else if (*bp == '@')
				return((bool) 0);
		}
	}
}

/*
** Get a string valued option.
** These are given as
**	cl=^Z
** Much decoding is done on the strings, and the strings are
** placed in area, which is a ref parameter which is updated.
** No checking on area overflow.
*/

char *
TDtgetstr(id, area)
char	*id;
char	**area;
{
	reg	char	*bp = IItbuf;

	for (;;)
	{
	/*
		bp = TDtskip(bp, id);
	*/
		bp = IITDtskip(bp, id);
		if (!*bp)
			return(0);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return(0);
		if (*bp != '=')
			continue;
		bp++;
		return(TDtdecode(bp, area));
	}
}

/*
** TDtdecode does the grung work to decode the
** string capability escapes.
*/

static char *
TDtdecode(astr, area)
char	*astr;
char	**area;
{
	reg	char	*str = astr;
	reg	char	*cp;
		char	c;
	reg	char	*dp;
		i4	i;

	cp = *area;
	while ((c = *str++) && c != ':')
	{
		switch (c)
		{

			case '^':
				c = *str++ & 037;
				break;

			case '\\':
				dp = ERx("E\033^^\\\\::n\nr\rt\tb\bf\f");
				c = *str++;
				do
				{
					if (*dp++ == c)
					{
						c = *dp++;
						break;
					}
					dp++;
				}
				while (*dp);
				if (CMdigit(&c))
				{
					c -= '0', i = 2;
					do
						c <<= 3, c |= *str++ - '0';
					while (--i && CMdigit(str));
				}
				break;
		}
		*cp++ = c;
	}
	*cp++ = 0;
	str = *area;
	*area = cp;
	return(str);
}
