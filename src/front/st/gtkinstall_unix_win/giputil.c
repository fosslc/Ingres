/*
** Copyright 2006, 2010 Ingres Corporation. All rights reserved 
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

# include <compat.h>
# include <gl.h>
# include <gv.h>
# include <cm.h>
# include <st.h>
# include <pc.h>
# include <nm.h>
# include <unistd.h>

# ifdef xCL_GTK_EXISTS

# include <gtk/gtk.h>

# include <gipsup.h>
# include <gipcb.h>
# include <gip.h>
# include <gipdata.h>
# include <giputil.h>

/*
** Name: giputil.c
**
** Description:
**
**      Module for useful utility functions used by GTK GUI interface to both
**      the Linux GUI installer and the Ingres Package manager.
**
** History:
**
**	09-Oct-2006 (hanje04)
**	    SIR 116877
**          Created.
**	25-Oct-2006 (hanje04)
**	    SIR 116877
**	    Add missing response file parameters.
**	    Replace Unix system calls with CL functions.
**	    Make sure buttons are enabled and dissabled correctly in each of
**	    the stages.
**	    Include "upgrade user databases" choice in upgrade summary
**	    Add rename functionality for upgrades.
**	26-Oct-2006 (hanje04)
**	    SIR 116877
**	    Hide all buttons during upgrade/modify execution
**	    Add code remove all files under II_SYSTEM if requested.
**	    Also show we're removing files in red in the modify summary.
**	06-Nov-2006 (hanje04)
**	    SIR 116877
**	    Make sure code only gets built on platforms that can build it.
**	17-Nov-2006 (hanje04)
**	    BUG 117149
**	    SD 111992
**	    If install fails, grey "Finish" button and ungrey "Quit".
**	21-Nov-2006 (hanje04)
**	    BUG 117166
**	    SD 112150
**	    Set umask to 0022 in install shell to ensure directory permissions
**	    are set correctly on creation by RPM. 
**	24-Nov-2006 (hanje04)
**	    SIR 116877
**	    Update increment_wizard_progress() to validate entry field in 
**	    NI_INSTID, NI_DBLOC, NI_LOGLOC & NI_MISC panes such that the
**	    next buttion is greyed out if the fields do not contain valid
**	    info.
**	    Make "local popup" modal and remove it from the desktop pager and
**	    task bar.
**	01-Dec-2006 (hanje04)
**	    Write date type alias value out to response file.
**	    Make sure all labels are set correctly for modify/upgrade mode.
**	    Show correct buttons on failure pane.
**	    Make sure ALL output is shown in failure pain when an error 
**	    occurs.
**	    Add border to locale popup and add icon to it.
**	    Make sure we use version info when removing packages.
**	07-Dec-2006 (hanje04)
**	    BUG 117278
**	    SD 112944
**	    Only display DBMS config parameters (II_DATABASE etc.) when DBMS
**	    package is selected for installation.
**	    Only add DBMS repsonse file parameters to response file if
**	    package has been selected.
**	    Also display DBMS config parameters in Modify summary if DBMS
**	    package is being added to an existing instance.
**	    Add DBMS response file parameters to respsone file for Modify if
**	    package is being added.
**	14-Dec-2006 (hanje04)
**          BUG 117330
**          Disable installation of doc package with GUI to avoid bug
**          117330 for beta. Actually problem will be addressed for refresh
**	19-Dec-2006 (hanje04)
**	    Bug 117353 
**	    SD 113656
**	    GUI stop response file generation failing because II_SYSTEM is
**	    added twice by gen_install_response_file().
**	15-Feb-2007 (hanje04)
**	    BUG 117700
**	    SD 113367
**	    Add commands to start instance after installation has completed
**	    successfully. For modifications, only start the instance if we
**	    are not removing everything.
**      02-Feb-2007 (hanje04)
**          SIR 116911
**          Move the following functions to front!st!gtkinstall_unix
**          gipmain.c as they cause compilations problems when building
**          iipackman on Window.
**
**              get_rename_cmd_len()
**              add_pkglst_to_rmv_command_line()
**              get_pkg_list_size()
**              calc_cmd_len()
**              generate_command_line()
**              add_pkglst_to_inst_command_line()
**
**	    Also replace PATH_MAX with MAX_LOC.
**	22-Feb-2007 (hanje04)
**	    SIR 116911
**	    Add GIPfullPath() as a cross platform way of validating
**	    full paths. LOisfull() only works for the platform the 
**	    binary is built on.
**	27-Feb-2007 (hanje04)
**	    BUG 117330
**	    Re-enable documentation check box for non renamed installs.
**	28-Feb-2007 (hanje04)
**	    SD 115861
**	    BUG 117801
**	    Re-do location validation for database and log file locations.
**	    All locations need to be validated everything any location is 
**	    updated. If any are found to be invalid both "next" and "back"
**	    buttons are now "greyed out" and the problem must be resolved
**	    before the user can continue.
**	    New function GIPvalidateLocs() has been added to do this.
**	01-Mar-2007 (hanje04)
**	    BUG 117812
**	    Replace text to use more generic tense so it makes sense as both
**	    a pre and post install summary.
**	12-Mar-2007 (hanje04)
**	    Correct typo in shutdown message.
**	14-Mar-2007 (hanje04)
**	    SD 116273
**	    BUG 117923
**	    Fix method for adding misc. options to response file. Need to
**	    explicitly set values whether ON or OFF.
**	26-Apr-2007 (hanje04)
**	    SD 116488
**	    BUG 118206
**	    Explicitly title pop-up windows so they always have one. Some
**	    window manager do not honor the implicit ones for message
**	    dialogs.
**	11-May-2007 (hanje04)
**	    BUG 118308
**	    Add demodb creation to summary and remove unecissary blank lines
**	    basic summary so it doesn't scroll.
**	15-Aug-2008 (hanje04)
**	    BUG 120782, SD 130225
**	    Update is_renamed() not to use CMupper(ptr++) as it causes problems
**	    when the macro is expanded under UTF8. (ptr++ is referenced multiple
**	    times, resulting in too many increments).
**	08-Oct-2008 (hanje04)
**	    Bug 121004
**	    SD 130670
**	    Add GIPValidateLocaleSettings() to check charset and timezone
**	    values. Also make sure values are validated when entering
**	    NI_LOCALE
**      10-Oct-2008 (hanje04)
**          Bug 120984
**          SD 131178
**          Add GIPValidatePkgName() to check package file names
**	30-Jan-2009 (hanje04)
**	    Bug 121580
**	    SD 133587
**	    Remove "Start Ingres with computer" from summary when DBMS is
**	    add during a modify
**	02-Jul-2009 (hanje04)
**	    Bug 122227
**	    SD 137277
**	    Pass II_UPGRADE_USER_DB as param name to rfapi not
**	    GIP_OP_UPGRADE_USER_DB. They are NOT the same thing.
**	13-Jul-2009 (hanje04)
**	    SIR 122309
**	    Update summary to include configuration type selected.
**	    Add II_CONFIG_TYPE to response file based on value set in GUI
**	10-Sep-2009 (hanje04)
**	    BUG 122571
**	    Add GIPAddDBLoc() and GIPAddLogLoc() for added aux locations to
**	    instance structure.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of unistd.h to eliminate gcc 4.3 warnings.
**	10-May-2010 (hanje04)
**	    SIR 123791
**	    Add "check" logic for new License acceptance dialog
**	    Add GIPdoValidation() to reduce code duplication for database
**	    location validation.
**	    Add logic for II_VWDATA location. 
**	    instmode now bit mask for vectorwise support, update checks 
**	    propriately.
**	    Remove unwanted misc_ops entries and add VW specific parameters
**	    to/from summary for IVW_INSTALL mode.
**	    Add VW config parameters to response file.
**	26-May-2010 (hanje04)
**	    SIR 123791
**	    Make sure next button is initially greyed out for lic dialog in
**	    "new install" mode too.
**	    Don't set II_CONFIG_TYPE for IVW_INSTALL response file or display
**	    in the summary.
**	    IVW blocksize and groupsize need to be rounded to the next 
**	    power of 2, add GIPnextPow2() to do this.
**	08-Jul-2010 (hanje04)
**	    SD 145482
**	    BUG 124054
**	    Fix exclusion of II_VWDATA from pre/post install summaries for
**	    non-Vectorwise installs.
**	
*/

const char instance_name_descrip[] = { "Ingres supports multiple instances installed on the same machine.\nEach instance is uniquely identified by an instance name.\n "};

/* static function prototypes */
static II_RFAPI_STATUS gen_install_response_file( II_RFAPI_STRING rflocstring );
static II_RFAPI_STATUS gen_upgrade_response_file( II_RFAPI_STRING rflocstring );
static II_RFAPI_STATUS gen_modify_response_file( II_RFAPI_STRING rflocstring );
static void gen_rf_name(void);

void
inst_pkg_sel( PKGIDX pkgidx )
{
    PKGLST	pkgbit;

    /* get the bit we want to manipulate */
    pkgbit = packages[pkgidx]->bit;

    /* add it to the install list */
    pkgs_to_install |=  pkgbit;

    /* make sure dependencies get installed too */
    pkgs_to_install |=  packages[pkgidx]->prereqs;

    /* set the buttons */
    set_inst_pkg_check_buttons();
}

void
inst_pkg_rmv( PKGIDX pkgidx )
{
    PKGLST	pkgbit;
    i4		i = 0;

    /* get the bit we want to manipulate */
    pkgbit = packages[pkgidx]->bit;

    /* remove it from the install list */
    pkgs_to_install &=  ~pkgbit;

    /*
    ** as we're removing a package we need to make sure
    ** it's dependents are also removed
    */
    while ( packages[i] != NULL )
    {
	if ( packages[i]->prereqs & pkgbit )
	    pkgs_to_install &= ~packages[i]->bit;
	i++;
    }

    /* set the buttons */
    set_inst_pkg_check_buttons();
}

   
void
ug_pkg_sel( PKGIDX pkgidx )
{
    PKGLST	pkgbit;

    /* get the bit we want to manipulate */
    pkgbit = packages[pkgidx]->bit;

    /* if it is not already installed install it */
    mod_pkgs_to_install |= ( pkgbit & 
				~selected_instance->installed_pkgs );

    /* if it's installed remove it from the 'to remove' list */
    mod_pkgs_to_remove &= ~( pkgbit & 
				selected_instance->installed_pkgs );

    /* make sure dependencies that aren't installed, get installed */
    mod_pkgs_to_install |= ( packages[pkgidx]->prereqs & 
				~selected_instance->installed_pkgs );

    /* make sure installed dependencies are not removed */
    mod_pkgs_to_remove&=~( packages[pkgidx]->prereqs & 
				selected_instance->installed_pkgs );

    /*
    ** The buttons should reflect the final state after install as completed.
    ** This will be:
    */
    					/* installed packages */
    set_mod_pkg_check_buttons( (selected_instance->installed_pkgs|
					/* + packages being installed */
					mod_pkgs_to_install)&
					/* - packages being removed */
					~mod_pkgs_to_remove);
}

