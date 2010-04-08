/*
** Copyright (c) 1985, 2008 Ingres Corporation
*/
# include	 <compat.h>
# include	 <gl.h>
# include	 <lo.h>
# include	 <pc.h>
# include	 <nm.h>
# include	 <si.h>
# include	 <st.h>
# include	 <array.h>
# include	 <ut.h>
# include	 <ds.h>
# include	 <me.h>
# include	 <er.h>
# include	 "uti.h"
# include	 <cm.h>
# include	 <descrip>
# include	 <varargs.h>
# include        <lib$routines.h>

/*
** NOTE:
**
** Old logic surrounding CHspace, and relying on autoincrement order,
** is too easy too disturb.  Hence this ruse.  This routine is not used
** to scan user strings, so we aren't worried about doublebyte spaces.
*/

static char Xc;
#define ISSPACE(x) ((Xc = x) == ' ' || Xc == '\t' || Xc == '\n')

/**
** utexe.c -
**
**	Contents:
**		UTexe -- Execute an INGRES subsystem.
**		UTcomline -- Get a command line.
**
** Execute the subsystem 'program' passing it the arguments
** given in arg1 through argN.  The arglist tells how to
** interpret the arguments.  The syntax of the arglist is
**
**	<arglist> ::= <argassgn> {, <argassgn>}
**				|
**		      empty
**
**	<argassgn> ::=	<arg> '=' <argspec>
**
**	<arg> ::=	<id>
**
**	<argspec> ::=	%S
**				|
**			%L
**				|
**			%N
**				|
**			%F
**				|
**			%B
**
**
** The <arg> are given in a file pointed to by II_UTEXE_DEF, or
** by the file "<program>.def" in ~ingres/files/deffiles if it exists, or
** "utexe.def" in "~ingres/files".  The file tells
** what operating system flags are needed to start up the program.
** For QBF the entry in the file is:
**
**	qbf	qbf.exe
**		database	%S
**		equel		-X%S
**		user		-u%S
**		table		%S
**		form		-F %S
**		update		-u
**		append		-a
**		retrieve	-r
**		entry		%S -f
**		flags		%B
**	
** The first line names the program and the name of the executable
** to find in ingres.bin.  On VMS, we use spawn, so we first check
** to see if qbf is a symbol in the CLI.  If not, then we define
** one for the qbf.exe in ingres.bin before running qbf.
** 
** For example, if the program wants to call qbf as if typing
**	qbf database entry -f
** the program issues the call
** 	UTexe("qbf", "entry = %S", "entry");
**
** Thus the program doesn't need to know about flags.
**
** The argument specs mean the following:
**	%S is a string
**	%L is a LOCATION
**	%N is an integer
**	%F is a double floating point.
**	%B is a series of strings that must be broken up
**	   with STgetword.
**	%D is a shared data structure
**	%R is a shared data structure to be returned by second process
**
**
**	History:
**	3/89 (bobm)	Remove DSload / store calls, and support
**			for %R, %D.  Add support for %N, %F conversion
**			to UTgetstr.  I don't really like allocating
**			memory here, but with major restructuring, it
**			would be difficult to handle this, and we
**			actually need it.  Cut out old history (all prior
**			to 1986 and probably irrelevent).
**	13-feb-90 (sylviap)
**			Added batch support.
**	14-mar-90 (sylviap)
**			Changed CLERRV to CL_ERR_DESC.
**	04-mar-91 (sylviap)
**			Changed ModDictDesc and ArrayOf(DsTemplate *) to PTR.
**			  dict and dsTab are no longer used and should always 
**			  be passed as NULL.
**			Deleted UTerr since errors should be passed back through
**			  the CL_ERR_DESC.
**	4/91 (Mike S)
**			Get UTexe definition file name using UTdeffile.
**	29-jul-1991 (mgw)
**			Initialize variable "status" to OK in UTeprog() to
**			prevent failure in the case where II_UTEXE_DEF is
**			set.
**	4-feb-1992 (mgw)
**			Added trace file handling.
**	10/13/92 (dkh) - Removed the use of the ArrayOf() construct which
**			 confused the alpha c compiler.
**	11/16/92 (dkh) - Changed to handle variable number of arguments
**			 on the alpha.
**      16-jul-93 (ed)
**	    added gl.h
**	11-aug-93 (ed)
**	    added missing includes
**      01-jul-1998 (kinte01)
**          Changed PC_NOWAIT to PC_NO_WAIT which is the way it is defined
**          in the Unix CL
**      22-oct-1998 (kinte01)
**	    Delimited identifiers are now handled correctly when being
**	    passed from one Ingres application to another -- i.e. from the
**	    tables utility doing a query (really calling qbf). the underlying
**	    call to qbf now handles the delimited identifier properly
**	    (bug 82672)
**	    Also removed if VMS, if pyramid, if alpha stuff so the routine is
**	    easier to maintain.
**	16-Nov-1998 (kinte01)
**	    Reverse part of the previous fix and cross integrate
**	    change 438485 to correct this problem on VMS
**	29-Jan-1998 (kinte01)
**	    Yet another reworking of the handling of delimited identifiers.
**	    Previous changes caused bug 95100 when saving report via RBF.
**      19-may-1999 (kinte01)
**          Added casts to quite compiler warnings & change nat to i4
**          where appropriate
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	07-feb-01 (kinte01)
**	    Add casts to quiet compiler warnings
**	04-jun-2001 (kinte01)
**	   Replace STindex with STchr
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.  Remove unused #define.
*/

