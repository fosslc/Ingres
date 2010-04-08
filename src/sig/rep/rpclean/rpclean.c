/*
** Copyright (c) 2004 Ingres Corporation
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef  NT_GENERIC
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <ctype.h>
#include "dblib.h"
#define ROWS_BEFORE_COMMIT 100
#define MAIN_CTX_SIGNITURE 'M'
#define TABLE_CTX_SIGNATURE 'T'

extern char *optarg;

static char *database = NULL;
static int verbose = 0;
static int full = 0;

static void procCommandLine(int argc, char *argv[]);
static void usage(char *progname);
void msetNoReadLock(dbChannel* chan, void* tctx);
void msetReadCommitted(dbChannel* chan, void* tctx);
void getBaseTables(dbChannel* chan, void* fnTable);
void processTable(dbChannel* chan, void* mctx);
void waitUntilFinished(dbChannel* chan, void* mctx);

void setNoReadLock(dbChannel* chan, void* tctx);
void setReadCommitted(dbChannel* chan, void* tctx);
void createTxTable(dbChannel* chan, void* tctx);
void addOldInputQueueTx(dbChannel* chan, void* tctx);
void addDistribQueueTx(dbChannel* chan, void* tctx);
void addOldDistribQueueTx(dbChannel* chan, void* tctx);
void commitTxTable(dbChannel* chan, void* tctx);
void selectOldShadowRows(dbChannel* chan, void* tctx);
void deleteShadowRow(dbChannel* chan, void* tctx);
void selectDeletedShadowRows(dbChannel* chan, void* tctx);
void modifyTable(dbChannel* chan, void* tctx);
void rollbackChanges(dbChannel* chan, void* tctx);
void commitSelChanges(dbChannel* chan, void* tctx);
void dropTmpTable(dbChannel* chan, void* tctx);
void releaseConnections(dbChannel* chan, void* tctx);
void deleteShadowTx(dbChannel* chan, void* tctx);
void deleteArchiveTx(dbChannel* chan, void* tctx);
void checkDeleteCount(dbChannel* chan, void* tctx);
void getNextRow(dbChannel* chan, void* tctx);
void setStructurePtrShadow(dbChannel* chan, void *tctx);
void setStructurePtrShadowIdx(dbChannel* chan, void *tctx);
void setStructurePtrArchive(dbChannel* chan, void *tctx);
void getTableInfo(dbChannel* chan1, void *tctx);
void processTableInfo(dbChannel* chan, void* tctx);
void isIndexPersistent(dbChannel* chan, void *tctx);
void processIndexPersistence(dbChannel* chan, void *tctx);
void getKey(dbChannel* chan, void *tctx);
void processKey(dbChannel* chan, void *tctx);
void createShadowIndex(dbChannel* chan, void *tctx);
void commitSelectChannel(dbChannel* chan, void* tctx);
void commitDeleteChannel(dbChannel* chan, void* tctx);
void modifyArchiveTable(dbChannel* chan, void *tctx);
void getTableLocation(dbChannel* chan, void *tctx);
void processTableLocation(dbChannel* chan, void *tctx);
void swapChannels(dbChannel* chan, void *tctx);
void modifyInputQueue(dbChannel* chan, void* mctx);
void modifyDistribQueue(dbChannel* chan, void* mctx);
void commitMain(dbChannel* chan, void* mctx);
void checkDate(dbChannel* chan, void* mctx);
void processDate(dbChannel* chan, void* mctx);

typedef struct
{
      short sourcedb;
      long trans;
      long seq;
} transid;

char        beforeDate[50];
int concurrency=1;              /* How many tables to clean concurrently */

typedef struct
{
      dbResultCallback resfn;
      dbFinishedCallback finfn;
} fnCallType;

typedef struct
{
      char storage_structure[16+2+1];
      char location_tmp[32+2+1];
      char is_compressed;
      char key_is_compressed;
      char duplicate_rows;
      char unique_rule;
      short  table_ifillpct;
      short  table_dfillpct;
      short  table_lfillpct;
      long table_minpages;
      long table_maxpages;
      char location[9216];      /* Enough for 256 locations ((32+2)*256) */
      char table_name[32+2+1];
      char column_name[32+2+1];
      char sort_dir[2];
      long key_seq;
      char *key;
} tableStructure;

typedef struct
{
      char signature;
      char *errstr;
      dbChannel* selectChan;
      dbChannel* deleteChan;
      char table_owner[32+2+1];
      char table_name[32+2+1];
      long table_no;
      char sql[10240];
      char key[10240];
      transid deleteKey;
      tableStructure shadowStructure;
      tableStructure shadowIdxStructure;
      tableStructure archiveStructure;
      tableStructure* structure; /* Pointer to one of the above two structures */
      long deletedShadowRows;
      long deletedArcRows;
      long totalCommitted;
      char indexPersistent;
      fnCallType* selectFn;
      fnCallType* deleteFn;
} arcctx;

typedef struct
{
      char signature;
      fnCallType* nextFn;
      char table_owner[32+2+1];
      char table_name[32+2+1];
      char sql[10240];
      char key[10240];
      char *errstr;
      long table_no;
      long dateCmp;
      char sysdate[25];
      tableStructure structure;
} mainctx;

long outStandingMods = 0;
int processingMainProcFlow = 0;
arcctx** arcPool;
int poolMax = 50;
int poolUsed = 0;
int poolProcessing = 0;

static fnCallType mainProcFlow[] = 
{
   {NULL, msetNoReadLock},
   {NULL, msetReadCommitted},
   {NULL, checkDate},
   {processDate, getBaseTables},
   {processTable, modifyInputQueue},
   {NULL, getTableInfo},
   {processTableInfo, getTableLocation},
   {processTableLocation, getKey},
   {processKey, modifyTable},
   {NULL, commitMain},
   {NULL, modifyDistribQueue},
   {NULL, getTableInfo},
   {processTableInfo, getTableLocation},
   {processTableLocation, getKey},
   {processKey, modifyTable},
   {NULL, commitMain},
   {NULL, waitUntilFinished},
   {NULL, NULL}
};

static fnCallType finishMain[] =
{
   {NULL, commitMain},
   {NULL, waitUntilFinished},
   {NULL, NULL}
};

