/*
**Copyright (c) 2009 Ingres Corporation
**
*/
/*
*	19-aug-1990 (owen)
*		Start history for this file.
*		Move from septool to septool_unix directory.
*	24-aug-1990 (owen)
*		Restrict valid users. Use SIfprintf,
*	17-jul-1991 (donj)
*		Change last printf to SIfprintf, even though its
*		in a comment. Take out include of stdio.h and put
*		in include of si.h.
*	25-jul-1991 (donj)
*		Add pvusr1 thru pvusr3 to test for authorized user
*		names.
*	15-Aug-1991 (donj)
*		Substitute PCcmdline for call to execvp. Add in all
*		the stuff needed, LOCATION and CL_ERR_DESC.
*	20-dec-1991 (lauraw)
*		Added pvusr4 to authorized user names.
*	08-jan-1992 (lauraw)
*		Put in a proper return statement.
*	10-jan-1992 (donj)
*		include er.h and wrap string constants with ERx()
*		macro.
*	11-jun-1992 (donj)
*		change NEEDLIBS fm COMPAT to LIBINGRES
*	14-jun-1992 (donj)
*		added user, ctdba, to authorized user list.
*	11-jan-1993 (dianeh)
*		added notauth, syscat, and super to authorized user list.
*	22-jan-1993 (dianeh)
*		looks like I managed to pick&put too many lines up here
*		and ended up with duplicating the ming hints and #includes
*		by mistake (must have hit 22y instead of 2y -- oops).
**	21-Oct-93 (GordonW)
**		Try to bypass PCcmd_line usage, that startups a
**		'/bin/sh program' which is one too many processes.
**	26-jun-1994 (pholman)
**		On Solaris, we lack strings.h, and must use alternatives
**		to bzero etc.
**	25-Oct-94 (GoprdonW)
**		upgrade to 6.4 logic, not use a shell.
**	12-May-95 (smiba01)
**		Add support for several SVR4 platforms
**		(dr6_us5, usl_us5, dr6_ues) as well as Solaris.
**	15-jun-95 (popri01)
**		Extend above support to nc4_us5.
**	18-aug-95 (rambe99)
**		Extend above support to sqs_ptx. 
**              Compensate for absence of <strings.h>.
**      20-jun-95 (pursch)
**              pym_us5: same as su4_us5: no <strings.h>
**	29-Nov-1995 (murf)
**		Added Solais for Intel support (sui_us5).
**		Extend above support to nc4_us5
**	11-oct-1995 (somsa01)
**		Ported to NT_GENERIC platforms.
**	21-feb-1996 (morayf)
**		Added rmx_us5 to those using memcpy, memset, strchr
**		for SNI RMx00 port.
**	10-jul-1996 (somsa01)
**		Added notauth, syscat, super, ctdba, and release users to NT
**		version.
**	01-Jan-1997 (merja01)
**	    The value of LD_LIBRARY_PATH is lost when a setuid is 
**	    done on axp.osf.  If the II_SYSTEM variable is set
**	    I set this environment variable to
**	    /lib:$II_SYSTEM/ingres/lib on axp.osf.
**	19-jan-1998 (somsa01)
**		Added devtest as a user, since it is used on NT.
**	09-mar-1998 (toumi01)
**		Similar to axp.osf, save LD_LIBRARY_PATH for Linux (lnx.us5).
**		Different routine is needed because of addition of /usr/lib
**		and problem with putenv with malloc'd sprintf buffer.
**	26-mar-1998 (kosma01 for bobmart)
**		Nicked merja01 logic for ris_us5/rs4_us5 (stuffing II_SYSTEM)
**		into LIBPATH (ris equiv. of LD_LIBRARY_PATH).
**	18-may-1998 (somsa01)
**		X-integration of change #433877 (somsa01) from main; addition
**		of "devtest" user.
**	31-jul-1998 (walro03)
**		Typo in 26-mar-1998 change: ris4_us5 should be ris_us5.
**	18-Sep-1998 (merja01)
**	    Modified LD_LIBRARY_PATH change for axp_osf.  Change to make
**	    this more generic caused the path to contain garbage.  The
**	    problem on axp_osf is LD_LIBARY_PATH is set, but it is set
**	    to garbage.  Updated sprintf to hard code /lib for axp_osf.
**	17-mar-1999 (somsa01)
**	    On NT, added double quotes around the command to be executed, to
**	    handle embedded spaces.
**	19-apr-1999 (hanch04)
**	    use cl hdrs
**	07-may-1999 (cucjo01)
**	    Fixed generic seg fault if program being executed is not in path
**	    or doesn't exist.  PCcmdline now takes parameter one as NULL
**	    which allows it to default to proper command interpreter.
**	    Added environment variable QASETUSER_INGRESPW on NT to override
**	    the default password of ingresnt.
**	    Display Windows NT error # and associated text if errors occur.
**	10-may-1999 (walro03)
**	    Remove obsolete version string dr6_ues.
**      03-jul-1999 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**	28-apr-2000 (somsa01)
**	    Added the 'edbc' user.
**      23-May-2000 (hweho01)
**          Added support for AIX 64-bit platform (ris_u64). 
**	15-Jun-2000 (hanje04)
**	    Added ibm_lnx to int_lnx routine for saving LD_LIBRARY_PATH
**	11-Sep-2000 (hanje04)
**	    Added axp_lnx (Alpha Linux) to save of LD_LIBRARY_PATH.
**	21-mar-2001 (somsa01)
**	    Changed II_SYSTEM to SystemLocationVariable for different
**	    product builds.
**      28-Mar-2001 (hweho01)
**          Changed ld_lib from char pointer to global char. array,  
**          so the environment variable LIBPATH or LD_LIBRARY_PATH  
**          will be set after a new user session for ris_u64, rs4_us5  
**          and axp_osf platforms.
**	15-May-2002 (hanje04)
**	   Added Itanium Linux (i64_lnx) to routine for saving LD_LIBRARY_PATH
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	02-Jun-2005 (hanje04)
**	    Add support for Mac OS X which needs DYLD_LIBRARY_PATH saving.
**	27-Sep-2005 (hanje04)
**	    Remove bogus re-declaration of i in NT_GENERIC code. No idea
**	    how that got in there.
**	17-Mar-2006 (drivi01)
**	    Bug 115855
**	    Added functionality for handoffqa user passwords to be overwritten
**	    by external variables.  If the following variables are set in the
**	    test environment, qasetuser will use the passwords set in the env.
**		QASETUSER_PVUSR1PW
**		QASETUSER_PVUSR2PW
**		QASETUSER_PVUSR3PW
**		QASETUSER_PVUSR4PW
**		QASETUSER_QATESTPW
**		QASETUSER_DEVTESTPW
**		QASETUSER_TESTENVPW
**		QASETUSER_NOTAUTHPW
**		QASETUSER_SYSCATPW
**		QASETUSER_SUPERPW
**		QASETUSER_CTDBAPW
**		QASETUSER_RELEASEPW
**	    If the variables above aren't defined then qasetuser will use hard 
**	    coded passwords.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX.
**	18-May-2009 (hanje04)
**	    Set ingresqa as group ID instead of ingres, report appropriate
**	    error if it's not present
**	18-Jun-2009 (bonro01)
**	    Allow passing additional directory paths to LD_LIBRARY_PATH by
**	    setting a QA_LD_LIBRARY_PATH that will get appended.
**	10-Aug-2009 (lunbr01)
**	    Remove SecureZeroMemory() calls added for prior fix (bug 115855)
**	    on Windows.  This was causing a crash on Win2008 because it was
**	    trying to write over the default password string constants, which
**	    are kept in protected memory in this environment.  The purpose for
**	    the calls was to hide the user passwords in memory, which is not
**	    that important for an internal test tool.
**	10-Aug-2009 (drivi01)
**	    Require elevation in this module on Vista.
*/

