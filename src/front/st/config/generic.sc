/*
**  Copyright (c) 1992, 2005 - Ingres Corporation
**
**
**  Name: generic.sc -- contains a couple of generic display loops which
**	control most of the forms in the INGRES configuration utility.
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	18-jan-93 (tyler)
**		Fixed typo which prevented the derived parameters from 	
**		from coming up properly.
**	26-apr-93 (tyler)
**		Modified code which restores original configuration from
**		backup to use PMfree() - previously omitted due to a
**		mysterious bug, now fixed.
**	24-may-93 (tyler)
**		Added argument to PMmInit() call.
**	21-jun-93 (tyler)
**		Added code which maintains a change ($II_CONFIG/config.log)
**		and implemented the previously unimplemented 'Protect'
**		function.  Switched to new interface for multiple context
**		PM functions.  The standard parameter form is now skipped
**		if a system components has only derived parameters. 
**	23-jul-93 (tyler)
**		Removed embedded strings.  Added ChangeLog menu item
**		which allows the user to browse the change log.  Confirmation 
**		prompts are now handled by the standard front-end utility
**		function IIUFccConfirmChoice().  Fixed bug which caused
**		cache resources for the default DBMS definition not to be
**		found.  PMmInit() interface changed.
**	04-aug-93 (tyler)
**		Fixed bug which caused 'Derived' menu item not to be
**		disabled when there aren't any derived parameters.
**	19-oct-93 (tyler)
**		Fixed bugs which prevented opening of change log on VMS.
**		Changed type of global variable "host" to (char *).
**		Made kludgy changes to make these routines work for
**		DBMS cache resources following change from "dmf" to
**		"dbms" in dbms cache resources names.
**	01-dec-93 (tyler)
**		Fixed bad calls to IIUGerr().
**	05-jan-94 (tyler)
**		Repopulate derived parameters tablefields after a
**		'Recalculate' operation.
**	3-feb-94 (robf)
**               Update secure operations.
**	22-feb-94 (tyler)
**		Addressed SIR 56742: Edit now toggles boolean values.
**		Fixed BUG 59434: asssign Help menu item to PF2.
**	8-jul-94 (robf)
**              Try to keep the two flavors of audit log names in sync
**	        better. Long term we should look at merging the two names.
**	30-nov-94 (nanpr01)
**		Fixed BUG #60404. Changed parameter from db_list to
**		database_list.
**	30-nov-94 (nanpr01)
**		Fixed BUG #60371. Added frskey mapping with the different 
**		CBF actions so that users can standardize the keys from
**		screen to screen by defining the key mapping of
**		frskey11-frskey19
**      21-Jan-95 (liibi01)
**              Fix Bug #66343. For c2_form, we should not disable the backup
**              option.
**      29-Jan-95 (liibi01)
**              Fix Bug #66338, fix problem with CBF form, if derived option
**              is choosen, boolean field doesn't toggle if try to edit and
**              user can input any invalid stuff into boolean field. 
**	3-may-1995 (hanch04)
**		Fix Bug #68431, checked for undefined value when
**		choosing an option not valid for componet.
**      8-may-1995 (hanch04)
**		Fix Bug #68412, changed value to use temp_val in the getrow
**		call when choosing to protect a parameter.
**      19-july-1995 (carly01)
**		Fix bug 69912 - replaced overuse of variable name_val with
**		variable bool_name_val.  name_val was reused between time
**		of setting and subsequent use, causing the Edit of 
**		cache_sharing to fail.
**      01-aug-95 (sarjo01) Bugs 70242, 70251, 70248.
**              NT: use II_TEMPORARY to build temp file path. Cannot
**              count on a #defined path (eg. /tmp/ ) existing.
**      07-aug-95 (harpa06)
**              Bug #70399 - Change the GLOBALREFerence of tempdir as a pointer
**              to character string (char *) to an array of MAX_LOC characters
**              since this is the way it was GLOBALly DEFined.
**      03-apr-97 (i4jo01)
**              Changed handling of parameters when an illegal value is 
**              entered. Should not place the illegal value into the 
**              protected data set. (b81120)
**	07-Feb-96 (rajus01)
**		Added bridge_protocols_form() prototype for Protocol Bridge. 
**		Added S_ST052C_bridge_protocols_menu  for Protocol Bridge.
**	06-jun-1996 (thaju02)
**	 	Added config_caches(), get_cache_status() and 
**		toggle_cache_status() to enable editing of 
**		buffer cache parameters through cbf, for variable page size
**		support.
**	30-jul-1996 (thaju02)
**	        Modified config_caches(), changed form title and table title
**	        parameters passed to generic_independent call.
**	05-aug-1996 (thaju02)
**	        config_caches() switched order of configure and edit menu-
**	        items, and changed the key mapping of the end menuitem to
**	        <pf2> for consistency.  Added help menuitem.
**	10-oct-1996 (canor01)
**		Replace hard-coded 'ii' prefixes with SystemCfgPrefix.
**	04-dec-1996 (reijio01)
**		Use ii_system_name and ii_installation_name (from config.sc)
**		to initialize genind.frm (used in cbf calls)
**	08-jan-1997 (funka01)
**		Remove the database menu option in dbms if ingres_menu      
**		is FALSE.  ingres_menu is GLOBALREF (defined in config.sc).
**		It is set to FALSE ifdef JASMINE.
**		Used variable dbms_second_menuitem in menu instead of
**		ERget( S_ST0354_databases_menu ).  Set up derived_table_title 
**		for Locking System for Jasmine.     
**	08-jan-1997 (funka01)
**		Updated DMF Cache to not have specific default for Jasmine.    
**	08-jan-1997 (funka01)
**		Removed cache sharing option for Jasmine.    
**	27-jan-1997 (thaju02)
**		fix for bug #80261 - 'Derived DBMS Cache Parameters for xxk'
**		table title eventually caused cbf to hang. 
**		In generic_dependent_form, copy table title to temporary
**		buffer for appending page size.
**	31-jan-1997 (thaju02)
**		bug #80450 - enforce in cbf that both dmcm and cache_sharing
**		can NOT be on.
**      08-apr-1997 (thaju02)
**		When cache_sharing is toggled, protected resources are not
**		preserved. Once resources have been set need to load 
**		protect.dat again to get latest changes.   
**		PMmWrite(protect_data, NULL) calls changed to 
**		PMmWrite(protect_data, &protect_file), because specifying
**		NULL ptr for file location resulted in output being directed
**		config.dat (see gl/glf/src/pm/pm.c).
**	02-jun-1997 (nanpr01)
**		Add a reformat message when log.block_size is changed.
**		Also add in the help that this is required. This was
**		requested by Irish Revenue Commissioner. 
**	07-aug-1997 (funka01)
**		Updated places where 2k was not used when all other page
**		sizes were for jasmine.
**      03-Oct-97 (fanra01)
**              Previous use of the restore option would set the field to
**              the last read value.
**              Add the parsing of the backup  data file and use those
**              values to initialise value during a restore operation.
**	16-Jun-1998 (thaal01)
**		Added derived parameter title for GCN, part of Bug 91342
**	30-jun-1998 (ocoro02)
**		Fixed typo in previous change - ERget instead of Erget.
**	26-Jul-98 (gordy)
**		Added configuration form for GCF/GCS security.
**	24-aug-1998 (hanch04)
**		Changed dual_log_file to dual_log.
**	 1-Oct-98 (gordy)
**		Combined net and bridge protocol menu items into single
**		menu key and added registry protocols as the old bridge
**		protocol key.
**	23-nov-1998 (nanpr01)
**		Raw log_file parameters are appearing in transaction log as 
**		well as in logiing parameters.
**      16-Mar-1999 (hanal04)
**              Added default_page_cache_off(). If the default_page_size
**              is updated warn the user if the respective cache size has 
**              not been enabled. b95887.
**  12-Aug-1999 (rodjo04) bug 98353.
**     Added 'derived recovery server parameter' title. Title was missing
**     causing a SEGV on unix.
**	27-Aug-1999 (hanch04)
**		The 2k page size cache can be turned off now that system
**		catalogs can be VPS.
**	04-Nov-1999 (hanch04)
**		The .p2k should not be used for 2k cache parameters.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-aug-2001 (wansh01) 
**          Changed cbf bridge configuration option to protocols and
**          vnodes. protocols using the same form as net protocols and  
**          vnode allows edit, create, destroy options.  
**	6-Nov-2002 (hanje04 for schka70)
**		BUG 109202
**		Don't assume that cache_name and cache_sharing are
**		independent parameters, and fix handling of their values
**	07-Jan-2002 (hanje04)
**		Ammend fix for bug 109202 to take into account that if we are 
**	 	saving cache_name and cache_sharing before checking to see if
**		they are derived then we must check against the short parameter
**		name. Actual parameter name must be preserved until after units
**		have been checked.
**	15-Jan-2002 (hanje04)
**	 	Further ammendment to fix for bug 109202 to remove declaration
**		of "pm_id" is it is declared as part of the function.
**      23-jan-2003 (chash01) integrate bug109055 fix
**              Add RMS specific code in appropriate place so that RMS gateway
**              can be properly configured with CBF.  Use cbf_srv_name to tell
**              if server is dbms, rms or something else (NULL).
**      26-Feb-2003 (wansh01) 
**          	Added das server protocol configuration. it used
**		the same form as net protocols
**	10-Apr-2003 (hanch04)
**		Increased the generic_independent form title to 72 characters.
**	08-Aug-2003 (drivi01)
**		Added error messages to the menu items that do not display on
**		the menu but can be activated by pressing 4 or 5 (frskey14 or
**		frskey15) and disabled actions associated with those menu items
**		under these circumstances.
**      28-Oct-2003 (hanal04) Bug 111182 INGCBT500
**              Extend the above change to prevent SIGSEGV when KeyPad 3 is
**               pressed inside the Net Server Configuration screen.
**	09-jun-2004 (abbjo03)
**	    Quote the now reserved word "cache" in FRS statements.
**	28-jun-2004 (abbjo03)
**	    Back out change of 9-jun-2004, because it breaks CBF on other
**	    platforms.
**	02-feb-2005 (abbjo03)
**	    Reinstate the 9-jun-2004 change, but this time with proper SQL
**	    quotes ('cache').
**	18-feb-2005 (devjo01)
**	    Only use S_ST0332_shared & S_ST0333_private when displaying
**	    the shared/private status of the cache.  Internally ingres
**	    configuration parameters are not localized (yet).
**	16-Mar-2006 (wanfr01)
**		SIR 115848
**		Pop up a message warning user if they set blob_etab_page_size
**		to a non-enabled page_size.
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Added calls to dbms_mllist_form() and key assignment to call
**          the DBMS Must Log DB list screen.
*/

