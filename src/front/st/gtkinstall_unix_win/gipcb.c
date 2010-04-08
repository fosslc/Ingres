/*
** Copyright 2006 Ingres Corporation. All rights reserved 
*/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

# include <compat.h>
# include <gl.h>
# include <cm.h>
# include <st.h>
# include <lo.h>
# include <pc.h>
# include <nm.h>

# ifdef xCL_GTK_EXISTS

# include <gtk/gtk.h>

# include <gip.h>
# include <gipinterf.h>
# include <gipsup.h>
# include <gipcb.h>
# include <gipdata.h>
# include <giputil.h>
				
/*
** Name: gipcb.c
**
** Description:
**
**      Module for Callback functions for GTK GUI interface to both
**	the Linux GUI installer and the Ingres Package manager.
**
** History:
**
**	09-Oct-2006 (hanje04)
**	    SIR 116877
**          Created.
**	25-Oct-2006 (hanje04)
**	    SIR 116877
**	    Fix up screen transitions across upgrade/new install modes.
**	    Move loading of Charset and Timzeone dialogs to setup in
**	    individual binaries.
**	    Add entry for II_GROUPID in GUI.
**	26-Oct-2006 (hanje04)
**	    SIR 116877
**	    Make sure the first entry is selected after the instances
**	    are loaded into InstanceTreeView in on_modify_instance_clicked()
**	    Merge the callbacks for New Install, Upgrade and Modify.
**	06-Nov-2006 (hanje04)
**	    SIR 116877
**	    Make sure code only gets built on platforms that can build it.
**	24-Nov-2006 (hanje04)
**	    SIR 116877
**	    Add validation to all user "entry" fields such that the user cannot
**	    proceed to the next pane if any of the entries on current pane are
**	    invalid.
**	    Make sure the "locale popup" only closes if an entry has been
**	    selected. More specifically make sure it doesn't close if the
**	    expander containing a previously selected item is closed.
**	    Force log file size spinner to validate the entry and add
**	    slider.
**      27-Nov-2006 (hanje04)
**          SIR 116877
**          Modify interface to browse_location_gtk22(), to allow directory
**	    selection widget to default to the entered value if it exists.
**	    Also improve path validation for database locations as they
**	    need to exist for the install to complete succesfully.
**	    Add callback function for selecting the DOC package.
**	07-Dec-2006 (hanje04)
**	    BUG 117278
**	    SD 112944
**	    When DBMS package is deselected point stage_names at
**	    adv_nodbms_stages[] so that the DBMS config panes are skipped.
**	    Add DBMS config panes to MA_UPGRADE mode when the DBMS package 
**	    is being added to an existing instance.
**	    Conditionalize which set of location Widgets are accessed by:
**
**		on_basic_install_checked()
**		on_adv_install_clicked()
**		on_ii_system_changed()
**		on_checkdbmspkg_toggled()
**		on_use_iisys_default_toggled()
**		browse_location_gtk22()
**		on_browse_*_clicked() (One for each location browse button)
**		get_selected_* (One for each location)
**		on_bkp_log_enabled_toggled()
**		update_location_entry()
**		on_primary_log_file_loc_changed()
**		on_bkp_log_file_loc_changed()
**		on_ii_system_entry_changed()
**		on_ug_checkdbmspkg_toggled()
**
**	    Also replace PATH_MAX with MAX_LOC.
**	14-Dec-2006 (hanje04)
**	    BUG 117330
**	    Disable installation of doc package with GUI to avoid bug
**	    117330 for beta. Actually problem will be addressed for refresh
**	16-Feb-2007 (hanje04)
**	    BUG 117700
**	    SD 113367
**	    For upgrade an modify, check the instance we are operating on
**	    isn't running. If it offer to shut it down.
**	22-Feb-2007 (hanje04)
**	    SIR 116911
**	    Replace LOisfull() with GIPpathFull() as a cross platform way of
**	    validating full paths. LOisfull() only works for the platform the 
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
**	01-Mar-2007 (hanje04)
**	    SD 115790
**	    BUG 117809
**	    Make sure "Remove all files" is unchecked when "Remove all
**	    components is unchecked. Reformat associated warning message
**	    too.
**	01-March-2007 (hanje04)
**	    SD 115915
**	    BUG 117814
**	    Re-initialize database and log location fields when "New
**	    Install" is clicked. Also force update of db and log locations
**	    when II_SYSTEM is updated. Callback functions are only called 
**	    when field are physically updated and this doesn't happen
**	    is the field already contains the string it is being updated with.
**	02-Mar-2007 (hanje04)
**	    SD 115856
**	    BUG 117818
**	    Disable location creation from browse dialog for all locations 
**	    apart from II_SYSTEM. This is to try and prevent the installation
**	    of the DBMS package failing because the created directory doesn't
**	    have write permissions for the instance owner.
**	    Also fix directory selection in browse window.
**	13-Mar-2007 (hanje04)
**	    SD 115856
**	    BUG 117818
**	    Break out of update action if new II_SYSTEM value is invalid.
**	    Otherwise Next button gets re-enabled by location validation.
**      15-Mar-2007 (hanje04)
**	    Add get_selected_RF_filename() to save off response file once
**	    location is chosen by user.
**	20-Mar-2007 (hanje04)
**	    Bug 117958
**	    SD 116502
**	    In on_modify_instnace_clicked(), initialize array entry counter to 
**	    0 and not 1 otherwise we miss the first entry. DOH!
**      27-Mar-2007 (hanje04)
**          BUG 118008
**          SD 116528
**          Always display instance selection pane even when there is only
**          one instance. Otherwise, users are not given the option not to
**          upgrade their user databases.
**	26-Apr-2007 (hanje04)
**	    SD 116488
**	    BUG 118206
**	    Explicitly title pop-up windows so they always have one. Some
**	    window manager do not honor the implicit ones for message
**	    dialogs.
**	23-Apr-2007 (hanje04)
**	    BUG 118090
**	    SD 116383
**	    Update warning dialog displayed when "Remove all files" is selected
**	    so it displays all the locations being removed, not just II_SYSTEM.
**	02-Aug-2007 (hanje04)
**	    BUG 118881
**	    Don't allow directory creation from browse dialog for II_SYSTEM
**	    because directory created will have permissions of 644.
**	08-Oct-2008 (hanje04)
**	    Bug 121004
**	    SD 130670
**	    Validate locale settings when timezone and charset fields are
**	    updated by user.
**	08-Jul-2009 (hanje04)
**	    SIR 122309
**	    Add callback function for configuration type dialog,
**	    on_ing_config_radio_changed()
**	11-Sep-2009 (hanje04)
**	    BUG 122571
**	    Reduce recursion by ensuring modify/remove handlers only
**	    do work when button are clicked on.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of nm.h and static declaration of
**	    update_location_entry to eliminate gcc 4.3 warnings.
**   
*/

static STATUS update_location_entry( i4 locidx, GtkEntry *entry );

gint
delete_event	(GtkButton       *button,
                  gpointer         user_data)
{
    GtkWidget	*quit_prompt;
    GtkWidget	*quit_label;
	
    DBG_PRINT("delete_event called\n");

# define QUIT_DIALOG_TITLE "Quit?"
    /* create quit prompt to confirm user's decision to quit app */
    quit_prompt = gtk_dialog_new_with_buttons(
					QUIT_DIALOG_TITLE,
					GTK_WINDOW( IngresInstall ),
					GTK_DIALOG_MODAL |
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_NO,
					GTK_RESPONSE_REJECT,
					GTK_STOCK_YES,
					GTK_RESPONSE_ACCEPT,
					NULL); 

    /* add a separator above the buttons */
    gtk_dialog_set_has_separator( GTK_DIALOG( quit_prompt ), TRUE ); 

# define QUIT_DIALOG_LABEL "Are you sure you want to quit?"
    /* Add some text */
    quit_label = gtk_label_new( QUIT_DIALOG_LABEL );
    gtk_widget_show( quit_label );
    gtk_box_pack_start( GTK_BOX( GTK_DIALOG( quit_prompt )->vbox ),
			quit_label,
			FALSE,
			FALSE,
			0);
    gtk_misc_set_alignment( GTK_MISC(quit_label), 0.5, 0.5);
    gtk_misc_set_padding( GTK_MISC(quit_label), 5, 0);
    
    /* prompt the user to confirm quit */
    if ( gtk_dialog_run( GTK_DIALOG( quit_prompt ) ) == GTK_RESPONSE_ACCEPT )
	gtk_main_quit(); /* confirmed, exit app */
    else
	gtk_widget_destroy( quit_prompt ); /* canceled, continue */
  
    return( TRUE );

	
}

void
on_basic_install_checked               (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget	*adv_config;
    GtkWidget	*adv_db_config;
	
    DBG_PRINT("on_basic_install_checked called\n");
    instmode=BASIC_INSTALL;
    /* point stage_names to the right array */
    stage_names=basic_stages;	
	
    /* hide the advanced progress items */
    adv_config=lookup_widget(IngresInstall, "adv_config");
    adv_db_config = lookup_widget( IngresInstall, "adv_db_config" );
    gtk_widget_hide(adv_config);
    adv_config=lookup_widget(IngresInstall, "instance_name_box");
    gtk_widget_hide(adv_config);
    gtk_widget_hide( adv_db_config );
}


void
on_adv_install_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget	*adv_config;
    GtkWidget	*adv_db_config;
	
    DBG_PRINT("on_adv_install_clicked called\n");
    instmode=ADVANCED_INSTALL;
    /* point stage_names to the right array */
    stage_names=adv_stages;
	
    /* activate advanced progress items */
    adv_config=lookup_widget(IngresInstall, "adv_config");
    adv_db_config = lookup_widget( IngresInstall, "adv_db_config" );
    gtk_widget_show(adv_config);
    adv_config=lookup_widget(IngresInstall, "instance_name_box");
    gtk_widget_show(adv_config);

    /* hide/show DBMS config panes in train */
    adv_db_config = lookup_widget( IngresInstall, "adv_db_config" );
    if ( pkgs_to_install & PKG_DBMS )
	gtk_widget_show( adv_db_config );
    else
	gtk_widget_hide( adv_db_config );
}