/*
** Manifest constants 
*/

# define	UTESTR		2043
# define	UTECOM		511
# define	UTEARGS		29
# define	UTETAG		71	/* < 100 - avoid conflict with FE */

static  STATUS	UTescan();
STATUS	UTlatebin();
STATUS	UTefind();
static STATUS	UTeapply();
static STATUS	UTestr();
static STATUS	UTenum();
static STATUS	UTeprog();
static STATUS	UTebinary();
STATUS	UTedefsym();
STATUS  UTcomline();
STATUS	UTeexe();
STATUS	UTegoodsym();
static STATUS	UTecom();
static STATUS	UTebreak();
FUNC_EXTERN VOID UTopentrace();
FUNC_EXTERN VOID UTlogtrace();
FUNC_EXTERN STATUS UTemodtype();
FUNC_EXTERN STATUS UTdeffile();
FUNC_EXTERN STATUS PCcmdut ();

/*
** Global data structure that holds the argument list for a program.
*/

/* UTEFORMAL defined in <UT.h>	*/

typedef struct
{
	i4		*ute_value;	/* Value of actual argument */
	UTEFORMAL	*ute_form;	/* Formal argument for this actual */
	char		ute_spec;	/* Spec of actual argument */
	i2		ute_dsId;	/* data structure Id (for %D) */
} UTEACTUAL;

typedef struct {
	union {
		char	*comline;
		Array	*comargv;
	} result;
	ModDict	*module;
} UT_COM;

/*
** These pointers point to the global data structures that the module
** operates on.
*/

static UTEFORMAL	*Utelines;	/* Points to the argument lines */
static int		Utelcnt;	/* Number of slots left in Utelines */
static char		*Utestr;	/* The pointer to the string buffer */
static int		Utescnt;	/* Number of characters left in Utestr*/
static char		*Utecom;	/* The current command buffer */
static char		*Utecombeg;	/* The begin of the current buffer */
static int		Uteccnt;	/* Number of characters left in Utecom*/
static i4		*ret_addr;
static i4		*ret_ptr;
GLOBALDEF bool		isProcess;

static char		*UTdebug;
/*
** UT.h defines the maximum number of parameters any UTexe error has.
*/
static UTERR		UTeerr ZERO_FILL;
static char		*wherefrom;

