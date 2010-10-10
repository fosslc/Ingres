/*
** Copyright 2006, 2010 Ingres Corporation. All rights reserved.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

# include <compat.h>
# include <unistd.h>
# include <libgen.h>
# if defined( xCL_GTK_EXISTS ) && defined( xCL_RPM_EXISTS )

# include <gl.h>
# include <clconfig.h>
# include <ex.h>
# include <me.h>
# include <nm.h>
# include <pc.h>
# include <st.h>
# include <si.h>
# include <erclf.h>
# include <cm.h>

# include <gtk/gtk.h>

# include <gipinterf.h>
# include <gipsup.h>
# include <gipcb.h>
# include <gip.h>
# include <gipdata.h>
# include <giputil.h>
# include "gipmain.h"

/*
** Name: gipmain.c
**
** Description:
**
**      Main module for the Linux GUI installer main() program.
**
** History:
**
**	09-Oct-2006 (hanje04)
**	    SIR 116877
**          Created.
**	25-Oct-2006 (hanje04)
**	    SIR 116877
**	    Add check to make sure program runs as root.
**	    Use EXIT_STATUS macro to determine outcome of install.
**	    Load Charset and Timezone dialogs during setup instead
**	    of at launch time.
**	    Add init_gu_train_icons to make sure "train" images are
**	    set correctly.
**	26-Oct-2006 (hanje04)
**	    SIR 116877
**	    Add ability to turn on tracing by setting II_GIP_TRACE
**	06-Nov-2006 (hanje04)
**	    SIR 116877
**	    Make sure code only get built on platforms that support it
**	21-Nov-2006 (hanje04)
**	    BUG 117167 
**	    SD 112193
**	    Make sure package check boxes are set correctly when only
**	    one instance is detected. Also make sure instance are loaded
**	    when only "modify" instances have been detected.
**	27-Nov-2006 (hanje04)
**	    Temporarily move inclusion of RPM headers outside of # ifdef
**	     xCL_HAS_RPM to prevent conflict with defn of u_char in
**	    sys/types.h with that in systypes.h. This really needs to be 	**	    addressed is systypes.h but it can't be changed so close to
**	    beta.
**	    The MANIFEST file prevents it from being built where it shouldn't
**	    anyway.
**	28-Nov-2006 (hanje04)
**	    Remove "realloc()" from code as it was causing problems when memory
**	    areas were moved under the covers. Shouldn't have been using it
**	    anyway.
**	09-Jan-2007 (hanje04)
**	    Bug 117445
**	    SD 113106
**	    Update error handler inst_proc_exit_handler() so that the installer
**	    process is not marked as failed if the handler has been invoked 
**	    for reasons other that the process exiting. (e.g. receiving a 
**	    non-fatal signal.)
**	02-Feb-2007 (hanje04)
**	    SIR 116911
**	    Move the following functions here from front!st!gtkinstall_unix_win
**	    giputil.c as they cause compilations problems when building
**	    iipackman on Window.
**
**		get_rename_cmd_len()
**		add_pkglst_to_rmv_command_line()
**		get_pkg_list_size()
**		calc_cmd_len()
**		generate_command_line()
**		add_pkglst_to_inst_command_line()
**
**	    Replace PATH_MAX with MAX_LOC.
**	27-Feb-2007 (hanje04)
**	    BUG 117330
**	    Don't add doc package with others to the command line. It
**	    can't be relocated and needs to be installed separately.
**	13-Mar-2007 (hanje04)
**	    BUG 117902
**	    Add missing version to remove command line for non renamed
**	    packages.
**	15-Mar-2007 (hanje04)
**	    BUG 117926
**	    SD 116393
**	    Use list of packages that are installed when removing old
**	    package info on upgrade. When adding them to the command line
**	    search backwards for '-' when looking for the rename suffix
**	    so we don't get caught out by ca-ingres packages.
**	27-Mar-2007 (hanje04)
**	    BUG 118008
**	    SD 116528
**	    Always display instance selection pane even when there is only
**	    one instance. Otherwise, users are not given the option not to
**	    upgrade their user databases.
**	18-Apr-2007 (hanje04)
**	    Bug 118087
**	    SD 115787
**	    Add progress bar to installation screen to indicate progress.
**	    Only re-write the output every 100 pulses otherwise we get
**	    flickering and far more CPU use than is really neccisary.
**	20-Apr-2007 (hanje04)
**	    Bug 118090
**	    SD 116383
**	    Use newly added database and log locations from instance
**	    structure to remove ALL locations if requested by the user.
**	    Update cleanup routine to use the instance tag to free the
**	    memory as each instance may now have multiple memory chunks
**	    associated with it. 
**      12-dec-2007 (huazh01)
**          Make sure the 'back' button is disabled on the initial screen.
**          This prevents installer crashes with SEGV if user does click
**          the 'back' button on the initial screen. 
**          This fixes bug 119603. 
**      03-jan-2008 (huazh01)
**          don't attempt to install the documentation package if the
**          check box for 'documentation package' is not pressed in, 
**          which means the documentation RPM might not be there.
**          This fixes bug 119687.
**	12-Aug-2008 (hanje04)
**	    BUG 120782, SD 130225
**	    Use "selected_instance" structure to check renamed status instead
**	    of calling is_renamed() again. Was causing problems with UTF8
**	    instances although this has also been addressed. Just a more
**	    efficient check.
**	13-Oct-2008 (hanje04)
**	    BUG 121048
**	    Add remove_temp_files() to remove temporary files and directories
**	    created by installation process.
**	30-Mar-2009 (hanje04)
**	    SD 133435, Bug 119603
**	    Make sure 'back button' is dissabled for BOTH startup screens not
**	    just upgrade mode.
**	06-Aug-2009 (hanje04)
**	    BUG 122571
**	    Add argument checking for file passed in from new Python layer.
**	    Entry point for importing info is now gipImportPkgInfo()
**	    Improve error handling for importing saveset and package info.
**	24-Nov-2009 (frima01)
**	    BUG 122571
**	    Add include of unistd.h, libgen.h, clconfig.h and nm.h plus
**	    static keyword to do_install to eliminate gcc 4.3 warnings.
**	15-Jan-2010 (hanje04)
**	    SIR 123296
**	    Config is now run post install from RC scripts. Update installation
**	    commands appropriately.
**	18-May-2010 (hanje04)
**	    SIR 123791
**	    Update installer for Vectorwise support.
**	     - Add new IVW_INSTALL "mode" and VW specific initialization.
**	     - Add init_ivw_cfg() to handle most of this
**	     - Add get_sys_mem() funtion to get the system memory for
**	       determining memory dependent defaults
**	25-May-2010 (hanje04)
**	    SIR 123791
**	    Vectorwise installs should default to VW instead of II
**	26-May-2010 (hanje04)
**	    SIR 123791
**	    Set pkgs_to_install=TYPICAL_VECTORWISE when initializing for
**	    IVW_INSTALL
**	17-Jun-2010 (hanje04)
**	    BUG 123942
**	    Ensure temporary rename dir is always created when rename is
**	    triggered. Was previously only being created for upgrade mode.
**	24-Jun-2010 (hanje04)
**	    BUG 124020
**	    Use instance ID of the install we're modifying when generating
**	    stop/star/config command line, not the default instID.
**	24-Jun-2010 (hanje04)
**	    BUG 124019
**	    Need to add INGCONFIG_CMD to command line length to stop buffer
**	    overrun when generating command line.
**	    Also add gipAppendCmdLine() to manage allocated buffer space and
**	    use it in place of STcat().
**	13-Jul-2010 (hanje04)
**	    BUG 124081
**	    For installs and modifies, save the response file off to 
**	    /var/lib/ingres/XX so that we can find it for subsequent
**	    installations and not inadvertantly overwrite config
**	01-Jul-2010 (hanje04)
**	    BUG 124256
**	    Stop miss reporting errors whilst cleaning up temporary
**	    files. Also don't give up first error, process the whole list.
**	01-Jul-2010 (hanje04)
**	    BUG 123942
**	    If only renamed instance exist, UM_RENAME won't be set so
**	    test for UM_TRUE also when creating temp rename location.
**	04-Oct-2010 (hanje04)
**	    BUG 124537
**	    Improve error reporting in install_control_thread().
**	    If an error occurs generating a response file, generating the 
**	    command line or spawning the installation process. Popup an
**	    error box to the user and abort the installation.
**	    Consolidate out/err display variables too.
*/

# define RF_VARIABLE "export II_RESPONSE_FILE"

/* local prototypes */
static STATUS create_temp_rename_loc( void );
static STATUS remove_temp_files( void );
static void inst_proc_exit_handler( int sig );
static void init_ug_train_icons( UGMODE state );
static STATUS do_install( char *inst_cmdline );
static STATUS gipAppendCmdLine( char *cmdbuff, char *cmd );
size_t get_rename_cmd_len( PKGLST pkglst );

