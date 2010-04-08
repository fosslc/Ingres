/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**  Project : Visual DBA
**
**  Author : Schalk Philippe
**
**  Name:    collisio.sc - build list of collisions
**
** Description:
**  Defines                                                             | Created based on      | Fonction Name
**                                                                      ----------------------------------------------
**  VdbaColl_Queue_Collision     - Queue collision report               |REPMGR\COLLRPT.SC      | queue_collision()
**  VdbaColl_Get_Collisions      - build list of collisions             |REPMGR\COLLLIST.SC     | get_collisions(...)
**  VdbaColl_Check_Insert        - check for insert collisions          |REPMGR\COLLLIST.SC     | check_insert()
**  VdbaColl_Check_Update_Delete - check for update/delete collisions   |REPMGR\COLLLIST.SC     | check_update_delete()
**  VdbaColl_tbl_fetch           - fetch table information              |REPMGR\TBLFETCH.SC     | RMtbl_fetch
**  VdbaColl_Create_Key_Clauses  - create key clauses                   |REPCOMN\CRKEYCLS.SC    | create_key_clauses()
**  VdbaColl_apostrophe          - double apostrophes in a string       |REPCOMN\CRKEYCLS.SC    | apostrophe  
**  Vdba_error_check             - check for database error             |REPCOMN\DBERRCHK.SC    | RPdb_error_check 
**  VdbaColl_edit_name           - edit an Ingres name                  |REPCOMN\DELIMNAM.C     | RPedit_name
**  VdbaColl_tblobj_name         - generate a table object name         |REPCOMN\TBLOBJS.C      | RPtblobj_name()
**  VdbaColl_Init_Sessions       - init  sessions                       |Specific Vdba
**  VdbaColl_Close_Sessions      - close sessions                       |Specific Vdba
**
**  History
**
**   04-Dec-2001 (schph01)
**      extracted and adapted from the winmain\collisio.sc source
**   14-Fev-2002 (schph01)
**      SIR #106648, remove the use of repltbl.sh
*****************************************************************************/

EXEC SQL INCLUDE sqlca;
EXEC SQL INCLUDE sqlda;

#include <sys/stat.h>
#include <tchar.h>

#include "libmon.h"
#include "collisio.h"
#include "replutil.h"
#include "reptbdef.h"
#include "lbmnmtx.h"

EXEC SQL BEGIN DECLARE SECTION;
typedef struct _dbsession
{
	int    connected;                  /* flag for open session */
	short  database_no;                /* database number       */
	char   vnode_name[DB_MAXNAME+1];   /* vnode                 */
	char   database_name[DB_MAXNAME+1];/* database name         */
	char   dba[DB_MAXNAME+1];          /* dba name              */
	char   full_name[DB_MAXNAME*2+3];  /* full connection name  */
	int    iSession;                   /* session number returned by GetSession */
	int    iHnodeStruc;                /* node handle return by OpenNodeStruc   */
} DBSESSION;

typedef struct _globalsession
{
	DBSESSION  *pSession;
	long       NumSessions;
	long       initial_session;
	long       initial_HnodeStruct;
} GLOBALSESSION;

EXEC SQL END DECLARE SECTION;

#define COLLISIOSC
#include "commfunc.h"
#undef COLLISIOSC

typedef struct _globalcollisionlist
{
	COLLISION_FOUND *pGlobCollisionList;
	int nNbCollisionAllocated;
}GLOBALCOLLISIONLIST;

ADF_CB *RMadf_cb = NULL;
FUNC_EXTERN ADF_CB *FEadfcb(void);

#define INCREMENTCOLLISIONLIST 10


static void VdbaColl_apostrophe(char *string);
#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
static long VdbaColl_Get_Collisions(char *localdbname, char *vnode_name,
                                    short localdb,char *local_owner,
                                    long *collision_count, long *records,
                                    LPERRORPARAMETERS lpError, TBLDEF *pRmTbl,
                                    GLOBALSESSION *pGlobSession,
                                    GLOBALCOLLISIONLIST *pColList);
#endif                                    
static int VdbaColl_Close_Sessions(GLOBALSESSION *pCurSession);
static int VdbaColl_Check_Insert( long *collision_count, long localdb, short targetdb,
                                  short sourcedb, long transaction_id, long sequence_no,
                                  long table_no,char *TransTime, long nPriority,
                                  char *local_owner, int nSvrTargetType,
                                  LPERRORPARAMETERS lpError,TBLDEF *pRmTbl,
                                  GLOBALSESSION *pCurSession,GLOBALCOLLISIONLIST *pColList);
static int VdbaColl_Check_Update_Delete(long trans_type, long *collision_count,
                                         long localdb, short targetdb, short old_sourcedb,
                                         long old_transaction_id, long old_sequence_no,
                                         short sourcedb,long transaction_id, long sequence_no
                                         ,char *TransTime, long nPriority, int nSvrTargetType,
                                         LPERRORPARAMETERS lpError,TBLDEF *pRmTbl,
                                         GLOBALSESSION *pTempSession,GLOBALCOLLISIONLIST *pColList);
static long VdbaColl_Create_Key_Clauses(char *where_clause, char *name_clause,
                                        char *insert_value_clause, char *update_clause,
                                        char *table_name, char *table_owner, long table_no, char *sha_name, short database_no,
                                        long transaction_id, long sequence_no,LPERRORPARAMETERS lpError);
#ifndef BLD_MULTI   /*To make available in Non multi-thread context only */                                     
static int VdbaColl_Init_Sessions( char *localdbname, char *vnode_name,
                                   long localdb,LPERRORPARAMETERS lpError,
                                   GLOBALSESSION *pGlobSess);
#endif                                   
static int GetSessionNumber4DbNum(int DbNo ,GLOBALSESSION *pGlobSess);
static COLLISION_FOUND *ReallocCollisionList( int nCurIndice, GLOBALCOLLISIONLIST *pColList);



/*{
** Name:    RPedit_name - edit an Ingres name
**
** Description:
**  Edit an Ingres name as specified.  The edited name is placed in a
**  global buffer.
**
** Inputs:
**  edit_type   - edit type
**            EDNM_ALPHA    - replace special chars with '_'.
**            EDNM_DELIMIT  - Ingres delimited name.
**            EDNM_SLITERAL - SQL single quoted string.
**            EDNM_CLITERAL - C double quoted string.
**            else      - no edit
**  name        - name to edit
**
** Outputs:
**  edited_name - This buffer is filled with the edited name
**                This buffer cannot be NULL
**
** Returns:
**  pointer to the edited name
**
** History:
**	06-Nov-2006 (kiria01) b117042
**	    Conform CMprint macro calls to relaxed bool form
**/
char * VdbaColl_edit_name(i4 edit_type,char *name,char  *edited_name)
{
	char tmp_name[DLNM_SIZE];
	char *t = tmp_name;
	char *n;

	switch (edit_type)
	{
	case EDNM_ALPHA:  /* alphanumeric */
		for (n = name; *n != EOS; CMnext(n))
		{
			if (CMalpha(n) || (n != name && CMdigit(n)))
				CMcpychar(n, t);
			else
				CMcpychar(ERx("_"), t);
			CMnext(t);
		}
		*t = EOS;
		break;

	case EDNM_DELIMIT:  /* delimited */
		CMcpychar(ERx("\""), t);
		CMnext(t);
		for (n = name; *n != EOS; CMnext(n))
		{
			CMcpychar(n, t);
			if (!CMcmpcase(t, ERx("\"")))
			{
				CMnext(t);
				CMcpychar(ERx("\""), t);
			}
			CMnext(t);
		}
		CMcpychar(ERx("\""), t);
		CMnext(t);
		*t = EOS;
		break;

	case EDNM_SLITERAL:  /* SQL quoted */
		CMcpychar(ERx("'"), t);
		CMnext(t);
		for (n = name; *n != EOS; CMnext(n))
		{
			if (!CMcmpcase(n, ERx("'")))
			{
				CMcpychar(ERx("'"), t);
				CMnext(t);
			}
			CMcpychar(n, t);
			CMnext(t);
		}
		CMcpychar(ERx("'"), t);
		CMnext(t);
		*t = EOS;
		break;

	case EDNM_CLITERAL:  /* C quoted */
		CMcpychar(ERx("\""), t);
		CMnext(t);
		for (n = name; *n != EOS; CMnext(n))
		{
			if (!CMcmpcase(n, ERx("\"")))
			{
				CMcpychar(ERx("\\"), t);
				CMnext(t);
			}
			CMcpychar(n, t);
			CMnext(t);
		}
		CMcpychar(ERx("\""), t);
		CMnext(t);
		*t = EOS;
		break;

	default:           /* no edit */
		t = name;
		break;
	}

	STcopy(tmp_name, edited_name);
	return (edited_name);
}

int LIBMON_GetCddsName (int hdl ,char *lpDbName, int cdds_num ,char *lpcddsname)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char  stmt[256];
	char  cdds_name[MAXOBJECTNAME];
	EXEC SQL END DECLARE SECTION;
	UCHAR szConnectName[MAX_CONNECTION_NAME];
	int ires,iSession;

	wsprintf(szConnectName,"%s::%s",GetVirtNodeName(hdl), lpDbName);
	ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
	if (ires!=RES_SUCCESS)
		return RES_E_CONNECTION_FAILED;

	wsprintf(stmt,
	    "SELECT DISTINCT cdds_name "
	    "FROM dd_cdds "
	    "WHERE cdds_no = %d",cdds_num);
	EXEC SQL EXECUTE IMMEDIATE :stmt
	    INTO :cdds_name;
	EXEC SQL BEGIN;
	    ManageSqlError(&sqlca);
	    suppspace(cdds_name);
	    lstrcpy(lpcddsname, cdds_name);
	EXEC SQL END;
	EXEC SQL COMMIT;
	ires = ReleaseSession(iSession,RELEASE_COMMIT);

	return ires;

}

