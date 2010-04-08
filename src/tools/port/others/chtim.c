/*
**Copyright (c) 2004 Ingres Corporation
*/
#ifndef lint
static char *sccsid = "@(#)chtim.c	1.2 (Don Gworek) 8/10/85";
#endif

/* 
 * chtim [-sR] [-p proto-file] [-am "date" or seconds] files ... 
 *
 * Change or report file time stamps
 *
 * -s report in shell script/archive format
 * -R recursive for directories
 * -p put the proto-file's time stamps on the following files
 * -a set access time stamp
 * -m set modification time stamp
 *
 * Default: report time stamps for the files
 *
 * unctime() routines borrowed from Berkeley dump(8)
 * Recusion based on Berkeley 4.3 chmod(1) and ls(1)
 *
 * History:
 *	9-june-1989 (blaise)
 *		Commented out text after #endifs
 *	1-feburary-1993 (smc)
 *		Added <generic.h> so axp_osf specific code could be defined.
 *              Changed time from long to specific time type time_t for 
 *              axp_osf, and predeclared dcmp.
 *	19-aug-93 (johnr)
 *		Added DIRENT ifdefs needed for su4_us5 (probably generic for
 *		other DIRENT systems).
 *	25-Aug-1993 (fredv)
 *		Added back the history for the previous change:
 *			include <tim.h> for ris_us5.
 *	28-Apr-1995 (wadag01)
 *		Added odt_us5 to the list of platforms that #include <time.h>.
 *		(SCO does not include it in its <sys/time.h> ). 
 *	17-jun-1995 (morayf)
 *		Added sos_us5 to the list of platforms that #include <time.h>.
 *  18-feb-97 (muhpa01)
 *		Removed local definition for utimes() for hp8_us5
 *      29-aug-1997 (musro02)
 *              Added sqs_ptx to the list of platforms that #include <time.h>.
 *      12-Mar-1997 (mosjo01/linke01)
 *              In function utimes(), changed prototype declaration for
 *		argument `file` and `newtime` to match the declaration in 
 *		file <sys/time.h>
 *	26-Sep-1997 (bonro01)
 *		Added dg8_us5 to platforms needing new prototypes for
 *		'file' and 'newtime'.
 *  14-oct-1997 (bobmart/bonro01)
 *      dg8_us5 broke convention and introduced two additional members
 *      to the utimbuf struct (it's no longer just two time_t members...)
 *      Since bzarch.h/config isn't available, I went for a generic
 *      solution of creating a static struct that's used to init the auto.
 *	26-Nov-1997 (allmi01)
 *		Added sgi_us5 (via !defined(_XOPEN_SOURCE_EXTENDED)) to
 *		list of platforms not needing utime and stdio)
 *	27-Mar-1998 (muhpa01)
 *		Omit prototype for utimes for hpb_us5
 *	17-Sep-1998 (kosma01)
 *		Omit function definition of utimes for sgi_us5 (IRIX64)
 *	28-Apr-1999 (toumi01)
 *		For Linux use file definition with const qualifier.
 *	10-may-1999 (walro03)
 *		Remove obsolete version string odt_us5.
 *	06-oct-1999 (toumi01)
 *		Change Linux config string from lnx_us5 to int_lnx.
 *      11-Oct-1999 (hweho01)
 *              Changed time from long to time_t type time_t for AIX 64-bit
 *              platform (ris_u64).
 *	15-Jun-2000 (hanje04)
 *		For OS/390 Linux - ibm_lnx use same file definition as
 *		int_lnx
 *      11-Sep-2000 (hanje04)
 *              Added axp_lnx (Alpha Linux) to Linux file defn and
 *              Changed time from long to time_t type time_t.
 *	19-jul-2001 (stephenb)
 *		Add support for i64_aix
 *	11-dec-2001 (somsa01)
 *		Added support for hp2_us5.
 *      08-Oct-2002 (hanje04)
 *          As part of AMD x86-64 Linux port, replace individual Linux
 *          defines with generic LNX define where apropriate
 *	21-May-2003 (bonro01)
 *	    Add support for HP Itanium (i64_hpu)
 *	13-mar-04 (toumi01)
 *	    Include time.h for all LINUXes to avoid compile errors or
 *	    warnings.
 *      18-Apr-2005 (hanje04)
 *          Add support for Max OS X (mac_osx).
 *          Based on initial changes by utlia01 and monda07.
 *	    Correctly prototype dcmp() to quiet warnings.
 *	29-Jun-05 (hweho01)
 *	    Correct the parameter in utime() for HP TRU64 (axp_osf).
 *       6-Nov-2006 (hanal04) SIR 117044
 *          Add int.rpl for Intel Rpath Linux build.
 *	08-Feb-2008 (hanje04)
 *	    SIR S119978
 *	    Add support for Intel OSX
 */

# ifdef SYS5
# if !defined(hp8_us5) && !defined(_XOPEN_SOURCE_EXTENDED) && \
     !defined(mg5_osx) && !defined(int_osx) 

# include <stdio.h>
# include <utime.h>
int utimes();

