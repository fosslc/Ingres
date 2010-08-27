/*
**  Copyright (c) 1992, 2010 Ingres Corporation
**
**  Name: config.sc -- contains main() and shared functions for the INGRES 
**	forms-based configuration utility.
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	16-feb-93 (tyler)
**		Simplified the help facility by making the mapping from
**		resources to help file names driven by the contents of a
**		PM file (cbfhelp.dat).  I also added code to look up (in
**		config.dat) an OS-specific command-line which will check
**		system resource after configuration changes.
**	26-apr-93 (tyler)
**		Replaced PCcmdline() calls to "iisetres" and "iivalres"
**		with direct calls to CRsetPMval() and CRvalidate() in
**		order to eliminate the unecessary system calls.	
**	24-may-93 (tyler)
**		Added argument to PMmInit() call.  Changed third argument
**		to CRsetPMval() calls and fourth argument to CRvalidate()
**		calls to file pointers.
**	21-jun-93 (tyler)
**		Added code to maintains a change log.  Switched to new
**		interface for mutiple context PM functions.  Added first
**		attempt at a locking mechanism to prevent multiple
**		invocations of this utility from trashing the configuration
**		files.
**	23-jul-93 (tyler)
**		Fixed a forms-mode error message which wasn't being
**		diplayed in a form.  Created browse_file() function to
**		allow viewing of configuration constraint violations,
**		system resource shortages, or configuration changes.
**		Fixed bug which caused change log entry produced by
**		'Restore' operation to omit value being changed.
**		PMmInit() interface changed.
**	26-jul-93 (tyler)
**		lock_config_data() is now called to lock config.dat.
**		Changed ii.$.prefs.syscheck_config to ii.$.prefs.syscheck_cbf
**	04-aug-93 (tyler)
**		Modified name used to open config.log on VMS to include
**		version number.  Made sure unlock_config_data() gets
**		called on all normal exit conditions.
**	19-sep-93 (tyler)
**		Fixed bugs which prevented opening of change log on VMS.
**		Change syscheck message Id string.  CR_NO_RULES_ERR status
**		(returned by CRload()) is now handled.  Modified other
**		error status symbols.  Replaced hard-coded error
**		messages with ERget() calls.  Call PMhost() instead
**		of GChostname().  Changed names of resources which
**		control syscheck behavior.
**	22-nov-93 (tyler)
**		A timestamp is now written to config.log at the start
**		of each session.  Modified usage message Id.
**	01-dec-93 (tyler)
**		Fixed bad calls to IIUGerr().
**	31-jan-94 (tyler)
**		Fixed BUG 57858 which prevented cbf from launching if
**		protect.dat exists but contains no data.  The fix 
**		is actually a work-around to a BUG in the VMS
**		implementation of SIfopen() which prevents empty
**		(but existing) SI_TXT files from being opened for
**		appending.  Fixed BUG 59023 which prevented cbf
**		from running in VMS "production" installations
**		(where II_INSTALLATION is not defined).
**	22-feb-94 (tyler)
**		Fixed BUG 53377: set up error handler necessary to allow
**		cbf to remove config.lck file on abnormal exits.
**		Fixed BUG 55601: Don't allow null values for any
**		resource values.
**	09-mar-94 (tyler)
**		Fixed BUG 59846: cbf now works correctly when invoked
**		on a non-master node of a VMS VAXcluster.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	15-Nov-94 (nanpr01)
**              Handle the error when it occurs and cleanup.
**              Bug # 61196.
**	30-Nov-94 (nanpr01)
**              Make the keys consistent from screen to screen and user
**		defined.
**              Bug # 60371.
**	03-feb-95 (rudtr01)
**		Bug #65312 (part 1) - On VMS production (system-level)
**		installations, II_INSTALLATION is NOT used.  The default
**		value for the installation code is II in this case.  For
**		all other environments, II_INSTALLATION is required.
**      02-feb-95 (sarjo01) Bug 66333
**              reset_resource(): added call to CRresetcache() to
**              assure that newly restored value is recomputed in
**              CRcompute().
**      14-feb-1995 (wolf) 
**              Change new "if VMS" to "ifdef VMS" to avoid compiler warning.
**	25-may-1995 (sweeney)
**		Unlock config.lck if we bail when II_INSTALLATION isn't set.
**	16-jun-95 (emmag)
**		Define main as ii_user_main for NT.
**	26-jun-95 (sarjo01)
**		Use NT_launch_cmd to run syscheck.
**	01-aug-95 (sarjo01) Bugs 70242, 70251, 70248.
**	 	NT: use II_TEMPORARY to build temp file path. Cannot
**		count on a #defined path (eg. /tmp/ ) existing.
**	09-aug-95 (wonst02)
**		For NT, set a PCatexit routine to call unlock_config_data().
**	16-aug-95 (wonst02)
**		Do not check return code from PCatexit--UNIX and VMS are VOID.
**	07-sep-1995 (abowler)
**		Trim the blanks from user values, rather than remove all of
**		them (Bug 71021)
**	24-sep-95 (tutto01)
**		Restored main, run as console app on NT.
**	11-jan-96 (tutto01)
**		Removed the use of NT_launch_cmd, which is no longer needed.
**      07-mar-1996 (canor01)
**              Call CRpath_constraint() to get a pointer to the functions
**              to verify validity of pathnames.
**	06-jun-1996 (thaju02)
**	 	Modified prototype void CRsetPMval() to PTR CRsetPMval(),
**		where CRsetPMval() returns warning message pointer for
**		variable page size support.
**	10-oct-1996 (canor01)
**		Replace hard-coded 'ii' prefixes with SystemCfgPrefix.
**   	12-dec-1996 (reijo01)
**              Use SystemExecName( ing or jas) in strings %sstart
**              to get generic startup executable.
**      08-jan-1997 (funka01)
**              Added GLOBALDEF ingres_menu (FALSE ifdef JASMINE).
**	16-apr-1997 (funka01)
**		Updated Error call for S_ST0312_no_ii_system to use 
**		SystemLocationVariable
**	16-apr-1997 (funka01)
**		Hey we need this updated for S_ST0313 too. Using F_ERROR
**		instead
**      03-Oct-97 (fanra01)
**              If the return status from TEname is fail initialise the
**              first character of terminal to EOS.
**      02-Jul-1999 (hweho01)
**              Included uf.h header file, it contains the function     
**              prototype of IIUFlcfLocateForm(). Without the explicit
**              declaration, the default int return value will truncate 
**              a 64-bit address on ris_u64 platform. 
**	15-jun-2000 (somsa01)
**		Updated MING hint to have product prefix in front of
**		program name.
**	01-Feb-2000 (hayke02)
**		In set_resource() check that sym is >= 0 before checking
**		whether syscheck resources are affected. This can occur
**		if we edit a parameter that affects syscheck resource (eg.
**		ii.<name>.ingstart.*.gcc) and then edit a parameter with
**		no .crs entry (eg. ii.<name>.gcc.*.tcp_ip.port). This
**		prevents a SEGV in strcmp() called from IIUGhfHtabFind().
**		This change fixes bug 100287.
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Aug-2001 (wansh01) 
**          in set_resource()  allowed input 'value' to be NULL. so it
**          can handle the add and delete of the resource.  
**	05-sep-2002 (somsa01)
**	    For the 32/64 bit build,
**	    call PCexeclp64 to execute the 64 bit version
**	    if II_LP64_ENABLED is true.
**	11-sep-2002 (somsa01)
**	    Prior change is not for Windows.
**      25-Sep-2002 (hanch04)
**          PCexeclp64 needs to only be called from the 32-bit version of
**          the exe only in a 32/64 bit build environment.
**	05-Oct-2002 (devjo01)
**	    NUMA changes.
**	15-nov-2002 (chash01)
**              Add GLOBALDEF char *cbf_srv_name. It will be used in startup.sc
**              and generic.sc.  It will have one of three values (NULL,
**              "dbms", or "rms").  Its purpose is to differentiate rms from
**              dbms, so that we can have both dbms.cache.p4k_status and
**              rms.cache.p4k_status, for example, to be configured. 	
**	15-apr-2003 (hanch04)
**	    CRsetPMval take an extra parameter.  Pass FALSE to not print out
**	    changes to stdout.
**	    This change fixes bug 100206.
**	05-nov-2003 (kinte01)
**	    ifdef call to CS_map_sys_segment since it is not available on
**	    VMS and not applicable 
**	26-jan-2004 (penga03)
**	    Added "#ifdef UNIX" call to CS_map_sys_segment since it is not 
**	    available on VMS and WINDOWS.
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	17-Jun-2004 (schka24)
**	    Add some buffer-overrun avoidance.
**	07-Jul-2004 (gorvi01)
**	    BUG# 112615 addresses that CBF utilility comes out of user interface 
**	    and enters a new shell when configuration is not saved and 
**	    config.dat is restored back. Modification to make PCcmdLine point
**	    to the correct syscheck command when configuration is not saved 
**	    and config.dat is restored back.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries.
**      06-Jun-2005 (mutma03)
**	    Disabled NUMA support for linux to solve the issue which prevents
**	    the cbf access for hostnames ending with .ca.com 
**	30-Aug-2005 (drivi01)
**	    Bug #115129
**	    Modified routine that scans symbol string against name string
**	    of config parameter to determine wildcard replacements
**	    to start scanning with the 0th element instead of 3rd.
**	    When we scan starting with 3rd element, hostname never
**	    get identified with a first '$' causing the first search
**	    for the name in config.dat to fail.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added 4 header file includes to eliminate gcc 4.3 warnings.
**	15-Dec-2009 (frima01) Bug 122490
**	    Needed to make include of clconfig.h, csev.h and cssminfo.h
**	    dependent on OS_THREADS_USED to work on VMS.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**     27-Jul-2010 (frima01) Bug 124137
**         Evaluate return code of NMloc to avoid using corrupt pointers.
**
/*
PROGRAM =	(PROG0PRFX)cbf
**
NEEDLIBS =	CONFIGLIB UTILLIB SHFRAMELIB SHEMBEDLIB COMPATLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

# include <compat.h>
# include <gl.h>
# include <si.h>
# include <ci.h>
# include <st.h>
# include <me.h>
# include <lo.h>
# include <ex.h>
# include <er.h>
# include <cm.h>
# include <nm.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <ui.h>
# include <te.h>
# include <gc.h>
# include <gcccl.h>
# include <erst.h>
# include <uigdata.h>
# include <stdprmpt.h>
# include <pc.h>
# include <cs.h>
# include <cx.h>
# include <pm.h>
# include <pmutil.h>
# include <util.h>
# include <simacro.h>
#ifdef OS_THREADS_USED
# include <clconfig.h>
# include <csev.h>
# include <cssminfo.h>
#endif /* OS_THREADS_USED */
# include <id.h>
# include "cr.h"
# include "uf.h"

