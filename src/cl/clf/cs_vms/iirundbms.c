/*
** Copyright 1987, 2008 by Ingres Corporation 
** All rights reserved.
*/

# include    <compat.h>
# include    <cs.h>
# include    <er.h>
# include    <gc.h>
# include    <gl.h>
# include    <lo.h>
# include    <pc.h>
# include    <pm.h>
# include    <me.h>
# include    <si.h>
# include    <st.h>
# include    <cv.h>
# include    <cm.h>
# include    <nm.h>

# include    <accdef.h>
# include    <descrip.h>
# include    <dvidef.h>
# include    <efndef.h>
# include    <iledef.h>
# include    <iodef.h>
# include    <iosbdef.h>
# include    <pqldef.h>
# include    <prcdef.h>
# include    <prvdef.h>
# include    <psldef.h>
# include    <stsdef.h>
# include    <ssdef.h>
# include    <uaidef.h>
# include    <lib$routines.h>
# include    <ots$routines.h>
# include    <starlet.h>
# include    <vmstypes.h>
# include    <astjacket.h>

/*
**
**  Name: IIRUNDBMS.C - Startup the OpenVMS INGRES DBMS Componets
**
**  Description:
**      This file contains the routines necessary to startup the OpenVMS
**      DBMS Server.  This progam created the specified image and passes
**	the commands and parameters to the new process.
**
**          main() - The main routine -- argument processing and
**		     server creation.
**
**  History:
**      4-Jun-1987 (fred)
**          Created.
**	18-Oct-1993 (eml)
**	    Complete rewrite to take advantage of the new startup mechanism.
**	    Previous history removed -- did not apply.
**	19-oct-93 (eml)
**	    Fix privilege names. Use SI instead of printf.
**	22-oct-1993 (eml)
**	    Error messages correctly reflect the status of PMload operation.
**		1.) Unable to open config.dat
**		2.) Syntax error in parsing condig.dat
**
**	    Changed the the process name of the created process to be:
**		iirun_'installition-code'_'mbxunt'.
**
**	    Corrected to link with frontmain, instead of the VAXCRTL.
**		Side effects:
**		1.) Updated code to replace unresolved symbols define
**		    in the standard run-time library.
**		2.) SI routines correctly output messages to stdout.
**
**	    Corrected truncated output in the errlog.log
**
**	    If a UIC is not specified in config.dat, processes are created
**	    as subprocess, instead of detached processes.
**
**	    Corrected uninitialized "dbms" variable (OpenVMS AXP)
**
**	22-nov-1993 (bryanp) B56610, B56611
**	    Quit, don't continue, if PMload fails.
**	    Pass an error handling function to PMload.
**
**	XX-nov-1993 (eml)
**	    Correctly mask and construct UIC value. Bug #57127
**
**	    Simplified the reading and parsing PM parameters. PM parameter
**	    parsing all happens in the same place. This change makes it
**	    eaisier to add new PM parameters, because the PM parameter
**	    table can be updated without changed to the code. In addition,
**	    all error checking can happen in the same place.
**	    Note: Currently errors are ignored.
**
**	    Revamped mechanism to send commands to subprocess. To send
**	    additional commands to the subprocess, it is only necessary to
**	    add the new DCL command to the table.
**	    
**	    Do not write UIC value to errlog.log if zero is specified.
**
**	    Disable control Y, C and ON interrupts to protect the dbms process
**	    from being aborted through the input mailbox.
**
**	    Added additional VMS error status messages to the errlog.log file.
**	    This is done using the new log_errmsg function.
**
**	    Updated VMS privileges to be current with OpenVMS 5.5-2.
**
**	    Post read to termination mailbox before commands are sent
**	    to the new process. This makes it easier to determine which
**	    commands cause the new process to terminates.
**
**	    Corrected the pragma member_alignment to restore current alignment
**	    characteristics.
**
**	31-jan-1994 (bryanp) B56613, B56614, B56810, B56811, B57101
**	    Restored the 6.4 version of iics_cvtuic.
**	    Check the return code from iics_cvtuic; if it fails, terminate the
**		iirundbms processing immediately. Check the status code even if
**		message-echoing is off.
**	    Log all errors both to the screen and to the error log.
**	    Log error messages regardless of setting of "echo" parameter. An
**		error is an error, regardless of whether parameter logging is
**		occurring.
**	    If SYS$GETMSG can't format the message status code, just print the
**		code in hex (some information is better than nothing).
**	    If server exits with SS$_NORMAL, advise user to consult the
**		error log for information about the failure. If the server exits
**		with something other than SS$_NORMAL, then format the failure
**		status as best as possible and then remap it into SS$_ABORT,
**		since returning arbitrary status values to PCexit/sys$exit seems
**		to result in garbage displays on the screen.
**	    Remove use of magic number "32" as length of server_type or
**		server_flavor fields.
**	    Use case-insensitive comparisons for vms_echo, vms_swapping,
**		vms_accounting, and vms_dump, so that "Off" works identically
**		to "OFF".
**	    Added code to vms_echo, vms_swapping, vms_accounting, and vms_dump
**		to reject any values other than ON or OFF.
**	    If II_IIRUNDBMS_ECHO_CMDS is defined, echo the actual iidbms
**		commands to the screen. This is useful for debugging the
**		contents of the start_commands table.
**	    Modified the handling of parenthesized privilege lists so that it
**		made more sense to me. The previous handling of privilege lists
**		allowed
**		    PRIV1,PRIV2,PRIV3,PRIV4
**		or
**		    (PRIV1),(PRIV2),(PRIV3),(PRIV4)
**		but not
**		    (PRIV1,PRIV2,PRIV3,PRIV4)
**		although this last form worked "by accident". The new parens
**		handling supports only the first and third forms, not the 2nd.
**	    Fixed handling of privilege "all" so that it worked properly when
**		it was the last privilege in a parenthesized privilege list.
**	    Added check so that unrecognized privileges are logged and rejected
**		as errors, rather than being quietly ignored.
**	    Added check for overly-long list of vms_privileges, to avoid memory
**		overwriting bug.
**	    Save return code from PMinit -- it's not a VOID function. Test the
**		PMinit return code; if PMinit fails, report the reason why and
**		quit running.
**	    Save return code from CVal -- it's not a VOID function either. Test
**		the CVal return code; if it fails, report why and quit running.
**		Typically, CVal() failure is due to a typo in config.dat.
**	    Corrected READALL privilege mask definition so that it works.
**		Also corrected UPGRADE, DOWNGRADE, GRPPRV, and SECURITY, since
**		they all had the same bug (all the bits in the second longword
**		were being handled wrong.)
**	    Corrected ACNT privilege mask definition so that it properly set
**		only the PRV$M_ACNT bitflag. PRV$M_ACNT and PRV$M_NOACNT are
**		synonyms, and only one flag need be set.
**	    Removed the ability to say "NOACNT" as a synonym for "ACNT". Even
**		though there are two privilege bitmask definitions in prvdef.h,
**		there's still only one "ACNT" privilege, and it's set by
**		specifying "ACNT".
**	    The maximum length for a string specified by TYPE_STR is 256; added
**		code to the TYPE_STR case to validate this. Introduced a new
**		MAX_STR_LEN manifest constant to replace the raw "256".
**	    Added some documentation about the "TYPE_*" definitions, since a
**		few of them goofed me up initially.
**	    Disambiguated some of the log_errmsg() info strings, so that the
**		message would indicate *which* SYS$CRMBX call failed (there are
**		several of them made).
**	    Reformatted PM variable table to fit on 80 column screens.
**	    Brought code in line with common DBMS group coding conventions.
**      22-Jan-1998 (horda03) Bug 68559.
**          When starting GCC servers, the server flavor must be specified
**          in the form "-instance=<server_flavor>", otherwise the GCC uses
**          the configuration defined for the default GCC.
**      22-feb-1998 (chash01)
**          RMCMD (Visual DBA backend server) startup takes two VMS CREPRC
**          calls, one ihere, one in rmcmd.exe.  When RMCMD server starts
**          the PID returned by CREPRC in this module is no longer valid.
**          WE have to first decode pid in the t_user_data field returned by
**          RMCMD server in termination  mailbox before print PID and 
**          server name to terminal.
**      24-may-99 (chash01)
**          oracle gateway startup need a definite location for 
**          sys$scratch, set it to ii_system:[ingres].
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	19-jul-2000 (kinte01)
**	    extraneous \ in STprintf statement. Found by 6.2 compiler
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	08-dec-2000 (kinte01)
**		Add impersonate privilege.  Impersonate replaces detach
**	06-mar-2001 (kinte01)
**		Add support for the multiple instances allowed for the
**		JDBC server
**	26-jun-2001 (kinte01)
**		Add EDBC support
**      09-aug-01, 21-aug-00 (chash01)
**          the same goes for rdb gateway, rdb gateway also needs a
**          definite location for sys$scratch.
**	28-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	12-Feb-2007 (bonro01)
**	    Remove JDBC package.
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**      20-mar-2009 (stegr01)
**          Add include <astjacket.h> to support posix threads on VMS on Itanium
*/

    /*
    ** Define event flags used for $QIO mailbox access to new process.
    */
