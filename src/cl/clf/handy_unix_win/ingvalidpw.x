/*
** Copyright (c) 1989, 2009 Ingres Corporation 
*/

# include "pwconfig.h"
# include "ingpwutil.h"
# include <sys/types.h>
# include <sys/stat.h>
# include <string.h>
# include <pwd.h>
# include <signal.h>

/*for mixed passwd support (NIS with local shadow)  */
# include <unistd.h>
# include <stdlib.h>
# include <ctype.h>


/*
** Name: ingvalidpw.c
**
** Description: This program is used to validate a given userid/password
**		pair in the shadow password file and is required set uid
**		to root.
**
** 
PROGRAM = ingvalidpw
SETUID = ROOT
**
** History:
**	28-June-1989 (fredv)
**		First created.
**	05-sep-89 (russ)
**		Modified for ODT.
**	17-jan-90 (russ)
**		Add ifdefs for sco_us5, which is the same as odt_us5,
**		in terms of password validation.
**	01-mar-90 (seng)
**		Add source to 6.2/02p code line.
**	13-Mar-90 (GordonW)
**		Make it more useful to everyone.
**	14-Mar-90 (GordonW)
**		Make sure we are root for security reasons.
**	14-Mar-90 (GordonW)
**		We need to check for effective user id as well.
**		Fix serveral more typos.
**		We now include "iinconfig.h" which is really a
**		wrapper for the INGRES include files.
**		This module also gets compiled within the user's
**		release area. Than "iiconfig.h" contains includes
**		to native include files.
**	23-mar-90 (russ)
**		Fix typo in define of GetPWnam for odt_us5.
**	17-apr-90 (russ)
**		Add error logging based on the II_INGVALIDPW_LOG environment
**		variable.
**	20-apr-90 (russ)
**		Make the error logging more generic so that users can compile
**		this program.
**	23-apr-90 (russ)
**		Change STlength to strlen, as part of the effort to make this
**		code more generic.
**	19-Nov-90 (jkb) add sqs_ptx ifdef 
**	06-jul-92 (swm)
**		Added dr6_us5 and dra_us5 to systems that have shadow
**		password validation. [ It appears that secure unix on
**		DRS6000 (dr6_uv1) does things in a different way. ]
**	31-mar-93 (vijay)
**		Use iiconfig.h and not compat.h.
**	7-oct-93 (tomm)
**		Fix for Solaris release- use shadow password ugliness.
**		Fix a couple of the debugging ifdef's (OK->E_OK).
**	14-oct-93( tomm)
**		Add HP8 shadow password entries
**	20-mar-95 (smiba01)
**		Added dr6_ues to #ifdef for shadow password validation.
**		Note: this change originally from ingres63p (02-Feb-94 (ajc))
**	15-jun-95 (popri01)
**		Add nc4_us5 to shadow password platforms
**	17-jul-95 (morayf)
**		Added SCO sos_us5 to odt_us5 use of set_auth_parameters().
**      23-jul-1995 (pursch)
**              Add pym_us5 to shadow.h users list.
**	06-Dec_1995 (murf)
**		Added sui_us5 to call correct get-password function.
**	12-dec-95 (morayf)
**		Added SNI RMx00 (rmx_us5) to those needing shadow.h etc.
**	12-nov-96 (merja01)
**              Added AXP.OSF optional shadow password capability
**      19-nov-1996 (canor01)
**              Changed name of 'iiconfig.h' to 'pwconfig.h' for portability
**              to Jasmine.
**	28-jul-1997 (walro03)
**		Updated for Tandem NonStop (ts2_us5).
**	28-nov-1997(angusm)
**	        Various protective changes to do with II_INGVALIDPW_LOG.	
**		(bug 87497).
**	21-Dec-1997
**		(lavcl02)
**		Allowed axp_osf to use set_auth_parameters()
**	09-feb-1998 (somsa01) X-integration of 433336 from oping12 (angusm)
**	    hp10_us5 addition, omitted from previous
**	05-mar-1998(musro02)
**	        For sqs_ptx, revised to use setreuid/setregid vs seteuid/setegid
**	
**	15-april-1998(angusm)
**		SIR 50300/Bug 71938: for platforms supporting shadow
**	        passwords, check expiry date on password - all
**		current platforms with shadow password functionality, 
**		less HP and Sequent.
**	20-april-1998(angusm)
**		Further revisions:
**	1) Move sco/osf-specific call to set_auth_parameters() as documented
**	   behaviour is that it needs to be first call in main().
**	2) More changes to tighten security, based on comments made by AusCert.
**		Thanks to Eric Halil, Dr Paul Dale of AUSCERT and
**	        Scott Merrilees of BHP for comments/help with this.
**	18-june-1998(angusm)
**		Omission in logic of 'iscan' caused SEGV on SCO OpenServer 
**	        and Sequent when parsing input. Bug 91561. 
**	16-jul-1998 (bobmart)
**		Re-define NULL for ris_us5; on AIX 3.2.5, NULL is defined as
**		"((void *)0)", and causes compilation errors in this module.
**		In later versions of AIX, NULL is defined simply as "0", 
**		which is more consistent with other unix platforms.
**	13-oct-98 (toumi01)
**		For lnx_us5 use shadow passwords conditionally based on the
**		state of the installed system (mkvalidpw checks /etc/shadow).
**	30-oct-1998 (loera01) Bug 44829:
**		Shadow password validation may fail if the password contains 
**		more than eight characters, because some Unix platforms append 
**		"noise" characters to the entries in the password file.  Changed
**		strcmp(crypted, pw->pw_passwd) to strncmp(crypted,
**		pw->pw_passwd,strlen(crypted)).
**      03-jul-99 (podni01)
**              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	06-oct-1999 (toumi01)
**		Change Linux config string from lnx_us5 to int_lnx.
**      06-jan-00 (bonro01)
**              Added SHADOW passwords support for DG/UX Intel (dgi_us5).
**	14-feb-2000 (loera01) Bug 100276
**		Added check of max member of passwd structure.  This field
**		may be used to determine password aging instead of the exp
** 		member.
**	13-Mar-2000 (ashco01/) Bug 97348
**		Added support for full length password checking on axp_osf.
**              1) Replaced call to getprpwnam() with getespwnam().
**              2) Replaced call to crypt() with dispcrypt() as crypt()
**                 only checks first 8 characters of password.
**     01-Aug-2000 (loera01) Bug 102237
**             Added hpb_us5 definition to GetPWnm determination.
**	14-jun-00 (hanje04)
**		For ibm_lnx  use shadow passwords conditionally based on the
**		state of the installed system (mkvalidpw checks /etc/shadow).
**      16-Nov-2000 (hweho01)
**              To fix the compiler error on axp_osf ( Rel. 4.0F ) 
**              1) Defined pw_passwd to ufld->fd_encrypt, according to   
**                 es_passwd struct in prot.h, ufld is a pointer to	  
**                 espw_field struct.
**              2) Defined pw_name  to ufld->fd_name. 
**	10-jan-01 (hanje04)
**		Added SHADOW password support for Alpha Linux (axp_lnx).
**	04-Dec-01 (hanje04)
**		Added SHADOW password support for IA 64 Linux (i64_lnx).
**	21-May-2003 (bonro01)
**		Re included Add support for HP Itanium (i64_hpu)
**      18-jul-03 (pickr01/raodi01)
**              Added "mixed" environment support, i.e installations where
**              we have SHADOW passwords and standard passwords. (for example
**              NIS clients on systems with local shadow passwords)
**              If user is not found in the shadow database search in the
**              local database. This feature is switchen on only, when
**              "MIXEDPWD" is defined.
**              Changes: 
**              renamed macros "pw_passwd", "pw_name" and "crypt" to
**                              PW_passwd", "PW_name" and "CRYPT"
**              and changed all references to these macros.
**              if GetPWnam does not find the user search again using getpwnam
**              suppres expiration check when searching user in standard passwd
**              (it is not defined)
**	20-Jan-2005 (rajus01) Bug # 113779 Startrak problem: INGNET 162
**	    On a standard HPUX system with no /etc/shadow file, the password 
**	    and aging information is obtained from /etc/passwd file.
**	    If the system has been converted to a trusted system, the password
**	    and aging information is obtained from the Protected Password 
**	    Database (/tcb/files/auth/). 
**	    
**	    It has been found that on HPUX trusted systems, ingvalidpw returns
**	    successful authentication for HPUX users whose account has either 
**	    expired or been disabled. Implemented user authentication using 
**	    getprpwnam() instead of getspnam() for HPUX trusted systems.
**      15-Mar-2005 (bonro01)
**          Added support for Solaris AMD64 a64_sol.
**	08-Jul-2005 (sheco02)
**	    Fix the x-integration change 477710 that removed part of the 
**	    HP Itanium (i64-hpu) support in change 463841 and apply the 
**	    change 470570.
** 	07-oct-2005 (Daryl Monge & ashco01) Star issue # 144288244
** 	    Fix various problems with the HP specific lockout and expiration 
**	    code. The code incorrectly tests various fields directly without 
**	    first checking the various flag field to see if a test is warrented.
**	    Also, the number of invalid login attempts was only testing against
**	    user specific limits and was not checking the system wide invalid 
**	    login attempt limit.
**	08-mar-2006 ( Daryl Monge & rajus01 ) SD # 103126 
**	    Daryl's comment: Apparently, login tools on the HP seem to allow an 
**	    expiration of zero to be treated the same as "empty" (NULL if you 
**	    will) and as a result there are situations where a user can login 
**	    to the system but the new ingvalidpw will reject the login.
**	    The fix involves the following change.
**	    Changing  # define EXPSET (pw->uflg.fg_expire) to
**	     # define EXPSET (pw->uflg.fg_expire && pw->pw_exp > 0)
**	13-jun-06 (hayke02)
**	    Change copyright notice from Computer Associates to Ingres
**	    Corporation.
**      10-oct-06 (Ralph Loen) Bug 116831
**          On HP, use shadow password structures and routines if
**          mkvalidpw finds /etc/shadow (SHADOW_EXISTS is defined).
**      11-oct-06 (Ralph Loen) Bug 116831
**          We now have three cases for HP/UX: trusted, shadowed,
**          and plain vanilla.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**      13-Nov-2006 (hanal04) Bug 117085
**          Don't don't redfine functions by declaring local valriable
**          equivalents on i64.hpu. Later versions of the compiler will
**          generate warnings and a positive return value which is
**          treated as a failure in mkvalidpw.
**      11-Sep-2008 (ashco01) Bug 120885
**          For HP Trusted systems, pw_mixed should point to
**          a PWD_ENTRY record.
**      12-Sep-2008 (ashco01) Bug 120720
**          Expired accounts (passwords) are not detected correctly on
**          HPUX Trusted systems. Test for a password change date of
**          zero (1-jan-1970) to detect these. (see getprpwnam man page).
**      18-Sep-2008 (ashco01) Bug 120885
**          Improve previous fix for Bug 120885. 
**      23-Mar-2007
**          SIR 117985
**          Add support for 64bit PowerPC Linux (ppc_lnx)
**      22-May-2008 (rajus01) Sir 120420, SD issue 116370.
**	    Added PAM support in Ingres, a contribution from the Ingres
**	    community. Moved the common code/definitions used by Ingvalidpam
**	    Ingvalidpw to separate files. Also did some code cleanup around
**	    debugging, and usage of return versus exit calls for consistency. 
**	22-Jun-2009 (kschendel) SIR 122138
**	    Look for both 32-bit and 64-bit config strings now.
**	    Caution!  this file is compiled by the installer after
**	    distribution, and (at present) the nifty sparc_sol, any_hpux,
**	    any_aix symbols are not available.  pwconfig.h has the primary
**	    config symbol and that's it.
**	28-Aug-09 (gordy)
**	    Assign username and password max lengths from platform
**	    specific symbols.  Otherwise, provide a default of 256
**	    as maximum length as this been declared a sufficient
**	    max length for long Ingres identifiers.
*/

