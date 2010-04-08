/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ivp.h
**
** Description:
**      Declarations and defines for the ICE HTML variable processor
**
** History:
**      02-nov-1996 (wilan06)
**          Created
**      21-feb-1997 (harpa06)
**          Modified call to ICEExecuteApp()
*/

# ifndef IVP_H_INCLUDED
# define IVP_H_INCLUDED

# include <idb.h>

# define TOKEN_QUERY_PARM       ERx(":")

/* bit fields used to select the user or system (or both) HTML var tables */
# define ICE_SYS_VAR            1
# define ICE_USR_VAR            2

# define SYS_HASH_TABLE_SIZE    64
# define USR_HASH_TABLE_SIZE    64

/* icevxqry.c */
ICERES ICEExecuteQuery (PICECLIENT pic, PICEREQUEST pir, char* pszQuery,
                        PICEHTMLFORMATTER pihf);
ICERES ICEGetDBConnection (PICECLIENT pic, PICEREQUEST pir, char** ppszDb, HIDBCONN* phconn);

/* ivpxproc.c */
ICERES ICEExecuteProcedure (PICECLIENT pic, PICEREQUEST pir, char* pszProc);

/* icevproc.c */
ICERES ICEProcessHTMLVariables (PICECLIENT pic);
ICERES ICEParseHTMLVariables (PICECLIENT pic, PICEREQUEST* ppir);
ICERES ICEInitHTMLVarStore (PICEREQUEST pir);
ICERES ICEQueryHTMLVar (PICEREQUEST pir, char* var, char** value, int which);
ICERES ICEStoreHTMLVar (PICEREQUEST pir, char* var, char* value);
ICERES ICEEnumHTMLVars (PICEREQUEST pir, char** var, char** value,
                        II_BOOL fFirst, II_UINT4 which);
ICERES ICEReleaseRequest (PICEREQUEST pir);
ICERES ICEGetDBConnection (PICECLIENT pic, PICEREQUEST pir,
                           char** ppszDb, HIDBCONN* phconn);

/* */
ICERES ICEExecuteProcedure (PICECLIENT pic, PICEREQUEST pir, char* value);
ICERES ICEExecuteReport (PICECLIENT pic, PICEREQUEST pir, char* repName,
                         char* repLoc);
ICERES ICEExecuteApp (PICECLIENT pic, PICEREQUEST pir, char* pszApp);

# endif /* IVP_H_INCLUDED */
