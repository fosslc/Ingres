/*
** Copyright (c) 2004 Ingres Corporation
*/
 
#include <compat.h>
#include <gl.h>
#include <cm.h>
#include <er.h>
#include <si.h>
#include <cv.h>
#include <pc.h>
#include <lo.h>
#include <st.h>
#include <me.h>
#include <sl.h>
#include <nm.h>
#include <pc.h>
#include <iicommon.h>
#include <adf.h>
#include <adulcol.h>
#include <aduucol.h>
 
/**
**
**  Name: ADUCOMPILE.C - Local collation languge compiler
**
**  Description:
**	Take a human readable language collation description file
**	and compile it into a data structure for use by collation routines.
**
**		main() - Main program
**		maketable() - Actual compiler
**		dumptbl() - dump the compiled result to a file
**
**
**  History:
**      03-may-89 (anton)
**          Created.
**	17-Jun-89 (anton)
**	    Moved to ADU from CL
**	03-aug-89 (dds)
**	    Added ming hint so aducompile goes to utility dir.
**	21-Dec-89 (anton)
**	    Usage message wrong - remove magic number - bug 9193
**      31-dec-1992 (stevet)
**          Added function prototypes.
**	21-jun-93 (geri)
**	    Added some bugfixes which were in the the 6.4 version:
**	    31996: PCexit with FAIL if error encountered;
**	    31892: better syntax checking and sane delimiter
**	    handling; 31993: multiple character pattern match
**	    rules were not handled correctly; added CMSET_attr
**	    call to initialize CM attribute table; changed read-in
**	    character set to use II_CHARSET symbol.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-mar-2001 (stephenb)
**	    Add support for unicode collation tables, denoted by a uce
**	    extennsion
**	24-dec-2001 (somsa01)
**	    In makeutable(), modified such that the varsize which is placed
**	    in the Unicode table is now a SIZE_TYPE rather than an i4.
**	12-dec-2001 (devjo01)
**	    Make sure 1st overflow entry of unicode collation table is 
**	    properly aligned for Tru64.
**	    Add messages to report if files cannot be opened.
**	05-sep-2002 (hanch04)
**          For the 32/64 bit build,
**	    Add lp64 directory for 64-bit add on.
**          Call PCexeclp64 to execute the 64 bit version
**          if II_LP64_ENABLED is true
**	11-sep-2002 (somsa01)
**	    Prior change is not for Windows.
**      25-Sep-2002 (hanch04)
**          PCexeclp64 needs to only be called from the 32-bit version of
**          the exe only in a 32/64 bit build environment.
**	03-Feb-2004 (gupsh01)
**	    Keep track of memory required to allocate the recomp_tbl for 
**	    unicode collation table. 
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**      04-Mar-2005 (hanje04)
**	    SIR 114034
**          Add support for reverse hybrid builds, i.e. 64bit exe needs to
**          call 32bit version.
**	06-Oct-2005 (hanje04)
**	    Remove printf() which was probably put in for debug at with my
**	    previous change.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	18-Mar-2010 (hanje04)
**	    SIR 123296
**	    Move location for collation files from FILES->ADMIN so that
**	    they will be in a writeable location for LSB builds.
*/

/*
**  Forward and/or External function references.
*/
 

/*
**  Definition of static variables and forward static functions.
*/
 
static	ADULTABLE	*maketable(char  *desfile);
static	STATUS		dumptbl(ADULTABLE  	*tbl,
				char       	*lang);
static	ADUUCETAB	*makeutable(char *desfile);
static	STATUS		dumputbl(ADUUCETAB	*tbl,
				 char		*colname);

 
static union
{
    ADULTABLE	tab;
    ADUUCETAB	utab;
    char	buff[INSTRUCTION_SIZE + ENTRY_SIZE];
} w;

#define		MAX_UVERS	32 /* max length of table version */

 
/*
PROGRAM =	aducompile

DEST  =		utility
 
NEEDLIBS =	ADFLIB COMPATLIB 
*/
 
/*{
** Name: main()	- collation compiler
**
** Description:
**      Top level of collation compiler.
**
** Inputs:
**	argc			argument count
**	argv			argument vector
**
** Outputs:
**	none
**
** History:
**      03-may-89 (anton)
**          Created.
**	17-Jun-89 (anton)
**	    Moved to ADU from CL
**	21-Dec-89 (anton)
**	    Fixed usage message - removed magic number - bug 9193
**    	01-jun-92 (andys)
**          Correct spelling of 'language'. [bug 44495]
**	21-jun-93 (geri)
**	    PCexit with FAIL if error encountered; added CMset_attr call
**	    to intialize CM attribute table; chaged read-in char set
**	    to use II_CHARSET symbol. These were in the 6.4 version.
**	14-mar-2001 (stephenb)
**	    Add optional unicode indicator.
**	06-sep-2002 (hanch04)
**	    The 32 and 64 bit version need to be run so call PCspawnlp64.
**	14-Jun-2004 (schka24)
**	    Use (safe) canned charmap setting routine.
**	04-Mar-2005 (hanje04)
**	    SIR 114034
**	    Add support for reverse hybrid builds, i.e. 64bit exe needs to
** 	    call 32bit version.
**	09-Mar-2007 (gupsh01)
**	    Add support for upper/lower case operations.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/
main(
int	argc,
char	*argv[])
{
    ADULTABLE	*tbl;
    ADUUCETAB	*utbl;
    STATUS   	stat;
    CL_ERR_DESC cl_err;
    bool	unicode = FALSE;
#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH32)
    char        *lp64enabled;
#endif

    _VOID_ MEadvise(ME_INGRES_ALLOC);

#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH32)

    /*
    ** Try to exec the 64-bit version
    */
    NMgtAt("II_LP64_ENABLED", &lp64enabled);
    if ( (lp64enabled && *lp64enabled) &&
       ( !(STbcompare(lp64enabled, 0, "ON", 0, TRUE)) ||
         !(STbcompare(lp64enabled, 0, "TRUE", 0, TRUE))))
    {
        PCspawnlp64(argc, argv);
    }
#endif  /* hybrid */
#if defined(conf_BUILD_ARCH_64_32) && defined(BUILD_ARCH64)
    {
	char        *lp32enabled;
	/*
	** Try to exec the 32-bit version
	*/
	NMgtAt("II_LP32_ENABLED", &lp32enabled);
	if ( (lp32enabled && *lp32enabled) &&
	( !(STbcompare(lp32enabled, 0, "ON", 0, TRUE)) ||
	!(STbcompare(lp32enabled, 0, "TRUE", 0, TRUE))))
	    PCspawnlp32(argc, argv);
    }