# endif
# endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/stat.h>

#ifndef	 DIRENT
#include <sys/dir.h>
#else
#include <dirent.h>
#endif

#include <generic.h>

#if defined(ris_us5) || defined(rs4_us5) || defined(ris_u64) || \
    defined(sos_us5) || defined(sqs_ptx) || defined(sgi_us5) || \
    defined(i64_aix) || defined(LINUX) || defined(mg5_osx) || \
    defined(int_osx)
#include <time.h>
#endif

#define TRUE 1
#define FALSE 0
#define ARGVAL() (*++(*argv) || (--argc && *++argv))

int     scriptf = FALSE;
int     Recursf = FALSE;
int     protof = FALSE;
int     accessf = FALSE;
int     modif = FALSE;

int     status = 0;
int     argc;
char  **argv;

static  dcmp(struct tm *, struct tm *);
#ifdef axp_osf
time_t	now;
#elif defined(ris_u64) || defined(i64_aix)
time_t  now;
#else
long    now;
#endif

char    nowyear[5];
char	command[1024];

struct tm  *localtime ();
struct timeval  newtime[2];
struct stat stbuf;

time_t unctime();


static char months[12][4] = {
    "jan", "feb", "mar", "apr", "may", "jun",
    "jul", "aug", "sep", "oct", "nov", "dec"
};

main (ARGC, ARGV)
int     ARGC;
char   *ARGV[];
{
    char   *s;
    argc = ARGC;
    argv = ARGV;
    (void) strcpy (command, *argv);
    (void) time (&now);
    s = (char *) ctime (&now);
    (void) sprintf (nowyear, "%.4s", (s + 20));
    newtime[0].tv_sec = newtime[1].tv_sec = 0;
    getoptions ();
    if (argc <= 0)
	usage ();
    for (; --argc >= 0; argv++)
	chtim (*argv);
    exit (status);
}

getoptions () {
    while (--argc > 0)
	if (**(++argv) != '-')
	    return;
	else
	    switch (*++(*argv)) {
		case '\0': 
		    return;
		case 'p': 
		    if (!ARGVAL ())
			usage ();
		    if (stat (*argv, &stbuf) == -1) {
			perror (*argv);
			exit (1);
		    }
		    newtime[0].tv_sec = stbuf.st_atime;
		    newtime[1].tv_sec = stbuf.st_mtime;
		    protof = TRUE;
		    break;
		case 'a': 
		    extract_time (0);
		    break;
		case 'm': 
		    extract_time (1);
		    break;
		default: 
		    for (; **argv != '\0'; *(*argv)++)
			switch (**argv) {
			    case 's': 
				scriptf = TRUE;
				break;
			    case 'R': 
				Recursf = TRUE;
				break;
			    default: 
				usage ();
			}
	    }
}

extract_time (option)
int     option;
{
    int     a, m;
    a = m = FALSE;
    for (;; *++(*argv))
	switch (**argv) {
	    case '\0': 
		if (--argc && *++argv)
		    goto set;
		else
		    usage ();
		break;
	    case 'a': 
		a = accessf = TRUE;
		break;
	    case 'm': 
		m = modif = TRUE;
		break;
	    default: 
		goto set;
	}
set: 
    if (!isdigit (**argv)) {
	if ((newtime[option].tv_sec = unctime (*argv)) < 0)
	    usage ();
    }
    else
	newtime[option].tv_sec = atol (*argv);
    if (a)
	newtime[0].tv_sec = newtime[option].tv_sec;
    if (m)
	newtime[1].tv_sec = newtime[option].tv_sec;
}

chtim (fname)
char   *fname;
{
    struct stat stb;
    if (stat (fname, &stb) == -1) {
	perror (fname);
	status++;
	return;
    }
    if (Recursf && stb.st_mode & S_IFDIR)
	status += chtimr (fname);
    if (newtime[0].tv_sec || newtime[1].tv_sec) {
	if (accessf && !modif)
	    newtime[1].tv_sec = stb.st_mtime;/* preserve m value */
	else
	    if (modif && !accessf)
		newtime[0].tv_sec = stb.st_atime;/* preserve a value */

	if (utimes (fname, newtime) != 0) {
	    status++;
	    perror (fname);
	}
    }
    else
	if (scriptf) {
	    if (stb.st_atime == stb.st_mtime)
		printf ("%s -am %d", command,  stb.st_atime);
	    else
		printf ("%s -a %d -m %d", command,  stb.st_atime, stb.st_mtime);
	    printf (" %s\n", fname);
	}
	else {
	    printf ("%s\n", fname);
	    print_time ("a", stb.st_atime);
	    print_time ("m", stb.st_mtime);
	    print_time ("c", stb.st_ctime);
	}
}

chtimr (dir)
char   *dir;
{
    register    DIR * dirp;

#ifndef DIRENT
    register struct direct *dp;
#else
    register struct dirent *dp;
#endif /* DIRENT */

    char    dirfile[1024];
    if ((dirp = opendir (dir)) == NULL) {
	perror (dir);
	return (1);
    }
    dp = readdir (dirp);
    dp = readdir (dirp);	/* read "." and ".." */
    for (dp = readdir (dirp); dp != NULL; dp = readdir (dirp)) {
	(void) sprintf (dirfile, "%s/%s", dir, dp -> d_name);
	chtim (dirfile);
    }
    closedir (dirp);
    return (0);
}

