/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : dbamvcfg.sc
//    Propagate for replicator
//
**   26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**   08-Jul-2002 (schph01)
**     BUG #108199 In insert statements, ensure there is a space after the
**     comma sign that separates the contains of two columns.
**   02-Jun-2010 (drivi01)
**     Remove yet another hardcoded buffer size.  
********************************************************************/
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>
#include "error.h"
#include "dba.h"
#include "dbaset.h"
#include "dbaginfo.h"
#include "dbaparse.h"
#include "dbadlg1.h"
#include "dbatime.h"
#include "settings.h"
#include "main.h"       // hwndMDIClient
#include "msghandl.h"
#include "dll.h"
#include "domdata.h"
#include "tools.h"
#include "catolist.h"
#include "monitor.h"
#include "resource.h"

/* Warning: redefinition */
# define	DB_PRIVATE	0000000		/* indicates if database is
						** public or private. If first
						** bit in iidatabase.ACCESS is
						** clear then database=private
						*/
# define	DB_GLOBAL	0000001		/* If first bit in
						** iidatabase.ACCESS is set
						** then public ("GLOBAL")
						*/

exec sql include sqlca;
exec sql include sqlda;
#include "sqltls.h"

static int CurReplType=REPLIC_NOINSTALL;

void SetCurReplicType(int iRepType)
{
  CurReplType=iRepType;
}

/*******************************************************************
** Function :Copy the replicator configuration from the target    **
**          database to the highlighted database, provided that   **
**          it is not of type 3.                                  **
** Author   : Lionel Storck                                       **
** Parameters :                                                   **
**       LpSourceNode : Node to copy from.                        **
**       LpSourceDb   : Database to copy from.                    **
**       LpTargetNode : Node to copy to.                          **
**       LpTargetDb   : Database to copy to.                      **
** Steps :                                                        **
**       1) Export the source replication objects                 **
**       2) Create the target replication object if they do not   **     
**          exist                                                 **
**       3) Import the replication object into the target node    ** 
**       4) Change the target replication table storage structure **
*******************************************************************/

typedef struct _ddExportFileList {
   LPUCHAR ExportTableName;
   UCHAR ExportFileName[_MAX_PATH];
   LPUCHAR SqlCreateObj;
} DDEXPORTFILELIST, far *LPDDEXPORTFILELIST;

static int Export_DD_Tables(LPUCHAR lpSourceNode, LPUCHAR lpSourceDb, 
                     LPDDEXPORTFILELIST lpFileList);
static int Import_DD_Tables(LPUCHAR lpTargetNode, LPUCHAR lpTargetDb, 
                     LPDDEXPORTFILELIST lpFileList);
static void Delete_Export_DD_Files(LPDDEXPORTFILELIST lpFileList);
static int Create_Replication_Objects(LPUCHAR lpTargetNode, 
                     LPUCHAR lpTargetDb, LPUCHAR lpDba);
static int AddDbIntoInstallation(LPUCHAR lpTargetNode);
static int ProcessRegisteredTables(LPUCHAR lpSourceNode, LPUCHAR lpSourceDb, 
                                   LPUCHAR lpTargetNode, LPUCHAR lpTargetDb);
static int CreateSupportTables(LPUCHAR lpTargetNode, LPUCHAR lpTargetDb,
            LPUCHAR table_name, LPUCHAR table_owner, int iTargetType);
static int CreateReplicationRules(LPUCHAR lpSourceNode, LPUCHAR lpTargetNode,
                                  LPUCHAR lpSourceDb, LPUCHAR lpTargetDb,  
                                  LPUCHAR table_name, LPUCHAR table_owner,
                                  int iSourceSession, int iTargetSession);
static int TableExists(LPUCHAR TableName, LPUCHAR table_owner, int *Exists);

int DbaPropagateReplicatorCfg(lpSourceNode, lpSourceDb,
                              lpTargetNode, lpTargetDb)
exec sql begin declare section;
   unsigned char *lpSourceNode;
   unsigned char *lpSourceDb;
   unsigned char *lpTargetNode;
   unsigned char *lpTargetDb;
exec sql end declare section;

   {

   static DDEXPORTFILELIST ddFileList[] = {  
       {"dd_connections",        "\0", "modify dd_connections to "
                                       "btree unique on database_no"},     
       {"dd_registered_tables",  "\0", "modify dd_registered_tables to "
                                       "btree unique on tablename"},     
       {"dd_registered_columns", "\0", "modify dd_registered_columns to "
                                       "btree unique on tablename, "
                                       "column_name"},
       {"dd_distribution",       "\0", "modify dd_distribution to "
                                       "btree unique on source, target, "
                                       "localdb, dd_routing"},
       {NULL,                    "\0", NULL}};

   int iret;

	iret=Export_DD_Tables(lpSourceNode, lpSourceDb, ddFileList);

   if (iret==RES_SUCCESS) 
      {
   	iret=Import_DD_Tables(lpTargetNode, lpTargetDb, ddFileList);
      if (iret==RES_SUCCESS)
         iret=ProcessRegisteredTables(lpSourceNode, lpSourceDb,
                                      lpTargetNode, lpTargetDb);
      }
   // Delete the export files no longer used.
   
   Delete_Export_DD_Files(ddFileList);

   return iret;
	}

/*******************************************************************
** Function : Created one export file per table given in the list **
** Parameters :                                                   **
**       LpSourceNode : Node to connect to.                       **
**       LpSourceDb   : Database to connect to.                   **
**       LpFileList   : Pointer to an array of tables names to be **
**                      exported and the coresponding export file **
**                      name(NULL upon entry, updated by this     **
**                      function)                                 **
*******************************************************************/


