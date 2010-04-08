/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <si.h>
# include <st.h>
# include <lo.h>
# include <er.h>
# include <cv.h>
# include <cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <fe.h>
# include <ermc.h>
# include <ug.h>

/*
** MARKER value initially placed in attribute bits to flag currently
** unspecified characters.  Allows us to catch double definitions.
** Must be some "impossible" value.
*/
#define MARKER 0XFFFF

#define THRESHOLD 40

/*
** fgets buffer length for file read, also length of buffer for message
** composition.
*/
#define BUFLEN 800

/**
** Name:	main.c -	chset_install program.
**
** Description:
**	This program "installs" a new character set, ie. it reads a text
**	description of a character set, and uses CMwrite_attr() to
**	set up the description for access by CMset_attr() in mainline
**	code.
**
** History:
**	4/90 (bobm)	Written
**      1/91 (stevet)   Fixed problem with definition file with 8-bit 
**                      characters. 
**	06-may-91 (sandyd)
**	    New definition of stderr in <si.h> (for shareable image vectoring 
**	    of data on VMS) made compile-time initialization of Errfp illegal.
**	    I moved that initialization down into the routine.
**	7-may-91 (jonb)
**	    Add ming hint to make this end up in utility.
**	12-jun-91 (sandyd)
**	    Removed CHINSTLIB from ming hints.  It doesn't exist (because
**	    this utility only has a main module), and that reference causes 
**	    ming warnings.
**	22-aug-91 (gillnh2o)
**	    Removed DEST = so that the executable goes into bin. 
**	26-aug-93 (dianeh)
**	    Added length argument (as 0) to calls to STindex().
**      20-oct-93 (huffman)
**          due to changes in <ug.h>, <fe.h> is now required for VMS 
**          compile.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-jun-2002 (fanra01)
**	    Sir 108122
**	    Add NT specific attributes for host, path and users.
**/

/*
**
PROGRAM = chset_install
NEEDLIBS = UGLIB COMPATLIB
*/

static char *Filename;		/* name of input file */
static FILE *Errfp;		/* file pointer for error output */
static i4  Linenumber;		/* current line number for diagnostics */
static i4  Errcount;		/* current error count */

static i4  Threshold = THRESHOLD;	/* compilation error threshold */

static FILE *locopen();
static VOID readfile();
static VOID parse_record();
static STATUS find_range();
static STATUS decode_bits();
static VOID init_attr();
static VOID end_attr();
static VOID fatal();
static VOID diagnostic();
static VOID lookup();
static VOID cmw_fatal();

main(argc,argv)
int argc;
char **argv;
{
	FILE *fp;
	char *chset;

	Errfp = stderr;

	if (argc < 3)
		fatal(E_MC0002_USAGE, (PTR) NULL, (PTR) NULL);

	/*
	** argc > 3 because we insist on character set / file arguments.
	** We allow "-" to say "read from stdin".
	*/
	for (++argv; argc > 3 && **argv == '-'; --argc,++argv)
	{
		++(*argv);
		switch(**argv)
		{
		case 'e':
			++(*argv);
			if (**argv == EOS)
				fatal(E_MC0001_NOEFILE, (PTR) NULL, (PTR) NULL);
			Errfp = locopen(*argv,ERx("w"),ERget(F_MC0002_ERROUT));
			break;
		case 't':
			++(*argv);
			CVan(*argv,&Threshold);
			if (Threshold < 1)
				Threshold = 1;
			break;
		default:
			fatal(E_MC0002_USAGE, (PTR) NULL, (PTR) NULL);
		}
	}

	/*
	** name of character set.
	*/
	chset = *argv;
	++argv;

	/*
	** final argument is file name.
	*/
	if (STequal(*argv,ERx("-")))
	{
		Filename = ERget(F_MC0003_STDIN);
		fp = stdin;
	}
	else
	{
		Filename = *argv;
		fp = locopen(Filename,ERx("r"),ERget(F_MC0004_INPUT));
	}

	readfile(fp,chset);

	PCexit(OK);
}

/*
** local routine to open file, using LO.  Returns file pointer, produces
** an exit on error.
**
** Inputs:
**	fn - filename
**	mode - mode.
**	what - purpose, for error message.
*/

static FILE *
locopen(fn,mode,what)
char *fn;
char *mode;
char *what;
{
	LOCATION loc;
	char lbuf[MAX_LOC+1];
	FILE *fp;

	STcopy(fn,lbuf);

	if (LOfroms(PATH&FILENAME,lbuf,&loc) != OK ||
					SIopen(&loc,mode,&fp) != OK)
		fatal(E_MC0003_FILEOPEN, (PTR) fn, (PTR) what);

	return fp;
}