void add_pkglst_to_rmv_command_line( char **cmd_line, PKGLST pkglist );
size_t get_pkg_list_size( PKGLST pkglist );
int calc_cmd_len(size_t *cmdlen);
int generate_command_line( char **cmd_line );
void add_pkglst_to_inst_command_line( char **cmd_line,
						PKGLST pkglist,
						bool renamed );

/* static variables */
static size_t	cmdlen = 0;

/* entry point to gipxml.c */
STATUS
gipImportPkgInfo( LOCATION *xmlfileloc, UGMODE *state, i4 *count );



int
main (int argc, char *argv[])
{
    char	exedir[MAX_LOC];
    char	pixmapdir[MAX_LOC];
    char	pkginfofile[MAX_LOC];
    char	tmpbuf[MAX_LOC];
    char	licfile[MAX_LOC];
    char	*known_instance_loc=NULL;
    char	*binptr=NULL;
    char	*tracevar;
    LOCATION	pkginfoloc;
    LOCATION	licfileloc;
    GtkListStore prog_list_store;
    GtkTextBuffer *lic_text_buff;
    GtkWidget	*widget_ptr; 
    i4		i=0;
    STATUS	rc;
	
    /* init threads */	
    g_thread_init(NULL);
    gdk_threads_init();

    /* initialize GTK */
    gtk_set_locale ();
    gtk_init (&argc, &argv);

    /* check we're running as root */
    if ( getuid() != (uid_t)0 )
    {
# define ERROR_NOT_ROOT "Error: Must be run as root\n"
            SIprintf( ERROR_NOT_ROOT );
            SIflush( stdout );
            return( FAIL );
    }

    /* turn on tracing? */
    NMgtAt( "II_GIP_TRACE", &tracevar );
    if ( tracevar && *tracevar )
        debug_trace = 1 ;

    /* installer the signal handler for SIGCHLD */
    EXsetsig( SIGCHLD, inst_proc_exit_handler );

    /* set defaults */
    userid = (char *)defaultuser;
    groupid = (char *)defaultuser;
    SET_II_SYSTEM( default_ii_system );

    /*
    ** Save set should have the following format:
    **
    **	rootdir
    **	   |
    **	   |-- bin (exedir)
    **	   |
    **	   |-- pixmaps
    **	   |
    **	   |-- rpm
    **	   |
    **	   ~
    **	   |
    **	ingres_install
    */

    /*
    ** To find the root of the install tree relative to the executable,
    ** copy the location locally so we can manipulate the string.
    **
    */
    STcopy( argv[0], tmpbuf );
    dirname( tmpbuf );

    /* make sure we have an absolute path */
    if ( *argv[0] != '/' )
    {
	getcwd( exedir, MAX_LOC );
	STcat( exedir, "/" );
	STcat( exedir, tmpbuf );
    }
    else
	strcpy( exedir, tmpbuf );

    /* Strip off 'bin' directory */
    binptr = STrstrindex( exedir, "bin", 0, FALSE );
    if ( binptr != NULL )
    {
	/* truncate exedir at /bin */
	binptr--;
	*binptr = '\0';
    }
		
    DBG_PRINT( "exedir = %s\n", exedir );
    if ( strlen(exedir) + strlen("/pixmaps") > MAX_LOC )
    {
# define ERROR_PATH_TOO_LONG "Error: Path too long\n"
	SIprintf(ERROR_PATH_TOO_LONG);
	exit(2);
    }
    /* store root directory */
    STcopy( exedir, new_pkgs_info.file_loc );

    STprintf( pixmapdir, "%s%s", exedir, "/pixmaps" );
    add_pixmap_directory( pixmapdir );
	
    /*
    ** Get info about the packages contained in the saveset and
    ** and the ones already installed. Location of file should
    ** have been passed in as an argument
    **
    ** (previous call)
    ** rc = query_rpm_package_info( argc, argv, &inst_state, &inst_count );
    */
    if ( argc > 1 )
    {
	if ( STlen(argv[1]) < MAX_LOC)
	{
	    STcopy(argv[1], pkginfofile);
	    LOfroms(PATH|FILENAME, pkginfofile, &pkginfoloc);
	    if (LOexist(&pkginfoloc) == OK)
    		rc = gipImportPkgInfo(&pkginfoloc, &inst_state, &inst_count );
	    else
# define ERROR_PKGINFO_NOT_FOUND "Error: %s does not exist\n"
		SIprintf(ERROR_PKGINFO_NOT_FOUND, pkginfofile);
	}
	else
	    SIprintf(ERROR_PATH_TOO_LONG);
    }
    else
# define ERROR_MISSING_ARGUMENT "Error: Missing argument, need pkginfo file\n"
	SIprintf(ERROR_MISSING_ARGUMENT);
	 
    if ( rc != OK )
    {
# define ERROR_PKG_QUERY_FAIL "Failed to query package info\n"
	SIprintf( ERROR_PKG_QUERY_FAIL );
	return( rc );
    }

    /* create main install window, hookup the quit prompt box and show it */
    IngresInstall=create_IngresInstall();
    g_signal_connect (G_OBJECT (IngresInstall), "delete_event",
                      G_CALLBACK (delete_event), NULL);
	
    /* initialize New Installation mode */
    if ( init_new_install_mode() != OK )
    {
# define ERROR_INSTALL_INIT_FAIL "Failed to initialize install mode\n"
	  printf( ERROR_INSTALL_INIT_FAIL );
	  return( FAIL );
    }
  
    /* load license file */
    STprintf( licfile, "%s/%s", exedir, "LICENSE" );
    LOfroms(PATH|FILENAME, licfile, &licfileloc);
    if (LOexist(&licfileloc) == OK)
    {
	widget_ptr=lookup_widget(IngresInstall, "inst_lic_view");
	lic_text_buff=gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget_ptr));
	write_file_to_text_buffer(licfile, lic_text_buff);
    }
    else
# define ERROR_LICENSE_NOT_FOUND "Error: %s does not exist\n"
	SIprintf(ERROR_LICENSE_NOT_FOUND, licfile);

    if ( inst_state & UM_RENAME|UM_TRUE )
    {
	/* 
	** We have a non-renamed instance so we'll need to
	** rename the RPMs. Create a temporary location for
	** doing this
	*/
	if ( create_temp_rename_loc() != OK )
 	{
# define ERROR_CREATE_TEMP_LOC "Failed to create temporary location\n"
	    printf( ERROR_CREATE_TEMP_LOC );
	    return( FAIL );
	}
    }
    
    /* If we found other installations then we're in upgrade mode */
    if ( inst_state & UM_TRUE )
    {
  	if ( init_upgrade_modify_mode() != OK )
 	{
# define ERROR_UPGRADE_INIT_FAIL "Failed to initialize upgrade mode\n"
	    printf( ERROR_UPGRADE_INIT_FAIL );
	    return( FAIL );
	}

	/* load upgrade license file */
	widget_ptr=lookup_widget(IngresInstall, "upg_lic_view");
	lic_text_buff=gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget_ptr));
	write_file_to_text_buffer(licfile, lic_text_buff);
    }

    /* load license file */
    /* set the correct starting screen */
    set_screen(START_SCREEN);

    /* show the wizard */
    gtk_widget_show(IngresInstall);
    
    gdk_threads_enter();
    gtk_main ();
    gdk_threads_leave();
  
    /* clean up before exit */
    if ( remove_temp_files() != OK )
    {
# define ERROR_REMOVE_TEMP_LOC "Failed to remove temporary location\n"
	printf( ERROR_REMOVE_TEMP_LOC );
    }

    while( existing_instances[i] != NULL )
    {
	MEfreetag( existing_instances[i]->memtag );
	existing_instances[i] = NULL;
	i++;
    }

    return( EXIT_STATUS );
}

