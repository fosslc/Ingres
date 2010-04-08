/*
**  Copyright (c) 1985, 2004 Ingres Corporation
**  All rights reserved.
*/

/*
NO_OPTIM = ris_u64 rs4_us5 i64_aix a64_sol
*/

# include	 <compat.h>
# include	 <gl.h>
# include        <clconfig.h>
# include        <systypes.h>
# include	 <lo.h>
# include	 <pc.h>
# include	 <cm.h>
# include	 <nm.h>
# include	 <si.h>
# include	 <st.h>
# include	 <ds.h>
# include	 <ut.h>
# include	 <er.h>
# include	 <me.h>
# ifdef VMS
# include	<descrip>
# endif /* VMS */

# ifdef xCL_007_FILE_H_EXISTS
# include	<sys/file.h>
# else
# define	F_OK	0
# endif

# include 	<stdarg.h>
# include	<errno.h>

/**
** Name:	utexe.c -	Utility Execute INGRES Sub-system Routine.
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
**
**	History:
**
**		Revision 1.2  86/04/25  10:44:47  daveb
**		3.0 porting changes
**		
**		Revision 30.18  86/01/22  13:14:20  daved
**		remove redefinition of buffer.
**
**		Revision 30.17  86/01/20  15:22:33  rpm
**		Integrated 3.0/23-25 x line code - RPM.
**
**		Revision 30.16  86/01/15  09:38:06  joe
**		Added marian's bug fix to UTebreak so that the users argument
**		is not changed.
**
**		Revision 30.15  86/01/15  09:03:27  joe
**		Included UTerr.h
**
**		Revision 30.14  86/01/15  09:01:07  joe
**		Made NOARG error return only the argument that is bad.
**		Also, if utexe.def can't be open, return a specific error code.
**
**		24-aug-87 (boba) -- Changed memory allocation to use MEreqmem.
**
**		10/87 (bobm)	Integrate 5.0 changes
**
**		10/87 (bobm)	Convert to use CM string scanning
**
**		3/89 (bobm)	Remove DSload / store calls, and support
**				for %R, %D.  Add support for %N, %F conversion
**				to UTgetstr.  I don't really like allocating
**				memory here, but with major restructuring, it
**				would be difficult to handle this, and we
**				actually need it.  Cut out history prior to
**				1986.
**	30-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add includes for <clconfig.h> and <systypes.h>.
**	
**		05-jan-90 (sylviap) 
**			Added parameter, err_log, for forking a detached 
**			process.
**		07-mar-90 (sylviap) 
**			Changed err_log to be a LOCATION.  Added CL_ERR_DESC* 
**			parameter, so we can report system call failures.
**		22-aug-90 (sylviap)
**                    	UTexe call PCdocmdline with append = TRUE so the error
**                    	log will will be appended, rather than written over.
**                    	(#32106)
**		04-mar-91 (sylviap)
**			Changed dict and dstTab to be PTR.  They are no longer 
**			used and should be passed as NULL.
**		4/91 (Mike S)
**			Get UTexe definition file name using UTdeffile.
**		29-jul-1991 (mgw)
**			Initialize variable "status" to OK in UTeprog() to
**			prevent failure in the case where II_UTEXE_DEF is
**			set.
**		4-feb-1992 (mgw)
**			Added trace file handling.
**              12-feb-1993 (smc)
**                      Tidied up forward declarations.
**		22-mar-1993 (blaise)
**			Changed to use varargs, so UTexe really does accept
**			a variable number of arguments (previously it took
**			only 12 or fewer)
**		24-mar-1993 (blaise)
**			Correction to above change; changed argvect from a
**			fixed size array to an i4* allocated at runtime.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      07-sep-93 (smc)
**          Made STprintf use %p for addresses.
**	19-oct-93 (vijay)
**	    Roll back the previous change which uses %p for STprintf of
**	    ute_value. This structure element is being used as an int value
**	    here.
**	19-oct-93 (swm)
**	    Comment on 19-oct-93 (vijay) change. Apparently, the ute_value
**	    can hold a pointer or an i4. Instead comment the i4 cast as a
**	    valid truncating cast. The wisdom from Dave Hung is:
**	    "The UTexe spec indicates that the %N format is for designating
**	     an integer. From looking through the frontend code for parameters
**	     that are tied to the %N format, I conclude that %N is really
**	     meant for normal (base 10) values.
**	     The original code for UTexe appears to always treat ute_value as
**	     a 4 byte integer value when dealing with %N. I suspect that the
**	     original implementors were using ute_value to hold either a
**	     pointer or an integer..."
**	    Added lint truncation warning comment to indicate that the
**	    truncating casts are OK.
**	10-mar-94 (swm)
**	    Bug #60473
**	    UTescan() was not stepping through nargs correctly because
**	    type of each vararg was changed from i4 to PTR but the change
**	    was not reflected in the nargs type. Corrected nargs to PTR *.
**	17-mar-94 (kirke)
**	    Added include of me.h for MEtfree()/IIMEtfree().
**	28-oct-1994 (canor01)
**		Unix command processor gets confused by single quotes when
**		they are embedded in a format enclosed in single quotes
**	20-apr-1995 (emmag)
**		NT porting changes.
**	19-apr-1995 (canor01)
**		added <errno.h>
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	6-feb-1996 (angusm)
**		Backed out change of 28-oct-1994 for delimited identifier
**		names containing embedded single quotes: causes bug 74091.
**		New fix for bug 57722 to be added later.
**	9-feb-1996 (angusm)
**		implement more restrictive version of original fix
**		for bug 57722: only handled embedded single quotes
**		if inside delimited identifier name.
**	9-apr-1996 (schte01)
**		Added NO_OPTIM for axp_osf to correct error E_LQ0030 when
**		invoking sql from starview.
**	23-sep-1996 (canor01)
**		Move global data definitions to utdata.c.
**  24-jun-1997 (rodjo04)
**      Bug 83393. Increased value of UTECOM to 1023 on Windows NT 
**      platform.  
**      14-Oct-97 (fanra01)
**              Modified the parsing of '"' delimited strings on NT.
**              Since the '"' character is treated as shell delimiter it needs
**              to be escaped.
**  24-jul-1998 (rigka01)
**      bug 86989. UTECOM setting is restricting the size of the command
**      that abf can pass to report.  The setting is 1023 for Windows NT
**      and 511 on Unix et al.  Increase UTECOM to 4094 on all platforms.
**      Move UTECOM to utcl.h
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	15-jun-1999 (schte01)
**		Removed NO_OPTIM for axp_osf.
**	27-jul-1999 (somsa01)
**	    In UTecom(), amended last change to not happen in the case
**	    where the string in question already contains a quote in the
**	    beginning. This means that somebody else already took the
**	    time to quote this string.
**      25-Aug-1999 (hweho01)
**          Turned of optimization for ris_u64. Symptom: run-time 
**          error during iidbdb creation. 
**          The compiler is AIX C ver. 3.6.0.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-Jun-2000
**	    On S390 Linux (ibm_lnx) va_start seems to be being resolved from
**	    stdarg.h and not varargs.h as intended causing compilation
**	    errors. Moving #include varargs.h above #include <si.h> has
**	    resolved this.
**	09-Aug-2000 (ahaal01)
**	    NO_OPTIM AIX (rs4_us5) to avoid error during iidbdb creation.
**  14-Jun-2000 (hanal04) Bug 101835 INGCBT 282
**      Add 'program' parameter to calls to UTeapply() that we can
**      determine the program.
**	12-Sep-2000 (hanje04)
**	    Moved '#include varargs.h' above 'lo.h' so that 'va_start' is
**	    is resolved correctly from stdargs.h and not varargs.h
**	21-dec-2000 (toumi01)
**	    Make the move of #include <varargs.h> conditional on ibm_lnx,
**	    which is the platform that needed the change.  Moving the
**	    include to the above location breaks Linux builds on Intel and
**	    on Alpha (Red Hat 7.0), and who knows where else.
**	30-Jan-2001 (hanje04)
**	    Added Alpha Linux (axp_lnx) to above conditional include of 
**	    varargs.h
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	29-apr-2002 (rigka01) bug# 106776
**	    Backslashes in parameter values for report result in E_RW1001.	
**	    For report param on NT, UTecom will add a set of double quotes
**	    to the param value when the string contains at least one blank 
**	    and at least one backslash.  In this case, use (%S) instead of
**	    "(%S") in UTeapply() as the template.
**	16-jan-2003 (hayke02)
**	    For rs4_us5, prototype UTeexe() with i4/int promotable type for the
**	    char first argument, spec, to prevent compiler error.
**	20-Aug-2003 (fanra01)
**	    Bug 110747
**	    Running multiple reports from a threaded process through this
** 	    function is prone to memory corruptions as the function is not
**	    thread safe.  There are global pointers to buffers declared
**	    on the stack.
**	    Modified the module to save this context into a per-thread
**	    storage area.  Includes the addition of another level of indirection.
**	03-oct-2003 (somsa01)
**	    Properly prototype functions.
**	27-nov-2003  (rigka01) Problem INGDBA268, bug 111365
**	    Add quotes to executable path+filename so that paths 
**	    containing blanks and other special characters will
**	    execute. This change is in UTlatebin().
**	05-Jan-2004 (hanje04)
**	    Typedef UTformArray to stop compilation errors on Linux concerning
**	    the ArrayOf(UTFORMAL) * in the declaration of UTescan.
**	14-Jan-2004 (hanje04)
**	    Change functions using old, single argument va_start (in 
**	    varargs.h), to use newer (ANSI) 2 argument version (in stdargs.h). 
**	    This is because the older version is no longer implemented in 
**	    version 3.3.1 of GCC.
**	    Also changed type of spec in UTeexe from char to char* to quite
**	    compiler warnings (and sometimes errors) on some platfoms.
**	15-Jun-2004 (schka24)
**	    Watch out for overruns when getting env variables.
**	    Remove "wherefrom", not used.
**	15-Jul-2004 (hanje04)
**	    Remove || from NO_OPTIM line as it's confusing mkjam
**	19-Apr-2005 (bonro01)
**	    Added NO_OPTIM for a64_sol
**	23-Sep-2005 (hanje04)
**	    Queit compiler warnings
**	29-May-2006 (hweho01)
**          Removed the prototype of UTeexe() that was specific for AIX,   
**          because the first parameter is a character pointer now.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	18-Mar-2010 (hanje04)
**        SIR 123296
**        For LSB builds, look in /usr/bin for compiler exe too
*/