# include	<compat.h>
# include       <cv.h>
# include	<si.h>
# include	<ci.h>
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
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>
# include	<te.h>
# include	<erst.h>
# include	<stdprmpt.h>
# include	<pc.h>
# include	<pm.h>
# include	<pmutil.h>
# include	<util.h>
# include	<simacro.h>
# include	"cr.h"
# include	"config.h"

exec sql include sqlca;
exec sql include sqlda;

exec sql begin declare section;
GLOBALREF char		*host;
GLOBALREF char		*ii_system;
GLOBALREF char		*ii_system_name;
GLOBALREF char		*ii_installation;
GLOBALREF char		*ii_installation_name;
exec sql end declare section;

GLOBALREF LOCATION	config_file;
GLOBALREF LOCATION	protect_file;
GLOBALREF LOCATION	change_log_file;
GLOBALREF FILE		*change_log_fp;
GLOBALREF char		*change_log_buf;
GLOBALREF char		tempdir [MAX_LOC];
GLOBALREF PM_CONTEXT	*config_data;
GLOBALREF PM_CONTEXT	*protect_data;
GLOBALREF PM_CONTEXT	*units_data;
GLOBALREF bool		ingres_menu;
GLOBALREF char          *cbf_srv_name;

bool generic_dependent_form( char *, char *, char *, char *, bool, bool );
bool dbms_dblist_form( char *, char *, char ** );
bool dbms_mllist_form( char *, char *, char ** );
bool security_mech_form( VOID );
bool net_protocols_form( char *, char * );
bool bridge_protocols_form( char * );
bool config_caches(char *, char *, char *, char *);
void get_cache_status(char *, char *, char **);
bool default_page_cache_off(char *pm_id, i4 *page_size, char *cache_share,
					char * cache_name);
bool toggle_cache_status(char *, char *, char *, char *);
STATUS copy_LO_file( LOCATION *, LOCATION * );

static LOCATION backup_data_file;
static char backup_data_buf[ MAX_LOC + 1 ];
static LOCATION backup_change_log_file;
static char backup_change_log_buf[ MAX_LOC + 1 ];
static LOCATION backup_protect_file;
static char backup_protect_buf[ MAX_LOC + 1 ];
#define MAX_AUDIT_LOGS 256 /* Max number of audit logs */
static char *audit_log_list[MAX_AUDIT_LOGS];
static PM_CONTEXT    *restore_data;