exec sql include sqlca;
exec sql include sqlda;
 
exec sql begin declare section;
GLOBALDEF char	*host; /* Host = Node, unless NUMA cluster Node */
GLOBALDEF char	*node;
GLOBALDEF char	*ii_system;
GLOBALDEF char	*ii_installation;
GLOBALDEF char	*ii_system_name;
GLOBALDEF char	*ii_installation_name;
GLOBALDEF char	tty_type[ MAX_LOC+1 ];
GLOBALDEF char  key_map_filename[ MAX_LOC + 1 ];
exec sql end declare section;

GLOBALDEF LOCATION	config_file;	
GLOBALDEF LOCATION	protect_file;	
GLOBALDEF LOCATION	units_file;	
GLOBALDEF LOCATION	change_log_file;	
GLOBALDEF LOCATION	key_map_file;	
GLOBALDEF FILE		*change_log_fp;
GLOBALDEF char		change_log_buf[ MAX_LOC + 1 ];	
GLOBALDEF PM_CONTEXT	*config_data;
GLOBALDEF PM_CONTEXT	*protect_data;
GLOBALDEF PM_CONTEXT	*units_data;
GLOBALDEF bool		server;
GLOBALDEF char		tempdir[ MAX_LOC ];
GLOBALDEF bool		ingres_menu=TRUE;
GLOBALDEF char          *cbf_srv_name = NULL;
GLOBALDEF char		Affinity_Arg[16];
GLOBALDEF char		*local_host;

