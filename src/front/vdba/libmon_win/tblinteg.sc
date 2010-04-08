/*****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**
**  Project : libmon library for the Ingres Monitor ActiveX control
**
**  Author : Schalk Philippe
**
**  Source:    tblinteg.sc - table integrity report
**
**  History
**
**   04-Dec-2001 (schph01)
**      extracted and adapted from the winmain\tblinteg.sc source
**   24-Mar-2004 (shah03)
**      (sir 112040) removed the usage of "_tcsinc", added "pointer+1" in the same place
**		(sir 112040) removed the usage of "_tccpy", added "x_strcpy" in th same place 
**   02-apr-2009 (drivi01)
**    	In efforts to port to Visual Studio 2008, clean up warnings.
******************************************************************************/

#include <tchar.h>
#include <sys/stat.h>
#include "libmon.h"
#include "replutil.h"
#include "commfunc.h"

EXEC SQL INCLUDE sqlda;

EXEC SQL BEGIN DECLARE SECTION;
char errortext[1024];
EXEC SQL END DECLARE SECTION;

#define MAX_COLS		IISQ_MAX_COLS
#define MAX_REP_COLNAME_WIDTH	DB_MAXNAME
#define MAX_SELECT_SIZE	(MAX_COLS * (MAX_REP_COLNAME_WIDTH + 10)) + 1000

typedef struct _connectinfo
{
    i4   Session1;
    i4   Session2;
    int  hNodeDb1;
    int  hNodeDb2;
    TCHAR OutputFileName[_MAX_PATH];
} CONNECTINFO , FAR * LPCONNECTINFO;

static void WriteErrortextInFile(HFILE CurHdlFile);
static char *end_table_integrity(HFILE *hdlFile,i4 start_session, IISQLDA *sqlda1,
                                 IISQLDA *sqlda2,LPCONNECTINFO lpConnectInf,
                                 LPERRORPARAMETERS lpErrParam,TBLDEF *pCurTbl);

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
static STATUS DatabaseConnect(HFILE CurHdlFile,char *CurNode,char *CurrentDbName ,
                              LPUCHAR DbaOwner,int *hnode ,int *Session,
                              LPTBLINPUTSTRING lpInputStrFormat );
#endif

static int LIBMON_VdbaColl_print_cols( HFILE report_fp, i2  sourcedb, 
                                       long transaction_id,long  sequence_no,
                                       char *target_wc,char *table_owner,
                                       LPTBLINPUTSTRING lpStrFormat,
                                       TBLDEF *pRmTbl);


BOOL Vdba_InitReportFile( HFILE *phdl, char *bufFileName, int bufSize )
{
	HFILE hf;

	if ( ESL_GetTempFileName(bufFileName, bufSize) != RES_SUCCESS ) {
		bufFileName[0] = '\0';
		return FALSE;
	}

	hf = _lcreat (bufFileName, 0);
	if (hf == HFILE_ERROR) {
		bufFileName[0] = '\0';
		return FALSE;
	}
	*phdl = hf;
	return TRUE;
}
BOOL Vdba_WriteBufferInFile(HFILE hdl,char *buftmp,int buflen)
{
	UINT uiresult;
	if ((VOID *)hdl == NULL || buflen == 0)
		return FALSE;
	
	uiresult =_lwrite(hdl, buftmp, buflen);
	if ((int)uiresult!=buflen)
	   return FALSE;
	return TRUE;
}

BOOL Vdba_CloseFile(HFILE *phdl)
{
	HFILE hf;
	if ((VOID *)*phdl == NULL)
		return FALSE;

	hf =_lclose(*phdl);
	if (hf == HFILE_ERROR)
		return FALSE;
	(VOID *)*phdl = NULL;
	return TRUE;
}

ADF_CB *RMadf_cb2 = NULL;
FUNC_EXTERN ADF_CB *FEadfcb(void);


/*{
** Name:	check_indexes - check for unique indexes
**
** Description:
**	Checks to see if indexes force the columns marked as
**	replication keys to be unique.
**
** Inputs:
**	table_name	- the name of the table to be checked
**	table_owner	- the name of the owner
**	table_no	- the table number
**	msg_func	- function to display messages
**
** Outputs:
**	none
**
** Returns:
**	OK - the table has a unique index
**	1527 - Cursor open error
**	1525 - Cursor fetch error
**	-1 - table does not have unique indexes
**/
static STATUS check_indexes( HFILE hdlFile, char *table_name,char *table_owner,i4 table_no,
                             LPTBLINPUTSTRING lpInStrFormat)
#if 0
EXEC SQL BEGIN DECLARE SECTION;
char   *table_name;
char   *table_owner;
i4     table_no;
EXEC SQL END DECLARE SECTION;
#endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	char index_name[DB_MAXNAME+1];
	char index_owner[DB_MAXNAME+1];
	i4   cnt1 = 0;
	i4   cnt2 = 0;
	i4   err = 0;
	i4   last = 0;
	EXEC SQL END DECLARE SECTION;
	char tmpf[1024];

	EXEC SQL DECLARE c3 CURSOR FOR
	    SELECT index_name, index_owner
	    FROM  iiindexes
	    WHERE  base_name = :table_name
	    AND base_owner = :table_owner
	    AND unique_rule = 'U';
	EXEC SQL OPEN c3 FOR READONLY;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO ,:errortext = ERRORTEXT);
	if (err)
	{
		/* open cursor failed */
		/*(*msg_func)(1, 1527, err);*/
		//"CHECK_INDEXES:  Open cursor failed with error -%d."
		//ResourceString(IDS_E_CHECK_INDEXES_OPEN_CURSOR)
		STprintf(tmpf,lpInStrFormat->lpCheckIndexesOpenCursor, err);
		Vdba_WriteBufferInFile(hdlFile,tmpf,x_strlen(tmpf));
		WriteErrortextInFile(hdlFile);
		return (1527);
	}
	while (!last)
	{
		EXEC SQL FETCH c3 INTO :index_name, :index_owner;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT, :last = ENDQUERY);
		if (err)
		{
			EXEC SQL CLOSE c3;
			/* Fetch of index names for table '%s.%s' failed */
			/*(*msg_func)(1, 1525, table_owner, table_name, err);*/
			//"CHECK_INDEXES:  Fetch of index names for table"
			//" '%s.%s' failed with error -%d."ResourceString(IDS_E_FETCH_INDEX)
			STprintf(tmpf,lpInStrFormat->lpFetchIndex,table_owner, table_name, err);
			Vdba_WriteBufferInFile(hdlFile,tmpf,x_strlen(tmpf));
			WriteErrortextInFile(hdlFile);
			return (1525);
		}
		if (!last)
		{
			EXEC SQL REPEATED SELECT COUNT(*) INTO :cnt1
			    FROM  iiindex_columns
			    WHERE   index_name = :index_name
			    AND   index_owner = :index_owner;
			EXEC SQL REPEATED SELECT COUNT(*) INTO :cnt2
			    FROM iiindex_columns i, dd_regist_columns d
			    WHERE i.index_name = :index_name
			    AND i.index_owner = :index_owner
			    AND i.column_name = d.column_name
			    AND d.table_no = :table_no
			    AND d.key_sequence > 0;
			/*
			** this index is unique and only uses key replicator
			** columns
			*/
			if (cnt1 == cnt2)
			{
				EXEC SQL CLOSE c3;
				return (OK);
			}
		}
	}
	EXEC SQL CLOSE c3;
	return (-1);  /*no unique indexes */
}


/*{
** Name:	check_unique_keys - check for unique keys
**
** Description:
**	Checks to see if keys or indexes force the columns marked as
**	replication keys to be unique.
**
** Inputs:
**	table_name	- the name of the table to be checked
**	table_owner	- the name of the owner
**	table_no	- the table number
**	dbname		- the name of the database in which the table is to be
**			  found
**	msg_func	- function to display messages
**
** Outputs:
**	none
**
** Returns:
**	OK - table has unique keys
**	-1 - no unique keys
**	1523 - INGRES error looking for table
**	1524 - INGRES error looking up columns
**	Other error returns from check_indexes (see below)
*/
static STATUS check_unique_keys(HFILE hdlFile,char *table_name,char *table_owner,i4 table_no,
                         char *dbname, LPTBLINPUTSTRING lpInputStrFormat)