extern mon4struct monInfo[MAX_THREADS];

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
int LIBMON_FilledSpecialRunNode( long hdl,  LPRESOURCEDATAMIN pResDtaMin,
                                 LPREPLICSERVERDATAMIN pReplSvrDtaBeforeRunNode,
                                 LPREPLICSERVERDATAMIN pReplSvrDtaAfterRunNode)
{
	int ires,iSession;
	char    stmt[256];
	BOOL    bStatementNecessary;
	UCHAR   szConnectName[MAX_CONNECTION_NAME];
	REPLICSERVERDATAMIN ReplSvrDta;
	
	int thdl=0; /*Default thdl for non thread functions */
	
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
	
	ires=LIBMON_EmbdGetFirstMonInfo(hdl,OT_DATABASE,pResDtaMin,
	                         OT_MON_REPLIC_SERVER,(void * )&ReplSvrDta,NULL, thdl);
	while (ires==RES_SUCCESS) {
		bStatementNecessary = TRUE;
		if (lstrcmp((char *)ReplSvrDta.RunNode,(char *)pReplSvrDtaBeforeRunNode->RunNode) == 0 &&
			ReplSvrDta.localdb == pReplSvrDtaBeforeRunNode->localdb &&
			ReplSvrDta.serverno == pReplSvrDtaBeforeRunNode->serverno ) {
			if (lstrcmp((char *)pReplSvrDtaAfterRunNode->RunNode,(char *)pReplSvrDtaAfterRunNode->LocalDBNode) != 0 )
				wsprintf(stmt, "INSERT INTO dd_server_special (localdb, server_no, vnode_name)"
				               " VALUES (%d,%d,'%s')",pReplSvrDtaAfterRunNode->localdb,
				                                      pReplSvrDtaAfterRunNode->serverno,
				                                      pReplSvrDtaAfterRunNode->RunNode);
			else
				bStatementNecessary = FALSE;
		}
		else {
			if (lstrcmp((char *)ReplSvrDta.LocalDBNode,(char *)ReplSvrDta.RunNode) != 0)
				wsprintf(stmt, "INSERT INTO dd_server_special( localdb, server_no, vnode_name)"
							   " VALUES (%d,%d,'%s')",ReplSvrDta.localdb, ReplSvrDta.serverno,
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
		ires=LIBMON_GetNextMonInfo(thdl, (void * )&ReplSvrDta);
	}
	
	ires = ReleaseSession(iSession,RELEASE_COMMIT);

	return ires;
}
#endif


int LIBMON_ReplMonGetQueues(int hnode,LPUCHAR lpDBName, int * pinput, int * pdistrib)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int iRowCount;
	EXEC SQL END DECLARE SECTION;
	UCHAR szConnectName[MAX_CONNECTION_NAME];
	int iSession,ires,iret;

	iret = RES_SUCCESS;

	wsprintf(szConnectName,"%s::%s",GetVirtNodeName(hnode), lpDBName);
	ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
	if (ires!=RES_SUCCESS)
		return ires;

	iRowCount=-1;
	EXEC SQL REPEATED SELECT count(*)
	    INTO :iRowCount FROM dd_input_queue;
	ManageSqlError(&sqlca); // Keep track of the SQL error if any
	if (iRowCount>=0)
		*pinput=iRowCount;
	else
		iret=RES_ERR;

	iRowCount=-1;
	EXEC SQL REPEATED SELECT count(*)
	    INTO :iRowCount FROM dd_distrib_queue;
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
**
**  filled the combobox with the string in lpInfo->lpCddsInfo[iCurCdds].tcDisplayInfo
**  and ComboBox_SetItemData (hWndComboCdds,index,lpInfo->lpCddsInfo[iCurCdds].CddsNumber)
**
**  return :
**    RES_SUCCESS
**    RES_E_CONNECTION_FAILED
**    RES_E_CANNOT_ALLOCATE_MEMORY
**    RES_E_NO_INFORMATION_FOUND
**
*/
int LIBMON_GetCddsNumInLookupTbl(int hdl, char *lpDbName, char *cdds_lookup_tbl, LPRETRIEVECDDSNUM lpInfo)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char  stmt[256];
	char  cdds_name[MAXOBJECTNAME];
	long  cdds_no;
	int iNbcdds,iCurCdds;
	EXEC SQL END DECLARE SECTION;
	UCHAR szConnectName[MAX_CONNECTION_NAME];
	int ires,iret,iSession;

	wsprintf(szConnectName,"%s::%s",GetVirtNodeName(hdl), lpDbName);
	ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
	if (ires!=RES_SUCCESS)
		return RES_E_CONNECTION_FAILED;

	// get the number of cdds for structures allocation
	iNbcdds = 0;
	wsprintf(stmt, "SELECT count(*) "
	               "FROM %s t, dd_cdds d "
	               "WHERE t.cdds_no = d.cdds_no",
	         cdds_lookup_tbl);
	EXEC SQL EXECUTE IMMEDIATE :stmt INTO  :iNbcdds;
	if (sqlca.sqlcode < 0)
	{
		ManageSqlError(&sqlca);
		ires = ReleaseSession(iSession,RELEASE_COMMIT);
		return RES_ERR;
	}

	if (iNbcdds)
	{
		lpInfo->lpCddsInfo = ESL_AllocMem(iNbcdds * sizeof(CDDSINFO));
		if (lpInfo->lpCddsInfo)
			lpInfo->iNumCdds = iNbcdds;
		else
		{
			ires = ReleaseSession(iSession,RELEASE_COMMIT);
			return RES_E_CANNOT_ALLOCATE_MEMORY;
		}
	}
	else
	{
		lpInfo->lpCddsInfo = NULL;
		lpInfo->iNumCdds = 0;
		ires = ReleaseSession(iSession,RELEASE_COMMIT);
		return RES_E_NO_INFORMATION_FOUND;
	}
	iCurCdds = 0;
	iret = RES_SUCCESS;
	wsprintf(stmt, "SELECT DISTINCT d.cdds_no, d.cdds_name "
	               "FROM %s t, dd_cdds d "
	               "WHERE t.cdds_no = d.cdds_no ORDER BY cdds_no",
	         cdds_lookup_tbl);
	EXEC SQL EXECUTE IMMEDIATE :stmt
		INTO   :cdds_no, :cdds_name;
	EXEC SQL BEGIN;
		if (sqlca.sqlcode < 0) {
			iret = RES_ERR;
			ManageSqlError(&sqlca);
			EXEC SQL ENDSELECT;
		}
		suppspace(cdds_name);
		wsprintf (lpInfo->lpCddsInfo[iCurCdds].tcDisplayInfo,"%d %s",cdds_no,cdds_name);
		lpInfo->lpCddsInfo[iCurCdds].CddsNumber = cdds_no;
		iCurCdds++;
	EXEC SQL END;
	EXEC SQL COMMIT;
	ires = ReleaseSession(iSession,RELEASE_COMMIT);

	return iret;
}


int LIBMON_MonitorReplicatorStopSvr ( int hdl, int svrNo ,LPUCHAR lpDbName, LPERRORPARAMETERS lpError )
{
	EXEC SQL BEGIN DECLARE SECTION;
	    char szStmt[MAXOBJECTNAME];
	EXEC SQL END DECLARE SECTION;
	UCHAR szConnectName[MAX_CONNECTION_NAME];
	int ires,iSession;

	wsprintf(szConnectName,"%s::%s",GetVirtNodeName(hdl), lpDbName);
	ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
	if (ires!=RES_SUCCESS)
	{
		lpError->iErrorID = RES_E_OPENING_SESSION;
		lstrcpy( lpError->tcParam1, szConnectName);
		return RES_E_OPENING_SESSION;
	}

	wsprintf(szStmt,"RAISE DBEVENT dd_stop_server%d",svrNo);
	EXEC SQL EXECUTE IMMEDIATE :szStmt;
	if (sqlca.sqlcode < 0) {
		ManageSqlError(&sqlca);
		lpError->iErrorID = RES_E_SEND_EXECUTE_IMMEDIAT;
		lpError->iParam1  = sqlca.sqlcode;
	}
	else
		lpError->iErrorID = RES_SUCCESS;


	// Don't used ManageSqlError because raise event return WARNING 710.
	// ManageSqlError(&sqlca);
	EXEC SQL COMMIT;
	ires = ReleaseSession(iSession,RELEASE_COMMIT);
	if (ires != RES_SUCCESS)
		lpError->iErrorID |= MASK_RES_E_RELEASE_SESSION;
	return lpError->iErrorID;
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
**  RES_SUCCESS
**  RES_E_MAX_CONN_SEND_EVENT
**  RES_E_DBA_OWNER_EVENT
**  RES_E_OPENING_SESSION
**  RES_E_SEND_EXECUTE_IMMEDIAT
*/

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
int LIBMON_SendEvent( char *vnode, char *dbname,char *evt_name,char *flag_name,
                      char *svr_no, LPERRORPARAMETERS lpError)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char    stmt[256];
	EXEC SQL END DECLARE SECTION;
	UCHAR extradata[EXTRADATASIZE];
	UCHAR szConnectName[MAX_CONNECTION_NAME];
	UCHAR szVnodeName[MAX_VNODE_NAME];
	LPUCHAR parentstrings [MAXPLEVEL];
	UCHAR DBAUsernameOntarget[MAXOBJECTNAME];
	char buf[MAXOBJECTNAME];
	char evt_name2[MAXOBJECTNAME];
	int iSession,ires,hdl,iret,irestype;
	
	int thdl=0; /*Default thdl for non thread functions */
	iret = RES_SUCCESS;
	lpError->iErrorID = RES_SUCCESS;

	// Get Database Owner
	hdl  = LIBMON_OpenNodeStruct (vnode);
	if (hdl == -1) {
		//"The maximum number of connections has been reached"
		//" - Cannot send event on %s::%s"
		lpError->iErrorID = RES_E_MAX_CONN_SEND_EVENT;
		lstrcpy( lpError->tcParam1, vnode);
		lstrcpy( lpError->tcParam2, dbname);
		return RES_E_MAX_CONN_SEND_EVENT;
	}
	// Temporary for activate a session
	ires = LIBMON_DOMGetFirstObject (hdl, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL, thdl);
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
		LIBMON_CloseNodeStruct(hdl);
		//"DBA owner on database : %s not found.  - Cannot send event on %s::%s."
		lpError->iErrorID = RES_E_DBA_OWNER_EVENT;
		lstrcpy( lpError->tcParam1, vnode);
		lstrcpy( lpError->tcParam2, parentstrings[0]);
		//wsprintf(buf,ResourceString(IDS_F_DBA_OWNER_EVENT),parentstrings[0],vnode,parentstrings[0]);
		//TS_MessageBox(TS_GetFocus(),buf, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
		return RES_E_DBA_OWNER_EVENT;
	}
	LIBMON_CloseNodeStruct (hdl);
	wsprintf(szVnodeName,"%s%s%s%s",vnode, LPUSERPREFIXINNODENAME, DBAUsernameOntarget, LPUSERSUFFIXINNODENAME);
	hdl  = LIBMON_OpenNodeStruct (szVnodeName);
	if (hdl == -1) {
		lpError->iErrorID = RES_E_MAX_CONN_SEND_EVENT;
		lstrcpy( lpError->tcParam1, vnode);
		lstrcpy( lpError->tcParam2, dbname);
		return RES_E_MAX_CONN_SEND_EVENT;
		//wsprintf(szMess,ResourceString(IDS_F_MAX_CONN_SEND_EVENT),vnode, dbname);
		//TS_MessageBox(TS_GetFocus(),szMess,NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
	}

	wsprintf(szConnectName,"%s::%s",szVnodeName,dbname);
	ires = Getsession(szConnectName,SESSION_TYPE_CACHENOREADLOCK, &iSession);
	if (ires!=RES_SUCCESS)  {
		LIBMON_CloseNodeStruct (hdl);
		lpError->iErrorID = RES_E_OPENING_SESSION;
		lstrcpy( lpError->tcParam1, szConnectName);
		return RES_E_OPENING_SESSION;
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
		lpError->iErrorID = RES_E_SEND_EXECUTE_IMMEDIAT;
		lpError->iParam1  = sqlca.sqlcode;
		iret = RES_E_SEND_EXECUTE_IMMEDIAT;
	}

	ires = ReleaseSession(iSession, RELEASE_COMMIT);
	LIBMON_CloseNodeStruct (hdl);

	if (ires != RES_SUCCESS)
		lpError->iErrorID |= MASK_RES_E_RELEASE_SESSION;

	return lpError->iErrorID;
}
#endif


