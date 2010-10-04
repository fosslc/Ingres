/*
**	Copyright (c) 1990, 2008 Ingres Corporation
*/

# include	<compat.h>
# include       <cs.h>
# include	<gl.h>
# include	<er.h>
# include	<ex.h>
# include	<lo.h>
# include	<si.h>
# include	<cm.h>
# include	<st.h>
# include	<pc.h>
# include	<eventflag.h>
#include <signal.h>
# include 	<descrip.h>
# include 	<efndef.h>
# include 	<iosbdef.h>
# include 	<prvdef.h>
# include 	<iodef.h>
# include 	<ssdef.h>
# include       <rmsdef.h>
# include       <efndef.h>
# include       <iosbdef.h>
# include       <stsdef.h>
#ifdef OS_THREADS_USED
# include       <pthread.h>
# include       <assert.h>
# include       <fscndef.h>
# include       <iledef.h>
#endif
# include       <chfdef.h>
# include       <cma$def.h>
# include	<lib$routines.h>
# include	<starlet.h>


/**
** Name:	frontstart.c - VMS initialization routine
**
** Description:
**	This file does most of the work previously done by frontmain.mar.
**
**	IIcompatmain	Initialize before calling program-specific main routine
**	IItrap_ctrlc	Add out-of-band AST for ^C
**	IInotrap_ctrlc	Remove out-of-band AST for ^C
**	IIctrlc_is_trapped Is the out-of-band AST active
**
** History:
**	6/90 (Mike S)	Initial version
**	8/90 (Mike S)   Null-terminate argument list for compatibility with
**			old frontmain.mar
**	10/90 (Mike S)  Make public routines to get and give up the
**			^C-trapping AST
**	6/91 (Mike S)	Add IIctrlc_is_trapped so callers know whether to
**			disable and re-enable ^C trap.
**			Allow large (> 2000 byte) command lines to be passed
**			via the mailbox.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**      11-may-95 (whitfield)
**          Changed command line parsing so that quoted string does not
**          automatically force a new token.
**	10-jul-95 (albany)
**	    Added check for EXEXIT within exception handler; allows
**	    the signalling program to PCexit quietly and gracefully.
**	16-oct-98 (kinte01)
**	    bug 93193: Parsing of delimited identifiers was not correctly
**	    handling the trailing quote. For example qbf -u"""seLEct"""
**	    would work but qbf -u"""seLEct""" -t t65_1 would fail.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**      21-Jun-2006 (ashco01)
**          Bug 114634 - replaced lib$signal() call in ctrlc_handler() 
**          with EXsignal() as lib$signal() failing to fire except_handler() 
**          resulting in termination of terminal monitor when CTRL-C 
**          entered. Suspect OS or compiler fault, reported to HP.
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      30-Jan-2009 (stegr01)
**          If we're running under the control of the debugger then don't
**          trap control-c's. Remove superfluous definitions of IO$M_ masks
**      25-jun-2009 (stegr01)
**          Under some weird circumstances the pthread manager will deliver
**          a CMA$_EXIT_THREAD exception to the primary thread - so allow
**          the except_handler to ignore this exception
**      26-jun-2009 (stegr01)
**          reinstate code to make main thread a secondary thread in order
**          to fix some weird Posix thread rundown problem where the
**          exit handler thread (thread -3) seems to get deadlocked with the
**          primary thread (thread 1) after calling sys$exit()
**      12-aug-2010 (joea)
**          Use VAXC$ESTABLISH to set up exception handlers.
**/

/* # define's */
# define ARGS_SIZE	2049		/* Maximum argument length */
# define SLASH		ERx("/")
# define EXTRA		ERx("/EXTRA=")

/* Record size of "extra" mailbox.  This must match the value in PCcmdline.c */
# define DCLCOMLINE	128		


typedef  struct dsc$descriptor_s DESCRIP_S;

/* GLOBALDEF's */

/* extern's */
FUNC_EXTERN i4 IIMISCsysin(), IIMISCinredirect(), IIMISCsysout();
FUNC_EXTERN i4 IIMISCoutredirect(), IIMISCsyserr(), IIMISCcommandline();
GLOBALREF   i4 iicl__feerror, iicl__fertnerror;