void
ug_pkg_rmv( PKGIDX pkgidx )
{
    PKGLST	pkgbit;
    i4	i=0;

    /* get the bit we want to manipulate */
    pkgbit = packages[pkgidx]->bit;

    /* if it is already installed, uninstall it. */
    mod_pkgs_to_remove |= ( pkgbit & 
				selected_instance->installed_pkgs );

    /* if it's not installed remove it from the 'to install' list */
    mod_pkgs_to_install &= ~( pkgbit & 
				~selected_instance->installed_pkgs );
    
    /*
    ** as we're removing a package we need to make sure
    ** it's dependencies are also removed
    */
    while ( packages[i] != NULL )
    {
	if ( packages[i]->prereqs & pkgbit )
	{
	    /* if the pre-req is installed add it to the 'to remove' list */
	    mod_pkgs_to_remove |= ( packages[i]->bit &
					selected_instance->installed_pkgs );

	    /* if it's not installed remove it from the 'to install' list */
	    mod_pkgs_to_install &= ~( packages[i]->bit &
					~selected_instance->installed_pkgs );
	}
	i++;
    }

    /*
    ** The buttons should reflect the final state after install as completed.
    ** This will be:
    */
    					/* installed packages */
    set_mod_pkg_check_buttons( (selected_instance->installed_pkgs|
					/* + packages being installed */
					mod_pkgs_to_install)&
					/* - packages being removed */
					~mod_pkgs_to_remove);

}

void
create_summary_tags(GtkTextBuffer *buffer)
{
	/*
	** create tags used to format text displayed by in the  
	** summary window
	*/
	
	
	gtk_text_buffer_create_tag (buffer, "heading",
			      "weight", PANGO_WEIGHT_BOLD,
			      "size", 12 * PANGO_SCALE,
			      NULL);
	gtk_text_buffer_create_tag (buffer, "bold",
			      "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag (buffer, "monospace",
			      "family", "monospace", NULL);
	gtk_text_buffer_create_tag (buffer, "italic",
			      "style", PANGO_STYLE_ITALIC, NULL);
	gtk_text_buffer_create_tag (buffer, "warning",
			      "foreground", "red", NULL);
}

void
write_installation_summary(GtkTextBuffer *buffer)
{
    GtkTextIter	iter, start, end;
    gchar	tmpbuf[50];
    gint i;
	
    /* Clear out the buffer so we can start from scratch */
    gtk_text_buffer_get_start_iter  (buffer, &start);
    gtk_text_buffer_get_end_iter    (buffer, &end);
    gtk_text_buffer_delete          (buffer, &start, &end);
	
    /* 
    ** find the start of the buffer, then insert text moving
    ** iterater as we go.
    */	
    /*
    ** NOTE: ALL THESE STRING NEEDS TO BE BROKEN OUT SO THEY
    ** CAN BE EASILY TRANSLATED!!!!!
    */
    /* LOOK AT UNICODE WORD WRAPPING TOO */
    gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
    gtk_text_buffer_insert_with_tags_by_name (buffer,
						&iter,
						instmode >= RFGEN_LNX ?
						"Response File Summary" :
						"Installation Summary",
						-1,
						"heading",
						NULL);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

    /* package info */
    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						"Components selected:\n",
						 -1,
						"bold",
						NULL);
    for ( i = 0 ; packages[i] ; i++ )
    {
	/* if the package is selected and not hidden, display it */
	if ( (pkgs_to_install & packages[i]->bit) &&
		packages[i]->hidden == FALSE )
	{
	    STprintf( tmpbuf, "\t%s\n", packages[i]->display_name );
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						tmpbuf,
						-1,
						"monospace",
						NULL);
	}
    }
	
    /* installation location */
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer,
				&iter, 
				"Installation Location (II_SYSTEM):\n\t",
				-1,
				"bold",
				NULL);
    gtk_text_buffer_insert_with_tags_by_name(buffer,
					&iter,
					iisystem.path, 
					-1,
					"monospace",
					NULL);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
    if ( instmode <= RFGEN_LNX )
    {
	gtk_text_buffer_insert(buffer,
				&iter,
				"The installation log file location:\n\t",
				-1);
									
	/* generate the location of the installation log and display it */
	STprintf( tmpbuf,
		"%s/%s",
		iisystem.path,
		"ingres/files/install.log\n\n" );
	gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						tmpbuf,
						-1,
    						"monospace",
						NULL);
    }

    if ( ! instmode & IVW_INSTALL )
    {
	/* Configuration type */
	gtk_text_buffer_insert_with_tags_by_name(buffer,
					&iter,
					"Configuration Type:\n\t",
					-1,
					"bold",
					NULL);
	gtk_text_buffer_insert_with_tags_by_name(buffer,
					&iter,
					ing_config_type.descrip[ing_config_type.val_idx],
						-1,
						"monospace",
						NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
    }
	
    gtk_text_buffer_insert_with_tags_by_name(buffer,
					&iter,
					"Instance Name:\n\t",
					-1,
					"bold",
					NULL);
	
    /* Construct instance name from instID */
    STprintf( tmpbuf, "Ingres %s", instID );
    gtk_text_buffer_insert_with_tags_by_name(buffer,
					&iter,
					tmpbuf, 
					-1,
					"monospace",
					NULL);

    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer,
					&iter, 
					instance_name_descrip,
					-1, 
					"italic",
					NULL );
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	


    /* Give more of a summary for the advanced install */
    if ( ! (instmode & BASIC_INSTALL) )
    {
	if ( pkgs_to_install & PKG_DBMS )
	{
	    /* DB file locations */
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						"Database file locations:\n",
						-1,
						"bold",
						NULL);	

	    for ( i = 1 ; dblocations[i] ; i++ )
	    {
		if ( i == INST_II_VWDATA &&  ~instmode&IVW_INSTALL )
		    continue; /* skip if it's not Vectorwise */

		STprintf( tmpbuf,
			"%s (%s): ",
			dblocations[i]->name, 
			dblocations[i]->symbol );
		gtk_text_buffer_insert(buffer,
				&iter,
				tmpbuf,
				-1);
		gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter, 
						dblocations[i]->path,
						-1,
						"monospace",
						NULL);
		gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	    }

	    /* Vectorwise Config */
	    if ( instmode & IVW_INSTALL )
	    {
		i=0;

		gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						"\nVectorWise Configuration:\n",
						-1,
						"bold",
						NULL);	
		while(vw_cfg_info[i])
		{
		    vw_cfg *param=vw_cfg_info[i];
		    STprintf( tmpbuf, "%s: ", param->descrip);
	            gtk_text_buffer_insert(buffer, &iter, tmpbuf, -1);
		    STprintf( tmpbuf, "%d%cB\n",
			     param->value, vw_cfg_units[param->unit] );
	            gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						tmpbuf,
						-1,
						"monospace",
						NULL);
		    i++;
		}
		gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	    }

	    /* Tx log info */
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						"\nTrasaction log(s):\n",
						-1,
						"bold",
						NULL);	
	    gtk_text_buffer_insert(buffer, &iter, "Log file size: ", -1);
	    STprintf( tmpbuf, "%dMB\n", txlog_info.size );
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						tmpbuf,
						-1,
						"monospace",
						NULL);
	    STprintf( tmpbuf, "%s: ", txlog_info.log_loc.name );
	    gtk_text_buffer_insert(buffer, &iter, tmpbuf, -1);
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
	    					txlog_info.log_loc.path,
						-1,
						"monospace",
						NULL);
	    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		
	    /* dual logging */
	    if ( txlog_info.duallog )
	    {
		gtk_text_buffer_insert(buffer,
					&iter,
					"Dual logging enabled\n",
					-1 );
		STprintf( tmpbuf, "%s: ", txlog_info.dual_loc.name );
		gtk_text_buffer_insert(buffer, &iter, tmpbuf, -1);
		gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter, 
						txlog_info.dual_loc.path,
						-1,
						"monospace",
						NULL);
		gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	    }
	    else
		gtk_text_buffer_insert(buffer,
				&iter,
				"Dual logging disabled\n",
				-1 );

	    if ( ! (instmode & IVW_INSTALL) )
	    {
	        gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						"\nDatabase server options:\n",
						-1,
						"bold",
						NULL);	
	        /* SQL92 Compliance */
	        gtk_text_buffer_insert(buffer,
				&iter,
				"Strict SQL-92 Compliance: ",
				-1);
	        gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						misc_ops & GIP_OP_SQL92 ?
						"Yes" : "No" ,
						-1,
						"monospace",
						NULL);
	        gtk_text_buffer_insert(buffer, &iter, "\n", -1);

	        /* Date Type Alias */
	        gtk_text_buffer_insert(buffer,
	    			&iter,
				"\"DATE\" type alias: ",
				-1);
	        gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						date_type_alias.values[date_type_alias.val_idx],
						-1,
						"monospace",
						NULL);
	        gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	    }
        }
		
	/* timezone info */	
	gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						"\nLocale Settings\n",
						-1,
						"bold",
						NULL);	
	gtk_text_buffer_insert(buffer, &iter, "Timezone: ", -1);
	gtk_text_buffer_insert_with_tags_by_name(buffer,
							&iter, 
							locale_info.timezone,
							-1,
							"monospace",
							NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	/* character set */
	gtk_text_buffer_insert(buffer, &iter, "Character Set: ", -1);
	gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter, 
						locale_info.charset,
						-1,
						"monospace",
						NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		
	/* installation owner */
	gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						"\nInstallation Owner:\n",
						-1,
						"bold",
						NULL);	
	gtk_text_buffer_insert(buffer, &iter, "Username: ", -1);
	gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter, 
						userid,
						-1,
						"monospace",
						NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	gtk_text_buffer_insert(buffer, &iter, "User Group: ", -1);
	gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter, 
						groupid,
						-1,
						"monospace",
						NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		
	/* misc config */
	gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						"\nOther Options\n",
						-1,
						"bold",
						NULL);	
	gtk_text_buffer_insert(buffer,
				&iter,
				"Start instance with computer: ",
				-1);
	gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter, 
						misc_ops & GIP_OP_START_ON_BOOT ?
						"Yes" : "No",
						-1,
						"monospace",
						NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);

	if ( ! instmode & IVW_INSTALL )
	{
	    gtk_text_buffer_insert(buffer,
				&iter,
				"Create a demo database: ",
				-1);
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						misc_ops & GIP_OP_INSTALL_DEMO ?
						"Yes" : "No" ,
						-1,
						"monospace",
						NULL);
	    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
# ifdef CREATE_DT_FOLDER
	    gtk_text_buffer_insert(buffer,
				&iter,
				"Create Ingres folder on Desktop: ",
				-1);
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter, 
						misc_ops & GIP_OP_DESKTOP_FOLDER ?
						"Yes" : "No" ,
						-1,
						"monospace",
						NULL);
	    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
# endif
        }
   }
		
}

GtkTreeModel *
initialize_timezones()
{
    GtkTreeIter	reg_iter, tz_iter;
    GtkTreeStore *timezone_store;
    gint i;
	
    /* create a tree store to hold the region/timezone info */
    timezone_store=gtk_tree_store_new(1, G_TYPE_STRING);
	
    /* Load the values from the timezones array into the tree store */
    for ( i = 0 ; timezones[i].region || timezones[i].timezone ; i++ )
    {
	if ( timezones[i].region )
	{
	    gtk_tree_store_append (timezone_store, &reg_iter, NULL);
	    gtk_tree_store_set (timezone_store,
				&reg_iter,
				0, 
				timezones[i].region,
				-1);
	}
	else if ( timezones[i].timezone )
	{
	    gtk_tree_store_append (timezone_store, &tz_iter, &reg_iter);
	    gtk_tree_store_set (timezone_store,
				&tz_iter,
				0, 
				timezones[i].timezone,
				-1);

	    /* save the iterater of the default timezone */
	    if ( ! STcompare( timezones[i].timezone, locale_info.timezone ) )
					timezone_iter=gtk_tree_iter_copy(
								&tz_iter );
	}
    }
	
    /* Return the pointer to the store */
    return GTK_TREE_MODEL(timezone_store);
}