/*
** UTexe
**
** UTexe gets a command line and then it calls
** PCcmdline to run the command.
**
** Note: On VMS, the first and third arguments to UTexe will always
** be NULL, since each front-end will run as a single process.
**
**	26-oct-1985 (peter)	Add long argument list for Pyramid to
**				get around the problem with a 12 param max.
*/
/* VARARGS */
UTexe(mode, err_log, dict, dsTab, program, err_code, arglist, va_alist)
	i4			mode;
	LOCATION                *err_log;
	PTR			dict;
	PTR			dsTab;
	char			*program;
	CL_ERR_DESC		*err_code;
	char			*arglist;
	va_dcl
{
	UT_COM		com;
	STATUS		status;
	SH_DESC		sh_desc;	/* First process file desc */
	SH_DESC		desc_ret;	/* Second process file desc */
	i4		wait;
	FILE		*trcfile = NULL; /* for II_UT_TRACE output */
	va_list		ap1;
	i4	        N;

	CL_CLEAR_ERR(err_code);
	wherefrom = "UTexe definition file not opened yet";
	sh_desc.sh_type = 0;		/* initialize */
	sh_desc.sh_keep = (mode & UT_KEEP) ? KEEP : 0;
	desc_ret.sh_type = 0;
	desc_ret.sh_keep = (mode & UT_KEEP) ? KEEP : 0;
	desc_ret.sh_reg.file.name = NULL;

	va_start(ap1);
	N = va_arg(ap1, i4);

	if ((status = UTcomline(&sh_desc, &desc_ret, mode, dict, dsTab,
			program, err_code, arglist, &com, N, 
			ap1)))
	{
		va_end(ap1);
		return status;
	}

	va_end(ap1);
	
	/*
	** At this point, the command line or argument array
	** is completely filled in. It can now be executed.
	*/
	if (com.module->modType == UT_PROCESS) 
	{
		UTopentrace(&trcfile); /* open trace file if II_UT_TRACE set */

		if (trcfile)  /* if II_UT_TRACE was set, log the command */
		{
			SIfprintf(trcfile, "%s\n", com.result.comline);
			_VOID_ SIclose(trcfile);
		}

		if (mode & UT_NOWAIT)
			wait = PC_NO_WAIT;
		else
			wait = PC_WAIT;
		status  = PCcmdut(com.result.comline, wait, err_log, err_code);

		/* log output if II_UT_TRACE set */
		UTlogtrace(err_log, status, err_code);
			
	}
	else 
	{
		/* call the module as a subroutine */
		status = (*com.module->modDesc->modFunc)(
				com.result.comargv->size,
				com.result.comargv->array);
	}

	MEtfree(UTETAG);

	return  status;
}


/*
** UTcomline
**
** Build the command line to be executed or returned to
** routines above.
**
*/

STATUS
UTcomline(sh_desc, desc_ret, mode, dict, dsTab, program, err_code, arglist, 
	  com, N, argn)
	SH_DESC			*sh_desc;
	SH_DESC			*desc_ret;
	i4			mode;
	PTR			dict;
	PTR			dsTab;
	char			*program;
	CL_ERR_DESC 		*err_code;
	char			*arglist;

	UT_COM			*com;
	i4			N;
	va_list			argn;
{
	char			*arg;
	char			argspec;
	char			*cp;
	i4			i;
	FILE			*fptr;
	STATUS			status;
	/*
	** These big buffers hold the strings from the file
	** and the resultant command line. The pointers make
	** them global.
	*/
	char			strings[UTESTR+1];
	static char		command[UTECOM+1];
	UTEFORMAL		lines[UTEARGS+1];
	UTEACTUAL		actual[UTEARGS+1];
	static Array		comargv;
	static i4		*comarray[UTEARGS];

	CL_CLEAR_ERR(err_code);
	NMgtAt("II_UTEXE_DEBUG", &UTdebug);
	if (UTdebug != NULL && *UTdebug == '\0')
		UTdebug = NULL;
	Utestr = strings;
	Utescnt = UTESTR;
	Utelines = lines;
	Utelcnt = UTEARGS;
	Utecom = command;
	Utecombeg = command;
	Uteccnt = UTECOM;
	if ((status = UTeprog(program, dict, &com->module, err_code)))
		return status;
	if (isProcess = com->module->modType == UT_PROCESS) {
		Utecom = command;
		Utecombeg = command;
		Uteccnt = UTECOM;
	}
	else {
		comargv.size = 0;
		comargv.array = comarray;
		com->result.comargv = &comargv;
	}

	UTebinary(program, com);

	if (status = UTescan(mode, arglist, actual, &com->module->modArgs,
				N, argn))
		return status;

	if (status = UTeapply(actual, N, dsTab, sh_desc, desc_ret,
			isProcess? NULL: com))
		return status;

	if (isProcess) {
		/* encode the first desc */
		i = DSencode(sh_desc, Utecom, 'S');
		if (i > Uteccnt)
			return (FAIL);
		Utecom += i;
		Uteccnt -= i;

		
		/* encode the second desc */
		i = DSencode(desc_ret, Utecom, 'R');
		if (i > Uteccnt)
			return UTEBIGCOM;
		Uteccnt -= i;

		com->result.comline = command;

		if (UTdebug != NULL)
		{
			SIprintf("\rCommand (length = %d) : %s\n",
				STlength(command), command);
			SIflush(stdout);
		}
	}
	else {	/* using comargv */
		if (UTdebug != NULL) {
			SIprintf("\rCommand argv (size = %d)\n",
				comargv.size);
			for (i=0; i < comargv.size; )
				SIprintf("\r%x\n",comargv.array[i++]);
		}
	}
				
	return OK;
}