/*{
** Name:  VdbaColl_GetValue_columns - retrieve column names and values
**        and fill
**
** Description:
**  fill he column names and values for the
**  specified entry in the specified table.
**
**  Constructs a where clause with which to identify a set of columns
**  associated with a given replication key.
**
** Inputs:
**  sourcedb        - integer database_no
**  transaction_id  - integer transaction_id
**  sequence_no     - integer sequence number within a transaction
**
** Outputs:
**  Column_Value - fill with value for this column.
**
** Returns:
**  None
*/
static int VdbaColl_GetValue_columns(i2  sourcedb,long	transaction_id,long  sequence_no,
                                     char *target_wc,char *Column_Value,COLDEF    *pCol_Cur,
                                     char *dlm_dba_owner, LPERRORPARAMETERS lpError,
                                     TBLDEF *pRmTbl)
{
	COLDEF	*col_p;
	i4 column_number;
	EXEC SQL BEGIN DECLARE SECTION;
	char	tmp[256];
	char	tmp_statement[256];
	char	tmp_char[256];
	long  tmp_int = 0;
	double	tmp_float;
	char	column_val[32];
	char	where_clause[1024];
	short	null_ind;
	EXEC SQL END DECLARE SECTION;
	DBEC_ERR_INFO errinfo;

	/* Build a where clause */
	*where_clause = EOS;
	for (column_number = 0, col_p = pRmTbl->col_p;
		column_number < pRmTbl->ncols; column_number++, *col_p++)
	{
		if (col_p->key_sequence > 0)	/* All key columns */
		{
			STprintf(tmp, ERx(" AND t.%s = s.%s "),
				col_p->dlm_column_name, col_p->dlm_column_name);
			STcat(where_clause, tmp);
		}
	}
	col_p = pCol_Cur;

	if (STcompare(col_p->column_datatype, ERx("integer")) == 0)
	{
		if (target_wc == NULL)
		{
			STprintf(tmp_statement,
			    ERx("SELECT INT4(t.%s) FROM %s.%s t, "
			        "%s s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
			    col_p->dlm_column_name,
			    dlm_dba_owner,
			    pRmTbl->dlm_table_name, pRmTbl->sha_name,
			    sourcedb, transaction_id, sequence_no);
			STcat(tmp_statement, where_clause);
		}
		else
		{
			STprintf(tmp_statement, ERx("SELECT INT4(t.%s)"
			         "FROM %s.%s t WHERE %s"),
			                     col_p->dlm_column_name,
			                     dlm_dba_owner,
			                     pRmTbl->dlm_table_name, target_wc);
		}
		EXEC SQL EXECUTE IMMEDIATE :tmp_statement
			INTO :tmp_int:null_ind;
		STprintf(column_val, ERx("%ld"), tmp_int);
	}
	else if ((STcompare(col_p->column_datatype, ERx("float")) == 0) ||
		(STcompare(col_p->column_datatype, ERx("money")) == 0))
	{
		STprintf(tmp, "Float column %s\n", col_p->column_name);
		if (target_wc == NULL)
		{
			STprintf(tmp_statement,
			    ERx("SELECT FLOAT8(t.%s) FROM %s.%s t,"
			        " %s s WHERE sourcedb = %d AND "
			        "transaction_id = %d AND sequence_no = %d"),
			    col_p->dlm_column_name,
			    dlm_dba_owner,
			    pRmTbl->dlm_table_name, pRmTbl->sha_name,
			    sourcedb, transaction_id, sequence_no);
			STcat(tmp_statement, where_clause);
		}
		else
		{
			STprintf(tmp_statement,
				ERx("SELECT FLOAT8(t.%s) FROM %s.%s t WHERE %s"),
				col_p->dlm_column_name,
				dlm_dba_owner,
				pRmTbl->dlm_table_name, target_wc);
		}
		EXEC SQL EXECUTE IMMEDIATE :tmp_statement
			INTO	:tmp_float:null_ind;
		STprintf(column_val, ERx("%.13g"), tmp_float,
			(char)RMadf_cb->adf_decimal.db_decimal);
	}
	else if (STequal(col_p->column_datatype, ERx("long varchar")) ||
		STequal(col_p->column_datatype, ERx("long byte")))
	{
		STcopy("**Unbounded Data Type**", column_val);
		errinfo.rowcount = 1;
	}
	else
	{
		STprintf(tmp, "Char column %s\n", col_p->column_name);
		if (target_wc == NULL)
		{
			STprintf(tmp_statement,
			    ERx("SELECT TRIM(CHAR(t.%s)) FROM %s.%s"
			        " t, %s s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
			                           col_p->dlm_column_name,
			                           dlm_dba_owner,
			                           pRmTbl->dlm_table_name, pRmTbl->sha_name,
			                           sourcedb, transaction_id, sequence_no);
			STcat(tmp_statement, where_clause);
		}
		else
		{
			STprintf(tmp_statement, ERx("SELECT TRIM(CHAR(t.%s)) FROM %s.%s t WHERE %s"),
			       col_p->dlm_column_name,
			       dlm_dba_owner,
			       pRmTbl->dlm_table_name, target_wc);
		}
		EXEC SQL EXECUTE IMMEDIATE :tmp_statement
			INTO  :tmp_char:null_ind;
		STprintf(column_val, ERx("%s"), tmp_char);
	}
	if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
	{
		if (errinfo.errorno)
		{
			lpError->iErrorID = RES_E_F_COLUMN_INFO;
			lstrcpy( lpError->tcParam1, pRmTbl->table_owner);
			lstrcat( lpError->tcParam1, ("."));
			lstrcat( lpError->tcParam1, pRmTbl->table_name);
			lstrcat( lpError->tcParam1, ("."));
			lstrcat( lpError->tcParam1, col_p->column_name);
			return RES_E_F_COLUMN_INFO;//"Error retrieving column information for '%s.%s.%s'"
		}
	}
	if (errinfo.rowcount > 0)
	{
		if (null_ind == -1)
			STcopy(ERx("NULL"), column_val);
		lstrcpy(Column_Value,column_val);
	}
	else
	{
		lstrcpy(Column_Value,"N/A");
	}
	return RES_SUCCESS;
}

# define TRUNC_TBL_NAMELEN	10	/* truncated table name len */


/*{
** Name:	RPtblobj_name - generate a table object name
**
** Description:
**	Generates the name for a table object.
**
** Inputs:
**	table_name	- the table name
**	table_no	- the table number
**	object_type - table object type constant
**
** Outputs:
**	object_name - the table object name
**
** Returns:
**	RES_SUCCESS or RES_ERR
**
** History:
**	27-Jan-98	(Philippe Schalk)
**		Created based on tblobjs.sc in replicator library.
*/
static long VdbaColl_tblobj_name(char *table_name,long table_no,
                                 long object_type,char *object_name, LPERRORPARAMETERS lpError)
{
	char dlm_table_name[DLMNAME_LENGTH];
	/* initialize object_name in case of early return */
	*object_name = EOS;

	/* validate table name */
	if (table_name == NULL || *table_name == EOS)
	{
		lpError->iErrorID = RES_E_BLANK_TABLE_NAME;
		return RES_E_BLANK_TABLE_NAME; //"TBLOBJ_NAME: Null or blank table name.\n"
	}
	/* validate table number */
	if (table_no < TBLOBJ_MIN_TBLNO || table_no > TBLOBJ_MAX_TBLNO)
	{
		lpError->iErrorID = RES_E_F_TBL_NUMBER;
		lpError->iParam1 = table_no;
		return (RES_E_F_TBL_NUMBER); //"TBLOBJ_NAME: Invalid table number of %d\n"
	}

	/* make the specified object name */
	switch (object_type)
	{
	/* C function names */
	case TBLOBJ_INS_FUNC:  /* insert table function */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
		    VdbaColl_edit_name(EDNM_ALPHA, table_name, dlm_table_name),
		    TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("ins"));
		break;

	case TBLOBJ_UPD_FUNC:  /* update table function */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
		    VdbaColl_edit_name(EDNM_ALPHA, table_name, dlm_table_name),
		    TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("upd"));
		break;

	case TBLOBJ_DEL_FUNC:  /* delete table function */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
		    VdbaColl_edit_name(EDNM_ALPHA, table_name, dlm_table_name),
		    TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("del"));
		break;

	case TBLOBJ_QRY_FUNC:  /* generic query table function */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
		    VdbaColl_edit_name(EDNM_ALPHA, table_name, dlm_table_name),
		    TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("qry"));
		break;

	/* Table object names */
	case TBLOBJ_ARC_TBL:  /* archive table */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
		    VdbaColl_edit_name(EDNM_ALPHA, table_name, dlm_table_name),
		    TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("arc"));
		break;

	case TBLOBJ_SHA_TBL:  /* shadow table */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
		    VdbaColl_edit_name(EDNM_ALPHA, table_name, dlm_table_name),
		    TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("sha"));
		break;

	case TBLOBJ_TMP_TBL:  /* temporary table */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
		    VdbaColl_edit_name(EDNM_ALPHA, table_name, dlm_table_name),
		    TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("tmp"));
		break;

	case TBLOBJ_REM_INS_PROC:  /* remote insert procedure */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
		    VdbaColl_edit_name(EDNM_ALPHA, table_name, dlm_table_name),
		    TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("rmi"));
		break;

	case TBLOBJ_REM_UPD_PROC:  /* remote update procedure */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
		    VdbaColl_edit_name(EDNM_ALPHA, table_name, dlm_table_name),
		    TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("rmu"));
		break;

	case TBLOBJ_REM_DEL_PROC:  /* remote delete procedure */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
		    VdbaColl_edit_name(EDNM_ALPHA, table_name, dlm_table_name),
		    TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("rmd"));
		break;

	case TBLOBJ_SHA_INDEX1:  /* shadow index 1 */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
		    VdbaColl_edit_name(EDNM_ALPHA, table_name, dlm_table_name),
		    TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("sx1"));
		break;

	default:
		lpError->iParam1 = object_type;
		lpError->iErrorID = RES_E_F_INVALID_OBJECT;
		return (RES_E_F_INVALID_OBJECT);// "TBLOBJ_NAME: Invalid object type of %d.\n"
	}

	return (RES_SUCCESS);
}


/*{
** Name:    Vdba_error_check - check for database error
**
** Description:
**  Verifies the success of a database operation.  If there is no error
**  and the number of rows affected agrees with the expected result, it
**  returns OK.  If there is a deadlock error or the INGRES log file
**  becomes full, DBEC_DEADLOCK is returned.  For any other kind of error,
**  RES_ERR is returned.
**
** Inputs:
**  flags   - integer combination of zero, one or more of the following
**        constants, joined with bitwise-OR:
**
**  Constant        Meaning
**  ------------    -------------------------------------------
**  SINGLE_ROW  The operation should affect only one row.
**
**  ZERO_ROWS_OK    The operation is considered successful even if it
**          does not affect any rows.
**
** Outputs:
**  errinfo - if this pointer is not NULL, the following members are
**        filled in:
**
**      errorno  - generic INGRES error
**      rowcount - number of rows affected by the last query
**
** Returns:
**  RES_SUCCESS      - database operation successful
**  DBEC_DEADLOCK    - serialization error
**  RES_ERR          - other database error
**/
long Vdba_error_check( long flags,DBEC_ERR_INFO *errinfo)
{
	EXEC SQL BEGIN DECLARE SECTION;
	long n, err;
	EXEC SQL END DECLARE SECTION;

	ManageSqlError(&sqlca);

	EXEC SQL INQUIRE_SQL (:n = ROWCOUNT, :err = ERRORNO);
	if (errinfo != (DBEC_ERR_INFO *)NULL)
	{
		errinfo->errorno = err;
		errinfo->rowcount = n;
	}
	if (err == GE_SERIALIZATION)
		return (DBEC_DEADLOCK);
	else if (err || (n != 1 && flags & DBEC_SINGLE_ROW) ||
			(n == 0 && !(flags & DBEC_ZERO_ROWS_OK)))
		return (RES_ERR);
	return (RES_SUCCESS);
}

/*
** Name:    VdbaColl_Get_Collisions    - build list of collisions
**
** Description:
**    Populate the collision_list[] array with any queue collisions found by
**    analyzing the local dd_distrib_queue.
** Inputs:
**    localdbname  - char string containing local dbname
**    vnode_name   - char string containing local vnode
**
** Outputs:
**   collision_count   - integer count of # of found collisions
**   records           - integer count of # of distribution queue records
**                       inspected.
*/
#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
static long VdbaColl_Get_Collisions ( char *localdbname, char *vnode_name, short localdb,
                                      char *local_owner, long *collision_count,
                                      long *records , LPERRORPARAMETERS lpError,
                                      TBLDEF *pRmTbl,GLOBALSESSION *pGlobSession,
                                      GLOBALCOLLISIONLIST *pColList)
{
EXEC SQL BEGIN DECLARE SECTION;
	int   err,transaction_id,sequence_no;
	short trans_type,old_sourcedb,targetdb,sourcedb;
	int   table_no = 0;
	int   old_transaction_id = 0;
	int   old_sequence_no = 0;
	char  trans_time[26];
	long  dd_priority;
	int   svrTargetType;
EXEC SQL END DECLARE SECTION;
	DBEC_ERR_INFO errinfo;
	long  tmp_sqlcode;
	IISQLDA    _sqlda;
	IISQLDA    *sqlda = &_sqlda;

	if ( VdbaColl_Init_Sessions(localdbname, vnode_name, localdb,lpError,pGlobSession) != RES_SUCCESS){
		ActivateSession(pGlobSession->initial_session);
		EXEC SQL COMMIT;
		if (pGlobSession->NumSessions == 0)
			return (RES_SUCCESS);    /* no entries to process */
		else
			return (RES_ERR);
	}
	ActivateSession(pGlobSession->initial_session);

	*collision_count = 0;
	*records = 0;

	/* ddba_messageit_text(tmp, 1314);*/		/* Ingres/Replicator */

	EXEC SQL DECLARE c3 CURSOR FOR
		SELECT	  q.targetdb, q.sourcedb, q.transaction_id,
			q.sequence_no, q.trans_type, q.table_no,
			q.old_sourcedb,  q.old_transaction_id,
			q.old_sequence_no,q.trans_time,q.dd_priority,c.target_type
		FROM	dd_distrib_queue q, dd_db_cdds c
		WHERE	 c.database_no = q.targetdb
		AND    c.cdds_no = q.cdds_no
		AND    c.target_type in (1, 2)
		ORDER	 BY targetdb, sourcedb, transaction_id, sequence_no;

	EXEC SQL OPEN c3 FOR READONLY;

	if (Vdba_error_check(0, &errinfo) != RES_SUCCESS)
	{
		EXEC SQL ROLLBACK;
		ActivateSession(pGlobSession->initial_session);
		lpError->iErrorID = RES_E_OPENING_CURSOR;
		return (RES_E_OPENING_CURSOR); // " Error opening cursor."
	}
	

	tmp_sqlcode = sqlca.sqlcode;
	while (tmp_sqlcode == 0)
	{
		ActivateSession(pGlobSession->initial_session);
		EXEC SQL FETCH c3
		     INTO :targetdb, :sourcedb, :transaction_id,
		          :sequence_no, :trans_type, :table_no,
		          :old_sourcedb, :old_transaction_id,
		          :old_sequence_no,:trans_time,:dd_priority,:svrTargetType;

		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			ActivateSession(pGlobSession->initial_session);
			EXEC SQL CLOSE c3;
			EXEC SQL ROLLBACK;
			lpError->iErrorID = RES_E_F_FETCHING_DATA;
			lpError->iParam1 = errinfo.errorno;
			return (RES_E_F_FETCHING_DATA); //("Error %d when fetching data from distribution queue.")
		}

		tmp_sqlcode = sqlca.sqlcode;
		if (sqlca.sqlcode == 0)
		{
			*records += 1;

			err = LIBMON_VdbaColl_tbl_fetch(table_no, FALSE, lpError,pRmTbl);
			if (err != RES_SUCCESS) {
				ActivateSession(pGlobSession->initial_session);
				EXEC SQL CLOSE c3;
				EXEC SQL ROLLBACK;
				return (RES_ERR);
			}

			switch(trans_type)
			{
			case TX_INSERT:
				VdbaColl_Check_Insert(collision_count, localdb, targetdb,
				                      sourcedb, transaction_id,sequence_no, table_no,
				                      trans_time,dd_priority, local_owner,
				                      svrTargetType,lpError,pRmTbl,pGlobSession,
				                      pColList);
				break;

			case TX_UPDATE:
			case TX_DELETE:
				VdbaColl_Check_Update_Delete((long)trans_type,
					collision_count, localdb, targetdb,
					old_sourcedb, old_transaction_id,
					old_sequence_no, sourcedb,
					transaction_id, sequence_no,trans_time,dd_priority,
					svrTargetType,lpError,pRmTbl,pGlobSession,pColList);
				break;
			}
		}
	}

	ActivateSession(pGlobSession->initial_session);
	EXEC SQL CLOSE c3;
	EXEC SQL COMMIT;

	return (RES_SUCCESS);
}
#endif