FUNC_EXTERN VOID	IItrap_ctrlc();
FUNC_EXTERN bool	EXsigarr_sys_report();
/* static's */
static VOID	check_redirects();
static VOID	ctrlc_handler();
static i4 	except_handler();
static VOID	get_extra();
static VOID	make_words();
static VOID  	open_SIfiles();


static $DESCRIPTOR(tt, ERx("TT"));
static i4 ctrlc_mask[2] = { 0, 1<<3 };

static i4 msgvec[2] = { 1, SS$_ABORT };	/* Generic "abort" message vector */

static i4 sysprv_mask[2] = { PRV$M_SYSPRV, 0 };

static const char sysin[] = ERx("SYS$INPUT");
static const char sysout[] = ERx("SYS$OUTPUT");
static const char syserr[] = ERx("SYS$ERROR");

static bool trapping_ctrlc = FALSE;

typedef struct
{
	char  	*default_name;		/* Default name for file */
	char 	*name;			/* Current name for file */
	char  	*mode;			/* File open mode */
	i4	(*default_err)();	/* Report error in default open */
	i4	(*redirect_err)();	/* Report error in redirected open */
} STDFILE;

static STDFILE stdfiles[3] = 
{ 
    { sysin, sysin, ERx("r"), IIMISCsysin, IIMISCinredirect },
    { sysout, sysout, ERx("w"), IIMISCsysout, IIMISCoutredirect },
    { syserr, syserr, ERx("w"), IIMISCsyserr, IIMISCsyserr }
};

#if defined(OS_THREADS_USED)
typedef struct __main_args_wrapper
{
    struct dsc$descriptor*  ifd_file;
    i4 (*main_rtn)();
} MAIN_ARGS_WRAPPER;

static MAIN_ARGS_WRAPPER main_arglst;

#define MAIN_THREAD_STACKSIZE 600000

VOID IIcompatmain_pthread(struct dsc$descriptor *ifd_file, i4 (*main_rtn)());
static VOID create_compatmain_thread (pthread_t* compatmain_tid, struct dsc$descriptor *ifd_file, i4 (*main_rtn)());
static VOID *main_jacket(void *arg);
static VOID set_primary_thread_name (pthread_t tid, struct dsc$descriptor *ifd_file);
#endif



static VOID __IIcompatmain(struct dsc$descriptor *ifd_file, i4 (*main_rtn)());

/*{
** Name:  IIcompatmain  Initialize before calling program-specific main routine
**
** Description:
**  Provide environment for an Ingres front-end program or ABF application
**  to run on VMS.
**
** Inputs:
**  ifd_file  struct dsc$descriptor*   File name from image file descriptor
**  main_rtn  i4 ()            The program-specific main to call
**
** Outputs:
**  None
**
**  Returns:
**      None.
**
**  Exceptions:
**      None.
**
** Side Effects:
**
** History:
**  6/90 (Mike S)   Initial Version
*/
VOID
IIcompatmain(struct dsc$descriptor *ifd_file, i4 (*main_rtn)())
{
#if defined(OS_THREADS_USED)

    /*
    ** Set the main thread name
    */

    set_primary_thread_name(pthread_self(), ifd_file);


    /*
    ** call the main function
    */

    IIcompatmain_pthread (ifd_file, main_rtn);
#else
     __IIcompatmain(ifd_file, main_rtn);
#endif
}


#if defined(OS_THREADS_USED)

VOID
IIcompatmain_pthread(struct dsc$descriptor *ifd_file, i4 (*main_rtn)())
{
    i4 main_sts;

    /*
    ** Create the "main" thread that will execute the real main()
    */

    pthread_t compatmain_tid;

    create_compatmain_thread (&compatmain_tid, ifd_file, main_rtn);


    /*
    ** wait for the main thread to complete
    ** (if it hasn't already called sys$exit()
    */

    pthread_join(compatmain_tid, &main_sts);

    sys$exit(main_sts);
}