# define TERM_EFN	1

# define MAX_STRING_OPTION_LENGTH   256

    /*
    ** Define type values for PM parameters. These constants are specified
    ** in the pm option table and used in the case table that parses the
    ** pm parameter values.
    */
# define TYPE_CLRFLG	0
# define TYPE_SETFLG	1
# define TYPE_INT	2
# define TYPE_UIC	3
# define TYPE_STR	4
# define TYPE_PQL	5
# define TYPE_PRIV	6
/*
** TYPE_CLRFLG: If the value given in config.dat matches the keyword in the
**		options table, CLEAR the flag in the target.
** TYPE_SETFLG: If the value given in config.dat matches the keyword in the
**		options table, SET the flag in the target.
** TYPE_INT:	Convert the value given in config.dat into an integer target.
** TYPE_UIC:	The value given in config.dat should be a User ID Code
** TYPE_STR:	The value given in config.dat is a string value. The value may
**		not exceed MAX_STRING_OPTION_LENGTH.
** TYPE_PQL:	The value given in config.dat is a quota value.
** TYPE_PRIV:	The value given in config.dat is a privileges list.
*/

    /* macros to allocate and initialize VMS descriptors
     */
# define $DESCALLOC( name ) struct dsc$descriptor_s \
	(name) = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL}

# define $DESCINIT( name, string ) \
	((void)((name).dsc$a_pointer = (char *)(string),\
	(name).dsc$w_length = STlength(string)))

# define $DESCFILL( name, size, string ) \
	((void)((name).dsc$a_pointer = (char *)(string),\
	(name).dsc$w_length = size))

# pragma member_alignment __save
# pragma nomember_alignment

    /* process quota block definitions
     */
struct pqldef
{
    unsigned char name;
    unsigned int  value;
};

# pragma member_alignment __restore

    /*
    ** PM parameter data area -- parameters initialized to
    ** default values.
    */
static char echo[ ] = "!.echo";
static const $DESCRIPTOR( image, "SYS$SYSTEM:LOGINOUT.EXE" );