/*{
** Name:   VdbaColl_Check_Insert - check for insert collisions
**
** Description:
**    check for an insert collision & report it if found.
**
** Inputs:
**    localdb        -
**    target         -
**    database_no    -
**    transaction_id -
**    sequence_no    -
**    table_no       -
**
** Outputs:
**    collision_count   -
**
** Returns:
**    Error Number
*/
static int VdbaColl_Check_Insert(  long  *collision_count , long  localdb,
                                   short targetdb         , short sourcedb,
                                   long  transaction_id   , long  sequence_no,
                                   long  table_no,char *trans_time,long nPriority,
                                   char *local_owner,int nSvrTargetType,
                                   LPERRORPARAMETERS lpError,
                                   TBLDEF *pRmTbl, GLOBALSESSION *pCurSession,
                                   GLOBALCOLLISIONLIST *pColList )
{
	DBEC_ERR_INFO errinfo;
	char    dlm_target_owner[DLMNAME_LENGTH+1];
	EXEC SQL BEGIN DECLARE SECTION;
	long    err,priority;
	long    cnt;
	char    select_statement[1024];
	char    where_clause[1024];
	int     hdlSess,i;
	EXEC SQL END DECLARE SECTION;
	ActivateSession(pCurSession->initial_session);

	err = VdbaColl_Create_Key_Clauses(where_clause, NULL, NULL, NULL,
		pRmTbl->table_name, pRmTbl->table_owner, pRmTbl->table_no,
		pRmTbl->sha_name, sourcedb, transaction_id, sequence_no,lpError);
	if (err != RES_SUCCESS)    {
		ActivateSession(pCurSession->initial_session);
		return err;
	}

	if (STequal(pRmTbl->table_owner, local_owner))
	{
		DBSESSION  *pDBTemp;
		pDBTemp = pCurSession->pSession;
		for (i = 1,pDBTemp++; i < pCurSession->NumSessions; ++i,pDBTemp++)
			if (pDBTemp->database_no == targetdb)
				break;
		VdbaColl_edit_name(EDNM_DELIMIT, pDBTemp->dba, dlm_target_owner);
	}
	else
	{
		lstrcpy( dlm_target_owner , pRmTbl->dlm_table_owner);
	}
	/* is the row to be inserted already in the user table on the target */
	STprintf(select_statement,ERx("SELECT COUNT(*) FROM %s.%s t WHERE %s"),
			 dlm_target_owner, pRmTbl->dlm_table_name, where_clause);

	hdlSess = GetSessionNumber4DbNum(targetdb ,pCurSession);
	if (hdlSess == -1)
		return RES_E_SESSION_NUMBER; //"CHECK_INSERT:  Error Session number."

	ActivateSession(hdlSess);

	EXEC SQL EXECUTE IMMEDIATE :select_statement INTO :cnt;
/* ----------------------------------------------------*/
	if ( Vdba_error_check(DBEC_SINGLE_ROW, &errinfo) != RES_SUCCESS)    {
		ActivateSession(pCurSession->initial_session);
		lpError->iErrorID = RES_E_IMMEDIATE_EXECUTION;
		lpError->iParam1 = errinfo.errorno;
		return RES_E_IMMEDIATE_EXECUTION; //"CHECK_INSERT:  Error %d on immediate execution."

	}
	if (cnt)	/* collision */
	{
		COLLISION_FOUND *pCollisionList = ReallocCollisionList( *collision_count, pColList);
		if (pCollisionList == NULL)
			return RES_E_CANNOT_ALLOCATE_MEMORY;
		pCollisionList[*collision_count].local_db           = (short)localdb;
		pCollisionList[*collision_count].remote_db          = targetdb;
		pCollisionList[*collision_count].type               = TX_INSERT;
		pCollisionList[*collision_count].resolved           = FALSE;
		pCollisionList[*collision_count].nSvrTargetType     = nSvrTargetType;
		pCollisionList[*collision_count].db1.sourcedb       = sourcedb;
		pCollisionList[*collision_count].db1.transaction_id = transaction_id;
		pCollisionList[*collision_count].db1.sequence_no    = sequence_no;
		pCollisionList[*collision_count].db1.table_no       = pRmTbl->table_no;
		pCollisionList[*collision_count].db1.nResolvedCollision = nPriority;
		lstrcpy(pCollisionList[*collision_count].db1.TransTime,trans_time);
		sourcedb = (short)0;
		transaction_id = sequence_no = 0;
		*trans_time=EOS;
		STprintf(select_statement, ERx("SELECT sourcedb, transaction_id, "
		                               "sequence_no,trans_time,dd_priority "
		                               "FROM %s t WHERE in_archive = 0 AND %s"),
		                                pRmTbl->sha_name, where_clause);
		hdlSess = GetSessionNumber4DbNum(targetdb,pCurSession);
		if (hdlSess == -1)
			return RES_E_SESSION_NUMBER;

		ActivateSession(hdlSess);
		/*EXEC SQL SET_SQL (SESSION = :targetdb);*/

		EXEC SQL EXECUTE IMMEDIATE :select_statement
		         INTO :sourcedb, :transaction_id, :sequence_no, :trans_time,:priority;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)  {
			ActivateSession(pCurSession->initial_session);
			lpError->iErrorID = RES_E_IMMEDIATE_EXECUTION;
			lpError->iParam1 = errinfo.errorno;
			return RES_E_IMMEDIATE_EXECUTION; // "CHECK_INSERT:  Error %d on immediate execution."
		}
		pCollisionList[*collision_count].db2.sourcedb       = sourcedb;
		pCollisionList[*collision_count].db2.transaction_id = transaction_id;
		pCollisionList[*collision_count].db2.sequence_no    = sequence_no;
		pCollisionList[*collision_count].db2.table_no       = pRmTbl->table_no;
		lstrcpy(pCollisionList[*collision_count].db2.TransTime,trans_time);
		pCollisionList[*collision_count].db2.nResolvedCollision = priority;
		*collision_count += 1;
	}
	ActivateSession(pCurSession->initial_session);
	return RES_SUCCESS;
}


/*{
** Name:	VdbaColl_Check_Update_Delete - check for update/delete collisions
**
** Description:
**	  check for an update or delete collision & report it if found.
**
** Inputs:
**	  trans_type	-
**	  localdb		-
**	  targetdb		-
**	  old_sourcedb	-
**	  old_transaction_id -
**	  old_sequence_no	 -
**	  database_no	 -
**	  transaction_id	-
**	  sequence_no	 -
**
** Outputs:
**	  collision_count	 -
**
** Returns:
**	  none
*/
static int VdbaColl_Check_Update_Delete( long trans_type,long *collision_count,
                                 long localdb, short targetdb,
                                 short old_sourcedb, long old_transaction_id,
                                 long old_sequence_no, short sourcedb,
                                 long transaction_id, long sequence_no,
                                 char *TransTime, long nPriority, int nSvrTargetType,
                                 LPERRORPARAMETERS lpError,TBLDEF *pRmTbl,
                                 GLOBALSESSION *pTempSession,
                                 GLOBALCOLLISIONLIST *pColList)
{
	DBEC_ERR_INFO errinfo;
	EXEC SQL BEGIN DECLARE SECTION;
	long	cnt;
	char	select_statement[1024];
	EXEC SQL END DECLARE SECTION;
	int 	hdlSess;

	hdlSess = GetSessionNumber4DbNum(targetdb,pTempSession);
	if (hdlSess == -1)
		return RES_E_SESSION_NUMBER;

	ActivateSession(hdlSess);

	STprintf(select_statement,
		ERx("SELECT COUNT(*) FROM %s WHERE sourcedb= %d AND "
			"transaction_id = %d AND sequence_no = %d AND in_archive = 0"),
		pRmTbl->sha_name, old_sourcedb, old_transaction_id,
		old_sequence_no);

	EXEC SQL EXECUTE IMMEDIATE :select_statement INTO :cnt;
	if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)  {
		ActivateSession(pTempSession->initial_session);
		lpError->iErrorID = RES_E_EXECUT_IMMEDIAT;
		lpError->iParam1 = errinfo.errorno;
		return RES_E_EXECUT_IMMEDIAT; // "Error %d when executing immediately"
	}
	if (cnt == 0)	{  /* possible collision */
		/*
		** check new_key flag in the source shadow table.  if
		** it's set, then we don't have a collision.
		*/
		ActivateSession(pTempSession->initial_session);
		/*EXEC SQL SET_SQL (SESSION = :initial_session);*/

		STprintf(select_statement,ERx("SELECT COUNT(*) FROM %s WHERE sourcedb = %d AND "
		                              "transaction_id = %d AND sequence_no = %d AND new_key = 1"),
		                              pRmTbl->sha_name, old_sourcedb, old_transaction_id,
		                              old_sequence_no);

		EXEC SQL EXECUTE IMMEDIATE :select_statement INTO :cnt;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)  {
			ActivateSession(pTempSession->initial_session);
			lpError->iParam1 = errinfo.errorno;
			lpError->iErrorID = RES_E_EXECUT_IMMEDIAT;
			return RES_E_EXECUT_IMMEDIAT; // Error %d when executing immediately
		}
		if (cnt == 0)	{ /* collision */
			COLLISION_FOUND *pCollisionList = ReallocCollisionList( *collision_count , pColList);
			if (pCollisionList==NULL)
				return RES_E_CANNOT_ALLOCATE_MEMORY;
			pCollisionList[*collision_count].local_db           = (short)localdb;
			pCollisionList[*collision_count].remote_db          = targetdb;
			pCollisionList[*collision_count].type               = trans_type;
			pCollisionList[*collision_count].nSvrTargetType     = nSvrTargetType;
			pCollisionList[*collision_count].resolved           = FALSE;
			pCollisionList[*collision_count].db1.sourcedb       = sourcedb;
			pCollisionList[*collision_count].db1.transaction_id = transaction_id;
			pCollisionList[*collision_count].db1.sequence_no    = sequence_no;
			pCollisionList[*collision_count].db1.table_no       = pRmTbl->table_no;
			pCollisionList[*collision_count].db1.old_source_db      = old_sourcedb;
			pCollisionList[*collision_count].db1.old_transaction_id = old_transaction_id;
			pCollisionList[*collision_count].db1.old_sequence_no    = old_sequence_no;
			lstrcpy(pCollisionList[*collision_count].db1.TransTime,TransTime);
			pCollisionList[*collision_count].db1.nResolvedCollision = nPriority;
			*collision_count += 1;
		}
	}
	ActivateSession(pTempSession->initial_session);
	return RES_SUCCESS;
}

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
static int VdbaColl_Init_Sessions( char *localdbname, char *vnode_name,
                                   long localdb,LPERRORPARAMETERS lpError,
                                   GLOBALSESSION *pGlobSess)
{
	long i;
	int ires,hdl = -1;
	char buf[MAXOBJECTNAME];
	char VnodeNameAndDBA [MAX_VNODE_NAME];
	char *pDest;
	int thdl=0; /*Default thdl for non thread functions */
	EXEC SQL BEGIN DECLARE SECTION;
		short database_no; 
		char  vnode_name2[DB_MAXNAME+1];
		char  database_name[DB_MAXNAME+1];
		char  dba[DB_MAXNAME+1];
		char tmp[MAX_CONNECTION_NAME];
		GLOBALSESSION *pTempSess;
		DBSESSION  *pTempDBSession;
	EXEC SQL END DECLARE SECTION;

	pTempSess = pGlobSess;
	ActivateSession(pTempSess->initial_session);
	EXEC SQL REPEATED SELECT COUNT(DISTINCT targetdb)
	     INTO  :pTempSess->NumSessions
	     FROM  dd_distrib_queue;
	if (sqlca.sqlcode < 0){
		ManageSqlError(&sqlca);//("Error counting potential connections.")
		lpError->iErrorID = RES_E_COUNT_CONNECTIONS;
		return RES_E_COUNT_CONNECTIONS;
	}

	if (pTempSess->NumSessions == 0)	{
		//There are no entries in the distribution queue.
		return RES_ERR;
	}
	pTempSess->pSession = (DBSESSION *)ESL_AllocMem((u_long)(pTempSess->NumSessions + 1) * sizeof(DBSESSION));
	if (pTempSess->pSession == NULL)	{
		lpError->iErrorID = RES_E_CANNOT_ALLOCATE_MEMORY;
		return RES_E_CANNOT_ALLOCATE_MEMORY;
	}
	pTempDBSession = pTempSess->pSession;

	/* Establish a connection to the local db */
	EXEC SQL REPEATED SELECT DBMSINFO('dba')
	    INTO    :pTempDBSession->dba;

	pDest=x_strstr(vnode_name,LIBMON_getLocalNodeString());//ResourceString((UINT)IDS_I_LOCALNODE)
	if (pDest != vnode_name) {
		x_strcpy(pTempDBSession->vnode_name,vnode_name);
	}
	else {
		// Get the II_HOSTNAME if vnode name is "(local)"
		memset (pTempDBSession->vnode_name,EOS,sizeof(pTempDBSession->vnode_name));
		GChostname (pTempDBSession->vnode_name,sizeof(pTempDBSession->vnode_name));
		if (pTempDBSession->vnode_name[0] == EOS)
			x_strcpy(pTempDBSession->vnode_name,vnode_name);
	}

	x_strcpy(pTempDBSession->database_name,localdbname);
	pTempDBSession->database_no = (short)localdb;
	pTempDBSession->iSession    = pTempSess->initial_session;
	pTempDBSession->iHnodeStruc = pTempSess->initial_HnodeStruct;
	if (*vnode_name == EOS)
		STcopy(pTempDBSession->database_name, pTempDBSession->full_name);
	else
		STprintf(pTempDBSession->full_name, ERx("%s::%s"),
			pTempDBSession->vnode_name, pTempDBSession->database_name);
 
	pTempDBSession->connected = TRUE;

	++pTempSess->NumSessions;

	// initialize iHnodeStruc and connected;
	for (i = 1,pTempDBSession++; i < pTempSess->NumSessions; ++i,pTempDBSession++) {
		pTempDBSession->connected = FALSE;
		pTempDBSession->iHnodeStruc = -1;
	}

	EXEC SQL DECLARE opendb_csr CURSOR FOR
	    SELECT  DISTINCT q.targetdb, d.database_name, d.vnode_name, d.database_owner
	    FROM    dd_distrib_queue q, dd_databases d
	    WHERE   q.targetdb = d.database_no;
	EXEC SQL OPEN opendb_csr FOR READONLY;
	if (sqlca.sqlcode < 0) {
		ManageSqlError(&sqlca);
		lpError->iErrorID = RES_E_OPENING_CURSOR;
		return RES_E_OPENING_CURSOR;
	}
	pTempDBSession = pTempSess->pSession;
	for (i = 1,pTempDBSession++; i < pTempSess->NumSessions; ++i,++pTempDBSession)
	{
		EXEC SQL FETCH opendb_csr
		    INTO :database_no,
		         :database_name,
		         :vnode_name2,
		         :dba;
		if (sqlca.sqlcode < 0){
			EXEC SQL CLOSE opendb_csr;
			EXEC SQL COMMIT;
			ManageSqlError(&sqlca);
			lpError->iErrorID = RES_E_OPENING_CURSOR;
			return RES_E_OPENING_CURSOR;
		}

		suppspace(vnode_name2);
		suppspace(database_name);
		suppspace(dba);

		pTempDBSession->database_no = database_no;
		x_strcpy(pTempDBSession->database_name, database_name);
		x_strcpy(pTempDBSession->vnode_name, vnode_name2);
		x_strcpy(pTempDBSession->dba, dba);

		switch (sqlca.sqlcode)
		{
		case 0:
			if (*(pTempDBSession->vnode_name) != EOS)	{
				STprintf(tmp, ERx("%s::%s"),pTempDBSession->vnode_name,
				                            pTempDBSession->database_name);
			}
			else	{
				STcopy(pTempDBSession->database_name, tmp);
			}
			// Format string with user for getsession()
			STcat(tmp,LPUSERPREFIXINNODENAME);
			STcat(tmp,pTempDBSession->dba);
			STcat(tmp,LPUSERSUFFIXINNODENAME);
			// Format string with user for OpenNodeStruct()
			STcopy(pTempDBSession->vnode_name, VnodeNameAndDBA);
			STcat(VnodeNameAndDBA,LPUSERPREFIXINNODENAME);
			STcat(VnodeNameAndDBA,pTempDBSession->dba);
			STcat(VnodeNameAndDBA,LPUSERSUFFIXINNODENAME);

			pTempDBSession->iHnodeStruc = OpenNodeStruct4Replication(VnodeNameAndDBA);
			if (pTempDBSession->iHnodeStruc < 0) {
				EXEC SQL CLOSE opendb_csr;
				EXEC SQL COMMIT;
				//"Maximum number of connections has been reached\n - Cannot display Collisions Report."
				lpError->iErrorID = RES_E_MAX_CONNECTIONS;
				return RES_E_MAX_CONNECTIONS;
			}
			ires = LIBMON_DOMGetFirstObject (pTempDBSession->iHnodeStruc, OT_DATABASE, 0, NULL, TRUE, NULL, buf, NULL, NULL, thdl);
			
			pTempDBSession->connected = FALSE;
			
			ires = Getsession4Replication(tmp,SESSION_TYPE_INDEPENDENT, &(pTempDBSession->iSession));
			if (ires != RES_SUCCESS) {
				EXEC SQL CLOSE opendb_csr;
				EXEC SQL COMMIT;
				lstrcpy( lpError->tcParam1,tmp);
				lpError->iErrorID = RES_E_OPENING_SESSION;
				return RES_E_OPENING_SESSION; //"Error while opening session on :%s"
			}
			pTempDBSession->connected = TRUE;

			ActivateSession(pTempSess->initial_session);
			break;

		case 100:
			goto closecsr;

		default:
			ManageSqlError(&sqlca); 
			EXEC SQL CLOSE opendb_csr;
			EXEC SQL COMMIT;
			lpError->iErrorID = RES_E_FETCHING_TARGET_DB;
			return RES_E_FETCHING_TARGET_DB; //("Error fetching target databases.")
		}
	}

closecsr:
	EXEC SQL CLOSE opendb_csr;
	EXEC SQL COMMIT;

	ActivateSession(pTempSess->initial_session);
	return (RES_SUCCESS);
}