void
on_iisystem_changed                    (GtkEditable     *editable,
                                        gpointer         user_data)
{
    GtkWidget	*iisys_default_checked;
    GtkWidget	*iisys_log_default_checked;
    GtkWidget	*log_loc;
    GtkWidget	*widget=NULL;
	
    char	*iisystem_ptr;
	
    /* 
    ** update the value for II_SYSTEM
    ** if we're using II_SYSTEM for the DB locations we need to update
    ** those too.
    */
    if ( ug_mode & UM_INST )
    {
	iisys_default_checked = lookup_widget( IngresInstall,
					"db_loc_iisys_default" );
	iisys_log_default_checked = lookup_widget( IngresInstall,
					"log_loc_iisys_default" );
	iisystem_ptr = iisystem.path;
    }
    else
    {
	iisys_default_checked = lookup_widget( IngresInstall,
					"mod_db_loc_iisys_default" );
	iisys_log_default_checked = lookup_widget( IngresInstall,
					"log_loc_iisys_default" );
	iisystem_ptr = selected_instance->inst_loc;
    }

    DBG_PRINT( "on_iisystem_changed called, II_SYSTEM=%s\n", iisystem_ptr );

    if ( gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON( iisys_default_checked ) ) == TRUE )
    {
	/* update the II_SYSTEM label */
	widget=lookup_widget(IngresInstall, ug_mode & UM_INST ?
				"IISystemLabel" : "mod_dbloc_iisystem" );
	gtk_label_set_text(GTK_LABEL(widget), iisystem_ptr);

	/* there are two labels in modify mode */
	if ( ug_mode & UM_MOD )
	{
	    widget=lookup_widget(IngresInstall, "mod_logloc_iisystem" );
	    gtk_label_set_text(GTK_LABEL(widget), iisystem_ptr);
	}

	/* II_DATABASE */
	widget=lookup_widget(IngresInstall, ug_mode & UM_INST ?
				"ii_database" : "mod_ii_database" );
	if ( STcompare( gtk_entry_get_text( GTK_ENTRY(widget) ),
						iisystem_ptr ) )
	    gtk_entry_set_text(GTK_ENTRY(widget), iisystem_ptr);
	else
	    on_ii_database_changed( GTK_EDITABLE( widget ), NULL );
	
	/* II_CHECKPOINT */
	widget=lookup_widget(IngresInstall, ug_mode & UM_INST ?
				"ii_checkpoint" : "mod_ii_checkpoint" );
	if ( STcompare( gtk_entry_get_text( GTK_ENTRY(widget) ),
						iisystem_ptr ) )
	    gtk_entry_set_text(GTK_ENTRY(widget), iisystem_ptr);
	else
	    on_ii_checkpoint_changed( GTK_EDITABLE( widget ), NULL );
	
	/* II_JOURNAL */
	widget=lookup_widget(IngresInstall, ug_mode & UM_INST ?
				"ii_journal" : "mod_ii_journal" );
	if ( STcompare( gtk_entry_get_text( GTK_ENTRY(widget) ),
						iisystem_ptr ) )
	    gtk_entry_set_text(GTK_ENTRY(widget), iisystem_ptr);
	else
	    on_ii_journal_changed( GTK_EDITABLE( widget ), NULL );
	
	/* II_WORK */
	widget=lookup_widget(IngresInstall, ug_mode & UM_INST ?
				"ii_work" : "mod_ii_work" );
	if ( STcompare( gtk_entry_get_text( GTK_ENTRY(widget) ),
						iisystem_ptr ) )
	    gtk_entry_set_text(GTK_ENTRY(widget), iisystem_ptr);
	else
	    on_ii_work_changed( GTK_EDITABLE( widget ), NULL );
	
	/* II_DUMP */
	widget=lookup_widget(IngresInstall, ug_mode & UM_INST ?
				"ii_dump" : "mod_ii_dump" );
	if ( STcompare( gtk_entry_get_text( GTK_ENTRY(widget) ),
						iisystem_ptr ) )
	    gtk_entry_set_text(GTK_ENTRY(widget), iisystem_ptr);
	else
	    on_ii_dump_changed( GTK_EDITABLE( widget ), NULL );
	
    }

    if ( gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON( iisys_log_default_checked ) ) == TRUE )
    {
	/* ...and the same for the log file locations */
	widget=lookup_widget(IngresInstall, ug_mode & UM_INST ?
				"primary_log_file_loc" : "mod_primary_log_file_loc");
	if ( STcompare( gtk_entry_get_text( GTK_ENTRY(widget) ),
						iisystem_ptr ) )
	    gtk_entry_set_text(GTK_ENTRY(widget), iisystem_ptr);
	else
	    on_primary_log_file_loc_changed( GTK_EDITABLE( widget ), NULL );
    }
}

void
on_typical_inst_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget	*component_list;
	
    /* only care if we've been clicked on */
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ) ) == FALSE )
	return;

    DBG_PRINT("on_typical_inst_clicked called\n");
    pkgs_to_install=TYPICAL_SERVER;
    /* set package check buttons correctly */
    set_inst_pkg_check_buttons( );
	
    /* lookup the package containers and grey then */
    component_list=lookup_widget(IngresInstall, "complist1");
    gtk_widget_set_sensitive( component_list, FALSE );
    component_list=lookup_widget(IngresInstall, "complist2");
    gtk_widget_set_sensitive( component_list, FALSE );	
}

void
on_typical_client_inst_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget	*component_list;
	
    /* only care if we've been clicked on */
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ) ) == FALSE )
	return;

    DBG_PRINT("on_typical_client_inst_clicked called\n");
    pkgs_to_install=TYPICAL_CLIENT;
    /* set package check buttons correctly */
    set_inst_pkg_check_buttons( );
	
    /* lookup the package containers and grey then */
    component_list=lookup_widget(IngresInstall, "complist1");
    gtk_widget_set_sensitive( component_list, FALSE );
    component_list=lookup_widget(IngresInstall, "complist2");
    gtk_widget_set_sensitive( component_list, FALSE );	
}

void
on_full_install_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget	*component_list;

    /* only care if we've been clicked on */
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ) ) == FALSE )
	return;

    DBG_PRINT("on_full_install_clicked called\n");
    /* set package list */
    pkgs_to_install=FULL_INSTALL;
    /* set package check buttons correctly */
    set_inst_pkg_check_buttons();	

    /* lookup the package containers and grey then */
    component_list=lookup_widget(IngresInstall, "complist1");
    gtk_widget_set_sensitive( component_list, FALSE );
    component_list=lookup_widget(IngresInstall, "complist2");
    gtk_widget_set_sensitive( component_list, FALSE );	
}


void
on_custom_install_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *component_list;
	
    /* only care if we've been clicked on */
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ) ) == FALSE )
	return;

    DBG_PRINT("on_custom_install_clicked called\n");
	
    /* lookup the package containers and ungrey then */
    component_list=lookup_widget(IngresInstall, "complist1");
    gtk_widget_set_sensitive( component_list, TRUE );
    component_list=lookup_widget(IngresInstall, "complist2");
    gtk_widget_set_sensitive( component_list, TRUE );
    
    /*set package check buttons correctly */
    set_inst_pkg_check_buttons();
}

void
set_inst_pkg_check_buttons()
{
    GtkWidget *checkbutton;
    
    DBG_PRINT("set_pkg_check_buttons called\n");
    /* 
    ** Lookup the check buttons for the packages
    ** and set them accordingly
    */
    checkbutton=lookup_widget(IngresInstall, "checkdbmspkg");
    gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_DBMS );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), 
					new_pkgs_info.pkgs &
					pkgs_to_install&PKG_DBMS);

    checkbutton=lookup_widget(IngresInstall, "checknetpkg");
    gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_NET );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),
					new_pkgs_info.pkgs &
					pkgs_to_install&PKG_NET);
    
    checkbutton=lookup_widget(IngresInstall, "checkodbcpkg");
    gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_ODBC );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),
					new_pkgs_info.pkgs &
					pkgs_to_install&PKG_ODBC);
    
    checkbutton=lookup_widget(IngresInstall, "checkreppkg");
    gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_REP );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), 
					new_pkgs_info.pkgs &
					pkgs_to_install&PKG_REP);
    
    checkbutton=lookup_widget(IngresInstall, "checkabfpkg");
    gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_ABF );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),
					new_pkgs_info.pkgs &
					pkgs_to_install&PKG_ABF);
    
    checkbutton=lookup_widget(IngresInstall, "checkstarpkg");
    gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_STAR );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),
					new_pkgs_info.pkgs &
					pkgs_to_install&PKG_STAR);
    
    checkbutton=lookup_widget(IngresInstall, "checkdocpkg");
    
    
    /* only allow (de)selection of doc package for unrenamed release */
    if ( inst_state & UM_RENAME )
	gtk_widget_set_sensitive( checkbutton, FALSE );
    else
    {
	gtk_widget_set_sensitive( checkbutton, new_pkgs_info.pkgs & PKG_DOC );
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),
     					new_pkgs_info.pkgs &
     					pkgs_to_install&PKG_DOC);
    }
    
} 

void
on_checkdbmspkg_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    GtkWidget	*adv_db_config;
    GtkWidget	*db_loc_frame;
    GtkWidget	*db_ansi_config;


    /* toggle the package bit with the button */
    gtk_toggle_button_get_active(togglebutton) ?
	inst_pkg_sel( DBMS ) : inst_pkg_rmv( DBMS ) ;

    /* may need to skip panes in advanced mode */
    if ( instmode == ADVANCED_INSTALL )
    {
	/* skip DBMS config panes if DBMS not selected */
	stage_names = pkgs_to_install & PKG_DBMS ? 
			adv_stages : adv_nodbms_stages ;
	
	/* hide/show DBMS config panes in train */
	db_loc_frame = lookup_widget( IngresInstall, "db_loc_frame" );
	gtk_widget_set_sensitive( db_loc_frame, pkgs_to_install & PKG_DBMS ?
						TRUE : FALSE );

	db_ansi_config = lookup_widget( IngresInstall, "db_ansi_config" );
	gtk_widget_set_sensitive( db_ansi_config, pkgs_to_install & PKG_DBMS ?
						TRUE : FALSE );

	adv_db_config = lookup_widget( IngresInstall, "adv_db_config" );
	if ( pkgs_to_install & PKG_DBMS )
	    gtk_widget_show( adv_db_config );
	else
	    gtk_widget_hide( adv_db_config );
    }
}


void
on_checknetpkg_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* toggle the package bit with the button */
    gtk_toggle_button_get_active(togglebutton) ?
	inst_pkg_sel( NET ) : inst_pkg_rmv( NET ) ;
}


void
on_checkodbcpkg_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{	
    /* toggle the package bit with the button */
    gtk_toggle_button_get_active(togglebutton) ?
	inst_pkg_sel( ODBC ) : inst_pkg_rmv( ODBC ) ;
}


void
on_checkreppkg_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* toggle the package bit with the button */
    gtk_toggle_button_get_active(togglebutton) ?
	inst_pkg_sel( REP ) : inst_pkg_rmv( REP ) ;
}


void
on_checkabfpkg_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* toggle the package bit with the button */
    gtk_toggle_button_get_active(togglebutton) ?
	inst_pkg_sel( ABF ) : inst_pkg_rmv( ABF ) ;
}


void
on_checkstarpkg_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* toggle the package bit with the button */
    gtk_toggle_button_get_active(togglebutton) ?
	inst_pkg_sel( STAR ) : inst_pkg_rmv( STAR ) ;
}


void
on_checkjdbcpkg_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* toggle the package bit with the button */
    gtk_toggle_button_get_active(togglebutton) ?
	inst_pkg_sel( JDBC ) : inst_pkg_rmv( JDBC ) ;
}


void
on_checkicepkg_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* toggle the package bit with the button */
    gtk_toggle_button_get_active(togglebutton) ?
	inst_pkg_sel( ICE ) : inst_pkg_rmv( ICE ) ;
}

void
on_checkdocpkg_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* toggle the package bit with the button */
    gtk_toggle_button_get_active(togglebutton) ?
	inst_pkg_sel( DOC ) : inst_pkg_rmv( DOC ) ;
}

void
on_use_iisys_default_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    GtkWidget *db_loc_table;
    GtkWidget *ii_system_entry;
	
    if ( ug_mode & UM_INST ) 
    {
	DBG_PRINT( "Use II_SYSTEM as default (New Install) toggled\n" );
	ii_system_entry=lookup_widget(IngresInstall, "ii_system_entry");
	db_loc_table=lookup_widget(IngresInstall, "db_loc_table");
	
    }
    else
    {
	DBG_PRINT( "Use II_SYSTEM as default (Modify) toggled\n" );
	ii_system_entry=lookup_widget(IngresInstall, "mod_iisystem_entry");
	db_loc_table=lookup_widget(IngresInstall, "mod_db_loc_table");
    }

    /* 
    ** If we're toggled on force update of all necessary fields 
    ** by calling on_iisystem_changed.
    */
    if ( gtk_toggle_button_get_active(togglebutton) == TRUE )
    {	
	    /* update the rest of the location fields */
	    on_iisystem_changed(GTK_EDITABLE(ii_system_entry), user_data);
		
	    /* make all locations grey */
	    gtk_widget_set_sensitive( db_loc_table, FALSE );
    }
    else
    {
	    update_location_entry( INST_II_SYSTEM,
				GTK_ENTRY( ii_system_entry ) );

	    /* make all locations active */
	    gtk_widget_set_sensitive( db_loc_table, TRUE );
    }
}

