/*
** Copyright (C) 1996, 1998 Ingres Corporation
*/

/*
**  Name:   cfconf.h
**
**  Description:
**      Definitions for external interface
**
**  History:
**      11-Feb-2000 (fanra01)
**          Created.
**
*/
# ifndef CFCONF_H_INCLUDED
# define CFCONF_H_INCLUDED

typedef struct _readline RL;
typedef struct _readline *PRL;

struct _readline
{
    PRL     next;
    PRL     prev;
    char*   text;
};

FUNC_EXTERN STATUS
CFBuildList( char* section[], PRL* begin );

FUNC_EXTERN STATUS
CFCreateConf( char* path, char* filename, LOCATION* loc, PRL begin );

FUNC_EXTERN STATUS
CFReadConf( char* path, char* filename, LOCATION* loc, PRL* begin );

FUNC_EXTERN STATUS
CFWriteConf( LOCATION* loc, PRL begin );

FUNC_EXTERN STATUS
CFFreeConf( PRL begin );

FUNC_EXTERN STATUS
CFBackupConf( LOCATION* loc, PRL begin );

FUNC_EXTERN PRL
CFScanConf( char* search, PRL begin, bool termblank );

# endif
