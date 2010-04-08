/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <si.h>
#include    <st.h>
#include    <lo.h>
#include    <nm.h>
#include    <pc.h>
#include    <tmtz.h>
#include    <me.h>

/*
NEEDLIBS = COMPAT
DEST = utility
**
NO_OPTIM = sqs_ptx usl_us5
*/

/**
**
**  name: iizic.c - Zoneinfo compiler for Ingres timezone support.
**
**  synopsis
**
**   iizic [ -v[erbose] ] [ -d[irectory][=]directory ] filename
**
**  description
**
**   iizic reads text from the file(s) named on the  command line
**   and  creates the time conversion information files specified
**   in this input.
**
**   For details of the rules language, please read LRC Agenda for
**   8/12/92.
**	
**  history:
**	25-sep-1992 (stevet)
**          Created.  Largely stolen from public domain version of 'iizic.c'.
**          This is only a preliminary version, further cleanup and testings
**          is required before this is ready for general use.    
**	12-jan-1993 (terjeb)
**	    Renamed optopt, optarg, optind and getopt so that they now are 
**	    called ioptopt, ioptarg, ioptind and igetopt. This cases confusion
**	    on the HP machine as these are defined in <unistd.h> as well
**	    as <stdio.h> on this machine.
**      03-mar-1993 (stevet)
**          Code clean up.  Added prototypes, moved constants from tmtzint.h 
**          over here.  Also remove direct call to qsort, added checks for 
**          return status form CL calls and fixed up the argument parsing.
**	24-aug-1993 (wolf)
**	    Add #include me.h to resolve link errors
**      13-sep-1993 (stevet)
**          Prototype main() and changed register variable to nat.
**	23-sep-1993 (walt)
**	    In TMtz_inzsub define writable copies of "Jan", "1", and "0" to
**	    pass to TMtz_rulesub. This is necessary because on Alphas (and
**	    under ANSI C) strings go into readonly memory and cannot be altered,
**	    and TMtz_rulesub can call CMtolower on these strings, getting an
**	    accvio.
**	21-oct-96 (mcgem01)
**	    Remove hard-coded references to iizic so that jasmine can
**	    rename this app.
**      28-aug-97 (musro02)
**          nooptim for sqs_ptx to avoid "ruleless zone" compiler errors.
**	20-apr-1999 (popri01)
**	    Add NO OPTIM for Unixware 7 (usl_us5) to eliminate
**	    "time overflow" msg.
**      21-apr-1999 (hanch04)
**          Replace STindex with STchr
**      09-Feb-2000 (linke01)
**          compiler error, got "time overflow" message, files can't be copied
**          to $ING_BUILD/files/zoneinfo. Added NO_OPTIM for usl_us5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-aug-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	20-oct-2001 (somsa01)
**	    Changed return of the main function, using PCexit() to end with
**	    the proper error number.
**	10-mar-2002 (somsa01)
**	    Added NEEDLIBS MING hint for successful 64-bit compilation.
**      16-May-2002 (hanch04)
**          Added lp64 directory for 32/64 bit builds.
**	20-Jul-2004 (hanje04)
**	    Set build directory to $ING_BUILD/utility
**      07-Mar-2005 (hanje04)
**          SIR 114034
**          Add support for reverse hybrid builds. First part of much wider
**          change to allow support of 32bit clients on pure 64bit platforms.
**          Changes made here allow application statically linked against
**          the standard 32bit Intel Linux release to run against the new
**          reverse hybrid ports without relinking.
**	11-Apr-2008 (hweho01)
**          a) Check whether the directory exists before calling LOcreate()
**          b) For Unix/Linux, set permissions to 0775 for the zoneinfo   
**             subdirectories.  
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/



/*  Internal symbols and structures */
#define SECSPERMIN	60
#define MINSPERHOUR	60
#define HOURSPERDAY	24
#define DAYSPERWEEK	7
#define DAYSPERNYEAR	365
#define DAYSPERLYEAR	366
#define SECSPERHOUR	(SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY	(SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR	12

#define TMTZ_SUNDAY	0
#define TMTZ_MONDAY	1
#define TMTZ_TUESDAY	2
#define TMTZ_WEDNESDAY	3
#define TMTZ_THURSDAY	4
#define TMTZ_FRIDAY	5
#define TMTZ_SATURDAY	6

#define TMTZ_JANUARY	0
#define TMTZ_FEBRUARY	1
#define TMTZ_MARCH	2
#define TMTZ_APRIL	3
#define TMTZ_MAY	4
#define TMTZ_JUNE	5
#define TMTZ_JULY	6
#define TMTZ_AUGUST	7
#define TMTZ_SEPTEMBER	8
#define TMTZ_OCTOBER	9
#define TMTZ_NOVEMBER	10
#define TMTZ_DECEMBER	11

#define TMTZ_YEAR_BASE	1900

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TMTZ_THURSDAY

/*
** Line codes.
*/

#define LC_RULE		0
#define LC_ZONE		1

/*
** Which fields are which on a Zone line.
*/

#define ZF_NAME		1
#define ZF_GMTOFF	2
#define ZF_RULE		3
#define ZF_FORMAT	4
#define ZF_TILYEAR	5
#define ZF_TILMONTH	6
#define ZF_TILDAY	7
#define ZF_TILTIME	8
#define ZONE_MINFIELDS	5
#define ZONE_MAXFIELDS	9

/*
** Which fields are which on a Zone continuation line.
*/

#define ZFC_GMTOFF	0
#define ZFC_RULE	1
#define ZFC_FORMAT	2
#define ZFC_TILYEAR	3
#define ZFC_TILMONTH	4
#define ZFC_TILDAY	5
#define ZFC_TILTIME	6
#define ZONEC_MINFIELDS	3
#define ZONEC_MAXFIELDS	7

/*
** Which files are which on a Rule line.
*/

#define RF_NAME		1
#define RF_LOYEAR	2
#define RF_HIYEAR	3
#define RF_COMMAND	4
#define RF_MONTH	5
#define RF_DAY		6
#define RF_TOD		7
#define RF_STDOFF	8
#define RF_ABBRVAR	9
#define RULE_FIELDS	10

/*
** Year synonyms.
*/

#define YR_MINIMUM	0
#define YR_MAXIMUM	1
#define YR_ONLY		2


/*
** Accurate only for the past couple of centuries;
** that will probably do.
*/

#define isleap(y) (((y) % 4) == 0 && ((y) % 100) != 0 || ((y) % 400) == 0)

#define DC_DOM		0	/* 1..31 */	/* unused */
#define DC_DOWGEQ	1	/* 1..31 */	/* 0..6 (Sun..Sat) */
#define DC_DOWLEQ	2	/* 1..31 */	/* 0..6 (Sun..Sat) */

#define MAX_YEAR    2038      
#define MIN_YEAR    1902

#define TMtz_emalloc(size)		TMtz_memcheck(TMtz_imalloc(size))
#define TMtz_erealloc(ptr, size)	TMtz_memcheck(TMtz_irealloc(ptr, size))
#define TMtz_ecpyalloc(ptr)		TMtz_memcheck(TMtz_icpyalloc(ptr))
#define TMtz_ecatalloc(oldp, newp)	TMtz_memcheck(TMtz_icatalloc(oldp, newp))

