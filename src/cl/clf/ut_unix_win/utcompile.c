/*
**Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<gl.h>
#include	<cm.h>
#include	<lo.h>
#include	<nm.h>
#include	<si.h>
#include	<pc.h>
#include	<ds.h>
#include	<ut.h>
#include	<st.h>
#include	<cv.h>
#include        "utlocal.h"
#include	<errno.h>
#include	<clconfig.h>

# ifndef DESKTOP
# include       <sys/param.h>
# endif /* DESKTOP */

/*
** Maximum size of generated command line.  This should be set
** to the maximum length of arguments accepted by exec.  On
** most machines this will be NCARGS, from <sys/param.h>.
*/

# ifdef xCL_070_LIMITS_H_EXISTS
#     include <limits.h>
# endif /* xCL_070_LIMITS_H_EXISTS */

# ifdef ARG_MAX
#     define CMDLINSIZ    (ARG_MAX)
# else
# if defined(rmx_us5) || defined(pym_us5) || defined(sgi_us5)||defined(rux_us5) 
#     define NCARGS           51200 /* NB - this is maximum value of tunable */
# elif defined(NT_GENERIC)
#     define NCARGS           MAX_CMDLINE 
# endif
#     define CMDLINSIZ    (NCARGS > 40000 ? 40000 : NCARGS)
# endif /* ARG_MAX */

/**
** Name:	utcompile.c -	Compile a File.
**
** Description:
** Compile the passed file and place the output in the passed output
** file.  The file is compiled according to its extension.  What
** compiler to use, and how to compile are stored in a file pointed
** to by the name II_COMPILE_DEF.  This file has templates of how to
** compile different extensions.  The files are structured as follows
**
**	<extension>		-- characters for extension
**		<command>	-- indented by a tab
**		<command>
**		  ...
**		<command>
**
**	A command is 
**		<Eqlcommand> <arg> <arg> ... <arg>
** 		<OScommand>  <arg> <arg> ... <arg>
**
**      An <Eqlcommand> is an equel command.
**		%E<id>
**
**	An <OScommand> is an operating system command
**		<id>
**
**	<arg>s are arguments.  They can be either
**		%I   -  The pathname of the input file.
**		%O   -  The pathname of the output file.
**		%N   -  The name of the input file minus the extension.
**		%F   -  User-defined parameters passed as-is.
**		<id> -   Some string.
**
** For example, the following file
**
**	qc
**		%Eeqc %I.qc
**		cc -c %I.c
**		mv %I.o %O
**		rm %I.o
**
**  compile .qc files as equel c files.
** If no such file is found, a default file is used.
**
** History:
**	4/91 (Mike S)
**	Construct compilation control filename for unbundled products.
**
**	Revision 6.4  90/03/19  Mike S
**	Add errfile, pristine and clerror arguments.
**	Use PCdocmdline to get appended output and stderr redirection
**
**	Revision 6.4  90/03/08  sylviap
**	Added a parameter to PCcmdline call to pass a CL_ERR_DESC.
**
**	Revision 6.4  90/01/08  sylviap
**	Added PCcmdline parameters.
**
**	Revision 6.1  88/07/13  wong
**	Check for legal extension for checking for file existence.
**
**	Revision 6.0  87/10/09  12:50:37  bobm
**	kanji changes - scan using CMnext
**	change CH calls to CM  887/10/01  bobm
**	ST[r]index changes for Kanji  87/04/14  bobm
**
**	Revision 30.3  86/01/20  15:12:15  joe
**	Fix for bug 7392.  Remove object file if compile fails.
**	Revision 30.2  85/08/28  17:56:46  daveb
**	UTS chokes on static function forward refs
**
**	Revision 3.0  85/05/07  11:59:08  wong
**	Lower-cased name of the compilation definition file
**	("UTcom.def" --> "utcom.def").
**
**	Revision 1.6  84/12/18  17:03:56  wong
**	Final 3.0 Integration.
**
**	25-Jul-91 (johnr)
**		Replaced include UT.h for debugging purposes only.
**	4-feb-1992 (mgw)
**		Added trace file handling for II_UT_TRACE
**	4-sep-1992 (mgw) Bug #46478
**		Don't suppress return statuses by always returning UT_CMP_FAIL
**		on any failure. Make them available for reporting. Also
**		added some missing error messages reported here in
**		common!hdr!hdr!erclf.msg to fix this bug.
**      12-feb-1993 (smc)
**              Added forward declarations.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	19-apr-95 (canor01)
**	    added <errno.h>
**      23-Jul-95 (fanra01)
**              Modified to call PCdocmdline to use PCdowindowcmdline.
**              This allows a compile to be started and run without having
**              a console showing.
**      24-Jul-95 (fanra01)
**              Replaced the original PCdocmdline call and placed #if defined
**              around NT specific changes.
**      02-May-96 (fanra01)
**              Added check for line feed ('\n') character at the end of line.
**              The check is added for Win95 as the command interpreter is
**              affected by it.
**	21-oct-1996 (canor01)
**	    Added UTcompile_ex() to allow extra user-defined parameters to
**	    be passed to the compile script.
**	09-jan-97 (mcgem01)
**          Add include for <clconfig.h>;
**          If xCL_070_LIMITS_H_EXISTS is defined, include
**          <limits.h> and define CMDLINSIZ as ARG_MAX.
**	20-mar-97 (mcgem01)
**	    Rename UT.h to utlocal.h to avoid conflict with gl\hdr\hdr
**	07-apr-1997 (canor01)
**	    Changed UTcompile_ex(): user-defined parameter is now an
**	    array of pointers instead of a single string.
**	16-may-97 (mcgem01)
**	    Minor changes to match the function prototypes.
**	02-jun-97 (mcgem01)
**	    The maximum command line length on NT is MAX_CMDLINE.
**	26-Nov-1997 (allmi01)
**	    Added sgi_us5 to list of platforms that set NCARGS.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      03-jul-1999 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Mar-2010 (hanje04)
**	    SIR 123296
**	    For LSB builds, look in /usr/bin for compiler exe too
**	29-Nov-2010 (frima01) SIR 124685
**	    Copleted UTcfile, UTcsuffix and UTcexecute prototypes.
*/