gint
init_new_install_mode(void)
{
    GtkWidget	*widget_ptr;
    GtkWidget	*locale_box;
    GtkWidget	*db_config_type;
    GtkWidget	*ing_misc_ops;
    GtkWidget	*ivw_misc_ops;
    GtkWidget	*ivw_cfg_box;

    char	*IDptr;
    gchar	textbuf[10];
    i4 		i = 1;


    /* set mode */
    runmode = MA_INSTALL;
    instmode = BASIC_INSTALL;

    /* check which product we're installing */
    if ( STcompare("vectorwise", new_pkgs_info.product) == OK )
    {	
# define RFGEN_IVW_TITLE "Ingres VectorWise Installation Wizard"
	gtk_window_set_title( GTK_WINDOW( IngresInstall ), RFGEN_IVW_TITLE );
	instmode |= IVW_INSTALL;
	stage_names = basic_ivw_stages;
	pkgs_to_install=TYPICAL_VECTORWISE;
	STcopy("VW", dfltID);
    }
    else
    {
# define RFGEN_TITLE "Ingres Installation Wizard"
	gtk_window_set_title( GTK_WINDOW( IngresInstall ), RFGEN_TITLE );
    }

    /* check installation ID */
    STprintf( textbuf, "Ingres %s", dfltID );
    while ( OK == inst_name_in_use( textbuf ) ) 
    {
	char IDbuf[2];

	STprintf( IDbuf, "%d", i++ );
	dfltID[1] = IDbuf[0];
	STprintf( textbuf, "Ingres %s", dfltID );
    } 

    IDptr = STrstrindex( default_ii_system, RFAPI_DEFAULT_INST_ID, 0, FALSE );
    IDptr[0] = instID[0] = dfltID[0];
    IDptr[1] = instID[1] = dfltID[1];

    /* and set the widgets */
    widget_ptr = lookup_widget( IngresInstall, "instname_label" );
    gtk_label_set_text( GTK_LABEL( widget_ptr ), textbuf );
    widget_ptr = lookup_widget(IngresInstall, "instID_entry2");
    gtk_entry_set_text( GTK_ENTRY( widget_ptr ), dfltID );
    
    /* set ii_system */
    widget_ptr = lookup_widget( IngresInstall, "ii_system_entry" );
    gtk_entry_set_text( GTK_ENTRY( widget_ptr), default_ii_system );

    /* Create dialogs to act as combo boxes */
    /* timezones */
    TimezoneTreeModel = initialize_timezones();
    if ( instmode & IVW_INSTALL )
	widget_ptr = lookup_widget( IngresInstall, "IVWTimezoneEntry" );
    else
	widget_ptr = lookup_widget( IngresInstall, "TimezoneEntry" );

    TimezoneDialog = create_text_view_dialog_from_model( TimezoneTreeModel,
							widget_ptr,
							timezone_iter );


    /* charsets */
    CharsetTreeModel = initialize_charsets( linux_charsets );
    if ( instmode & IVW_INSTALL )
	widget_ptr = lookup_widget( IngresInstall, "IVWCharsetEntry" );
    else
	widget_ptr = lookup_widget( IngresInstall, "CharsetEntry" );

    CharsetDialog = create_text_view_dialog_from_model( CharsetTreeModel,
							widget_ptr,
							charset_iter );

    /* hide/show required widgets */
    widget_ptr=lookup_widget(IngresInstall, "bkp_log_file_loc");
    gtk_widget_set_sensitive(widget_ptr, FALSE);
    widget_ptr=lookup_widget(IngresInstall, "bkp_log_browse");
    gtk_widget_set_sensitive(widget_ptr, FALSE);
    widget_ptr=lookup_widget(IngresInstall, "bkp_log_label");
    gtk_widget_set_sensitive(widget_ptr, FALSE);	
    widget_ptr=lookup_widget(IngresInstall, "RfPlatformBox" );
    gtk_widget_hide(widget_ptr);
    widget_ptr=lookup_widget(IngresInstall, "RfEndTable" );
    gtk_widget_hide(widget_ptr);

    /* b119603, make sure back button is disabled on both screens. */ 
    widget_ptr=lookup_widget(IngresInstall, "ug_back_button" );
    gtk_widget_set_sensitive(widget_ptr, FALSE);
    widget_ptr=lookup_widget(IngresInstall, "back_button" );
    gtk_widget_set_sensitive(widget_ptr, FALSE);

    
    /* set default package lists */
    set_inst_pkg_check_buttons();
    
    /* create formatting tags for summary & finish screens */
    widget_ptr=lookup_widget(IngresInstall, "SummaryTextView");
    create_summary_tags( gtk_text_view_get_buffer(
				GTK_TEXT_VIEW( widget_ptr ) ) );
    widget_ptr=lookup_widget(IngresInstall, "FinishTextView");
    create_summary_tags( gtk_text_view_get_buffer( 
			 	GTK_TEXT_VIEW( widget_ptr ) ) );
							
    /* initialize vectorwise config */
    locale_box=lookup_widget(IngresInstall, "locale_box");
    db_config_type=lookup_widget(IngresInstall, "config_type_box");
    ing_misc_ops=lookup_widget(IngresInstall, "ing_misc_ops_frame");
    ivw_misc_ops=lookup_widget(IngresInstall, "ivw_misc_ops_frame");
    ivw_cfg_box=lookup_widget(IngresInstall, "ivw_cfg_box");

    if ( instmode & IVW_INSTALL )
    {

	/* hide/show vectorwise related config in train */
	gtk_widget_hide(locale_box);
	gtk_widget_hide(db_config_type);
	gtk_widget_hide(ing_misc_ops);
	gtk_widget_show(ivw_misc_ops);
	gtk_widget_show(ivw_cfg_box);

	init_ivw_cfg();
    }
    else
    {
	gtk_widget_show(locale_box);
	gtk_widget_show(db_config_type);
	gtk_widget_show(ing_misc_ops);
	gtk_widget_hide(ivw_misc_ops);
	gtk_widget_hide(ivw_cfg_box);
    }

    /* Set default config type */
    widget_ptr=lookup_widget(IngresInstall, "ing_cfg_type_radio_tx");
    gtk_button_clicked(GTK_BUTTON(widget_ptr));

    return(OK);
}

gint
init_upgrade_modify_mode(void)
{
    GtkWidget	*instance_swindow;
    GtkWidget	*new_inst_table;
    GtkWidget	*ug_install_table;
    GtkWidget	*inst_sel_table;
    GtkWidget	*ug_mod_table;
    GtkWidget	*widget_ptr;
    GtkTreeModel *instance_list;
    GtkTreeSelection *ug_select;
    GtkTreeIter	first;

 
    /* set the run mode */
    runmode=MA_UPGRADE;
    ug_mode|=UM_TRUE;

    new_inst_table=lookup_widget(IngresInstall, "new_inst_table");
    ug_install_table=lookup_widget(IngresInstall, "ug_install_table");
    inst_sel_table=lookup_widget(IngresInstall, "inst_sel_table");
    ug_mod_table=lookup_widget(IngresInstall, "ug_mod_table");

    /* 
    ** If an older installation is detect then upgrade is our default mode
    ** we only set UM_MOD initially if only instances of the same version 
    ** have been detected
    **
    ** Then initialize train apropriately
    */
    if ( inst_state & UM_UPG )
    {
	ug_mode|=UM_UPG;
	gtk_widget_hide(new_inst_table);
	gtk_widget_hide(ug_install_table);
	gtk_widget_hide(ug_mod_table);
    }
    else
    {
	ug_mode|=UM_MOD;		
	gtk_widget_hide(new_inst_table);
	gtk_widget_show(ug_install_table);
	gtk_widget_show(ug_mod_table);
    }

    gtk_widget_show(inst_sel_table);
	  
    /* setup multiple installation stuff if needed */
    int 	i=0;
    
    /* initialized container for displaying instances */
    instance_list=initialize_instance_list();

    
    /* attach instance list to window */
    InstanceTreeView = gtk_tree_view_new_with_model( instance_list );
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW ( InstanceTreeView ), TRUE);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW ( InstanceTreeView ),
					    COL_NAME);
    g_object_unref (instance_list);
    /* pack it into the scrollwindow */
    instance_swindow=lookup_widget( IngresInstall, "instance_swindow" );
    gtk_container_add( GTK_CONTAINER( instance_swindow ),
				InstanceTreeView );
    
    /* add the appropriate columns */
    add_instance_columns( GTK_TREE_VIEW( InstanceTreeView ) );
    
    gtk_list_store_clear( GTK_LIST_STORE( instance_list ) );
    while ( i < inst_count )
    {
	/* and load that into the list */
	    if ( existing_instances[i]->action & ug_mode )
		add_instance_to_store( GTK_LIST_STORE(instance_list),
					existing_instances[i] );
    
 	    i++;
	    DBG_PRINT("Added instance %d of %d to %s store\n",
					i,
					inst_count,
					ug_mode & UM_MOD ?
					"modify" : "upgrade" );
    }

    /* Create and connect selection handler */
    ug_select = gtk_tree_view_get_selection (
				GTK_TREE_VIEW( InstanceTreeView ));
    gtk_tree_selection_set_mode (ug_select, GTK_SELECTION_SINGLE);
    g_signal_connect (G_OBJECT (ug_select),
			"changed",
			G_CALLBACK (instance_selection_changed),
			NULL); 

    /* select the first item */
    gtk_tree_model_get_iter_first( instance_list, &first );
    gtk_tree_selection_select_iter( ug_select, &first );

    gtk_widget_show( InstanceTreeView );

    /* set installed packages check boxes for the selected instance */
    set_mod_pkg_check_buttons( selected_instance->installed_pkgs );

    /* display the right icons on the train */
    init_ug_train_icons( inst_state );

    /* create formatting tags for summary & finish screens */
    widget_ptr=lookup_widget(IngresInstall, "UgSummaryTextView");
    create_summary_tags( gtk_text_view_get_buffer( 
      GTK_TEXT_VIEW( widget_ptr ) ) );
    widget_ptr=lookup_widget(IngresInstall, "UgFinishTextView");
    create_summary_tags( gtk_text_view_get_buffer( 
      GTK_TEXT_VIEW( widget_ptr ) ) );

   return( OK );
}