#if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*table_name;
char	*table_owner;
i4	table_no;
EXEC SQL END DECLARE SECTION;
#endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	char unique_rule[2];
	char table_indexes[2];
	i4   cnt1 = 0;
	i4   cnt2 = 0;
	i4   err = 0;
	EXEC SQL END DECLARE SECTION;
	char tmp[257];
	char tmpf[1024];

	EXEC SQL REPEATED SELECT unique_rule, table_indexes
	    INTO    :unique_rule, :table_indexes
	    FROM  iitables
	    WHERE   table_name = :table_name
	    AND table_owner = :table_owner;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT, :cnt1 = ROWCOUNT);
	if (err)
	{
		/*TS_MessageBox(TS_GetFocus(),tmp,NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);*/
		/*(*msg_func)(1, 1523, err, table_owner, table_name, dbname);*/
		//"CHECK_UNIQUE_KEYS:  DBMS error -%d looking for table '%s.%s'"
		//" in database %s."ResourceString(IDS_E_CHECK_UNIQUE_KEYS_DBMS_ERROR)
		STprintf(tmpf,lpInputStrFormat->lpCheckUniqueKeys,err, table_owner, table_name, dbname);
		Vdba_WriteBufferInFile(hdlFile,tmpf,x_strlen(tmpf));
		return (1523);
	}
	else if (cnt1 == 0)
	{
		/* table doesn't exist. Probably horizontal partition */
		return (OK);
	}

	if (CMcmpcase(unique_rule, ERx("U")) == 0)
	{
		EXEC SQL REPEATED SELECT COUNT(*) INTO :cnt1
			FROM	iicolumns
			WHERE	table_name = :table_name
			AND	key_sequence > 0
			AND	table_owner = :table_owner;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
		if (err)
		{
			/*(*msg_func)(1, 1524, err, table_owner, table_name,dbname);*/
			//"CHECK_UNIQUE_KEYS:  Error %d looking up columns for table '%s.%s'"
			//" in database %s."ResourceString(IDS_E_LOOKING_UP_COLUMNS)
			STprintf(tmp ,lpInputStrFormat->lpLookingUPColumns,err, table_owner, table_name, dbname);
			Vdba_WriteBufferInFile(hdlFile,tmpf,x_strlen(tmpf));
			return (1524);
		}
		EXEC SQL REPEATED SELECT COUNT(*) INTO :cnt2
			FROM	iicolumns i, dd_regist_columns d
			WHERE	i.column_name = d.column_name
			AND	i.table_name = :table_name
			AND	i.table_owner = :table_owner
			AND	d.table_no = :table_no
			AND	i.key_sequence > 0
			AND	d.key_sequence > 0;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
		if (err)
		{
			/*(*msg_func)(1, 1524, err, table_owner, table_name,dbname);*/
			//"CHECK_UNIQUE_KEYS:  Error %d looking up columns for table '%s.%s'"
			//" in database %s."ResourceString(IDS_E_LOOKING_UP_COLUMNS)
			STprintf(tmpf ,lpInputStrFormat->lpLookingUPColumns ,err, table_owner, table_name, dbname);
			Vdba_WriteBufferInFile(hdlFile,tmpf,x_strlen(tmpf));
			return (1524);
		}
		if (cnt1 == cnt2)	/* base table makes keys unique */
			return (OK);
	}
	if (CMcmpcase(table_indexes, ERx("Y")) == 0)
		return (check_indexes(hdlFile,table_name, table_owner, table_no, lpInputStrFormat));
	return (-1);
}


/*{
** Name:	table_integrity_report - table integrity report
**
** Description:
**  Compares a table in 2 databases with the specified routing
**  in the specified time period (of replication) and reports on
**  discrepancies.
**
** Inputs:
**  db1          - first database number
**  db2          - second database number
**  table_no     - table number
**  cdds_no      - CDDS number
**  begin_time   - begin date/time
**  end_time     - end date/time
**  order_clause - order clause
**
** Outputs:
**  buffer 
**  0 - OK
**  otherwise error
**
** Returns:
** buffer
*/
#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
TCHAR *LIBMON_table_integrity( int   CurrentNodeHdl,
                               TCHAR* CurDbName,
                               short db1,
                               short db2,
                               long  table_no,
                               short cdds_no,
                               TCHAR* begin_time,
                               TCHAR* end_time,
                               TCHAR* order_clause,
                               LPTBLINPUTSTRING lpStringFormat,
                               LPERRORPARAMETERS lpErrorParameters)