/*
** the real work - read the input, construct a CMATTR, and write it out.
**
** Inputs:
**	fp - open file to read.
**	chset - character set name.
*/
static VOID
readfile(fp,chset)
FILE *fp;
char *chset;
{
	char buf[BUFLEN+1];
	STATUS readst;
	CMATTR attr;
	CL_ERR_DESC clerr;
	char *ptr;

	init_attr(&attr);

	Errcount = Linenumber = 0;

	/*
	** read record, strip comments, skip empty and comment lines
	*/
	while ((readst = SIgetrec(buf,BUFLEN,fp)) == OK)
	{
		++Linenumber;
		ptr = STindex(buf,"#",0);
		if (ptr != NULL)
			*ptr = EOS;
		STtrmwhite(buf);
		if (buf[0] != EOS)
			parse_record(buf,&attr);
	}

	if (readst != ENDFILE)
		fatal(E_MC0004_READFAIL, (PTR) NULL, (PTR) NULL);

	end_attr(&attr);

	if (Errcount > 0)
		fatal(E_MC0005_SYNTAXERR, (PTR) Filename, (PTR) NULL);

	if (CMwrite_attr(chset,&attr,&clerr) != OK)
		cmw_fatal(&clerr);
}

/*
** handle a file record
**
** record format:
**
**	<hex code>[-<hex code>][<tab><attributes>][<tab><hex code>]
**
**	third field relevent only if 'u' appears in attribute bits.
**
** If a syntax error occurs, a diagnostic message is produced with an
** immediate return (we don't want to produce redundant messages for
** each member of a whole range of characters - one syntax error per line
** is enough).
**
** Inputs:
**	buf - file record
**	attr - CMATTR to modify
*/
static VOID
parse_record(buf,attr)
char *buf;
CMATTR *attr;
{
	char *tok[3];
	i4 count;
	i4 c1,c2;
	i4 abits;
	i4 xcase;
	i4 xv;

	/*
	** tokenize buffer
	*/
	count = 3;
	STgetwords(buf,&count,tok);

	if (count < 1)
		return;

	/*
	** leading token is range of characters
	*/
	if (find_range(tok[0],&c1,&c2) != OK)
		return;

	/*
	** second token is attribute specification.  This may be defaulted
	** to 0.
	*/
	if (count < 2)
		abits = 0;
	else
	{
		if (decode_bits(tok[1],&abits) != OK)
			return;
	}

	/*
	** third token is case translation character.  If attribute bits
	** don't contain _U, this is irrelevent.  If defaulted, we apply
	** the 'a' - 'A' rule.  Note that we can produce E_MC0006_CASEBOUNDS
	** for BOTH sides of the if statement - if default sends upper case
	** character out of range, we want to produce the error message.
	*/
	if ((abits & CM_A_UPPER) != 0)
	{
		if (count < 3)
			xcase = c1 + 'a' - 'A';
		else
		{
			if (CVahxl(tok[2],&xv) != OK)
			{
				diagnostic(E_MC0007_BADHEX,
						(PTR) tok[2], (PTR) NULL);
				return;
			}
			xcase = xv;
		}
		if (xcase <= 0 || (xcase + c2 - c1) > 255)
		{
			diagnostic(E_MC0006_CASEBOUNDS,(PTR) NULL,(PTR) NULL);
			return;
		}
	}


	/*
	** fill in the attributes and case translation.  If the attributes
	** are != MARKER, we've already specified this character.
	*/
	for ( ; c1 <= c2; ++c1)
	{
		if ((attr->attr)[c1] != MARKER)
		{
			diagnostic(E_MC0008_DUPCODE,(PTR) &c1, (PTR) NULL);
			return;
		}
		(attr->attr)[c1] = abits;
		if ((abits & CM_A_UPPER) != 0)
		{
			(attr->xcase)[c1] = xcase;
			++xcase;
		}
	}
}

