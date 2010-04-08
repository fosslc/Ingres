/*
** Copyright Ingres Corporation 2006. All rights reserved
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

# include <compat.h>
# include <gl.h>
# include <st.h>
# include <si.h>
# include <nm.h>
# include <lo.h>

# ifdef xCL_GTK_EXISTS

# include <gtk/gtk.h>

# include <gipinterf.h>
# include <gipsup.h>
# include <gipcb.h>
# include <gip.h>
# include <gipdata.h>
# include <giputil.h>
# include "iipackman.h"

/*
** Name: iipackman.c
**
** Description:
**
**      Main module for respsonse file generation program. Uses same GUI
**      interface and the Linux GUI installer.
**
** History:
**
**	05-Oct-2006 (hanje04)
**	    SIR 116877
**     	    Created.
**	25-Oct-2006 (hanje04)
**	    SIR 116877
**	    Load Charset and Timezone dialogs during setup instead of
**	    launch.
**	26-Oct-2006 (hanje04)
**	    SIR 116877
**	    Add ability to turn on tracing by setting II_GIP_TRACE.
**	06-Nov-2006 (hanje04)
**	    SIR 116877
**	    Need to be able to NOT build the GTK code if it's not available
**	    on a given platform. Currently only supported on Linux.
**	02-Feb-2007 (hanje04)
**	    SIR 116877
**	    Add function stub for install_control_thread(), to resolve
**	    undefined references from inginstgui specific code in
**	    front!st!gtkinstall_unix_win giputil.c
**	22-Feb-2007 (hanje04)
**	    SIR 116911
**	    Initialize new_pkgs_info.pkgs so that any of the packages can be
**	    selected by the package manger.
*/

STATUS
main (int argc, char *argv[])
{
    GtkWidget	*widget_ptr; 
    LOCATION	pixmaploc;
    char	*tracevar;
    STATUS	rc;
	
    /* initialize GTK */
    gtk_set_locale ();
    gtk_init (&argc, &argv);
 
    /* Get location of pixmap files */
    rc = NMloc( FILES, PATH, "pixmaps", &pixmaploc );
    if ( rc != OK )
    {
	DBG_PRINT( "NMloc failed with %d\n", rc );
	return(rc);
    }

    /* turn on tracing? */
    NMgtAt( "II_GIP_TRACE", &tracevar );
    if ( tracevar && *tracevar )
	debug_trace = 1 ;

    DBG_PRINT( "Pixmap dir = %s\n", pixmaploc.string );
    add_pixmap_directory( pixmaploc.string );
	
    /* set defaults */
    userid = (char *)defaultuser;
    groupid = (char *)defaultuser;
    SET_II_SYSTEM( default_ii_system );

    
    /* create main install window, hookup the quit prompt box and show it */
    IngresInstall=create_IngresInstall();
    g_signal_connect (G_OBJECT (IngresInstall), "destroy",
                      G_CALLBACK (delete_event), NULL);
	
    /* initialize mode */
    if ( init_rfgen_mode() != OK )
    {
	  printf( "Failed to initialize new installation\n" );
	  return(1);
    }
  
    /* set start screen */
    set_screen(START_SCREEN);
    /* show the wizard */
    gtk_widget_show(IngresInstall);
    
    gtk_main ();
  
    /* clean up before exit */
    rfgen_cleanup();

    return( EXIT_STATUS );
}