static fnCallType processTableFlow[] =
{
   {NULL, setNoReadLock},
   {NULL, setReadCommitted},
   {NULL, createTxTable},
   {NULL, addOldInputQueueTx},
   {NULL, addDistribQueueTx},
   {NULL, addOldDistribQueueTx},
   {NULL, commitSelectChannel},
   {NULL, selectOldShadowRows},
/*   {deleteShadowRow, selectDeletedShadowRows},*/ /*TODO: So when are delete transaction rows deleted? */
   {deleteShadowRow, commitDeleteChannel},
   {NULL, swapChannels},
   {NULL, setStructurePtrShadow},
   {NULL, getTableInfo},
   {processTableInfo, getTableLocation},
   {processTableLocation, setStructurePtrShadowIdx},
   {NULL, getTableInfo},
   {processTableInfo, isIndexPersistent},
   {processIndexPersistence, getKey},
   {processKey, getTableLocation},
   {processTableLocation, setStructurePtrShadow},
   {NULL, modifyTable},
   {NULL, setStructurePtrShadowIdx},
   {NULL, createShadowIndex},
   {NULL, commitSelectChannel},
   {NULL, setStructurePtrArchive},
   {NULL, getTableInfo},
   {processTableInfo, getTableLocation},
   {processTableLocation, modifyTable},
   {NULL, commitSelectChannel},
   {NULL, rollbackChanges},
   {NULL, releaseConnections},
   {NULL, NULL}
};
      
static fnCallType deleteShadowFlow[] =
{
   {NULL, deleteShadowTx},
   {NULL, deleteArchiveTx},
   {NULL, checkDeleteCount},
   {NULL, getNextRow},
   {NULL, NULL}
};

int
main(int argc, char *argv[])
{
   /* Get before date -- check that it is not in the future, default to now (?) */
   /* For each replicated table: */
   /*    Populate temp table */
   /*    set up locking modes */
   /*    get 2 connections */
   /*    select old rows for deletion */
   /*    For each row */
   /*       delete from shadow, archive tables */
   /*    select delete trans rows for deletion */
   /*    For each row */
   /*       delete from shadow, archive tables */
   /*    release connections */
   /*    modify shadow and archive tables */
   /* modify queue tables */
   /* Wait until all connections are released */

   /* TODO: Add date handling -- check for the future: 
      select int4(interval('seconds', 'now' - date('26-may-2000')))    >  0  if date < now*/

   mainctx* ctx;
   dbChannel* chan;
   fnCallType* mainFn = mainProcFlow;
   
   procCommandLine(argc, argv);
   dbInitialize(database, NULL, NULL, 1+(concurrency*2));
   
   printf("Starting rpclean\n");
   chan = dbGetChannel(0);
   if (chan == NULL)
   {
      fprintf(stderr, "Unable to get a valid database channel\n");
      exit(-2);
   }
   
   ctx = (mainctx*) malloc(sizeof(mainctx));
   if (ctx == NULL)
   {
      fprintf(stderr, "Not enough memory to allocate main context.\n");
      exit(-3);
   }
   ctx->signature = MAIN_CTX_SIGNITURE;
   
   arcPool = (arcctx**)malloc(sizeof(arcctx*)*poolMax);
   if (arcPool == NULL)
   {
      fprintf(stderr, "Not enough memory to allocate pool.\n");
      exit(-4);
   }

   memset(ctx, 0, sizeof(ctx));
   processingMainProcFlow = 1;
   ctx->nextFn = mainFn;
   (ctx->nextFn->finfn)(chan, ctx);
   while (processingMainProcFlow == 1 || outStandingMods > 0)
   {
      arcctx *tablectx;

      dbWaitForAsyncOpps(TRUE);
      if (poolProcessing < poolUsed)
      {
         int i;
         for(i=0; i<poolUsed; i++)
         {
            tablectx = arcPool[i];
            if (tablectx->selectChan == NULL)
            {
               /* dbGetChannel cannot be called from a callback invoked by the Open API */
               tablectx->selectChan = dbGetChannel(0);
               tablectx->deleteChan = dbGetChannel(0);
               (tablectx->selectFn->finfn)(tablectx->selectChan, tablectx);
               poolProcessing++;
            }
         }
      }
   }
   return 0;
}

/* mainProcFlow functions */
void
msetNoReadLock(dbChannel* chan, void* tctx)
{
   mainctx* tablectx = (mainctx*)tctx;
   
   if (verbose) printf("Initialising session - setting readlock mode\n");

   (tablectx->nextFn)++;
   dbQuery(chan, "set lockmode session where readlock = nolock",
           tablectx->nextFn->resfn, tablectx->nextFn->finfn, tablectx);
}

void
msetReadCommitted(dbChannel* chan, void* tctx)
{
   mainctx* tablectx = (mainctx*)tctx;
   
   if (verbose) printf("Initialising session - setting isolation level\n");

   (tablectx->nextFn)++;
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "error: %s\n", chan->status.messageText);
      return;
   }
   dbQuery(chan, "set session isolation level read committed",
           tablectx->nextFn->resfn, tablectx->nextFn->finfn, tablectx);
}

void
checkDate(dbChannel* chan, void* mctx)
{
   /* Select the list of replicator registered tables */
   mainctx* ctx = (mainctx*)mctx;
   
   if (verbose) printf("Checking start date\n");
   
   (ctx->nextFn)++;
   dbSetDV(chan, 0, &ctx->dateCmp);
   dbSetDV(chan, 1, ctx->sysdate);
   dbSetParam(chan, 0, string, strlen(beforeDate), beforeDate );
   
   dbQuery(chan, 
           "select int4(interval('seconds', 'now' - date( ~V ))), trim(char(date('now')))",
           ctx->nextFn->resfn, ctx->nextFn->finfn, ctx);
}

void
processDate(dbChannel* chan, void* mctx)
{
   mainctx* ctx = (mainctx*)mctx;

   if (verbose) printf("Sysdate retrieved\n");

   VARCHAR2STR((ctx->sysdate), (ctx->sysdate));
   if (ctx->dateCmp < 0)
   {
      fprintf(stderr, "Warning: date given is in the future -- resetting to 'now'\n");
      strcpy(beforeDate, ctx->sysdate);
   }
   if (strcmp(beforeDate,"now")==0)
   {
      strcpy(beforeDate, ctx->sysdate);
   }
   dbGetNext(chan, ctx);   
}

