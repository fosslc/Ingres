/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<dbms.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<lo.h>
# include	<st.h>
# include	<nm.h>
# include	<cm.h>
# include	<qr.h>
# include	<er.h>
# include	"erqr.h"
# include	<qrhelp.h>


/**
** Name:	qrsqhelp.c -	performs help on sql statements
**
** Description:
**	Originally extracted from sqhelp.c in the old sql terminal monitor.
**	This file handles the reading of the qrhlpsql.hlp file in II_HELPDIR,
**	the text source of the help sql and help [sql statement]. This file
**	defines:
**
**	qrsqhelp	routine to do this
**	qrsqreadnum	reads which message number we're at.
**	findsqlhelp	processes the help for all of "help sql"
**	IIQRghf_GetHelpFile	Locate the help file 'qrhlpsql'.
**
** History:
**	20-aug-87 (daver) Modified from sqhelp.c
**	26-may-88 (bruceb)	Fix for bug 2697
**		Increased numbuf buffer size by 1 to avoid fatal overflow.
**	17-aug-88 (bruceb)
**		Moved sqhelp.txt file from ING_FILES directory to II_HELPDIR
**		directory and renamed qrhlpsql.hlp.  Added IIQRghf routine
**		as part of this change.
**	28-oct-88 (kathryn) Added Release 6 commands
**	11-Jul-89 (bryanp)
**		The interface to IIQRghf_GetHelpFile was flawed because it
**		attempted to return a location whose location buffer was built
**		as a local variable in IIQRghf_GetHelpFile. Since this buffer
**		does not persist after the end of the IIQRghf_GetHelpFile call,
**		it cannot be used to return a value to the caller. Fixed by
**		making the buffer static. Also added an LOcopy call after the
**		NMloc call, for the same basic reason (to provide a buffer
**		with a reasonable persistence).
**		Finally, the HP MPE/XL filing system needs a slightly
**		different set of LO calls to correctly locate the help file.
**		Added some system-specific code for MPE/XL help file processing.
**	22-sep-89 (teresal) Added new syntax for Terminator (Release 6.3)
**		commands and STAR commands.
**	15-nov-90 (kathryn)
**		Added syntax for B1 Security commands. Added QRcomment, 
**		QRenable and QRdisable as first keywords.
**	22-mar-91 (kathryn) Added new syntax for Release 6.4.
**	10-jul-91 (kathryn) Changed event to dbevent.
**	13-apr-93 (shelby) Added FUNC_EXTERN declaration for qrtokcompare().
**		Without this, help tablename doesn't work on the TI1500, and I
**		suspect on other boxes as well.
**      02-oct-94 (Bin Li) Fix bug 63283,set the id pointer as 2160 for the
**              'Alter table' statement to point to the corresponding message
**              in the qrhlpsql.hlp file. 
**	11-feb-94 (robf)
**              Add Secure 2.0 extensions.
**	14-mar-95 (peeje01) Cross integration from 6500db_su4_us42
**		Original comment appears below:
** **	05-nov-93 (hming) When using the double-byte help files, the "HELP ..."
** **              command doesn't work. This is because some of the double-byte
** **              characters have 0x7e(~) as their second byte.
** **              Added checks for double-byte character.
**	24-apr-96 (angusm)
**	add CREATE/DROP SYNONYM	(bug 74426)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/

/* # define's */

# define	HELPDELIM	'~'
# define	GETNUM	1
# define	SKIP	2
# define	PRINT	3

/*	types of statements */
# define	QRENDOFDATA	0
# define	QRSINGLE	1
# define	QRFIRSTHALF	2
# define	QRCOMBO		3
# define	QRSECONDHALF	4

/*	corresponding first keywords */
# define	QRALTER		10
# define	QRCOMMENT	15
# define	QRCREATE	20
# define	QRDIRECT	30
# define	QRDISABLE       35
# define	QRDROP		40
# define        QRENABLE        45
# define	QRRAISE		50
# define	QRREGISTER	60
# define	QRREMOVE	70