static char *server_type   = "dbms";
static char *server_flavor = "*";
static char inbuf[MAX_STRING_OPTION_LENGTH+4] = "NLA0:";
static char errbuf[MAX_STRING_OPTION_LENGTH+4] = "NLA0:";
static char outbuf[MAX_STRING_OPTION_LENGTH+4] = "NLA0:";
static char dbms[MAX_STRING_OPTION_LENGTH+4] =
			"II_SYSTEM:[INGRES.BIN]IIDBMS.EXE";

static int msg_echo = 1;
static $DESCALLOC( input );
static $DESCALLOC( error );
static $DESCALLOC( output );
static unsigned int uic = 0;
static unsigned int baspri = 4;
static unsigned int stsflg = PRC$M_NOUAF;
static unsigned int prvadr[2] = { PRV$M_NETMBX | PRV$M_TMPMBX, 0 };
static struct pqldef quota[PQL$_LENGTH];

    /*
    ** Note that the "echo" parameter must be the first parameter in the
    ** table so that echoing can be turned off. In addition the NULL entry
    ** at the end of the table must be present.
    */
static const struct
{
    const char *const PM_name;
    void *const target;
    const char *const keyword;
    const unsigned int parameter;
    const int  type;
} pm_option[ ] = {

{ echo,				&msg_echo,  "OFF",1,		  TYPE_CLRFLG },
{ "!.vms_uic",			&uic,	    NULL, 0,	          TYPE_UIC },
{ "!.vms_image",		dbms,	    NULL, 0,		  TYPE_STR },
{ "!.vms_error",		errbuf,	    NULL, 0,		  TYPE_STR },
{ "!.vms_output",		outbuf,	    NULL, 0,		  TYPE_STR },
{ "!.vms_priority",		&baspri,    NULL, 0,		  TYPE_INT },
{ "!.vms_privileges",		prvadr,	    NULL, 0,		  TYPE_PRIV },
{ "!.vms_ast_limit",		quota,	    NULL, PQL$_ASTLM,     TYPE_PQL },
{ "!.vms_io_buffered",          quota,	    NULL, PQL$_BIOLM,     TYPE_PQL },
{ "!.vms_buffer_limit",         quota,	    NULL, PQL$_BYTLM,     TYPE_PQL },
{ "!.vms_io_direct",            quota,	    NULL, PQL$_DIOLM,     TYPE_PQL },
{ "!.vms_file_limit",           quota,	    NULL, PQL$_FILLM,     TYPE_PQL },
{ "!.vms_page_file",            quota,	    NULL, PQL$_PGFLQUOTA, TYPE_PQL },
{ "!.vms_queue_limit",          quota,	    NULL, PQL$_TQELM,     TYPE_PQL },
{ "!.vms_maximum_working_set",  quota,	    NULL, PQL$_WSQUOTA,   TYPE_PQL },
{ "!.vms_working_set",          quota,	    NULL, PQL$_WSDEFAULT, TYPE_PQL },
{ "!.vms_enqueue_limit",        quota,	    NULL, PQL$_ENQLM,     TYPE_PQL },
{ "!.vms_extent",               quota,	    NULL, PQL$_WSEXTENT,  TYPE_PQL },
{ "!.vms_job_table_quota",      quota,	    NULL, PQL$_JTQUOTA,   TYPE_PQL },
{ "!.vms_swapping",		&stsflg,    "OFF",PRC$M_PSWAPM,   TYPE_SETFLG },
{ "!.vms_accounting",		&stsflg,    "OFF",PRC$M_NOACNT,   TYPE_SETFLG },
{ "!.vms_dump",			&stsflg,    "ON", PRC$M_IMGDMP,   TYPE_SETFLG },
{ NULL,				NULL,	    NULL, NULL,		  0 }

};

/*
**  Privilege table
*/
static const struct
{
    char	 *prv_name;
    unsigned int  prv_code_low;
    unsigned int  prv_code_hi;
} prv_table[ ] = {
	{ "ACNT",	PRV$M_NOACNT, 0 },
	{ "ALL",	-1, -1 }, 
	{ "ALLSPOOL",	PRV$M_ALLSPOOL, 0 },
	{ "ALTPRI",	PRV$M_SETPRI, 0 },
	{ "BUGCHK",	PRV$M_BUGCHK, 0 },
	{ "BYPASS",	PRV$M_BYPASS, 0 },
	{ "CMEXEC",	PRV$M_CMEXEC, 0 },
	{ "CMKRNL",	PRV$M_CMKRNL, 0 },
	{ "IMPERSONATE", PRV$M_IMPERSONATE, 0 },
	{ "DIAGNOSE",   PRV$M_DIAGNOSE, 0 },
	{ "DOWNGRADE",	0, PRV$M_DOWNGRADE >> 32 },
	{ "EXQUOTA",	PRV$M_EXQUOTA, 0 },
	{ "GROUP",	PRV$M_GROUP,  0 },
	{ "GRPNAM",	PRV$M_GRPNAM, 0 },
	{ "GRPPRV",	0, PRV$M_GRPPRV >> 32 },
	{ "LOG_IO",	PRV$M_LOG_IO, 0 },
	{ "MOUNT",	PRV$M_MOUNT,  0 },
	{ "NETMBX",	PRV$M_NETMBX, 0 },
	{ "NOALL",	0,	      0 },
	{ "OPER",	PRV$M_OPER,   0 },
	{ "PFNMAP",	PRV$M_PFNMAP, 0 },
	{ "PHY_IO",	PRV$M_PHY_IO, 0 },
	{ "PRMCEB",	PRV$M_PRMCEB, 0 },
	{ "PRMGBL",	PRV$M_PRMGBL, 0 },
	{ "PRMMBX",	PRV$M_PRMMBX, 0 },
	{ "PSWAPM",	PRV$M_PSWAPM, 0 },
	{ "READALL",	0, PRV$M_READALL >> 32 },
	{ "SECURITY",	0, PRV$M_SECURITY >> 32 },
	{ "SETPRV",	PRV$M_SETPRV, 0 },
	{ "SHARE",	PRV$M_SHARE,  0 },
	{ "SHMEM",	PRV$M_SHMEM,  0 },
	{ "SYSGBL",	PRV$M_SYSGBL, 0 },
	{ "SYSLCK",	PRV$M_SYSLCK, 0 },
	{ "SYSNAM",	PRV$M_SYSNAM, 0 },
	{ "SYSPRV",	PRV$M_SYSPRV, 0 },
	{ "TMPMBX",	PRV$M_TMPMBX, 0 },
	{ "UPGRADE", 	0, PRV$M_UPGRADE >> 32 },
	{ "VOLPRO", 	PRV$M_VOLPRO, 0 },
	{ "WORLD",  	PRV$M_WORLD, 0 },
	{ "DETACH",	PRV$M_DETACH, 0 },
	{ NULL,		0 } };

    /*
    ** DCL commands sent to new process. Certain commands have parameters
    ** substituted into them with STprintf.
    */