static STATUS
create_temp_rename_loc( void )
{
    LOCATION            tmploc; /* temporary LOCATION  storage */
    char		tmpbuf[50]; /* temporary path storage */
    char                *stptr; /* misc string pointer */
    PID                 pid; /* process ID of current process */
    STATUS              clstat; /* cl return codes */

    /* find a temporary dir */
    NMgtAt( TMPVAR, &stptr );
    if ( stptr == NULL || *stptr == '\0' )
    {
        stptr = TMPDIR ;
    }
    STcopy( stptr, rndirstr );
    clstat = LOfroms( PATH, rndirstr, &rndirloc );

    if ( clstat != OK )
	return( clstat );

    /* use the PID to create genreate a rename dir name */
    PCpid( &pid );
    STprintf( tmpbuf, "%s.%d", RPM_RENAME_DIR, pid );
    clstat = LOfroms( PATH, tmpbuf, &tmploc );

    if ( clstat != OK )
	return( clstat );

    /* now combine them */
    clstat = LOaddpath( &rndirloc, &tmploc, &rndirloc );	

    if ( clstat != OK )
	return( clstat );

    /* finally create the directory */
    clstat = LOcreate( &rndirloc );
    return( clstat );

}

static STATUS
remove_temp_files( void )
{
    char		*stdiofiles[] = { inst_stdout, inst_stderr, NULL }; 
    i2			i = 0;
    LOCATION		tmpfileloc; 
    STATUS              clstat; /* cl return codes */

    /* Clean up stdin/out files used during installation */
    while( i < 2 )
    {
	if ( *stdiofiles[i] )
	{
	    LOfroms( PATH & FILENAME, stdiofiles[i], &tmpfileloc );
	    if ( LOexist( &tmpfileloc ) == OK )
        	clstat = LOdelete( &tmpfileloc );

	    if ( clstat != OK )
		DBG_PRINT("Error occurred removing %s\n", stdiofiles[i]);
	}
	i++;
    }
	
    /* if we have a rename location and it's valid, remove it */
    if ( rndirstr && ( clstat = LOexist( &rndirloc ) == OK ) )
        clstat = LOdelete( &rndirloc );

    return( clstat );
}

GtkTreeModel *
initialize_instance_list()
{
     GtkTreeIter	row_iter;
     GtkListStore *instance_store;
     gint i;
	
     /* create a list store to hold the instance info */
     instance_store=gtk_list_store_new(NUM_COLS,
					G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_STRING);	
     /* Return the pointer to the list */
     return GTK_TREE_MODEL( instance_store );
}

static void
inst_proc_exit_handler( int sig )
{
    PID	cpid; /* child pid */
    int status, exitstatus;

    if ( sig == SIGCHLD )
    {
	/* clear SIGCHLD by calling PCreap(). */
	cpid = PCreap();
	if ( cpid > 0 )
	{
	    childinfo.exit_status = PCwait( cpid );
	    switch( childinfo.exit_status )
	    {
		case E_CL1623_PC_WT_INTR:
		case E_CL1628_PC_WT_EXEC:
		    childinfo.alive = TRUE;
		    break;
		default:
		    childinfo.alive = FALSE;
	    }
	}
	else
	    childinfo.exit_status = FAIL;

	if ( childinfo.alive == TRUE )
	    DBG_PRINT( "Child pid %d, received not fatal signal\n",
				cpid );
	else
	    DBG_PRINT( "Child pid %d, reaped with exit code %d\n",
				cpid, childinfo.exit_status  );
	   
    }
    else
	/* something went very wrong */
	PCexit( FAIL );

    /* replant the handler */
    EXsetsig( sig, inst_proc_exit_handler );
}

static void
init_ug_train_icons( UGMODE state )
{
    GtkWidget	*upgrade_inst_mod;
    GtkWidget	*upgrade_inst_ug;
    GtkWidget	*upgrade_inst_multi;

    /* look up icons we need to affect */
    upgrade_inst_mod = lookup_widget( IngresInstall, "upgrade_inst_mod" );
    upgrade_inst_ug = lookup_widget( IngresInstall, "upgrade_inst_ug" );
    upgrade_inst_multi = lookup_widget( IngresInstall, "upgrade_inst_multi" );

    /* hide all and show based on state */
    gtk_widget_hide( upgrade_inst_mod );
    gtk_widget_hide( upgrade_inst_ug );
    gtk_widget_hide( upgrade_inst_multi );

    if ( state & UM_MOD && state & UM_UPG )
	gtk_widget_show( upgrade_inst_multi );
    else if ( state & UM_MOD )
	gtk_widget_show( upgrade_inst_mod );
    else
	gtk_widget_show( upgrade_inst_ug );
}

void *
install_control_thread( void *arg )
{
    GtkWidget		*OutTextView;
    GtkWidget		*ErrTextView;
    GtkWidget		*InstProgBar;
    GtkTextBuffer	*out_buffer;
    GtkTextBuffer	*err_buffer;
    II_RFAPI_STATUS	rfstat;
    PID			ppid;
    char	*inst_cmdline;
    char	*errstr = NULL;
# define MAX_ERR_LEN 250
    char	errbuf[MAX_ERR_LEN];
    int	i = 1; /* loop counter */

    /* initialize log file names */
    PCpid( &ppid );
    STprintf( inst_stdout, "%s.%d", STDOUT_LOGFILE_NAME, ppid );
    STprintf( inst_stderr, "%s.%d", STDERR_LOGFILE_NAME, ppid );

    /* generate response file */
    if ( rfstat = generate_response_file() != II_RF_ST_OK )
    {
# define ERROR_CREATE_RF_FAIL "Response file creation failed with:\n\t"
	STlpolycat( 2, MAX_ERR_LEN,
		ERROR_CREATE_RF_FAIL,
		IIrfapi_errString( rfstat ), &errbuf[0] );
	errstr = errbuf;
    }
	
    /* generate installation command line */
    if ( errstr == NULL && generate_command_line( &inst_cmdline ) != OK )
    {
# define ERROR_GEN_CMD_FAIL "Failed to generate command line"
	errstr = ERROR_GEN_CMD_FAIL;
        DBG_PRINT( "Command line is %s\n", inst_cmdline );
    }

    if ( errstr == NULL && do_install( inst_cmdline ) != OK )
    {
# define ERROR_RUN_CMD_FAIL  "Failed to spawn installation process"
	errstr = ERROR_RUN_CMD_FAIL;
        DBG_PRINT( "Command line is %s\n", inst_cmdline );
    }

    if ( errstr != NULL )
    {
	/*
	** Something went wrong before the install was launched
	** notify the user and bail
	*/

	/* mark the stage as failed */
	stage_names[current_stage]->status = ST_FAILED;

	/* write output to failure window */
	gdk_threads_enter();
	ErrTextView = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"InstFailErrorTextView" :
						"UpgFailErrorTextView" );
	err_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( ErrTextView ) );
	gtk_text_buffer_set_text( err_buffer, errstr, STlen(errstr) );

        /* notify user */
	popup_error_box( errstr );

	/* move on again */
	on_master_next_clicked( NULL, NULL );

	gdk_threads_leave();

	/* quit thread */
	return(NULL); 
    }

    /* get GTK thread lock before doing GTK stuff */
    gdk_threads_enter();

    /* lookup text buffer to write to etc */
    OutTextView = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"InstOutTextView" :
						"UpgOutTextView" );
    out_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(
						 OutTextView ) );
    ErrTextView = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"InstErrTextView" :
						"UpgErrTextView" );
    err_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(
						 ErrTextView ) );
    InstProgBar = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"installprogress" : 
						"upgprogress" );

    gtk_progress_bar_set_pulse_step( GTK_PROGRESS_BAR( InstProgBar ), 0.01 );

    /* release GTK thread lock so that we don't hang the app whilst we sleep */
    gdk_threads_leave();

    /* poll the output files until the installer child exits */
    do
    {
	/* only re-write output every 100 loops to stop flickering */
	if ( i > 100 )
	{
            /*  wait and then try again, the output files may just be empty */
	    DBG_PRINT( "Calling write_file_to_text_buffer()\n" );
	    if ( write_file_to_text_buffer( inst_stdout, out_buffer ) != OK )
		DBG_PRINT( "ERROR: Failed to write to output buffer\n" );

	    if ( write_file_to_text_buffer( inst_stderr, err_buffer ) != OK )
		DBG_PRINT( "ERROR: Failed to write to output buffer\n" );
	    
	    /* reset counter */
	    i = 0;
	}

	/* get GTK thread lock */
	gdk_threads_enter();

	/* Show we're doing something */
	gtk_progress_bar_pulse( GTK_PROGRESS_BAR( InstProgBar ) );

	/* release GTK thread lock */
	gdk_threads_leave();

	PCsleep( 20 );
	i++; /* bump count */
    } while ( childinfo.alive ) ;

    /* one last read to make sure we got everything */
    if ( write_file_to_text_buffer( inst_stdout, out_buffer ) != OK )
	DBG_PRINT( "ERROR: Failed to write to output buffer\n" );

    if ( write_file_to_text_buffer( inst_stderr, err_buffer ) != OK )
	DBG_PRINT( "ERROR: Failed to write to output buffer\n" );

    if ( childinfo.exit_status != OK )
    {
# define ERROR_COULD_NOT_COMPLETE_INSTALL "The installation process did not\ncomplete successfully"
	/* take GTK thread lock */
	gdk_threads_enter();
	popup_error_box( ERROR_COULD_NOT_COMPLETE_INSTALL );
	/* mark the stage as failed */
	stage_names[current_stage]->status = ST_FAILED;

	/* write output to failure window */
	ErrTextView = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"InstFailErrorTextView" :
						"UpgFailErrorTextView" );
	err_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( ErrTextView ) );

	OutTextView = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"InstFailOutputTextView" :
						"UpgFailOutputTextView" );
	out_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( OutTextView ) );

	/* Release the lock before calling write_file_to_buffer() */
	gdk_threads_leave();

	if ( write_file_to_text_buffer( inst_stderr, err_buffer ) != OK )
	     DBG_PRINT( "ERROR: Failed to write to error buffer\n" );

	if ( write_file_to_text_buffer( inst_stdout, out_buffer ) != OK )
	     DBG_PRINT( "ERROR: Failed to write to output buffer\n" );

    }

    if ( ug_mode & UM_INST|UM_MOD && LOexist( &rfnameloc ) == OK )
    {
	char	 rfsavestr[MAX_LOC] = {'\0'};
	LOCATION rfsaveloc;
	char	*instid;
	
	/* Install was successfull, copy response file to /var/lib/ingres/XX */
	if ( ug_mode & UM_INST )
	    instid = instID;
	else
	    instid = selected_instance->instance_ID;

	rfstat = gen_rf_save_name( rfsavestr, instid );

	if ( rfstat == OK )
	    rfstat = LOfroms(PATH & FILENAME, rfsavestr, &rfsaveloc);

	if ( rfstat == OK )
	    rfstat = SIcopy(&rfnameloc, &rfsaveloc);

	if ( rfstat != OK )
	{
	    char warnmsg[MAX_LOC + 50 ];

# define WARN_COULD_NOT_SAVE_RF "Could not save response file: %s"
	    STprintf( warnmsg, WARN_COULD_NOT_SAVE_RF, rfsavestr );
	    gdk_threads_enter();
	    popup_warning_box( warnmsg );
	    gdk_threads_leave();
	}
    }
	    
    /* Install is complete, remove the response file */
    if ( LOexist( &rfnameloc ) == OK )
    	LOdelete( &rfnameloc );

    /* get GTK thread lock again before moving on */
    gdk_threads_enter();
        
    on_master_next_clicked( NULL, NULL ); /* move on again */

    /* release GTK thread lock */
    gdk_threads_leave();

    /* done */
    return(NULL); 
}

