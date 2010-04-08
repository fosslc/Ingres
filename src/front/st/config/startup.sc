/*
** Copyright (c) 1992, 2008 Ingres Corporation
**
**
**
**  Name: startup.sc -- contains functions associated with the startup
**	configuration form of the INGRES configuration utility.
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	16-feb-92 (tyler)
**		Changed "startup" to "ingstart" in PM function calls.
**	21-jun-93 (tyler)
**		Switched to new interface for multiple context PM
**		functions.
**	23-jul-93 (tyler)
**		Removed embedded strings.  Added ChangeLog menu item
**		which allows the user to browse the change log.  Fixed
**		bug which broke renaming/deletion/creation of DBMS
**		cache resources.  Whitespace is now stripped from
**		definition names input by the user.  Added 'Preferences'
**		menu item.
**	04-aug-93 (tyler)
**		Support name server configuration.  Reverse positions
**		of Preferences and ChangeLog menu choices.
**	19-oct-93 (tyler)
**		The "Startup Count" column now reflects the current
**		configuration of the transaction logs.  Cleaned up some
**		unnecessary code.  Fixed bug introduced in previous
**		integration which caused the copy_rename_nuke() to
**		crash.  Added "Security" menu item.  Changed type of
**		global variable "host" to (char *).  Changed "dmf" to
**		"dbms" in dbms cache resources names.  Changed
**		ii.$.dbms.$.$.config.connect to
**		ii.$.dbms.$.$.config.dmf_connect.
**	01-dec-93 (tyler)
**		Fixed bad calls to IIUGerr().
**	05-jan-94 (tyler)
**		Disable 'Security' menu on clients.
**	3-feb-94 (robf)
**              Update security handling, add frskey2 for Quit
**	22-feb-94 (tyler)
**		Fixed BUGS 58145, 58910, and 59101 -  all related to
**		handling of invalid configuration definition names.
**		Fixed BUG 54460: assign PF4 to Quit.
**	16-nov-94 (shust01)
**              fixed bug from ICL that has do with error message in CBF
**              when adding a number of servers, and the row is outside the
**              the range of the form.
**	30-nov-94 (nanpr01)
**              fixed bug # 60371. Added frskey mappings so that users 
**              can have uniform keyboard mapping for diff. CBF actions.
**	26-jun-95 (sarjo01)
**		Use NT_launch_cmd for running rcpstat so no console
**		windows created.
**	11-jan-96 (tutto01)
**		Removed use of NT_launch_cmd.  No longer needed.
**	07-Feb-96 (rajus01)
**		Added support for Protocol Bridge Server.
**	22-feb-96 (harpa06)
**		Added entry for Internet Communication.
**	06-jun-1996 (thaju02)
**		Added scan_priv_cache_parm() for variable page size support,
**		to copy/rename cache parameters when duplicating dbms server.
**      24-sep-96 (i4jo01)
**              Make Internet Communication entry available on client      
**              installations.
**	10-oct-1996 (canor01)
**		Replace hard-coded 'ii' prefixes with SystemCfgPrefix.
**      01-nov-1996 (hanch04)
**            Prevent changing startup count for Internet Communication 
**	      and name server
**      20-nov-1996 (canor01)
**		Fix typo in change from 10-oct-96 that left a STprintf()
**		with a trailing percent sign.
**      12-dec-1996 (reijo01)
**              Use SystemExecName( ing or jas) in strings %sstart
**              to get generic startup executable.
**      20-dec-1996 (reijo01)
**              Use global variables ii_system_name and ii_installation_name
**              to initialize forms.
**      26-dec-1996 (harpa06)
**              Removed ICE from CBF. ICE now has it's own configuration
**		program.
**      09-jan-1997 (funka01)
**              Use GLOBREF ingres_menu for setup of form.                   
**              Added Locking system parameters for Jasmine.
**	03-mar-96 (harpa06)
**		Added static ICE parameters back to CBF. They shouldn't have
**		been removed in the first place.
**	22-oct-97 (nanpr01)
**		bug 86531 : The intent of allowing _ was to have it embedded
**		between characters not to use it by itself or as a leading
**		or trailing name. This is now enforced in cbf.
**	27-apr-1998 (rigka01)
**		bug 90660 - add remote command to cbf on Windows NT
**	24-mar-98 (mcgem01)
**		Add configuration of the Informix, Sybase, Oracle, SQL Server
**		and ODBC gateways.
**	27-apr-1998 (rigka01)
**		bug 90660 - add remote command to cbf on Windows NT
**      03-jun-98 (mcgem01)
**              Remove the Duplicate configuration option for DBMS server
**              when running the Workgroup Edition of Ingres II.
**	03-jul-98 (ainjo01)
**		Reorder the components in the startup frame, so that the 
**		DBMS components are grouped, and then followed by Net server
**		and the gateways.   Also, make the rmcmd configuration
**		generic, since it applies across all platforms.
**	03-jun-98 (mcgem01)
**		Remove the Duplicate configuration option for DBMS server
**		when running the Workgroup Edition of Ingres II.
**	21-jul-98 (nicph02)
**		Bug 91358: Recovery server was not configurable anymore
**		from CBF!!! Cleaned up some source code.
**	26-Jul-98 (gordy)
**		Added component for GCF/GCS security.
**	26-Aug-1998 (hanch04)
**		Finish making rmcmd generic.
**      22-Sep-98 (fanra01)
**              Add ice server configuration.
**	19-nov-1998 (mcgem01)
**		It is now possible to set the startup count for ICE and the
**		gateways so that these can be switched off when not required.
**		Like rmcmd, these components have a maximum startup count of 1.
**		The history of CBF changes and the preferences menus should 
**		always be available, for some reason rmcmd was switching 
**		these off.  
**	04-dec-1998 (mcgem01)
**		The naming convention used for the gateways has been 
**		modified so that it's now ii.<machine>.gateway.oracle.blah
**		instead of ii.<machine>.oracle.*.blah.   CBF had to be 
**		modified slightly to accomodate this.
**	10-aug-1999 (kinte01)
**		Add configuration information for the RMS (98235) & 
**		RDB (98243) gateways
**	22-Feb-00 (rajus01)
**		Support JDBC Server configuration.
**      29-jun-2000 (hanje04)
**              Exempted Linux (int_lnx) from WORKGROUP exclusion of 
**              multi server startup
**	16-aug-2000 (somsa01)
**		Added ibm_lnx to above change.
**      17-jan-2002 (loera01) SIR 106858
**              Always display the Security menu.  Both clients and servers 
**              now configure security options.
**      21-Feb-2002 (bolke01)
**              Moved the open of the change log below the test for 
**              a valid name.  The returns from this section caused
**              a crash of CBF if no value was specified twice!
**              (bug 102964 )
**      14-jun-2002 (mofja02) 
**              Added support for DB2 UDB.
**      23-jan-2003 (chash01) integrate bug109055 fix
**              modify code to allow RMS configuration to have menu items 
**              (Cache) and (Derived) by setting cbf_srv_name (NULL, dbms, or
**              rms) and by replacing certain hard coded "dbms" with owner.
**	27-Feb-2003 (wansh01)
**		Added support for DAS server. 
**	08-Aug-2003 (drivi01)
**		Disabled the functions associated with keys that are mapped to
**		inactive menu items when menu items are not displayed on the
**		menu bar.
**	07-Apr-2004 (vansa02)
**		Bug:112041
**		While duplicating, cbf_srv_name is initialized to "dbms" if the
**		component type is DBMS server.cbf_srv_name is already is initial
**		ized to "dbms" during configuring if the component type is DBMS
**		server. This bug araised when DBMS server is duplicated and then
**		configured as cbf_srv_name remains NULL while duplicating.
**	13-apr-2004 (abbjo03)
**	    When returning from updating a Transaction Log, we cannot use a
**	    calculated (dataset) row number with putrow, which wants a
**	    displayed row number (or the current row by default.
**	05-Jun-2004 (hanje04)
**	    Remove stray ++row which was left in after previous change removed
**	    all other trace of it.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	16-Aug-2004 (rajus01) Bug # 112832, Problem #INGNET 146
**	    Ingres Bridge server duplicates only the inbound_limit parameter 
**	    when duplicate menu command option is chosen from CBF main screen.
**	18-feb-2005 (devjo01)
**	    Replace S_ST0333_private & S_ST0332_shared with hard coded
**	    english strings, as config.dat parameters should not be
**	    localized.   Correct compiler warnings.
**      13-feb-2006 (loera01) Bug 115533
**          In copy_rename_nuke(), added missing code required to create a
**          named DAS server.  After writing the DAS entries, a warning popup
**          advises the user to re-configure the startup address.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	12-Feb-2007 (bonro01)
**	    Remove JDBC package.
**      05-Jun-2008 (Ralph Loen) SIR 120457
**          Added format_port_rollups() to configure new port syntax for
**          TCP/IP.
**      16-Oct-1008 (Ralph Loen) SIR 120457
**          Bumped up S_ST06BD, S_ST06BE and S_ST06BF to S_ST06BF, S_ST06C0 and
**          S_ST06C1, resectively, to resolve potential cross-integration
**          issues from the ingres2006r3 code line.
**      26-May-2009 (hweho01) Bug 122109
**          Avoid SEGV in CRsetPMval(), setup a valid handler for the config
**          change log in format_port_rollups(). 
**	26-Aug-2009 (kschendel) b121804
**	    generic-independent-form is bool, declare it as such here even
**	    if the return value is never used.
*/