void
getBaseTables(dbChannel* chan, void* mctx)
{
   /* Select the list of replicator registered tables */
   mainctx* ctx = (mainctx*)mctx;

   printf("Started at %s, all stale entries before %s will be cleaned\n",
          ctx->sysdate, beforeDate);
   
   if (verbose) printf("Retrieving registered tables\n");
   
   if (ctx->sysdate[0]=='\0')
   {
      fprintf(stderr, "Date is invalid\n");
      exit(99);
   }
   
   (ctx->nextFn)++;
   dbSetDV(chan, 0, ctx->table_owner);
   dbSetDV(chan, 1, ctx->table_name);
   dbSetDV(chan, 2, &ctx->table_no);
   dbQuery(chan, 
           "select trim(table_owner), trim(table_name), table_no "
           "from dd_regist_tables",
           ctx->nextFn->resfn, ctx->nextFn->finfn, ctx);
}

static char *
getEditName(char *tablename)
{
   static char editname[32+1];
   char *n;
   char *t = editname;
   
   for(n=tablename; *n != '\0'; n++)
   {
      if (isalpha(*n) || (n != tablename && isdigit(*n)))
         *t = *n;
      else
         *t = '_';
      t++;
   }
   *t = '\0';
   
   return editname;
}

void
processTable(dbChannel* chan, void* mctx)
{
   /* Process each registered table - setup the context and call the first select flow function*/
   arcctx* tablectx;
   mainctx* ctx;
   ctx = (mainctx*)mctx;

   VARCHAR2STR((ctx->table_owner), (ctx->table_owner));
   VARCHAR2STR((ctx->table_name), (ctx->table_name));

   if (verbose) printf("Cleaning %s.%s\n", ctx->table_owner, ctx->table_name);
   
   tablectx = (arcctx*) malloc(sizeof(arcctx));
   if (tablectx == NULL)
   {
      fprintf(stderr, "Not enough memory to create context. %s.%s will not be cleaned\n",
              ctx->table_owner, ctx->table_name);
   }
   else
   {
      outStandingMods++;           /* Increment counter so we can tell when everything has finished */
   
      memset(tablectx, 0, sizeof(arcctx));
      tablectx->signature = TABLE_CTX_SIGNATURE;
      strcpy(tablectx->table_owner, ctx->table_owner);
      strcpy(tablectx->table_name, ctx->table_name);
      tablectx->table_no = ctx->table_no;
      tablectx->selectChan = NULL;
      tablectx->deleteChan = NULL;
      tablectx->selectFn = processTableFlow;
      tablectx->deleteFn = deleteShadowFlow;

      sprintf(tablectx->shadowIdxStructure.table_name, "%.10s%.5ldsx1", 
              getEditName(tablectx->table_name), tablectx->table_no);
      sprintf(tablectx->shadowStructure.table_name, "%.10s%.5ldsha", 
              getEditName(tablectx->table_name), tablectx->table_no);
      sprintf(tablectx->archiveStructure.table_name, "%.10s%.5ldarc", 
              getEditName(tablectx->table_name), tablectx->table_no);

      if (poolUsed == poolMax)
      {
         arcctx** newPool;
         newPool = (arcctx**)realloc(arcPool, sizeof(arcctx*)*(poolMax+50));
         if (newPool == NULL)
         {
            fprintf(stderr, "Not enough memory to increase pool size. %s.%s will not be cleaned\n",
                    ctx->table_owner, ctx->table_name);
            free(tablectx);
            dbGetNext(chan, ctx);
            return;
         }
         arcPool = newPool;
         poolMax += 50;
      }
      arcPool[poolUsed++] = tablectx;
   }
   dbGetNext(chan, ctx);
}

void
commitMain(dbChannel* chan, void* mctx)
{
   mainctx* ctx= (mainctx*)mctx;
   
   if (verbose) printf("Commiting main channel\n");

   (ctx->nextFn)++;
   dbCommit(chan, ctx->nextFn->finfn, ctx);
}

void
modifyInputQueue(dbChannel* chan, void* mctx)
{
   mainctx* ctx = (mainctx*)mctx;
   if (full == 0)               /* Don't proceed to modify the queues if full has not been set */
   {
      ctx->nextFn = finishMain;
      (ctx->nextFn->finfn)(chan, mctx);
      return;
   }

   if (verbose) printf("Modifying dd_input_queue\n");
   
   strcpy(ctx->table_name, "dd_input_queue");
   ctx->errstr="Modifying dd_input_queue";
   strcpy(ctx->structure.table_name, "dd_input_queue");
   ctx->structure.key = ctx->key;
   (ctx->nextFn)++;
   (ctx->nextFn->finfn)(chan, mctx);
}

void
modifyDistribQueue(dbChannel* chan, void* mctx)
{
   mainctx* ctx = (mainctx*)mctx;
   if (verbose) printf("Modifying dd_distrib_queue\n");
   
   strcpy(ctx->table_name, "dd_distrib_queue");
   ctx->errstr="Modifying dd_distrib_queue";
   strcpy(ctx->structure.table_name, "dd_distrib_queue");
   ctx->structure.key = ctx->key;
   (ctx->nextFn)++;
   (ctx->nextFn->finfn)(chan, mctx);
}

void
waitUntilFinished(dbChannel* chan, void* mctx)
{
   /* Exit when everything has finished */
   mainctx* ctx;

   if (verbose) printf("Waiting for table cleaning to finish\n");
   ctx = (mainctx*)mctx;
   free(ctx);
   dbReleaseChannel(&chan);
   processingMainProcFlow = 0;
   return;
}
/* End of mainProcFlow functions */


/* processTableFlow functions */
void
setNoReadLock(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Initialising session - setting readlock mode\n", 
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->selectFn)++;
   tablectx->errstr = "Unable to set lock mode";
   dbQuery(chan, "set lockmode session where readlock = nolock",
           tablectx->selectFn->resfn, tablectx->selectFn->finfn, tablectx);
}

void
setReadCommitted(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }
   if (verbose) printf("%s.%s Initialising session - setting isolation level\n",
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->selectFn)++;
   tablectx->errstr = "Unable to set isolation level";
   dbQuery(chan, "set session isolation level read committed",
           tablectx->selectFn->resfn, tablectx->selectFn->finfn, tablectx);
}

