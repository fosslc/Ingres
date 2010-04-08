/*
** Copyright (c) 2004 Ingres Corporation
*/
#include <iiapi.h>
#include <time.h>		/* for difftime */
#ifndef  NT_GENERIC
#include <sys/time.h>
#endif
#include <string.h>
#include <stdlib.h>
#include "dblib.h"

/*
 *
 * The default maximun number of channels - 
 * The OpenIngres OpenAPI will not allow more than approx 28 open connections..
 *
 */
static maxChannels = 20;

#define CONNECT_TIMEOUT    -1      /* Don't timeout on connecting to the database */
#define WAIT_TIME        1000      /* Time waited between checking for timeouts   */

/* 
 *
 * Channel statuses - used for the value of dbInternalChannelInfoType.status
 *
 */
#define BROKEN                         -2 /* Not connected to database */
#define TIMEDOUT                       -1 /* Query has been timedout   */
#define INUSE                           0 /* Connected but in use         */
#define FREE                            1 /* Connected and available   */

#define MAX_PARMS 30
#define MAX_RESULTS 30

typedef II_VOID	(II_FAR II_CALLBACK *iiapi_callback)( II_PTR closure, II_PTR parmBlock );

/*
 *
 * Internal structures
 *
 */
typedef struct
{
      char *sql;
      dbRepeatHandle     *repHndl;
      IIAPI_QUERYPARM    qy;
      IIAPI_SETDESCRPARM sd;
      IIAPI_PUTPARMPARM  pp;
      IIAPI_GETDESCRPARM gd;
      IIAPI_GETCOLPARM   gc;
      IIAPI_GETQINFOPARM gq;
      IIAPI_CLOSEPARM    cl;
      IIAPI_GETEINFOPARM ge;
      IIAPI_COMMITPARM   cm;
      IIAPI_ROLLBACKPARM rb;
      IIAPI_CANCELPARM   cn;
      IIAPI_GENPARM*     gp;
      IIAPI_DESCRIPTOR   userDescriptors[MAX_PARMS]; /* Generated */
      IIAPI_DATAVALUE    userParameters[MAX_PARMS]; /* Generated */
      IIAPI_DESCRIPTOR   descriptors[MAX_PARMS+3]; /* Generated - +3 for def repeat queries */
      IIAPI_DATAVALUE    parameters[MAX_PARMS+3]; /* Generated - +3 for def repeat queries */
      IIAPI_DATAVALUE    results[MAX_RESULTS];
} dbQueryInfo;

typedef struct 
{
      int               status;
      IIAPI_CONNPARM    co;
      IIAPI_DISCONNPARM dc;
      II_PTR            transaction;
      dbQueryInfo       query;
      dbResultCallback   resCallback;
      dbFinishedCallback finishedCallback;
      time_t            timeoutTime;
} dbInternalChannelInfoType;

typedef struct {
      char database[255];
      char username[255];
      char password[255];
      int maxChannels;
      int numUsed;
      int numBroken;
      dbFinishedCallback nextFn;
      dbChannel* finishedChannel;
      dbChannel* pool[1];
} dbChannelPoolType;

/*
 *
 * Channel Pool variables..
 *
 */
static dbChannelPoolType* dbChannelPool = NULL;
static dbInternalChannelInfoType* dbChannelInfo = NULL;
static dbChannel* dbChannels = NULL;

/*
 *
 * Private function prototypes.
 *
 */
static Boolean initDBLib(void);
static Boolean allocChannelPool(char* database, char* username, char* password);
static void freeChannelPool(void);
static void dbConnect(dbChannel* chan);
static void dbCloseChannels(void);
static void dbClose(dbChannel* chan, void *unused); /* Also used as a callback */



/*
 *
 * Private callback prototypes
 *
 */
static void dbDisconnectCallback(dbChannel* chan, IIAPI_DISCONNPARM* dc);
static void dbConnectFinished(dbChannel* chan, IIAPI_CONNPARM *co);
static void dbFinished(dbChannel* chan, void*unused);
static void dbSetDescriptors(dbChannel* chan, IIAPI_QUERYPARM *qy);
static void dbPutParms(dbChannel* chan, IIAPI_SETDESCRPARM *sd);
static void dbGetDescriptors(dbChannel* chan, IIAPI_PUTPARMPARM *pp);
static Boolean dbSetErrorEncountered(dbChannel* chan);
static void dbGotData(dbChannel* chan, void* unused);
static void dbTransEnded(dbChannel* chan, void* unused);
static void dbRetryRepeatQuery(dbChannel* chan, void *unused);
static void dbDefRepeatQuery(dbChannel* chan, void* unused);

Boolean
dbInitialize(char* database, char* user, char* password, int maxChans)
{
   /* 
    *
    * Initialize the underling database libraries, allocate the memory for the channel pool
    * and then connect to the database specified.
    *
    * Returns TRUE if the memory allocation and database library initialization was successfull,
    * even if none of the connections succeeded, or the database name and or user details were 
    * incorrect.
    *
    */
   maxChannels = maxChans;
   if (initDBLib() == FALSE || allocChannelPool(database, user, password) == FALSE)
   {
      freeChannelPool();
      return FALSE;
   }

   return TRUE;
}