static STATUS
do_install( char *inst_cmdline )
{
    PID		instpid;
    STATUS	forkrc;
    char	*argv[5];

    if ( inst_cmdline == NULL || *inst_cmdline == '\0' )
	return( FAIL );

    /* prepare command for execution */
    argv[0] = "/bin/sh" ;
    argv[1] = "-p" ;
    argv[2] = "-c" ;
    argv[3] = inst_cmdline ;
    argv[4] = NULL ;

    /* fork to run the install */
    instpid = PCfork(&forkrc);
    if ( instpid == 0 ) /* child, run the install */
    {
	i4	rc = -1;
	pid_t	cpid = getpid();
  
	/* restore the default handler in the child */
	EXsetsig( SIGCHLD, SIG_DFL );

	/* launch install */
	execvp( argv[0], argv );
	DBG_PRINT( "Child %d, exited with %d\n", cpid ,rc );
	sleep( 2 );
	PCexit( rc );
    }
    else if ( instpid > 0 ) /* parent, return to caller */
    {
	DBG_PRINT( "Installation launched with pid = %d\n", instpid );
	childinfo.alive = TRUE;
        /* give the install process a chance to start up */
	PCsleep( 2000 );
	return( forkrc );
    }
    else /* fork failed */
	return( forkrc );
    
}

