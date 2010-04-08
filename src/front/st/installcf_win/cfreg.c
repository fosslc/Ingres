/*
** Copyright (c) 2000 Ingres Corporation
*/

/*
** Name: cfreg.c
**
** Description:
**      Functions to read registry keys for environment information.
**
**      GetRegistryValue        Gets a registry value from a specific key.
**      TestForKey              Tries to open the key.
**      GetIngresSystemPath     Return value of II_SYSTEM.
**      GetIngresICECFPath      Return the path on the ICE ColdFuision
**                              extension.
**      IsColdFusionInstalled   Returns the version of CF studio installed.
**      GetColdFusionPath       Returns the path of the extension directory.
**
** History:
**      12-Feb-2000 (fanra01)
**          Created.
*/
# include <compat.h>

# include "cfinst.h"

int
GetRegistryValue (HKEY hTree, PTCHAR pwszRegKey, PTCHAR pwszRegEntry,
    PTCHAR * pwszValue, LPDWORD lpType);

/*
** cfstring table contains the executable string and a version string for
** each ColdFusion release.  To add a new coldfusion version add a define
** in the _cfvers enum and add the reg key and version string to the table.
*/
TCHAR* cfstring[MAX_CFVERS][2] =
{
    NULL, NULL,
    "Studio45", "4.5.000",
    "Studio4",  "4.00.000"
};

/*
**  Registry keys
*/
TCHAR tcIngresSystemEnv[] =
{
    TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment")
};

TCHAR tcColdFusionPath[] =
{
    TEXT("SOFTWARE\\Allaire\\%s\\FileLocations")
};

TCHAR tcColdFusionInstalled[] =
{
    TEXT("SOFTWARE\\Allaire\\ColdFusion Studio\\")
};

TCHAR tcColdFusionVTMPath[] = TEXT("%s\\ingres\\ice\\ColdFusion");

TCHAR tcIngresSystemPath[_MAX_PATH] = {0};
TCHAR tcIngresICECFPath[_MAX_PATH] = {0};
TCHAR tcCurrentUser[_MAX_PATH] = {0};
TCHAR tcColdFusionPathExt[_MAX_PATH] = {0};

static TCHAR  szUserName[1024] = {0};

/*
** Name: GetRegistryValue
**
** Description:
**      Returns the value of a specific entry held in a key.
**
** Inputs:
**      hTree           handle of one of the existing registry trees.
**                      HKEY_CLASSES_ROOT
**                      HKEY_CURRENT_USER
**                      HKEY_LOCAL_MACHINE
**                      HKEY_USERS
**                      HKEY_PERFORMANCE_DATA
**      pszRegKey       pointer to char string containing full registry key.
**
** Outputs:
**      pwszValue       pointer to char string containing entry value.
**      lpType          pointer to variable to receive value type.
**
** Returns:
**      status  OK      succeeded
**              !0      error
**
** History:
**      12-Feb-2000 (mcgem01)
**          Created.
**
*/
int
GetRegistryValue( HKEY hTree, PTCHAR pwszRegKey, PTCHAR pwszRegEntry,
    PTCHAR * pwszValue, LPDWORD lpType )
{
    int         status = 0;
    HKEY        hSubkey = 0;
    TCHAR       tcClass[MAX_PATH];
    DWORD       dwClassLen = MAX_PATH;
    DWORD       dwNumSubkeys;
    DWORD       dwMaxSubkeys;
    DWORD       dwMaxClass;
    DWORD       dwNumValues;
    DWORD       dwMaxValueName;
    DWORD       dwMaxValueData;
    DWORD       dwSecDesc;
    DWORD       dwType;
    DWORD       i;
    FILETIME    filetime;
    LPTSTR      lpName = NULL;
    BYTE*       pData  = NULL;
    BOOL        blFound = FALSE;

    if (RegOpenKeyEx( hTree,
        pwszRegKey, 0, KEY_QUERY_VALUE, &hSubkey ) != ERROR_SUCCESS)
    {
        status = GetLastError();
        PRINTF( "Error: %d - Unable to open registry key %s.\n", status,
            pwszRegKey );
    }
    else
    {
        /*
        ** Get some information about the largest entry names and values.
        */
        if (RegQueryInfoKey( hSubkey, tcClass, &dwClassLen, NULL,
            &dwNumSubkeys, &dwMaxSubkeys, &dwMaxClass, &dwNumValues,
            &dwMaxValueName, &dwMaxValueData, &dwSecDesc, &filetime )
            != ERROR_SUCCESS)
        {
            status = GetLastError();
            PRINTF( "Error: %d - Unable to query registry key.\n", status );
        }
        else
        {
            /*
            **  dwMaxValueName is number of characters.  May need to convert
            **  this into number of bytes in the future.
            */
            dwMaxValueName += 1;        /* include null terminator character */

            HALLOC( lpName, char, dwMaxValueName, NULL );
            if (lpName != NULL)
            {
                HALLOC( pData, BYTE, dwMaxValueData, NULL );
                if (pData != NULL)
                {
                    for (i = 0; (i < dwNumValues) && (blFound == FALSE); i++)
                    {
                        DWORD dwNameLen = dwMaxValueName;
                        DWORD dwDataLen = dwMaxValueData;

                        /*
                        ** Enumerate each entry and compare it with the
                        ** requested item.
                        */
                        if (RegEnumValue( hSubkey, i, lpName, &dwNameLen, NULL,
                            &dwType, pData, &dwDataLen ) != ERROR_SUCCESS)
                        {
                            status = GetLastError();
                            break;
                        }
                        else
                        {
                            if (STRCMP( pwszRegEntry, lpName ) == 0)
                            {
                                blFound = TRUE;
                                *pwszValue = pData;
                                *lpType = dwType;
                            }
                        }
                    }
                }
            }

            if ((lpName == NULL) || (pData == NULL))
            {
                status = GetLastError();
                PRINTF( "Error: %d - Unable to allocate memory.\n", status );
            }
        }
        if (hSubkey != 0)
        {
            CloseHandle( hSubkey );
        }
        if (lpName != NULL)
        {
            HFREE( lpName );
        }
        if ((blFound == FALSE) && (pData != NULL))
        {
            HFREE( pData );
            status = 1;
        }
    }
    return (status);
}