static BOOL AddCol2List(LPOBJECTLIST *lplpColList,
                        LPUCHAR      lpColName,
                        int          ColType,
                        BOOL         bwNulls,
                        BOOL         bwDefault)
{
   LPOBJECTLIST   ls;
   LPCOLUMNPARAMS lpCol;
   ls =  AddListObjectTail(lplpColList, sizeof(COLUMNPARAMS));
   if (!ls)
     return FALSE;
   lpCol = (LPCOLUMNPARAMS)ls->lpObject;
   x_strcpy(lpCol->szColumn, lpColName);
   lpCol->uDataType = ColType;
   lpCol->bNullable = bwNulls;
   lpCol->bDefault  = bwDefault;
   return TRUE;
}

  
static int Export_DD_Tables(LPUCHAR lpSourceNode, LPUCHAR lpSourceDb, 
                     LPDDEXPORTFILELIST lpFileList)

   {
   int iret=RES_SUCCESS;
   exec sql begin declare section;
   unsigned char szDba[MAXOBJECTNAME];
   unsigned char szUser[MAXOBJECTNAME];
   exec sql end declare section;
   UCHAR szConnectName[2*MAXOBJECTNAME+2];
   int iSession;

   wsprintf(szConnectName,"%s::%s",lpSourceNode, lpSourceDb);
   iret = Getsession(szConnectName,SESSION_TYPE_INDEPENDENT, &iSession);
   if (iret!=RES_SUCCESS)
      return iret;

   exec sql repeated select dbmsinfo('dba'), dbmsinfo('username')
      into :szDba, :szUser;
   if (x_strcmp(szDba, szUser))
      return RES_NOTDBA;

   while (lpFileList->ExportTableName) 
      {
      GetTempFileName(0, "dba", 0, lpFileList->ExportFileName);
      iret=ExportTableData(lpSourceNode, lpSourceDb, szDba,
                        lpFileList->ExportTableName,
                        lpFileList->ExportFileName, FALSE, NULL);
      if (iret==RES_ENDOFDATA) 
         iret=RES_SUCCESS;
      if (iret!=RES_SUCCESS) 
         break;
      lpFileList++;
      }
   if (iret==RES_SUCCESS)
      iret=ReleaseSession(iSession, RELEASE_COMMIT);
   else 
      ReleaseSession(iSession, RELEASE_ROLLBACK);
   return iret;
   }

/*******************************************************************
** Function : Imports each table given by the lpFileList.         **
**            The data in the table are first deleted.            **
** Parameters :                                                   **
**       LpTargetNode : Node to connect to.                       **
**       LpTargetDb   : Database to connect to.                   **
**       LpFileList   : Pointer to an array of tables names to be **
**                      loaded and the coresponding export file   **
**                      name                                      **
*******************************************************************/

static int Import_DD_Tables(LPUCHAR lpTargetNode, LPUCHAR lpTargetDb, 
                     LPDDEXPORTFILELIST lpFileList)

   {
   int iret, iSession;
   UCHAR szConnectName[2*MAXOBJECTNAME+2];

   exec sql begin declare section;
   unsigned char szDba[MAXOBJECTNAME];
   unsigned char szUser[MAXOBJECTNAME];
   int iRowCount;
   exec sql end declare section;
   UCHAR szSqlStm[2048];

   wsprintf(szConnectName,"%s::%s",lpTargetNode, lpTargetDb);
   iret = Getsession(szConnectName,SESSION_TYPE_INDEPENDENT, &iSession);

   if (iret!=RES_SUCCESS)
      return iret;

   // Get the DBA name

   exec sql repeated select dbmsinfo('dba'), dbmsinfo('username')
      into :szDba, :szUser;
   if (x_strcmp(szDba, szUser))
      {
      ReleaseSession(iSession, RELEASE_ROLLBACK);
      return RES_NOTDBA;
      }

   CommitSession(iSession);

 	 iret=Create_Replication_Objects(lpTargetNode, lpTargetDb, szDba);

   ActivateSession(iSession);
   CommitSession(iSession);


   // check the version of replicator

	 exec sql repeated select count (*) into :iRowCount
        from iitables
		    where table_name='dd_mobile_opt'; 

	 if (sqlca.sqlcode < 0)  {
      ReleaseSession(iSession, RELEASE_ROLLBACK);
      return RES_ERR;
   }
   CommitSession(iSession);
   if (   (iRowCount!=0 && CurReplType==REPLIC_V10) ||
           (iRowCount==0 && CurReplType==REPLIC_V105)  ) {
      ReleaseSession(iSession, RELEASE_ROLLBACK);
      //"Replicator versions are different on source and target nodes. Propagate failed."
      TS_MessageBox(TS_GetFocus(),ResourceString(IDS_ERR_REPLICATION_VERS),
                    NULL, MB_OK | MB_ICONHAND | MB_TASKMODAL);
      return RES_ERR;
   }

   while (lpFileList->ExportTableName && iret==RES_SUCCESS) 
      {

	   wsprintf(szSqlStm, "modify %s to truncated", 
               lpFileList->ExportTableName);
      iret=ExecSQLImm(szSqlStm,FALSE, NULL, NULL, NULL);

      if (iret=RES_ENDOFDATA)
         iret=RES_SUCCESS;
      if (iret!=RES_SUCCESS) 
         break;

      wsprintf(szSqlStm, "copy table %s() from '%s'",
                       lpFileList->ExportTableName, 
                       lpFileList->ExportFileName);
      iret=ExecSQLImm(szSqlStm,FALSE, NULL, NULL, NULL);
      if (iret==RES_ENDOFDATA) 
         iret=RES_SUCCESS;
      if (iret!=RES_SUCCESS) 
         break;

      iret=ExecSQLImm(lpFileList->SqlCreateObj, FALSE,
                     NULL, NULL, NULL);
      if (iret==RES_ENDOFDATA) 
         iret=RES_SUCCESS;
      if (iret!=RES_SUCCESS) 
         break;

      lpFileList++;
      }

   if (iret==RES_SUCCESS)
      iret=AddDbIntoInstallation(lpTargetNode);

   if (iret==RES_SUCCESS)
      iret=ReleaseSession(iSession, RELEASE_COMMIT);
   else 
      ReleaseSession(iSession, RELEASE_ROLLBACK);
   return iret;
   }

/*******************************************************************
** Function : Cleans up the Imports file given by the lpFileList. **
** Parameters :                                                   **
**       LpFileList   : Pointer to an array of file names to be   **
**                      deleted                                   **
*******************************************************************/

static void Delete_Export_DD_Files(LPDDEXPORTFILELIST lpFileList)

   {

   OFSTRUCT ofFileStruct;

   while (*(lpFileList->ExportFileName)) 
      OpenFile((LPCSTR)(lpFileList++)->ExportFileName, &ofFileStruct, OF_DELETE);

   }

/*******************************************************************
** Function : Creates the replication objects (remote commands)   **
** Parameters :                                                   **
**       lpTargetNode and lpTarget db, respectively target node   **
**                    and target database.                        **
*******************************************************************/

