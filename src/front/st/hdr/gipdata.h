/*
** Copyright 2006 Ingres Corporation. All rights reserved.
*/

/*
** Name: gipdata.h
** 
** Description:
**	Header file for defining GLOBALREFs for Linux GUI installer and
**	package manager.
**
** History:
**	02-Oct-2006 (hanje04)
**	    SIR 116877
**	    Created.
**	25-Oct-2006 (hanje04)
**	    SIR 116877
**	    Add globals for Charset and Timzone dialogs
**	    Add groupid
**	01-Dec-2006 (hanje04)
**	    Update GLOBALREF for existing_instance.
**	    Add GLOBALREF for date_type_alias structure.
**	07-Dec-2006 (hanje04)
**	    BUG 117278
**	    SD 112944
**	    Add adv_nodbms_stages[] for case where DBMS package is NOT selected
**	    for installation. 
**	22-Feb-2007 (hanje04)
**	    SIR 116911
**	    Add run_platform to define the OS we're running on so we can tell
**	    if it differs from the deployment platform.
**	08-Jul-2009 (hanje04)
**	    SIR 122309
**	    Add GLOBALREF for ing_config_type.
**	19-May-2010 (hanje04)
**	    SIR 123791
**	    Add GLOBALREFs for new VW config structures and stages.
*/

/* Global refs */
/* Instance location and ID */
GLOBALREF char LnxDefaultII_SYSTEM[];
GLOBALREF char WinDefaultII_SYSTEM[];
GLOBALREF char *default_ii_system;
GLOBALREF location iisystem;
GLOBALREF char instID[];
GLOBALREF char dfltID[];

/* DB locations */
GLOBALREF location *dblocations[];

/* transaction log */
GLOBALREF log_info txlog_info;

/* vectorwise config */
GLOBALREF vw_cfg *vw_cfg_info[];
GLOBALREF char vw_cfg_units[];

/* misc options */
GLOBALREF misc_op_info *misc_ops_info[];
GLOBALREF MISCOPS misc_ops;
GLOBALREF MISCOPS WinMiscOps;

/* locale settings */
GLOBALREF ing_locale locale_info;
GLOBALREF TIMEZONE timezones[];
GLOBALREF CHARSET linux_charsets[];
GLOBALREF CHARSET windows_charsets[];

/* ingres config type */
GLOBALREF struct _ing_config_type {
		II_RFAPI_PNAME	rfapi_name;	
		const char	*values[4];
		const char	*descrip[4];
		enum {
		    TXN,
		    BI,
		    CM,
		    TRAD
		    }		val_idx;
		} ing_config_type ;
/* date type alias */
GLOBALREF struct _date_type_alias {
		II_RFAPI_PNAME	rfapi_name;	
		const char	*values[2];
		enum {
		    ANSI,
		    INGRES,
		    }		val_idx;
	} date_type_alias ;
/* packages */
GLOBALREF package *packages[] ;
GLOBALREF package *licpkgs[] ;

/* GTK Globals */
GLOBALREF GtkWidget *IngresInstall;
GLOBALREF GtkWidget *CharsetDialog;
GLOBALREF GtkWidget *TimezoneDialog;
GLOBALREF GtkWidget *InstanceTreeView;
GLOBALREF GtkTreeIter *timezone_iter;		
GLOBALREF GtkTreeIter *charset_iter;		
GLOBALREF GtkTreeModel *CharsetTreeModel;
GLOBALREF GtkTreeModel *TimezoneTreeModel;

/* Installer variables */
GLOBALREF i4 instmode;
GLOBALREF i4 runmode;
GLOBALREF PKGLST pkgs_to_install;
GLOBALREF PKGLST mod_pkgs_to_install;
GLOBALREF PKGLST mod_pkgs_to_remove;
GLOBALREF const gchar install_notebook[];
GLOBALREF const gchar upgrade_notebook[];
GLOBALREF const gchar *current_notebook;
GLOBALREF bool upgrade;
GLOBALREF UGMODE ug_mode;
GLOBALREF UGMODE inst_state;
GLOBALREF instance *existing_instances[];
GLOBALREF instance *selected_instance;
GLOBALREF saveset new_pkgs_info;
GLOBALREF const char *WinServiceID;
GLOBALREF const char *WinServicePwd;
GLOBALREF char *userid;
GLOBALREF char *groupid;
GLOBALREF const char *defaultuser;
GLOBALREF i4	inst_count;
GLOBALREF i4 debug_trace;

/* Stage info */
GLOBALREF stage_info *adv_stages[];
GLOBALREF stage_info *adv_ivw_stages[];
GLOBALREF stage_info *adv_nodbms_stages[];
GLOBALREF stage_info *basic_stages[];
GLOBALREF stage_info *basic_ivw_stages[];
GLOBALREF stage_info *rfgen_lnx_stages[];
GLOBALREF stage_info *rfgen_win_stages[];
GLOBALREF stage_info *ug_stages[];
GLOBALREF stage_info **stage_names;
GLOBALDEF i4 current_stage;

/* Response file and installation globals */
GLOBALREF LOCATION	rfnameloc ;
GLOBALREF char	rfnamestr[] ;
GLOBALREF char	rfdirstr[] ;
GLOBALREF LOCATION      rndirloc ;
GLOBALREF char          rndirstr[] ;
GLOBALREF II_RFAPI_PLATFORMS dep_platform ;
GLOBALREF II_RFAPI_PLATFORMS run_platform ;
GLOBALREF char  inst_stdout[];
GLOBALREF char  inst_stderr[];
GLOBALREF _childinfo childinfo;