static VOID 
set_primary_thread_name (pthread_t tid, struct dsc$descriptor *ifd_file)
{
    /* set the thread name for debug purposes */

    i4 sts;
    ILE2 itmlst[] = {{0, FSCN$_NAME, NULL},
                     {0, 0, NULL}};

    char main_name[256];
    sts = sys$filescan (ifd_file, itmlst, NULL, NULL, NULL);
    if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
    strncpy (main_name, (char *)itmlst[0].ile2$ps_bufaddr, itmlst[0].ile2$w_length);
    main_name[itmlst[0].ile2$w_length] = 0;

    assert (pthread_setname_np(tid, main_name, NULL) == 0);
}


static VOID 
create_compatmain_thread (pthread_t* compatmain_tid, struct dsc$descriptor *ifd_file, i4 (*main_rtn)())
{

    /* create a thread whose start rtn (indirectly) is __IIcompatmain */

    pthread_attr_t attr;


    assert (pthread_attr_init (&attr) == 0);


    /* make sure we can JOIN to this thread */

    assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) == 0);


    assert (pthread_attr_setstacksize(&attr, MAIN_THREAD_STACKSIZE) == 0);

    main_arglst.ifd_file = ifd_file;
    main_arglst.main_rtn = main_rtn;


    assert (pthread_create (compatmain_tid, /* where to park the new thread's tid */
                            &attr,          /* thread attribute block */
                            main_jacket,    /* thread start rtn */
                            &main_arglst) == 0);   /* thread start arg */


    assert (pthread_attr_destroy (&attr) == 0);
}


/*
** Thread start routine
** Unpack the arguments and call the real main
**
*/


static VOID*
main_jacket(void *arg)
{
    MAIN_ARGS_WRAPPER* args_wrapper = (MAIN_ARGS_WRAPPER *)arg;
    __IIcompatmain(args_wrapper->ifd_file, args_wrapper->main_rtn);
}

#endif /* OS_THREADS_USED */

VOID
__IIcompatmain(ifd_file, main_rtn)
struct dsc$descriptor *ifd_file;
i4	(*main_rtn)();
{
	char 	image_name[256];/* Image file name */
	u_i2	input_length;	/* Size of command line */	
	char	*slash;
	i4	status;

	static char input_args[ARGS_SIZE+DCLCOMLINE];/* Unprocessed arguments */
	static 	$DESCRIPTOR(input_desc, input_args);

	char 	output_args[ARGS_SIZE];		/* Processed arguments */
	i4 	argc;		/* Number of arguments for main_rtn */
	char 	*argv[64];	/* arguments for main_rtn */

#if defined(axm_vms)
        DECC$CRTL_INIT();
#endif
	/* The first argument is the name from the IFD */
	STcopy(ifd_file->dsc$a_pointer, image_name);
	image_name[ifd_file->dsc$w_length] = EOS;
	argc = 1;
	argv[0] = image_name;	

	/* Get command line from LIB$GET_FOREIGN */
	status = lib$get_foreign(&input_desc, 0, &input_length);
	input_args[input_length] = EOS;		/* Null-terminate it */

	/* 
	** See if "/EXTRA=device" is the last part of the command.  If so, we 
	** must read the device for further command input.
	*/
	slash = STrindex(input_args, ERx("/"), 0);
	if (slash != NULL)
	{
	    if (0 == STbcompare(slash, 0, EXTRA, STlength(EXTRA), TRUE))
	    {
	    	get_extra(input_args, slash);
	    }
	}

	/* Break command into words */
	make_words(input_args, output_args, &argc, argv);

	/* Check for redirection */
	check_redirects(&argc, argv);
	argv[argc] = NULL;		/* Null-terminate arg list */

	/* Open standard SI files */
	open_SIfiles();

	/* Establish exception hander */
	VAXC$ESTABLISH(except_handler);	

	/* Establish control-c exception handler */

        IItrap_ctrlc();

	/* Call the user's program, and exit with its status */
	PCexit( (*main_rtn)(argc, argv) );
}

static u_i2	tt_chan = 0;