void
createTxTable(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Examining queued transactions (dd_input_queue)\n",
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->selectFn)++;
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }
   tablectx->errstr = "Unable to create transaxtion table";
   dbQuery(chan, 
           "declare global temporary table session.reptranstmp "
           " as "
           "    select sourcedb, transaction_id, sequence_no "
           "    from dd_input_queue "
           " on commit preserve rows "
           " with norecovery, "
           "      duplicates ",
           tablectx->selectFn->resfn, tablectx->selectFn->finfn, tablectx);
}

void
addOldInputQueueTx(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Examining old queued transactions (dd_input_queue)\n",
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->selectFn)++;
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }
   tablectx->errstr = "Unable to add old transactions from dd_input_queue";
   dbQuery(chan, 
           "insert into session.reptranstmp "
           "select old_sourcedb, old_transaction_id, old_sequence_no "
           " from dd_input_queue",
           tablectx->selectFn->resfn, tablectx->selectFn->finfn, tablectx);
}

void
addDistribQueueTx(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Examining queued transactions (dd_distrib_queue)\n",
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->selectFn)++;
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }
   tablectx->errstr = "Unable to add transactions from dd_distrib_queue";
   dbQuery(chan, 
           "insert into session.reptranstmp "
           "select sourcedb, transaction_id, sequence_no "
           " from dd_distrib_queue",
           tablectx->selectFn->resfn, tablectx->selectFn->finfn, tablectx);
}

void
addOldDistribQueueTx(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Examining old queued transactions (dd_distrib_queue)\n",
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->selectFn)++;
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }
   tablectx->errstr = "Unable to add old transactions from dd_distrib_queue";
   dbQuery(chan, 
           "insert into session.reptranstmp "
           "select old_sourcedb, old_transaction_id, old_sequence_no "
           " from dd_distrib_queue",
           tablectx->selectFn->resfn, tablectx->selectFn->finfn, tablectx);
}

void
commitSelectChannel(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Committing select channel changes\n",
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->selectFn)++;
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n\t%s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText, tablectx->sql);
      rollbackChanges(chan, tablectx);
      return;
   }
   tablectx->errstr = "Unable to commit";
   dbCommit(chan, tablectx->selectFn->finfn, tablectx);
}

void
selectOldShadowRows(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;

   if (verbose) printf("%s.%s Retrieving shadow (%s.%s) table rows for deletion\n",
                       tablectx->table_owner, tablectx->table_name,
                       tablectx->table_owner, tablectx->shadowStructure.table_name);

   (tablectx->selectFn)++;
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }
   tablectx->errstr = "Unable to retrieve shadow table rows";
   dbResetParams(chan);
   dbSetDV(chan, 0, &tablectx->deleteKey.sourcedb);
   dbSetDV(chan, 1, &tablectx->deleteKey.trans);
   dbSetDV(chan, 2, &tablectx->deleteKey.seq);
   sprintf(tablectx->sql, 
           "select s.old_sourcedb, s.old_transaction_id, s.old_sequence_no "
           "from %s.%s s, %s.%s s2 "
           "where not exists (select t.sourcedb, t.transaction_id, t.sequence_no "
           "from session.reptranstmp t "
           "where t.sourcedb = s.old_sourcedb "
           "and t.transaction_id = s.old_transaction_id "
           "and t.sequence_no = s.old_sequence_no) "
           "and s2.transaction_id = s.old_transaction_id "
           "and s2.sourcedb = s.old_sourcedb "
           "and s2.sequence_no = s.old_sequence_no "
           "and s2.trans_time < date('%s') ",
           tablectx->table_owner, tablectx->shadowStructure.table_name,
           tablectx->table_owner, tablectx->shadowStructure.table_name,
           beforeDate);
   dbQuery(chan, tablectx->sql,
           tablectx->selectFn->resfn, tablectx->selectFn->finfn, tablectx);
}

void
deleteShadowRow(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Entering delete flow\n",
                       tablectx->table_owner, tablectx->table_name);

   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }

   tablectx->deleteFn = deleteShadowFlow; /* Reset delete flow each time */
   (tablectx->deleteFn->finfn)(tablectx->deleteChan, tablectx);
}

void
commitDeleteChannel(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Committing delete channel changes\n",
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->selectFn)++;
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }
   tablectx->errstr = "Unable to commit";
   dbCommit(tablectx->deleteChan, tablectx->selectFn->finfn, tablectx);
}

void
selectDeletedShadowRows(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Selecting delete trans rows from shadow table\n",
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->selectFn)++;
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }
   tablectx->errstr = "Unable to retrieve delete trans shadow table rows";
   dbResetParams(chan);
   dbSetDV(chan, 0, &tablectx->deleteKey.sourcedb);
   dbSetDV(chan, 1, &tablectx->deleteKey.trans);
   dbSetDV(chan, 2, &tablectx->deleteKey.seq);
   sprintf(tablectx->sql, 
           "select s.sourcedb, s.transaction_id, s.sequence_no "
           "from %s.%s s "
           "where s.trans_type = 3 "
           "and s.trans_time < date('%s') "
           "and not exists (select t.sourcedb, t.transaction_id, t.sequence_no "
           "from session.reptranstmp t "
           "where t.sourcedb = s.sourcedb "
           "and t.transaction_id = s.transaction_id "
           "and t.sequence_no = s.sequence_no) ",
           tablectx->table_owner, tablectx->shadowStructure.table_name,
           beforeDate);
   dbQuery(chan, tablectx->sql,
           tablectx->selectFn->resfn, tablectx->selectFn->finfn, tablectx);
}

void
rollbackChanges(dbChannel* chan, void* tctx)
{
   /* Must commit changes before calling this function!!! */
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Rollingback changes\n",
                       tablectx->table_owner, tablectx->table_name);

   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
   }
   tablectx->errstr = "Unable to rollback changes";

   /* Can't use function pointer table as we sometimes jump steps to get here */
   dbRollback(tablectx->deleteChan, dropTmpTable, tablectx);
}