#if defined(axp_osf)
#include <sys/types.h>
#include <sys/security.h>
#include <prot.h>
# define        gotit
# define        GetPWnam                getespwnam
# define        USR_LEN                 AUTH_MAX_UNAME_SIZE
# define        PWD_LEN                 AUTH_MAX_CIPHERTEXT_LENGTH
typedef         struct es_passwd        PWD_ENTRY;
# define        PW_passwd               ufld->fd_encrypt
# define        CRYPT(a,b)              dispcrypt(a,b,pw->ufld->fd_oldcrypt) 
# define        PW_name                 ufld->fd_name
#endif /* defined(axp_osf) */

#if defined(odt_us5) || defined(sco_us5) || defined(sos_us5)
# define SecureWare
# undef  BITSPERBYTE
#include <sys/security.h>
#include <sys/audit.h>
#include <prot.h>
# define	gotit
# define	GetPWnam	getprpwnam
# define	USR_LEN		256
# define	PWD_LEN		\
	(AUTH_CIPHERTEXT_SIZE(AUTH_SEGMENTS(AUTH_MAX_PASSWD_LENGTH))+1)
typedef		struct	pr_passwd PWD_ENTRY;
# define	PW_passwd	ufld.fd_encrypt
#endif

#if defined(su4_us5) || defined(su9_us5) || defined(a64_sol) || \
    defined(usl_us5) || \
    ((defined(int_lnx) || defined(ibm_lnx) || defined(axp_lnx) || \
      defined(i64_lnx) || defined(a64_lnx) || defined(i64_hpu) || \
      defined(hp8_us5) || defined(hpb_us5) || defined(hp2_us5) || \
      defined(int_rpl) || defined(ppc_lnx)) && defined(SHADOW_EXISTS))