void 
dbTerminate(void)
{
   IIAPI_TERMPARM tm;
   
   dbCloseChannels();
   IIapi_terminate(&tm);
}

dbChannel* 
dbGetChannel(time_t timeout)
{
   dbChannel* freeChannel = NULL;
   dbInternalChannelInfoType* intChan;
   int i;
   time_t now, startTime;
   long oldused, oldbroken;
   
   time(&startTime);
   if (dbChannelPool->numBroken == dbChannelPool->maxChannels)
      return NULL;                  /* All channels are broken */

#ifdef TRACE   
   printf("ChannelPool:    max %d   used %d   broken %d\n",
          dbChannelPool->maxChannels,
          dbChannelPool->numUsed,
          dbChannelPool->numBroken);
   oldused = dbChannelPool->numUsed;
   oldbroken = dbChannelPool->numBroken;
#endif

   while (freeChannel == NULL)
   {
      time(&now);
      if (timeout > 0 && difftime(startTime+timeout, now) <= 0)
         break;                     /* Timedout waiting for a connection to be released */
      
      if (dbChannelPool->numUsed == dbChannelPool->maxChannels - dbChannelPool->numBroken &&
          dbChannelPool->numUsed > 0)
      {
         dbWaitForAsyncOpps(TRUE);  /* Wait for channel to become free */
#ifdef TRACE
         if (dbChannelPool->numUsed != oldused || dbChannelPool->numBroken != oldbroken)
         {
            printf("ChannelPool:    max %d   used %d   broken %d\n",
                   dbChannelPool->maxChannels,
                   dbChannelPool->numUsed,
                   dbChannelPool->numBroken);
            oldused = dbChannelPool->numUsed;
            oldbroken = dbChannelPool->numBroken;
         }
#endif
      }
      
      for(i=0; i<dbChannelPool->maxChannels; i++)
      {
         intChan = (dbInternalChannelInfoType*)dbChannelPool->pool[i]->reserved;
         
         if (intChan->status == FREE)
         {
            time_t now;
            
            intChan->status = INUSE;
            dbChannelPool->numUsed++;
#ifdef TRACE
            printf("dbGetChannel - 1 allocated --> num used now = %d\n", dbChannelPool->numUsed);
#endif
            freeChannel = dbChannelPool->pool[i];
            dbResetTimeout(freeChannel, timeout);
            break;
         }
      }
   }

   return freeChannel;
}

void 
dbReleaseChannel(dbChannel** chan)
{
   dbInternalChannelInfoType* intChan;
   
   intChan = (dbInternalChannelInfoType*) (*chan)->reserved;
   intChan->status = FREE;
   *chan = NULL;
   dbChannelPool->numUsed--;
#ifdef TRACE
   printf("dbReleaseChannel: 1 released --> num used now = %d\n", dbChannelPool->numUsed);
#endif
}

void 
dbResetChannels(char* database, char* user, char* password)
{
   /* TODO: Implement this function -- should close and reopen all connections using the */
   /*            username and password specified.                                                                   */
}

void
dbResetTimeout(dbChannel* chan, int timeout)
{
   dbInternalChannelInfoType* intChan;
   time_t now;

   intChan = (dbInternalChannelInfoType*) chan->reserved;
   if (timeout > 0)
   {
      time(&now);
      intChan->timeoutTime = now + timeout;
   }
   else
      intChan->timeoutTime = 0;
}

void
dbResetParams(dbChannel* chan)
{
   dbInternalChannelInfoType* intChan;

   intChan = (dbInternalChannelInfoType*) chan->reserved;
   if (intChan->query.qy.qy_stmtHandle != NULL) /* Must finish current query first */
      return;
   memset(intChan->query.userParameters, 0, sizeof(intChan->query.userParameters));
   memset(intChan->query.userDescriptors, 0, sizeof(intChan->query.userDescriptors));
}

Boolean
dbSetParam(dbChannel* chan, int paramNumber, int type, int maxLength, void *value)
{
   dbInternalChannelInfoType* intChan;
   IIAPI_DESCRIPTOR* desc;

   intChan = (dbInternalChannelInfoType*) chan->reserved;
   if (intChan->query.qy.qy_stmtHandle != NULL) /* Must finish current query first */
      return FALSE;
   
   if (paramNumber >= MAX_PARMS)
      return FALSE;
   
   desc = intChan->query.userDescriptors;
   
   desc[paramNumber].ds_dataType = type;
   desc[paramNumber].ds_nullable = FALSE;
   desc[paramNumber].ds_length   = maxLength;
   desc[paramNumber].ds_precision = 0;
   desc[paramNumber].ds_scale     = 0;
   desc[paramNumber].ds_columnType = IIAPI_COL_QPARM;
   desc[paramNumber].ds_columnName = NULL;

   intChan->query.userParameters[paramNumber].dv_value = value;
   intChan->query.userParameters[paramNumber].dv_null = FALSE;
   if (type == string)
   {
      intChan->query.userParameters[paramNumber].dv_length = strlen((char *)value);
   }
   else
   {
      intChan->query.userParameters[paramNumber].dv_length = maxLength;
   }
   
   return TRUE;
}