/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN	char	*qrscanstmt();
FUNC_EXTERN	STATUS	IIQRghf_GetHelpFile();
FUNC_EXTERN	bool	findsqlhelp();

/* static's */
typedef struct
{
	char	*name;
	i4	file_num;
	i4	type;
	i4	match_id;
} QRSQKEYWD;

/*
** NOTE:  Don't add a keyword with code '1'.  I am 'mis'-using that code
** in the qrhlpsql.hlp file to contain information for translators as to the
** syntax required by that file.
*/
static	QRSQKEYWD	qrsq_1stkeys[] =
{
	{ ERx("alter"),                 0,      QRFIRSTHALF,    QRALTER},
	{ ERx("comment"),               2600,   QRSINGLE,    	QRCOMMENT},
	{ ERx("commit"),		2000,	QRSINGLE,	0},
	{ ERx("copy"),			2001,	QRSINGLE,	0},
	{ ERx("create"),		0,	QRFIRSTHALF,	QRCREATE},
	{ ERx("declare"),		460,	QRSINGLE,	0},
	{ ERx("delete"),		600,	QRSINGLE,	0},
	{ ERx("direct"),		0,	QRFIRSTHALF,	QRDIRECT},
	{ ERx("disable"),               0,      QRFIRSTHALF,    QRDISABLE},
	{ ERx("drop"),			500,	QRCOMBO,	QRDROP},
	{ ERx("enable"),                0,      QRFIRSTHALF,    QRENABLE},
	{ ERx("grant"),			2050,	QRSINGLE,	0},
	{ ERx("insert"),		700,	QRSINGLE,	0},
	{ ERx("message"),		470,	QRSINGLE,	0},
	{ ERx("modify"),		900,	QRSINGLE,	0},
	{ ERx("raise"),			0,	QRFIRSTHALF,	QRRAISE},
	{ ERx("register"),              3100,   QRCOMBO,        QRREGISTER},
	{ ERx("remove"),	        3120,   QRCOMBO,        QRREMOVE},
	{ ERx("revoke"),	        2350,   QRSINGLE,       0},
	{ ERx("rollback"),		2070,	QRSINGLE,	0},
	{ ERx("save"),			2002,	QRSINGLE,	0},
	{ ERx("savepoint"),		2003,	QRSINGLE,	0},
	{ ERx("select"),		100,	QRSINGLE,	0},
	{ ERx("set"),			2080,	QRSINGLE,	0},
	{ ERx("sql"),			1000,	QRSINGLE,	0},
	{ ERx("update"),		800,	QRSINGLE,	0},
	{ ERx("help"),			1001,	QRSINGLE,	0},
	{ ERx("'datatypes'"),		5000,	QRSINGLE,	0},
	{ ERx("'set_functions'"),	5005,	QRSINGLE,	0},
	{ ERx("'numeric_functions'"), 	5010,	QRSINGLE,	0},
	{ ERx("'type_functions'"),	5011,	QRSINGLE,	0},
	{ ERx("'string_functions'"),	5012,	QRSINGLE,	0},
	{ ERx(""),			0,	QRENDOFDATA,	0},
};

