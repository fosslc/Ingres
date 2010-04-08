/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**
**  Project : libmon library for the Ingres Monitor ActiveX control
**
**  Author : Schalk Philippe
**
**  Source : chkdistr.sc - check distributed configuration
**
**  Defines
**  Vdba_check_distrib_config       check_distrib_config    - check distributed configuration
**  Vdba_check_remote_config        check_remote_config     - check remote database configuration
**  Vdba_check_databases            check_databases         - check databases
**  Vdba_check_regist_tables        check_regist_tables     - check registered tables
**  Vdba_check_regist_columns       check_regist_columns    - check registered columns
**  Vdba_check_cdds                 check_cdds              - check cdds
**  Vdba_check_db_cdds              check_db_cdds           - check db_cdds
**  Vdba_check_paths                check_paths             - check paths
**
**  Vdba_open_remote_db             open_remote_db          - open a remote database
**  Vdba_set_session                set_session             - set session
**
**  History
**
**  04-Dec-2001 (schph01)
**      extracted and adapted from the winmain\chkdistr.sc source
**  24-Mar-2004 (shaha03)
**  (sir 112040) Call to LIBMON_DOMGetFirstObject() is changed with an added thread_handle parameter thdl at the end
**  08-May-2009 (drivi01)
**	In effort to port to Visual Studio 2008, clean up the warnings.
***********************************************************************/
#include <sys/stat.h>
#include <tchar.h>
#include "libmon.h"
#include "replutil.h"
#include "commfunc.h"
#include "domdloca.h"
#include "lbmnmtx.h"

extern LBMNMUTEX lbmn_mtx_Info[MAXNODES];

typedef struct _genericinfo
{
    i4    iLocal_Session;
    HFILE hdlfile;
    LPCHKINPUTSTRING lpInStr;
} GENERICINFO , FAR * LPGENERICINFO;

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
static void Vdba_check_remote_config   (char *r_db, i2 r_dbno, LPGENERICINFO lpInf);
#endif
static STATUS Vdba_check_databases     (i4 sess_a, i4 sess_b, i4 *n_err, LPGENERICINFO lpInf);
static STATUS Vdba_check_regist_tables (i4 sess_a, i4 sess_b, i4 *n_err, LPGENERICINFO lpInf);
static STATUS Vdba_check_regist_columns(i4 sess_a, i4 sess_b, i4 *n_err, LPGENERICINFO lpInf);
static STATUS Vdba_check_cdds          (i4 sess_a, i4 sess_b, i4 *n_err, LPGENERICINFO lpInf);
static STATUS Vdba_check_db_cdds       (i4 sess_a, i4 sess_b, i4 *n_err, LPGENERICINFO lpInf);
static STATUS Vdba_check_paths         (i4 sess_a, i4 sess_b, i4 *n_err, LPGENERICINFO lpInf);
static STATUS Vdba_set_session         (i4 sess_no, LPGENERICINFO lpInf);
static STATUS Vdba_open_remote_db      (char *r_db, int * r_sess, LPGENERICINFO lpInf);
static void Vdba_Check_StoreSqlError   ( char *Error, DBEC_ERR_INFO errinfo ,char *DefaultText, LPGENERICINFO lpInf);
static char *VdbaSessType             (i4 iSessNum , LPGENERICINFO lpCurrInfo);


static char *VdbaSessType(i4 iSessNum , LPGENERICINFO lpCurrInfo)
{
	return ((iSessNum == lpCurrInfo->iLocal_Session) ? ("local") : ("remote"));
}