void
on_use_iisys_log_default_toggled           (GtkToggleButton *togglebutton,
                               		    gpointer         user_data)
{
    GtkWidget *log_loc_table;
    GtkWidget *log_file_loc;
    GtkWidget *ii_system_entry;
	
    if ( ug_mode & UM_INST ) 
    {
	DBG_PRINT( "Use II_SYSTEM as log (New Install) toggled\n" );
	ii_system_entry=lookup_widget(IngresInstall, "ii_system_entry");
	log_loc_table=lookup_widget(IngresInstall, "log_loc_table");
	log_file_loc=lookup_widget(IngresInstall, "primary_log_file_loc");
	
    }
    else
    {
	DBG_PRINT( "Use II_SYSTEM as log (Modify) toggled\n" );
	ii_system_entry=lookup_widget(IngresInstall, "mod_iisystem_entry");
	log_loc_table=lookup_widget(IngresInstall, "mod_log_loc_table");
	log_file_loc=lookup_widget(IngresInstall, "mod_primary_log_file_loc");
    }

    /* 
    ** If we're toggled on, update the primary log field.
    */
    if ( gtk_toggle_button_get_active(togglebutton) == TRUE )
    {	
	    /* update the location field */
	    gtk_entry_set_text( GTK_ENTRY( log_file_loc ),
				gtk_entry_get_text( GTK_ENTRY(
					ii_system_entry ) ) );
		
	    /* make log location grey */
	    gtk_widget_set_sensitive( log_loc_table, FALSE );
    }
    else
	/* make all locations active */
	gtk_widget_set_sensitive( log_loc_table, TRUE );
}
void
browse_location( gchar *location_entry_name )
{

    GtkWidget *dir_browser;

    DBG_PRINT("on_browse_clicked called\n");
    /* Create a new file selection widget */
# define	BROWSE_DIALOG_TITLE "Choose a location"
    /*
    ** GtkFileChooser widgets were introduced with v 2.6, we currently
    ** have to support 2.2 and up. Comment out code for now.
    ** dir_browser = gtk_file_chooser_dialog_new( BROWSE_DIALOG_TITLE,
    **					GTK_WINDOW(IngresInstall),
    **					GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
    **					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    **				      	GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
    **				      	NULL);
    */
   

    /* show the dialog */
    gtk_widget_show(dir_browser);

    /* 
    ** GtkFileChooser widgets were introduced with v 2.6, we currently
    ** have to support 2.2 and up. Comment out code for now.
    ** wait for user response
    if (gtk_dialog_run (GTK_DIALOG (dir_browser)) == GTK_RESPONSE_ACCEPT)
    {	

	char *dirname;
	GtkWidget *location_entry;

	/* get user specified location
	dirname = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(
							dir_browser ) );

	/* update the text fields which will update II_SYSTEM
	location_entry = lookup_widget(IngresInstall, location_entry_name ); 
	gtk_entry_set_text( GTK_ENTRY( location_entry ), dirname );
	g_free (dirname);	
    }

    gtk_widget_destroy(dir_browser);
    */
}

void
browse_location_gtk22				( GCallback callback,
						GtkWidget *location_entry,
						bool crloc )
{
    GtkWidget *dir_browser;
    gchar	entry_path[MAX_LOC];
    gchar	*pathptr = entry_path;
    LOCATION	pathloc;

    /* create file selection dialog */
    dir_browser = gtk_file_selection_new( BROWSE_DIALOG_TITLE ); 

    /* make it look like we want */
    gtk_widget_hide( GTK_FILE_SELECTION( dir_browser )->fileop_del_file );
    gtk_widget_hide( GTK_FILE_SELECTION( dir_browser )->fileop_ren_file );

    /*
    ** Work around for bug
    ** Only allow directory creation of requested by caller
    */
    if ( crloc == FALSE )
        gtk_widget_hide( GTK_FILE_SELECTION( dir_browser )->fileop_c_dir );

    gtk_widget_hide( GTK_FILE_SELECTION( dir_browser )->selection_entry );
    gtk_widget_set_sensitive( GTK_FILE_SELECTION( dir_browser )->file_list,
				FALSE );

    /* get path from entry field */
    STlcopy( (char *)gtk_entry_get_text( GTK_ENTRY( location_entry ) ),
		entry_path,
		MAX_LOC );

    DBG_PRINT( "Pre LO path: %s", entry_path );
    /* If the path exists, use it as the default */
    LOfroms( PATH, entry_path, &pathloc );
    if ( LOexist( &pathloc ) == OK )
    {
	/*
	** find the end of the location string and append / if we need to.
	** This makes the dialog correctly set the default selection
	*/
	while( *pathptr != '\0' )
	    pathptr++;
    
	pathptr--;
	if ( *pathptr != '/' );
	    STcat( entry_path, "/" );

	DBG_PRINT( "Location: %s\nexists, using as default\n", entry_path );
	gtk_file_selection_set_filename( GTK_FILE_SELECTION( dir_browser ), 
					    entry_path );
    }

    /* Connect selection handler */
    g_signal_connect( GTK_FILE_SELECTION(dir_browser)->ok_button,
			"clicked",
			G_CALLBACK( callback ), 
			dir_browser );

    /* Make sure dialog closes when buttons are clicked */
    g_signal_connect_swapped (GTK_FILE_SELECTION (dir_browser)->ok_button,
                            "clicked",
                            G_CALLBACK (gtk_widget_destroy), 
                            dir_browser);

    g_signal_connect_swapped (GTK_FILE_SELECTION (dir_browser)->cancel_button,
                             "clicked",
                             G_CALLBACK (gtk_widget_destroy),
                             dir_browser); 

    /* show the dialog */
    gtk_widget_show(dir_browser);

}

void
on_browse_iisys_clicked		(GtkButton       *button,
                                gpointer         user_data)
{
    GtkWidget *location_entry;

    /* get current value and pass it the browser as the default*/
    location_entry = lookup_widget(IngresInstall, "ii_system_entry" );

    /*
    ** not supported until GTK 2.6
    ** browse_location( "ii_system_entry" );
    */
    browse_location_gtk22( G_CALLBACK( get_selected_ii_system ),
				location_entry,
				FALSE );
}

void
get_selected_ii_system		( GtkWidget *widget,
				gpointer user_data )
{
    GtkWidget *location_entry;
    GtkWidget *file_selector;
	
    file_selector = GTK_WIDGET( user_data );

    /* update the text fields which will update II_SYSTEM */
    location_entry = lookup_widget(IngresInstall, "ii_system_entry" ); 
    gtk_entry_set_text( GTK_ENTRY( location_entry ),
			    gtk_file_selection_get_filename(
				GTK_FILE_SELECTION(  file_selector ) ) );

}

void
on_browse_iidb_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *location_entry;

    /* get current value and pass it the browser as the default*/
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"ii_database" : "mod_ii_database" );

    /*
    ** not supported until GTK 2.6
    ** browse_location( "ii_system_entry" );
    */
    browse_location_gtk22( G_CALLBACK( get_selected_database ),
				location_entry,
				FALSE );
}

void
get_selected_database		( GtkWidget *widget,
					gpointer user_data )
{
    GtkWidget *location_entry;
    GtkWidget *file_selector;
	
    file_selector = GTK_WIDGET( user_data );

    /* update the text fields which will update II_SYSTEM */
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"ii_database" : "mod_ii_database" ); 
    gtk_entry_set_text( GTK_ENTRY( location_entry ),
			    gtk_file_selection_get_filename(
				GTK_FILE_SELECTION(  file_selector ) ) );

}

void
on_browse_ckp_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *location_entry;

    /* get current value and pass it the browser as the default*/
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"ii_checkpoint" : "mod_ii_checkpoint" );

    /*
    ** not supported until GTK 2.6
    ** browse_location( "ii_system_entry" );
    */
    browse_location_gtk22( G_CALLBACK( get_selected_checkpoint ),
				location_entry,
				FALSE );
}

void
get_selected_checkpoint		( GtkWidget *widget,
					gpointer user_data )
{
    GtkWidget *location_entry;
    GtkWidget *file_selector;
	
    file_selector = GTK_WIDGET( user_data );

    /* update the text fields which will update II_SYSTEM */
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"ii_checkpoint" : "mod_ii_checkpoint" );
    gtk_entry_set_text( GTK_ENTRY( location_entry ),
			    gtk_file_selection_get_filename(
				GTK_FILE_SELECTION(  file_selector ) ) );

}

void
on_browse_jnl_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *location_entry;

    /* get current value and pass it the browser as the default*/
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"ii_journal" : "mod_ii_journal" );

    /*
    ** not supported until GTK 2.6
    ** browse_location( "ii_system_entry" );
    */
    browse_location_gtk22( G_CALLBACK( get_selected_journal ),
				location_entry,
				FALSE );
}

void
get_selected_journal		( GtkWidget *widget,
					gpointer user_data )
{
    GtkWidget *location_entry;
    GtkWidget *file_selector;
	
    file_selector = GTK_WIDGET( user_data );

    /* update the text fields which will update II_SYSTEM */
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"ii_journal" : "mod_ii_journal" );
    gtk_entry_set_text( GTK_ENTRY( location_entry ),
			    gtk_file_selection_get_filename(
				GTK_FILE_SELECTION(  file_selector ) ) );

}


void
on_browse_work_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *location_entry;

    /* get current value and pass it the browser as the default*/
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"ii_work" : "mod_ii_work" );

    /*
    ** not supported until GTK 2.6
    ** browse_location( "ii_work" );
    */
    browse_location_gtk22( G_CALLBACK( get_selected_work ),
				location_entry,
				FALSE );
}

void
get_selected_work		( GtkWidget *widget,
					gpointer user_data )
{
    GtkWidget *location_entry;
    GtkWidget *file_selector;
	
    file_selector = GTK_WIDGET( user_data );

    /* update the text fields which will update II_SYSTEM */
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"ii_work" : "mod_ii_work" );
    gtk_entry_set_text( GTK_ENTRY( location_entry ),
			    gtk_file_selection_get_filename(
				GTK_FILE_SELECTION(  file_selector ) ) );

}


void
on_browse_dump_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *location_entry;

    /* get current value and pass it the browser as the default*/
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"ii_dump" : "mod_ii_dump" );

    /*
    ** not supported until GTK 2.6
    ** browse_location( "ii_dump" );
    */
    browse_location_gtk22( G_CALLBACK( get_selected_dump ),
				location_entry,
				FALSE );
}

void
get_selected_dump		( GtkWidget *widget,
					gpointer user_data )
{
    GtkWidget *location_entry;
    GtkWidget *file_selector;
	
    file_selector = GTK_WIDGET( user_data );

    /* update the text fields which will update II_SYSTEM */
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"ii_dump" : "mod_ii_dump" );
    gtk_entry_set_text( GTK_ENTRY( location_entry ),
			    gtk_file_selection_get_filename(
				GTK_FILE_SELECTION(  file_selector ) ) );

}


void
on_browse_plog_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *location_entry;

    /* get current value and pass it the browser as the default*/
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"primary_log_file_loc" :
					"mod_ii_database" );

    /*
    ** not supported until GTK 2.6
    ** browse_location( "primary_log_file_loc" );
    */
    browse_location_gtk22( G_CALLBACK( get_selected_plog ),
				location_entry,
				FALSE );
}

void
get_selected_plog		( GtkWidget *widget,
					gpointer user_data )
{
    GtkWidget *location_entry;
    GtkWidget *file_selector;
	
    file_selector = GTK_WIDGET( user_data );

    /* update the text fields which will update II_SYSTEM */
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"primary_log_file_loc" :
					"mod_ii_database" );

    gtk_entry_set_text( GTK_ENTRY( location_entry ),
			    gtk_file_selection_get_filename(
				GTK_FILE_SELECTION(  file_selector ) ) );

}