# include	<compat.h>
# include	<si.h>
# include	<ci.h>
# include	<cv.h>
# include       <qu.h>
# include	<st.h>
# include	<me.h>
# include	<lo.h>
# include	<ex.h>
# include	<er.h>
# include	<cm.h>
# include	<nm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <gcccl.h>
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>
# include	<te.h>
# include	<erst.h>
# include	<uigdata.h>
# include	<stdprmpt.h>
# include	<pc.h>
# include       <cs.h>
# include       <cx.h>
# include	<pm.h>
# include	<pmutil.h>
# include	<util.h>
# include	<simacro.h>
# include	"cr.h"
# include	"config.h"

exec sql include sqlca;
exec sql include sqlda;

/*
** global variables
*/
exec sql begin declare section;
GLOBALREF char		*host;
GLOBALREF char		*ii_system;
GLOBALREF char		*ii_system_name;
GLOBALREF char		*ii_installation_name;
GLOBALREF char		*ii_installation;
exec sql end declare section;

GLOBALREF char          *node;
GLOBALREF char          *local_host;
GLOBALREF bool		server;
GLOBALREF PM_CONTEXT	*config_data;
GLOBALREF LOCATION	change_log_file;
GLOBALREF FILE		*change_log_fp;
GLOBALREF char		*change_log_buf;
GLOBALREF bool		ingres_menu;
GLOBALREF char          *cbf_srv_name;

/*
** local form name declarations
*/
exec sql begin declare section;
static char *startup_config = ERx( "startup_config" );
exec sql end declare section;

/*
** external functions
*/
FUNC_EXTERN bool generic_independent_form( char *, char *, char *, char * );
FUNC_EXTERN u_i2 transaction_logs_form( char * );

void scan_priv_cache_parm(char *, char *, char *, bool);
void format_port_rollups ( char *serverType, char *instance, char *countStr);
 
/*
** copy_rename_nuke() copies, renames or destroys a component definition.
*/
static void
copy_rename_nuke( char *new_name, bool keep_old )
{
	exec sql begin declare section;
	char comp_type[ BIG_ENOUGH ];
	char comp_name[ BIG_ENOUGH ];
	char *new = new_name;
	i4 old_row = -1;
        char *gcd_warnmsg = "Duplicate DAS server has the same network listen addresses as the default DAS server.  Please select the 'Configure->Protocols' menu to configure a unique address";
	exec sql end declare section;

	PM_SCAN_REC state;
	STATUS status, insert_status;
	char resource[ BIG_ENOUGH ];
	char expbuf[ BIG_ENOUGH ];
	char *regexp, *last, *name, *value;
	char *owner, *instance, *param;
	bool is_dbms = FALSE;
	bool is_rms = FALSE;
	bool is_net = FALSE;
	bool is_bridge = FALSE;
	bool is_das  = FALSE;
	bool private_cache = FALSE;
	char *cp;

	
	/* strip whitespace */
	if( new_name != NULL )
	{
		bool	first;
		char	lastchar;

		STzapblank( new_name, new_name );

		/* abort if no input */
		if( *new_name == EOS )
			return;

		/* abort if non-alphanumeric input (underbars are OK
		** only if they are in the middle) */
		first = 1; 
		for( cp = new_name; *cp != EOS; CMnext( cp ) )
		{
			if( !CMalpha( cp ) && !CMdigit( cp ) && *cp != '_' )
			{
				exec frs message
					:ERget( S_ST0419_invalid_name )
					with style = popup;
				return;
			}
			if ((first) && (*cp == '_' ))
			{
				exec frs message
					:ERget( S_ST0419_invalid_name )
					with style = popup;
				return;
			}
			if (first)
			  first = 0;
			lastchar = *cp;
		}
		if (lastchar == '_')
		{
			exec frs message
				:ERget( S_ST0419_invalid_name )
				with style = popup;
			return;
		}
	}

	/* open change log file for appending */
	if( SIfopen( &change_log_file , ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&change_log_fp ) != OK )
	{
		EXdelete();
		unlock_config_data();
		IIUGerr( S_ST0323_unable_to_open, UG_ERR_FATAL, 1,
			change_log_buf );
	}

	SIfprintf( change_log_fp, ERx( "\n" ) );

	exec frs getrow startup_config components (
		:comp_type = type,
		:comp_name = name
	);

	if( STcompare( comp_name,
		ERget( S_ST030F_default_settings ) ) == 0
	)
		instance = ERx( "*" );
	else
		instance = comp_name;

	if( STcompare( comp_type , ERget( S_ST0307_dbms_server ) )
		== 0 )
	{
		is_dbms = TRUE;	
		owner = ERx( "dbms" );
		cbf_srv_name = ERx ( "dbms" );
	}

	else if( STcompare( comp_type ,
		ERget( S_ST0308_net_server ) ) == 0 )
	{
		is_net = TRUE;
		owner = ERx( "gcc" );
	}
	else if( STcompare( comp_type ,
		ERget( S_ST0690_das_server) ) == 0 )
	{
		is_das = TRUE;
		owner = ERx( "gcd" );
	}
	else if( STcompare( comp_type ,
		ERget( S_ST052B_bridge_server) ) == 0 )
	{
		is_bridge = TRUE;
		owner = ERx( "gcb" );
	}

        else if( STcompare( comp_type ,
                ERget( S_ST055C_oracle_gateway) ) == 0 )
        {
                owner = ERx( "gateway" );
		instance = ERx( "oracle" );
        }

        else if( STcompare( comp_type ,
                ERget( S_ST055F_informix_gateway) ) == 0 )
        {
                owner = ERx( "gateway" );
		instance = ERx( "informix" );
        }

        else if( STcompare( comp_type ,
                ERget( S_ST0563_sybase_gateway) ) == 0 )
        {
                owner = ERx( "gateway" );
		instance = ERx( "sybase" );
        }

        else if( STcompare( comp_type ,
                ERget( S_ST0566_mssql_gateway) ) == 0 )
        {
                owner = ERx( "gateway" );
		instance = ERx( "mssql" );
        }

        else if( STcompare( comp_type ,
                ERget( S_ST0569_odbc_gateway) ) == 0 )
        {
                owner = ERx( "gateway" );
                instance = ERx( "odbc" );
        }

        else if( STcompare( comp_type ,
                ERget( S_ST0644_rms_gateway) ) == 0 )
        {
                is_rms = TRUE;
                owner = ERx( "rms" );
		cbf_srv_name = ERx ( "rms" );
        }

        else if( STcompare( comp_type ,
                ERget( S_ST0649_rdb_gateway) ) == 0 )
        {
                owner = ERx( "gateway" );
		instance = ERx( "rdb" );
        }

        else if( STcompare( comp_type ,
                ERget( S_ST0677_db2udb_gateway) ) == 0 )
        {
                owner = ERx( "gateway" );
		instance = ERx( "db2udb" );
        }

	else if( STcompare( comp_type ,
		ERget( S_ST0309_star_server ) ) == 0
	)
		owner = ERx( "star" );

	STprintf( expbuf, ERx( "%s.%s.%s.%s.%%" ), 
		  SystemCfgPrefix, host, owner, instance );

	regexp = PMmExpToRegExp( config_data, expbuf );

	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status = PMmScan(
		config_data, NULL, &state, last, &name, &value ) )
	{
		/* set 'last' to handle deletion during scan */
		last = name;

		/* extract parameter name */
		param = PMmGetElem( config_data, 4, name );

		/* destroy old resource if 'keep_old' not TRUE */ 
		if( !keep_old && PMmDelete( config_data, name ) == OK )
		{
			SIfprintf( change_log_fp, ERx( "DELETE %s\n" ),
				name );
			SIflush( change_log_fp );
		}

		/* insert resource if 'new' not NULL */ 
		if( new != NULL )
		{
			/* compose duplicate resource name */
			STprintf( resource, ERx( "%s.%s.%s.%s.%s" ), 
				  SystemCfgPrefix, host, owner, new, param );
			if( PMmInsert( config_data, resource, value ) == OK )
			{
				SIfprintf( change_log_fp,
					ERx( "CREATE %s: (%s)\n" ),
					resource, value );
				SIflush( change_log_fp );
			}
		}

		/* check for private DBMS cache */
		if( ( is_dbms || is_rms )&& STcompare( ERx( "cache_sharing" ),
			param ) == 0 && !ValueIsBoolTrue( value )
		)
			private_cache = TRUE;
	}

	/* scan Net protocol resources */
	if( is_net || is_bridge || is_das )
	{
		char *protocol;

		STprintf( expbuf, ERx( "%s.%s.%s.%s.%%.%%" ), 
			  SystemCfgPrefix, host, owner, instance );

		regexp = PMmExpToRegExp( config_data, expbuf );

		for( status = PMmScan( config_data, regexp, &state,
			NULL, &name, &value ); status == OK; status =
			PMmScan( config_data, NULL, &state, last, &name,
			&value ) )
		{
			/* set 'last' to handle deletion during scan */
			last = name;

			/* extract protocol name */
			protocol = PMmGetElem( config_data, 4, name );

			/* extract parameter name */
			param = PMmGetElem( config_data, 5, name );

			/* destroy old resource if 'keep_old' not TRUE */ 
			if( !keep_old && PMmDelete( config_data, name )
				== OK )
			{
				SIfprintf( change_log_fp,
					ERx( "DELETE %s\n" ),
					name );
				SIflush( change_log_fp );
			}

			/* insert resource if 'new' not NULL */ 
			if( new != NULL )
			{
				/* compose duplicate resource name */
				if( is_net )
				    STprintf( resource,
					  ERx( "%s.%s.gcc.%s.%s.%s" ),
					  SystemCfgPrefix, host, new, 
					  protocol, param );
                                 else if( is_das )
                                     STprintf( resource,
                                           ERx( "%s.%s.gcd.%s.%s.%s" ),
                                           SystemCfgPrefix, host, new,
                                           protocol, param );
				else if( is_bridge )
				    STprintf( resource,
					  ERx( "%s.%s.gcb.%s.%s.%s" ),
					  SystemCfgPrefix, host, new, 
					  protocol, param );
				
                                if( PMmInsert( config_data, resource,
                                       value ) == OK )
				{
					SIfprintf( change_log_fp,
						ERx( "CREATE %s: (%s)\n" ),
						resource, value );
					SIflush( change_log_fp );
				}
			} /* if( new != NULL ) */
		} /* for( status = PMmScan( config_data, regexp, &state... */
	} /* if( is_das || is_net || is_bridge ) */

	/* scan private DBMS cache resources */
	if( private_cache )
	{
		scan_priv_cache_parm(instance, new, NULL, keep_old);
		scan_priv_cache_parm(instance, new, ERx("cache"), keep_old);
	 	if(!ingres_menu) 
		scan_priv_cache_parm(instance, new, ERx("p2k"), keep_old);
		scan_priv_cache_parm(instance, new, ERx("p4k"), keep_old);
		scan_priv_cache_parm(instance, new, ERx("p8k"), keep_old);
		scan_priv_cache_parm(instance, new, ERx("p16k"), keep_old);
		scan_priv_cache_parm(instance, new, ERx("p32k"), keep_old);
		scan_priv_cache_parm(instance, new, ERx("p64k"), keep_old);

		/* compose hidden dmf connect resource name */
		STprintf( resource,
 			  ERx( "%s.%s.%s.private.%s.config.dmf_connect" ),
			  SystemCfgPrefix, host, owner, instance );
	}
	else
	{
		/* compose hidden dmf connect resource name */
		STprintf( resource,
			  ERx( "%s.%s.%s.shared.%s.config.dmf_connect" ),
			  SystemCfgPrefix, host, owner, instance );
	}

	/* delete hidden dmf connect resource */
	(void) PMmDelete( config_data, resource );

	if( !keep_old )
	{
		/* get current row number */
                exec frs inquire_frs table startup_config  (
			:old_row = rowno(components)
		);
		--old_row;

		/* compose name of existing startup resource name */
		STprintf( resource, ERx( "%s.%s.%sstart.%s.%s" ), 
			  SystemCfgPrefix, host, SystemExecName, instance,
			  owner );	

		/* delete it */
		if( PMmDelete( config_data, resource ) == OK )
		{
			SIfprintf( change_log_fp, ERx( "DELETE %s\n" ),
				resource );
			SIflush( change_log_fp );
		}

		/* delete tablefield row */
		exec frs deleterow startup_config components; 
	}

	if( new != NULL )
	{
		/* compose duplicate startup resource */
		STprintf( resource, ERx( "%s.%s.%sstart.%s.%s" ), 
			  SystemCfgPrefix, host, SystemExecName, new, owner );	

		/* insert it and update tablefield if necessary */
                if( ( insert_status = PMmInsert( config_data, resource,
                    is_das ? ERx( "1" ) : ERx( "0" ) ) ) == OK )
		{	
                        if (is_das)
                            SIfprintf( change_log_fp, ERx( "CREATE %s: (1)\n" ),
                                resource );
                        else
                            SIfprintf( change_log_fp, ERx( "CREATE %s: (0)\n" ),
                                resource );
			SIflush( change_log_fp );

			/* old_row > -1 if old row was deleted */
			if( old_row >= 0 )
			{
			    if (is_das)
                                    exec frs insertrow startup_config
					components :old_row
				(
					type = :comp_type,
					name = :new,
					enabled = :ERx( "1" )	
				); 
                             else
				exec frs insertrow startup_config
					components :old_row
				(
					type = :comp_type,
					name = :new,
					enabled = :ERx( "0" )	
				); 
			}
			else
			{
                            if (is_das)
				exec frs insertrow startup_config components (
					type = :comp_type,
					name = :new,
					enabled = :ERx( "1" )	
				); 
                             else
				exec frs insertrow startup_config components (
					type = :comp_type,
					name = :new,
					enabled = :ERx( "0" )	
				); 
			}
		}
                if (is_das && insert_status == OK)
                    exec frs message :gcd_warnmsg with style=popup;
	}

	/* close change log after appending */
	(void) SIclose( change_log_fp );
	
	/* write config.dat */
	if( PMmWrite( config_data, NULL ) != OK )
		IIUGerr( S_ST0318_fatal_error, UG_ERR_FATAL, 1,
			ERget( S_ST0314_bad_config_write ) );
}
 
