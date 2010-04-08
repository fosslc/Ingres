/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<si.h>
# include	<st.h>
# include	<me.h>
# include	<gc.h>
# include	<ex.h>
# include	<er.h>
# include	<cm.h>
# include	<nm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>
# include	<pc.h>
# include	<lo.h>
# include	<iplist.h>
# include	<erst.h>
# include	<uigdata.h>
# include	<stdprmpt.h>
# include	<ip.h>
# include	<ipcl.h>

/*
**  Name: ipenv -- functions to manipulate the user's environment
**
**  Entry points:
**
**    ip_environment -- guarantee that environment will support FRS
**    ip_forminit -- set up environment to support FRS
**
**  History:
**	xx-xxx-92 (jonb)
**		Created.
**	05-mar-93 (jonb)
**		Beautified and commented.
**	08-mar-93 (jonb)
**		Explicitly unset II_CHARSET and II_CHARSETxx when using
**		message files from the install area; there won't be any
**		descriptor files there (b49844).
**	16-mar-93 (jonb)
**		Brought comment style into conformance with standards.
**	31-mar-93 (jonb)
**		ERclose before attempting to access a new message file.
**	13-jul-93 (tyler)
**		Changed MAX_TXT to MAX_MANIFEST_LINE.  Fixed incorrect usage
**		of STequal().
**	14-jul-93 (tyler)
**		Replaced changes lost from r65 lire codeline.
**	23-aug-93 (dianeh)
**		Changed instances of Ingres to INGRES in error messages,
**		where appropriate.
**	3-sep-93 (kellyp)
**		Define logical_name "" causes seg vio. Modify to do deassign 
**		when the value is "".
**	09-sep-93 (kellyp)
**		Modified previous change so that deassign is replaced with
**		define logical_name " ".  This way if the logical_name does not
**		exist, it does not produce an error message which in turn causes
**		to program to abort with AV.
**	29-sep-93 (tyler)
**		Cleaned up previous History.  Fixed previous "fix" (not
**		tested on Unix) which broke ingbuild.
**	01-oct-93 (vijay)
**		More stuff to undo the "fix". Don't use " " for logical name
**		on Unix. /bin/sh does not like the logical name " ".
**	19-oct-93 (tyler)
**		Removed another instance of " " introduced by kellyp
**		on 9-sep-93.
**	11-nov-93 (tyler)
**		Ported to IP Compatibility Layer.
**	10-dec-93 (tyler)
**		Replaced LOfroms() with calls to IPCL_LOfroms().
**	05-jan-94 (tyler)
**		Allow ingbuild to (attempt to) run against the standard
**		message and frs files if the bootstrap versions are not
**	 	available - this will allow users to free up disk space
**		after installation.
**      08-Apr-94 (michael)
**              Changed references of II_AUTHORIZATION to II_AUTH_STRING where
**              appropriate.
**	17-oct-96 (mcgem01)
**		S_ST016A now takes SystemProductName as a parameter.
**      22-nov-96 (reijo01)
**              Replaced all instances of ingres directory with
**              SystemLocationSubdirectory
**	29-May-1998 (hanch04)
**		Added ip_init_calicense
**	09-jun-1998 (hanch04)
**		We don't use II_AUTH_STRING for Ingres II
**	30-nov-1998 (toumi01)
**		INGRESII is overloaded to mean that CA licensing is in
**		effect (not necessarily true).  Define, locally,
**		INGRESII_CALICENSE to have this meaning and override
**		it for Linux (lnx_us5).
**	04-oct-1999 (toumi01)
**		Modify Linux test for CA licensing to check VERS setting,
**		since at least one build configuration of the Linux version
**		now _does_ include CA licensing.
**	06-oct-1999 (toumi01)
**		Change Linux config string from lnx_us5 to int_lnx.
**	28-apr-2000 (somsa01)
**		Changed II_SYSTEM to SystemLocationVariable.
**	07-jun-2000 (somsa01)
**		Modified E_ST056A for multiple products.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	11-Jun-2004 (somsa01)
**		Cleaned up code for Open Source.
**	17-Jun-2004 (schka24)
**	    Safer environment variable handling.
*/

/*  Declarations of local functions... */

static STATUS	open_termcap();
static void	dflt_environment();
static void	termcap_list();

GLOBALREF char  *installDir;

/*
** ip_environment() -- set environment variables required to allow
**	installation utility to function in the absence of a complete
**	installation.
*/