void
on_browse_blog_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *location_entry;

    /* get current value and pass it the browser as the default*/
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"bkp_log_file_loc" :
					"mod_bkp_log_file_loc" );

    /*
    ** not supported until GTK 2.6
    ** browse_location( "bkp_log_file_loc" );
    */
    browse_location_gtk22( G_CALLBACK( get_selected_blog ),
				location_entry,
				FALSE );
}

void
get_selected_blog		( GtkWidget *widget,
					gpointer user_data )
{
    GtkWidget *location_entry;
    GtkWidget *file_selector;
	
    file_selector = GTK_WIDGET( user_data );

    /* update the text fields which will update II_SYSTEM */
    location_entry = lookup_widget(IngresInstall, ug_mode & UM_INST ?
					"bkp_log_file_loc" :
					"mod_bkp_log_file_loc" );
    gtk_entry_set_text( GTK_ENTRY( location_entry ),
			    gtk_file_selection_get_filename(
				GTK_FILE_SELECTION(  file_selector ) ) );

}


void
on_bkp_log_enabled_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    GtkWidget *bkp_log_file_loc;
    GtkWidget *bkp_log_browse;
    GtkWidget *bkp_log_label;
    GtkWidget *primary_log_file_loc;
	
    if ( ug_mode & UM_INST )
    {
	bkp_log_file_loc=lookup_widget(IngresInstall, "bkp_log_file_loc");
	bkp_log_browse=lookup_widget(IngresInstall, "bkp_log_browse");
	bkp_log_label=lookup_widget(IngresInstall, "bkp_log_label");
	primary_log_file_loc=lookup_widget(IngresInstall,
						"primary_log_file_loc");
    }
    else
    {
	bkp_log_file_loc=lookup_widget(IngresInstall, "mod_bkp_log_file_loc");
	bkp_log_browse=lookup_widget(IngresInstall, "mod_bkp_log_browse");
	bkp_log_label=lookup_widget(IngresInstall, "mod_bkp_log_label");
	primary_log_file_loc=lookup_widget(IngresInstall,
						"mod_primary_log_file_loc");
    }
	
    if ( gtk_toggle_button_get_active(togglebutton) == TRUE )
    {
	/* Backup trans log enabled so show the fields */
	gtk_widget_set_sensitive(bkp_log_file_loc, TRUE);	
	gtk_widget_set_sensitive(bkp_log_browse, TRUE);
	gtk_widget_set_sensitive(bkp_log_label, TRUE);
	txlog_info.duallog=TRUE;
    }
    else
    {
	/* Backup trans log dissabled so hide the fields */
	gtk_widget_set_sensitive(bkp_log_file_loc, FALSE);	
	gtk_widget_set_sensitive(bkp_log_browse, FALSE);
	gtk_widget_set_sensitive(bkp_log_label, FALSE);
	txlog_info.duallog=FALSE;
    }

    /* fake an update to invoke validation */
    on_bkp_log_file_loc_changed( GTK_EDITABLE( bkp_log_file_loc ), NULL );
}

static STATUS
update_location_entry( i4 locidx, GtkEntry *entry )
{
    GtkWidget	*next_button;
    GtkWidget	*db_loc_iisys_default;
    char	*iisystem_ptr;
    i4		i = 1; /* loop counter, skip II_SYSTEM */
    LOCATION	dataloc; /* LO structure for validating entry */
		

    /* store new value for locations */
    STlcopy( gtk_entry_get_text( entry ),
		dblocations[locidx]->path,
		MAX_LOC );

    /*
    ** Don't update anything until we've validated the entry,
    ** If there is an invalid location, disable the next button
    ** and popup an error
    */
    next_button=lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"next_button":
						"ug_next_button" );
    iisystem_ptr = ug_mode & UM_INST ?
			iisystem.path : selected_instance->inst_loc ;

    /* first deal with II_SYSTEM as it's a bit different */
    LOfroms( PATH, iisystem_ptr, &dataloc );

    if ( ! GIPpathFull( &dataloc ) )
    {
	DBG_PRINT( "New path %s for location %s is not valid\n", 
		dblocations[locidx]->path,
		dblocations[locidx]->name );
	gtk_widget_set_sensitive( next_button, FALSE );
	return( FAIL );
    }
    else
	gtk_widget_set_sensitive( next_button, TRUE );

    /* validate all locations */
    GIPvalidateLocs( iisystem_ptr );
	
    return( OK );
}
void
on_ii_database_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{
    char	*iisystem_ptr;

    iisystem_ptr = ug_mode & UM_INST ? iisystem.path :
					selected_instance->inst_loc ;

    if ( update_location_entry( INST_II_DATABASE,
				GTK_ENTRY( editable ) ) == OK  )
	DBG_PRINT(
	"on_ii_database_changed called: II_SYSTEM=%s\n\t\tII_DATABASE=%s\n",
			iisystem_ptr, dblocations[INST_II_DATABASE]->path );
}

void
on_ii_checkpoint_changed               (GtkEditable     *editable,
                                        gpointer         user_data)
{
    char	*iisystem_ptr;

    iisystem_ptr = ug_mode & UM_INST ? iisystem.path :
					selected_instance->inst_loc ;

    if ( update_location_entry( INST_II_CHECKPOINT,
				GTK_ENTRY( editable ) ) == OK )
	DBG_PRINT(
	"on_ii_checkpoint_changed called: II_SYSTEM=%s\n\t\tII_CHECKPOINT=%s\n",
			iisystem_ptr, dblocations[INST_II_CHECKPOINT]->path );
}

void
on_ii_journal_changed                  (GtkEditable     *editable,
                                        gpointer         user_data)
{
    char	*iisystem_ptr;

    iisystem_ptr = ug_mode & UM_INST ? iisystem.path :
					selected_instance->inst_loc ;

    if ( update_location_entry( INST_II_JOURNAL,
				GTK_ENTRY( editable ) ) == OK )
	DBG_PRINT(
	"on_ii_journal_changed called: II_SYSTEM=%s\n\t\tII_JOURNAL=%s\n",
			iisystem_ptr, dblocations[INST_II_JOURNAL]->path );
}

void
on_ii_work_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
    char	*iisystem_ptr;

    iisystem_ptr = ug_mode & UM_INST ? iisystem.path :
					selected_instance->inst_loc ;

    if ( update_location_entry( INST_II_WORK,
				GTK_ENTRY( editable ) ) == OK )
	DBG_PRINT(
	"on_ii_work_changed called: II_SYSTEM=%s\n\t\tII_WORK=%s\n",
			iisystem_ptr, dblocations[INST_II_WORK]->path );
}

void
on_ii_dump_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
    char	*iisystem_ptr;

    iisystem_ptr = ug_mode & UM_INST ? iisystem.path :
					selected_instance->inst_loc ;

    if ( update_location_entry( INST_II_DUMP,
				GTK_ENTRY( editable ) ) == OK )
	DBG_PRINT(
	"on_ii_dump_changed called: II_SYSTEM=%s\n\t\tII_DUMP=%s\n",
			iisystem_ptr, dblocations[INST_II_DUMP]->path );
}

void
on_primary_log_file_loc_changed        (GtkEditable     *editable,
                                        gpointer         user_data)
{
    GtkWidget	*db_loc_iisys_default;
    char	*iisystem_ptr;
    LOCATION	logloc;

    /* make sure we're referencing the correct items */
    iisystem_ptr = ug_mode & UM_INST ?
			iisystem.path :
			selected_instance->inst_loc ;

    /* copy new value internally */
    STlcopy( gtk_entry_get_text(GTK_ENTRY(editable)),
		txlog_info.log_loc.path,
		MAX_LOC);
	
    /* check all log locations are valid */
    GIPvalidateLocs( iisystem_ptr );
	    
    DBG_PRINT(
    "on_primary_log_file_changed called: II_SYSTEM=%s\n\t\tlog_file_loc=%s\n",
		iisystem_ptr, gtk_entry_get_text(GTK_ENTRY(editable)) );

}

void
on_bkp_log_file_loc_changed            (GtkEditable     *editable,
                                        gpointer         user_data)
{
    GtkWidget	*next_button;
    LOCATION	logloc;
    char	*iisystem_ptr;

    iisystem_ptr = ug_mode & UM_INST ? iisystem.path :
					selected_instance->inst_loc ;


    STlcopy( gtk_entry_get_text(GTK_ENTRY(editable)),
		txlog_info.dual_loc.path,
		MAX_LOC);
	
    /* check all log locations are valid */
    GIPvalidateLocs( iisystem_ptr );

    DBG_PRINT(
    "on_bkp_log_file_changed called: II_SYSTEM=%s\n\t\tdual_file_loc=%s\n",
		iisystem_ptr, gtk_entry_get_text( GTK_ENTRY(editable)) );

}

void
on_ii_system_entry_changed             (GtkEditable     *editable,
                                        gpointer         user_data)
{

    /* update II_SYSTEM from new value in field */
    if ( ug_mode & UM_INST )
    {
	STlcopy( gtk_entry_get_text(GTK_ENTRY(editable)),
		iisystem.path,
		MAX_LOC );
	DBG_PRINT("on_ii_system_entry_changed: II_SYSTEM=%s\n", iisystem.path );
    }
	
    if ( update_location_entry( INST_II_SYSTEM,
				GTK_ENTRY( editable ) ) != OK )
	return; /* validation faild so don't update anything */

    /* now update everything else accordingly */
    on_iisystem_changed( editable, user_data);
}

void
on_install_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget	*widget_ptr;
    GError	*error = NULL;
    char	cmdline[PATH_MAX];

    /* 
    ** For everything other than new installations, check
    ** if selected instance is running. If it is, prompt
    ** for shutdown.
    */
    if ( ! ( ug_mode & UM_INST )  )
    {
	STprintf( cmdline, INGSTATUS_CMD, selected_instance->instance_ID );
	if ( PCcmdline( NULL, cmdline, TRUE, NULL, NULL ) == OK )
	    {
	    if ( prompt_shutdown() != OK )
		/* user decided not to shutdown instance so return to summary */
		return;
	    else
	    {
		/* shutdown installation */
	 	STprintf( cmdline, INGSTOP_CMD,
			selected_instance->instance_ID );
		if ( PCcmdline( NULL, cmdline, TRUE, NULL, NULL ) != OK )
		{
		    char	tmpbuf[PATH_MAX];
# define SHUTDOWN_FAILED "Unable to shutdown the Ingres %s instance. The instance must be shutdown before the installation can continue."
		    STprintf( tmpbuf, SHUTDOWN_FAILED,
			selected_instance->instance_ID );
		    popup_error_box( tmpbuf );
		    return;
		}
	    }
	}
    }

    /* Jump to the install screen */
    widget_ptr=lookup_widget( IngresInstall, current_notebook );
    gtk_notebook_set_current_page( GTK_NOTEBOOK( widget_ptr ),
				    ug_mode & UM_INST ?
				    NI_INSTALL : UG_DO_INST );
	
    /* mark the this stage complete */
    stage_names[current_stage]->status=ST_COMPLETE;
    current_stage++;
    stage_names[current_stage]->status=ST_VISITED;	
    increment_wizard_progress();

    /* create tags in output buffer */
    widget_ptr=lookup_widget(IngresInstall, 
				ug_mode & UM_INST ?
				"InstOutTextView" : "UpgOutTextView" );
    create_summary_tags( gtk_text_view_get_buffer( 
					GTK_TEXT_VIEW( widget_ptr ) ) );

    /* create thread to perform installation */
    if ( ! g_thread_create( install_control_thread, NULL, FALSE, &error ) )
	DBG_PRINT( "ERROR: Failed to create installer thread:\n%s\n",
							error->message );
	
}