static char	*UTdebug = "Y";	

#define	DBGprintf	if(UTdebug!=NULL)SIprintf

/*
** Manifest constants 
*/

# define	UTESTR		2043
# define	UTEARGS		29
# define	UTETAG		71	/* <100 avoids conflict with FEgettag */

/*
** Global data structure that holds the argument list for a program.
*/

/* UTEFORMAL defined in <ut.h>	*/

typedef struct
{
	PTR		ute_value;	/* Value of actual argument */
	UTEFORMAL	*ute_form;	/* Formal argument for this actual */
	char		ute_spec;	/* Spec of actual argument */
	i2		ute_dsId;	/* data structure Id (for %D) */
} UTEACTUAL;

typedef ArrayOf(UTEFORMAL) UTFormArray;

static STATUS	UTescan(i4, register char *, UTEACTUAL *, UTFormArray *,
			CL_ERR_DESC *, i4, PTR *);
static STATUS	UTeapply(UTEACTUAL *, i4, PTR, char *, SH_DESC *, SH_DESC *,
			 UT_COM *, CL_ERR_DESC *);
static STATUS	UTestr(UTEACTUAL *, UT_COM *, CL_ERR_DESC *, char *);
static STATUS	UTenum(UTEACTUAL *, UT_COM *, CL_ERR_DESC *);
static STATUS	UTeprog(register char *, PTR, ModDict **, CL_ERR_DESC *);
static STATUS	UTebinary(char *, UT_COM *, CL_ERR_DESC *);
static STATUS	UTebreak(UTEACTUAL *, UT_COM *, CL_ERR_DESC *, char *);
static STATUS	UTecom(char *, CL_ERR_DESC *, char *, UTEACTUAL *);

STATUS		UTlatebin(char *, CL_ERR_DESC *);
STATUS		UTeexe(char *, UTEACTUAL *, UT_COM *, CL_ERR_DESC *);

FUNC_EXTERN STATUS	UTdeffile(char *, char *, LOCATION *, CL_ERR_DESC *);
FUNC_EXTERN i4		UTemodtype(char *);

/*
** These pointers point to the global data structures that the module
** operates on.
*/

typedef struct _ut_context
{    
    i2          metag;
    UTEFORMAL   *Utelines;  /* Points to the argument lines */
    int         Utelcnt;    /* Number of slots left in Utelines */
    char        *Utestr;    /* The pointer to the string buffer */
    int         Utescnt;    /* Number of characters left in Utestr*/
    char        *Utecom;    /* The current command buffer */
    char        *Utecombeg; /* The begin of the current buffer */
    int         Uteccnt;    /* Number of characters left in Utecom*/
    i4          *ret_addr;  /* address of shared data struct
                               returned by second process */
    i4          *ret_ptr;   /* ptr to arg passed to UTexe 
                               which holds ret_addr */
    char        strings[UTESTR+1];
    char        command[UTECOM+1];
    UTEFORMAL   lines[UTEARGS+1];
    Array       comargv;
    i4          *comarray[UTEARGS];
}UTCONTEXT;