Boolean
dbSetDV(dbChannel* chan, int paramNumber, void* value)
{
   dbInternalChannelInfoType* intChan;

   intChan = (dbInternalChannelInfoType*) chan->reserved;
   
   if (paramNumber >= MAX_RESULTS)
      return FALSE;
   
   intChan->query.results[paramNumber].dv_value = value;

   return TRUE;
}
   
void
dbResetDV(dbChannel* chan)
{
   dbInternalChannelInfoType* intChan;

   intChan = (dbInternalChannelInfoType*) chan->reserved;
   if (intChan->query.qy.qy_stmtHandle != NULL) /* Must finish current query first */
      return;
   memset(intChan->query.results, 0, sizeof(intChan->query.results));
}

Boolean
dbQuery(dbChannel* chan, char* sql, 
        dbResultCallback resFn, dbFinishedCallback finishedFn, void* appData)
{
                                /* Set up information */
                                /* call IIapi_query */
   dbInternalChannelInfoType* intChan;
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;

   chan->appData = appData;
   if (intChan->query.qy.qy_stmtHandle != NULL) /* Must finish current query first */
   {
      (*finishedFn)(chan, appData);
      return FALSE;
   }

   intChan->query.sql = sql;
   intChan->query.repHndl = NULL;
   intChan->resCallback = resFn;
   intChan->finishedCallback = finishedFn;
   
   intChan->query.qy.qy_genParm.gp_callback = (iiapi_callback)dbSetDescriptors;
   intChan->query.qy.qy_genParm.gp_closure = chan;
   intChan->query.qy.qy_connHandle = intChan->co.co_connHandle;
   intChan->query.qy.qy_queryType = IIAPI_QT_QUERY;
   intChan->query.qy.qy_queryText = sql;
   intChan->query.qy.qy_parameters = intChan->query.userParameters[0].dv_value == NULL ?
      FALSE : TRUE;
   intChan->query.qy.qy_tranHandle = intChan->transaction;
   intChan->query.gp = &intChan->query.qy.qy_genParm;

   IIapi_query(&intChan->query.qy);
   intChan->transaction = intChan->query.qy.qy_tranHandle;
   return TRUE;
}

Boolean
dbRepeatQuery(dbChannel* chan, dbRepeatHandle *repHndl, char* sql, 
              dbResultCallback resFn, dbFinishedCallback finishedFn, void* appData)
{
      /* Repeat query */
                                /* Set up as per normal select */
                                /* Set descriptors adding repeat handle */
                                /* Put parameters */
                                /* Get result descriptors, or rows altered */
                                /* If  E_AP0014_INVALID_REPEAT_ID endSelect, define repeat handle and try again */
   dbInternalChannelInfoType* intChan;
   IIAPI_DESCRIPTOR* desc;

   intChan = (dbInternalChannelInfoType*) chan->reserved;
   desc = intChan->query.descriptors;

   desc[0].ds_dataType   = IIAPI_HNDL_TYPE;
   desc[0].ds_length     = 4;
   desc[0].ds_columnType = IIAPI_COL_SVCPARM;
   intChan->query.repHndl = repHndl;
   intChan->query.parameters[0].dv_value = &intChan->query.repHndl->handle;
   intChan->query.parameters[0].dv_length = 4;
   
   chan->appData = appData;

   intChan->query.sql = sql;
   intChan->resCallback = resFn;
   intChan->finishedCallback = finishedFn;
   
   intChan->query.qy.qy_genParm.gp_callback = (iiapi_callback)dbSetDescriptors;
   intChan->query.qy.qy_genParm.gp_closure = chan;
   intChan->query.qy.qy_connHandle = intChan->co.co_connHandle;
   intChan->query.qy.qy_queryType = IIAPI_QT_EXEC_REPEAT_QUERY;
   intChan->query.qy.qy_queryText = sql;
   intChan->query.qy.qy_parameters = TRUE;
   intChan->query.qy.qy_tranHandle = intChan->transaction;
   intChan->query.gp = &intChan->query.qy.qy_genParm;

   if (repHndl->handle == NULL)
   {
      if (intChan->query.qy.qy_stmtHandle != NULL) /* Must finish current query first */
      {
         (*finishedFn)(chan, appData);
         return FALSE;
      }
      dbDefRepeatQuery(chan, NULL);
      return TRUE;
   }
   else
      IIapi_query(&intChan->query.qy);
   intChan->transaction = intChan->query.qy.qy_tranHandle;
   return TRUE;
}