/*
** UTescan (includes former UTefind )
**
** Scan the arglist to find argument and argument spec
** pairs.  The syntax is <arg> = %<argspec> [, ...]
** Then find the formal argument decriptor for the actual arg.
**
** ARGUMENTS
**	list	The argument list to scan.
**	actual	actual argument descriptors
**	formal	Array of formal arguments
**	N	number of actual arguments
**	argn	ptr array of actual arguments
**
*/

static STATUS
UTescan(mode, list, actual, formal, N, argn)
i4		mode;
register char	*list;
UTEACTUAL	*actual;
FormalArray	*formal;
i4		N;
va_list		argn;
{
	register char		*argname;
	register UTEFORMAL	*fp;
	int			arglen;
	i4			i;
	i4			j;
	char			*save;

	for (i = 0; i < N && *list; i++, actual++)
	{
		actual->ute_value = (i4 *) va_arg(argn, i4 *);

		save = list;
		/*
		** Skip white space, get to the first character
		** of an argument name.
		*/
		while (ISSPACE(*list++))
			;
		argname = --list;
		while (!ISSPACE(*list) && *list != '=' && *list)
			list++;
		arglen = list - argname;

		/*
		** List now points to the first white space after the
		** argument name.  Scan to the EQUAL sign.
		*/
		while (ISSPACE(*list++))
			;
		if (list[-1] != '=')
			return (FAIL);
		/*
		** Look for the start of the argument spec.
		** It is marked with a %.
		*/
		while (ISSPACE(*list++))
			;
		/*
		** Now just past some sort of non white character.
		** It should be a %.
		*/
		if (list[-1] != '%')
			return (FAIL);
		/*
		** This character is the spec.
		*/
		if ((actual->ute_spec = *list++) == '\0')
			return (FAIL);
		/*
		** Just scanned off actual argument and spec. Now look
		** for end of string or next comma. This gets string
		** set up for the next call of scan.
		*/
		while (ISSPACE(*list++))
			;

		list--;
		if (*list == ',') {
			/* move past comma */
			list++;
		}
		else if (*list)
			/* neither a comma nor null */
			return  (FAIL);

		/* if actual arg is a shared data structure
		** get dsId from next arg
		*/
		if (actual->ute_spec == 'D')
		{
			actual->ute_dsId = (i2) va_arg(argn, i4);
		}
		else if (actual->ute_spec == 'R')
		{
			ret_addr = (i4 *) actual->ute_value;
			actual->ute_dsId = (i2) va_arg(argn, i4);
		}

		/*
		** Find the formal argument from the module
		** dictionary entry.
		*/
		for (j = 0; j < formal->size; j++)
		{
			fp = &formal->array[j];
			if (!STbcompare(fp->ute_arg, 0, argname, arglen, TRUE))
			{
				actual->ute_form = fp;
				goto nextactual;
			}
		}
		if (mode & UT_STRICT)
		{
                        char    buf[13];
                        i4     len;
                                    
                        len = min(12, arglen);
			STlcopy(argname, buf, len);
                        return (FAIL);
		}
		/* if no formal argument don't treat as an error
		** just skip the actual 
		*/
		DBGprintf("No formal argument found for actual: %s\n",
			argname);
		actual->ute_form = NULL;

nextactual:	;
	}
	return OK;
}

/*{
** Name:		UTeapply
**
** History:
**	24-sep-1985 (joe)
**		Added support for %C and %E.
*/