#endif  /* reverse hybrid */

    /* Set CM character set stuff */

    stat = CMset_charset(&cl_err);
    if (stat != OK)
    {
	SIprintf("Error while processing character set attribute file.\n");
	PCexit(FAIL);
    }

    _VOID_ SIeqinit();

    if (argc != 3 && argc != 4)
    {
        SIprintf("Usage: aducompile description-file language-file [-u]\n");
        PCexit(FAIL);
    }

    if (argc == 4)
    {
	/* must be -u flag for unicode */
	if (STbcompare(argv[3], 0, "-u", 0, TRUE))
	{
	    SIprintf("Usage: aducompile description-file language-file [-u]\n");
	    PCexit(FAIL);
	}
	unicode = TRUE;
    }

    if (unicode)
    {
	utbl = makeutable(argv[1]);
	if (!utbl)
	{
	    SIprintf("aducompile: description-file syntax error\n");
	    PCexit(FAIL);
	}

	stat = dumputbl(utbl, argv[2]);
	if (stat)
	    SIprintf("aducompile: dumptbl %s failed with %x\n", argv[2], stat);
    }
    else
    {
	tbl = maketable(argv[1]);
	if (!tbl)
	{
	    SIprintf("aducompile: description-file syntax error\n");
	    PCexit(FAIL);
	}

	stat = dumptbl(tbl, argv[2]);
	if (stat)
	    SIprintf("aducompile: dumptbl %s failed with %x\n", argv[2], stat);
    }
    PCexit(stat ? FAIL : OK);
}

/*{
** Name: dumptbl()	- dump table to a file
**
** Description:
**	Dump a compiled collation table to a disk file for later
**	Invocation of aducolinit.
**
** Inputs:
**	tbl			point to collation table
**	lang			language name (really file name)
**
** Outputs:
**	none
**
**	Returns:
**		OK
**		!OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-may-89 (anton)
**          Created.
**	17-Jun-89 (anton)
**	    Moved to ADU from CL
**	14-Jan-2004 (gupsh01)
**	    Included output location in CMdump_col.
*/
 
static STATUS
dumptbl(
ADULTABLE	*tbl,
char		*lang)
{
    CL_ERR_DESC	syserr;
    i4		tlen;
 
    tlen = sizeof(*tbl) + tbl->nstate * sizeof(tbl->statetab);
    tlen = (tlen + COL_BLOCK) & ~(COL_BLOCK-1);
    return CMdump_col(lang, (PTR)tbl, tlen, &syserr, CM_COLLATION_LOC);
}

/*{
** Name: maketable - compile a collation description
**
** Description:
**	Read a collation description file and create a collation table.
**	The table is a modified TRIE structure with a complete map
**	at the top level and sparse maps below.
**
** Inputs:
**	desfile			collation description filename
**
** Outputs:
**	none
**
**	Returns:
**	    pointer to collation table on success
**	    NULL	on failure
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-may-89 (anton)
**          Created.
**	17-Jun-89 (anton)
**	    Moved to ADU from CL
**	17-Jul-89 (anton)
**	    replace 16 with COL_MULTI and use adulstrinit not aducolinit
**	21-jun-93 (geri)
**	    Added 2 bugfixes which were in the the 6.4 version:
**	    31892: better syntax checking and sane delimiter
**	    handling; 31993: multiple character pattern match
**	    rules were not handled correctly.
**	1-oct-04 (toumi01)
**	    On file-not-found error, make that clear rather than just issuing
**	    a bogus and misleading syntax error message.
**	1-May-2008 (kibro01) b120307
**	    When we find a "+", just check the next character isn't also a "+",
**	    since it ought to be possible to set the collation relative to "+"
**	    by having a line "++1:<".
*/
static ADULTABLE *
maketable(
char	*desfile)
{
    FILE      	*fp;
    i4        	n;
    char     	buf[MAX_LOC];
    u_char     	*s;
    register i4  lno = 0;
    register i4  errcnt = 0;
    i2        	*sp, v, t, tt;
    struct	ADUL_tstate *ts;
    LOCATION   	loc;
    ADULcstate	cs;
 
    STcopy(desfile, buf);
    LOfroms(FILENAME, buf, &loc);
    if (SIopen(&loc, "r", &fp) != OK)
    {
	SIprintf("aducompile: Unable to open input file \"%s\".\n",
		 loc.string );
        return NULL;    
    }

    w.tab.magic = ADUL_MAGIC;
    w.tab.nstate = 0;
    for (n = 0; n < 256; n++)
        w.tab.firstchar[n] = n * COL_MULTI;
 
    while (SIgetrec(buf, sizeof buf, fp) != ENDFILE)
    {
        lno++;
        if (*buf == ':')
        {
            /* line is a comment */
            continue;
        }
        adulstrinit(&w.tab, (u_char*)buf, &cs);
        v = 0;
        sp = NULL;
        while ((t = *(s = adulptr(&cs))) != ':')
        {
	    /* If this is '+', just confirm that the next character isn't,
	    ** since the construction "++1:<" would be fine, setting < as the
	    ** character 1 place beyond + (kibro01) b120307
	    */
            if (t == '+' && (*(s+1) != '+'))
            {
                if (*++s == '*')
                {
                    v = ADUL_SKVAL;
                    while (*++s != ':' && *s != '\n')
                        continue;
                    if (*s == '\n')
                    {
                        /* syntax error: report line and line number  */
                        SIprintf("Syntax error on line %d: %s", lno, buf);
                        errcnt++;
                        s = NULL;
                    }
                    break;
                }
                tt = 0;
                while ((t = *s - '0') <= 9 && t >= 0)
                {
                    tt = tt * 10 + t;
                    s++;
                }
                adulstrinit(&w.tab, s, &cs);
                if (sp == NULL)
                    sp = &v;
                *sp += tt;
                continue;
            }
            if (t == '\n')
            {
                /* syntax error - line has no ':' */
                SIprintf("Syntax error on line %d: %s", lno, buf);
                errcnt++;
                s = NULL;
                break;
            }
            t = adultrans(&cs);
            adulnext(&cs);
            if (sp == NULL)
            {
                v = t;
                sp = &v;
            }
            else
            {
                ts = &w.tab.statetab[w.tab.nstate];
                ts->match = *sp;
                ts->flags = ADUL_LMULT;
                *sp = w.tab.nstate++ | ADUL_TMULT;
                ts->nomatch = t;
                sp = &ts->nomatch;
            }
        }
 
        if (s == NULL)
        {
            /* syntax error occurred */
            continue;	/* to the next line */
        }

        sp = &w.tab.firstchar[255 & *++s];

        ts = NULL;

        if (*s == '\n')
        {
            SIprintf("Syntax error on line %d (no substitution string): %s",
                lno, buf);
            continue;
        }

        while (*++s != '\n')
        {
            while (*sp & ADUL_TMULT)
            {
                ts = &w.tab.statetab[*sp & ADUL_TDATA];
                if (ts->testchr == *s)
                {
                    sp = &ts->match;
                    if (*++s == '\n')
                    {
                        goto done;
                    }
                }
                else
                {
                    sp = &ts->nomatch;
                }
            }
            ts = &w.tab.statetab[w.tab.nstate];
            ts->match = ts->nomatch = *sp;
            *sp = ADUL_TMULT | w.tab.nstate++;
            sp = &ts->match;
            ts->testchr = *s;
            ts->flags = ADUL_FINAL;
        }
    done:
        if (ts)
            ts->flags ^= ADUL_FINAL;
        *sp = v;
    }
 
    if (errcnt == 0)
        return &w.tab;

    return NULL;
}