typedef struct _TMTZ_RULE 
{
    char 	*r_filename;
    i4		r_linenum;
    char 	*r_name;
    i4		r_loyear;	/* for example, 1986 */
    i4		r_hiyear;	/* for example, 1986 */
    char 	*r_yrtype;
    i4		r_month;	/* 0..11 */
    i4		r_dycode;	/* see below */
    i4		r_dayofmonth;
    i4		r_wday;
    i4	r_tod;		/* time from midnight */
    i4		r_todisstd;	/* above is standard time if TRUE */
				/* or wall clock time if FALSE */
    i4	r_stdoff;	/* offset from standard time */
    char 	*r_abbrvar;	/* variable part of abbreviation */
    i4		r_todo;		/* a rule to do (used in outzone) */
    i4	r_temp;		/* used in outzone */
}TMTZ_RULE;

typedef struct _TMTZ_ZONE {
    char 	*z_filename;
    i4	        z_linenum;
    char        *z_name;
    i4	z_gmtoff;
    char 	*z_rule;
    char 	*z_format;
    i4	z_stdoff;
    TMTZ_RULE	*z_rules;
    i4		z_nrules;
    TMTZ_RULE	z_untilrule;
    i4	z_untiltime;
}TMTZ_ZONE;

typedef struct _TMTZ_LOOKUP {
    char 	*l_word;
    i4          l_value;
}TMTZ_LOOKUP;


/*  Static function references  */
static VOID	TMtz_addtt (i4 starttime, i4  type);
static i4	TMtz_addtype (i4 gmtoff,  char * abbr, i4  isdst,
			    i4  ttisstd);
static i4	TMtz_ciequal (char * ap,  char * bp);
static VOID	TMtz_eat (char * name, i4  num);
static VOID	TMtz_eats (char * name, i4  num, char * rname, i4  rnum);
static i4	TMtz_eitol (i4 i);
static VOID	TMtz_error (char * message);
static char 	**TMtz_getfields (char * buf);
static i4	TMtz_gethms (char * string,  char * errstrng, i4  signable);
static VOID	TMtz_infile (char * filename);
static VOID	TMtz_inrule (char ** fields, i4  nfields);
static i4	TMtz_inzcont (char ** fields, i4  nfields);
static i4	TMtz_inzone (char ** fields, i4  nfields);
static i4	TMtz_inzsub (char ** fields, i4  nfields, i4  iscont);
static i4	TMtz_itsabbr (char * abbr,  char * word);
static char     *TMtz_memcheck (char * tocheck);
static VOID	TMtz_newabbr (char * abbr);
static i4	TMtz_oadd (i4 t1, i4 t2);
static i4	TMtz_rpytime (TMTZ_RULE * rp, i4  wantedy);
static VOID	TMtz_rulesub (TMTZ_RULE * rp,
			      char * loyearp, char * hiyearp,
		              char * typep, char * monthp,
		              char * dayp, char * timep);
static i4	TMtz_tadd (i4 t1, i4 t2);
static VOID	TMtz_usage (VOID);
static i4	TMtz_yearistype (i4 year,  char * type);
static VOID	TMtz_writezone ( char * name);
static char     *TMtz_icatalloc(char * old, char * new);
static char     *TMtz_icpyalloc(char * string);
static char     *TMtz_imalloc(i4 n);
static char     *TMtz_irealloc(char * pointer, i4  size);
static VOID     TMtz_ifree(char * pointer);
static VOID     TMtz_isscanf(i4 count, char * string, i4  * val1,
			     i4  *val2, i4  *val3);
static VOID	TMtz_associate(VOID);
static VOID	TMtz_outzone (TMTZ_ZONE * zp, i4  ntzones);
static VOID     TMtz_qsort(TMTZ_RULE *rules, i4  left, i4  right);
static VOID     TMtz_swap(TMTZ_RULE  *rules, i4  i, i4  j);
static TMTZ_LOOKUP  *TMtz_byword(char *string, TMTZ_LOOKUP * lp);
static STATUS   TMtz_igetopt(i4  nargc,
			    char  **nargv, 
			    char  *ostr,
			    i4    *ioptind,
			    char  *ioptarg,
			    char  *ioptopt);

/*  Static variable references  */
static i4	charcnt;
static i4	errors;
static char 	*filename;
static i4	linenum;
static i4	max_time;
static i4	min_time;
static i4	noise;
static char 	*rfilename;
static i4	rlinenum;
static i4	timecnt;
static i4	typecnt;
static char *	directory;
static i4	ats[TM_MAX_TIMECNT];
static u_char	types[TM_MAX_TIMECNT];
static i4	gmtoffs[TM_MAX_TZTYPE];
static char	isdsts[TM_MAX_TZTYPE];
static char	abbrinds[TM_MAX_TZTYPE];
static char	ttisstds[TM_MAX_TZTYPE];
static char	chars[TM_MAX_TZLBL];
static TMTZ_RULE *rules;
static i4	 nrules;	/* number of rules */
static TMTZ_ZONE *zones;
static i4	 nzones;	/* number of zones */
static char *	appname;

static TMTZ_LOOKUP 	line_codes[] = {
	"Rule",		LC_RULE,
	"Zone",		LC_ZONE,
	NULL,		0
};
static TMTZ_LOOKUP 	mon_names[] = {
	"January",	TMTZ_JANUARY,
	"February",	TMTZ_FEBRUARY,
	"March",	TMTZ_MARCH,
	"April",	TMTZ_APRIL,
	"May",		TMTZ_MAY,
	"June",		TMTZ_JUNE,
	"July",		TMTZ_JULY,
	"August",	TMTZ_AUGUST,
	"September",	TMTZ_SEPTEMBER,
	"October",	TMTZ_OCTOBER,
	"November",	TMTZ_NOVEMBER,
	"December",	TMTZ_DECEMBER,
	NULL,		0
};
static TMTZ_LOOKUP 	wday_names[] = {
	"Sunday",	TMTZ_SUNDAY,
	"Monday",	TMTZ_MONDAY,
	"Tuesday",	TMTZ_TUESDAY,
	"Wednesday",	TMTZ_WEDNESDAY,
	"Thursday",	TMTZ_THURSDAY,
	"Friday",	TMTZ_FRIDAY,
	"Saturday",	TMTZ_SATURDAY,
	NULL,		0
};
static TMTZ_LOOKUP 	lasts[] = {
	"last-Sunday",		TMTZ_SUNDAY,
	"last-Monday",		TMTZ_MONDAY,
	"last-Tuesday",		TMTZ_TUESDAY,
	"last-Wednesday",	TMTZ_WEDNESDAY,
	"last-Thursday",	TMTZ_THURSDAY,
	"last-Friday",		TMTZ_FRIDAY,
	"last-Saturday",	TMTZ_SATURDAY,
	NULL,			0
};
static TMTZ_LOOKUP 	begin_years[] = {
	"minimum",	YR_MINIMUM,
	"maximum",	YR_MAXIMUM,
	NULL,		0
};
static TMTZ_LOOKUP 	end_years[] = {
	"minimum",	YR_MINIMUM,
	"maximum",	YR_MAXIMUM,
	"only",		YR_ONLY,
	NULL,		0
};
static  i4	len_months[2][MONSPERYEAR] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
static  i4	len_years[2] = {
	DAYSPERNYEAR, DAYSPERLYEAR
};