STATUS
init_rfgen_mode(void)
{
    GtkWidget	*widget_ptr;
    char	*IDptr;

    /* set the title */
# define RFGEN_TITLE "Ingres Package Manager"
    gtk_window_set_title( GTK_WINDOW( IngresInstall ), RFGEN_TITLE );

    /* set mode */
    runmode = MA_INSTALL;
    instmode = ( dep_platform == II_RF_DP_LINUX ?
			RFGEN_LNX : RFGEN_WIN );
    ug_mode = UM_INST;

    widget_ptr = lookup_widget( IngresInstall, "rfdef_linux_radio" );
# ifdef NT_GENERIC 
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget_ptr ), FALSE );
# endif
   
    /* set default instance name */
    STcopy( RFAPI_DEFAULT_INST_ID, dfltID );
    STcopy( dfltID, instID );
 
    /* and set the widgets */
    widget_ptr = lookup_widget(IngresInstall, "instID_entry2");
    gtk_entry_set_text( GTK_ENTRY( widget_ptr ), dfltID );

    /* set ii_system */
    widget_ptr = lookup_widget( IngresInstall, "ii_system_entry" );
    gtk_entry_set_text( GTK_ENTRY( widget_ptr), default_ii_system );

    /* Create dialogs to act as combo boxes */
    /* timezones */
    TimezoneTreeModel = initialize_timezones();
    widget_ptr = lookup_widget( IngresInstall, "TimezoneEntry" );
    TimezoneDialog = create_text_view_dialog_from_model( TimezoneTreeModel,
							widget_ptr,
							timezone_iter );


    /* charsets */
    CharsetTreeModel = initialize_charsets( linux_charsets );
    widget_ptr = lookup_widget( IngresInstall, "CharsetEntry" );
    CharsetDialog = create_text_view_dialog_from_model( CharsetTreeModel,
							widget_ptr,
							charset_iter );
    
  
    /* hide backup log location */
    widget_ptr=lookup_widget(IngresInstall, "bkp_log_file_loc");
    gtk_widget_set_sensitive(widget_ptr, FALSE);
    widget_ptr=lookup_widget(IngresInstall, "bkp_log_browse");
    gtk_widget_set_sensitive(widget_ptr, FALSE);
    widget_ptr=lookup_widget(IngresInstall, "bkp_log_label");
    gtk_widget_set_sensitive(widget_ptr, FALSE);	
  
    /* set default package lists */
    new_pkgs_info.pkgs = FULL_INSTALL;
    set_inst_pkg_check_buttons();
  
    /* hide/show all required train items */
    widget_ptr=lookup_widget(IngresInstall, "adv_config");
    gtk_widget_show(widget_ptr);
    widget_ptr=lookup_widget(IngresInstall, "instance_name_box");
    gtk_widget_show(widget_ptr);
    widget_ptr=lookup_widget(IngresInstall, "RfPlatformBox" );
    gtk_widget_show(widget_ptr);
    widget_ptr=lookup_widget(IngresInstall, "RfEndTable" );
    gtk_widget_show(widget_ptr);
    widget_ptr=lookup_widget(IngresInstall, "ProgExpBox" );
    gtk_widget_hide(widget_ptr);
    widget_ptr=lookup_widget(IngresInstall, "InstEndTable" );
    gtk_widget_hide(widget_ptr);

    /* create formatting tags for summary & finish screens */
    widget_ptr=lookup_widget(IngresInstall, "RFSummaryTextView");
    create_summary_tags( gtk_text_view_get_buffer( 
  				GTK_TEXT_VIEW( widget_ptr ) ) );
    widget_ptr=lookup_widget(IngresInstall, "RFPreviewTextView");
    create_summary_tags( gtk_text_view_get_buffer( 
  				GTK_TEXT_VIEW( widget_ptr ) ) );
							
    return(OK);
}

STATUS
rfgen_cleanup( void )
{
    /* remove response file */
    if ( LOexist( &rfnameloc ) == OK )
	LOdelete( &rfnameloc );

    /* Free gtk stuff */
    gtk_tree_iter_free( timezone_iter );
    gtk_tree_iter_free( charset_iter );
}

/*
** Function stub for install_control_thread() which is referenced in
** the callback functions for packman but only used in inginstgui.
*/
void *
install_control_thread( void *arg )
{
    return;
}
# else /* xCL_GTK_EXISTS */

/*
** Dummy main() to quiet link errors on platforms which don't support
** GTK & RPM
*/
i4
main()
{
    return( FAIL );
}

# endif /* xCL_GTK_EXISTS */