/*
** Name:    TestForKey
**
** Description:
**      Function attempts to open the specified key.
**
** Inputs:
**      hTree           handle of one of the existing registry trees.
**                      HKEY_CLASSES_ROOT
**                      HKEY_CURRENT_USER
**                      HKEY_LOCAL_MACHINE
**                      HKEY_USERS
**                      HKEY_PERFORMANCE_DATA
**      pszRegKey       pointer to char string containing full registry key.
**
** Outputs:
**      None.
**
** Returns:
**      TRUE            key exists
**      FALSE           key does not exist or error.
**
** History:
**      12-Feb-2000 (fanra01)
**          Created.
*/
BOOL
TestForKey( HKEY hTree, PTCHAR regkey )
{
    HKEY    hSubkey = 0;
    int     status = 0;

    if (RegOpenKeyEx( hTree, regkey, 0,
        KEY_QUERY_VALUE, &hSubkey ) != ERROR_SUCCESS)
    {
        status = GetLastError();
        return (FALSE);
    }
    if (hSubkey)
    {
        CloseHandle( hSubkey );
    }
    return (TRUE);
}

/*
** Name:    GetIngresSystemPath
**
** Description:
**      Retrieves the value of II_SYSTEM
**
** Inputs:
**      None.
**
** Outputs:
**      tcIngresSystemPath  updated with II_SYSTEM.
**
** Returns:
**      pointer tcIngresSystemPath
**      NULL    if not found.
**
** History:
**      12-Feb-2000 (fanra01)
**          Created.
**
*/
PTCHAR
GetIngresSystemPath()
{
    DWORD type = 0;
    PTCHAR value = NULL;

    if (STRLENGTH( tcIngresSystemPath ) == 0)
    {
        if ((GetRegistryValue( HKEY_LOCAL_MACHINE, tcIngresSystemEnv,
            TEXT("II_SYSTEM"), &value, &type )) != 0)
        {
            return (NULL);
        }
        else
        {
            STRCOPY( value, tcIngresSystemPath );
            HFREE( value );
        }
    }
    return (tcIngresSystemPath);
}