/*
**      ming hints
PROGRAM = qasetuser 
**
NEEDLIBS = LIBINGRES
**
OWNER = root 
** 
MODE = SETUID
**
*/

#include <compat.h>
#include <clconfig.h>
#include <gl.h>
#include <si.h>
#include <st.h>   
#include <pc.h>   
#include <lo.h>   
#include <er.h> 
#include <ep.h>  

#undef	abs
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
# if defined(su4_us5) || defined(dr6_us5) || \
     defined(usl_us5) || defined(nc4_us5) || defined(sqs_ptx) || \
     defined(pym_us5) || defined(sui_us5) || defined(NT_GENERIC) || \
     defined(rmx_us5) || defined(rux_us5)
# define        bcopy(a,b,c)    memcpy(b,a,c)
# define    	bzero(s, n)     memset(s, 0, n)
# define        index          strchr
# else
#include <strings.h>
# endif /* su4_us5 dr6_us5 usl_us5 nc4_us5 pym_us5 sqs_ptx sui_us5 */
#include <errno.h>
#ifndef NT_GENERIC
#include <pwd.h>
#include <unistd.h>
# ifdef OSX
#include <grp.h>
# endif
#else
#include <windows.h>
#endif

int	direct_fork();

#define	CMD_LINE_MAX	1024
#define PATHLEN		256