void
dropTmpTable(dbChannel* chan, void* tctx)
{
   /* Must commit changes before calling this function!!! */
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Dropping tmp table\n",
                       tablectx->table_owner, tablectx->table_name);

   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
   }
   tablectx->errstr = "Unable to drop tmp table";

   /* Can't use function pointer table as we sometimes jump steps to get here */
   dbQuery(tablectx->selectChan, "drop table session.reptranstmp", NULL, commitSelChanges, tablectx);
}

void
commitSelChanges(dbChannel* chan, void* tctx)
{
   /* Must commit changes before calling this function!!! */
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Committing select changes\n",
                       tablectx->table_owner, tablectx->table_name);

   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
   }
   tablectx->errstr = "Unable to commit select changes";

   /* Can't use function pointer table as we sometimes jump steps to get here */
   dbCommit(tablectx->selectChan, releaseConnections, tablectx);
}

void
releaseConnections(dbChannel* chan, void* tctx)
{
   /* Must commit changes before calling this function!!! */
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Releasing connections\n",
                       tablectx->table_owner, tablectx->table_name);

   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
   }

   if (tablectx->selectChan != NULL)
   {
      dbReleaseChannel(&tablectx->selectChan);
   }
   if (tablectx->deleteChan != NULL)
   {
      dbReleaseChannel(&tablectx->deleteChan);
   }
   
   if (tablectx->deleteChan == NULL && tablectx->selectChan == NULL)
   {
      printf("%s.%s cleaned\n", tablectx->table_owner, tablectx->table_name);
      free(tablectx);
      outStandingMods--;
   }
}
/* End of processTableFlow functions */


/* deleteShadowFlow functions */
void
deleteShadowTx(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Deleting shadow table row\n",
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->deleteFn)++;
   tablectx->errstr = "Unable to delete shadow table rows";
   sprintf(tablectx->sql,
           "delete from %s.%s "
           "where sourcedb = ~V "
           "and transaction_id = ~V "
           "and sequence_no = ~V",
           tablectx->table_owner, tablectx->shadowStructure.table_name);

   dbResetParams(chan);
   dbSetParam(chan, 0, integer, 
              sizeof(tablectx->deleteKey.sourcedb), &tablectx->deleteKey.sourcedb);
   dbSetParam(chan, 1, integer, 
              sizeof(tablectx->deleteKey.trans), &tablectx->deleteKey.trans);
   dbSetParam(chan, 2, integer,
              sizeof(tablectx->deleteKey.seq), &tablectx->deleteKey.seq);

   dbQuery(chan, tablectx->sql,
           tablectx->deleteFn->resfn, tablectx->deleteFn->finfn, tablectx);
}

void
deleteArchiveTx(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Deleting archive table row\n",
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->deleteFn)++;
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }
   tablectx->deletedShadowRows += chan->status.numrows;
   
   tablectx->errstr = "Unable to delete archive table rows";
   
   sprintf(tablectx->sql,
           "delete from %s.%s "
           "where sourcedb = ~V "
           "and transaction_id = ~V "
           "and sequence_no = ~V",
           tablectx->table_owner, tablectx->archiveStructure.table_name);

   dbResetParams(chan);
   dbSetParam(chan, 0, integer, 
              sizeof(tablectx->deleteKey.sourcedb), &tablectx->deleteKey.sourcedb);
   dbSetParam(chan, 1, integer, 
              sizeof(tablectx->deleteKey.trans), &tablectx->deleteKey.trans);
   dbSetParam(chan, 2, integer,
              sizeof(tablectx->deleteKey.seq), &tablectx->deleteKey.seq);

   dbQuery(chan, tablectx->sql,
           tablectx->deleteFn->resfn, tablectx->deleteFn->finfn, tablectx);
}

void
checkDeleteCount(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Checking delete count for commit\n",
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->deleteFn)++;
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }
   tablectx->deletedArcRows += chan->status.numrows;
   
   printf("Tx [ '%s.%s' %hd %ld %ld ] deleted\n",
          tablectx->table_owner, 
          tablectx->table_name,
          tablectx->deleteKey.sourcedb,
          tablectx->deleteKey.trans,
          tablectx->deleteKey.seq);
   
   if ((tablectx->deletedArcRows + tablectx->deletedShadowRows) > 
       (ROWS_BEFORE_COMMIT + tablectx->totalCommitted))
   {
      tablectx->errstr = "Unable to commit deleted rows";
      tablectx->totalCommitted += tablectx->deletedArcRows + tablectx->deletedShadowRows;
      dbCommit(chan, tablectx->deleteFn->finfn, tablectx);
   }
   else
   {
      (tablectx->deleteFn->finfn)(chan, tablectx);
   }
}

void
getNextRow(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Getting next row to delete\n",
                       tablectx->table_owner, tablectx->table_name);

   (tablectx->deleteFn)++;
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }

   dbGetNext(tablectx->selectChan, tablectx);
}
/* End of deleteShadowFlow functions */

/* start of modifyShadowFlow */
void
setStructurePtrShadow(dbChannel* chan, void *tctx)
{
   arcctx* tablectx = (arcctx*)tctx;

   (tablectx->selectFn)++;
   tablectx->structure = &tablectx->shadowStructure;
   tablectx->structure->key = "transaction_id, sourcedb, sequence_no";
   (tablectx->selectFn->finfn)(chan, tctx);
}

void
setStructurePtrShadowIdx(dbChannel* chan, void *tctx)
{
   arcctx* tablectx = (arcctx*)tctx;

   (tablectx->selectFn)++;
   tablectx->structure = &tablectx->shadowIdxStructure;
   tablectx->structure->key = tablectx->key;
   (tablectx->selectFn->finfn)(chan, tctx);
}

void
setStructurePtrArchive(dbChannel* chan, void *tctx)
{
   arcctx* tablectx = (arcctx*)tctx;

   (tablectx->selectFn)++;
   tablectx->structure = &tablectx->archiveStructure;
   tablectx->structure->key = "transaction_id, sourcedb, sequence_no";
   (tablectx->selectFn->finfn)(chan, tctx);
}

void
swapChannels(dbChannel* chan, void *tctx)
{
   arcctx* tablectx = (arcctx*)tctx;

   if (full == 0)               /* Don't modify the tables */
   {
      rollbackChanges(chan, tablectx);
      return;
   }

   if (chan == tablectx->selectChan)
      chan = tablectx->deleteChan;
   else
      chan = tablectx->selectChan;
   
   (tablectx->selectFn)++;
   (tablectx->selectFn->finfn)(chan, tctx);
}