void
on_master_next_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget	*notebook;
    GtkWidget	*wizard_progress;
    i4		next_stage;
    bool	switch_run_mode=FALSE;
	
    /* mark the current stage completed if it didn't failed */
    if ( stage_names[current_stage]->status != ST_FAILED )
	stage_names[current_stage]->status=ST_COMPLETE;
	
    if ( runmode == MA_UPGRADE )
    {
	/* 
	** progress in upgrade land in non-linear so
	** where next is, is dependent on where we are.
	*/
	DBG_PRINT("Moving UG notebook\n");
	switch ( stage_names[current_stage]->stage )
	{
	    GtkWidget	*widget_ptr;
			
	    case UG_START:
		DBG_PRINT("On UG_START, moving to");
		if ( inst_state & UM_MOD && inst_state & UM_UPG )
		{
		   next_stage=current_stage=UG_MULTI;
		   DBG_PRINT("UG_MULTI\n");
		}
		else if ( inst_state&UM_MOD )
		{
		   next_stage=current_stage=UG_SSAME;
		   DBG_PRINT("UG_SSAME\n");
		}
		else
		{
		   next_stage=current_stage=UG_SOLD;				
		   DBG_PRINT("UG_SOLD\n");
		}
		break;
	    case UG_SSAME:
		DBG_PRINT("In UG_SELECT, going to %s\n",ug_mode&UM_MOD ?
					"New Installation" : "UG_MOD" );
		if ( ug_mode & UM_INST )
		   switch_run_mode=TRUE;
		else
		   next_stage=current_stage=UG_SELECT;
		break;
	    case UG_SOLD:
		DBG_PRINT("In UG_SOLD, going to %s\n",ug_mode&UM_INST ?
					 "New Installation" : "UG_SUMMARY" );
		if ( ug_mode&UM_INST )
		   switch_run_mode=TRUE;
		else
		   next_stage=current_stage=UG_SELECT;
		break;
	    case UG_MULTI:
		if ( ug_mode&UM_INST )
		   switch_run_mode=TRUE;
		else
		   next_stage=current_stage=UG_SELECT;
		break;
	    case UG_SELECT:
		DBG_PRINT("In UG_SELECT, going to %s\n",ug_mode&UM_MOD ?
						"UM_MOD" : "UG_SUMMARY" );
		if ( ug_mode&UM_MOD )
		   next_stage=current_stage=UG_MOD;
		else
		   next_stage=current_stage=UG_SUMMARY;
		break;
 	    case UG_MOD:
		/*
		** if we're adding the DBMS package then we need to show
		** the DBMS config panes, otherwise, on to the summary screen
		*/
		next_stage = current_stage = mod_pkgs_to_install & PKG_DBMS  ?
						UG_MOD_DBLOC : UG_SUMMARY ;
		break;
	    case UG_DO_INST:
		if ( stage_names[current_stage]->status == ST_FAILED )
		{
		    next_stage=current_stage=UG_FAIL;
		    stage_names[next_stage]->status = ST_FAILED;
		}
		else
		{
		    next_stage=current_stage=UG_DONE;
		}
		break;
	    default:
		next_stage=++current_stage;
	}
    }
    else
    {		
	/* mark the next stage visited */
	if ( stage_names[current_stage]->status == ST_FAILED )
	{
	    /* find fail screen */
	    while (stage_names[current_stage]->stage != NI_FAIL )
		 current_stage++;

	    next_stage=current_stage;
	    stage_names[next_stage]->status=ST_FAILED;
	}
	else
	{
	    next_stage=++current_stage;
	    if ( stage_names[next_stage]->status == ST_INIT )
		stage_names[next_stage]->status = ST_VISITED;
	}
    }
	
    if ( switch_run_mode == TRUE )
    {
	/*
	** At this point we know we're now doing an new install
	** so switch runmode and drop into the experience level
	** screen in the install_notebook
	*/
	/* switch runmode */
	runmode = ( runmode == MA_INSTALL ? MA_UPGRADE : MA_INSTALL ) ;
	set_screen( START_SCREEN + 1 ); /* skip the welcome screen */
    }
    else
    {
	/* lookup the master notebook and advance it to the next_stage*/
	notebook = lookup_widget( IngresInstall, current_notebook );
	gtk_notebook_set_current_page( GTK_NOTEBOOK( notebook ),
					stage_names[next_stage]->stage );
    }
		
    /* bump the progress bar too */
    increment_wizard_progress();
}

void
on_master_back_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *notebook;
    GtkWidget *wizard_progress;
    i4	  prev_stage;
    bool  switch_run_mode=FALSE;
	
	
    if ( runmode == MA_UPGRADE )
    {
	/* 
	** progress in upgrade land in non-linear so
	** where 'previous' is, is dependent on where we are.
	*/
	DBG_PRINT("Moving UG notebook\n");
	switch ( stage_names[current_stage]->stage )
	{
	    GtkWidget	*widget_ptr;
			
	    case UG_SSAME:
	    case UG_SOLD:
	    case UG_MULTI:
		prev_stage=current_stage=UG_START;
		break;
	    case UG_SELECT:
		if ( inst_state & UM_MOD && inst_state & UM_UPG )
		    prev_stage=current_stage=UG_MULTI;
		else if ( inst_state & UM_MOD )
		    prev_stage=current_stage=UG_SSAME;
		else
		    prev_stage=current_stage=UG_SOLD;
		break;
	    case UG_MOD:
		DBG_PRINT("Back clicked from UG_MOD\n");
		DBG_PRINT(" prev_stage=UG_SELECT\n");
		prev_stage=current_stage=UG_SELECT;
		break;
	    case UG_SUMMARY:
		if ( ug_mode & UM_MOD )
		    /*
		    ** if we're adding the DBMS package then we need to show
		    ** the DBMS config panes, otherwise, back to the
		    ** modify screen
		    */
		    prev_stage=current_stage = mod_pkgs_to_install & PKG_DBMS  ?
						UG_MOD_MISC : UG_MOD ;
		else
		    prev_stage=current_stage=UG_SELECT;
		break;
	    default:
		prev_stage=--current_stage;
	}
    }
    else
    {
	if ( current_stage == ( START_SCREEN + 1 )	
				 && (ug_mode & UM_TRUE) )	
	    switch_run_mode=TRUE;
	else
	    prev_stage=--current_stage;
    }
	
    if ( switch_run_mode == TRUE )
    {
	/*
	** At this point we know we're going backwards from 
	** experience level screen in the install_notebook
	** and we came from the upgrade_notebook so we need 
	** to switch back
	*/
	/* switch runmode */
	runmode=MA_INSTALL ? MA_UPGRADE : MA_INSTALL ;
	set_screen( inst_state & UM_UPG && inst_state & UM_MOD ? UG_MULTI :
				inst_state & UM_UPG ? UG_SOLD : UG_SSAME );
    }
    else
    {
	notebook=lookup_widget(IngresInstall,current_notebook);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),
					stage_names[prev_stage]->stage);
    }
	
    /* bump the progress bar too */
    increment_wizard_progress();
}

void
on_upgrade_instance_clicked     (GtkButton       *button,
                                 gpointer         user_data)
{
    GtkWidget	*new_inst_table;
    GtkWidget	*ug_install_table;
    GtkWidget	*inst_sel_table;
    GtkWidget	*upgrade_install_label;
    GtkWidget	*ug_mod_table;
    GtkTreeModel *instance_list;
    GtkTreeSelection *ug_selection;
    GtkTreeIter	first;
    i4		i=0;
	
    /* only interested in toggled on */
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ) ) == FALSE )
	return;

    /* We're doing an upgrade so unset UM_INST in ug_mode */
    ug_mode&=~UM_INST;
    ug_mode|=UM_UPG;
    ug_mode&=~UM_MOD;
	
    /* load the instance list */
    instance_list = gtk_tree_view_get_model(
				GTK_TREE_VIEW( InstanceTreeView ) );
    gtk_list_store_clear( GTK_LIST_STORE( instance_list ) );
    while ( i < inst_count )
    {
	/* and load that into the list */
	if ( existing_instances[i]->action & UM_UPG )
	    add_instance_to_store( GTK_LIST_STORE(instance_list), 
					existing_instances[i] );
	i++;
    }
	
    /* select the first entry */
    ug_selection = gtk_tree_view_get_selection(
				GTK_TREE_VIEW( InstanceTreeView ) );
					
    gtk_tree_model_get_iter_first( instance_list, &first );
    gtk_tree_selection_select_iter( ug_selection, &first );

    /* lookup the widgets we want to show/hide */
    upgrade_install_label=lookup_widget(IngresInstall, "upgrade_install_label");
    new_inst_table=lookup_widget(IngresInstall, "new_inst_table");
    ug_install_table=lookup_widget(IngresInstall, "ug_install_table");
    inst_sel_table=lookup_widget(IngresInstall, "inst_sel_table");
    ug_mod_table=lookup_widget(IngresInstall, "ug_mod_table");

    gtk_label_set_text(GTK_LABEL(upgrade_install_label), "Upgrade");
    gtk_widget_hide(new_inst_table);
    gtk_widget_show(ug_install_table);
    gtk_widget_hide(ug_mod_table);
    gtk_widget_show(inst_sel_table);
}


void
on_new_instance_clicked      (GtkButton       *button,
                              gpointer         user_data)
{
    GtkWidget	*ii_system_entry;
    GtkWidget	*new_inst_table;
    GtkWidget	*ug_install_table;
    GtkWidget	*inst_sel_table;
    GtkWidget	*upgrade_install_label;
    GtkWidget	*ug_mod_table;

    /* only interested in toggled on */
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ) ) == FALSE )
	return;

    /* We're doing an install so set UM_INST in ug_mode */
    ug_mode|=UM_INST;
    ug_mode&=~UM_MOD;
    ug_mode&=~UM_UPG;

    /* make sure all locations reflect correct II_SYSTEM */
    DBG_PRINT( "New Instance: set II_SYSTEM=%s\n", default_ii_system );
    ii_system_entry = lookup_widget( IngresInstall, "ii_system_entry" );
    gtk_entry_set_text( GTK_ENTRY( ii_system_entry ), default_ii_system );
    on_ii_system_entry_changed( GTK_EDITABLE( ii_system_entry ), NULL );

    /* lookup the widgets we want to show/hide */
    new_inst_table=lookup_widget(IngresInstall, "new_inst_table");
    ug_install_table=lookup_widget(IngresInstall, "ug_install_table");
    inst_sel_table=lookup_widget(IngresInstall, "inst_sel_table");
    ug_mod_table=lookup_widget(IngresInstall, "ug_mod_table");
	
    /* for multi new install we want, new instance only */
    gtk_widget_show(new_inst_table);
    gtk_widget_hide(inst_sel_table);
    gtk_widget_hide(ug_install_table);
    gtk_widget_hide(ug_mod_table);
}


void
on_modify_instance_clicked       (GtkButton       *button,
                                  gpointer         user_data)
{
    GtkWidget	*new_inst_table;
    GtkWidget	*ug_install_table;
    GtkWidget	*inst_sel_table;
    GtkWidget	*upgrade_install_label;
    GtkWidget	*ug_mod_table;
    GtkTreeModel *instance_list;
    GtkTreeSelection *ug_selection;
    GtkTreeIter	first;
    i4	i=0;
	
    /* only interested in toggled on */
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ) ) == FALSE )
	return;

    /* We're doing a modify so unset UM_INST in ug_mode */
    ug_mode&=~UM_INST;
    ug_mode&=~UM_UPG;
    ug_mode|=UM_MOD;
	
    /* load the instance list if we found more than 1 */
    instance_list = gtk_tree_view_get_model(
				GTK_TREE_VIEW( InstanceTreeView ) );
    gtk_list_store_clear( GTK_LIST_STORE( instance_list ) );
    while ( i < inst_count )
    {
	/* and load that into the list */
	if ( existing_instances[i]->action & UM_MOD )
	    add_instance_to_store( GTK_LIST_STORE( instance_list ), 
					existing_instances[i] );
		
	i++;
    }

    /* select the first entry */
    ug_selection = gtk_tree_view_get_selection(
				GTK_TREE_VIEW( InstanceTreeView ) );
					
    gtk_tree_model_get_iter_first( instance_list, &first );
    gtk_tree_selection_select_iter( ug_selection, &first );

    /* lookup the widgets we want to show/hide */
    new_inst_table=lookup_widget(IngresInstall, "new_inst_table");
    ug_install_table=lookup_widget(IngresInstall, "ug_install_table");
    inst_sel_table=lookup_widget(IngresInstall, "inst_sel_table");
    ug_mod_table=lookup_widget(IngresInstall, "ug_mod_table");

    gtk_widget_hide(new_inst_table);
    gtk_widget_show(ug_install_table);
    gtk_widget_show(ug_mod_table);
    gtk_widget_show(inst_sel_table);
}