/* forward declarations */
static  FILE            *UTcfile(void);
static  STATUS          UTcsuffix(FILE *fp, char *suffix, char ***ret);
static  STATUS          UTcexecute(
	register char	*line,
	char		*iname,
	char		*oname,
	char		*name,
	LOCATION	*errfile,
	i4		first,
	char		**parms,
	CL_ERR_DESC	*clerror);

static	STATUS		UTtrcopen = FAIL;	/* OK if trace file opened */
static	LOCATION	UTtracefile ZERO_FILL;	/* Location of tracefile */
static	char		UTtfbuf[MAX_LOC+1] = {EOS}; /* buffer for UTtracefile */

# define	UTMAXSUF	 4
# define	UTMAXNAME	32
# define	UTMAXLINE	10

# define UT_COM_DEF   "utcom.def"
# define UT_COM_TMPLT "utcom.%s"

/*{
** Name:	UTcompile()
**
** Returns:
**	{STATUS}  OK, no errors.
**		  UT_CO_FILE	"utcom.xxx" could not be opened or read.
**		  UT_CO_SUF	illegal extension.
**		  UT_CO_IN	input file does not exist.
**		(internal)
**		  UT_CO_CHARS
**		  UT_CO_LINES
*/
STATUS
UTcompile ( infile, outfile, errfile, pristine, clerror )
LOCATION	*infile;
LOCATION	*outfile;
LOCATION	*errfile;
bool		*pristine;
CL_ERR_DESC	*clerror;
{
    return(UTcompile_ex(infile, outfile, errfile, NULL, FALSE, pristine, clerror ));
}