static LOCATION		help_file;	
static PM_CONTEXT	*help_data;
static char		*syscheck_command = NULL;
static FILE		*config_fp;
static FILE		*protect_fp;
static char		cx_node_name_buf[CX_MAX_NODE_NAME_LEN+2];
static char		cx_host_name_buf[CX_MAX_HOST_NAME_LEN+2];

FUNC_EXTERN void startup_config_form();

void
display_error( STATUS status, i4  nargs, ER_ARGUMENT *er_args )
{
	i4 language;
	char buf[ BIG_ENOUGH ];
	i4 len;
	CL_ERR_DESC cl_err;

	if( ERlangcode( NULL, &language ) != OK )
		return;
	if( ERlookup( status, NULL, ER_TEXTONLY, NULL, buf, sizeof( buf ),
		language, &len, &cl_err, nargs, er_args ) == OK
	)
		F_PRINT( ERx( "%s\n" ), buf );
}

STATUS
copy_LO_file( LOCATION *loc1, LOCATION *loc2 )
{
	FILE *fp1, *fp2;
	char buf[ SI_MAX_TXT_REC + 1 ];
	bool loc1_ok = TRUE;

	if( SIfopen( loc1 , ERx( "r" ), SI_TXT, SI_MAX_TXT_REC, &fp1 ) != OK )
		loc1_ok = FALSE;

	if( SIfopen( loc2 , ERx( "w" ), SI_TXT, SI_MAX_TXT_REC, &fp2 ) != OK )
		return( FAIL );

	while( loc1_ok && SIgetrec( buf, sizeof( buf ), fp1 ) == OK )
		SIfprintf( fp2, ERx( "%s" ), buf ); 

	if( loc1_ok && SIclose( fp1 ) != OK )
		return( FAIL );

	if( SIclose( fp2 ) != OK )
		return( FAIL );

	return( OK );
}

# define MAX_ROW	20
# define MAX_COL	78

STATUS
browse_file( LOCATION *loc, char *text, bool end )
{
	exec sql begin declare section;	
	char	*title =  text;
	char	buf[ SI_MAX_TXT_REC + 1 ];
	i4	cur = 1;
	bool	top = TRUE;
	char	*form_name = ERx( "file_browser" );
	i4	record;
	exec sql end declare section;	
	i4	lines = 0;
	FILE	*fp;

	if( SIfopen( loc, "r", SI_TXT, SI_MAX_TXT_REC, &fp ) != OK )
		return( FAIL );

	if( IIUFgtfGetForm( IIUFlcfLocateForm(), form_name ) != OK )
	{
		FEendforms();
		EXdelete();
		F_ERROR( "\n\"%s\" form not found.\n\n", form_name );
	}

	exec frs inittable :form_name text read;

	exec frs display :form_name;

	exec frs initialize ( title = :title );

	exec frs begin;

		exec frs clear field text;

		while( SIgetrec( buf, SI_MAX_TXT_REC, fp ) == OK )
		{
			buf[ MAX_COL ] = EOS;
			exec frs loadtable :form_name text (
				line = :buf
			);
			++lines;
		}
		(void) SIclose(fp);

		if( end )
		{
			cur = lines;
			top = FALSE;
			exec frs scroll :form_name text to :cur; 
		}	

		if( lines <= MAX_ROW )
		{
			/* display just the 'End' menu item */
			while( TRUE )
			{
				exec frs redisplay;

				exec frs submenu;	

				exec frs activate menuitem
					:ERget( FE_End ), frskey3;
				exec frs begin;

					exec frs breakdisplay;

				exec frs end;
			}
		}
		else
		{
			while( TRUE )
			{
				exec frs redisplay;

				exec frs submenu;	

				exec frs activate menuitem
					:ERget( S_ST035D_scroll_up_menu ),
					frskey18;
				exec frs begin;

					if( top )
						--cur;
					else
					{
						cur -= MAX_ROW;
						top = TRUE;
					}	
					if( cur < 1 )
						cur = 1;

					exec frs scroll :form_name text to :cur;

				exec frs end;

				exec frs activate menuitem
					:ERget( S_ST035E_scroll_down_menu ),
					frskey12;
				exec frs begin;

					if( top )
					{
						cur += MAX_ROW;
						top = FALSE;
					}
					else
						++cur;
					if( cur > lines )
						cur = lines;

					exec frs scroll :form_name text to :cur;

				exec frs end;

				exec frs activate menuitem
					:ERget( S_ST0349_page_up_menu ), 
					frskey19;
				exec frs begin;

					if( top )
						cur -= MAX_ROW - 1;
					else
					{
						cur -= 2 * MAX_ROW - 2;
						top = TRUE;
					}
					if( cur < 1 )
						cur = 1;

					exec frs scroll :form_name text to :cur;

				exec frs end;

				exec frs activate menuitem
					:ERget( S_ST034A_page_down_menu ),
					frskey13;
				exec frs begin;

					if( top )
					{
						cur += 2 * MAX_ROW - 1;
						top = FALSE;
					}
					else
						cur += MAX_ROW - 1;
					if( cur > lines )
						cur = lines;

					exec frs scroll :form_name text
						to :cur;

				exec frs end;

				exec frs activate menuitem
					:ERget( S_ST034B_top_menu ),
					frskey17;
				exec frs begin;

					cur = 1;
					top = TRUE;
					exec frs scroll :form_name text
						to :cur;

				exec frs end;

				exec frs activate menuitem
					:ERget( S_ST034C_bottom_menu ),
					frskey11;
				exec frs begin;

					cur = lines;
					top = FALSE;
					exec frs scroll :form_name text
						to :cur; 

				exec frs end;

				exec frs activate menuitem
					:ERget( FE_End ), frskey3;
				exec frs begin;

					exec frs breakdisplay;

				exec frs end;
			}
		}

	exec frs end; 

	return( OK );
}