/*
** Name:    GetIngresICECFPath
**
** Description:
**      Retrieves the value of ColdFusion directory under II_SYSTEM
**
** Inputs:
**      None.
**
** Outputs:
**      tcIngresICECFPath  updated with II_SYSTEM.
**
** Returns:
**      tcIngresICECFPath   pointer to path string
**      NULL                if not found
**
** History:
**      12-Feb-2000 (fanra01)
**          Created.
**
*/
PTCHAR
GetIngresICECFPath()
{
    DWORD type = 0;
    PTCHAR value = NULL;

    if (STRLENGTH( tcIngresICECFPath ) == 0)
    {
        if (STRLENGTH( tcIngresSystemPath ) == 0)
        {
            if ((GetRegistryValue( HKEY_LOCAL_MACHINE, tcIngresSystemEnv,
                TEXT("II_SYSTEM"), &value, &type)) != 0)
            {
                return (NULL);
            }
            else
            {
                STRCOPY( value, tcIngresSystemPath );
                HFREE( value );
            }
        }
        SPRINTF( tcIngresICECFPath, "%s\\ingres\\ice\\ColdFusion",
                 tcIngresSystemPath );
    }
    return (tcIngresICECFPath);
}

/*
** Name: SetIngreICEPath
**
** Description:
**      Returns the path for the Ingres ICE extensions
**
** Inputs:
**      ipath               path to the Ingres installation
**
** Outputs:
**      tcIngresICECFPath   updated with the extension path.
**
** Returns:
**      PTCHAR              pointer to tcIngresICECFPath
**
** History:
**      12-Feb-2000 (fanra01)
**          Created.
**
*/
PTCHAR
SetIngresICEPath( char* ipath )
{
    PTCHAR  retval = tcIngresICECFPath;

    if((ipath != NULL) && (STRLENGTH( ipath ) != 0))
    {
        SPRINTF( tcIngresICECFPath, "%s\\ingres\\ice\\ColdFusion", ipath );
    }
    else
    {
        retval = NULL;
    }
    return (retval);
}

/*
** Name: IsColdFusionInstalled
**
** Description:
**      Determines the version of ColdFusion studio installed.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      Returns the enumerated version index.
**
** History:
**      12-Feb-2000 (fanra01)
**          Created
*/
int
IsColdFusionInstalled()
{
    int     retval = 0;
    int     i;
    TCHAR   cfkey[MAX_PATH];

    for (i = CF45; i < MAX_CFVERS; i+=1)
    {
        SPRINTF( cfkey, "%s%s", tcColdFusionInstalled, cfstring[i][1] );
        if (TestForKey( HKEY_LOCAL_MACHINE, cfkey  ) == TRUE)
        {
            retval = i;
            break;
        }
    }
    if (i == MAX_CFVERS)
    {
        PRINTF( "Error: Unable to locate a version of ColdFusion Studio.\n" );
    }
    return (retval);
}

/*
** Name: GetColdFusionPath
**
** Description:
**      Returns the path for the ColdFusion extensions
**
** Inputs:
**      ver                 enuerated ColdFusion version index.
**
** Outputs:
**      tcColdFusionPathExt updated with the extension path.
**
** Returns:
**      PTCHAR              pointer to tcColdFusionPathExt
**
** History:
**      12-Feb-2000 (fanra01)
**          Created.
**
*/
PTCHAR
GetColdFusionPath( CFVERS ver )
{
    PTCHAR  retval = NULL;
    TCHAR   cfkey[MAX_PATH];
    PTCHAR  value;
    DWORD   type;

    if(STRLENGTH( tcColdFusionPathExt ) == 0)
    {
        SPRINTF( cfkey, tcColdFusionPath, cfstring[ver][0] );
        if ((GetRegistryValue (HKEY_CURRENT_USER, cfkey, TEXT("ProgramDir"),
            &value, &type)) == 0)
        {
            SPRINTF( tcColdFusionPathExt, "%s\\Extensions",
                 value );
            HFREE( value );
            retval = tcColdFusionPathExt;
        }
    }
    else
    {
        retval = tcColdFusionPathExt;
    }
    return (retval);
}

/*
** Name: SetColdFusionPath
**
** Description:
**      Returns the path for the ColdFusion extensions
**
** Inputs:
**      cfpath      path to the coldfusion installation
**
** Outputs:
**      tcColdFusionPathExt     Updated with the extension path.
**
** Returns:
**      PTCHAR  pointer to tcColdFusionPathExt
**
** History:
**      12-Feb-2000 (fanra01)
**          Created.
**
*/
PTCHAR
SetColdFusionPath( char* cfpath )
{
    PTCHAR  retval = tcColdFusionPathExt;

    if((cfpath != NULL) && (STRLENGTH( cfpath ) != 0))
    {
        SPRINTF( tcColdFusionPathExt, "%s\\Extensions", cfpath );
    }
    else
    {
        retval = NULL;
    }
    return (retval);
}