bool
generic_independent_form( char *form_title, char *table_title, char *pm_id,
	char *def_name )
{
	exec sql begin declare section;
	char *name, *value, *short_name;
	char in_buf[ BIG_ENOUGH ];
	char name_val[ BIG_ENOUGH ]; 
	char bool_name_val[ BIG_ENOUGH ]; /*b69912*/
	char *my_name;
	char *title = form_title;
	char description[ BIG_ENOUGH ];
	char *form_name;
	char host_field[ 21 ];
	char def_name_field[ 21 ];
	char temp_buf[ MAX_LOC + 1 ];
	i4 lognum;
	i4 activated = 0;
	char	fldname[33];
	char dbms_second_menuitem[20];
	char dbms_third_menuitem[40];
	char vnode_registry_menu[ BIG_ENOUGH ];
	exec sql end declare section;
	char *audlog=ERx("audit_log_");
	i4  audlen;
	i4     number;

	LOCATION *IIUFlcfLocateForm();
	STATUS IIUFgtfGetForm();
	PM_SCAN_REC state;
	STATUS status;
	i4 independent, dependent, len, i;
	char *cache_name = NULL;
	char *cache_sharing = NULL;
	bool changed = FALSE;
	char *comp_type;
	bool backup_mode;
	char *owner, instance[ BIG_ENOUGH ];
	char expbuf[ BIG_ENOUGH ];
	char *regexp;
	char *db_list;
	char *ml_list;
	char derived_table_title[ BIG_ENOUGH ];
	char real_pm_id[ BIG_ENOUGH ];
	char *pm_host = host;
	char *p;
	bool c2_form=FALSE;
	bool doing_aud_log=FALSE;
	bool avoid_set_resource; /* bug #80450 */
        bool blRestore = FALSE;

	STcopy(ERget( S_ST0354_databases_menu ),dbms_second_menuitem);

	STcopy(ERget( S_ST06C3_mustlogDB_menu ),dbms_third_menuitem);

	exec frs message :ERget( S_ST0344_selecting_parameters );

	/* first component of pm_id identifies component type */
	comp_type = PMmGetElem( config_data, 0, pm_id );

	/*
	** Since the forms system doesn't allow nested calls to the
	** same form, dmf_independent must be used if this function is
	** being called to drive the independent DBMS cache parameters
	** form.
	*/
	if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
	{
		form_name = ERx( "dmf_independent" );
		/* HACK: skip past "dmf." */
		p = pm_id;
		CMnext( p );
		CMnext( p );
		CMnext( p );
		CMnext( p );
   	        if ( cbf_srv_name != NULL )
      		  STprintf( real_pm_id, ERx( "%s.%s" ), cbf_srv_name, p );
	}
	else if( STcompare( comp_type, ERx( "c2" ) ) == 0 )
	{
		/*
		** C2 form is independent
		*/
		form_name = ERx("c2_independent");
		STcopy( pm_id, real_pm_id );
		c2_form=TRUE;
	}
	else
	{
		if( STcompare(comp_type, ERx( "dbms" ) ) == 0 )
		{
	  	  if (!ingres_menu)
                  {
	    	      *dbms_second_menuitem=EOS;
	    	      *dbms_third_menuitem=EOS;
                  }
        	}
		form_name = ERx( "generic_independent" );
		STcopy( pm_id, real_pm_id );
	}

	if( IIUFgtfGetForm( IIUFlcfLocateForm(), form_name ) != OK )
		IIUGerr( S_ST031A_form_not_found, UG_ERR_FATAL, 1, form_name );

	/* center description (72 characters wide) */
	STcopy( table_title, temp_buf );
	len = (72 - STlength( temp_buf )) / 2;
	for( i = 0, p = description; i < len; i++, CMnext( p ) )
		STcopy( ERx( " " ), p );	
	STcopy( temp_buf, p );

	/* set value of my_name */
	if (c2_form)
	 	my_name = NULL;
	else  if( def_name == NULL )
		my_name = STalloc( ERget( S_ST030F_default_settings ) );
	else
	{
		if( STcompare( def_name, ERx( "*" ) ) == 0 )
			my_name = STalloc(
				ERget( S_ST030F_default_settings ) );
		else
			my_name = STalloc( def_name ); 
	}

	/* extract resource owner name from real_pm_id */
	owner = PMmGetElem( config_data, 0, real_pm_id );

	len = PMmNumElem( config_data, real_pm_id );
		
	/* compose "instance" */
	if( len == 2 )
	{
		/* instance name is second component */
		name = PMmGetElem( config_data, 1, real_pm_id );
		if( STcompare( name,
			ERget( S_ST030F_default_settings ) ) == 0
		)
			STcopy( ERx( "*" ), instance );
		else
			STcopy( name, instance ); 
		STcat( instance, ERx( "." ) );
	}
	else if( len > 2 )
	{
		char *p;
		/* instance name is all except first */

		for( p = real_pm_id; *p != '.'; CMnext( p ) );
		CMnext( p );
		STcopy( p, instance );
		STcat( instance, ERx( "." ) );
	}
	else
		STcopy( ERx( "" ), instance );

	/* Set protocol menu item name */
	if ( ! STcompare( comp_type, ERx( "gcb" ) ) ) 
	    STcopy( ERget( S_ST041C_bridge_vnode_menu ), vnode_registry_menu );
	else
	    STcopy( ERget( S_ST045A_registry_menu ), vnode_registry_menu );
	     
	exec frs inittable :form_name params read;

	if (c2_form)
	{
		exec frs inittable :form_name auditlogs read;
		for(lognum=1;lognum<=MAX_AUDIT_LOGS;lognum++)
		{
			audit_log_list[lognum-1]=NULL;
		}
	}

	exec frs display :form_name;

	exec frs initialize (
		title = :title,
		host = :host,
		ii_installation = :ii_installation,
		ii_installation_name = :ii_installation_name,
		ii_system = :ii_system,
		ii_system_name = :ii_system_name,
		description = :description
	);

	exec frs begin; /* form initialization */

	if( STcompare( comp_type, ERx( "c2" ) ) == 0 )
		pm_host = ERx( "*" );


	/* find out if this is the dbms_independent form */
	if( STcompare( comp_type, ERx( "dbms" ) ) != 0 ) 
	{
		exec frs set_frs menu '' (
			active( :ERget( S_ST0354_databases_menu ) ) = 0
		);
                exec frs set_frs menu '' (
                        active( :ERget( S_ST06C3_mustlogDB_menu ) ) = 0
                );
		exec frs set_frs menu '' (
			active( :ERget( S_ST0356_cache_menu ) ) = 0
		);
	}

	/* find out if this is the net_independent form */
	/* for das server activate protocol menu        */
	/* and deactivate vnode_registry_menu           */

	if (( STcompare( comp_type, ERx( "gcc" ) ) ) && 
	    ( STcompare( comp_type, ERx( "gcb" ) ) ) ) 
	{
	    if ( STcompare( comp_type, ERx( "gcd" ) ) )  
		exec frs set_frs menu '' ( 
                         active( :ERget( S_ST0357_protocols_menu) ) = 0);
    	    exec frs set_frs menu '' ( active( :vnode_registry_menu ) = 0 );
	}


	/* find out if this is the security_independent form */
	if ( STcompare( comp_type, ERx( "gcf" ) ) ) 
		exec frs set_frs menu '' (
			active( :ERget( S_ST0453_mechanisms_menu) ) = 0 );

      	/* find out if this is the rms_independent form */
      	if( STcompare( comp_type, ERx( "rms" ) ) == 0 ) 
      	{
      		exec frs set_frs menu '' (
      			active( :ERget( S_ST0356_cache_menu ) ) = 1
      		);
      	}
	/* display definition name if applicable */
	if( my_name != NULL )
		exec frs putform( def_name = :my_name);


	/* check whether this is a DBMS cache definition */
	if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
	{
		exec sql begin declare section;
		char cache_desc[ BIG_ENOUGH ];
		exec sql end declare section;

		/* if so, turn off backup option */
		backup_mode = FALSE;

		/* and display cache_desc */
		value = PMmGetElem( config_data, 1, real_pm_id );
		if( STcompare( ERx("shared"), value ) == 0 )
			exec frs putform(
				cache_desc = :ERget( S_ST0332_shared )
			);
		else
		{
			name = STalloc( ERget( S_ST0333_private ) );
			value = STalloc( PMmGetElem( config_data, 2,
				real_pm_id ) );
			if( STcompare( value, ERx( "*" ) ) == 0 )
			{
				MEfree( value );
				value = STalloc(
					ERget( S_ST030F_default_settings ) );
			}
			STprintf( cache_desc, ERx( "%s ( %s=%s )" ), 
				name, ERget( S_ST0334_owner ), value );
			MEfree( name );
			MEfree( value );
			exec frs putform(
				cache_desc = :cache_desc
			);
		}
	}
	else
	{
		/* this is not a DBMS cache definition, so */
		backup_mode = TRUE;

		/* prepare a backup LOCATION for the data file */
		STcopy( ERx( "" ), temp_buf );
		LOfroms( FILENAME, temp_buf, &backup_data_file );
		LOuniq( ERx( "cacbfd1" ), NULL, &backup_data_file );
		LOtos( &backup_data_file, &name );
		STcopy( tempdir, backup_data_buf ); 
		STcat( backup_data_buf, name );
		LOfroms( PATH & FILENAME, backup_data_buf, &backup_data_file );
	
		/* prepare a backup LOCATION for the change log */
		STcopy( ERx( "" ), temp_buf );
		LOfroms( FILENAME, temp_buf, &backup_change_log_file );
		LOuniq( ERx( "cacbfc1" ), NULL, &backup_change_log_file );
		LOtos( &backup_change_log_file, &name );
		STcopy( tempdir, backup_change_log_buf ); 
		STcat( backup_change_log_buf, name );
		LOfroms( PATH & FILENAME, backup_change_log_buf,
			&backup_change_log_file );
	
		/* prepare a backup LOCATION for the protected data file */
		STcopy( ERx( "" ), temp_buf );
		LOfroms( FILENAME, temp_buf, &backup_protect_file );
		LOuniq( ERx( "cacbfp1" ), NULL, &backup_protect_file );
		LOtos( &backup_protect_file, &name );
		STcopy( tempdir, backup_protect_buf ); 
		STcat( backup_protect_buf, name );
		LOfroms( PATH & FILENAME, backup_protect_buf,
			&backup_protect_file );
	
		/* write configuration backup */ 
		if( PMmWrite( config_data, &backup_data_file ) != OK )
		{
			IIUGerr( S_ST0326_backup_failed, 0, 0 ); 
			LOdelete( &backup_data_file );
			if( my_name != NULL )
				MEfree( my_name );
			return( FALSE );
		}

		/* write change log backup */ 
		if( copy_LO_file( &change_log_file, &backup_change_log_file )
			!= OK ) 
		{
			IIUGerr( S_ST0326_backup_failed, 0, 0 ); 
			(void) LOdelete( &backup_change_log_file );
			return( FALSE );
		}

		/* write protected data backup */ 
		if( PMmWrite( protect_data, &backup_protect_file ) != OK )
		{
			IIUGerr( S_ST0326_backup_failed, 0, 0 ); 
			LOdelete( &backup_protect_file );
			if( my_name != NULL )
				MEfree( my_name );
			return( FALSE );
		}
                PMmInit (&restore_data);
                PMmLowerOn(restore_data);
                if (PMmLoad(restore_data, &backup_data_file,
                    display_error) != OK)
                {
                    IIUGerr( S_ST053A_bad_restore_read,UG_ERR_FATAL, 0 );
                }
                else
                {
                    blRestore = TRUE;
                }
	}

	/* prepare expbuf */
	STprintf( expbuf, ERx( "%s.%s.%s.%%" ), SystemCfgPrefix, 
		  pm_host, real_pm_id );

	regexp = PMmExpToRegExp( config_data, expbuf );

	/* initialize independent and dependent resource counters */ 
	independent = dependent = 0;

	/* get the name components for this set of resources */
	len = PMmNumElem( config_data, expbuf );

	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		exec sql begin declare section;
		char *units;
		exec sql end declare section;

		/* extract parameter name from full resource */
		short_name = PMmGetElem( config_data, len - 1, name );

 		/* save dbms cache_name value */
 		if( STcompare( short_name, ERx( "cache_name" ) ) == 0 )
 			cache_name = value;
 
 		/* save dbms cache_sharing value */
 		if( STcompare( short_name, ERx( "cache_sharing" ) ) == 0 )
 			cache_sharing = value;
 
		/* skip derived resources */
		if( CRderived( name ) )
		{
			++dependent;
			continue;
		}
		++independent;

		/* get units */
		if( PMmGet( units_data, name, &units ) != OK )
			units = ERx( "" );

		/* use just parameter name from now on */
		name=short_name;
		
		/* skip db_list and assume it is only a dbms parameter */
		if( STcompare( name, ERx( "database_list" ) ) == 0 )
		{
			db_list = value;
			continue;
		}
                /* skip ml_list and assume it is only a dbms parameter */
                if( STcompare( name, ERx( "mustlog_db_list" ) ) == 0 )
                {
                        ml_list = value;
                        continue;
                }
		if( STbcompare( name, 8, ERx( "log_file" ), 8, 0 ) == 0 )
		{
			continue;
		}
		if( STbcompare( name, 8, ERx( "dual_log" ), 8, 0 ) == 0 )
		{
			continue;
		}
		if( STbcompare( name, 12, ERx( "raw_log_file" ), 12, 0 ) == 0 )
		{
			continue;
		}
		if( STbcompare( name, 12, ERx( "raw_dual_log" ), 12, 0 ) == 0 )
		{
			continue;
		}

		if (c2_form)
		{
			audlen=STlength(audlog);

			if(STbcompare(name, audlen, audlog, audlen, 1)==0)
			{
			    CVal(name+STlength(audlog), &lognum);
			    if(lognum>0 && lognum <=MAX_AUDIT_LOGS)
			    {
				audit_log_list[lognum-1]=value;
			    }
			}
			else
			{
			    exec frs loadtable :form_name params (
				name = :name,
				value = :value,
				units = :units
			    );
			}
		}
		else
		{
		    exec frs loadtable :form_name params (
			name = :name,
			value = :value,
			units = :units
		    );
		}
	}
	/*
	** Now we  have the audit logs load them in order.
	*/
	if (c2_form)
	{
		for(lognum=1; lognum<=MAX_AUDIT_LOGS; lognum++)
		{
			value=audit_log_list[lognum-1];
			if(!value)
				value="";
			exec frs loadtable :form_name auditlogs 
					(log_n=:lognum, value=:value);
		}
	}
	/* set derived_table_title */
	if( STcompare( comp_type, ERx( "dbms" ) ) == 0 )
		STcopy( ERget( S_ST036C_derived_dbms ), derived_table_title );
      	else if( STcompare( comp_type, ERx( "rms" ) ) == 0 )
      		STcopy( ERget( S_ST0544_derived_rms ), derived_table_title ); 
		/* Name server derived table title added */
	else if( STcompare( comp_type, ERx( "gcn" ) ) == 0 )
		STcopy( ERget( S_ST0541_derived_name ), derived_table_title ); 
		/* added (thaal01) b91342 */
	else if( STcompare( comp_type, ERx( "recovery" ) ) == 0 )
		STcopy( ERget( S_ST0542_derived_recovery), derived_table_title );
	else if( STcompare( comp_type, ERx( "lock" ) ) == 0 ) {
		if(!ingres_menu)
		  STcopy( ERget( S_ST0371_derived_locking ), 
		  derived_table_title );
        }
	else if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
		STcopy( ERget( S_ST036D_derived_cache ), derived_table_title );
	else if( STcompare( comp_type, ERx( "gcc" ) ) == 0 )
		STcopy( ERget( S_ST036E_derived_net ), derived_table_title );
	else if( STcompare( comp_type, ERx( "c2" ) ) == 0 )
		STcopy( ERget( S_ST0379_derived_security ), 
			derived_table_title );
	else if( STcompare( comp_type, ERx( "secure" ) ) == 0 )
		STcopy( ERget( S_ST037B_security_param_deriv ), 
			derived_table_title );
	else if( STcompare( comp_type, ERx( "star" ) ) == 0 )
		STcopy( ERget( S_ST036F_derived_star ), derived_table_title );
	else if( STcompare( real_pm_id, ERx( "rcp.log" ) ) == 0 )
		STcopy( ERget( S_ST0370_derived_logging ),
			derived_table_title );
	else if( STcompare( real_pm_id, ERx( "rcp.lock" ) ) == 0 )
		STcopy( ERget( S_ST0371_derived_locking ),
			derived_table_title );

	if( independent == 0 )
	{
		exec frs message :ERget( S_ST0321_no_independent )
			with style = popup;
		(void) generic_dependent_form( form_title, derived_table_title,
			real_pm_id, def_name, TRUE, backup_mode );
		exec frs breakdisplay;
	}

	if( dependent == 0 )
	{
		/* no dependent resources found */
		exec frs set_frs menu '' (
			active( :ERget( S_ST0352_derived_menu ) ) = 0
		);
	}
      	if( STcompare( comp_type, ERx( "rms" ) ) == 0 )
        {
      	    exec frs set_frs menu '' (
      		    active( :ERget( S_ST0352_derived_menu ) ) = 1
      	    );
        }

	exec frs end; /* form initialization */

	exec frs activate menuitem :ERget( FE_Edit ),frskey12;
	exec frs begin;

		/*
		** Check which table field we are on
		*/
		exec frs inquire_frs form  (:fldname=field);
		if(!STcompare(fldname,"params"))
		{
		     /* get parameter name */
		     exec frs getrow :form_name params ( :name_val = name );
		     /* compose name of resource to be set */
		     STprintf( temp_buf, ERx( "%s.%s.%s." ), SystemCfgPrefix,
			       pm_host, owner );
		     STcat( temp_buf, instance );
		     STcat( temp_buf, name_val );
		     doing_aud_log=FALSE;
		}
		else
		{
		     /* get auditlog name */
		     exec frs getrow :form_name auditlogs ( :lognum = log_n );
		     /* compose name of resource to be set */
		     STprintf( temp_buf, ERx( "%s.%s.c2.audit_log_%d" ), 
			       SystemCfgPrefix, pm_host,  lognum );
		     doing_aud_log=TRUE;
		}

		avoid_set_resource = FALSE; /* bug #80450 */

		/* just toggle if resource is boolean */
		if( CRtype( CRidxFromPMsym( temp_buf ) ) == CR_BOOLEAN )
		{
			/* look up current value */
			/*b69912*/
			exec frs getrow :form_name :fldname ( :bool_name_val = value );

			/* toggle boolean value */
			if( ValueIsBoolTrue( bool_name_val ) )
			    STcopy( ERx( "OFF" ), in_buf );	
			else
			{
			    /*
			    ** bug #80450 - both cache_sharing and dmcm
			    ** cannot be turned on
			    */

			    char parambuf[ BIG_ENOUGH ];

			    STcopy( bool_name_val, in_buf );

			    if ( STcompare(name_val,
					ERx("cache_sharing")) == 0 )
			    {
				STprintf( parambuf, ERx("%s.%s.%s."),
					SystemCfgPrefix, pm_host, owner );
				STcat( parambuf, instance );
				STcat( parambuf, ERx("dmcm") );
				if ( PMmGet(config_data, parambuf, &value) == OK
					&& !(ValueIsBoolTrue(value)) )
				{
				    /* 
				    ** dmcm is OFF, thus cache_sharing 
				    ** can be turned ON.
				    */
				    STcopy( ERx( "ON" ), in_buf );
				}
				else
				{
				    avoid_set_resource = TRUE;
				    exec frs message 
					:ERget( S_ST0444_dmcm_warning ) 
					with style = popup;

				}
			    }
			    else if ( STcompare(name_val,
				ERx("dmcm")) == 0 )
			    {
                                STprintf( parambuf, ERx("%s.%s.%s."),
                                        SystemCfgPrefix, pm_host, owner );
                                STcat( parambuf, instance );
                                STcat( parambuf, ERx("cache_sharing") );
                                if ( PMmGet(config_data, parambuf, &value) == OK
                                        && !(ValueIsBoolTrue(value)) )
                                {
                                    /*
                                    ** cache_sharing is OFF, thus dmcm
                                    ** can be turned ON.
                                    */
                                    STcopy( ERx( "ON" ), in_buf );
                                }
				else
                                {
                                    avoid_set_resource = TRUE;
                                    exec frs message 
                                        :ERget( S_ST0444_dmcm_warning ) 
                                        with style = popup;
 
                                }
			    }
			    else
				STcopy( ERx( "ON" ), in_buf );	
			}
		}
		else
		{
			/* prompt for new value */
			exec frs prompt (
				:ERget( S_ST0328_value_prompt ),
				:in_buf
			) with style = popup;
		}

		if( !avoid_set_resource)  /* bug #80450, do not call 
					  ** set_resource, since parameter 
					  ** value is not to be changed 
					  */
		{
		    if ( set_resource( temp_buf, in_buf ) )
		    {
			/* change succeeded */ 
			changed = TRUE;

			if( STcompare( name_val, ERx( "cache_name" ) ) == 0 )
				cache_name = in_buf;

			if( STcompare( name_val, ERx( "cache_sharing" ) ) == 0 )
				cache_sharing = in_buf;

                        /* b95887 - Is DMF cache enabled for this pg size */
                        if(( STcompare( name_val, ERx("default_page_size")) == 0) ||
                         (STcompare( name_val, ERx("blob_etab_page_size")) == 0))
                        {
                           (void)CVal(in_buf, &number);
                           if(default_page_cache_off(pm_id, &number, 
					cache_sharing, cache_name))
                              exec frs message 
                                        :ERget(S_ST0449_no_cache_pg_default)
					with style = popup;
                        }

			if ((STcompare( real_pm_id, ERx( "rcp.log" ) ) == 0) &&
			    (STcompare( name_val, ERx( "block_size") ) == 0))
				exec frs message :ERget(S_ST0533_reformat_log)
					with style = popup;

			/*
			** For various historical reasons we have 
			** two sets of log names. CBF reads in c2.audit_log_X
			** but SXF reads c2.log.audit_log_X, so make sure
			** they stay in sync
			*/
			if(doing_aud_log==TRUE)
			{
			     STprintf( temp_buf, 
				       ERx( "%s.%s.c2.log.audit_log_%d" ), 
				       SystemCfgPrefix, pm_host,  lognum );
			      (VOID) set_resource( temp_buf, in_buf ); 
			}
			/* 
			** update tablefield. Note this works since  all 
			** tablefields have a column called "value" which 
			** holds the pmvalue
			*/

			exec frs putrow :form_name :fldname ( value = :in_buf );

			/* load config.dat */
			if( PMmLoad( config_data, &config_file,
				display_error ) != OK )
			{
				IIUGerr( S_ST0315_bad_config_read,
					UG_ERR_FATAL, 0 );
			}

			/*
			** 08-apr-1997 (thaju02)
			** load protect.dat to get latest changes
			*/
			if( PMmLoad( protect_data, &protect_file,
				display_error ) != OK )
			{
				IIUGerr( S_ST0320_bad_protect_read,
					UG_ERR_FATAL, 0 );
			}
		    }
		}

	exec frs end;

	exec frs activate menuitem :dbms_second_menuitem,frskey13;
	exec frs begin;

		exec frs inquire_frs menu '' (:activated = Active (
			:ERget ( S_ST0354_databases_menu ) )
		);
		if (activated == 0)
		{
		  exec frs message :ERget( S_ST0683_menu_item_invalid )
			with style=popup;
			exec frs resume;
		}
		else
			activated = 0;

		if( def_name && dbms_dblist_form( def_name, db_list, &db_list ) )
 		  changed = TRUE;
                else
                if (!def_name)
                   exec frs message :ERget( S_ST0683_menu_item_invalid )
                        with style = popup;

	exec frs end;

	exec frs activate menuitem :ERget( S_ST0356_cache_menu ),frskey14;
	exec frs begin;

		char pm_id[ BIG_ENOUGH ];
		char *arg1, *arg2;

		exec frs inquire_frs menu '' (:activated = Active (
			:ERget ( S_ST0356_cache_menu ) )
		);
		if (activated == 0)
		{
		  exec frs message :ERget( S_ST0683_menu_item_invalid )
			with style=popup;
			exec frs resume;
		}
		else
			activated = 0;

		if((cache_name==NULL || cache_sharing==NULL) && ingres_menu)
		{
			IIUGerr( S_ST0322_missing_cache, 0, 0 );
			exec frs resume;
		}
		if( ValueIsBoolTrue( cache_sharing ) && ingres_menu )
		{
			STprintf( pm_id, ERx( "dmf.shared.%s" ), cache_name );
		}
		else
		{
			STprintf( pm_id, ERx( "dmf.private.%s" ), def_name );
		}

                if (cbf_srv_name != NULL &&
                        STcompare(cbf_srv_name, ERx("rms")) == 0)
                {
   		    arg1 = STalloc( ERget( S_ST0545_config_rms_caches ) );
   		    arg2 = STalloc( ERget( S_ST0546_rms_caches ) );
                }
                else
                {
   		    arg1 = STalloc( ERget( S_ST0442_config_caches ) );
   		    arg2 = STalloc( ERget( S_ST0443_dbms_caches ) );
                }
		if(!ingres_menu && cache_name==NULL) 
		  cache_name = STalloc( ERget( S_ST030F_default_settings ) );
		if (config_caches( arg1, arg2, pm_id, cache_name))
		    changed = TRUE;
                
		MEfree( arg1 );
		MEfree( arg2 );
	exec frs end;

	exec frs activate menuitem :ERget( S_ST0357_protocols_menu),frskey11;
	exec frs begin;
                if ( def_name  && 
		   ( STcompare( comp_type, ERx("gcc") )  == 0 ||
		     STcompare( comp_type, ERx("gcb") )  == 0 ||
		     STcompare( comp_type, ERx("gcd") )  == 0 ) )
		    changed = net_protocols_form( comp_type, def_name );
		else
		{
                   exec frs message :ERget( S_ST0683_menu_item_invalid )
                        with style = popup;
		}
	exec frs end;

	exec frs activate menuitem :vnode_registry_menu,frskey19;
	exec frs begin;
                if ( ! STcompare( comp_type, ERx("gcc") ) )
		    changed = net_protocols_form( ERx("registry"), NULL );
                else  if ( ! STcompare( comp_type, ERx("gcb") ) ) 
		    changed = bridge_protocols_form( def_name );
		else
		{
                   exec frs message :ERget( S_ST0683_menu_item_invalid )
                        with style = popup;
		}
	exec frs end;

	exec frs activate menuitem :ERget( S_ST0352_derived_menu ),frskey15;
	exec frs begin;
 		char parambuf[ BIG_ENOUGH ];
 
		exec frs inquire_frs menu '' (:activated = Active (
			:ERget ( S_ST0352_derived_menu ) )
		);
		if (activated == 0)
		{
		  exec frs message :ERget( S_ST0683_menu_item_invalid )
			with style=popup;
			exec frs resume;
		}
		else
			activated = 0;

		if( generic_dependent_form( form_title, derived_table_title,
			pm_id, def_name, FALSE, backup_mode ) )
		{
			changed = TRUE;
		}
 		/* Refresh cache_sharing and cache_name in case either is derived */
 		STprintf( parambuf, ERx("%s.%s.%s."),
 			SystemCfgPrefix, pm_host, owner );
 		STcat( parambuf, instance );
 		STcat( parambuf, ERx("cache_sharing") );
 		if (PMmGet(config_data, parambuf, &cache_sharing) != OK)
 		    cache_sharing = NULL;
 		STprintf( parambuf, ERx("%s.%s.%s."),
 			SystemCfgPrefix, pm_host, owner );
 		STcat( parambuf, instance );
 		STcat( parambuf, ERx("cache_name") );
 		if (PMmGet(config_data, parambuf, &cache_name) != OK)
 		    cache_name = NULL;

	exec frs end;

	exec frs activate menuitem :ERget( S_ST0353_reset_menu ),frskey16;
	exec frs begin;

		exec frs inquire_frs form  (:fldname=field);
		if(!STcompare(fldname,"params"))
		{
			exec frs getrow :form_name params ( :name_val = name );
			STprintf( temp_buf, ERx( "%s.%s.%s." ), 
				  SystemCfgPrefix, pm_host, owner );
			STcat( temp_buf, instance );
			STcat( temp_buf, name_val );
                        PMmGet( restore_data, temp_buf, &value);
			doing_aud_log=FALSE;
		}
		else
		{
			exec frs getrow :form_name auditlogs (:lognum = log_n,
					:name_val=value);
			if(name_val[0]==EOS)
				exec frs resume;
			STprintf(name_val,"audit_log_%d",lognum);
			STprintf( temp_buf, "%s.%s.c2.%s", SystemCfgPrefix,
				  pm_host, name_val);
                        PMmGet( restore_data, temp_buf, &value);
			doing_aud_log=TRUE;
		}


		if( CONFCH_YES != IIUFccConfirmChoice( CONF_GENERIC,
			NULL, name_val, NULL, NULL,
			S_ST0329_reset_prompt,
			S_ST0382_reset_parameter,
			S_ST0383_no_reset_parameter,
			NULL, TRUE ) )
		{
			exec frs resume;
		}

		if( reset_resource( temp_buf, &value, TRUE ) )
		{
			exec frs putrow :form_name :fldname ( value = :value );
			changed = TRUE;
			if(doing_aud_log==TRUE)
			{
			    /* Reset the other name as well */
			    STprintf( temp_buf, "%s.%s.c2.log.%s",
				      SystemCfgPrefix, pm_host, name_val);
			    (VOID) reset_resource( temp_buf, &value, TRUE ) ;
			}
		}

	exec frs end;

	exec frs activate menuitem :ERget( S_ST0453_mechanisms_menu ),frskey17;
	exec frs begin;
                if ( ! STcompare( comp_type, ERx("gcf") ) )
		    changed = security_mech_form();
		else
		{
                   exec frs message :ERget( S_ST0683_menu_item_invalid )
                        with style = popup;
		}
	exec frs end;

	exec frs activate menuitem :ERget( S_ST035F_change_log_menu ),frskey18;
	exec frs begin;
		if( browse_file( &change_log_file,
			ERget( S_ST0347_change_log_title ), TRUE ) != OK )
		{
			exec frs message :ERget( S_ST0348_no_change_log )
				with style = popup;
		}
	exec frs end;

	exec frs activate menuitem :dbms_third_menuitem,frskey20;
	exec frs begin;

		exec frs inquire_frs menu '' (:activated = Active (
			:ERget ( S_ST06C3_mustlogDB_menu ) )
		);
		if (activated == 0)
		{
		  exec frs message :ERget( S_ST0683_menu_item_invalid )
			with style=popup;
			exec frs resume;
		}
		else
			activated = 0;

		if( def_name && dbms_mllist_form( def_name, ml_list, &ml_list ) )
 		  changed = TRUE;
                else
                if (!def_name)
                   exec frs message :ERget( S_ST0683_menu_item_invalid )
                        with style = popup;

	exec frs end;

	exec frs activate menuitem :ERget( FE_Help ), frskey1;
	exec frs begin;

		char help_id[ BIG_ENOUGH ];

		exec frs getrow :form_name params ( :name_val = name );
		STprintf( help_id, ERx( "%s.%s" ), owner, name_val );
		if( !get_help( help_id ) )
			exec frs message :ERget( S_ST0346_no_help_available ) 
				with style = popup;
	exec frs end;

	exec frs activate menuitem :ERget( FE_End ), frskey3;
	exec frs begin;

		if( backup_mode && changed )
		{
			char *p, *object;
			i4 choice;

			p = object = STalloc( description );
			while( CMwhite( p ) )
				CMnext( p );

			choice = IIUFccConfirmChoice( CONF_END,
				NULL, p, NULL, NULL );

			MEfree( object );

			if( choice == CONFCH_NO )	
			{
				/* reinitialize protect.dat context */ 
				PMmFree( protect_data );
				(void) PMmInit( &protect_data );
				PMmLowerOn( protect_data );

				/* reinitialize config.dat context */ 
				PMmFree( config_data );
				(void) PMmInit( &config_data );
				PMmLowerOn( config_data );

				CRsetPMcontext( config_data );

				exec frs message
					:ERget( S_ST031B_restoring_settings );

				/* restore protected data file */
				if( PMmLoad( protect_data,
					&backup_protect_file,
					display_error ) != OK )
				{
					IIUGerr( S_ST0315_bad_config_read,
						UG_ERR_FATAL, 0 );
				}
				if (PMmWrite(protect_data, &protect_file) != 
					OK)
				{
					IIUGerr( S_ST0314_bad_config_write,
						UG_ERR_FATAL, 0 );
				}

				/* restore configuration data file */
				if( PMmLoad( config_data, &backup_data_file,
					display_error ) != OK )
				{
					IIUGerr( S_ST0315_bad_config_read,
						UG_ERR_FATAL, 0 );
				}
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

				/* delete backup of protected data file */
				(void) LOdelete( &backup_protect_file );

				/* delete backup of change log */
				(void) LOdelete( &backup_change_log_file );
			}
			else if( choice != CONFCH_YES )	
				exec frs resume;
		}
		exec frs breakdisplay;
	exec frs end;

	if( backup_mode )
	{
		/* remove backup files */
		LOdelete( &backup_data_file );
		LOdelete( &backup_protect_file );
		LOdelete( &backup_change_log_file );
	}

	if( my_name != NULL )
		MEfree( my_name );

        if(blRestore == TRUE)
        {
            PMmFree (restore_data);
        }

	return( changed );
}