/*
** Clear the "interrupt signaled" event flag, and establish the 
** out-of-band AST.
*/
VOID 
IItrap_ctrlc()
{
	i4 status;
	IOSB	iosb;

	sys$clref(INTR_SIG_EF);

	status = 1;
	if (tt_chan == 0)
		status = sys$assign(&tt, &tt_chan, 0, 0);
	if ((status & 1) != 0)
	{
	    status = sys$qiow(EFN$C_ENF, tt_chan, 
	    		      IO$_SETMODE | IO$M_OUTBAND | IO$M_TT_ABORT,
	    		      &iosb, 0, 0,
	    		      ctrlc_handler,  ctrlc_mask, 0, 0, 0, 0);
	}

	trapping_ctrlc = TRUE;
}

/*
** Remove the ctrlc out-of-band AST
*/
VOID
IInotrap_ctrlc()
{
	i4 status;
	IOSB	iosb;

	status = 1;
	if (tt_chan == 0)
		status = sys$assign(&tt, &tt_chan, 0, 0);
	if ((status & 1) != 0)
	{
	    status = sys$qiow(EFN$C_ENF, tt_chan, 
	    		      IO$_SETMODE | IO$M_OUTBAND | IO$M_TT_ABORT,
	    		      &iosb, 0, 0,
	    		      0,  ctrlc_mask, 0, 0, 0, 0);
	}

	trapping_ctrlc = FALSE;
}

/* 
** 	Are we currently trapping ^C ?
*/
bool
IIctrlc_is_trapped()
{
	return trapping_ctrlc;
}


/*
**	Control-C handler.
**
**	On ctrl-c, clear the interrupt event flag, and signal an interrupt.
*/
static VOID
ctrlc_handler()
{
	sys$clref(INTR_SIG_EF);

        /* NOTE: B114634.
        ** lib$signal(SS$_CONTROLC) call failing to fire except_handler() 
        ** on VMS 7.3, replaced this with call to EXsignal() - ashco01 */
        EXsignal(SS$_CONTROLC, 0);
}


/*
** make_words
**
** Break command line up into words, using these rules:
**
** A word is a string of non-whitespace characters
** OR 
** A word is a parenthesis-delimited string.
** OR
** A word is a '"'-quoted string.  Inside a '"'-quoted string, a quote is
** represented by a doubled '"'-quote.  If in a parenthesized string, we
** translate the doubled '"'-quote to a backslash-escaped '"'-quote.  Else,
** we translate the doubled '"'-quote to a single '"'-quote.
**
** Parenthesized strings are not recognized within a quoted string.
**
** Except in a quoted string, all UPPERCASE characters are changed to
** lowercase.  The delimiting quotes of a quoted string are deleted, except
** inside parentheses.
*/

# define OUT_WORD 	0	/* Outside of a word */
# define IN_WORD 	1	/* Inside a word */
# define IN_QUOTE	3	/* Inside a quoted word */
# define IN_PAREN	5	/* Inside a parenthesized word */
# define IN_PAREN_QUOTE 7	/* Inside a quoted string in a paren'ed word */

