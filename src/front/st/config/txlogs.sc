/*
**  Copyright (c) 1992, 2005 Ingres Corporation
**
**
**  Name: txlogs.sc -- contains function which controls the transaction
**	log configuration form of the OpenINGRES Configuration Utility.
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	16-feb-93 (tyler)
**		Added code (totally untested due to dependency on "rcpstat"
**		utility) to lend some functionality to this form.
**	21-jun-93 (tyler)
**		Fixed many bugs.
**	23-jul-93 (tyler)
**		Added missing functionality.
**	19-oct-93 (tyler)
**		The code sections which call csinstall to install
**		shared memory and deal with "raw" transaction logs
**		is now included using #ifdef UNIX (rather than
**		#ifndef VMS) directives.  Modified transaction_logs_form()
**		to return a bitmask which tells the top level form
**		which logs are enabled.  Changed type of global variable
**		"host" to (char *).
**	22-nov-93 (tyler)
**		Changed message identifiers.
**	22-nov-93 (tyler)
**		Don't assume NMgtIngAt() returns with FAIL STATUS if
**		symbol isn't defined.  Copy buffer which is returned by
**		NMgtIngAt().  Only call "iichkraw" if UNIX defined. 
**		Fixed various other bugs.
**	01-dec-93 (tyler)
**		Fixed bad calls to IIUGerr().  Make this form read-only
**		only when "rcpstat -online" exits OK.
**	05-jan-94 (tyler)
**		Fixed typo in display_message() callback which caused
**		forminit error messages.  Fixed BUG 56871 which prevented 
**		lookup of raw transaction log size.
**	31-jan-94 (tyler)
**		Don't blindly remove shared memory (on Unix) before
**		exiting without first checking whether the server is
**		on-line, because it may have been started since the
**		screen was entered.  Pipe output of "rcpstat -online"
**		to /dev/null in case server has been shut down so
**		CSinitiate() error messages aren't spewed to the screen.
**	22-feb-94 (tyler)
**		Fixed BUG 59321 introduced by previous change.  Fixed BUG
**		59434: asssign Help menu item to PF2.  Fixed BUG 59023:
**		added prompt to Create operation explaining that both
**		transaction logs must be deleted in order to change
**		transaction log sizes.
**	30-nov-94 (nanpr01)
**		Fixed BUG 60371. Added frskey mappings so that key mappings
**		are uniform in CBF from screen to screen.
**	18-jul-95 (duursma)
**		As part of fix for BUG 69827, removed #ifdef VMS code that
**		would call NMgtAt() instead of NMgtIngAt() to translate
**		II_DUAL_LOG and II_DUAL_LOG_NAME.  VMS now calls its new
**		NMgtIngAt() just like Unix.
**	23-aug-95 (albany)  Bug #69432
**		Added code to display log file info on clustered VMS
**		installations.  In transaction_logs_form(), the host 
**		name must be appended to the log file name.
**	29-sep-95 (albany)
**		Remove incorrect preprocessor assumption on previous
**		fix; correct formatting.
**	02-oct-95 (duursma)
**		Put back fix for bug #69560 by harpa06 which was lost earlier:
**		Change minimum transaction log size from 8 to 16 MB.
**	16-oct-95 (tutto01)
**		Changed "/dev/null" to "NUL" for the output of rcpstat on
**		Windows NT.
**    	12-mar-96 (hanch04)
**        	Solaris 2.5 returns 0x7FFFFFFF for size in the stat call in
**		LOinfo for a raw partition.  2.4 and earlier returned 0.
**		We must check for both.
**	22-mar-96 (nick)
**	    Remove above change to test for magic number 0x7FFFFFFF by
**	    hanch04 ; it missed the case for a raw dual log and should
**	    be fixed in LO anyway.
**	17-oct-96 (mcgem01)
**		Misc error messages now take product name as a variables.
**		Replace hard-coded 'ii' references with SystemCfgPrefix.
**	03-nov-1997 (canor01)
**	    Add support for files greater than 2GB in size.  Replace 
**	    rcp.file.size with rcp.file.kbytes.
**      20-jul-1998 (nanpr01)
**          Added support for stripped log project.
**	24-aug-1998 (hanch04)
**	    Added support for stripped log project using multiple log locations
**	    instead of multiple log files.
**	    Added insert and delete location options.
**	01-sep-1998 (hanch04)
**	    Need to get return status from PMmGet.  It's not guaranteed to 
**	    return NULL for a config value that is not found.
**	17-Sep-1999 (bonro01)
**	    Modified to support Unix Clusters.
**	03-nov-1999 (hanch04)
**	    Added increase_log_partions and decrease_log_partions to help
**	    calculate the number of log_writer threads through a rule.
**	20-jan-2000 (hanch04)
**	    Bug #100091
**	    Set num_part for number of log partitions to be LG_MAX_FILE if 
**	    the maximum number of parts was read in.
**	    Restrict the minimum log file partition size to be 16 meg.
**	28-feb-2000 (somsa01)
**	    To fully support LARGEFILE64, changed the type of size-keeping
**	    variables relevent to log size to OFFSET_TYPE in
**	    transaction_log_form().
**	02-mar-2000 (hanch04)
**	    Bug #100091
**	    Corrrectly set num_part to be the number of log parts read in.
**	    After destroying the log, config.dat must be written to disk.
**	01-may-2000 (somsa01)
**	    Updated use of 'iichkraw' to reflect the product prefix.
**	02-may-2000 (hanch04)
**	    Cast the log file size in meg to an i4.
**	16-jun-2000 (somsa01)
**	    The product prefix of 'iichkraw' should be SystemCfgPrefix,
**	    not systemVarPrefix.
**	16-jun-2000 (hanch04)
**	    Dual log needs to look for file.kbytes.  This change already 
**	    was made for the primary log.
**	    iraw2 should be set to true if the dual log is raw.
**	19-jun-2000 (hanch04)
**	    decrease_log_partions when destroying a raw log.
**	17-aug-2000 (hanch04)
**	    Before destroying the primary/dual log file it should be disabled
**	    so that if the other log file is active, the header will be
**	    updated.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-sep-2000 (hanch04)
**	    If the disable fails before destroying, continue anyway.
**	23-jul-2001 (devjo01)
**	    Use CX to determine if and how we "decorate" log file 
**	    names for clustering.  Correct a few memory stomps.
**	01-Aug-2002 (devjo01)
**	    Make sure "decorated" transaction file name does not exceed
**	    DI_FILENAME_MAX chars in length.
**	08-Oct-2002 (devjo01)
**	    Establish default PM context from 'node' when playing with
**	    logs.
**	19-jun-2003 (drivi01)
**	    Updated code to have cbf look for a record of a raw log in  
**	    config.dat in the case of someone deleting a raw log manually  
**	    from $II_SYSTEM/ingres/log, and allow user to destroy it.
**	17-jun-2004 (xu$we01)
**	    Now "yes" or "no" will be displayed in the "Raw" column of
**	    CBF Transaction Log Configuration screen (bug 112517).
**	28-Sep-2004 (lakvi01)
**	    Stopped creation of secondary log file, if the actual size of
**	    primary log file is not same as the one mentioned in config.dat
**      28-jan-2005 (zhahu02)
**          Updated so that when on-line, the log file cannot be erased (INGSRV2966/b113075).
**      14-Feb-2005 (jammo01) b111830 INGSRV 2713
**          options for reformatting the log file, along with info about
**          it being raw or not, gets disappeared. The options for erase
**          reformat and disable menu get disabled.
**	06-Apr-2005 (mutma03)
**	    Fixed the logname for clustered application to reflect the 
**	    node.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for i4 values.
**	13-sep-2005 (abbjo03)
**	    On VMS, bypass checking the first part of the log directory which
**	    cannot be created even if we find it doesn't exist, since it has
**	    to be an actual or concealed device.
**	19-Jul-2007 (bonro01)
**	    Restrict the minimum log file partition size to be 32 meg.
**	13-Aug-2007 (kibro01) b118802
**	    Use CMsetPMval instead of PMmInsert so that the derived parameters
**	    get calculated correctly.
**	12-Feb-2008 (kibro01) b119545
**	    Update change for bug 118802 so that change to tx log size is
**	    logged in config.log.
**	26-Feb-2008 (hanje04)
**	    SIR 119978
**	    Replace GLOBALDEF for change_log_file an change_log_buf to GLOBALREF
**	    as both are already defined and having the GLOBALDEF in the libconfig.a
**	    causes link errors on some platforms.
**      08-Apr-2008 (horda03) Bug 77085
**          There can now be a large number of TX log files (split into partitions).
**          On a machine without Largefile support OFFSET_TYPE could be an "i4". If
**          the total size of the partitioned TX log file is >2GB then the size displayed
**          would be negative. Keep the total size in a u_i8.
**	26-Aug-2009 (kschendel) b121804
**	    Fix bad call to reset-resource.  Fix out-of-control tabbing that
**	    make that part of the code unreadable.
**      16-Oct-2009 (smeke01) b122732
**          Added a check to see if the end-user has protected the derived 
**          parameter cb_interval_mb. If so, then use the value in 
**          cb_interval_mb as part of the calculations to find the minimum 
**          size allowed for the tx log. 
**      24-Nov-2009 (frima01) Bug 122490
**          Added include of cv.h to eliminate gcc 4.3 warnings.
**	25-Mar-2010 (hanje04)
**	    SIR 123296
**	    Use full paths (via NMloc) when calling csinstall, rcpstat etc.
**	    so they can be found for LSB installation without being in the path.
**      29-Mar-2010 (hanal04) Bug 123486
**          log_status is a bit mask to contain PRIMARY_LOG_ENABLED
**          and/or SECONDARY_LOG_ENABLED. Do not lose the existing value
**          when calling LOexist().
**      30-Mar-2010 (hanal04) Bug 123491
**          Renamed "Insert" menu option to "AddLocation".
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/

# include	<compat.h>
# include	<gl.h>
# include	<iicommon.h>
# include	<si.h>
# include	<pc.h>
# include	<cs.h>
# include	<ci.h>
# include	<di.h>
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
# include	<lk.h>
# include	<cx.h>
# include	<lg.h>
# include	<ui.h>
# include	<te.h>
# include	<erst.h>
# include	<uigdata.h>
# include	<stdprmpt.h>
# include	<pc.h>
# include	<pm.h>
# include	<pmutil.h>
# include	<util.h>
# include	<simacro.h>
# include	<cv.h>
# include	"cr.h"
# include	"config.h"
# include	<cv.h>

# define TX_MIN_LOG_FILE_MB 32

exec sql include sqlca;
exec sql include sqlda;

/*
** global variables
*/
exec sql begin declare section;
GLOBALREF char		*host;
GLOBALREF char		*node;
GLOBALREF char		*ii_system;
GLOBALREF char		*ii_installation;
exec sql end declare section;