/*{
** Name:  Vdba_check_distrib_config - check distributed configuration
**
** Description:
**  Create a report showing any problems with the configuration
**  in databases in the dd_connections table.
**
** Inputs:
**  CurrentNodeHdl    - Current node Handle
**  dbname            - Current Database Name
**  lpStringFormat    - structure with all strings used for error or warning messages
**
**
** Outputs:
**  lpErrorParameters - structure contain the error and parameters used to
**                      display message.
**
** Returns:
**  char* or NULL if error before SQL statement execution.
**
*/

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */               
char *LIBMON_check_distrib_config( int CurrentNodeHdl,char *dbname ,
                                    LPCHKINPUTSTRING lpStringFormat,
                                    LPERRORPARAMETERS lpErrorParameters)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char  remote_vnode[DB_MAXNAME+1];
	char  remote_dbname[DB_MAXNAME+1];
	char  remote_db[MAX_CONNECTION_NAME];
	short remote_dbno;
	char  local_dbname[DB_MAXNAME+1];
	char  report_date[26];
	long  cnt;
	long  done = 0;
	long  err = 0;
	EXEC SQL END DECLARE SECTION;
	char * lpFileContents,*VnodeName;
	char OutputFileName[_MAX_PATH+1],tmpf[1024],tmp[1024],szConnectName[MAX_CONNECTION_NAME];
	DBEC_ERR_INFO errinfo;
	BOOL bSuccess, c1_open = FALSE;
	i4   i;
	u_i4 un;
	int   ires,iretlen,LenFile;
	GENERICINFO GlobalInfo;
	
   if(!CreateLBMNMutex(&lbmn_mtx_Info[CurrentNodeHdl])) /*Creating a Mutex: Will ignore if the mutex is already created */
		return RES_ERR;
   if(!OwnLBMNMutex(&lbmn_mtx_Info[CurrentNodeHdl],INFINITE)) /*Owning the Mutex */
		return RES_ERR;
		
	memset (&GlobalInfo,'\0',sizeof (GENERICINFO));
	GlobalInfo.lpInStr = lpStringFormat;
	bSuccess = Vdba_InitReportFile(&GlobalInfo.hdlfile, OutputFileName, sizeof(OutputFileName));
	if ( !bSuccess) {
		// "Cannot open report file '%s'"IDS_F_OPEN_REPORT
		lpErrorParameters->iErrorID = RES_E_OPEN_REPORT;
		lstrcpy(lpErrorParameters->tcParam1 , OutputFileName);
		(void)UnownLBMNMutex(&lbmn_mtx_Info[CurrentNodeHdl]); /*Releasing the mutex before return*/
		return NULL;
	}

	VnodeName = GetVirtNodeName (CurrentNodeHdl);

	STprintf(szConnectName,"%s::%s",VnodeName, dbname);
	ires = Getsession4Replication(szConnectName,SESSION_TYPE_INDEPENDENT, &GlobalInfo.iLocal_Session);
	if (ires != RES_SUCCESS)  {
		//"Failure while opening session on :%s"IDS_F_OPENING_SESSION
		lpErrorParameters->iErrorID = RES_E_OPENING_SESSION;
		lstrcpy(lpErrorParameters->tcParam1 , szConnectName);
		Vdba_CloseFile(&GlobalInfo.hdlfile);
		ESL_RemoveFile(OutputFileName);
		(void)UnownLBMNMutex(&lbmn_mtx_Info[CurrentNodeHdl]); /*Releasing the mutex before return*/
		return NULL;
	}
	EXEC SQL REPEATED SELECT DBMSINFO('DATABASE'), DATE('now')
		INTO :local_dbname, :report_date;
	if (Vdba_error_check(DBEC_SINGLE_ROW, &errinfo) != RES_SUCCESS)
	{
		/* Unexpected row count of %d. IDS_E_UNEXPECTED_ROW*/
		STprintf(tmpf,lpStringFormat->lpUnexpectedRow,errinfo.rowcount);
		/*
		** Error getting database name and report time.IDS_E_GET_DB_NAME
		*/
		STprintf(tmp,lpStringFormat->lpGetDbName);
		Vdba_Check_StoreSqlError(tmp,errinfo,tmpf,&GlobalInfo);
		err = 1744;
	}

	if (!err)
	{
		/* CA-Ingres Replicator IDS_I_INGRES_REPLIC*/
		/* ddba_messageit_text(tmp, 1314);*/
		STprintf (tmp,lpStringFormat->lpIngresReplic);
		un = 40 + STlength(tmp) / (u_i4)2;
		STprintf(tmpf, ERx("%*s\r\n"), un, tmp);
		Vdba_WriteBufferInFile(GlobalInfo.hdlfile,tmpf,lstrlen(tmpf));

		/* Distributed Configuration Checker Report */
		/* ddba_messageit_text(tmp, 1418);ResourceString(IDS_I_DISTRIB_CONFIG)*/
		STprintf (tmp,lpStringFormat->lpDistribConfig);
		un = 40 + STlength(tmp) / (u_i4)2;
		STprintf(tmpf, ERx("%*s\r\n"), un, tmp);
		Vdba_WriteBufferInFile(GlobalInfo.hdlfile,tmpf,lstrlen(tmpf));

		/* On local database %s */
		STtrmwhite(local_dbname);
		/* ddba_messageit_text(tmp, 1429, local_dbname);ResourceString(IDS_I_LOC_DB)*/
		STprintf (tmp,lpStringFormat->lpLocDb,local_dbname);
		un = 40 + STlength(tmp) / (u_i4)2;
		STprintf(tmpf, ERx("%*s\r\n"), un, tmp);
		Vdba_WriteBufferInFile(GlobalInfo.hdlfile,tmpf,lstrlen(tmpf));

		STtrmwhite(report_date);
		un = 40 + STlength(report_date) / (u_i4)2;
		STprintf(tmpf, ERx("%*s\r\n"), un, report_date);
		Vdba_WriteBufferInFile(GlobalInfo.hdlfile,tmpf,lstrlen(tmpf));

		EXEC SQL REPEATED SELECT COUNT(*)
			INTO :cnt
			FROM dd_paths;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Unexpected row count of %d. IDS_E_UNEXPECTED_ROW*/
			STprintf(tmpf,lpStringFormat->lpUnexpectedRow,errinfo.rowcount);
			/*
			** Error checking dd_paths table for distribution
			** data.IDS_E_DISTRIB_DD_PATH_TBL
			*/
			STprintf(tmp,lpStringFormat->lpDistribDDpathTbl);
			Vdba_Check_StoreSqlError(tmp,errinfo,tmpf,&GlobalInfo);
			err = 1574;
		}
		else if (cnt == 0)
		{
			/*
			** No pathway data found -- Data will not be replicated
			** from this database.
			*/
			/* ddba_messageit_text(tmp, 1575);ResourceString(IDS_E_PROPAGATION_PATH)*/
			STprintf(tmp,lpStringFormat->lpPropagationPath);
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(GlobalInfo.hdlfile,tmpf,lstrlen(tmpf));
		}
	}

	if (!err)
	{
		EXEC SQL DECLARE C1 CURSOR FOR
			SELECT DISTINCT d.vnode_name, d.database_name,
			                d.database_no
			FROM  dd_databases d, dd_db_cdds c
			WHERE d.local_db = 0
			AND   d.database_no = c.database_no
			AND   c.target_type in (1, 2)
			ORDER BY database_no;
		EXEC SQL OPEN c1 FOR READONLY;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/*
			** Error opening local cursor for retrieving database
			** information.ResourceString(IDS_E_OPEN_LOCAL_CURSOR)
			*/
			STprintf(tmp,lpStringFormat->lpOpenLocalCursor);
			Vdba_Check_StoreSqlError(tmp,errinfo,"",&GlobalInfo);
			err = 1413;
		}
		else
		{
			c1_open = TRUE;
		}
	}

	while (!err && !done)
	{
		if ((err = Vdba_set_session(GlobalInfo.iLocal_Session,&GlobalInfo)) != OK)
			continue;

		EXEC SQL FETCH c1
			INTO :remote_vnode, :remote_dbname, :remote_dbno;
		EXEC SQL INQUIRE_SQL (:done = ENDQUERY);
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/*
			** Error fetching database information from local
			** cursor.IDS_E_FETCHING_DB
			*/
			/* ddba_messageit_text(tmp, 1414);*/
			STprintf(tmp, lpStringFormat->lpFetchingDB);
			Vdba_Check_StoreSqlError(tmp,errinfo,"",&GlobalInfo);
			err = 1414;
			continue;
		}
		else if (done)
		{
			continue;
		}

		STtrmwhite(remote_vnode);
		STtrmwhite(remote_dbname);
		if (*remote_vnode == EOS)
			STprintf(remote_db, ERx("%s"), remote_dbname);
		else
			STprintf(remote_db, ERx("%s::%s"), remote_vnode,
				remote_dbname);
		/* EXEC FRS MESSAGE :remote_db;*/

		Vdba_check_remote_config(remote_db, remote_dbno,&GlobalInfo);
	}

	if ((i = Vdba_set_session(GlobalInfo.iLocal_Session,&GlobalInfo)) != OK)
	{
		if (!err)
			err = i;
	}
	else
	{
		if (c1_open)
		{
			EXEC SQL CLOSE c1;
			if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
			{
				/*
				** Error closing local cursor on database
				** information.IDS_E_CLOSE_LOC_CURSOR
				*/
				STprintf(tmp,lpStringFormat->lpCloseLocCursor);
				Vdba_Check_StoreSqlError(tmp,errinfo,"",&GlobalInfo);
				if (!err)
					err = 1765;
			}
		}

		if (err)
		{
			EXEC SQL ROLLBACK;
			if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
			{
				/* Error on local rollback. 1762 IDS_E_LOCAL_ROLLBACK*/
				STprintf(tmp,lpStringFormat->lpLocalRollback);
				Vdba_Check_StoreSqlError(tmp,errinfo,"",&GlobalInfo);
			}
		}
		else
		{
			EXEC SQL COMMIT;
			if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
			{
				/* Error on local commit. IDS_E_LOCAL_COMMIT*/
				STprintf(tmp,lpStringFormat->lpLocalCommit);
				Vdba_Check_StoreSqlError(tmp,errinfo,"",&GlobalInfo);
				err = 1763;
			}
		}
	}

	if ( ReleaseSession(GlobalInfo.iLocal_Session,RELEASE_COMMIT) == RES_ERR) {
		lstrcpy(tmp,lpStringFormat->lpReleaseSession);
		Vdba_Check_StoreSqlError(tmp,errinfo,"",&GlobalInfo);//ResourceString(IDS_ERR_RELEASE_SESSION)
	}

	Vdba_CloseFile(&GlobalInfo.hdlfile);

	LenFile = Vdba_GetLenOfFileName(OutputFileName);
	if (LenFile == 0)	{
		ESL_RemoveFile(OutputFileName);
		(void)UnownLBMNMutex(&lbmn_mtx_Info[CurrentNodeHdl]); /*Releasing the mutex before return*/
		return NULL;
	}
	lpFileContents = ESL_AllocMem(LenFile+1);

	if (ESL_FillBufferFromFile( OutputFileName, lpFileContents,LenFile,
	                            &iretlen, FALSE) != RES_SUCCESS)
	{
		lpErrorParameters->iErrorID |= MASK_RES_E_READ_TEMPORARY_FILE;
	}
	ESL_RemoveFile(OutputFileName);
	
	(void)UnownLBMNMutex(&lbmn_mtx_Info[CurrentNodeHdl]); /*Releasing the mutex before return*/
	return lpFileContents;
}
#endif

void LIBMON_Vdba_FreeMemory(char **tmp)
{
	if ( *tmp!=NULL)
		ESL_FreeMem(*tmp);
	*tmp = NULL;
}