static VOID
make_words(in, out, argc, argv)
char	*in;		/* Input line */
char	*out;		/* output line */
i4	*argc;		/* Number of words found (output) */
char	*argv[];	/* Pointers to words found (output) */
{
	char *next_in;
	i4 state = OUT_WORD;

 	for (next_in = in + CMbytecnt(in); 
	     *in != EOS; 
	     in  = next_in, next_in = in + CMbytecnt(in))
	{
		switch (state)
		{
		    case OUT_WORD:
		    	/* Outside a word */

			if (CMwhite(in))	/* Skip whitespace */
			    break;
                        
			argv[(*argc)++] = out;	/* Begin next word */

			if ('"' == *in)				
			{
			    state = IN_QUOTE;	/* In quoted word */
			    break;
			}
			
	                if ('(' == *in)        	/* In parenthesized word */
				state = IN_PAREN;
   			else
				state = IN_WORD; /* In ordinary word */

			CMcpychar(in, out);	/* Copy first char of word */
			CMtolower(out, out);	/* Lowercase it */
			CMnext(out);
			break;

		    case IN_WORD:
		    	/* Inside an ordinary word */
			
			if (CMwhite(in))
			{
			    *out++ = EOS;	/* End word */
			    state = OUT_WORD;
			}
			else if ('"' == *in)
			{
                            state = IN_QUOTE;   /* In quoted word */
			}
			else
			{
			    CMcpychar(in, out);	/* Copy first char of word */
			    CMtolower(out, out);/* Lowercase it */
			    CMnext(out);
			}			    			    
			break;

		    case IN_QUOTE:
	    		/* Inside a quoted word */
							
			if ('"' == *in)
			{
			    if ('"' == *next_in)
			    {
			    	STcopy(ERx("\""), out);  /* Escaped quote */
			    	out += STlength(out);
			    	CMnext(next_in);
				/* handle trailing quote properly */
				if ('"' == *next_in)
				{
				   CMnext(next_in);
				   if (' ' == *next_in)
				   {
				      *out++ = EOS;   /* End word */
				      state = OUT_WORD;
				   }
				}
			    }
			    else
			    {
				*out++ = EOS;	/* End word */
				state = OUT_WORD;
			    }
			}
			else
			{
			    CMcpychar(in, out);	/* Copy character */
			    CMnext(out);
			}                     
		        break;

		    case IN_PAREN_QUOTE:
	    		/* Inside a quoted word */
							
			if ('"' == *in)
			{
			    if ('"' == *next_in)
			    {
			    	STcopy(ERx("\\\""), out);  /* Escaped quote */
			    	out += STlength(out);
			    	CMnext(next_in);
			    }
			    else
			    {
				CMcpychar(in, out);
				CMnext(out);
				state = IN_PAREN;
			    }
			}
			else
			{
			    CMcpychar(in, out);	/* Copy character */
			    CMnext(out);
			}                     
		        break;

		    case IN_PAREN:

			/* Inside a parenthesized word */
			
			CMcpychar(in, out);	
			CMtolower(out, out);
			CMnext(out);

			if (')' == *in)
			{
			    *out++ = EOS;		/* End word */
			    state = OUT_WORD;
			    break;
			}
			else if ('"' == *in)
			{
			    state = IN_PAREN_QUOTE;
			}
			break;
		}
	    }
	
	    /* Close anything that's open */
	    if (IN_PAREN_QUOTE == state)
	    	*out++ = '"';
	    if (IN_PAREN == state || IN_PAREN_QUOTE == state)
	    	*out++ = ')';
	    *out = EOS;
}	    

/*
** get_extra
**
** Get the extra parameters from the mailbox.  This occurs when PCcmdut
** sends a command which is longer than the maximum DCL length.
*/
static VOID
get_extra(input_args, slash)
char *input_args;	/* Beginning of input arguments */
char *slash;		/* Pointer to the "/" in the "/EXTRA=device" */
{
	LOCATION loc;
	char	lbuf[MAX_LOC+1];
	FILE 	*file;
	i4	count;
	STATUS 	status;

	/* Open mailbox */
	STcopy(slash + STlength(EXTRA), lbuf);
	status = LOfroms(PATH&FILENAME, lbuf, &loc);
	if (OK == status)
		status = SIfopen(&loc, ERx("r"), SI_TXT, 0, &file);
	if (OK != status)
	{
		status = IIMISCcommandline();
		PCexit(status);
	}

	/* Read from mailbox */
	for ( ; ; )
	{
	    status = SIread(file, (i4)DCLCOMLINE, &count, slash);	
	    if (OK != status)
	    	break;

	    slash += count;	
	    *slash = EOS;
	    if ((slash - input_args) >= ARGS_SIZE)
	    {
	    	/* Too big a command line; exit with an error */
	    	PCexit(RMS$_TNS);
	    }
	    
	    /* If the last character read was a NULL, we're done */
	    if (EOS == *(slash-1))
	    	break;
	}

	SIclose(file);
}

