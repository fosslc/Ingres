/*
** Copyright (c) 1994, 1997 Ingres Corporation
*/
# ifndef	REPNTSVC_H_INCLUDED
# define	REPNTSVC_H_INCLUDED

/**
** Name:	repntsvc.h - Replicator NT Service include
**
** Description:
**	Defines for NT Services.
**
** History:
**	13-jan-97 (joea)
**		Created based on repntsvc.h in replicator library.
**      12-Jun-97 (fanra01)
**              Change fromt CA-Ingres to OpenIngres.
**      25-Jun-97 (fanra01)
**              Add definitions for replicator service name and process name.
**              Add definition of enivronment name.
**	22-mar-98 (mcgem01)
**		Replace replicator executable name and location.
**	20-apr-98 (mcgem01)
**		Product name change to Ingres.
**	28-oct-98 (abbjo03)
**		Add TEXT macro to COMPONENT_REP_PROCESS. Remove
**		COMPONENT_ENV_VAR.
**/

# define	SVC_NAME	TEXT("Ingres_Replicator")
# define	DISPLAY_NAME	TEXT("Ingres Replicator")

# define SYSTEM_COMPONENT_NAME  TEXT("Replicator")
# define COMPONENT_REP_PROCESS  TEXT("repserv.exe")

# define	MAX_SERVERS		999
# define	DEFAULT_NUM_SERVERS	10

/*
**  Name:   OPTVALS
**
**  Description:
**      Structure holds some of the parameters parsed from the runrepl.opt
**      file.
**
**      szNode      Target node
**      szDBname    Database name
*/

typedef struct tagOPTVALS
{
    char    szNode[DB_MAXNAME+1];
    char    szDBname[DB_MAXNAME+1];
}OPTVALS, *POPTVALS;

# endif