int
main(
i4  argc, 
char * argv[])
{
    i4	     i, j;
    char     c;
    char     ioptarg[MAX_LOC + 1];
    i4       ioptind = 1;	/* index into parent argv vector */
    STATUS   status;

    appname = argv[0];

    while( (status = TMtz_igetopt(argc, argv, "d:v", &ioptind, ioptarg, &c))
	   == OK)
    {
	if( c == ' ')
	    break;
	switch (c) 
	{
	  case 'd':
	    directory = ioptarg;
	    break;
	  case 'v':
	    noise = TRUE;
	    break;
	  default:
	    TMtz_usage();
	    PCexit(FAIL);
	    break;
	}
    }

    /* if no filename specified, return error */
    if (status != OK || ioptind == argc )
    {
	TMtz_usage();	/* usage message by request */
        PCexit(FAIL);
    }

    zones = (TMTZ_ZONE *) TMtz_emalloc(0);
    rules = (TMTZ_RULE *) TMtz_emalloc(0);
    /* Get rules files */
    for (i = ioptind; i < argc; ++i)
	TMtz_infile(argv[i]);
    if (errors)
	PCexit(FAIL);
    /* Order rules */
    TMtz_associate();
    /* Write output */
    for (i = 0; i < nzones; i = j) 
    {
	/*
	** Find the next non-continuation zone entry.
	*/
	for (j = i + 1; j < nzones && zones[j].z_name == NULL; ++j)
		;
	TMtz_outzone(&zones[i], j - i);
    }
    (errors == 0) ? PCexit(OK) : PCexit(FAIL);
}




/*
** Memory allocation.
*/

static char *
TMtz_memcheck(
char  	*ptr)
{
    if (ptr == NULL) 
    {
	(VOID) perror(appname);
	PCexit(FAIL);
    }
    return ptr;
}


/*
** Error handling.
*/

static VOID
TMtz_eats(
char  	*name,
i4	num,
char  	*rname,
i4	rnum)
{
    filename = name;
    linenum = num;
    rfilename = rname;
    rlinenum = rnum;
}

static VOID
TMtz_eat(
char  	*name,
i4	num)
{
    TMtz_eats(name, num, (char *) NULL, -1);
}

static VOID
TMtz_error(
char  	*string)
{
    (VOID) SIprintf("\"%s\", line %d: %s\n", filename, linenum, string);
    if (rfilename != NULL)
	(VOID) SIprintf(" (rule from \"%s\", line %d)",	rfilename, rlinenum);
    (VOID) SIprintf("\n");
    ++errors;
}

static VOID
TMtz_usage()
{
    (VOID) SIprintf(
	    "%s: usage is %s [ -v[erbose]] [ -d[irectory][=] directory ] filename [filename ... ]\n", appname, appname);
}

static VOID
TMtz_associate()
{
	 TMTZ_ZONE *	zp;
	 TMTZ_RULE *	rp;
	 i4		base, out;
	 i4		i;

	if (nrules != 0)
		(VOID) TMtz_qsort(rules, 0, nrules-1);
	for (i = 0; i < nzones; ++i) {
		zp = &zones[i];
		zp->z_rules = NULL;
		zp->z_nrules = 0;
	}
	for (base = 0; base < nrules; base = out) {
		rp = &rules[base];
		for (out = base + 1; out < nrules; ++out)
			if (strcmp(rp->r_name, rules[out].r_name) != 0)
				break;
		for (i = 0; i < nzones; ++i) {
			zp = &zones[i];
			if (strcmp(zp->z_rule, rp->r_name) != 0)
				continue;
			zp->z_rules = rp;
			zp->z_nrules = out - base;
		}
	}
	for (i = 0; i < nzones; ++i) {
		zp = &zones[i];
		if (zp->z_nrules == 0) {
			/*
			** Maybe we have a local standard time offset.
			*/
			TMtz_eat(zp->z_filename, zp->z_linenum);
			zp->z_stdoff = TMtz_gethms(zp->z_rule, "unruly zone",
						   TRUE);
			/*
			** Note, though, that if there's no rule,
			** a '%s' in the format is a bad thing.
			*/
			if (strchr(zp->z_format, '%') != 0)
				TMtz_error("%s in ruleless zone");
		}
	}
	if (errors)
		PCexit(FAIL);
}

static VOID
TMtz_infile(
char *	name)
{
    FILE 	*fp;
    char 	**fields;
    char 	*cp;
    TMTZ_LOOKUP  *lp;
    i4		nfields;
    i4		wantcont;
    i4		num;
    char	buf[BUFSIZ];
    char        mchar;
    STATUS      status;
    LOCATION    loc;


    LOfroms(FILENAME&PATH, name, &loc);

    if (status = SIfopen( &loc, "r", SI_VAR, sizeof buf, &fp)) {
	(VOID) SIprintf("%s: Can't open \n", appname);
	(VOID) perror(name);
	PCexit(FAIL);
    }
    wantcont = FALSE;
    for (num = 1; ; ++num) {
	/* Store file name */
	TMtz_eat(name, num);
	if ((status = SIgetrec(buf, (i4) sizeof buf, fp)) != OK)
		break;
	mchar = '\n';
	cp = STchr(buf, mchar);
	if (cp == NULL) {
		TMtz_error("line too long");
		PCexit(FAIL);
	}
	*cp = '\0';
	fields = TMtz_getfields(buf);
	nfields = 0;
	while (fields[nfields] != NULL) {
	        if (TMtz_ciequal(fields[nfields], "-"))
		    fields[nfields] = "";
		++nfields;
	}
	if (nfields != 0)
	{
	    if(wantcont) 
		wantcont = TMtz_inzcont(fields, nfields);
	    else 
	    {
		lp = TMtz_byword(fields[0], line_codes);
		if (lp == NULL)
		    TMtz_error("input line of unknown type");
		else
		{
		    switch ((i4) (lp->l_value)) 
		    {
		      case LC_RULE:
			TMtz_inrule(fields, nfields);
			wantcont = FALSE;
			break;
		      case LC_ZONE:
			wantcont = TMtz_inzone(fields, nfields);
			break;
		      default:	/* "cannot happen" */
			(VOID) SIprintf("%s: panic: Invalid l_value %d\n",
				appname, lp->l_value);
			PCexit(FAIL);
		    }
		}
	    }
	    TMtz_ifree( (char *) fields);
	}
    }
    if (SIclose(fp)) {
	(VOID) SIprintf("%s: Error closing \n", appname);
	(VOID) perror(filename);
	PCexit(FAIL);
    }
    if (wantcont)
	TMtz_error("expected continuation line not found");
}

/*
** Convert a string of one of the forms
**	h	-h 	hh:mm	-hh:mm	hh:mm:ss	-hh:mm:ss
** into a number of seconds.
** A null string maps to zero.
** Call error with errstring and return zero on errors.
*/