STATUS
UTcompile_ex ( 
LOCATION	*infile, 
LOCATION	*outfile,
LOCATION	*errfile,
char		**parms,
bool		append,
bool		*pristine,
CL_ERR_DESC	*clerror
)
{
	FILE	*fp;
	STATUS	status = OK;
	i4	first;

	/* UTS doesn't like static functions */
	STATUS          UTcexecute(
	register char	*line,
	char		*iname,
	char		*oname,
	char		*name,
	LOCATION	*errfile,
	i4		first,
	char		**parms,
	CL_ERR_DESC	*clerror);
	FILE	*UTcfile(void);
	STATUS	UTcsuffix(FILE *fp, char *suffix, char ***ret);


	/* We can redirect everything */
	*pristine = TRUE;
	CL_CLEAR_ERR( clerror );

	if ( (fp = UTcfile()) == (FILE *)NULL )
		status = UT_CO_FILE;
	else
	{
		char	**exe;
		char	name[UTMAXNAME+1];
		char	suffix[UTMAXSUF+1];
		char	buf[MAX_LOC];

		LOdetail( infile, buf, buf, name, suffix, buf );

		/* Check extension first */
		if ( (status = UTcsuffix(fp, suffix, &exe)) == OK )
		{ /* legal extension */
			if ( LOexist(infile) != OK )
			{
				status = UT_CO_IN_NOT;
			}
			else
			{ /* ... file exists */
				char		*inname;
				char		*outname;
				LOCATION	loc;
# ifdef	SUN
				char	na[UTMAXNAME+1];
				char	path[MAX_LOC];
# endif		/* SUN */

				LOcopy(infile, buf, &loc);
				LOfstfile(name, &loc);
				LOtos(&loc, &inname);
# ifdef SUN
				LOsave();
				LOdetail(outfile, path, path, na, suffix, path);
				chdir(path);
				outname = outfile->fname;
# else
				LOtos(outfile, &outname);
# endif		/* SUN */
				for (first = (append?FALSE:TRUE); 
				     *exe != (char *)NULL; 
				     ++exe, first = FALSE)
				{
					if ( (status = UTcexecute(
							 *exe, inname, outname,
							 name, errfile, first,
							 parms,
							 clerror))
						 != OK )
					{
						LOdelete(outfile);
						break;
					}
				}
			}
		}

		SIclose(fp);
	}

# ifdef	SUN
	LOreset();
# endif

	return status;
}


/*
** UTcfile
**
**	Open the compile file.  This is in the II_CONFIG directory:
**		For core INGRES:	utcom.def
**		For unbundled products:	utcom.pnm
**			"pnm" is the first three characters of the part name,
**			lowercased if need be.
**
**	4/91 (Mike S)
**	Construct compilation control filename for unbundled products.
**	09-jun-1995 (canor01)
**	    pass buffer to NMgetpart() for reentrancy
*/


static FILE *
UTcfile(void)
{
	FILE		*fp;
	char		filebuf[20];
	char		*filename;
	char		part[9];
	char		extension[4];

	if (NMgetpart(part) == NULL)
	{
	    NMgtAt("II_COMPILE_DEF", &filename);
            if (filename != NULL && *filename != '\0')
	    {
		LOCATION loc;

		LOfroms( PATH & FILENAME, filename, &loc );
		SIfopen( &loc, "r", SI_TXT, 1, &fp);
		return ( fp );
	    }
	    else
	    {
		filename = UT_COM_DEF;
	    }
	}
	else
	{
		STncpy( extension, part, 3);
		extension[3]='\0';
		CVlower(extension);
		filename = STprintf(filebuf, UT_COM_TMPLT, extension);
	}
	NMf_open("r", filename, &fp);

	return ( fp );
}

/*
** UTcsuffix
**
**	Given a suffix and the compile file, find the commands to
**	use when compiling the file.
**
** History:
**	4-sep-1992 (mgw) Bug #46478
**		N counter (space left in strbuf[]) wasn't being decremented
**		by as much as sp (pointer to next position to write to in
**		strbuf[]) was being incremented resulting in too many
**		SIgetrec() calls in the unlikely event that the buffer was
**		filled up. Fixing this will allow the correct error to be
**		reported in that case.
**      12-feb-1993 (smc)
**              Removed forward declaration of STindex now it is formally
**              prototyped.
**      02-May-96 (fanra01)
**              Added check for line feed ('\n') character at the end of line.
**              The check is added for Win95 as the command interpreter is
**              affected by it.
*/

