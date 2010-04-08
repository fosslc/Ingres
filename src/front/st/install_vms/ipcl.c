/*
**  Copyright (c) 1993, 2001 Ingres Corporation
**
**  ipcl.c -- VMS IP compatibility layer intended to isolate the platform
**	dependent code upon which front!st!install depends.
**
**  History:
**	11-nov-93 (tyler)
**		Created.  This version only supports the iimaninf
**		executable; additional functions will need to be
**		implemented to to support the ingbuild executable.
**	10-dec-93 (tyler)
**		Added IPCL_LOfroms().
**	10-dec-93 (tyler)
**		Previous change was botched.  This change required to
**		submit correct version.
**	11-jan-93 (joplin)
**		Changed IPCL_LOfroms to translate device logical names
**		and to initialize empty LOC structures.
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	16-oct-2001 (kinte01)
**	    Add include of si.h to pick up definition of FILE due to 
**	    change 453278 for bug 105848
*/

# include <compat.h>
# include <cm.h>
# include <er.h>
# include <id.h>
# include <lo.h>
# include <st.h>
# include <me.h>
# include <si.h>
# include <iplist.h>
# include <ip.h>
# include <ipcl.h>
# include <erst.h>

/*
** IPCLbuildPath() -- construct a native path given a generic path.
*/ 

char *IPCLbuildPath( char *generic_path, IPCL_ERR_FUNC *error )
{
	char *locbuf = (char *) ip_alloc_blk( MAX_LOC + 1 );
	char *p = generic_path;

	if( *p == '(' )
	{
		char *pp;


		for( pp = p; *pp != ')' && *pp != EOS; CMnext( pp ) );
		if( *pp == EOS )
		{
			error( "Expecting ')' in path specification." );
			return( NULL );
		}
		CMnext( p );
		*pp = EOS;
		STcopy( p, locbuf );
		STcat( locbuf, ERx( ":" ) );
		*pp = ')';
		p = pp;
		CMnext( p );
		if( *p == EOS )
			return( locbuf );
		if( *p != '!' )
		{
			error( "Expecting '!' in path specification." );
			return( NULL );
		}
		CMnext( p );
	}
	else
		STcopy( ERx( "" ), locbuf );	

	STcat( locbuf, ERx( "[" ) );
	STcat( locbuf, p );
	for( p = locbuf; *p != EOS; CMnext( p ) )
	{
		if( *p == '!' )
			*p = '.';
	}
	STcat( locbuf, ERx( "]" ) );

	return( locbuf );
}

/*
** IPCLbuildPermission() -- construct a native PERMISSION given a
**	generic permission string from the release description
**	file (release.dat).
*/ 

PERMISSION IPCLbuildPermission( char *s, IPCL_ERR_FUNC *error )
{
	static PERMISSION permission;
	i4 len = STlength( s );

	if( STbcompare( ERx( ",setuid" ), 0, s + len - 7, 0, FALSE ) != 0 )
	{
		permission = ip_stash_string( s );
		return( permission );
	}
	permission = ip_alloc_blk( len - 6 );
	MEcopy( s, len - 7, permission );
	permission[ len - 7 ] = EOS;

	return( permission );
}

/*
** IPCLcreateDir -- create a directory, given a native relative directory
**	specification.
*/

STATUS
IPCLcreateDir( char *dirName )
{
	STATUS status;
	char cmdline[ 256 ];
	char locBuf[ MAX_LOC + 1 ];
	i4 flagword = LO_I_TYPE;
	LOINFORMATION locInfo;
	LOCATION loc;

	STcopy( dirName, locBuf );

	if( (status = LOfroms( PATH, locBuf, &loc )) != OK )
		return( status );
	
	if( LOinfo( &loc, &flagword, &locInfo ) == OK ) 
		return( OK );

	status = LOcreate( &loc );

	return( status );
}

/*
** IPCL_LOaddPath() -- add a relative directory path to an INGRES LOCATION,
**	given the native representation of the relative path.
*/

void IPCL_LOaddPath( LOCATION *loc, char *dirPath )
{
	char *cp;
	i4 componentCount = MAX_LOC, i;
	char *pathComponents[ 100 ];

	dirPath = STalloc( dirPath );
	for( cp = dirPath; *cp != EOS; CMnext( cp ) )
	{
		if ( *cp == '[' || *cp == '.' || *cp == ']' )
			*cp = ' ';
	}

	/* parse whitespace delimited directories */ 
	STgetwords( dirPath, &componentCount, pathComponents );

	/* Add to the directory path */
	for( i = 0; i < componentCount; i++ )
		LOfaddpath( loc, pathComponents[ i ], loc );

	MEfree( dirPath );
}

/*
** Data structure used to store temporary environment settings.
*/

static struct {
	char *name;
	char *value;
	char *old_value;
} tmpEnv[ 100 ];

static i4 numEnv = -1;

/*
** IPCLsetEnv() -- set name = value in the current environment.
*/