void
set_screen(gint stage_num)
{
    GtkWidget *notebook;
    GtkWidget *master_notebook;
    GtkWidget *wizard_progress;
    GtkWidget	*inst_type_icon;
    gint	   next_stage, it_stage;
    
    current_stage=stage_num;
	
    if ( runmode == MA_UPGRADE )
    {
	current_notebook=upgrade_notebook;
	stage_names=ug_stages;
    }
    else
    {
	current_notebook=install_notebook;
	if (instmode == BASIC_INSTALL )
	    stage_names=basic_stages;
	else if ( instmode == ADVANCED_INSTALL )
	    stage_names=adv_stages;
	else if ( instmode == RFGEN_LNX )
	    stage_names=rfgen_lnx_stages;
	else if ( instmode == RFGEN_WIN )
	    stage_names=rfgen_win_stages;
    }
    /* lookup the master notebook */
    master_notebook=lookup_widget(IngresInstall, "master_notebook");
    gtk_notebook_set_current_page(GTK_NOTEBOOK(master_notebook), runmode );
	
    /* Now jump to the requested page of the appropriate notebook */
    notebook=lookup_widget(IngresInstall,current_notebook);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),
					stage_names[stage_num]->stage);
		
}

void
on_use_default_ID_radio_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget	*instname_part1;
    GtkWidget	*instname_part2;
	
    DBG_PRINT("on_use_default_id_radio_clicked called\n");
    instname_part1=lookup_widget(IngresInstall, "instID_entry1");
    instname_part2=lookup_widget(IngresInstall, "instID_entry2");
    
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ) ) == TRUE )
    {
	/* shade the custom fields and reset instID to default  */
	gtk_widget_set_sensitive( instname_part1, FALSE );
	gtk_entry_set_text( GTK_ENTRY(instname_part2), dfltID );
	gtk_widget_set_sensitive( instname_part2, FALSE );
    }
    else
    {
	gtk_entry_set_editable( GTK_ENTRY(instname_part2), TRUE );
	gtk_widget_set_sensitive( instname_part1, TRUE );
	gtk_widget_set_sensitive( instname_part2, TRUE );
    }
}

void
on_instID_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
    GtkWidget *next_button;
    const gchar *IDbuf;
    gchar  tmpbuf[MAX_LOC];
    size_t pathsize=(STlen(default_ii_system) - 2 ); /* 
						      ** length of 
						      ** II_SYSTEM less
						      ** II_INSTALLATION
						      */
	
    /* Lookup next button and grey it out if instID isn't right */
    next_button=lookup_widget(IngresInstall, "next_button");
    /* Get value set by user in drop down */
    IDbuf=gtk_entry_get_text( GTK_ENTRY( editable ) );
	
    /* Set II_INSTALLATION */
    if ( CMupper( &IDbuf[0] ) && ( CMdigit( &IDbuf[1] ) || CMupper( &IDbuf[1] ) ) )
    {
	DBG_PRINT("New value for II_INSTALLATION = %s\n", IDbuf);
	/* construct inst name */
	STprintf( tmpbuf, "Ingres %s", IDbuf );
	if ( inst_name_in_use(tmpbuf) == FAIL )
	{
	    instID[0]=IDbuf[0];
	    instID[1]=IDbuf[1];
	    gtk_widget_set_sensitive( next_button, TRUE );
	}
	else
	{
# define ERROR_INST_NAME_IN_USE "The instance name you have\nselected is already in use."
	    STprintf( tmpbuf, ERROR_INST_NAME_IN_USE, IDbuf );
	    popup_error_box( tmpbuf );
	    gtk_widget_set_sensitive( next_button, FALSE );
	}
    }
    else			
	gtk_widget_set_sensitive( next_button, FALSE );
	
    DBG_PRINT("Installation ID is: %s\n", instID);
	
    /* If II_SYSTEM hasn't been changed by the user, update that too */
    if ( ! strncmp(iisystem.path, default_ii_system, pathsize ) )
    {
	GtkWidget *ii_system_entry;
	
	/* correct the installation ID in II_SYSTEM */
	STlcopy(iisystem.path, tmpbuf, MAX_LOC);
	tmpbuf[pathsize]=instID[0];
	tmpbuf[pathsize + 1]=instID[1];
		
	/* update the text fields which will update II_SYSTEM */
	ii_system_entry=lookup_widget(IngresInstall, "ii_system_entry"); 
	gtk_entry_set_text(GTK_ENTRY(ii_system_entry), tmpbuf);
    }
}

void
on_txlog_size_spinner_value_changed    (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
    GtkWidget	*log_size_hscale;
    gdouble	spinner_value;

    log_size_hscale = lookup_widget( IngresInstall, ug_mode & UM_INST ?
					"log_size_hscale" :
					"mod_log_size_hscale" );

    /* update log file size slider */
    spinner_value = gtk_spin_button_get_value( spinbutton );
    gtk_range_set_value( GTK_RANGE( log_size_hscale ), spinner_value );

    /* update the transaction log file size from input value */
    txlog_info.size = (size_t)spinner_value;
    DBG_PRINT("TX Log size = %d\n", txlog_info.size );
}

/*
** GtkComboBox was not introduced until GTK 2.4. We need to support
** 2.2 and up so comment out for now
**
** void
** on_ii_timezone_name_changed            (GtkComboBox     *combobox,
**                                        gpointer         user_data)
** {	
**     GtkTreeModel	*tz_list;
**     GtkTreeIter		tz_iter;
**     gchar		*tz_name;
**     
** 	
**     /* Get the active iter and the model from the combobox
**     tz_list=gtk_combo_box_get_model( combobox );
**     gtk_combo_box_get_active_iter( combobox, &tz_iter );
** 
**     /* use them to get the current timezone choice.
**     gtk_tree_model_get( tz_list, &tz_iter, 0, &tz_name, -1 );
** 
**     /* copy the result to the global buffer
**     DBG_PRINT( "Timezone changed to %s\n", tz_name );
**     STcopy( tz_name, locale_info.timezone );
** 
**     /* free temp storage
**     g_free(tz_name);
** }
** 
** 
** void
** on_ii_charset_changed                  (GtkComboBox     *combobox,
**                                         gpointer         user_data)
** {
** 
**     GtkTreeModel	*cs_list;
**     GtkTreeIter		cs_iter;
**     gchar		*cs_name;
**     
**     /* Get the active iter and the model from the combobox 
**     cs_list=gtk_combo_box_get_model( combobox );
**     gtk_combo_box_get_active_iter( combobox, &cs_iter );
** 
**     /* use them to get the current charset choice.
**     gtk_tree_model_get( cs_list, &cs_iter, 0, &cs_name, -1 );
** 
** 
**     /* copy the result to the global buffer
**     DBG_PRINT( "Charset changed to %s\n", cs_name );
**     STcopy( cs_name, locale_info.charset );
** 
**     /* free temp storage
**     g_free(cs_name);
** }
** 
*/
void
on_sql92_enabled_ckeck_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	misc_ops ^= GIP_OP_SQL92;
	
	DBG_PRINT("GIP_OP_SQL92 is %s\n", misc_ops & GIP_OP_SQL92 ? "on" : "off" );
}

void
on_default_inst_owner_clicked         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{	
    GtkWidget	*inst_owner_entry;
    GtkWidget	*next_button;
	
    inst_owner_entry = lookup_widget(IngresInstall, "inst_owner_entry");
    next_button = lookup_widget( IngresInstall, "next_button");
		
    /* Toggle default user option */
    if ( gtk_toggle_button_get_active( togglebutton ) == TRUE )
        misc_ops &= ~GIP_OP_INST_CUST_USR;
    else
        misc_ops |= GIP_OP_INST_CUST_USR;
	
    if ( misc_ops & GIP_OP_INST_CUST_USR )
    {	
	GtkWidget	*next_button;
	/* No */
	/* Unshade the username entry field */
	gtk_widget_set_sensitive( inst_owner_entry, TRUE );

	/*
	** check the contents of the field. If it's empty, 
	** grey the next button
	*/
	userid = (char *)gtk_entry_get_text( GTK_ENTRY(inst_owner_entry) );
    }
    else
    {
	gtk_widget_set_sensitive( inst_owner_entry, FALSE );
	userid = (char *)defaultuser;
    }

    if ( groupid[0] == '\0' || userid[0] == '\0' )
	gtk_widget_set_sensitive( next_button, FALSE );
    else
	gtk_widget_set_sensitive( next_button, TRUE );
}

void
on_inst_owner_entry_changed            (GtkEditable     *editable,
                                        gpointer         user_data)
{
    GtkWidget	*next_button;

    next_button=lookup_widget(IngresInstall, "next_button");
    /*
    ** check the contents of the field. If it's empty, 
    ** grey the next button otherwise, un-grey it
    */
    userid=(char *)gtk_entry_get_text( GTK_ENTRY(editable) );
    gtk_widget_set_sensitive( next_button, ( userid[0] == '\0' ||
						groupid[0] == '\0' ) ?
						FALSE : TRUE );
}

void
on_start_on_boot_check_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* toggle start on boot */
    misc_ops ^= GIP_OP_START_ON_BOOT;
}

void
on_demodb_check_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* toggle installing demo */
    misc_ops ^= GIP_OP_INSTALL_DEMO;
}


void
on_ingres_desktop_check_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* toggle creation of desktop Ingres folder */
    misc_ops ^= GIP_OP_DESKTOP_FOLDER;
}

void
on_remove_inst_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget	*inst_comp_box1;
    GtkWidget	*inst_comp_box2;
    GtkWidget	*remove_all_files_check;
	
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE )
	return;
    /*
    ** grey out component selection and activate
    ** "delete files" check box
    */
    inst_comp_box1=lookup_widget( IngresInstall, "inst_comp_box1" );
    inst_comp_box2=lookup_widget( IngresInstall, "inst_comp_box2" );
    remove_all_files_check=lookup_widget( IngresInstall,
					"remove_all_files_check" );
    gtk_widget_set_sensitive( inst_comp_box1, FALSE );
    gtk_widget_set_sensitive( inst_comp_box2, FALSE );
    gtk_widget_set_sensitive( remove_all_files_check, TRUE );
	
    /* set the relvent flags */
    ug_mode |= UM_RMV;
    mod_pkgs_to_install=PKG_NONE;
    mod_pkgs_to_remove=selected_instance->installed_pkgs;
    set_mod_pkg_check_buttons( PKG_NONE );
}


void
on_modify_inst_comp_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget	*inst_comp_box1;
    GtkWidget	*inst_comp_box2;
    GtkWidget	*remove_all_files_check;
	
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE )
	return;
    /*
    ** show component selection boxes and grey out
    ** "delete files" check box and turn it off
    */
    inst_comp_box1=lookup_widget( IngresInstall, "inst_comp_box1" );
    inst_comp_box2=lookup_widget( IngresInstall, "inst_comp_box2" );
    remove_all_files_check=lookup_widget( IngresInstall,
					"remove_all_files_check" );
    gtk_widget_set_sensitive( inst_comp_box1, TRUE );
    gtk_widget_set_sensitive( inst_comp_box2, TRUE );
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( remove_all_files_check ),
					FALSE );
    gtk_widget_set_sensitive( remove_all_files_check, FALSE );
	
    /* set the relvent flags */
    ug_mode &= ~UM_RMV;
    mod_pkgs_to_install=PKG_NONE;
    mod_pkgs_to_remove=PKG_NONE;
    set_mod_pkg_check_buttons( selected_instance->installed_pkgs );
	
}