GtkTreeModel *
initialize_charsets( CHARSET *charsets )
{
    GtkTreeIter	reg_iter, cs_iter;
    GtkTreeStore *charset_store;
    gint i;
	
    /* create a tree store to hold the region/charset info */
    charset_store=gtk_tree_store_new(1, G_TYPE_STRING);
	
    /* Load the values from the charset array into the tree store */
    for ( i = 0 ; charsets[i].region || charsets[i].charset ; i++ )
    {
	if ( charsets[i].region )
	{
	    gtk_tree_store_append (charset_store, &reg_iter, NULL);
	    gtk_tree_store_set (charset_store,
				&reg_iter,
				0, 
				charsets[i].region,
				-1);
	}
	else if ( charsets[i].charset )
	{
	    gtk_tree_store_append (charset_store, &cs_iter, &reg_iter );

	    gtk_tree_store_set (charset_store,
				&cs_iter,
				0, 
				charsets[i].charset,
				-1);

	    /* save the iterater of the default charset */
	    if ( ! STcompare( charsets[i].charset, locale_info.charset ) )
					charset_iter=gtk_tree_iter_copy(
								&cs_iter );
	}
    }
	
    /* Return the pointer to the store */
    return GTK_TREE_MODEL(charset_store);
}

II_RFAPI_STATUS
generate_response_file()
{
    II_RFAPI_STATUS	rfstat; /* response file API status info */
    STATUS		clstat; /* cl return codes */

    /* generate file name */
    gen_rf_name();

    /* remove existing response file */
    if ( LOexist( &rfnameloc ) == OK &&
    	LOdelete( &rfnameloc ) != OK )
	return( II_RF_ST_IO_ERROR );

    if ( ug_mode & UM_INST )
	rfstat = gen_install_response_file( (II_RFAPI_STRING)rfnameloc.string );
    else if ( ug_mode & UM_UPG )
	rfstat = gen_upgrade_response_file( (II_RFAPI_STRING)rfnameloc.string );
    else if ( ug_mode & UM_MOD )
	rfstat = gen_modify_response_file( (II_RFAPI_STRING)rfnameloc.string );
    else
	return( II_RF_ST_FAIL );

    return( rfstat );
}

static void
gen_rf_name(void)
{
    LOCATION		tmploc; /* temporary LOCATION  storage */
    char		*stptr; /* misc string pointer */
    PID			pid; /* process ID of current process */
    STATUS		clstat; /* cl return codes */

    /* find a temporary dir */
    NMgtAt( TMPVAR, &stptr );
    if ( stptr == NULL || *stptr == '\0' )
    {
	stptr = TMPDIR ;
    } 
    STcopy( stptr, rfdirstr );
    LOfroms( PATH, rfdirstr, &rfnameloc );

    /* generate response file name from current pid */
    PCpid( &pid );
    STprintf( rfnamestr, "%s.%d", RESPONSE_FILE_NAME, pid );
    LOfroms( FILENAME, rfnamestr, &tmploc );

    /* combine path and filename */
    LOstfile( &tmploc, &rfnameloc );

    DBG_PRINT("Response file name is %s\n", rfnameloc.string );
}