void
dbGetNext(dbChannel* chan, void *dummy)
{
   dbInternalChannelInfoType* intChan;
   int numColumns;
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;

   /* Count the number of parameters supplied */
   for(numColumns=0; 
       numColumns < MAX_RESULTS, intChan->query.results[numColumns].dv_value != NULL; 
       numColumns++);
   
   if (dbSetErrorEncountered(chan) ||
       numColumns == 0 || 
       /* Must have the same number of columns as expected */
       intChan->query.gd.gd_descriptorCount != numColumns) 
   {
      dbEndQuery(chan);         /* Can't get the next row if there are errors... */
      return;
   }

   intChan->query.gc.gc_genParm.gp_callback = (iiapi_callback) dbGotData;
   intChan->query.gc.gc_genParm.gp_closure = chan;
   intChan->query.gc.gc_stmtHandle = intChan->query.qy.qy_stmtHandle;
   intChan->query.gc.gc_rowCount = 1;
   intChan->query.gc.gc_columnCount = numColumns;
   intChan->query.gc.gc_columnData = intChan->query.results;
   intChan->query.gp = &intChan->query.gc.gc_genParm;
   IIapi_getColumns(&intChan->query.gc);
}

void
dbEndQuery(dbChannel* chan)
{
   dbInternalChannelInfoType* intChan;
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;
   intChan->query.gq.gq_genParm.gp_callback = (iiapi_callback) dbClose;
   intChan->query.gq.gq_genParm.gp_closure = chan;
   intChan->query.gq.gq_stmtHandle = intChan->query.qy.qy_stmtHandle;
   intChan->query.gp = &intChan->query.gq.gq_genParm;
   IIapi_getQueryInfo(&intChan->query.gq);
}

void 
dbCommit(dbChannel* chan, dbFinishedCallback finishedFn, void *appdata)
{
   dbInternalChannelInfoType* intChan;
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;

   chan->appData = appdata;
   if (intChan->query.qy.qy_stmtHandle != NULL) /* Must finish current query first */
   {
      (*finishedFn)(chan, chan->appData);
      return;
   }
   if (intChan->transaction == NULL) /* No transaction to commit */
   {
      (*finishedFn)(chan, chan->appData);
      return;
   }

   intChan->query.cm.cm_genParm.gp_callback = (iiapi_callback) dbTransEnded;
   intChan->query.cm.cm_genParm.gp_closure = chan;
   intChan->query.cm.cm_tranHandle = intChan->transaction;
   intChan->query.gp = &intChan->query.cm.cm_genParm;
   intChan->finishedCallback = finishedFn;
   
   IIapi_commit(&intChan->query.cm);
}

void 
dbRollback(dbChannel* chan, dbFinishedCallback finishedFn, void *appdata)
{
   dbInternalChannelInfoType* intChan;
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;

   chan->appData = appdata;
   if (intChan->query.qy.qy_stmtHandle != NULL) /* Must finish current query first */
   {
      (*finishedFn)(chan, chan->appData);
      return;
   }
   if (intChan->transaction == NULL) /* No transaction to commit */
   {
      (*finishedFn)(chan, chan->appData);
      return;
   }

   intChan->query.rb.rb_genParm.gp_callback = (iiapi_callback) dbTransEnded;
   intChan->query.rb.rb_genParm.gp_closure = chan;
   intChan->query.rb.rb_tranHandle = intChan->transaction;
   intChan->query.rb.rb_savePointHandle = NULL;
   intChan->query.gp = &intChan->query.rb.rb_genParm;
   intChan->finishedCallback = finishedFn;

   IIapi_rollback(&intChan->query.rb);
}

void
dbWaitForAsyncOpps(Boolean block)
{
   IIAPI_WAITPARM waitparm;
   dbFinishedCallback nextFn;
   dbChannel* finishedChannel;
   dbInternalChannelInfoType* intChan;
   int i;
   time_t now;
   
   /* Check that the lib is initialised */
   if (dbChannelPool == NULL)
      return;
   
   /* Wait for next opperation to complete */
   waitparm.wt_timeout = (block == TRUE ? -1 : WAIT_TIME);
   IIapi_wait(&waitparm);
   
   /* Process any completed request */
   nextFn = dbChannelPool->nextFn;
   finishedChannel = dbChannelPool->finishedChannel;
   if (nextFn != NULL && finishedChannel != NULL)
   {
      dbChannelPool->nextFn = NULL;
      dbChannelPool->finishedChannel = NULL;
      (*nextFn)(finishedChannel, finishedChannel->appData);
   }
   
   /* Check for timeouts */
   time(&now);
   for(i=0; i<dbChannelPool->maxChannels; i++)
   {
      intChan = (dbInternalChannelInfoType*) dbChannelPool->pool[i]->reserved;
      if (intChan->status == INUSE && 
          intChan->timeoutTime > 0 &&
          difftime(now, intChan->timeoutTime) > 0)
      {
         intChan->query.cn.cn_genParm.gp_callback = (iiapi_callback) NULL;
         intChan->query.cn.cn_genParm.gp_closure  = &dbChannelPool->pool[i];
         intChan->query.cn.cn_stmtHandle = intChan->query.qy.qy_stmtHandle;
         intChan->status = TIMEDOUT; /* Don't want to timeout subsequent calls... */
         IIapi_cancel(&intChan->query.cn);
#ifdef TRACE
         printf("Timeout on channel\n");
#endif
      }
   }
}