void
ip_environment( void )
{
	char *cp;

	IPCLsetEnv( ERx( "II_MSGDIR" ), ip_install_loc(), TRUE );
	cp = ERget( F_ST0100_install_menuitem );
	if( STequal( cp, ERx( "ERget: Message File not found." ) ) ) 
	{
		LOCATION loc;
		char locBuf[ MAX_LOC + 1 ];

		/*
		** Messages were not found in bootstrap installation
		** directory, so assume they are in the standard directory.
		*/
		NMgtAt( SystemLocationVariable, &cp );
		STlcopy( cp, locBuf, sizeof(locBuf)-20-1 );
		IPCL_LOfroms( PATH, locBuf, &loc );
		LOfaddpath(&loc, SystemLocationSubdirectory, &loc);
		LOfaddpath(&loc, ERx("files"), &loc);
		LOfaddpath(&loc, ERx("english"), &loc);
		LOtos( &loc, &cp );
		IPCLsetEnv( ERx( "II_MSGDIR" ), cp, TRUE );
	}
	else
		IPCLsetEnv( ERx( "II_CONFIG" ), ip_install_loc(), TRUE );
	NMgtAt( ERx( "II_INSTALLATION"), &cp );
	if( cp && *cp )
	{
		char ii_charsetxx[ 100 ];	

		STlpolycat( 2, sizeof(ii_charsetxx)-1,
			ERx( "II_CHARSET" ), cp, ii_charsetxx );
		IPCLsetEnv( ii_charsetxx, ERx( "" ), TRUE );
	}
}

/*  ip_forminit() -- go into forms mode.
**
**  Get to the point where we can successfully do "exec frs forms", and
**  leave the terminal initialized for forms mode.  This function assumes
**  that if "exec frs forms" fails to change to forms mode, the reason is
**  that TERM_INGRES is defined to an incorrect termcap.  There are probably
**  other possible reasons, but I've never seen an example.  We keep prompting
**  for a termcap name as long as either TERM_INGRES is undefined or the
**  "forms" statement fails.
*/

void
ip_forminit()
{
    char *cp;
    char ttype[16];
    bool need_expl;

    exec sql begin declare section;
    i4  rowcnt;
    exec sql end declare section;

    for (need_expl = TRUE;;)
    {
	NMgtAt(ERx("TERM_INGRES"),&cp);
	if (cp && *cp)
	{
	    exec frs forms;

	    /* 
	    ** See if the "forms" command worked.  If it did, we'll get
	    ** a row count; if not, we'll get an error message, and the
	    ** variable won't get changed.  We ought to be able to do
	    ** this by querying on errorno, but it doesn't seem to get
	    ** reset by a successful execution after it's failed once.  
	    */

	    rowcnt = -1;
	    exec frs inquire_frs frs (:rowcnt = rows);
	    if (rowcnt != -1)
		return;
	}

	/* Just give the verbose explanation once. */

	if (need_expl)
	{
	    SIprintf(ERget(S_ST016A_term_explanation), SystemProductName);
	    need_expl = FALSE;
	}

	/* Ask for termcap name; display a list if it's requested. */

        for (;;)
	{
	    SIprintf(ERget(S_ST016B_enter_term));
	    SIgetrec(ttype, sizeof(ttype)-1, stdin);
	    STtrmwhite(ttype);   /* Lose trailing newline */

	    if (*ttype == EOS)
		termcap_list();
	    else
	    {
		IPCLsetEnv( ERx( "TERM_INGRES" ), ttype, TRUE );
		break;
	    }
	}
    }
}

/*  dflt_environment() -- set a logical if it isn't set already. */

static void
dflt_environment(name, value)
char *name, *value;
{
    char *curr;

    NMgtAt(name, &curr);
    if (!curr || !*curr)
	IPCLsetEnv( name, value, TRUE );
}

/*  open_termcap() -- attempt to open the termcap file
**
**  This function uses the standard procedure to find the termcap file:
**  if II_TERMCAP_FILE is defined, it contains the full path; otherwise,
**  it's file "termcap" in wherever II_CONFIG points if that's defined,
**  or $II_SYSTEM/ingres/files if it isn't.
*/

static STATUS
open_termcap(filep)
FILE **filep;
{
    char *cp;
    LOCATION tcloc;
    char tcbuf[MAX_LOC + 1];

    NMgtAt(ERx("II_TERMCAP_FILE"), &cp);
    if (cp && *cp)
    {
	STlcopy(cp, tcbuf, sizeof(tcbuf)-1);
	IPCL_LOfroms( PATH & FILENAME, tcbuf, &tcloc );
    }
    else
    {
	NMgtAt(ERx("II_CONFIG"), &cp);
	if (cp && *cp)
	{
	    STlcopy(cp, tcbuf, sizeof(tcbuf)-1);
	    IPCL_LOfroms( PATH, tcbuf, &tcloc );
	}
	else
	{
	    NMgtAt(SystemLocationVariable, &cp);
	    if (cp && *cp)
	    {
		STlcopy(cp, tcbuf, sizeof(tcbuf)-1);
		IPCL_LOfroms( PATH, tcbuf, &tcloc );
		LOfaddpath(&tcloc, SystemLocationSubdirectory, &tcloc);
		LOfaddpath(&tcloc, ERx("files"), &tcloc);
	    }
	    else
	    {
		return FAIL;
	    }
	}
	LOfstfile( ERx( "termcap" ), &tcloc );
    }
    return SIfopen(&tcloc, ERx("r"), SI_TXT, MAX_MANIFEST_LINE, filep);
}