# ifdef EDBC
static char *start_commands[ ] = {
	{ "$ SET NOCONTROL_Y" },
	{ "$ SET NOCONTROL_C" },
	{ "$ SET NOON"        },
        { "$ DEFINE/JOB SYS$SCRATCH EDBC_ROOT:[EDBC]" },
	{ "$ iidbms :== $%s"  },
	{ "$ iidbms %s %s%s"  }, /* Bug 68559 */
	{ "$ LOGOUT"          },
	{ 0 }
};
#else
static char *start_commands[ ] = {
	{ "$ SET NOCONTROL_Y" },
	{ "$ SET NOCONTROL_C" },
	{ "$ SET NOON"        },
        { "$ DEFINE/JOB SYS$SCRATCH II_SYSTEM:[INGRES]" },
	{ "$ iidbms :== $%s"  },
	{ "$ iidbms %s %s%s"  }, /* Bug 68559 */
	{ "$ LOGOUT"          },
	{ 0 }
};
#endif
#define COMMAND_LINE_SYSSCRATCH  3
#define COMMAND_LINE_IMAGE_NAME  4
#define COMMAND_LINE_SERVER_TYPE 5


/*
** Forward declarations of static function:
*/
static STATUS iics_cvtuic(
		struct dsc$descriptor *uic_dsc,
		i4                *uic_ptr);

/*
** Name: log_errmsg	- logs a user and VMS error status to errlog.log
**
** Description:
**	This routine converts OpenVMS status code to an OpenVMS text
**	error message, then puts the specified message with ": status"
**	appended and then the VMS error message output to the errlog.log
**
** Inputs:
**	usrmsg		- user specified error text (24 characters)
**	code		- VMS status code
**	flg		- 0 send message to error.log only,
**			  1 write to standard out and to errlog.log
**
** Outputs:
**	None
**
** Returns:
**	void
**
** History:
**	22-nov-1993 (eml)
**	    Created to allow the use ot ERoptlog to write all error messages.
**	31-jan-1994 (bryanp)
**	    Even if SYS$GETMSG fails, report the "usrmsg" anyway.
*/
static void
log_errmsg( const char *usrmsg,
	    const unsigned int code,
	    const unsigned int flg )
{
    static $DESCALLOC( bufadr );

    char stsbuf[256];
    char msgbuf[256];
    unsigned short len;
    STATUS status;

    $DESCFILL( bufadr, sizeof( stsbuf ), stsbuf );
    status = sys$getmsg( code, &len, &bufadr, 0x0F, NULL );
    if ( !(status & 1) )
    {
	/*
	** "code" was not a valid VMS message status, or at least not one which
	** we could turn into message text. So just format it in hex. Assume
	** that the result is exactly eight characters long (does CVlx null
	** terminate its output? If so, could set len = STlength(stsbuf);).
	*/
	CVlx(code, stsbuf);
	len = 8;
    }

    STcopy( usrmsg, msgbuf );
    STcat( msgbuf, ": Status" );
    stsbuf[len] = '\0';

    ERoptlog( msgbuf, stsbuf );

    if ( flg )
    {
	SIprintf( "%s = %s\n", msgbuf, stsbuf );
	SIflush( stdout );
    }

    return;
}

/*
** Name: pmerr_func	    - error handling function for PM errors.
**
** Description:
**	This routine is called when a PM error is encountered. Typically, this
**	is some sort of syntax error revolving around the contents of
**	config.dat. This routine formats the error message and displays it to
**	the screen.
**
** Inputs:
**	err_number	    - Some specific error number.
**	num_arguments	    - number ofentries in the args array.
**	args		    - an array of error message arguments.
**
** Outputs:
**	NOne
**
** Returns:
**	VOID
**
** History:
**	22-nov-1993 (bryanp)
**	    Created for handling config.dat errors.
*/
static VOID
pmerr_func( STATUS err_number, i4 num_arguments, ER_ARGUMENT *args)
{
    STATUS	status;
    char	message[ER_MAX_LEN];
    i4		text_length;
    CL_ERR_DESC	sys_err;

    status = ERslookup( err_number, (CL_ERR_DESC*)0,
			    0,
			    (char *)NULL,
			    message,
			    ER_MAX_LEN,
			    (i4) -1,
			    &text_length,
			    &sys_err,
			    num_arguments,
			    args
			  );

    if ( status != OK )
    {
	SIprintf( "IIRUNDBMS: Unable to look up PM error %x\n", err_number );
    }
    else
    {
	SIprintf( "IIRUNDBMS: %.*s\n", text_length, message );
    }
    return;
}