static STATUS
UTcsuffix(fp, suffix, ret)
FILE	*fp;
char	*suffix;
char	***ret;
{
	register char	**line;
	char		*cp;
	register i4	N;
	register char	*sp;

	static char	*exe[UTMAXLINE];
	static char	strbuf[BUFSIZ];

	line	= exe;
	N	= BUFSIZ - 1;
	sp	= strbuf;
	*line	= sp;

	while (SIgetrec(*line, N, fp) == OK)
	{
		if (*sp == '\t')
			continue;

		if ((cp = STchr(sp, '\n')) != (char *)NULL)
			*cp = '\0';

		if (STcmp(sp, suffix ) == 0)
		{
			register i4	n = 0;
				i4	len;

			while (SIgetrec(sp, N, fp) == OK &&
					n < UTMAXLINE && N > 0)
			{
                            if (*sp != '\t')
                            	break;
                            ++sp;
                            *line = sp;
                            /*  add a check for line feed before length      */
                            /*  is calculated.                               */
                            if ((cp = STchr(sp, '\n')) != (char *)NULL)
                                *cp = '\0';
                            len = STlength(sp) + 1;
                            sp += len;
                            N = N - len - 1;
                            ++line;
                            ++n;
			}

			if (n >= UTMAXLINE)
				return UT_CO_LINES;

			if (N <= 0)
				return UT_CO_CHARS;

			*ret = exe;
			*line = (char *)NULL;
			return OK;
		}
	}

	return UT_CO_SUF_NOT;
}


/*
** UTcexecute
**
**	Given a command line from the compile file, execute that
**	command line doing the appropiate thing.
** History:
**	07-jan-1997 (canor01)
**	    Increase size of command buffer from BUFSIZ to ARG_MAX (largest
**	    argument size for spawned command).
**	08-jan-1997 (canor01)
**	    Above change has potential to overflow stack.  Get dynamic
**	    memory instead.
**	18-Mar-2010 (hanje04)
**	    SIR 123296
**	    For LSB builds, look in /usr/bin for compiler exe too
*/

static STATUS
UTcexecute(line, iname, oname, name, errfile, first, parms, clerror)
register char	*line;
char		*iname;
char		*oname;
char		*name;
LOCATION	*errfile;
i4		first;
char		**parms;
CL_ERR_DESC	*clerror;
{
	register char	*ip,		/* points into line at '%' */
			*cp;		/* points into spot in command */
	char		*command;
	char		swchar;
	FILE		*Trace = NULL;
	STATUS		rval;
	i4		i;

	command = MEreqmem( 0, CMDLINSIZ, FALSE, &rval );
	if ( command == NULL )
	    return (rval);

	cp = command;

	while ((ip = STchr(line, '%')) != (char *)NULL)
	{
		while (line < ip)
		{
			CMcpychar(line,cp);
			CMnext(cp);
			CMnext(line);
		}

		CMnext(line);

		/*
		** only interested in comparing against characters
		** in the native character set, so swchar can be a
		** single byte.  Increment line using CMnext, however.
		** We don't want second bytes of multibyte characters.
		*/
		swchar = *line;
		CMnext(line);
		switch (swchar)
		{
		  case '%':
			*cp++ = '%';	/* native char - legit */
			break;

		  case 'E':
		  {
			LOCATION	loc;
			char		*c;
			char		file[MAX_LOC];

			c = file;

			while (!(CMwhite(line)))
			{
				CMcpychar(line,c);
				CMnext(c);
				CMnext(line);
			}

			*c++ = '\0';

			NMloc(BIN, FILENAME, file, &loc);
# ifdef conf_LSB_BUILD
			/* some exes are in /usr/bin */
			if ( LOexist(&loc) != OK )
			    NMloc(UBIN, FILENAME, file, &loc);
# endif

			LOtos(&loc, &c);
			STcopy(c, cp);
			cp += STlength(c);

			break;
		  }

		  case 'I':
			STcopy(iname, cp);
			cp += STlength(iname);
			break;

		  case 'O':
			STcopy(oname, cp);
			cp += STlength(oname);
			break;
		
		  case 'N':
			STcopy(name, cp);
			cp += STlength(name);
			break;

		  case 'F':
			if ( parms )
			{
			    for ( i = 0; parms[i]; i++ )
			    {
				if ( i )
				{
				    STcopy( " ", cp );
				    cp++;
				}
			        STcopy(parms[i], cp);
			        cp += STlength(parms[i]);
			    }
			}
			break;

		}
	}

	STcopy(line, cp);

# ifdef UT_DEBUG
	if (UT_CO_DEBUG)
		SIprintf("%s", command);
# endif

	UTopentrace(&Trace);	/* see if tracing is requested */

	if (Trace)		/* if so, then write command to trace file */
	{
		SIfprintf(Trace, "%s\n", command);
		_VOID_ SIclose(Trace);
	}
# if !defined(NT_GENERIC)
 	rval = PCdocmdline((LOCATION *)NULL, command, PC_WAIT, !first,
 			    errfile, clerror);
# else
        {
	rval = PCdowindowcmdline((LOCATION *)NULL, command, PC_WAIT, !first,
			    HIDE ,errfile, clerror);
        }
# endif
	UTlogtrace(errfile, rval);	/* log error output to trace file */

	MEfree(command);

	return rval;
}