STATUS do_csinstall(void);
STATUS call_rcpstat( char *arguments );
void do_ipcclean(void);


void
startup_config_form( void )
{
	exec sql begin declare section;
	char *name, *value;
	char comp_type[ BIG_ENOUGH ];
	char comp_name[ BIG_ENOUGH ];
	char enabled_val[ BIG_ENOUGH ];
	char in_buf[ BIG_ENOUGH ];
	char *enabled1;
	char *enabled2;
	i4 activated = 0;
	i4 curr_row;
	i4 max_row;
	char temp_buf[ BIG_ENOUGH ];
	char log_buf[ BIG_ENOUGH ];
	exec sql end declare section;

	LOCATION *IIUFlcfLocateForm();
	STATUS IIUFgtfGetForm();
	PM_SCAN_REC state;
	STATUS status;
	char *regexp;
	char expbuf[ BIG_ENOUGH ];
	char pm_id[ BIG_ENOUGH ];
        bool is_gcc = FALSE;
        bool is_das = FALSE;
	bool use_secure;

	if( IIUFgtfGetForm( IIUFlcfLocateForm(), startup_config ) != OK )
	{
		FEendforms();
		EXdelete();
		F_ERROR( "\nbad news: form \"%s\" not found.\n\n",
			(char *) startup_config );
	}

	exec frs inittable startup_config components read;

	exec frs display startup_config;

	exec frs initialize (
		host = :host,
		ii_installation = :ii_installation,
		ii_installation_name = :ii_installation_name,
		ii_system = :ii_system,
		ii_system_name = :ii_system_name
	);
	exec frs begin;

	exec frs loadtable startup_config components (
		type = :ERget( S_ST0300_name_server ),
		name = :ERget( S_ST030F_default_settings ),
		enabled = :ERx( "1" ) 
	);
        
	if(!ingres_menu) {
	exec frs loadtable startup_config components (
		type = :ERget( S_ST0304_locking_system ),
		name = :ERget( S_ST030F_default_settings ),
		enabled = :ERx( "1" ) 
	);
	}

	if( server )
	{
		/* scan DBMS definitions */
		STprintf( expbuf, ERx( "%s.%s.%sstart.%%.dbms" ), 
			  SystemCfgPrefix, host, SystemExecName );
		regexp = PMmExpToRegExp( config_data, expbuf );
		for( status = PMmScan( config_data, regexp, &state, NULL,
			&name, &value ); status == OK; status =
			PMmScan( config_data, NULL, &state, NULL, &name,
			&value ) )
		{
			name = PMmGetElem( config_data, 3, name );
			if( STcompare( name, ERx( "*" ) ) == 0 )
				name = ERget( S_ST030F_default_settings );

			exec frs loadtable startup_config components (
				name = :name,
				type = :ERget( S_ST0307_dbms_server ), 
				enabled = :value
			);
		}	
		/* examine transaction logs */

		{
                    STATUS      csinstall;

		    exec frs message :ERget( S_ST038A_examining_logs ); 

                    csinstall = do_csinstall();

                    if ( OK == call_rcpstat( ERx( "-exist -silent" ) ) &&
                         OK == call_rcpstat( ERx( "-enable -silent" ) ) )
                    {
                            enabled1 = ERx( "1" );
                    }
                    else
                            enabled1 = ERx( "0" );

                    if ( OK == call_rcpstat( ERx( "-exist -dual -silent" ) ) &&
                         OK == call_rcpstat( ERx( "-enable -dual -silent" ) ) )
                    {
                            enabled2 = ERx( "1" );
                    }
                    else
                            enabled2 = ERx( "0" );

                    if(OK == csinstall)
                    {
                        do_ipcclean();
                    }
                }

	}


	/* disable "Security" menu item if security auditing is not setup */ 
	STprintf( temp_buf, ERx( "%s.$.c2.%%" ), SystemCfgPrefix );
	regexp = PMmExpToRegExp( config_data, temp_buf );
	if( !server || 
		PMmScan( config_data, regexp, &state, NULL, &name, &value )
		!= OK )
	{
		use_secure=FALSE;
	}
	else
		use_secure=TRUE;

	/* check for ICE definition */
  	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.icesvr" ), SystemCfgPrefix,
            host,SystemExecName );
  	regexp = PMmExpToRegExp( config_data, expbuf );
  	if ( PMmScan( config_data, regexp, &state, NULL, &name, &value )
  		== OK )
  	{
            for( status = PMmScan( config_data, regexp, &state, NULL,
                &name, &value ); status == OK; status =
                PMmScan( config_data, NULL, &state, NULL, &name,
                &value ) )
            {
                name = PMmGetElem( config_data, 3, name );
                if( STcompare( name, ERx( "*" ) ) == 0 )
                    name = ERget( S_ST030F_default_settings );

                exec frs loadtable startup_config components (
                    name = :name,
                    type = :ERget( S_ST0578_ice_server ),
                    enabled = :value
                );
            }
  	}
  
	if( server )
	{
		/* scan Star definitions */
		STprintf( expbuf, ERx( "%s.%s.%sstart.%%.star" ), 
			  SystemCfgPrefix, host, SystemExecName );
		regexp = PMmExpToRegExp( config_data, expbuf );
		for( status = PMmScan( config_data, regexp, &state, NULL,
			&name, &value ); status == OK; status =
			PMmScan( config_data, NULL, &state, NULL, &name,
			&value ) )
		{
			name = PMmGetElem( config_data, 3, name );
			if( STcompare( name, ERx( "*" ) ) == 0 )
				name = ERget( S_ST030F_default_settings );

			exec frs loadtable startup_config components (
				name = :name,
				type = :ERget( S_ST0309_star_server ), 
				enabled = :value
			);
		}

		exec frs loadtable startup_config components (
			type = :ERget( S_ST0304_locking_system ), 
			name = :ERget( S_ST030F_default_settings ),
			enabled = :ERx( "1" ) 
		);

		exec frs loadtable startup_config components (
			type = :ERget( S_ST0303_logging_system ), 
			name = :ERget( S_ST030F_default_settings ),
			enabled = :ERx( "1" ) 
		);

		STprintf( log_buf, "%s_LOG_FILE", SystemVarPrefix );
		exec frs loadtable startup_config components (
			type = :ERget( S_ST0305_transaction_log ),
			name = :log_buf,
			enabled = :enabled1
		);

		STprintf( temp_buf, "%s_DUAL_LOG", SystemVarPrefix );
		exec frs loadtable startup_config components (
			type = :ERget( S_ST0305_transaction_log ),
			name = :temp_buf,
			enabled = :enabled2
		);

		exec frs loadtable startup_config components (
			type = :ERget( S_ST0301_recovery_server ),
			name = :ERget( S_ST030F_default_settings ),
			enabled = :ERx( "1" ) 
		);

		exec frs loadtable startup_config components (
			type = :ERget( S_ST0302_archiver_process ),
			name = :ERget( S_ST030A_not_configurable ),
			enabled = :ERx( "1" ) 
		);

                /* scan Remote Command definitions */
                STprintf( expbuf, ERx( "%s.%s.%sstart.%%.rmcmd" ),
                          SystemCfgPrefix, host, SystemExecName );
                regexp = PMmExpToRegExp( config_data, expbuf );
                status=PMmScan( config_data, regexp, &state, NULL,
                        &name, &value);
                if (status)
                    exec frs loadtable startup_config components (
                        type = :ERget( S_ST02FE_rmcmd ),
                        name = :ERget( S_ST030A_not_configurable ),
                        enabled = :ERx( "0" )
                    );
                else
                {
                    exec frs loadtable startup_config components (
                        type = :ERget( S_ST02FE_rmcmd ),
                        name = :ERget( S_ST030A_not_configurable ),
                        enabled = :value
                    );
                }

	}

        exec frs loadtable startup_config components (
            type = :ERget( S_ST034E_security_menu ), 
            name = :ERget( S_ST030F_default_settings ),
            enabled = :ERx( "1" ) 
            );

	/* scan Net definitions */
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.gcc" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = ERget( S_ST030F_default_settings );

		exec frs loadtable startup_config components (
			name = :name,
			type = :ERget( S_ST0308_net_server ), 
			enabled = :value
		);
	}

	/* scan das server definitions */
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.gcd" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = ERget( S_ST030F_default_settings );

		exec frs loadtable startup_config components (
			name = :name,
			type = :ERget( S_ST0690_das_server ), 
			enabled = :value
		);
	}

	/* scan Bridge definitions */
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.gcb" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = ERget( S_ST030F_default_settings );

		exec frs loadtable startup_config components (
			name = :name,
			type = :ERget( S_ST052B_bridge_server ), 
			enabled = :value
		);
	}	

	/* 
	** Scan Oracle Gateway definitions 
	*/
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.oracle" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = ERget( S_ST030F_default_settings );

		exec frs loadtable startup_config components (
			name = :name,
			type = :ERget( S_ST055C_oracle_gateway ), 
			enabled = :value
		);
	}	

	/* 
	** Scan Informix Gateway definitions 
	*/
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.informix" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = ERget( S_ST030F_default_settings );

		exec frs loadtable startup_config components (
			name = :name,
			type = :ERget( S_ST055F_informix_gateway ), 
			enabled = :value
		);
	}	

	/* 
	** Scan Sybase Gateway definitions 
	*/
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.sybase" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = ERget( S_ST030F_default_settings );

		exec frs loadtable startup_config components (
			name = :name,
			type = :ERget( S_ST0563_sybase_gateway ), 
			enabled = :value
		);
	}	

	/* 
	** Scan SQL Server Gateway definitions 
	*/
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.mssql" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = ERget( S_ST030F_default_settings );

		exec frs loadtable startup_config components (
			name = :name,
			type = :ERget( S_ST0566_mssql_gateway ), 
			enabled = :value
		);
	}	

	/* 
	** Scan ODBC Gateway definitions 
	*/
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.odbc" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = ERget( S_ST030F_default_settings );

		exec frs loadtable startup_config components (
			name = :name,
			type = :ERget( S_ST0569_odbc_gateway ), 
			enabled = :value
		);
	}	

	/* scan RMS definitions */
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.rms" ), 
		SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = ERget( S_ST030F_default_settings );

			exec frs loadtable startup_config components (
			name = :name,
			type = :ERget( S_ST0644_rms_gateway ), 
			enabled = :value
		);
	}
	
	/* 
	** Scan RDB Gateway definitions 
	*/
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.rdb" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = ERget( S_ST030F_default_settings );

		exec frs loadtable startup_config components (
			name = :name,
			type = :ERget( S_ST0649_rdb_gateway ), 
			enabled = :value
		);
	}	

	/* 
	** Scan DB2UDB Gateway definitions 
	*/
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.db2udb" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = ERget( S_ST030F_default_settings );

		exec frs loadtable startup_config components (
			name = :name,
			type = :ERget( S_ST0677_db2udb_gateway ), 
			enabled = :value
		);
	}	

	exec frs end;

	exec frs activate menuitem :ERget( S_ST0350_configure_menu ),frskey11;
	exec frs begin;

		char *arg1, *arg2;
		
		exec frs getrow startup_config components (
			:comp_type = type,
			:comp_name = name
		);
		if( STcompare( comp_type, ERget( S_ST0305_transaction_log ) )
			== 0 )
		{
			u_i2 log_status = transaction_logs_form( comp_name ); 

			/*
			** The following assumes that the II_LOG_FILE and
			** II_DUAL_FILE entries are consecutive and in that
			** order, as loaded above.
			*/
			exec frs inquire_frs table startup_config
			    (:curr_row = rowno, :max_row = maxrow);
			if (STcompare(comp_name, log_buf) == 0)
			{
			    enabled1 = (log_status & PRIMARY_LOG_ENABLED) ?
				ERx("1") : ERx("0");
			    exec frs putrow startup_config components
				(enabled = :enabled1);
			    enabled2 = (log_status & SECONDARY_LOG_ENABLED) ?
				ERx("1") : ERx("0");
			    if (++curr_row <= max_row && curr_row > 0)
				exec frs putrow startup_config components
				    :curr_row (enabled = :enabled2);
			}
			else
			{
			    enabled2 = (log_status & SECONDARY_LOG_ENABLED) ?
				ERx("1") : ERx("0");
			    exec frs putrow startup_config components
				(enabled = :enabled2);
			    enabled1 = (log_status & PRIMARY_LOG_ENABLED) ?
				ERx("1") : ERx("0");
			    if (--curr_row > 0 && curr_row <= max_row)
				exec frs putrow startup_config components
				    :curr_row (enabled = :enabled1);
			}

			exec frs redisplay;

			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST0303_logging_system ) )
			== 0 )
		{
			arg1 = STalloc( ERget( S_ST0368_configure_logging ) );
			arg2 = STalloc( ERget( S_ST0369_logging_parameters ) );
			(void) generic_independent_form( arg1, arg2,
				ERx( "rcp.log" ), NULL );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST0304_locking_system ) )
			== 0 )
		{
			arg1 = STalloc( ERget( S_ST036A_configure_locking ) );
			arg2 = STalloc( ERget( S_ST036B_locking_parameters ) );
			if (ingres_menu) 
			  (void) generic_independent_form( arg1, arg2,
				ERx( "rcp.lock" ), NULL );
			else  
			  (void) generic_independent_form( arg1, arg2,
				ERx( "lock"), NULL );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST0307_dbms_server ) )
			== 0 )
		{
			if( STcompare( comp_name,
				ERget( S_ST030F_default_settings ) ) == 0
			)
				STcopy( ERx( "*" ), comp_name );
			STprintf( pm_id, ERx( "dbms.%s" ), comp_name );
			arg1 = STalloc( ERget( S_ST0360_configure_dbms ) );
			arg2 = STalloc( ERget( S_ST0361_dbms_parameters ) );
                        cbf_srv_name = ERx( "dbms" );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST0308_net_server ) )
			== 0 )
		{
			if( STcompare( comp_name,
				ERget( S_ST030F_default_settings ) ) == 0
			)
				STcopy( ERx( "*" ), comp_name );
			STprintf( pm_id, ERx( "gcc.%s" ), comp_name );
			arg1 = STalloc( ERget( S_ST0364_configure_net ) );
			arg2 = STalloc( ERget( S_ST0365_net_parameters ) );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST0690_das_server ) )
			== 0 )
		{
			if( STcompare( comp_name,
				ERget( S_ST030F_default_settings ) ) == 0
			)
				STcopy( ERx( "*" ), comp_name );
			STprintf( pm_id, ERx( "gcd.%s" ), comp_name );
			arg1 = STalloc( ERget( S_ST0691_configure_das ) );
			arg2 = STalloc( ERget( S_ST0692_das_parameters ) );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}

		if( STcompare( comp_type, ERget( S_ST052B_bridge_server) )
			== 0 )
		{
			if( STcompare( comp_name,
				ERget( S_ST030F_default_settings ) ) == 0
			)
				STcopy( ERx( "*" ), comp_name );
			STprintf( pm_id, ERx( "gcb.%s" ), comp_name );
			arg1 = STalloc( ERget( S_ST0529_configure_bridge ) );
			arg2 = STalloc( ERget( S_ST052A_bridge_parameters ) );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST055C_oracle_gateway) )
			== 0 )
		{
			STcopy( ERx( "oracle" ), comp_name );
			STcopy( ERx( "gateway.oracle" ), pm_id );
			arg1 = STalloc( ERget( S_ST055A_configure_oracle ) );
			arg2 = STalloc( ERget( S_ST055B_oracle_parameters ) );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST055F_informix_gateway) )
			== 0 )
		{
			STcopy( ERx( "informix" ), comp_name );
			STprintf( pm_id, ERx( "gateway.%s" ), comp_name );
			arg1 = STalloc( ERget( S_ST055D_configure_informix ) );
			arg2 = STalloc( ERget( S_ST055E_informix_parameters ) );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST0563_sybase_gateway) )
			== 0 )
		{
			STcopy( ERx( "sybase" ), comp_name );
			STprintf( pm_id, ERx( "gateway.%s" ), comp_name );
			arg1 = STalloc( ERget( S_ST0561_configure_sybase ) );
			arg2 = STalloc( ERget( S_ST0562_sybase_parameters ) );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST0566_mssql_gateway) )
			== 0 )
		{
			STcopy( ERx( "mssql" ), comp_name );
			STprintf( pm_id, ERx( "gateway.%s" ), comp_name );
			arg1 = STalloc( ERget( S_ST0564_configure_mssql ) );
			arg2 = STalloc( ERget( S_ST0565_mssql_parameters ) );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST0569_odbc_gateway) )
			== 0 )
		{
			STcopy( ERx( "odbc" ), comp_name );
			STprintf( pm_id, ERx( "gateway.%s" ), comp_name );
			arg1 = STalloc( ERget( S_ST0567_configure_odbc ) );
			arg2 = STalloc( ERget( S_ST0568_odbc_parameters ) );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST0649_rdb_gateway) )
			== 0 )
		{
			STcopy( ERx( "rdb" ), comp_name );
			STcopy( ERx( "gateway.rdb" ), pm_id );
			arg1 = STalloc( ERget( S_ST0647_configure_rdb ) );
			arg2 = STalloc( ERget( S_ST0648_rdb_parameters ) );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST0644_rms_gateway ) )
			== 0 )
		{
			if( STcompare( comp_name,
				ERget( S_ST030F_default_settings ) ) == 0
			)
				STcopy( ERx( "*" ), comp_name );
			STprintf( pm_id, ERx( "rms.%s" ), comp_name );
			arg1 = STalloc( ERget( S_ST0642_configure_rms ) );
			arg2 = STalloc( ERget( S_ST0643_rms_parameters ) );
			cbf_srv_name = ERx( "rms" );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST0677_db2udb_gateway ) )
			== 0 )
		{
			STcopy( ERx( "db2udb" ), comp_name );
			STcopy( ERx( "gateway.db2udb" ), pm_id );
			arg1 = STalloc( ERget( S_ST0675_configure_db2udb ) );
			arg2 = STalloc( ERget( S_ST0676_db2udb_parameters ) );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST0309_star_server ) )
			== 0 )
		{
			if( STcompare( comp_name,
				ERget( S_ST030F_default_settings ) ) == 0
			)
				STcopy( ERx( "*" ), comp_name );
			STprintf( pm_id, ERx( "star.%s" ), comp_name );
			arg1 = STalloc( ERget( S_ST0366_configure_star ) );
			arg2 = STalloc( ERget( S_ST0367_star_parameters ) );
			(void) generic_independent_form( arg1, arg2, pm_id,
				comp_name );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
                if( STcompare( comp_type, ERget( S_ST0301_recovery_server ))
                        == 0 )
                {
                        arg1 = STalloc( ERget( S_ST037E_configure_recovery ));
                        arg2 = STalloc( ERget( S_ST037F_recovery_parameters ));
                        (void) generic_independent_form( arg1, arg2,
                                ERx( "recovery.*" ), NULL );
                        MEfree( arg1 );
                        MEfree( arg2 );
                        exec frs resume;
                }
		if( STcompare( comp_type, ERget( S_ST0300_name_server ) )
			== 0 )
		{
			arg1 = STalloc( ERget(
				S_ST0375_configure_name_server ) );
			arg2 = STalloc( ERget( S_ST0376_iigcn_parameters ) );
			(void) generic_independent_form( arg1, arg2,
				ERx( "gcn" ), NULL );
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		}
		if( STcompare( comp_type, ERget( S_ST034E_security_menu ) )
			== 0 )
		{
		    if(use_secure)
		    {
		    /*
		    ** Submenu: SecurityOptions Auditing End
		    */
		    exec frs submenu;
		    exec frs activate menuitem :ERget(S_ST0450_auth_system_menu), frskey15;
		    exec frs begin;
			arg1 = STalloc(ERget(S_ST0451_configure_auth_system));
			arg2 = STalloc(ERget(S_ST0452_auth_system_parameter));
			(void) generic_independent_form(arg1, arg2, ERx("gcf"), NULL);
			MEfree( arg1 );
			MEfree( arg2 );
		    exec frs end;

		    exec frs activate menuitem :ERx("SecurityOptions"),frskey16;
		    exec frs begin;
			arg1 = STalloc( ERget( S_ST037C_configure_sec_params ) );
			arg2 = STalloc( ERget( S_ST037A_security_params ) );
			(void) generic_independent_form( arg1, arg2, ERx( "secure" ), NULL );
			MEfree( arg1 );
			MEfree( arg2 );
		    exec frs end;
		    exec frs activate menuitem :ERget( S_ST037D_auditing_menu ), frskey17;
		    exec frs begin;
			arg1 = STalloc( ERget( S_ST0377_configure_security ) );
			arg2 = STalloc( ERget( S_ST0378_security_parameters ) );
			(void) generic_independent_form( arg1, arg2, ERx("c2"), NULL ) ;
			MEfree( arg1 );
			MEfree( arg2 );
		    exec frs end;
		    exec frs activate menuitem :ERget( FE_End ), frskey3;
		    exec frs begin;
				/* Ends submenu */
				;
		    exec frs end;
		    exec frs resume;
		    }
		    else
		    {
			arg1 = STalloc(ERget(S_ST0451_configure_auth_system));
			arg2 = STalloc(ERget(S_ST0452_auth_system_parameter));
			(void) generic_independent_form(arg1, arg2, ERx("gcf"), NULL);
			MEfree( arg1 );
			MEfree( arg2 );
			exec frs resume;
		    }
		}
	
  		if( STcompare(comp_type, ERget(S_ST0578_ice_server))
  			== 0 )
  		{
			if( STcompare( comp_name,
				ERget( S_ST030F_default_settings ) ) == 0
			)
				STcopy( ERx( "*" ), comp_name );

			STprintf( pm_id, ERx( "icesvr.%s" ), comp_name );
  			arg1 = STalloc( ERget(
  				S_ST042F_config_internet_comm ) );
  			arg2 = STalloc( ERget( S_ST0430_internet_parameters ) );
  			(void) generic_independent_form( arg1, arg2, pm_id,
                            comp_name);
  			MEfree( arg1 );
  			MEfree( arg2 );
  			exec frs resume;
  		}

		exec frs message 'Sorry, unknown menuitem found. '
			with style = popup;

	exec frs end;

	exec frs activate menuitem :ERget( S_ST0355_edit_count_menu ),frskey12;
	exec frs begin;

                is_das = FALSE;
                is_gcc = FALSE;

		exec frs inquire_frs menu '' (:activated = Active (
			:ERget ( S_ST0355_edit_count_menu ) )
		);
		if (activated == 0 )
		{
			exec frs resume;
		}
		else
			activated = 0;

		if ( ! STcompare( comp_type , ERget(S_ST0300_name_server ) ) )
			exec frs resume;

		if ( ! STcompare( comp_type , ERget(S_ST034E_security_menu) ) )
			exec frs resume;


		exec frs getrow startup_config components (
			:comp_type = type,
			:comp_name = name,
			:enabled_val = enabled
		);

		/* prompt for new startup count */
		exec frs prompt (
			:ERget( S_ST0310_start_count_prompt ),
			:in_buf
		) with style = popup;

		if( STcompare( comp_name,
			ERget( S_ST030F_default_settings ) ) == 0
		)
			STcopy( ERx( "*" ), comp_name );

		else if ( STcompare( comp_name,
			ERget( S_ST030A_not_configurable ) ) == 0
		) 
			STcopy( ERx( "*" ), comp_name );

		if( STcompare( comp_type ,
			ERget( S_ST0307_dbms_server ) ) == 0
		)
			STcopy( ERx( "dbms" ), comp_type );
		else if( STcompare( comp_type ,
			ERget( S_ST0308_net_server ) ) == 0
		)
                {
			STcopy( ERx( "gcc" ), comp_type );
                        is_gcc = TRUE;
                }
		else if( STcompare( comp_type ,
			ERget( S_ST0690_das_server ) ) == 0
		)
                {
			STcopy( ERx( "gcd" ), comp_type );
                        is_das = TRUE;
                }
		else if( STcompare( comp_type ,
			ERget( S_ST052B_bridge_server ) ) == 0
		)
			STcopy( ERx( "gcb" ), comp_type );


		else if( STcompare( comp_type ,
			ERget( S_ST0309_star_server ) ) == 0
		)
			STcopy( ERx( "star" ), comp_type );

		else if( STcompare( comp_type ,
			ERget( S_ST02FE_rmcmd ) ) == 0
		)
		{
			STcopy( ERx( "rmcmd" ), comp_type );
			if (STcompare(&in_buf[0],"1")!=0  && 
			    STcompare(&in_buf[0],"0")!=0)
			{
			    exec frs message 
				:ERget( S_ST0448_startupcnt_zero_one)
				with style = popup;
			    exec frs resume; 
			}
		}
		else if (STcompare( comp_type, ERget(S_ST0578_ice_server))==0)
		{
			STcopy( ERx( "icesvr" ), comp_type );
			if (STcompare(&in_buf[0],"1")!=0  && 
			    STcompare(&in_buf[0],"0")!=0)
			{
			    exec frs message 
				:ERget( S_ST0448_startupcnt_zero_one)
				with style = popup;
			    exec frs resume; 
			}
		}
		else if (STcompare( comp_type, 
				ERget( S_ST055C_oracle_gateway)) == 0)
		{
			STcopy( ERx( "oracle" ), comp_type );
			if (STcompare(&in_buf[0],"1")!=0  && 
			    STcompare(&in_buf[0],"0")!=0)
			{
			    exec frs message 
				:ERget( S_ST0448_startupcnt_zero_one)
				with style = popup;
			    exec frs resume; 
			}
		}
		else if(STcompare( comp_type , 
				ERget( S_ST055F_informix_gateway )) == 0)
		{
			STcopy( ERx( "informix" ), comp_type );
			if (STcompare(&in_buf[0],"1")!=0  && 
			    STcompare(&in_buf[0],"0")!=0)
			{
			    exec frs message 
				:ERget( S_ST0448_startupcnt_zero_one)
				with style = popup;
			    exec frs resume; 
			}
		}
		else if(STcompare( comp_type , 
				ERget( S_ST0563_sybase_gateway ) ) == 0)
		{
			STcopy( ERx( "sybase" ), comp_type );
			if (STcompare(&in_buf[0],"1")!=0  && 
			    STcompare(&in_buf[0],"0")!=0)
			{
			    exec frs message 
				:ERget( S_ST0448_startupcnt_zero_one)
				with style = popup;
			    exec frs resume; 
			}
		}
		else if (STcompare( comp_type , 
				ERget( S_ST0566_mssql_gateway )) == 0)
		{
			STcopy( ERx( "mssql" ), comp_type );
			if (STcompare(&in_buf[0],"1")!=0  && 
			    STcompare(&in_buf[0],"0")!=0)
			{
			    exec frs message 
				:ERget( S_ST0448_startupcnt_zero_one)
				with style = popup;
			    exec frs resume; 
			}
		}
		else if(STcompare( comp_type , 
				ERget( S_ST0569_odbc_gateway)) == 0)
		{
			STcopy( ERx( "odbc" ), comp_type );
			if (STcompare(&in_buf[0],"1")!=0  && 
			    STcompare(&in_buf[0],"0")!=0)
			{
			    exec frs message 
				:ERget( S_ST0448_startupcnt_zero_one)
				with style = popup;
			    exec frs resume; 
			}
		}
		else if( STcompare( comp_type ,
			ERget( S_ST0677_db2udb_gateway ) ) == 0)
		{
			STcopy( ERx( "db2udb" ), comp_type );
                       if (STcompare(&in_buf[0],"1")!=0  &&
                            STcompare(&in_buf[0],"0")!=0)
                        {
                            exec frs message
                                :ERget( S_ST0448_startupcnt_zero_one)
                                with style = popup;
                            exec frs resume;
                        }
                }

		else if( STcompare( comp_type ,
			ERget( S_ST0644_rms_gateway ) ) == 0)
		{
			STcopy( ERx( "rms" ), comp_type );
                       if (STcompare(&in_buf[0],"1")!=0  &&
                            STcompare(&in_buf[0],"0")!=0)
                        {
                            exec frs message
                                :ERget( S_ST0448_startupcnt_zero_one)
                                with style = popup;
                            exec frs resume;
                        }
                }

		else if (STcompare( comp_type, 
				ERget( S_ST0649_rdb_gateway)) == 0)
		{
			STcopy( ERx( "rdb" ), comp_type );
			if (STcompare(&in_buf[0],"1")!=0  && 
			    STcompare(&in_buf[0],"0")!=0)
			{
			    exec frs message 
				:ERget( S_ST0448_startupcnt_zero_one)
				with style = popup;
			    exec frs resume; 
			}
		}

		STprintf( temp_buf, ERx( "%s.%s.%sstart.%s.%s" ),
			  SystemCfgPrefix, host, SystemExecName, comp_name, 
			  comp_type );

		if( set_resource( temp_buf, in_buf ) )
		{
			exec frs putrow startup_config components (
				enabled = :in_buf
			);
		}
                /*
                ** Indicate that port rollups are supported for DAS or GCC
                ** TCP ports with the syntax XXnn or numeric ports.  A plus 
                ** sign ("+") is appended to the port code if this format is
                ** detected and no plus sign already exists.
                **
                ** If a plus sign exists for a port, and rollups are not
                ** to be supported, the plus signs are removed.
                */
                if ( is_das || is_gcc )
                    format_port_rollups( comp_type, comp_name, in_buf);

	exec frs end;

	exec frs activate menuitem :ERget( S_ST0351_duplicate_menu ),frskey13;
	exec frs begin;

		exec frs inquire_frs menu '' (:activated = Active (
			:ERget ( S_ST0351_duplicate_menu ) )
		);
		if (activated == 0)
		{
			exec frs resume;
		}
		else
			activated = 0;

		exec frs prompt (
			:ERget( S_ST032E_create_prompt ),
			:in_buf
		) with style = popup;

		exec frs message :ERget( S_ST0341_duplicating );

		copy_rename_nuke( in_buf, TRUE );

	exec frs end;

	exec frs activate menuitem :ERget( FE_Rename ),frskey14;
	exec frs begin;

		exec frs inquire_frs menu '' (:activated = Active (
			:ERget ( FE_Rename ) )
		);
		if ( activated == 0 )
		{
			exec frs resume;
		}
		else 
			activated = 0;

		exec frs prompt (
			:ERget( S_ST032F_rename_prompt ),
			:in_buf
		) with style = popup;

		exec frs message :ERget( S_ST0342_renaming );

		copy_rename_nuke( in_buf, FALSE );

	exec frs end;

	exec frs activate menuitem :ERget( FE_Destroy ),frskey15;
	exec frs begin;
		{
		i4 n;

		exec frs inquire_frs menu '' (:activated = Active (
			:ERget ( FE_Destroy ) )
		);
		if ( activated == 0 )
		{
			exec frs resume;
		}
		else	
			activated = 0;

		exec frs getrow startup_config components (
			:enabled_val = enabled
		);

		CVan( enabled_val, &n );
		if( n > 0 )
		{
			exec frs message :ERget( S_ST0374_startup_not_zero )
				with style = popup;
			exec frs resume;
		}

		if( CONFCH_YES != IIUFccConfirmChoice( CONF_GENERIC,
			NULL, NULL, NULL, NULL,
			S_ST0340_destroy_prompt,
			S_ST0384_destroy_component,
			S_ST0385_no_destroy_component,
			NULL, TRUE ) )
		{
			exec frs resume;
		}

		exec frs message :ERget( S_ST0343_destroying );

		copy_rename_nuke( NULL, FALSE );

		}
	exec frs end;

	exec frs activate menuitem :ERget( S_ST035F_change_log_menu ), frskey18;
	exec frs begin;

		if( browse_file( &change_log_file,
			ERget( S_ST0347_change_log_title ), TRUE ) != OK )
		{
			exec frs message :ERget( S_ST0348_no_change_log )
				with style = popup;
		}

	exec frs end;

	exec frs activate menuitem :ERget( S_ST034D_preferences_menu ),frskey19; 
	exec frs begin;

		char *arg1, *arg2;
		
		arg1 = STalloc( ERget( S_ST0372_configure_preferences ) );
		arg2 = STalloc( ERget( S_ST0373_system_preferences ) );
		(void) generic_independent_form( arg1, arg2, ERx( "prefs" ), NULL );
		MEfree( arg1 );
		MEfree( arg2 );
		exec frs resume;

	exec frs end;

	exec frs activate menuitem :ERget( FE_Help ), frskey1;
	exec frs begin;

		if( !get_help( ERx( "cbf.general" ) ) )
			exec frs message 'No help available.'
				with style = popup;

	exec frs end;

	exec frs activate menuitem :ERget( FE_Quit ), frskey2;
	exec frs begin;
		exec frs breakdisplay;
	exec frs end;

	exec frs activate before column components all;
	exec frs begin;
		exec frs getrow startup_config components (
			:comp_type = type,
			:comp_name = name 
		);
		if(
#if !defined(WORKGROUP_EDITION) || defined(int_lnx) || defined (int_rpl) \
                                || defined(ibm_lnx)

			STcompare( comp_type, ERget( S_ST0307_dbms_server ) )
			== 0 ||
#endif /* WORKGROUP_EDITION || int_lnx || int_rpl || ibm_lnx */

			STcompare( comp_type, ERget( S_ST0308_net_server ) )
			== 0 || 
			STcompare( comp_type, ERget( S_ST0690_das_server ) )
			== 0 || 
			STcompare( comp_type, ERget( S_ST0309_star_server ) )
			== 0 ||
			STcompare( comp_type, ERget( S_ST0644_rms_gateway ) )
			== 0 ||
			STcompare( comp_type, ERget( S_ST0677_db2udb_gateway ) )
			== 0 ||
			STcompare( comp_type, ERget( S_ST052B_bridge_server ) )
			== 0 )
		{
			exec frs set_frs menu '' (
				active( :ERget( S_ST0350_configure_menu ) )
					= 1
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST0355_edit_count_menu ) )
					= 1
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST0351_duplicate_menu ) )
					= 1
			);
			if( STcompare( comp_name,
				ERget( S_ST030F_default_settings ) ) == 0 )
			{
				exec frs set_frs menu '' (
					active( :ERget( FE_Destroy ) ) = 0
				);
				exec frs set_frs menu '' (
					active( :ERget( FE_Rename ) ) = 0
				);
			}
			else
			{
				exec frs set_frs menu '' (
					active( :ERget( FE_Destroy ) ) = 1
				);
				exec frs set_frs menu '' (
					active( :ERget( FE_Rename ) ) = 1
				);
			}
		}
		else if(
			STcompare( comp_type,
				ERget( S_ST02FE_rmcmd ) ) == 0 )
		{
			exec frs set_frs menu '' (
				active( :ERget( S_ST0350_configure_menu ) )
					= 0
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST0355_edit_count_menu ) )
					= 1
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST0351_duplicate_menu ) )
					= 0
			);
			exec frs set_frs menu '' (
				active( :ERget( FE_Destroy ) ) = 0
			);
			exec frs set_frs menu '' (
				active( :ERget( FE_Rename ) ) = 0
			);
		}
		else if(
			STcompare( comp_type,
				ERget( S_ST0578_ice_server ) ) == 0 ||
			STcompare( comp_type,
				ERget( S_ST055C_oracle_gateway ) ) == 0 ||
			STcompare( comp_type,
				ERget( S_ST055F_informix_gateway ) ) == 0 ||
			STcompare( comp_type,
				ERget( S_ST0563_sybase_gateway ) ) == 0 ||
			STcompare( comp_type,
				ERget( S_ST0566_mssql_gateway ) ) == 0 ||
			STcompare( comp_type,
				ERget( S_ST0569_odbc_gateway ) ) == 0 ||
			STcompare( comp_type,
				ERget( S_ST0677_db2udb_gateway ) ) == 0 ||
			STcompare( comp_type,
				ERget( S_ST0649_rdb_gateway ) ) == 0 
		)
		{
			exec frs set_frs menu '' (
				active( :ERget( S_ST0350_configure_menu ) )
					= 1
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST0355_edit_count_menu ) )
					= 1
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST034D_preferences_menu ) )
					= 1
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST035F_change_log_menu ) )
					= 1
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST0351_duplicate_menu ) )
					= 0
			);
			exec frs set_frs menu '' (
				active( :ERget( FE_Destroy ) ) = 0
			);
			exec frs set_frs menu '' (
				active( :ERget( FE_Rename ) ) = 0
			);
		}
		else if(

			STcompare( comp_type,
				ERget( S_ST0305_transaction_log ) ) == 0 )
		{
			exec frs set_frs menu '' (
				active( :ERget( S_ST0350_configure_menu ) )
					= 1
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST0355_edit_count_menu ) )
					= 0
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST0351_duplicate_menu ) )
					= 0
			);
			exec frs set_frs menu '' (
				active( :ERget( FE_Destroy ) ) = 0
			);
			exec frs set_frs menu '' (
				active( :ERget( FE_Rename ) ) = 0
			);
		}
		else if(
			STcompare( comp_type,
				ERget( S_ST0300_name_server ) ) == 0 ||
			STcompare( comp_type,
				ERget( S_ST0303_logging_system ) ) == 0 ||
			STcompare( comp_type,
                                ERget( S_ST0304_locking_system ) ) == 0 ||
                        STcompare( comp_type,
                                ERget( S_ST0301_recovery_server ) ) == 0 ||
                        STcompare( comp_type,
                                ERget( S_ST0305_transaction_log ) ) == 0 ||
                        STcompare( comp_type,
                                ERget( S_ST034E_security_menu ) ) == 0 )
		{
			exec frs set_frs menu '' (
				active( :ERget( S_ST0350_configure_menu ) )
					= 1
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST0355_edit_count_menu ) )
					= 0
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST0351_duplicate_menu ) )
					= 0
			);
			exec frs set_frs menu '' (
				active( :ERget( FE_Destroy ) ) = 0
			);
			exec frs set_frs menu '' (
				active( :ERget( FE_Rename ) ) = 0
			);
		}
		else if(
			STcompare( comp_type,
				ERget( S_ST0302_archiver_process ) ) == 0 )
		{
			exec frs set_frs menu '' (
				active( :ERget( S_ST0350_configure_menu ) )
					= 0
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST0355_edit_count_menu ) )
					= 0
			);
			exec frs set_frs menu '' (
				active( :ERget( S_ST0351_duplicate_menu ) )
					= 0
			);
			exec frs set_frs menu '' (
				active( :ERget( FE_Destroy ) ) = 0
			);
			exec frs set_frs menu '' (
				active( :ERget( FE_Rename ) ) = 0
			);
		}
	exec frs end;
}