GLOBALREF char		*local_host;
GLOBALREF char		Affinity_Arg[16];
GLOBALREF LOCATION	config_file;
GLOBALREF PM_CONTEXT	*config_data;
GLOBALREF PM_CONTEXT	*protect_data;
GLOBALREF FILE          *change_log_fp;
GLOBALREF LOCATION      change_log_file;
GLOBALREF char          change_log_buf[];

exec sql begin declare section;
static char thermometer[ BIG_ENOUGH ]; 
exec sql end declare section;

/*
** local forms
*/
exec sql begin declare section;
static char *log_graph = ERx( "log_graph" );
exec sql end declare section;

/*
** Forward declares.
*/
STATUS do_csinstall(void);
STATUS call_rcpstat( char *arguments );
STATUS call_rcpconfig( char *arguments );
void do_ipcclean(void);

static void
init_graph( bool create, i4 size )
{
	exec sql begin declare section;
	char msg[ BIG_ENOUGH ];
	exec sql end declare section;

	if( create )
		STprintf( msg, ERget( S_ST0446_creating_log ), size );
	else
		STprintf( msg, ERget( S_ST0447_erasing_log ), size );

	exec frs putform log_graph ( msg = :msg ); 

	*thermometer = EOS;

	/* clear the menu line */ 
	exec frs message '';
}

static void
update_graph( void )
{
	STcat( thermometer, ERx( "|" ) );
	exec frs putform log_graph ( thermometer = :thermometer ); 
	exec frs redisplay;
}

static void
display_message( char *text )
{
	exec sql begin declare section;
	char *msg = text;
	exec sql end declare section;

	exec frs message :msg with style = popup;	
}

static void
set_log_partions( char * new_value )
{
	char configStr[ MAX_LOC ];		/* config str to look up */

	if ( STcompare(new_value, ERx( "0" )) == 0 )
	    new_value = ERx( "1" );
	STprintf( configStr, "%s.%s.rcp.log.log_file_parts",
	    SystemCfgPrefix, node );
	set_resource( configStr, new_value);
}

static void
format_log_name( char *outbuf, char *basename, i4 partno, char *fileext )
{
	i4	choppos;
	char	svch = '\0';

	choppos = DI_FILENAME_MAX - ( STlength(basename) + 4 );

	if ( choppos < CX_MAX_NODE_NAME_LEN+1 )
	{
	    svch = *(fileext + choppos);
	    *(fileext + choppos) = '\0';
	}

	STprintf( outbuf, "%s.l%02d%s", basename, partno, fileext );

	if ( '\0' != svch )
	{
	    *(fileext + choppos) = svch;
	}
}