bool
generic_dependent_form( char *form_title, char *table_title, char *pm_id,
	char *def_name, bool handle_save, bool backup_mode )
{
	exec sql begin declare section;
	char *name, *value;
	char in_buf[ BIG_ENOUGH ];
	char protect_val[ BIG_ENOUGH ];
	char name_val[ BIG_ENOUGH ];
	char *my_name = def_name;
        char temp_val[ BIG_ENOUGH ];
	char *protect;
	char description[ BIG_ENOUGH ];
	char *form_name;
	char *title = form_title;
	char temp_buf[ BIG_ENOUGH ];
	exec sql end declare section;

	LOCATION *IIUFlcfLocateForm();
	STATUS IIUFgtfGetForm();
	PM_SCAN_REC state;
	STATUS status;
	char expbuf[ BIG_ENOUGH ];
	char *regexp;
 	bool no_protect = FALSE;
	i4 len, i;
	char *owner, instance[ BIG_ENOUGH ];
	bool changed = FALSE;
	char *comp_type;
	char *p;
	char real_pm_id[ BIG_ENOUGH ];
	char *pm_host = host;
	char full_title[ BIG_ENOUGH ];
	bool c2_form=FALSE;

        STcopy(table_title, full_title);

	/* first component of pm_id identifies component type */
	comp_type = PMmGetElem( config_data, 0, pm_id );

	exec frs message :ERget( S_ST0345_selecting_derived );

	if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
	{
		form_name = ERx( "dmf_dependent" );
		/* HACK: skip past "dmf." */
		p = pm_id;
		CMnext( p );
		CMnext( p );
		CMnext( p );
		CMnext( p );
                if ( cbf_srv_name != NULL )
     		    STprintf( real_pm_id, ERx( "%s.%s" ), cbf_srv_name, p );
        	/*
        	** variable page size project: supporting multiple caches
		** append page size to table title for clarification
        	*/
        	if (PMmNumElem(config_data, pm_id) == 4)
        	{
            	    	if (STcompare(PMmGetElem(config_data, 3, pm_id), 
					ERx("p2k")) == 0)
                		STcat(full_title, ERx(" for 2k"));
            	    	if (STcompare(PMmGetElem(config_data, 3, pm_id), 
					ERx("p4k")) == 0)
                		STcat(full_title, ERx(" for 4k"));
            		if (STcompare(PMmGetElem(config_data, 3, pm_id), 
					ERx("p8k")) == 0)
                		STcat(full_title, ERx(" for 8k"));
            		if (STcompare(PMmGetElem(config_data, 3, pm_id), 
					ERx("p16k")) == 0)
                		STcat(full_title, ERx(" for 16k"));
            		if (STcompare(PMmGetElem(config_data, 3, pm_id), 
					ERx("p32k")) == 0)
                		STcat(full_title, ERx(" for 32k"));
            		if (STcompare(PMmGetElem(config_data, 3, pm_id), 
					ERx("p64k")) == 0)
                		STcat(full_title, ERx(" for 64k"));
        	}
		else
                	STcat(full_title, ERx(" for 2k"));

	}
	else
	{
		form_name = ERx( "generic_dependent" );
		STcopy( pm_id, real_pm_id );
	}

	if( IIUFgtfGetForm( IIUFlcfLocateForm(), form_name ) != OK )
	{
		FEendforms();
		EXdelete();
		F_ERROR( "\n\"%s\" form not found.\n\n", form_name );
	}

	/* center description (72 characters wide) */
	STcopy( full_title, temp_buf );
	len = (72 - STlength( temp_buf )) / 2;
	for( i = 0, p = description; i < len; i++, CMnext( p ) )
		STcopy( ERx( " " ), p );	
	STcopy( temp_buf, p );

	/* set value of my_name */
	if( STcompare( comp_type, ERx( "c2" ) ) == 0 )
		c2_form=TRUE;
	else
		c2_form=FALSE;
	if(c2_form)
		my_name=NULL;
	else if( def_name == NULL )
		my_name = STalloc( ERget( S_ST030F_default_settings ) );
	else
	{
		if( STcompare( def_name, ERx( "*" ) ) == 0 )
			my_name = STalloc(
				ERget( S_ST030F_default_settings ) );
		else
			my_name = STalloc( def_name ); 
	}

	/* extract resource owner name from real_pm_id */
	owner = PMmGetElem( config_data, 0, real_pm_id );

	len = PMmNumElem( config_data, real_pm_id );
		
	/* compose "instance" */
	if( len == 2 )
	{
		/* instance name is second component */
		name = PMmGetElem( config_data, 1, real_pm_id );
		if( STcompare( name,
			ERget( S_ST030F_default_settings ) ) == 0
		)
			STcopy( ERx( "*" ), instance );
		else
			STcopy( name, instance ); 
		STcat( instance, ERx( "." ) );
	}
	else if( len > 2 )
	{
		char *p;
		/* instance name is all except first */

		for( p = real_pm_id; *p != '.'; CMnext( p ) );
		CMnext( p );
		STcopy( p, instance );
		STcat( instance, ERx( "." ) );
	}
	else
		STcopy( ERx( "" ), instance );

	exec frs inittable :form_name params read;

	exec frs display :form_name;

	exec frs initialize (
		host = :host,
		title = :title,
		ii_installation = :ii_installation,
		ii_installation_name = :ii_installation_name,
		ii_system = :ii_system,
		ii_system_name = :ii_system_name,
		title = :title,
		description = :description
	);

	exec frs begin; /* form initialization */

        /*
        ** 08-apr-1997 (thaju02)
        ** load protect.dat to get latest changes.
        */
        PMmFree( protect_data );
        (void) PMmInit( &protect_data );
        PMmLowerOn( protect_data );
        if (PMmLoad(protect_data, &protect_file, display_error) != OK)
            IIUGerr( S_ST0320_bad_protect_read,UG_ERR_FATAL, 0 );

	if (c2_form)
		pm_host = ERx( "*" );

	/* display definition name if there is one */
	if( my_name != NULL )
		exec frs putform( def_name = :my_name );

	/* display cache_desc if this is a cache definition */
	if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
	{
		exec sql begin declare section;
		char cache_desc[ BIG_ENOUGH ];
		exec sql end declare section;

		value = PMmGetElem( config_data, 1, real_pm_id );
		if( STcompare( ERx("shared"), value ) == 0 )
			exec frs putform(
				cache_desc = :ERget( S_ST0332_shared )
			);
		else
		{
			name = STalloc( ERget( S_ST0333_private ) );
			value = STalloc( PMmGetElem( config_data, 2,
				real_pm_id ) );
			if( STcompare( value, ERx( "*" ) ) == 0 )
				value = STalloc(
					ERget( S_ST030F_default_settings ) );
			STprintf( cache_desc, ERx( "%s (%s=%s)" ), 
				name, ERget( S_ST0334_owner ), value );
			MEfree( name );
			MEfree( value );
			exec frs putform(
				cache_desc = :cache_desc
			);
		}
	}

	/* construct expbuf contents */
	STprintf( expbuf, ERx( "%s.%s.%s.%%" ), SystemCfgPrefix,
		  pm_host, real_pm_id );

	len = PMmNumElem( config_data, expbuf );

	regexp = PMmExpToRegExp( config_data, expbuf );

	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		exec sql begin declare section;
		char *units;
		exec sql end declare section;

		PM_SCAN_REC state;
		char *full_name;

		/* skip non-derived resources */
		if( ! CRderived( name ) )
			continue;

		/* get units */
		if( PMmGet( units_data, name, &units ) != OK )
			units = ERx( "" );

		/* save full resource name */
		full_name = name;

		/* extract parameter from full resource name */
		name = PMmGetElem( config_data, len - 1, name );

		/* get value of column "protected" */
		regexp = PMmExpToRegExp( protect_data, full_name );
		if( PMmScan( protect_data, regexp, &state, NULL,
			NULL, &value ) == OK
		)
			protect = ERget( S_ST0330_yes );
		else
			protect = ERget( S_ST0331_no );

		exec frs loadtable :form_name params (
			name = :name,
			value = :value,
			units = :units,
			protected = :protect
		);
	}

	exec frs end; /* form initialization */

	exec frs activate menuitem :ERget( FE_Edit ),frskey12;
	exec frs begin;
	{
		bool protected = FALSE;

		exec sql begin declare section;
		i4 record;
		char fld_val  [ BIG_ENOUGH ];
		exec sql end declare section;
       

		
                /* get parameter name and value */
                exec frs getrow :form_name params ( :name_val = name,
                                                    :fld_val = value
                                                  );

                STprintf( temp_buf, ERx( "%s.%s.%s." ), SystemCfgPrefix,
			  pm_host, owner );
                STcat( temp_buf, instance );
                STcat( temp_buf, name_val );

                if (CRtype( CRidxFromPMsym( temp_buf ) ) == CR_BOOLEAN )
                {
                    exec frs getrow :form_name params ( :temp_val = value );
                    /* toggle boolean value */
                    if ( ValueIsBoolTrue( temp_val) ) 
                        STcopy( ERx( "OFF"), in_buf );
                    else
                        STcopy( ERx("ON"), in_buf );
                }
                else {

		    /* prompt for new value */
 	               exec frs prompt (
			:ERget( S_ST0328_value_prompt ),
			:in_buf
              	       ) with style = popup;
                }

		/* remove from protected resource to change */
		if( PMmDelete( protect_data, temp_buf ) == OK )
			protected = TRUE;

		if( set_resource( temp_buf, in_buf ) )
		{
			/* change succeeded */ 
			changed = TRUE;

			if( protected )
			{
				/* reinsert value into protect data set */
				(void) PMmInsert( protect_data, temp_buf,
					in_buf );
			}

			/* load config.dat */
			if( PMmLoad( config_data, &config_file,
				display_error ) != OK )
			{
				IIUGerr( S_ST0315_bad_config_read,
					UG_ERR_FATAL, 0 );
			}

			/*
			** 08-apr-1997 (thaju02)
			** load protect.dat to get latest changes.
			*/
			if( PMmLoad( protect_data, &protect_file,
				display_error ) != OK )
			{
				IIUGerr( S_ST0320_bad_protect_read,
					UG_ERR_FATAL, 0 );
			}

			/* clear tablefield */
                        exec frs clear field params; 

			regexp = PMmExpToRegExp( config_data, expbuf );

			/* reload tablefield - contents may have changed */
			for( status = PMmScan( config_data, regexp,
				&state, NULL, &name, &value ); status == OK;
				status = PMmScan( config_data, NULL, &state,
				NULL, &name, &value ) )
			{
				exec sql begin declare section;
				char *units;
				exec sql end declare section;

				PM_SCAN_REC state;
				char *full_name;

				/* skip non-derived resources */
				if( ! CRderived( name ) )
					continue;

				/* get units */
				if( PMmGet( units_data, name, &units ) != OK )
					units = ERx( "" );

				/* save full resource name */
				full_name = name;
		
				/* extract parameter from full resource name */
				name = PMmGetElem( config_data, len - 1,
					name );

				/* get value of column "protected" */
				if( no_protect )
					protect = ERget( S_ST0331_no );
				else
				{
					regexp = PMmExpToRegExp( protect_data,
						full_name );
					if( PMmScan( protect_data, regexp,
						&state, NULL, NULL, &value )
						== OK
					)
						protect =
							ERget( S_ST0330_yes );
					else
						protect =
							ERget( S_ST0331_no );
				}

				exec frs loadtable :form_name params (
					name = :name,
					value = :value,
					units = :units,
					protected = :protect
				);
			}

			/* scroll back to selected row */
			name = STalloc( name_val );
			exec frs unloadtable :form_name params (
				:name_val = name,
				:record = _record
			);
			exec frs begin;

				if( STcompare( name_val, name ) == 0 )
				{
					exec frs scroll :form_name params
						to :record;
					break;
				}

			exec frs end;	
			MEfree( name );
		}
		else if (protected)
			(void) PMmInsert( protect_data, temp_buf, fld_val );
	}

	exec frs end;

	exec frs activate menuitem :ERget( S_ST0358_protect_menu ),frskey16;
	exec frs begin;

		/* 
		** 08-apr-1997 (thaju02)
		** load protect.dat to get latest changes.
                PMmFree( protect_data );
                (void) PMmInit( &protect_data );
                PMmLowerOn( protect_data );
		if (PMmLoad(protect_data, &protect_file, display_error) != OK)
		    IIUGerr( S_ST0320_bad_protect_read,UG_ERR_FATAL, 0 );
		*/

		/* check whether resource is protected */
		exec frs getrow :form_name params (
			:name_val = name,
			:temp_val = value,
			:protect_val = protected
		);

		STprintf( temp_buf, ERx( "%s.%s.%s." ), SystemCfgPrefix,
			  pm_host, owner );
		STcat( temp_buf, instance );
		STcat( temp_buf, name_val );

		if( STequal( protect_val, ERget( S_ST0330_yes ) ) != 0 )
		{
			(void) PMmDelete( protect_data, temp_buf );
			protect = ERget( S_ST0331_no );
		}
		else
		{
			(void) PMmInsert( protect_data, temp_buf, temp_val );
			protect = ERget( S_ST0330_yes );
		}

		exec frs putrow :form_name params (
			protected = :protect
		);

		changed = TRUE;

		if( PMmWrite( protect_data, &protect_file ) != OK )
			IIUGerr( S_ST0314_bad_config_write, UG_ERR_FATAL, 0 );

	exec frs end;

	exec frs activate menuitem :ERget( S_ST035C_recalculate_menu ),frskey17;
	exec frs begin;

		/* check whether resource is protected */
		exec frs getrow :form_name params (
			:protect_val = protected
		);
		if( STequal( protect_val, ERget( S_ST0330_yes ) ) != 0 )
		{
			exec frs message :ERget( S_ST032B_resource_protected )
				with style = popup;
			exec frs resume;
		}

		exec frs getrow :form_name params ( :name_val = name );

		if( CONFCH_YES != IIUFccConfirmChoice( CONF_GENERIC,
			NULL, name_val, NULL, NULL,
			S_ST032A_recalculate_prompt,
			S_ST0380_recalc_parameter,
			S_ST0381_no_recalc_parameter,
			NULL, TRUE ) )
		{
			exec frs resume;
		}

		STprintf( temp_buf, ERx( "%s.%s.%s." ), SystemCfgPrefix,
			  pm_host, owner );
		STcat( temp_buf, instance );
		STcat( temp_buf, name_val );

		if( reset_resource( temp_buf, &value, TRUE ) )
		{
			exec sql begin declare section;
			i4 record;
			exec sql end declare section;

			changed = TRUE;

			/* clear tablefield */
			exec frs clear field params;

			regexp = PMmExpToRegExp( config_data, expbuf );

			/* reload tablefield - contents may have changed */
			for( status = PMmScan( config_data, regexp,
				&state, NULL, &name, &value ); status == OK;
				status = PMmScan( config_data, NULL, &state,
				NULL, &name, &value ) )
			{
				exec sql begin declare section;
				char *units;
				exec sql end declare section;

				PM_SCAN_REC state;
				char *full_name;

				/* skip non-derived resources */
				if( ! CRderived( name ) )
					continue;

				/* get units */
				if( PMmGet( units_data, name, &units ) != OK )
					units = ERx( "" );

				/* save full resource name */
				full_name = name;
		
				/* extract parameter from full resource name */
				name = PMmGetElem( config_data, len - 1,
					name );

				/* get value of column "protected" */
				if( no_protect )
					protect = ERget( S_ST0331_no );
				else
				{
					regexp = PMmExpToRegExp( protect_data,
						full_name );
					if( PMmScan( protect_data, regexp,
						&state, NULL, NULL, &value )
						== OK
					)
						protect =
							ERget( S_ST0330_yes );
					else
						protect =
							ERget( S_ST0331_no );
				}

				exec frs loadtable :form_name params (
					name = :name,
					value = :value,
					units = :units,
					protected = :protect
				);
			}

			/* scroll back to selected row */
			name = STalloc( name_val );
			exec frs unloadtable :form_name params (
				:name_val = name,
				:record = _record
			);
			exec frs begin;

				if( STcompare( name_val, name ) == 0 )
				{
					exec frs scroll :form_name params
						to :record;
					break;
				}

			exec frs end;	
			MEfree( name );
		}
	exec frs end;

	exec frs activate menuitem :ERget( S_ST035F_change_log_menu ),frskey18;
	exec frs begin;
		if( browse_file( &change_log_file,
			ERget( S_ST0347_change_log_title ), TRUE ) != OK )
		{
			exec frs message :ERget( S_ST0348_no_change_log )
				with style = popup;
		}
	exec frs end;

	exec frs activate menuitem :ERget( FE_Help ), frskey1;
	exec frs begin;

		char help_id[ BIG_ENOUGH ];

		exec frs getrow :form_name params ( :name_val = name );
		STprintf( help_id, ERx( "%s.%s" ), owner, name_val );
		if( !get_help( help_id ) )
			exec frs message 'No help available.'
				with style = popup;
	exec frs end;

	exec frs activate menuitem :ERget( FE_End ), frskey3;
	exec frs begin;

		if( handle_save && backup_mode && changed )
		{
			char *p, *object;
			i4 choice;

			p = object = STalloc( description );
			while( CMwhite( p ) )
				CMnext( p );

			choice = IIUFccConfirmChoice( CONF_END,
				NULL, p, NULL, NULL );

			MEfree( object );

			if( choice == CONFCH_NO )	
			{
				/* reinitialize protect.dat context */ 
				PMmFree( protect_data );
				(void) PMmInit( &protect_data );
				PMmLowerOn( protect_data );

				/* reinitialize config.dat context */ 
				PMmFree( config_data );
				(void) PMmInit( &config_data );
				PMmLowerOn( config_data );

				CRsetPMcontext( config_data );

				exec frs message
					:ERget( S_ST031B_restoring_settings );

				/* restore protected data file */
				if( PMmLoad( protect_data,
					&backup_protect_file,
					display_error ) != OK )
				{
					IIUGerr( S_ST0315_bad_config_read,
						UG_ERR_FATAL, 0 );
				}
				if (PMmWrite(protect_data, &protect_file) != 
					OK )
				{
					IIUGerr( S_ST0314_bad_config_write,
						UG_ERR_FATAL, 0 );
				}

				/* restore configuration data file */
				if( PMmLoad( config_data, &backup_data_file,
					display_error ) != OK )
				{
					IIUGerr( S_ST0315_bad_config_read,
						UG_ERR_FATAL, 0 );
				}
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

				/* delete backup of protected data file */
				(void) LOdelete( &backup_protect_file );

				/* delete backup of change log */
				(void) LOdelete( &backup_change_log_file );
			}
			else if( choice != CONFCH_YES )	
				exec frs resume;
		}
		exec frs breakdisplay;

	exec frs end;

	if( my_name != NULL )
		MEfree( my_name );	
	return( changed );
}