static int Create_Replication_Objects(LPUCHAR lpTargetNode, 
            LPUCHAR lpTargetDb, LPUCHAR lpDba)
   {
   
   exec sql begin declare section;
   int iRowCount;
   exec sql end declare section;

   int iAnswer;
   UCHAR szTxt[MAXOBJECTNAME*6]; // TO BE FINISHED string should be loaded from res.
   UCHAR szTxt2[MAXOBJECTNAME*3];
   UCHAR tmpusername[MAXOBJECTNAME];
   HWND  hwndParent;  // Emb Sep. 13, 95: parent needed for message box...

	exec sql repeated select count (*) into :iRowCount
      from iitables
		where table_name='dd_installation' 
       and  table_owner=dbmsinfo('dba');

	if (sqlca.sqlcode)
      return RES_ERR;

  exec sql commit;

	if (iRowCount!=0)	/* No need to create replication objects */
		{
      return RES_SUCCESS;
		}
   //  " Warning : The replication objects are not installed"
   //  " in the target Vnode (%s::%s)\n\n"
   //  " Do you want to install them ?\n",
   wsprintf(
       szTxt, 
       ResourceString ((UINT)IDS_W_REPL_NOT_INSTALL),
       lpTargetNode, 
       lpTargetDb);

   hwndParent = TS_GetFocus();
   iAnswer=TS_MessageBox(hwndParent, szTxt,
                      ResourceString ((UINT)IDS_I_PROPAGATE_REPOBJ), //Propagate Replication Objects
                      MB_YESNO | MB_ICONEXCLAMATION | MB_TASKMODAL);
   if (iAnswer==IDNO)
       return RES_USER_CANCEL;

   DBAGetUserName(lpTargetNode,tmpusername);
 
   if (x_strcmp(lpTargetNode,ResourceString((UINT)IDS_I_LOCALNODE)) == 0)
       wsprintf(szTxt, "repcat %s -u%s", lpTargetDb,tmpusername);
   else
       wsprintf(szTxt, "repcat %s::%s -u", lpTargetNode,lpTargetDb,tmpusername);

   //
   // Installing Replication objects for %s::%s
   wsprintf(
       szTxt2, 
       ResourceString ((UINT)IDS_I_INSTALL_REPLOBJ),
       lpTargetNode,
       lpTargetDb);

   execrmcmd1(lpTargetNode, szTxt, szTxt2, FALSE);

   return RES_SUCCESS; 
   }

static int AddDbIntoInstallation(lpTargetNode)
exec sql begin declare section;
unsigned char *lpTargetNode;
exec sql end declare section;

   {
   exec sql begin declare section;
   int iRowCount;
   exec sql end declare section;
   long lSqlErr;

	exec sql repeated select count(*) into :iRowCount from dd_installation;
	if (sqlca.sqlcode)
      return RES_ERR;

	if (iRowCount==0)
		{
		exec sql insert into dd_installation
			 (database_no,node_name,target_type)
			 select database_no,node_name,target_type
				from dd_connections
				where database_name=dbmsinfo('database') and
				      node_name=:lpTargetNode;
      lSqlErr=sqlca.sqlcode;
		exec sql inquire_sql (:iRowCount='rowcount');
		if (sqlca.sqlcode || iRowCount!=1)
			return RES_ERR;
		}
   return RES_SUCCESS;
   }
static int ProcessRegisteredTables(lpSourceNode, lpSourceDb, 
                                   lpTargetNode, lpTargetDb)
exec sql begin declare section;
unsigned char *lpSourceNode;
unsigned char *lpSourceDb;
unsigned char *lpTargetNode;
unsigned char *lpTargetDb;
exec sql end declare section;
{
   exec sql begin declare section;
	   unsigned char table_name[MAXOBJECTNAME];
     unsigned char table_owner[MAXOBJECTNAME]; 
	   unsigned char tables_created[2];
     unsigned char rules_created[2];
     int table_no;
     int target_type;
	   int	last = 0;
	   int	err = 0;
     short tnull, rnull;
   exec sql end declare section;

   int iret, iTargetSess, iSourceSess;
   UCHAR szConnectName[MAXOBJECTNAME*2+2];

   wsprintf(szConnectName,"%s::%s",lpSourceNode, lpSourceDb);
   iret = Getsession(szConnectName,SESSION_TYPE_INDEPENDENT, &iSourceSess);

   if (iret!=RES_SUCCESS)
      return iret;

   wsprintf(szConnectName,"%s::%s",lpTargetNode, lpTargetDb);
   iret = Getsession(szConnectName,SESSION_TYPE_INDEPENDENT, &iTargetSess);

   if (iret!=RES_SUCCESS) {
      ReleaseSession(iSourceSess, RELEASE_ROLLBACK);
      return iret;
   }

   iret=ActivateSession(iSourceSess);
   if (iret!=RES_SUCCESS) {
      ReleaseSession(iSourceSess, RELEASE_ROLLBACK);
      ReleaseSession(iTargetSess, RELEASE_ROLLBACK);
      return iret;
   }


	exec sql repeated select target_type
      into :target_type
	   from dd_connections
	   where database_name = :lpTargetDb
	   and node_name = :lpTargetNode;

   if (sqlca.sqlcode==0) {
      exec sql commit;
	    exec sql declare cloc cursor for
		     select tablename,dbmsinfo('dba'),table_no,tables_created,rules_created
			         from dd_registered_tables;
      if (sqlca.sqlcode!=0)
         iret=RES_ERR;
      else {
         // to be finished, table owner has to be picked up from dd_registered 
         // table when possible, instead of being forced by dbmsinfo('dba') in the
         // following select.

	       exec sql open cloc for readonly;
         for (last=0;!last;) {
		        exec sql fetch cloc
			               into   :table_name,          :table_owner,     :table_no,
			                      :tables_created:tnull,:rules_created:rnull ;
		        exec sql inquire_sql (:err='errorno', :last='endquery');
            if (err) {
               iret=RES_ERR;
               break;
            }
            if (last) {
              iret=RES_SUCCESS;
              break;
            }
            iret=ActivateSession(iTargetSess);
            if (iret!=RES_SUCCESS)
               break;
		        if (tnull!=-1) {
		           if (*tables_created=='T' || *tables_created=='t') {
			           iret=CreateSupportTables(lpTargetNode, lpTargetDb,
                                       table_name, table_owner, target_type);
			           if (iret=RES_ERR)
                    break;
			         }
            }
		        if (rnull==-1) {
		           if (*rules_created=='R' || *rules_created=='r') {
			            iret = CreateReplicationRules(lpSourceNode, lpTargetNode,
                                                  lpSourceDb,   lpTargetDb,  
                                                  table_name,   table_owner, 
                                                  iSourceSess,  iTargetSess);
			            if (iret=RES_ERR)
                     break;
			         }
            }
            iret=ActivateSession(iSourceSess);
            if (iret!=RES_SUCCESS)
               break;
         }
      }
   }
   else
      iret=RES_ERR;
   if (iret==RES_SUCCESS) {
      iret=ReleaseSession(iSourceSess, RELEASE_COMMIT);
      iret=ReleaseSession(iTargetSess, RELEASE_COMMIT);
   }
   else {
      ReleaseSession(iSourceSess, RELEASE_ROLLBACK);
      ReleaseSession(iTargetSess, RELEASE_ROLLBACK);
   }
   return iret;
}