int
generate_command_line( char **cmd_line )
{
    char	tmpbuf[MAX_LOC];
    GtkWidget   *checkButton;

    /* calculate the length of the command line we're going to generate */
    if ( calc_cmd_len(&cmdlen) || cmdlen == 0 )
	return( FAIL );

    DBG_PRINT("Allocating %d bytes for command line", cmdlen);
    /* allocate memory for the command line */
    *cmd_line = malloc( cmdlen );

    if ( *cmd_line == NULL )
	return( FAIL );

    **cmd_line = '\0';
    /* whole command wrapped in '( )' so all output can be re-directed */
    /* Add opening '(' and set umask and response file */
    STprintf( tmpbuf, "( %s ; %s=%s; ",
		UMASK_CMD,
		RF_VARIABLE,
		rfnameloc.string );
    gipAppendCmdLine(*cmd_line, tmpbuf);

    /* write to buffer */
    if ( ug_mode & UM_INST ) /* new installation */
    {
	/* do we need to rename? */
 	if ( inst_state & UM_RENAME )
	{
	    /* 
	    ** Use the 'iirpmrename' to re-package the RPM packages
	    ** to include the instance ID in the package name. The
	    ** new packages will be created in the directory pointed
	    ** to by 'rndirloc'
	    */
	
	    /* CD to temporary dir */
	    STprintf( tmpbuf, "cd %s && %s/bin/%s ",
			rndirstr,
			new_pkgs_info.file_loc,
			RPM_RENAME_CMD );
 	    gipAppendCmdLine( *cmd_line, tmpbuf );

	    /* add package list */
	    add_pkglst_to_inst_command_line( cmd_line, pkgs_to_install, FALSE );

	    /* add installation id and command suffix */
	    STprintf( tmpbuf, " %s && ", instID );
 	    gipAppendCmdLine( *cmd_line, tmpbuf );

	    /*
	    ** Finally, update the saveset info to reflect the new
	    ** location of the RPM packages.
	    */
	    STcopy( rndirstr, new_pkgs_info.file_loc );
	}
	    

	/* rpm command and II_SYSTEM prefix */
	STprintf( tmpbuf, "%s %s=%s ",
		RPM_INSTALL_CMD,
		RPM_PREFIX,
		iisystem.path );
 	gipAppendCmdLine( *cmd_line, tmpbuf );

	add_pkglst_to_inst_command_line( cmd_line, pkgs_to_install,
					(bool)( inst_state & UM_RENAME ) );


	/*
	** Run the setup using the init script
	** If everything went OK start the instance, if it fails,
	** make sure it's shut down properly
	*/
	gipAppendCmdLine( *cmd_line, " && ( " );
	STprintf( tmpbuf, INGCONFIG_CMD, instID, rfnameloc.string );
 	gipAppendCmdLine( *cmd_line, tmpbuf );
	gipAppendCmdLine( *cmd_line, " && " );
	STprintf( tmpbuf, INGSTART_CMD, instID );
 	gipAppendCmdLine( *cmd_line, tmpbuf );
	gipAppendCmdLine( *cmd_line, " || ( " );
	STprintf( tmpbuf, INGSTOP_CMD, instID );
 	gipAppendCmdLine( *cmd_line, tmpbuf );
	gipAppendCmdLine( *cmd_line, " ; exit 1 ) ) " );

        checkButton = lookup_widget(IngresInstall, "checkdocpkg");

        /* add doc last if it's needed: 
        **
        ** 1. no doc on renamed instance. 
        ** 2. no doc if the check box for documentation pkg is not 
        ** pressed in, which means documentation RPM might not be there. 
        ** (b119687)
        */ 
	if ( pkgs_to_install & PKG_DOC && 
             ~inst_state & UM_RENAME   &&
             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton))
           ) 
	{
	    STprintf( tmpbuf, " && %s ", RPM_INSTALL_CMD );
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	    STprintf( tmpbuf, "%s/%s/%s-documentation-%s.%s.%s ",
		new_pkgs_info.file_loc,
		new_pkgs_info.format,
		new_pkgs_info.pkg_basename,
		new_pkgs_info.version,
		new_pkgs_info.arch,
		new_pkgs_info.format);
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	}
    }
    else if ( ug_mode & UM_UPG ) /* upgrade existing */
    {
	PKGLST upg_list;

	/*
	** The list of packages we're installing as part of the upgrade
	** may not be identical to the list of packages already installed.
	** The upg_list is therefore the list of packages which are both
	** installed and present in the saveset.
	*/
	upg_list = new_pkgs_info.pkgs & selected_instance->installed_pkgs;

	/* 
	** There are a number of RPM issues associated with upgrading.
	** These can be avoided by using a 2 phase upgrade process.
	**
	** We first force install the new packages as an additional RPM
	** instance.
	*/

	/* do we need to rename? */
 	if ( selected_instance->action & UM_RENAME )
	{
	    /* 
	    ** Use the 'iirpmrename' to re-package the RPM packages
	    ** to include the instance ID in the package name. The
	    ** new packages will be created in the directory pointed
	    ** to by 'rndirloc'
	    */
	
	    /* CD to temporary dir */
	    STprintf( tmpbuf, "cd %s && %s/bin/%s ",
			rndirstr,
			new_pkgs_info.file_loc,
			RPM_RENAME_CMD );
 	    gipAppendCmdLine( *cmd_line, tmpbuf );

	    /* add package list */
	    add_pkglst_to_inst_command_line( cmd_line, upg_list, FALSE );

	    /* add installation id and command suffix */
	    STprintf( tmpbuf, " %s && ", selected_instance->instance_ID );
 	    gipAppendCmdLine( *cmd_line, tmpbuf );

	    /*
	    ** Finally, update the saveset info to reflect the new
	    ** location of the RPM packages.
	    */
	    STcopy( rndirstr, new_pkgs_info.file_loc );
	}

	/* rpm command and II_SYSTEM prefix */
	STprintf( tmpbuf, "%s %s %s=%s ",
		RPM_INSTALL_CMD,
		RPM_FORCE_INSTALL,
		RPM_PREFIX,
		selected_instance->inst_loc );
 	gipAppendCmdLine( *cmd_line, tmpbuf );

	add_pkglst_to_inst_command_line( cmd_line, upg_list,
					(bool)( selected_instance->action
							& UM_RENAME ) );

	/*
	** Add command separator, use && so that if the first part
	** fails we don't continue and trash the system
	*/
	gipAppendCmdLine( *cmd_line, "&& " );

	/*
	** The old package info is then removed from the RPM
	** database.
	*/
 	gipAppendCmdLine( *cmd_line, RPM_UPGRMV_CMD );
	add_pkglst_to_rmv_command_line( cmd_line,
					    selected_instance->installed_pkgs );

	/*
	** Run the setup using the init script
	** If everything went OK start the instance, if it fails,
	** make sure it's shut down properly
	*/
	gipAppendCmdLine( *cmd_line, " && ( " );
	STprintf( tmpbuf, INGCONFIG_CMD, selected_instance->instance_ID,
			 rfnameloc.string );
 	gipAppendCmdLine( *cmd_line, tmpbuf );
	gipAppendCmdLine( *cmd_line, " && " );
	STprintf( tmpbuf, INGSTART_CMD, selected_instance->instance_ID );
 	gipAppendCmdLine( *cmd_line, tmpbuf );
	gipAppendCmdLine( *cmd_line, " || ( " );
	STprintf( tmpbuf, INGSTOP_CMD, selected_instance->instance_ID );
 	gipAppendCmdLine( *cmd_line, tmpbuf );
	gipAppendCmdLine( *cmd_line, " ; exit 1 ) ) " );

	/* add doc last if it's needed */
	if ( upg_list & new_pkgs_info.pkgs & PKG_DOC )
	{
	    STprintf( tmpbuf, " && %s ", RPM_INSTALL_CMD );
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	    STprintf( tmpbuf, "%s/%s/%s-documentation-%s.%s.%s ",
		new_pkgs_info.file_loc,
		new_pkgs_info.format,
		new_pkgs_info.pkg_basename,
		new_pkgs_info.version,
		new_pkgs_info.arch,
		new_pkgs_info.format);
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	}
    }
    else if ( ug_mode & UM_MOD ) /* modify existing */
    {
	/*
	** Modify can also be a 2 phase process as user can choose to
	** both add and remove packages. Perform the removal first.
	*/
	if ( mod_pkgs_to_remove != PKG_NONE )
	{
	    /*
	    ** if only the doc package is being removed,
	    ** skip adding any command line for the rest
	    */
	    if ( mod_pkgs_to_remove != PKG_DOC )
	    {
		/* rpm command */
		gipAppendCmdLine( *cmd_line, RPM_REMOVE_CMD );
		add_pkglst_to_rmv_command_line( cmd_line, mod_pkgs_to_remove );
	    }
	   
	    /* add doc last if it's needed */
	    if ( mod_pkgs_to_remove & PKG_DOC )
	    {
		STprintf( tmpbuf, " && %s %s-documentation-%s ",
		    RPM_REMOVE_CMD,
		    selected_instance->pkg_basename,
		    selected_instance->version );
		gipAppendCmdLine( *cmd_line, tmpbuf );
	    }

	    /* add command separator if there's more to follow */
	    if ( mod_pkgs_to_install != PKG_NONE )
	        gipAppendCmdLine( *cmd_line, "&& " );
	    /* remove $II_SYSTEM/ingres/. if we've been asked to */
	    else if ( misc_ops & GIP_OP_REMOVE_ALL_FILES )
	    {
		auxloc	*locptr;

		/* remove database locations first... */
		STprintf( tmpbuf, " && %s ", RM_CMD );
		gipAppendCmdLine( *cmd_line, tmpbuf );

		locptr = selected_instance->datalocs;
		if ( locptr == NULL )
		    DBG_PRINT( "auxlocs is NULL, no aux locations found\n" );

		while ( locptr != NULL )
		{
		    STprintf( tmpbuf, "%s/ingres/%s/ ",
				locptr->loc,
				dblocations[locptr->idx]->dirname );
		    gipAppendCmdLine( *cmd_line, tmpbuf );
		    locptr = locptr->next_loc ;
		}
		    
		/* ...then log file locations... */
		STprintf( tmpbuf, " && %s ", RM_CMD );
		gipAppendCmdLine( *cmd_line, tmpbuf );

		locptr = selected_instance->loglocs;
		if ( locptr == NULL )
		    DBG_PRINT( "auxlocs is NULL, no aux locations found\n" );

		while ( locptr != NULL )
		{
		    STprintf( tmpbuf, "%s/ingres/log/ ",
				locptr->loc );
		    gipAppendCmdLine( *cmd_line, tmpbuf );
		    locptr = locptr->next_loc ;
		}
		    
		/* ...then II_SYSTEM */
		STprintf( tmpbuf, "%s/ingres/", selected_instance->inst_loc );
		gipAppendCmdLine( *cmd_line, tmpbuf );
	    }

	}

	/* Now install the new packages */
	if ( mod_pkgs_to_install != PKG_NONE )
	{
	    /*
	    ** if only the doc package is being removed,
	    ** skip adding any command line for the rest
	    */
	    if ( mod_pkgs_to_install != PKG_DOC )
	    {
	        /* do we need to rename? */
 	        if ( selected_instance->action & UM_RENAME )
	        {
		    /* 
		    ** Use the 'iirpmrename' to re-package the RPM packages
		    ** to include the instance ID in the package name. The
		    ** new packages will be created in the directory pointed
		    ** to by 'rndirloc'
		    */
		
		    /* CD to temporary dir */
		    STprintf( tmpbuf, "cd %s && %s/bin/%s ",
				rndirstr,
				new_pkgs_info.file_loc,
				RPM_RENAME_CMD );
		    gipAppendCmdLine( *cmd_line, tmpbuf );

		    /* add package list */
		    add_pkglst_to_inst_command_line( cmd_line,
						mod_pkgs_to_install,
						FALSE );
    
		    /* pass it as ID to renamd command */
		    STprintf( tmpbuf, " %s && ",
				selected_instance->instance_ID );
		    gipAppendCmdLine( *cmd_line, tmpbuf );

		    /*
		    ** Finally, update the saveset info to reflect the new
		    ** location of the RPM packages.
		    */
		    STcopy( rndirstr, new_pkgs_info.file_loc );
		}

		/* rpm command and II_SYSTEM prefix */
		STprintf( tmpbuf, "%s %s=%s ",
		    RPM_INSTALL_CMD,
		    RPM_PREFIX,
		    selected_instance->inst_loc );
		gipAppendCmdLine( *cmd_line, tmpbuf );

		/* add packages */
		add_pkglst_to_inst_command_line( cmd_line, mod_pkgs_to_install,
					(bool)( selected_instance->action
							& UM_RENAME ) );

	    } /* ! doc only */

	    /* add doc last if it's needed */
	    if ( mod_pkgs_to_install & new_pkgs_info.pkgs & PKG_DOC )
	    {
		if ( mod_pkgs_to_install != PKG_DOC )
		    STprintf( tmpbuf, " && %s ", RPM_INSTALL_CMD );
		else
		    STprintf( tmpbuf, " %s ", RPM_INSTALL_CMD );

		gipAppendCmdLine( *cmd_line, tmpbuf );
		STprintf( tmpbuf, "%s/%s/%s-documentation-%s.%s.%s ",
		    new_pkgs_info.file_loc,
		    new_pkgs_info.format,
		    new_pkgs_info.pkg_basename,
		    new_pkgs_info.version,
		    new_pkgs_info.arch,
		    new_pkgs_info.format);
		gipAppendCmdLine( *cmd_line, tmpbuf );
	    }
	}

	/*
	** Run the setup using the init script.
	** If we're not removing everything and the installation went OK,
	** start the instance. If it fails, make sure it's shut down properly
	*/
	if ( mod_pkgs_to_remove != selected_instance->installed_pkgs &&
		( mod_pkgs_to_install != PKG_NONE || 
		 mod_pkgs_to_remove != PKG_NONE ) )
	{
	    gipAppendCmdLine( *cmd_line, " && ( " );
	    STprintf( tmpbuf, INGCONFIG_CMD, selected_instance->instance_ID,
			rfnameloc.string );
 	    gipAppendCmdLine( *cmd_line, tmpbuf );
	    gipAppendCmdLine( *cmd_line, " && " );
	    STprintf( tmpbuf, INGSTART_CMD, selected_instance->instance_ID );
 	    gipAppendCmdLine( *cmd_line, tmpbuf );
	    gipAppendCmdLine( *cmd_line, " || ( " );
	    STprintf( tmpbuf, INGSTOP_CMD, selected_instance->instance_ID );
 	    gipAppendCmdLine( *cmd_line, tmpbuf );
	    gipAppendCmdLine( *cmd_line, " ; exit 1 ) ) " );
	}
    }
    else /* unknown or NULL state */
	return( FAIL );

    /* add closing ')' */
    gipAppendCmdLine( *cmd_line, " ) " );

    /* finally add log files */
    STprintf( tmpbuf, " 1> %s", inst_stdout );
    gipAppendCmdLine( *cmd_line, tmpbuf );
    STprintf( tmpbuf, " 2> %s", inst_stderr );
    gipAppendCmdLine( *cmd_line, tmpbuf );

    return( OK );
    
}
	 