bool
set_resource( char *name, char *value )
{
	exec sql begin declare section;
	char in_buf[ BIG_ENOUGH ]; 
        char *constraint, *temp;
	char *warnmsg;
	exec sql end declare section;
	CL_ERR_DESC cl_err;
	STATUS status;
	LOCATION valid_msg_file;
	char valid_msg_buf[ MAX_LOC + 1 ];
	FILE *valid_msg_fp;
	LOCATION backup_data_file;
	char backup_data_buf[ MAX_LOC + 1 ];
	LOCATION backup_change_log_file;
	char backup_change_log_buf[ MAX_LOC + 1 ];
	char temp_buf[ MAX_LOC + 1 ];
	i4 sym;
	PM_SCAN_REC state;
	char *regexp, *scan_name, *scan_value;
	bool syscheck_required = FALSE;
        char tmp_buf[1024];


	exec frs message :ERget( S_ST0324_validating );

	/* prepare a LOCATION for validation output */
	STcopy( ERx( "" ), temp_buf );
	LOfroms( FILENAME, temp_buf, &valid_msg_file );
	LOuniq( ERx( "cacbfv" ), NULL, &valid_msg_file );
	LOtos( &valid_msg_file, &temp );
	STcopy( tempdir, valid_msg_buf );
	STcat( valid_msg_buf, temp );
# ifdef VMS
        STcat( valid_msg_buf, ERx( ".tmp" ) );
# endif /* VMS */
	LOfroms( PATH & FILENAME, valid_msg_buf, &valid_msg_file );

	if( SIfopen( &valid_msg_file, ERx( "w" ), SI_TXT, SI_MAX_TXT_REC,
		&valid_msg_fp ) != OK )
	{
		EXdelete();
		IIUGerr( S_ST0323_unable_to_open, UG_ERR_FATAL, 1,
			valid_msg_buf );
	}

	if( CRvalidate( name, value, &constraint, valid_msg_fp ) != OK )
	{
		SIclose( valid_msg_fp );
		(void) browse_file( &valid_msg_file,
			ERget( S_ST0325_constraint_violation ), FALSE );
		return( FALSE );
	}
	else
		SIclose( valid_msg_fp );
	(void) LOdelete( &valid_msg_file );

	exec frs redisplay;

	/* prepare a backup LOCATION for the data file */
	STcopy( ERx( "" ), temp_buf );
	LOfroms( FILENAME, temp_buf, &backup_data_file );
	LOuniq( ERx( "cacbfd2" ), NULL, &backup_data_file );
	LOtos( &backup_data_file, &temp );
	STcopy( tempdir, backup_data_buf ); 
	STcat( backup_data_buf, temp );
	LOfroms( PATH & FILENAME, backup_data_buf, &backup_data_file );
	
	/* prepare a backup LOCATION for the change log */
	STcopy( ERx( "" ), temp_buf );
	LOfroms( FILENAME, temp_buf, &backup_change_log_file );
	LOuniq( ERx( "cacbfc2" ), NULL, &backup_change_log_file );
	LOtos( &backup_change_log_file, &temp );
	STcopy( tempdir, backup_change_log_buf ); 
	STcat( backup_change_log_buf, temp );
	LOfroms( PATH & FILENAME, backup_change_log_buf,
		&backup_change_log_file );


	/* get name of syscheck program if available */
	STprintf( tmp_buf, ERx( "%s.*.cbf.syscheck_command" ),
	          SystemCfgPrefix );
	(void) PMmGet( config_data, tmp_buf, &syscheck_command );


	/* determine whether to run system resource checker */
	STprintf(tmp_buf, ERx("%s.$.cbf.syscheck_mode"), SystemCfgPrefix);
	sym = CRidxFromPMsym( name );
	if( syscheck_command != NULL && PMmGet( config_data,
		tmp_buf, &scan_value ) == OK &&
		ValueIsBoolTrue( scan_value ) && (sym >= 0))
	{
		/* check whether "syscheck" resources are affected */
		STprintf( in_buf, ERx( "%s.%s.syscheck.%%" ), 
			  SystemCfgPrefix, host );
		regexp = PMmExpToRegExp( config_data, in_buf );
		for( status = PMmScan( config_data, regexp, &state, NULL,
			&scan_name, &scan_value ); status == OK; status =
			PMmScan( config_data, NULL, &state, NULL, &scan_name,
			&scan_value ) )
		{
			i4 syscheck_sym = CRidxFromPMsym( scan_name );

			if( CRdependsOn( syscheck_sym, sym ) )
			{
				syscheck_required = TRUE;
				break;
			}
		}
	}
	
	if( syscheck_required )
	{
		/* write a backup of the data file */ 
		if( PMmWrite( config_data, &backup_data_file ) != OK )
		{
			IIUGerr( S_ST0326_backup_failed, 0, 0 ); 
			(void) LOdelete( &backup_data_file );
			return( FALSE );
		}
	
		/* write a backup of the change log */ 
		if( copy_LO_file( &change_log_file, &backup_change_log_file )
			!= OK ) 
		{
			IIUGerr( S_ST0326_backup_failed, 0, 0 ); 
			(void) LOdelete( &backup_change_log_file );
			return( FALSE );
		}
	}

	/* open change log file for appending */
	if( SIfopen( &change_log_file , ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&change_log_fp ) != OK )
	{
		EXdelete();
		IIUGerr( S_ST0323_unable_to_open, UG_ERR_FATAL, 1,
			change_log_buf );
	}

	exec frs message :ERget( S_ST0311_recalculating );

	/* set resources and write changes to config.log */
        if (warnmsg = CRsetPMval( name, value, change_log_fp, FALSE, TRUE ))
	    exec frs message :warnmsg with style=popup;

	/* close change log after appending */
	(void) SIclose( change_log_fp );

	if( PMmWrite( config_data, NULL ) != OK )
	{
		IIUGerr( S_ST0318_fatal_error, UG_ERR_FATAL, 1,
			ERget( S_ST0314_bad_config_write ) );
	}
	exec frs redisplay;

	/* call system resource checker if one exists */ 
	if( syscheck_required )
	{
		exec frs message :ERget( S_ST0316_cbf_syscheck_message );
		if( PCcmdline( NULL, syscheck_command, PC_WAIT, NULL,
			&cl_err ) != OK )
		{
			/* report syscheck failure */
			exec frs prompt (
				:ERget( S_ST0317_syscheck_failure ),
				:in_buf
			) with style = popup;
	
			if( *in_buf == 'y' || *in_buf == 'Y' ) 
			{
				PMmFree( config_data );

				(void) PMmInit( &config_data );

				PMmLowerOn( config_data );

				CRsetPMcontext( config_data );

				exec frs message
					:ERget( S_ST031B_restoring_settings );

				if( PMmLoad( config_data, &backup_data_file,
					NULL ) != OK )
				{
					IIUGerr( S_ST0315_bad_config_read,
						UG_ERR_FATAL, 0 );
				}
				/* rewrite config.dat */
				if( PMmWrite( config_data, NULL ) != OK )
				{
					IIUGerr( S_ST0314_bad_config_write,
						UG_ERR_FATAL, 0 );
				}

				/* delete change log */
				(void) LOdelete( &change_log_file );

				/* restore change log from backup */
				if( copy_LO_file( &backup_change_log_file,
					&change_log_file ) != OK )
				{
					(void) LOdelete(
						&backup_change_log_file );
					IIUGerr( S_ST0326_backup_failed,
						UG_ERR_FATAL, 0 );
				}

				/* delete backup of data file */
				(void) LOdelete( &backup_data_file );

				/* delete backup of change log */
				(void) LOdelete( &backup_change_log_file );

				return( FALSE );
			}
		}
		/* delete backup of data file */
		(void) LOdelete( &backup_data_file );

		/* delete backup of change log */
		(void) LOdelete( &backup_change_log_file );
	}
	return( TRUE );
}