print_time (label, t)
char   *label;
#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || \
    defined(i64_aix) || defined(a64_lnx)
time_t  t;
#else
long    t;
#endif
{
    char   *s;
    s = (char *) ctime (&t);
    if (strncmp ((s + 20), nowyear, 4))
	printf ("\t%s %-13.12s%-5.4s(%d)\n", label, (s + 4), (s + 20), t);
    else
	printf ("\t%s %-18.15s(%d)\n", label, (s + 4), t);
}

char   *
        substring (str, substr)
char   *str, *substr;
{
    while (isspace (*str) && (*str != '\0'))
	str++;
    while (!isspace (*str) && (*str != '\0'))
	*substr++ = *str++;
    *substr = '\0';
    return (str);
}

/*
 * Convert a date to seconds since Jan 1, 1970.  If an error, return -1.
 */
time_t
unctime (str)
char   *str;
{
    struct tm   tm;
    time_t emitl ();
    char   *s, word[30], *substring ();
    if (strlen (str) >= 30)
	return (-1);

    str = substring (str, word);/* extract month number */
    if ((tm.tm_mon = get_month (word)) < 0)
	return (-1);

    str = substring (str, word);/* extract day */
    tm.tm_mday = atoi (word);

    str = substring (str, word);/* extract time */
    tm.tm_hour = atoi (word);
    for (s = word; (*s != '\0') && (*s != ':'); s++);
    if (*s == ':')
	tm.tm_min = atoi (++s);
    else
	return (-1);		/* mistake in format */
    for (; (*s != '\0') && (*s != ':'); s++);
    if (*s == ':')
	tm.tm_sec = atoi (++s);
    else
	tm.tm_sec = 0;		/* assume zero */

    str = substring (str, word);/* if no year given, */
    if (*word)			/* assume this year. */
	tm.tm_year = atoi (word) - 1900;
    else
	tm.tm_year = atoi (nowyear) - 1900;

    return (emitl (&tm));
}

get_month (str)
char   *str;
{
    int     i;
    char   *strp;
    int     uclc_diff = 'a' - 'A';
    for (strp = str; !isspace (*strp) && *strp != '\0'; strp++)
	if (isupper (*strp))
	    *strp += uclc_diff;
    for (i = 0; i < 12; i++)
	if (!strncmp (months[i], str, 3))
	    return (i);
    return (-1);
}

/*
 * Routine to convert a localtime(3) format date back into
 * a system format date.
 *
 *	Use a binary search.
 */
time_t
emitl (dp)
struct tm  *dp;
{
    time_t conv;
    register int    i, bit;
    struct tm   dcopy;

    dcopy = *dp;
    dp = &dcopy;
    conv = 0;
    for (i = 30; i >= 0; i--) {
	bit = 1 << i;
	conv |= bit;
	if (dcmp (localtime (&conv), dp) > 0)
	    conv &= ~bit;
    }
    return (conv);
}

/*
 * Compare two localtime dates, return result.
 */
#define DECIDE(a) \
if (dp -> a > dp2 -> a) \
	return (1); \
if (dp -> a < dp2 -> a) \
	return (-1)

static
dcmp (dp, dp2)
register struct tm *dp, *dp2;
{
    DECIDE (tm_year);
    DECIDE (tm_mon);
    DECIDE (tm_mday);
    DECIDE (tm_hour);
    DECIDE (tm_min);
    DECIDE (tm_sec);
    return (0);
}

usage () {
    fprintf (stderr, "Usage: %s [-sR] [-p proto-file]", command);
    fprintf (stderr, " [-am \"date\" or seconds] files ...\n");
    exit (1);
}

# ifdef SYS5
# if !defined(hp8_us5) && !defined(_XOPEN_SOURCE_EXTENDED) && \
     !defined(hpb_us5) && !defined(sgi_us5) && !defined(hp2_us5) && \
     !defined(i64_hpu) && !defined(mg5_osx) && !defined(int_osx)

int
utimes( file, newtime )
# if defined(dgi_us5) || defined(dg8_us5) || defined(int_lnx) || \
     defined(ibm_lnx) || defined(axp_lnx) || defined(a64_lnx) || \
     defined(axp_osf) || defined(int_rpl)

const char *file;
# else
char *file;
# endif
# if defined(dgi_us5) || defined(dg8_us5)
const struct timeval *newtime;
# else
struct timeval *newtime;
# endif
{
#if DGUX
    static struct utimbuf ztime;        /* guaranteed init'ed to zero */
    struct utimbuf ntime = ztime;
#else
    struct utimbuf ntime;
#endif

    ntime.actime = newtime[0].tv_sec;
    ntime.modtime = newtime[0].tv_sec;

    return( utime( file, &ntime ) );
}

# endif
# endif