/*
** Name: makeutable - make unicode collation element table
**
** Descrription:
**	Reads input collation element text file and compiles a table.
**	also reads the Unicode character database and adds attributes for 
**	each character found. Currently we only take note of canonical
**	decomposition for normalization.
**
** Inputs:
**	desfile	- collation element description file
**
** Outputs:
**	None
**
** Returns:
**	utbl - collation element table
**
** History:
**	14-mar-2001 (stephenb)
**	    Created
**	5-apr-2001 (stephenb)
**	    Add code to read unciode character database.
**	17-may-2001 (stephenb)
**	    Read in combining class from character database.
**	24-dec-2001 (somsa01)
**	    Modified such that the varsize which is placed in the Unicode
**	    table is now a SIZE_TYPE rather than an i4.
**	12-dec-2001 (devjo01)
**	    Make sure 1st overflow entry is properly aligned.
**	    Add messages to report if files cannot be opened.
**	17-jul-2001 (gupsh01)
**	    Modified the placing of the decomposition enty to 
**	    decomp structure in the file, to be the first entry. 
**      04-Mar-2005 (hanje04)
**          Add support for reverse hybrid builds, i.e. 64bit exe needs to
**          call 32bit version.
**	17-May-2007 (gupsh01)
**	    Added support for reverse sorting of the accented characters.
**	    This is done to support @backwards tag in custom collation file. 
**	08-Aug-2007 (kiria01) b118917
**	    Detect lines relating to conditional special caseing and ignore them.
**	08-Aug-2007 (gupsh01)
**	    Fixed the typo for CASE_IGNORABLE setting. Also fixed the 
**	    handling of Final_Sigma in the special_casing.txt.
**	08-Aug-2007 (gupsh01)
**	    Fixed the decomposition mapping which were not being set for 
**	    composite characters.
**	24-Nov-2008 (gupsh01)
**	    When alloating the memory for recombination table make sure
**	    we initialize the buffer. 
*/
static	ADUUCETAB *
makeutable(
	   char		*desfile)
{
    char     		buf[MAX_UCOL_REC];
    FILE      		*fp;
    FILE		*dfp;
    LOCATION   		loc;
    char		table_vers[MAX_UVERS] = "";
    i4			lno = 0;
    char		*recptr;
    char		*separator;
    ADUUCETAB		*tab = &w.utab;
    i4			num_rvals = 0;
    ADUUCETAB		*epointer = (ADUUCETAB *)w.buff; /* entries first */
    /* then instructions */    
    char		*ipointer; 
    SIZE_TYPE		*varsize;
    char		*tptr;
    ADU_UCEFILE		*rrec = NULL;
    ADU_UCEFILE		*cerec = NULL;
    char		csval[7];
    char		svalue[21];
    char		*ceptr;
    char		*comment;
    u_i4		cval;
    i4			numce;
    bool		combent;
    i4			i;
    /* Stuff to track the size of the recomb_tbl entries */
    u_i2        	**recomb_def2d;
    u_i2        	*recomb_def1d;
    char        	*recombtbl;
    char        	*tracker;
    SIZE_TYPE		*recomb_tbl_size;
    i4          	tr = 0;
    i4          	j = 0;
    STATUS		stat;
    i4			rcbufsz;
    char		*tempbuf;
    i4			current_bytes;
    char		*upperval;
    char		*lowerval;
    char		*titleval;
    char		*endval;
    i4 			bts = 0;
    i4 			lcnt = 0;
    i4 			tcnt = 0;
    i4 			ucnt = 0;
    i4 			row = 0;
    i4 			col = 0;
    bool		backward_set = FALSE;

    /* open file */
    STcopy(desfile, buf);
    LOfroms(FILENAME, buf, &loc);
    if (SIopen(&loc, "r", &fp) != OK)
    {
	SIprintf("aducompile: Unable to open input file \"%s\".\n",
		 loc.string );
        return NULL;    
    }

    varsize = (SIZE_TYPE *)(w.buff + ENTRY_SIZE);
    recomb_tbl_size = (SIZE_TYPE *)(varsize + 1);
    ipointer = (char*)(recomb_tbl_size + 1);
    ipointer = ME_ALIGN_MACRO(ipointer, sizeof(PTR));

    /* this is a sparse table, make sure we initialize it */
    MEfill(INSTRUCTION_SIZE + ENTRY_SIZE, 0, w.buff);
    *varsize = 0;

    /* read data */
    while (SIgetrec(buf, sizeof(buf), fp) != ENDFILE)
    {
	lno++;
	if (buf[0] == '#')
	    /* comment line */
	    continue;
	(VOID)STtrmwhite(buf);
	if (STcompare(buf, "") == 0)
	    /* blank line */
	    continue;
	/* should first find version */
	if (table_vers[0] == EOS)
	{
	    if (STbcompare(buf, 8, "@version", 8, TRUE) == 0)
	    {
		/* we don't parse the version string, maybe we should */
		STlcopy(buf+9, table_vers, MAX_UVERS);
		continue;
	    }
	    else
	    {
		SIprintf("Syntax error on line %d, version line must be first \
non-comment line in the file, ignored\n", lno);
		continue;
	    }
	}
	/* then alternate weights (optional), currently un-supported */
	if (STbcompare(buf, 10, "@alternate", 10, TRUE) == 0)
	{
	    SIprintf("Syntax error on line %d, alternate weights are not \
currently supported, ignored\n", lno);
	    continue;
	}

	/* now backwards lines (also not currently supported) */
	if (STbcompare(buf, 10, "@backwards", 10, TRUE) == 0)
	{ 
    	    backward_set = TRUE;
	    continue;
	}

	/* and rearrange lines */
	if (STbcompare(buf, 10, "@rearrange", 10, TRUE) == 0)
	{
	    bool	strend = FALSE;
	    u_i4	rval;

	    for (recptr = buf + 10;;)
	    {
		/* skip blanks */
		if ((recptr = STskipblank(recptr, STlength(recptr))) == NULL)
		{
		    /* blank string, ignore */
		    SIprintf("Syntax error on line %d, no characters in \
rearrange list, ignoring\n", lno);
		    strend = TRUE;
		    break;
		}
		/* find next comma separator */
		if ((separator = STindex(recptr, ",", 0)) == NULL)
		{
		    strend = TRUE;
		    separator = recptr + 4;
		}
		if (separator - recptr != 4)
		{
		    SIprintf("Syntax error on line %d, characters in a rearrange\
line must be a comma separated list of 4 digit hex values, ABORTING\n", lno);
		    tab = NULL;
		    break;
		}
		STlcopy(recptr, csval, 4);
		if (CVahxl(csval, &rval) != OK)
		{
		    SIprintf("Syntax error on line %d, characters in a rearrange\
line must be a comma separated list of 4 digit hex values, ABORTING\n", lno);
		    tab = NULL;
		    break;
		}
		epointer[rval].flags |= CE_REARRANGE;
		if (strend)
		    break;
		recptr = separator + 1;
	    }
	    if (tab == NULL)
		break;
	    continue;
	}

	/* finally, the entries themselves */
	
	/* first find the semicolon separator */
	if ((separator = STindex(buf, ";", 0)) == NULL)
	{
	    SIprintf("Syntax error on line %d, character values and collation \
elements must be separated with a semicolon, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}
	if (separator - buf > 21)
	{
	    SIprintf("Syntax error on line %d, combining characters can contain \
a maximum of 4 values, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}
	combent = FALSE;
	STlcopy(buf, svalue, separator - buf);
	if (STtrmwhite(svalue) > 4)
	{
	    /* we have combining character sequence */
	    char	*cptr = svalue;
	    char	*sptr;
	    int		num_vals;
	    bool	strend = FALSE;

	    combent = TRUE;
	    for (num_vals = 0;;)
	    {
		/* find space */
		if ((sptr = STindex(cptr, " ", 0)) == NULL)
		{
		    strend = TRUE;
		    sptr = svalue + STlength(svalue);
		}
		if (sptr - cptr > 4)
		{
		    SIprintf("Syntax error on line %d, combining character \
sequences must be sets of 4 digit hex values, ABORTING\n", lno);
		    tab = NULL;
		    break;
		}
		(VOID)STlcopy(cptr, csval, 4);
		if (CVahxl(csval, &cval) != OK)
		{
		    SIprintf("Syntax error on line %d, combining character \
sequences must be sets of 4 digit hex values, ABORTING\n", lno);
		    tab = NULL;
		    break;
		}
		if (num_vals == 0)
		{
		    /* 
		    ** first value, mark as combining in table and create
		    ** a new combing element record in the instruction area
		    */
		    epointer[cval].flags |= CE_COMBINING;
		    rrec = (ADU_UCEFILE *)ipointer;
		    rrec->type = CE_COMBENT;
		}
		rrec->entry.adu_combent.values[num_vals++] = (u_i2)cval;
		if (strend)
		    break;
		cptr = sptr + 1;
	    }
	    if (tab == NULL)
		break;
	    /* end of combining character list */
	    rrec->entry.adu_combent.num_values = num_vals;
	    tptr = ipointer + sizeof(ADU_UCEFILE) + (num_vals * sizeof(u_i2));
	    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
	    *varsize = *varsize + (tptr - ipointer);
	    ipointer = tptr;
	    cerec = rrec;
	}
	else
	{
	    (VOID)STlcopy(svalue, csval, 4);
	    if (CVahxl(csval, &cval) != OK)
	    {
		SIprintf("Syntax error on line %d, character values must be \
4 digit hex numbers, ABORTING\n", lno);
		tab = NULL;
		break;
	    }
	}
	/* now parse the CE values */
	/* find comment */
	comment = STindex(buf, "#", 0);
	for (ceptr = separator + 1, numce = 0;;numce++)
	{
	    char	*cbrace;
	    i4		level;

	    /* find the open brace */
	    if ((separator = STindex(ceptr, "[", 0)) == NULL || 
		(comment && separator > comment))
		break;
	    /* find close brace */
	    if ((cbrace = STindex(ceptr, "]", 0)) == NULL)
	    {
		SIprintf("Syntax error on line %d, There must be at least one\
collation element list, ABORTING\n", lno);
		tab = NULL;
		break;
	    }
	    /* 
	    ** strict format here. there should be a period separated list
	    ** of 4 digit hex values. There are a maximum of MAX_CE_LEVELS of
	    ** these values
	    */
	    for (level = 0; level < MAX_CE_LEVELS; level++)
	    {
		bool	endce = FALSE;
		u_i4	ceval;

		if (separator + 4*level + level + 1 == cbrace)
		    endce = TRUE;
		else if (level > 0 && separator[4*level + level + 1] != '.')
		{
		    SIprintf("Syntax error on line %d, collation elements must be \
a period separated list of 4 digit hex values, ABORTING\n", lno);
		    tab = NULL;
		    break;
		}
		STlcopy(&separator[4*level + level + 2], csval, 4);
		if (CVahxl(csval, &ceval) != OK)
		{
		    SIprintf("Syntax error on line %d, collation elements must be \
a period separated list of 4 digit hex values, ABORTING\n", lno);
		    tab = NULL;
		    break;
		}
		if (numce >= 1)
		{
		    /* more than one collation element list, add to overflow */
		    if (numce == 1 && level == 0)
		    {
			epointer[cval].flags |= CE_HASOVERFLOW;
			rrec = (ADU_UCEFILE *)ipointer;
			rrec->type = CE_OVERFLOW;
			rrec->entry.adu_ceoverflow.cval = (u_i2)cval;
		    }
		    rrec->entry.adu_ceoverflow.oce[numce-1].ce[level] = 
			(u_i2)ceval;
		}
		else if (combent)
		    cerec->entry.adu_combent.ce[level] = (u_i2)ceval;
		else
		    epointer[cval].ce[level] = (u_i2)ceval;
		if (endce)
		{
		    level++;
		    break;
		}
	    }
	    if (tab == NULL)
		break;
	    /* there should be at least 3 levels */
	    if (level < 3)
	    {
		SIprintf("Syntax error on line %d, there must be at least 3 \
levels in the CE list, ABORTING\n", lno);
		tab = NULL;
		break;
	    }
	    else if (level == 3)
	    {
		if (combent)
		    cerec->entry.adu_combent.ce[3] = 0;
		else
		    epointer[cval].ce[3] = 0;
	    }
	    /* note that we have a non-synthesized CE list */
	    if (!combent)
		epointer[cval].flags |= CE_HASMATCH;
	    /* 
	    ** check for alternate collation indicator (we do nothing with it
	    ** right now, but we'll make a note of it
	    */
	    if (separator[1] == '*')
	    {
		if (combent)
		    cerec->entry.adu_combent.flags |= CE_VARIABLE;
		else if (numce)
		    rrec->entry.adu_ceoverflow.oce[numce-1].variable = TRUE;
		else
		    epointer[cval].flags |= CE_VARIABLE;
	    }
	    else if (separator[1] != '.')
	    {
		SIprintf("Syntax error on line %d, collation elements must be \
a period separated list of 4 digit hex values, ABORTING\n", lno);
		tab = NULL;
		break;
	    }
	    ceptr = cbrace + 1;
	}
	if (numce == 0)
	{
	    SIprintf("Syntax error on line %d, There must be at least one\
collation element list, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}
	else if (numce >= 2)
	{
	    /* there is one in the base table, rest are overflow */
	    rrec->entry.adu_ceoverflow.num_values = numce - 1;
	    tptr = ipointer + sizeof(ADU_UCEFILE) + 
		((numce - 1) * sizeof(rrec->entry.adu_ceoverflow.oce) 
		 * MAX_CE_LEVELS);
	    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
	    *varsize = *varsize + (tptr - ipointer);
	    ipointer = tptr;
	}
    }

    /* open character database */
    NMloc(ADMIN, PATH, (char *)NULL, &loc);
    LOfaddpath(&loc, "collation", &loc);
#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
    LOfaddpath(&loc, "lp64", &loc);
#elif defined(conf_BUILD_ARCH_64_32) && defined(BUILD_ARCH32)
    LOfaddpath(&loc, "lp32", &loc);
#endif /* hybrid */
    LOfstfile("udata.ucd", &loc);
    if (SIopen(&loc, "r", &dfp) != OK)
    {
	SIprintf("aducompile: Unable to open character database \"%s\".\n", loc.string );
        return NULL;
    }

    /* read database and update table */
    lno = 0;
    while (SIgetrec(buf, sizeof(buf), dfp) != ENDFILE)
    {
	char		*efptr;
	bool		last_entry;
	u_i4		dval;
	char		*space;
	i4		tmp = INSTRUCTION_SIZE;
	char  		GC[3] = {0};

	lno++;
	if (buf[0] == '#')
	    /* comment line */
	    continue;
	(VOID)STtrmwhite(buf);
	if (STcompare(buf, "") == 0)
	    /* blank line */
	    continue;

	/* 
	** file format should be fairly strict, there are a number of
	** fields separated by semi-colons. The first field always
	** contains the code point value. Although the spec states that 
	** ranges are valid, the master database only contains single values
	** (since it describes the decomposition of the character), for
	** our purposes, we will assume a single code point value only
	*/

	/* find first separator */
 	if ((separator = STindex(buf, ";", 0)) == NULL)
	{
	    SIprintf("Syntax error on character database line %d, character \
values must be semi-colon separated, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}
	if (separator - buf < 4 || separator - buf > 6)
	{
	    SIprintf("Syntax error on character database line %d, only single \
character entries alowed in main database, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}
	else if (separator - buf > 4)
	{
	    /*
	    ** It would appear the 3.1.0 version of the database has some
	    ** UCS4 characters in it (4 bytes == more than 4 hex digits).
	    ** Since we currently only support UCS2, we'll silently ignore 
	    ** these and continue to the next record
	    */
	    continue;
	}
	STlcopy(buf, csval, 4);
	if (CVahxl(csval, &cval) != OK)
	{
	    SIprintf("Syntax error on character database line %d, characters \
must appear in the first field as hex values, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}

	/* Set the flags for CASEIGNORABLE */
	if ((cval == 0x0027) || (cval == 0x00AD) || (cval == 0x2019))
	   epointer[cval].flags |= CE_CASEIGNORABLE;
	
	/* Set the flag for reverse collation */
        if (backward_set) 
	  epointer[cval].flags |= CE_COLLATEFRENCH;

	separator++;

	/* Parse general Category of the character (4th field) */
	for (i = 1; i < 2; i++)
	{
	    if ((separator = STindex(separator, ";", 0)) == NULL)
	    {
		SIprintf("Syntax error on character database line %d, There \
must be at least 6 semi-colon separated fields on each line, ABORTING\n", lno);
		tab = NULL;
		break;
	    }
	    separator++;
	}
	if ((efptr = STindex(separator, ";", 0)) == NULL)
	{
	    SIprintf("Syntax error on character database line %d, There \
must be at least 6 semi-colon separated fields on each line, ABORTING\n", lno);
	    tab = NULL;
	    break;
	} 
	STlcopy(separator, GC, ((efptr - separator <= 2) ? efptr - separator : 2));
	separator++;

	if ((STbcompare(GC, 2, "Mn",2, 0) == 0) ||
	      (STbcompare(GC, 2, "Me",2, 0) == 0) ||
	      (STbcompare(GC, 2, "Cf",2, 0) == 0) ||
	      (STbcompare(GC, 2, "Lm",2, 0) == 0) ||
	      (STbcompare(GC, 2, "Sk",2, 0) == 0))
	    epointer[cval].flags |= CE_CASEIGNORABLE;

	/* find combining class (4th field) */
	if ((separator = STindex(separator, ";", 0)) == NULL)
	{
	    SIprintf("Syntax error on character database line %d, There \
must be at least 6 semi-colon separated fields on each line, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}
	separator++;
	/* we're now one character past the 3rd separator, on field 4 */
	if ((efptr = STindex(separator, ";", 0)) == NULL)
	{
	    SIprintf("Syntax error on character database line %d, There \
must be at least 6 semi-colon separated fields on each line, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}
	STlcopy(separator, csval, 
		((efptr - separator <= 4) ? efptr - separator : 4));
	if (CVal(csval, (i4 *)&dval) != OK)
	{
	    SIprintf("Syntax error on character database line %d, Combining \
classes should be a single decimal value, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}
	epointer[cval].comb_class = dval;
	/* now find decomposition field (6th field) */
	for (i = 3; i < 5; i++)
	{
	    if ((separator = STindex(separator, ";", 0)) == NULL)
	    {
		SIprintf("Syntax error on character database line %d, There \
must be at least 6 semi-colon separated fields on each line, ABORTING\n", lno);
		tab = NULL;
		break;
	    }
	    separator++;
	}
	/* we're now one character past the 5th separator, on field 6 */
	if ((efptr = STindex(separator, ";", 0)) == NULL)
	{
	    SIprintf("Syntax error on character database line %d, There \
must be at least 6 semi-colon separated fields on each line, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}
	/* 
	/* check for emty (no) decomposition as well as
	** for compatibility decompositions, we're not currently
	** interested in these. They are indicated by a tag at the start
	** of the field that has the form <tag name>
	*/
	if ((separator != efptr) && (*separator != '<'))
	{
	/* 
	** if we got here, we have a non-empty canonical decomposition, 
	** store it in the table
	*/
	rrec = (ADU_UCEFILE *)ipointer;
	rrec->type = CE_CDECOMP;
	rrec->entry.adu_decomp.cval = (u_i2)cval;
	last_entry = FALSE;
	for (num_rvals = 1;;num_rvals++)
	{
	    if ((space = STindex(separator, " ", 0)) == NULL || space > efptr)
	    {
		last_entry = TRUE;
		space = efptr;
	    }
	    /* should have a 4 digit hex value */
	    if (space - separator != 4)
	    {
		SIprintf("Syntax error on character database line %d, canonical \
decompositions should be space separated 4 digit hex values, ABORTING\n", lno);
		tab = NULL;
		break;
	    }		
	    STlcopy(separator, csval, 4);
	    if (CVahxl(csval, &dval) != OK)
	    {
		SIprintf("Syntax error on character database line %d, canonical \
decompositions should be space separated 4 digit hex values, ABORTING\n", lno);
		tab = NULL;
		break;
	    }
	    rrec->entry.adu_decomp.dvalues[num_rvals - 1] = (u_i2)dval;
	    if (last_entry)
		break;
	    separator = space + 1;
	}
	if (tab == NULL)
	    break;
	rrec->entry.adu_decomp.num_values = num_rvals;
	epointer[cval].flags |= CE_HASCDECOMP;
  	epointer[cval].decomp = &rrec->entry.adu_decomp;
	/* re-set pointer */
	tptr = ipointer + sizeof(ADU_UCEFILE) + (num_rvals * sizeof(u_i2));
	tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
	*varsize = *varsize + (tptr - ipointer);
	ipointer = tptr;
	}

/* LOWER-UPPER SUPPORT
** After 12th ';' character there is a upper case unit
** After 13th ';' character there is a lower case unit
** After 14th ';' character there is a title case unit
** An empty unit means the code point itself is that unit 
** 0041;LATIN CAPITAL LETTER A;Lu;0;L;;;;;N;;;;0061;
** 0061;LATIN SMALL LETTER A;Ll;0;L;;;;;N;;;0041;;0041 
*/
	for (i = 5; i < 11; i++)
        {
	    if ((separator = STindex(separator, ";", 0)) == NULL)
	    {
		SIprintf("Syntax error on character database line %d, There \
must be at least 12 semi-colon separated fields on each line, ABORTING\n", lno);
		tab = NULL;
		break;
	    }
	    separator++;
	}
	while (*separator != ';')
	  separator++;
        separator++;
	if (separator && *separator != '\0')
	{
	    /* Must initialize to hold default values */
	    char ptrs[3][8] = {{0},{0}}; /* UPPER, LOWER, TITLE */
	    i4 cnt[3] = {0}; 		 /* UPPER, LOWER, TITLE */
	    i4 index = 0;
	    i4 chcnt = 0;     /* the index of characters */
	    i4 counter = 0;   /* counts # of characters */
	    bool caseset = FALSE;

            while ( (separator) && (index < 3) && (chcnt < 8) && (*separator != '\0'))
	    {
	      if (*separator == ';')
	      {
		  index++;
	          counter = 0;
	          chcnt = 0;
	      }
	      else
	      { 
		ptrs[index][chcnt++] = *separator;
		cnt[index] = ++counter;
	      } 
	      separator++;
	    }

	    for (index = 0; index < 3; index++)
	    {
	      if (cnt[index])
	      {
	          if (caseset == FALSE)
		  {
		    /* initialize a ADU_CASEMAP for this code point */
		    caseset = TRUE;
		    epointer[cval].flags |= CE_HASCASE;
            	    rrec = (ADU_UCEFILE *)ipointer;
            	    rrec->type = CE_CASEMAP;
            	    rrec->entry.adu_casemap.cval = cval;

		    /* minimum 1 code point value is present */
		    rrec->entry.adu_casemap.num_uvals = cnt[0] ? cnt[0]/4 : 1;
		    rrec->entry.adu_casemap.num_lvals = cnt[1] ? cnt[1]/4 : 1;
		    rrec->entry.adu_casemap.num_tvals = cnt[2] ? cnt[2]/4 : 1;

		    /* by default the lower/upper/titlecase is codepoint itself*/
                    *(u_i2 *)(rrec->entry.adu_casemap.csvals) = (u_i2)cval;
                    *((u_i2 *)(rrec->entry.adu_casemap.csvals) + 1) = (u_i2)cval;
                    *((u_i2 *)(rrec->entry.adu_casemap.csvals) + 2) = (u_i2)cval;
		    /* set rrec pointer in tab */
	          }
		  if (cnt[index]/4 > 1) 
		  { 
		    /* Should not reach here: more than 1 character 
		    ** separated by space  needs scan 
		    */
		    SIprintf ("Syntax error on character database line %d, Case mapping for 0x%s has more than 1 character\n", lno, ptrs[index]);
                    tab = NULL;
                    break;
		  }
		  /* set the appropriate field */
	          if (CVahxl(ptrs[index], &dval)!= OK)
                  {
                    SIprintf ("Syntax error on character database line %d, uppercase code point value 0x%s must be 4 digit hex value, ABORTING\n", lno, ptrs[index]);
                    tab = NULL;
                    break;
		  }	
		  switch (index)
		  { 
		    /* Codes in udata.ucd are in order: upper lower title. 
		    ** Storage order is lower title upper as in Specialcasing 
		    */
		    case 0:
                      *((u_i2 *)(rrec->entry.adu_casemap.csvals) + 2) = (u_i2)dval;
		    break;
		    case 1:
                      *((u_i2 *)(rrec->entry.adu_casemap.csvals)) = (u_i2)dval;
		    break;
		    case 2:
                      *((u_i2 *)(rrec->entry.adu_casemap.csvals) + 1) = (u_i2)dval;
		    break;
		    default:
		    break;
		  }
	      }
	    }
/* Reset the table pointers */
	    if (caseset)
	    {
		/* re-set pointer */
  		epointer[cval].casemap = &rrec->entry.adu_casemap;
		num_rvals = rrec->entry.adu_casemap.num_lvals + rrec->entry.adu_casemap.num_tvals + rrec->entry.adu_casemap.num_uvals - 1; 
		tptr = ipointer + sizeof(ADU_UCEFILE) + (num_rvals * sizeof(u_i2));
		tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
		*varsize = *varsize + (tptr - ipointer);
		ipointer = tptr;
	    } 
#ifdef xDebug
	    /* Print the ADU_CASEMAP structure */
	    if (caseset)
	    {
	        SIprintf ("\n ADU_CASEMAP[%04x] { \n \t rrec->entry.adu_casemap.num_lvals = %d \n \t rrec->entry.adu_casemap.num_tvals = %d \n \t rrec->entry.adu_casemap.num_uvals = %d \n \t", 
		cval, 
		rrec->entry.adu_casemap.num_lvals, 
		rrec->entry.adu_casemap.num_tvals, 
		rrec->entry.adu_casemap.num_uvals);
	        for (index = 0; index < 3; index++)
	          SIprintf (" dvalues[%d] = %04x ", index, *(u_i2 *)((rrec->entry.adu_casemap.csvals) + index));
	        SIprintf ("\n}\n");
	    } 
#endif
	} /* reached EOS */	
    }
    /*
    ** open specialcasing.txt file.
    ** <code>; <lower> ; <title> ; <upper> ; (<condition_list> ;)? # <comment>
    */

    NMloc(ADMIN, PATH, (char *)NULL, &loc);
    LOfaddpath(&loc, "collation", &loc);
#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
    LOfaddpath(&loc, "lp64", &loc);
#elif defined(conf_BUILD_ARCH_64_32) && defined(BUILD_ARCH32)
    LOfaddpath(&loc, "lp32", &loc);
#endif /* hybrid */
    LOfstfile("specialcasing.txt", &loc);
    if (SIopen(&loc, "r", &dfp) != OK)
    {
	SIprintf("aducompile: Unable to open specialcasings.txt file \"%s\".\n", loc.string );
        return NULL;
    }

    /* read file and update collation table */
    lno = 0;
    while (SIgetrec(buf, sizeof(buf), dfp) != ENDFILE)
    {
	char		*efptr;
	bool		last_entry;
	u_i4		dval;
	char		*space;
	i4		tmp = INSTRUCTION_SIZE;

	lno++;
	if (buf[0] == '#')
	    /* comment line */
	    continue;

	(VOID)STtrmwhite(buf);
	if (STcompare(buf, "") == 0)
	    /* blank line */
	    continue;

	/* find first separator */
 	if ((separator = STindex(buf, ";", 0)) == NULL)
	{
	    SIprintf("Syntax error on character database line %d, character \
values must be semi-colon separated, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}
	if (separator - buf < 4 || separator - buf > 6)
	{
	    SIprintf("Syntax error on character database line %d, only single \
character entries alowed in main database, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}
	else if (separator - buf > 4)
	{
	    /*
	    ** It would appear the 3.1.0 version of the database has some
	    ** UCS4 characters in it (4 bytes == more than 4 hex digits).
	    ** Since we currently only support UCS2, we'll silently ignore 
	    ** these and continue to the next record
	    */
	    continue;
	}
	STlcopy(buf, csval, 4);
	if (CVahxl(csval, &cval) != OK)
	{
	    SIprintf("Syntax error on character database line %d, characters \
must appear in the first field as hex values, ABORTING\n", lno);
	    tab = NULL;
	    break;
	}
	while (*separator == ' ' || *separator == ';')
	  separator++;

	if (separator && *separator != '\0')
	{
	    /* Must initialize to hold default values */
	    char ptrs[3][7][8] = {{0},{0},{0}}; /* UPPER, LOWER, TITLE */
	    i4 cnt[3] = {0}; 		 /* UPPER, LOWER, TITLE */
	    i4 index = 0;
	    i4 chcnt = 0;     /* the index of characters */
	    i4 unitcnt = 0;     /* the index of characters */
	    i4 counter = 0;   /* counts # of characters */
	    bool caseset = FALSE;

            while ( (separator) && (index < 3) && (chcnt < 9) && (*separator != '\0'))
	    {
	      if (*separator == ';')
	      {
		  index++;
	          counter = 0;
	          chcnt = 0;
		  unitcnt = 0;
		  if (*(separator + 1) == ' ')
	      	   separator++;
	      }
	      else if (*separator == ' ')
	      {
		  if ((*(separator + 1) != ';') || 
			(*(separator + 1) != ' ') || 
			(*(separator + 1) != '#')
		     )
		  {
		     unitcnt++;
	             chcnt = 0;
		  }
		}
	      else if (*separator == '#')
		  continue;
	      else
	      { 
		ptrs[index][unitcnt][chcnt++] = *separator;
		cnt[index] = ++counter;
	      } 
	      separator++;
	    }

	    /*
	    ** At this point we have processed the numeric data relating to
	    ** the UPPER, LOWER, TITLE where relevant and now we must check
	    ** whether this is a Conditional mapping. Currently we do not
	    ** support Conditionals especially locale form so if we find we
	    ** have such a line we quietly ignore it.
	    */
	    while (*separator == ' ')
		separator++;
	    if (*separator && *separator != '#')
	    {

		/*
		** Specialcasing present - handle appropriatly.
		**
		** For now we drop line for Locale-sensitive mappings.
		** Include Final_Sigma.
		*/
		if (STbcompare(separator, 11, "Final_Sigma", 11, TRUE) != 0)
		  continue;
	    }

	    lcnt = cnt[0] ? cnt[0]/4 : 1;
	    tcnt = cnt[1] ? cnt[1]/4 : 1;
	    ucnt = cnt[2] ? cnt[2]/4 : 1;
	    bts = lcnt + tcnt + ucnt;
	    row = 0;
	    col = 0;
	    for (index = 0; index < bts; index++)
	    {
	          if (caseset == FALSE) 
		  {
		    /* initialize a ADU_CASEMAP for this code point */
		    caseset = TRUE;
		    epointer[cval].flags |= CE_HASCASE;
            	    rrec = (ADU_UCEFILE *)ipointer;
            	    rrec->type = CE_CASEMAP;
            	    rrec->entry.adu_casemap.cval = cval;

		    /* minimum 1 code point value is present */
		    rrec->entry.adu_casemap.num_lvals = lcnt;
		    rrec->entry.adu_casemap.num_tvals = tcnt;
		    rrec->entry.adu_casemap.num_uvals = ucnt;
	          }
	          if (CVahxl(ptrs[row][col], &dval)!= OK)
		  {
                      SIprintf ("Syntax error on character database line %d, code point value 0x%s must be 4 digit hex value, ABORTING\n", lno, ptrs[index]);
                      tab = NULL;
                      break;
		  }
                  *((u_i2 *)(rrec->entry.adu_casemap.csvals) + index) = (u_i2)dval;

		  /* Now increment the row or column */
		  if ((index == (lcnt - 1)) || (index == (lcnt + tcnt - 1)) || 
			(index == (bts - 1))) 
		  { 
		    row++;
		    col = 0; 
		  }
		  else 
		    col++;
		    
	    }
	    if (caseset)
	    { 
		/* re-set pointer */
  		epointer[cval].casemap = &rrec->entry.adu_casemap;
		num_rvals = rrec->entry.adu_casemap.num_lvals + rrec->entry.adu_casemap.num_tvals + rrec->entry.adu_casemap.num_uvals - 1; 
		tptr = ipointer + sizeof(ADU_UCEFILE) + (num_rvals * sizeof(u_i2));
		tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
		*varsize = *varsize + (tptr - ipointer);
		ipointer = tptr;
	    }
#ifdef xDebug
	    if (caseset)
	    { 
	        /* Print the ADU_CASEMAP structure */
	        SIprintf ("ADU_CASEMAP[%04x] { \n \t rrec->entry.adu_casemap.num_lvals = %d \n \t rrec->entry.adu_casemap.num_tvals = %d \n \t rrec->entry.adu_casemap.num_uvals = %d \n \t", 
		cval, 
		rrec->entry.adu_casemap.num_lvals, 
		rrec->entry.adu_casemap.num_tvals, 
		rrec->entry.adu_casemap.num_uvals);
	        for (index = 0; index < 
		(rrec->entry.adu_casemap.num_lvals +
		 rrec->entry.adu_casemap.num_tvals + 
		 rrec->entry.adu_casemap.num_uvals); index++)
	          SIprintf (" val[%d] = %04x ", index, *(u_i2 *)((rrec->entry.adu_casemap.csvals) + index));
	        SIprintf ("\n}\n");
	    } 
#endif
	}
    }
    /* Track memory needed for the recomb_tbl stuff */
    *recomb_tbl_size = 0;

    tracker = recombtbl = MEreqmem(0, RECOMBSIZE, TRUE, &stat);
    if (stat)
    {
      SIprintf ("Error cannot allocate memory for recomb table. Aborting \n");
      return (NULL);
    }
    rcbufsz = RECOMBSIZE;

    recomb_def1d = (u_i2 *) recombtbl;
    tracker += sizeof(u_i2) * 256;
    tracker = ME_ALIGN_MACRO(tracker, sizeof(PTR));

    recomb_def2d = (u_i2 **)tracker;
    tracker += sizeof(u_i2 *) * 256;
    tracker = ME_ALIGN_MACRO(tracker, sizeof(PTR));

    /* set the table for recomb_def2d */
    for ( tr=0; tr < 256; tr++)
      (recomb_def2d)[tr] = recomb_def1d;

    for ( j=0; j < ENTRY_SIZE/sizeof(ADUUCETAB); j++)
      epointer[j].recomb_tab = recomb_def2d;

    /* track the memory required for setting the recomb_tbl */  
    for ( j=0; j < ENTRY_SIZE/sizeof(ADUUCETAB); j++)
    {
      i4          k;
      u_i2 	  firstchar;
      u_i2 	  secondchar;

      if (epointer[j].flags & CE_HASCDECOMP)
      {
        firstchar = epointer[j].decomp->dvalues[0]; 
        secondchar = epointer[j].decomp->dvalues[1]; 
        if (epointer[firstchar].recomb_tab == recomb_def2d)
        {
  	  epointer[firstchar].recomb_tab = (u_i2 **) tracker;
          tracker += sizeof(u_i2 *) * 256;
          tracker = ME_ALIGN_MACRO(tracker, sizeof(PTR));
  
  	  for (k =0; k < 256; k++)
  	  {
  	    if (k == ((secondchar & 0xFFFF) >> 8))
  	    {
              epointer[firstchar].recomb_tab[k] = (u_i2 *)tracker;
              tracker += sizeof(u_i2) * 256;
              tracker = ME_ALIGN_MACRO(tracker, sizeof(PTR));
  	     }
             else
               epointer[firstchar].recomb_tab[k] = recomb_def1d;
  	  }
        }
        else if (epointer[firstchar].recomb_tab[(secondchar & 0xFFFF) >> 8] == 
			recomb_def1d)
        {
          epointer[firstchar].recomb_tab[(secondchar & 0xFFFF) >> 8] =
  			(u_i2 *) tracker;
          tracker += sizeof(u_i2) * 256;
          tracker = ME_ALIGN_MACRO(tracker, sizeof(PTR));
        }
        epointer[firstchar].recomb_tab[
        (secondchar & 0xFFFF) >> 8][secondchar & 0xFF] = cval;
      }

      current_bytes = tracker - recombtbl;

      /* Expand the memory pool if we reach threshold */
      if (rcbufsz <= current_bytes + sizeof(u_i2 *) * 256)
      {
	/* obtain new buffer */
	rcbufsz += RECOMBSIZE;
        tempbuf = (char *)MEreqmem(0, rcbufsz, TRUE, &stat);
        if ( stat )
 	{	
          printf ("ERROR:: Cannot allocate memory for recomb table\n");
	  if (recombtbl)
	    MEfree (recombtbl);
	  return (NULL);
	}

	/* copy the original buffer to new buffer */
        MEcopy(recombtbl , current_bytes, tempbuf);
        tracker = tempbuf + current_bytes;
        tracker = ME_ALIGN_MACRO(tracker, sizeof(PTR));

        /* free the old buffer */
        MEfree(recombtbl);

        recombtbl = tempbuf;
      }
    }

    for ( j=0; j < ENTRY_SIZE/sizeof(ADUUCETAB); j++)
      epointer[cval].decomp = 0;

    *recomb_tbl_size = *recomb_tbl_size + (tracker - recombtbl);
    MEfree (recombtbl);

    return (tab);
}

/*{
** Name: dumputbl()	- dump unicode table to a file
**
** Description:
**	Dump a compiled unicode collation table to a disk file for later
**	Invocation of aducolinit.
**
** Inputs:
**	tbl			point to collation table
**	colname			collation name (really file name)
**
** Outputs:
**	none
**
**	Returns:
**		OK
**		!OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-mar-2001 (stephenb)
**	    Created.
**	14-jan-2004 (gupsh01)
**	    Included output location for CMdump_col.
*/
 
static STATUS
dumputbl(
	 ADUUCETAB	*tbl,
	 char		*colname)
{
    CL_ERR_DESC	syserr;
    i4		tlen;
 
    tlen = ENTRY_SIZE + INSTRUCTION_SIZE;
    tlen = (tlen + COL_BLOCK) & ~(COL_BLOCK-1);
    return CMdump_col(colname, (PTR)tbl, tlen, &syserr, CM_COLLATION_LOC);
}