bool
reset_resource( char *name, char **value,
	bool reset_derived )
{
	char resource[ BIG_ENOUGH ]; 
	char *symbol;
	i4 idx, len, i;
	char *old_value;

	if( reset_derived )
	{
		/* save old value in order to reset before set_resource */
		(void) PMmGet( config_data, name, &old_value );
		old_value = STalloc( old_value );
	}

	/* delete old value in order to pick up the default */
	PMmDelete( config_data, name );

	/* get rule symbol for resource */ 
	idx = CRidxFromPMsym( name );
	if (idx== -1)
	{
	    /*
	    ** Value not in CR, probably dynamically created.
	    ** For now use the name
	    */
	    symbol=name;
	}
	else 
	{
	    symbol = CRsymbol( idx );
	}
		
	/* set defaults as required by CRcompute() */
	len = PMmNumElem( config_data, symbol );
	for( i = 0; i < len; i++ )
	{
		if( STcompare( PMmGetElem( config_data, i, symbol ),
			ERx( "$" ) ) == 0
		)
			PMmSetDefault( config_data, i, PMmGetElem(
				config_data, i, name ) );
		else
			PMmSetDefault( config_data, i, NULL );
	}

	if(idx!= -1)
	{
	    /* recompute the resource if known */
            CRresetcache(); 
	    if( CRcompute( idx, value ) != OK )
	    {
		IIUGerr( S_ST0318_fatal_error, UG_ERR_FATAL, 1,
			ERget( S_ST032C_undefined_symbol ) );
	    }

	    /* copy contents of internal CRcompute() buffer */
	    *value = STalloc( *value );
	}

	if( reset_derived )
	{
		/* reset old value so change log entry is correct */
		PMmInsert( config_data, name, old_value ); 

		/* then call set_resource to update derived */
		return( set_resource( name, *value ) );
	}
	/* otherwise, just set the resource and reload */
	PMmInsert( config_data, name, *value );
	if( PMmWrite( config_data, NULL ) != OK )
	{
		IIUGerr( S_ST0318_fatal_error, UG_ERR_FATAL, 1,
			ERget( S_ST0314_bad_config_write ) );
	}
	return( TRUE );
}

bool get_help( char *param_id )
{
	char buf[ BIG_ENOUGH], *file, *title;

	/* look up help file name */
	STprintf( buf, ERx( "%s.file" ), param_id );
	if( PMmGet( help_data, buf, &file ) != OK )
		return( FALSE );

	/* look up help title */
	STprintf( buf, ERx( "%s.title" ), param_id );
	if( PMmGet( help_data, buf, &title ) != OK )
		return( FALSE );

	/* display help */
	FEhelp( file, title );
	return( TRUE );
}

void
session_timestamp( FILE *fp )
{
	char		host[ GCC_L_NODE + 1 ];
	char		*user;
	char		terminal[ 100 ]; 
	char		time[ 100 ]; 
	SYSTIME		timestamp;

	/* get user name */
	user = (char *) MEreqmem( 0, 100, FALSE, NULL );
	IDname( &user );

	/* get terminal id */
	if (TEname( terminal ) == FAIL)
        {
            terminal[0] = EOS;
        }

	/* get host name */
	GChostname( host, sizeof( host ) );

	/* get timestamp */
	TMnow( &timestamp );
	TMstr( &timestamp, time );

	SIfprintf( fp, "\n*********************" );
	SIfprintf( fp, "\n*** BEGIN SESSION ***" );
	SIfprintf( fp, "\n*********************\n" );
	SIfprintf( fp, ERx( "\n%-15s%s" ), "host:", host );
	SIfprintf( fp, ERx( "\n%-15s%s" ), "user:", user );
	SIfprintf( fp, ERx( "\n%-15s%s" ), "terminal:", terminal );
	SIfprintf( fp, ERx( "\n%-15s%s\n" ), "time:", time );
}