#endif


static int VdbaColl_Close_Sessions(GLOBALSESSION *pCurSession)
{
	DBSESSION  *pTempSess;

	if (pCurSession->pSession == NULL)
		return (RES_SUCCESS);

	pTempSess = pCurSession->pSession;

	for (pTempSess++; (pTempSess - pCurSession->pSession) < pCurSession->NumSessions; pTempSess++){
		if (pTempSess->connected == TRUE)  {
			ActivateSession(pTempSess->iSession);
			ReleaseSession(pTempSess->iSession,RELEASE_COMMIT);
		}
		if ((pTempSess - pCurSession->pSession) >0 && pTempSess->iHnodeStruc >= 0 )
			LIBMON_CloseNodeStruct (pTempSess->iHnodeStruc);
	}
	ESL_FreeMem (pCurSession->pSession);
	pCurSession->pSession = NULL;
	return (RES_SUCCESS);
}

/*{
** Name:  VdbaColl_Create_Key_Clauses - create key clauses
**
** Description:
**    Creates clauses of SQL statements for a given transaction (for
**    instance, builds the where clause using the data in the base table that
**   corresponds to the replication key).
**
** Inputs:
**    table_name    -
**    table_owner   -
**    table_no      -
**    sha_name      -
**    database_no   -
**    transaction_id -
**    sequence_no    -
**
** Outputs:
**    where_clause        -
**    name_clause         -
**    insert_value_clause -
**    update_clause       -
**    lpError             - filled if error  found
**
** Returns:
**	  RES_SUCCESS or ?
**/
static long
VdbaColl_Create_Key_Clauses(char *where_clause,char *name_clause,char *insert_value_clause,
                            char    *update_clause, char    *table_name, char    *table_owner,
                            long    table_no, char    *sha_name,short    database_no,
                            long    transaction_id,long    sequence_no ,LPERRORPARAMETERS lpError)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char  *table_name;