static i4
TMtz_gethms(
char 	*string,
char  	*errstring,
i4	signable)
{
    i4	hh, mm, ss, sign;

    if (string == NULL || *string == '\0')
	return 0;
    if (!signable)
	sign = 1;
    else if (*string == '-') {
	sign = -1;
	++string;
    } else	sign = 1;

    TMtz_isscanf(3, string, &hh, &mm, &ss);
    if (hh < 0 || hh >= HOURSPERDAY ||
	mm < 0 || mm >= MINSPERHOUR ||
	ss < 0 || ss > SECSPERMIN) 
    {
	TMtz_error(errstring);
	return 0;
    }
    return TMtz_eitol(sign) *
	(TMtz_eitol(hh * MINSPERHOUR + mm) *
	TMtz_eitol(SECSPERMIN) + TMtz_eitol(ss));
}

static VOID
TMtz_inrule(
char    **fields,
i4	nfields)
{
    static TMTZ_RULE	r;

    if (nfields != RULE_FIELDS) 
    {
	TMtz_error("wrong number of fields on Rule line");
	return;
    }
    if (*fields[RF_NAME] == '\0') 
    {
	TMtz_error("nameless rule");
	return;
    }
    r.r_filename = filename;
    r.r_linenum = linenum;
    r.r_stdoff = TMtz_gethms(fields[RF_STDOFF], "invalid saved time", TRUE);
    TMtz_rulesub(&r, fields[RF_LOYEAR], fields[RF_HIYEAR], fields[RF_COMMAND],
		 fields[RF_MONTH], fields[RF_DAY], fields[RF_TOD]);
    r.r_name = TMtz_ecpyalloc(fields[RF_NAME]);
    r.r_abbrvar = TMtz_ecpyalloc(fields[RF_ABBRVAR]);
    rules = (TMTZ_RULE *) TMtz_erealloc((char *) rules,
	       (i4) ((nrules + 1) * sizeof *rules));
    rules[nrules++] = r;
}

static i4
TMtz_inzone(
char    **fields,
i4	nfields)
{
    i4	    i;
    char    buf[132];

    if (nfields < ZONE_MINFIELDS || nfields > ZONE_MAXFIELDS) 
    {
	TMtz_error("wrong number of fields on Zone line");
	return FALSE;
    }
    for (i = 0; i < nzones; ++i)
	if (zones[i].z_name != NULL &&
		STcompare(zones[i].z_name, fields[ZF_NAME]) == 0) 
	{
	    (VOID) STprintf(buf,
			    "duplicate zone name %s (file \"%s\", line %d)",
			    fields[ZF_NAME], zones[i].z_filename, 
			    zones[i].z_linenum);
	    TMtz_error(buf);
	    return FALSE;
	}
    return TMtz_inzsub(fields, nfields, FALSE);
}

static i4
TMtz_inzcont(
char  	**fields,
i4	nfields)
{
    if (nfields < ZONEC_MINFIELDS || nfields > ZONEC_MAXFIELDS) 
    {
	TMtz_error("wrong number of fields on Zone continuation line");
	return FALSE;
    }
    return TMtz_inzsub(fields, nfields, TRUE);
}

static i4
TMtz_inzsub(fields, nfields, iscont)
char    **fields;
i4	nfields;
i4	iscont;
{
    char *		cp;
    char                chr='%';
    TMTZ_ZONE	        z;
    i4		        i_gmtoff, i_rule, i_format;
    i4		        i_untilyear, i_untilmonth;
    i4		        i_untilday, i_untiltime;
    i4		        hasuntil;

    /*
    **	23-sep-1993 (walt)
    **	Define writable copies of "Jan", "1", and "0" to pass to TMtz_rulesub.
    **	This is necessary because on Alphas (and under ANSI C) strings go into
    **	readonly memory and cannot be altered - and TMtz_rulesub can call
    **	CMtolower on these strings, getting an accvio.
    */
    char    jan_string[] = "Jan";
    char    one_string[] = "1";
    char    zero_string[] = "0";

    if (iscont) 
    {
	i_gmtoff = ZFC_GMTOFF;
	i_rule = ZFC_RULE;
	i_format = ZFC_FORMAT;
	i_untilyear = ZFC_TILYEAR;
	i_untilmonth = ZFC_TILMONTH;
	i_untilday = ZFC_TILDAY;
	i_untiltime = ZFC_TILTIME;
	z.z_name = NULL;
    } 
    else 
    {
	i_gmtoff = ZF_GMTOFF;
	i_rule = ZF_RULE;
	i_format = ZF_FORMAT;
	i_untilyear = ZF_TILYEAR;
	i_untilmonth = ZF_TILMONTH;
	i_untilday = ZF_TILDAY;
	i_untiltime = ZF_TILTIME;
	z.z_name = TMtz_ecpyalloc(fields[ZF_NAME]);
    }
    z.z_filename = filename;
    z.z_linenum = linenum;
    z.z_gmtoff = TMtz_gethms(fields[i_gmtoff], "invalid GMT offset", TRUE);
    if ((cp = STchr(fields[i_format], chr)) != 0) 
    {
	if (*++cp != 's' || STchr(cp, chr) != NULL) 
	{
	    TMtz_error("invalid abbreviation format");
		return FALSE;
	}
    }
    z.z_rule = TMtz_ecpyalloc(fields[i_rule]);
    z.z_format = TMtz_ecpyalloc(fields[i_format]);
    hasuntil = nfields > i_untilyear;
    if (hasuntil) 
    {
	z.z_untilrule.r_filename = filename;
	z.z_untilrule.r_linenum = linenum;
	TMtz_rulesub(&z.z_untilrule,
		     fields[i_untilyear],
		     "only",
		     "",
		     (nfields > i_untilmonth) ? fields[i_untilmonth] :
		     jan_string,
		     (nfields > i_untilday) ? fields[i_untilday] : one_string,
		     (nfields > i_untiltime) ? fields[i_untiltime] :
		     zero_string);
	z.z_untiltime = TMtz_rpytime(&z.z_untilrule, z.z_untilrule.r_loyear);
	if (iscont && nzones > 0 && z.z_untiltime < max_time &&
		z.z_untiltime > min_time &&
		zones[nzones - 1].z_untiltime >= z.z_untiltime) 
	{
	    TMtz_error("Zone continuation line end time is not after end time of previous line");
	    return FALSE;
	}
    }
    zones = (TMTZ_ZONE *) TMtz_erealloc((char *) zones,
		(i4) ((nzones + 1) * sizeof *zones));
    zones[nzones++] = z;
    /*
    ** If there was an UNTIL field on this line,
    ** there's more information about the zone on the next line.
    */
    return hasuntil;
}