/*{
** Name:  Vdba_check_remote_config - check remote database configuration
**
** Description:
**  Check a remote database configuration against the local one.
**
** Inputs:
**  r_db   - remote database name
**  r_dbno - remote database number
**  lpInf  - structure for generic informations (handle file, local session number,etc...)
**
** Outputs:
**  none
**
** Returns:
**  none
*/
#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
static void Vdba_check_remote_config(char *r_db,i2 r_dbno, LPGENERICINFO lpInf)
{
	int thdl=0; /*Default thread for Non multi-thread context*/
	EXEC SQL BEGIN DECLARE SECTION;
	i4 l_sess = lpInf->iLocal_Session;
	int r_sess ;
	char tmp[1024];
	EXEC SQL END DECLARE SECTION;
	bool  remote_open = FALSE;
	i4    n_err = 0;
	STATUS err = 0;
	char tmpf[1024];
	char buf[MAXOBJECTNAME];
	char CurrentVnodeName[MAX_VNODE_NAME];
	char CurrentDBName[MAXOBJECTNAME];
	char tmpVnode[MAXOBJECTNAME];
	int iHnodeStruc;

	memset (CurrentVnodeName,'\0',MAX_VNODE_NAME);
	memset (CurrentDBName,'\0',MAXOBJECTNAME);
	memset (tmpVnode,'\0',MAXOBJECTNAME);

	/* Report on remote database %s (%d): */
	/* ddba_messageit_text(tmp, 1419, r_db, r_dbno) ResourceString(IDS_I_REPORT_DB)*/
	STprintf(tmp,lpInf->lpInStr->lpReportDB,r_db, r_dbno);
	STprintf(tmpf, ERx("\r\n\r\n%s\r\n"), tmp);
	Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

	if ( GetNodeAndDB(CurrentVnodeName,CurrentDBName,r_db) != RES_ERR)
		Vdba_GetValidVnodeName(CurrentVnodeName,tmpVnode);

	iHnodeStruc = OpenNodeStruct4Replication(tmpVnode);
	if (iHnodeStruc == -1) {
		STprintf(tmp,lpInf->lpInStr->lpMaxConnection);//ResourceString(IDS_E_MAX_CONNECTION)
		Vdba_WriteBufferInFile(lpInf->hdlfile,tmp,x_strlen(tmp));
		return;
	}

	LIBMON_DOMGetFirstObject (iHnodeStruc, OT_DATABASE, 0, NULL, TRUE, NULL, buf, NULL, NULL, thdl);
	
	err = Vdba_open_remote_db(r_db, &r_sess,lpInf);
	if (!err)
	{
		remote_open = TRUE;
		err = Vdba_check_databases(l_sess, r_sess, &n_err,lpInf)      ||
		      Vdba_check_databases(r_sess, l_sess, &n_err,lpInf)      ||
		      Vdba_check_cdds(l_sess, r_sess, &n_err,lpInf)           ||
		      Vdba_check_cdds(r_sess, l_sess, &n_err,lpInf)           ||
		      Vdba_check_db_cdds(l_sess, r_sess, &n_err,lpInf)        ||
		      Vdba_check_db_cdds(r_sess, l_sess, &n_err,lpInf)        ||
		      Vdba_check_paths(l_sess, r_sess, &n_err,lpInf)          ||
		      Vdba_check_paths(r_sess, l_sess, &n_err,lpInf)          ||
		      Vdba_check_regist_tables(l_sess, r_sess, &n_err,lpInf)  ||
		      Vdba_check_regist_tables(r_sess, l_sess, &n_err,lpInf)  ||
		      Vdba_check_regist_columns(l_sess, r_sess, &n_err,lpInf) ||
		      Vdba_check_regist_columns(r_sess, l_sess, &n_err,lpInf);
	}

	if (err)
	{
		/*
		** Error prohibits further checking on database
		** no %d.
		*/
		STprintf(tmp, lpInf->lpInStr->lpProhibits, r_dbno);//ResourceString(IDS_E_PROHIBITS)
		STprintf(tmpf, ERx("%s\r\n"), tmp);
		Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
		if (iHnodeStruc != -1)
			LIBMON_CloseNodeStruct (iHnodeStruc);
		ResetSessionUserIdentifier();
	}
	else if (n_err == 0)
	{
		/* No problems found. */
		STprintf(tmp, lpInf->lpInStr->lpNoProblem);// ResourceString(IDS_I_NO_PROBLEM)
		STprintf(tmpf, ERx("%s\r\n"), tmp);
		Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
	}

	if (remote_open == TRUE && Vdba_set_session(r_sess,lpInf) == OK)
	{
		if (r_sess != lpInf->iLocal_Session) {
			if ( ReleaseSession(r_sess,RELEASE_COMMIT) == RES_ERR)
			{
				STprintf(tmp, lpInf->lpInStr->lpNoRelease);//"Cannot release the session"
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
			}
		}
		if (iHnodeStruc != -1)
			LIBMON_CloseNodeStruct (iHnodeStruc);
		remote_open = FALSE;
		ResetSessionUserIdentifier();
	}
}
#endif


/*{
** Name:  Vdba_open_remote_db - open a remote database
**
** Description:
**  Open a remote database for configuration checking.
**
** Inputs:
**  r_db    - remote database name
**  r_sess  - remote database session number
**  lpInf   - structure for generic informations (handle file, local session number,etc...)
**
** Outputs:
**	none
**
** Returns:
**  0 - OK
**  else - Error
*/
static STATUS Vdba_open_remote_db( char *r_db,int * r_sess,
                                   LPGENERICINFO lpInf)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	*db = r_db;
	char	username[DB_MAXNAME+1];
	char	dba[DB_MAXNAME+1];
	i4	cnt;
	EXEC SQL END DECLARE SECTION;
	DBEC_ERR_INFO	errinfo;
	int ires;
	char	tmp[1024];
	char	tmpf[1024];
	ires = Getsession4Replication(r_db,SESSION_TYPE_INDEPENDENT, r_sess);
	if (ires != RES_SUCCESS) {
		Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo);
		/* Error connecting to %s. */
		STprintf(tmp, lpInf->lpInStr->lpErrorConnecting, db);//ResourceString(IDS_E_CONNECTING)
		Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
		return (1415);
	}

	/* See if user is the DBA; if not, reconnect as DBA. */
	EXEC SQL REPEATED SELECT DBMSINFO('DBA'), DBMSINFO('USERNAME')
		INTO	:dba, :username;
	if (Vdba_error_check(DBEC_SINGLE_ROW, &errinfo) != RES_SUCCESS)
	{
		/* Unexpected row count of %d. */
		STprintf(tmpf, lpInf->lpInStr->lpUnexpectedRow, errinfo.rowcount);//ResourceString(IDS_E_UNEXPECTED_ROW)
		/* Error getting DBA name from database %s. */
		STprintf(tmp, lpInf->lpInStr->lpDBAName, db);//ResourceString(IDS_F_DBA_NAME)
		Vdba_Check_StoreSqlError(tmp,errinfo,tmpf,lpInf);

		if ( ReleaseSession(*r_sess,RELEASE_COMMIT) == RES_ERR)
		{
			STprintf(tmpf, lpInf->lpInStr->lpNoRelease);//"Cannot release the session"
			Vdba_Check_StoreSqlError(tmp,errinfo,tmpf,lpInf);
		}
		return 1760;
	}

	if (STcompare(dba, username))
	{
		/* EXEC SQL DISCONNECT SESSION :sess;*/
		/* EXEC SQL CONNECT :db SESSION :sess IDENTIFIED BY :dba;*/
		if ( ReleaseSession(*r_sess,RELEASE_COMMIT) == RES_ERR)
		{
			STprintf(tmpf, lpInf->lpInStr->lpNoRelease);//"Cannot release the session"
			Vdba_Check_StoreSqlError(tmp,errinfo,tmpf,lpInf);
			//MessageWithHistoryButton(GetFocus(),ResourceString(IDS_ERR_RELEASE_SESSION));
		}

		SetSessionUserIdentifier(dba);
		ires = Getsession4Replication(r_db,SESSION_TYPE_INDEPENDENT, r_sess);
		if (ires != RES_SUCCESS) {
			/* Error connecting to %s. */
			STprintf(tmp, lpInf->lpInStr->lpErrorConnecting, db);//ResourceString(IDS_E_CONNECTING)
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			return (1415);
		}

	}

	EXEC SQL REPEATED SELECT COUNT(*) INTO :cnt
		FROM	iitables
		WHERE	LOWERCASE(table_name) = 'dd_databases'
		AND	table_owner = DBMSINFO('dba');
	if (Vdba_error_check(DBEC_SINGLE_ROW, &errinfo) != RES_SUCCESS)
	{
		/* Unexpected row count of %d. */
		STprintf(tmpf,lpInf->lpInStr->lpUnexpectedRow,errinfo.rowcount);//ResourceString(IDS_E_UNEXPECTED_ROW)
		/*
		** Error verifying existence of Replicator database objects in
		** database %s.
		*/
		STprintf(tmp,lpInf->lpInStr->lpReplicObjectInDB,db);//ResourceString(IDS_E_REPLIC_OBJECT_IN_DB)
		Vdba_Check_StoreSqlError(tmp,errinfo,tmpf,lpInf);
		ReleaseSession(*r_sess,RELEASE_COMMIT);
		ResetSessionUserIdentifier();
		return (1767);
	}
	else if (cnt == 0)
	{
		/*
		** Replicator database objects do not exist in database %s.
		** Catalog dd_databases not found.
		*/
		STprintf(tmp,lpInf->lpInStr->lpReplicObjectExist,db);//ResourceString(IDS_E_REPLIC_OBJECT_EXIST)
		STprintf(tmpf, ERx("%s\r\n"), tmp);
		Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
		ReleaseSession(*r_sess,RELEASE_COMMIT);
		ResetSessionUserIdentifier();
		return (1768);
	}

	return (OK);
}