/*
**  Termcaps that are listed in the "short list".  These must match on
**  either the first or the second termcap name alternative in the termcap
**  file, but the second one will be displayed no matter which matches.
*/

static char *popularTerms[] =
{
    "ansinf",
    "vt100",
    "vt100nl",
    "vt100f",
    "vt100fnl",
    "vt100nk",
    "xsun",
    "wxterm",
    NULL      /* (Required to terminate the list.) */
};

/*
**  termcap_list() -- display a long or short list of available termcaps
**
**  This function reads through the termcap file looking for definition
**  header lines, which are defined to be any line that starts with either
**  a digit or an alphabetic.  This isn't actually 100% true, but close
**  enough; read on.)  Such lines look basically like this...
**
**  xx|blurfl|foo|text description of the terminal being described:\
**
**  ...where "xx", "blurfl", and "foo" are alternative names for the same
**  termcap.  There must be at least one of them, and there may be any number,
**  but is pretty common.  We throw out any line that doesn't contain a
**  vertical bar, which filters out any non-header lines that crept in due
**  to the non-100% truthfulness mentioned above.
**
**  The most "common" name generally appears in the second position, "blurfl"
**  in the above example, so that's the one we display.  In the case of
**  the short list, we do the same thing -- that is, we read through the
**  entire termcap file -- but we only display lines that match an entry
**  in the popularTerms array above.  The match can be on either the first
**  ("xx") or the second ("blurfl") alternative terminal name in the 
**  definition.
**
*/

# define MAX_LINES 22

static void
termcap_list()
{
    bool biglist;
    char ans[5];
    char line[ MAX_MANIFEST_LINE + 1 ];
    char *cp, *name, *expl;
    i4  lcnt, popx;
    FILE *tcfile;

    /* Find the termcap file and attempt to open it... */

    if (open_termcap(&tcfile) != OK)
    {
	SIprintf(ERget(E_ST0171_no_term_list));
	return;
    }

    /* Find out if the user wants a long or a short list. */

    for (;;)
    {
	SIprintf(ERget(S_ST0170_term_list_prompt));
	SIgetrec(ans, sizeof(ans)-1, stdin);
	SIflush(stdin);
	STtrmwhite(ans);
	biglist = !CMcmpcase(ans, ERx("?"));
	if (biglist || *ans == EOS)
	    break;
    }

    SIprintf("\n");
    lcnt = 0;

    /* Read the termcap file... */

    while ( OK == SIgetrec(line, MAX_MANIFEST_LINE, tcfile) )
    {
	if (CMalpha(line) || CMdigit(line))  /* Test for definition header */
	{
	    for (cp = name = line, expl = NULL; *cp; CMnext(cp))
	    {
		if (!CMcmpcase(cp, ERx(":")))
		    *cp = EOS;   /* Terminate all strings at ":" */

		if (!CMcmpcase(cp, ERx("|")))
		{
		    *cp = EOS;   /* Also terminate strings at "|" */

		    /* 
		    ** If there's just one name, we'll use that, and the
		    ** explanation follows the first bar.  If there's more
		    ** than one, we'll use the second one... 
		    */

		    if (name == line && expl != NULL)
			name = expl;

		    expl = cp;
		    CMnext(expl);
		}
	    }

	    if (expl == NULL)
		continue;  /* No "|" was ever found.  Ignore the line. */

	    STtrmwhite(name);   /* Lose leading and trailing whitespace. */
	    name = STskipblank(name, MAX_MANIFEST_LINE);

	    STtrmwhite(expl);
	    expl = STskipblank(expl, MAX_MANIFEST_LINE);

	    if (!biglist)  /* Short list -- see if it matches the table */
	    {
		for ( popx = 0; popularTerms[popx]; popx++ )
		{
		    /* 
		    ** "name" points at the second alternative if there is
		    ** one, otherwise the first.  "line" definitely points
		    ** at the first.  We'll take a match on either. 
		    */

		    if ( !STscompare(name, 0, popularTerms[popx], 0) ||
			 !STscompare(line, 0, popularTerms[popx], 0) )
			break;
		}

		if (!popularTerms[popx]) 
		    continue;
	    }

	    /* 
	    ** Pause and wait for confirmation every MAX_LINES lines.  It
	    ** would be nice to do this based on the actual screen size,
	    ** but as far as I know there's no nice way of finding that out. 
	    */

	    if (++lcnt > MAX_LINES)
	    {
		for (;;)
		{
		    SIprintf(ERget(S_ST0172_pause_prompt));
		    SIgetrec(ans, sizeof(ans)-1, stdin);
		    STtrmwhite(ans);
		    if (!CMcmpcase(ans, ERx("s")))
			break;
		    if (*ans == EOS)
		    {
			lcnt = 0;
			break;
		    }
		}

		if (lcnt)
		    break;
	    }

	    /* 
	    ** Print out the line.  Again, wouldn't it be nice to format this
	    ** based on the real width, rather than an arbitrary 80?  
	    */

	    SIprintf(ERx("%-14s %-64s\n"),name,expl);
	}
    }

    SIclose(tcfile);
}