static STATUS
UTeapply(actual, N, dsTab, sh_desc, desc_ret, com)
	UTEACTUAL		*actual;
	i4			N;
	PTR			dsTab;
	SH_DESC			*sh_desc;
	SH_DESC			*desc_ret;
	UT_COM			*com;	/* the whoel shebang */
{
	register UTEACTUAL	*arg;
	register char	*line;
	register char	*ip;		/* points into line at '%' */
	STATUS		status = 0;
	int		i;
	bool		isProcess = (com == NULL);

	for (arg = actual, i = 0; i < N; i++, arg++) {
		/*
		** If the formal arg is NULL just skip the actual
		*/
		if (arg->ute_form == NULL)
			continue;

		line = arg->ute_form->ute_line;
		if (isProcess){
			*Utecom++ = ' '; Uteccnt--;
		}

		while ((ip = STchr(line, '%')) != NULL)
		{
			if (isProcess) {
				*ip = '\0';
				if (status = UTecom(line))
					return status;
				*ip = '%';
			}
			line = ip+1;
			switch (*line++)
			{
			  case '%':
				if (isProcess) {
					status = UTecom("%");
				}
				break;

			  case 'S':
				status = UTestr(arg, com);
				break;

			  case 'N':
				status = UTenum(arg, com);
				break;

			  case 'B':
				status = UTebreak(arg, com);
				break;

			  case 'C':
				status = UTeexe('C', arg, com);
				break;

			  case 'E':
				status = UTeexe('E', arg, com);
				break;

			  default:
				return (FAIL);
			}
			if (status != OK)
				 return status;
		}
		if (isProcess) {
			if (status = UTecom(line))
				return status;
		}
	}
	return OK;
}

/*{
** Name:	UTgetstr	-	Get a string from a %S or %L
**
** Description:
**	Get the string value from an actual argument specified with %S
**	or %L.
**
** Inputs:
**	arg		-	The actual argument.
**
** Outputs:
**	cp		-	The place to put the string pointer.
**
** History:
**	24-sep-1985 (joe)
**		Written
*/
static STATUS
UTgetstr(arg, cp)
UTEACTUAL	*arg;
char		**cp;
{
	char	b[80];

	switch (arg->ute_spec)
	{
	case 'S':
		*cp = (char *) arg->ute_value;
		break;
	case 'L':
		LOtos((LOCATION *) arg->ute_value, cp);
		break;
	case 'N':
		STprintf(b,"%d", (i4) arg->ute_value);
                *cp = STtalloc(UTETAG,b);
		break;
	case 'F':
		STprintf(b,"%f", *((f8 *) arg->ute_value));
		*cp = STtalloc(UTETAG,b);
		break;
	default:
		b[0] = arg->ute_spec; b[1] = '\0';
		return (FAIL);
	}
	return OK;
}

/*
** UTestr
**
** Insert a string into the command line or arg array.
*/

static STATUS
UTestr(arg, com)
	UTEACTUAL	*arg;
	UT_COM		*com;
{
	char	*cp;
	int	i;
	i4	rval;

	if ((rval = UTgetstr(arg, &cp)) != OK)
		return rval;

	if (com == NULL) {
		return UTecom(cp);
	}
	else {
		i = arg->ute_form - com->module->modArgs.array;
		com->result.comargv->array[i + 1] = (int *)cp;
	}
	return OK;
}

/*
** UTebreak
**
** Insert several strings into the command line or argument
** array quoting each string.
*/

static STATUS
UTebreak(arg, com)
	UTEACTUAL	*arg;
	UT_COM		*com;
{
	char	*cp;
	char	buf[MAX_LOC];
	char	tbuf[MAX_LOC];
	char	*words[20];
	char	**wp;
	i4	count;
	int	i;
	i4	rval;
	i4	dlm = FALSE;

	if ((rval = UTgetstr(arg, &cp)) != OK)
		return rval;
	if (STlength(cp) >= MAX_LOC)
		return  (FAIL);
	count = 20;
	STcopy(cp, tbuf);
	STgetwords(tbuf, &count, words);
	for (cp = buf, wp = words; count > 0; count--, wp++)
	{
		if (com == NULL) 
		{
	                char *  ptr = *wp;
	                i4     i;
	                i4     len = STlength(ptr);

                        if (dlm == FALSE)
                        {
                            CMcopy (ERx ("\""), 1, cp);
                            CMnext (cp);
                        }

 			if ( (STchr(ptr, ' ') != NULL ) ||
				(STchr(ptr, '\"') != NULL )  )
			{
                                dlm = TRUE;
			}


                        for (i=0; (i < len); CMbyteinc(i, ptr), CMnext(ptr))
                        {
			    if (CMcmpcase(ptr,"\"") == 0)
			    {
				CMcopy (ERx ("\"\""), 2, cp);
				CMnext (cp);
				CMnext (cp);
				if (i > 3)
				{
				   CMcopy (ERx ("\""), 1, cp);
				   CMnext (cp);
                	           if (CMcmpcase(ptr,".") == 0)
                                   {
                                     CMcpychar (ptr, cp);
                                     CMnext (cp);  
                                     CMcopy (ERx ("\"\""), 2, cp);
                                     CMnext (cp);
                                     CMnext (cp);
                                     CMcpychar (ptr, cp);
                                     CMcopy (ERx ("\"\""), 2, cp);
                                     CMnext (cp);
                                     CMnext (cp);
                                   }
				}
			    }
                            if (CMcmpcase(ptr," ") == 0) 
                            {
			        CMcopy (ERx (" "), 1, cp);
                                CMnext (cp);
                            }
                            else
                            {
                                CMcpychar(ptr, cp);
                                CMnext (cp);
                            }
                        }
                        if (dlm == FALSE) 
                        {
                            CMcopy (ERx ("\""), 1, cp);
                            CMnext (cp);
                        }
                        CMcopy (ERx (" "), 1, cp);
                        CMnext (cp);

		}
		else 
		{
			com->result.comargv->array[com->result.comargv->size++] = (int *)*wp;
		}
	}

	if (com == NULL) 
	{
		*cp = '\0';
		return UTecom(buf);
	}

	return OK;
}