/*{
** Name:	Vdba_check_databases - check databases
**
** Description:
**	Compare the dd_databases tables in sess_a vs. sess_b.
**
** Inputs:
**  sess_a - Ingres session number
**  sess_b - Ingres session number
**  n_err  - Error count
**  lpInf  - structure with generic informations (handle file, local session number,etc...)
**
** Outputs:
**  none
**
** Returns:
**  0 - OK
**  else - Error
*/
static STATUS Vdba_check_databases( i4 sess_a,i4  sess_b,i4 *n_err,
                                    LPGENERICINFO lpInf)
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	session_a = sess_a;
	i4	session_b = sess_b;
	i2	database_no;
	char	database_name[DB_MAXNAME+1];
	char	database_name_b[DB_MAXNAME+1];
	char	vnode_name[DB_MAXNAME+1];
	char	vnode_name_b[DB_MAXNAME+1];
	char	dbms_type[9];
	char	dbms_type_b[9];
	i4	done = 0;
	STATUS	err = 0;
	EXEC SQL END DECLARE SECTION;
	char tmp[1024],tmpf[1024];
	DBEC_ERR_INFO errinfo;
	BOOL  c2_open = FALSE;
	i4 i;

	if ((err = Vdba_set_session(session_a,lpInf)) != OK)
		++*n_err;

	if (!err)
	{
		EXEC SQL DECLARE c2 CURSOR FOR
			SELECT database_no, database_name, vnode_name,
				dbms_type
			FROM dd_databases
			ORDER BY database_no;
		EXEC SQL OPEN c2 FOR READONLY;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)	{
			/* Error opening cursor on %s dd_databases table. */
			STprintf(tmp, lpInf->lpInStr->lpOpenCursor,VdbaSessType(session_a,lpInf)); //ResourceString(IDS_E_OPEN_CURSOR)
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1422;
			++*n_err;
		}
		else
		{
			c2_open = TRUE;
		}
	}

	while (!err && !done)
	{
		if ((err = Vdba_set_session(session_a,lpInf)) != OK)
		{
			++*n_err;
			continue;
		}

		EXEC SQL FETCH c2
			INTO	:database_no, :database_name, :vnode_name,
				:dbms_type;
		EXEC SQL INQUIRE_SQL (:done = ENDQUERY);
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)	{
			/* Error fetching from %s dd_databases cursor. */
			STprintf(tmp,lpInf->lpInStr->lpFetching,VdbaSessType(session_a,lpInf));//ResourceString(IDS_E_FETCHING)
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1423;
			++*n_err;
			continue;
		}
		else if (done)
		{
			continue;
		}

		STtrmwhite(vnode_name);
		STtrmwhite(database_name);
		STtrmwhite(dbms_type);

		if ((err = Vdba_set_session(session_b,lpInf)) != OK)
		{
			++*n_err;
			continue;
		}

		EXEC SQL REPEATED SELECT database_name, vnode_name, dbms_type
			INTO	:database_name_b, :vnode_name_b, :dbms_type_b
			FROM	dd_databases
			WHERE	database_no = :database_no;

		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)	{
			/*
			** Error checking %s dd_databases table for
			** database no %d.
			*/
			STprintf(tmp,lpInf->lpInStr->lpCheckingDDDB,VdbaSessType(session_b,lpInf),database_no);//ResourceString(IDS_E_CHECKING_DD_DB)
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1425;
			++*n_err;
			continue;
		}
		else if (errinfo.rowcount == 0)
		{
			/* DD_DATABASES ERROR: */
			/*ddba_messageit_text(tmp, 1426);*/
			STprintf(tmp,lpInf->lpInStr->DDDatabasesError);//ResourceString(IDS_E_DD_DATABASES_ERROR)
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

			/*
			** Record for database no %d, database name '%s'\n
			** not found in %s database.
			*/
			/*ddba_messageit_text(tmp, 1427, database_no,*/
				/*database_name, VdbaSessType(session_b,lpInf));*/
			STprintf(tmp,lpInf->lpInStr->lpRecordDB,database_no,database_name, VdbaSessType(session_b,lpInf));//ResourceString(IDS_E_RECORD_DB)
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
			++*n_err;
			continue;
		}

		STtrmwhite(vnode_name_b);
		STtrmwhite(database_name_b);
		STtrmwhite(dbms_type_b);

		if (session_a == lpInf->iLocal_Session)
		{
			if (STcompare(database_name_b, database_name))
			{
				/* DD_DATABASES ERROR: */
				/*ddba_messageit_text(tmp, 1426);*/
				STprintf(tmp,lpInf->lpInStr->DDDatabasesError);//ResourceString(IDS_E_DD_DATABASES_ERROR)
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** Database no %d has %s name of '%s',
				** but %s name of '%s'.ResourceString(IDS_F_DB_NAME)
				*/
				STprintf(tmp,lpInf->lpInStr->lpDbName, database_no,
				                                VdbaSessType(session_a,lpInf),
				                                database_name,
				                                VdbaSessType(session_b,lpInf),
				                                database_name_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}

			if (STcompare(vnode_name_b, vnode_name))
			{
				/* DD_DATABASES ERROR: */
				/*ddba_messageit_text(tmp, 1426);*/
				STprintf(tmp,lpInf->lpInStr->DDDatabasesError);//ResourceString(IDS_E_DD_DATABASES_ERROR)
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** Database no %d has %s vnode name of '%s',
				** but %s vnode name of '%s'.ResourceString(IDS_E_DBNO_AND_VNODE)
				*/
				STprintf(tmp,lpInf->lpInStr->lpDbNoAndVnode, database_no,
				                                      VdbaSessType(session_a,lpInf), vnode_name,
				                                      VdbaSessType(session_b,lpInf), vnode_name_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}

			if (STcompare(dbms_type_b, dbms_type))
			{
				/* DD_DATABASES ERROR: */
				/*ddba_messageit_text(tmp, 1426);*/
				STprintf(tmp,lpInf->lpInStr->DDDatabasesError);//ResourceString(IDS_E_DD_DATABASES_ERROR)
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** Database no %d has %s DBMS type of '%s',
				** but %s DBMS type of '%s'.ResourceString(IDS_E_DBNO_AND_DBMS)
				*/
				STprintf(tmp,lpInf->lpInStr->lpDbNoAndDBMS, database_no,
				                                     VdbaSessType(session_a,lpInf), dbms_type,
				                                     VdbaSessType(session_b,lpInf), dbms_type_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}
		}
	}

	i = Vdba_set_session(session_a,lpInf);
	if (i != OK)
	{
		++*n_err;
		if (!err)
			err = i;
	}
	else if (c2_open)
	{
		EXEC SQL CLOSE c2;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error closing cursor on %s dd_databases table. ResourceString(IDS_E_CLOSE_CURSOR_DD_DB)*/
			STprintf(tmp,lpInf->lpInStr->lpCloseCursorDDDB,VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			if (!err)
				err = 1430;
			++*n_err;
		}
	}

	return (err);
}


/*{
** Name:  Vdba_check_regist_tables - check registered tables
**
** Description:
**  Compare the dd_regist_tables tables in sess_a vs. sess_b.  Also checks
**  that columns have been registered & support tables created.
**
** Inputs:
**  sess_a  - Ingres session number
**  sess_b  - Ingres session number
**  n_err   - Error count
**  lpInf  - structure with generic informations (handle file, local session number,etc...)
**
** Outputs:
**  none
**
** Returns:
**  0 - OK
**  else - Error
*/
static STATUS Vdba_check_regist_tables( i4 sess_a, i4 sess_b, i4 *n_err,
                                        LPGENERICINFO lpInf)
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	session_a = sess_a;
	i4	session_b = sess_b;
	i4	table_no;
	char	table_name[DB_MAXNAME+1];
	char	table_name_b[DB_MAXNAME+1];
	char	table_owner[DB_MAXNAME+1];
	char	table_owner_b[DB_MAXNAME+1];
	char	registered[26];
	char	created[26];
	i2	cdds_no;
	i2	cdds_no_b;
	i4	done = 0;
	STATUS	err = 0;
	EXEC SQL END DECLARE SECTION;
	char	tmp[1024];
	char	tmpf[1024];

	DBEC_ERR_INFO	errinfo;
	bool		c3_open = FALSE;
	i4		i;

	if ((err = Vdba_set_session(session_a,lpInf)) != OK)
		++*n_err;

	if (!err)
	{
		EXEC SQL DECLARE c3 CURSOR FOR
			SELECT	table_no, table_name, table_owner, cdds_no
			FROM	dd_regist_tables
			ORDER	BY table_no;
		EXEC SQL OPEN c3 FOR READONLY;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error opening cursor on %s dd_regist_tables table. ResourceString( IDS_E_OPEN_CUR_DD_REG )*/
			STprintf(tmp, lpInf->lpInStr->lpOpenCurDDreg, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1432;
			++*n_err;
		}
		else
		{
			c3_open = TRUE;
		}
	}

	while (!err && !done)
	{
		if ((err = Vdba_set_session(session_a,lpInf)) != OK)
		{
			++*n_err;
			continue;
		}

		EXEC SQL FETCH c3
			INTO :table_no, :table_name, :table_owner, :cdds_no;
		EXEC SQL INQUIRE_SQL (:done = ENDQUERY);
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error fetching from %s dd_regist_tables cursor.ResourceString(IDS_E_FETCH_DD_REG) */
			STprintf(tmp,lpInf->lpInStr->lpFetchDDReg , VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1433;
			++*n_err;
			continue;
		}
		else if (done)
		{
			continue;
		}
		STtrmwhite(table_name);
		STtrmwhite(table_owner);

		if ((err = Vdba_set_session(session_b,lpInf)) != OK)
		{
			++*n_err;
			continue;
		}

		EXEC SQL REPEATED SELECT table_name, table_owner, cdds_no,
		         columns_registered, supp_objs_created
		    INTO :table_name_b, :table_owner_b, :cdds_no_b,
		         :registered, :created
		    FROM dd_regist_tables
		    WHERE table_no = :table_no;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/*
			** Error checking %s dd_regist_tables table for
			** table no %d.ResourceString(IDS_E_CHECK_DD_REG)
			*/
			STprintf(tmp, lpInf->lpInStr->lpCheckDDReg, VdbaSessType(session_b,lpInf),table_no);
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1435;
			++*n_err;
			continue;
		}
		else if (errinfo.rowcount == 0)
		{
			/* DD_REGIST_TABLES ERROR: ResourceString(IDS_E_DD_REG_TBL_ERROR)*/
			STprintf(tmp, lpInf->lpInStr->lpDDRegTblError);
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

			/*
			** Record for table no %d, table name '%s',
			** table owner '%s' not found in %s database.ResourceString(IDS_E_RECORD_NO_NOT_FOUND)
			*/
			STprintf(tmp,lpInf->lpInStr->lpRecordNoNotFound, table_no, table_name,
			                                          table_owner, VdbaSessType(session_b,lpInf));
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
			++*n_err;
			continue;
		}

		STtrmwhite(table_name_b);
		STtrmwhite(table_owner_b);
		STtrmwhite(created);
		STtrmwhite(registered);
		if (*created == EOS)
		{
			/* DD_REGIST_TABLES ERROR: */
			STprintf(tmp,lpInf->lpInStr->lpDDRegTblError);
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

			/*
			** Support tables not created for %s table no %d,
			** table name '%s', table owner '%s'.ResourceString(IDS_E_SUPPORT_TBL_NOT_CREATE)
			*/
			STprintf( tmp, lpInf->lpInStr->lpSupportTblNotCreate,
			          VdbaSessType(session_b,lpInf), table_no,
			          table_name_b, table_owner_b);
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
			++*n_err;
		}

		if (*registered == EOS)
		{
			/* DD_REGIST_TABLES ERROR: */
			STprintf(tmp, lpInf->lpInStr->lpDDRegTblError);//ResourceString(IDS_E_DD_REG_TBL_ERROR)
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

			/*
			** Columns have not been registered for %s table
			** no %d, table name '%s', table owner '%s'.ResourceString(IDS_E_COLUMN_REGISTRED)
			*/
			STprintf( tmp,lpInf->lpInStr->lpColumnRegistred, 
			          VdbaSessType(session_b,lpInf), table_no,
			          table_name_b, table_owner_b);
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
			++*n_err;
		}

		if (session_a == lpInf->iLocal_Session)
		{
			if (STcompare(table_name_b, table_name))
			{
				/* DD_REGIST_TABLES ERROR: */
				STprintf(tmp, lpInf->lpInStr->lpDDRegTblError);//ResourceString(IDS_E_DD_REG_TBL_ERROR)
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** Table no %d has %s name of '%s',
				** but %s name of '%s'.ResourceString(IDS_E_TBL_NAME_ERROR)
				*/
				STprintf( tmp, lpInf->lpInStr->lpTblNameError, table_no,
				          VdbaSessType(session_a,lpInf), table_name,
				          VdbaSessType(session_b,lpInf), table_name_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}

			if (STcompare(table_owner_b, table_owner))
			{
				/* DD_REGIST_TABLES ERROR: */
				STprintf(tmp, lpInf->lpInStr->lpDDRegTblError);//ResourceString(IDS_E_DD_REG_TBL_ERROR)
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** Table no %d has %s owner of '%s',
				** but %s owner of '%s'.ResourceString(IDS_E_TBL_OWNER_ERROR)
				*/
				STprintf(tmp,lpInf->lpInStr->lpTblOwnerError, table_no,
				         VdbaSessType(session_a,lpInf), table_owner,
				         VdbaSessType(session_b,lpInf), table_owner_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}

			if (cdds_no_b != cdds_no)
			{
				/* DD_REGIST_TABLES ERROR: */
				STprintf(tmp, lpInf->lpInStr->lpDDRegTblError);//ResourceString(IDS_E_DD_REG_TBL_ERROR)
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** Table no %d has %s CDDS no %d,
				** but %s CDDS no of %d.ResourceString(IDS_E_DB_CDDS_OPEN_CURSOR)
				*/
				STprintf(tmp,lpInf->lpInStr->lpDBCddsOpenCursor, table_no,
				         VdbaSessType(session_a,lpInf), cdds_no,
				         VdbaSessType(session_b,lpInf), cdds_no_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}
		}
	}

	i = Vdba_set_session(session_a,lpInf);
	if (i != OK)
	{
		++*n_err;
		if (!err)
			err = i;
	}
	else if (c3_open)
	{
		EXEC SQL CLOSE c3;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error closing cursor on %s dd_regist_tables table.ResourceString(IDS_E_DD_REGIST_CLOSE_CURSOR) */
			STprintf(tmp,lpInf->lpInStr->lpDDRegistCloseCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			if (!err)
				err = 1443;
			++*n_err;
		}
	}

	return (err);
}


/*{
** Name:  Vdba_check_regist_columns - check registered columns
**
** Description:
**  Compare the dd_regist_columns table in sess_a vs. sessb.
**
** Inputs:
**  sess_a - Ingres session number
**  sess_b - Ingres session number
**  n_err  - Error count
**  lpInf  - structure with generic informations (handle file, local session number,etc...)
**
** Outputs:
**  none
**
** Returns:
**  0 - OK
**  else - Error
*/
static STATUS Vdba_check_regist_columns(i4 sess_a,i4 sess_b,i4 *n_err,
                                        LPGENERICINFO lpInf)
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	session_a = sess_a;
	i4	session_b = sess_b;
	i4	table_no;
	char	table_name[DB_MAXNAME+1];
	char	column_name[DB_MAXNAME+1];
	i4	key_sequence;
	i4	key_sequence_b;
	i4	column_sequence;
	i4	column_sequence_b;
	i4	done = 0;
	STATUS	err = 0;
	EXEC SQL END DECLARE SECTION;
	char	tmp[1024];
	char	tmpf[1024];
	DBEC_ERR_INFO	errinfo;
	bool		c4_open = FALSE;
	i4		i;

	if ((err = Vdba_set_session(session_a,lpInf)) != OK)
		++*n_err;

	if (!err)
	{
		EXEC SQL DECLARE c4 CURSOR FOR
		    SELECT c.table_no, t.table_name, c.column_name,
		           c.key_sequence, c.column_sequence
		    FROM  dd_regist_columns c, dd_regist_tables t
		    WHERE c.table_no = t.table_no
		    ORDER BY table_no, column_name;

		EXEC SQL OPEN c4 FOR READONLY;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/*
			** Error opening cursor on %s dd_regist_columns
			** table.ResourceString(IDS_E_DD_REGIST_COL_OPEN_CURSOR)
			*/
			STprintf(tmp,lpInf->lpInStr->lpDDRegistColOpenCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1445;
			++*n_err;
		}
		else
		{
			c4_open = TRUE;
		}
	}

	while (!err && !done)
	{
		if ((err = Vdba_set_session(session_a,lpInf)) != OK)
		{
			++*n_err;
			continue;
		}

		EXEC SQL FETCH c4
		    INTO :table_no, :table_name, :column_name,
		         :key_sequence, :column_sequence;
		EXEC SQL INQUIRE_SQL (:done = ENDQUERY);

		STtrmwhite(column_name);

		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error fetching from %s dd_regist_columns cursor. ResourceString(IDS_E_DD_REGIST_COL_FETCH_CURSOR)*/
			STprintf(tmp, lpInf->lpInStr->lpDDRegistColFetchCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1446;
			++*n_err;
			continue;
		}
		else if (done)
			continue;

		if ((err = Vdba_set_session(session_b,lpInf)) != OK)
		{
			++*n_err;
			continue;
		}

		EXEC SQL REPEATED SELECT key_sequence, column_sequence
		    INTO :key_sequence_b, :column_sequence_b
		    FROM dd_regist_columns
		    WHERE table_no = :table_no
		    AND column_name = :column_name;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/*
			** Error checking %s dd_regist_columns table for
			** table no %d, column name '%s'.ResourceString(IDS_E_DD_REGIST_COL_CHECKING)
			*/
			STprintf( tmp,lpInf->lpInStr->lpDDRegistColChecking,
			          VdbaSessType(session_b,lpInf),
			          table_no, column_name);
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1447;
			++*n_err;
			continue;
		}
		else if (errinfo.rowcount == 0)
		{
			/* DD_REGIST_COLUMNS ERROR:ResourceString(IDS_E_DD_REGIST_COL_ERROR) */
			STprintf(tmp, lpInf->lpInStr->lpDDRegistColError);
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

			/*
			** Record for table no %d, column name '%s'\n
			** not found in %s database.ResourceString(IDS_E_RECORD_TABLE)
			*/
			STprintf( tmp, lpInf->lpInStr->lpRecordTable, table_no, column_name,
			          VdbaSessType(session_b,lpInf));
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
			++*n_err;
			continue;
		}

		if (session_a == lpInf->iLocal_Session)
		{
			if (key_sequence_b != key_sequence)
			{
				/* DD_REGIST_COLUMNS ERROR:ResourceString(IDS_E_DD_REGIST_COL_ERROR) */
				STprintf(tmp,lpInf->lpInStr->lpDDRegistColError);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** Table no %d, column name '%s' has %s key
				** sequence of %d, but %s key sequence of %d.ResourceString(IDS_E_KEY_SEQUENCE_ERROR)
				*/
				STprintf(tmp,lpInf->lpInStr->lpKeySequenceError , table_no, column_name,
				         VdbaSessType(session_a,lpInf), key_sequence,
				         VdbaSessType(session_b,lpInf), key_sequence_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}

			if (column_sequence_b != column_sequence)
			{
				/* DD_REGIST_COLUMNS ERROR: ResourceString(IDS_E_DD_REGIST_COL_ERROR)*/
				STprintf(tmp, lpInf->lpInStr->lpDDRegistColError);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** Table no %d, column name '%s' has %s column
				** sequence of %d, but %s column sequence of %d.ResourceString(IDS_E_COLUMN_SEQUENCE)
				*/
				STprintf(tmp, lpInf->lpInStr->lpColumnSequence,table_no, column_name,
				         VdbaSessType(session_a,lpInf), column_sequence,
				         VdbaSessType(session_b,lpInf), column_sequence_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}
		}
	}

	i = Vdba_set_session(session_a,lpInf);
	if (i != OK)
	{
		++*n_err;
		if (!err)
			err = i;
	}
	else if (c4_open)
	{
		EXEC SQL CLOSE c4;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/*
			** Error closing cursor on %s dd_regist_columns
			** table.ResourceString(IDS_E_REGIST_COL_CLOSE_CURSOR)
			*/
			STprintf(tmp, lpInf->lpInStr->lpRegistColCloseCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			if (!err)
				err = 1751;
			++*n_err;
		}
	}

	return (err);
}


/*{
** Name:  Vdba_check_cdds - check cdds
**
** Description:
**  Compare the dd_cdds tables in sess_a vs. sess_b.
**
** Inputs:
**  sess_a - Ingres session number
**  sess_b - Ingres session number
**  n_err  - Error count
**  lpInf  - structure with generic informations (handle file, local session number,etc...)
**
** Outputs:
** none
**
** Returns:
** 0 - OK
** else - Error
*/
static STATUS Vdba_check_cdds( i4 sess_a,i4 sess_b,i4 *n_err,
                               LPGENERICINFO lpInf)
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4 session_a = sess_a;
	i4 session_b = sess_b;
	i2 cdds_no;
	char cdds_name[DB_MAXNAME+1];
	char cdds_name_b[DB_MAXNAME+1];
	i2 collision_mode;
	i2 collision_mode_b;
	i2 error_mode;
	i2 error_mode_b;
	i4 done = 0;
	STATUS err = 0;
	EXEC SQL END DECLARE SECTION;
	char tmp[1024],tmpf[1024];

	DBEC_ERR_INFO errinfo;
	BOOL c5_open = FALSE;
	i4   i;

	if ((err = Vdba_set_session(session_a,lpInf)) != OK)
		++*n_err;

	if (!err)
	{
		EXEC SQL DECLARE c5 CURSOR FOR
		    SELECT cdds_no, cdds_name, collision_mode, error_mode
		    FROM   dd_cdds
		    ORDER  BY cdds_no;
		EXEC SQL OPEN c5 FOR READONLY;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error opening cursor on %s dd_cdds table. ResourceString(IDS_E_CDDS_OPEN_CURSOR)*/
			STprintf(tmp, lpInf->lpInStr->lpCddsOpenCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1410;
			++*n_err;
		}
		else
		{
			c5_open = TRUE;
		}
	}

	while (!err && !done)
	{
		if ((err = Vdba_set_session(session_a,lpInf)) != OK)
		{
			++*n_err;
			continue;
		}

		EXEC SQL FETCH c5
		    INTO :cdds_no, :cdds_name, :collision_mode,
		         :error_mode;
		EXEC SQL INQUIRE_SQL (:done = ENDQUERY);
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error fetching from %s dd_cdds cursor.ResourceString(IDS_E_CDDS_FETCH_CURSOR) */
			STprintf(tmp, lpInf->lpInStr->lpCddsFetchCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1412;
			++*n_err;
			continue;
		}
		else if (done)
		{
			continue;
		}

		STtrmwhite(cdds_name);
		if ((err = Vdba_set_session(session_b,lpInf)) != OK)
		{
			++*n_err;
			continue;
		}

		EXEC SQL REPEATED SELECT cdds_name, collision_mode, error_mode
		    INTO :cdds_name_b, :collision_mode_b, :error_mode_b
		    FROM dd_cdds
		    WHERE cdds_no = :cdds_no;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error checking %s dd_cdds table for CDDS no %d. ResourceString(IDS_E_CDDS_CHECKING)*/
			STprintf(tmp,lpInf->lpInStr->lpCddsChecking, VdbaSessType(session_b,lpInf), cdds_no);
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1417;
			++*n_err;
			continue;
		}
		else if (errinfo.rowcount == 0)
		{
			/* DD_CDDS ERROR:  ResourceString(IDS_E_CDDS_CHECK_ERROR)*/
			STprintf(tmp, lpInf->lpInStr->lpCddsCheckError);
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

			/*
			** Record for CDDS no %d, CDDS name '%s'\n
			** not found in %s database.ResourceString(IDS_E_CDDS_NO_FOUND)
			*/
			STprintf(tmp, lpInf->lpInStr->lpCddsNoFound, cdds_no, cdds_name, VdbaSessType(session_b,lpInf));
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
			++*n_err;
			continue;
		}

		STtrmwhite(cdds_name_b);
		if (session_a == lpInf->iLocal_Session)
		{
			if (STcompare(cdds_name_b, cdds_name))
			{
				/* DD_CDDS ERROR:  ResourceString(IDS_E_CDDS_CHECK_ERROR)*/
				STprintf(tmp,lpInf->lpInStr->lpCddsCheckError);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** CDDS no %d has %s name of '%s',
				** but %s name of '%s'.ResourceString(IDS_E_CDDS_NAME)
				*/
				STprintf( tmp, lpInf->lpInStr->lpCddsName, cdds_no,
				          VdbaSessType(session_a,lpInf), cdds_name,
				          VdbaSessType(session_b,lpInf), cdds_name_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}

			if (collision_mode_b != collision_mode)
			{
				/* DD_CDDS ERROR:ResourceString(IDS_E_CDDS_CHECK_ERROR) */
				STprintf(tmp, lpInf->lpInStr->lpCddsCheckError);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** CDDS no %d has %s collision mode of %d,
				** but %s collision mode of %d.ResourceString(IDS_E_CDDS_COLLISION)
				*/
				STprintf(tmp, lpInf->lpInStr->lpCddsCollision, cdds_no,
				         VdbaSessType(session_a,lpInf), collision_mode,
				         VdbaSessType(session_b,lpInf), collision_mode_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}

			if (error_mode_b != error_mode)
			{
				/* DD_CDDS ERROR: ResourceString(IDS_E_CDDS_CHECK_ERROR)*/
				STprintf(tmp, lpInf->lpInStr->lpCddsCheckError);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** CDDS no %d has %s error mode of %d,
				** but %s error mode of %d.ResourceString(IDS_E_CDDS_ERRORMODE)
				*/
				STprintf( tmp, lpInf->lpInStr->lpCddsErrormode, cdds_no,
				          VdbaSessType(session_a,lpInf), error_mode,
				          VdbaSessType(session_b,lpInf), error_mode_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}
		}
	}

	i = Vdba_set_session(session_a,lpInf);
	if (i != OK)
	{
		++*n_err;
		if (!err)
			err = i;
	}
	else if (c5_open)
	{
		EXEC SQL CLOSE c5;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error closing cursor on %s dd_cdds table. ResourceString(IDS_E_DD_CDDS_CLOSE_CURSOR)*/
			STprintf(tmp, lpInf->lpInStr->lpDDCddsCloseCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			if (!err)
				err = 1464;
			++*n_err;
		}
	}

	return (err);
}


/*{
** Name:	Vdba_check_db_cdds - check db_cdds
**
** Description:
**  Compare the dd_db_cdds tables in sess_a vs. sess_b.
**
** Inputs:
**  sess_a - Ingres session number
**  sess_b - Ingres session number
**  n_err  - Error count
**  lpInf  - structure with generic informations (handle file, local session number,etc...)
**
** Outputs:
**  none
**
** Returns:
**  0 - OK
**  else - Error
*/
static STATUS Vdba_check_db_cdds( i4 sess_a,i4 sess_b,i4 *n_err,
                                  LPGENERICINFO lpInf)
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	session_a = sess_a;
	i4	session_b = sess_b;
	i2	cdds_no;
	i2	database_no;
	i2	target_type;
	i2	target_type_b;
	i4	done = 0;
	STATUS	err = 0;
	EXEC SQL END DECLARE SECTION;
	char	tmp[1024];
	char	tmpf[1024];

	DBEC_ERR_INFO	errinfo;
	bool		c6_open = FALSE;
	i4		i;

	if ((err = Vdba_set_session(session_a,lpInf)) != OK)
		++*n_err;

	if (!err)
	{
		EXEC SQL DECLARE c6 CURSOR FOR
		    SELECT cdds_no, database_no, target_type
		    FROM   dd_db_cdds
		    ORDER  BY cdds_no, database_no;
		EXEC SQL OPEN c6 FOR READONLY;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error opening cursor on %s dd_db_cdds table.  ResourceString(IDS_E_CHECK_DB_CDDS_OPEN_CURSOR)*/
			STprintf(tmp, lpInf->lpInStr->lpCheckDBCddsOpenCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1745;
			++*n_err;
		}
		else
		{
			c6_open = TRUE;
		}
	}

	while (!err && !done)
	{
		if ((err = Vdba_set_session(session_a,lpInf)) != OK)
		{
			++*n_err;
			continue;
		}

		EXEC SQL FETCH c6
			INTO :cdds_no, :database_no, :target_type;
		EXEC SQL INQUIRE_SQL (:done = ENDQUERY);
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error fetching from %s dd_db_cdds cursor. ResourceString(IDS_E_CDDS_FETCHING_CURSOR)*/
			STprintf(tmp, lpInf->lpInStr->lpCddsFetchingCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1746;
			++*n_err;
			continue;
		}
		else if (done)
		{
			continue;
		}

		if ((err = Vdba_set_session(session_b,lpInf)) != OK)
		{
			++*n_err;
			continue;
		}

		EXEC SQL REPEATED SELECT target_type
			INTO	:target_type_b
			FROM	dd_db_cdds
			WHERE	cdds_no = :cdds_no
			AND	database_no = :database_no;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/*
			** Error checking %s dd_db_cdds table for
			** CDDS no %d, database no %d.ResourceString(IDS_E_CDDS_CHECKING_DB_NO)
			*/
			STprintf(tmp,lpInf->lpInStr->lpCddsCheckingDBno, 
			         VdbaSessType(session_b,lpInf),cdds_no, database_no);
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1747;
			++*n_err;
			continue;
		}
		else if (errinfo.rowcount == 0)
		{
			/* DD_DB_CDDS ERROR: ResourceString(IDS_E_CDDS_ERROR)*/
			STprintf(tmp, lpInf->lpInStr->lpCddsError);
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

			/*
			** Record for CDDS no %d, database no %d\r\n
			** not found in %s database.ResourceString(IDS_E_CDDS_DB_NO)
			*/
			STprintf(tmp,lpInf->lpInStr->lpCddsDBno, cdds_no, database_no,
			         VdbaSessType(session_b,lpInf));
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
			++*n_err;
			continue;
		}

		if (session_a == lpInf->iLocal_Session)
		{
			if (target_type_b != target_type)
			{
				/* DD_DB_CDDS ERROR: ResourceString(IDS_E_CDDS_ERROR)*/
				STprintf(tmp, lpInf->lpInStr->lpCddsError);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** CDDS no %d, database no %d has %s target
				** type of %d, but %s target type of %d.ResourceString(IDS_E_CDDS_TARGET_TYPE)
				*/
				STprintf(tmp, lpInf->lpInStr->lpCddsTargetType, cdds_no, database_no,
				         VdbaSessType(session_a,lpInf), target_type,
				         VdbaSessType(session_b,lpInf), target_type_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}
		}
	}

	i = Vdba_set_session(session_a,lpInf);
	if (i != OK)
	{
		++*n_err;
		if (!err)
			err = i;
	}
	else if (c6_open)
	{
		EXEC SQL CLOSE c6;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error closing cursor on %s dd_db_cdds table.ResourceString(IDS_E_CDDS_CLOSE_CURSOR) */
			STprintf(tmp, lpInf->lpInStr->lpCddsCloseCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			if (!err)
				err = 1750;
			++*n_err;
		}
	}

	return (err);
}


/*{
** Name:  Vdba_check_paths - check paths
**
** Description:
**  Compare the dd_paths tables in sess_a vs. sess_b.
**
** Inputs:
**  sess_a - Ingres session number
**  sess_b - Ingres session number
**  n_err  - Error count
**  lpInf  - structure with generic informations (handle file, local session number,etc...)
**
** Outputs:
**  none
**
** Returns:
**  0 - OK
**  else - Error
*/
static STATUS Vdba_check_paths( i4 sess_a,i4 sess_b,i4 *n_err,
                                LPGENERICINFO lpInf)
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	session_a = sess_a;
	i4	session_b = sess_b;
	i2	cdds_no;
	i2	localdb;
	i2	sourcedb;
	i2	targetdb;
	i2	final_target;
	i2	final_target_b;
	i4	done = 0;
	STATUS	err = 0;
	EXEC SQL END DECLARE SECTION;
	char	tmp[1024];
	char	tmpf[1024];

	DBEC_ERR_INFO errinfo;
	bool c7_open = FALSE;
	i4 i;

	if ((err = Vdba_set_session(session_a,lpInf)) != OK)
		++*n_err;

	if (!err)
	{
		EXEC SQL DECLARE c7 CURSOR FOR
		    SELECT cdds_no, localdb, sourcedb, targetdb,
		           final_target
		    FROM dd_paths
		    ORDER BY cdds_no, localdb, sourcedb, targetdb;
		EXEC SQL OPEN c7 FOR READONLY;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error opening cursor on %s dd_paths table.ResourceString(IDS_E_PATH_OPEN_CURSOR) */
			STprintf(tmp, lpInf->lpInStr->lpPathOpenCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1752;
			++*n_err;
		}
		else
		{
			c7_open = TRUE;
		}
	}

	while (!err && !done)
	{
		if ((err = Vdba_set_session(session_a,lpInf)) != OK)
		{
			++*n_err;
			continue;
		}

		EXEC SQL FETCH c7
		    INTO :cdds_no, :localdb, :sourcedb, :targetdb,
		         :final_target;
		EXEC SQL INQUIRE_SQL (:done = ENDQUERY);
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error fetching from %s dd_paths cursor.ResourceString(IDS_E_PATH_FETCHING_CURSOR) */
			STprintf(tmp, lpInf->lpInStr->lpPathFetchingCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1753;
			++*n_err;
			continue;
		}
		else if (done)
		{
			continue;
		}

		if ((err = Vdba_set_session(session_b,lpInf)) != OK)
		{
			++*n_err;
			continue;
		}

		EXEC SQL REPEATED SELECT final_target
		    INTO    :final_target_b
		    FROM    dd_paths
		    WHERE   cdds_no = :cdds_no
		    AND localdb  = :localdb
		    AND sourcedb = :sourcedb
		    AND targetdb = :targetdb;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/*
			** Error checking %s dd_paths table for
			** CDDS no %d, localdb no %d, sourcedb no %d,
			** targetdb no %d.ResourceString(IDS_E_DD_PATH_TBL)
			*/
			STprintf(tmp,lpInf->lpInStr->lpDDPathTbl ,VdbaSessType(session_b,lpInf),
			                                   cdds_no, localdb, sourcedb, targetdb);
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			err = 1754;
			++*n_err;
			continue;
		}
		else if (errinfo.rowcount == 0)
		{
			/* DD_PATHS ERROR:ResourceString(IDS_E_DD_PATH_ERROR) */
			STprintf(tmpf, lpInf->lpInStr->lpDDPathError);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

			/*
			** Record for CDDS no %d, localdb no %d,
			** sourcedb no %d, targetdb no %d not found in %s
			** database.ResourceString(IDS_E_RECORD_CDDS_NO)
			*/
			STprintf(tmp, lpInf->lpInStr->lpRecordCddsNo, cdds_no, localdb,
			         sourcedb, targetdb, VdbaSessType(session_b,lpInf));
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
			++*n_err;
			continue;
		}

		if (session_a == lpInf->iLocal_Session)
		{
			if (final_target_b != final_target)
			{
				/* DD_PATHS ERROR: ResourceString(IDS_E_DD_PATH_ERROR)*/
				STprintf(tmpf, lpInf->lpInStr->lpDDPathError);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));

				/*
				** CDDS no %d, localdb no %d,
				** sourcedb no %d, targetdb no %d\r\n
				** has %s final_target of %d,
				** but %s final_target of %d.ResourceString(IDS_E_CDDS_LOCAL_NO)
				*/
				STprintf(tmp, lpInf->lpInStr->lpCddsLocalNo, cdds_no,
				         localdb, sourcedb, targetdb,
				         VdbaSessType(session_a,lpInf), final_target,
				         VdbaSessType(session_b,lpInf), final_target_b);
				STprintf(tmpf, ERx("%s\r\n"), tmp);
				Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
				++*n_err;
			}
		}
	}

	i = Vdba_set_session(session_a,lpInf);
	if (i != OK)
	{
		++*n_err;
		if (!err)
			err = i;
	}
	else if (c7_open)
	{
		EXEC SQL CLOSE c7;
		if (Vdba_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != RES_SUCCESS)
		{
			/* Error closing cursor on %s dd_paths table.ResourceString(IDS_E_CLOSING_CURSOR) */
			STprintf(tmp, lpInf->lpInStr->lpClosingCursor, VdbaSessType(session_a,lpInf));
			Vdba_Check_StoreSqlError(tmp,errinfo,"",lpInf);
			if (!err)
				err = 1758;
			++*n_err;
		}
	}

	return (err);
}