Boolean
dbErrorEncountered(dbChannel* chan)
{
   if (chan == NULL)
      return TRUE;              /* Invalid info -> assume an error */

   if (chan->status.type == DBSTATUS_SUCCESS || chan->status.type == DBSTATUS_NODATA)
      return FALSE;             /* Not an error */
   else
      return TRUE;              /* Error */
}

/*
 *
 *
 * Internal functions....
 *
 *
 */
static Boolean
initDBLib(void)
{
   IIAPI_INITPARM initparm;

   initparm.in_timeout = -1;
   initparm.in_version = IIAPI_VERSION_1;
   IIapi_initialize(&initparm);
   if (initparm.in_status != IIAPI_ST_SUCCESS)
      return FALSE;
   else
      return TRUE;   
}

static Boolean
allocChannelPool(char* database, char* username, char* password)
{
   int i;

   dbChannelPool = (dbChannelPoolType *)
      calloc((maxChannels - 1), sizeof(dbChannelPoolType) + (sizeof(dbChannel*)));
   if (dbChannelPool == NULL)
      return FALSE;
   
   dbChannelPool->maxChannels = maxChannels;
   dbChannelPool->numUsed = 0;
   dbChannelPool->numBroken = 0;
   strncpy(dbChannelPool->database, database, sizeof(dbChannelPool->database));

   if (username == NULL) 
      dbChannelPool->username[0] = '\0';
   else
      strncpy(dbChannelPool->username, username, sizeof(dbChannelPool->username));
   
   if (password == NULL)
      dbChannelPool->password[0] = '\0';
   else
      strncpy(dbChannelPool->password, password, sizeof(dbChannelPool->password));

   dbChannelPool->database[sizeof(dbChannelPool->database)-1] = '\0';
   dbChannelPool->username[sizeof(dbChannelPool->username)-1] = '\0';
   dbChannelPool->password[sizeof(dbChannelPool->password)-1] = '\0';
   memset(dbChannelPool->pool, 0, sizeof(dbChannel*) * dbChannelPool->maxChannels);


   dbChannels = (dbChannel *)
      calloc(dbChannelPool->maxChannels, sizeof(dbChannel));
   if (dbChannels == NULL)
      return FALSE;

   /* Allocate internal channel information */
   dbChannelInfo = (dbInternalChannelInfoType *)
      calloc(dbChannelPool->maxChannels, sizeof(dbInternalChannelInfoType));
   if (dbChannelInfo == NULL)
      return FALSE;

   for(i=0; i<dbChannelPool->maxChannels; i++)
   {
      dbChannelPool->pool[i] = &dbChannels[i];
      dbChannels[i].reserved = (void *) &dbChannelInfo[i];
      dbChannelInfo[i].status = INUSE; /* Mark as in use during login */
      (dbChannelPool->numUsed)++;
#ifdef TRACE
      printf("allocChannelPool: 1 connectiong --> num used now = %d\n", dbChannelPool->numUsed);
#endif
      dbConnect(&dbChannels[i]);
   }
   return TRUE;
}

static void
freeChannelPool(void)
{
   if (dbChannelPool != NULL)
      free(dbChannelPool);
   if (dbChannelInfo != NULL)
      free(dbChannelInfo);
   if (dbChannels != NULL)
      free(dbChannels);
   dbChannels = NULL;
   dbChannelPool = NULL;
   dbChannelInfo = NULL;

   return;
}

static void
dbConnect(dbChannel* chan)
{
   dbInternalChannelInfoType* intChan;
   time_t now;

   if (chan == NULL || chan->reserved == NULL)
      return;                   /* Nothing else we can do here... */
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;
   time(&now);
   intChan->timeoutTime = 0;

   intChan->co.co_genParm.gp_callback = (iiapi_callback) dbConnectFinished;
   intChan->co.co_genParm.gp_closure  = chan;
   intChan->co.co_target              = dbChannelPool->database;
   intChan->co.co_username            = dbChannelPool->username[0] == '\0' ? NULL :
      dbChannelPool->username;
   intChan->co.co_password            = dbChannelPool->password[0] == '\0' ? NULL :
      dbChannelPool->password;
   intChan->co.co_timeout             = CONNECT_TIMEOUT;
   intChan->co.co_connHandle          = NULL;
   intChan->co.co_tranHandle          = NULL;

   intChan->query.gp = &intChan->co.co_genParm;
   IIapi_connect(&intChan->co);
   return;
}