static	QRSQKEYWD	qrsq_2ndkeys[] =
{
	{ ERx("view"),			200,	QRSECONDHALF,	QRCREATE},
	{ ERx("table"),			300,	QRSECONDHALF,	QRCREATE},
	{ ERx("index"),			400,	QRSECONDHALF,	QRCREATE},
	{ ERx("procedure"),		450,	QRSECONDHALF,	QRCREATE},
	{ ERx("profile"),		3250,	QRSECONDHALF,	QRCREATE},
	{ ERx("error"),                 480,    QRSECONDHALF,   QRRAISE},
	{ ERx("permit"),		2005,	QRSECONDHALF,	QRCREATE},
	{ ERx("permit"),		2010,	QRSECONDHALF,	QRDROP},
	{ ERx("integrity"),		2004,	QRSECONDHALF,	QRCREATE},
	{ ERx("integrity"),		2015,	QRSECONDHALF,	QRDROP},
	{ ERx("procedure"),		2017,	QRSECONDHALF,	QRDROP},
	{ ERx("profile"),		3252,	QRSECONDHALF,	QRDROP},
	{ ERx("group"),                 2100,   QRSECONDHALF,   QRALTER},
	{ ERx("location"),              2120,   QRSECONDHALF,   QRALTER},
	{ ERx("profile"),               3251,   QRSECONDHALF,   QRALTER},
	{ ERx("role"),                  2150,   QRSECONDHALF,   QRALTER},
        { ERx("table"),                 2160,   QRSECONDHALF,   QRALTER},
	{ ERx("security_audit"),        3253,   QRSECONDHALF,   QRALTER},
	{ ERx("user"),                  2170,   QRSECONDHALF,   QRALTER},
	{ ERx("group"),                 2200,   QRSECONDHALF,   QRCREATE},
	{ ERx("location"),              2210,   QRSECONDHALF,   QRCREATE},
	{ ERx("role"),                  2220,   QRSECONDHALF,   QRCREATE},
	{ ERx("rule"),                  2230,   QRSECONDHALF,   QRCREATE},
	{ ERx("security_alarm"),   	2240,   QRSECONDHALF,   QRCREATE},
	{ ERx("synonym"),   			2260,   QRSECONDHALF,   QRCREATE},
	{ ERx("synonym"),   			2270,   QRSECONDHALF,   QRDROP},
	{ ERx("user"),                  2250,   QRSECONDHALF,   QRCREATE},
	{ ERx("group"),                 2300,   QRSECONDHALF,   QRDROP},
	{ ERx("location"),              2310,   QRSECONDHALF,   QRDROP},
	{ ERx("role"),                  2320,   QRSECONDHALF,   QRDROP},
	{ ERx("rule"),                  2330,   QRSECONDHALF,   QRDROP},
	{ ERx("security_alarm"),        2335,   QRSECONDHALF,   QRDROP},
	{ ERx("user"),                  2340,   QRSECONDHALF,   QRDROP},
 	{ ERx("security_audit"),        2400,   QRSECONDHALF,   QRENABLE},
 	{ ERx("security_audit"),        2500,   QRSECONDHALF,   QRDISABLE},
	{ ERx("link"),                  3030,   QRSECONDHALF,   QRCREATE},
	{ ERx("connect"),               3040,   QRSECONDHALF,   QRDIRECT},
	{ ERx("disconnect"),            3043,   QRSECONDHALF,   QRDIRECT},
	{ ERx("execute"),               3046,   QRSECONDHALF,   QRDIRECT},
	{ ERx("link"),                  3050,   QRSECONDHALF,   QRDROP},
	{ ERx("dbevent"),               3200,   QRSECONDHALF,   QRCREATE},
	{ ERx("dbevent"),               3210,   QRSECONDHALF,   QRDROP},
	{ ERx("dbevent"),               3220,   QRSECONDHALF,   QRRAISE},
	{ ERx("dbevent"),               3230,   QRSECONDHALF,   QRREGISTER},
	{ ERx("dbevent"),               3240,   QRSECONDHALF,   QRREMOVE},
	{ ERx(""),			0,	QRENDOFDATA,	0},
};

static	bool	qrsqreadnum();
static 	char	helpfn[MAX_LOC + 1];