/*
** Name: UTopentrace
**
** Description: 
**	Opens the trace output file if requested by the II_UT_TRACE environment
**	variable. Only tests II_UT_TRACE once and fills the UTtracefile
**	location appropriately if it's set. II_UT_TRACE should contain the
**	name of the trace file to write to. If that turns out to be a bad
**	location, we'll wing it and use "trace.ut".
**
** Inputs:
**	none
**
** Outputs:
**	Trace - open trace file stream FILE pointer if II_UT_TRACE is set.
**
** Side Effects:
**	Fills UTtracefile location if II_UT_TRACE is set and sets UTtrcopen
**	to OK if file was successfully opened. The UTtrcopen setting can be
**	used in UTlogtrace() to determine whether II_UT_TRACE is set.
**
** History:
**	4-feb-1992 (mgw)
**		written
*/
VOID
UTopentrace(Trace)
FILE	**Trace;
{
	char		*uttrcbuf = NULL;  /* tmp char buffer for NMgtAt() */
	static	bool	first_pass = TRUE; /* only call NMgtAt() once */

	if (first_pass)
	{
		NMgtAt("II_UT_TRACE", &uttrcbuf);
		if (uttrcbuf != NULL && *uttrcbuf != EOS)
		{
			STncpy( UTtfbuf, uttrcbuf, MAX_LOC);
			UTtfbuf[ MAX_LOC ] = EOS;
			if (LOfroms(FILENAME&PATH, UTtfbuf, &UTtracefile) != OK)
			{
				STncpy( UTtfbuf, "trace.ut", MAX_LOC);
				UTtfbuf[ MAX_LOC ] = EOS;
				_VOID_ LOfroms(FILENAME & PATH, UTtfbuf,
				               &UTtracefile);
			}
		}
		first_pass = FALSE;
	}
	if (UTtfbuf[0] != EOS)
		UTtrcopen = SIfopen(&UTtracefile, "a", SI_TXT, (i4)0,
		                    Trace);
}


/*
** Name: UTlogtrace
**
** Description: 
**	Opens the trace output file if requested and concatinates the
**	log output to the trace file.
**
** Inputs:
**	errfile - LOCATION of log file containing output from command
**	status  - status of command - if !OK then there should be some
**	          diagnostic output in errfile.
**
** History:
**	4-feb-1992 (mgw)
**		written
*/
VOID
UTlogtrace(errfile, status)
LOCATION	*errfile;
STATUS		status;
{
	FILE	*fp;

	if (UTtrcopen != OK)
		return;

	if (SIfopen(&UTtracefile, "a", SI_TXT, (i4) 0, &fp) != OK)
		return;

	/* if (status != OK && errfile != NULL) - can't count on status */
	if (errfile != NULL)
		 _VOID_ SIcat(errfile, fp);

	SIfprintf(fp, "\n--------------\n");

	SIclose(fp);
}