/*
** parse range token, return values.  The token should consist of a
** hexadecimal value, optionally followed by a dash and another value.
**
** Inputs:
**	str - range token
**
** Outputs:
**	c1, c2 - returned range.
**
** returns OK, or FAIL for syntax error (will have produced diagnostic).
*/
static STATUS
find_range(str,c1,c2)
char *str;
i4  *c1, *c2;
{
	char *ptr;
	char *tok[2];
	i4 count;
	i4 xv1, xv2;

	ptr = STindex(str,"-",0);
	if (ptr != NULL)
	{
		*ptr = ' ';
		count = 2;
		STgetwords(str,&count,tok);
		if (count < 2)
		{
			diagnostic(E_MC0009_BADRANGE,(PTR) NULL,(PTR) NULL);
			return FAIL;
		}
		if (CVahxl(tok[0],&xv1) != OK)
		{
			diagnostic(E_MC0007_BADHEX,(PTR) tok[0],(PTR) NULL);
			return FAIL;
		}
		if (CVahxl(tok[1],&xv2) != OK)
		{
			diagnostic(E_MC0007_BADHEX,(PTR) tok[1],(PTR) NULL);
			return FAIL;
		}
	}
	else
	{
		if (CVahxl(str,&xv1) != OK)
		{
			diagnostic(E_MC0007_BADHEX,(PTR) str,(PTR) NULL);
			return FAIL;
		}
		xv2 = xv1;
	}

	if (xv1 > xv2)
	{
		diagnostic(E_MC000A_BADORDER,(PTR) NULL,(PTR) NULL);
		return FAIL;
	}

	if (xv1 > 255 || xv2 > 255 || xv1 < 0 || xv2 < 0)
	{
		diagnostic(E_MC000B_ILLCHAR,(PTR) NULL, (PTR) NULL);
		return FAIL;
	}

	*c1 = xv1;
	*c2 = xv2;

	return OK;
}

/*
** decode attribute bits.  Return OK, or FAIL for syntax errors (diagnostic
** will have been produced)
**
** Inputs:
**	str - attribute bit string.
**
** Outputs:
**	val - bits.
**
** History:
**      25-jun-2002 (fanra01)
**	    Add decode for NT specific hostname, pathname and username.
*/
static STATUS
decode_bits(str,val)
char *str;
i4  *val;
{
	i4 acc;

	acc = 0;

	for ( ; *str != EOS; ++str)
	{
		switch (*str)
		{
		case 'a':
			if ((acc & (CM_A_UPPER|CM_A_LOWER)) != 0)
			{
				diagnostic(E_MC000C_ALPHABIT,
						(PTR) NULL,(PTR) NULL);
				return FAIL;
			}
			acc |= CM_A_NCALPHA;
			break;
		case 'u':
			if ((acc & (CM_A_NCALPHA|CM_A_LOWER)) != 0)
			{
				diagnostic(E_MC000C_ALPHABIT,
						(PTR) NULL,(PTR) NULL);
				return FAIL;
			}
			acc |= CM_A_UPPER;
			break;
		case 'l':
			if ((acc & (CM_A_NCALPHA|CM_A_UPPER)) != 0)
			{
				diagnostic(E_MC000C_ALPHABIT,
						(PTR) NULL,(PTR) NULL);
				return FAIL;
			}
			acc |= CM_A_LOWER;
			break;
		case 'p':
			if ((acc & CM_A_CONTROL) != 0)
			{
				diagnostic(E_MC000D_PRINTBIT,
						(PTR) NULL,(PTR) NULL);
				return FAIL;
			}
			acc |= CM_A_PRINT;
			break;
		case 'c':
			if ((acc & CM_A_PRINT) != 0)
			{
				diagnostic(E_MC000D_PRINTBIT,
						(PTR) NULL,(PTR) NULL);
				return FAIL;
			}
			acc |= CM_A_CONTROL;
			break;
		case 'd':
			acc |= CM_A_DIGIT;
			break;
		case 'w':
			acc |= CM_A_SPACE;
			break;
		case 's':
			acc |= CM_A_NMSTART;
			break;
		case 'n':
			acc |= CM_A_NMCHAR;
			break;
		case 'o':
			acc |= CM_A_OPER;
			break;
		case 'x':
			acc |= CM_A_HEX;
			break;
		case '1':
			acc |= CM_A_DBL1;
			break;
		case '2':
			acc |= CM_A_DBL2;
			break;
# if defined(NT_GENERIC)
		case 'H':
			acc |= CM_A_HOST;
			break;
		case 'P':
			acc |= CM_A_PATH;
			break;
		case 'U':
			acc |= CM_A_USER;
			break;
# endif
		default:
			str[1] = EOS;
			diagnostic(E_MC000E_ILLATTR,(PTR) str, (PTR) NULL);
			return FAIL;
		}
	}

	*val = acc;
	return OK;
}