static VOID
TMtz_rulesub(
TMTZ_RULE *rp,
char  	 *loyearp,
char  	 *hiyearp,
char  	 *typep,
char  	 *monthp,
char  	 *dayp,
char  	 *timep)
{
    TMTZ_LOOKUP  *lp;
    char 	*cp;
    char        chr_lt = '<';
    char        chr_gt = '>';
    if ((lp = TMtz_byword(monthp, mon_names)) == NULL) 
    {
	TMtz_error("invalid month name");
	return;
    }
    rp->r_month = lp->l_value;
    rp->r_todisstd = FALSE;
    cp = timep;
    if (*cp != '\0') 
    {
	cp += STlength(cp) - 1;
	CMtolower(cp, cp);
	switch (*cp) 
	{
	  case 's':
	    rp->r_todisstd = TRUE;
	    *cp = '\0';
	    break;
	  case 'w':
	    rp->r_todisstd = FALSE;
	    *cp = '\0';
	    break;
	}
    }
    rp->r_tod = TMtz_gethms(timep, "invalid time of day", FALSE);
    /*
    ** Year work.
    */
    cp = loyearp;
    if ((lp = TMtz_byword(cp, begin_years)) != NULL) 
	switch ((i4) lp->l_value) 
	{
	  case YR_MINIMUM:
	    rp->r_loyear = MIN_YEAR;
	    break;
	  case YR_MAXIMUM:
	    rp->r_loyear = MAX_YEAR;
	    break;
	  default:	/* "cannot happen" */
	    (VOID) SIprintf("%s: panic: Invalid l_value %d\n",
			    appname, lp->l_value);
	    PCexit(FAIL);
	} 
    else 
    {
	TMtz_isscanf( 1, cp, &rp->r_loyear, NULL, NULL);
	if (rp->r_loyear < MIN_YEAR || rp->r_loyear > MAX_YEAR) 
	{
	    TMtz_error("invalid starting year");
	    if (rp->r_loyear > MAX_YEAR)
		return;
	}
    }
    cp = hiyearp;
    if ((lp = TMtz_byword(cp, end_years)) != NULL) 
	switch ((i4) lp->l_value) 
	{
	  case YR_MINIMUM:
	    rp->r_hiyear = MIN_YEAR;
	    break;
	  case YR_MAXIMUM:
	    rp->r_hiyear = MAX_YEAR;
	    break;
	  case YR_ONLY:
	    rp->r_hiyear = rp->r_loyear;
	    break;
	  default:	/* "cannot happen" */
	    (VOID) SIprintf("%s: panic: Invalid l_value %d\n",
			    appname, lp->l_value);
	    PCexit(FAIL);
	} 
    else
    {
	TMtz_isscanf( 1, cp, &rp->r_hiyear, NULL, NULL);
	if (rp->r_hiyear < MIN_YEAR || rp->r_hiyear > MAX_YEAR) 
	{
	    TMtz_error("invalid ending year");
	    if (rp->r_hiyear < MIN_YEAR)
		return;
	}
    }
    if (rp->r_hiyear < MIN_YEAR)
 	return;
    if (rp->r_loyear < MIN_YEAR)
 	rp->r_loyear = MIN_YEAR;
    if (rp->r_hiyear > MAX_YEAR)
	rp->r_hiyear = MAX_YEAR;
    if (rp->r_loyear > rp->r_hiyear) 
    {
	TMtz_error("starting year greater than ending year");
	return;
    }
    if (*typep == '\0')
	rp->r_yrtype = NULL;
    else 
    {
	if (rp->r_loyear == rp->r_hiyear) 
	{
	    TMtz_error("typed single year");
		return;
	}
	rp->r_yrtype = TMtz_ecpyalloc(typep);
    }
    /*
    ** Day work.
    ** Accept things such as:
    **	1
    **	last-Sunday
    **	Sun<=20
    **	Sun>=7
    */
    if ((lp = TMtz_byword(dayp, lasts)) != NULL) 
    {
	rp->r_dycode = DC_DOWLEQ;
	rp->r_wday = lp->l_value;
	rp->r_dayofmonth = len_months[1][rp->r_month];
    } 
    else 
    {

	if ((cp = STchr(dayp, chr_lt)) != 0)
	    rp->r_dycode = DC_DOWLEQ;
	else if ((cp = STchr(dayp, chr_gt)) != 0)
	    rp->r_dycode = DC_DOWGEQ;
	else 
	{
	    cp = dayp;
	    rp->r_dycode = DC_DOM;
	}
	if (rp->r_dycode != DC_DOM) 
	{
	    *cp++ = 0;
	    if (*cp++ != '=') 
	    {
		TMtz_error("invalid day of month");
		return;
	    }
	    if ((lp = TMtz_byword(dayp, wday_names)) == NULL) 
	    {
		TMtz_error("invalid weekday name");
		return;
	    }
	    rp->r_wday = lp->l_value;
	}
	TMtz_isscanf(1, cp, &rp->r_dayofmonth, NULL, NULL);
	if (rp->r_dayofmonth <= 0 ||
	    (rp->r_dayofmonth > len_months[1][rp->r_month])) 
	{
	    TMtz_error("invalid day of month");
	    return;
	}
    }
}

/*
** Write timezone information file
*/
static VOID
TMtz_writezone(
char * 	name)
{
    FILE          *fid;
    LOCATION      loc, loc2;
    char          dirname[MAX_LOC];
    char          filename[MAX_LOC];
    char          *cp=name;
    char          *lcp=name;
    TM_TZ_CB      tz_cb;
    u_char        tz_type[TM_MAX_TIMECNT];
    i4            tz_time[TM_MAX_TIMECNT];
    i4       i, count;
    TM_TZINFO        *ttisp;
    STATUS           status=OK;
    char             chr='/';
    

    /* Create directory */
    if(directory == NULL)
    {
	NMloc(FILES, PATH, "zoneinfo", &loc);
#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
        LOfaddpath(&loc, "lp64", &loc);
#endif
#if defined(conf_BUILD_ARCH_64_32) && defined(BUILD_ARCH32)
	{
	    /*
	    ** Reverse hybrid support must be available in ALL
	    ** 32bit binaries
	    */
	    char    *rhbsup;
	    NMgtAt("II_LP32_ENABLED", &rhbsup);
	    if ( (rhbsup && *rhbsup) &&
	     ( !(STbcompare(rhbsup, 0, "ON", 0, TRUE)) ||
	     !(STbcompare(rhbsup, 0, "TRUE", 0, TRUE))))
	        status = LOfaddpath(&loc, "lp32", &loc);
	}
#endif /* reverse hybrid */

    }
    else
    {
	if (STlength(directory) + 1 + STlength(name) >= MAX_LOC) 
	{
	    SIprintf("%s: File name %s/%s too long\n", 
		     appname, directory, name);
	    PCexit(FAIL);
	}

	STcopy(directory, dirname);
	LOfroms(PATH, dirname, &loc);
    }

    STcopy(name,filename);

    /* name might be in directory structure */
    do{
	while ((cp = STchr(cp, chr)) != 0) 
	{
	    *cp = '\0';
	    STcopy(name,filename);
	    if((status = LOfaddpath(&loc, lcp, &loc)) != OK)
		break;
	    if( LOexist( &loc ) != OK )
	    {
		if( ( status = LOcreate(&loc) ) != OK )
		    break;
#ifdef UNIX
		/* Set correct permissions for time zone directories */ 
		if( (status = chmod( loc.string, 0775)) != OK ) 
		    break;
#endif /* UNIX */
	    }
	    *cp = '/';
	    CMnext(cp);
	    lcp = cp;
	    STcopy(cp,filename);
	}    
	if(status != OK)
	    break;
	if((status = LOfroms(FILENAME, filename, &loc2)) != OK)
	    break;
	if((status = LOstfile(&loc2, &loc)) != OK)
	    break;
	status = SIfopen( &loc, "w", SI_VAR, 0, &fid);
    }while( FALSE);
    if (status != OK)
    {
	SIprintf("%s: Cannot create %s\n", appname, loc.string);
	PCexit(FAIL);
    }

    for (i = 0; i < timecnt; ++i) 
	I4ASSIGN_MACRO( ats[i], tz_time[i]);

    tz_cb.timecnt = timecnt;
    for (i = 0; i < timecnt; ++i) 
	tz_type[i] = types[i];
    for (i = 0; i < typecnt; ++i) 
    {
	ttisp = &tz_cb.tzinfo[i];
	ttisp->gmtoff=gmtoffs[i];
	ttisp->isdst = (u_char) isdsts[i];
	if (ttisp->isdst != 0 && ttisp->isdst != 1)
		return;
	ttisp->abbrind = abbrinds[i];
	if (ttisp->abbrind < 0 ||
	    ttisp->abbrind > charcnt)
	    return;
    }
    for (i = 0; i < charcnt; ++i)
	tz_cb.tzlabel[i] = chars[i];
    tz_cb.tzlabel[i] = '\0';    /* ensure '\0' at end */
    
    do{
	if((status = SIwrite( sizeof( TM_TZ_CB), (char *)&tz_cb, &count,
		     fid)) != OK)
	    break;
	if((status = SIwrite( (i4)tz_cb.timecnt,(char *)&tz_type, 
		     &count, fid)) != OK)
	    break;
	if((status = SIwrite((i4)(tz_cb.timecnt*4),
		     (char *)&tz_time, &count, fid)) != OK)
	    break;
	SIclose(fid);
    }while( FALSE);

    if (status != OK)
    {
	SIprintf("%s: Error writing timezone file: %s\n",
		     appname, loc.string);
	PCexit(FAIL);
    }
    else if( noise == TRUE)
    {
	SIprintf("Successfully created timezone information file: %s\n", 
		     loc.string);	
    }

}

