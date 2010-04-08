/*
** Copyright (c) 1983, 2008 Ingres Corporation
*/
# include	<compat.h> 
# include	<gl.h>
# include	<lo.h> 
# include	<si.h> 
# include	<st.h>
# include	<pc.h> 
# include	<nm.h>
# include	<ut.h>
# include	<cm.h>
# include	<st.h>
# include	<cv.h>
# include	<er.h>
# include	<descrip>
# include       <lib$routines.h>

/*
**
**
** Compile the passed file and place the output in the passed output
** file.  The file is compiled according to its extension.  What
** compiler to use, and how to compile are stored in a file pointed
** to by the name II_ING_UTCOM.  This file has templates of how to
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
**		<id> -   Some string.
**
** For example, the following file
**
**	qc
**		%Eeqc %I.qc
**		cc %I.c /object=%0
**		delete %I.c
**
**  compile .qc files as equel c files.
** If no such file is found, a default file is used.
**
**	History
**		10/13/83 (dd) -- VMS CL
**		4/18/84  (jrc)	 Modified to use new pccmdline,
**				 Look at II_ING_UTCOM,
**				 Only defined symbols if needed.
**		13-feb-90 (sylviap)
**			Added parameters to PCcmdline for batch support.	
**              14-mar-90 (sylviap)
**                      Passing a CLERR_DESC to PCcmdline.  Interface change
**			affects calls to PCcmdintr.
**		10-apr-90 (Mike S) 
**			Add errfile, pristine, and clerror arguments
**		4/91 (Mike S)
**			Construct compilation control filename for unbundled 
**			products.
**		4-feb-1992 (mgw)
**			Added trace file handling keyed on II_UT_TRACE
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	04-jun-2001 (kinte01)
**	   Replace STindex with STchr
**	26-oct-2001 (kinte01)
**	   Clean up compiler warnings
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.  Add missing function
**	    return types.
*/
static STATUS UTcexecute();
static STATUS UTcsuffix();
VOID UTopentrace();
VOID UTlogtrace();
FUNC_EXTERN STATUS PCcmdcloseout();

static	STATUS		UTtrcopen = FAIL;	/* OK if trace file opened */
static	LOCATION	UTtracefile ZERO_FILL;	/* Location of tracefile */
static	char		UTtfbuf[MAX_LOC+1] = {EOS}; /* buffer for UTtracefile */

/*
**	these sizes are not large enough to handle VMS 4.0 file names
**
# define	UTMAXSUF	4
# define	UTMAXNAME	9
*/
# define	UTMAXSUF	MAX_LOC
# define	UTMAXNAME	MAX_LOC

# define 	UT_COM_DEF   "utcom.def"
# define 	UT_COM_TMPLT "utcom.%s"

STATUS
UTcompile(infile, outfile, errfile, pristine, clerror)
LOCATION	*infile;
LOCATION	*outfile;
LOCATION 	*errfile;
i4		*pristine;
CL_ERR_DESC	*clerror;
{
	char		**exe;
	FILE		*fp;
	i4		status;
	char		*exep;
	char		*inname;
	char		*outname;
	char		suffix[UTMAXSUF+1];
	char		name[UTMAXNAME+1];
	LOCATION	loc;
	char		buf[MAX_LOC];
	char		*cp;

	CL_CLEAR_ERR(clerror);
	*pristine = TRUE;		/* We can redirect all output on VMS */

	/* 
	** Open compile commands file. This is in the II_CONFIG directory:
	**	For core INGRES:        utcom.def
	**	For unbundled products: utcom.pnm
	**		"pnm" is the first three characters of the part name,
	**		lowercased if need be.
	**	These can be overridden by the logical II_ING_UTCOM.
	*/
	NMgtAt("II_ING_UTCOM", &cp);
	if (cp != NULL && *cp != '\0')
	{
		STcopy(cp, buf);
		LOfroms(FILENAME, buf, &loc);
		SIopen(&loc, "r", &fp);
	}
	else 
	{
		char 	filebuf[20];
		char    *filename;
		char    part[9];
		char    extension[4];

		if ((NMgetpart(part)) == NULL)
		{
			filename = UT_COM_DEF;
		}
		else
		{
			STlcopy(part, extension, 3);
			CVlower(extension);
			filename = STprintf(filebuf, UT_COM_TMPLT, extension);
		}
		NMf_open("r", filename, &fp);
	}
	if (fp == NULL)
		return UT_CO_FILE;
	if (LOexist(infile))
	{
		SIclose(fp);
		return UT_CO_IN_NOT;
	}
	LOdetail(infile, buf, buf, name, suffix, buf);
	if ((status = UTcsuffix(fp, suffix, &exe)))
	{
		SIclose(fp);
		return status;
	}
	LOcopy(infile, buf, &loc);
	LOfstfile(name, &loc);
	LOtos(&loc, &inname);
	LOtos(outfile, &outname);

	/*
	**  We execute all commands with "append" turned on, so we have to
	**  call PCcmdcloseout when we're done to close the log file.
	*/
	for (; *exe != NULL; exe++)
	{
		if ((status = UTcexecute(*exe, inname, outname, name,
					 errfile, clerror)) 
			!= OK)
		{
			SIclose(fp);
			LOdelete(outfile);
			PCcmdcloseout(FALSE);
			return status;
		}
	}
	SIclose(fp);
	PCcmdcloseout(FALSE);
	return OK;
}

/*
** UTcsuffix
**
**	Given a suffix and the compile file, find the commands to
**	use when compiling the file.
**
*/