static int CreateSupportTables(lpTargetNode, lpTargetDb, table_name, 
               table_owner, target_type)
unsigned char *lpTargetNode;
unsigned char *lpTargetDb;
exec sql begin declare section; 
unsigned char *table_name;
unsigned char *table_owner;
int target_type;
exec sql end declare section;

   {
   exec sql begin declare section;
   unsigned char szShadowTableName[MAXOBJECTNAME];
   unsigned char szArchiveTableName[MAXOBJECTNAME];
   unsigned char column_name[MAXOBJECTNAME];
   int iRowCount, iret;
   exec sql end declare section;
   int freebytes, ilocworksession;
   int iTableExists;
   LPUCHAR lpSqlStm;
   TABLEPARAMS TablePrm;

   iret=TableExists(table_name, table_owner, &iTableExists);
   if (iret!=RES_SUCCESS)
      return iret;

   if (!iTableExists)   
      return RES_ERR;

   wsprintf (szShadowTableName, "%s_s", table_name);

   iret=TableExists(szShadowTableName, table_owner, &iTableExists);
   if (iret!=RES_SUCCESS)
      return iret;

   if (!iTableExists)   // Shadow table doesn't exist
      {

      lpSqlStm=ESL_AllocMem(SEGMENT_TEXT_SIZE+1);
      if (!lpSqlStm)
         return RES_ERR;
      freebytes=SEGMENT_TEXT_SIZE;
      lpSqlStm=ConcatStr(lpSqlStm, "create table ", &freebytes);
      lpSqlStm=ConcatStr(lpSqlStm, szShadowTableName, &freebytes);
      lpSqlStm=ConcatStr(lpSqlStm, " (", &freebytes);
      exec sql repeated select column_name 
               into   :column_name 
               from   dd_registered_columns
               where  tablename=table_name
               and   key_column='K';
      exec sql begin;
         suppspace(column_name);
         lpSqlStm=ConcatStr(lpSqlStm, column_name, &freebytes);
         lpSqlStm=ConcatStr(lpSqlStm, ",", &freebytes);
      exec sql end;

      lpSqlStm=ConcatStr(lpSqlStm, "database_no smallint not null,"
	                     "transaction_id integer not null,"
	                     "sequence_no integer not null,"
	                     "trans_time date with null,"
	                     "distribution_time date with null,"
	                     "in_archive integer1 not null with default,"
	                     "dd_routing smallint not null with default,"
	                     "trans_type smallint not null not default,"
	                     "dd_priority smallint not null with default,"
	                     "new_key smallint not null with default,"
	                     "old_database_no smallint not null with default,"
	                     "old_transaction_id integer not null with default,"
	                     "old_sequence_no integer not null with default)"
                        " with journaling", &freebytes);

      if (!lpSqlStm)
         return RES_ERR;

      iret=ExecSQLImm(lpSqlStm,FALSE, NULL, NULL, NULL);
      ESL_FreeMem(lpSqlStm);

      if (iret!=RES_SUCCESS)
         return iret;

      lpSqlStm=ESL_AllocMem(SEGMENT_TEXT_SIZE+1);
      if (!lpSqlStm)
         return RES_ERR;
      freebytes=SEGMENT_TEXT_SIZE;

      wsprintf (lpSqlStm, "modify %s to btree on "
                        "transaction_id, database_no, sequence_no",
                        szShadowTableName);

      iret=ExecSQLImm(lpSqlStm,FALSE, NULL, NULL, NULL);
      ESL_FreeMem(lpSqlStm);

      if (iret==RES_ENDOFDATA)
         iret=RES_SUCCESS;    // table is empty
      if (iret!=RES_SUCCESS)
         return iret;

      lpSqlStm=ESL_AllocMem(SEGMENT_TEXT_SIZE+1);
      if (!lpSqlStm)
         return RES_ERR;
      freebytes=SEGMENT_TEXT_SIZE;

      lpSqlStm=ConcatStr(lpSqlStm, "create index on ", &freebytes);
      lpSqlStm=ConcatStr(lpSqlStm, szShadowTableName, &freebytes);
      lpSqlStm=ConcatStr(lpSqlStm, " (", &freebytes);
      exec sql repeated select column_name 
               into   :column_name 
               from   dd_registered_columns
               where  tablename=table_name
               and   key_column='K';
      exec sql begin;
         suppspace(column_name);
         lpSqlStm=ConcatStr(lpSqlStm, column_name, &freebytes);
         lpSqlStm=ConcatStr(lpSqlStm, ",", &freebytes);
      exec sql end;

      lpSqlStm=ConcatStr(lpSqlStm, ") with structure = btree", &freebytes);

      if (!lpSqlStm)
         return RES_ERR;

      iret=ExecSQLImm(lpSqlStm,FALSE, NULL, NULL, NULL);
      ESL_FreeMem(lpSqlStm);

      if (iret!=RES_SUCCESS)
         return iret;

	   exec sql insert into dd_support_table (table_name)
			values (:szShadowTableName);

      if  (sqlca.sqlcode)
         return RES_ERR;
      }

	if (target_type==1)  // full peer needs an archive table
   	{
		wsprintf(szArchiveTableName, "%s_a", table_name);
      iret=TableExists(szArchiveTableName, table_owner, &iTableExists);
      if (iret!=RES_SUCCESS)
         return iret;

      if (!iTableExists)   // Archive table doesn't exist, create it
		   {
            // Create the archive table
            ZEROINIT(TablePrm);
            x_strcpy(TablePrm.szNodeName,lpTargetNode); 
            x_strcpy(TablePrm.DBName,    lpTargetDb); 
            x_strcpy(TablePrm.objectname,table_name); 
            x_strcpy(TablePrm.szSchema,  table_owner); 
            iret=GetDetailInfo(lpTargetNode,OT_TABLE, (void *) &TablePrm,
                     FALSE, &ilocworksession);

            if (iret!=RES_SUCCESS)
               {
                 FreeAttachedPointers(&TablePrm, OT_TABLE);
                 return iret;
               }

                         // add extra columns
            if (!AddCol2List(&(TablePrm.lpColumns),"database_no"   ,INGTYPE_INT2,FALSE,FALSE) ||
                !AddCol2List(&(TablePrm.lpColumns),"transaction_id",INGTYPE_INT4,FALSE,FALSE) ||
                !AddCol2List(&(TablePrm.lpColumns),"sequence_no"   ,INGTYPE_INT4,FALSE,FALSE) 
               )
                 {
                   FreeAttachedPointers(&TablePrm, OT_TABLE);
                   return RES_ERR; 
                 }

            x_strcpy(TablePrm.objectname, szArchiveTableName); 
            TablePrm.bJournaling = TRUE;

            iret=DBAAddObject (lpTargetNode, OT_TABLE,(void *) &TablePrm);
            FreeAttachedPointers(&TablePrm, OT_TABLE);

            if (iret!=RES_SUCCESS)
               return iret;

            // Add the archive table into the replicated database set

				exec sql insert into dd_replicated_tables (tablename)
					values (:szArchiveTableName);
         	exec sql inquire_sql (:iret='errorno',:iRowCount='rowcount');
				if (iret!=0 || iRowCount != 1)
               return RES_ERR;
            // Modify the archive table storage structure

            lpSqlStm=ESL_AllocMem(SEGMENT_TEXT_SIZE+1);
            if (!lpSqlStm)
               return RES_ERR;
            freebytes=SEGMENT_TEXT_SIZE;
            lpSqlStm=ConcatStr(lpSqlStm,"modify ", &freebytes);
            lpSqlStm=ConcatStr(lpSqlStm,szArchiveTableName, &freebytes);
            lpSqlStm=ConcatStr(lpSqlStm,"to btree on transaction_id,"
                                        "database_no, sequence_no",
                               &freebytes);
            if (!lpSqlStm)
               return RES_ERR;

            iret=ExecSQLImm(lpSqlStm,FALSE, NULL, NULL, NULL);
            ESL_FreeMem(lpSqlStm);

            if (iret==RES_ENDOFDATA)
              iret=RES_SUCCESS;

            if (iret!=RES_SUCCESS)
               return iret;
		   }
	   }
   return iret;
   }