char  *table_owner;
long  table_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	long    column_count,i;
	char    dlm_column_name[DLMNAME_LENGTH];
	char    column_value[MAX_COLS][32];
	EXEC SQL BEGIN DECLARE SECTION;
	char    column_name[MAX_COLS][DB_MAXNAME+1];
	char    column_datatype[MAX_COLS][32];
	long    column_length[MAX_COLS];
	long    err;
	long    rows;
	char    tmp_statement[256];
	long    tmp_int;
	double  tmp_float;
	VARCHAR struct
	{
		short   tmp_len;
		char    tmp_char[2048];
	} tmp_vch;
	short     null_ind;
	EXEC SQL END DECLARE SECTION;
	char    decimal_char = EOS;

	if (decimal_char == EOS)
		decimal_char = (char)FEadfcb()->adf_decimal.db_decimal;

	if (where_clause)
		*where_clause = EOS;
	if (name_clause)
		*name_clause = EOS;
	if (insert_value_clause)
		*insert_value_clause = EOS;
	if (update_clause)
		*update_clause = EOS;
	column_count = 0;
	EXEC SQL REPEATED SELECT DISTINCT TRIM(d.column_name),
	         TRIM(LOWERCASE(column_datatype)),
	         column_length
	    INTO :column_name[column_count],
	         :column_datatype[column_count],
	         :column_length[column_count]
	    FROM dd_regist_columns d, iicolumns i
	    WHERE  i.table_name = :table_name
	    AND    i.table_owner = :table_owner
	    AND    d.table_no = :table_no
	    AND    d.key_sequence > 0
	    AND    d.column_name = i.column_name;
	EXEC SQL BEGIN;
		column_count++;
	EXEC SQL END;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO);
	if (err)
	{
		lpError->iParam1 = err;
		lstrcpy(lpError->tcParam1,table_owner);
		lstrcpy(lpError->tcParam2,table_name);
		lpError->iErrorID = RES_E_F_GETTING_COLUMNS;
		return (RES_E_F_GETTING_COLUMNS); //"Error %d getting columns for '%s.%s'"
	}

	if (column_count == 0)
	{
		lstrcpy(lpError->tcParam1,table_owner);
		lstrcpy(lpError->tcParam2,table_name);
		lpError->iErrorID = RES_E_F_REGISTRED_COL;
		return (RES_E_F_REGISTRED_COL); /* No registered columns for '%s.%s'. */
	}

	for (i = 0; i < column_count; i++)
	{
		VdbaColl_edit_name(EDNM_DELIMIT, column_name[i], dlm_column_name);
		if (STequal(column_datatype[i], ERx("integer")))
		{
			STprintf(tmp_statement,
				ERx("SELECT INT4(%s) FROM %s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
				dlm_column_name, sha_name, database_no,
				transaction_id, sequence_no);
			EXEC SQL EXECUTE IMMEDIATE :tmp_statement
				INTO	:tmp_int:null_ind;
			EXEC SQL INQUIRE_SQL (:rows = ROWCOUNT, :err = ERRORNO);
			if (err)
			{
				//wsprintf(tmp,ResourceString(IDS_ERR_IMMEDIATE_EXECUTION),err);
				//TS_MessageBox(TS_GetFocus(),tmp, NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);
				lpError->iErrorID = RES_E_IMMEDIATE_EXECUTION;
				lpError->iParam1 = err;
				return (RES_E_IMMEDIATE_EXECUTION); /* "CHECK_INSERT:  Integer execute immediate failed with error %d."*/
			}
			if (null_ind == -1)
				rows = 0;
			if (rows > 0)
				STprintf(column_value[i], ERx("%d"), tmp_int);
		}
		else if (STequal(column_datatype[i], ERx("float")) ||
			STequal(column_datatype[i], ERx("money")))
		{
			STprintf(tmp_statement,
				ERx("SELECT FLOAT8(%s) FROM %s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
				dlm_column_name, sha_name, database_no,
				transaction_id, sequence_no);
			EXEC SQL EXECUTE IMMEDIATE :tmp_statement
				INTO	:tmp_float:null_ind;
			EXEC SQL INQUIRE_SQL (:rows = ROWCOUNT, :err = ERRORNO);
			if (err)	{
				/* "CHECK_INSERT:  Float execute immediate failed with error %d." */
				//wsprintf(tmp,ResourceString(IDS_F_FLOAT_EXEC_IMMEDIATE),err);
				//TS_MessageBox(TS_GetFocus(),tmp, NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);

				lpError->iErrorID = RES_E_FLOAT_EXEC_IMMEDIATE;
				lpError->iParam1 = err;
				return (RES_E_FLOAT_EXEC_IMMEDIATE);/* "CHECK_INSERT:  Float execute immediate failed with error %d." */
			}
			if (null_ind == -1)
				rows = 0;
			if (rows > 0)
				STprintf(column_value[i], ERx("%.13g"),
					tmp_float, decimal_char);
		}
		else
		{
			STprintf(tmp_statement,
				ERx("SELECT VARCHAR(MAX(%s)) FROM %s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
				dlm_column_name, sha_name, database_no,
				transaction_id, sequence_no);
			EXEC SQL EXECUTE IMMEDIATE :tmp_statement
				INTO	:tmp_vch:null_ind;
			EXEC SQL INQUIRE_SQL (:rows = ROWCOUNT, :err = ERRORNO);
			if (err)	{
				/* "CHECK_INSERT:  Char execute immediate failed with error %d." */
				//wsprintf(tmp,ResourceString(IDS_ERR_CHAR_EXECUTE_IMMEDIATE),err);
				//TS_MessageBox(TS_GetFocus(),tmp, NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);

				lpError->iErrorID = RES_E_CHAR_EXECUTE_IMMEDIATE;
				lpError->iParam1 = err;
				return (RES_E_CHAR_EXECUTE_IMMEDIATE);
			}
			if (null_ind == -1)
				rows = 0;
			if (rows > 0)
			{
				tmp_vch.tmp_char[tmp_vch.tmp_len] = EOS;
				VdbaColl_apostrophe(tmp_vch.tmp_char);
				STprintf(column_value[i], ERx("'%s'"),
					tmp_vch.tmp_char);
			}
		}
		if (where_clause)
		{
			if (i > 0)
				STcat(where_clause, ERx(" AND "));
			STcat(where_clause, ERx("t."));
			STcat(where_clause, dlm_column_name);
			if (rows == 0)
			{
				STcat(where_clause, ERx(" IS NULL "));
			}
			else
			{
				STcat(where_clause, ERx(" = "));
				STcat(where_clause, column_value[i]);
			}
		}
		if (name_clause)
		{
			if (i > 0)
			{
				STcat(name_clause, ERx(", "));
			}
			STcat(name_clause, dlm_column_name);
		}
		if (insert_value_clause)
		{
			if (i > 0)
			{
				STcat(insert_value_clause, ERx(", "));
			}
			STcat(insert_value_clause, column_value[i]);
		}
		if (update_clause)
		{
			if (i > 0)
			{
				STcat(update_clause, ERx(", "));
			}
			STcat(update_clause, dlm_column_name);
			STcat(update_clause, ERx(" = "));
			STcat(update_clause, column_value[i]);
		}
	}

	return (RES_SUCCESS);
}


/*{
** Name:	VdbaColl_apostrophe - double apostrophes in a string
**
** Description:
**	  Doubles apostrophes in a null-termilonged string, except leading
**	  and trailing.
**	  eg,	 'O'Toole, Peter' --> 'O''Toole, Peter'
**		  'I don't know how it's done' --> 'I don''t know how it''s done'
**
** Assumptions:
**	  The 'string' parameter is expected to be a buffer with enough
**	  space to duplicate the apostrophes present in the string,
**	  potentially almost twice as long as the original.
**	  Also, 'string' is presumed to have leading and trailing
**	  apostrophes, since they are not looked at.
**
** Inputs:
**	  string - pointer to a null-termilonged string, in a buffer that
**		  has extra space to expand any apostrophes in the string
**
** Outputs:
**	  string - pointer to the same null-termilonged string, with all
**		  original apostrophes duplicated
**
** Returns:
**	  none
**
** Side effects:
**	  The size of 'string' is n characters longer than the original,
**	  where n is the number of apostrophes in the original.
*/
static void VdbaColl_apostrophe(char  *string)
{
	char   tmp[1000];
	char   *p;
	char   *apos = ERx("\'");

	for (p = string; *p != EOS; CMnext(p))
	{
		if (CMcmpcase(p, apos) == 0)
		{
			CMnext(p);
			STcopy(p, tmp);
			STcopy(apos, p);
			STcat(p, tmp);
		}
	}
}


/*{
** Name:  LIBMON_VdbaColl_tbl_fetch - fetch table information
**
** Description:
**  Retrieve information on a table and construct the names of all related
**  objects
**
** Inputs:
**  able_no    - table number
**  force       - force the information to be refreshed
**
** Outputs:
**  lpError    - if error found
**  pRmTbl     - fill pRmTbl structure
**
** Returns:
**  RES_SUCCESS or ...
*/
long LIBMON_VdbaColl_tbl_fetch(long table_no,int force, LPERRORPARAMETERS lpError, TBLDEF *pRmTbl)
{
	DBEC_ERR_INFO errinfo;
	int err;
	COLDEF *col_p;
	TBLDEF *CurTbl;
	EXEC SQL BEGIN DECLARE SECTION;
		short   null_ind = 0;
		char    stmt[2048];
		long    cdds_tbl_exists;
		i4 page_size;
		i2 target_type;
		i4 ncols;

		char table_name[DB_MAXNAME+1];
		char table_owner[DB_MAXNAME+1];
		i2 cdds_no;
		char columns_registered[26];
		char supp_objs_created[26];
		char rules_created[26];
		char cdds_lookup_table[DB_MAXNAME+1];
		char prio_lookup_table[DB_MAXNAME+1];
		char index_used[DB_MAXNAME+1];
		long nTbNo;

		char column_name2[DB_MAXNAME+1];
		char column_datatype2[DB_MAXNAME+1];
		long column_length2;
		long column_scale2;
		long column_sequence2;
		long key_sequence2;
		char column_nulls2[2];
		char column_defaults2[2];
	EXEC SQL END DECLARE SECTION;
	nTbNo = table_no;

	if (!pRmTbl)
		return (RES_ERR);

	if (pRmTbl->table_no == table_no && !force)
	{
		//pRmTbl = &tbl;
		return (RES_SUCCESS);
	}
	if (pRmTbl->ncols > 0 && pRmTbl->col_p != NULL)
	{
		ESL_FreeMem (pRmTbl->col_p);
		pRmTbl->ncols = 0;
	}
	CurTbl = pRmTbl;
	CurTbl->table_no = table_no;

	/* Populate tbldef based on table_no */
	EXEC SQL REPEATED SELECT TRIM(table_name), TRIM(table_owner),
	        cdds_no, columns_registered,
	        supp_objs_created, rules_created,
	        TRIM(cdds_lookup_table), TRIM(prio_lookup_table),
	        TRIM(index_used)
	    INTO
	        :table_name, 
	        :table_owner,
	        :cdds_no, 
	        :columns_registered,
	        :supp_objs_created, 
	        :rules_created,
	        :cdds_lookup_table, 
	        :prio_lookup_table,
	        :index_used
	    FROM    dd_regist_tables
	    WHERE   table_no = :nTbNo;

	CurTbl->cdds_no = cdds_no;
	x_strcpy(CurTbl->table_name, table_name);
	x_strcpy(CurTbl->table_owner, table_owner);
	x_strcpy(CurTbl->columns_registered, columns_registered);
	x_strcpy(CurTbl->supp_objs_created, supp_objs_created);
	x_strcpy(CurTbl->rules_created, rules_created);
	x_strcpy(CurTbl->cdds_lookup_table, cdds_lookup_table);
	x_strcpy(CurTbl->prio_lookup_table, prio_lookup_table);
	x_strcpy(CurTbl->index_used, index_used);

	if (Vdba_error_check(DBEC_SINGLE_ROW, &errinfo) != RES_SUCCESS) {
		if (errinfo.errorno)
		{
			lpError->iErrorID = RES_E_SQL;
			lpError->iParam1  = errinfo.errorno;
			return (RES_E_SQL);
		}
		else
		{
			lpError->iErrorID = RES_E_TBL_NUMBER;
			lpError->iParam1 = CurTbl->cdds_no;
			return (RES_E_TBL_NUMBER); //"TBL_FETCH:  Table number %d not found in dd_regist_tables."
		}
	}

	VdbaColl_edit_name(EDNM_DELIMIT, CurTbl->table_name, CurTbl->dlm_table_name);
	VdbaColl_edit_name(EDNM_DELIMIT, CurTbl->table_owner, CurTbl->dlm_table_owner);

	suppspace(CurTbl->columns_registered);
	suppspace(CurTbl->supp_objs_created);
	suppspace(CurTbl->rules_created);

	EXEC SQL REPEATED SELECT table_pagesize
	    INTO    :page_size
	    FROM    iitables
	    WHERE   LOWERCASE(table_name) = LOWERCASE(:table_name)
	    AND LOWERCASE(table_owner) = LOWERCASE(:table_owner);

	CurTbl->page_size = page_size;

	if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
	{
		lpError->iErrorID = RES_E_SQL;
		lpError->iParam1  = errinfo.errorno;
		return (RES_E_SQL);
	}

	/*
	** If the base table doesn't exist, we're presumably being called from
	** MoveConfig (which still moves everything in sight).  Set target_type
	** to URO and return.
	*/
	if (!errinfo.rowcount)
	{
		CurTbl->target_type = REPLIC_UNPROT_RO; //TARG_UNPROT_READ;
		CurTbl->ncols = 0;
		CurTbl->col_p = col_p = NULL;
		return (RES_SUCCESS);
	}

	cdds_tbl_exists = FALSE;
	if (*CurTbl->cdds_lookup_table != EOS)
	{
		EXEC SQL REPEATED SELECT COUNT(*)
		    INTO    :cdds_tbl_exists
		    FROM    iitables
		    WHERE   LOWERCASE(table_name) = LOWERCASE(:cdds_lookup_table)
		    AND     LOWERCASE(table_owner) = LOWERCASE(:table_owner);
		if (Vdba_error_check(0, &errinfo) != RES_SUCCESS)
		{
			lpError->iErrorID = RES_E_SQL;
			lpError->iParam1  = errinfo.errorno;
			return (RES_E_SQL);
		}
	}

	if (cdds_tbl_exists)
	{
		wsprintf(stmt,
		    "SELECT MIN(target_type) FROM dd_db_cdds c, dd_databases d,"
		    "%s l WHERE c.cdds_no = l.cdds_no AND c.database_no = "
		    "d.database_no AND d.local_db = 1", CurTbl->cdds_lookup_table);
		EXEC SQL EXECUTE IMMEDIATE :stmt
		    INTO :target_type:null_ind;

		CurTbl->target_type = target_type;
	}
	else
	{
		cdds_no = CurTbl->cdds_no;
		EXEC SQL REPEATED SELECT c.target_type INTO :target_type
		    FROM   dd_db_cdds c, dd_databases d
		    WHERE  c.cdds_no = :cdds_no
		    AND c.database_no = d.database_no
		    AND d.local_db = 1;

		CurTbl->target_type = target_type;
	}
	if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
	{
		lpError->iErrorID = RES_E_SQL;
		lpError->iParam1  = errinfo.errorno;
		return (RES_E_SQL);
	}
	else if (errinfo.rowcount == 0 || null_ind == -1)
	{
		/*
		** The table happens to exist in this database but the
		** dd_db_cdds catalog or lookup table don't expect it to
		** be here.  Warn the user and specify the CDDS as URO
		** so we won't create support objects.
		*/
		//"TBLFETCH:  The relationship between CDDS number %d and the "
		//"local database has not been established.  Please use CDDS/Databases to define "
		//"it."
		CurTbl->target_type = REPLIC_UNPROT_RO; //TARG_UNPROT_READ;
		lpError->iParam1 = CurTbl->cdds_no;
		lpError->iErrorID = RES_E_F_RELATIONSHIP;
		return (RES_E_F_RELATIONSHIP);
	}

	EXEC SQL REPEATED SELECT COUNT(*) INTO :ncols
		FROM	dd_regist_columns
		WHERE	table_no = :nTbNo;

	CurTbl->ncols = ncols;

	if (Vdba_error_check(DBEC_SINGLE_ROW, &errinfo) != RES_SUCCESS)
	{
		lpError->iErrorID = RES_E_SQL;
		lpError->iParam1  = errinfo.errorno;
		return (RES_E_SQL);
	}

	if (CurTbl->ncols == 0)
	{
		//"Table '%s.%s' has no column registration.  A table cannot be"
		//"processed for replication without first registering it."
		lpError->iErrorID = RES_E_NO_COLUMN_REGISTRATION;
		lstrcpy(lpError->tcParam1,CurTbl->table_owner);
		lstrcpy(lpError->tcParam2,CurTbl->table_name);
		return (RES_E_NO_COLUMN_REGISTRATION);
	}

		err = VdbaColl_tblobj_name(CurTbl->table_name, CurTbl->table_no,TBLOBJ_ARC_TBL,
		                           CurTbl->arc_name, lpError);
		if (err != RES_SUCCESS)
			return (err);
		err = VdbaColl_tblobj_name(CurTbl->table_name, CurTbl->table_no,TBLOBJ_SHA_TBL,
		                           CurTbl->sha_name,lpError);
		if ( err!= RES_SUCCESS)
			return (err);
		err = VdbaColl_tblobj_name(CurTbl->table_name, CurTbl->table_no, TBLOBJ_REM_DEL_PROC,
		                           CurTbl->rem_del_proc,lpError);
		if ( err!= RES_SUCCESS)
			return (err);

		err = VdbaColl_tblobj_name(CurTbl->table_name, CurTbl->table_no,TBLOBJ_REM_UPD_PROC,
		                           CurTbl->rem_upd_proc,lpError);
		if (err!= RES_SUCCESS)
			return (err);

		err = VdbaColl_tblobj_name(CurTbl->table_name, CurTbl->table_no,TBLOBJ_REM_INS_PROC,
		                           CurTbl->rem_ins_proc,lpError);
		if ( err != RES_SUCCESS)
			return (err);

	col_p = (COLDEF *)ESL_AllocMem((u_i4)CurTbl->ncols * sizeof(COLDEF));
	if (col_p == NULL)
	{
		lpError->iErrorID = RES_E_CANNOT_ALLOCATE_MEMORY;
		return (RES_E_CANNOT_ALLOCATE_MEMORY);
	}
	CurTbl->col_p = col_p;
	nTbNo = CurTbl->table_no;
	x_strcpy(table_name, CurTbl->table_name);
	x_strcpy(table_owner, CurTbl->table_owner);

	EXEC SQL REPEATED SELECT TRIM(c.column_name),
	        LOWERCASE(TRIM(c.column_datatype)),
	        c.column_length, c.column_scale,
	        d.column_sequence, d.key_sequence,
	        c.column_nulls, c.column_defaults
	    INTO
	        :column_name2,
	        :column_datatype2,
	        :column_length2, 
	        :column_scale2,
	        :column_sequence2, 
	        :key_sequence2,
	        :column_nulls2, 
	        :column_defaults2
	    FROM    iicolumns c, dd_regist_columns d
	    WHERE   LOWERCASE(c.table_name) = LOWERCASE(:table_name)
	    AND LOWERCASE(c.table_owner) = LOWERCASE(:table_owner)
	    AND LOWERCASE(c.column_name) = LOWERCASE(d.column_name)
	    AND d.table_no = :nTbNo
	    ORDER   BY d.column_sequence;
	EXEC SQL BEGIN;
		x_strcpy (col_p->column_name, column_name2);
		x_strcpy (col_p->column_datatype, column_datatype2);
		col_p->column_length = column_length2;
		col_p->column_scale = column_scale2;
		col_p->column_sequence = column_sequence2;
		col_p->key_sequence = key_sequence2;
		x_strcpy (col_p->column_nulls, column_nulls2);
		x_strcpy (col_p->column_defaults, column_defaults2);

		VdbaColl_edit_name(EDNM_DELIMIT, col_p->column_name,
		                   col_p->dlm_column_name);
		col_p++;
		if ((col_p - CurTbl->col_p) >= CurTbl->ncols)
			EXEC SQL ENDSELECT;
	EXEC SQL END;
	if (Vdba_error_check(0, &errinfo) != RES_SUCCESS)
	{
		if (errinfo.errorno)
		{
			lpError->iErrorID = RES_E_SQL;
			lpError->iParam1  = errinfo.errorno;
			return (RES_E_SQL);
		}
		else
		{
			//"TBLFETCH:  Inconsistent count of registered column for table '%s'."
			//wsprintf(tmp,ResourceString(IDS_ERR_INCONSISTENT_COUNT),CurTbl->table_name);
			//TS_MessageBox(TS_GetFocus(),tmp, NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);
			lpError->iErrorID = RES_E_INCONSISTENT_COUNT;
			lstrcpy(lpError->tcParam1, CurTbl->table_name);
			return (RES_E_INCONSISTENT_COUNT); //"TBLFETCH:  Inconsistent count of registered column for table '%s'."
		}
	}
	return (RES_SUCCESS);
}




int GetSessionNumber4DbNum(int DbNo ,GLOBALSESSION *pGlobSess)
{
	DBSESSION  *pTempSess;

	pTempSess = pGlobSess->pSession;

	for (pTempSess++; (pTempSess - pGlobSess->pSession) < pGlobSess->NumSessions; pTempSess++){
		if (pTempSess->database_no == DbNo){
		return pTempSess->iSession;
		}
	}
	return -1;
}

static int InitializeCollisionList(GLOBALCOLLISIONLIST *pCollisionList)
{
	pCollisionList->pGlobCollisionList = ESL_AllocMem(INCREMENTCOLLISIONLIST * sizeof(COLLISION_FOUND));
	if (pCollisionList->pGlobCollisionList == NULL)
	{
		return RES_E_CANNOT_ALLOCATE_MEMORY;
	}
	else
		pCollisionList->nNbCollisionAllocated = INCREMENTCOLLISIONLIST;
	return RES_SUCCESS;
}

static COLLISION_FOUND *ReallocCollisionList( int nCurIndice ,GLOBALCOLLISIONLIST *pCollList)
{
	if (nCurIndice > ( pCollList->nNbCollisionAllocated - 1 ))
	{
		int nOldSize = pCollList->nNbCollisionAllocated * sizeof(COLLISION_FOUND);
		int nNewSize = nOldSize + INCREMENTCOLLISIONLIST * sizeof(COLLISION_FOUND);
		COLLISION_FOUND *pCur = ESL_ReAllocMem(pCollList->pGlobCollisionList,nNewSize,nOldSize);
		if (pCur)
		{
			pCollList->nNbCollisionAllocated += INCREMENTCOLLISIONLIST;
			pCollList->pGlobCollisionList = pCur;
		}
		else
		{
			if (!pCollList->pGlobCollisionList)
				ESL_FreeMem(pCollList->pGlobCollisionList);
			pCollList->pGlobCollisionList = NULL;
			pCollList->nNbCollisionAllocated = 0;
		}
	}
	return pCollList->pGlobCollisionList;
}

static void FreeCollisionList(GLOBALCOLLISIONLIST *pCurCollList)
{
	if (pCurCollList->pGlobCollisionList != NULL)
		ESL_FreeMem(pCurCollList->pGlobCollisionList);
	pCurCollList->pGlobCollisionList = NULL;
	pCurCollList->nNbCollisionAllocated = 0;
}


static int GetDBVNode4DbNum(int DbNo,char *dbname,char *vnodename,GLOBALSESSION *pGlobSess)
{
	DBSESSION  *pTempSess;

	for (pTempSess = pGlobSess->pSession;
	        (pTempSess - pGlobSess->pSession) < pGlobSess->NumSessions; pTempSess++) 
	{
		if (pTempSess->database_no == DbNo){
			lstrcpy(dbname,pTempSess->database_name);
			lstrcpy(vnodename,pTempSess->vnode_name);
		return 1;
		}
	}
	return 0;
}

/*{
** Name:	Vdba_GetValidVnodeName - Verify the vnode name with the local vnode
**
** Description:
**		Verify the vnode name with the local vnode.
**			if lpVNode is equal to the local vnode connect with the vnode name (local) only
**				if the lpVNode is not in vnode list.
**			if the lpVNode is in the list return lpVNode.
**
** Inputs:
**		lpVNode : Vnode name 
**
** Outputs:
**		lpValidVnode : vnode name specific for Vdba
**
** Returns:
**		void
**/

void Vdba_GetValidVnodeName(LPUCHAR lpVNode,LPUCHAR lpValidVnode)
{
	STcopy(lpVNode,lpValidVnode);
	return;

/*	NODEDATAMIN nodedata;
	int ires;
	char BaseHostName[MAXOBJECTNAME];
	char CurVnodeName[MAXOBJECTNAME];
	char *pDest;


	memset (BaseHostName,'\0',MAXOBJECTNAME);
	memset (CurVnodeName,'\0',MAXOBJECTNAME);

	pDest = x_strstr(lpVNode,ResourceString((UINT)IDS_I_LOCALNODE));
	if (pDest == lpVNode)	{
		STcopy(lpVNode,lpValidVnode);
		return;
	}

	// Get local vnode
	GChostname (BaseHostName,MAXOBJECTNAME);
	if (*BaseHostName == EOS)	{//"Local Vnode Name not found"
		TS_MessageBox(TS_GetFocus(),ResourceString(IDS_ERR_VNODE_NOT_FOUND),
									NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);
		return;
	}

	if (STequal(BaseHostName,lpVNode) == 0)  {
		// lpVirtNode is different of BaseHostName. 
		STcopy(lpVNode,lpValidVnode);
		return;
	}
	else	{
		// String Equal : Change lpVirtNode by "(local)" only
		//   if the vnode name is not in vnode list.
		//   if the node name is in the list return this vnode name
		STcopy(ResourceString((UINT)IDS_I_LOCALNODE),lpValidVnode);
		ires=GetFirstMonInfo(0,0,NULL,OT_NODE,(void * )&nodedata,NULL);
		while (ires==RES_SUCCESS) {
			if (nodedata.igwtype==GWTYPE_NONE) { // only Ingres Nodes
				if (STequal(nodedata.NodeName,BaseHostName) != 0)	{
					STcopy(BaseHostName,lpValidVnode);
					break;
				}
			}
			ires=GetNextMonInfo((void * )&nodedata);
		}
	}
*/
}


int  OpenNodeStruct4Replication  (LPUCHAR lpVirtNode)
{
	char CurrentVnodeName[MAX_VNODE_NAME];

	memset (CurrentVnodeName,'\0',MAX_VNODE_NAME);

	Vdba_GetValidVnodeName(lpVirtNode,CurrentVnodeName);
	if (*CurrentVnodeName == EOS)
		return LIBMON_OpenNodeStruct (lpVirtNode);
	else
		return LIBMON_OpenNodeStruct (CurrentVnodeName);
}

int Getsession4Replication( UCHAR *lpsessionname,int sessiontype,int * SessNum)
{
	char CurrentVnodeName[MAX_CONNECTION_NAME];
	char CurrentDBName[MAXOBJECTNAME];
	char tmpVnode[MAXOBJECTNAME];
	char tmp[MAX_CONNECTION_NAME];

	memset (CurrentVnodeName,'\0',MAX_CONNECTION_NAME);
	memset (CurrentDBName,'\0',MAXOBJECTNAME);
	memset (tmpVnode,'\0',MAXOBJECTNAME);
	if ( GetNodeAndDB(CurrentVnodeName,CurrentDBName,lpsessionname) != RES_ERR)
		Vdba_GetValidVnodeName(CurrentVnodeName,tmpVnode);
	if (*tmpVnode != EOS)
		STprintf (tmp,"%s::%s",tmpVnode,CurrentDBName);
	else
		STcopy(lpsessionname,tmp);

	return Getsession( tmp,sessiontype, SessNum);
}

static int VdbaCollRetrieveEnd ( TBLDEF *pTbl , GLOBALSESSION *pGlobSess,GLOBALCOLLISIONLIST *pTemp )
{
	if ( ReleaseSession(pGlobSess->initial_session,RELEASE_ROLLBACK) == RES_ERR )
		return RES_E_RELEASE_SESSION; //IDS_ERR_RELEASE_SESSION

	VdbaColl_Close_Sessions(pGlobSess);
	if (pTbl->ncols > 0 && pTbl->col_p != NULL)	{
		ESL_FreeMem (pTbl->col_p);
		pTbl->ncols = 0;
	}
	memset(pTbl,0,sizeof(TBLDEF));

	FreeCollisionList(pTemp);
	return RES_SUCCESS;
}

void LIBMON_VdbaCollFreeMemory( LPRETVISUALINFO lpCurVisual )
{
	int i;
	if (!lpCurVisual->lpRetVisualSeq)
		return;
	if (lpCurVisual->iNumVisualSequence)
	{
		for (i = 0; i < lpCurVisual->iNumVisualSequence ;i++)
		{
			VISUALCOL *lptmpColumn;
			lptmpColumn = lpCurVisual->lpRetVisualSeq[i].VisualCol;
			if (lptmpColumn)
				ESL_FreeMem(lptmpColumn);
		}
		ESL_FreeMem(lpCurVisual->lpRetVisualSeq);
		lpCurVisual->lpRetVisualSeq = NULL;
		lpCurVisual->iNumVisualSequence = 0;
	}
}

/*{
** Name:  LIBMON_VdbaColl_Retrieve_Collision - fill structure LPRETVISUALINFO with collision.
**
** Description:
**    Check the records in the dd_distrib_queue table for
**    collisions in their target database. Fill LPRETVISUALINFO structure with
**    information about any detected collisions.
**
** Inputs:
**    CurrentNodeStruct     - Current node handle
**    dbname                - name of the database to which the caller is connected
**
** Outputs:
**    fill LPRETVISUALINFO structure lpRetCol
**    fill LPERRORPARAMETERS structure lpError if error found.
**
** Returns:
**    RES_E_OPENING_SESSION
**    RES_E_SELECT_FAILED
**    RES_E_CANNOT_ALLOCATE_MEMORY
*/
#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
int LIBMON_VdbaColl_Retrieve_Collision( int CurrentNodeHdl ,char *dbname ,
                                        LPRETVISUALINFO lpRetCol, LPERRORPARAMETERS lpError)
{
	DBEC_ERR_INFO errinfo;
	EXEC SQL BEGIN DECLARE SECTION;
	char  select_statement[1024];
	long  transaction_id,sequence_no;
	char  srcDBname[DB_MAXNAME+1],srcVnodeName[DB_MAXNAME+1];
	char  vnode_name[DB_MAXNAME+1];
	char  local_owner[DB_MAXNAME+1];
	short sourcedb,local_db_num,srcDBnum; 
	long  err;
	char  target_owner[DB_MAXNAME+1];
	char  dlm_target_owner[DLMNAME_LENGTH+1];
	EXEC SQL END DECLARE SECTION;
	BOOL bError = FALSE;
	BOOL bDba = FALSE;
	int     ires, hdlSess;
	long    collision_count = 0,records = 0,i,column_number;
	LPUCHAR VnodeName;
	UCHAR   szConnectName[MAX_CONNECTION_NAME],where_clause[1024];
	COLDEF  *col_p;
	VISUALCOL *pVCTemp;
	TBLDEF     tbl;
	GLOBALSESSION GlobSession;
	GLOBALCOLLISIONLIST CollisionList;

	memset(&CollisionList, 0 ,sizeof (GLOBALCOLLISIONLIST));
	memset(&GlobSession, 0 ,sizeof (GLOBALSESSION));
	memset(&tbl, 0 ,sizeof (TBLDEF));
	tbl.table_no = -1;
	memset(lpError, 0 ,sizeof (ERRORPARAMETERS));
	lpError->iErrorID = RES_SUCCESS;
	RMadf_cb = FEadfcb();

	VnodeName = GetVirtNodeName (CurrentNodeHdl);
	x_strcpy(vnode_name,VnodeName);

	GlobSession.initial_HnodeStruct = CurrentNodeHdl;

	wsprintf(szConnectName,"%s::%s",VnodeName, dbname);
	ires = Getsession4Replication(szConnectName,SESSION_TYPE_INDEPENDENT, &GlobSession.initial_session);
	if (ires != RES_SUCCESS)  {
		ManageSqlError(&sqlca);
		lstrcpy( lpError->tcParam1,szConnectName);
		lpError->iErrorID = RES_E_OPENING_SESSION;
		return RES_E_OPENING_SESSION;
	}
	EXEC SQL REPEATED SELECT database_no INTO :local_db_num
	    FROM dd_databases
	    WHERE local_db = 1;
	if (Vdba_error_check(DBEC_SINGLE_ROW, &errinfo) != RES_SUCCESS) {
		VdbaCollRetrieveEnd ( &tbl , &GlobSession,&CollisionList);
		lpError->iErrorID = RES_E_SELECT_FAILED;
		return RES_E_SELECT_FAILED;
	}

	err = InitializeCollisionList(&CollisionList);
	if ( err != RES_SUCCESS)
	{
		VdbaCollRetrieveEnd ( &tbl ,&GlobSession,&CollisionList);
		lpError->iErrorID = err;
		return err; // RES_E_CANNOT_ALLOCATE_MEMORY
	}

	EXEC SQL SELECT DBMSINFO('DBA') INTO :local_owner;

	err = VdbaColl_Get_Collisions(dbname, vnode_name,(SHORT)local_db_num ,local_owner, &collision_count,
	                              &records, lpError,&tbl,&GlobSession,&CollisionList);
	if (err != RES_SUCCESS)
	{
		VdbaCollRetrieveEnd (&tbl, &GlobSession,&CollisionList);
		return err; 
	}

	if (collision_count)
	{
		lpRetCol->lpRetVisualSeq = ESL_AllocMem( collision_count * sizeof(STOREVISUALINFO));
		if (!lpRetCol->lpRetVisualSeq)
		{
			VdbaCollRetrieveEnd (&tbl, &GlobSession,&CollisionList);
			lpError->iErrorID = RES_E_CANNOT_ALLOCATE_MEMORY;
			return RES_E_CANNOT_ALLOCATE_MEMORY;
		}
		lpRetCol->iNumVisualSequence = collision_count;
	}
	else
		lpError->iErrorID = RES_SUCCESS;


	for (i = 0; i < collision_count; i++)
	{
		COLLISION_FOUND *pCollisionList = CollisionList.pGlobCollisionList;
		VISUALCOL *VisualColTmp;
		int iret;
		ActivateSession(GlobSession.initial_session);
		err = LIBMON_VdbaColl_tbl_fetch(pCollisionList[i].db1.table_no, FALSE,lpError,&tbl);
		if (err != RES_SUCCESS)
		{
			VdbaCollRetrieveEnd ( &tbl , &GlobSession,&CollisionList);
			return err; 
		}

		bDba = STequal(tbl.table_owner, local_owner);
		//iRetSeq = 0;
		////////////////iRetSeq = AddTransaction (pCurClass,pCollisionList[i].db1.transaction_id);
		//if (!iRetSeq)
		//	break;
		lstrcpy(lpRetCol->lpRetVisualSeq[i].VisualSeq.table_name,tbl.table_name);
		lstrcpy(lpRetCol->lpRetVisualSeq[i].VisualSeq.table_owner,tbl.table_owner);
		lpRetCol->lpRetVisualSeq[i].VisualSeq.cdds_no = tbl.cdds_no;
		lpRetCol->lpRetVisualSeq[i].VisualSeq.type = pCollisionList[i].type;
		lpRetCol->lpRetVisualSeq[i].VisualSeq.transaction_id = pCollisionList[i].db1.transaction_id;
		lpRetCol->lpRetVisualSeq[i].VisualSeq.old_transaction_id = pCollisionList[i].db1.old_transaction_id;
		lpRetCol->lpRetVisualSeq[i].VisualSeq.sequence_no = pCollisionList[i].db1.sequence_no;
		lpRetCol->lpRetVisualSeq[i].VisualSeq.nb_Col = tbl.ncols;
		lstrcpy(lpRetCol->lpRetVisualSeq[i].VisualSeq.SourceTransTime,pCollisionList[i].db1.TransTime);
		lstrcpy(lpRetCol->lpRetVisualSeq[i].VisualSeq.TargetTransTime,pCollisionList[i].db2.TransTime);
		lpRetCol->lpRetVisualSeq[i].VisualSeq.nTblNo = tbl.table_no;
		lpRetCol->lpRetVisualSeq[i].VisualSeq.nSourceDB = pCollisionList[i].db1.sourcedb;
		lpRetCol->lpRetVisualSeq[i].VisualSeq.nSvrTargetType = pCollisionList[i].nSvrTargetType;
		lpRetCol->lpRetVisualSeq[i].VisualSeq.SourceResolvedCollision = pCollisionList[i].db1.nResolvedCollision;

		iret = GetDBVNode4DbNum(pCollisionList[i].local_db,
		                        lpRetCol->lpRetVisualSeq[i].VisualSeq.Localdb,
		                        lpRetCol->lpRetVisualSeq[i].VisualSeq.LocalVnode,
		                        &GlobSession);
		if ( !iret )
		{
			lstrcpy(lpRetCol->lpRetVisualSeq[i].VisualSeq.Localdb,"");
			lstrcpy(lpRetCol->lpRetVisualSeq[i].VisualSeq.LocalVnode,"");
		}

		iret = GetDBVNode4DbNum(pCollisionList[i].db1.sourcedb,
		                        lpRetCol->lpRetVisualSeq[i].VisualSeq.Sourcedb,
		                        lpRetCol->lpRetVisualSeq[i].VisualSeq.SourceVnode,
		                        &GlobSession);
		if ( !iret )
		{
			srcDBnum = pCollisionList[i].db1.sourcedb;
			EXEC SQL REPEATED SELECT database_name,vnode_name INTO :srcDBname,:srcVnodeName 
				FROM dd_databases
				WHERE database_no = :srcDBnum;
			if (Vdba_error_check(DBEC_SINGLE_ROW, &errinfo) != RES_SUCCESS)
			{
				lstrcpy(lpRetCol->lpRetVisualSeq[i].VisualSeq.Sourcedb,"");
				lstrcpy(lpRetCol->lpRetVisualSeq[i].VisualSeq.SourceVnode,"");
			}
			else
			{
				suppspace(srcDBname);
				suppspace(srcVnodeName);
				lstrcpy(lpRetCol->lpRetVisualSeq[i].VisualSeq.Sourcedb,srcDBname);
				lstrcpy(lpRetCol->lpRetVisualSeq[i].VisualSeq.SourceVnode,srcVnodeName);
			}
		}

		iret = GetDBVNode4DbNum(pCollisionList[i].remote_db,lpRetCol->lpRetVisualSeq[i].VisualSeq.Targetdb,lpRetCol->lpRetVisualSeq[i].VisualSeq.TargetVnode,&GlobSession);
		if ( !iret )
		{
			lstrcpy(lpRetCol->lpRetVisualSeq[i].VisualSeq.Targetdb,"");
			lstrcpy(lpRetCol->lpRetVisualSeq[i].VisualSeq.TargetVnode,"");
		}

		VisualColTmp = (VISUALCOL *) ESL_AllocMem((u_i4)tbl.ncols *  sizeof(VISUALCOL));
		lpRetCol->lpRetVisualSeq[i].iNumVisualColumns = tbl.ncols ;
		if (!VisualColTmp )
		{
			VdbaCollRetrieveEnd (&tbl ,&GlobSession,&CollisionList);
			lpError->iErrorID = RES_E_CANNOT_ALLOCATE_MEMORY;
			return RES_E_CANNOT_ALLOCATE_MEMORY;
		}

		// Get Colonne info
		//pVisualCol = (VISUALCOL *)ESL_AllocMem((u_i4)tbl.ncols * sizeof(VISUALCOL));
		//if (pVisualCol == NULL)
		//{
		//	VdbaCollRetrieveEnd ();
		//	return RES_E_CANNOT_ALLOCATE_MEMORY;
		//}

		for (column_number = 0, col_p = tbl.col_p, pVCTemp = VisualColTmp ;
		     column_number < tbl.ncols; column_number++, *col_p++,*pVCTemp++)
		{
			lstrcpy(pVCTemp->column_name ,col_p->column_name);
			pVCTemp->key_sequence = col_p->key_sequence;
			switch (pCollisionList[i].type)
			{
				case TX_INSERT:
				{
				// Get Column data in local database.
				ActivateSession(GlobSession.initial_session);

				lstrcpy(pVCTemp->dataColSource,"");
				err = VdbaColl_GetValue_columns( pCollisionList[i].db1.sourcedb,
				                                 pCollisionList[i].db1.transaction_id,
				                                 pCollisionList[i].db1.sequence_no,
				                                 (char *)NULL,
				                                 pVCTemp->dataColSource,
				                                 col_p,
				                                 tbl.dlm_table_owner,lpError,&tbl);
				if (err != RES_SUCCESS) 
				{
					bError = TRUE;
					break;
				}
				// retrieve column data in remote_db
				err = VdbaColl_Create_Key_Clauses(where_clause, NULL, NULL,
				                                  NULL, tbl.table_name, tbl.table_owner,
				                                  tbl.table_no, tbl.sha_name,
				                                  pCollisionList[i].db1.sourcedb,
				                                  pCollisionList[i].db1.transaction_id,
				                                  pCollisionList[i].db1.sequence_no,lpError);
				if (err != RES_SUCCESS) 
				{
					bError = TRUE;
					break;
				}
				hdlSess = GetSessionNumber4DbNum(pCollisionList[i].remote_db, &GlobSession);
				if (hdlSess == -1)
				{
					bError = TRUE;
					break;
				}
				ActivateSession(hdlSess);
				lstrcpy(pVCTemp->dataColTarget,"");
				if (bDba)
				{
					EXEC SQL SELECT DBMSINFO('DBA') INTO :target_owner;
					VdbaColl_edit_name(EDNM_DELIMIT, target_owner, dlm_target_owner);
				}
				else
					lstrcpy(dlm_target_owner,tbl.dlm_table_owner);
				err = VdbaColl_GetValue_columns( pCollisionList[i].db2.sourcedb,
				                                 pCollisionList[i].db2.transaction_id,
				                                 pCollisionList[i].db2.sequence_no,
				                                 where_clause,
				                                 pVCTemp->dataColTarget,
				                                 col_p, dlm_target_owner,lpError,&tbl);
				if (err != RES_SUCCESS)
					bError = TRUE;
				}
				break;
			case TX_UPDATE:
			case TX_DELETE:
				{
				ActivateSession(GlobSession.initial_session);
				STprintf(select_statement, ERx("SELECT sourcedb, "
				                           "transaction_id, sequence_no FROM %s WHERE sourcedb= %d AND transaction_id = "
				                           "%d AND sequence_no = %d AND in_archive = 0"),
				                           tbl.sha_name,
				                           pCollisionList[i].db1.sourcedb,
				                           pCollisionList[i].db1.transaction_id,
				                           pCollisionList[i].db1.sequence_no);
				EXEC SQL EXECUTE IMMEDIATE :select_statement
				    INTO    :sourcedb, :transaction_id,
				            :sequence_no;
				if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo)!=RES_SUCCESS)
				{
					ManageSqlError(&sqlca);//"select failed."
					lpError->iErrorID = RES_E_SQL;
					bError = TRUE;
					break;
				}
				err = VdbaColl_GetValue_columns( sourcedb, transaction_id,
				                                 sequence_no, (char *)NULL,
				                                 pVCTemp->dataColSource, col_p,
				                                 tbl.table_owner,lpError,&tbl);
				if (err != RES_SUCCESS)
				{
					bError = TRUE;
					break;
				}

				err = VdbaColl_Create_Key_Clauses(where_clause, NULL, NULL,
				                                  NULL, tbl.table_name, tbl.table_owner,
				                                  tbl.table_no, tbl.sha_name, sourcedb,
				                                  transaction_id, sequence_no,lpError);
				if (err != RES_SUCCESS)
				{
					bError = TRUE;
					break;
				}
				hdlSess = GetSessionNumber4DbNum(pCollisionList[i].remote_db,&GlobSession);
				if (hdlSess == -1)
				{
					bError = TRUE;
					break;
				}
				ActivateSession(hdlSess);

				if (bDba)
				{
					EXEC SQL SELECT DBMSINFO('DBA') INTO :target_owner;
					VdbaColl_edit_name(EDNM_DELIMIT, target_owner, dlm_target_owner);
				}
				else
					lstrcpy(dlm_target_owner,tbl.table_owner);

				err = VdbaColl_GetValue_columns( sourcedb, transaction_id,
				                                 sequence_no, where_clause,
				                                 pVCTemp->dataColTarget, col_p,
				                                 dlm_target_owner,lpError,&tbl);
				if (err != RES_SUCCESS)
				{
					bError = TRUE;
					break;
				}

				}
			break;
			}
			lpRetCol->lpRetVisualSeq[i].VisualCol = VisualColTmp;
		}
	}
	VdbaCollRetrieveEnd (&tbl, &GlobSession,&CollisionList);
	return err;
}
#endif