static void
dbConnectFinished(dbChannel* chan, IIAPI_CONNPARM *co)
{
   dbInternalChannelInfoType* intChan;
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;
   (dbChannelPool->numUsed)--;
#ifdef TRACE
   printf("dbConnectFinished: 1 finished --> num used now = %d\n", dbChannelPool->numUsed);
#endif
   if (dbSetErrorEncountered(chan))
   {
      intChan->status = BROKEN;
      (dbChannelPool->numBroken)++;
   }
   else
   {
      intChan->status = FREE;
   }
   return;
}

static void
dbSetDescriptors(dbChannel* chan, IIAPI_QUERYPARM *qy)
{
   dbInternalChannelInfoType* intChan;
   int numParms;

   intChan = (dbInternalChannelInfoType*) chan->reserved;

   if (dbSetErrorEncountered(chan))    /* Dont progress if there are errors */
   {
      dbEndQuery(chan);
      return;
   }
   
   if (qy->qy_parameters == FALSE) /* Nothing to set... */
   {
      dbPutParms(chan, &intChan->query.sd);
      return;
   }
   
   switch (intChan->query.qy.qy_queryType)
   {
      case IIAPI_QT_EXEC_REPEAT_QUERY:
         memcpy(&intChan->query.descriptors[1], intChan->query.userDescriptors,
                sizeof(intChan->query.userDescriptors));
         memcpy(&intChan->query.parameters[1], intChan->query.userParameters,
                sizeof(intChan->query.userParameters));
         break;
      case IIAPI_QT_DEF_REPEAT_QUERY:
         memcpy(&intChan->query.descriptors[3], intChan->query.userDescriptors,
                sizeof(intChan->query.userDescriptors));
         memcpy(&intChan->query.parameters[3], intChan->query.userParameters,
                sizeof(intChan->query.userParameters));
         break;
      default:
         memcpy(intChan->query.descriptors, intChan->query.userDescriptors,
                sizeof(intChan->query.userDescriptors));
         memcpy(intChan->query.parameters, intChan->query.userParameters,
                sizeof(intChan->query.userParameters));
         break;
   }
   
   /* Count the number of parameters supplied & create the descriptor set */
   for(numParms=0;
       numParms < MAX_PARMS, intChan->query.parameters[numParms].dv_value != NULL; 
       numParms++);
   
   intChan->query.sd.sd_genParm.gp_callback = (iiapi_callback) dbPutParms;
   intChan->query.sd.sd_genParm.gp_closure = chan;
   intChan->query.sd.sd_stmtHandle = intChan->query.qy.qy_stmtHandle;
   intChan->query.sd.sd_descriptorCount = numParms;
   intChan->query.pp.pp_parmCount = numParms;
   intChan->query.sd.sd_descriptor = intChan->query.descriptors;
   intChan->query.gp = &intChan->query.sd.sd_genParm;
   IIapi_setDescriptor(&intChan->query.sd);
}

static void
dbPutParms(dbChannel* chan, IIAPI_SETDESCRPARM *sd)
{
   int i;
   dbInternalChannelInfoType* intChan;
   intChan = (dbInternalChannelInfoType*) chan->reserved;

   if (dbSetErrorEncountered(chan))
   {
      dbEndQuery(chan);
      return;
   }
   
   if (intChan->query.sd.sd_descriptorCount == 0)
   {
      dbGetDescriptors(chan, &intChan->query.pp);
      return;
   }
   
   intChan->query.pp.pp_parmCount = intChan->query.sd.sd_descriptorCount;
   intChan->query.pp.pp_stmtHandle = intChan->query.qy.qy_stmtHandle;
   intChan->query.pp.pp_parmData = intChan->query.parameters;
   intChan->query.pp.pp_moreSegments = FALSE;
   if (intChan->query.qy.qy_queryType == IIAPI_QT_DEF_REPEAT_QUERY)
      intChan->query.pp.pp_genParm.gp_callback = (iiapi_callback) dbEndQuery;
   else
      intChan->query.pp.pp_genParm.gp_callback = (iiapi_callback) dbGetDescriptors;
   intChan->query.pp.pp_genParm.gp_closure = chan;
   intChan->query.gp = &intChan->query.pp.pp_genParm;
   IIapi_putParms(&intChan->query.pp);
}

static void
dbGetDescriptors(dbChannel* chan, IIAPI_PUTPARMPARM *pp)
{
   dbInternalChannelInfoType* intChan;

   intChan = (dbInternalChannelInfoType*) chan->reserved;

   if (dbSetErrorEncountered(chan))
   {
      dbEndQuery(chan);
      return;
   }
   
   intChan->query.gd.gd_genParm.gp_callback = (iiapi_callback) dbGetNext;
   intChan->query.gd.gd_genParm.gp_closure = chan;
   intChan->query.gd.gd_stmtHandle = intChan->query.qy.qy_stmtHandle;
   intChan->query.gp = &intChan->query.gd.gd_genParm;
   IIapi_getDescriptor(&intChan->query.gd);
}