static int CreateReplicationRules(LPUCHAR lpSourceNode, LPUCHAR lpTargetNode,
                                  LPUCHAR lpSourceDb, LPUCHAR lpTargetDb,  
                                  LPUCHAR tablename, LPUCHAR table_owner,
                                  int iSourceSession,int iTargetSession)
   {
   exec sql begin declare section;
   unsigned char *table_name=tablename;
   exec sql end declare section;

   typedef struct _DDProcsAndRules 
      {
      int objtype;           // OT_PROC or OT_RULE
      LPUCHAR lpNameFormat;  // "%s__xx" used with sprintf
      } 
      DDPROCANDRULES, FAR *LPDDPROCANDRULES;
   void *lpObjStruct ;
   PROCEDUREPARAMS ProcPrm;
   RULEPARAMS RulePrm;

   UCHAR szObjName[MAXOBJECTNAME];
   int iret, ilocworksession;

   static DDPROCANDRULES DDProcAndRules[]=
      {{OT_PROCEDURE, "%s__i2"},
       {OT_PROCEDURE, "%s__u2"},
       {OT_PROCEDURE, "%s__d2"},
       {OT_PROCEDURE, "%s__i" },
       {OT_RULE,      "%s__1" },
       {OT_PROCEDURE, "%s__u" },
       {OT_PROCEDURE, "%s__d" },
       {OT_RULE,      "%s__2" },
       {OT_RULE,      "%s__3" },
       {OT_RULE,      NULL}};
   LPDDPROCANDRULES lpDDProcAndRule=DDProcAndRules;

   while (lpDDProcAndRule->lpNameFormat) 
      {
      wsprintf(szObjName, lpDDProcAndRule->lpNameFormat, table_name);

      if (lpDDProcAndRule->objtype==OT_PROCEDURE)
         {
         x_strcpy (ProcPrm.objectname,  szObjName);
         x_strcpy (ProcPrm.objectowner, table_owner);
         x_strcpy (ProcPrm.DBName,      lpSourceDb);
         lpObjStruct=(void *) &ProcPrm;
         }
      else
         {
         x_strcpy (RulePrm.DBName,      lpSourceDb);
         x_strcpy (RulePrm.RuleName,    szObjName);
         x_strcpy (RulePrm.RuleOwner,   table_owner);
         x_strcpy (RulePrm.TableName,   table_name);
         x_strcpy (RulePrm.TableOwner,  table_owner);
         lpObjStruct=(void *) &RulePrm;
         }


      iret=GetDetailInfo(lpSourceNode,lpDDProcAndRule->objtype, 
               lpObjStruct, FALSE, &ilocworksession);

      if (iret!=RES_SUCCESS)
         {
           FreeAttachedPointers(lpObjStruct, lpDDProcAndRule->objtype);
           return iret;
         }

      if (lpDDProcAndRule->objtype==OT_PROCEDURE)
         x_strcpy (ProcPrm.DBName, lpTargetDb);
      else
         x_strcpy (RulePrm.DBName, lpTargetDb);

      iret=DBAAddObjectLL (NULL, lpDDProcAndRule->objtype, lpObjStruct,
                        iTargetSession);

      FreeAttachedPointers(lpObjStruct, lpDDProcAndRule->objtype);
      if (iret!=RES_SUCCESS)
         return iret;
      }

   iret=ActivateSession(iTargetSession);
   if (iret!=RES_SUCCESS)
      return iret;

	 exec sql update dd_registered_tables
	          set rules_created = 'R'
	          where tablename = :table_name;
   if (sqlca.sqlcode)
      return RES_ERR;
   return RES_SUCCESS;
   }

static int TableExists(table_name, table_owner, lpRowCount)
exec sql begin declare section;
unsigned char *table_name;
unsigned char *table_owner;
int *lpRowCount;
exec sql end declare section;
	{
	exec sql begin declare section;
	int iret=0;
	exec sql end declare section;

	int iRowCount=0;

	exec sql repeated select count(*) 
		 into :iRowCount
		 from iitables
		 where table_name=:table_name and
		       table_owner=:table_owner;
	exec sql inquire_sql(:iret='errorno');
	if (iret<0)
		return RES_ERR;

   *lpRowCount=iRowCount;

	return RES_SUCCESS;
	}