static VOID
TMtz_outzone(zpfirst, zonecount)
TMTZ_ZONE   *zpfirst;
i4	   zonecount;
{
    TMTZ_ZONE 	*zp;
    TMTZ_RULE 	*rp;
    i4		i, j;
    i4		usestart, useuntil;
    i4	starttime, untiltime;
    i4	gmtoff;
    i4	stdoff;
    i4		year;
    i4	startoff;
    i4		startisdst;
    i4		startttisstd;
    i4		type;
    char	startbuf[BUFSIZ];

    /*
    ** Now. . .finally. . .generate some useful data!
    */
    timecnt = 0;
    typecnt = 0;
    charcnt = 0;
    /*
    ** Two guesses. . .the second may well be corrected later.
    */
    gmtoff = zpfirst->z_gmtoff;
    stdoff = 0;
    for (i = 0; i < zonecount; ++i) 
    {
	usestart = i > 0;
	useuntil = i < (zonecount - 1);
	zp = &zpfirst[i];
	TMtz_eat(zp->z_filename, zp->z_linenum);
	startisdst = -1;
	if (zp->z_nrules == 0) 
	{
	    type = TMtz_addtype(TMtz_oadd(zp->z_gmtoff, zp->z_stdoff),
				zp->z_format, zp->z_stdoff != 0,
				startttisstd);
	    if (usestart)
		TMtz_addtt(starttime, type);
	    gmtoff = zp->z_gmtoff;
	    stdoff = zp->z_stdoff;
	} 
	else 
	    for (year = MIN_YEAR; year <= MAX_YEAR; ++year) 
	    {
		if (useuntil && year > zp->z_untilrule.r_hiyear)
		    break;
		/*
		** Mark which rules to do in the current year.
		** For those to do, calculate rpytime(rp, year);
		*/
		for (j = 0; j < zp->z_nrules; ++j) 
		{
		    rp = &zp->z_rules[j];
		    TMtz_eats(zp->z_filename, zp->z_linenum,
			 rp->r_filename, rp->r_linenum);
		    rp->r_todo = year >= rp->r_loyear &&
			year <= rp->r_hiyear &&
			    TMtz_yearistype(year, rp->r_yrtype);
		    if (rp->r_todo)
			rp->r_temp = TMtz_rpytime(rp, year);
		}
		for ( ; ; ) 
		{
		    i4	k;
		    i4	jtime, ktime;
		    i4	offset;
		    char	buf[BUFSIZ];

		    if (useuntil) 
		    {
			/*
			** Turn untiltime into GMT
			** assuming the current gmtoff and
			** stdoff values.
			*/
			offset = gmtoff;
			if (!zp->z_untilrule.r_todisstd)
				offset = TMtz_oadd(offset, stdoff);
			untiltime = TMtz_tadd(zp->z_untiltime, -offset);
		    }
		    /*
		    ** Find the rule (of those to do, if any)
		    ** that takes effect earliest in the year.
		    */
		    k = -1;
		    for (j = 0; j < zp->z_nrules; ++j) 
		    {
			rp = &zp->z_rules[j];
			if (!rp->r_todo)
			    continue;
			TMtz_eats(zp->z_filename, zp->z_linenum,
				rp->r_filename, rp->r_linenum);
			offset = gmtoff;
			if (!rp->r_todisstd)
			    offset = TMtz_oadd(offset, stdoff);
			jtime = rp->r_temp;
			if (jtime == min_time ||
			    jtime == max_time)
			    continue;
			jtime = TMtz_tadd(jtime, -offset);
			if (k < 0 || jtime < ktime) 
			{
			    k = j;
			    ktime = jtime;
			}
		    }
		    if (k < 0)
			break;	/* go on to next year */
		    rp = &zp->z_rules[k];
		    rp->r_todo = FALSE;
		    if (useuntil && ktime >= untiltime)
			break;
		    if (usestart) 
		    {
			if (ktime < starttime) 
			{
			    stdoff = rp->r_stdoff;
			    startoff = TMtz_oadd(zp->z_gmtoff,
					    rp->r_stdoff);
			    (VOID) STprintf(startbuf,
					    zp->z_format,
					    rp->r_abbrvar);
			    startisdst = rp->r_stdoff != 0;
			    continue;
			}
			if (ktime != starttime && startisdst >= 0)
			    TMtz_addtt(starttime, 
				       TMtz_addtype(startoff, startbuf, 
						    startisdst, startttisstd));
			usestart = FALSE;
		    }
		    TMtz_eats(zp->z_filename, zp->z_linenum,
			 rp->r_filename, rp->r_linenum);
		    (VOID) STprintf(buf, zp->z_format,
				    rp->r_abbrvar);
		    offset = TMtz_oadd(zp->z_gmtoff, rp->r_stdoff);
		    type = TMtz_addtype(offset, buf, rp->r_stdoff != 0,
					rp->r_todisstd);
		    if (timecnt != 0 || rp->r_stdoff != 0)
			TMtz_addtt(ktime, type);
			gmtoff = zp->z_gmtoff;
			stdoff = rp->r_stdoff;
		}
	    }
	/*
	** Now we may get to set starttime for the next zone line.
	*/
	if (useuntil) 
	{
	    starttime = TMtz_tadd(zp->z_untiltime,
			          -gmtoffs[types[timecnt - 1]]);
	    startttisstd = zp->z_untilrule.r_todisstd;
	}
    }
    TMtz_writezone(zpfirst->z_name);
}

static VOID
TMtz_addtt(
i4	starttime,
i4	type)
{
    if (timecnt != 0 && type == types[timecnt - 1])
	return;	/* easy enough! */
    if (timecnt >= TM_MAX_TIMECNT) 
    {
	TMtz_error("too many transitions?!");
	PCexit(FAIL);
    }
    ats[timecnt] = starttime;
    types[timecnt] = (u_char)type;
    ++timecnt;
}