static II_RFAPI_STATUS
gen_install_response_file( II_RFAPI_STRING rflocstring )
{
    II_RFAPI_HANDLE	rfhandle; /* handle for respone file API */
    II_RFAPI_PARAM	param; /* RF API parameter info */
    char		tmpbuf[MAX_FNAME]; /* misc string buffer */
    II_RFAPI_STATUS	rfstat; /* response file API status info */
    i4			i = 0; /* misc counter */

    /* initialise response file handle */
    rfstat = IIrfapi_initNew( &rfhandle,
				dep_platform,
				(II_RFAPI_STRING)rfnameloc.string,
				II_RF_OP_CLEANUP_AFTER_WRITE );

    if ( rfstat != II_RF_ST_OK )
    {
	if ( &rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
	return( rfstat );
    }

    while ( packages[i] != NULL )
    {
	if ( pkgs_to_install & packages[i]->bit )
	{
	    param.name = packages[i]->rfapi_name;
	    param.value = "YES"; 
	    param.vlen = STlen("YES");
	    rfstat = IIrfapi_addParam( &rfhandle, &param );

	    if ( rfstat != II_RF_ST_OK )
	    {
		DBG_PRINT(
		    "Failed adding %s package to response file with error:\n %s\n"
			, packages[i]->pkg_name,
			IIrfapi_errString( rfstat ) );
		if ( rfhandle != NULL )
		    IIrfapi_cleanup( &rfhandle );
		return( rfstat );
	    }
	}
	i++;
    }
   
    /* II_SYSTEM */
    param.name = II_RF_SYSTEM;
    param.value = iisystem.path;
    param.vlen = STlen(iisystem.path);
    rfstat = IIrfapi_addParam( &rfhandle, &param );
    if ( rfstat != II_RF_ST_OK )
    {
	DBG_PRINT(
	    "Failed adding II_SYSTEM to response file with error:\n %s\n",
			IIrfapi_errString( rfstat ) );
	if ( rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
	return( rfstat );
    }
    
    /* installation ID */
    param.name = II_INSTALLATION;
    param.value = instID;
    param.vlen = STlen("instID");
    rfstat = IIrfapi_addParam( &rfhandle, &param );
    if ( rfstat != II_RF_ST_OK )
    {
	DBG_PRINT(
	    "Failed adding II_INSTALLATION to response file with error:\n %s\n",
			IIrfapi_errString( rfstat ) );
	if ( rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
	return( rfstat );
    }

    /* Config Type */
    if ( ! instmode & IVW_INSTALL )
    {
	param.name = II_CONFIG_TYPE;
	param.value = 
	    (II_RFAPI_STRING)ing_config_type.values[ing_config_type.val_idx];
	param.vlen = STlen(param.value);
	rfstat = IIrfapi_addParam( &rfhandle, &param );
	if ( rfstat != II_RF_ST_OK )
	{
	    DBG_PRINT(
	      "Failed adding II_CONFIG_TYPE to response file with error:\n %s\n",
			IIrfapi_errString( rfstat ) );
	    if ( rfhandle != NULL )
		IIrfapi_cleanup( &rfhandle );
	    return( rfstat );
	}
    }

    /* locale info */
    /* Character set */
    param.name = II_CHARSET;
    param.value = locale_info.charset;
    param.vlen = STlen( locale_info.charset );
    rfstat = IIrfapi_addParam( &rfhandle, &param );

    if ( rfstat != II_RF_ST_OK )
    {
	DBG_PRINT(
	    "Failed adding II_CHARSET to response file with error:\n %s\n",
			IIrfapi_errString( rfstat ) );
	if ( rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
	return( rfstat );
    }

    /* Timezone */
    param.name = II_TIMEZONE_NAME;
    param.value = locale_info.timezone;
    param.vlen = STlen( locale_info.timezone );
    rfstat = IIrfapi_addParam( &rfhandle, &param );

    if ( rfstat != II_RF_ST_OK )
    {
	DBG_PRINT(
	    "Failed adding II_TIMEZONE_NAME to response file with error:\n %s\n",
			IIrfapi_errString( rfstat ) );
	if ( rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
	return( rfstat );
    }

    if ( pkgs_to_install & PKG_DBMS )
    {
	/* Add values for DB locations, skipping II_SYSTEM */
	i = 1;
	while ( dblocations[i] != NULL )
	{
	    if ( i == INST_II_VWDATA &&  ~instmode&IVW_INSTALL )
	    {
		i++;
		continue; /* skip if it's not Vectorwise */
	    }

	    param.name = dblocations[i]->rfapi_name; /* variable name */
	    param.value = dblocations[i]->path; /* value */
	    param.vlen = STlen( dblocations[i]->path ); /* size */	
	    rfstat = IIrfapi_addParam( &rfhandle, &param );

	    if ( rfstat != II_RF_ST_OK )
	    {
		DBG_PRINT(
		    "Failed adding %s to response file with error:\n %s\n",
			dblocations[i]->name,
			IIrfapi_errString( rfstat ) );
		if ( rfhandle != NULL )
		    IIrfapi_cleanup( &rfhandle );
		return( rfstat );
	    }

	    i++;
	}

	/* Vectorwise config */
	if ( instmode & IVW_INSTALL )
	{
	    i=0;
	    while(vw_cfg_info[i])
	    {
		vw_cfg *vwparam=vw_cfg_info[i];
		param.name=vwparam->rfapi_name;
		param.value = STprintf( tmpbuf, "%d%c",
				 vwparam->value, vw_cfg_units[vwparam->unit] );
		param.vlen = STlen(tmpbuf);
		rfstat = IIrfapi_addParam( &rfhandle, &param );

		if ( rfstat != II_RF_ST_OK )
		{
		    DBG_PRINT(
			"Failed adding '%s' to response file.\nError: %s\n",
			    vwparam->descrip,
			    IIrfapi_errString( rfstat ) );
		    if ( rfhandle != NULL )
			IIrfapi_cleanup( &rfhandle );
		    return( rfstat );
		}

		i++;
	    }
	}

	/* Transaction Log */
	/* size */
	param.name = II_LOG_FILE_SIZE_MB;
	param.value = STprintf( tmpbuf, "%d", txlog_info.size );
	param.vlen = STlen( tmpbuf );
	rfstat = IIrfapi_addParam( &rfhandle, &param );

	if ( rfstat != II_RF_ST_OK )
	{
	    DBG_PRINT(
	    "Failed adding II_LOG_FILE_SIZE_MB to response file with error:\n %s\n",
			IIrfapi_errString( rfstat ) );
	    if ( rfhandle != NULL )
		IIrfapi_cleanup( &rfhandle );
	    return( rfstat );
	}

	/* location */
	param.name = txlog_info.log_loc.rfapi_name;
	param.value = txlog_info.log_loc.path;
	param.vlen = STlen( txlog_info.log_loc.path );
	rfstat = IIrfapi_addParam( &rfhandle, &param );

	if ( rfstat != II_RF_ST_OK )
	{
	    DBG_PRINT(
	    "Failed adding II_LOG_FILE to response file with error:\n %s\n",
			IIrfapi_errString( rfstat ) );
	    if ( rfhandle != NULL )
		IIrfapi_cleanup( &rfhandle );
	    return( rfstat );
	}

	/* Dual log? */
	if ( txlog_info.duallog == TRUE )
	{
	    param.name = txlog_info.dual_loc.rfapi_name;
	    param.value = txlog_info.dual_loc.path;
	    param.vlen = STlen( txlog_info.dual_loc.path );
	    rfstat = IIrfapi_addParam( &rfhandle, &param );

	    if ( rfstat != II_RF_ST_OK )
	    {
		DBG_PRINT(
		    "Failed adding II_DUAL_LOG to response file with error:\n %s\n",
			IIrfapi_errString( rfstat ) );
		if ( rfhandle != NULL )
		    IIrfapi_cleanup( &rfhandle );
		return( rfstat );
	    }
	}
	/* Date Type Alias */
	param.name = II_DATE_TYPE_ALIAS;
	param.value =
	    (II_RFAPI_STRING)date_type_alias.values[date_type_alias.val_idx];
	param.vlen = STlen( param.value );
	rfstat = IIrfapi_addParam( &rfhandle, &param );

	if ( rfstat != II_RF_ST_OK )
	{
	    DBG_PRINT(
	    "Failed adding II_DATE_TYPE_ALIAS to response file with error:\n %s\n",
			IIrfapi_errString( rfstat ) );
	    if ( rfhandle != NULL )
		IIrfapi_cleanup( &rfhandle );
	    return( rfstat );
	}

    }
    /* Add add the other options */
    rfstat = addMiscOps( &rfhandle,
			dep_platform & II_RF_DP_LINUX ?
			misc_ops : WinMiscOps );

    if ( rfstat != II_RF_ST_OK )
    {
	DBG_PRINT(
	    "Failed adding misc options to response file with error:\n %s\n",
			IIrfapi_errString( rfstat ) );
	if ( &rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
    }

    /* Write out response file */
    rfstat = IIrfapi_writeRespFile( &rfhandle );

    if ( rfstat != II_RF_ST_OK )
    {
	DBG_PRINT(
	    "Error writing response file:\n %s\n",
			IIrfapi_errString( rfstat ) );
	if ( &rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
    }

    return( rfstat );
}

static II_RFAPI_STATUS
gen_upgrade_response_file( II_RFAPI_STRING rflocstring )
{
    II_RFAPI_HANDLE	rfhandle; /* handle for respone file API */
    II_RFAPI_PARAM	param; /* RF API parameter info */
    II_RFAPI_STATUS	rfstat; /* response file API status info */
    i4			i = 0; /* misc counter */

    /* initialise response file handle */
    rfstat = IIrfapi_initNew( &rfhandle,
				dep_platform,
				(II_RFAPI_STRING)rfnameloc.string,
				II_RF_OP_CLEANUP_AFTER_WRITE );

    if ( rfstat != II_RF_ST_OK )
    {
	if ( &rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
	return( rfstat );
    }

    /* only one upgrade option for now */
    if ( misc_ops & GIP_OP_UPGRADE_USER_DB )
    {
	param.name = II_UPGRADE_USER_DB ;
	param.value = "YES" ;
	param.vlen = STlen( "YES" );
	rfstat = IIrfapi_addParam( &rfhandle, &param );
    }
    
    if ( rfstat != II_RF_ST_OK )
    {
	if ( &rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
	return( rfstat );
    }
    
    /* Write out response file */
    rfstat = IIrfapi_writeRespFile( &rfhandle );

    if ( rfstat != II_RF_ST_OK )
    {
	if ( &rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
    }

    return( rfstat );

}

static II_RFAPI_STATUS
gen_modify_response_file( II_RFAPI_STRING rflocstring )
{
    II_RFAPI_HANDLE	rfhandle; /* handle for respone file API */
    II_RFAPI_PARAM	param; /* RF API parameter info */
    II_RFAPI_STATUS	rfstat; /* response file API status info */
    char		tmpbuf[MAX_FNAME]; /* misc string buffer */
    i4			i = 0; /* misc counter */

    /* initialise response file handle */
    rfstat = IIrfapi_initNew( &rfhandle,
				dep_platform,
				(II_RFAPI_STRING)rfnameloc.string,
				II_RF_OP_CLEANUP_AFTER_WRITE );

    if ( rfstat != II_RF_ST_OK )
    {
	if ( &rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
	return( rfstat );
    }

    /* dbms config if the package as been added */
    if ( mod_pkgs_to_install & PKG_DBMS )
    {
	/* Add values for DB locations skipping II_SYSTEM */
	i = 1;
	while ( dblocations[i] != NULL )
	{
	    param.name = dblocations[i]->rfapi_name; /* variable name */
	    param.value = dblocations[i]->path; /* value */
	    param.vlen = STlen( dblocations[i]->path ); /* size */	
	    rfstat = IIrfapi_addParam( &rfhandle, &param );

	    if ( rfstat != II_RF_ST_OK )
	    {
		if ( rfhandle != NULL )
		    IIrfapi_cleanup( &rfhandle );
		return( rfstat );
	    }

	    i++;
	}

	/* Transaction Log */
	/* size */
	param.name = II_LOG_FILE_SIZE_MB;
	param.value = STprintf( tmpbuf, "%d", txlog_info.size );
	param.vlen = STlen( tmpbuf );
	rfstat = IIrfapi_addParam( &rfhandle, &param );

	if ( rfstat != II_RF_ST_OK )
	{
	    if ( rfhandle != NULL )
		IIrfapi_cleanup( &rfhandle );
	    return( rfstat );
	}

	/* location */
	param.name = txlog_info.log_loc.rfapi_name;
	param.value = txlog_info.log_loc.path;
	param.vlen = STlen( txlog_info.log_loc.path );
	rfstat = IIrfapi_addParam( &rfhandle, &param );

	if ( rfstat != II_RF_ST_OK )
	{
	    if ( rfhandle != NULL )
		IIrfapi_cleanup( &rfhandle );
	    return( rfstat );
	}

	/* Dual log? */
	if ( txlog_info.duallog == TRUE )
	{
	    param.name = txlog_info.dual_loc.rfapi_name;
	    param.value = txlog_info.dual_loc.path;
	    param.vlen = STlen( txlog_info.dual_loc.path );
	    rfstat = IIrfapi_addParam( &rfhandle, &param );

	    if ( rfstat != II_RF_ST_OK )
	    {
		if ( rfhandle != NULL )
		    IIrfapi_cleanup( &rfhandle );
		return( rfstat );
	    }
	}
	/* Date Type Alias */
	param.name = II_DATE_TYPE_ALIAS;
	param.value =
	    (II_RFAPI_STRING)date_type_alias.values[date_type_alias.val_idx];
	param.vlen = STlen( param.value );
	rfstat = IIrfapi_addParam( &rfhandle, &param );

	if ( rfstat != II_RF_ST_OK )
	{
	    if ( rfhandle != NULL )
		IIrfapi_cleanup( &rfhandle );
	    return( rfstat );
	}

    }
    /* only one modify param for now */
    if ( ug_mode & UM_RMV && misc_ops & GIP_OP_REMOVE_ALL_FILES )
    {
	param.name = GIP_OP_REMOVE_ALL_FILES;
	param.value = "YES" ;
	param.vlen = STlen( "YES" );
	rfstat = IIrfapi_addParam( &rfhandle, &param );
    }
    
    if ( rfstat != II_RF_ST_OK )
    {
	if ( &rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
	return( rfstat );
    }
    
    /* Write out response file */
    rfstat = IIrfapi_writeRespFile( &rfhandle );

    if ( rfstat != II_RF_ST_OK )
    {
	if ( &rfhandle != NULL )
	    IIrfapi_cleanup( &rfhandle );
    }

    return( rfstat );
}

II_RFAPI_STATUS
addMiscOps( II_RFAPI_HANDLE *Handleptr, MISCOPS miscops )
{
    i4			i = 0;
    II_RFAPI_PARAM	param; /* RF API parameter info */
    II_RFAPI_STATUS 	rfstat = II_RF_ST_OK ;

    /* scroll through the list of options */
    while ( misc_ops_info[i] != NULL && rfstat == II_RF_ST_OK )
    {
	switch( misc_ops_info[i]->bit )
	{
	    /* check for special cases */
	    case GIP_OP_INST_CUST_USR:   
	    case GIP_OP_INST_CUST_GRP:   
		if ( miscops & misc_ops_info[i]->bit )
		{
		    /* add installation owner */
		    param.name = misc_ops_info[i]->rfapi_name;
		    if ( misc_ops_info[i]->bit == GIP_OP_INST_CUST_USR )
		    {
			param.value = userid;
			param.vlen = STlen( userid );
		    }
		    else
		    {
			param.value = groupid;
			param.vlen = STlen( groupid );
		    }
		    rfstat = IIrfapi_addParam( Handleptr, &param );
		}
		break;
	    case GIP_OP_WIN_START_CUST_USR:
		if ( miscops & misc_ops_info[i]->bit )
		{
		    /* add service user */
		    param.name = misc_ops_info[i]->rfapi_name;
		    param.value = (II_RFAPI_PVALUE)WinServiceID;
		    param.vlen = STlen( WinServiceID );
		    rfstat = IIrfapi_addParam( Handleptr, &param );

		    if ( rfstat != II_RF_ST_OK )
			break;

		    /* add password too */
		    param.name = II_SERVICE_START_USERPASSWORD;
		    param.value = (II_RFAPI_PVALUE)WinServicePwd;
		    param.vlen = STlen( WinServicePwd );
		    rfstat = IIrfapi_addParam( Handleptr, &param );

		    if ( rfstat != II_RF_ST_OK )
			break;

		}
		break;
	    default:
		/* for the rest say "YES" if it's on and "NO" if it's not. */
		if ( miscops & misc_ops_info[i]->bit )
		{
		    param.value = "YES"; 
		    param.vlen = STlen("YES");
		}
		else
		{
		    param.value = "NO"; 
		    param.vlen = STlen("NO");
		}

        	/* add parameters for the ones that are set  */
		param.name = misc_ops_info[i]->rfapi_name;
		rfstat = IIrfapi_addParam( Handleptr, &param );
		break;
	}
		
	i++;
    }

    return( rfstat );
    
}

void 
popup_error_box( gchar *errmsg )
{
    GtkWidget	*prompt;
    GtkWidget	*label;
    GtkWidget	*hbox;
    GtkWidget	*image;
	
# define ERROR_DIALOG_TITLE "Error"
    /* create quit prompt to confirm user's decision to quit app */
    prompt = gtk_dialog_new_with_buttons(
					ERROR_DIALOG_TITLE,
					GTK_WINDOW( IngresInstall ),
					GTK_DIALOG_MODAL |
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_OK,
					GTK_RESPONSE_ACCEPT,
					NULL); 

    /* add a separator above the buttons */
    gtk_dialog_set_has_separator( GTK_DIALOG( prompt ), TRUE ); 

    /* create the vbox */
    hbox = gtk_hbox_new( FALSE, 0 );
    gtk_widget_show( hbox );
    gtk_box_pack_start( GTK_BOX( GTK_DIALOG( prompt )->vbox ) ,
			hbox, FALSE, FALSE, 0 );

    /* create the image */
    image = gtk_image_new_from_stock( "gtk-dialog-error", GTK_ICON_SIZE_DIALOG );
    gtk_widget_show( image );
    gtk_box_pack_start( GTK_BOX( hbox ), image, FALSE, FALSE, 0 );
    gtk_misc_set_padding( GTK_MISC( image ), 5, 0);

    /* Add some text */
    label = gtk_label_new( errmsg );
    gtk_widget_show( label );
    gtk_box_pack_start( GTK_BOX( hbox ),
			label,
			FALSE,
			FALSE,
			0);
    gtk_misc_set_alignment( GTK_MISC(label), 0.5, 0.5);
    gtk_misc_set_padding( GTK_MISC(label), 5, 0);
    
    /* prompt the user to confirm quit */
    gtk_dialog_run( GTK_DIALOG( prompt ) );
    gtk_widget_destroy ( prompt );
}

STATUS
inst_name_in_use( char *instname )
{
    i4 i = 0 ; /* loop counter */

    /* No existing instances, not inst IDs used */
    if ( *existing_instances == NULL )
	return( FAIL );

    /* Scroll through the instance names */
    while ( i < inst_count )
    {
	/* if we get a match, return TRUE */
	if ( STcompare( instname,
		existing_instances[i]->instance_name ) == OK )
	    return( OK );
	i++;
    }

    /* no match found */
    return( FAIL );
}

bool
is_renamed( char *basename )
{
    char *idxptr;

    /*
    ** Ingres package basenames have 2 major formats:
    **
    ** 		<package name>
    **	&	<package name>-XX
    **
    ** where XX is an installation ID.
    ** Therefore if the last 2 characters of the package name match
    ** the rules for an installation ID, it been renamed.
    */

    /* find the last '-' in the string */
    idxptr = STrindex( basename, "-", 0 );

    /* check the next 2 characters match an installation ID */


    if ( idxptr != NULL )
    {
	CMnext( idxptr );
	if ( CMupper( idxptr ) )
	{
            CMnext( idxptr );
            if ( CMupper( idxptr ) || CMdigit( idxptr ) )
              return( TRUE );
	}
    }

    return( FALSE );
}

void
increment_wizard_progress(void)
{
    GtkWidget *pos_image;
    GtkWidget *next_button;
    GtkWidget *back_button;
    GtkWidget *install_button;
    GtkWidget *write_button;
    GtkWidget *finish_button;
    GtkWidget *quit_button;
    II_RFAPI_STATUS	rfstat;
    int 	  i;
	
    /* Set the 'status' images based on the status info for each stage */
    for ( i = 0 ; stage_names[i] != NULL ; i++ )
    {
	pos_image=lookup_widget(IngresInstall, stage_names[i]->stage_name);

	DBG_PRINT("current_stage=%d\n", current_stage);
	if ( i < current_stage )			
	    if ( stage_names[i]->status == ST_FAILED )
		gtk_image_set_from_stock(GTK_IMAGE(pos_image),
					"gtk-cancel",
					GTK_ICON_SIZE_BUTTON);
	    else if ( stage_names[i]->status == ST_COMPLETE )
		gtk_image_set_from_stock(GTK_IMAGE(pos_image),
					"gtk-apply",
					GTK_ICON_SIZE_BUTTON);
	    else
	    {
		gtk_image_set_from_stock(GTK_IMAGE(pos_image),
					"gtk-yes",
					GTK_ICON_SIZE_BUTTON);			
		gtk_widget_set_sensitive(pos_image, FALSE );
	    }
	else if ( i == current_stage )
	{
	    if ( stage_names[i]->status == ST_FAILED )
	    {
		gtk_image_set_from_stock(GTK_IMAGE(pos_image),
					"gtk-no",
					GTK_ICON_SIZE_BUTTON);			
		gtk_widget_set_sensitive(pos_image, TRUE );
	    }
	    else
	    {
		gtk_image_set_from_stock(GTK_IMAGE(pos_image),
					"gtk-yes",
					GTK_ICON_SIZE_BUTTON);
		gtk_widget_set_sensitive(pos_image, TRUE);
	    }
	}
	else
	{
	    if ( stage_names[i]->status == ST_COMPLETE )
	    {
		gtk_image_set_from_stock(GTK_IMAGE(pos_image),
					"gtk-apply",
					GTK_ICON_SIZE_BUTTON);
		gtk_widget_set_sensitive(pos_image, TRUE );
	    }
	    else
	    {	
		gtk_image_set_from_stock(GTK_IMAGE(pos_image),
					"gtk-yes",
					GTK_ICON_SIZE_BUTTON);			
		gtk_widget_set_sensitive(pos_image, FALSE );
	    }
	}
    }
	
    if ( runmode == MA_UPGRADE )
    {
	/* grey out or active back or next bottons as appropriate */
	next_button=lookup_widget(IngresInstall, "ug_next_button");
	back_button=lookup_widget(IngresInstall, "ug_back_button");
	install_button=lookup_widget(IngresInstall, "upgrade_button");
	finish_button=lookup_widget(IngresInstall, "ug_finish_button");
	quit_button=lookup_widget(IngresInstall, "ug_quit_button");
	
	switch ( stage_names[current_stage]->stage )
	{
	    GtkWidget *widget_ptr;
	    gchar	tmpbuf[100];
			
	    case UG_START:
		gtk_widget_set_sensitive(back_button, FALSE);
		gtk_widget_set_sensitive(next_button, TRUE);
		break;
	    case UG_LIC:
		widget_ptr=lookup_widget(IngresInstall, "upg_lic_accept" );
		gtk_widget_set_sensitive(back_button, TRUE);
    		if ( gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(widget_ptr)) != TRUE )
		    gtk_widget_set_sensitive(next_button, FALSE);
		break;
	    case UG_SELECT:
		gtk_widget_show(next_button);
		gtk_widget_set_sensitive(next_button, TRUE);
		gtk_widget_set_sensitive(back_button, TRUE);
		gtk_widget_hide(install_button);
		/*
		** set the text to read modify or upgrade depending on what
		** we're actuall doing
		*/
		widget_ptr=lookup_widget(IngresInstall, "multi_ug_mod_label" );
		STprintf( tmpbuf, UG_MOD_UPG_TXT, (ug_mode & UM_UPG) ? 
							"upgrade" : "modify" );
		gtk_label_set_text( GTK_LABEL(widget_ptr), tmpbuf );
		widget_ptr=lookup_widget(IngresInstall, "ug_button_label" );
		gtk_label_set_text_with_mnemonic( GTK_LABEL(widget_ptr), (ug_mode & UM_UPG) ? 
							"_Upgrade" : "_Modify" );
		widget_ptr=lookup_widget(IngresInstall, "ug_all_usr_db_box" );
		if ( ug_mode & UM_UPG )
		    gtk_widget_show( widget_ptr );
		else
		    gtk_widget_hide( widget_ptr );
		break;
	    case UG_SUMMARY:
		gtk_widget_hide(next_button);
		gtk_widget_show(install_button);
			
		/* set text correctly for mode */
		widget_ptr=lookup_widget(IngresInstall, "ug_sum_label1");
		STprintf(tmpbuf, UG_SUM_LABEL1_TXT, (ug_mode & UM_UPG) ?
							"Upgrade" : "Modify" );
		gtk_label_set_markup(GTK_LABEL(widget_ptr), tmpbuf);
		widget_ptr=lookup_widget(IngresInstall, "ug_sum_label2");
		STprintf(tmpbuf, UG_SUM_LABEL2_TXT, (ug_mode & UM_UPG) ?
							"Upgrade" : "Modify" );
		gtk_label_set_markup(GTK_LABEL(widget_ptr), tmpbuf);
		widget_ptr=lookup_widget(IngresInstall, "ug_sum_label3");
		STprintf(tmpbuf, UG_SUM_LABEL3_TXT, (ug_mode & UM_UPG) ?
							"upgrade" : "modify" );
		gtk_label_set_markup(GTK_LABEL(widget_ptr), tmpbuf);
		widget_ptr=lookup_widget(IngresInstall, "ug_inst_label4");
		STprintf(tmpbuf, UG_SUM_LABEL4_TXT, (ug_mode & UM_UPG) ?
						"upgraded" : "modified" );
		gtk_label_set_markup(GTK_LABEL(widget_ptr), tmpbuf);
		widget_ptr=lookup_widget(IngresInstall, "ug_inst_label5");
		STprintf(tmpbuf, UG_SUM_LABEL5_TXT, (ug_mode & UM_UPG) ?
						"upgraded" : "modified" );
		gtk_label_set_markup(GTK_LABEL(widget_ptr), tmpbuf);
		widget_ptr=lookup_widget(IngresInstall, "ug_inst_label6");
		STprintf(tmpbuf, UG_SUM_LABEL6_TXT, (ug_mode & UM_UPG) ?
							"Upgrade" : "Modify" );
		gtk_label_set_markup(GTK_LABEL(widget_ptr), tmpbuf);
		widget_ptr=lookup_widget(IngresInstall, "ug_inst_label7");
		STprintf(tmpbuf, UG_SUM_LABEL7_TXT, (ug_mode & UM_UPG) ?
							"Upgrade" : "Modify" );
		gtk_label_set_markup(GTK_LABEL(widget_ptr), tmpbuf);
		widget_ptr=lookup_widget(IngresInstall,
						 "upgrade_install_label");
		gtk_label_set_text(GTK_LABEL(widget_ptr), (ug_mode & UM_UPG) ?
							"Upgrade" : "Modify" );

		/* set the "go" button text */
		widget_ptr=lookup_widget(IngresInstall,
						 "ug_button_label");
		gtk_label_set_text_with_mnemonic(GTK_LABEL(widget_ptr), (ug_mode & UM_UPG) ?
							"_Upgrade" : "_Modify" );
			
		/* Write the summary before we switch windows */
		widget_ptr=lookup_widget(IngresInstall, "UgSummaryTextView");
		write_upgrade_summary( gtk_text_view_get_buffer(
						GTK_TEXT_VIEW( widget_ptr ) ) );
		/* Write the summary before we switch windows */
		widget_ptr=lookup_widget(IngresInstall, "UgFinishTextView");
		write_upgrade_summary( gtk_text_view_get_buffer(
					GTK_TEXT_VIEW( widget_ptr ) ) );
		break;
	    case UG_DO_INST:
		gtk_widget_hide( back_button );
		gtk_widget_hide( next_button );
		gtk_widget_hide( install_button );
		gtk_widget_hide( finish_button );
		gtk_widget_hide( quit_button );
		break;
	    case UG_DONE:
	    case UG_FAIL:
		gtk_widget_show(back_button);
		gtk_widget_set_sensitive(back_button, FALSE);
		gtk_widget_show(finish_button);
		gtk_widget_set_sensitive(finish_button, TRUE);
		gtk_widget_show(quit_button);
		gtk_widget_set_sensitive(quit_button, FALSE);
		if ( stage_names[current_stage]->stage == UG_DONE )
	 	{
		    /* Write the summary before we switch windows */
		    widget_ptr=lookup_widget(IngresInstall, "UgFinishTextView");
		    write_upgrade_summary( gtk_text_view_get_buffer(
					GTK_TEXT_VIEW( widget_ptr ) ) );
		}
		break;
	    default:
		gtk_widget_show(next_button);
		gtk_widget_set_sensitive(next_button, TRUE);
		gtk_widget_set_sensitive(back_button, TRUE);
		gtk_widget_hide(install_button);
		break;
	}
    }
    else
    {
	/* grey out or active back or next bottons as appropriate */
	next_button=lookup_widget(IngresInstall, "next_button");
	back_button=lookup_widget(IngresInstall, "back_button");
	install_button=lookup_widget(IngresInstall, "install_button");
	finish_button=lookup_widget(IngresInstall, "finish_button");
	write_button=lookup_widget(IngresInstall, "write_button");
	quit_button=lookup_widget(IngresInstall, "quit_button");
		
	switch ( stage_names[current_stage]->stage )
	{
	    GtkWidget *widget_ptr;
		
	    case NI_START:
		gtk_widget_set_sensitive(back_button, (ug_mode & UM_TRUE) ?
							TRUE : FALSE );
		gtk_widget_set_sensitive(next_button, TRUE);
		break;
	    case NI_LIC:
		widget_ptr=lookup_widget(IngresInstall, "inst_lic_accept" );
		gtk_widget_set_sensitive(back_button, TRUE);
    		if ( gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(widget_ptr)) != TRUE )
		    gtk_widget_set_sensitive(next_button, FALSE);
		break;
	    case NI_INSTID:
		/* validate instID entry */
		widget_ptr = lookup_widget( IngresInstall, "instID_entry2" );
		on_instID_changed( GTK_EDITABLE( widget_ptr ), NULL );
		gtk_widget_show(next_button);
		gtk_widget_set_sensitive(back_button, TRUE);
		gtk_widget_hide(install_button);
		gtk_widget_hide(write_button);
		break;
	    case NI_DBLOC:
		/* validate location entries */
		widget_ptr = lookup_widget( IngresInstall, "ii_system_entry" );
		on_ii_system_entry_changed( GTK_EDITABLE( widget_ptr ), NULL );
		gtk_widget_show(next_button);
		gtk_widget_set_sensitive(back_button, TRUE);
		gtk_widget_hide(install_button);
		gtk_widget_hide(write_button);
		break;
	    case NI_LOGLOC:
		widget_ptr = lookup_widget( IngresInstall,
						"primary_log_file_loc" );
		on_primary_log_file_loc_changed( GTK_EDITABLE( widget_ptr ),
						NULL );
		gtk_widget_show(next_button);
		gtk_widget_set_sensitive(back_button, TRUE);
		gtk_widget_hide(install_button);
		gtk_widget_hide(write_button);
		break;
	    case NI_LOCALE:
		gtk_widget_show(next_button);
		gtk_widget_set_sensitive(back_button, TRUE);
		gtk_widget_hide(install_button);
		gtk_widget_hide(write_button);
		GIPValidateLocaleSettings();
		break;
	    case NI_MISC:
		/* validate instance owner info */
		if ( groupid[0] == '\0' || userid[0] == '\0' )
		    gtk_widget_set_sensitive( next_button, FALSE );
		else
		    gtk_widget_set_sensitive( next_button, TRUE );
		gtk_widget_show(next_button);
		gtk_widget_set_sensitive(back_button, TRUE);
		gtk_widget_hide(install_button);
		gtk_widget_hide(write_button);
		break;
	    case NI_SUMMARY:
		/* Write the summary before we switch windows */
		widget_ptr=lookup_widget(IngresInstall, "SummaryTextView");
		write_installation_summary( gtk_text_view_get_buffer(
						GTK_TEXT_VIEW( widget_ptr ) ) );
		gtk_widget_hide(next_button);
		gtk_widget_show(install_button);
		break;
	    case RF_SUMMARY:
		/* Write the summary before we switch windows */
		widget_ptr=lookup_widget(IngresInstall, "RFSummaryTextView");
		write_installation_summary( gtk_text_view_get_buffer(
						GTK_TEXT_VIEW( widget_ptr ) ) );
		break;
	    case NI_INSTALL:
		gtk_widget_hide(back_button);
		gtk_widget_hide(next_button);
		gtk_widget_hide(install_button);
		gtk_widget_hide(finish_button);
		gtk_widget_hide(quit_button);
		break;
	    case NI_DONE:
	    case NI_FAIL:
		gtk_widget_set_sensitive(back_button, FALSE);
		gtk_widget_show(back_button);
		gtk_widget_show(finish_button);
		gtk_widget_set_sensitive(quit_button, FALSE);
		gtk_widget_show(quit_button);

		if ( stage_names[current_stage]->stage == NI_DONE )
		{
		    /* Write the summary before we switch windows */
		    widget_ptr=lookup_widget(IngresInstall, "FinishTextView");
		    write_installation_summary( gtk_text_view_get_buffer(
						GTK_TEXT_VIEW( widget_ptr ) ) );
		}
		break;
	    case RF_DONE:
		if ( ( rfstat = generate_response_file() ) != II_RF_ST_OK )
		{
		    char	tmpbuf[MAX_LOC];

# define ERROR_CREATE_RF_FAIL "Response file creation failed with:\n\t%s"
		    STprintf( tmpbuf,
				ERROR_CREATE_RF_FAIL,
				IIrfapi_errString( rfstat ) );
		    popup_error_box( tmpbuf );
		}

		widget_ptr=lookup_widget(IngresInstall, "RFPreviewTextView");
		write_file_to_text_buffer( rfnameloc.string,
					gtk_text_view_get_buffer(
						GTK_TEXT_VIEW( widget_ptr ) ) );
		gtk_widget_set_sensitive(next_button, FALSE);
		gtk_widget_show(write_button);
		gtk_widget_set_sensitive(back_button, TRUE);
		gtk_widget_show(back_button);
		gtk_widget_show(finish_button);
		gtk_widget_set_sensitive(quit_button, FALSE);
		gtk_widget_show(quit_button);
		break;
	    default:
		gtk_widget_show(next_button);
		gtk_widget_set_sensitive(next_button, TRUE);
		gtk_widget_set_sensitive(back_button, TRUE);
		gtk_widget_hide(install_button);
		gtk_widget_hide(write_button);
		break;
	}
    }
}

void
write_upgrade_summary(GtkTextBuffer *buffer)
{
    GtkTextIter	iter, start, end;
    gchar	tmpbuf[50];
    gint i;
	
    /* Clear out the buffer so we can start from scratch */
    gtk_text_buffer_get_start_iter( buffer, &start );
    gtk_text_buffer_get_end_iter( buffer, &end );
    gtk_text_buffer_delete( buffer, &start, &end );
    
    /* initialize */
    gtk_text_buffer_get_iter_at_offset( buffer, &iter, 0 );
    
    /* Title */
    STprintf(tmpbuf, "%s Summary", ug_mode & UM_MOD ?
    	   			    "Modify" : "Upgrade" );
    gtk_text_buffer_insert_with_tags_by_name( buffer, &iter, tmpbuf,
						-1, "heading", NULL);
    gtk_text_buffer_insert( buffer, &iter, "\n\n", -1 );
    
    /* instance info */
    STprintf(tmpbuf, "Instance %s:\n\t", ug_mode & UM_MOD ?
    		   				"modified" : "upgraded" );
    gtk_text_buffer_insert_with_tags_by_name( buffer, &iter, tmpbuf,
					    		-1, "bold", NULL );
    gtk_text_buffer_insert_with_tags_by_name( buffer, &iter, 
					selected_instance->instance_name, -1,
    					"monospace", NULL);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
    					    "Instance location:\n\t", -1,
					    "bold", NULL);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, 
					    selected_instance->inst_loc, -1,
					    "monospace", NULL);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
    
    if ( ug_mode & UM_UPG )
    {
	/* upgrade */
	/* old component list */
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
						"Old version:\n\t", -1,
						"bold", NULL);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, 
						selected_instance->version, -1,
						"monospace", NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

	/* new component list */
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
						"New version:\n\t", -1,
						"bold", NULL);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, 
						new_pkgs_info.version, -1,
						"monospace", NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
	
	/* upgrade info */
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
						"Upgrade user databases: ",
						-1, "bold", NULL);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, 
					misc_ops & GIP_OP_UPGRADE_USER_DB ?
						"Yes" : "No", -1,
						"monospace", NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
    }
    else
    {
	/* modify */
	/* Warn if we're removing the directory */
	if ( misc_ops & GIP_OP_REMOVE_ALL_FILES )
	{
	    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, 
						"Remove all files: ", -1,
						"bold", NULL );
	    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, 
	    				misc_ops & GIP_OP_REMOVE_ALL_FILES ?
						"Yes" : "No", -1, "monospace",
	    				misc_ops & GIP_OP_REMOVE_ALL_FILES ?
					 "warning" : NULL, NULL);
	    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
	}

	/* package info */
	if ( mod_pkgs_to_install == PKG_NONE )
	{
	    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
						   "Components added:\n",
						     -1, "bold", NULL);
	    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, 
	    					   "\tNONE",	-1,
						   "monospace", NULL);
	    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	}
	else
	{
	    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
						   "Components added:\n",
					 	   -1, "bold", NULL);
	    
	    for ( i = 0 ; packages[i] ; i++ )
	    {
		/* if the package is selected and not hidden, display it */
		if ( (mod_pkgs_to_install & packages[i]->bit) )
		{
		   STprintf( tmpbuf, "\t%s\n", packages[i]->display_name );
		   gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
							    tmpbuf, -1,
							    "monospace", NULL);
		}
	    }
	}
	gtk_text_buffer_insert( buffer, &iter, "\n", -1 );
	
	/* packages being removed */
	if ( mod_pkgs_to_remove == PKG_NONE )
	{
	    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
						"Components removed:\n",
						-1, "bold", NULL);
	    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\tNONE",
							-1, "monospace", NULL);
	    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	}
	else
	{
	    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
						"Components removed:\n",
						-1, "bold", NULL);
	    for ( i = 0 ; packages[i] ; i++ )
	    {
		/* if the package is selected and not hidden, display it */
		if ( (mod_pkgs_to_remove & packages[i]->bit) )
		{
		    STprintf( tmpbuf, "\t%s\n", packages[i]->display_name );
		    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
							tmpbuf, -1,
							"monospace", NULL);
		}
	    }
	}
		
	gtk_text_buffer_insert( buffer, &iter, "\n", -1 );

	if ( mod_pkgs_to_install & PKG_DBMS )
	{
	    /* DB file locations */
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						"Database file locations:\n",
						-1,
						"bold",
						NULL);	

	    for ( i = 1 ; dblocations[i] ; i++ )
	    {
		STprintf( tmpbuf,
			"%s (%s): ",
			dblocations[i]->name, 
			dblocations[i]->symbol );
		gtk_text_buffer_insert(buffer,
				&iter,
				tmpbuf,
				-1);
		gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter, 
						dblocations[i]->path,
						-1,
						"monospace",
						NULL);
		gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	    }
	    /* Tx log info */
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						"\nTrasaction log(s):\n",
						-1,
						"bold",
						NULL);	
	    gtk_text_buffer_insert(buffer, &iter, "Log file size: ", -1);
	    STprintf( tmpbuf, "%dMB\n", txlog_info.size );
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						tmpbuf,
						-1,
						"monospace",
						NULL);
	    STprintf( tmpbuf, "%s: ", txlog_info.log_loc.name );
	    gtk_text_buffer_insert(buffer, &iter, tmpbuf, -1);
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
	    					txlog_info.log_loc.path,
						-1,
						"monospace",
						NULL);
	    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		
	    /* dual logging */
	    if ( txlog_info.duallog )
	    {
		gtk_text_buffer_insert(buffer,
					&iter,
					"Dual logging enabled\n",
					-1 );
		STprintf( tmpbuf, "%s: ", txlog_info.dual_loc.name );
		gtk_text_buffer_insert(buffer, &iter, tmpbuf, -1);
		gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter, 
						txlog_info.dual_loc.path,
						-1,
						"monospace",
						NULL);
		gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	    }
	    else
		gtk_text_buffer_insert(buffer,
				&iter,
				"Dual logging disabled\n",
				-1 );

	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						"\nDatabase server options:\n",
						-1,
						"bold",
						NULL);	
	    /* SQL92 Compliance */
	    gtk_text_buffer_insert(buffer,
				&iter,
				"Strict SQL-92 Compliance: ",
				-1);
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						misc_ops & GIP_OP_SQL92 ?
						"Yes" : "No" ,
						-1,
						"monospace",
						NULL);
	    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

	    /* Date Type Alias */
	    gtk_text_buffer_insert(buffer,
				&iter,
				"\"DATE\" type alias: ",
				-1);
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						date_type_alias.values[date_type_alias.val_idx],
						-1,
						"monospace",
						NULL);
	    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

	    /* misc config */
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						"\nOther Options\n",
						-1,
						"bold",
						NULL);	

	    gtk_text_buffer_insert(buffer,
				&iter,
				"Create a demo database: ",
				-1);
	    gtk_text_buffer_insert_with_tags_by_name(buffer,
						&iter,
						misc_ops & GIP_OP_INSTALL_DEMO ?
						"Yes" : "No" ,
						-1,
						"monospace",
						NULL);
	    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	}
    }
}