int
calc_cmd_len(size_t *cmdlen)
{
    int i; /* bit counter */   
    int	numpkgs = 0;
    int pkg_path_len;

    /* response file */
    *cmdlen = STlen( UMASK_CMD ) + 
		STlen( RF_VARIABLE ) +
		STlen( rfnameloc.string ) +
		7 ;

    if ( ug_mode & UM_INST ) /* new installation */
    {
	/* do we need to rename? */
 	if ( inst_state & UM_RENAME )
	    *cmdlen += get_rename_cmd_len( pkgs_to_install );

	/*
	** rpm command:
	** rpm -ivh --prefix $II_SYSTEM 
	*/
 	*cmdlen += STlen( RPM_INSTALL_CMD ) /* RPM command */
			+ STlen( RPM_PREFIX ) /* --prefix flag */
			+ STlen( iisystem.path ) /* value for II_SYSTEM */
			+ 5 ; /* space for extras */

	/* package list */
	*cmdlen += get_pkg_list_size( pkgs_to_install );

	/*
	** finally add doc if it's needed
	**
	** rpm -ivh <doc-pkg>
	*/
 	if ( pkgs_to_install & PKG_DOC )
	    *cmdlen += STlen( RPM_INSTALL_CMD )
			+ get_pkg_list_size( PKG_DOC );
			+ 5 ; /* extras */
    }
    else if ( ug_mode & UM_UPG ) /* upgrade */
    {
	/* do we need to rename? */
 	if ( selected_instance->action & UM_RENAME )
	    *cmdlen += get_rename_cmd_len( selected_instance->installed_pkgs );

	/*
	** rpm command:
	** rpm -e --justdb <pkglst> ; rpm -ivh --prefix $II_SYSTEM <pkg list>
	*/
 	*cmdlen += STlen( RPM_UPGRMV_CMD ) /* Remove command */
			+ STlen( RPM_INSTALL_CMD ) /* Install command */
			+ STlen( RPM_PREFIX ) /* --prefix flag */
			+ STlen( iisystem.path ) /* value for II_SYSTEM */
			+ 5 ; /* space for extras */

	/* package list (x2, once for each command) */
	*cmdlen += 2 * get_pkg_list_size( selected_instance->installed_pkgs );

	/*
	** finally add doc if it's needed and available
	**
	** rpm -ivh <doc-pkg>
	*/
 	if ( selected_instance->installed_pkgs & new_pkgs_info.pkgs & PKG_DOC )
	    *cmdlen += STlen( RPM_INSTALL_CMD )
			+ get_pkg_list_size( PKG_DOC );
			+ 5 ; /* extras */
    }
    else if ( ug_mode & UM_MOD ) /* modify */
    {
	/* do we need to rename? */
 	if ( selected_instance->action & UM_RENAME )
	    *cmdlen += get_rename_cmd_len( mod_pkgs_to_install );

	/* packages to remove */
	if ( mod_pkgs_to_remove != PKG_NONE )
	{
 	    *cmdlen += STlen( RPM_REMOVE_CMD ); /* RPM command */
	    *cmdlen += get_pkg_list_size( mod_pkgs_to_remove );

	    /*
	    ** finally remove doc if it needed
	    **
	    ** rpm -e <doc-pkg>
	    */
 	    if ( mod_pkgs_to_remove & PKG_DOC )
	        *cmdlen += STlen( RPM_REMOVE_CMD )
			+ get_pkg_list_size( PKG_DOC );
			+ 5 ; /* extras */
	}

	/* packages to add */
	if ( mod_pkgs_to_install != PKG_NONE )
	{
	    /* rpm command */
 	    *cmdlen += STlen( RPM_INSTALL_CMD ) /* RPM command */
			+ STlen( RPM_PREFIX ) /* --prefix flag */
			+ STlen( iisystem.path ) /* value for II_SYSTEM */
			+ 5 ; /* space for extras */

	    *cmdlen += get_pkg_list_size( mod_pkgs_to_install );

	    /*
	    ** add doc if it's needed and available
	    **
	    ** rpm -ivh <doc-pkg>
	    */
	    if ( mod_pkgs_to_install & new_pkgs_info.pkgs & PKG_DOC )
		*cmdlen += STlen( RPM_INSTALL_CMD )
			+ get_pkg_list_size( PKG_DOC );
			+ 5 ; /* extras */
	}

	/* finally, remove $II_SYSTEM/ingres/. if we've been asked to */
	if ( misc_ops & GIP_OP_REMOVE_ALL_FILES )
	{
	    auxloc	*locptr;

	    *cmdlen += STlen( RM_CMD ) /* rm -rf */
			+ STlen( selected_instance->inst_loc ) /* II_SYSTEM */
			+ 9; /* for /ingres/.  and spaces */

	    /* now add on space for any auxillery locations */
	    locptr = selected_instance->datalocs;
	    while ( locptr != NULL )
	    {
		*cmdlen += STlen( locptr->loc );
		locptr = locptr->next_loc ;
	    }
	    
	    locptr = selected_instance->loglocs;
	    while ( locptr != NULL )
	    {
		*cmdlen += STlen( locptr->loc );
		locptr = locptr->next_loc ;
	    }
	} 
    }
    else /* should never get here */
	return(FAIL);

    /*
    ** On success, start the installation, if that fails then make sure it's
    ** shut down.
    **
    ** configuration and startup commands:
    ** && ( /etc/init.d/ingresXX configure /tmp/iirfinstall.6854 &&
    **		 /etc/init.d/ingresII start || 
    **	( /etc/init.d/ingresII stop > /dev/null 2>&1 ; exit 1 ) ) 
    */
    *cmdlen += STlen( INGCONFIG_CMD )
		+ STlen( rfnameloc.string )
		+ STlen( INGSTART_CMD )
		+ STlen( INGSTOP_CMD )
		+ 40 ; /* misc characters etc. */

    /* log files */
    *cmdlen += STlen( inst_stdout ) + STlen( inst_stderr ) + 5 ;

    DBG_PRINT( "%d bytes have been allocated for the command line\n", *cmdlen );
    return(OK);
}

size_t
get_pkg_list_size( PKGLST pkglist )
{

    size_t	list_size;
    size_t	pkg_path_len;
    u_i4 	i = 1; 
    int 	numpkgs = 0;

    /* set the path length */
    pkg_path_len = STlen( new_pkgs_info.file_loc ) /* saveset location */
		 + STlen( new_pkgs_info.format ) /* package file sub dir */
		 + 3 ; /* space for extras */ 

    /* package list */
    while ( i <= ( MAXI4 ) )
    {
	if ( pkglist & i )
	    numpkgs++; 
	i <<= 1;
    }

    DBG_PRINT("%d packages in list\n", numpkgs);
    list_size = numpkgs * ( MAX_REL_LEN + 
			MAX_PKG_NAME_LEN +
    			MAX_VERS_LEN +
    			MAX_FORMAT_LEN +
			2 + /* installation ID (for reamed packages ) */
			5 + /* separators */
    			pkg_path_len + 1 );
    
    /* finally add on fudge factor for spaces and ;s etc */
    list_size += 5;

    return(list_size);
    
}

