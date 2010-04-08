/*
** Copyright (c) 2004 Ingres Corporation
*/
#ifndef DBLIB_H
#define DBLIB_H
#include <iiapi.h>
/*
 *
 * Helper macros
 *
 */
#define DBSTATUS_ERROR    1
#define DBSTATUS_NODATA   2
#define DBSTATUS_WARNING  3
#define DBSTATUS_MESSAGE  4
#define DBSTATUS_TIMEDOUT 5
#define DBSTATUS_SUCCESS  0

#define MAX_ERRMESS      255
#define MAX_CHANNELS     24

#define VARCHAR2STR(s, v) {short len; char *vc;\
 vc = v; \
 memcpy(&len, vc, 2); \
 len = len >= sizeof(s) ? sizeof(s)-1 : len; \
 memcpy(s, &vc[2], len); \
 s[len] = '\0'; }

/*
 *
 * Database library public structures & typedefs
 *
 */
typedef int Boolean;

enum dataTypes 
{
   string      = IIAPI_CHA_TYPE,
   integer     = IIAPI_INT_TYPE,
   longInteger = IIAPI_INT_TYPE,
   byte        = IIAPI_BYTE_TYPE
};

typedef struct
{
      char messageText[MAX_ERRMESS];
      long type;
      long code;
      long numrows;
} dbStatus;

typedef struct 
{
      dbStatus status;          /* TODO: Should this be returned from a seperate call? */
      void* appData;
      void* reserved;
} dbChannel;

typedef struct {
      Boolean null;
      long    length;
      void*   value;
} dbResults;

typedef struct 
{
      long id1;
      long id2;
      char* idStr;
      II_PTR handle;
} dbRepeatHandle;

typedef void (*dbResultCallback)(dbChannel* chan, void* appData);
typedef void (*dbFinishedCallback)(dbChannel* chan, void* appData);



/*
 *
 * Database library function prototypes
 *
 */
Boolean dbInitialize(char* database, char* user, char* password, int maxChans);
void dbTerminate(void);

dbChannel* dbGetChannel(time_t timeout);
void dbReleaseChannel(dbChannel** chan);
void dbResetChannel(char* database, char* user, char* password);

Boolean dbQuery(dbChannel* chan, char* sql, 
                dbResultCallback resFn, dbFinishedCallback finishedFn, void* appData);
Boolean dbRepeatQuery(dbChannel* chan, dbRepeatHandle *repHndl, char* sql, 
                      dbResultCallback resFn, dbFinishedCallback finishedFn, void* appData);

void dbGetNext(dbChannel* chan, void *dummy);
void dbEndQuery(dbChannel* chan);

void dbCommit(dbChannel* chan, dbFinishedCallback finishedFn, void *appdata);
void dbRollback(dbChannel* chan, dbFinishedCallback finishedFn, void *appdata);

void dbWaitForAsyncOpps(Boolean block);

Boolean dbErrorEncountered(dbChannel* chan);

void dbResetTimeout(dbChannel* chan, int timeout);
void dbResetParams(dbChannel* chan);
Boolean dbSetParam(dbChannel* chan, int paramNumber, int type, int maxLength, void *value);
Boolean dbSetDV(dbChannel* chan, int paramNumber, void* value);
void dbResetDV(dbChannel* chan);

/* End of dbLibrary header */
#endif
