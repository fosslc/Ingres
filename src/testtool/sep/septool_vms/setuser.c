/*
**  Setuser.c
**
** Here is a fragment of code that implements a setuser command.  The function
** setuser get the PCB address from a process global symbol.  The main
** function calls the setuser function via kernel mode.  This requires VAXC
** 3.0 or above and the link command must include
** sys$system:sys.stb/sel,sys$system:sysdef.stb/sel
**
**  History:
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	11-jun-1992 (donj)
**	    Change logic to double check setting and resetting the username
**	    to handle the cases where VMS swapped us and we really didn't
**	    set or reset the username as we intended. Also add the restric-
**	    ions used in unix versions to only set to certain usernames.
**	12-jun-1992 (donj)
**	    Removed ability to just state username. Now this code works just
**	    as the UNIX version works. Both an authorized username AND a cmd
**	    line are mandatory. Other change; the SIprintf calls were not
**	    printing anything. Changed them to printf calls and they work.
**	    Possibly the pragma is screwing with SI. Since this is VMS specific
**	    I figure I can get away with it.
**	30-mar-1993 (donj)
**	    Added tracing features. Using new external routines for privileged
**	    functions.
**	17-dec-1993 (donj)
**	     Added more allowable usernames. Added a function to validate
**           username series (i.e. name0 thru name99).
**	 4-jan-1994 (donj)
**	    Actually I left the legal_unames() function out from the last
**	    change. This change I put it in.
**	13-mar-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from testtool code as the use is no longer allowed
**	30-jul-2007 (abbjo03)
**	    Allow user names like "tingresxx".
*/

#include    <compat.h>
#include    <er.h>
#include    <ex.h>
#include    <cm.h>
#include    <me.h>
#include    <nm.h>
#include    <si.h>
#include    <st.h>
#include    <pc.h>
#include    <lo.h>
#include    <ssdef.h>
#undef	    main

#define	    USERNAME_MAX    64
#define	    CMD_LINE_MAX    256

GLOBALDEF    i4            SEPtcsubs = 0 ;
GLOBALDEF    i4            tracing = 0 ;
GLOBALDEF    i4            trace_nr = 0 ;
GLOBALDEF    FILE         *traceptr ;
GLOBALDEF    char         *trace_file ;
GLOBALDEF    LOCATION      traceloc ;

GLOBALDEF    PID           SEPpid ;                 /* SEP's PID              */
GLOBALDEF    char          SEPpidstr [6] ;          /* SEP's PID as a string. */

FUNC_EXTERN  STATUS        SEP_Trace_Open () ;
FUNC_EXTERN  STATUS        SEP_Trace_Set () ;
FUNC_EXTERN  i4            setusr () ;
FUNC_EXTERN  char         *getusr () ;

bool    legal_unames ( char *uname, char *uname_root, i4 i_min, i4 i_max ) ;

char                       err_buffer [600] ;
i4                         mainstep ;
i4                         arg [4] ;

/*
**  exception handling
*/
    EX_CONTEXT             ex ;
    STATUS                 ex_val ;

/* routine for handling of exceptions */

static       EX            main_exhandler () ;