bool
config_caches( char *form_title, char *table_title, char *pm_id, char *def_name)
{
    exec sql begin declare section;
    char	cache_type[ BIG_ENOUGH ]; 
    char	status_flag[ BIG_ENOUGH ];
    char 	description[ BIG_ENOUGH ];
    char 	temp_buf[ MAX_LOC + 1 ];
    char	*title = form_title;
    char	*config_cache = ERx( "config_cache" );
    char	cache_name[ BIG_ENOUGH ];
    char	*stat_val;
    char	fldname[33];
    char	*my_name, *name, *value;
    char	cache_desc[BIG_ENOUGH];
    char	*default_cache;
    exec sql end declare section;
 
    LOCATION 	*IIUFlcfLocateForm();
    STATUS 	IIUFgtfGetForm();
    char	*p;
    i4		len, i;
    char	*regexp;
    char	new_pm_id[BIG_ENOUGH];
    char	arg1[BIG_ENOUGH];
    char	arg2[BIG_ENOUGH];
    bool 	changed = FALSE;
 
    STcopy(def_name, cache_name);
    STcopy(pm_id, new_pm_id);
    STcopy(ERget(S_ST0362_configure_cache), arg1);
    STcopy(ERget(S_ST0363_cache_parameters), arg2);

    /* set value of def_name */
    if( def_name == NULL )
	my_name = STalloc( ERget( S_ST030F_default_settings ) );
    else
    {
    	if( STcompare( def_name, ERx( "*" ) ) == 0 )
    	    my_name = STalloc( ERget( S_ST030F_default_settings ) );
    	else
    	    my_name = STalloc( def_name );
    }

    /* display cache_desc */
    value = PMmGetElem( config_data, 1, pm_id );
    if( STcompare( ERx("shared"), value ) == 0 )
	STcopy(ERget( S_ST0332_shared ), cache_desc);
    else
    {
	name = STalloc( ERget( S_ST0333_private ) );
	value = STalloc( PMmGetElem( config_data, 2, pm_id ) );
	if( STcompare( value, ERx( "*" ) ) == 0 )
	    value = STalloc( ERget( S_ST030F_default_settings ) );
	STprintf( cache_desc, ERx( "%s ( %s=%s )" ),
			name, ERget( S_ST0334_owner ), value );
	MEfree( name );
	MEfree( value );
    }

    /*
    ** center table description (62 characters wide)
    */
    STcopy( table_title, temp_buf );
    len = (62 - STlength( temp_buf )) / 2;
    for( i = 0, p = description; i < len; i++, CMnext( p ) )
	STcopy( ERx( " " ), p );
    STcopy( temp_buf, p );

    /*
    ** get form from form index file
    */
    if( IIUFgtfGetForm( IIUFlcfLocateForm(), config_cache ) != OK )
    {
	FEendforms();
	EXdelete();
	F_ERROR( "\nbad news: form \"%s\" not found.\n\n", 
		(char *) config_cache );
    }

    exec frs inittable :config_cache components read;
 
    exec frs display :config_cache;
 
    exec frs initialize 
    (
	title = :title,
	host = :host,
	ii_installation = :ii_installation,
	ii_installation_name = :ii_installation_name,
	ii_system = :ii_system,
	ii_system_name = :ii_system_name,
	def_name = :my_name,
	cache_desc = :cache_desc,
	description = :description
    );

    exec frs begin;
	exec frs set_frs menu ''
	( active( :ERget( FE_Edit ) ) = 1 );
	exec frs set_frs menu ''
	( active( :ERget( S_ST0350_configure_menu ) ) = 1 );
	exec frs set_frs menu ''
	( active( :ERget( FE_End ) ) = 1 );
  
	/*
	** check if dmf cache for 2k pages is already setup/inuse
	*/
	get_cache_status( pm_id, ERget( S_ST0445_p2k ), &stat_val);
	exec frs loadtable :config_cache components
	(
	    cache = :ERget( S_ST0431_dmf_cache_2k ),
	    status = :stat_val
	);
	/*
	** check if dmf cache for 4k pages is already setup/inuse
	*/
	get_cache_status( pm_id, ERget( S_ST0437_p4k ), &stat_val);
	exec frs loadtable :config_cache components
	(
	    cache = :ERget( S_ST0432_dmf_cache_4k ),
	    status = :stat_val
	);

	get_cache_status( pm_id, ERget( S_ST0438_p8k ), &stat_val);
        exec frs loadtable :config_cache components
        (
            cache = :ERget( S_ST0433_dmf_cache_8k ),
            status = :stat_val
        );

	get_cache_status( pm_id, ERget( S_ST0439_p16k ), &stat_val);
        exec frs loadtable :config_cache components
        (
            cache = :ERget( S_ST0434_dmf_cache_16k ),
            status = :stat_val
        );

	get_cache_status( pm_id, ERget( S_ST0440_p32k ), &stat_val);
        exec frs loadtable :config_cache components
        (
            cache = :ERget( S_ST0435_dmf_cache_32k ),
            status = :stat_val
        );

        if ((cbf_srv_name != NULL) && 
                 (STcompare(cbf_srv_name, ERx("dbms")) == 0))
        {
             get_cache_status( pm_id, ERget( S_ST0441_p64k ), &stat_val );
             exec frs loadtable :config_cache components
             (
                 cache = :ERget( S_ST0436_dmf_cache_64k ),
                 status = :stat_val
             );
        }
    exec frs end;

    exec frs activate menuitem :ERget( S_ST0350_configure_menu ), frskey11;
    exec frs begin;
	exec frs getrow :config_cache components
	(
	    :cache_type = 'cache',
	    :status_flag = status
	);

	if (STcompare(cache_type, ERget( S_ST0431_dmf_cache_2k )) == 0)
	{
	    if(!ingres_menu)
		STcat(new_pm_id, ERx(".p2k"));
	    STcat(arg1, ERx(" for 2k Buffers"));
	    STcat(arg2, ERx(" for 2k Buffers"));
	}
	if (STcompare(cache_type, ERget( S_ST0432_dmf_cache_4k )) == 0)
	{
	    STcat(new_pm_id, ERx(".p4k"));
	    STcat(arg1, " for 4k Buffers");
	    STcat(arg2, " for 4k Buffers");
	}
	if (STcompare(cache_type, ERget( S_ST0433_dmf_cache_8k )) == 0)
	{
	    STcat(new_pm_id, ERx(".p8k"));
	    STcat(arg1, " for 8k Buffers");
	    STcat(arg2, " for 8k Buffers");
	}
	if (STcompare(cache_type, ERget( S_ST0434_dmf_cache_16k )) == 0)
	{
	    STcat(new_pm_id, ERx(".p16k"));
	    STcat(arg1, " for 16k Buffers");
	    STcat(arg2, " for 16k Buffers");
	}
	if (STcompare(cache_type, ERget( S_ST0435_dmf_cache_32k )) == 0)
	{
	    STcat(new_pm_id, ERx(".p32k"));
	    STcat(arg1, " for 32k Buffers");
	    STcat(arg2, " for 32k Buffers");
	}
	if (STcompare(cache_type, ERget( S_ST0436_dmf_cache_64k )) == 0)
	{
            if ((cbf_srv_name != NULL) &&
                 (STcompare(cbf_srv_name, ERx("dbms")) == 0))
   	    {
                STcat(new_pm_id, ERx(".p64k"));
   	        STcat(arg1, " for 64k Buffers");
   	        STcat(arg2, " for 64k Buffers");
            }
	}

	if (generic_independent_form( arg1, arg2, new_pm_id, 
			def_name ))
	    changed = TRUE; 

	/* refresh parameters */
	STcopy(pm_id, new_pm_id);
	STcopy(ERget(S_ST0362_configure_cache), arg1);
	STcopy(ERget(S_ST0363_cache_parameters), arg2);
	exec frs resume;
    exec frs end;

    exec frs activate menuitem :ERget( FE_Edit ), frskey12;
    exec frs begin;
        exec frs getrow :config_cache components
        (
            :cache_type = 'cache',
            :status_flag = status
        );
 
	if (STcompare(cache_type, ERget( S_ST0431_dmf_cache_2k )) == 0)
	    changed = toggle_cache_status(config_cache, fldname,
				pm_id, ERget( S_ST0445_p2k ));
        if (STcompare(cache_type, ERget( S_ST0432_dmf_cache_4k )) == 0)
            changed = toggle_cache_status(config_cache, fldname,
                                pm_id, ERget( S_ST0437_p4k ));
        if (STcompare(cache_type, ERget( S_ST0433_dmf_cache_8k )) == 0)
            changed = toggle_cache_status(config_cache, fldname,
                                pm_id, ERget( S_ST0438_p8k ));
        if (STcompare(cache_type, ERget( S_ST0434_dmf_cache_16k )) == 0)
            changed = toggle_cache_status(config_cache, fldname,
                                pm_id, ERget( S_ST0439_p16k ));
        if (STcompare(cache_type, ERget( S_ST0435_dmf_cache_32k )) == 0)
            changed = toggle_cache_status(config_cache, fldname,
                               pm_id, ERget( S_ST0440_p32k ));
        if (STcompare(cache_type, ERget( S_ST0436_dmf_cache_64k )) == 0)
            if ((cbf_srv_name != NULL) &&
                 (STcompare(cbf_srv_name, ERx("dbms")) == 0))
                changed = toggle_cache_status(config_cache, fldname,
                              pm_id, ERget( S_ST0441_p64k ));
        exec frs resume;
    exec frs end;

    exec frs activate menuitem :ERget( FE_Help ), frskey1;
    exec frs begin;
 
	if( !get_help( "dbms.buf_caches" ) )
	    exec frs message :ERget( S_ST0346_no_help_available )
                                with style = popup;
    exec frs end;

    exec frs activate menuitem :ERget( FE_End ), frskey3;
    exec frs begin;
	exec frs breakdisplay;
    exec frs end;
    return(changed);
}