# include <shadow.h>
# define	gotit
# define	GetPWnam	getspnam
typedef		struct spwd	PWD_ENTRY;
/*  */
# define	PW_passwd	sp_pwdp
# define	PW_name		sp_namp
#endif


#if defined(su4_us5) || defined(su9_us5) || defined(a64_sol) || \
    defined(usl_us5) || \
    ((defined(int_lnx) || defined(int_rpl) || defined(ibm_lnx) || \
      defined(axp_lnx) || defined(i64_lnx) || defined(a64_lnx)) && defined(SHADOW_EXISTS))
# include <sys/time.h>
# define SECHOUR		3600
# define HOURDAY		24
# define	pw_exp		sp_expire
# define        pw_max          sp_max
# define        pw_lstchg       sp_lstchg
# define	CHECK_EXP
struct	timeval tv;
char	*t2;
# define	GETTIME		gettimeofday( &tv , (void *)t2 )
# define	EXPSET		(pw->pw_exp >= 0)
# define        MAXSET          (pw->pw_max > 0)
#endif

#if defined(hp8_us5) || defined(hpb_us5) || defined(hp2_us5) || \
    defined(i64_hpu)
#if defined(HP_TRUSTED)
#define gotit
#include <hpsecurity.h>
#include <prot.h>
#define	GetPWnam	 getprpwnam  
typedef	struct pr_passwd PWD_ENTRY;
# define	pw_passwd	ufld.fd_encrypt
# define	pw_name		ufld.fd_name
# define        USR_LEN         256
# define	PWD_LEN		\
	(AUTH_CIPHERTEXT_SIZE(AUTH_SEGMENTS(AUTH_MAX_PASSWD_LENGTH))+1)