static i4
TMtz_addtype(
i4	gmtoff,
char  	*abbr,
i4	isdst,
i4	ttisstd)
{
    i4	i, j;

    /*
    ** See if there's already an entry for this zone type.
    ** If so, just return its index.
    */
    for (i = 0; i < typecnt; ++i) 
    {
	if (gmtoff == gmtoffs[i] && isdst == isdsts[i] &&
		strcmp(abbr, &chars[abbrinds[i]]) == 0 &&
		ttisstd == ttisstds[i])
			return i;
    }
    /*
    ** There isn't one; add a new one, unless there are already too
    ** many.
    */
    if (typecnt >= TM_MAX_TZTYPE) 
    {
	TMtz_error("too many local time types");
	PCexit(FAIL);
    }
    gmtoffs[i] = gmtoff;
    isdsts[i] = (char)isdst;
    ttisstds[i] = (char)ttisstd;

    for (j = 0; j < charcnt; ++j)
	if (strcmp(&chars[j], abbr) == 0)
		break;
    if (j == charcnt)
	TMtz_newabbr(abbr);
    abbrinds[i] = (char)j;
    ++typecnt;
    return i;
}

static i4
TMtz_yearistype(
i4	year,
char  	*type)
{
    if (type == NULL || *type == '\0')
	return TRUE;
    if (STcompare(type, "uspres") == 0)
	return (year % 4) == 0;
    if (STcompare(type, "nonpres") == 0)
	return (year % 4) != 0;
    for ( ; ; )
	PCexit(FAIL);
}

/* case-insensitive equality */
static i4
TMtz_ciequal(
char *	ap,
char *	bp)
{
    if(STbcompare( ap, 0, bp, 0, TRUE) == 0)
	return TRUE;
    return FALSE;
}

/* Check to see if we have a abbreviated string */
static i4
TMtz_itsabbr(abbr, word)
char *	abbr;
char *	word;
{
    while(*abbr != '\0')
    {
	while(!CMalpha(word) && !CMdigit(word) && *word != '\0')
	    CMnext(word);
	if(*word == '\0' || CMcmpnocase( abbr, word) != 0)
	    return FALSE;
	CMnext(abbr);
	CMnext(word);
    }
    return TRUE;
}

static  TMTZ_LOOKUP *
TMtz_byword(
char * 		word,
TMTZ_LOOKUP * 	table)
{
	  TMTZ_LOOKUP *	foundlp;
	  TMTZ_LOOKUP *	lp;

	if (word == NULL || table == NULL)
		return NULL;
	/*
	** Look for exact match.
	*/
	for (lp = table; lp->l_word != NULL; ++lp)
		if (TMtz_ciequal(word, lp->l_word))
			return lp;
	/*
	** Look for inexact match.
	*/
	foundlp = NULL;
	for (lp = table; lp->l_word != NULL; ++lp)
		if (TMtz_itsabbr(word, lp->l_word))
			if (foundlp == NULL)
				foundlp = lp;
			else	return NULL;	/* multiple inexact matches */
	return foundlp;
}

static char **
TMtz_getfields(
char *	cp)
{
    char 	*dp;
    char 	**array;
    i4	        nsubs;

    if (cp == NULL)
	return NULL;
    array = (char **) TMtz_emalloc((i4) ((STlength(cp) + 1) * sizeof *array));
    nsubs = 0;
    for ( ; ; ) 
    {
	while (CMspace(cp) || (!CMprint(cp) && *cp != '\0'))
		CMnext(cp);
	if (*cp == '\0' || *cp == '#')
		break;
	array[nsubs++] = dp = cp;
	do 
	{
	    if ((*dp = *cp) != '"')
	    {
		CMnext(dp);
		CMnext(cp);
	    }
	    else
	    {
		CMnext(cp);
		while ((*dp = *cp) != '"')
		{
		    CMnext(cp);
		    if (*dp != '\0')
			CMnext(dp);
		    else	
			TMtz_error("Odd number of quotation marks");
		}
		CMnext(cp);
	    }
	} while (*cp != '\0' && *cp != '#' && CMprint(cp) && !CMspace(cp));
	if (CMspace(cp) || !CMprint(cp))
	    CMnext(cp);
	*dp = '\0';
    }
    array[nsubs] = NULL;
    return array;
}

/* Calculate overflow */
static i4
TMtz_oadd(
i4	t1,
i4	t2)
{
    i4	t;

    t = t1 + t2;
    if (t2 > 0 && t <= t1 || t2 < 0 && t >= t1) 
    {
	TMtz_error("time overflow");
	PCexit(FAIL);
    }
    return t;
}

static i4
TMtz_tadd(
i4	t1,
i4	t2)
{
    i4	t;

    if (t1 == max_time && t2 > 0)
	return max_time;
    if (t1 == min_time && t2 < 0)
	return min_time;
    t = t1 + t2;
    if (t2 > 0 && t <= t1 || t2 < 0 && t >= t1) 
    {
	TMtz_error("time overflow");
	PCexit(FAIL);
    }
    return t;
}

/*
** Given a rule, and a year, compute the date - in seconds since January 1,
** 1970, 00:00 LOCAL time - in that year that the rule refers to.
*/

static i4
TMtz_rpytime(
TMTZ_RULE 	*rp,
i4	        wantedy)
{
    i4	        y, m, i;
    i4	dayoff;			/* with a nod to Margaret O. */
    i4	t;

    dayoff = 0;
    m = TMTZ_JANUARY;
    y = EPOCH_YEAR;
    while (wantedy != y) 
    {
	if (wantedy > y) 
	{
	    i = len_years[isleap(y)];
		++y;
	} 
	else 
	{
	    --y;
	    i = -len_years[isleap(y)];
	}
	dayoff = TMtz_oadd(dayoff, TMtz_eitol(i));
    }
    while (m != rp->r_month) 
    {
	i = len_months[isleap(y)][m];
	dayoff = TMtz_oadd(dayoff, TMtz_eitol(i));
	++m;
    }
    i = rp->r_dayofmonth;
    if (m == TMTZ_FEBRUARY && i == 29 && !isleap(y)) 
    {
	if (rp->r_dycode == DC_DOWLEQ)
		--i;
	else 
	{
	    TMtz_error("use of 2/29 in non leap-year");
	    PCexit(FAIL);
	}
    }
    --i;
    dayoff = TMtz_oadd(dayoff, TMtz_eitol(i));
    if (rp->r_dycode == DC_DOWGEQ || rp->r_dycode == DC_DOWLEQ) 
    {
	i4	wday;

#define LDAYSPERWEEK	((i4) DAYSPERWEEK)
	wday = TMtz_eitol(EPOCH_WDAY);
	/*
	** Don't trust mod of negative numbers.
	*/
	if (dayoff >= 0)
	    wday = (wday + dayoff) % LDAYSPERWEEK;
	else 
	{
	    wday -= ((-dayoff) % LDAYSPERWEEK);
	    if (wday < 0)
		wday += LDAYSPERWEEK;
	}
	while (wday != TMtz_eitol(rp->r_wday))
	    if (rp->r_dycode == DC_DOWGEQ) 
	    {
		dayoff = TMtz_oadd(dayoff, (i4) 1);
		if (++wday >= LDAYSPERWEEK)
			wday = 0;
		++i;
	    } 
	    else 
	    {
		dayoff = TMtz_oadd(dayoff, (i4) -1);
		if (--wday < 0)
		    wday = LDAYSPERWEEK;
		--i;
	    }
	if (i < 0 || i >= len_months[isleap(y)][m]) 
	{
	    TMtz_error("no day in month matches rule");
	    PCexit(FAIL);
	}
    }
    t = (i4) dayoff * SECSPERDAY;
    /*
    ** Cheap overflow check.
    */
    if (t / SECSPERDAY != dayoff) {
	if (wantedy == rp->r_hiyear)
	    return max_time;
	if (wantedy == rp->r_loyear)
	    return min_time;
	TMtz_error("time overflow");
	PCexit(FAIL);
    }
    return TMtz_tadd(t, rp->r_tod);
}