/*
** UTenum
**
** Insert a number into the command line or command arg array.
*/

static STATUS
UTenum(arg, com)
	UTEACTUAL	*arg;
	UT_COM		*com;
{
	char	buf[30];
	int	i;


	if (com == NULL) {
		if (arg->ute_spec == 'N')
			STprintf(buf, "%d", (i4) arg->ute_value);
		else if (arg->ute_spec == 'F')
			STprintf(buf, "%f", *((f8 *) arg->ute_value), '.');
		else if (arg->ute_spec == 'S')
			STcopy((char *) arg->ute_value, buf);
		else
		{
			buf[0] = arg->ute_spec; buf[1] = '\0';
			return (FAIL);
		}
		return(UTecom(buf));
	}
	else {
		i = arg->ute_form - com->module->modArgs.array;
		com->result.comargv->array[i + 1] = arg->ute_value;
	}

	return OK;
}

/*{
** Name:	UTeexe		-	Get the value for a %C or %E
**
** Description:
**	Gets a string from the actual.  This string is supposed to
**	be the name of the executable (if spec is C) or the full path
**	name of the executable (if spec is E) to run for this invocation.
**
** Inputs:
**	spec	-	One of C or E.
**	arg	-	The actual argument.
**	com	-	The command structure.
**
** Side Effects
**	Calls UTlatebin to add the name of the executable to the command
**	line being built.
**
** History:
**	24-sep-1985	(joe)
*/
STATUS
UTeexe(spec, arg, com)
char		spec;
UTEACTUAL	*arg;
UT_COM		*com;
{
	char	*cp;
	STATUS	rval;

	arg->ute_spec = 'S';
	if ((rval = UTgetstr(arg, &cp)) != OK)
		return rval;
	arg->ute_spec = 'E';
	if (com == NULL)
	{
		if (spec == 'C')
		{
			if (UTegoodsym(cp) != OK)
				return (FAIL);
		}
		else
		{
			UTedefsym("IIutexe", cp);
			cp = "IIutexe";
		}
		UTlatebin(cp);
	}
	else
	{
		return (FAIL);
	}
	return OK;
}

/*
** UTeprog
**
** Scan through the module dictionary looking for the module name
** that is given.
*/