#if 0
EXEC SQL BEGIN DECLARE SECTION;
i2  db1;
i2  db2;
i4  table_no;
i2  cdds_no;
EXEC SQL END DECLARE SECTION;
#endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	char dbname1[DB_MAXNAME+1];
	char dbname2[DB_MAXNAME+1];
	i4   err;
	char localowner[DB_MAXNAME+1];
	char targetowner[DB_MAXNAME+1];
	char target_table_owner[DB_MAXNAME+1];
	char dlm_target_table_owner[DLMNAME_LENGTH];
	char select_stmt[MAX_SELECT_SIZE];
	char target_select_stmt[MAX_SELECT_SIZE];
	char target_base_select_stmt[MAX_SELECT_SIZE];
	char save_select_stmt[MAX_SELECT_SIZE];
	char target_save_select_stmt[MAX_SELECT_SIZE];
	i2   database_no1;
	i4   transaction_id1 = 0;
	i4   sequence_no1 = 0;
	i2   database_no2;
	i4   transaction_id2 = 0;
	i4   sequence_no2 = 0;
	i4   column_count;
	char database1[DB_MAXNAME*2+3];
	char database2[DB_MAXNAME*2+3];
	char node1[DB_MAXNAME+1];
	char node2[DB_MAXNAME+1];
	char tmp[1024];
	char base_select_stmt[MAX_SELECT_SIZE];
	char base_where_clause[1024];
	char where_clause[1024];
	short null_ind[MAX_COLS];
	short null_ind2[MAX_COLS];
	i4 start_session = -1;
	i2 target_type;
	i4 cnt;
	i4 done = FALSE;
	EXEC SQL END DECLARE SECTION;

	TBLDEF   CurTbl;
	COLDEF     *col_p;
	CONNECTINFO ConnectInf;
	HFILE hdlfile = (HFILE)NULL;
	i4 records    = 0;
	i4 in_1_not_2 = 0;
	i4 in_2_not_1 = 0;
	i4 different  = 0;
	i4 no_keys    = 1; /* flag for key columns found */
	i4 jj;
	u_i4 i;
	UCHAR tmpf[1024], szConnectName[MAX_CONNECTION_NAME];
	IISQLDA _sqlda1, _sqlda2;
	IISQLDA *sqlda1 = &_sqlda1;
	IISQLDA *sqlda2 = &_sqlda2;
	int ires;
	BOOL bSuccess;
	LPUCHAR VnodeName;
	ConnectInf.Session1 = -1;
	ConnectInf.Session2 = -1;
	ConnectInf.hNodeDb1 = -1;
	ConnectInf.hNodeDb2 = -1;
	start_session = -1;
	ConnectInf.OutputFileName[0] = EOS;

	memset(&CurTbl,0,sizeof(TBLDEF));
	CurTbl.table_no = -1;

	lpErrorParameters->iErrorID = RES_SUCCESS;
	/*
	** Initialize char arrays node1 and dbname1 before using them.
	*/
	MEfill((u_i2)sizeof(node1), '\0', node1);
	MEfill((u_i2)sizeof(node2), '\0', node2);
	MEfill((u_i2)sizeof(dbname1), '\0', dbname1);
	MEfill((u_i2)sizeof(dbname2), '\0', dbname2);

	VnodeName = GetVirtNodeName    (CurrentNodeHdl);
	wsprintf(szConnectName,"%s::%s",VnodeName,CurDbName );
	ires = Getsession4Replication(szConnectName,SESSION_TYPE_INDEPENDENT, &start_session);
	if (ires != RES_SUCCESS)  {
		lpErrorParameters->iErrorID = RES_E_OPENING_SESSION; //IDS_F_OPENING_SESSION
		lstrcpy(lpErrorParameters->tcParam1,szConnectName);
		//STprintf(tmpf,ResourceString(IDS_F_OPENING_SESSION),szConnectName);
		//MessageWithHistoryButton(GetFocus (), tmpf);
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}
	err = LIBMON_VdbaColl_tbl_fetch(table_no, FALSE,lpErrorParameters,&CurTbl);
	if (err != RES_SUCCESS) {
		end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
		return NULL;
	}

	sqlda1->sqln = 0;
	sqlda2->sqln = 0;

	bSuccess = Vdba_InitReportFile(&hdlfile,ConnectInf.OutputFileName, sizeof(ConnectInf.OutputFileName));
	if ( !bSuccess) {
		lpErrorParameters->iErrorID = RES_E_OPEN_REPORT; //IDS_F_OPEN_REPORT
		lstrcpy(lpErrorParameters->tcParam1,ConnectInf.OutputFileName);
		//STprintf(tmp,ResourceString(IDS_F_OPEN_REPORT),OutputFileName);
		//TS_MessageBox(TS_GetFocus(),tmp,NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}

	/*EXEC SQL INQUIRE_SQL (:start_session = SESSION);*/
	if (ActivateSession(start_session) != RES_SUCCESS) {
		/*TS_MessageBox(TS_GetFocus(),tmp,NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);*/
		//"Activate Session  number : %d failed"ResourceString(IDS_E_ACTIV_SESSION)
		//STprintf(tmp,lpActiveSession,start_session);
		//Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		lpErrorParameters->iErrorID = RES_E_ACTIV_SESSION;
		lpErrorParameters->iParam1  = start_session;
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}
	EXEC SQL REPEATED SELECT TRIM(database_name), TRIM(vnode_name)
	    INTO :dbname1, :node1
	    FROM dd_databases
	    WHERE database_no = :db1;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO);
	if (err)
	{
		/* error %d retrieving database name for database %d */
		//"TABLE_INTEGRITY: Error %d retrieving database name for database %d."
		STprintf(tmpf,lpStringFormat->lpResTblIntegError, err, db1);
		Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}
	if (*dbname1 == EOS)
	{
		/* invalid database name %s or node %s for database number %d ResourceString(IDS_E_INVALID_DB)*/
		STprintf(tmp, lpStringFormat->lpInvalidDB, dbname1, node1, db1);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}

	if (*node1 == EOS)
		STprintf(database1, ERx("%s"), dbname1);
	else
		STprintf(database1, ERx("%s::%s"), node1, dbname1);

	EXEC SQL REPEATED SELECT target_type
		INTO	:target_type
		FROM	dd_db_cdds
		WHERE	database_no = :db1
		AND	cdds_no = :cdds_no;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO);
	if (err)
	{
		/* error %d retrieving target_type for database %d and cdds %d */
		//"TABLE_INTEGRITY: Error %d retrieving target_type for "
		//"database %d and cdds %d."ResourceString(IDS_E_TBL_INTEG_TARGET_TYPE)
		STprintf(tmp, lpStringFormat->lpTblIntegTargetType,err, db1, cdds_no);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}
	if (target_type != REPLIC_FULLPEER && target_type != REPLIC_PROT_RO)
	{
		/*ddba_messageit(1, 1558, database1, target_type);*///ResourceString(IDS_E_DB_UNPROTREADONLY)
		//"Cannot run integrity report on URO target.\n"
		//"Database '%s' in CDDS %d is an Unprotected Read Only target. The Integrity"ResourceString(IDS_E_DB_UNPROTREADONLY)
		//"Report can only be run on Full Peer and Protected Read Only targets."
		//
		STprintf(tmp,lpStringFormat->lpDbUnprotreadonly,database1,cdds_no);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}

	EXEC SQL REPEATED SELECT TRIM(database_name), TRIM(vnode_name)
		INTO	:dbname2, :node2
		FROM	dd_databases
		WHERE	database_no = :db2;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO);
	if (err)
	{
		/* error %d retrieving database name for database %d */
		//"TABLE_INTEGRITY: Error %d retrieving database name for database %d."ResourceString(IDS_E_TBL_INTEG_ERROR)
		STprintf(tmp,lpStringFormat->lpResTblIntegError, err, db2);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}
	if (*dbname2 == EOS)
	{
		/* invalid database name %s or node %s for database number %d */ //ResourceString(IDS_E_INVALID_DB)
		/*ddba_messageit_text(tmp, 1538, dbname2, node2, db2);*/
		/*return (1538);*/
		STprintf(tmp,lpStringFormat->lpInvalidDB,dbname2, node2, db2);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}
	if (*node2 == EOS)
		STprintf(database2, ERx("%s"), dbname2);
	else
		STprintf(database2, ERx("%s::%s"), node2, dbname2);

	EXEC SQL REPEATED SELECT target_type
		INTO	:target_type
		FROM	dd_db_cdds
		WHERE	database_no = :db2
		AND	cdds_no = :cdds_no;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO);
	if (err)
	{
		/* error %d retrieving target_type for database %d and cdds %d */
		STprintf(tmp,lpStringFormat->lpTblIntegTargetType,err, db2, cdds_no);//ResourceString(IDS_E_TBL_INTEG_TARGET_TYPE)
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}
	if (target_type != REPLIC_FULLPEER && target_type != REPLIC_PROT_RO)
	{
		/*ddba_messageit(1, 1558, database2, target_type);*/
		//"Cannot run integrity report on URO target.\n"
		//"Database '%s' in CDDS %d is an Unprotected Read Only target. The Integrity"
		//"Report can only be run on Full Peer and Protected Read Only targets."
		//ResourceString(IDS_E_DB_UNPROTREADONLY)
		STprintf(tmp,lpStringFormat->lpDbUnprotreadonly,database2,cdds_no);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}

	if ( DatabaseConnect(hdlfile,node1 ,dbname1 , localowner, &ConnectInf.hNodeDb1,
	     &ConnectInf.Session1,lpStringFormat) == RES_ERR)
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);

	if ( DatabaseConnect(hdlfile,node2 ,dbname2 , targetowner, &ConnectInf.hNodeDb2,
	     &ConnectInf.Session2,lpStringFormat) == RES_ERR)
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);

	if (STequal(CurTbl.table_owner, localowner))
		STcopy(targetowner, target_table_owner);
	else
		STcopy(CurTbl.table_owner, target_table_owner);
	STprintf(tmp,lpStringFormat->lpIngresReplic);//"Ingres Replicator"ResourceString(IDS_I_INGRES_REPLIC)
	i = 40 + STlength(tmp) / (u_i4)2;
	STprintf(tmpf, ERx("%*s\r\n"), i, tmp);
	Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));

	/* Table integrity report */
	/*ddba_messageit_text(tmp, 1352);*/
	STprintf(tmp,lpStringFormat->lpTblIntegReport);//"Table Integrity Report"ResourceString(IDS_E_TBL_INTEG_REPORT)
	i = 40 + STlength(tmp) / (u_i4)2;
	STprintf(tmpf, ERx("%*s\r\n"), i, tmp);
	Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));

	/* For table '%s.%s' in '%s'*/
	STprintf(tmp,lpStringFormat->lpForTbl,CurTbl.table_owner, CurTbl.table_name,dbname1); //ResourceString(IDS_I_FOR_TBL)
	i = 40 + STlength(tmp) / (u_i4)2;
	STprintf(tmpf, ERx("%*s\r\n"), i, tmp);
	Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));

	/* And table '%s.%s' in '%s'ResourceString(IDS_I_AND_TBL)*/
	STprintf(tmp,lpStringFormat->lpIandtbl ,target_table_owner, CurTbl.table_name,dbname2);
	i = 40 + STlength(tmp) / (u_i4)2;
	STprintf(tmpf, ERx("%*s\r\n\r\n"), i, tmp);
	Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));

	ActivateSession(ConnectInf.Session1);
	/*EXEC SQL SET_SQL (session = :Session1);*/

	if (check_unique_keys(hdlfile,CurTbl.table_name, CurTbl.table_owner,
		CurTbl.table_no, database1,lpStringFormat))
	{
		/*ddba_messageit_text(tmp, 1522);*/
		STprintf(tmp,lpStringFormat->lpIRegKeyColumns);//ResourceString(IDS_I_REG_KEY_COLUMNS)"Registered key columns do not force the table to be"
		STprintf(tmpf, ERx("     %s\r\n"), tmp);
		Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));
		/*ddba_messageit_text(tmp, 1570, database1);*/
		STprintf(tmp,lpStringFormat->lpIUnique,database1);//"unique in %s." IDS_I_UNIQUE
		STprintf(tmpf, ERx("     %s\r\n"), tmp);
		Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));
	}
	ActivateSession(ConnectInf.Session2);
	/*EXEC SQL SET_SQL (session = :Session2);*/
	if (check_unique_keys(hdlfile,CurTbl.table_name, target_table_owner,
		CurTbl.table_no, database2,lpStringFormat))
	{
		/*ddba_messageit_text(tmp, 1522);*/
		STprintf(tmp,lpStringFormat->lpIRegKeyColumns);//ResourceString(IDS_I_REG_KEY_COLUMNS)
		STprintf(tmpf, ERx("     %s\r\n"), tmp);
		Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));
		/*ddba_messageit_text(tmp, 1570, database2);*/
		STprintf(tmp,"unique in %s.",database2);
		STprintf(tmpf, ERx("     %s\r\n"), tmp);
		Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));
	}
	ActivateSession(ConnectInf.Session1);
	/*EXEC SQL SET_SQL (session = :Session1);*/

	*base_where_clause = EOS;
	STprintf(base_where_clause, ERx("WHERE cdds_no = %d "), cdds_no);
	STtrmwhite(begin_time);
	STtrmwhite(end_time);
	if (*begin_time != EOS)
	{
		STprintf(tmp, ERx(" AND trans_time >= DATE('%s') "), begin_time);
		STcat(base_where_clause, tmp);
	}
	if (*end_time != EOS)
	{
		STprintf(tmp, ERx(" AND trans_time <= DATE('%s') "), end_time);
		STcat(base_where_clause, tmp);
	}
	STprintf(select_stmt,
		ERx("SELECT sourcedb, transaction_id, sequence_no"));
	column_count = 3;
	sqlda1->sqlvar[0].sqldata = (char *)&database_no1;
	sqlda1->sqlvar[0].sqlind = &null_ind[0];
	sqlda2->sqlvar[0].sqldata = (char *)&database_no2;
	sqlda2->sqlvar[0].sqlind = &null_ind2[0];
	sqlda1->sqlvar[1].sqldata = (char *)&transaction_id1;
	sqlda1->sqlvar[1].sqlind = &null_ind[1];
	sqlda2->sqlvar[1].sqldata = (char *)&transaction_id2;
	sqlda2->sqlvar[1].sqlind = &null_ind2[1];
	sqlda1->sqlvar[2].sqldata = (char *)&sequence_no1;
	sqlda1->sqlvar[2].sqlind = &null_ind[2];
	sqlda2->sqlvar[2].sqldata = (char *)&sequence_no2;
	sqlda2->sqlvar[2].sqlind = &null_ind2[2];

	for (col_p = CurTbl.col_p; column_count < CurTbl.ncols + 3;
		column_count++, *col_p++)
	{
		if (STequal(col_p->column_datatype, ERx("varchar")) ||
			STequal(col_p->column_datatype, ERx("date")))
		{
			STprintf(tmp, ERx(", CHAR(t.%s)"),
				col_p->dlm_column_name);
			STcat(select_stmt, tmp);
		}
		else if (STequal(col_p->column_datatype, ERx("long varchar")) ||
			STequal(col_p->column_datatype, ERx("long byte")))
		{
			STcat(select_stmt, ", CHAR('**Unbounded Data Type**')");
		}
		else
		{
			STcat(select_stmt, ERx(", t."));
			STcat(select_stmt, col_p->dlm_column_name);
		}
		if (STbcompare(col_p->column_datatype, 4, ERx("date"), 4, 1) == 0)
		{
			/*sqlda1->sqlvar[column_count].sqldata = (char *)MEreqmem(0, 32, TRUE, &mem_stat);*/
			/*sqlda2->sqlvar[column_count].sqldata = (char *)MEreqmem(0, 32, TRUE, &mem_stat);*/
			sqlda1->sqlvar[column_count].sqldata = (char *)ESL_AllocMem(32);
			sqlda2->sqlvar[column_count].sqldata = (char *)ESL_AllocMem(32);
		}
		else if (	STequal(col_p->column_datatype, ERx("long varchar")) ||
					STequal(col_p->column_datatype, ERx("long byte")))
		{
			//sqlda1->sqlvar[column_count].sqldata =(char *)MEreqmem(0, 24, TRUE, &mem_stat);
			//sqlda2->sqlvar[column_count].sqldata =(char *)MEreqmem(0, 24, TRUE, &mem_stat); 
			sqlda1->sqlvar[column_count].sqldata = (char *)ESL_AllocMem(24);
			sqlda2->sqlvar[column_count].sqldata = (char *)ESL_AllocMem(24);
		}
		else
		{
			/*sqlda1->sqlvar[column_count].sqldata = (char *)MEreqmem(0, 4 + col_p->column_length, TRUE, &mem_stat);*/
			/*sqlda2->sqlvar[column_count].sqldata = (char *)MEreqmem(0, 4 + col_p->column_length, TRUE, &mem_stat);*/
			sqlda1->sqlvar[column_count].sqldata = (char *)ESL_AllocMem(4 + col_p->column_length);
			sqlda2->sqlvar[column_count].sqldata = (char *)ESL_AllocMem(4 + col_p->column_length);
		}
		sqlda1->sqlvar[column_count].sqlind = &null_ind[column_count];
		sqlda2->sqlvar[column_count].sqlind = &null_ind2[column_count];
		if (col_p->key_sequence > 0)
		{
			no_keys = 0;
			if (!STequal(base_where_clause, ERx("WHERE ")))
				STcat(base_where_clause, ERx(" AND "));
			STprintf(tmp, ERx("t.%s = s.%s"), col_p->dlm_column_name,
				col_p->dlm_column_name);
			STcat(base_where_clause, tmp);
		}
	}
	if (column_count == 3)
	{
		/* No replicated columns for table '%s.%s' found. */
		/*ddba_messageit(1, 1359, CurTbl.table_owner, CurTbl.table_name);ResourceString(IDS_E_TBL_INTEG_NO_REPLIC_COL)*/
		STprintf(tmp,lpStringFormat->lpTblIntegNoReplicCol,CurTbl.table_owner, 
		                                                CurTbl.table_name);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}
	if (no_keys)
	{
		/* No key columns for table '%s.%s' found */
		/*ddba_messageit(1, 1367, CurTbl.table_owner, CurTbl.table_name);ResourceString(IDS_E_KEY_COL_FOUND)*/
		STprintf(tmp,lpStringFormat->lpKeyColFound,CurTbl.table_owner, CurTbl.table_name);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
		/*TS_MessageBox(TS_GetFocus(),tmp, NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);*/
		/*return (1367);*/
	}
	sqlda1->sqlvar[column_count].sqldata = (char *)ESL_AllocMem(32);
	sqlda1->sqlvar[column_count].sqlind = &null_ind[column_count];
	sqlda2->sqlvar[column_count].sqldata = (char *)ESL_AllocMem(32);
	sqlda2->sqlvar[column_count].sqlind = &null_ind2[column_count];
	STcopy(select_stmt, target_select_stmt);
	STprintf(tmp, ERx(", CHAR(s.trans_time) FROM %s s, %s.%s t "),
	         CurTbl.sha_name, CurTbl.table_owner, CurTbl.dlm_table_name);
	STcat(select_stmt, tmp);
	VdbaColl_edit_name(EDNM_DELIMIT, target_table_owner, dlm_target_table_owner);
	STprintf(tmp, ", CHAR(s.trans_time) FROM %s s, %s.%s t ",
	         CurTbl.sha_name, dlm_target_table_owner, CurTbl.dlm_table_name);
	STcat(target_select_stmt, tmp);
	STcopy(select_stmt, base_select_stmt);
	STcopy(target_select_stmt, target_base_select_stmt);
	STcat(select_stmt, base_where_clause);
	STcat(target_select_stmt, base_where_clause);
	if (*order_clause != EOS)
	{
		STcat(select_stmt, ERx(" ORDER BY "));
		STcat(select_stmt, order_clause);
		STcat(target_select_stmt, ERx(" ORDER BY "));
		STcat(target_select_stmt, order_clause);
	}
	STcopy(select_stmt, save_select_stmt);
	STcopy(target_select_stmt, target_save_select_stmt);
	sqlda1->sqln = (i2)column_count + 1;
	sqlda2->sqln = (i2)column_count + 1;

	EXEC SQL PREPARE s1 FROM :select_stmt;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
	if (err)
	{
		/*
		** Problem with configuration for database '%s' table
		** '%s.%s'.  Prepare statement failed with error %d.
		*/
		/*ddba_messageit(1, 1361, database1, CurTbl.table_owner,CurTbl.table_name, err);ResourceString(IDS_E_TBL_INTEG_PROB_CONFIG)*/
		//"TABLE_INTEGRITY:  Problem with configuration for database '%s'\r\n"
		//" table '%s.%s'.\r\n  Prepare statement failed with error -%d."
		STprintf(tmp,lpStringFormat->lpTblIntegProbConfig,database1,
		                                               CurTbl.table_owner,
		                                               CurTbl.table_name, err);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		WriteErrortextInFile(hdlfile);
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}
	EXEC SQL DESCRIBE s1 INTO :sqlda1;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
	if (err)
	{
		/* Describe statement for database %s failed with error %d ResourceString(IDS_E_DESCRIBE_STATEMENT)*/
		STprintf(tmp,lpStringFormat->lpDescribeStatement,database1, err);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		WriteErrortextInFile(hdlfile);
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}
	EXEC SQL DECLARE c1 CURSOR FOR s1;
	EXEC SQL OPEN c1 FOR READONLY;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
	if (err)
	{
		/* Open cursor statement for database %s failed with error %d ResourceString(IDS_E_TBL_INTEG_OPEN_CURSOR)*/
		/*ddba_messageit(1, 1364, database1, err);*/
		STprintf(tmp,lpStringFormat->lpTblIntegOpenCursor,database1, err);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		WriteErrortextInFile(hdlfile);
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
	}

	for (i = 0; !done; i++)
	{
		EXEC SQL FETCH c1 USING DESCRIPTOR :sqlda1;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT, :done = ENDQUERY);
		if (err)
		{
			/* Fetch for database %s failed with error %d ResourceString(IDS_TBL_INTEG_FETCH_INTEG)*/
			/*ddba_messageit(1, 1365, database1, err);*/
			STprintf(tmp,lpStringFormat->lpTblIntegFetch,database1, err);
			Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
			WriteErrortextInFile(hdlfile);
			return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
		}
		if (!done)
		{
			ActivateSession(ConnectInf.Session2);
			/*EXEC SQL SET_SQL (session = :Session2);*/
			STprintf(tmp, ERx("sourcedb = %d AND transaction_id "
			                  "= %d AND sequence_no = %d"), database_no1, transaction_id1, sequence_no1);
			if (*base_where_clause == EOS)
				STcopy(ERx("WHERE "), where_clause);
			else
				STprintf(where_clause, ERx("%s AND "),
				         base_where_clause);
			STcat(where_clause, tmp);
			STcopy(target_base_select_stmt, target_select_stmt);
			STcat(target_select_stmt, where_clause);
			EXEC SQL PREPARE s2 FROM :target_select_stmt;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
			if (err)
			{
				/*
				** Problem with configuration for database
				** '%s' table '%s.%s'.  Prepare statement
				** failed with error %d.ResourceString(IDS_E_TBL_INTEG_PROB_CONFIG)
				*/
				STprintf(tmp,lpStringFormat->lpTblIntegProbConfig,database2,CurTbl.table_owner,
				                                                         CurTbl.table_name,err);
				Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
				WriteErrortextInFile(hdlfile);
				return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
			}
			EXEC SQL DESCRIBE s2 INTO :sqlda2;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
			if (err)
			{
				/* Describe statement for database %s failed with error %d */
				/*ddba_messageit(1, 1362, database2, err);ResourceString(IDS_E_DESCRIBE_STATEMENT)*/
				STprintf(tmp,lpStringFormat->lpDescribeStatement,database2, err);
				Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
				WriteErrortextInFile(hdlfile);
				return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
			}
			EXEC SQL EXECUTE IMMEDIATE :target_select_stmt
				USING :sqlda2;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO,:cnt = ROWCOUNT);
			if (err)
			{
				/* Execute immediate from %s failed with error %d */
				/*ddba_messageit(1, 1360, database2, err);ResourceString(IDS_E_TBL_INTEG_EXEC_IMM)*/
				STprintf(tmp,lpStringFormat->lpTblIntegExecImm,database2, err);
				Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
				WriteErrortextInFile(hdlfile);
				return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
			}
			if (!cnt)
			{
				in_1_not_2++;
				/* Row only in %s, not in %s */
				/*ddba_messageit_text(tmp, 1353, database1,*/
					/*database2);*/
				STprintf(tmp,lpStringFormat->lpRowOnlyIn, database1,database2);//ResourceString(IDS_ROW_ONLY_IN)"Row only in %s, not in %s"
				STprintf(tmpf, ERx("\r\n%s\r\n"), tmp);
				Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));
				/* database no:  %d, transaction id:  %d, sequence no:  %d */
				/*ddba_messageit_text(tmp, 1354, database_no1,*/
				/*transaction_id1, sequence_no1);*/
				STprintf(tmpf,lpStringFormat->lpTblIntegDBTransac,database_no1, transaction_id1, sequence_no1); //ResourceString(IDS_TBL_INTEG_DB_TRANSAC)
				Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));
				ActivateSession(ConnectInf.Session1);
				/*EXEC SQL SET_SQL (session = :Session1);*/
				LIBMON_VdbaColl_print_cols(hdlfile, database_no1,transaction_id1,
				                    sequence_no1,(char *)NULL, CurTbl.table_owner,lpStringFormat,&CurTbl);
			}
			else
			{
				for (jj = 0; jj<column_count; jj++)
				{
					if (sqlda1->sqlvar[jj].sqllen !=
						sqlda2->sqlvar[jj].sqllen)
					{
						/* Table definition mismatch between %s and %s. */
						/*ddba_messageit(1, 1366,	database1, database2);	ResourceString(IDS_TBL_INTEG_TBL_DEF)*/
						STprintf(tmp,lpStringFormat->lpTblIntegTblDef,database1, database2);
						Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
						return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
						/*return (1366);*/
					}
					if (MEcmp(sqlda1->sqlvar[jj].sqldata,
						sqlda2->sqlvar[jj].sqldata,
						sqlda1->sqlvar[jj].sqllen) != 0)
					{
						different++;
						/* Row is different in %s than in %s */
						/*ddba_messageit_text(tmp, 1356,database1, database2);ResourceString(IDS_TBL_INTEG_ROW_DIFFERENCES)*/
						STprintf(tmpf,lpStringFormat->lpTblIntegRowDifferences,database1, database2 );
						Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));
						/* database no:  %d, transaction id:  %d, sequence no:  %d */
						/*ddba_messageit_text(tmp, 1354,database_no1,transaction_id1,sequence_no1);*/
						STprintf(tmpf,lpStringFormat->lpTblIntegSourceDB,database_no1,transaction_id1,sequence_no1 );//ResourceString(IDS_TBL_INTEG_SOURCE_DB)
						Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));
						/*EXEC SQL SET_SQL (session = :Session1);*/
						ActivateSession(ConnectInf.Session1);
						LIBMON_VdbaColl_print_cols(hdlfile,database_no1,transaction_id1,sequence_no1,(char *)NULL,CurTbl.table_owner,lpStringFormat,&CurTbl);
						/*EXEC SQL SET_SQL (	session = :);*/
						ActivateSession(ConnectInf.Session2);
						LIBMON_VdbaColl_print_cols(hdlfile,database_no1,transaction_id1,sequence_no1,(char *)NULL,target_table_owner,lpStringFormat,&CurTbl);
					}
				}
			}
		}
		ActivateSession(ConnectInf.Session1);
		/*EXEC SQL SET_SQL (session = :Session1);*/
	}
	EXEC SQL CLOSE c1;

	/* find ones missing from db # 2 */
	ActivateSession(ConnectInf.Session2);
	/*EXEC SQL SET_SQL (session = :Session2);*/
	EXEC SQL PREPARE s2 FROM :target_select_stmt;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
	if (err)
	{
		/*
		** Problem with configuration for database '%s' table
		** '%s.%s'.  Prepare statement failed with error %d.
		*/
		/*ddba_messageit(1, 1361, database2, CurTbl.table_owner,CurTbl.table_name, err);ResourceString(IDS_E_TBL_INTEG_PROB_CONFIG)*/
		STprintf(tmp,lpStringFormat->lpTblIntegProbConfig,database2,
		                                               target_table_owner,
		                                               CurTbl.table_name, err);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		WriteErrortextInFile(hdlfile);
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
		/*return (1361);*/
	}
	EXEC SQL DESCRIBE s2 INTO :sqlda1;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
	if (err)
	{
		/* Describe statement for database %s failed with error %d */
		/*ddba_messageit(1, 1362, database2, err);*/
		//"TABLE_INTEGRITY:  Describe statement for database %s failed "
		//"with error -%d.ResourceString(IDS_E_DESCRIBE_STATEMENT)"
		STprintf(tmp,lpStringFormat->lpTblIntegProbConfig,database2, err);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		WriteErrortextInFile(hdlfile);
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
		/*TS_MessageBox(TS_GetFocus(),tmp, NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);*/
		/*return (1362);*/
	}
	EXEC SQL DECLARE c2 CURSOR FOR s2;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
	if (err)
	{
		/* Declare statement for database %s failed with error %d ResourceString(IDS_E_DESCRIBE_STATEMENT)*/
		/*ddba_messageit(1, 1362, database2, err);*/
		STprintf(tmp,lpStringFormat->lpTblIntegProbConfig,database2, err);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		WriteErrortextInFile(hdlfile);
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
		/*TS_MessageBox(TS_GetFocus(),tmp, NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);*/
		/*return (1362);*/
	}
	EXEC SQL OPEN c2 FOR READONLY;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
	if (err)
	{
		/* Open cursor statement for database %s failed with error %d */
		/*ddba_messageit(1, 1364, database2, err);ResourceString(IDS_E_TBL_INTEG_OPEN_CURSOR)*/
		STprintf(tmp,lpStringFormat->lpTblIntegOpenCursor,database2, err);
		Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
		WriteErrortextInFile(hdlfile);
		return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
		/*TS_MessageBox(TS_GetFocus(),tmp, NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);*/
		/*return (1364);*/
	}
	/*STprintf(msgbuff, "Comparing contents in %s with %s", database2,*/
		/*database1);*/
	/*EXEC FRS MESSAGE :msgbuff;*/

	done = FALSE;
	for (i = 0; !done; i++)
	{
		EXEC SQL FETCH c2 USING DESCRIPTOR :sqlda1;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT, :done = ENDQUERY);
		if (err)
		{
			/* Fetch for database %s failed with error %d */
			/*ddba_messageit(1, 1365, database2, err);ResourceString(IDS_TBL_INTEG_FETCH_INTEG)*/
			STprintf(tmp,lpStringFormat->lpTblIntegFetch, database2, err);
			Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
			WriteErrortextInFile(hdlfile);
			return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
			/*TS_MessageBox(TS_GetFocus(),tmp, NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);*/
			/*return (1365);*/
		}
		if (!done)
		{
			/*EXEC SQL SET_SQL (session = :Session1);*/
			ActivateSession(ConnectInf.Session1);
			STprintf(tmp, ERx("sourcedb = %d AND transaction_id "
			                  "= %d AND sequence_no = %d"), database_no1, transaction_id1, sequence_no1);
			if (*base_where_clause == EOS)
				STcopy(ERx("WHERE "), where_clause);
			else
				STprintf(where_clause, ERx("%s AND "),
					base_where_clause);
			STcat(where_clause, tmp);
			STcopy(base_select_stmt, select_stmt);
			STcat(select_stmt, where_clause);
			EXEC SQL PREPARE s3 FROM :select_stmt;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO, :errortext = ERRORTEXT);
			if (err)
			{
				/*
				** Problem with configuration for database
				** '%s' table '%s.%s'.  Prepare statement
				** failed with error %d.
				*/
				/*ddba_messageit(1, 1361, database1,CurTbl.table_owner, CurTbl.table_name,err);ResourceString(IDS_E_TBL_INTEG_PROB_CONFIG)*/
				STprintf(tmp,lpStringFormat->lpTblIntegProbConfig,database1,CurTbl.table_owner,
				                                               CurTbl.table_name,err);
				Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
				WriteErrortextInFile(hdlfile);
				return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
				/*TS_MessageBox(TS_GetFocus(),tmp, NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);*/
				/*return (1361);*/
			}
			EXEC SQL DESCRIBE s3 INTO :sqlda2;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
			if (err)
			{
				/* Describe statement for database %s failed with error %d ResourceString(IDS_E_DESCRIBE_STATEMENT)*/
				/*ddba_messageit(1, 1362,database1, err );*/
				STprintf(tmp,lpStringFormat->lpDescribeStatement,database1, err);
				Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
				WriteErrortextInFile(hdlfile);
				return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
				/*return (1362);*/
			}
			EXEC SQL EXECUTE IMMEDIATE :select_stmt
				USING :sqlda2;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO,:cnt = ROWCOUNT);
			if (err)
			{
				/* Execute immediate from %s failed with error %d */
				/*ddba_messageit(1, 1360, database2, err);*/
				/*return (1360);ResourceString(IDS_E_TBL_INTEG_EXEC_IMM)*/
				STprintf(tmp,lpStringFormat->lpTblIntegExecImm,database1, err);
				Vdba_WriteBufferInFile(hdlfile,tmp,x_strlen(tmp));
				WriteErrortextInFile(hdlfile);
				return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
			}
			if (!cnt)
			{
				in_2_not_1++;
				/* Row only in %s, not in %s */
				/*ddba_messageit_text(tmp, 1353, database2,database1);*/
				STprintf(tmp, lpStringFormat->lpRowOnlyIn,database2,database1);//ResourceString(IDS_ROW_ONLY_IN)
				STprintf(tmpf, ERx("\r\n%s\r\n"), tmp);
				Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));
				/* database no:  %d, transaction id:  %d, sequence no:  %d */
				/*ddba_messageit_text(tmp, 1354, database_no1,transaction_id1, sequence_no1);*/
				STprintf(tmpf, lpStringFormat->lpTblIntegDBTransac,database_no1,transaction_id1, sequence_no1 );//ResourceString(IDS_TBL_INTEG_DB_TRANSAC)
				Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));
				/*EXEC SQL SET_SQL (session = :Session2);*/
				ActivateSession(ConnectInf.Session2);
				LIBMON_VdbaColl_print_cols(hdlfile, database_no1,transaction_id1, sequence_no1,(char *)NULL,target_table_owner,lpStringFormat,&CurTbl);
			}
		}
		ActivateSession(ConnectInf.Session2);
		/*EXEC SQL SET_SQL (session = :Session2);*/
	}
	EXEC SQL CLOSE c2;

	if (in_1_not_2 + in_2_not_1 + different == 0)
	{
		/* No differences found. */
		/*ddba_messageit_text(tmp, 1743);ResourceString(IDS_TBL_INTEG_NO_DIFFERENCES)*/
		STprintf(tmpf, lpStringFormat->lpTblIntegNoDifferences);
		Vdba_WriteBufferInFile(hdlfile,tmpf,x_strlen(tmpf));
	}
	return end_table_integrity(&hdlfile,start_session, sqlda1, sqlda2,&ConnectInf,lpErrorParameters,&CurTbl);
}
#endif 