void
getTableInfo(dbChannel* chan, void *tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   mainctx* mctx = (mainctx*)tctx;
   char *basetable;
   char *owner;
   char **error;
   fnCallType** nextFn;
   tableStructure* tstruct;
   
   if (tablectx->signature == TABLE_CTX_SIGNATURE)
   {
      basetable = tablectx->table_name;
      owner = tablectx->table_owner;
      error = &tablectx->errstr;
      nextFn = &tablectx->selectFn;
      tstruct = tablectx->structure;
   }
   else
   {
      basetable = mctx->table_name;
      owner = mctx->table_owner;
      error = &mctx->errstr;
      nextFn = &mctx->nextFn;
      tstruct = &mctx->structure;
   }
   
   if (verbose) printf("%s.%s Getting table structure (%s)\n",
                       owner, basetable, tstruct->table_name);
   
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              owner, basetable, *error, chan->status.messageText);
      if (tablectx->signature == TABLE_CTX_SIGNATURE)
      {
          rollbackChanges(chan, tablectx);
          return;
      }
   }

   (*nextFn)++;
   *error = "Unable to get table structure";
   
   dbResetParams(chan);
   dbResetDV(chan);
   dbSetParam(chan, 0, string, strlen(tstruct->table_name), tstruct->table_name);
   dbSetParam(chan, 1, string, strlen(owner), owner);
   dbSetDV(chan, 0, tstruct->storage_structure);
   dbSetDV(chan, 1, tstruct->location);
   dbSetDV(chan, 2, &tstruct->is_compressed);
   dbSetDV(chan, 3, &tstruct->key_is_compressed);
   dbSetDV(chan, 4, &tstruct->duplicate_rows);
   dbSetDV(chan, 5, &tstruct->unique_rule);
   dbSetDV(chan, 6, &tstruct->table_ifillpct);
   dbSetDV(chan, 7, &tstruct->table_dfillpct);
   dbSetDV(chan, 8, &tstruct->table_lfillpct);
   dbSetDV(chan, 9, &tstruct->table_minpages);
   dbSetDV(chan, 10, &tstruct->table_maxpages);

   dbQuery(chan,
           "select TRIM(storage_structure), TRIM(location_name), is_compressed, key_is_compressed, "
           "duplicate_rows, unique_rule, table_ifillpct, table_dfillpct, table_lfillpct, "
           "table_minpages, table_maxpages from iitables "
           "where LOWERCASE(table_name) = LOWERCASE( ~V ) and table_owner = ~V",
           (*nextFn)->resfn, (*nextFn)->finfn, tctx);
}

void
processTableInfo(dbChannel* chan, void* tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   mainctx* mctx = (mainctx*)tctx;
   char *basetable;
   char *owner;
   char **error;
   fnCallType** nextFn;
   tableStructure* tstruct;
   
   if (tablectx->signature == TABLE_CTX_SIGNATURE)
   {
      basetable = tablectx->table_name;
      owner = tablectx->table_owner;
      error = &tablectx->errstr;
      nextFn = &tablectx->selectFn;
      tstruct = tablectx->structure;
   }
   else
   {
      basetable = mctx->table_name;
      owner = mctx->table_owner;
      error = &mctx->errstr;
      nextFn = &mctx->nextFn;
      tstruct = &mctx->structure;
   }
   
   if (verbose) printf("%s.%s processing table structure (%s)\n",
                       owner, basetable, tstruct->table_name);
   
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              owner, basetable, *error, chan->status.messageText);
      if (tablectx->signature == TABLE_CTX_SIGNATURE)
      {
         rollbackChanges(chan, tablectx);
         return;
      }
   }

   *error = "Processing table structure";
   VARCHAR2STR((tstruct->storage_structure), (tstruct->storage_structure));   
   VARCHAR2STR((tstruct->location), (tstruct->location));   
   dbGetNext(chan, tctx);
}

void
isIndexPersistent(dbChannel* chan, void *tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s Determining if index is Persistent\n",
                       tablectx->table_owner, tablectx->table_name);
   
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }

   (tablectx->selectFn)++;
   tablectx->errstr = "Unable to determine indexes persistence";
   
   dbResetParams(chan);
   dbResetDV(chan);
   
   dbSetParam(chan, 0, string, sizeof(tablectx->structure->table_name), tablectx->structure->table_name);
   dbSetParam(chan, 1, string, sizeof(tablectx->table_owner), tablectx->table_owner);
   dbSetDV(chan, 0, &tablectx->indexPersistent);

   dbQuery(chan,
           "select persistent from iiindexes where index_name = ~V and index_owner = ~V ",
           tablectx->selectFn->resfn, tablectx->selectFn->finfn, tablectx);
}

void
processIndexPersistence(dbChannel* chan, void *tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   if (verbose) printf("%s.%s processing index persistence\n",
                       tablectx->table_owner, tablectx->table_name);
   
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText);
      rollbackChanges(chan, tablectx);
      return;
   }

   tablectx->errstr = "Processing index persistence";
   dbGetNext(tablectx->selectChan, tablectx);
}

void
getKey(dbChannel* chan, void *tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   mainctx* mctx = (mainctx*)tctx;
   char *basetable;
   char *owner;
   char **error;
   fnCallType** nextFn;
   tableStructure* tstruct;
   
   if (tablectx->signature == TABLE_CTX_SIGNATURE)
   {
      basetable = tablectx->table_name;
      owner = tablectx->table_owner;
      error = &tablectx->errstr;
      nextFn = &tablectx->selectFn;
      tstruct = tablectx->structure;
   }
   else
   {
      basetable = mctx->table_name;
      owner = mctx->table_owner;
      error = &mctx->errstr;
      nextFn = &mctx->nextFn;
      tstruct = &mctx->structure;
   }
   
   if (verbose) printf("%s.%s Getting key\n", owner, basetable);
   
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              owner, basetable, *error, chan->status.messageText);
      if (tablectx->signature == TABLE_CTX_SIGNATURE)
      {
         rollbackChanges(chan, tablectx);
         return;
      }
   }

   (*nextFn)++;
   *error = "Unable to determine key";
   
   dbResetParams(chan);
   dbResetDV(chan);
   
   tstruct->key[0] = '\0';
   dbSetParam(chan, 0, string, sizeof(tstruct->table_name), tstruct->table_name);
   dbSetParam(chan, 1, string, strlen(owner), owner);
   dbSetDV(chan, 0, tstruct->column_name);
   dbSetDV(chan, 1, &tstruct->key_seq);
   dbSetDV(chan, 2, tstruct->sort_dir);

   dbQuery(chan,
           "select TRIM(column_name), key_sequence, sort_direction "
           "from iicolumns "
           "where LOWERCASE(table_name) = ~V and table_owner = ~V "
           "and key_sequence!=0 "
           "and LOWERCASE(column_name) !='tidp' "
           "order by key_sequence",
           (*nextFn)->resfn, (*nextFn)->finfn, tctx);
}

