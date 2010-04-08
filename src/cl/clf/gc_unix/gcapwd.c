/*
**Copyright (c) 2004, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <qu.h>
#include    <ctype.h>
#include    <clsigs.h>
#include    <cv.h> 
#include    <gc.h>
#include    <si.h>
#include    <clnfile.h>
#include    <nm.h>
#include    <pc.h>
#include    <tm.h>
#include    <pwd.h>
#include    <diracc.h>
#include    <handy.h>
#include    "gcacli.h"
#include    <errno.h>

#include    <cs.h>

# ifdef OSX
/* Need to use directory services on OSX */
STATUS
usrauthosx( char *user, char *password, CL_ERR_DESC *sys_err );
# endif

/*{
** Name: GCusrpwd() - check user/passwd
**
** Description:
**	Checks if the password is valid for the user name in the 
**	system password file /etc/passwd.  
**
** Returns:
**	FAIL:	password didn't match or bad user name
**	OK:	user name and password valid
**
** History:
**	05-Jun-89 (seiwald)
**	    Written.
**	26-Sep-89 (seiwald)
**	    Added CL_ERR_DESC per CL requirements.
**	13-Nov-89 (seiwald)
**	    Return GC_LOGIN_FAIL if the user name is not found in the
**	    password file.
**	10-Aug-90 (gautam)
**	    Added shadow password support from 6202p codeline
**	17-Aug-90 (gautam)
**	    Corrected logic in shadow password  getpwnam() statement
**	21-Aug-90 (seiwald)
**	    Scrunched; no more setting II_SHADOW_PWD; no more calling
**	    CL_RESV_FD, as fds are used only transiently.
**	27-Feb-91 (seiwald)
**	    Stash result of NMgtAt - it may not be around long.
**	29-Oct-91 (gautam)
**	    Don't call crypt() if NO_CRYPT is defined
**      27-sep-91 (jillb--DGC)
**          Added TYPESIG cast to signal( SIGCLD, SIG_DFL) portion of
**              sigsave equals statement to prevent compiler warning about
**              incompatible pointer types.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**	19-apr-95 (canor01)
**	    added <errno.h>
**	03-jun-1996 (canor01)
**	    iiCLgetpwnam() needs to be passed a pointer to passwd struct.
**      12-nov-96 (merja01)
**        AXP.OSF has optional shadow password capability.  Use    
**        II_SHADOW_PWD to dictate  use of ingvalidpw.  The
**        xCL_065_SHADOW_PASSWD will be set by default.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	28-may-99 (matbe01/ahaal01)
**	    The pipe for ingvalidpw picks up a file descriptor of 0 on
**	    AIX (rs4_us5) which is closed right before the dup thus causing
**	    a broken pipe.  A second pipe has been added when this
**	    situation occurs and it is used in place of the first pipe.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	14-feb-00 (loera01) bug 100276
**	    For hp8_us5 only, check for password expiration from pw_age 
**	    member of passwd structure.
**	28-Jun-2005 (hanje04)
**	    Add support for Mac OSX. There is no crypt() on OSX so we have
**	    to use Directory Services. Code is in cl!clf!handy
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	15-Jul-09 (gordy)
**	    Increase size of temp buffer for longer names and add
**	    check for buffer overrun.
*/