/*{
** Name:	qrsqhelp	- deal with help on sql statements
**
** Description:
**	Open the SQL help file in II_HELPDIR (qrhlpsql.hlp) and read until the
**	appropriate help message is found.  When it's found, print it on
**	standard output, and close the help file.
**
** Inputs:
**	qrb		the ubiquitous, omniscient QRB
**
**	helpnum		the number corresponding to the proper sql help item
**
** Outputs:
**	none
**
**	Returns:
**		none
**
** Side Effects:
**	places the text in the qrb's output buffer.
**
** History:
**	20-aug-87 (daver) Extracted from sqhelp.c
*/
VOID
qrsqhelp(qrb, helpnum)
QRB	*qrb;
i4	helpnum;	/* help msg number to print */
{

	i4	c;
	FILE	*iop;
	LOCATION helploc;
	bool	done = FALSE;
	i4	number;
	i2	op;

	/*  Get the location for, and open, the SQL help file */
	if ((IIQRghf_GetHelpFile(&helploc) != OK)
		|| SIopen(&helploc, ERx("r"), &iop))
	{
		qrprintf(qrb, ERget(E_QR0005_Unable_to_open_sqhelp));
		return;
	}

	op = GETNUM;
	while ( !done )
	{

		if ( op == GETNUM )
		{
			if ( qrsqreadnum( &number, iop ) == FALSE )
				done = TRUE;
			else if ( helpnum == number )
				op = PRINT;
			else
				op = SKIP;
		}
		else if ( op == SKIP )
		{
			for ( c = SIgetc( iop ); c != HELPDELIM && c != EOF;
				c = SIgetc( iop ) )
			{
				if (CMdbl1st(&c))
					c = SIgetc(iop);
			}
			if ( c == HELPDELIM )
				op = GETNUM;
			else
				done = TRUE;
		}
		else		/* op == PRINT */
		{
			for ( c = SIgetc( iop ); c != HELPDELIM && c != EOF;
				c = SIgetc( iop ) )
			{
				qrputc(qrb, c);
				if (CMdbl1st(&c))
				{
					c = SIgetc(iop);
					qrputc(qrb, c);
				}
			}
			done = TRUE;
		}
	}
	qrputc(qrb, (char)EOS);

	SIclose( iop );
}

static	bool
qrsqreadnum( number, iop )
i4	*number;
FILE	*iop;
{
# define numbsize 20
	char	numbuf[numbsize+1];
	char	*numptr;
	char	c = EOF;
	i2	i;

	for ( i = 0; i <= numbsize; numbuf[i++] = '\0' );
	numptr = numbuf;

	for( i = 0, c = SIgetc( iop );
		c != EOF && c != HELPDELIM && i <= numbsize;
		c = SIgetc( iop ) )
	{
		if ( c != '\n' )
		{
			*numptr++ = c;
			i++;
		}
	}
	if ( c == EOF )
		return( FALSE );

	*numptr++ = '\0';

	if ( CVan( numbuf, number ) )
		return( FALSE );
	else
		return( TRUE );
}