/*{
** Name:	free_sqlda - free SQLDA
**
** Description:
**	Frees malloc'd space in sqlda
**
** Inputs:
**	sqlda	-
**
** Outputs:
**	none
**
** Returns:
**	none
*/
static void free_sqlda(IISQLDA	*sqlda)
{
	i4	i;

	/* 3 local variables, the rest are allocated */
	for (i = 3; i < sqlda->sqln; ++i)
		ESL_FreeMem(sqlda->sqlvar[i].sqldata);
		/*MEfree(sqlda->sqlvar[i].sqldata);*/
}


/*{
** Name:	end_table_integrity - close table integrity report
**
** Description:
**	Clean up & disconnect at the end of the table_integrity report,
**		Read temporary file and return the contents.
**
** Inputs:
**	start_session -
**	sqlda1 -
**	sqlda2 -
**
** Outputs:
**	none
**
** Returns:
**	Return file content in buffer.
*/
static TCHAR *end_table_integrity(HFILE *hdlFile,i4 start_session,
                                  IISQLDA *sqlda1,IISQLDA *sqlda2,
                                  LPCONNECTINFO lpConnectInf,
                                  LPERRORPARAMETERS lpErrParam,TBLDEF *pCurTbl)
{
	int LenFile,iretlen;
	char *lpFileContents = NULL;

	Vdba_CloseFile(hdlFile);
	free_sqlda(sqlda1);
	free_sqlda(sqlda2);

	if (pCurTbl && pCurTbl->ncols > 0 && pCurTbl->col_p != NULL)	{
		ESL_FreeMem (pCurTbl->col_p);
		pCurTbl->ncols = 0;
	}
	if (pCurTbl)
		memset(pCurTbl,0,sizeof(TBLDEF));

/* Close all sessions if different of -1 */
	if (lpConnectInf->Session1 != -1)	{
		if (ActivateSession(lpConnectInf->Session1) != RES_SUCCESS)//"Error in ActivateSession."
			lpErrParam->iErrorID |= MASK_RES_E_ACTIVATE_SESSION;
		if ( ReleaseSession(lpConnectInf->Session1,RELEASE_COMMIT) == RES_ERR)//"Error in ReleaseSession."
			lpErrParam->iErrorID |= MASK_RES_E_RELEASE_SESSION;
	}
	if (lpConnectInf->hNodeDb1 != -1)
		LIBMON_CloseNodeStruct (lpConnectInf->hNodeDb1);

	if (lpConnectInf->Session2 != -1)	{
		if (ActivateSession(lpConnectInf->Session2) != RES_SUCCESS) {
			lpErrParam->iErrorID |= MASK_RES_E_ACTIVATE_SESSION;
		}
		if ( ReleaseSession(lpConnectInf->Session2,RELEASE_COMMIT) == RES_ERR)
			lpErrParam->iErrorID |= MASK_RES_E_RELEASE_SESSION;
	}
	if (lpConnectInf->hNodeDb2 != -1)
		LIBMON_CloseNodeStruct (lpConnectInf->hNodeDb2);

	if (start_session != -1)	{
		if (ActivateSession(start_session) != RES_SUCCESS) {
			lpErrParam->iErrorID |= MASK_RES_E_ACTIVATE_SESSION;
		}
		if ( ReleaseSession(start_session,RELEASE_COMMIT) == RES_ERR)
			lpErrParam->iErrorID |= MASK_RES_E_RELEASE_SESSION;
	} 
/* End Close sessions  */

	LenFile = Vdba_GetLenOfFileName(lpConnectInf->OutputFileName);
	if (LenFile == 0)	{
		ESL_RemoveFile(lpConnectInf->OutputFileName);
		return NULL;
	}
	lpFileContents = ESL_AllocMem(LenFile+1);

	if (ESL_FillBufferFromFile(lpConnectInf->OutputFileName, lpFileContents,LenFile,
						   &iretlen, FALSE) != RES_SUCCESS)
		lpErrParam->iErrorID |= MASK_RES_E_READ_TEMPORARY_FILE;
		//"Error read on temporary file."
		//TS_MessageBox(TS_GetFocus(),ResourceString(IDS_ERR_READ_TEMPORARY_FILE),
		//		   NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);

	ESL_RemoveFile(lpConnectInf->OutputFileName);
	return lpFileContents;
}
int Vdba_GetLenOfFileName( char *FileName)
{
	struct _stat StatusInfo;
	int result;
	
	if ( x_strlen(FileName) == 0	)
		return 0;

	/* Get data associated with "stat.c": */
	result = _stat( FileName, &StatusInfo );

	/* Check if statistics are valid: */
	if( result != 0 )
		return 0;
	return StatusInfo.st_size;
}

