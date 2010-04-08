/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**
**  Name: dblist.sc -- contains function which controls the DBMS server
**	database access list form of the INGRES Configuration Utility.
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to new interface for multiple context PM
**		functions.
**	23-jul-93 (tyler)
**		Changed multiple context PM interfaces again. 
**	19-oct-93 (tyler)
**		Change type fo global variable "host" to (char *).
**	01-dec-93 (tyler)
**		Fixed bad calls to IIUGerr().
**	22-feb-94 (tyler)
**		Fixed BUG 59434: assign Help menu item to PF2.
**	30-nov-94 (nanpr01)
**		Fixed BUG 60404: Change parameter name for db_list to
**		database_list.
**	30-nov-94 (nanpr01)
**		Fixed BUG 65806: Initialized db_count to BIG_ENOUGH
** 		so that STgetwords work correctly.
**	30-nov-94 (nanpr01)
**		Fixed BUG 60371: Bound frskeys with the different CBF
** 		actions. If users want it they can do so by defining
**		frskey11 - frskey19 map.
**	27-nov-95 (nick)
**		VMS no longer requires a different format for database_list
**		as it isn't passed on the command line but obtained via PM.
**		Fixes #72814.
**	10-oct-1996 (canor01)
**		Replace hard-coded 'ii' prefixes with SystemCfgPrefix.
**	19-dec-1996 (reijo01)
**		Initialize ii_system_name and ii_installation_name in 
**		form.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
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
# include	<uigdata.h>
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

/*
** global variables
*/
exec sql begin declare section;
GLOBALREF char		*host;
GLOBALREF char		*ii_system;
GLOBALREF char		*ii_installation;
GLOBALREF char		*ii_system_name;
GLOBALREF char		*ii_installation_name;
exec sql end declare section;

GLOBALREF LOCATION	config_file;
GLOBALREF PM_CONTEXT	*config_data;

/*
** local forms
*/
exec sql begin declare section;
static char *dbms_dblist = ERx( "dbms_dblist" );
exec sql end declare section;

bool
dbms_dblist_form( char *dbms_id, char *db_list_in, char **db_list_out )
{
	exec sql begin declare section;
	char *def_name;
	char in_buf[ BIG_ENOUGH ];
	exec sql end declare section;

	LOCATION *IIUFlcfLocateForm();
	STATUS IIUFgtfGetForm();
	bool changed = FALSE;

	if( IIUFgtfGetForm( IIUFlcfLocateForm(), dbms_dblist ) != OK )
	{
		IIUGerr( S_ST031A_form_not_found, UG_ERR_FATAL, 1,
			dbms_dblist );
	}

	if( STcompare( dbms_id, ERx( "*" ) ) == 0 )
		def_name = ERget( S_ST030F_default_settings );
	else
		def_name = dbms_id;

	exec frs inittable dbms_dblist databases read;

	exec frs display dbms_dblist;

	exec frs initialize (
		host = :host,
		ii_installation = :ii_installation,
		ii_installation_name = :ii_installation_name,
		ii_system = :ii_system,
		ii_system_name = :ii_system_name,
		def_name = :def_name
	);

	exec frs begin;
	{
		exec sql begin declare section;
		char *db_names[ BIG_ENOUGH ];
		exec sql end declare section;

		char *db_list_copy, *p;
		i4 db_count = BIG_ENOUGH, i;

		/* make copy of db_list for parsing */
		db_list_copy  = STalloc( db_list_in );

# ifdef VMS
		/* 
		** on VMS, convert non-whitespace to whitespace.
		** nick> this is only here now to convert any
		** 'old' parenthesised entries to the current
		** format ; it can be removed at some stage.
		*/
		for (p = db_list_copy; *p != EOS; p++)
		{
		    if (*p == '(' || *p == ',' || *p == ')')
		    {
		    	*p = ' ';
			changed = TRUE;
		    }
		}
# endif /* VMS */

		/* parse whitespace delimited words from db_list_copy */
		STgetwords( db_list_copy, &db_count, db_names );

		/* load tablefield */
		for( i = 0; i < db_count; i++ )
		{
			exec frs loadtable dbms_dblist databases (
				name = :db_names[ i ]
			);
		}
		MEfree( db_list_copy );
	}
	exec frs end;

	exec frs activate menuitem 'Add',frskey11; 
	exec frs begin;
		/* prompt for new database name */
		exec frs prompt (
			:ERget( S_ST031C_add_db_prompt ),
			:in_buf
		) with style = popup;

		exec frs insertrow dbms_dblist databases 0 (
			name = :in_buf
		);
		changed = TRUE;
	exec frs end;

	exec frs activate menuitem 'Remove',frskey14;
	exec frs begin;
		exec sql begin declare section;
		i4 rows;
		exec sql end declare section;

		/* don't try to deleterow if tablefield empty */
		exec frs inquire_frs table dbms_dblist (
			:rows = datarows( databases )
		);
		if( rows < 1 ) 
			exec frs resume;

		exec frs deleterow dbms_dblist databases;
		changed = TRUE;
	exec frs end;

	exec frs activate menuitem :ERget( FE_Help ), frskey1;
	exec frs begin;
		if( !get_help( ERx( "dbms.database_list" ) ) )
			exec frs message 'No help available.'
				with style = popup;
	exec frs end;

	exec frs activate menuitem :ERget( FE_End ), frskey3;
	exec frs begin;
		exec sql begin declare section;
		i4 state;
		char db_name[ BIG_ENOUGH ];
		char db_list_buf[ BIG_ENOUGH ];	
		exec sql end declare section;
		char resource[ BIG_ENOUGH ];
		bool init = TRUE;

		if( changed )
		{
			STcopy( ERx( "" ), db_list_buf );
			/* re-construct db_list resource */
			exec frs unloadtable dbms_dblist databases (
				:db_name = name,
				:state = _state
			);
			exec frs begin;

				/* skip deleted rows */
				if( state == 4 )
					continue; /* hope this is OK */
				
				if( !init )
					STcat( db_list_buf, ERx( " " ) );
				else
					init = FALSE;
				STcat( db_list_buf, db_name );

			exec frs end;

			/* set the new db_list resource */
			STprintf( resource, ERx( "%s.%s.dbms.%s.database_list"),
				  SystemCfgPrefix, host, dbms_id );
			PMmDelete( config_data, resource );
			PMmInsert( config_data, resource, db_list_buf );

			/* write config.dat */
			if( PMmWrite( config_data, &config_file ) != OK )
			{
				IIUGerr( S_ST0314_bad_config_write, 0, 0 ); 
				return( FALSE );
			}

			/*
			** duplicate and return the modified db_list -
			** this will cause a minor memory leak.
			*/
			*db_list_out = STalloc( db_list_buf );
		}
		exec frs breakdisplay;
	exec frs end;

	return( changed );
}