main(argc, argv)
i4   argc;
char *argv[];
{
    i4                      i ;
    i4                      nu ;
    i4                      ret_val ;

    LOCATION               *lo_null = NULL ;
    CL_ERR_DESC             cl_err ;

    static char            *present_user ;
    static char            *past_user ;
    static char            *future_user ;

    char                    cmd_line [CMD_LINE_MAX] ;
    char                   *cm_ptr ;
    char                   *name ;

    if (argc <= 1)
    {
	printf(ERx("\n Usage: Setuser <username> <command>\n\n"));
	PCexit (OK);
    }

    NMgtAt(ERx("SETUSER_TRACE"),&cm_ptr);
    if (cm_ptr != NULL && *cm_ptr != EOS)
    {
	mainstep = 1;
	PCpid(&SEPpid);
	CVla((SEPpid & 0x0ffff), SEPpidstr);
	SEP_Trace_Open();
	SEP_Trace_Set(ERx("ALL"));

	if (cm_ptr != NULL)
	    MEfree(cm_ptr);
    }

/*
** *************************************************************
**    setup exception handling
** *************************************************************
*/
    ex_val = OK;
    if (EXdeclare(main_exhandler, &ex) != OK)
    {
	ex_val = FAIL;
    }

    if (ex_val == OK)
    {
/*
** *************************************************************
**    MainLine Code
** *************************************************************
*/
	present_user = getusr();
	if (tracing)
	{
	    SIfprintf(traceptr,"Main%s(%02d)> After getusr(), present_user = %s\n",
		      SEPpidstr,mainstep++, present_user);
	    SIflush(traceptr);
	}
	future_user  = (char *)MEreqmem(0,USERNAME_MAX,TRUE,(STATUS *)NULL);
	past_user    = (char *)MEreqmem(0,USERNAME_MAX,TRUE,(STATUS *)NULL);

	for (name = argv[1]; *name != EOS; CMnext(name))
	    CMtoupper(name,name);

	STcopy(argv[1],future_user);
	if (tracing)
	{
	    SIfprintf(traceptr,"Main%s(%02d)> future_user = %s\n",
		      SEPpidstr,mainstep++,future_user);
	    SIflush(traceptr);
	}

        /*
        ** ********************************************************* **
        ** If you're adding new usernames, add single user names as
        ** STbcompare()'s; add multiple names like name0 thru name99 as
        **
        **  !legal_unames(future_user, ERx("name" ),  0, 99) &&
        **
        ** legal_unames() doesn't allow leading zeros such as name00
        ** thru name09 thru name99. To add those kinds, do the following:
        **
        **  !legal_unames(future_user, ERx("name0"),  0,  9) &&
        **  !legal_unames(future_user, ERx("name" ), 10, 99) &&
        **
        ** ********************************************************* **
        */

	if (STbcompare(present_user,0,ERx("donj"      ),0,TRUE)&&
	    STbcompare(future_user, 0,ERx("testenv"   ),0,TRUE)&&
	    STbcompare(future_user, 0,ERx("qatest"    ),0,TRUE)&&
	    STbcompare(future_user, 0,ERx("ingres"    ),0,TRUE)&&
	    STbcompare(future_user, 0,ERx("super"     ),0,TRUE)&&
	    STbcompare(future_user, 0,ERx("syscat"    ),0,TRUE)&&
	    STbcompare(future_user, 0,ERx("notauth"   ),0,TRUE)&&
	    STbcompare(future_user, 0,ERx("rplus_inst"),0,TRUE)&&
	    STbcompare(future_user, 0,ERx("stage_inst"),0,TRUE)&&
	    STbcompare(future_user, 0,ERx("release"   ),0,TRUE)&&
	    STbcompare(future_user, 0, ERx("tingres"), 7, TRUE) &&
	    !legal_unames(future_user, ERx("ingusr"),  0, 47) &&
            !legal_unames(future_user, ERx("pvusr" ),  1,  4))
	{
	    printf(ERx("\n The username, %s, is not authorized!\n\n"),future_user);
	    PCexit (OK);
	}
	STcopy(present_user,past_user);
	if (tracing)
	{
	    SIfprintf(traceptr,"Main%s(%02d)> past_user = %s\n",
		      SEPpidstr,mainstep++,past_user);
	    SIflush(traceptr);
	}

	if (tracing)
	{
	    SIfprintf(traceptr,"Main%s(%02d)> before setusr(%s)\n",
		      SEPpidstr,mainstep++,future_user);
	    SIflush(traceptr);
	}

	ret_val = setusr(future_user);
	if (tracing)
	{
	    SIfprintf(traceptr,"Main%s(%02d)> after setusr(%s)\n",
		      SEPpidstr,mainstep++,future_user);
	    SIflush(traceptr);
	}
	if (argc == 2) PCexit (OK);


	for (i=0; i < CMD_LINE_MAX; i++)
	    cmd_line[i] = '\0';

	STprintf(cmd_line, ERx("%s"), argv[2]);

	if (argc > 3)
	    for (i=3; i <= (argc-1); i++)
	    {
		for ( cm_ptr=argv[i], nu=0; (*cm_ptr) && (nu == 0);
		      cm_ptr=CMnext(cm_ptr))
		    if (CMupper(cm_ptr))
			nu=1;

		if (nu == 1)
		    STprintf(cmd_line, ERx("%s %c%s%c"), cmd_line, '"',argv[i],'"');
		else
		    STprintf(cmd_line, ERx("%s %s"), cmd_line, argv[i]);
	    }

	if (tracing)
	{
	    SIfprintf(traceptr,"Main%s(%02d)> Before PCcmdline(), cmd_line = %s\n",
		      SEPpidstr,mainstep++,cmd_line);
	    SIflush(traceptr);
	}
	ret_val = PCcmdline(NULL, cmd_line, PC_WAIT, lo_null, &cl_err);
    }

/* *******************************************************************
**		    Close up and set username back.
** *******************************************************************
*/
    EXinterrupt(EX_OFF);    /* disable exceptions while getting out of here */

    if (tracing)
    {
	SIfprintf(traceptr,"Main%s(%02d)> After  PCcmdline(), ret_val = %d, cl_err = %d\n",
		  SEPpidstr,mainstep++, ret_val, cl_err);
	SIfprintf(traceptr,"Main%s(%02d)> Before getusr(), present_user = %s\n",
		  SEPpidstr,mainstep++, present_user);
	SIflush(traceptr);
    }
    present_user = getusr();
    if (tracing)
    {
	SIfprintf(traceptr,"Main%s(%02d)> After  getusr(), present_user = %s\n",
		  SEPpidstr,mainstep++, present_user);
	SIfprintf(traceptr,"Main%s(%02d)> After  getusr(), past_user = %s\n",
		  SEPpidstr,mainstep++,    past_user);
	SIflush(traceptr);
    }

    ret_val = setusr(past_user);

    if (tracing)	    /* Close trace file if opened. */
    {
	SIclose(traceptr);
	tracing = FALSE;
    }

    EXdelete();
    PCexit (ret_val);
}
/* End of Main */