void
instance_selection_changed (GtkTreeSelection *selection,
					gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *instance_tree;
    gchar	*inst_name, *version, *inst_loc;
    gint	i=0;

    if (gtk_tree_selection_get_selected (selection, &instance_tree, &iter))
    {
	GtkWidget	*widget_ptr;

	gtk_tree_model_get (instance_tree, &iter, 
				COL_NAME, &inst_name,
				COL_VERSION, &version,
				COL_LOCATION, &inst_loc,
				-1);

	DBG_PRINT("The following instance has been selected:\nName: %s\nVersion: %s\nlocation%s\n",
		inst_name, version, inst_loc);
	
	while ( i < inst_count )
	{
	    /* set the gui package info for the select package */
	    if ( ! STcompare( existing_instances[i]->instance_name, inst_name ) )
	    {
		selected_instance=existing_instances[i];
		break;
	    }
	    i++;
	}
			
	/* reset package info */
	mod_pkgs_to_install = PKG_NONE;
	mod_pkgs_to_remove = PKG_NONE;
	set_mod_pkg_check_buttons( selected_instance->installed_pkgs );
	
	/* hide DBMS config */
	widget_ptr = lookup_widget( IngresInstall, "mod_db_config" );
	gtk_widget_hide( widget_ptr );

	/* free temporary storage */
       	g_free(inst_name);
	g_free(version);
	g_free(inst_loc);
    }
}

void
on_ug_checkdbmspkg_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    GtkWidget	*mod_iisystem_entry;
    GtkWidget	*mod_db_config;

    if ( gtk_toggle_button_get_active( togglebutton ) == TRUE )
	/* turned on */
	ug_pkg_sel( DBMS );
    else
	/* turned off */
	ug_pkg_rmv( DBMS );

    /* check if we need the DBMS config panes */
    mod_db_config = lookup_widget( IngresInstall, "mod_db_config" );

    if ( mod_pkgs_to_install & PKG_DBMS )
    {
	GtkWidget	*widget_ptr;

	/* train entries */
	gtk_widget_show( mod_db_config );

	/* hidden entry field used to trigger update */
	mod_iisystem_entry = lookup_widget( IngresInstall,
						"mod_iisystem_entry" );
	gtk_entry_set_text( GTK_ENTRY(mod_iisystem_entry) , 
				selected_instance->inst_loc  );

 	/* set locations to default to II_SYSTEM */
	widget_ptr = lookup_widget( IngresInstall,
					"mod_db_loc_iisys_default" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget_ptr ), TRUE );
    }
    else
    {
	/* train entries */
	gtk_widget_hide( mod_db_config );
    }
	
}


void
on_ug_checknetpkg_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if ( gtk_toggle_button_get_active( togglebutton ) == TRUE )
	/* turned on */
	ug_pkg_sel( NET );
    else
	/* turned off */
	ug_pkg_rmv( NET );
}


void
on_ug_checkodbcpkg_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* turned on */
    if ( gtk_toggle_button_get_active( togglebutton ) == TRUE )
	/* turned on */
	ug_pkg_sel( ODBC );
    else
	/* turned off */
	ug_pkg_rmv( ODBC );
}


void
on_ug_checkreppkg_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* turned on */
    if ( gtk_toggle_button_get_active( togglebutton ) == TRUE )
	/* turned on */
	ug_pkg_sel( REP );
    else
	/* turned off */
	ug_pkg_rmv( REP );
}


void
on_ug_checkabfpkg_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* turned on */
    if ( gtk_toggle_button_get_active( togglebutton ) == TRUE )
	/* turned on */
	ug_pkg_sel( ABF );
    else
	/* turned off */
	ug_pkg_rmv( ABF );
}


void
on_ug_checkstarpkg_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* turned on */
    if ( gtk_toggle_button_get_active( togglebutton ) == TRUE )
	/* turned on */
	ug_pkg_sel( STAR );
    else
	/* turned off */
	ug_pkg_rmv( STAR );
}


void
on_ug_checkicepkg_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* turned on */
    if ( gtk_toggle_button_get_active( togglebutton ) == TRUE )
	/* turned on */
	ug_pkg_sel( ICE );
    else
	/* turned off */
	ug_pkg_rmv( ICE );
}

void
on_ug_checkdocpkg_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* turned on */
    if ( gtk_toggle_button_get_active( togglebutton ) == TRUE )
	/* turned on */
	ug_pkg_sel( DOC );
    else
	/* turned off */
	ug_pkg_rmv( DOC );
}

void
on_save_rf_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *SaveRFDialog;
    char	*locptr;
    char	DefSaveLoc[MAX_LOC];

# define SAVE_RESPONSE_FILE_DIALOG_TITLE "Save Response File"
    NMgtAt( "II_SYSTEM", &locptr );
    STprintf( DefSaveLoc, "%s/%s", locptr, RESPONSE_FILE_NAME );
    SaveRFDialog = gtk_file_selection_new( BROWSE_DIALOG_TITLE ); 
    gtk_file_selection_set_filename( GTK_FILE_SELECTION( SaveRFDialog ), 
					DefSaveLoc );


    /* Connect selection handler */
    g_signal_connect( GTK_FILE_SELECTION(SaveRFDialog)->ok_button,
			"clicked",
			G_CALLBACK( get_selected_RF_filename ), 
			SaveRFDialog );

    /* Make sure dialog closes when buttons are clicked */
    g_signal_connect_swapped (GTK_FILE_SELECTION (SaveRFDialog)->ok_button,
                             "clicked",
                             G_CALLBACK (gtk_widget_destroy), 
                             SaveRFDialog);

    g_signal_connect_swapped (GTK_FILE_SELECTION (SaveRFDialog)->cancel_button,
                             "clicked",
                             G_CALLBACK (gtk_widget_destroy),
                             SaveRFDialog); 

    /* show the dialog */
    gtk_widget_show(SaveRFDialog);

    /*
    ** GtkFileChooser widgets were introduced with v 2.6, we currently
    ** have to support 2.2 and up. Comment out code for now.
    ** SaveRFDialog = gtk_file_chooser_dialog_new(
    **					SAVE_RESPONSE_FILE_DIALOG_TITLE,
    **					GTK_WINDOW( IngresInstall ),
    **					GTK_FILE_CHOOSER_ACTION_SAVE,
    **					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    **					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
    **					NULL);
    ** gtk_file_chooser_set_do_overwrite_confirmation(
    **					GTK_FILE_CHOOSER( SaveRFDialog ),
    **					TRUE);
    **
    ** Use II_SYSTEM as the default save location 
    ** gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( SaveRFDialog ),
    **					DefSaveLoc );
    ** gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER( SaveRFDialog ),
    **					"Response File" );
    **
    ** if (gtk_dialog_run( GTK_DIALOG( SaveRFDialog ) ) == GTK_RESPONSE_ACCEPT)
    ** {
    **	LOCATION newrfloc;
    **	char *filename;
    **	char tmpbuf[MAX_LOC];
    **	STATUS	rc;
    **
    **	filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(
    **							SaveRFDialog ));
    **
    **	LOfroms( FILENAME & PATH, filename, &newrfloc );
    **	rc = SIcopy( &rfnameloc, &newrfloc );
    **	if ( rc != OK )
    **	{
# define ERROR_RF_SAVE_FAILED "Failed to save response file to %s\n"
    **	    STprintf( tmpbuf, ERROR_RF_SAVE_FAILED, filename );
    **	    popup_error_box( tmpbuf );
    **	}
    **	else
    **	    DBG_PRINT( "Response file saved at: %s\n", filename );
    **
    **	g_free (filename);
    **    }
    **
    ** gtk_widget_destroy( SaveRFDialog );
    */
}

void
on_rfdef_linux_radio_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    GtkWidget	*ii_system_entry;
    char	pathbuf[MAX_LOC];


    /* toggle deployment platform  */
    dep_platform = ( dep_platform == II_RF_DP_LINUX ) ?
			II_RF_DP_WINDOWS : II_RF_DP_LINUX ;
    instmode = ( dep_platform == II_RF_DP_LINUX ?
			RFGEN_LNX : RFGEN_WIN );
    set_screen( RF_PLATFORM );

    /* Set defaults accordingly */
    default_ii_system = ( dep_platform & II_RF_DP_WINDOWS ? 
			WinDefaultII_SYSTEM : LnxDefaultII_SYSTEM ) ;

    ii_system_entry = lookup_widget( IngresInstall, "ii_system_entry" );
    if ( STcompare( instID, RFAPI_DEFAULT_INST_ID ) == OK )
    {
	/* II_INSTALLATION has not been changed so use defaults */
	gtk_entry_set_text( GTK_ENTRY( ii_system_entry ), default_ii_system ); 
    }
    else
    {
	/* II_INSTALLATION's been changed so append it to the default */
	STlcopy( default_ii_system,
			pathbuf,
			( STlen( default_ii_system ) - 2 ) );
	STcat( pathbuf, instID );
	gtk_entry_set_text( GTK_ENTRY( ii_system_entry ), pathbuf );
    }
}


void
on_WinStartIngSysButton_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    GtkWidget	*WinSvcAccBox;
    GtkWidget	*WinUsrPwdBox;
    GtkWidget	*next_button;

    next_button=lookup_widget(IngresInstall, "next_button");
    WinSvcAccBox = lookup_widget( IngresInstall, "WinSvcAccBox" );
    WinUsrPwdBox = lookup_widget( IngresInstall, "WinUsrPwdBox" );

    WinMiscOps ^= GIP_OP_WIN_START_AS_SERVICE;

    if ( WinMiscOps & GIP_OP_WIN_START_AS_SERVICE )
    {
	gtk_widget_set_sensitive( WinSvcAccBox, TRUE );

	if ( WinMiscOps & GIP_OP_WIN_START_CUST_USR )
	{
	    gtk_widget_set_sensitive( WinUsrPwdBox, TRUE );
	    gtk_widget_set_sensitive( next_button,
				 	( WinServiceID[0] == '\0' ||
					WinServicePwd[0] == '\0' ) ?
					FALSE : TRUE );
	}
	else
	{
	    gtk_widget_set_sensitive( WinUsrPwdBox, TRUE );
	    gtk_widget_set_sensitive( next_button, TRUE );
	}
    }
    else
    {
	gtk_widget_set_sensitive( WinSvcAccBox, FALSE );
	gtk_widget_set_sensitive( next_button, TRUE );
    }


}


void
on_WinStartIngAsSysRadio_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    GtkWidget	*WinUsrPwdBox;
    GtkWidget	*next_button;

    WinUsrPwdBox = lookup_widget( IngresInstall, "WinUsrPwdBox" );
    next_button=lookup_widget(IngresInstall, "next_button");

    WinMiscOps ^= GIP_OP_WIN_START_CUST_USR;

    if ( WinMiscOps & GIP_OP_WIN_START_CUST_USR )
    {
	gtk_widget_set_sensitive( WinUsrPwdBox, TRUE );
	if ( WinServiceID == NULL || WinServicePwd == NULL )
	{
	    gtk_widget_set_sensitive( next_button, FALSE );
	}
	else
	{
	    gtk_widget_set_sensitive( next_button,
				 	( WinServiceID[0] == '\0' ||
					WinServicePwd[0] == '\0' ) ?
					FALSE : TRUE );
	}
    }
    else
    {
	gtk_widget_set_sensitive( WinUsrPwdBox, TRUE );
	gtk_widget_set_sensitive( next_button, TRUE );
    }
}