static STATUS
UTeprog(modname, dict, theModule, err_code)
	register char	*modname;
	PTR		dict;
	ModDict		**theModule;
	CL_ERR_DESC	*err_code;
{
	FILE		*fp = NULL;
	UTEFORMAL	*arg = Utelines;
	i4		cnt = Utelcnt;
	STATUS		status = OK;
	i2		len;
	char		*sp;
	char		*cp = NULL;
	LOCATION	loc;
	char		buf[MAX_LOC];
	ModDict		*aModule;
	FUNC_EXTERN ModDict	*UTetable();

	*theModule = NULL;

	/* module dictionary must be in a file */
	if (cp == NULL) {
		/* look for reset default */
		NMgtAt("II_UTEXE_DEF", &cp);
	}
	if (cp != NULL && *cp != '\0') {
		wherefrom = cp;
		STcopy(cp, buf);
		DBGprintf("\rThe moduleDictionaryFile = %s\n", cp);
		LOfroms(FILENAME, buf, &loc);
		SIopen(&loc, "r", &fp);
	}
	else {
		LOCATION utloc;
		char    locbuf[MAX_LOC];
		char    *utfilename;

		status = UTdeffile(modname, locbuf, &utloc, err_code);
		if (status == OK)
		{
			/* Get UTexe definition file name */
			wherefrom = "default UTexe definition file";
			LOtos(&utloc, &utfilename);
			DBGprintf("\rHere I am with %s\n", utfilename);
			status = SIfopen(&utloc, ERx("r"), SI_TXT, 0, &fp);
		}
		else
		{
			DBGprintf("\rInternal error %d finding UTexe file\n",
				  status);
		}
	}

	if (status != OK || fp == NULL)
                return (FAIL);
	sp = Utestr;
	while (SIgetrec(sp, Utescnt, fp) == OK) {
		char	*endOfLine;
		/*
		** If there is a <tab> then it is not a module
		** line.  Once the module line is found, the
		** line should have at least 3 strings. The first is
		** the name of the module. The second is the name
		** of the executable. Third is the module type.
		** Another string may be present for FEutargopen use.
		*/
		if (*sp == '\t')
			continue;
		
		endOfLine = sp+STlength(sp);
		while (ISSPACE(*sp))
			sp++;
		cp = sp;
		while (!ISSPACE(*cp++))
			;
		cp[-1] = '\0';
		while (ISSPACE(*cp++))
			;
		--cp;
		if (*cp == '\0') {
			SIclose(fp);
			return (FAIL);
		}
		
		if (STbcompare(sp, 0, modname, 0, TRUE) == 0) {
			DBGprintf("\rHere I am having found the module entry\n");
			/* allocate a Module structure */
			*theModule = (ModDict *)MEreqmem(0, sizeof(ModDict), FALSE, NULL);
			aModule = *theModule;
			aModule->modName = sp;
			aModule->modDesc = (ModDesc *) MEreqmem(0, 
				sizeof(ModDesc), FALSE, NULL);
			aModule->modDesc->modLoc = cp;
			aModule->modDesc->modFunc = NULL;

			while (!ISSPACE(*cp++)) ;
			cp[-1] = '\0';
			/*
			** Just got the module name.  See if it is
			** %. If so then set modLoc to NULL to
			** signal that we don't have an executable yet.
			*/
			if (STcompare("%", aModule->modDesc->modLoc) == 0)
				aModule->modDesc->modLoc = NULL;

			/* skip to modType field and terminate string */
			while (ISSPACE(*cp++)) ;
			Utestr = --cp;		/* temp. use of Utestr */
			while (!ISSPACE(*cp++) && *cp != '\0') ;
			cp[-1] = '\0';
			aModule->modType = UTemodtype(Utestr);
			if (aModule->modType == NULL) {
				return (FAIL);
			}
			aModule->modArgs.array = Utelines;
			Utestr = endOfLine;

			while (Utescnt > 0 && cnt > 0 &&
				!SIgetrec(Utestr, Utescnt, fp))
			{
				len = STlength(Utestr);
				if (Utestr[len - 1] == '\n')
					Utestr[len - 1] = '\0';
				len++;
				if (*Utestr != '\t')
					break;
				cp = Utestr+len;	/* new buffer */
				while (ISSPACE(*Utestr))
					++Utestr;
				arg->ute_arg = Utestr;
				while (!ISSPACE(*Utestr) && *Utestr != '\0')
					++Utestr;
				if (*Utestr == '\0')
					continue;
				*Utestr = '\0';
				for (++Utestr; ISSPACE(*Utestr); ++Utestr)
					;
				arg->ute_line = Utestr;
				/*
				** blanks are now significant.  Look only
				** for tab delimeter.
				*/
				while (*Utestr != '\t' && *Utestr != '\0')
					++Utestr;
				*Utestr = '\0';
				Utestr = cp; Utescnt -= len;
				arg++; cnt--;
			}
			aModule->modArgs.size = arg - Utelines;
			arg->ute_arg = NULL;
			SIclose(fp);
			return OK;
		}
	}
	SIclose(fp);

	DBGprintf("\rHere I am returning NOPROG from UTeprog()\n");
	return (FAIL);
}


/*
** UTebinary
**
** Place the command in the command string.
** The character pointer points to the name of the executable.
*/