static STATUS
UTcsuffix(fp, suffix, ret)
	FILE	*fp;
	char	*suffix;
	char	***ret;
{
	char		*cp;
	register i4	N;
	i4		i = 0;
	register char	*sp;
	i4		len;
	static char	*exe[11];
	static char	strbuf[1024];

	N = 1023;
	sp = strbuf;
	exe[i] = sp;
	while (SIgetrec(exe[i],N, fp) == OK)
	{
		len = STlength(sp);
		if (*sp == '\t')
			continue;
		if (sp[len - 1] == '\n')
			sp[len - 1]  = '\0';
		if (STbcompare(sp, 0, suffix, 0, TRUE) == 0)
		{
			while(SIgetrec(sp, N, fp) == OK && i < 10 && N > 0)
			{
				len = STlength(sp);
				if (*sp != '\t')
					break;
				exe[i++] = sp + 1;
				sp += len;
				*(sp - 1) = NULL;	/* remove new line */
				N -= len;
			}
			if (i >= 10)
				return UT_CO_LINES;
			if (N <= 0)
				return UT_CO_CHARS;
			exe[i] = NULL;
			*ret = exe;
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
*/

static STATUS
UTcexecute(line, iname, oname, name, errfile, clerror)
	register char	*line;
	char		*iname;
	char		*oname;
	char		*name;
	LOCATION	*errfile;
	CL_ERR_DESC	*clerror;
{
	register char	*ip,		/* points into line at '%' */
			*cp;		/* points into spot in command */
	i4		status;
	char		command[256];
	char		string[255];
	char		*c;
	char		file[256];
	FILE		*Trace = NULL;
	STATUS		rval;
	STATUS		(*pcfunc)();
	STATUS		PCcmdnointr();
	STATUS		PCcmdintr();

	pcfunc = PCcmdintr;
	cp = command;
	while ((ip = STchr(line, '%')) != NULL)
	{
		while (line < ip)
			*cp++ = *line++;
		line++;
		switch (*line++)
		{
		  case '%':
			*cp++ = '%';
			break;

		  case 'E':
		  {
			struct dsc$descriptor_s	desc;
			struct dsc$descriptor_s	res;
			LOCATION		loc;
			char			*strptr;
			char			buf[256];

			pcfunc = PCcmdnointr;
			c = file;
			while (!(CMwhite(line)))
				*c++ = *line++;
			*c++ = '\0';
			desc.dsc$b_class = DSC$K_CLASS_S;
			desc.dsc$b_dtype = DSC$K_DTYPE_T;
			desc.dsc$a_pointer = file;
			desc.dsc$w_length = STlength(file);
			res.dsc$b_class = DSC$K_CLASS_S;
			res.dsc$b_dtype = DSC$K_DTYPE_T;
			res.dsc$a_pointer = buf;
			res.dsc$w_length = 256;
			if (!(lib$get_symbol(&desc, &res) & 01) ||
			    res.dsc$w_length == 0)
			{
				NMloc(BIN, FILENAME, file, &loc);
				LOtos(&loc, &c);
				STcopy(c, string);
				STprintf(buf, "%s :== $%s.exe", file, string);
				PCcmdnointr(NULL, buf, FALSE, errfile, TRUE, 
					    clerror);
			}
			STcopy(file, cp);
			cp += STlength(cp);
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
		}
	}
	STcopy(line, cp);

	UTopentrace(&Trace);	/* see if tracing is requested */

	if (Trace)	/* if so, then write command to trace file */
	{
		SIfprintf(Trace, "%s\n", command);
		_VOID_ SIclose(Trace);
	}

	rval = (*pcfunc)(NULL, command, NULL, errfile, TRUE, clerror);

	UTlogtrace(errfile, rval, clerror);/* log error output to trace file */

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
			_VOID_ STlcopy(uttrcbuf, UTtfbuf, MAX_LOC);
			if (LOfroms(FILENAME&PATH, UTtfbuf, &UTtracefile) != OK)
			{
				_VOID_ STlcopy("trace.ut", UTtfbuf, MAX_LOC);
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
**	clerr	- CL_ERR_DESC structure for getting OS level error msgs
**
** History:
**	4-feb-1992 (mgw)
**		written
*/
VOID
UTlogtrace(errfile, status, clerr)
LOCATION	*errfile;
STATUS		status;
CL_ERR_DESC	*clerr;
{
	FILE	*fp;		/* stream for trace file (II_UT_TRACE) */

	CL_CLEAR_ERR(clerr);
	if (UTtrcopen != OK)	/* only set if II_UT_TRACE is on and trace */
		return;		/* file was successfully created */

	if (SIfopen(&UTtracefile, "a", SI_TXT, (i4) 0, &fp) != OK)
		return;

	if (status != OK && errfile != NULL)
		_VOID_ SIcat(errfile, fp);

	if (status != OK)
	{
		/*
		** Bug 38790 - print OS error messages if available
		**
		** Use lang 1 which is guaranteed to be ok. The system
		** will return OS errors in a system dependent way
		** regardless of our language.
		*/
		i4	lang = 1;
		i4	msg_len = ER_MAX_LEN;
		char	msg_buf[ER_MAX_LEN];
		CL_ERR_DESC	sys_err;

		msg_buf[0] = EOS;
		if (ERlookup(0, clerr, ER_TIMESTAMP, NULL, msg_buf,
		             msg_len, lang, &msg_len, &sys_err, 0,
		             NULL) == OK)
		{
			if (msg_buf[0] != EOS)
				SIfprintf(fp, "\n%s\n", msg_buf);
			else
				SIfprintf(fp, "\nERROR\n");
		}
	}

	SIfprintf(fp, "\n--------------\n"); /* add separator for readability */

	_VOID_ SIclose(fp);
}