static void WriteErrortextInFile(HFILE CurHdlFile)
{
	LPTSTR BufDest = NULL;
	char tmpf[1024];

	BufDest = Vdba_DuplicateStringWithCRLF(errortext);
	if (BufDest)
	{
		STprintf(tmpf, ERx("\r\n%s\r\n"),BufDest );
		ESL_FreeMem(BufDest);
	}
	else
		STprintf(tmpf, ERx("\r\n%s\r\n"),errortext );
	Vdba_WriteBufferInFile(CurHdlFile,tmpf,lstrlen(tmpf));

}

/*{
** Name:	DatabaseConnect - connection for database
**
** Description:
**	connect database
**
** Inputs:
**	CurNode			- current node name
**	CurrentDbName	- Current Database name
**
** Outputs:
**	DbaOwner		- dba owner used for this connection.
**	hnode			- hnode number returned by the connection.
**	Session			- Session Number returned by the connection.
**
** Returns:
**	RES_SUCCESS or RES_ERR
*/
#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
static STATUS DatabaseConnect(HFILE CurHdlFile,char *CurNode,char *CurrentDbName ,
                              LPUCHAR DbaOwner,int *hnode ,int *Session,
                              LPTBLINPUTSTRING lpInputStrFormat  )
{
	int thdl=0;/*Default thdl for non thread functions */
	
EXEC SQL BEGIN DECLARE SECTION;
	i4	cnt;
EXEC SQL END DECLARE SECTION;
	char    dbConnect[MAX_CONNECTION_NAME];
	LPUCHAR parentstrings [MAXPLEVEL];
	char    buf[MAXOBJECTNAME*2+2];
	char    bufowner[MAXOBJECTNAME+2];
	char    extradata[EXTRADATASIZE];
	char    tmp[200];
	int     irestype,ires,err;
	LPUCHAR node;
	int hNodeDb;

	node = CurNode;
	*hnode = -1;
	hNodeDb = OpenNodeStruct4Replication(node);
	if (hNodeDb == -1) {
		//"Maximum number of connections has been reached"
		//" - Cannot display integrity report."ResourceString(IDS_E_MAX_NUM_CONNECTION)
		STprintf(tmp,lpInputStrFormat->lpMaxNumConnection);
		Vdba_WriteBufferInFile(CurHdlFile,tmp,x_strlen(tmp));
		return (RES_ERR);
	}

	parentstrings[0]=CurrentDbName;
	parentstrings[1]=NULL;
	ires = DOMGetObjectLimitedInfo(hNodeDb,
	                               CurrentDbName,
	                               "",
	                               OT_DATABASE,
	                               0,
	                               parentstrings,
	                               TRUE,
	                               &irestype,
	                               buf,
	                               bufowner,
	                               extradata);
	LIBMON_CloseNodeStruct (hNodeDb);
	if (ires!=RES_SUCCESS)
	{
		EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
		STprintf(tmp,lpInputStrFormat->lpConnectDb,err, CurrentDbName); //ResourceString(IDS_E_CONNECT_DB)
		Vdba_WriteBufferInFile(CurHdlFile,tmp,x_strlen(tmp));
		return (RES_ERR);
	}
	lstrcpy(DbaOwner,bufowner);
	lstrcat(node,LPUSERPREFIXINNODENAME);
	lstrcat(node,bufowner);
	lstrcat(node,LPUSERSUFFIXINNODENAME);
	// Format string with user for getsession()
	STprintf(dbConnect,"%s::%s",node,CurrentDbName);

	*hnode = OpenNodeStruct4Replication(node);
	ires = LIBMON_DOMGetFirstObject (*hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL, thdl);
	ires = Getsession4Replication(dbConnect,SESSION_TYPE_INDEPENDENT, Session);
	if (ires != RES_SUCCESS)	{
		/* Error %d connecting to database %s */
		EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
		STprintf(tmp,lpInputStrFormat->lpConnectDb,err, CurrentDbName);//ResourceString(IDS_E_CONNECTION)
		Vdba_WriteBufferInFile(CurHdlFile,tmp,x_strlen(tmp));
		return (RES_ERR);
	}

	EXEC SQL REPEATED SELECT COUNT(*)
		INTO :cnt
		FROM iitables
		WHERE LOWERCASE(table_name) = 'dd_databases'
		AND	table_owner = DBMSINFO('dba');
	EXEC SQL INQUIRE_SQL (:err = ERRORNO,:errortext = ERRORTEXT);
	if (err)
	{
		/*
		** Error verifying existence of Replicator database objects in
		** database %s.
		*/
		STprintf(tmp,lpInputStrFormat->lpReplicObjectInDB,CurrentDbName); // ResourceString(IDS_E_REPLIC_OBJECT_IN_DB)
		Vdba_WriteBufferInFile(CurHdlFile,tmp,x_strlen(tmp));
		WriteErrortextInFile(CurHdlFile);
		return RES_ERR;
	}
	else if (cnt == 0)
	{
		/*
		** Replicator database objects do not exist in database %s.
		** Catalog dd_databases not found.
		*/
		/*ddba_messageit(1, 1768, database1);*/
		STprintf(tmp,lpInputStrFormat->lpReplicObjectExist,CurrentDbName);//IDS_E_REPLIC_OBJECT_EXIST
		Vdba_WriteBufferInFile(CurHdlFile,tmp,x_strlen(tmp));
		return RES_ERR;
	}
	return (RES_SUCCESS);
}
#endif