int
write_file_to_text_buffer(char *fileloc, GtkTextBuffer *buffer)
{
    GtkTextIter	iter, start, end;
    FILE	*file_fd;
    gchar	readbuf[50];
    gint i;
	
	DBG_PRINT( "Reading file: %s\n", fileloc );
    /* initialize */
    /* get GTK thread lock before doing GTK stuff */
    gdk_threads_enter();

    /* Clear out the buffer so we can start from scratch */
    gtk_text_buffer_get_start_iter  (buffer, &start);
    gtk_text_buffer_get_end_iter    (buffer, &end);
    gtk_text_buffer_delete          (buffer, &start, &end);
	
    gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
    gdk_threads_leave();

    /* open stdout output */
    file_fd = fopen( fileloc, "r" );

    if ( file_fd == NULL )
	return( FAIL );

    /*
    ** Now read output from file in OUTPUT_READSIZE chunks and
    ** write it to the buffer until we get and EOF
    */
    while ( fgets( readbuf, OUTPUT_READSIZE, file_fd ) )
    {
	/* get GTK thread lock before doing GTK stuff */
	gdk_threads_enter();
	gtk_text_buffer_insert( buffer, &iter, readbuf, -1 );
	gdk_threads_leave();
        fflush( file_fd );
    }

    /* close log file */
    fclose( file_fd );

    return( OK );
	
}