/*{
** Name:	Vdba_set_session - set session
**
** Description:
**    Set the current session.
**
** Inputs:
**    sess_no - Session number to set to
**    lpInf  - structure with generic informations (handle file, local session number,etc...)
**
** Outputs:
**    none
**
** Returns:
**   OK
**   else - Error
*/
static STATUS Vdba_set_session(i4 sess_no, LPGENERICINFO lpInf)
{
	char tmp[1024];
	char tmpf[1024];

	if ( ActivateSession(sess_no) != RES_SUCCESS) {
		/* Error setting to %s session %d. ResourceString(IDS_E_SETTING_SESSION)*/
		if ((VOID *)lpInf->hdlfile != NULL)	{
			STprintf(tmp,lpInf->lpInStr->lpSettingSession,VdbaSessType(sess_no,lpInf),sess_no);
			STprintf(tmpf, ERx("%s\r\n"), tmp);
			Vdba_WriteBufferInFile(lpInf->hdlfile,tmpf,lstrlen(tmpf));
		}
		return (1416);
	}
	return (OK);
}

/*{
** Name:    Vdba_Check_StoreSqlError
**
** Description:
**        Save the sql error text in temporary file.
**
** Inputs:
**      sess_no      - Session number to set to
**      TextError    - Text for explain error.
**      errinfo      - filled by Vdba_error_check().
**      DefaultText  - display this text error if errinfo.errorno == 0
**      lpInf        - structure with generic informations (handle file, local session number,etc...)
**
** Outputs:
**    none
**
** Returns:
**    none
**
*/
static void Vdba_Check_StoreSqlError( char *Error, DBEC_ERR_INFO errinfo ,
                                      char *DefaultText, LPGENERICINFO lpInf)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char  errortext[257];
	EXEC SQL END DECLARE SECTION;
	char tmpText[1024*2];
	LPTSTR BufDest = NULL;

	if ( x_strlen(DefaultText) && errinfo.errorno == 0 )
		x_strcpy(errortext,DefaultText);
	else
		EXEC SQL INQUIRE_SQL (:errortext = ERRORTEXT);

	BufDest = Vdba_DuplicateStringWithCRLF(errortext);
	if (BufDest)
	{
		STprintf(tmpText, ERx("%s\r\n%s\r\n"), Error,BufDest );
		ESL_FreeMem(BufDest);
	}
	else
		STprintf(tmpText, ERx("%s\r\n%s\r\n"), Error,errortext );
	Vdba_WriteBufferInFile(lpInf->hdlfile,tmpText,lstrlen(tmpText));
}
