/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**
**  Name: mllist.sc -- contains function which controls the DBMS server
**	must log DB list form of the INGRES Configuration Utility.
**
**  History:
**	 8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**		Copied/modified from dblist.sc
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
static char *dbms_mllist = ERx( "dbms_mllist" );
exec sql end declare section;

bool
dbms_mllist_form( char *dbms_id, char *db_list_in, char **db_list_out )
{
	exec sql begin declare section;
	char *def_name;
	char in_buf[ BIG_ENOUGH ];
	exec sql end declare section;

	LOCATION *IIUFlcfLocateForm();
	STATUS IIUFgtfGetForm();
	bool changed = FALSE;

	if( IIUFgtfGetForm( IIUFlcfLocateForm(), dbms_mllist ) != OK )
	{
		IIUGerr( S_ST031A_form_not_found, UG_ERR_FATAL, 1,
			dbms_mllist );
	}

	if( STcompare( dbms_id, ERx( "*" ) ) == 0 )
		def_name = ERget( S_ST030F_default_settings );
	else
		def_name = dbms_id;

	exec frs inittable dbms_mllist databases read;

	exec frs display dbms_mllist;

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
			exec frs loadtable dbms_mllist databases (
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

		exec frs insertrow dbms_mllist databases 0 (
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
		exec frs inquire_frs table dbms_mllist (
			:rows = datarows( databases )
		);
		if( rows < 1 ) 
			exec frs resume;

		exec frs deleterow dbms_mllist databases;
		changed = TRUE;
	exec frs end;

	exec frs activate menuitem :ERget( FE_Help ), frskey1;
	exec frs begin;
		if( !get_help( ERx( "dbms.mustlog_db_list" ) ) )
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
			exec frs unloadtable dbms_mllist databases (
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
			STprintf( resource, ERx( "%s.%s.dbms.%s.mustlog_db_list"),
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