/*****************************************************************************
   Return   RES_SQLNOEXEC  if Getsession or Releasesession returns an error.
   Value    RES_SUCCESS    if localdb exist.
            RES_ERR        if localdb does not exist.
            RES_NOGRANT    no grant.
******************************************************************************/
int ReplicatorLocalDbInstalled(int hnode,LPUCHAR lpDbName ,int ReplicVersion)
{
   UCHAR szConnectName[2*MAXOBJECTNAME+2];
   int iSession,ires,iret;
	 int iRowCount=0;
   
   iret = RES_ERR;

   wsprintf(szConnectName,"%s::%s",GetVirtNodeName(hnode), lpDbName);
   ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
   if (ires!=RES_SUCCESS)
      return RES_SQLNOEXEC;
   if (ReplicVersion == REPLIC_V11)
	    exec sql repeated select count(*) into :iRowCount from dd_databases where local_db=1;
   else
	    exec sql repeated select count(*) into :iRowCount from dd_installation;

   ManageSqlError(&sqlca); // Keep track of the SQL error if any

   if (sqlca.sqlcode == -34000L)
      iret = RES_NOGRANT;

   ires = ReleaseSession(iSession, RELEASE_COMMIT);
   if (ires != RES_SUCCESS)
      return RES_SQLNOEXEC;
   
   if ( iRowCount == 1 )
	    iret = RES_SUCCESS;
   
   return iret;
}




int ReplMonGetQueues(int hnode,LPUCHAR lpDBName, int * pinput, int * pdistrib)
{
   UCHAR szConnectName[2*MAXOBJECTNAME+2];
   int iSession,ires,iret;
   int iRowCount;
  
   iret = RES_SUCCESS;

   wsprintf(szConnectName,"%s::%s",GetVirtNodeName(hnode), lpDBName);
   ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
   if (ires!=RES_SUCCESS)
      return ires;

   iRowCount=-1;
   exec sql repeated select count(*) into :iRowCount from dd_input_queue;
   ManageSqlError(&sqlca); // Keep track of the SQL error if any
   if (iRowCount>=0)
      *pinput=iRowCount;
   else
      iret=RES_ERR;

   iRowCount=-1;
   exec sql repeated select count(*) into :iRowCount from dd_distrib_queue;
   ManageSqlError(&sqlca); // Keep track of the SQL error if any
   if (iRowCount>=0)
      *pdistrib=iRowCount;
   else
      iret=RES_ERR;

   ires = ReleaseSession(iSession, RELEASE_COMMIT);
   if (ires != RES_SUCCESS)
      iret=RES_ERR;
   
   return iret;
}