static void
dbCloseChannels(void)
{
   int i;
   dbInternalChannelInfoType* intChan;
   
   while (dbChannelPool->numUsed > 0)
   {
      if (dbChannelPool->numBroken + dbChannelPool->numUsed != dbChannelPool->maxChannels)
      {
         for(i=0; i<dbChannelPool->maxChannels; i++)
         {
            intChan = (dbInternalChannelInfoType*) dbChannelPool->pool[i]->reserved;
            if (intChan->status == FREE)
            {
               intChan->status = INUSE;
               dbChannelPool->numUsed++;
               intChan->dc.dc_genParm.gp_callback = (iiapi_callback) dbDisconnectCallback;
               intChan->dc.dc_genParm.gp_closure = dbChannelPool->pool[i];
               intChan->dc.dc_connHandle = intChan->query.qy.qy_connHandle;
               IIapi_disconnect(&intChan->dc);
            }
         }
      }
      dbWaitForAsyncOpps(TRUE);
   }
   return;
}

static void
dbDisconnectCallback(dbChannel* chan, IIAPI_DISCONNPARM* dc)
{
   dbInternalChannelInfoType* intChan;
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;
   intChan->status = BROKEN;
   dbChannelPool->numUsed--;
   dbChannelPool->numBroken++;
}

static void
dbClose(dbChannel* chan, void *unused)
{
   /* Close the statement. */
   /* If repeat query had an invalid handle -- recompile     */
   /* if Defining a repeat query -- retry the compiled query */
   dbInternalChannelInfoType* intChan;
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;
   if (intChan->query.qy.qy_queryType == IIAPI_QT_EXEC_REPEAT_QUERY &&
       chan->status.code == E_AP0014_INVALID_REPEAT_ID)
   {
      intChan->query.cl.cl_genParm.gp_callback = (iiapi_callback) dbDefRepeatQuery;
   }
   else if (intChan->query.qy.qy_queryType == IIAPI_QT_DEF_REPEAT_QUERY)
   {
      intChan->query.cl.cl_genParm.gp_callback = (iiapi_callback) dbRetryRepeatQuery;
      if (intChan->query.gq.gq_mask & IIAPI_GQ_REPEAT_QUERY_ID)
         intChan->query.repHndl->handle = intChan->query.gq.gq_repeatQueryHandle;
      else
         intChan->query.repHndl->handle = NULL;
   }
   else
   {
      intChan->query.cl.cl_genParm.gp_callback = (iiapi_callback) dbFinished;
      if (intChan->query.gq.gq_mask & IIAPI_GQ_ROW_COUNT)
         chan->status.numrows = intChan->query.gq.gq_rowCount;
      else
         chan->status.numrows = 0;
   }
   
   intChan->query.cl.cl_genParm.gp_closure = chan;
   intChan->query.cl.cl_stmtHandle = intChan->query.qy.qy_stmtHandle;
   
   IIapi_close(&intChan->query.cl);
}

static void
dbFinished(dbChannel* chan, void* unused)
{
   dbInternalChannelInfoType* intChan;
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;
   if (dbChannelPool->nextFn != NULL)
   {
      dbFinishedCallback nextFn = dbChannelPool->nextFn;
      dbChannel* finishedChannel= dbChannelPool->finishedChannel;
      if (nextFn != NULL && finishedChannel != NULL)
      {
         dbChannelPool->nextFn = NULL;
         dbChannelPool->finishedChannel = NULL;
         (*nextFn)(finishedChannel, finishedChannel->appData);
      }
   }
   
   dbChannelPool->nextFn = intChan->finishedCallback;
   dbChannelPool->finishedChannel = chan;
   memset(&intChan->query, 0, sizeof(dbQueryInfo));
   return;
}

static void
dbGotData(dbChannel* chan, void* unused)
{
   dbInternalChannelInfoType* intChan;
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;
   if (dbSetErrorEncountered(chan) || chan->status.type == DBSTATUS_NODATA)
   {
      dbEndQuery(chan);         /* Can't get the next row if there are errors... */
      return;
   }

   (intChan->resCallback)(chan, chan->appData);
   return;
}