void
get_cache_status(char *pm_id, char *page, char **stat_val)
{
    PM_SCAN_REC	state;
    char	real_pm_id[ BIG_ENOUGH ];
    char	expbuf[ BIG_ENOUGH ];
    char	*regexp;
    char	*p;
    char	*name;
    char	*pm_host = host;

    p = pm_id;
    CMnext( p );
    CMnext( p );
    CMnext( p );
    CMnext( p );
    if (cbf_srv_name != NULL)
        STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ),
                        cbf_srv_name, p, page);
    STprintf( expbuf, ERx( "%s.%s.%s" ), SystemCfgPrefix, pm_host, real_pm_id );
    if ( PMmGet ( config_data, (char *)&expbuf, stat_val ) != OK )
	*stat_val = ERx( "" );
}

bool
toggle_cache_status(char *form, char *fldname, char *pm_id, char *page)
{
    exec sql begin declare section;
    char	field[33];
    char	*formname = form;
    exec sql end declare section;
    char	*p;
    char        real_pm_id[BIG_ENOUGH];
    char        expbuf[BIG_ENOUGH];
    char        *regexp;
    char	*pm_host = host;
    char	status_flag[4];
    char	tmppage[6];

    STcopy(page, tmppage);

    exec frs inquire_frs form (:field = field);

    exec frs getrow :formname :field (:status_flag = status);

    p = pm_id;
    CMnext( p );
    CMnext( p );
    CMnext( p );
    CMnext( p );
    if (cbf_srv_name != NULL)
        STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ),
                      cbf_srv_name, p, tmppage );
    STprintf( expbuf, ERx( "%s.%s.%s" ), SystemCfgPrefix, pm_host, real_pm_id );
    
    if (ValueIsBoolTrue(status_flag))
    {
	STcopy(ERx("OFF"), status_flag);
    }
    else
    {
	STcopy(ERx("ON"), status_flag);
    }

    exec frs putrow :formname :field (status = :status_flag);

    if (set_resource(expbuf, status_flag))
    {
	if (PMmLoad(config_data, &config_file, display_error) != OK)
	    IIUGerr( S_ST0315_bad_config_read,UG_ERR_FATAL, 0 );

	/*
	** 08-apr-1997 (thaju02) load protect.dat to get latest changes
	*/
	if (PMmLoad(protect_data, &protect_file, display_error) != OK)
	    IIUGerr( S_ST0320_bad_protect_read,UG_ERR_FATAL, 0 );

	exec frs redisplay;
	exec frs sleep 3;
    }

    return(TRUE);
}