/*
** Name: main			-main routine.
**
** Description:
**	This routine is the main control routine for iirundbms. Starting an
**	iidbms server consists of:
**	    1) Opening and reading the PM data
**	    2) Validating server arguments, converting to VMS internal format.
**	    3) Creating mailbox for communication with server.
**	    4) creating server processing.
**	    5) Sending server process its commands.
**	    6) Checking whether server startup succeeded or not.
**
**	iirundbms command line format is:
**	    iirundbms <server type> <server flavor>
**	where server type is something like "dbms", "recovery", "star", and
**	server flavor is something like "public", "nonames", "benchmark".
**
** Inputs:
**	argc			- number of arguments (should be 1, 2 or 3)
**	argv			- argument array.
**
** Outputs:
**	None
**
** Returns:
**	a VMS status code is returned.
**
** History:
**	31-jan-1994 (bryanp)
**	    Added comment header.
**      21-Jan-1998 (horda03) Bug 68559
**          For GCC servers where a server flavour has been specified,
**          command line to start the server must specify the server
**          flavour in the form; "-instance=<server_flavor>" in order
**          for the GCC to pickup the correct configuration details.
**      22-feb-1998 (chash01)
**          RMCMD (Visual DBA backend server) startup takes two VMS CREPRC
**          calls, one ihere, one in rmcmd.exe.  When RMCMD server starts
**          the PID returned by CREPRC in this module is no longer valid.
**          WE have to first decode pid in the t_user_data field returned by
**          RMCMD server in termination  mailbox before print PID and 
**          server name to terminal.
**      31-Aug-2007 (ashco01) Bug #113490 & Bug #119021.
**          Corrected detection of 'instance name' for GCB & GCD.
**      05-Dec-2007 (ashco01) Bug #119561.
**          Ensure that all Ingres back-end detached processes define the 
**          SYS$SCRATCH logical as this is referenced via II_TEMPORARY
**          when placing temporary DCL files.
**      10-Dec-2007 (ashco01) Bug #119561
**          Define SYS$SCRATCH within all detached processes.
*/
main( int argc, char **argv )
{
    static $DESCALLOC( prcnam );
    unsigned int pqlcnt = 0;
    $DESCALLOC( uicdsc );
    char *param;
    char prcbuf[16];
    ACCDEF2 accmsg;
    unsigned int pid;
    unsigned short chan;
    unsigned short term;
    unsigned int mbxunt;
    int dviitem;
    char buf[128], tmp[128];
    II_VMS_MASK_LONGWORD  efc_state;
    char	*log_start_commands;
    IOSB	iosb;
    ER_ARGUMENT dummy_arg;
    i4		i;
    STATUS status;
    i4          gcc = 0;
    i4          gcc_instance = 0;
    i4          gcb = 0;
    i4          gcb_instance = 0;
    i4          gcd = 0;
    i4          gcd_instance = 0;

	/*
	** setup the type and server_flavor parameters
	*/
    if ( argc >= 2 )
    {
	server_type = argv[1];

        /* Are we starting a GCB, GCC or GCD ? */

        if (STbcompare( server_type, 3, "gcc", 3, TRUE ) == 0) gcc++;
        if (STbcompare( server_type, 3, "gcb", 3, TRUE ) == 0) gcb++;
        if (STbcompare( server_type, 3, "gcd", 3, TRUE ) == 0) gcd++;
    }

    if ( argc >= 3 )
    {
	server_flavor = argv[2];

        /* Need to precede server_flavor with "-instance="
        ** if we're starting a GCB, GCC or GCD.
        */
        gcc_instance = gcc;
        gcb_instance = gcb;
        gcd_instance = gcd;
    }

	/*
	** initialize PM routines and setup the default
	** search parameters for config.dat
	*/
    status = PMinit( );
    if (status)
    {
	pmerr_func(status, 0, &dummy_arg);
	return (status);
    }

    switch( status = PMload((LOCATION *)NULL, pmerr_func) )
    {
	case OK:
	    /* Loaded sucessfully */
	    break;

	case PM_FILE_BAD:
	    /* syntax error */
	    if (status != FAIL)	/* As of Nov 1993, PM.H defines PM_FILE_BAD 1 */
		pmerr_func(status, (i4)0, &dummy_arg);
	    return (status);

	default: 
	    /* unable to open file */
	    if (status != FAIL)	/* FAIL is a useless status to report... */
		pmerr_func(status, (i4)0, &dummy_arg);
	    return (status);
    }

#ifdef EDBC
    PMsetDefault( 0, ERx( "ed" ) );
#else
    PMsetDefault( 0, ERx( "ii" ) );
#endif
    PMsetDefault( 1, PMhost( ) );
    PMsetDefault( 2, server_type );
    PMsetDefault( 3, server_flavor );

	/* read and process pm parameters
	 */
    for ( i = 0; pm_option[i].PM_name != NULL; i++ )
    {
	status = PMget( pm_option[i].PM_name, &param );
	if ( status != OK )
	    continue;

	switch ( pm_option[i].type )
	{
	    unsigned int *target;

	    case TYPE_CLRFLG:
		if ( STbcompare(param, 0, "off", 0, TRUE) != 0 &&
		     STbcompare(param, 0, "on", 0, TRUE)  != 0 )
		{
		    SIprintf("IIRUNDBMS: %s value must be ON or OFF\n",
				pm_option[i].PM_name);
		    SIflush(stdout);
		    log_errmsg( "Must be ON or OFF", SS$_BADPARAM, 1 );
		    return (SS$_BADPARAM);
		}

		target = (unsigned int *)pm_option[i].target;

		if ( STbcompare(param, 0, pm_option[i].keyword, 0, TRUE) == 0)
		     *target &= ~(int)pm_option[i].parameter;

		if ( msg_echo && (STscompare( pm_option[i].PM_name, 0, echo, 0 ) == 0) )
		     ERoptlog( pm_option[i].PM_name, param );

		break;

	    case TYPE_SETFLG:
		if ( STbcompare(param, 0, "off", 0, TRUE) != 0 &&
		     STbcompare(param, 0, "on", 0, TRUE)  != 0 )
		{
		    SIprintf("IIRUNDBMS: %s value must be ON or OFF\n",
				pm_option[i].PM_name);
		    SIflush(stdout);
		    log_errmsg( "Must be ON or OFF", SS$_BADPARAM, 1 );
		    return (SS$_BADPARAM);
		}

		target = (unsigned int *)pm_option[i].target;

		if ( STbcompare(param, 0, pm_option[i].keyword, 0, TRUE) == 0)
		     *target |= (int)pm_option[i].parameter;

		if ( msg_echo )
		     ERoptlog( pm_option[i].PM_name, param );

		break;

	    case TYPE_INT:
		target = (unsigned int *)pm_option[i].target;

		status = CVal( param, target );
		if (status)
		{
		    SIprintf("IIRUNDBMS: %s value must be an integer\n",
				pm_option[i].PM_name);
		    SIflush(stdout);
		    pmerr_func(status, 0, &dummy_arg);
		    return (SS$_BADPARAM);
		}

		if ( msg_echo )
		{
		    STprintf( buf, "%d", *target );
		    ERoptlog( pm_option[i].PM_name, buf );
		}

		break;

	    case TYPE_UIC:
	    {
		$DESCINIT( uicdsc, param );

		status = iics_cvtuic( &uicdsc, (char *)pm_option[i].target );

		if ( !(status & 1) )
		{
		    log_errmsg( "vms_uic invalid", status, 1 );
		    return (status);				/* B56811 */
		}

		if ( msg_echo )
		{
		    if ( *(unsigned int *)(pm_option[i].target) != 0 )
		    {
			STprintf( buf, "%s [%o,%o]", param,
				  ((uic >> 16) & 0xffff), (uic & 0xffff) );
			ERoptlog( pm_option[i].PM_name, buf );
		    }
		}

		break;
            }

	    case TYPE_STR:
		 if (STlength(param) > MAX_STRING_OPTION_LENGTH)
		 {
		    SIprintf("IIRUNDBMS: Max length for %s is %d\n",
				pm_option[i].PM_name, MAX_STRING_OPTION_LENGTH);
		    SIflush(stdout);
		    return (SS$_BADPARAM);
		 }

		 STcopy( param, (char *)pm_option[i].target );

		 if ( msg_echo )
		    ERoptlog( pm_option[i].PM_name, param );

	         break;

	    case TYPE_PQL:
		    /*
		    ** build VMS process quota block
		    */
		quota[pqlcnt].name = (char)pm_option[i].parameter;

		status = CVal( param, &quota[pqlcnt].value );
		if (status)
		{
		    SIprintf("IIRUNDBMS: %s value must be an integer\n",
				pm_option[i].PM_name);
		    SIflush(stdout);
		    pmerr_func(status, 0, &dummy_arg);
		    return (SS$_BADPARAM);
		}

		pqlcnt++;

		if ( msg_echo )
		    ERoptlog( pm_option[i].PM_name, param );

		break;

	    case TYPE_PRIV:
	    {
		char prvbuf[512];
		char *p, *q;
		i4 j;

		    /*
		    ** Remove white space then
		    ** convert the string to upper case, then 
		    ** remove leading and trailing parens
		    */
		if (STlength(param) >= sizeof(prvbuf))
		{
		    SIprintf("IIRUNDBMS: vms_privileges are too long\n");
		    SIprintf("    Actual length (%d) exceeds maximum (%d)\n",
				STlength(param), sizeof(prvbuf));
		    SIflush(stdout);
		    return (SS$_BADPARAM);
		}

		STcopy( param, prvbuf);
		STtrmwhite( prvbuf );
		CVupper( prvbuf );

		    /*
		    ** Scan the comma seperated privilege list and set the
		    ** privileges for each privileges keywork in the list.
		    */
		for ( p = prvbuf; p != 0 && *p != 0; p = q )
		{
		    if ( (q = STindex( p, ERx( "," ), 0 )) != NULL )
			*q++ = '\0';
		    else if ( (q = STindex( p, ERx( ")" ), 0 )) != NULL )
			*q++ = '\0';

		    if ( *p == '(' )
			p++;

		    for ( j = 0; prv_table[j].prv_name != NULL; j++ )
		    {
			if ( STscompare( p, 4, prv_table[j].prv_name, 4 ) == 0 )
			{
			    prvadr[0] |= prv_table[j].prv_code_low;
			    prvadr[1] |= prv_table[j].prv_code_hi;
			    if ( msg_echo )
				ERoptlog( pm_option[i].PM_name, prv_table[j].prv_name );
			    break;
			}
		    }
		    if (prv_table[j].prv_name == NULL)
		    {
			/*
			** We failed to find privilege "p" in our table
			*/
			SIprintf("IIRUNDBMS: Syntax error in privilege list\n");
			SIprintf("    Error near byte %d of string %s",
				    p - prvbuf, param);
			SIflush(stdout);
			ERoptlog("Unrecognized privilege:", p);
			return (SS$_BADPARAM);
		    }
		}

		break;
	    }

	    default:
		break;
	}
    }

	/*
	** create mailbox for passing commands to the new process.
	** Get the device name of the mailbox to to be used as input
	** stream for new process.
	*/
    status = sys$crembx( 0, &chan, 0, 0, 0, PSL$C_USER, NULL );
    if ( !(status & 1) )
    {
	log_errmsg( "SYS$CREMBX(1) failed", status, 1 );
	return ( status );
    }

    dviitem = DVI$_DEVNAM;
    $DESCFILL( input, sizeof( inbuf ), inbuf );
    status = lib$getdvi( &dviitem, &chan, NULL, NULL, &input, &input );
    if ( !(status & 1) )
    {
	log_errmsg( "LIB$GETDVI(1) failed", status, 1 );
	return ( status );
    }

	/*
	** Create the mailbox to receive the termination message, then
	** get the mailbox unit number to be passed to SYS$CREPRC
	*/
    status = sys$crembx( 0, &term, sizeof( ACCDEF2 ), 0, 0, PSL$C_USER, NULL );
    if ( !(status & 1) )
    {
	log_errmsg( "SYS$CREMBX(2) failed", status, 1 );
	return ( status );
    }

    dviitem = DVI$_UNIT;
    status = lib$getdvi( &dviitem, &term, NULL, &mbxunt, NULL );
    if ( !(status & 1) )
    {
	log_errmsg( "LIB$GETDVI(2) failed", status, 1 );
	return ( status );
    }

	/*
	** establish a process name using the installition code,
	** and the unid number of the termination mailbox.
	*/
    NMgtAt( "II_INSTALLATION", &param );
    if ( param && *param )
	STprintf( prcbuf, "IIRUN_%s_%d", param, mbxunt );
    else
	STprintf( prcbuf, "IIRUN_%d", mbxunt );

	/*
	** Initialize descriptors for $CREPRC call
	*/
    $DESCINIT( error, errbuf );
    $DESCINIT( output, outbuf );
    $DESCINIT( prcnam, prcbuf );
    quota[pqlcnt].name = PQL$_LISTEND;

	/* 
	** create the new process (with DCL) to run the Ingres
	** componet.
	*/
    status = sys$creprc( &pid,
			 &image,
			 &input,
			 &output,
			 &error,
			 prvadr,
			 pqlcnt ? &quota : NULL,
			 &prcnam,
			 baspri,
			 uic,
			 mbxunt,
			 stsflg  );

    if ( !(status & 1) )
    {
	log_errmsg( "Unable to create process", status, 1 );
	return ( status );
    }

	/*
	** Post a read to the termination mailbox
	*/
    status = sys$qio( TERM_EFN, term, IO$_READVBLK, NULL, NULL, NULL,
		      &accmsg, sizeof( ACCDEF2 ), 0, 0, 0, 0 );
    if ( !(status & 1) )
    {
	log_errmsg( "Termination Mailbox Err", status, 1 );
	return ( status );
    }

	/*
	** send compontent startup command to the new process
	*/
    NMgtAt("II_IIRUNDBMS_ECHO_CMDS", &log_start_commands);

    for ( i = 0; start_commands[i] != NULL; i++ )
    {
	status = sys$readef( TERM_EFN, &efc_state);
	if ( !(status & 1) )
	{
	    log_errmsg( "Error reading Event Flag", status, 1 );
	    return ( status );
	}

	if ( status == SS$_WASSET )
	    break;

        /*
        ** 24-may-99 (chash01)
        **     oracle gateway startup need a definite location for 
        **     sys$scratch, set it to ii_system:[ingres].
        ** 21-aug-00 (chash01)
        **     the same goes for rdb
        */
        if ( i == COMMAND_LINE_SYSSCRATCH )
        {
	    STcopy(start_commands[i], buf );
        }
	else if (i == COMMAND_LINE_IMAGE_NAME)
	    STprintf(buf, start_commands[i], dbms);
	else if (i == COMMAND_LINE_SERVER_TYPE)
	{
            /* If a GCB, GCC or GCD private server has been specified
            ** then preceed the server flavor with "-instance".
            */
            STprintf(buf, start_commands[i], server_type,
                     ( gcb_instance || gcc_instance || gcd_instance )
                     ? "-instance=" : "", server_flavor);
	}
	else
	    STcopy(start_commands[i], buf );

	if (log_start_commands && *log_start_commands)
	    SIprintf("%s\n", buf);

	status = sys$qiow(EFN$C_ENF, chan, IO$_WRITEVBLK|IO$M_NOW, &iosb,
			   NULL, NULL, buf, STlength( buf ), 0, 0, 0, 0 );

	if ( status & 1) status = iosb.iosb$w_status;
	if ( !(status & 1) )
	{
	    log_errmsg( "Unable to send start", status, 1 );
	    return ( status );
	}
    }

	/* wait for termination message
	 */
    sys$waitfr( TERM_EFN );
    sys$dassgn( chan );
    sys$dassgn( term );
    status = ((ACCDEF *)(&accmsg))->acc$l_finalsts & ~STS$M_INHIB_MSG;
    if ( ((ACCDEF *)(&accmsg))->acc$l_finalsts == 0 )
    {
        /*
        ** for RMCMD we can use save PID, must get from
        ** t_user_data field of termination mailbox.
        */
        if (STbcompare( server_type, 5, "rmcmd", 5, TRUE ) == 0)
        {
            char  *cpid = &accmsg.acc$t_user_data[0];

            for (i = 0; cpid[i] != ' '; i++);
            cpid[i++] = '\0';
            CVal(cpid, &pid); 
	    STprintf( buf, "Server process id" );
            STprintf( tmp, "%x : Server Name = %s",  
                      pid, &accmsg.acc$t_user_data[i]);
        }
        else
        { 
	    STprintf( buf, "Server process id" );
            STprintf( tmp, "%x : Server Name = %s",
                      pid, accmsg.acc$t_user_data );

        }
	SIprintf( "%s = %s\n", buf, tmp );
	SIflush( stdout );

	if ( msg_echo )
	    ERoptlog( buf, tmp );
    }
    else
    {
	if (status == SS$_NORMAL)
	{
	    /*
	    ** This indicates that the server started up, uncovered some sort
	    ** of problem, reported the problem to the error log (hopefully),
	    ** and shut itself down. In this case, the user needs to be notified
	    ** to check the error log to find the real reason why the server
	    ** failed to start. (B57101).
	    */
	    SIprintf("IIRUNDBMS: Server failed to start.\n");
	    SIprintf("    Consult II_CONFIG:ERRLOG.LOG for failure reason.\n");
	    SIflush(stdout);
	}
	else
	{
	    log_errmsg( "Unable to start server", status, 1 );
            if (STbcompare( server_type, 5, "rmcmd", 5, TRUE ) == 0)
            {
                SIprintf( "%s\n", accmsg.acc$t_user_data );
	        SIflush( stdout );
            }
	}
	/*
	**	Having reported status, return with all errors mapped to
	**	SS$_ABORT. The status code with which we return is sent to
	**	sys$exit by PCexit(), but some status code which we might have
	**	here are not really good things to send to sys$exit (for
	**	example, if you return "image file not found" in "status", then
	**	sys$exit tries to format "image file not found" for the user,
	**	but "image file not found" (0x388b2) requires a parameter (the
	**	image file name), which we do not pass, so sys$exit ends up
	**	formatting garbage all over the screen.)
	*/
	status = SS$_ABORT;
    }

    return ( status );
}