# include <sys/time.h>
# define SECHOUR		3600
# define HOURDAY		24
# define	pw_exp		ufld.fd_expire
# define        pw_lstchg       ufld.fd_schange
# define	pw_lock		ufld.fd_lock
# define	pw_nlogin	ufld.fd_nlogins
# define	pw_maxtries	ufld.fd_max_tries
# define	CHECK_HPEXP
# define	CHECK_HPLCK
struct	timeval tv;
void	*t2 = NULL;
# define	GETTIME		gettimeofday( &tv , t2 )
# define 	EXPSET (pw->uflg.fg_expire && pw->pw_exp > 0)
#elif !defined(SHADOW_EXISTS)
#define gotit
#define       GetPWnam        getspwnam
typedef struct s_passwd       PWD_ENTRY;
#endif /* #if !defined(SHADOW_EXISTS) */
#endif /* hpux */

/* 
** To allow authentication against /etc/passwd and /etc/shadow:  if entry 
** is not found in the shadow database, check the standard password database.
** Don't switch on if we search in /etc/passwd only (i.e., gotit is not 
** defined yet) 
*/
#if defined(gotit)
#if !defined(HP_TRUSTED)
#define MIXEDPWD_ON
#endif
#endif

#ifndef	gotit
# define	gotit
# define	GetPWnam	getpwnam
typedef		struct passwd	PWD_ENTRY;
#endif