GLOBALREF bool		isProcess;

static METLS_KEY    uttlskey ZERO_FILL;
static bool         utinitialized = FALSE;

/*
** UT.h defines the maximum number of parameters any UTexe error has.
*/
static UTERR		UTeerr ZERO_FILL;

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
**       2-dec-1993 (smc)
**		Bug #58882
**		Made arglist parameters portable PTR to avoid pointer
**		truncations.
**  20-Aug-2003 (fanra01)
**      Add initialization of the the thread local store key and
**      creation of the context area.
**	05-Jan-2004 (hanje04)
**	    Typedef UTformArray to stop compilation errors on Linux concerning
**	    the ArrayOf(UTFORMAL) * in the declaration of UTescan.
**	14-Jan-2004 (hanje04)
**	    Change functions using old, single argument va_start (in 
**	    varargs.h), to use newer (ANSI) 2 argument version (in stdargs.h). 
**	    This is because the older version is no longer implemented in 
**	    version 3.3.1 of GCC.
*/
STATUS
UTexe(i4 mode, 
	LOCATION *err_log, 
	PTR dict, 
	PTR dsTab, 
	char *program, 
	CL_ERR_DESC *err_code, 
	char *arglist, 
	...)
{
	UT_COM		com;
	STATUS		status;
	SH_DESC		sh_desc;	/* First process file desc */
	SH_DESC		desc_ret;	/* Second process file desc */
	PTR		*argvect;	/* Hold parameter addresses */
	i4		wait;		/* wait for child process to execute? */
	FILE		*trcfile = NULL; /* for II_UT_TRACE output */
	i4		N;
	i4		i;
	va_list		args;
    UTCONTEXT*  utctx;

    /*
    ** If this is the first time through create a thread local store key.
    ** There is no initialization routine for this facility.
    */
    if (utinitialized == FALSE)
    {
        utinitialized = TRUE;
        status = MEtls_create( &uttlskey );
        if (status != OK)
        {
            return(status);
        }
    }
    /*
    ** Try and get the context for this thread.
    ** If there isn't one create one.
    */
    if ((status = MEtls_get( &uttlskey, (PTR*)&utctx )) == OK)
    {
        if (utctx == NULL)
        {
            utctx = (UTCONTEXT*)MEreqmem( 0, sizeof(UTCONTEXT), TRUE, &status );
            if (utctx == NULL)
                return(status);
            if ((status = MEtls_set( &uttlskey, (PTR)utctx )) == OK)
            {
                /*
                ** Assign a unique tag for this invocation
                */
                utctx->metag = MEgettag();
            }
        }
    }
	/*
	** Get the arguments from the variable-length list. The first one is
	** N, the number of arguments; the rest go into argvect
	*/
	va_start(args, arglist);
	N = va_arg(args, i4);
	argvect = (PTR *)MEreqmem(utctx->metag, sizeof(PTR) * N, FALSE, 
				  (STATUS *)NULL);
	for (i = 0; i < N; i++)
	{
		argvect[i] = va_arg(args, PTR);
	}
	va_end(args);

	CL_CLEAR_ERR( err_code );

	sh_desc.sh_type = 0;		/* initialize */
	sh_desc.sh_keep = (mode & UT_KEEP) ? KEEP : 0;
	desc_ret.sh_type = 0;
	desc_ret.sh_keep = (mode & UT_KEEP) ? KEEP : 0;
	desc_ret.sh_reg.file.name = NULL;
	if ((status = UTcomline(&sh_desc, &desc_ret, mode, dict, dsTab,
			program, arglist, &com, err_code, N, argvect)))
	{
		return status;
	}
	/* Is parent going to wait for child? */
	if (mode & UT_NOWAIT)
		wait = PC_NO_WAIT;
	else
		wait = PC_WAIT;
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
			
              	status  = PCdocmdline( (LOCATION *)NULL, com.result.comline,
                       	wait, TRUE, err_log, err_code);

		UTlogtrace(err_log, status); /* log output if II_UT_TRACE set */
	}
	else
	{
		/* call the module as a subroutine */
		status = (*com.module->modDesc->modFunc)(
				com.result.comargv->size,
				com.result.comargv->array);
	}

	MEtfree(utctx->metag);
    /*
    ** Initialize the thread local store entry.
    ** NB. This does not free the associated underlying thread local store
    ** entries.  There is a potential leak here.
    */
    MEtls_set( &uttlskey, NULL );
    /*
    ** Free the context
    */
    MEfree( (PTR)utctx );
	return  status;
}


/*
** UTcomline
**
** Build the command line to be executed or returned to
** routines above.
**
*/