static
UTebinary(prog, com)
char	*prog;
UT_COM	*com;
{
	ModDict			*mod = com->module;
	char			*cp;
	LOCATION		loc;
	char			buf[256];
	int			ret;

	if (mod->modType == UT_PROCESS)
	{
		if (mod->modDesc->modLoc == NULL)
			return OK;
		if (UTegoodsym(prog) != OK)
		{
			NMloc(BIN, FILENAME, mod->modDesc->modLoc, &loc);
			LOtos(&loc, &cp);
			if ((ret = UTedefsym(prog, cp)) != OK)
				return ret;
		}
		cp = prog;
		ret = UTecom(cp);
	}
	else {
		cp = mod->modName;
		com->result.comargv->size = mod->modArgs.size + 1;
		com->result.comargv->array[0]
			= (int *)cp;
		ret = OK;
	}

	DBGprintf("\rHere is the binary name: %s\n", cp);

	return ret;
}

/*
** Name:	UTedefsym	-	Define a symbol.
** 
** Description:
**	Sends a command to define a symbol to the subprocess.
**
** Inputs:
**	sym	-	The name of the symbol.
**	exe	-	The full path of the executable.
**
** Side Effects
**	Defines a symbol in the subprocess held by PCcmdline.
**
** History
**	24-sep-1985 (joe)
**		Written
**	4-feb-92 (mgw)
**		Added code to write the symbol definition to the trace
**		file if II_UT_TRACE is set.
*/
STATUS
UTedefsym(sym, exe)
char	*sym;
char	*exe;
{
	char	    buf[256];
	CL_ERR_DESC err_code;
	FILE        *trc = NULL;

	/* create < symbol :== $equivalent > in a buffer */
	if (STlength(sym) + STlength(exe) + STlength(" :== $") > 255)
		return (FAIL);
	STpolycat(3, sym, " :== $", exe, buf);

	UTopentrace(&trc);	/* open trace file if II_UT_TRACE set */

	if (trc)		/* if II_UT_TRACE was set, log the command */
	{
		SIfprintf(trc, "%s\n", buf);
		_VOID_ SIclose(trc);
	}

	PCcmdline(0, buf, 0, NULL, &err_code);
	return OK;
}

STATUS
UTegoodsym(cp)
char	*cp;
{
	char			buf[256];
	struct dsc$descriptor_s	res;
	struct dsc$descriptor_s	desc;

	desc.dsc$b_class = DSC$K_CLASS_S;
	desc.dsc$b_dtype = DSC$K_DTYPE_T;
	desc.dsc$a_pointer = cp;
	desc.dsc$w_length = STlength(desc.dsc$a_pointer);
	res.dsc$b_class = DSC$K_CLASS_S;
	res.dsc$b_dtype = DSC$K_DTYPE_T;
	res.dsc$a_pointer = buf;
	res.dsc$w_length = 256;
	if ((lib$get_symbol(&desc, &res) & 01))
		return OK;
	else
		return FAIL;
}

static STATUS
UTecom(buf)
char	*buf;
{
	i4	len;

	if ((len = STlength(buf)) > Uteccnt)
		return (FAIL);

        STcopy(buf, Utecom);
        Utecom  += len;
        Uteccnt -= len;
        return OK;

}

/*{
** Name:	UTlatebin	-	We just got the name of an executable.
**
** Description:
**	This routine has to stick the string on the front of the buffer
**	UTecom has been putting characters into.  We have to shift
**	what is in there down and add the new cp to the front.
**
** Inputs:
**	cp	-	The string to stick on the front of the buffer.
*/
STATUS
UTlatebin(cp)
char	*cp;
{
	i4	len;
	char	*p1;
	char	*p2;

	if ((len = STlength(cp) + 1) > Uteccnt)
		return (FAIL);

	/*
	** Since cp + a space will fit, it is safe to
	** move the current characters in Utecom down len
	** places.
	** p1 points len characters ahead in the buffer
	** p2 points at the current character to copy.
	*/
	for (p1 = Utecom + len - 1, p2 = Utecom - 1; p2 >= Utecombeg; p1--, p2--)
		*p1 = *p2;
	/*
	** Now copy buf in and replace p1 with a space.
	*/
	STcopy(cp, Utecombeg);
	*p1 = ' ';
	Utecom  += len;
	Uteccnt -= len;
	return OK;
}