/*
** Name:    sendEvent - raise a dbevent
**
** Description:
**  Raises dbevent to notify server(s) about changed startup flag.  If
**  svr_no is empty, sends an event to all servers.
**
** Inputs:
**  evt_name    - event name
**  flag_name   - server parameter
**  svr_no      - server number
**
** Outputs:
**  none
**
** Returns:
**  RES_SUCCESS ou RES_ERR
*/
int SendEvent(char *vnode, char *dbname,char *evt_name,char *flag_name,char *svr_no)
{
    char evt_name2[MAXOBJECTNAME];
    UCHAR extradata[EXTRADATASIZE];
    LPUCHAR parentstrings [MAXPLEVEL];
    UCHAR DBAUsernameOntarget[MAXOBJECTNAME];
    char buf[MAXOBJECTNAME];
    int iSession,ires,hdl,iret,irestype;
    UCHAR szConnectName[2*MAXOBJECTNAME+2];
    UCHAR szVnodeName[2*MAXOBJECTNAME+2];
    UCHAR szMess[2*MAXOBJECTNAME+2];
    EXEC SQL BEGIN DECLARE SECTION;
    char    stmt[MAXOBJECTNAME*4];
    EXEC SQL END DECLARE SECTION;
    iret = RES_SUCCESS;

    // Get Database Owner
    hdl  = OpenNodeStruct (vnode);
    if (hdl == -1) {
        //"The maximum number of connections has been reached"
        //" - Cannot send event on %s::%s"
        wsprintf(szMess, ResourceString(IDS_F_MAX_CONN_SEND_EVENT),vnode, dbname);
        TS_MessageBox(TS_GetFocus(),szMess,NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
        return RES_SQLNOEXEC;
    }
    // Temporary for activate a session
    ires = DOMGetFirstObject (hdl, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
    parentstrings [0] = dbname;
    parentstrings [1] = NULL;
    iret = DOMGetObjectLimitedInfo( hdl,
                                    parentstrings [0],
                                    (UCHAR *)"",
                                    OT_DATABASE,
                                    0,
                                    parentstrings,
                                    TRUE,
                                    &irestype,
                                    buf,
                                    DBAUsernameOntarget,
                                    extradata
                                   );
    if (iret != RES_SUCCESS) {
        CloseNodeStruct(hdl,FALSE);
        //"DBA owner on database : %s not found.  - Cannot send event on %s::%s."
        wsprintf(buf,ResourceString(IDS_F_DBA_OWNER_EVENT),parentstrings[0],vnode,parentstrings[0]);
        TS_MessageBox(TS_GetFocus(),buf, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
        return 0L;
    }
    CloseNodeStruct (hdl, FALSE);
    wsprintf(szVnodeName,"%s%s%s%s",vnode, LPUSERPREFIXINNODENAME, DBAUsernameOntarget, LPUSERSUFFIXINNODENAME);
    hdl  = OpenNodeStruct (szVnodeName);
    if (hdl == -1) {
        wsprintf(szMess,ResourceString(IDS_F_MAX_CONN_SEND_EVENT),vnode, dbname);
        TS_MessageBox(TS_GetFocus(),szMess,NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
        return RES_SQLNOEXEC;
    }

    wsprintf(szConnectName,"%s::%s",szVnodeName,dbname);
    ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
    if (ires!=RES_SUCCESS)  {
        CloseNodeStruct (hdl, FALSE);
        return RES_ERR;
    }

    if (*svr_no == '\0')
        x_strcpy( evt_name2,evt_name );
    else
        wsprintf(evt_name2, "%s%s", evt_name, svr_no);
    if (flag_name != NULL && *flag_name != '\0')
        wsprintf(stmt, "RAISE DBEVENT %s '%s'", evt_name2, flag_name);
    else
        wsprintf(stmt, "RAISE DBEVENT %s", evt_name2);
    EXEC SQL EXECUTE IMMEDIATE :stmt;
    // Don't used ManageSqlError because raise event return WARNING 710.
    // ManageSqlError(&sqlca); // Keep track of the SQL error if any
    if (sqlca.sqlcode < 0) {
        ManageSqlError(&sqlca);
        iret = RES_ERR;
    }

    ires = ReleaseSession(iSession, RELEASE_COMMIT);
    CloseNodeStruct (hdl, FALSE);

    if (ires != RES_SUCCESS)
        return RES_ERR;

    return iret;
}


int MonitorReplicatorStopSvr ( int hdl, int svrNo ,LPUCHAR lpDbName  )
{
    EXEC SQL BEGIN DECLARE SECTION;
        char szStmt[MAXOBJECTNAME];
    EXEC SQL END DECLARE SECTION;
    UCHAR szConnectName[2*MAXOBJECTNAME+2];
    int ires,iSession;

    wsprintf(szConnectName,"%s::%s",GetVirtNodeName(hdl), lpDbName);
    ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
    if (ires!=RES_SUCCESS)
        return RES_ERR;

    wsprintf(szStmt,"RAISE DBEVENT dd_stop_server%d",svrNo);
    EXEC SQL EXECUTE IMMEDIATE :szStmt;
	// Don't used ManageSqlError because raise event return WARNING 710.
	// ManageSqlError(&sqlca);
    EXEC SQL COMMIT;
    ires = ReleaseSession(iSession,RELEASE_COMMIT);
    return ires;

}

int GetCddsNumInLookupTblAndFillCtrl(int hdl, LPUCHAR lpDbName, LPUCHAR cdds_lookup_tbl,HWND hWndComboCdds)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	stmt[MAXOBJECTNAME*4];
	char	cdds_name[MAXOBJECTNAME];
	long	cdds_no;
	EXEC SQL END DECLARE SECTION;
	UCHAR szConnectName[2*MAXOBJECTNAME];
	int ires,iSession,index ;

	ComboBox_ResetContent(hWndComboCdds);
	wsprintf(szConnectName,"%s::%s",GetVirtNodeName(hdl), lpDbName);
	ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
	if (ires!=RES_SUCCESS)
		return RES_ERR;

	wsprintf(stmt,
		"SELECT DISTINCT d.cdds_no, d.cdds_name "
		"FROM %s t, dd_cdds d "
		"WHERE t.cdds_no = d.cdds_no ORDER BY cdds_no",
		cdds_lookup_tbl);
	EXEC SQL EXECUTE IMMEDIATE :stmt
		INTO	:cdds_no, :cdds_name;
	EXEC SQL BEGIN;
	    ManageSqlError(&sqlca);
		suppspace(cdds_name);
		wsprintf (stmt,"%d %s",cdds_no,cdds_name);
		index = ComboBox_AddString(hWndComboCdds,stmt);
		if (index != CB_ERR)
			ComboBox_SetItemData(hWndComboCdds,index,cdds_no);
	EXEC SQL END;
	EXEC SQL COMMIT;
	ires = ReleaseSession(iSession,RELEASE_COMMIT);
	
	return ires;
}

int GetVnodeDatabaseAndFillCtrl(int hdl, LPUCHAR lpDbName,HWND hWndCAlistBox)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_no;
	char vnode_name[MAXOBJECTNAME];
	char db_name[MAXOBJECTNAME*4];
	EXEC SQL END DECLARE SECTION;
	UCHAR szConnectName[2*MAXOBJECTNAME];
	char full_db_name[(MAXOBJECTNAME*2)+3]; // 3 for db_no space.
	int ires,iSession,index ;

	CAListBox_ResetContent(hWndCAlistBox);


	wsprintf(szConnectName,"%s::%s",GetVirtNodeName(hdl), lpDbName);
	ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
	if (ires!=RES_SUCCESS)
		return RES_ERR;


	EXEC SQL REPEATED SELECT DISTINCT d.database_no, database_name, vnode_name
		INTO	:db_no, :db_name, :vnode_name
		FROM	dd_databases d, dd_db_cdds c
		WHERE	d.local_db = 0
		AND d.database_no = c.database_no
		AND c.target_type IN (1, 2)
		AND d.vnode_name != 'mobile';
	EXEC SQL BEGIN;
	    ManageSqlError(&sqlca);
		suppspace(db_name);
		suppspace(vnode_name);
		wsprintf(full_db_name, "%d %s::%s",db_no, vnode_name, db_name);
		index=CAListBox_AddString(hWndCAlistBox, full_db_name );
		if (index != CALB_ERR && index != CALB_ERRSPACE)
			CAListBox_SetItemData(hWndCAlistBox,index,db_no);

	EXEC SQL END;
	EXEC SQL COMMIT;
	ires = ReleaseSession(iSession,RELEASE_COMMIT);
	
	return ires;

}

int GetCddsName (int hdl ,LPUCHAR lpDbName, int cdds_num ,LPUCHAR lpcddsname)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	stmt[MAXOBJECTNAME*4];
	char	cdds_name[MAXOBJECTNAME];
	EXEC SQL END DECLARE SECTION;
	UCHAR szConnectName[2*MAXOBJECTNAME];
	int ires,iSession;

	wsprintf(szConnectName,"%s::%s",GetVirtNodeName(hdl), lpDbName);
	ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
	if (ires!=RES_SUCCESS)
		return RES_ERR;

	wsprintf(stmt,
		"SELECT DISTINCT cdds_name "
		"FROM dd_cdds "
		"WHERE cdds_no = %d",cdds_num);
	EXEC SQL EXECUTE IMMEDIATE :stmt
		INTO	:cdds_name;
	EXEC SQL BEGIN;
	    ManageSqlError(&sqlca);
		suppspace(cdds_name);
		lstrcpy(lpcddsname, cdds_name);
	EXEC SQL END;
	EXEC SQL COMMIT;
	ires = ReleaseSession(iSession,RELEASE_COMMIT);

	return ires;

}

int FilledSpecialRunNode( long hdl,  LPRESOURCEDATAMIN pResDtaMin,
						  LPREPLICSERVERDATAMIN pReplSvrDtaBeforeRunNode,
						  LPREPLICSERVERDATAMIN pReplSvrDtaAfterRunNode)
{
	int ires,iSession;
	char	stmt[MAXOBJECTNAME*4];
	BOOL bStatementNecessary;
	UCHAR szConnectName[2*MAXOBJECTNAME];
	REPLICSERVERDATAMIN ReplSvrDta;
	wsprintf(szConnectName,"%s::%s",GetVirtNodeName(hdl),
									pReplSvrDtaBeforeRunNode->ParentDataBaseName);
	ires = Getsession(szConnectName,SESSION_TYPE_INDEPENDENT, &iSession);
	if (ires!=RES_SUCCESS)
		return RES_ERR;

	wsprintf(stmt, "DELETE FROM dd_server_special");
	EXEC SQL EXECUTE IMMEDIATE :stmt;
	if (sqlca.sqlcode != 0 && sqlca.sqlcode != 100) {
		ManageSqlError(&sqlca);
		ReleaseSession(iSession,RELEASE_ROLLBACK);
		return RES_ERR;
	}
	
	ires=GetFirstMonInfo(hdl,OT_DATABASE,pResDtaMin,
						 OT_MON_REPLIC_SERVER,(void * )&ReplSvrDta,NULL);
	while (ires==RES_SUCCESS) {
		bStatementNecessary = TRUE;
		if (lstrcmp((char *)ReplSvrDta.RunNode,(char *)pReplSvrDtaBeforeRunNode->RunNode) == 0 &&
			ReplSvrDta.localdb == pReplSvrDtaBeforeRunNode->localdb &&
			ReplSvrDta.serverno == pReplSvrDtaBeforeRunNode->serverno ) {
			if (lstrcmp((char *)pReplSvrDtaAfterRunNode->RunNode,(char *)pReplSvrDtaAfterRunNode->LocalDBNode) != 0 )
				wsprintf(stmt, "INSERT INTO dd_server_special (localdb, server_no, vnode_name)"
							   " VALUES (%d, %d, '%s')",pReplSvrDtaAfterRunNode->localdb,
													  pReplSvrDtaAfterRunNode->serverno,
													  pReplSvrDtaAfterRunNode->RunNode);
			else
				bStatementNecessary = FALSE;
		}
		else {
			if (lstrcmp((char *)ReplSvrDta.LocalDBNode,(char *)ReplSvrDta.RunNode) != 0)
				wsprintf(stmt, "INSERT INTO dd_server_special( localdb, server_no, vnode_name)"
							   " VALUES (%d, %d, '%s')",ReplSvrDta.localdb, ReplSvrDta.serverno,
													ReplSvrDta.RunNode);
			else
				bStatementNecessary = FALSE;
		}
		if (bStatementNecessary) {
			ActivateSession(iSession);
			EXEC SQL EXECUTE IMMEDIATE :stmt;
			if (sqlca.sqlcode != 0) {
				ManageSqlError(&sqlca);
				ReleaseSession(iSession,RELEASE_ROLLBACK);
				return RES_ERR;
			}
		}
		ires=GetNextMonInfo((void * )&ReplSvrDta);
	}

	ires = ReleaseSession(iSession,RELEASE_COMMIT);

	return ires;
}

int GenerateSql4changeAccessDB( BOOL bDBPublic ,LPUCHAR DBname,LPUCHAR VnodeName)
{
	EXEC SQL BEGIN DECLARE SECTION;
		long		bit_n = DB_GLOBAL;
		long		bit; /* specifies default access DB_PRIVATE */
		char	sbuffer[256];
		char	db_name[MAXOBJECTNAME];
	EXEC SQL END DECLARE SECTION;
	UCHAR szConnectName[2*MAXOBJECTNAME];
	int ires,iSession;

	wsprintf(szConnectName,"%s::iidbdb",VnodeName);
	ires = Getsession(szConnectName,SESSION_TYPE_INDEPENDENT, &iSession);
	if (ires!=RES_SUCCESS)
		return RES_ERR;

	wsprintf(sbuffer, "SET SESSION WITH PRIVILEGES=ALL");
	EXEC SQL EXECUTE IMMEDIATE :sbuffer;

	bit = ( bDBPublic ? DB_GLOBAL : DB_PRIVATE);

	if (bit == DB_PRIVATE) 
	{
		wsprintf(sbuffer, "GRANT NOACCESS ON DATABASE %s TO PUBLIC", DBname);
	}
	else
	{
		wsprintf(sbuffer, "GRANT ACCESS ON DATABASE %s TO PUBLIC", DBname);
	}
	EXEC SQL EXECUTE IMMEDIATE :sbuffer;
	if (sqlca.sqlcode != 0) {
		ManageSqlError(&sqlca);
		ReleaseSession(iSession,RELEASE_ROLLBACK);
		return RES_ERR;
	}

	x_strcpy(db_name,DBname);
	EXEC SQL UPDATE iidatabase i SET access = ((i.access / (2 * :bit_n)) * 2 * :bit_n)
												+ MOD(i.access, :bit_n) + :bit
			WHERE i.name = :db_name;
	if (sqlca.sqlcode != 0) {
		ManageSqlError(&sqlca);
		ReleaseSession(iSession,RELEASE_ROLLBACK);
		return RES_ERR;
	}
	ReleaseSession(iSession,RELEASE_COMMIT);
	return ires;
}

int	ExecuteProcedureLocExtend(pProcedure)
	EXEC SQL BEGIN DECLARE SECTION;
	char *pProcedure;
	EXEC SQL END DECLARE SECTION;
{
	EXEC SQL SET SESSION WITH PRIVILEGES=ALL;

	EXEC SQL EXECUTE IMMEDIATE :pProcedure;

	if (sqlca.sqlcode < 0) {
		ManageSqlError(&sqlca);
		return RES_ERR;
	}
	return RES_SUCCESS;
}

int GetReplicatorTableNumber (int hdl ,LPUCHAR lpDbName,
					LPUCHAR lpTableName,LPUCHAR lpTableOwner, int *nTableNum)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	stmt[MAXOBJECTNAME*4];
	int		Tbl_num;
	EXEC SQL END DECLARE SECTION;
	UCHAR szConnectName[MAXOBJECTNAME*2];
	int ires,iret,iSession;

	wsprintf(szConnectName,"%s::%s",GetVirtNodeName(hdl), lpDbName);
	ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
	if (ires!=RES_SUCCESS)
		return RES_ERR;

	wsprintf(stmt,
		"SELECT table_no "
		"FROM dd_regist_tables "
		"WHERE table_name = '%s' "
		"AND table_owner = '%s'",StringWithoutOwner(lpTableName),lpTableOwner);
	EXEC SQL EXECUTE IMMEDIATE :stmt
		INTO	:Tbl_num;
	if (sqlca.sqlcode < 0)
	{
		ManageSqlError(&sqlca);
		*nTableNum =-1;
		iret = RES_ERR;
		ReleaseSession(iSession,RELEASE_ROLLBACK);
	}
	else
	{
		iret = RES_SUCCESS;
		*nTableNum = Tbl_num;
		ReleaseSession(iSession,RELEASE_COMMIT);
	}
	return iret;
}