/*{
** Name:	findsqlhelp	- call sqhelp if its a "help sql" statement
**
** Description:
**	This routine is called to determine if this is a "help sql keyword"
**	statement, and if it is, to call sqhelp with the appropriate number.
**
** Inputs:
**	qrb		ubiquitous QRB
**	s		ptr to the next word in the statement buffer, not
**			necessarily null-terminated at end of token.
**
** Outputs:
**	none
**
**	Returns:
**		TRUE if it is a "help sql keyword" statement
**		FALSE if it isn't
**
** Side Effects:
**	if it IS a "help sql keyword" statement the qrb's output
**	buffer will be filled with "help sql" text
**
** History:
**	20-aug-1987 (daver)	First Written.
**	22-feb-1988 (neil)	Allow any quote as we want to get to single
**				rather than double.
**	07-nov-1988 (kathryn)	Bug# 3918
**		No longer qrtokcompare NULL string with a token.
**		qrtokcompare() uses STbcompare so comparison of NULL
**		always returns TRUE.
**      15-aug-1990 (kathryn) Bug 6547
**              Issue syntax error for help statements with extraneous
**              characters following legal help_statement. 
**		EX: "help help garbage" will now display syntax error rather 
**		than displaying output of "help help" command.
*/
bool
findsqlhelp(qrb, s)
QRB	*qrb;
char	*s;
{
	i4	i, j, dummy;
	i4	len1, len2;
	QRSQKEYWD	*key1, *key2;
	char	*t,*t1;

	/*
	** If the input token begins with a double quote (which is allowed
	** for compatibility with previous versions), then convert to
	** single quotes.
	*/
	if (*s == '"')
	{
	    *s = '\'';
	    if (t = STindex(s, ERx("\""), 0))
		*t = '\'';
	}

	/*
	** first, look through the list of 1st keywords for our string, s.
	** if we find a match, it may be a single keyword, the first half
	** of a double keyword, or a "combo" keyword (a combo keyword is
	** one like "drop", which can either be a single keyword,
	** i.e, "help drop", OR a double keyword, i.e. "help drop permit").
	**
	** if it is a single keyword, we call qrsqhelp with the corresponding
	** number in the file, and return.
	**
	** if it is a double keyword, we need to call qrscanstmt to find
	** the next keyword name. then we check all those in the 2nd keyword
	** list that match the id. if we find one, we call qrsqhelp
	** with the corresonding number (from the 2nd keyword array) that
	** is in the file qrhlpsql.hlp, and return.
	**
	** if we don't find one, and it is a "combo" keyword, we call
	** qrsqhelp with the corresponding number (from the 1st keyword array)
	** that is in the file qrhlpsql.hlp, and return. if it isn't a "combo"
	** keyword, generate a syntax error.
	**
	** if we can't find it in the first list, then its probably a
	** legal table name. return false.
	*/


	for (i = 0; (key1 = &qrsq_1stkeys[i]), key1->type != QRENDOFDATA; i++)
	{
	    /* check against all first keywords */

	    /* since 's' is not NULL-terminated, must use qrtokcompare() */
	    /* qrtokcompare() returns TRUE on exact match */
	    if (qrtokcompare(s, key1->name))
	    {
		/* find out what the next word is */
	        t = qrscanstmt(s, &dummy, FALSE);

		if (key1->type == QRSINGLE)
		{
		    if (*t != EOS)
		    {
				/* Garbage on the line */
			qrprintf(qrb, ERget(E_QR0007_invalid_help_syntax), s);
			qrprintf(qrb, ERget(E_QR0008_Type_help_help));
		    }
		    else
		    	qrsqhelp(qrb, key1->file_num);
		    return (TRUE);
		}
		else	/* look for matching 2nd keyword */
		{
		    /*
		    ** we've matched the keyword, but its not a single
		    ** keyword. there are now 3 possible cases here:
		    **	1) its a double keyword
		    **	2) its a combo keyword ("drop" & "drop view" are legal)
		    **	3) its a syntax error (like "help create blob")
		    ** search the second keyword list now...
		    */


		    if (*t != EOS) 	/* BUG  3918 */
		    {

		    	for (j = 0; (key2 = &qrsq_2ndkeys[j]), 
				key2->type != QRENDOFDATA; j++)
		    	{
				/*
				** only look at those second keywords that match
				** with the first keywords, anyway.
				*/
				if (key1->match_id == key2->match_id)
				{
				    /* again, must use qrtokcompare() */
				    if (qrtokcompare(t, key2->name))
				    {

					/* find out what the next word is */
					t1 = qrscanstmt(t, &dummy, FALSE);

					if (*t1 != EOS) 
					{
					    /* Garbage on the line */
			  		    qrprintf(qrb, 
			  		 ERget(E_QR0007_invalid_help_syntax),s);
			  		    qrprintf(qrb,
					 ERget(E_QR0008_Type_help_help));
					}
					else
						qrsqhelp(qrb, key2->file_num);
					return (TRUE);
				    }
				}
		    	}
		    }
		    /*
		    ** if we are here, we haven't found the 2nd keyword.
		    ** check if the first word was a "combo", meaning that
		    ** its valid by itself or with a second keyword. example:
		    ** "help drop" and "help drop view" are both legal.
		    */
		    if (key1->type == QRCOMBO)
		    {
			qrsqhelp(qrb, key1->file_num);
		    }
		    else
		    {
			/*
			** its a double keyword; we found the first half,
			** but didn't find the second half. let the
			** user know that "help help" is available
			*/
			qrprintf(qrb,
			    ERget(E_QR0007_invalid_help_syntax),
			    s);
			qrprintf(qrb,
			    ERget(E_QR0008_Type_help_help));
		    }
		    /*
		    ** always return true here because we did find
		    ** that first keyword. they're obvously trying to
		    ** get the text-file-style help and don't want the
		    ** help-table-style help.
		    */
		    return(TRUE);

		}   /* end else if a QRSINGLE and a match */

	    }	    /* end if the 1st keyword matched */

	}	    /* end for loop matching all first keywords */

	return (FALSE);
}