static VOID
TMtz_newabbr(
char * 	string)
{
    i4	i;

    i = (i4)STlength(string) + 1;
    if (charcnt + i >= TM_MAX_TZLBL) 
    {
	TMtz_error("too many, or too long, time zone abbreviations");
	PCexit(FAIL);
    }
    (VOID) STcopy(string, &chars[charcnt]);
    charcnt += TMtz_eitol(i);
}


static i4
TMtz_eitol(
i4     i)
{
    i4	l;
    
    l = i;
    if (i < 0 && l >= 0 || i == 0 && l != 0 || i > 0 && l <= 0) 
    {
	(VOID) SIprintf("%s: %d did not sign extend correctly\n",
			appname, i);
	PCexit(FAIL);
    }
	return l;
}

VOID 
TMtz_isscanf(
i4  	count,
char  	*string,
i4  	*val1,
i4  	*val2,
i4  	*val3)
{
    char    *cp=string;

    *val1=0;
    if(count > 1)
    	*val2=0;
    if(count > 2)
    	*val3=0;
    while(*cp != ':' && *cp != '\0')
    {
    	if(CMdigit(cp))
    	    *val1=10 * (*val1) + *cp - '0';
    	CMnext(cp);    	
    }
    if(count > 1 && *cp != '\0')
    {
    	CMnext(cp);
    	while(*cp != ':' && *cp != '\0')
    	{
    	    if(CMdigit(cp))
    	    	*val2=10 * (*val2) + *cp - '0';
    	    CMnext(cp);
    	}
    }    	        	

    if(count > 2 && *cp != '\0')
    {
    	CMnext(cp);
    	while(*cp != ':' && *cp != '\0')
    	{
    	    if(CMdigit(cp))
    	    	*val3=10 * (*val3) + *cp - '0';
    	    CMnext(cp);
    	}
    }    	        	
}


#define nonzero(n)	(((n) == 0) ? 1 : (n))

char *
TMtz_imalloc(
i4	n)
{
    STATUS  status;
    PTR     p;

    p = MEreqmem( 0, nonzero(n), FALSE, &status); 

    if (status != OK)
    {
	SIprintf("%s: Error allocating memory in TMtz_imalloc\n", appname);
	PCexit(FAIL);
    }

    return (char *) p;

}

char *
TMtz_irealloc(
char *old,
i4  size)
{
    PTR		new;
    SIZE_TYPE	cpsize;
    STATUS	status;

    new = TMtz_imalloc(size);

    if(old != NULL)
    {
	status = MEsize(old, &cpsize);
	if (status != OK)
	{
	    SIprintf("%s: Error allocating memory in TMtz_irealloc\n", appname);
	    PCexit(FAIL);
	}
	MEcopy(old, cpsize, new);
	TMtz_ifree(old);
    }
    return (char *) new;
}

char *
TMtz_icatalloc(
char * old,
char * new)
{
    char *	result;
    i4          oldsize, newsize;

    newsize = (new == NULL) ? 0 : (i4)STlength(new);
    if (old == NULL)
	oldsize = 0;
    else if (newsize == 0)
		return old;
	else	oldsize = (i4)STlength(old);
	if ((result = TMtz_irealloc(old, oldsize + newsize + 1)) != NULL)
		if (new != NULL)
			(void) strcpy(result + oldsize, new);
	return result;
}

char *
TMtz_icpyalloc(
char * string)
{
	return TMtz_icatalloc((char *) NULL, string);
}

VOID
TMtz_ifree(
char *p)
{
    STATUS status;
    
    status = MEfree((PTR)p);

    if (status != OK)
    {
	SIprintf("%s: Error releasing memory in TMtz_ifree\n", appname);
	PCexit(FAIL);
    }
}

static VOID
TMtz_qsort(
TMTZ_RULE  *rules,
i4         left,
i4         right)
{
    i4  i, last;
    
    if( left >= right)
        return;
    TMtz_swap(rules, left, (left + right)/2);
    last = left;
    for(i = left+1; i <= right; i++)
	if ( STcompare( (rules+i)->r_name, (rules+left)->r_name) < 0)
	    TMtz_swap(rules, ++last, i);
    TMtz_swap(rules, left, last);
    TMtz_qsort(rules, left, last-1);
    TMtz_qsort(rules, last+1, right);
}

static VOID
TMtz_swap(
TMTZ_RULE  *rules,
i4         i,
i4         j)
{
    TMTZ_RULE temp;
    
    STRUCT_ASSIGN_MACRO(*(rules+i), temp);
    STRUCT_ASSIGN_MACRO(*(rules+j), *(rules+i));
    STRUCT_ASSIGN_MACRO(temp, *(rules+j));
}    

    


static STATUS
TMtz_igetopt(
i4	nargc,
char	**nargv, 
char    *ostr,
i4      *ioptind,
char    *ioptarg,
char    *ioptopt)    
{
    char	*place="";      /* option letter processing */
    char	*oli;		/* option letter list index */

    *ioptopt = ' ';
    if (*ioptind >= nargc || *(place = nargv[*ioptind]) != '-' ||
	!*++place)
	return(OK);
    if (*place == '-') 
    {		        /* found "--" */
	*ioptind += 1;
	return(OK);
    }
    if ((*ioptopt = *place++) == ':' ||
	((oli = STchr(ostr, *ioptopt)) == NULL)) 
    {
	if (!*place)
	    *ioptind += 1;
	SIprintf("%s: illegal option -- \n", appname);
	return(FAIL);
    }
    if (*++oli != ':') 
    {			                /* don't need argument */
	ioptarg = NULL;
	if (!*place)
	    *ioptind += 1;
    }
    else 
    {					/* need an argument */
	if (*place)			
	{
	    while(*place != '=' && *place != ' ')
	    {
		if( *place == EOS)
		{
		    if (nargc > (*ioptind += 1)) 
			STcopy(nargv[*ioptind], ioptarg);
		    else
		    {
			*ioptopt = ' ';
			ioptarg = "";
		    }
		    break;
		}
		place++;
	    }
	    STcopy(place+1, ioptarg);
	}
	else 
	    if (nargc <= (*ioptind += 1)) 
	    {	                        /* no arg */
		place = "";
		ioptarg = "";
		*ioptopt = ' ';
	    }
	    else				/* white space */
		STcopy(nargv[*ioptind], ioptarg);
	place = "";
	*ioptind += 1;
    }
    return(OK);				/* dump back option letter */
}