/*
** This routine unlocks the config.dat file at PCexit time.
*/
VOID
cbf_exit( VOID )
{
	unlock_config_data();
}

STATUS
cbf_handler( EX_ARGS *args )
{
	STATUS		FEhandler();

	unlock_config_data();
	return( FEhandler( args ) );
}

main( i4  argc, char *argv[] )
{
	EX_CONTEXT	context;
	STATUS		status;
	i4		info;
	CL_ERR_DESC	cl_err;
	char		config_buf[ MAX_LOC + 1 ];	
	char		protect_buf[ MAX_LOC + 1 ];	
	char		units_buf[ MAX_LOC + 1 ];	
	char		help_buf[ MAX_LOC + 1 ];	
	FILE		*mutex_fp;
	LOCATION	mutex_loc;
	char		mutex_locbuf[ MAX_LOC+ 1 ];
	char		*mutex_file;
	char		*name, *value;
	char		*ii_keymap;
	PM_SCAN_REC	state;
	char		*temp;   
#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH32)
	char		*lp64enabled;
#endif  /* hybrid */
	CX_NODE_INFO    *pni;
	i4		rad;
	i4		gotcontext = 0;

	exec sql begin declare section;
        char		tmp_buf[ BIG_ENOUGH ];
        char		locbuf[ BIG_ENOUGH ];
	exec sql end declare section;

	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	MEadvise( ME_INGRES_ALLOC );

#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH32)

	/*
	** Try to exec the 64-bit version
	*/
	NMgtAt("II_LP64_ENABLED", &lp64enabled);
	if ( (lp64enabled && *lp64enabled) &&
	   ( !(STbcompare(lp64enabled, 0, "ON", 0, TRUE)) ||
	     !(STbcompare(lp64enabled, 0, "TRUE", 0, TRUE))))
	{
	    PCexeclp64(argc, argv);
	}
#endif  /* hybrid */

# ifdef NT_GENERIC
        NMgtAt( ERx("II_TEMPORARY"), &temp );
        if( temp == NULL || *temp == EOS )
        {
            temp = ".";
        }
	STlcopy(temp, tempdir, sizeof(tempdir)-1-1);
	STcat(tempdir, ERx("\\"));
# else 
	STcopy(TEMP_DIR, tempdir);
# endif

	local_host = PMhost();

	/*
	** Call CXget_context with various flags to try and
	** determine users intent.   On a NUMA cluster node,
	** we may want to set a specific RAD, since we need
	** this context for transaction log operations.
	*/
	if ( OK == CXget_context( &argc, argv,
		CX_NSC_CONSUME|CX_NSC_SET_CONTEXT, 
		cx_node_name_buf, CX_MAX_NODE_NAME_LEN+1 ) )
	{
	    /*
	    ** Here on no '-rad'/'-node' & non-numa, or one of either
	    ** a local '-node' spec, or a '-rad'.
	    **
	    ** We're working on a local node, RAD context is set,
	    ** and any -node / -rad parameter has been eaten.
	    */
	    gotcontext = 1;
	}
	else if ( OK == CXget_context( &argc, argv, 
		     CX_NSC_CONSUME|CX_NSC_IGNORE_NODE|
		     CX_NSC_SET_CONTEXT, NULL, 0 ) )
	{
	    /*
	    ** Both a RAD and a NODE were specified, eat
	    ** the RAD, leave the node, and set NUMA context
	    ** based on RAD, then try another pass to
	    ** make sure we don't have an invalid node
	    ** specification.
	    */
	    gotcontext = 1;
	    if ( OK != CXget_context( &argc, argv,
		    CX_NSC_CONSUME|CX_NSC_NODE_GLOBAL|CX_NSC_REPORT_ERRS,
		    cx_node_name_buf, CX_MAX_NODE_NAME_LEN+1 ) )
	    {
		PCexit(FAIL);
	    }
	}
	    
	while (1)
	{
	    rad = argc;	/* use rad as work var */
	    if ( OK == CXget_context( &argc, argv,
		         CX_NSC_CONSUME|CX_NSC_NODE_GLOBAL,
			 tmp_buf, CX_MAX_NODE_NAME_LEN+1 ) &&
		 rad != argc )
	    {
		/*
		** Hmm, dicy.  -node parameter was provided,
		** but was directed against a remote node.
		** We can run on any local node, but none was
		** specified.
		*/
		STcopy( tmp_buf, cx_node_name_buf );
		if ( gotcontext )
		{
		    /*
		    ** No sweat, user was kind enough to explicitly
		    ** set NUMA context with 1st rad/local node param.
		    */
		    break;
		}

		/*
		** We can run on any local node, but none was
		** specified.  Search for any free.
		*/
		for ( rad = CXnuma_rad_count(); rad > 0; rad-- )
		{
		    if ( CXnuma_rad_configured( local_host, rad ) &&
			 OK == CXset_context(local_host, rad) 
#ifdef UNIX
			 && OK == CS_map_sys_segment(&cl_err)
#endif /*UNIX*/   
                    )
		    {
			/*
			** Found one.  Redundant CS_map_sys_segment
			** is harmless, since logic in ME_attach,
			** detects multiple attachments, and just
			** returns address previously assigned.
			*/
			gotcontext = 1;
			break;
		    }
		}
		break;
	    }

	    /*
	    ** Check for old-style cbf <nodename> syntax,
	    ** and reparse if needed.
	    */
	    if ( argc == 2 && argv[1] != tmp_buf )
	    {
		STprintf( tmp_buf, ERx( "-node=%s" ), argv[1] );
		argv[1] = tmp_buf;
		continue;
	    }

	    /* Already did reparse, or command options are hopeless */
	    break;
	}

	if ( !gotcontext )
	{
	    /*
	    ** No context was provided at all, or bad or
	    ** multiple rad and/or node specs were specified.
	    ** Call once more, and yell at the user.
	    */
	    (void)CXget_context( &argc, argv,
				 CX_NSC_STD_NUMA, NULL, 0 );
	    PCexit(FAIL);
	}

	if ( argc > 2 )
	{
	    ERROR( ERget( S_ST0427_cbf_usage ) );
	}

	/* get local host name */
	host = node = cx_node_name_buf;

#if 0 /* disable NUMA support to solve the issue with hostname
         getting translated back to network address */
	/*
	** Node name, and host name are identical, except in the case
	** of a NUMA node in an Ingres cluster.   In that case, 'host'
	** is the name of the machine configured with multiple NUMA
	** nodes, and 'node' is the name of a specific node.   'host' 
	** is used to obtain most of the parameters, but parameters
	** specific to the virtual node (such as log file specs), are
	** qualified by 'node'.
	*/
	if ( CXcluster_enabled() )
	{
	    pni = CXnode_info( node, 0 );
	    if ( NULL == pni )
	    {
		ERROR( ERget( S_ST0427_cbf_usage ) );
	    }
	    if ( pni->cx_host_name_l )
	    {
		STlcopy( pni->cx_host_name, cx_host_name_buf,
			 CX_MAX_HOST_NAME_LEN );
		host = cx_host_name_buf;
	    }

	    if ( CXnuma_cluster_configured() )
	    {
		pni = CXnode_info( CXnode_name(NULL), 0 );
		if ( NULL == pni )
		{
		    ERROR( ERget( S_ST0427_cbf_usage ) );
		}
		STprintf( Affinity_Arg, ERx( "-rad=%d" ), pni->cx_rad_id );
	    }
	}
#endif
	/* prepare LOCATION for config.dat */
	if (NMloc( ADMIN, FILENAME, ERx( "config.dat" ), &config_file ) != OK )
		PCexit(FAIL);
	LOcopy( &config_file, config_buf, &config_file );

	/* prepare LOCATION for protect.dat */
	if (NMloc( ADMIN, FILENAME, ERx( "protect.dat" ), &protect_file ) != OK )
		PCexit(FAIL);
	LOcopy( &protect_file, protect_buf, &protect_file );

	/* prepare LOCATION for cbfunits.dat */
	if (NMloc( FILES, FILENAME, ERx( "cbfunits.dat" ), &units_file ) != OK )
		PCexit(FAIL);
	LOcopy( &units_file, units_buf, &units_file );

	/* prepare LOCATION for cbfhelp.dat */
	if (NMloc( FILES, FILENAME, ERx( "cbfhelp.dat" ), &help_file ) != OK )
		PCexit(FAIL);
	LOcopy( &help_file, help_buf, &help_file );

	/* prepare LOCATION for config.log */
	if (NMloc( LOG, FILENAME, ERx( "config.log" ), &change_log_file ) != OK )
		PCexit(FAIL);
	LOcopy( &change_log_file, change_log_buf, &change_log_file );

	/* load cbfhelp.dat */
	(void) PMmInit( &help_data );
	status = PMmLoad( help_data, &help_file, NULL );
	if( status != OK )
	{
		F_ERROR( ERget( S_ST0319_unable_to_load ), help_buf );
	}

	/* load cbfunits.dat */
	(void) PMmInit( &units_data );
	status = PMmLoad( units_data, &units_file, NULL );
	if( status != OK )
	{
		F_ERROR( ERget( S_ST0319_unable_to_load ), units_buf );
	}

	/* load config.dat */
	(void) PMmInit( &config_data );
	PMmLowerOn( config_data );
	status = PMmLoad( config_data, &config_file, NULL );
	if( status != OK )
	{
		ERROR( ERget( S_ST0315_bad_config_read ) );
	}

	/* load protect.dat */
	(void) PMmInit( &protect_data );
	PMmLowerOn( protect_data );
	status = PMmLoad( protect_data, &protect_file, NULL );

	/* intialize configuration rules system */
	CRinit( config_data );

	status = CRload( NULL, &info );

	switch( status )
	{
		case CR_NO_RULES_ERR:
			SIfprintf( stderr, ERget( S_ST0418_no_rules_found ) );
			PCexit( FAIL );

		case CR_RULE_LIMIT_ERR:
			SIfprintf( stderr,
				ERget( S_ST0416_rule_limit_exceeded ), info );
			PCexit( FAIL );

		case CR_RECURSION_ERR:
			SIfprintf( stderr,
				ERget( S_ST0417_recursive_definition ),
				CRsymbol( info ) );
			PCexit( FAIL );
	}

	if( lock_config_data( ERx( "cbf" ) ) != OK )
		PCexit( FAIL );

	PCatexit( cbf_exit );

	if( EXdeclare( cbf_handler, &context ) != OK )
	{
		EXdelete();
		PCexit( FAIL );
	}

	/* user must have write access to config.dat */ 
	if( SIfopen( &config_file, ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&config_fp ) != OK )
	{
		EXdelete();
		IIUGerr( S_ST0323_unable_to_open, UG_ERR_FATAL, 1,
			config_buf );
	}
	(void) SIclose( config_fp );

	/* user must have write access to protect.dat */ 
	if( SIfopen( &protect_file, ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&protect_fp ) != OK )
	{
		if( SIfopen( &protect_file, ERx( "w" ), SI_TXT, SI_MAX_TXT_REC,
			&protect_fp ) != OK )
		{
			EXdelete();
			IIUGerr( S_ST0323_unable_to_open, UG_ERR_FATAL, 1,
				protect_buf );
		}
	}
	(void) SIclose( protect_fp );

	/* open change log file for appending */
	if( SIfopen( &change_log_file , ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&change_log_fp ) != OK )
	{
		EXdelete();
		IIUGerr( S_ST0323_unable_to_open, UG_ERR_FATAL, 1,
			change_log_buf );
	}

	session_timestamp( change_log_fp );

	/* close change log after appending */
	(void) SIclose( change_log_fp );

	if (FEforms() != OK)
        {
          EXdelete();
          SIflush(stderr);
          PCexit(FAIL);
        }

	/* New function key maps for CBF */
	NMgtAt( ERx( "II_CBF_KEYS" ), &ii_keymap);
        if( ii_keymap == NULL || *ii_keymap == EOS )
	{
		/* prepare LOCATION for default key files */
		exec frs inquire_frs frs(:tty_type = terminal);
		if (STbcompare(tty_type, 2, ERx("vt"), 2, TRUE) == 0) {
			if (NMloc( FILES, FILENAME, ERx( "cbf.map" ), 
				&key_map_file) != OK )
				PCexit(FAIL);
			STcopy(key_map_file.string,key_map_filename);
			exec frs set_frs frs (mapfile = :key_map_filename);
		}

	}
	else
	{
		STlcopy(ii_keymap,key_map_filename, sizeof(key_map_filename)-1);
		exec frs set_frs frs (mapfile = :key_map_filename);
	}
	exec frs set_frs frs ( outofdatamessage = 0 );

	/* get name of syscheck program if available */
	STprintf( tmp_buf, ERx( "%s.*.cbf.syscheck_command" ),
	          SystemCfgPrefix );
	(void) PMmGet( config_data, tmp_buf, &syscheck_command );

	PMmSetDefault( config_data, 1, host );

	STprintf(tmp_buf, ERx( "%s.$.cbf.syscheck_mode" ), SystemCfgPrefix);
	if( syscheck_command != NULL && PMmGet( config_data,
		tmp_buf, &value ) == OK &&
		ValueIsBoolTrue( value ) )
	{
		/*
		** ### FIX ME
		** If the Name Server is running, syscheck should be
		** disabled and a reasonable message should be displayed
		** explaining that there is no point in checking system
		** resources, since the running system has eaten some
		** amount of them. 
		*/ 

		exec frs message :ERget( S_ST0316_cbf_syscheck_message );
		if( PCcmdline( NULL, syscheck_command, PC_WAIT, NULL,
			&cl_err ) != OK )
		{
			STprintf( tmp_buf, ERget( S_ST0327_syscheck_disabled ),
				  SystemProductName );
			exec frs message
				:tmp_buf
				with style = popup; 
			syscheck_command = NULL;
		}
	}

	/* determine whether this is server */
	STprintf(tmp_buf, ERx("%s.$.%sstart.%%.dbms"), SystemCfgPrefix, 
		     SystemExecName);
	if( PMmScan( config_data, PMmExpToRegExp( config_data, tmp_buf ), 
	             &state, NULL, &name, &value ) == OK ) 
	{
		server = TRUE; 
	}
	else
		server = FALSE;

	PMmSetDefault( config_data, 1, NULL );

        STprintf (locbuf, "%s_INSTALLATION", SystemVarPrefix);

        ii_installation_name = STalloc (locbuf);

        ii_system_name = STalloc (SystemLocationVariable);

	/* get value of II_SYSTEM */
	NMgtAt( SystemLocationVariable, &ii_system );
        if( ii_system == NULL || *ii_system == EOS
	  || STlength(ii_system) > MAX_LOC-1)
	{
		FEendforms();
		EXdelete();
		F_ERROR( ERget(S_ST0312_no_ii_system), SystemLocationVariable );
	}
	ii_system = STalloc( ii_system );
	
	/* get value of II_INSTALLATION */
	/* Rather arbitrary trim to prevent overruns */
	NMgtAt( ERx( "II_INSTALLATION" ), &ii_installation );
        if( ii_installation == NULL || *ii_installation == EOS
	  || STlength(ii_installation) > 10)
	{
#ifdef VMS
		/*
		** VMS system level installations do not
		** have II_INSTALLATION set, but have an 
		** internal installation code of ...
		*/
		ii_installation = ERx( "AA" );
#else
		/*
		** all other environments must have 
		** II_INSTALLATION set properly.
		*/
		FEendforms();
		EXdelete();
		F_ERROR( ERget( S_ST0313_no_ii_installation ), SystemVarPrefix );
#endif
	}
	else
		ii_installation = STalloc( ii_installation );

	startup_config_form();	

	exec frs clear screen;

	FEendforms();
	EXdelete();
	PCexit( OK );
}