/**
** Name:	IIQRghf_GetHelpFile	- get the help file location.
**
** Description:
**	This will return a location containing the full path of the
**	help file used to service help sql and help sql_keyword.
**	This will first see if the II_HELPDIR name is defined, and if so,
**	use that directory plus the file name to generate the help file
**	name.  If it is not defined, it will use the ING_FILES/II_LANGUAGE
**	directory.
**
** Inputs:
**
** Outputs:
**	helploc	- location with full path name of sqhelp.hlp.
**	  NOTE: helploc contains pointers into 'helpfn', a
**		static variable. That means each call to this routine
**		overwrites the results of the previous call. Be
**		careful of this if you use this function!
**
**	Returns:
**		OK or FAIL if error occurs.  This will print errors.
**
** History:
**	17-aug-88 (bruceb)
**		Swiped from runtime's FEhlpnam() to avoid bringing in
**		all the forms code in that file, and modified for
**		this specific use.
**	11-Jul-89 (bryanp)
**		The interface to IIQRghf_GetHelpFile was flawed because it
**		attempted to return a location whose location buffer was built
**		as a local variable in IIQRghf_GetHelpFile. Since this buffer
**		does not persist after the end of the IIQRghf_GetHelpFile call,
**		it cannot be used to return a value to the caller. Fixed by
**		making the buffer static. Also added an LOcopy call after the
**		NMloc call, for the same basic reason (to provide a buffer
**		with a reasonable persistence).
**		Finally, the HP MPE/XL filing system needs a slightly
**		different set of LO calls to correctly locate the help file.
**		Added some system-specific code for MPE/XL help file processing.
**	17-Jun-2004 (schka24)
**	    Avoid overrun on the returned buffer.
*/

STATUS
IIQRghf_GetHelpFile(helploc)
LOCATION	*helploc;
{
	STATUS		clerror;
	LOCATION	temploc;
	char		*hfile;
	char		*dirname;
	char		langstr[ER_MAX_LANGSTR + 1];
	i4		langcode;

	/* If II_HELPDIR defined, use it */

	NMgtAt(ERx("II_HELPDIR"), &dirname);
	if ((dirname != NULL) && (*dirname != EOS))
	{
	    /* Something specified */
	    STlcopy(dirname, helpfn, sizeof(helpfn)-1);
	    clerror = LOfroms(PATH, helpfn, helploc);
	}
	else
	{
	    /* Now get the language name and add to FILES directory */
	    clerror = ERlangcode(NULL, &langcode);
	    if (clerror == OK)
	    {
		/* If ERlangcode works, ERlangstr will too */
		_VOID_ ERlangstr(langcode, langstr);
		clerror = NMloc(FILES, PATH, langstr, &temploc);
	    }
	    /*
	    ** Since NMloc returns pointers to its own static buffer, make
	    ** a copy of the location using our static buffer, so that at
	    ** least our buffer will persist beyond the next NMloc call.
	    */
	    if (clerror == OK)
	        LOcopy( &temploc, helpfn, helploc );
	}

	if (clerror == OK)
	{
	    /*
	    ** helploc and helpfn now contain the location of the help files
	    ** directory. Now add the help file name.
	    */
#ifdef hp9_mpe
	    clerror = LOfstfile(ERx("qrhlpsql"), helploc);
#else
	    clerror = LOfstfile(ERx("qrhlpsql.hlp"), helploc);
#endif
	}

	if (clerror != OK)
	{
	    IIUGerr(clerror, 0, 0, 0);
	    return(FAIL);
	}

	return(OK);
}
