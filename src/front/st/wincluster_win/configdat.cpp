/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ConfigDat.cpp - Update the config.dat file for High Availability Option
** 
** Description:
** 	Scan config.dat to change 'machine name' to 'cluster name', using the PM
** 	routines of the CL to read and write information to and from config.dat.
** 
** History:
**	25-may-2004 (wonst02)
**	    Created.
** 	03-aug-2004 (wonst02)
** 		Add header comments.
*/

#include "stdafx.h"
#include "wincluster.h"
#include ".\configdat.h"

WcConfigDat::WcConfigDat(void) : verbose(false)
{
}

WcConfigDat::~WcConfigDat(void)
{
}

bool WcConfigDat::Init(void)
{
	PMmInit( &pm_id );
	PMmLowerOn( pm_id );

	if( PMmLoad( pm_id, NULL, PMerror ) != OK )
		return FALSE;

	/* prepare LOCATION for config.log */
	NMloc( FILES, FILENAME, ERx( "config.log" ), &change_log_file );

	/* open change log file for appending */
	if( SIfopen( &change_log_file , ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&change_log_fp ) != OK )
	{
		MyError( IDS_OPENWERROR, change_log_file.path );
		return false;
	}
	return true;
}

// Scan for resource names matching the pattern. Change the names "from" -> "to".
bool WcConfigDat::ScanResources(LPSTR lpszPattern, LPCSTR from, LPCSTR to)
{
	char *name = NULL;
	char *value = NULL;
	LPSTR rexp = lpszPattern;
	char *sOld = NULL;
	char *sNew = NULL;
	char newname[1024] = "";
	char tempbuf[BIG_ENOUGH];

	while( PMmScan( pm_id, rexp, &state, NULL, &name, &value ) == OK )
	{
	    if (from && to)
	    {
			sOld = strstr(name, from);
			if (sOld &&
		    	strlen(name) - strlen(from) + strlen(to) < sizeof(newname))
			{
		    	newname[0] = '\0';
		    	strncat( newname, name, (sOld - name) );
		    	strcat( newname, to );
		    	strcat( newname, sOld + strlen(from) );
		    	PMmDelete( pm_id, name );
		    	PMmInsert( pm_id, newname, value );
		    	SIfprintf( change_log_fp, "\nDELETE %s", name );
		    	STprintf( tempbuf, "%s (%s)", newname, value );
		    	SIfprintf( change_log_fp, "\nADD %s\n", tempbuf );
			}
	    }
	    rexp = NULL;
	}
	return true;
}

// Set a resource to a new value.
bool WcConfigDat::SetResource(char *name, char *value)
{
	char tempbuf[BIG_ENOUGH];

	if (name && value)
	{
		PMmDelete( pm_id, name );
		PMmInsert( pm_id, name, value );
		SIfprintf( change_log_fp, "\nDELETE %s", name );
		STprintf( tempbuf, "%s (%s)", name, value );
		SIfprintf( change_log_fp, "\nADD %s\n", tempbuf );
	}
	return true;
}

bool WcConfigDat::Term(void)
{
	if( PMmWrite( pm_id, NULL ) != OK )
	{
		MyError( IDS_CONFIGWRITEERROR, change_log_file.path );
		return false;
	}

	/* close change log after appending */
	(void) SIclose( change_log_fp );

	return true;
}

//
// Function passed to PMload() and PMmLoad() to display error messages.
//
void PMerror( STATUS status, i4  nargs, ER_ARGUMENT *args )
{
	i4 language;
	char buf[ BIG_ENOUGH ];
	i4 len;
	CL_ERR_DESC cl_err;

	if( ERlangcode( ERx( "english" ), &language ) != OK )
		return;
	if( ERlookup( status, NULL, ER_TEXTONLY, NULL, buf, sizeof( buf ),
		language, &len, &cl_err, nargs, args ) == OK )
	{
		MyError( IDS_PMERROR, buf );
	}
}