void
add_instance_columns(GtkTreeView *treeview)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeModel *model = gtk_tree_view_get_model (treeview);	
    
    /* instance name */
    renderer=gtk_cell_renderer_text_new();
    column=gtk_tree_view_column_new_with_attributes( "Instance Name",
							renderer,
							"text",
							COL_NAME,
							NULL);
    gtk_tree_view_column_set_sort_column_id (column, COL_NAME);
    gtk_tree_view_append_column (treeview, column);
	
    /* instance version */
    renderer=gtk_cell_renderer_text_new();
    column=gtk_tree_view_column_new_with_attributes( "Version",
							renderer,
							"text",
							COL_VERSION,
							NULL);
    gtk_tree_view_column_set_sort_column_id (column, COL_VERSION);
    gtk_tree_view_append_column (treeview, column);
	
    /* instance location */
    renderer=gtk_cell_renderer_text_new();
    column=gtk_tree_view_column_new_with_attributes( "Location",
							renderer,
							"text",
							COL_LOCATION,
							NULL);
    gtk_tree_view_column_set_sort_column_id (column, COL_LOCATION);
    gtk_tree_view_append_column (treeview, column);
	
}

int
add_instance_to_store( GtkListStore *instance_store,
			instance *instance_info )
{
    GtkTreeIter	row_iter;
    gint i;
	
    gtk_list_store_append( instance_store, &row_iter);
    gtk_list_store_set( instance_store, &row_iter,
			COL_NAME, instance_info->instance_name,
			COL_VERSION, instance_info->version,
			COL_LOCATION, instance_info->inst_loc,
			-1 ); 
    return(0);
}