void
processKey(dbChannel* chan, void *tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   mainctx* mctx = (mainctx*)tctx;
   char *basetable;
   char *owner;
   char **error;
   fnCallType** nextFn;
   tableStructure* tstruct;
   
   if (tablectx->signature == TABLE_CTX_SIGNATURE)
   {
      basetable = tablectx->table_name;
      owner = tablectx->table_owner;
      error = &tablectx->errstr;
      nextFn = &tablectx->selectFn;
      tstruct = tablectx->structure;
   }
   else
   {
      basetable = mctx->table_name;
      owner = mctx->table_owner;
      error = &mctx->errstr;
      nextFn = &mctx->nextFn;
      tstruct = &mctx->structure;
   }
   
   if (verbose) printf("%s.%s processing key\n",
                       owner, basetable);
   
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              owner, basetable, *error, chan->status.messageText);
      if (tablectx->signature == TABLE_CTX_SIGNATURE)
      {
         rollbackChanges(chan, tablectx);
         return;
      }
   }

   if (tstruct->key[0] != '\0')    /* Only add the comma if not the first column */
      strcat(tstruct->key, ", ");
   
   VARCHAR2STR((tstruct->column_name), (tstruct->column_name));
   strcat(tstruct->key, tstruct->column_name);
   
   *error = "Processing key";
   dbGetNext(chan, tctx);
}

void
getTableLocation(dbChannel* chan, void *tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   mainctx* mctx = (mainctx*)tctx;
   char *basetable;
   char *owner;
   char **error;
   fnCallType** nextFn;
   tableStructure* tstruct;
   
   if (tablectx->signature == TABLE_CTX_SIGNATURE)
   {
      basetable = tablectx->table_name;
      owner = tablectx->table_owner;
      error = &tablectx->errstr;
      nextFn = &tablectx->selectFn;
      tstruct = tablectx->structure;
   }
   else
   {
      basetable = mctx->table_name;
      owner = mctx->table_owner;
      error = &mctx->errstr;
      nextFn = &mctx->nextFn;
      tstruct = &mctx->structure;
   }

   if (verbose) printf("%s.%s Getting Table location (%s)\n",
                       owner, basetable, tstruct->table_name);
   
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              owner, basetable, *error, chan->status.messageText);
      if (tablectx->signature == TABLE_CTX_SIGNATURE)
      {
         rollbackChanges(chan, tablectx);
         return;
      }
   }

   (*nextFn)++;
   *error = "Unable to determine table location";
   
   dbResetParams(chan);
   dbResetDV(chan);
   
   dbSetParam(chan, 0, string, strlen(tstruct->table_name), tstruct->table_name);
   dbSetParam(chan, 1, string, strlen(owner), owner);
   dbSetDV(chan, 0, tstruct->location_tmp);

   dbQuery(chan,
           "select TRIM(location_name) "
           "from iimulti_locations "
           "where LOWERCASE(table_name) = ~V and table_owner = ~V ",
           (*nextFn)->resfn, (*nextFn)->finfn, tctx);
}

void
processTableLocation(dbChannel* chan, void *tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   mainctx* mctx = (mainctx*)tctx;
   char *basetable;
   char *owner;
   char **error;
   fnCallType** nextFn;
   tableStructure* tstruct;
   
   if (tablectx->signature == TABLE_CTX_SIGNATURE)
   {
      basetable = tablectx->table_name;
      owner = tablectx->table_owner;
      error = &tablectx->errstr;
      nextFn = &tablectx->selectFn;
      tstruct = tablectx->structure;
   }
   else
   {
      basetable = mctx->table_name;
      owner = mctx->table_owner;
      error = &mctx->errstr;
      nextFn = &mctx->nextFn;
      tstruct = &mctx->structure;
   }
   
   if (verbose) printf("%s.%s processing table location\n",
                       owner, basetable);
   
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              owner, basetable, *error, chan->status.messageText);
      if (tablectx->signature == TABLE_CTX_SIGNATURE)
      {
         rollbackChanges(chan, tablectx);
         return;
      }
   }

   strcat(tstruct->location, ", ");
   
   VARCHAR2STR((tstruct->location_tmp), (tstruct->location_tmp));
   strcat(tstruct->location, tstruct->location_tmp);
   
   *error = "Processing table location";
   dbGetNext(chan, tctx);
}