STATUS
GCusrpwd( user, password, sys_err )
char		*user;
char		*password;
CL_ERR_DESC	*sys_err;
{
	static char *pwdcmd = (char *)-1;
	static char pwdbuf[ 256 ];
	char	buffer[514];
#ifdef any_hpux
#define WEEK (60 * 60 * 24 * 7)      /* 1 week in seconds */
	i4      weeks;
	char    amax[2] = {'\0','\0'};
	char    age[3] = {'\0','\0','\0'};
	char    *ptr;
	i4      tmax;
	i4      total = -1;
	SYSTIME current;
#endif /* hpux */

	STATUS status = OK;

	CL_CLEAR_ERR( sys_err );

#ifdef	NO_CRYPT
	return(GC_LOGIN_FAIL);
#else

# ifdef OSX
	/* There is no crypt on MAC OSX we have to use Directory Services */
	return( usrauthosx( user, password, sys_err ) ) ;
# endif

	/*
	** Find command name for checking passwords.
	** If NULL, read password file directly.
	*/

	if( pwdcmd == (char *)-1 )
	{
	    NMgtIngAt( "II_SHADOW_PWD", &pwdcmd );

#ifdef	xCL_065_SHADOW_PASSWD
	    if( !pwdcmd || !*pwdcmd )
/*  if xCL_065_SHADOW_PASSWD is defined on axp_osf and
 *  II_SHADOW_PWD is not pointing to ingvalidpw, 
 *  set pwdcmd to NULL.  This will force us to 
 *  read /etc/passwd
 */
#ifdef axp_osf             
		pwdcmd = NULL; 
#else         
		pwdcmd = "ingvalidpw";
#endif
#else	/* xCL_065_SHADOW_PASSWD */
	    if( !pwdcmd || !*pwdcmd )
		pwdcmd = NULL;
#endif	/* xCL_065_SHADOW_PASSWD */

	    if( pwdcmd )
	    {
		STncpy( pwdbuf, pwdcmd, sizeof( pwdbuf ) - 1 );
		pwdbuf[ sizeof( pwdbuf ) - 1 ] = EOS;
		pwdcmd = pwdbuf;
	    }
	}

	/* 
	** Check password 
	*/

	if( !pwdcmd )
	{
	    /* 
	    ** Check password file directly 
	    */

	    struct passwd	*pw, *getpwnam();
	    struct passwd	pwd;
	    char		pwnam_buf[512];
	    char		*crypted, *crypt();

	    if( !( pw = iiCLgetpwnam( user, &pwd, 
				      pwnam_buf, sizeof(pwnam_buf) ) ) )
	    {
		status = GC_LOGIN_FAIL;
	    }
	    else if( !( crypted = crypt( password, pw->pw_passwd ) ) )
	    {
		status = GC_LSN_FAIL;
	    }
	    else if( STcompare( crypted, pw->pw_passwd ) != 0 )
	    {
		status = GC_LOGIN_FAIL;
	    }
#ifdef any_hpux
	    /*
	    ** Check for password expiration from pw_age member of passwd
	    ** structure.
	    */
	    if (CVrxl(pw->pw_age,&total) == OK && total > 0)
	    {
	        TMnow(&current);
	        ptr = pw->pw_age;
	        amax[0] = *ptr++;       /* maximum weeks */
	        *ptr++;       	        /* ignore minimum weeks */
	        age[0] = *ptr++;        /* age */
	        age[1] = *ptr;
	        if (CVrxl(age,&total) == OK && CVrxl(amax,&tmax) == OK)
	        {
	            weeks = (total + tmax + 1) - 
	                (current.TM_secs / WEEK); /* weeks left */
	            if (!weeks)
	            {
	                status = GC_LOGIN_FAIL;
	            }
	        }
	    }
# endif /* any_hpux */
	}
	else  if ( (STlength( user ) + 
		    STlength( password ) + 2) > sizeof( buffer ) )
	    status = GC_LOGIN_FAIL;
	else
	{
	    /* 
	    ** Use pwdcmd to validate password 
	    */

	    TYPESIG	(*sigsave)();
	    int		p[2];
	    int		p2[2];
	    PID		pid;
	    int		retpipe;

            sigsave = (TYPESIG (*)())signal( SIGCLD, SIG_DFL );

	    retpipe = pipe(p);
	    if( retpipe == 0 && p[0] == 0 )
	    {
		retpipe = pipe(p2);
		close(p[0]);
		close(p[1]);
		p[0] = p2[0];
		p[1] = p2[1];
	    }
	    if( retpipe < 0 )
	    {
		status = GC_LSN_FAIL;
	    }
	    else if( !( pid = PCfork(&status) ) )
	    {
		/* Child execs pwdcmd */

		close( 0 );
		dup( p[0] );
		close( p[0] );
		close( p[1] );
		execl( pwdcmd, pwdcmd, (char *)0 );
		PCexit( -1 );
	    }
	    else if( pid > 0 )
	    {
		/* Parent writes to child and gathers status */

		STprintf(buffer, "%s %s", user, password);
		close( p[0] );
		write( p[1], buffer, STlength( buffer ) + 1 );
		close( p[1] );
		
		if( (status = PCwait( pid )) != OK )
		    status = GC_LOGIN_FAIL;
		(VOID)signal(SIGCLD, sigsave);
	    }
	}

	GCTRACE(2)( "GCusrpwd: user %s status %x\n", user, status );

	return status;
#endif /* NO_CRYPT */
}
