/*
** Copyright 2006 Ingres Corporation. All rights reserved.
*/

/*
** Name: giputil.h
** 
** Description:
**	Header file for useful functions used by the linux installer and 
**	response file generator
**
** History:
**	09-Oct-2006 (hanje04)
**	    SIR 116877
**	    Created.
**	25-Oct-2006 (hanje04)
**	    SIR 116877
**	    Add prototype for check_locale_selection.
**	16-Feb-2006 (hanje04)
**	    BUG 117700
**	    SD 113367
**	    Add prototype for prompt_shutdown(). Used when running instance
**	    is detected.
**	22-Feb-2007 (hanje04)
**	    SIR 116911
**	    Add prototype for GIPpathFull()
**	27-Feb-2007 (hanje04)
**	    SD 115861
**	    BUG 117801
**	    Add prototype for GIPvalidateLocs().
**	08-Oct-2008 (hanje04)
**	    Bug 121004
**	    SD 130670
**	    Add prototype for GIPValidateLocaleSettings()
**	10-Oct-2008 (hanje04)
**	    Bug 120984
**	    SD 131178
**	    Add prototype for GIPValidatePkgName()
**	10-Sep-2009 (hanje04)
**	    BUG 122571
**	    Add GIPAddDBLoc() and GIPAddLogLoc() for added aux locations to
**	    instance structure.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added prototype for write_file_to_text_buffer.
**	27-May-2010 (hanje04)
**	    SIR 123791
**	    Add prototype for GIPnextPow2(), GIPisPow2 macro.
*/

/* MACROS */
# ifdef UNIX
# define SET_II_SYSTEM(p)	{ \
		STcopy( RFAPI_LNX_DEFAULT_INST_LOC, LnxDefaultII_SYSTEM ); \
		STcopy( RFAPI_WIN_DEFAULT_INST_LOC, WinDefaultII_SYSTEM ); \
		p = LnxDefaultII_SYSTEM ; }
# define TMPDIR "/tmp"
# define TMPVAR "TMPDIR"
# else
# define SET_II_SYSTEM(p)	{ \
		STcopy( RFAPI_LNX_DEFAULT_INST_LOC, LnxDefaultII_SYSTEM ); \
		STcopy( RFAPI_WIN_DEFAULT_INST_LOC, WinDefaultII_SYSTEM ); \
		p = WinDefaultII_SYSTEM ; }
# define TMPDIR "."
# define TMPVAR "TEMP"
# endif
# ifndef GIPisPow2
# define GIPisPow2(x) ((x & (x - 1)) == 0)
# endif


	

/* function prototypes */
void
inst_pkg_sel				( PKGIDX pkgidx );

void
inst_pkg_rmv				( PKGIDX pkgidx );

void
ug_pkg_sel				( PKGIDX pkgidx );

void
ug_pkg_rmv				( PKGIDX pkgidx );

void
create_summary_tags			( GtkTextBuffer *buffer );

void
write_installation_summary		( GtkTextBuffer *buffer );

GtkTreeModel *
initialize_timezones			( void );

GtkTreeModel *
initialize_charsets			( CHARSET *charsets );

II_RFAPI_STATUS
generate_response_file			( void );

II_RFAPI_STATUS
addMiscOps				( II_RFAPI_HANDLE *Handleptr,
					MISCOPS miscops );
void 
popup_error_box				( gchar *errmsg );

STATUS
inst_name_in_use			( char	*instname );

bool
is_renamed				( char *basename );

void
increment_wizard_progress		( void );

void
write_upgrade_summary			( GtkTextBuffer *buffer );

void
add_instance_columns			(GtkTreeView *treeview);

int
add_instance_to_store			( GtkListStore *list_store,
					instance *instance_info );

void *
install_control_thread			( void *arg );

void
set_mod_pkg_check_buttons		( PKGLST packages );

GtkWidget *
create_text_view_dialog_from_model	( GtkTreeModel *model,
					GtkWidget *entry,
					GtkTreeIter *dflt_iter );
gboolean
check_locale_selection			( GtkTreeSelection *selection,
                                          GtkTreeModel *model,
                                          GtkTreePath *path,
                                          gboolean path_currently_selected,
                                          gpointer data);
STATUS
prompt_shutdown				( void );

bool
GIPpathFull				( LOCATION *loc );

STATUS
GIPvalidateLocs				( char	*iisystem );

void
GIPValidateLocaleSettings		( void );

STATUS
GIPValidatePkgName			( char *pkgname, char *format );

STATUS
GIPAddDBLoc				(instance *inst,
					 i4 type,
					 const char *path);

STATUS
GIPAddLogLoc				(instance *inst,
					 i4 type,
					 const char *path);

int
write_file_to_text_buffer		(char *fileloc,
					 GtkTextBuffer *buffer);

i4
GIPnextPow2				(i4  num);