/*
** Exception handler.  If an exception gets this far, we'll issue an abort
** message, followed by the exception message, and then exit.
*/
static 
i4 except_handler(signal_array, mechanism_array)
i4 	signal_array[];
i4 	mechanism_array[];
{
	i4 condition = signal_array[1];
        char sysrepbuf[1000];
	bool syserror;

        if (condition == CMA$_EXIT_THREAD)
        {
            return (SS$_RESIGNAL);
        }
        else if (condition == SS$_UNWIND)
        {
            return (SS$_UNWIND);
        }
        else
	{
	    if (SS$_CONTROLC == condition)
		signal_array[0] = 1;	/* This exception takes no parameters */

	    if ( EXEXIT == condition )
		PCexit( OK );		/* We just want to exit quietly */

	    sys$putmsg(msgvec, 0, 0, 0);      	/* Our generic ABORT message */
	    syserror = EXsigarr_sys_report(signal_array, sysrepbuf);
	    if (syserror)
	    {
	    	/* It's a VMS error */
	    	SIfprintf(stderr, "%s\n", sysrepbuf);
	    }
	    else
	    {
    		i4 fe_errorvec[5];
    		$DESCRIPTOR(rtn_desc, ERx(""));
			
    		/* If it's EXFEBUG or EXFEMEM, we may have a routine name */
    		if ((condition == EXFEBUG || condition == EXFEMEM) &&
    		    signal_array[0] >= 4)
    		{
    		    /* We have a routine name for the message */
    		    fe_errorvec[0] = 4;		
    		    fe_errorvec[1] = (i4)&iicl__fertnerror;
    		    fe_errorvec[2] = 0x0F02;
    		    rtn_desc.dsc$a_pointer = (char *)signal_array[2];
    		    rtn_desc.dsc$w_length = STlength(signal_array[2]);
    		    fe_errorvec[4] = (i4)&rtn_desc;
    		}    		        		    
    		else
    		{
    		    /* We have only the error code */
    		    fe_errorvec[0] = 3;		
    		    fe_errorvec[1] = (i4)&iicl__feerror;
    		    fe_errorvec[2] = 0x0F01;
    		}    		        		    
		fe_errorvec[3] = condition;
		sys$putmsg(fe_errorvec, 0, 0, 0);
	    }
    	    PCexit(SS$_ABORT);			/* And exit */
	}
}

/*
** Open the stdin, stdout, and stderr files.  By default, these are opened
** on SYS$INPUT, SYS$OUTPUT, and SYS$ERROR; the first two may be redirected
** on the command line.  If anything fails, we exit.
*/
static VOID 
open_SIfiles()
{
    LOCATION 	loc;
    char 	lbuf[MAX_LOC+1];
    i4 		prvmask[2];
    STATUS 	status;
    FILE 	*file;
    i4		i;
    STDFILE	*sfile;

    /* First, as a precaution, disable sysprv */
    sys$setprv(0, sysprv_mask, 0, prvmask);

    /* Try to open all files */
    for (i = 0; i < 3; i++)    	
    {
    	sfile = stdfiles + i;
	STcopy(sfile->name, lbuf);
	status = LOfroms(FILENAME&PATH, lbuf, &loc);
	if (OK == status)
		status = SIfopen(&loc, sfile->mode, SI_TXT, 0, &file);

	    if (OK != status)
	    {
		/* Issue message */
		if (sfile->name == sfile->default_name)
		    status = (*sfile->default_err)();
		else
		    status = (*sfile->redirect_err)();

		PCexit(status);	/* Exit */
	    }
	}

	/* Re-enable sysprv, if it was enabled */
	if ((prvmask[0] & PRV$M_SYSPRV) != 0)
	    sys$setprv(1, sysprv_mask, 0, 0);
}

/*
**	check_redirects
**
**	Check for I/O redirection on the command line.
*/
static VOID
check_redirects(argc, argv)
i4 *argc;
char *argv[];
{
	i4 i;
	i4 words = *argc;
	i4 numargs = 1;

	for (i = 1; i < words; i++)
	{
	    if ('<' == argv[i][0])
	    	stdfiles[0].name = argv[i] + 1;		/* Input redirection */
	    else if ('>' == argv[i][0])
	    	stdfiles[1].name = argv[i] + 1;		/* Output redirection */
	    else
	    	argv[numargs++] = argv[i];		/* A real parameter */
	}

	*argc = numargs;
}	    