#ifndef USR_LEN
# define	USR_LEN		256
#endif

#ifndef PWD_LEN
# define	PWD_LEN		256
#endif

#ifndef PW_passwd
#define PW_passwd pw_passwd
#endif
	
#ifndef PW_name
#define PW_name pw_name
#endif
	
#ifndef CRYPT
#define CRYPT crypt
#endif


main( argc, argv )
int	argc;
char	*argv[];
{

#if defined(axp_osf)
	char	*dispcrypt();
        char    *crypt();
#elif !defined(i64_hpu)
	char 	*CRYPT();
#endif
#ifdef MIXEDPWD_ON
    struct passwd *pw_mixed;
#endif
	PWD_ENTRY	*pw;
#if !defined(i64_hpu)
	PWD_ENTRY	*GetPWnam();
#endif
	char	*crypted;
	char    user[USR_LEN+1];
	char    password[PWD_LEN+1];
	FILE	*fp = NULL;
	int	status;

/*  */
        char *encrypted_pwd;
        int check_again=0;

/*  */
#ifdef xDEBUG
       char *debug_user;
#endif
 
# if defined(odt_us5) || defined(sco_us5) || defined(sos_us5) || defined(axp_osf)
	/*
	** SCO, OSF needs to call this setup function
	*/
	set_auth_parameters(argc,argv);
#endif

	status = getDebugFile(&fp);
	if( status != E_OK )
   	    exit( status );
        status = getUserAndPasswd( user, password, fp );
	if( status != E_OK )
	   exit( status );
	/*
	** first make sure we are root
	*/
	if(getuid() && geteuid())
	{
	    INGAUTH_DEBUG_MACRO(fp, 1,
		"error: you are not root: getuid = %d, geteuid = %d\n", 
		getuid(),geteuid());
	      exit(E_NOTROOT);
	}

	/*
	** get the username entry in /etc/passwd or other
	*/
	if((pw = GetPWnam(user)) == NULL) 
	{
/*  */
#ifdef MIXEDPWD_ON
            if((pw_mixed=getpwnam(user)) == NULL)
            {
#endif
	      INGAUTH_DEBUG_MACRO(fp, 1,
			"error: getpwnam failed: can't get password\n");
	      exit (E_NOUSER);
/*  */
#ifdef MIXEDPWD_ON
           }
           else 
           {
              encrypted_pwd=pw_mixed->pw_passwd;
#ifdef xDEBUG
          debug_user=pw_mixed->pw_name;
#endif
              check_again=1;
           }
#endif
	}
        else 
        {
           encrypted_pwd=pw->PW_passwd;
#ifdef xDEBUG
          debug_user=pw->PW_name;
#endif
           check_again=0;
        }
	if (setgid(getgid()) < 0)
	{   
	    INGAUTH_DEBUG_MACRO(fp,1, "setgid\n");
	    exit(E_NOTROOT);
	}
	if (setuid(getuid()) < 0)
	{   
	    INGAUTH_DEBUG_MACRO(fp,1,"setuid\n");
	    exit(E_NOTROOT);
	}

	/*
	** now crypt incoming passwd and match it
	*/
        /* : which crypt function is used depends wheter the password 
        ** was found in shadow or not
        */
        if(!check_again) 
        {
          crypted=CRYPT( password, encrypted_pwd );
        }
        else 
        {
          crypted=crypt( password, encrypted_pwd );
        }
	if( crypted == NULL)
	{
	    INGAUTH_DEBUG_MACRO(fp,1,"error: crypt returned NULL\n");
	    exit(E_NOCRYPT);
	}
#ifdef	xDEBUG
	printf("user: %s, passwd: %s crypt: %s crypt_passwd %s pw_name %s\n", 
		user, password, crypted, encrypted_pwd, debug_user);
#endif

#ifdef	PASS_OK
	INGAUTH_DEBUG_MACRO(fp,1,"return E_OK\n");
	exit(E_OK);
#endif

# ifdef CHECK_HPEXP
	/* HP system specific checks.  HP security systems are based on 
	   SecureWare systems. We check a number of things here.  Still not 
           entirely sure this is every check that is needed.  The HP 
	   documentation is not very complete and there is no api for doing 
           the entire process for you.  For example, there is an account 
	   lifetime flag and value, different from simple password expiration, 
	   that is not checked here.
	   - check that today is not past the accounts expiration date.
	   - Make sure that this account has not had too many invalid login 
	     attempts.
	   - check that the account has not been manually locked out by an 
	     administrator.
	 */
#ifdef xDEBUG
	INGAUTH_DEBUG_MACRO(fp,0,"HP specific values being tested for uid=%ld:\n\
 password last changed=%s\
 Admin lock:=%d  Account expiration=%s\
 Invalid logins=%d set?=%d user fd_max_tries=%d set?=%d system fd_max_tries=%d set?=%d\n",
	       pw->ufld.fd_uid,
	       pw->uflg.fg_schange?ctime(&pw->ufld.fd_schange):"<unset>\n",
	       pw->ufld.fd_lock, EXPSET?ctime(&pw->ufld.fd_expire):"<unset>\n",
	       pw->ufld.fd_nlogins, pw->uflg.fg_nlogins,
	       pw->ufld.fd_max_tries, pw->uflg.fg_max_tries,
	       pw->sfld.fd_max_tries, pw->sflg.fg_max_tries
	 );
#endif
	{
	    if ( pw->ufld.fd_schange == 0 ) {
	        INGAUTH_DEBUG_MACRO(fp,0,"Error: password expired\n");
	        exit(E_EXPIRED);
	    }
	}

	{
	    int maxtries = -1; /* Smaller than all possible values of nlogin */
	    /* The max invalid login attempt tries can either be specified by 
	       user or system wide. The HPUX api does not handle any of this 
	       algorithm for the application so we must apply
	     these tests each in turn ourselves.
	    */
	    maxtries = pw->sflg.fg_max_tries?pw->sfld.fd_max_tries:maxtries;
					   /* use system max tries if set */
	    maxtries = pw->uflg.fg_max_tries?pw->ufld.fd_max_tries:maxtries;
				   /* override with user max tries if set */
	    if ( pw->uflg.fg_nlogins ) {
	       if( pw->pw_nlogin >= maxtries )
	       {
	           INGAUTH_DEBUG_MACRO(fp,1,"Error: Login Account locked due to failed login attempts\n");
  	            exit(E_LOCKED);
	       }
	    }
	}

  	if ( pw->pw_lock ) {
	  INAUTH_DEBUG_MACRO(fp,1,"Error: Login Account locked\n");
	  exit(E_LOCKED);
	}

# endif

# ifdef CHECK_HPLCK

        if( ( pw->pw_nlogin != 0 ) && ( pw->pw_nlogin >= pw->pw_maxtries ) )
        {
	    INGAUTH_DEBUG_MACRO(fp, 1,
		"Error: Login Account locked due to failed login attempts\n");
 	    exit(E_LOCKED);
        }

 	if ( pw->pw_lock != '\0' )
        {
	    INGAUTH_DEBUG_MACRO(fp,1,"Error: Login Account locked\n");
 	    exit(E_LOCKED);
        }

# endif

# ifdef CHECK_EXP
/* , no exp check if passwd was found in passwd, but not in shadow */
        if(!check_again)
        {
 	  GETTIME;
 	  if ( EXPSET &&  (pw->pw_exp * SECHOUR * HOURDAY) < tv.tv_sec )
 	    exit(E_EXPIRED);
          if ( MAXSET && (tv.tv_sec / (SECHOUR * HOURDAY))
              > (pw->pw_lstchg + pw->pw_max) )
 	    exit(E_EXPIRED);

 	  if ( !MAXSET && !pw->pw_lstchg )
	    exit(E_EXPIRED);  
         }
# endif

        if( strncmp( crypted, encrypted_pwd, strlen(crypted)) != 0 )
	{
	    INGAUTH_DEBUG_MACRO(fp,1,"error: password mis-match\n");
	    exit(E_NOMATCH);
	}
	INGAUTH_DEBUG_MACRO(fp,1,"return E_OK\n");
	exit(E_OK);
}
