/*
**	Copyright (c) 2004 Ingres Corporation
*/
#ifndef _ICM_H
#define _ICM_H

# include <ill.h>

/*
** Name:    icm.h                   - Definitions for connection manager.
**
** Description:
**  Contains definitions used in the ICE Connection Manager.
**
** History:
**      29-oct-96 (harpa06)
**          Created.
**      28-jul-97 (harpa06)
**          Removed "Token" from DBH and CMUSER structures. It isn't used.
**      15-jan-1998 (harpa06)
**          Added support for obtaining DBMS error messages. s88270
**      06-May-98 (fanra01)
**          Use queue structures for linking.
**          Add queue offset to comparison prototypes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
/*
** Database handle status structure:
**  DBH_IN_USE      = Handle is in use by a transaction
**  DBH_AVAILABLE   = Handle is free for transaction use
**  DBH_UNAVAILABLE = Handle is permanently unavailable. About to be
**                    destroyed.
*/
typedef enum _DBHSTATUS
{
    DBH_IN_USE, DBH_AVAILABLE, DBH_UNAVAILABLE
} DBHSTATUS;

/*
** Database connection structure - Used to keep track information about a
** database being used by an ICE client.
*/
typedef struct _DBC
{
    ILL         DBCQueue;   /* All connected databases */
    ILL         UsrQueue;   /* Connected databases grouped by user */
    char        dbname [DB_MAXNAME + 1];
                            /* Name of the opened database */
    ILLHDR      DBHList;    /* List of connection handles to the database */
} DBC;

/*
** Connection handle structure - Used to keep track of handles to a single
** database.
*/
typedef struct _DBH
{
    ILL         DBHQueue;   /* All connection handles */
    ILL         DBQueue;    /* Connection handles grouped by database */
    char        *Userid;    /* client user id */
    char        *Password;  /* Client password */
    II_PTR      *handle;    /* Connection handle */
    DBHSTATUS   status;     /* Connection status */
    i4     last_used;  /* last activity time on this connection */
    DBC*        pDBC;
} DBH;

/* CMUSER structure - Used to hold list of databases opened by each user. */
typedef struct _CMUSER
{
    ILL             CMUQueue;       /* All users */
    char            *Userid;        /* Name of user */
    char            *Password;      /* Password of user */
    ILLHDR          DBCList;        /* List of opened databases */
} CMUSER;


/* Prototypes */
STATUS DropConnection (char *dbName, II_PTR hConn, char **message);

int CmpDBName (void* item, i4  nQueue, void* key);
int CmpICEUSER (void* item, i4  nQueue, void* key);
int CmpCMUser (void* item, i4  nQueue, void* key);
int CmpConnection (void* item, i4  nQueue, void* key);

#endif