void
add_pkglst_to_inst_command_line( char **cmd_line, PKGLST pkglist, bool renamed )
{
    char	tmpbuf[MAX_LOC]; /* path buffer */
    i4		i; /* loop counter */

    /*
    ** Package location and names differ between renamed
    ** and regular packages.
    */
 
    /* add core package first (if we want it) as it's a bit different */
    if ( pkglist & PKG_CORE )
    {
	gipAppendCmdLine( *cmd_line, new_pkgs_info.file_loc );

	/* and file name of package */
	if ( renamed )
	{
	    STprintf( tmpbuf, "/%s-%s-%s.%s.%s ",
		new_pkgs_info.pkg_basename,
		ug_mode & UM_INST ? 
		instID : selected_instance->instance_ID,
		new_pkgs_info.version,
		new_pkgs_info.arch,
		new_pkgs_info.format);
	}
	else
	{
	    STprintf( tmpbuf, "/%s/%s-%s.%s.%s ",
		new_pkgs_info.format,
		new_pkgs_info.pkg_basename,
		new_pkgs_info.version,
		new_pkgs_info.arch,
		new_pkgs_info.format);
	}
	gipAppendCmdLine( *cmd_line, tmpbuf );
    }

    /* now do the rest, skipping core, and documentation */
    i = 1;
    while( packages[i] != NULL )
    {
	/* skip doc package, it's different */    
	if ( packages[i]->bit == PKG_DOC )
	{
	    i++;
	    continue;
	}

	if( pkglist & packages[i]->bit )
	{
	    /* add package location */
	    gipAppendCmdLine( *cmd_line, new_pkgs_info.file_loc );

	    if ( renamed )
	    {
		/* and file name of package */
		STprintf( tmpbuf, "/%s-%s-%s-%s.%s.%s ",
			new_pkgs_info.pkg_basename,
			packages[i]->pkg_name,
			ug_mode & UM_INST ? 
			instID : selected_instance->instance_ID,
			new_pkgs_info.version,
			new_pkgs_info.arch,
			new_pkgs_info.format);

	    }
	    else
	    {
		/* and file name of package */
		STprintf( tmpbuf, "/%s/%s-%s-%s.%s.%s ",
			new_pkgs_info.format,
			new_pkgs_info.pkg_basename,
			packages[i]->pkg_name,
			new_pkgs_info.version,
			new_pkgs_info.arch,
			new_pkgs_info.format);
	    }
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	}
	i++;
    }
}

void
add_pkglst_to_rmv_command_line( char **cmd_line, PKGLST pkglist )
{
    char	tmpbuf[MAX_LOC]; /* path buffer */
    i4		i; /* loop counter */
    bool	renamed = FALSE ; 

    /*
    ** add core package first (if we're removing it)
    ** as it's a bit different
    */
    if ( pkglist & PKG_CORE )
    {
	STprintf( tmpbuf, "%s-%s ",
			selected_instance->pkg_basename,
			selected_instance->version );
        gipAppendCmdLine( *cmd_line, tmpbuf );
    }

    /*
    ** Now do the rest, skipping core. The package location and
    ** names differ between renamed and regular packages.
    */
    renamed = selected_instance->action & UM_RENAME;
    DBG_PRINT("%s %s a renamed package\n",
		selected_instance->pkg_basename,
		renamed ? "is" : "is not" );
    i = 1;

    while( packages[i] != NULL )
    {
	/* skip doc package, it's different */    
	if ( packages[i]->bit == PKG_DOC )
	{
	    i++;
	    continue;
	}

	if( pkglist & packages[i]->bit )
	{
	    /* add package */
	    if ( renamed )
	    {
		char *ptr1, *ptr2;

		STcopy( selected_instance->pkg_basename, tmpbuf );
		/*
		** search backwards for the '-' so we don't get
		** caught out by ca-ingres packages
		*/
		ptr1 = STrindex( tmpbuf, "-", 0 );
		ptr2 = STrindex( selected_instance->pkg_basename, "-", 0 );
		STprintf( ++ptr1, "%s-%s-%s ",
				packages[i]->pkg_name, 
				++ptr2,
				selected_instance->version );
		
	    }
	    else
		STprintf( tmpbuf, "%s-%s-%s ",
			selected_instance->pkg_basename,
			packages[i]->pkg_name,
			selected_instance->version );

	    DBG_PRINT("Adding %s to removal command line\n", tmpbuf);
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	}
	i++;
    }
}

size_t
get_rename_cmd_len( PKGLST pkglst )
{
    size_t 	len ;

    len = STlen( rndirstr ) /* temporary output directory */
	  + STlen( new_pkgs_info.file_loc ) /* saveset location */
	  + STlen( RPM_RENAME_CMD ) /* rename command */
	    + 5; /* "cd " + spaces etc */

    len += get_pkg_list_size( pkglst );

    return( len );
}

STATUS
get_sys_mem(SIZE_TYPE *sysmemkb)
{
    char	meminfo[] = "/proc/meminfo";
# define MB_SIZE 29
    char	membuf[MB_SIZE];
    LOCATION	meminfo_loc;
    FILE	*meminfo_f;

    LOfroms(PATH|FILENAME, meminfo, &meminfo_loc);
    if (LOexist(&meminfo_loc) != OK)
	return FAIL;

    DBG_PRINT("Reading %s\n", meminfo);
    if (SIopen(&meminfo_loc, "r", &meminfo_f) != OK )
	return FAIL;

    while(SIgetrec(membuf, MB_SIZE, meminfo_f) != ENDFILE)
    {
	char *ptr;
	/*
        ** line we're looking for looks like this:
        ** MemTotal:         510264 kB
	*/
	if ( (ptr = STstrindex(membuf, "MemTotal:", 0, TRUE)) != NULL )
	{
	    char *memval;

	    /* find the value */
	    while(! CMdigit(ptr))
		ptr++;
	    memval=ptr;

	    /* find the end */
	    while(CMdigit(ptr))
		ptr++;
	    *ptr='\0';

	    /* pass it back */
	    *sysmemkb=atoi(memval);
	    DBG_PRINT("System memory detected as %sKb, returned as %dKb\n",
			memval, *sysmemkb);
	}
    }
    SIclose(meminfo_f);
    return(OK);
}
    
STATUS
init_ivw_cfg()
{
    GtkWidget *widget_ptr;
    i4	i = 0;
    char field_buf[100];
    SIZE_TYPE	sysmemkb;

    DBG_PRINT("Checking system memory\n");
    if (get_sys_mem(&sysmemkb) != OK)
	sysmemkb=0;

    /* initialize vectorwise config */
    while (vw_cfg_info[i])
    {
	vw_cfg *param=vw_cfg_info[i];

	/*
	**  if we can, use the amount of system memory to set default for
	** memory parameters
        */
	if ( sysmemkb > 0 && param->bit &
		 (GIP_VWCFG_MAX_MEMORY|GIP_VWCFG_BUFFERPOOL) )
	{
	    if (param->bit & GIP_VWCFG_MAX_MEMORY)
	    {
		/* default to 50% of system memory */
		param->dfval = sysmemkb/(1024 * 2);
		param->dfunit = VWMB;
	    }
	    if (param->bit & GIP_VWCFG_BUFFERPOOL)
	    {
		/* default to 25% of system memory */
		param->dfval = sysmemkb/(1024 * 4);
		param->dfunit = VWMB;
	    }

	    /* find a sensible unit */
	    if (param->dfval > 1024)
	    {
		param->dfval = param->dfval/1024;
	 	param->dfunit = VWGB;
	    }
	    param->value=param->dfval;
	    param->unit=param->dfunit;
	   
	}
		
	/* set field value */
	STprintf(field_buf, "%s_val", param->field_prefix);
	widget_ptr=lookup_widget(IngresInstall, field_buf);
	DBG_PRINT("Setting %s to %d\n", field_buf, param->value);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget_ptr), param->value);

	/* set units */
	if (param->unit != VWNOUNIT)
	{
	    STprintf(field_buf, "%s_unit", param->field_prefix);
	    DBG_PRINT("Setting default unit for %s\n", field_buf);
	    widget_ptr=lookup_widget(IngresInstall, field_buf);
	    gtk_combo_box_set_active(GTK_COMBO_BOX(widget_ptr), param->unit);
	}
	i++;
    }
}

static STATUS
gipAppendCmdLine( char *cmdbuff, char *newcmd )
{
    STATUS status = OK;

    if ( STlen(newcmd) + STlen(cmdbuff) + 1 > cmdlen )
    {
	/*
	** we're going to blow the command line buffer so error
	** and exit cleanly
	*/
# define ERROR_CMDLINE_BUFF_OVERRUN "FATAL ERROR: Command line exceeds allocated space. Aborting"
	gdk_threads_enter();
	popup_error_box( ERROR_CMDLINE_BUFF_OVERRUN );
	gdk_threads_leave();
	gtk_main_quit();
    }
    else
        STcat(cmdbuff, newcmd);

    return(status);
}
# else /* xCL_GTK_EXISTS & xCL_RPM_EXISTS */

/*
** Dummy main() to quiet link errors on platforms which don't support 
** GTK & RPM
*/
i4
main()
{
    return( FAIL );
}

# endif /* xCL_GTK_EXISTS & xCL_RPM_EXISTS */