void
scan_priv_cache_parm(char *instance, char *new, char *page, 
	bool keep_old)
{
    PM_SCAN_REC	state;
    STATUS	status;
    char	*last, *name, *value, *param, *regexp;
    char	resource[BIG_ENOUGH];
    char	expbuf[BIG_ENOUGH];
 
    if (page == NULL)
   	STprintf( expbuf, ERx( "%s.%s.%s.private.%s.%%" ),
   		  SystemCfgPrefix, host, cbf_srv_name, instance );
    else
	STprintf( expbuf, ERx( "%s.%s.%s.private.%s.%s.%%" ),
		  SystemCfgPrefix, host, cbf_srv_name, instance, page);

    regexp = PMmExpToRegExp( config_data, expbuf );

    for( status = PMmScan( config_data, regexp, &state,
		NULL, &name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, last, &name, &value ) )
    {
	/* set 'last' to handle deletion during scan */
	last = name;

	/* extract parameter name */
	if (page == NULL)
	    param = PMmGetElem( config_data, 5, name );
	else
	    param = PMmGetElem( config_data, 6, name );

	/* destroy old resource if 'keep_old' not TRUE */ 
	if( !keep_old && PMmDelete( config_data, name ) == OK )
	{
	    SIfprintf( change_log_fp, ERx( "DELETE %s\n" ), name );
	    SIflush( change_log_fp );
	}

	/* insert resource if 'new' not NULL */ 
	if( new != NULL )
	{
	    /* compose duplicate resource name */
	    if (page == NULL)
	        STprintf( resource, ERx( "%s.%s.%s.private.%s.%s" ), 
			  SystemCfgPrefix, host, cbf_srv_name,
                          new, param );
	    else
		STprintf( resource, ERx( "%s.%s.%s.private.%s.%s.%s" ), 
			  SystemCfgPrefix, host, cbf_srv_name,
                          new, page, param);


	    if( PMmInsert( config_data, resource, value ) == OK )
	    {
		SIfprintf( change_log_fp, ERx( "CREATE %s: (%s)\n" ),
			resource, value );
		SIflush( change_log_fp );
	    }
	}
    }
}