/////////////////////////////////////////////////////////////////////////////////////
// Utility functions for text formatting - replacement of CR with CRLF
// With test in case CRLF already in source string
// Buffer need to be remove by the caller function
// return : - The address of new buffer allocated
//          - NULL if no carriage return in string or allocation memory error.

LPTSTR Vdba_DuplicateStringWithCRLF(LPTSTR pBufSrc)
{
	char  tcNewLine     = ('\n');
	char  tcCarriageRet = ('\r');
	char  tcNL[]  = ("\n");
	TCHAR  tcCR[]  = ("\r");
	TCHAR  *tcDest,*tcDestTemp,*tcPrev;
	TCHAR  *tcCurr  = (LPTSTR)pBufSrc;
	int nbAddlen = 0;
	tcPrev = tcCurr;

	while (*tcCurr)
	{
		if (*tcCurr == tcNewLine && *tcPrev != tcCarriageRet)
			nbAddlen++;
		tcPrev = tcCurr;
		/*tcCurr = _tcsinc(tcCurr);*/
		tcCurr = tcCurr+1;
	}

	if (nbAddlen == 0) // it's not necessary to add CRNL in this string.
		return NULL;

	tcDest = (TCHAR*)ESL_AllocMem ((STlength (pBufSrc) + nbAddlen + 1)* sizeof(TCHAR));
	if (!tcDest)
		return NULL;

	tcCurr  = (LPTSTR)pBufSrc;
	tcDestTemp  = tcDest;
	tcPrev      = tcCurr;
	while (*tcCurr)
	{
		if (*tcCurr == tcNewLine && *tcPrev != tcCarriageRet)
		{
			/*_tccpy(tcDestTemp,tcCR);*/
			x_strcpy(tcDestTemp,tcCR);
			/*tcDestTemp = _tcsinc(tcDestTemp);*/
			tcDestTemp = tcDestTemp+1;
		}
		/*_tccpy(tcDestTemp,tcCurr);*/
		x_strcpy(tcDestTemp,tcCurr);
		tcPrev = tcCurr;
		/*tcCurr = _tcsinc(tcCurr);
		tcDestTemp = _tcsinc(tcDestTemp);*/
		tcCurr = tcCurr+1;
		tcDestTemp = tcDestTemp+1;
	}

	*tcDestTemp = ('\0');
	return tcDest;
}