/*VARARGS9*/
STATUS
UTcomline(sh_desc, desc_ret, mode, dict, dsTab, program, arglist, com, err_code,
	  N, argn)
	SH_DESC			*sh_desc;
	SH_DESC			*desc_ret;
	i4			mode;
	PTR			dict;
	PTR			dsTab;
	char			*program,
				*arglist;
	UT_COM			*com;
	CL_ERR_DESC		*err_code;
	i4			N;
	PTR			*argn;
{
	i4			i;
	STATUS			status;
	UTEACTUAL   actual[UTEARGS+1];
    UTCONTEXT*  utctx;

    if ((status = MEtls_get( &uttlskey, (PTR*)&utctx )) != OK)
    {
        return(status);
    }

	NMgtAt("II_UTEXE_DEBUG", &UTdebug);
	if (UTdebug != NULL && *UTdebug == '\0')
		UTdebug = NULL;

	utctx->Utestr = utctx->strings;
	utctx->Utescnt = UTESTR;
	utctx->Utelines = utctx->lines;
	utctx->Utelcnt = UTEARGS;
	utctx->Utecom = utctx->command;
	utctx->Utecombeg = utctx->command;
	utctx->Uteccnt = UTECOM;
	if ((status = UTeprog(program, dict, &com->module, err_code)))
		return status;
	if (isProcess = com->module->modType == UT_PROCESS) {
		utctx->Utecom = utctx->command;
		utctx->Utecombeg = utctx->command;
		utctx->Uteccnt = UTECOM;
	}
	else {
		utctx->comargv.size = 0;
		utctx->comargv.array = utctx->comarray;
		com->result.comargv = &utctx->comargv;
	}

	UTebinary(program, com, err_code);

	if (status = UTescan(mode, arglist, actual, (UTFormArray *)&com->module->modArgs,
				err_code, N, argn))
		return status;

#ifdef NT_GENERIC  /* (?:) operator doesn't work with ptrs */
	if (status = UTeapply(actual, N, dsTab, program,  sh_desc, desc_ret,
			NULL, err_code))
#else
	if (status = UTeapply(actual, N, dsTab, program,  sh_desc, desc_ret,
			isProcess? NULL: com, err_code))
#endif
		return status;

	if (isProcess) {
		/* encode the first desc */
		i = DSencode(sh_desc, utctx->Utecom, 'S');
		if (i > utctx->Uteccnt)
		{
			SETCLERR (err_code, 0, UTEBIGCOM);
			return (UTEBIGCOM);
		}
		utctx->Utecom += i;
		utctx->Uteccnt -= i;

		
		/* encode the second desc */
		i = DSencode(desc_ret, utctx->Utecom, 'R');
		if (i > utctx->Uteccnt)
			return UTEBIGCOM;
		utctx->Uteccnt -= i;

		com->result.comline = utctx->command;

		if (UTdebug != NULL)
		{
			SIprintf("\rCommand (length = %d) : %s\n",
				STlength(utctx->command), utctx->command);
			SIflush(stdout);
		}
	}
	else {	/* using comargv */
		if (UTdebug != NULL) {
			SIprintf("\rCommand argv (size = %d)\n",
				utctx->comargv.size);
			for (i=0; i < utctx->comargv.size; )
				SIprintf("\r%x\n",utctx->comargv.array[i++]);
			SIflush(stdout);
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
**	formal	array of formal arguments
**	N	number of actual arguments
**	argn	ptr array of actual arguments
**	err_code
**		Error descriptor
**
*/

static STATUS
UTescan(
	i4		mode,
	register char	*list,
	UTEACTUAL	*actual,
	UTFormArray	*formal,
	CL_ERR_DESC	*err_code,
	i4		N,
	PTR		*argn)
{
	register char		*argname;
	register UTEFORMAL	*fp;
	int			arglen;
	i4			i;
	i4			j;
	char			*save;
    STATUS      status;
    UTCONTEXT*  utctx;

    if ((status = MEtls_get( &uttlskey, (PTR*)&utctx )) != OK)
    {
        return(status);
    }

	for (i = 0; i < N && *list; i++, actual++, argn++) 
	{
		actual->ute_value = *argn;

		save = list;
		/*
		** Skip white space, get to the first character
		** of an argument name.
		*/
		while (CMwhite(list))
			CMnext(list);
		argname = list;
		while (!CMwhite(list) && *list != '=' && *list)
			CMnext(list);
		arglen = list - argname;

		/*
		** List now points to the first white space after the
		** argument name.  Scan to the EQUAL sign.
		*/
		while (CMwhite(list))
			CMnext(list);
		if (*list != '=')
		{
			SETCLERR (err_code, 0, UTENOEQUAL);
			return (UTENOEQUAL);
		}
		CMnext(list);

		/*
		** Look for the start of the argument spec.
		** It is marked with a %.
		*/
		while (CMwhite(list))
			CMnext(list);
		/*
		** Now it should be a %.
		*/
		if (*list != '%')
		{
			SETCLERR (err_code, 0, UTENOPERCENT);
			return (UTENOPERCENT);
		}
		CMnext(list);

		/*
		** This character is the spec.
		*/
		actual->ute_spec = *list;
		CMnext(list);
		if (actual->ute_spec == '\0')
		{
			SETCLERR (err_code, 0, UTENOSPEC);
			return (UTENOSPEC);
		}
		/*
		** Just scanned off actual argument and spec. Now look
		** for end of string or next comma. This gets string
		** set up for the next call of scan.
		*/
		while (CMwhite(list))
			CMnext(list);

		if (*list == ',') 
		{
			/* move past comma */
			CMnext(list);
		}
		else if (*list)
		{
			/* neither a comma nor null */
			SETCLERR (err_code, 0, UTEBADARG);
			return (UTEBADARG);
		}

		/* if actual arg is a shared data structure
		** get dsId from next arg
		*/
		if (actual->ute_spec == 'D')
			actual->ute_dsId = (i2)(SCALARP)*(++argn);
		else if (actual->ute_spec == 'R')
		{
			utctx->ret_addr = (i4 *) *argn;
			actual->ute_dsId = (i2)(SCALARP)*(++argn);
		}

		/*
		** Find the formal argument from the module
		** dictionary entry.
		*/
		for (j = 0; j < formal->size; j++)
		{
			fp = &formal->array[j];
			if (!STncasecmp(fp->ute_arg, argname, arglen ))
			{
				actual->ute_form = fp;
				goto nextactual;
			}
		}
		if (mode & UT_STRICT)
		{
			char	buf[13];
			u_i4	len;

			len = min(12, arglen);
			STncpy( buf, argname, len);
			argname[len]='\0';
			SETCLERR (err_code, 0, UTENOARG);
			return (UTENOARG);
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
**    16-apr-1999 (johco01)
**            Added check for the param argument for a report.
**            If on a UNIX platform ensure that single quotes are used
**            Otherwise make sure double quotes are used to delimit a
**            string being passed through the subsystem.
**    14-Jun-2000 (hanal04) Bug 101835 INGCBT 282
**            Restrict the above change to the REPORT case only as
**            ABF images expect "-a %S".
**    19-Oct-2000 (inifa01)
**            Further restrict above change to REPORT case with
**            parameters containing double quotes.  As reports
**            generated by ABF containing single quotes fail.
**    27-mar-2001 (rodjo04) bug 107117 ingcbt 395
**            Added logic to johcol01's change on 16-apr-1999, so that
**            we will check to see if the double quote is escaped. 
**    19-Apr-2001 (inifa01) bug 104514 ingcbt 347
**	      Pass 'program' and 'arg' parameter down to UTecom() and functions 
**	      which will call UTecom(), so that program and argument can be 
**	      determined at that point.
**    29-apr-2002 (rigka01) bug# 106776
**	    Backslashes in parameter values for report result in E_RW1001.	
**	    For report param on NT, UTecom will add a set of double quotes
**	    to the param value when the string contains at least one blank 
**	    and at least one backslash.  In this case, use (%S) instead of
**	    "(%S") in UTeapply() as the template.
**    12-nov-2002 (drivi01) Took out change made by johco01 on 16-apr-1999 to
** 	      give user control of utexe.def param argument for a report back.
**            To eliminate bug associated with his fix bug #97545, I edited
**	      utexe.def and for param argument put '(%S)'.
*/

static STATUS
UTeapply(actual, N, dsTab, program, sh_desc, desc_ret, com, err_code)
	UTEACTUAL		*actual;
	i4			N;
	PTR			dsTab;
	char			*program;
	SH_DESC			*sh_desc;
	SH_DESC			*desc_ret;
	UT_COM			*com;	/* the whoel shebang */
	CL_ERR_DESC		*err_code;
{
	register UTEACTUAL	*arg;
	register char	*line;
	register char	*ip;		/* points into line at '%' */
	STATUS		status = FALSE;
	int		i;							  
	bool		isProcess = (com == NULL);
	char   *param_string;
	bool    found_delim = FALSE;
	UTCONTEXT*  utctx;

    if ((status = MEtls_get( &uttlskey, (PTR*)&utctx )) != OK)
    {
        return(status);
    }

	for (arg = actual, i = 0; i < N; i++, arg++) 
	{
		/*
		** If the formal arg is NULL just skip the actual
		*/
		if (arg->ute_form == NULL)
			continue;
	
		/* Retain Conrads fix to bug 80869 but limit to report
                ** param containing double quotes.
                */
        /* Added logic to Conrads fix so that we ignore escaped double quotes*/
           
                if ((STcompare("param", arg->ute_form->ute_arg) == 0) &&
                    (STequal(program, ERx("report"))))
                {
                    if ((param_string = STindex((char *)arg->ute_value, "\"", STlength((char *)arg->ute_value))) != NULL)
                    {
                        do
                        {
                            param_string = CMprev(param_string, (char *)arg->ute_value);
                            if  (STbcompare("\\", 1, param_string, 1, 1) != 0  )
                            {
# ifdef UNIX
                                STcopy("'(%S)'",line);
# else
                                STcopy("\"(%S)\"",line);
# endif
                                found_delim = TRUE;
                                break;
                            }
                            else
                            {
                                param_string =  CMnext(param_string); 
                                param_string =  CMnext(param_string);
                            }
                        }while ( (param_string = STindex(param_string, "\"", STlength(param_string))) != NULL);
                      if (!found_delim)
                            line = arg->ute_form->ute_line;
                    }
                    else
                        line = arg->ute_form->ute_line;
                }
                else
#ifdef NT_GENERIC
                if ((STcompare("param",arg->ute_form->ute_arg) == 0) &&
                    (STequal(program, ERx("report"))))
                {
		/* Account for the check in UTecom() that adds double
		** quotes within parenthesis when param value contains at
		** least one embedded space and at least one backslash
	        ** by checking for the same condition here and using 
		** (%S) instead of "(%S)" as the template.
	        ** Result after UTecom will then be ("%S") instead of
		** "("%S")" and will avoid error E_RW1001.
		*/ 
			if (STindex( arg->ute_value, " ", 0 ) != NULL &&
			    STindex( arg->ute_value, "\\", 0 ) != NULL && 
			    STcompare("\"(%S)\"",arg->ute_form->ute_line) 
				== 0 )
			    /* outer double quotes already included */
                        	STcopy("(%S)",line);
			else
                        	line = arg->ute_form->ute_line;
		}
 		else 
#endif
                    line = arg->ute_form->ute_line;

		if (isProcess)
		{
			*utctx->Utecom++ = ' '; utctx->Uteccnt--;
		}

		while ((ip = STchr(line, '%')) != NULL)
		{
			if (isProcess) 
			{
				*ip = '\0';
				if (status = UTecom(line, err_code, program, arg))
					return status;
				*ip = '%';
			}
			line = ip+1;
			switch (*line++)
			{
			  case '%':
				if (isProcess) {
					status = UTecom("%", err_code, NULL, NULL); 
				}
				break;

			  case 'S':
				status = UTestr(arg, com, err_code, program); 
				break;

			  case 'N':
				status = UTenum(arg, com, err_code); 
				break;

			  case 'B':
				status = UTebreak(arg, com, err_code, program); 
				break;

			  case 'C':
				status = UTeexe("C", arg, com, err_code);
				break;

			  case 'E':
				status = UTeexe("E", arg, com, err_code);
				break;

			  default:
				SETCLERR (err_code, 0, UTEBADSPEC);
				return (UTEBADSPEC);
			}
			if (status != OK)
				 return status;
		}
		if (isProcess) 
		{
			if (status = UTecom(line, err_code, program, arg)) 
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
UTgetstr(arg, cp, err_code)
UTEACTUAL	*arg;
char		**cp;
CL_ERR_DESC	*err_code;
{
	char	b[80];
    STATUS  status;
    UTCONTEXT*  utctx;

    if ((status = MEtls_get( &uttlskey, (PTR*)&utctx )) != OK)
    {
        return(status);
    }

	switch (arg->ute_spec)
	{
	case 'S':
		*cp = (char *) arg->ute_value;
		break;
	case 'L':
		LOtos((LOCATION *) arg->ute_value, cp);
		break;
	case 'N':
		STprintf(b,"%d", (i4)(SCALARP) arg->ute_value);
		*cp = STtalloc(utctx->metag,b);
		break;
	case 'F':
		STprintf(b,"%f", *((f8 *) arg->ute_value));
		*cp = STtalloc(utctx->metag,b);
		break;
	default:
		b[0] = arg->ute_spec; b[1] = '\0';
		SETCLERR (err_code, 0, UTEBADTYPE);
		return (UTEBADTYPE);
	}
	return OK;
}

/*
** UTestr
**
** Insert a string into the command line or arg array.
*/

static STATUS
UTestr(arg, com, err_code , program)
	UTEACTUAL	*arg;
	UT_COM		*com;
	CL_ERR_DESC	*err_code;
	char		*program;
{
	char	*cp;
	int	i;
	i4	rval;

	if ((rval = UTgetstr(arg, &cp, err_code)) != OK)
		return rval;

	if (com == NULL) {
		return UTecom(cp, err_code, program, arg); 
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
**
** History:
**      15-Oct-97 (fanra01)
**          Modified for escaping embedded '"' characters on NT.
*/

static STATUS
UTebreak(arg, com, err_code, program)
	UTEACTUAL	*arg;
	UT_COM		*com;
	CL_ERR_DESC	*err_code;
	char		*program;
{
	char	*cp;
	char	tbuf[MAX_LOC];		/* temp buffer -- bug #6422 */
	char	buf[MAX_LOC];
	char	*words[20];
	char	**wp;
	i4	count;

	i4	rval;
        i4      dlm = FALSE;

	if ((rval = UTgetstr(arg, &cp, err_code)) != OK)
		return rval;
	if (STlength(cp) >= MAX_LOC)
	{
		SETCLERR (err_code, 0, UTEBIGCOM);
		return (UTEBIGCOM);
	}
	count = 20;
	STcopy(cp, tbuf);	/* use tbuf instead of cp -- bug #6422 */
	STgetwords(tbuf, &count, words);
	for (cp = buf, wp = words; count > 0; count--, wp++)
	{
		if (com == NULL) {
# ifdef NT_GENERIC
                char *  ptr = *wp;
                i4      i;
                i4      len = STlength(ptr);

                        if (dlm == FALSE)
                        {
                            CMcopy (ERx ("\""), 1, cp);
                            CMnext (cp);
                        }
                        for (i=0; (i < len); CMbyteinc(i, ptr), CMnext(ptr))
                        {
                            if (CMcmpcase(ptr,"\"") == 0)
                            {
                                CMcopy (ERx ("\\\""), 2, cp);
                                CMnext (cp);
                                CMnext (cp);
                                dlm = (dlm == FALSE) ? TRUE : FALSE;
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
# else
			*cp++ = '\"';
			STcopy(*wp, cp);
			cp += STlength(*wp);
			*cp++ = '\"';
			*cp++ = ' ';
# endif
		}
		else {
			com->result.comargv->array[com->result.comargv->size++] = (int *)*wp;
		}
	}

	if (com == NULL) {
		*cp = '\0';
		return UTecom(buf, err_code , program , arg); 
	}

	return OK;
}

/*
** UTenum
**
** Insert a number into the command line or command arg array.
*/

static STATUS
UTenum(arg, com, err_code)
	UTEACTUAL	*arg;
	UT_COM		*com;
	CL_ERR_DESC	*err_code;
{
	char	buf[30];
	int	i;


	if (com == NULL) {
		if (arg->ute_spec == 'N')
			STprintf(buf, "%d", (i4)(SCALARP) arg->ute_value);
		else if (arg->ute_spec == 'F')
			STprintf(buf, "%f", *((f8 *) arg->ute_value), '.');
		else if (arg->ute_spec == 'S')
			STcopy((char *) arg->ute_value, buf);
		else
		{
			buf[0] = arg->ute_spec; buf[1] = '\0';
			SETCLERR (err_code, 0, UTEBADTYPE);
			return (UTEBADTYPE);
		}
		return(UTecom(buf, err_code, NULL, arg)); 
	}
	else {
		i = arg->ute_form - com->module->modArgs.array;
		com->result.comargv->array[i + 1] = (i4 *)arg->ute_value;
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
UTeexe(spec, arg, com, err_code)
char		*spec;
UTEACTUAL	*arg;
UT_COM		*com;
CL_ERR_DESC	*err_code;
{
	char	*cp;
	STATUS	rval;

	arg->ute_spec = 'S';
	if ((rval = UTgetstr(arg, &cp, err_code)) != OK)
		return rval;
	arg->ute_spec = 'E';
	if (com == NULL)
	{
# ifdef VMS
		if (*spec == 'C')
		{
			if (UTegoodsym(cp) != OK)
			{
				SETCLERR (err_code, 0, UTENOSYM);
				return (UTENOSYM);
			}
		}
		else
		{
			UTedefsym("IIutexe", cp, err_code);
			cp = "IIutexe";
		}
# endif
		UTlatebin(cp, err_code);
	}
	else
	{
		SETCLERR (err_code, 0, UTELATEEXE);
		return (UTELATEEXE);
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
	UTEFORMAL	*arg;
	i4		cnt;
	i2		len;
	char		*sp;
	char		*cp = NULL;
	LOCATION	loc;
	char		buf[MAX_LOC];
	ModDict		*aModule;
	FUNC_EXTERN	ModDict	*UTetable();
	STATUS		status = OK;
    UTCONTEXT*  utctx;

    if ((status = MEtls_get( &uttlskey, (PTR*)&utctx )) != OK)
    {
        return(status);
    }

    arg = utctx->Utelines;
    cnt = utctx->Utelcnt;
    
	*theModule = NULL;

	/* module dictionary must be in a file */
	if (cp == NULL) {
		/* look for reset default */
		NMgtAt("II_UTEXE_DEF", &cp);
	}
	if (cp != NULL && *cp != '\0') {
		STlcopy(cp, buf, sizeof(buf)-1);
		DBGprintf("\rThe moduleDictionaryFile = %s\n", buf);
		LOfroms(FILENAME, buf, &loc);
		SIopen(&loc, "r", &fp);
	}
	else {
		LOCATION utloc;
		char	locbuf[MAX_LOC];
		char	*utfilename;

		status = UTdeffile(modname, locbuf, &utloc, err_code);

		if (status == OK)
		{
			/* Get UTexe definition file name */
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
	{
		SETCLERR (err_code, 0, UT_EXE_DEF);
		return (UT_EXE_DEF);
	}
	sp = utctx->Utestr;
	while (SIgetrec(sp, utctx->Utescnt, fp) == OK) {
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
		while (CMwhite(sp))
			CMnext(sp);
		cp = sp;
		while (!CMwhite(cp))
			CMnext(cp);
		*cp = '\0';
		CMnext(cp);
		while (CMwhite(cp))
			CMnext(cp);
		if (*cp == '\0') 
		{
			SIclose(fp);
			SETCLERR (err_code, 0, UTENOBIN);
			return (UTENOBIN);
		}
		
		if (STcasecmp(sp, modname ) == 0) {
			DBGprintf("\rHere I am having found the module entry\n");
			/* allocate a Module structure */
			*theModule = (ModDict *) MEreqmem(0, sizeof (ModDict),
				FALSE, (STATUS *) NULL);
			aModule = *theModule;
			aModule->modName = sp;
			aModule->modDesc = (ModDesc *) MEreqmem(0,
				sizeof (ModDesc), FALSE, (STATUS *) NULL);
			aModule->modDesc->modLoc = cp;
			aModule->modDesc->modFunc = NULL;

			while (!CMwhite(cp))
				CMnext(cp);
			*cp = '\0';
			CMnext(cp);
/* BEGIN 4.0 */
			/*
			** Just got the module name.  See if it is
			** %. If so then set modLoc to NULL to
			** signal that we don't have an executable yet.
			*/
			if (STcompare("%", aModule->modDesc->modLoc) == 0)
				aModule->modDesc->modLoc = NULL;
/* END 4.0 */

			/* skip to modType field and terminate string */
			while (CMwhite(cp))
				CMnext(cp);
			utctx->Utestr = cp;		/* temp. use of Utestr */
			CMnext(cp);
			while (!CMwhite(cp) && *cp != '\0')
				CMnext(cp);
			*cp = '\0';
			CMnext(cp);
			aModule->modType = UTemodtype(utctx->Utestr);
			if (aModule->modType == 0)
			{
				SETCLERR (err_code, 0, UTENoModuleType);
				return (UTENoModuleType);
			}
			aModule->modArgs.array = utctx->Utelines;
			utctx->Utestr = endOfLine;

			while (utctx->Utescnt > 0 && cnt > 0 &&
				!SIgetrec(utctx->Utestr, utctx->Utescnt, fp))
			{
				len = STlength(utctx->Utestr);
				if (utctx->Utestr[len - 1] == '\n')
					utctx->Utestr[len - 1] = '\0';
				len++;
				if (*utctx->Utestr != '\t')
					break;
				cp = utctx->Utestr+len;	/* new buffer */
				while (CMwhite(utctx->Utestr))
					CMnext(utctx->Utestr);
				arg->ute_arg = utctx->Utestr;
				while (!CMwhite(utctx->Utestr) && *utctx->Utestr != '\0')
					CMnext(utctx->Utestr);
				if (*utctx->Utestr == '\0')
					continue;
				*utctx->Utestr = '\0';
				for (CMnext(utctx->Utestr); CMwhite(utctx->Utestr); CMnext(utctx->Utestr))
					;
				arg->ute_line = utctx->Utestr;
				/*
				** blanks are now significant.  Look only
				** for tab delimeter.
				*/
				while (*utctx->Utestr != '\t' && *utctx->Utestr != '\0')
					CMnext(utctx->Utestr);
				*utctx->Utestr = '\0';
				utctx->Utestr = cp; utctx->Utescnt -= len;
				arg++; cnt--;
			}
			aModule->modArgs.size = arg - utctx->Utelines;
			arg->ute_arg = NULL;
			SIclose(fp);
			return OK;
		}
	}
	SIclose(fp);

	DBGprintf("\rHere I am returning NOPROG from UTeprog()\n");
	SETCLERR (err_code, 0, UTENOPROG);
	return (UTENOPROG);
}


/*
** UTebinary
**
** Place the command in the command string.
** The character pointer points to the name of the executable.
*/

static STATUS
UTebinary(prog, com, err_code)
char	    *prog;
UT_COM	    *com;
CL_ERR_DESC *err_code;
{
	ModDict			*mod = com->module;
	char			*cp;
	LOCATION		loc;
	int			ret;

	if (mod->modType == UT_PROCESS)
	{
/* BEGIN 4.0 */
		if (mod->modDesc->modLoc == NULL)
			return OK;
# ifdef VMS
		if (UTegoodsym(prog) != OK)
		{
			NMloc(BIN, FILENAME, mod->modDesc->modLoc, &loc);
			LOtos(&loc, &cp);
			if ((ret = UTedefsym(prog, cp, err_code)) != OK)
				return ret;
		}
		cp = prog;
# else
		NMloc(BIN, FILENAME, mod->modDesc->modLoc, &loc);
# ifdef conf_LSB_BUILD
		/* If it's not under $II_SYSTEM/ingres/bin, check /usr/bin */
		if ( LOexist(&loc) != OK )
		    NMloc(UBIN, FILENAME, mod->modDesc->modLoc, &loc);
# endif
		LOtos(&loc, &cp);
# endif /* VMS */
/* END 4.0 */
		ret = UTecom(cp, err_code, prog, NULL);
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

# ifdef VMS
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
*/
STATUS
UTedefsym(sym, exe, err_code)
char		*sym;
char		*exe;
CL_ERR_DESC	*err_code;
{
	char	buf[256];

	/* create < symbol :== $equivalent > in a buffer */
	if (STlength(sym) + STlength(exe) + STlength(" :== $") > 255)
	{
		SETCLERR (err_code, 0, UTEBIGCOM);
		return (UTEBIGCOM);
	}
	STpolycat(3, sym, " :== $", exe, buf);
	PCcmdline(0, buf, PC_WAIT, NULL);
	return OK;
}

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
# endif /* VMS */
/*
** Name:	UTecom	- append string argument to command line 
** 
** Description:
** 	append a string to the command-line buffer	
**
** Inputs:
**	buf 		-	buffer holding string value	
**	err_code	-	structure for CL error	
**
** Side Effects
**	none
**
** History
**	24-sep-1985 (joe)
**		First created
**  28-oct-1994 (canor01)
**      Unix command processor gets confused by single quotes when
**      they are embedded in a format enclosed in single quotes
**		(bug 57722)
**	2-feb-1996 (angusm)
**		Backed out above change, since it breaks passing of parameters
**		from ABF to RBF, VIFRED, REPORT etc. (bug 74091): can be problem 
**		for applications which worked under 6.4 but will not work because
**		of bug 74091. Fix for 57722 needs to be re-implemented.
**	9-feb-1996 (angusm)
**		Add version history for UTecom.
**		implement more restrictive version of original fix
**		for bug 57722: only handled embedded single quotes
**		if inside delimited identifier name. 
**		Logic is: 
**		- if we have an embedded quote within
**		a delimited identifier, then escape it for shell as per original
**		fix. 
**		- otherwise, use exising default processing. 
**		This fix does not provide for handling delimited ids 
**		containing quotes which are not in table names of the 
**		form [<schema>].<table> : 
**		e.g. in param names passed to report. This would require
**		rewriting of utexe.def.
**      14-Oct-97 (fanra01)
**              Modified the parsing of '"' delimited strings on NT.
**              Since the '"' character is treated as shell delimiter it needs
**              to be escaped.
**	17-nov-1997 (canor01)
**		If there are embedded spaces in the string on NT, then quote
**		the entire string.
**	27-jul-1999 (somsa01)
**	    Amended last change to not happen in the case where the string
**	    in question already contains a quote in the beginning. This
**	    means that somebody else already took the time to quote this
**	    string.
**  	8-Aug-2000 (rodjo04) Bug 102293 INGCBT 295
**     		Reworked logic so that if a table (in form user.tablename)
**      	has a single quote, escape it only on UNIX platform. Also to
**      	escape double quotes if on NT platform.
**      19-Oct-2000 (inifa01) bug 102986 INGCBT 311
**              ABF call report truncates first significant digit of money value
**              when report is generated. Passing (x='$12345.00') to report
**              generates 2345.00.
**              Fix was to insert the string to command line with dollar sign
**              escaped only on UNIX.
**	19-April-2001 (inifa01) bug 104514 INGCBT 347
**		Modify above fix, was causing handoffqa sep tests vis16 and vis44 
**		to fail.  Restrict fix to report param.
*/
static STATUS
UTecom( buf, err_code, program, arg)
char	    *buf;
CL_ERR_DESC *err_code;
char	    *program;
UTEACTUAL   *arg;
{
	i4	len;
        i4      dlmlen = 0;
        i4      tmplen = 0;
	i4	i, cnt;
	PTR	cp;
	bool	dlm = FALSE;
	bool	indlm = FALSE;
	bool	embquote = FALSE;
    STATUS  status;
    UTCONTEXT*  utctx;

    if ((status = MEtls_get( &uttlskey, (PTR*)&utctx )) != OK)
    {
        return(status);
    }

	if ((len = STlength(buf)) > utctx->Uteccnt)
	{
   		SETCLERR (err_code, 0, UTEBIGCOM);
		return (UTEBIGCOM);
	}

	cp = buf;
/*
**	DELIM ID:
** "table name" or "schema name"."table name"
**		OR
** "schema name".tbl
** schema_name."table"
** "delim user"
*/

	if (
	   ( (*cp == '"') && (*(cp+len-1) == '"') ) ||
	   ( (STchr(buf, '.') != NULL) && 
			((*cp == '"') || (*(cp+len-1) == '"') ) )
	   )
		dlm = TRUE;

	if (dlm)	
	{
		for (i=0, cnt=0; (i < len) && (dlm == FALSE || embquote == FALSE); i++)
		{
			if (*(cp+i) == '"')
			{
				if (indlm == FALSE)
					indlm = TRUE;
				else
					indlm = FALSE;
			}
			if ( (indlm == TRUE) && (*(cp+i) == '\'') )
				embquote=TRUE;
		}
	}
	
# ifdef NT_GENERIC
                /*
                ** If the string has a double quote delimiter, escape the
                ** quote and  enclose the whole string in double quotes.
                */
                if (dlm == TRUE)
                {
                char *ptr = utctx->Utecom;

                    for (indlm = FALSE, i=0; (i < len); CMbyteinc(i, ptr),
                         CMnext(ptr))
                    {
                        if (CMcmpcase((cp+i),"\"") == 0)
                        {
                            if (indlm == FALSE)
                            {
                                CMcopy (ERx ("\"\\\""), 3, ptr);
                                indlm = TRUE;
                            }
                            else
                            {
                                CMcopy (ERx ("\\\"\""), 3, ptr);

                                indlm = FALSE;
                            }
                            /*
                            **  3 characters appended but adjust for two
                            **  since original length already includes one.
                            */
                            dlmlen+=2;
                            CMnext (ptr);
                            CMnext (ptr);
                        }
                        else
                        {
                            CMcpychar((cp+i), ptr);
                        }
                    }
                }
                else
                {
		    char *tmpstr;

		    /* 
		    ** check buf for embedded spaces in pathnames
		    ** but ignore leading/trailing 
		    ** also ignore if it is a switch (begin with "-")
		    ** And, ignore if it begins with a '"' (which means
		    ** that somebody else took the time to quote this
		    ** string already).
		    */
		    tmpstr = STalloc( STskipblank(buf, STlength(buf)) );
		    if ( tmpstr != NULL )
		    {
		        STtrmwhite( tmpstr );

		        if ( *tmpstr != '-' && 
			     *tmpstr != '"' &&
			     STchr( tmpstr, ' ' ) != NULL &&
			     STchr( tmpstr, '\\' ) != NULL )
		        {
			    STpolycat( 3, "\"", buf, "\"", utctx->Utecom );
			    dlmlen += 2;
			}
		        else
		        {
                            STcopy(buf, utctx->Utecom);
		        }
		    }
		    MEfree(tmpstr);
                }
		
# else
if ((dlm == FALSE) || (embquote == FALSE))
        {
                /* default */
		/* Begin bug 102986 . UNIX bug (inifa01) */

                /* If buf contains $ as would precede a money value, escape $ */
		if(arg && program) /* Only escape '$' if report param */ 
		  if ((STcompare("param",arg->ute_form->ute_arg) == 0) &&
                    (STequal(program, ERx("report")))) 
                      if(STindex(buf, "$", len) != NULL)
                      {
                        char *cptr=utctx->Utecom;

                        for(i=0; (i < len) ; CMbyteinc(i,cptr ),CMnext(cptr))
                          if(CMcmpcase((cp+i),"$") == 0)
                          {
                              CMcopy(ERx("\\$"),3,cptr);
                              CMnext(cptr);
                              tmplen +=1;
                          }
                          else
                          {
                              CMcpychar((cp+i), cptr);
                          }
                      } /* End 102986 */
                      else /* Nothing inserted, then just copy */
                      	STcopy(buf, utctx->Utecom);
		   else 
                     STcopy(buf, utctx->Utecom);
                else
		  STcopy(buf, utctx->Utecom);  

		utctx->Utecom += (len + dlmlen + tmplen);
                utctx->Uteccnt -= len;
                return OK;
        }

	indlm = FALSE;

	for (cp=buf; *cp; cp++)
	{
		if (*cp == '"') 
		{
			if (indlm == FALSE)
				indlm = TRUE;
			else
				indlm = FALSE;
		}

		if ( (indlm == TRUE) && (*cp == '\'') )
		{
			if (len + 3 > utctx->Uteccnt)
			{
				SETCLERR(err_code, 0, UTEBIGCOM);
				return(UTEBIGCOM);
			}
			STcopy("'\\''", utctx->Utecom);
			len += 4;
			utctx->Utecom += 4;
		}
		else
		{
			*utctx->Utecom = *cp;
			utctx->Utecom++;
		}
	}
	utctx->Uteccnt -= len;
	return OK;
# endif /* NT_GENERIC */

	utctx->Utecom += len + dlmlen;
	utctx->Uteccnt -= len;
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
**	27-nov-2003  (rigka01) Problem INGDBA268, bug 111365
**		Add quotes to executable path+filename so that paths 
**		containing blanks and other special characters will
**		execute.
*/
STATUS
UTlatebin(cp, err_code)
char	    *cp;
CL_ERR_DESC *err_code;
{
	i4	len;
	char	*p1;
	char	*p2;
#ifdef NT_GENERIC
	char	*p3;
#endif
    STATUS  status;
    UTCONTEXT*  utctx;

    if ((status = MEtls_get( &uttlskey, (PTR*)&utctx )) != OK)
    {
        return(status);
    }

#ifdef NT_GENERIC
	/* for performance reasons, adding 3 instead of STlength(" \"\"")
	** will later add one blank and a total of two quotes 
	*/ 
	if ((len = STlength(cp) + 3 ) > utctx->Uteccnt)
#else
	if ((len = STlength(cp) + 1 ) > utctx->Uteccnt)
#endif
	{
		SETCLERR (err_code, 0, UTEBIGCOM);
		return (UTEBIGCOM);
	}

	/*
	** Since cp + a space will fit, it is safe to
	** move the current characters in Utecom down len
	** places.
	** p1 points len characters ahead in the buffer
	** p2 points at the current character to copy.
	*/
	for (p1 = utctx->Utecom + len - 1, p2 = utctx->Utecom - 1; p2 >= utctx->Utecombeg; p1--, p2--)
		*p1 = *p2;
	/*
	** Now copy buf in and replace p1 with a space.
	*/
#ifdef NT_GENERIC
	p3=utctx->Utecombeg;
	*(p3++)='\"';
	STcopy(cp, p3);
	p3=p1; 
	p3--;
	*p3='\"';
#else
	STcopy(cp, utctx->Utecombeg);
#endif
	*p1 = ' ';
	utctx->Utecom  += len;
	utctx->Uteccnt -= len;
	return OK;
}
