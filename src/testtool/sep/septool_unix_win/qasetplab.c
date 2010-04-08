/*
** setplab - set process label. This runs a command (by default csh)
** at a different label than the invoker.
**
** To install you need to do (SunOS CMW):
**  chpriv af=proc_setid,proc_setil,proc_setsl,sys_trans_label qasetplab
**
**
** Parameters:
**		qasetplab <label> [-u <user>] <command> [<args>...]
**
** History:
**	26-aug-1993 (robf,arc,cwaldman,pholman)
**		First created, for CMW.   No dount, will need to be
**		# ifdefed later.
**	26-aug-1993 (pholman)
**		Add  xCL_SUNOS_CMW and xCL_B1_SECURE ifdefs, to allow
**		compilation on non-secure machines.
**	27-aug-1993 (pholman)
**		Extend to allow -uflag, providing qasetuser functionality
**	08-dec-1993 (pholman)
**		Use SL routines instead of machine-specific code.
**	03-mar-1994 (pholman)
**		Upgrade to use new ES 1.1 SL.
**	15-mar-1994 (ajc)
**		Fixed typo.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**      ming hints
PROGRAM = qasetplab
**
NEEDLIBS = LIBINGRES
** 
MODE = SETUID
*/

# include <compat.h>
# include <clsecret.h>
# include <si.h>
# include <st.h>   
# include <pc.h>   
# include <lo.h>   
# include <er.h>   
# include <ctype.h>
# include <pwd.h>
# include <gl.h>
# include <sl.h>

#define	CMD_LINE_MAX	256

main(argc,argv) 
int	argc;
char	*argv[];
{
	i4		i,ret_val, err, thisarg;
	i4		ouid,uid  = -1;
	i4		status;
	char		cmd_line[CMD_LINE_MAX];
	struct passwd   *pass;
	LOCATION	*lo_null = NULL;
	CL_ERR_DESC	cl_err;
# ifdef xCL_B1_SECURE
	SL_LABEL	clearance;
	SL_EXTERNAL	e_label;

	if (argc <= 1) {
	    SIfprintf(stderr,ERx("qasetplab: No label/command supplied\n"));
	    SIfprintf(stderr,ERx("Syntax: qasetplab <label> [-u <user> ] <command>\n"));
	    PCexit(1);
	}		

	if (SLhaslabels()!=OK)
	{
	    SIfprintf(stderr,ERx("qasetplab: no security labels support\n"));
	    PCexit(1);
	}
	/*
	** Set the process label
	*/
	e_label.elabel   = argv[1];
	e_label.l_elabel = STlength(argv[1]);
	e_label.l_emax    = SL_MAX_EXTERNAL;

	if (SLinternal(&e_label, &clearance, &cl_err) != OK)
	{
	    SIfprintf(stderr,ERx("qasetplab: SLinternal fails for label \"%s\"\n"),
		argv[1]);
	    PCexit(1);
        }
	
	switch(SLsetplabel(&clearance, &cl_err))
	{
	    case OK:
		break;
	    case SL_LABEL_ERROR:
		SIfprintf(stderr, ERx("Invalid Label\n"));
	        exit(0);
	    case SL_NOT_SUPPORT:
		SIfprintf(stderr, ERx("SLsetplabel not implemented\n"));
		exit(0);
	    default:
		SIfprintf(stderr, ERx("SLsetplabel failed\n"));
		exit(0);
	}
	/*
	** Look for -u option
	*/
	thisarg = 2;

	if (!STcompare(argv[2], ERx("-u")))
	{
	    if (STcompare(argv[3], ERx("ingres"))  &&
                STcompare(argv[3], ERx("pvusr1"))  &&
                STcompare(argv[3], ERx("pvusr2"))  &&
                STcompare(argv[3], ERx("pvusr3"))  &&
                STcompare(argv[3], ERx("pvusr4"))  &&
                STcompare(argv[3], ERx("qatest"))  &&
                STcompare(argv[3], ERx("testenv")) &&
                STcompare(argv[3], ERx("notauth")) &&
                STcompare(argv[3], ERx("syscat")) &&
                STcompare(argv[3], ERx("super")) &&
                STcompare(argv[3], ERx("ctdba"))   &&
                STcompare(argv[3], ERx("release")))
            {
                SIfprintf(stderr,ERx("Not allowed to assume user id for: %s\n"),
			 argv[3]);
                exit(1);
             }
             pass = getpwnam(argv[3]);
             if(setuid(pass->pw_uid) == -1)
	     {
                 SIfprintf(stderr, 
			ERx("Could not set uid to %d\n"),pass->pw_uid);
                 exit(1);
            }
	    thisarg = 4;
	}
	/*
	execvp(argv[2],&argv[2]);
	*/
	for (i= 0; i < CMD_LINE_MAX; i++)
	    cmd_line[i] = '\0';

	for (i=thisarg; i <= (argc-1); i++)
            STprintf(cmd_line, ERx("%s %s"), cmd_line, argv[i]);

        ret_val = PCcmdline(NULL, cmd_line, PC_WAIT, lo_null, &cl_err);

# else
    SIfprintf(stderr, ERx("This is not a B1 system !\n"));
    exit(0);
# endif
	return ret_val;
}