/*
** initialize CMATTR.  We set all attribute masks to MARKER so that we can
** tell when a given character has been multiply specified.  We make the
** xcase array an identity so that we can spot many to one mappings, and
** unmapped lower cases.
*/
static VOID
init_attr(attr)
CMATTR *attr;
{
	i4 i;

	for (i=0; i < 256; ++i)
	{
		(attr->xcase)[i] = i;
		(attr->attr)[i] = MARKER;
	}
}

/*
** post-process CMATTR.  This means:
**
**	1) default all those still set to MARKER.
**
**	2) set translation for lower case characters.
**
**	3) put out error messages for bad case translation, ie. for upper
**	   case translated to something other than a lower case character,
**	   lower case with no upper case translation, many-to-one mappings.
**
**	We do this stuff after the fact because we don't know what order
**	characters are going to be processed in, and the lower case translation
**	for a given upper case character may not have been read yet when we
**	process it.  We have to wait until we've done everything to see if
**	the case translation makes sense.
*/
static VOID
end_attr(attr)
CMATTR *attr;
{
	i4 i;
	i4 other;

	/*
	** default MARKER's, check that translations of all upper case
	** are lower case, set corresponding lower case translation.
	** If lower case translation has already been set, produce the
	** "many-to-1" error.
	*/
	for (i=0; i < 256; ++i)
	{
		if ((attr->attr)[i] == MARKER)
			(attr->attr)[i] = CM_A_CONTROL;
		if (((attr->attr)[i] & CM_A_UPPER) != 0)
		{
			other = (unsigned char) (attr->xcase)[i];
			if (((attr->attr)[other] & CM_A_LOWER) == 0)
				diagnostic(E_MC000F_XLBAD,
						(PTR) &i, (PTR) &other);
			else
			{
				/* 
                                ** Added cast (unsigned char) to fix   
				** problem with 8-bit characters      
                                */
				if ((unsigned char)((attr->xcase)[other]) != other)
					diagnostic(E_MC0010_MANYTO1,
						(PTR) &other, (PTR) NULL);
				else
					(attr->xcase)[other] = i;
			}
		}
	}

	/*
	** Check that all lower cases are translated.
	**
	** second loop because we set the xcase array above, and again,
	** know nothing about the order.
	*/
	for (i=0; i < 256; ++i)
	{
		if (((attr->attr)[i] & CM_A_LOWER) != 0)
		{
			other = (unsigned char) (attr->xcase)[i];
			/* 
                        ** Added cast (unsigned char) to fix problem with 8-bit
                        ** characters.
                        */
			if (((unsigned char)((attr->attr)[other]) & CM_A_UPPER) == 0)
				diagnostic(E_MC0011_NOUPPER,
						(PTR) &i, (PTR) NULL);
		}
	}
}

/*
** fatal error message - print message and exit.
*/
static VOID
fatal(idx,a1,a2)
ER_MSGID idx;
PTR a1, a2;
{
	IIUGerr(idx,UG_ERR_ERROR,3,ERget(F_MC0001_CMDNAME),a1,a2);
	IIUGerr(E_MC0014_ABORT,UG_ERR_FATAL,0);

	PCexit(1);	/* safety */
}

/*
** diagnostic - print error message with filename and line number.
** increment count.  When we hit threshold count, bailout with fatal error.
*/
static VOID
diagnostic(idx,a1,a2)
ER_MSGID idx;
PTR a1, a2;
{
	IIUGerr(idx,UG_ERR_ERROR,4,(PTR) Filename,(PTR) &Linenumber,a1,a2);

	++Errcount;

	if (Errcount > Threshold)
	{
		Errcount = 0;
		diagnostic(E_MC0012_TOOMANY, (PTR) NULL, (PTR) NULL);
		fatal(E_MC0005_SYNTAXERR, (PTR) Filename, (PTR) NULL);
	}
}

/*
** fatal error due to CMwrite_attr failure - cover ERlookup syntax for CL.
*/
static VOID
cmw_fatal(err)
CL_ERR_DESC *err;
{
	char		buf[BUFLEN+1];
	i4		len;
	CL_ERR_DESC	ec;

	len = BUFLEN;

	if (ERlookup(0, err, ER_TEXTONLY, (i4) 0, buf,
				BUFLEN, iiuglcd_langcode(),
				&len, &ec, 0, (ER_ARGUMENT *) NULL) != OK)

		STcopy(ERget(F_MC0005_GFAIL),buf);

	fatal(E_MC0013_CMWRTERR, (PTR) buf, (PTR) NULL);
}