/*{
** Name:  VdbaColl_print_cols - print column names and values
**
** Description:
**  Print (to file pointer report_fp) the column names and values for the
**  specified entry in the specified table.  Used by at least 2 reports.
**
**  Constructs a where clause with which to identify a set of columns
**  associated with a given replication key.  Format and print to
**  the file handle addressed by report_fp.
**
** Inputs:
**  report_fp       - file handler for output file
**  sourcedb        - integer database_no
**  transaction_id  - integer transaction_id
**  sequence_no     - integer sequence number within a transaction
**  target_wc       - 
**  table_owner     - owner of the table

** Outputs:
**  None
**
** Returns:
**  RES_SUCCESS or RES_ERR
*/
static int LIBMON_VdbaColl_print_cols( HFILE report_fp, i2  sourcedb, 
                                       long transaction_id,long  sequence_no,
                                       char *target_wc,char *table_owner,
                                       LPTBLINPUTSTRING lpStrFormat,
                                       TBLDEF *pRmTbl)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char    tmp[256];
	char    tmp_statement[256];
	char    tmp_char[256];
	short    tmp_int = 0;
	double  tmp_float;
	char    column_val[32];
	char    where_clause[1024];
	short    null_indF;
	short   null_indC;
	short  tmpnull_ind;
	EXEC SQL END DECLARE SECTION;
	char tmpf[1024];
	char   dlm_table_owner[DLMNAME_LENGTH];
	DBEC_ERR_INFO errinfo;
	COLDEF        *col_p;
	i4            column_number;

	VdbaColl_edit_name(EDNM_DELIMIT, table_owner, dlm_table_owner);

	RMadf_cb2 = FEadfcb();

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

	for (column_number = 0, col_p = pRmTbl->col_p;
		column_number < pRmTbl->ncols; column_number++, *col_p++)
	{
		if (STcompare(col_p->column_datatype, ERx("integer")) == 0)
		{
			if (target_wc == NULL)
			{
				STprintf(tmp_statement,
					ERx("SELECT INT4(t.%s) FROM %s.%s t, "
						"%s s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
					col_p->dlm_column_name,
					dlm_table_owner,
					pRmTbl->dlm_table_name, pRmTbl->sha_name,
					sourcedb, transaction_id, sequence_no);
				STcat(tmp_statement, where_clause);
			}
			else
			{
				STprintf(tmp_statement, ERx("SELECT INT4(t.%s)"
						 "FROM %s.%s t WHERE %s"),
					col_p->dlm_column_name,
					dlm_table_owner,
					pRmTbl->dlm_table_name, target_wc);
			}
			EXEC SQL EXECUTE IMMEDIATE :tmp_statement
				INTO	:tmp_int:tmpnull_ind;
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
					    dlm_table_owner,
					    pRmTbl->dlm_table_name, pRmTbl->sha_name,
					    sourcedb, transaction_id, sequence_no);
				STcat(tmp_statement, where_clause);
			}
			else
			{
				STprintf(tmp_statement,
					ERx("SELECT FLOAT8(t.%s) FROM %s.%s t WHERE %s"),
					col_p->dlm_column_name,
					dlm_table_owner,
					pRmTbl->dlm_table_name, target_wc);
			}
			EXEC SQL EXECUTE IMMEDIATE :tmp_statement
				INTO	:tmp_float:null_indF;
			STprintf(column_val, ERx("%.13g"), tmp_float,
				(char)RMadf_cb2->adf_decimal.db_decimal);
		}
		else if (STequal(col_p->column_datatype, ERx("long varchar")) ||
			STequal(col_p->column_datatype, ERx("long byte")))
		{
			STcopy(lpStrFormat->lpUnBoundedDta, column_val); //"**Unbounded Data Type**"
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
					dlm_table_owner,
					pRmTbl->dlm_table_name, pRmTbl->sha_name,
					sourcedb, transaction_id, sequence_no);
				STcat(tmp_statement, where_clause);
			}
			else
			{
				STprintf(tmp_statement, ERx("SELECT TRIM(CHAR(t.%s)) FROM %s.%s t WHERE %s"),
				         col_p->dlm_column_name,
				         dlm_table_owner,
				         pRmTbl->dlm_table_name, target_wc);
			}
			EXEC SQL EXECUTE IMMEDIATE :tmp_statement
				INTO  :tmp_char:null_indC;
			STprintf(column_val, ERx("%s"), tmp_char);
		}
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			if (errinfo.errorno)
			{
				STprintf(tmpf, lpStrFormat->lpColumnInfo,
				                  table_owner, pRmTbl->table_name,
				                  col_p->column_name);//ResourceString(IDS_F_COLUMN_INFO)
				Vdba_WriteBufferInFile(report_fp,tmpf,x_strlen(tmpf));
				return RES_ERR; //"Error retrieving column information for '%s.%s.%s'"
			}
		}
		if (errinfo.rowcount > 0)
		{
			if (null_indC == -1)
				STcopy(ERx("NULL"), column_val);
			STprintf(tmpf, ERx("     %s%s:  %s\r\n"),
			         col_p->key_sequence > 0 ? ERx("*") : ERx(" "),
			         col_p->column_name, column_val);
			Vdba_WriteBufferInFile(report_fp,tmpf,x_strlen(tmpf));
		}
		else
		{
			//Column information for '%s.%s' is missing.  Record deleted?\r\n"
			STprintf(tmpf,lpStrFormat->lpInformationMissing, table_owner, pRmTbl->table_name);
			Vdba_WriteBufferInFile(report_fp,tmpf,x_strlen(tmpf));
			return RES_ERR;
		}
	}
	return RES_SUCCESS;
}