u_i2
transaction_logs_form( char *log_sym ) 
{ 
	exec sql begin declare section;
	char state[ 100 ];
	char *tmp1, *tmp2, *tmp3;
	char size1[ 25 ], size2[ 25 ];
	char enabled1[ 25 ], enabled2[ 25 ];
	char raw1[ 25 ], raw2[ 25 ];
	char filename1[MAX_LOC + 1], filename2[ MAX_LOC + 1];
	char parts1[ 25 ], parts2[ 25 ];
	i4 LOinfo_flag;
	char transaction_log[25];

	char *path1[LG_MAX_FILE + 1], *path2[LG_MAX_FILE + 1];
	char loc1_buf[LG_MAX_FILE + 1][ MAX_LOC + 1 ];
	char loc2_buf[LG_MAX_FILE + 1][ MAX_LOC + 1 ];
	char file1_buf[LG_MAX_FILE + 1][ MAX_LOC + 1 ];
	char file2_buf[LG_MAX_FILE + 1][ MAX_LOC + 1 ];
	char dir1_loc[LG_MAX_FILE + 1][ MAX_LOC + 1 ];
	char dir2_loc[LG_MAX_FILE + 1][ MAX_LOC + 1 ];
	char dir1_rawloc[LG_MAX_FILE + 1][ MAX_LOC + 1 ];
	char dir2_rawloc[LG_MAX_FILE + 1][ MAX_LOC + 1 ];
	char *dir1[LG_MAX_FILE + 1], *dir2[LG_MAX_FILE + 1];
	char *file1[LG_MAX_FILE + 1], *file2[LG_MAX_FILE + 1];
	char tmp_buf[ BIG_ENOUGH ];
	char fileext[CX_MAX_NODE_NAME_LEN+2];
	exec sql end declare section;

	u_i8 lsize1 = 0, lsize2 = 0;
	char cmd[ BIG_ENOUGH ];
	LOCATION loc1[LG_MAX_FILE + 1];
	LOCATION loc2[LG_MAX_FILE + 1];
	LOINFORMATION loc_info;
	LOCATION *IIUFlcfLocateForm();
	STATUS IIUFgtfGetForm();
	bool changed = FALSE;
	bool read_only = FALSE;
	CL_ERR_DESC cl_err;
	bool transactions = FALSE;
	bool log_file_exists = FALSE;
	bool dual_log_exists = FALSE;
	u_i2 log_status = 0;
        STATUS lo_status = 0;
	u_i2 loop = 0;
	u_i2 num_parts = 0;
	u_i2 num_parts1 = 0;
	u_i2 num_parts2 = 0;
	bool israw1 = FALSE;
	bool israw2 = FALSE;
	char configStr[ MAX_LOC ];		/* config str to look up */
	char configPfx[ MAX_LOC ];		/* config str to look up */
# ifdef UNIX
	i4   csinstall;
# endif /*UNIX*/

	STprintf( configPfx, "%s.%s.rcp", SystemCfgPrefix, node );

	if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
	    STcopy( "transaction_logs", transaction_log );
	else
	    STcopy( "transaction_dual", transaction_log );


	if( IIUFgtfGetForm( IIUFlcfLocateForm(), transaction_log ) != OK )
	{
	    IIUGerr( S_ST031A_form_not_found, UG_ERR_FATAL, 1,
		transaction_log );
	}

	exec frs message :ERget( S_ST038A_examining_logs ); 

# ifdef UNIX
	if( OK != (csinstall = do_csinstall()) )
	{
		exec frs message :ERget( S_ST038A_examining_logs ); 
	}
# endif /*UNIX*/

	/*
	** ### CHECK FOR ACTIVE LOCAL OR REMOTE CLUSTER SERVER PROCESS.
	*/
	if ( OK == call_rcpstat( ERx( "-online -silent" ) ) )
	{
		STprintf( tmp_buf, ERget( S_ST039A_log_ops_disabled ),
			  SystemProductName );
		exec frs message :tmp_buf
			with style = popup;
		read_only = TRUE;
		STcopy( ERget( S_ST0424_on_line ), state ); 
	}
	else if( OK == call_rcpstat( ERx( "-transactions -silent" ) ) )
	{
		STcopy( ERget( S_ST0426_off_line_transactions ), state ); 
		transactions = TRUE;
	}
	else
		STcopy( ERget( S_ST0425_off_line ), state ); 

	exec frs inittable :transaction_log log_locations read;
	exec frs inittable :transaction_log log_file_atts read;

# ifndef UNIX
	/* make 'raw' column in both tablefields invisible unless UNIX */
	exec frs set_frs column :transaction_log log_file_atts (
		invisible( raw ) = 1
	);

# endif /* UNIX */
	exec frs set_frs frs ( timeout = 30 );

	exec frs display :transaction_log;
	

	exec frs initialize (
		host = :node,
		ii_installation = :ii_installation,
		ii_system = :ii_system,
		state = :state
	);

	exec frs begin;

		/* 
		** if this is a clustered installation, we need to
		** append the host name to the log file...
		*/
		fileext[0] = '\0';
		if ( CXcluster_configured() )
		{
		    CXdecorate_file_name( fileext, node );
		}

		for  (loop = 0; loop < LG_MAX_FILE; loop++)
		{
		    dir1[loop] = NULL;
		    dir2[loop] = NULL;
		    file1[loop] = NULL;
		    file2[loop] = NULL;
		    path1[loop] = NULL;
		    path2[loop] = NULL;
		}
		tmp1 = tmp2 = tmp3 = NULL;

        	/* get primrary transaction log information */
		for  (loop = 0; loop < LG_MAX_FILE; loop++)
		{
		    /* if not stripped log and defined old fashioned way */
		    STprintf( configStr, "%s.log.log_file_%d",
			 configPfx, loop + 1);
		    if( OK == PMmGet( config_data, configStr, &tmp1))
			tmp1 = STalloc( tmp1 );
		    else
			tmp1 = NULL;
		    STprintf( configStr, "%s.log.log_file_name",
			configPfx );
		    if( OK == PMmGet( config_data, configStr, &tmp2))
			tmp2 = STalloc( tmp2 );
		    else
			tmp2 = NULL;
		    STprintf( configStr, "%s.log.raw_log_file_%d",
			 configPfx, loop + 1);
		    if( OK == PMmGet( config_data, configStr, &tmp3))
			tmp3 = STalloc( tmp3 );
		    else
			tmp3 = NULL;

		    if( tmp2 == NULL || *tmp2 == EOS )		
		    {
			STcopy( "", filename1 );
		    }
		    else
		    {
			STcopy( tmp2, filename1 );
		    }
		    if( tmp1 == NULL || *tmp1 == EOS ||
			tmp2 == NULL || *tmp2 == EOS )
		    {
			path1[loop] = ERx( "" );
			break;
		    }
		    else
		    {
        		STcopy( tmp1, loc1_buf[loop] );
        		STcopy( tmp1, dir1_loc[loop] );
			if( tmp3 != NULL && *tmp3 != EOS )
			{
        		    STcopy( tmp3, dir1_rawloc[loop] );
			    israw1 = TRUE;
			}
			if ( israw1 )
			   STcopy( ERget( S_ST0330_yes ), raw1 );
			else
			   STcopy( ERget( S_ST0331_no ), raw1 ); 
        		LOfroms( PATH, loc1_buf[loop], &loc1[loop] );
        		LOfaddpath( &loc1[loop], SystemLocationSubdirectory,
			    &loc1[loop] );
        		LOfaddpath( &loc1[loop], ERx( "log" ), &loc1[loop] );

        		LOtos( &loc1[loop], &dir1[loop] );
			dir1[loop] = STalloc( dir1[loop] );
			format_log_name( file1_buf[loop], tmp2,
					 loop + 1, fileext );
			file1[loop] = STalloc( file1_buf[loop] );
        		LOfstfile( file1[loop], &loc1[loop] );
        		LOtos( &loc1[loop], &path1[loop] );
			num_parts1 = loop + 1;
		    }
		    MEfree( tmp1 );
		    MEfree( tmp2 );
		    MEfree( tmp3 );
		    tmp1 = tmp2 = tmp3 = NULL;
		}
		    tmp1 = tmp2 = NULL;
		for  (loop = 0; loop < num_parts1; loop++)
		{
		    if( path1[loop] && *path1[loop] != EOS &&
			LOexist( &loc1[loop] ) == OK )
		    {

			log_file_exists = TRUE;
			/* primary transaction log exists */

			LOinfo_flag = LO_I_SIZE;
			if (LOinfo(&loc1[0], &LOinfo_flag, &loc_info) != OK
			    || loc_info.li_size == 0) 
			{
			    char *value;
			    bool havevalue = FALSE;

				/* use ii.$.rcp.file.size */
			    STprintf( tmp_buf, "%s.file.kbytes",
				      configPfx );
			    if ( PMmGet( config_data, tmp_buf,
					 &value ) == OK )
			    {
				int ivalue;

				havevalue = TRUE;
				CVal( value, &ivalue );
				loc_info.li_size = (u_i8) ivalue;
				STprintf( size1, ERx( "%u" ),
					  (i4)(loc_info.li_size / 1024L) );
			    }
			    if ( havevalue == FALSE )
			    {
				STprintf( tmp_buf, "%s.file.size",
					  configPfx );
				if( PMmGet( config_data, tmp_buf, 
					    &value ) == OK )
				{
				    int ivalue;

				    havevalue = TRUE;
				    CVal( value, &ivalue );
				    loc_info.li_size = ivalue;
				    STprintf( size1, ERx( "%u" ),
					      (i4)(loc_info.li_size / 1048576L));
				}
			    }
			    if ( havevalue == FALSE )
			    {
				exec frs message
				    :ERget(
					S_ST038B_no_primary_size )
				    with style = popup;
				STprintf( size1, ERx( "???????" ) );
			    }
			    break;
			}
			else
                        {
			    lsize1 += (u_i8) loc_info.li_size;
			    STprintf( size1, ERx( "%u" ),
				(i4)(lsize1 / 1048576L ));
			}
		    }
		}

		if (log_file_exists)
		{
		    /* determine whether primary log is enabled */
		    if (OK == call_rcpstat( ERx( "-enable -silent" ) ))
		    {
			STcopy( ERget( S_ST0330_yes ), enabled1 );
			log_status |= PRIMARY_LOG_ENABLED;
		    }
		    else
			STcopy( ERget( S_ST0331_no ), enabled1 );
		}
		else
		{
		    *size1 = EOS;
		    *enabled1 = EOS;
		    *raw1 = EOS;
		}
		
		STprintf( parts1, ERx( "%d" ), num_parts1);

		/* get secondary transaction log information */
 
		for  (loop = 0; loop < LG_MAX_FILE; loop++)
		{
		    STprintf( configStr, "%s.log.dual_log_%d",
			 configPfx, loop + 1);
		    if( OK == PMmGet( config_data, configStr, &tmp1))
			tmp1 = STalloc( tmp1 );
		    else
			tmp1 = NULL;
		    STprintf( configStr, "%s.log.dual_log_name",
			configPfx );
		    if( OK == PMmGet( config_data, configStr, &tmp2))
			tmp2 = STalloc( tmp2 );
		    else
			tmp2 = NULL;
		    STprintf( configStr, "%s.log.raw_dual_log_%d",
			 configPfx, loop + 1);
		    if( OK == PMmGet( config_data, configStr, &tmp3))
			tmp3 = STalloc( tmp3 );
		    else
			tmp3 = NULL;

		    if( tmp2 == NULL || *tmp2 == EOS )		
		    {
			STcopy( "", filename2 );
		    }
		    else
		    {
			STcopy( tmp2, filename2 );
		    }

		    if( tmp1 == NULL || *tmp1 == EOS ||
			tmp2 == NULL || *tmp2 == EOS )		
		    {
			path2[loop] = ERx( "" );
			break;
		    }
		    else
		    {
        		STcopy( tmp1, loc2_buf[loop] );
        		STcopy( tmp1, dir2_loc[loop] );
			if( tmp3 != NULL && *tmp3 != EOS )
			{
        		    STcopy( tmp3, dir2_rawloc[loop] );
		            israw2 = TRUE;	
			}
			if ( israw2 )
			   STcopy( ERget( S_ST0330_yes ), raw2 );
			else
			   STcopy( ERget( S_ST0331_no ), raw2 );
        		LOfroms( PATH, loc2_buf[loop], &loc2[loop] );
        		LOfaddpath( &loc2[loop], SystemLocationSubdirectory,
			    &loc2[loop] );
        		LOfaddpath( &loc2[loop], ERx( "log" ), &loc2[loop] );
        		LOtos( &loc2[loop], &dir2[loop] );
			dir2[loop] = STalloc( dir2[loop] );
			format_log_name( file2_buf[loop], tmp2,
					 loop + 1, fileext );
			file2[loop] = STalloc( file2_buf[loop] );
        		LOfstfile( file2[loop], &loc2[loop] );
        		LOtos( &loc2[loop], &path2[loop] );
			num_parts2 = loop + 1;
		    }
		    MEfree( tmp1 );
		    MEfree( tmp2 );
		    MEfree( tmp3 );
		    tmp1 = tmp2 = tmp3 = NULL;
		}
		for  (loop = 0; loop < num_parts2; loop++)
		{
		    if( path2[loop] && *path2[loop] != EOS &&
			LOexist( &loc2[loop] ) == OK )
		    {
			/* secondary transaction log exists */

			dual_log_exists = TRUE;
 			LOinfo_flag = LO_I_SIZE;
			if( LOinfo( &loc2[0], &LOinfo_flag, &loc_info ) != OK
			    || loc_info.li_size == 0 )
			{
			    char *value;
			    bool havevalue = FALSE;

				/* use ii.$.rcp.file.size */
			    STprintf( tmp_buf, "%s.file.kbytes", configPfx );
			    if ( PMmGet( config_data, tmp_buf,
					 &value ) == OK )
			    {
				int ivalue;

				havevalue = TRUE;
				CVal( value, &ivalue );
				loc_info.li_size = ivalue;
				STprintf( size2, ERx( "%d" ),
					  (i4)(loc_info.li_size / 1024L) );
			    }
			    if ( havevalue == FALSE )
			    {
				STprintf( tmp_buf, "%s.file.size",
					  configPfx );
				if( PMmGet( config_data, tmp_buf, 
					    &value ) == OK )
				{
				    int ivalue;

				    havevalue = TRUE;
				    CVal( value, &ivalue );
				    loc_info.li_size = ivalue;
				    STprintf( size2, ERx( "%d" ),
					      (i4)(loc_info.li_size / 1048576L));
				}
			    }
			    if ( havevalue == FALSE )
			    {
				exec frs message
				    :ERget(
					S_ST038C_no_secondary_size)
				    with style = popup;
				STprintf( size2, ERx( "???????" ) );
			    }
			    break;
			}
			else
			{
			    lsize2 += (u_i8) loc_info.li_size;
			    STprintf( size2, ERx( "%d" ), 
				(i4)(lsize2 / 1048576L ));
			}
		    }
		}

		if (dual_log_exists)
		{
		    /* determine whether primary log is enabled */
		    if( OK == call_rcpstat( ERx( "-enable -dual -silent" ) ) )
		    {
			STcopy( ERget( S_ST0330_yes ), enabled2 );
			log_status |= SECONDARY_LOG_ENABLED;
		    }
		    else
			STcopy( ERget( S_ST0331_no ), enabled2 );
		}
		else
		{
		    *size2 = EOS;
		    *enabled2 = EOS;
		    *raw2 = EOS;
		}
		
		STprintf( parts2, ERx( "%d" ), num_parts2);

		if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
		{
		    exec frs loadtable :transaction_log log_file_atts (
		
			parts = :parts1,
			size = :size1,
			enabled = :enabled1,
			raw = :raw1,
			name = :filename1);

		    for  (loop = 0; loop < num_parts1; loop++)
		    {
			if ( israw1 )
			{
			    exec frs loadtable :transaction_log log_locations (
				path = :dir1_rawloc[loop] );
			}
			else
			{
			    exec frs loadtable :transaction_log log_locations (
				path = :dir1_loc[loop] );
			}
		    }
		}
		else
		{
		    exec frs loadtable :transaction_log log_file_atts (
		
			parts = :parts2,
			size = :size2,
			enabled = :enabled2,
			raw = :raw2,
			name = :filename2);

		    for  (loop = 0; loop < num_parts2; loop++)
		    {
			if ( israw2 )
			{
			    exec frs loadtable :transaction_log log_locations (
				path = :dir2_rawloc[loop] );
			}
			else
			{
			    exec frs loadtable :transaction_log log_locations (
				path = :dir2_loc[loop] );
			}
		    }
		}


		/* deactivate invalid options */
		if( read_only )
		{
			exec frs set_frs menu '' (
				active( :ERget( FE_Create ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( FE_AddLoc ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( FE_Delete ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( S_ST0359_reformat_menu ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( S_ST035A_erase_menu ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( S_ST035B_disable_menu ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( FE_Destroy ) ) = 0
			);
		}

		exec frs resume column log_locations path;

	exec frs end;

	exec frs activate timeout;
	exec frs begin;
	{
		/*
		** Check whether the logging system has gone on-line
		** at each timeout (if form is not in read_only mode),
		** and make it read-only if it is on-line. 
		*/
		if( PCcmdline( (LOCATION *) NULL,
			ERx( "rcpstat -online -silent"), PC_WAIT, (LOCATION *) NULL,
			&cl_err ) == OK )
		{
			STprintf( tmp_buf, ERget( S_ST039A_log_ops_disabled ),
				  SystemProductName );
			exec frs message :tmp_buf
				with style = popup;

			read_only = TRUE;

			STcopy( ERget( S_ST0424_on_line ), state ); 

			exec frs putform :transaction_log ( state = :state );	

			/* deactivate invalid options */
			exec frs set_frs menu '' (
				active( :ERget( FE_Create ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( FE_AddLoc ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( FE_Delete ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( S_ST0359_reformat_menu ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( S_ST035A_erase_menu ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( S_ST035B_disable_menu ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( FE_Destroy ) ) = 0
			);
		}
		exec frs resume;
	}
	exec frs end;
		
	exec frs activate menuitem :ERget( FE_Create ),frskey11; 
	exec frs begin;
	{
	    exec sql begin declare section;
	    char size[ BIG_ENOUGH ];
	    i4  startrow;
	    char **log_dir, **log_file;
	    exec sql end declare section;
	    i4 kbytes;
	    bool format = FALSE;

	    /*
	    ** If no log exists, prompt for new size (in megabytes),
	    ** enforce 32 megabytes.  SIR: It would be better to
	    ** refer to the rules for the minimum megabytes.
	    */
	    if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
	    {
		if ( num_parts1 == 0 )
		    exec frs resume;
		log_dir = dir1;
		log_file = file1;
		STcopy( size2, size );
	    }
	    else
	    {
		if ( num_parts2 == 0 )
		    exec frs resume;
		log_dir = dir2;
		log_file = file2;
		STcopy( size1, size );

	    }
	    startrow = 14;
	    if ((( *enabled1 != EOS ) || ( *enabled2 != EOS )) &&
		( num_parts1 != num_parts2 ))
	    {
		exec frs message
		    :ERget( S_ST0572_same_no_log_partition ) 
		    with style = popup;
	    }
	    else
	    {		
		if( *size == EOS )
		{
		    i4 mbytes;
		    char value[ BIG_ENOUGH ];
		    char *cp_interval_mb;
		    i4 i_cp_interval_mb;
		    i4 min_logfile_mb = -1;

prompt_log_size:

		    /* Check if cp_interval_mb is protected */
		    STprintf( tmp_buf, "%s.log.cp_interval_mb", configPfx );
		    if (PMmGet( protect_data, tmp_buf, &cp_interval_mb ) == OK)
		    {
			/* 
			** The logging derived paramter cp_interval_mb is protected. 
			** Calculate the minimum logfile size based on cp_interval_mb,
			** rounding up.
			*/
			CVal( cp_interval_mb, &i_cp_interval_mb );
			min_logfile_mb = (i4)(((i_cp_interval_mb * 4000) / 1024.0) + 0.9);
		    }

		    if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
		    {
			exec sql begin declare section;
			char size_prompt[ER_MAX_LEN];
			exec sql end declare section;

			if ( min_logfile_mb > TX_MIN_LOG_FILE_MB * num_parts1 )
			{
			    STprintf( size_prompt,
				ERget( S_ST045B_protected_cpi_prompt ),
				i_cp_interval_mb, num_parts1, min_logfile_mb  );
			}
			else
			{
			    STprintf(size_prompt,
				ERget( S_ST039C_log_size_prompt),
				num_parts1, TX_MIN_LOG_FILE_MB * num_parts1  );
			}

			exec frs prompt ( 
			    :size_prompt,
			    :size
			    ) with style = popup;
		    }
		    else
		    {
			exec sql begin declare section;
			char size_prompt[ER_MAX_LEN];
			exec sql end declare section;

			if ( min_logfile_mb > TX_MIN_LOG_FILE_MB * num_parts2 )
			{
			    STprintf( size_prompt,
				ERget( S_ST045B_protected_cpi_prompt ),
				i_cp_interval_mb, num_parts2, min_logfile_mb  );
			}
			else
			{
			    STprintf(size_prompt,
				ERget( S_ST039C_log_size_prompt),
				num_parts2, TX_MIN_LOG_FILE_MB * num_parts2  );
			}

			exec frs prompt ( 
			    :size_prompt,
			    :size
			    ) with style = popup;

		    }

		    if( CVal( size, &mbytes ) != OK )
		    {
			exec frs message
			    :ERget( S_ST039D_integer_expected ) 
			    with style = popup;
			goto prompt_log_size;
		    }

		    if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
		    {
			exec sql begin declare section;
			char too_small[ER_MAX_LEN];
			exec sql end declare section;

			if ( (min_logfile_mb > TX_MIN_LOG_FILE_MB * num_parts1) && (min_logfile_mb > mbytes) )
			{
			    STprintf( too_small,
				ERget( S_ST045C_protected_cpi_small ),
				i_cp_interval_mb, num_parts1, min_logfile_mb  );

			    exec frs message
			    :too_small
			    with style = popup;
			    goto prompt_log_size;
			}
			else
			if( (mbytes/num_parts1) < TX_MIN_LOG_FILE_MB )
			{
			    STprintf( too_small,
				ERget( S_ST039E_log_too_small),
				num_parts1, TX_MIN_LOG_FILE_MB * num_parts1  );

			    exec frs message
			    :too_small
			    with style = popup;
			    goto prompt_log_size;
			}
		    }
		    else
		    {
			exec sql begin declare section;
			char too_small[ER_MAX_LEN];
			exec sql end declare section;

			if ( (min_logfile_mb > TX_MIN_LOG_FILE_MB * num_parts2) && (min_logfile_mb > mbytes) )
			{
			    STprintf( too_small,
				ERget( S_ST045C_protected_cpi_small ),
				i_cp_interval_mb, num_parts2, min_logfile_mb  );

			    exec frs message
			    :too_small
			    with style = popup;
			    goto prompt_log_size;
			}
			else
			if( (mbytes/num_parts2) < TX_MIN_LOG_FILE_MB )
			{
			    STprintf( too_small,
				ERget( S_ST039E_log_too_small),
				num_parts2, TX_MIN_LOG_FILE_MB * num_parts2  );

			    exec frs message
			    :too_small
			    with style = popup;
			    goto prompt_log_size;
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

		    STprintf( value, ERx( "%ld" ), mbytes * 1024L );

		    STprintf( tmp_buf, "%s.file.kbytes", configPfx );
		    /* Now set the value and all dependent parameters
		    ** (kibro01) b118802
		    ** And use change_log_fp so it's logged in config.log
		    ** (kibro01) b119545
		    */
		    CRsetPMval(tmp_buf, value, change_log_fp, FALSE, TRUE);

		    /* close change log after appending */
		    (void) SIclose( change_log_fp );

		    if( PMmWrite( config_data, NULL ) != OK )
		    {
			IIUGerr( S_ST0314_bad_config_write,
			    UG_ERR_FATAL, 1, SystemProductName );
		    }
		}
		else
		{
		    if(STcompare(log_sym, ERx("II_DUAL_LOG")) == 0)
		    {
 			int ival=0, n_ival=0, isize1=0, size_b=0;
			char* value=NULL;
			char configStr[BIG_ENOUGH], new_value[BIG_ENOUGH];
			char msg_config_size[BIG_ENOUGH]={0};
			char msg_primary_size[BIG_ENOUGH]={0};
	        
			STprintf(tmp_buf, "%s.file.kbytes", configPfx);
			if (PMmGet(config_data, tmp_buf, &value) == OK)
			{
			    CVal(value, &ival);
			    n_ival=ival/1024;
			}
			CVal(size1, &isize1);

	            
			if(isize1 == n_ival)
			{
			    if(CONFCH_YES!= IIUFccConfirmChoice(CONF_GENERIC,
						NULL, NULL, NULL, NULL,
						S_ST0428_log_size_warning,
						S_ST0429_log_size_ok,
						S_ST042A_log_size_not_ok,
						NULL, TRUE ) )
			    {
				exec frs resume;
			    }
			}
			else
			{
			    STprintf(msg_config_size, "%d", n_ival);
			    STprintf(msg_primary_size, "%d", isize1);
			    if(CONFCH_YES!= IIUFccConfirmChoice(CONF_GENERIC, 
					msg_config_size, msg_primary_size, NULL, NULL,
					S_ST043A_inconsistent_log, 
					S_ST043B_correct_config, 
					S_ST043C_no_correction, 
					NULL, 
					TRUE))
			    {
				exec frs resume;
			    }
			    else
			    {
				STprintf(tmp_buf, "%s.file.kbytes", configPfx);
				size_b = (isize1*1024);
				CVna(size_b, new_value); 
				value = &new_value[0];
				reset_resource(tmp_buf, &value, TRUE);
			    }            
			}
		    }
		}

		if( IIUFgtfGetForm( IIUFlcfLocateForm(), log_graph ) != OK )
		{
		    IIUGerr( S_ST031A_form_not_found, UG_ERR_FATAL, 1,
			log_graph );
		}

		exec frs display log_graph with style = popup(
		    startcolumn = 4, startrow = :startrow
		    );

		exec frs initialize;
	
		exec frs begin;

		exec frs redisplay;

		PMmSetDefault( config_data, 0, SystemCfgPrefix );
		PMmSetDefault( config_data, 1, node );
		if( (kbytes = write_transaction_log( TRUE,
		    config_data, log_dir, log_file,
		    display_message, init_graph, 63,
		    update_graph )) == 0 )
		{
		    exec frs message
			:ERget( S_ST038D_log_creation_failed )
			with style = popup;
		}
		else
		{
		    exec sql begin declare section;	
		    char *no_str = ERget( S_ST0331_no );
		    exec sql end declare section;	

		    STprintf( size, ERx( "%ld" ),
			kbytes / 1024L );

		    exec frs putrow :transaction_log log_file_atts 1 (
			size = :size,
			enabled =  :no_str,
			raw =  :no_str
			);

				/* update log_status mask */
		    if( STcompare( log_sym,
			ERx( "II_LOG_FILE" ) ) == 0 )	
		    {
			u_i2 mod_status = log_status |
			    PRIMARY_LOG_ENABLED;
			log_status = mod_status -
			    PRIMARY_LOG_ENABLED;
			STcopy(ERget( S_ST0330_yes ),enabled1);
		    }
		    else
		    {
			u_i2 mod_status = log_status |
			    SECONDARY_LOG_ENABLED;
			log_status = mod_status -
			    SECONDARY_LOG_ENABLED;
			STcopy(ERget( S_ST0330_yes ),enabled2);
		    }

		    exec frs set_frs menu :transaction_log (
			active( :ERget( FE_Create ) ) = 0
			);

		    exec frs set_frs menu :transaction_log (
			active( :ERget( FE_AddLoc ) ) = 0
			);

		    exec frs set_frs menu :transaction_log (
			active( :ERget( FE_Delete ) ) = 0
			);

		    exec frs set_frs menu :transaction_log (
			active( :ERget( S_ST0359_reformat_menu ) )
			= 1
			);

		    exec frs set_frs menu :transaction_log (
			active( :ERget( S_ST035A_erase_menu ) )
			= 1
			);

		    exec frs set_frs menu :transaction_log (
			active( :ERget( FE_Destroy ) ) = 1
			);

		    format = TRUE;
		}

		exec frs breakdisplay; 

		exec frs end;

		if( format )
		    goto reformat;
	    }
	}
	exec frs end;

	exec frs activate menuitem :ERget( S_ST0359_reformat_menu ),frskey12;
	exec frs begin;
	{
		char *cmd;

		if( transactions )
		{
			exec frs message
				:ERget( S_ST0389_transaction_warning )
				with style = popup;

			if( CONFCH_YES != IIUFccConfirmChoice( CONF_GENERIC,
				NULL, NULL, NULL, NULL,
				S_ST0386_reformat_prompt,
				S_ST0387_reformat_log,
				S_ST0388_no_reformat_log,
				NULL, TRUE ) )
			{
				exec frs resume;
			}
		}

reformat:

		if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )	
		{
			if ( num_parts1 == 0 )
		    	    exec frs resume;
			cmd = ERx( "-force_init_log -silent" );
		}
		else
		{
			if ( num_parts2 == 0 )
		    	    exec frs resume;
			cmd = ERx( "-force_init_dual -silent" );
		}

		exec frs message :ERget( S_ST038E_formatting_log );

		if( OK != call_rcpconfig( cmd ) )
		{
			exec frs message :ERget( S_ST038F_log_format_failed )
				with style = popup;
		}
		else
		{
			exec frs putrow :transaction_log log_file_atts 1 (
				enabled = :ERget( S_ST0330_yes )
			);

			/* update log_status mask */
			if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
			{
				log_status |= PRIMARY_LOG_ENABLED;
				STcopy(ERget( S_ST0330_yes ),enabled1);
			}
			else
			{
				log_status |= SECONDARY_LOG_ENABLED;
				STcopy(ERget( S_ST0330_yes ),enabled2);
			}

			exec frs set_frs menu :transaction_log (
				active( :ERget( S_ST035B_disable_menu ) ) = 1
			);
		}
	}
	exec frs end;

	exec frs activate menuitem :ERget( S_ST035B_disable_menu ),frskey13;
	exec frs begin;
	{
		char *cmd;
		char other_enabled[ BIG_ENOUGH ];

		if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
		{
			if ( num_parts1 == 0 )
		    	    exec frs resume;
			cmd = ERx( "-disable_log -silent" );

			STcopy( enabled2, other_enabled);
		}
		else
		{
			if ( num_parts2 == 0 )
		    	    exec frs resume;
			cmd = ERx( "-disable_dual -silent" );

			STcopy( enabled1, other_enabled);
		}

		if( *other_enabled == EOS )
		{
			exec frs message :ERget( S_ST0390_cant_disable_log )
				with style = popup;
			exec frs resume;
		}

		exec frs message :ERget( S_ST0391_disabling_log ); 

		if( OK != call_rcpconfig( cmd ) )
		{
			exec frs message :ERget( S_ST0392_log_disable_failed )
				with style = popup;
		}
		else
		{
			exec frs putrow :transaction_log log_file_atts 1 (
				enabled =  :ERget( S_ST0331_no )
			);

			/* update log_status mask */
			if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
			{
				u_i2 mod_status = log_status | 
					PRIMARY_LOG_ENABLED;
				log_status = mod_status -
					PRIMARY_LOG_ENABLED;
			}
			else
			{
				u_i2 mod_status = log_status |
					SECONDARY_LOG_ENABLED;
				log_status = mod_status -
					SECONDARY_LOG_ENABLED;
			}

			exec frs set_frs menu :transaction_log (
				active( :ERget( S_ST035B_disable_menu ) ) = 0
			);

			if( *other_enabled != EOS && STcompare( other_enabled,
				ERget( S_ST0331_no ) ) == 0 )
			{
				/* update log_status mask */
			    if( STcompare( log_sym,
				ERx( "II_LOG_FILE" ) ) == 0 )
			    {
				log_status |= PRIMARY_LOG_ENABLED;
				STcopy(ERget( S_ST0330_yes ),enabled1);
			    }
			    else
			    {
				log_status |= SECONDARY_LOG_ENABLED;
				STcopy(ERget( S_ST0330_yes ),enabled2);
			    }

			    exec frs message
				:ERget( S_ST0393_other_log_enabled )
				with style = popup;
			}
		}
	}
	exec frs end;

	exec frs activate menuitem :ERget( S_ST035A_erase_menu ),frskey14;
	exec frs begin;
	{
		exec sql begin declare section;
		i4 startrow;
		char **log_dir, **log_file;
		exec sql end declare section;
		bool format = FALSE;

		if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
		{
			if ( num_parts1 == 0 )
		    	    exec frs resume;
			log_dir = dir1;
			log_file = file1;
		}
		else
		{
			if ( num_parts2 == 0 )
		    	    exec frs resume;
			log_dir = dir2;
			log_file = file2;
		}
		startrow = 14;

		if( IIUFgtfGetForm( IIUFlcfLocateForm(), log_graph ) != OK )
		{
			IIUGerr( S_ST031A_form_not_found, UG_ERR_FATAL, 1,
				log_graph );
		}

		exec frs display log_graph
			with style = popup( startcolumn = 4,
			startrow = :startrow );

		exec frs initialize;
	
		exec frs begin;

			exec frs redisplay;

                if(  PCcmdline( (LOCATION *) NULL,
                        ERx( "rcpstat -online -silent"), PC_WAIT, (LOCATION *) NULL,
                        &cl_err ) == OK )
                {
                        STprintf( tmp_buf, ERget( S_ST039A_log_ops_disabled ),
                                  SystemProductName );
                        exec frs message :tmp_buf
                                with style = popup;

                        read_only = TRUE;

                        STcopy( ERget( S_ST0424_on_line ), state );

                        exec frs putform :transaction_log ( state = :state );

                }else
			if( write_transaction_log( FALSE, config_data,
				log_dir, log_file, display_message,
				init_graph, 63, update_graph ) == 0 )
			{
				exec frs message
					:ERget( S_ST0394_log_erase_failed )
					with style = popup;
			}
			else
				format = TRUE;

			exec frs breakdisplay; 

		exec frs end;

		if( format )
			goto reformat;
	}
	exec frs end;

	exec frs activate menuitem :ERget( FE_Destroy ),frskey15; 
	exec frs begin;
	{
		exec sql begin declare section;
		char *empty;
		exec sql end declare section;
		LOCATION *loc;
		char  *cmd;

		if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
		{
			if ( num_parts1 == 0 )
		    	    exec frs resume;
			loc = &loc1[0];
			cmd = ERx( "-disable_log -silent" );

		}
		else
		{
			if ( num_parts2 == 0 )
		    	    exec frs resume;
			loc = &loc2[0];
			cmd = ERx( "-disable_dual -silent" );
		}

                if(  PCcmdline( (LOCATION *) NULL,
                        ERx( "rcpstat -online -silent"), PC_WAIT, (LOCATION *) NULL,
                        &cl_err ) == OK )
                {
                        STprintf( tmp_buf, ERget( S_ST039A_log_ops_disabled ),
                                  SystemProductName );
                        exec frs message :tmp_buf
                                with style = popup;

                        read_only = TRUE;

                        STcopy( ERget( S_ST0424_on_line ), state );

                        exec frs putform :transaction_log ( state = :state );

                        exec frs breakdisplay;
                }

		if( transactions )
		{
			exec frs message
				:ERget( S_ST0389_transaction_warning )
				with style = popup;
		}

		if( CONFCH_YES != IIUFccConfirmChoice( CONF_GENERIC,
			NULL, NULL, NULL, NULL,
			S_ST0396_destroy_log_prompt,
			S_ST0397_destroy_log,
			S_ST0398_no_destroy_log,
			NULL, TRUE ) )
		{
			exec frs resume;
		}

		exec frs message :ERget( S_ST0391_disabling_log ); 
 
		if( OK != call_rcpconfig( cmd ) )
		{
		    /*
		    ** If the disable fails before destroying, continue anyway.
		    */
		}

		exec frs message :ERget( S_ST039B_destroying_log );

		if( STcompare( log_sym, ERx( "II_LOG_FILE" )) == 0)
		    num_parts = num_parts1;
		else
		    num_parts = num_parts2;
		for  (loop = 0; loop < num_parts; loop++)
		{
		    lo_status = LOexist( &loc[loop] );
		    if(( LOdelete( &loc[loop] ) != OK ) &&  (lo_status != LO_NO_SUCH))
		    {
			exec frs message :ERget( S_ST0399_log_destroy_failed )
			    with style = popup;
			exec frs resume;
		    }
		    if( STcompare( log_sym, ERx( "II_LOG_FILE" )) == 0)
		    {
			if ( israw1 )
			{

			    if( dir1[loop] != NULL )
			    {
				MEfree( dir1[loop] );
				dir1[loop] = NULL;
				MEfree( file1[loop] );
				file1[loop] = NULL;
			    }
			    exec frs deleterow :transaction_log log_locations;
			    STprintf( tmp_buf, "%s.log.log_file_%d",
				 configPfx, loop + 1);
			    (void) PMmDelete( config_data, tmp_buf );
			    STprintf( tmp_buf, "%s.log.raw_log_file_%d",
				 configPfx, loop + 1);
			    (void) PMmDelete( config_data, tmp_buf );
			    if( PMmWrite( config_data, NULL ) != OK )
			    {
				IIUGerr( S_ST0314_bad_config_write,
				    UG_ERR_FATAL, 1, SystemProductName );
			    }
			 }
		    }
		    else
		    {
			if ( israw2 )
			{
			    if( dir2[loop] != NULL )
			    {
				MEfree( dir2[loop] );
				dir2[loop] = NULL;
				MEfree( file2[loop] );
				file2[loop] = NULL;
			    }
			    exec frs deleterow :transaction_log log_locations;
			    STprintf( tmp_buf, "%s.log.dual_log_%d",
				 configPfx, loop + 1);
			    (void) PMmDelete( config_data, tmp_buf );
			    STprintf( tmp_buf, "%s.log.raw_dual_log_%d",
				 configPfx, loop + 1);
			    (void) PMmDelete( config_data, tmp_buf );
			    if( PMmWrite( config_data, NULL ) != OK )
			    {
				IIUGerr( S_ST0314_bad_config_write,
				    UG_ERR_FATAL, 1, SystemProductName );
			    }
			 }
		    }
		}
		if( STcompare( log_sym, ERx( "II_LOG_FILE" )) == 0)
		{
		    if ( israw1 )
		    {
			num_parts1 = 0;
			STprintf( parts1, ERx( "%d" ), num_parts1);
			exec frs putrow :transaction_log log_file_atts 1 (parts = :parts1 );
		    }
		}
		else
		{
		    if ( israw2 )
		    {
			num_parts2 = 0;
			STprintf( parts2, ERx( "%d" ), num_parts2);
			exec frs putrow :transaction_log log_file_atts 1 (parts = :parts2 );
		    }
		}

		empty = ERx( "" );

		exec frs putrow :transaction_log log_file_atts 1 (
			size = :empty,
			enabled = :empty,
			raw = :empty
		);

		exec frs set_frs menu '' (
			active( :ERget( FE_Create ) ) = 1
		);

		exec frs set_frs menu '' (
			active( :ERget( FE_AddLoc ) ) = 1
		);

		if( STcompare( log_sym, ERx( "II_LOG_FILE" )) == 0)
		{
		    if ( num_parts1 > 0 )
		    exec frs set_frs menu '' (
			active( :ERget( FE_Delete ) ) = 1);
		}
		else
		{
		    if ( num_parts2 > 0 )
		    exec frs set_frs menu '' (
			active( :ERget( FE_Delete ) ) = 1);
		}

		exec frs set_frs menu '' (
			active( :ERget( S_ST0359_reformat_menu ) ) = 0
		);

		exec frs set_frs menu '' (
			active( :ERget( S_ST035A_erase_menu ) ) = 0
		);

		exec frs set_frs menu '' (
			active( :ERget( S_ST035B_disable_menu ) ) = 0
		);

		exec frs set_frs menu '' (
			active( :ERget( FE_Destroy ) ) = 0
		);

		/* update log_status mask */
		if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
		{
			u_i2 mod_status = log_status | PRIMARY_LOG_ENABLED;
			log_status = mod_status - PRIMARY_LOG_ENABLED;
			*enabled1 = EOS;
			*size1 = EOS;
		}
		else
		{
			u_i2 mod_status = log_status | SECONDARY_LOG_ENABLED;
			log_status = mod_status - SECONDARY_LOG_ENABLED;
			*enabled2 = EOS;
			*size2 = EOS;
		}
	}
	exec frs end;

	exec frs activate menuitem :ERget( FE_AddLoc ),frskey16; 
	exec frs begin;
	{
	    exec sql begin declare section;
	        char newlocation[ BIG_ENOUGH ];
	        char size[ BIG_ENOUGH ];
	        i4 rowstate;
	        i4 rowno;
	        char errmsg[ BIG_ENOUGH ];
	    exec sql end declare section;
	    LOCATION newloc;
	    char newloc_buf[ BIG_ENOUGH ];

	    exec frs getrow :transaction_log log_file_atts 1 (
		:size = size);

	    if( *size == EOS )
	    {

		if(((STcompare( log_sym, ERx( "II_LOG_FILE" )) == 0) && 
		    (num_parts1 < LG_MAX_FILE )) || 
		    ((STcompare( log_sym, ERx( "II_DUAL_LOG" )) == 0) &&
			(num_parts2 < LG_MAX_FILE )))
		{

		    exec frs prompt ( :ERget( S_ST0571_log_location_prompt),
			:newlocation ) with style = popup;

		    if (*newlocation  ==  EOS)
		    {
			exec frs resume;
		    }
		    STcopy( newlocation, newloc_buf );
		    LOfroms( PATH, newloc_buf, &newloc );
#ifndef VMS
		    /*
		    ** On VMS, the first part of a path has to be an actual or
		    ** concealed device, which even if it's found not to exist
		    ** cannot be created, so bypass this check.
		    */
		    if ( LOexist( &newloc ) != OK )
			if ( LOcreate( &newloc ) != OK )
			{
			    STcopy( ERget( S_ST0573_mkdir_failed ), errmsg );
			    STcat( errmsg, newloc_buf );
			    exec frs message
				:errmsg
				with style = popup;
			    exec frs resume;
			}
#endif
		    LOfaddpath( &newloc, SystemLocationSubdirectory, &newloc);
		    if ( LOexist( &newloc ) != OK )
			if ( LOcreate( &newloc ) != OK )
			{
			    STcopy( ERget( S_ST0573_mkdir_failed ), errmsg );
			    STcat( errmsg, newloc_buf );
			    exec frs message
				:errmsg
				with style = popup;
			    exec frs resume;
			}
		    LOfaddpath( &newloc, ERx( "log" ), &newloc );
		    if ( LOexist( &newloc ) != OK )
			if ( LOcreate( &newloc ) != OK )
			{
			    STcopy( ERget( S_ST0573_mkdir_failed ), errmsg );
			    STcat( errmsg, newloc_buf );
			    exec frs message
				:errmsg
				with style = popup;
			    exec frs resume;
			}

		    exec frs inquire_frs table :transaction_log ( 
			:rowno = rowno );	

		    if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
		    {
			num_parts1++;
			STprintf( parts1, ERx( "%d" ), num_parts1);
			set_log_partions( parts1 );
			exec frs putrow :transaction_log log_file_atts 1 (
			    parts = :parts1 );
			if ( num_parts1 > LG_MAX_FILE )
			    exec frs set_frs menu '' (
				active( :ERget( FE_AddLoc ) ) = 0 );
			if ( num_parts1 == 1 )
			    rowno = 0;
		    }
		    else
		    {
			num_parts2++;
			STprintf( parts2, ERx( "%d" ), num_parts2);
			set_log_partions( parts2 );
			exec frs putrow :transaction_log log_file_atts 1 (
			    parts = :parts2 );
			if ( num_parts2 > LG_MAX_FILE )
			    exec frs set_frs menu '' (
				active( :ERget( FE_AddLoc ) ) = 0 );
			if ( num_parts2 == 1 )
			    rowno = 0;
		    }
		    exec frs insertrow :transaction_log log_locations :rowno (
			path = :newlocation );

		    loop = 0;
		    exec frs unloadtable :transaction_log log_locations (
			:newlocation = path, :rowstate = _state );
		    exec frs begin;
		    {
			if( rowstate != 4 )
			{
			    if( STcompare( log_sym, ERx( "II_LOG_FILE" )) == 0)
			    {
				if( dir1[loop] != NULL )
				{
				    MEfree( dir1[loop] );
				    dir1[loop] = NULL;
				    MEfree( file1[loop] );
				    file1[loop] = NULL;
				}
				STprintf( tmp_buf, "%s.log.log_file_%d",
					   configPfx, loop + 1);
				STcopy( newlocation, loc1_buf[loop] );
				STcopy( newlocation, dir1_loc[loop] );
				LOfroms( PATH, loc1_buf[loop], &loc1[loop] );
				LOfaddpath( &loc1[loop],
				    SystemLocationSubdirectory,
				    &loc1[loop] );
				LOfaddpath( &loc1[loop], ERx( "log" ),
				    &loc1[loop] );
				LOtos( &loc1[loop], &dir1[loop] );
				dir1[loop] = STalloc( dir1[loop] );
				format_log_name( file1_buf[loop], filename1,
					 loop + 1, fileext );
				file1[loop] = STalloc( file1_buf[loop] );
				LOfstfile( file1[loop], &loc1[loop] );
				LOtos( &loc1[loop], &path1[loop] );
			    }
			    else
			    {
				if( dir2[loop] != NULL )
				{
				    MEfree( dir2[loop] );
				    dir2[loop] = NULL;
				    MEfree( file2[loop] );
				    file2[loop] = NULL;
				}
				STprintf( tmp_buf, "%s.log.dual_log_%d",
					   configPfx, loop + 1);
				STcopy( newlocation, loc2_buf[loop] );
				STcopy( newlocation, dir2_loc[loop] );
				LOfroms( PATH, loc2_buf[loop], &loc2[loop] );
				LOfaddpath( &loc2[loop],
				    SystemLocationSubdirectory,
				    &loc2[loop] );
				LOfaddpath( &loc2[loop], ERx( "log" ),
				    &loc2[loop] );
				LOtos( &loc2[loop], &dir2[loop] );
				dir2[loop] = STalloc( dir2[loop] );
				format_log_name(file2_buf[loop], filename2,
					 loop + 1, fileext );
				file2[loop] = STalloc( file2_buf[loop] );
				LOfstfile( file2[loop], &loc2[loop] );
				LOtos( &loc2[loop], &path2[loop] );
			    }


			    (void) PMmDelete( config_data, tmp_buf );

			    (void) PMmInsert(config_data, tmp_buf, newlocation);

			    loop++;
			}
		    }
		    exec frs end;
		    if (PMmWrite(config_data, NULL ) != OK) 
		    {
			IIUGerr( S_ST0314_bad_config_write, UG_ERR_FATAL, 1, 
			    SystemProductName );
		    }
		}
	    }
	}
	exec frs end;

	exec frs activate menuitem :ERget( FE_Delete ),frskey18; 
	exec frs begin;
	{
	    exec sql begin declare section;
	        i4 rowno;
	        i4 rowstate;
		char size[ BIG_ENOUGH ];
		char location[ BIG_ENOUGH ];
	    exec sql end declare section;

	    exec frs getrow :transaction_log log_file_atts 1 (
		:size = size);

	    if( *size == EOS )
	    {
		if(((STcompare( log_sym, ERx( "II_LOG_FILE" )) == 0) && 
		    (num_parts1 > 0)) ||
		    ((STcompare( log_sym, ERx( "II_DUAL_LOG" )) == 0) &&
			(num_parts2 > 0)))
		{
		    exec frs inquire_frs table :transaction_log ( :rowno = rowno );

		    exec frs deleterow :transaction_log log_locations;
		    if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
		    {
			num_parts1--;
			STprintf( parts1, ERx( "%d" ), num_parts1);
			set_log_partions( parts1 );
			exec frs putrow :transaction_log log_file_atts 1 (
			    parts = :parts1 );
		    }
		    else
		    {
			num_parts2--;
			STprintf( parts2, ERx( "%d" ), num_parts2);
			set_log_partions( parts2 );
			exec frs putrow :transaction_log log_file_atts 1 (
			    parts = :parts2 );
		    }
		    exec frs set_frs menu '' (
			active( :ERget( FE_AddLoc ) ) = 1 );

		    loop = 0;
		    exec frs unloadtable :transaction_log log_locations (
			:location = path, :rowstate = _state );
		    exec frs begin;
		    {
			if( rowstate != 4 )
			{
			    if( STcompare( log_sym, ERx( "II_LOG_FILE" )) == 0)
			    {
				if( dir1[loop] != NULL )
				{
				    MEfree( dir1[loop] );
				    dir1[loop] = NULL;
				    MEfree( file1[loop] );
				    file1[loop] = NULL;
				}
				STprintf( tmp_buf, "%s.log.log_file_%d",
					   configPfx, loop + 1);
				STcopy( location, loc1_buf[loop] );
				STcopy( location, dir1_loc[loop] );
				LOfroms( PATH, loc1_buf[loop], &loc1[loop] );
				LOfaddpath( &loc1[loop],
				    SystemLocationSubdirectory,
				    &loc1[loop] );
				LOfaddpath( &loc1[loop], ERx( "log" ),
				    &loc1[loop] );
				LOtos( &loc1[loop], &dir1[loop] );
				dir1[loop] = STalloc( dir1[loop] );
				format_log_name(file1_buf[loop], filename1,
					 loop + 1, fileext );
				file1[loop] = STalloc( file1_buf[loop] );
				LOfstfile( file1[loop], &loc1[loop] );
				LOtos( &loc1[loop], &path1[loop] );
			    }
			    else
			    {
				if( dir2[loop] != NULL )
				{
				    MEfree( dir2[loop] );
				    dir2[loop] = NULL;
				    MEfree( file2[loop] );
				    file2[loop] = NULL;
				}
				STprintf( tmp_buf, "%s.log.dual_log_%d",
					   configPfx, loop + 1);
				STcopy( location, loc2_buf[loop] );
				STcopy( location, dir2_loc[loop] );
				LOfroms( PATH, loc2_buf[loop], &loc2[loop] );
				LOfaddpath( &loc2[loop],
				    SystemLocationSubdirectory,
				    &loc2[loop] );
				LOfaddpath( &loc2[loop], ERx( "log" ),
				    &loc2[loop] );
				LOtos( &loc2[loop], &dir2[loop] );
				dir2[loop] = STalloc( dir2[loop] );
				format_log_name( file2_buf[loop], filename2,
				 loop + 1, fileext );
				file2[loop] = STalloc( file2_buf[loop] );
				LOfstfile( file2[loop], &loc2[loop] );
				LOtos( &loc2[loop], &path2[loop] );
			    }

			    (void) PMmDelete( config_data, tmp_buf );

			    (void) PMmInsert( config_data, tmp_buf, location);
			    loop++;
			}
		    }
		    exec frs end;

		    if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
		    {
			STprintf( tmp_buf, "%s.log.log_file_%d", 
			  configPfx, loop + 1);
			num_parts1 = loop;
			path1[loop] = ERx( "" );
			if( dir1[loop] != NULL )
			{
			    MEfree( dir1[loop] );
			    dir1[loop] = NULL;
			    MEfree( file1[loop] );
			    file1[loop] = NULL;
			}
		    }
		    else
		    {
			STprintf( tmp_buf, "%s.log.dual_log_%d",
			  configPfx, loop + 1);
			num_parts2 = loop;
			path2[loop] = ERx( "" );
			if( dir2[loop] != NULL )
			{
			    MEfree( dir2[loop] );
			    dir2[loop] = NULL;
			    MEfree( file2[loop] );
			    file2[loop] = NULL;
			}
		    }

		    (void) PMmDelete( config_data, tmp_buf );


		    if (PMmWrite(config_data, NULL ) != OK) 
		    {
			IIUGerr( S_ST0314_bad_config_write, UG_ERR_FATAL, 1, 
			    SystemProductName );
		    }
		}
	    }
	}
	exec frs end;

	exec frs activate menuitem :ERget( FE_Help ), frskey1;
	exec frs begin;
		if( !get_help( ERx( "dbms.txlogs" ) ) )
			exec frs message 'No help available.'
				with style = popup;
	exec frs end;

	exec frs activate menuitem :ERget( FE_End ), frskey3;
	exec frs begin;
# ifdef UNIX
		if ( csinstall == OK )
		{
			exec frs message :ERget( S_ST0395_cleaning_up_memory ); 
			do_ipcclean();
		}
# endif /* UNIX */

		/* unset timeout */
		exec frs set_frs frs ( timeout = 0 );

		exec frs breakdisplay;
	exec frs end;

	exec frs activate before column log_locations all;
	exec frs begin;
	{
		exec sql begin declare section;
		char path[LG_MAX_FILE][ MAX_LOC + 1 ];
		char enabled[ 25 ];
		char size[ 25 ];
		char raw[ 25 ];
		char parts[ 25 ];
		exec sql end declare section;

		
		if( read_only )
			exec frs resume;

		exec frs getrow :transaction_log log_file_atts 1 (
			:parts = parts,
			:size = size,
			:enabled = enabled,
			:raw = raw
		);

		if( *parts == '0' )
		{
			/* symbols missing */

			exec frs set_frs menu '' (
				active( :ERget( FE_Create ) ) = 0 
			);

			exec frs set_frs menu '' (
				active( :ERget( FE_AddLoc ) ) = 1 
			);

			exec frs set_frs menu '' (
				active( :ERget( FE_Delete ) ) = 0 
			);

			exec frs set_frs menu '' (
				active( :ERget( S_ST0359_reformat_menu ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( S_ST035A_erase_menu ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( S_ST035B_disable_menu ) ) = 0
			);

			exec frs set_frs menu '' (
				active( :ERget( FE_Destroy ) ) = 0
			);
		}
		else
		if (( israw1 || israw2 ) && (LO_NO_SUCH == LOexist( &loc1[loop] )))
		{
				exec frs set_frs menu '' ( 
					active( :ERget( FE_Create ) ) = 0
				);
				exec frs set_frs menu '' (
					active( :ERget( FE_AddLoc ) ) = 0
				);
				exec frs set_frs menu '' (
					active( :ERget( FE_Delete ) ) = 0
				);
				exec frs set_frs menu '' (
					active( :ERget(
					S_ST0359_reformat_menu ) ) = 0
				);
				exec frs set_frs menu '' (
				       	active( :ERget(
					S_ST035A_erase_menu ) ) = 0
				);
				exec frs set_frs menu '' (
					active( :ERget(
					S_ST035B_disable_menu ) ) = 0
				);
				exec frs set_frs menu '' (
					active( :ERget( FE_Destroy ) ) = 1
				);
		}
		else
		{
			/* symbols are set */

			if( *size == EOS )
			{
				/* transaction log doesn't exist */

				exec frs set_frs menu '' (
					active( :ERget( FE_Create ) ) = 1
				);

				exec frs set_frs menu '' (
					active( :ERget( FE_AddLoc ) ) = 1
				);

				exec frs set_frs menu '' (
					active( :ERget( FE_Delete ) ) = 1
				);

				exec frs set_frs menu '' (
					active( :ERget(
					S_ST0359_reformat_menu ) ) = 0
				);

				exec frs set_frs menu '' (
					active( :ERget(
					S_ST035A_erase_menu ) ) = 0
				);

				exec frs set_frs menu ''
				(
					active( :ERget(
					S_ST035B_disable_menu ) ) = 0
				);

				exec frs set_frs menu '' (
					active( :ERget( FE_Destroy ) ) = 0
				);
			}
			else
			{
				/* transactio log exists */

				exec frs set_frs menu '' (
					active( :ERget( FE_Create ) ) = 0
				);

				exec frs set_frs menu '' (
					active( :ERget( FE_AddLoc ) ) = 0
				);

				exec frs set_frs menu '' (
					active( :ERget( FE_Delete ) ) = 0 
				);

				exec frs set_frs menu '' (
					active( :ERget(
					S_ST0359_reformat_menu ) ) = 1
				);	

				exec frs set_frs menu '' (
					active( :ERget(
					S_ST035A_erase_menu ) ) = 1
				);

				if( STcompare( enabled, ERget( S_ST0330_yes ) )
					== 0 )
				{
					exec frs set_frs menu '' (
						active( :ERget(
						S_ST035B_disable_menu ) ) = 1
					);
				}
				else
				{
					exec frs set_frs menu '' (
						active( :ERget(
						S_ST035B_disable_menu ) ) = 0
					);
				}

				exec frs set_frs menu '' (
					active( :ERget( FE_Destroy ) ) = 1
				);
			}
		}
	}
	exec frs end;

	PMmSetDefault( config_data, 1, NULL );
	return( log_status );
}


STATUS
do_csinstall()
{
# ifdef UNIX
    CL_ERR_DESC cl_err;
    char	cmd[MAX_LOC + 80];
    LOCATION	loc;
    char	*locstr;

    NMloc(SUBDIR, PATH, "utility", &loc);
    LOtos(&loc, &locstr);
    STprintf( cmd, ERx( "%s/csinstall %s >/dev/null" ), locstr, Affinity_Arg );
    return ( PCcmdline((LOCATION *) NULL, cmd,
		    PC_WAIT, (LOCATION *) NULL, &cl_err ) );
# else
    return OK;
# endif /* UNIX */
}

STATUS
call_rcpstat( char *arguments )
{
    CL_ERR_DESC cl_err;
    char	cmd[MAX_LOC+80+CX_MAX_NODE_NAME_LEN];
    LOCATION	loc;
    char	*locstr;
    char	*target, *arg;
    char	*redirect;
    
# ifdef VMS
	redirect = ERx( "" );
# else /* VMS */
# ifdef NT_GENERIC 
	redirect = ERx( ">NUL" );
# else
	redirect = ERx( ">/dev/null" );
# endif /* NT_GENERIC */
# endif /* VMS */

    target = arg = ERx( "" );

    if ( CXcluster_enabled() && STcompare( node, local_host ) != 0 )
    {
	target = node;
	arg = ERx( "-node" );
    }

    NMloc(BIN, FILENAME, "rcpstat", &loc);
    LOtos(&loc, &locstr);
    STprintf( cmd, ERx( "%s %s %s %s %s" ), locstr, Affinity_Arg,
	      arg, target, arguments, redirect );
    return PCcmdline( (LOCATION *) NULL, cmd, 
		      PC_WAIT, (LOCATION *) NULL, &cl_err );
}

STATUS
call_rcpconfig( char *arguments )
{
    CL_ERR_DESC cl_err;
    char	cmd[MAX_LOC+80+CX_MAX_NODE_NAME_LEN];
    LOCATION	loc;
    char	*locstr;
    char	*target, *arg;
    char	*redirect;
    
# ifdef VMS
	redirect = ERx( "" );
# else /* VMS */
# ifdef NT_GENERIC 
	redirect = ERx( ">NUL" );
# else
	redirect = ERx( ">/dev/null" );
# endif /* NT_GENERIC */
# endif /* VMS */

    target = arg = ERx( "" );

    if ( CXcluster_enabled() && STcompare( node, local_host ) != 0 )
    {
	target = node;
	arg = ERx( "-node" );
    }

    NMloc(BIN, FILENAME, "rcpconfig", &loc);
    LOtos(&loc, &locstr);
    STprintf( cmd, ERx( "%s %s %s %s %s" ), locstr, Affinity_Arg,
	      arg, target, arguments, redirect );
    return PCcmdline( (LOCATION *) NULL, cmd, 
		      PC_WAIT, (LOCATION *) NULL, &cl_err );
}

void
do_ipcclean(void)
{
# ifdef UNIX
    CL_ERR_DESC cl_err;
    char	cmd[80];
    LOCATION	loc;
    char	*locstr;

    NMloc(SUBDIR, PATH, "utility", &loc);
    LOtos(&loc, &locstr);
    STprintf( cmd, ERx( "%s/ipcclean %s" ), locstr, Affinity_Arg );
    (void) PCcmdline((LOCATION *) NULL, cmd,
	    PC_WAIT, (LOCATION *) NULL, &cl_err );
# endif /* UNIX */
}