int	debug = 0;
#define	DEBUG	if(debug)printf

#ifdef NT_GENERIC
HANDLE           phToken;
TOKEN_PRIVILEGES tkp;
BOOL             status;
#endif

main(argc,argv) 
int	argc;
char	*argv[];
{
	int		i,ret_val,uid  = -1;
	struct passwd	*pass;
# ifdef OSX
	struct passwd	*ingpwd;
	struct group	*inggrp;
# endif
	char		cmd_line[CMD_LINE_MAX];
	char		**arglist;
	int		argn = 1;
	char		*shell;
    char		*ingres_pw;
    char		*pvusr1_pw;
    char		*pvusr2_pw;
    char		*pvusr3_pw;
    char		*pvusr4_pw;
    char		*qatest_pw;
    char		*devtest_pw;
    char		*testenv_pw;
    char		*notauth_pw;
    char		*syscat_pw;
    char		*super_pw;
    char		*ctdba_pw;
    char		*release_pw;


	if (argc <= 1) {
		SIfprintf(stderr, ERx("No uid passed\n"));
		exit(0);
	}		
	if(!STcompare(argv[argn], "-d")) {
		debug = 1;
		argn++;
	}

        if (STcompare(argv[argn], ERx("ingres"))  &&
            STcompare(argv[argn], ERx("pvusr1"))  &&
            STcompare(argv[argn], ERx("pvusr2"))  && 
            STcompare(argv[argn], ERx("pvusr3"))  && 
            STcompare(argv[argn], ERx("pvusr4"))  && 
            STcompare(argv[argn], ERx("qatest"))  &&
            STcompare(argv[argn], ERx("devtest")) &&
            STcompare(argv[argn], ERx("testenv")) &&
            STcompare(argv[argn], ERx("notauth")) &&
            STcompare(argv[argn], ERx("syscat")) &&
            STcompare(argv[argn], ERx("super")) &&
            STcompare(argv[argn], ERx("ctdba")) &&
            STcompare(argv[argn], ERx("release")))
	{
        SIfprintf(stderr,
        ERx("Not allowed to assume user id for: %s\n"), argv[argn]);
        exit(1);
	}

#ifdef NT_GENERIC
	  ELEVATION_REQUIRED();	 
	  ingres_pw=getenv("QASETUSER_INGRESPW");
	  if(!ingres_pw)
	 	  ingres_pw="ingresnt";
	  pvusr1_pw=getenv("QASETUSER_PVUSR1PW");
	  if(!pvusr1_pw)
	 	  pvusr1_pw="ca-pvusr1";
	  pvusr2_pw=getenv("QASETUSER_PVUSR2PW");
	  if(!pvusr2_pw)
	 	  pvusr2_pw="ca-pvusr2";
	  pvusr3_pw=getenv("QASETUSER_PVUSR3PW");
	  if(!pvusr3_pw)
	 	  pvusr3_pw="ca-pvusr3";
	  pvusr4_pw=getenv("QASETUSER_PVUSR4PW");
	  if(!pvusr4_pw)
	 	  pvusr4_pw="ca-pvusr4";
	  qatest_pw=getenv("QASETUSER_QATESTPW");
	  if(!qatest_pw)
	 	  qatest_pw="ca-qatest";
	  devtest_pw=getenv("QASETUSER_DEVTESTPW");
	  if(!devtest_pw)
	 	  devtest_pw="ca-devtest";
	  testenv_pw=getenv("QASETUSER_TESTENVPW");
	  if(!testenv_pw)
	 	  testenv_pw="ca-testenv";
	  notauth_pw=getenv("QASETUSER_NOTAUTHPW");
	  if(!notauth_pw)
	 	  notauth_pw="ca-notauth";
	  syscat_pw=getenv("QASETUSER_SYSCATPW");
	  if(!syscat_pw)
	 	  syscat_pw="ca-syscat";
	  super_pw=getenv("QASETUSER_SUPERPW");
	  if(!super_pw)
	 	  super_pw="ca-super";
	  ctdba_pw=getenv("QASETUSER_CTDBAPW");
	  if(!ctdba_pw)
	 	  ctdba_pw="ca-ctdba";
	  release_pw=getenv("QASETUSER_RELEASEPW");
	  if(!release_pw)
	 	  release_pw="ca-release";
	  
        if (strcmp(argv[argn], "ingres")==0)
	  status=LogonUser("ingres", ".", ingres_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 
        if (strcmp(argv[argn], "pvusr1")==0)
	  status=LogonUser("pvusr1", ".", pvusr1_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 
        if (strcmp(argv[argn], "pvusr2")==0)
	  status=LogonUser("pvusr2", ".", pvusr2_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 
        if (strcmp(argv[argn], "pvusr3")==0)
	  status=LogonUser("pvusr3", ".", pvusr3_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 
        if (strcmp(argv[argn], "pvusr4")==0)
	  status=LogonUser("pvusr4", ".", pvusr4_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 
        if (strcmp(argv[argn], "qatest")==0)
	  status=LogonUser("qatest", ".", qatest_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 
        if (strcmp(argv[argn], "devtest")==0)
	  status=LogonUser("devtest", ".", devtest_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 
        if (strcmp(argv[argn], "testenv")==0)
	  status=LogonUser("testenv", ".", testenv_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 
        if (strcmp(argv[argn], "notauth")==0)
	  status=LogonUser("notauth", ".", notauth_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 
        if (strcmp(argv[argn], "syscat")==0)
	  status=LogonUser("syscat", ".", syscat_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 
        if (strcmp(argv[argn], "super")==0)
	  status=LogonUser("super", ".", super_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 
        if (strcmp(argv[argn], "ctdba")==0)
	  status=LogonUser("ctdba", ".", ctdba_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 
        if (strcmp(argv[argn], "release")==0)
	  status=LogonUser("release", ".", release_pw, LOGON32_LOGON_BATCH,
	                   LOGON32_PROVIDER_DEFAULT, &phToken); 

	if (status==FALSE) {
      DWORD rc, dwRetCode;
      LPTSTR lpszText;
      
      dwRetCode=GetLastError();
      rc=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_ARGUMENT_ARRAY |
                       FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       NULL,
                       dwRetCode,
                       LANG_NEUTRAL,
                       (LPTSTR)&lpszText,
                       0,
                       NULL);

      if (rc)
	     SIfprintf(stderr, ERx("Could not find %s; Error [%d] - %s"), argv[argn], dwRetCode, lpszText);
      else
	     SIfprintf(stderr, ERx("Could not find %s"), argv[argn]);
 
	  exit(1);
	}
#else
	pass = getpwnam(argv[argn]);
        if(!pass) {
	  SIfprintf(stderr, ERx("Could not find %s\n"), argv[argn]);
	  exit (1);
	}

# ifdef OSX
	/* Need to set group ID on Mac too. Use ingresqa group */
	inggrp = getgrnam("ingresqa");
	if(!inggrp) {
	  SIfprintf(stderr, ERx("Could not find group ingresqa\n"));
	  exit(1);
	}
	if(setgid(inggrp->gr_gid) == -1) {
	    SIfprintf(stderr, ERx("Could not set gid to %d\n"), inggrp->gr_gid);
	    perror("Failed with:");
	    exit(1);
	}
# endif
	if(setuid(pass->pw_uid) == -1) {
	    SIfprintf(stderr, ERx("Could not set uid to %d\n"),pass->pw_uid);
	    exit(1);
	}
#endif

	shell=getenv("SHELL");
	if(!shell)
#ifdef NT_GENERIC
		shell="cmd";
#else
		shell="/bin/sh";
#endif

	/* setup direct fork args */
	argn++;
        arglist = &argv[argn];

#ifdef	xDEBUG
	i=0;
	while(arglist[i])
		DEBUG(ERx("arglist[%d] = %s\n"), i, arglist[i++]);
#endif

	/* direct_fork */
	if(!direct_fork(shell, &arglist))
		exit(0);

	/* didn't fork must use PCcmd_line*/
	bzero(cmd_line, sizeof(cmd_line));

	for (i=argn; i <= (argc-1); i++)
            sprintf(cmd_line, ERx("%s %s"), cmd_line, argv[i]);

	DEBUG(ERx("Using PCcmd_line, %s\n"), cmd_line);

    ret_val = PCcmdline(NULL, cmd_line, PC_WAIT,
           (LOCATION *)NULL, (CL_ERR_DESC *)NULL);

    exit(ret_val);
}

direct_fork(shellp, arglistptr)
char	*shellp;
char	**arglistptr;
{
	int	ret_val = -1;
	char	**arglist = (char**)*arglistptr;
	int	i, l, endit = 0, n;
	char	*pathlist, *ptr, *pp;
	char	path[PATHLEN], slot[PATHLEN];
	char	*narglist[50];
#ifdef NT_GENERIC
	STARTUPINFO startinfo;
	PROCESS_INFORMATION procinfo;
	char arg_line[256], full_path[PATHLEN];
	int j, exitcode;
#endif

	/* any prg to test? */
	i=0;
	n=0;
	if(!arglist[i]) {
		narglist[n++]=shellp;
		DEBUG(ERx("narglist[%d] = %s\n"), n-1, narglist[n-1]);
		narglist[n++]="-i";
		DEBUG(ERx("narglist[%d] = %s\n"), n-1, narglist[n-1]);
	}
	while(arglist[i]) {
		narglist[n++]=arglist[i++];
		DEBUG(ERx("narglist[%d] = arglist[%d] = %s\n"),
			 n-1, i-1, narglist[n-1]);
	}
	narglist[n++] = '\0';
	strcpy(path, narglist[0]);

#ifdef	xDEBUG
	i=0;
	while(narglist[i])
		DEBUG(ERx("narglist[%d] = %s\n"), i, narglist[i++]);
#endif

	/* Is prg a direct path? */
#ifdef NT_GENERIC
	if(!index(path, '\\')) {
		if (SearchPath(NULL,path,".com",sizeof(full_path),full_path, 
		               NULL)==0)
		  if (SearchPath(NULL,path,".exe",sizeof(full_path),full_path, 
		                 NULL)==0)
		    if (SearchPath(NULL,path,".bat",sizeof(full_path),
			           full_path,NULL)==0)
		      if (SearchPath(NULL,path,".cmd",sizeof(full_path),
				   full_path,NULL)==0)
		        return(-1);
		sprintf (path, "\"%s\"", full_path);
	}
	else
	{
	    char	pathtmp[PATHLEN];

	    strcpy(pathtmp, path);
	    sprintf(path, "\"%s\"", pathtmp);
	}

	narglist[0]=path;
#else
	if(!index(path, '/')) {
		if(!(pathlist=getenv(ERx("PATH"))))
			return(-1);
		DEBUG(ERx("pathlist = %s\n"), pathlist);

		/* check each slot in path */
		ptr = pathlist;
		while(*ptr) {
			/* find next environ entry */
			pp = index(ptr, ':');
			if(!pp) {
				endit = 1;
				pp = &pathlist[strlen(pathlist)];
			}

			/* extract the entry */
			bzero(slot, sizeof(slot));
			bcopy(ptr, slot, (pp-ptr));
			DEBUG(ERx("ptr = %x, pp = %x, slot = %s\n"),
				 ptr, pp, slot);

			/* expend to path */
			sprintf(path, ERx("%s/%s"), slot, narglist[0]);
			DEBUG(ERx("path = %s\n"), path);

			/* check it */
			errno = 0;
			if(!access(path, F_OK)) {
				if(!access(path, X_OK))
					break;
			}

			/* step to next */
			if(!endit)
				pp++;
			ptr = pp;
		}

		/* we failed ? */
		if(errno != 0 )
			return(-1);
		narglist[0]=path;
	}
#endif

	DEBUG(ERx("Using execvp %s\n"), path);
#ifdef	xDEBUG
	i=0;
	while(narglist[i])
		DEBUG(ERx("narglist[%d] = %s\n"), i, narglist[i++]);
#endif

	/* start it up */
#ifdef NT_GENERIC
	strcpy (arg_line, "");
	j = 0;
	while (narglist[j]!=NULL)
	{
	  strcat (arg_line, narglist[j]);
	  strcat (arg_line, " ");
	  j++;
	}

	startinfo.cb = sizeof (STARTUPINFO);
	startinfo.lpReserved = 0;
	startinfo.lpDesktop = NULL;
	startinfo.lpTitle = NULL;
	startinfo.dwX = 0;
	startinfo.dwY = 0;
	startinfo.dwXSize = 0;
	startinfo.dwYSize = 0;
	startinfo.dwXCountChars = 0;
	startinfo.dwYCountChars = 0;
	startinfo.dwFillAttribute = 0;
	startinfo.dwFlags = 0;
	startinfo.cbReserved2 = 0;
	startinfo.lpReserved2 = 0;

	status=CreateProcessAsUser(phToken, NULL, arg_line, NULL,
	                           NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL,
	                           NULL, &startinfo, &procinfo);
	CloseHandle(phToken);
	if (status==FALSE) { 
      DWORD rc, dwRetCode;
      LPTSTR lpszText;
      
      dwRetCode=GetLastError();
      rc=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_ARGUMENT_ARRAY |
                       FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       NULL,
                       dwRetCode,
                       LANG_NEUTRAL,
                       (LPTSTR)&lpszText,
                       0,
                       NULL);

      if (rc)
  	     SIfprintf(stderr, ERx("Could not execute %s; Error [%d] - %s"), arg_line, dwRetCode, lpszText);
      else
	     SIfprintf(stderr, ERx("Could not execute %s"), arg_line);

	  ret_val=0;
	  return(ret_val);
	}
	else
	{
	  WaitForSingleObject(procinfo.hProcess,INFINITE);
	  GetExitCodeProcess(procinfo.hProcess,&exitcode);
	  exit(exitcode);
	}
#else
 
     /* For the below platforms loose LD_LIBRARY_PATH/LIB_PATH definitions 
     ** when running under su binaries; put II_SYSTEM in env.
     */
 
/* Make sure we pass LD_LIBRARY_PATH on to new user shell */
#if defined( axp_osf ) || defined( ris_us5 ) || defined( rs4_us5 ) || \
    defined( ris_u64) || defined(LNX) || defined(OSX)
	if (getenv(SystemLocationVariable))
	{
	    char *qa_lib_path = NULL;
	    char ld_lib[CMD_LINE_MAX];
#if defined( ris_us5 ) || defined( rs4_us5 ) || defined( ris_u64)
	    char *ld_prefix="LIBPATH=/lib:/usr/lib:";
# elif defined(OSX)
	    char *ld_prefix="DYLD_LIBRARY_PATH=/lib:/usr/lib:";
# else
	    char *ld_prefix="LD_LIBRARY_PATH=/lib:/usr/lib:";
# endif
	    char *ld_suffix="/ingres/lib";

	    if ((sizeof(ld_lib)) < (STlength(getenv(SystemLocationVariable)) + STlength(ld_prefix) + STlength(ld_suffix)))
	    {
		SIfprintf(stderr, ERx("qasetuser: %s string too long in (DY)LD_LIBRARY_PATH save routine\n"), SystemLocationVariable);
		exit(1);
	    }
	    STprintf(ld_lib, "%s%s%s", ld_prefix, getenv(SystemLocationVariable), ld_suffix);
	    qa_lib_path = getenv("QA_LD_LIBRARY_PATH");
	    if( qa_lib_path )
	    {
		if( (sizeof(ld_lib)) < (STlength(ld_lib) + 1 + STlength(qa_lib_path)) )
		{
		    SIfprintf(stderr, ERx("qasetuser: %s string too long in (DY)LD_LIBRARY_PATH save routine\n"), qa_lib_path);
		    exit(1);
		}
		STcat(ld_lib, ":");
		STcat(ld_lib, qa_lib_path);
	    }
            DEBUG(ERx("ld_lib is %s\n"), ld_lib);
	    putenv(ld_lib);
	}
	else
	{
	    SIfprintf(stderr, ERx("qasetuser: %s not set in (DY)LD_LIBRARY_PATH save routine\n"), SystemLocationVariable);
	    exit(1);
	}
#endif

	
	execvp(narglist[0], &narglist[0]);
	perror(ERx("execvp"));
	ret_val = 0;
	return(ret_val);
#endif
}