void
set_mod_pkg_check_buttons(PKGLST packages)
{
    GtkWidget *checkbutton;
	
    DBG_PRINT("set_mod_pkg_check_buttons called\n");
    /* 
    ** Lookup the check buttons for the packages
    ** and set them accordingly
    */
    checkbutton=lookup_widget(IngresInstall, "ug_checkdbmspkg");
    gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_DBMS );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), 
				    new_pkgs_info.pkgs &
				    packages&PKG_DBMS);
    
    checkbutton=lookup_widget(IngresInstall, "ug_checknetpkg");
    gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_NET );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),
				    new_pkgs_info.pkgs &
				    packages&PKG_NET);
    
    checkbutton=lookup_widget(IngresInstall, "ug_checkodbcpkg");
    gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_ODBC );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),
				    new_pkgs_info.pkgs &
				    packages&PKG_ODBC);
    
    checkbutton=lookup_widget(IngresInstall, "ug_checkreppkg");
    gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_REP );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), 
				    new_pkgs_info.pkgs &
				    packages&PKG_REP);
    
    checkbutton=lookup_widget(IngresInstall, "ug_checkabfpkg");
    gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_ABF );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),
				    new_pkgs_info.pkgs &
				    packages&PKG_ABF);
    
    checkbutton=lookup_widget(IngresInstall, "ug_checkstarpkg");
    gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_STAR );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),
				    new_pkgs_info.pkgs &
				    packages&PKG_STAR);
    
    checkbutton=lookup_widget(IngresInstall, "ug_checkdocpkg");
    if ( selected_instance->action & UM_RENAME )
	gtk_widget_set_sensitive( checkbutton, FALSE );
    else 
    {
	gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_DOC );
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),
    					new_pkgs_info.pkgs &
    					packages&PKG_DOC);
    }
}

GtkWidget *
create_text_view_dialog_from_model	( GtkTreeModel *model,
					  GtkWidget *entry,
					  GtkTreeIter *dflt_iter )
{
    GtkWidget	*dialog;
    GtkWidget	*ScrolledWindow;
    GtkWidget	*TreeView;
    GtkTreeIter	parent;
    GtkCellRenderer	*renderer;
    GtkTreeSelection	*selected_entry;
    GdkPixbuf	*iconPixBuf;
	
    /* create dialog for displaying tree view */
    dialog = gtk_dialog_new();
    gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );
    gtk_window_set_title( GTK_WINDOW( dialog ), "Select a value" );
    gtk_window_set_skip_taskbar_hint( GTK_WINDOW( dialog ), TRUE );
    gtk_window_set_skip_pager_hint( GTK_WINDOW( dialog ), TRUE );

    /* Add icon to dialog */
    iconPixBuf = create_pixbuf( "enterprise.ico");
    if ( iconPixBuf )
    {
	gtk_window_set_icon( GTK_WINDOW( dialog ), iconPixBuf );
	gdk_pixbuf_unref( iconPixBuf );
    }

    /* create scrolled window to display tree view */
    ScrolledWindow = gtk_scrolled_window_new( NULL, NULL );
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (ScrolledWindow),
					   GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ScrolledWindow),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_ALWAYS);
    gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ),
				ScrolledWindow, TRUE, TRUE, 0);
    gtk_widget_show( ScrolledWindow );

    /* create tree view */
    TreeView = gtk_tree_view_new_with_model( model );
    gtk_tree_view_set_rules_hint( GTK_TREE_VIEW( TreeView ), FALSE );
    gtk_tree_view_set_headers_visible( GTK_TREE_VIEW( TreeView ), FALSE );

    /* create selection and handler */
    selected_entry = gtk_tree_view_get_selection( GTK_TREE_VIEW( TreeView ) );
    gtk_tree_selection_set_mode( selected_entry, GTK_SELECTION_BROWSE );
    gtk_tree_selection_set_select_function( selected_entry, 
						check_locale_selection,
						dflt_iter, NULL );

    /* Add column for display */
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( TreeView ),
							    -1, NULL,
							    renderer, "text",
							    0, NULL );
    gtk_widget_show( TreeView );

    /* now add it to the scrolled window */
    gtk_container_add( GTK_CONTAINER( ScrolledWindow ), TreeView );

    /* set the dialog size */
    gtk_window_set_default_size( GTK_WINDOW( dialog ), 100, 200 );
    gtk_window_set_position( GTK_WINDOW( dialog ), GTK_WIN_POS_MOUSE );

    /* add the callbacks */
    g_signal_connect( selected_entry,
                             "changed", 
                             G_CALLBACK (update_locale_entry),
                             entry );
    g_signal_connect( selected_entry,
                             "changed", 
                             G_CALLBACK (popup_list_selected),
                             dialog );
    
    /* finally set the default selection */
    /* get the parent row of the defaut */
    gtk_tree_model_iter_parent( model, &parent, dflt_iter );
    /* expand it */
    gtk_tree_view_expand_row( GTK_TREE_VIEW( TreeView ),
				gtk_tree_model_get_path( model, &parent ),
				TRUE );
    /* select the default */
    gtk_tree_selection_select_iter( selected_entry, dflt_iter );

    return( dialog ); 
}

gboolean
check_locale_selection	( GtkTreeSelection *selection,
				GtkTreeModel *model,
				GtkTreePath *path,
				gboolean path_currently_selected,
				gpointer data )
{
    GtkTreeIter	iter;

    /* only allow selection of "child" itererators */
    gtk_tree_model_get_iter( model, &iter, path );
    if ( gtk_tree_model_iter_has_child( model, &iter ) == TRUE )
	return( FALSE );
    else
	return( TRUE );
}

STATUS
prompt_shutdown( void )
{
    GtkWidget 	*prompt; /* popup widget */
    gint	resp; /* response */

# define SHUTDOWN_RUNNING_INSTANCE "The Ingres %s instance appears to be running. It must be shutdown for the installation to continue, do you want to shut it down now?"
    /* create message box to prompt user */
    prompt = gtk_message_dialog_new( GTK_WINDOW( IngresInstall ),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					SHUTDOWN_RUNNING_INSTANCE,
					selected_instance->instance_ID );
# define QUESTION_DIALOG_TITLE "Question"
    gtk_window_set_title( GTK_WINDOW( prompt ), QUESTION_DIALOG_TITLE );
    /* shutdown running instance? */
    resp = gtk_dialog_run( GTK_DIALOG( prompt ) ) ;
    gtk_widget_destroy( prompt ) ;

    if ( resp == GTK_RESPONSE_YES )
	return( OK );
    else
	return( FAIL );

}

/*
** Name: GIPdoValidation
**
** Description:
**	Check whether LOCATION is valid and set next/back buttons
**	appropriately
**
** Inputs:
**	LOCATION loc - LOCATION to check
**	GtkWidget *next_button - Pointer to next button to change
**	GtkWidget *back_button - Pointer to back button to change
**
** Outputs:
**
**	None
**
** Returns:
**	STATUS OK or FAIL
**
** History:
**	18-May-2010 (hanje04)
**	    Created.
*/
STATUS
GIPdoValidation(LOCATION *loc, GtkWidget *next_button, GtkWidget *back_button)
{
    /*
    ** Check we've got a full path.
    ** For installer (not package manager) check the database locations
    ** also exist. II_SYSTEM doesn't have to.
    */
    if ( ( ! GIPpathFull( loc ) ) || 
	( instmode != RFGEN_LNX &&
 	    instmode != RFGEN_WIN &&
	    ( ! LOexist( loc ) == OK ) ) )
    {
	gtk_widget_set_sensitive( next_button, FALSE );
	gtk_widget_set_sensitive( back_button, FALSE );
	return( FAIL );
    }
    else
    {
	gtk_widget_set_sensitive( next_button, TRUE );
	gtk_widget_set_sensitive( back_button, TRUE );
    }
    return(OK);
}

/*
** Name: GIPpathFull
**
** Description:
**
**	Check whether LOCATION contains a full path for given platform
**
** Inputs:
**	
**	LOCATION loc - LOCATION to check
**
** Outputs:
**
**	None
**
** Returns:
**
**	TRUE if LOCATION contains a full path
**	FALSE if LOCATION doesn't contain a full path
**
** History:
**
**	22-Feb-2007 (hanje04)
**	    Created.
*/
bool
GIPpathFull( LOCATION *loc )
{
    bool	ret_val = FALSE;

    /* if we're deploying where we're running we can just use LOisfull */
    if ( run_platform == dep_platform )
	return( LOisfull( loc ) );

    if ( dep_platform == II_RF_DP_LINUX )
	/* from UNIX LOisfull() */
	return ( loc->path != NULL && loc->path[0] == '/' );
    else if ( dep_platform == II_RF_DP_WINDOWS )
    {
	/* based on Windows LOisfull() */
	if ( loc->path )
	{
            if ( ( CMalpha( &loc->path[0] ) &&
		    loc->path[1] == ':' &&
		    loc->path[2] == '\\' ) ||
		    ( loc->path[0] == '\\' &&
			loc->path[1] == '\\' &&
			loc->path[2] != '\\' &&
			STindex( &loc->path[3], "\\", 0 ) != NULL ) )
			    ret_val = TRUE;
	}
    }

    return( ret_val );

}