void
on_WinUsrEntry_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{
    GtkWidget	*next_button;

    next_button=lookup_widget(IngresInstall, "next_button");

    /*
    ** check the contents of the field. If it's empty, 
    ** grey the next button otherwise, un-grey it
    */
    WinServiceID=gtk_entry_get_text( GTK_ENTRY(editable) );
    if ( WinServiceID == NULL || WinServicePwd == NULL )
    {
	gtk_widget_set_sensitive( next_button, FALSE );
    }
    else
    {
	gtk_widget_set_sensitive( next_button,
			 	( WinServiceID[0] == '\0' ||
				WinServicePwd[0] == '\0' ) ?
				FALSE : TRUE );
    }

}


void
on_WinPwdEntry_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{

    GtkWidget	*next_button;

    next_button=lookup_widget(IngresInstall, "next_button");
    /*
    ** check the contents of the field. If it's empty, 
    ** grey the next button otherwise, un-grey it
    */
    WinServicePwd=gtk_entry_get_text( GTK_ENTRY(editable) );
    if ( WinServiceID == NULL || WinServicePwd == NULL )

    {
	gtk_widget_set_sensitive( next_button, FALSE );
    }
    else
    {
	gtk_widget_set_sensitive( next_button,
			 	( WinServiceID[0] == '\0' ||
				WinServicePwd[0] == '\0' ) ?
				FALSE : TRUE );
    }
}


void
on_WinAddIngPathCheck_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    WinMiscOps ^= GIP_OP_WIN_ADD_TO_PATH;
}


void
on_WinInstDemoClick_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    WinMiscOps ^= GIP_OP_INSTALL_DEMO;
}


void
on_remove_all_files_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    misc_ops ^= GIP_OP_REMOVE_ALL_FILES;    
 
    /* warn user when selected */
    if ( misc_ops & GIP_OP_REMOVE_ALL_FILES )
    {
	GtkWidget	*warning;
	char		*msgbuf;
	char tmpbuf[ MAX_LOC + 3 ];
	SIZE_TYPE	bufsize;
	auxloc	*locptr;
	STATUS		status;

	/*
	** Need to warn about removing all locations, allocate the memory
	** needed print all the locations in a single string.
	*/
# define WARNING_REMOVE_FILES "All files under the following locations will be removed:\n"
	bufsize = selected_instance->numdlocs * ( MAX_LOC + 3 ) 
			+ selected_instance->numllocs * ( MAX_LOC + 3 )
			+ STlen( WARNING_REMOVE_FILES )
			+ MAX_LOC + 3;
	msgbuf = MEreqmem( 0, bufsize, FALSE, &status );
	
	/* start with II_SYSTEM */
	STcopy( WARNING_REMOVE_FILES, msgbuf );
	STprintf( tmpbuf, "\t%s/ingres\n", selected_instance->inst_loc );
	STcat( msgbuf, tmpbuf );

	/* then database locations */
	locptr = selected_instance->datalocs;
	if ( locptr == NULL )
	    DBG_PRINT( "datalocs is NULL, no aux locations found\n" );

	while ( locptr != NULL )
	{

	    STprintf( tmpbuf, "\t%s/ingres/%s\n",
			locptr->loc,
			dblocations[locptr->idx]->dirname );
	    STcat( msgbuf, tmpbuf );
	    locptr = locptr->next_loc ;
	}
	    
	/* and then log file locations */
	locptr = selected_instance->loglocs;
	if ( locptr == NULL )
	    DBG_PRINT( "loglocs is NULL, no aux locations found\n" );

	while ( locptr != NULL )
	{

	    STprintf( tmpbuf, "\t%s/ingres/log\n",
			locptr->loc );
	    STcat( msgbuf, tmpbuf );
	    locptr = locptr->next_loc ;
	}

	/* create the dialog */
	warning = gtk_message_dialog_new( GTK_WINDOW( IngresInstall ),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_WARNING,
					GTK_BUTTONS_CLOSE,
					msgbuf );

# define WARNING_DIALOG_TITLE "Warning"
	gtk_window_set_title( GTK_WINDOW( warning ), WARNING_DIALOG_TITLE );

	/* run it */
	gtk_dialog_run( GTK_DIALOG( warning ) );
	gtk_widget_destroy( warning );

	MEfree( msgbuf );
    }
}


void
on_up_all_usr_db_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    misc_ops ^= GIP_OP_UPGRADE_USER_DB;
}

void
on_change_charset_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
    /* Display selection dialog */
    gtk_dialog_run( GTK_DIALOG( CharsetDialog ) );
    gtk_widget_hide( CharsetDialog );
}

void
on_change_timezone_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
    /* Display selection dialog */
    gtk_dialog_run( GTK_DIALOG( TimezoneDialog ) );
    gtk_widget_hide( TimezoneDialog );
}

void
popup_list_selected	(GtkTreeSelection *selection,
					gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar	*value;

    /* only send response if item has been selected */
    if ( gtk_tree_selection_get_selected( selection, &model, &iter ) )
        gtk_dialog_response( GTK_DIALOG( data ), GTK_RESPONSE_OK );
}

void
update_locale_entry( GtkTreeSelection *selection,
					gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar	*value;

    if ( gtk_tree_selection_get_selected(selection, &model, &iter ) )
    {
        DBG_PRINT( "locale entry_updated\n" );
	gtk_tree_model_get( model, &iter, 0, &value, -1);
	gtk_entry_set_text( GTK_ENTRY( data ), value );
    }
}

void
on_default_inst_group_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget	*inst_group_entry;
    GtkWidget	*next_button;
	
    inst_group_entry = lookup_widget(IngresInstall, "inst_group_entry");
    next_button = lookup_widget( IngresInstall, "next_button");
		
    /* Toggle default user option */
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ) )
					== TRUE )
        misc_ops &= ~GIP_OP_INST_CUST_GRP;
    else
        misc_ops |= GIP_OP_INST_CUST_GRP;
	
    if ( misc_ops & GIP_OP_INST_CUST_GRP )
    {	
	GtkWidget	*next_button;
	/* No */
	/* Unshade the username entry field */
	gtk_widget_set_sensitive( inst_group_entry, TRUE );

	/*
	** check the contents of the field. If it's empty, 
	** grey the next button
	*/
	groupid = (char *)gtk_entry_get_text( GTK_ENTRY(inst_group_entry) );
		
    }
    else
    {
	gtk_widget_set_sensitive( inst_group_entry, FALSE );
	groupid = (char *)defaultuser;
    }

    if ( groupid[0] == '\0' || userid[0] == '\0' )
	gtk_widget_set_sensitive( next_button, FALSE );
    else
	gtk_widget_set_sensitive( next_button, TRUE );
}

void
on_inst_group_entry_changed            (GtkEditable     *editable,
                                        gpointer         user_data)
{
    GtkWidget	*next_button;

    next_button=lookup_widget(IngresInstall, "next_button");
    /*
    ** check the contents of the field. If it's empty, 
    ** grey the next button otherwise, un-grey it
    */
    groupid = (char *)gtk_entry_get_text( GTK_ENTRY(editable) );
    gtk_widget_set_sensitive( next_button, ( userid[0] == '\0' ||
						groupid[0] == '\0' ) ?
						FALSE : TRUE );
}

void
on_CharsetEntry_changed                (GtkEditable     *editable,
                                        gpointer         user_data)
{
    locale_info.charset = (char *)gtk_entry_get_text( GTK_ENTRY(editable) );
    GIPValidateLocaleSettings();
}


void
on_TimezoneEntry_changed               (GtkEditable     *editable,
                                        gpointer         user_data)
{
    locale_info.timezone = (char *)gtk_entry_get_text( GTK_ENTRY(editable) );
    GIPValidateLocaleSettings();
}

void
on_TermTypeEntry_changed               (GtkEditable     *editable,
                                        gpointer         user_data)
{

}

void
on_log_size_hscale_value_changed       (GtkRange        *range,
                                        gpointer         user_data)
{
    GtkWidget	*txlog_size_spinner;

    /* lookup spinner and set new value */
    txlog_size_spinner = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"txlog_size_spinner" :
						"mod_txlog_size_spinner" );
    gtk_spin_button_set_value( GTK_SPIN_BUTTON( txlog_size_spinner ), 
				gtk_range_get_value( range ) );

}

void
on_ansi_date_type_radio_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    /* toggle setting for II_DATE_TYPE_ALIAS */
    date_type_alias.val_idx = date_type_alias.val_idx == ANSI ? INGRES : ANSI ;
    DBG_PRINT( "II_DATE_TYPE_ALIAS set to %s\n",
		date_type_alias.values[date_type_alias.val_idx] );
}

void
get_selected_RF_filename		( GtkWidget *widget,
					gpointer user_data )
{
    GtkWidget *file_selector;
    LOCATION newrfloc;
    char *filename;
    char tmpbuf[MAX_LOC];
    STATUS  rc;
  
    file_selector = GTK_WIDGET( user_data );
    filename = (char *)gtk_file_selection_get_filename(
					GTK_FILE_SELECTION( file_selector ) );
  
    LOfroms( FILENAME & PATH, filename, &newrfloc );
    rc = SIcopy( &rfnameloc, &newrfloc );
    if ( rc != OK )
    {
# define ERROR_RF_SAVE_FAILED "Failed to save response file to %s\n"
        STprintf( tmpbuf, ERROR_RF_SAVE_FAILED, filename );
        popup_error_box( tmpbuf );
    }
    else
        DBG_PRINT( "Response file saved at: %s\n", filename );

}
void
on_ing_config_tx_radio_selected            (GtkRadioButton  *radiobutton,
                                        gpointer         user_data)
{
    GtkWidget	*date_alias_radio;

    ing_config_type.val_idx = TXN;
    DBG_PRINT( "II_CONFIG_TYPE set to %s\n",
		ing_config_type.values[ing_config_type.val_idx] );

    /* Make sure date alias has the correct default */
    date_alias_radio=lookup_widget(IngresInstall, "ansi_date_type_radio");
    if (! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(date_alias_radio)) )
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(date_alias_radio), TRUE);
}

void
on_ing_config_bi_radio_selected            (GtkRadioButton  *radiobutton,
                                        gpointer         user_data)
{
    GtkWidget	*date_alias_radio;

    ing_config_type.val_idx = BI;
    DBG_PRINT( "II_CONFIG_TYPE set to %s\n",
		ing_config_type.values[ing_config_type.val_idx] );
    /* Make sure date alias has the correct default */
    date_alias_radio=lookup_widget(IngresInstall, "ansi_date_type_radio");
    if (! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(date_alias_radio)) )
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(date_alias_radio), TRUE);

}

void
on_ing_config_cm_radio_selected            (GtkRadioButton  *radiobutton,
                                        gpointer         user_data)
{
    GtkWidget	*date_alias_radio;

    ing_config_type.val_idx = CM;
    DBG_PRINT( "II_CONFIG_TYPE set to %s\n",
		ing_config_type.values[ing_config_type.val_idx] );

    /* Make sure date alias has the correct default */
    date_alias_radio=lookup_widget(IngresInstall, "ansi_date_type_radio");
    if (! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(date_alias_radio)) )
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(date_alias_radio), TRUE);

}

void
on_ing_config_classic_radio_selected            (GtkRadioButton  *radiobutton,
                                        gpointer         user_data)
{
    GtkWidget	*date_alias_radio;

    ing_config_type.val_idx = TRAD;
    DBG_PRINT( "II_CONFIG_TYPE set to %s\n",
		ing_config_type.values[ing_config_type.val_idx] );

    /* Make sure date alias has the correct default */
    date_alias_radio=lookup_widget(IngresInstall, "ingres_date_type_radio");
    if (! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(date_alias_radio)))
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(date_alias_radio), TRUE);
}

# endif /* xCL_GTK_EXISTS */