void format_port_rollups ( char *serverType, char *instance, char *countStr) 
{
    char expbuf[ BIG_ENOUGH ];
    exec sql begin declare section;
        char *regexp, *last, *name, *value;
        char *startCntBuff;
        char *gcd_warnmsg = "Setting startup count to one";
    exec sql end declare section;
    char *protocol;
    char *type;
    STATUS status;
    char newPort[GCC_L_PORT];
    u_i2 offset = 0;
    u_i2 portNum = 0;
    u_i2 numericPort = 0;
    u_i2 startCount = 0;
    u_i2 startOffset = 0;
    PM_SCAN_REC state;
    typedef struct
    {
        QUEUE port_q;
        char *port;
        char *protocol;
    }  PORT_QUEUE;
    PORT_QUEUE pq, *pp;
    QUEUE *q;
    
    startCntBuff = countStr;
    QUinit ( &pq.port_q );

    STprintf( expbuf, ERx( "%s.%s.%s.%s.%%.port" ),
        SystemCfgPrefix, host, serverType, instance );

    regexp = PMmExpToRegExp( config_data, expbuf );

    /*
    ** Search for items pertaining to the present server type and
    ** instance with a TCP protocol and port.
    */
    for( status = PMmScan( config_data, regexp, &state, NULL,
        &name, &value ); status == OK; status =
        PMmScan( config_data, NULL, &state, NULL, &name,
        &value ) )
    {   
        /* extract protocol name */
        protocol = PMmGetElem( config_data, 4, name );

        /* extract port */
        type = PMmGetElem( config_data, 5, name );

        if ( !STcompare(protocol, "tcp_ip" ) ||
             !STcompare(protocol, "wintcp" ) ||
             !STcompare(protocol, "tcp_wol" ) ||
             !STcompare(protocol, "tcp_dec" ))
        {
            pp = (PORT_QUEUE *)MEreqmem( 0, 
                sizeof(PORT_QUEUE), TRUE, NULL );
            pp->port = STalloc( value );
            pp->protocol = STalloc( protocol );
            QUinsert( (QUEUE *)pp, &pq.port_q );
            
            /*
            ** Check for a numeric port.
            **
            ** If input is not strictly numeric, input is 
            ** ignored.
            **
            ** Extract the numeric port and check for expected
            ** syntax: n{n}+.
            */
            numericPort = 0;
            if ( portNum == 0 )
                for( offset = 0; CMdigit( &value[offset] ); offset++ )
                    numericPort = (numericPort * 10) + (value[offset] - '0');
            /*
            ** Extract the startup count.
            */
            for( startOffset = 0, startCount = 0; 
                CMdigit( &startCntBuff[startOffset] ); startOffset++ )
            {
                startCount = (startCount * 10) + 
                    (startCntBuff[startOffset] - '0');
            }
                             
            /*
            ** If the port is explicitly numeric, the user may 
            ** not want the port number to increment.  Confirm 
            ** before proceeding.
            */
            if ( ! value[ offset ] && numericPort && startCount > 1)
            {
                if  (CONFCH_NO == 
                    IIUFccConfirmChoice( CONF_GENERIC,
                        NULL, value, NULL, NULL,
                        S_ST06BF_multiple_das, 
                        S_ST06C0_allow_multiple,
                        S_ST06C1_disallow_multiple, NULL, 
                        TRUE ) ) 
                {
                    exec frs message :gcd_warnmsg with 
                        style=popup;
                    STprintf( expbuf, 
                        ERx( "%s.%s.%sstart.%s.%s" ),
                            SystemCfgPrefix, host, 
                            SystemExecName, instance, 
                            serverType );
                    startCntBuff[0] = '1';
                    startCntBuff[1] = '\0';
                    if( set_resource( expbuf, startCntBuff ) )
                    {
                        exec frs putrow startup_config 
                            components ( enabled = 
                            :startCntBuff);
                    }
                    startCount = 1;
                }
            }
        } /* if (!STcompare("tcp_ip"... */
    } /* for( status = PMmScan( config_data... */

    /*
    ** Go through the queue and re-write the ports
    ** with the rollup indicator if the startup count is greater
    ** than one and no indicator exists.  If the startup count
    ** is less than 2, remove the indicator if it exists.
    */
    for (q = pq.port_q.q_prev; q != &pq.port_q; 
        q = q->q_prev)
    {
        pp = (PORT_QUEUE *)q;
        for( portNum = 0, offset = 0; CMalpha( &pp->port[0] ) 
           && (CMalpha( &pp->port[1] ) || 
              CMdigit( &pp->port[1] )); )
        {
            /*
            ** A two-character symbolic port permits rollup
            ** without special formatting.
            */
            offset = 2;
            if ( ! pp->port[offset] )
                break;
    
            /*
            ** A one or two-digit base subport may be 
            ** specified.
            */
            if ( CMdigit( &pp->port[offset] ) )
            {
                portNum = (portNum * 10) + (pp->port[offset++] - '0');
    
                if ( CMdigit( &pp->port[offset] ) )
                {
                    portNum = (portNum * 10) + (pp->port[offset++] - '0');
    
                    /*
                    ** An explicit base subport must be in 
                    ** the range [0,15].  A minimum of
                    ** 14 is required to support
                    ** rollup.
                    */
                    if ( portNum > 14 )  
                    {
                        offset = 0;
                        break;
                    }
                }
            } /* if ( CMdigit ( &pp->port[offset] ) */
  
            /* 
            ** Unconditionally break from this loop.
            */
            break;

        } /* for( ; CMalpha( &pp->port[0] ) ... */
        
        /*
        ** Subport values greater than 14 cannot roll up.  Ignore them.
        */
        if ( portNum > 14 )
           continue;

        /*
        ** Check for a numeric port.
        **
        ** If input is not strictly numeric, input is 
        ** ignored.
        **
        ** Extract the numeric port and check for expected
        ** syntax: n{n}+.
        */
        numericPort = 0;
        if ( portNum == 0 )
        {
            for( offset = 0; CMdigit( &pp->port[offset] ); offset++ )
                numericPort = (numericPort * 10) + (pp->port[offset] - '0');
        }
        
        /*
        ** Ports with the format "XX" are ignored.
        */
        if ( offset < 3 && !numericPort )
            continue;

        /*
        ** Existing ports with rollups are ignored if the startup count
        ** is greater the one.
        */
        if ( startCount > 1 && ( pp->port[ offset ] == '+' ) )
            continue;

        /*
        ** If no rollup indicator is present, and no multiple startups
        ** have been specified, ignore.
        */ 
        if ( startCount < 2 && ! pp->port[ offset ] )
            continue;

        /*
        ** If multiple startups are not specified, remove the rollup
        ** indicator if it exists.
        **
        ** After this, all exceptions should have been handled and
        ** the port is re-written with the plus indicator.
        */
        if ( startCount < 2 && pp->port[ offset ] == '+' )
        {
            pp->port[ offset ] = '\0';
            STprintf( newPort, "%s", pp->port );
        }
        else 
            STprintf( newPort, "%s+", pp->port );

        STprintf( expbuf, ERx( "%s.%s.%s.%s.%s.%s" ),
            SystemCfgPrefix, host, serverType, 
                instance, pp->protocol, type );

	/* open change log file for appending */
	if( SIfopen( &change_log_file , ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&change_log_fp ) != OK )
	{
		EXdelete();
		IIUGerr( S_ST0323_unable_to_open, UG_ERR_FATAL, 1,
			change_log_buf );
	}
        CRsetPMval( expbuf, newPort, change_log_fp, 
            FALSE, TRUE );
	(void) SIclose( change_log_fp );

        PMmWrite ( config_data, NULL );

    } /* for ( q = pq.port... */
    
    /*
    ** De-allocate any items in the port queue.
    */
    for (q = pq.port_q.q_prev; q != &pq.port_q; 
        q = pq.port_q.q_prev)
    {
        pp = (PORT_QUEUE *)q;
        MEfree((PTR)pp->port);
        MEfree((PTR)pp->protocol);
        QUremove (q);
        MEfree((PTR)q);
    }
} 