/*
** Name: GIPvalidateLocs
**
** Description:
**
**	Check that all database and log locations contain valid entries.
**
** Inputs:
**	
**	iisystem - pointer to value for II_SYSTEM to check against.
**
** Outputs:
**
**	None
**
** Returns:
**
**	OK if all locations are valid
**	FAIL if ANY of the locations are found to be invalid
**
** History:
**
**	27-Feb-2007 (hanje04)
**	    Created.
*/
STATUS
GIPvalidateLocs( char *iisystem )
{
    GtkWidget	*next_button;
    GtkWidget	*back_button;
    GtkWidget	*db_loc_iisys_default;
    GtkWidget	*log_loc_iisys_default;
    GtkWidget	*ivw_loc_iidb_default;
    LOCATION	locloc;

    /* lookup the widgets we're going to act on */
    next_button=lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"next_button":
						"ug_next_button" );
    back_button=lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"back_button":
						"ug_back_button" );
    db_loc_iisys_default=lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"db_loc_iisys_default" :
						"mod_db_loc_iisys_default" );
    log_loc_iisys_default=lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"log_loc_iisys_default" :
						"mod_log_loc_iisys_default" );
    ivw_loc_iidb_default=lookup_widget( IngresInstall, "ivw_loc_iidb_default" );
    /*
    ** If there is an invalid location, disable the next button and return
    ** If it's the same as II_SYSTEM it's OK so skip the validation
    */
    if ( gtk_toggle_button_get_active( 
		GTK_TOGGLE_BUTTON( db_loc_iisys_default ) ) == FALSE )
    {
	int	i = 1; /* skip II_SYSTEM */

	/* database locations */
	while( dblocations[i] != NULL )
	{
	    /* 
	    ** if it's the same as II_SYSTEM it's OK so skip it
	    ** same goes for II_VWDATA as we check it later
	    */
	    if (i == INST_II_VWDATA ||
		( ! STcompare( iisystem, dblocations[i]->path) ))
	    {
		i++;
		continue;
	    }

	    LOfroms( PATH, dblocations[i]->path, &locloc ) ;
	    /*
	    ** Check we've got a full path.
	    ** For installer (not package manager) check the database locations
	    ** also exist. II_SYSTEM doesn't have to.
	    */
	    if ( GIPdoValidation(&locloc, next_button, back_button) != OK )
	    {
		DBG_PRINT( "New path %s for location %s is not valid\n", 
			dblocations[i]->path,
			dblocations[i]->name );
		return( FAIL );
	    }
	    i++;
	}
    }
    else
    {
	gtk_widget_set_sensitive( next_button, TRUE );
	gtk_widget_set_sensitive( back_button, TRUE );
    }

    /* primary log */
    if ( gtk_toggle_button_get_active( 
		GTK_TOGGLE_BUTTON( log_loc_iisys_default ) ) == FALSE &&
	STcompare( iisystem, txlog_info.log_loc.path ) != 0 )
    {
	LOfroms( PATH, txlog_info.log_loc.path, &locloc ) != OK;

        if ( GIPdoValidation(&locloc, next_button, back_button ) != OK )
	{
	    DBG_PRINT( "New path %s for txlog is not valid\n", 
			txlog_info.log_loc.path);
	    return( FAIL );
	}
    }

    /* dual log */
    if ( txlog_info.duallog == TRUE )
    {
	if ( STcompare( iisystem, txlog_info.dual_loc.path ) != 0 )
	{
	    LOfroms( PATH, txlog_info.dual_loc.path, &locloc ) != OK;

	    if ( GIPdoValidation(&locloc, next_button, back_button) != OK )
	    {
	        DBG_PRINT( "New path %s for txlog is not valid\n", 
			txlog_info.dual_loc.path);
		return( FAIL );
	    }
	}
    }

    /* vectorwise data */
    if ( gtk_toggle_button_get_active( 
		GTK_TOGGLE_BUTTON( ivw_loc_iidb_default ) ) == FALSE &&
		 STcompare( dblocations[INST_II_DATABASE]->path,
				dblocations[INST_II_VWDATA]->path ) != 0 &&
		 STcompare( iisystem, dblocations[INST_II_VWDATA]->path) ) 
    {
	LOfroms( PATH, dblocations[INST_II_VWDATA]->path, &locloc ) != OK;

        if ( GIPdoValidation(&locloc, next_button, back_button )!= OK )
	{
	    DBG_PRINT( "New path %s for %s is not valid\n", 
			dblocations[INST_II_VWDATA]->path,
			dblocations[INST_II_VWDATA]->name);
	    return( FAIL );
	}
    }
    return( OK );
}

/*
** Name: GIPvalidateLocateSettings
**
** Description:
**
**	Check that timezone and charset values entered by user are valid,
**	"grey out" Next button if they're not.
**
** Inputs:
**	
**	None
**
** Outputs:
**
**	None
**
** Returns:
**
**	None
**
** History:
**
**	08-Oct-2008 (hanje04)
**	    Created.
*/
void 
GIPValidateLocaleSettings()
{
    GtkWidget   *next_button;
    bool	CSok = FALSE;
    bool	TZok = FALSE;
    CHARSET	*charsets;
    i2		i = 0;

    next_button=lookup_widget(IngresInstall, "next_button");
    /*
    ** check the contents of local_info are valid,
    ** if not, disable next button
    */
    /* timezone */
    while ( ! (timezones[i].region == NULL && timezones[i].timezone == NULL) )
    {
	if ( timezones[i].timezone && 
	    ( STcompare(timezones[i].timezone, locale_info.timezone) == OK ) )
	{
	    TZok = TRUE;
	    break;
	}
	i++;
    }

    /* charset */
    charsets = ( dep_platform & II_RF_DP_WINDOWS ?
		windows_charsets : linux_charsets );
    i = 0;
    while ( ! (charsets[i].region == NULL && charsets[i].charset == NULL) )
    {
	if ( charsets[i].charset &&
	    ( STcompare(charsets[i].charset, locale_info.charset) == OK ))
	{
	    CSok = TRUE;
	    break;
	}
	i++;
    }
    gtk_widget_set_sensitive( next_button, ( ( CSok == TRUE && TZok == TRUE ) ?
                                                 TRUE : FALSE ) );

}

/*
** Name: GIPValidatePkgName()
**
** Description:
**
**	Check that a aribtrary file name is a valid Ingres package name
**	for a specified format
**
** Inputs:
**	
**	char	*pkgname - Package file name string
**	char	*format - Package format (RPM DEB etc)
**
** Outputs:
**
**	None
**
** Returns:
**
**	OK - Valid package name
**	FAIL - Invalid package name
**
** History:
**
**	10-Oct-2008 (hanje04)
**	    Created.
*/
STATUS
GIPValidatePkgName( char *pkgname, char *format )
{
    char 	pnbuf[MAX_FNAME];
    char	*pnptr;
    char	vbuf[10];

    if ( pkgname != NULL && format != NULL )
    {
	if ( STcompare(new_pkgs_info.format, format ) == 0 &&
		STncmp(new_pkgs_info.pkg_basename, pkgname, 
		    STlen( new_pkgs_info.pkg_basename ) ) == 0 )
	{
	    /* looks good so far, has it been renamed */
            STlcopy( pkgname, pnbuf, MAX_FNAME );

	    /* strip of everything after the package name */
	    STprintf( vbuf, "%d.%d", GV_MAJOR, GV_MINOR );
	    pnptr = STindex( pnbuf, vbuf, 0 );
	    if ( pnptr )
	    {
	        --pnptr;
	        *pnptr = '\0';
	    }

	    if ( ! is_renamed(pnbuf) )
		return(OK);
	}
    }
    return(FAIL);
}

/*
** Name: GIPAddDBLoc()
**
** Description:
**
**	Add location to list of database locations defined for a given
**	instance (instance.datalocs)
**
** Inputs:
**	
**	instance	*inst - Instance structure to update
**	i4		*type - Location type (index in dblocations)
**	const char	*path - Path to add
**
** Outputs:
**
**	None
**
** Returns:
**
**	OK - location added
**	FAIL - ME error
**
** History:
**	10-Sept-2009 (hanje04)
**	    Created.
*/
STATUS
GIPAddDBLoc(instance *inst, i4 type, const char *path)
{
    auxloc      *locptr=inst->datalocs;
    auxloc      **pptr=NULL;
    STATUS	mestat;

    while ( locptr != NULL && locptr->next_loc != NULL )
                locptr = locptr->next_loc ;
    pptr = locptr ? &locptr->next_loc : & inst->datalocs;

    /* allocate memory for new location */
    *pptr = (auxloc *)MEreqmem( inst->memtag,
                                    sizeof( auxloc ),
                                    TRUE,
                                    &mestat );
    locptr=*pptr;

    if ( mestat != OK )
	return( mestat );

            /* fill values */
    locptr->idx = type ;
    STlcopy( path, locptr->loc, MAX_LOC-1 ) ;
    inst->numdlocs++;

    DBG_PRINT( "%s added as %s location for %s instance\n",
                            locptr->loc,
                            dblocations[type]->name,
                            inst->instance_name );
    return(OK);
}

/*
** Name: GIPAddLogLoc()
**
** Description:
**
**	Add location to list of tx log locations defined for a given
**	instance (instance.loglocs)
**
** Inputs:
**	
**	instance	*inst - Instance structure to update
**	i4		*type - Location type (0 = primary, 1 = dual)
**	const char	*path - Path to add
**
** Outputs:
**
**	None
**
** Returns:
**
**	OK - location added
**	FAIL - ME error
**
** History:
**	14-Sept-2009 (hanje04)
**	    Created.
*/
STATUS
GIPAddLogLoc(instance *inst, i4 type, const char *path)
{
    auxloc      *locptr=inst->loglocs;
    auxloc      **pptr=NULL;
    STATUS	mestat;

    while ( locptr != NULL && locptr->next_loc != NULL )
                locptr = locptr->next_loc ;
    pptr = locptr ? &locptr->next_loc : & inst->datalocs;

    /* allocate memory for new location */
    *pptr = (auxloc *)MEreqmem( inst->memtag,
                                    sizeof( auxloc ),
                                    TRUE,
                                    &mestat );
    locptr=*pptr;

    if ( mestat != OK )
	return( mestat );

            /* fill values */
    locptr->idx = type ;
    STlcopy( path, locptr->loc, MAX_LOC-1 ) ;
    inst->numllocs++;

    DBG_PRINT( "%s added as %s log location for %s instance\n",
                            locptr->loc,
                            (type == 0) ? "primary" : "dual",
                            inst->instance_name );
    return(OK);
}

/*
** Name: GIPnextPow2()
**
** Description:
**
**	Find the next power of 2 for any given number, taken from:
**	http://graphics.stanford.edu/~seander/bithacks.html (public domain)
**
** Inputs:
**	
**	i4		num - Number to round up
**
** Outputs:
**
**	None
**
** Returns:
**
**	Next power of 2 on success
**
** History:
**	27-May-2010 (hanje04)
**	    SIR 123791
**	    Created.
*/
i4
GIPnextPow2(i4  num)
{
    
    num--;
    num |= num >> 1;
    num |= num >> 2;
    num |= num >> 4;
    num |= num >> 8;
    num |= num >> 16;
    num++;
 
   return(num);
}
/*
** ATTENTION:
**	 only static functions below this line
*/

# endif /* xCL_GTK_EXISTS */