/*
** Returns TRUE if value is a valid directory.  Prompts for
** creation if it doesn't exist.
*/
static bool
is_directory( char *value, bool *canceled )
{
        LOCATION        loc;
        i2              flag;
        char            tmp_val[ BIG_ENOUGH ];
        char            *ptv;
        char            *pv;

        *canceled = FALSE;

        LOfroms( PATH, value, &loc );

        if ( LOisdir( &loc, &flag ) != OK )
        {
                i4  choice;

                choice = IIUFccConfirmChoice( CONF_GENERIC,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,
                                              S_ST042C_no_directory,
                                              S_ST042D_create_dir,
                                              S_ST042E_no_create_dir,
                                              NULL,
                                              NULL );
                if ( choice == CONFCH_YES )
                {
                    if ( LOcreate( &loc ) != OK )
                        return ( FALSE );
                }
                else
                {
                    *canceled = TRUE;
                    return ( FALSE );
                }
        }
        else if ( !(flag & ISDIR) )
                return ( FALSE );

        return ( TRUE );
}


/*
** Returns TRUE if value is a valid file
*/
static bool
is_file( char *value )
{
        LOCATION        loc;
        i4              flag;
        LOINFORMATION   locinfo;

        LOfroms( PATH & FILENAME, value, &loc );

        flag = LO_I_TYPE;
        if( LOinfo( &loc, &flag, &locinfo ) != OK )
        {
                if ( LOcreate( &loc ) != OK )
                        return ( FALSE );
        }
        else if ( !( locinfo.li_type == LO_IS_FILE ) )
                return ( FALSE );

        return ( TRUE );
}

/* return pointer to functions that verify valid pathnames */
BOOLFUNC *
CRpath_constraint(CR_NODE *n, u_i4 constraint_id )
{
        PTR ptr;
	bool has_constraint = FALSE;

	if ( unary_constraint( n, constraint_id ) == TRUE )
	{
		if ( constraint_id == ND_DIRECTORY )
			return (is_directory);
		else
			return (is_file);
	}
	return NULL;
	
}