static Boolean
dbSetErrorEncountered(dbChannel* chan)
{
   dbInternalChannelInfoType* intChan;
   IIAPI_GETEINFOPARM ge;
   
   if (chan == NULL)
      return TRUE;              /* Invalid info -> assume an error */
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;
   
   chan->status.messageText[0] = '\0';

   if (intChan->status == TIMEDOUT)
   {
      strncpy(chan->status.messageText, "Request Timed out", sizeof(chan->status.messageText));
      chan->status.messageText[sizeof(chan->status.messageText)-1] = '\0';
      chan->status.type = DBSTATUS_TIMEDOUT;
      chan->status.code = -1;
      return TRUE;
   }

   if (intChan->query.gp->gp_status == IIAPI_ST_SUCCESS)
   {
      chan->status.type = DBSTATUS_SUCCESS;
      chan->status.code = 0;
      return FALSE;             /* No error */
   }
   if (intChan->query.gp->gp_status == IIAPI_ST_NO_DATA)
   {
      strncpy(chan->status.messageText, "No data returned", sizeof(chan->status.messageText));
      chan->status.messageText[sizeof(chan->status.messageText)-1] = '\0';
      chan->status.type = DBSTATUS_NODATA;
      chan->status.code = 0;
      return FALSE;             /* No error */
   }  

   ge.ge_errorHandle = intChan->query.gp->gp_errorHandle;
   if (ge.ge_errorHandle != NULL)
   {
      IIapi_getErrorInfo(&ge);
      strncpy(chan->status.messageText, ge.ge_message, sizeof(chan->status.messageText));
      chan->status.messageText[sizeof(chan->status.messageText)-1] = '\0';
      switch (ge.ge_type)
      {
         case IIAPI_GE_ERROR:
            chan->status.type = DBSTATUS_ERROR;
            break;
         case IIAPI_GE_WARNING:
            chan->status.type = DBSTATUS_WARNING;
            break;
         case IIAPI_GE_MESSAGE:
            chan->status.type = DBSTATUS_MESSAGE;
            break;
      }
      chan->status.code = ge.ge_errorCode;
   }
   else
   {
      strncpy(chan->status.messageText, "Unknown error", sizeof(chan->status.messageText));
      chan->status.messageText[sizeof(chan->status.messageText)-1] = '\0';
      chan->status.type = DBSTATUS_ERROR;
      chan->status.code = -1;
   }
   return TRUE;                 /* Error found */
}

static void
dbTransEnded(dbChannel* chan, void* unused)
{
   dbInternalChannelInfoType* intChan;
   
   intChan = (dbInternalChannelInfoType*) chan->reserved;
   dbSetErrorEncountered(chan);
   intChan->transaction = NULL; /* Reset transaction handle */

   if (intChan->finishedCallback != NULL) /* Only callback if needed */
   {
      if (dbChannelPool->nextFn != NULL)
      {
         dbFinishedCallback nextFn = dbChannelPool->nextFn;
         dbChannel* finishedChannel= dbChannelPool->finishedChannel;
         if (nextFn != NULL && finishedChannel != NULL)
         {
            dbChannelPool->nextFn = NULL;
            dbChannelPool->finishedChannel = NULL;
            (*nextFn)(finishedChannel, finishedChannel->appData);
         }
      }
      dbChannelPool->nextFn = intChan->finishedCallback;
      dbChannelPool->finishedChannel = chan;
   }
   return;
}

static void
dbDefRepeatQuery(dbChannel* chan, void* unused)
{
   /* Def repeat query */
                                /* Set up select as per normal select */
                                /* - IIAPI_QT_DEF_REPEAT_QUERY */
                                /* Define the parameters */
                                /* Set descriptor */
                                /* Put parameters */
                                /* Get query Info -- for repeat handle */
   dbInternalChannelInfoType* intChan;
   IIAPI_DESCRIPTOR* desc;

   intChan = (dbInternalChannelInfoType*) chan->reserved;
   desc = intChan->query.descriptors;

   desc[0].ds_dataType   = IIAPI_INT_TYPE;
   desc[0].ds_length     = 4;
   desc[0].ds_columnType = IIAPI_COL_SVCPARM;
   intChan->query.parameters[0].dv_value = &intChan->query.repHndl->id1;
   intChan->query.parameters[0].dv_length = 4;

   desc[1].ds_dataType   = IIAPI_INT_TYPE;
   desc[1].ds_length     = 4;
   desc[1].ds_columnType = IIAPI_COL_SVCPARM;
   intChan->query.parameters[1].dv_value = &intChan->query.repHndl->id2;
   intChan->query.parameters[1].dv_length = 4;
   
   desc[2].ds_dataType   = IIAPI_CHA_TYPE;
   desc[2].ds_length     = strlen(intChan->query.repHndl->idStr);
   desc[2].ds_columnType = IIAPI_COL_SVCPARM;
   intChan->query.parameters[2].dv_value = &intChan->query.repHndl->idStr;
   intChan->query.parameters[2].dv_length = desc[2].ds_length;
      
   intChan->query.qy.qy_genParm.gp_callback = (iiapi_callback)dbSetDescriptors;
   intChan->query.qy.qy_genParm.gp_closure = chan;
   intChan->query.qy.qy_connHandle = intChan->co.co_connHandle;
   intChan->query.qy.qy_queryType = IIAPI_QT_DEF_REPEAT_QUERY;
   intChan->query.qy.qy_parameters = TRUE;
   intChan->query.qy.qy_tranHandle = intChan->transaction;
   intChan->query.gp = &intChan->query.qy.qy_genParm;

   IIapi_query(&intChan->query.qy);
   intChan->transaction = intChan->query.qy.qy_tranHandle;
   return;
}

static void
dbRetryRepeatQuery(dbChannel* chan, void *unused)
{
   /* Wrapper for dbRepeatQuery */
   dbInternalChannelInfoType* intChan;

   intChan = (dbInternalChannelInfoType*) chan->reserved;

   dbRepeatQuery(chan, intChan->query.repHndl, intChan->query.sql, intChan->resCallback,
                 intChan->finishedCallback, chan->appData);
}