int LIBMON_UpdateTable4ResolveCollision( int nStatusPriority ,
                                  int nSrcDB ,
                                  int nTransactionID ,
                                  int nSequenceNo ,
                                  char *pTblName,
                                  int nTblNo, LPERRORPARAMETERS lpError)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int  nPriority;
		int  nSourceDB;
		int  nTransID;
		int  nSeqNo;
		char szTblShaName[MAXOBJECTNAME];
		char tmp_statement[256];
	EXEC SQL END DECLARE SECTION;

	nPriority   = nStatusPriority;
	nSourceDB   = nSrcDB;
	nTransID    = nTransactionID;
	nSeqNo = nSequenceNo;

	EXEC SQL REPEATED UPDATE dd_distrib_queue SET dd_priority = :nPriority
	         WHERE sourcedb = :nSourceDB 
	         AND   transaction_id = :nTransID
	         AND   sequence_no = :nSeqNo;
	if (sqlca.sqlcode < 0) // || sqlca.sqlcode ==100)
	{
		ManageSqlError(&sqlca);//("Update dd_distrib_queue failed.")
		// MessageWithHistoryButton(GetFocus (),ResourceString(IDS_ERR_UPDATE_DD_DISTRIB_QUEUE));
		lpError->iErrorID = RES_E_UPDATE_DD_DISTRIB_QUEUE;
		return RES_E_UPDATE_DD_DISTRIB_QUEUE;
	}
	// Generate table shadow name.
	if (VdbaColl_tblobj_name( pTblName,nTblNo,TBLOBJ_SHA_TBL,
	                          szTblShaName,lpError) != RES_SUCCESS)
	{
		lpError->iErrorID = RES_E_GENERATE_SHADOW;
		return RES_E_GENERATE_SHADOW; //("Generate the shadow table name failed.")
	}
	wsprintf(tmp_statement, ( "UPDATE %s SET dd_priority = %d "
	                            "WHERE sourcedb = %d "
	                            "AND   transaction_id = %d "
	                            "AND   sequence_no = %d"), szTblShaName,
	                                                       nPriority,
	                                                       nSourceDB,
	                                                       nTransID,
	                                                       nSequenceNo);
	EXEC SQL EXECUTE IMMEDIATE :tmp_statement;
	if (sqlca.sqlcode < 0 ) //|| sqlca.sqlcode ==100)
	{
		ManageSqlError(&sqlca);
		lpError->iErrorID = RES_E_F_SHADOW_TBL;
		lstrcpy(lpError->tcParam1,szTblShaName);
		return RES_E_F_SHADOW_TBL; //"Update %s failed."
	}
	return RES_SUCCESS;
}