/*
**  main_exhandler
**
**  Description:
**  Exception handler for main routine
**
*/

static EX
main_exhandler(exargs)
EX_ARGS	*exargs;
{
    EX                     exno ;
    PID                    pid ;

    EXinterrupt(EX_OFF);    /* disable more interrupts */
    exno = EXmatch(exargs->exarg_num, 1, EXINTR);

    /* identify incoming interrupt */

    if (exno)
    {
	STcopy(ERx("ERROR: control-c was entered"), err_buffer);
    }
    else
    {
	STcopy(ERx("ERROR: unknown exception took place"), err_buffer);
    }

    if (tracing)
    {
	SIfprintf(traceptr,"Main%s(%02d)> %s\n",
		      SEPpidstr, mainstep++, err_buffer);
	SIflush(traceptr);
    }
    /*
    **	try to gracefully recover
    */

    return (EXDECLARE);    
}

bool
legal_unames ( char *uname, char *uname_root, i4  i_min, i4  i_max )
{
        char           *cptr1 = uname ;
        bool            ret_bool = FALSE ;
	i4              len_of_root ;
	i4              len_of_uname ;
	i4              i ;

	if ((len_of_uname = STlength(uname))
		> (len_of_root = STlength(uname_root)))
	{
	    for (i=0; i<len_of_root; i++)
		CMnext(cptr1);

	    if ((((len_of_uname - len_of_root) == 1) || (*cptr1 != '0'))
                && (CVan(cptr1,&i) == OK)
                && (( i >= i_min ) && ( i <= i_max )))
		ret_bool = TRUE;
	}
	return (ret_bool);
}