/* default_page_cache_off() returns TRUE if the respective DMF cache
** has not been enabled.
*/
bool
default_page_cache_off(char *pm_id, i4 *page_size, char *cache_share,
					char *cache_name)
{
    PM_SCAN_REC state;
    char        real_pm_id[ BIG_ENOUGH ];
    char        expbuf[ BIG_ENOUGH ];
    char        private[ BIG_ENOUGH ];
    char        *regexp;
    char        *p;
    char        *name;
    char        *stat_val;
    char        *pm_host = host;
    bool        shared;

    if(STcompare(cache_share, ERx("ON")) == 0)
    {
       shared = TRUE;
       STcopy(ERget( S_ST0332_shared ), private);
    }
    else
    {
       STcopy(ERget( S_ST0333_private ), private);
       shared = FALSE;
    }

    /* If shared cache the config.dat entry has cache_name else we use
    ** the config identity.
    */
    if(shared)
    {
       STcat(private, ERx("."));
       STcat(private, cache_name);
    }
    else
    {
       /* pm_id of format dbms.* or dbms.ident move point to '.' */
       p = pm_id;
       CMnext( p );
       CMnext( p );
       CMnext( p );
       CMnext( p );
       STcat(private, p);
    }
    p = private;

    if (cbf_srv_name != NULL)
    {
       switch(*page_size)
       {
         case P2K:
               STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ), 
                         cbf_srv_name, p, ERget( S_ST0445_p2k ) );
               break;
         case P4K:
               STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ),
                         cbf_srv_name, p, ERget( S_ST0437_p4k ));
   
               break;
         case P8K:
               STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ),
                         cbf_srv_name, p, ERget( S_ST0438_p8k ));
               break;
         case P16K:
               STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ),
                         cbf_srv_name, p, ERget( S_ST0439_p16k ));
               break;
         case P32K:
               STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ),
                         cbf_srv_name, p, ERget( S_ST0440_p32k ));
               break;
         case P64K:
               STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ),
                         cbf_srv_name, p, ERget( S_ST0441_p64k ));
               break;
       }
    }
    STprintf( expbuf, ERx( "%s.%s.%s" ), SystemCfgPrefix, pm_host, real_pm_id );
    if ( PMmGet ( config_data, (char *)&expbuf, &stat_val ) != OK )
        stat_val = ERx( "" );

    if(STcompare(stat_val, ERx("OFF")) == 0)
       return(TRUE);
    else
       return(FALSE);
}