void
modifyTable(dbChannel* chan, void *tctx)
{
   /* structure pointer must be set to the correct table first */
   arcctx* tablectx = (arcctx*)tctx;
   mainctx* mctx = (mainctx*)tctx;
   char *basetable;
   char *owner;
   char **error;
   char *sql;
   fnCallType** nextFn;
   tableStructure* tstruct;
   
   if (tablectx->signature == TABLE_CTX_SIGNATURE)
   {
      basetable = tablectx->table_name;
      owner = tablectx->table_owner;
      error = &tablectx->errstr;
      nextFn = &tablectx->selectFn;
      tstruct = tablectx->structure;
      sql = tablectx->sql;
   }
   else
   {
      basetable = mctx->table_name;
      owner = mctx->table_owner;
      error = &mctx->errstr;
      nextFn = &mctx->nextFn;
      tstruct = &mctx->structure;
      sql = mctx->sql;
   }
   
   printf("Modifying %s.%s\n", owner, tstruct->table_name);
   if (verbose) printf("%s.%s Modifying table (%s)\n",
                       owner, basetable, tstruct->table_name);
   
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n", 
              owner, basetable, *error, chan->status.messageText);
      if (tablectx->signature == TABLE_CTX_SIGNATURE)
      {
         rollbackChanges(chan, tablectx);
         return;
      }
   }
   if (tstruct->key[0] == '\0') /* Don't make changes unless all info has been obtained */
   {
      fprintf(stderr, "%s.%s Index key not retrieved.\n", owner, basetable);
      if (tablectx->signature == TABLE_CTX_SIGNATURE)
      {
         rollbackChanges(chan, tctx);
         return;
      }
   }
      
   (*nextFn)++;
   *error = "Unable to modify table";
   
   dbResetParams(chan);
   dbResetDV(chan);
   
   sprintf(sql, "modify %s to %s on %s with location = (%s) ", 
           tstruct->table_name, tstruct->storage_structure,
           tstruct->key, tstruct->location);

   if ((strcmp(tstruct->storage_structure, "ISAM") == 0) ||
       (strcmp(tstruct->storage_structure, "BTREE") == 0) ||
       (strcmp(tstruct->storage_structure, "HASH") == 0))
   {
      sprintf(&sql[strlen(sql)], ", fillfactor = %d",
              tstruct->table_ifillpct);
      if (strcmp(tstruct->storage_structure, "BTREE") == 0)
         sprintf(&sql[strlen(sql)],", nonleaffill = %d, leaffill = %d ",
                 tstruct->table_dfillpct, 
                 tstruct->table_lfillpct);
   }
   else if (strcmp(tstruct->storage_structure, "HASH") == 0)
   {
      sprintf(&sql[strlen(sql)],
              ", maxpages = %ld, minpages = %ld",
              tstruct->table_maxpages, 
              tstruct->table_minpages);
   }
   
   dbQuery(chan, sql, (*nextFn)->resfn, (*nextFn)->finfn, tctx);
}

void
createShadowIndex(dbChannel* chan, void *tctx)
{
   arcctx* tablectx = (arcctx*)tctx;
   
   printf("Creating Index %s.%s\n", tablectx->table_owner, tablectx->shadowStructure.table_name);
   if (verbose) printf("%s.%s Creating shadow index\n",
                       tablectx->table_owner, tablectx->table_name);
   
   if (dbErrorEncountered(chan))
   {
      fprintf(stderr, "%s.%s %s: %s\n\t%s\n", 
              tablectx->table_owner, tablectx->table_name,
              tablectx->errstr, chan->status.messageText, tablectx->sql);
      rollbackChanges(chan, tablectx);
      return;
   }

   tablectx->errstr = "Unable to create shadow index";
   
   dbResetParams(chan);
   dbResetDV(chan);

   if (tablectx->indexPersistent == 'Y')
   {
      /* No need to create index, so just call the finished callback directly */
      if (verbose) printf("%s.%s: Index does not need to be created\n", 
                          tablectx->table_owner, tablectx->table_name);
      (modifyTable)(chan, tablectx);
      return;
   }

   (tablectx->selectFn)++;
   if (tablectx->key[0] == '\0')
   {
      /* Can't create the index --> no key was retrieved */
      if (verbose) printf("%s.%s: Index cannot be created, no index key was found\n", 
                          tablectx->table_owner, tablectx->table_name);
      (tablectx->selectFn->finfn)(chan, tablectx);
      return;
   }
   
   sprintf(tablectx->sql, 
           "create index %s on %s (%s, in_archive) with structure = %s, key = (%s), location = (%s), persistence ",
           tablectx->structure->table_name, tablectx->shadowStructure.table_name, tablectx->key, 
           tablectx->structure->storage_structure, tablectx->key, tablectx->structure->location);

   if ((strcmp(tablectx->structure->storage_structure, "ISAM") == 0) ||
       (strcmp(tablectx->structure->storage_structure, "BTREE") == 0) ||
       (strcmp(tablectx->structure->storage_structure, "HASH") == 0))
   {
      sprintf(&tablectx->sql[strlen(tablectx->sql)], ", fillfactor = %d",
              tablectx->structure->table_ifillpct);
      if (strcmp(tablectx->structure->storage_structure, "BTREE") == 0)
         sprintf(&tablectx->sql[strlen(tablectx->sql)],", nonleaffill = %d, leaffill = %d ",
                 tablectx->structure->table_dfillpct, 
                 tablectx->structure->table_lfillpct);
   }
   else if (strcmp(tablectx->structure->storage_structure, "HASH") == 0)
   {
      sprintf(&tablectx->sql[strlen(tablectx->sql)],
              ", maxpages = %ld, minpages = %ld",
              tablectx->structure->table_maxpages, 
              tablectx->structure->table_minpages);
   }
   
   dbQuery(chan, tablectx->sql,
           tablectx->selectFn->resfn, tablectx->selectFn->finfn, tablectx);
}
/* end of modifyShadowFlow */

static void
procCommandLine(int argc, char *argv[])
{
   int i;
   
   strcpy(beforeDate, "now");
   concurrency=10;
   while ((i = getopt(argc, argv, "b:d:n:vf")) != EOF)
   {
      switch(i)
      {
         case 'b':
            if (*optarg == '-')
               usage(argv[0]);
            else
               strcpy(beforeDate,optarg);
            break;
         case 'd':
            if (*optarg == '-')
               usage(argv[0]);
            else
               database = optarg;
            break;
         case 'n':
            if (*optarg == '-')
               usage(argv[0]);
            else
            {
               concurrency = atoi(optarg);
               if (concurrency < 1)
                  concurrency = 1;
               else if (concurrency > MAX_CHANNELS)
                  concurrency = MAX_CHANNELS;
            }
            break;
         case 'v':
            printf("Processing command line args\n");
            verbose = 1;
            break;
         case 'f':
            full = 1;
            break;
         default:
            usage(argv[0]);
      }
   }
   if (database == NULL)
      usage(argv[0]);
}

static void
usage(char *progname)
{
   fprintf(stderr, "Usage: %s -d database [-b beforedate] [-n concurrency] [-f] [-v]\n", progname);
   fprintf(stderr, "       where database is the database to clean\n");
   fprintf(stderr, "             beforedate specifies the latest rows that can be cleaned\n");
   fprintf(stderr, "             concurrency is how many tables to clean concurrently\n");
   exit(-1);
}