/*{
** Name: iics_cvtuic	- Convert string to VMS User Identification Code (UIC)
**
** Description:
**      This routine converts the string passed in to a longword binary UIC 
**      value.  UIC's can come in a number of forms, namely: 
**		[<octal_number>, <octal_number>]
**		[<string_identifier>, <string_identifier>]
**		[<string_identifier>]
**		<string_identifier>
**
** Inputs:
**      uic_dsc                         String descriptor passed in
**
** Outputs:
**      uic_ptr                         place to put output answer
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-Jun-1987 (fred)
**          Created.
**	23-Jul-1991 (rog)
**	    Changed sys$asctoid() to sys$getuai().  The former has problems
**	    trying to get certain UIC's from the UAF file.
**	31-Jul-1991 (rog)
**	    Fix above change: sys$getuai() works slightly differently
**	    than sys$asctoid().
**	02-Aug-1991 (rog)
**	    As it turns out, we need to use sys$getuai() when we are handed
**	    a username, and sys$asctoid() when we are handed an identifier.
**	31-jan-1994 (bryanp)
**	    Incorporated this routine from 6.4 code into the 6.5 version.
**	    As called in the 6.5 version, we don't need to null-terminate the
**		uic string upon entry -- it's already correctly null-terminated.
**	    Fixed B56810 by handling a zero length string specially.
*/
static STATUS
iics_cvtuic(
struct dsc$descriptor *uic_dsc,
i4                *uic_ptr)
{
    char                *uic_str = uic_dsc->dsc$a_pointer;
    STATUS		status;
    II_VMS_RIGHTS_ID	group;
    II_VMS_RIGHTS_ID	owner;
    int			count;
    char		c;
    ILE3		itmlst[] = {
			    { sizeof(owner), UAI$_UIC, &owner, 0},	
			    { 0, 0, 0, 0}
			};
    
    /*
    ** In the new world of PM configuration parameters, a user may set the
    ** vms_uic field to a string of length 0, thusly:
    **	    ii.*.dbms.*.vms_uic:
    ** In this case, we need to handle this as valid, and meaning "uic=0"
    */
    if (uic_dsc->dsc$w_length == 0)
    {
	*uic_ptr = 0;
	return (SS$_NORMAL);
    }

    for (;;)
    {
	if ((*uic_str != '[') && (*uic_str != '<'))
	{
	    /* This is the "username" case. */
	    status = sys$getuai(0, 0, uic_dsc, itmlst, 0, 0, 0);
	    *uic_ptr = owner;
	    break;
	}
	uic_str++;
	uic_dsc->dsc$a_pointer++;
	for (c = *uic_str, count = 0;
		(c != ',' && c != ']' && c != '>' && c != 0);
		c = *(++uic_str), count++)
	    ;
	uic_dsc->dsc$w_length = count;
	status = ots$cvt_to_l(uic_dsc, &group, sizeof(group), 0);
	if ((status & 1) == 0)
	{
	    status = sys$asctoid(uic_dsc, &group, 0);
	    if ((status & 1) == 0)
	    {
		break;
	    }
	}
	else
	{
	    group <<= 16;
	}
	if (c && c != ',')
	{
	    /* [ alpha ] or [ nnn ] */
	    *uic_ptr = group;
	    status = SS$_NORMAL;
	    break;
    	}
	uic_dsc->dsc$a_pointer = ++uic_str;
	for (count = 0;
		*uic_str && *uic_str != ']' && *uic_str != '>';
		uic_str++, count++)
	    ;
	uic_dsc->dsc$w_length = count;
	status = ots$cvt_to_l(uic_dsc, &owner, sizeof(owner), 0);
	if ((status & 1) == 0)
	{
	    status = sys$asctoid(uic_dsc, &owner, 0);
	    if ((status & 1) == 0)
	    {
		break;
	    }
	    if ((group & 0xffff0000) != (owner & 0xffff0000))
	    {
		status = SS$_IVIDENT;
		break;
	    }
	}
	*uic_ptr = ((group & 0xffff0000) | (owner & 0x0000ffff));
	return(SS$_NORMAL);
    }
    return(status);
}