void
IPCLsetEnv( char *name, char *value, bool temp )
{
	i4 i;
	char buf[ 512 ];

	/*
	** This should look up the current JOB logical before changing its
	** value and store it in the old_value field of tmpEnv[].
	*/
	if( value == NULL || *value == EOS )
		value = ERx( " " );	

	STpolycat( 5, ERx( "define/job " ), name, ERx( " \"" ), value,
		ERx( "\"" ), buf);

	(void) ip_cmdline( buf, (ER_MSGID) NULL );

	if( temp )
	{
		for( i = 0; i <= numEnv; i++ )
		{
			if( STcompare( name, tmpEnv[ i ].name ) == 0 )
				break;
		}
		if( i > numEnv )
			i = ++numEnv;
		tmpEnv[ i ].name = ip_stash_string( name ); 
		tmpEnv[ i ].value = ip_stash_string( value ); 
		tmpEnv[ i ].old_value = NULL;
	}
}

/*
** IPCLclearTmpEnv() - undo environment settings made with IPCLsetEnv().
**	On VMS, this should reassign previously old logical values.
*/

void
IPCLclearTmpEnv( void )
{
	i4 i;

	if( numEnv < 0 )
		return;
	for( i = 0; i <= numEnv; i++ )
	{
		char buf[ 512 ];

		if( tmpEnv[ i ].old_value == NULL )
		{
			STpolycat( 2, ERx( "deassign/job " ),
				tmpEnv[ i ].name, buf);
			(void) ip_cmdline( buf, (ER_MSGID) NULL );
		}
		else
		{
			char buf[ 512 ];

			STpolycat( 5, ERx( "define/job " ), tmpEnv[ i ].name,
				ERx( " \"" ), tmpEnv[ i ].old_value,
				ERx( "\"" ), buf);
			(void) ip_cmdline( buf, (ER_MSGID) NULL );
		}
	}
}

/*
** IPCLresetTmpEnv() - restore environment settings made by previous calls
**	to IPCLsetEnv(). 
*/

void
IPCLresetTmpEnv( void )
{
	i4 i;

	if( numEnv < 0 )
		return;
	for( i = 0; i <= numEnv; i++ )
	{
		char buf[ 512 ];

		STpolycat( 5, ERx( "define/job " ), tmpEnv[ i ].name,
			ERx( " \"" ), tmpEnv[ i ].value, ERx( "\"" ), buf);
		(void) ip_cmdline( buf, (ER_MSGID) NULL );
	}
}

/*
** IPCL_froms() - temporary replacement for broken VMS CL interface.
*/
 
STATUS IPCL_LOfroms( LOCTYPE what, char *locBuf, LOCATION *loc )
{
	char *cp;
	i4 componentCount = MAX_LOC, i;
	char *pathComponents[ 100 ];
	char *pathBuf;
	char tmpBuf[ MAX_LOC + 1 ];
	STATUS status;

	if( STlength(locBuf) > MAX_LOC )
		return( FAIL );

	if( what == FILENAME || *locBuf == EOS )
		return( LOfroms( what, locBuf, loc ) );

	/* Make a copy of the original location */ 
	STcopy( locBuf, tmpBuf );
	pathBuf = tmpBuf;

	/* Parse off the device name and check to see if its a logical name */
	if ( (cp = STindex( tmpBuf, ERx(":"), 0 )) != NULL )
	{
		/* NULL terminate the device and try to translate it */
		*cp = EOS;
		NMgtAt(tmpBuf, &cp);
		if (cp && *cp)
		{
			/* Form a new name using the translated device name */
			STcopy( cp , tmpBuf);

			/* Concatenate the colon if it got translated away */
			if (STindex( tmpBuf, ERx(":"), 0 ) == NULL)
				STcat( tmpBuf, ERx(":") );

			/* Concatenate the string after the device name */
			cp = STindex( locBuf, ERx(":"), 0 );
			CMnext(cp);
			STcat( tmpBuf, cp );
		}

		/* If no logical name then restore the original */
		else
		{
			STcopy( locBuf, tmpBuf);
		}
	}
 

	for( cp = pathBuf; *cp != EOS; CMnext( cp ) )
	{
		/* don't replace anything after trailing ']' */
		if ( STindex( cp, ERx( "]" ), 0 ) == NULL ) 
			break;

		if( *cp == '[' || *cp == '.' || *cp == ']' )
			*cp = ' ';
	}
 
	/* parse whitespace delimited directories */
	STgetwords( pathBuf, &componentCount, pathComponents );
 
	STcopy( pathComponents[ 0 ], locBuf );

	(void) LOfroms( PATH, locBuf, loc );

	if( componentCount <= 1 )
	{
		return( OK );
	}
 
	/* Add to the directory path */
	for( i = 1; i < componentCount - 1; i++ )
	{
		if( (status = LOfaddpath( loc, pathComponents[ i ], loc ))
			!= OK )
		{
			return( status );
		}
	}

	switch( what )
	{
		case PATH & FILENAME:
			if( (status = LOfstfile( pathComponents[ i ], loc ))
				!= OK )
			{
				return( status ); 
			}
			break;

		case PATH:
			if( (status = LOfaddpath( loc, pathComponents[ i ],
				loc )) != OK )
			{
				return( status );
			} 
			break;

		default:
			/* not supported */
			;
	}
 
	return( OK );
}

