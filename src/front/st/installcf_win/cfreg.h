/*
** Copyright (c) 1996, 1998 Ingres Corporation
*/

/*
**  Name:   cfreg.h
**
**  Description:
**      Definitions for external interface
**
**  History:
**      13-Feb-2000 (fanra01)
**          Created.
**
*/
# ifndef CFREG_H_INCLUDED
# define CFREG_H_INCLUDED

/*
** Enumerate the ColdFusion versions.  This is used for the cfstring table.
*/
typedef enum _cfvers
{
    CFNULL, CF45, CF4, MAX_CFVERS
}CFVERS;

FUNC_EXTERN int
GetRegistryValue( HKEY hTree, PTCHAR pwszRegKey, PTCHAR pwszRegEntry,
                  PTCHAR * pwszValue, LPDWORD lpType );

FUNC_EXTERN BOOL
TestForKey( HKEY hTree, PTCHAR regkey );

FUNC_EXTERN PTCHAR
GetIngresSystemPath( void );

FUNC_EXTERN PTCHAR
GetIngresICECFPath( void );

FUNC_EXTERN PTCHAR
SetIngresICEPath( char* ipath );

FUNC_EXTERN int
IsColdFusionInstalled( void );

FUNC_EXTERN PTCHAR
GetColdFusionPath( CFVERS ver );

FUNC_EXTERN PTCHAR
SetColdFusionPath( char* cfpath );

# endif
