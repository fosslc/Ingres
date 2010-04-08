/*****************************************************************************
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : adbtofst.c
**    Project  : adbtofst.exe executable
**
**    Purpose  : This program is used by RMCMD to execute auditdb followed by
**               fastload, where auditdb generates a temporary file (through
**               -file) and fastload takes this file as input (through -file
**               also).
**               It is built only on windows platforms, since under unix a
**               shell script is used instead
**
**    usage : adbtofst [-uusername] fload_table fload_dbname audited_table 
**                     audited_database [other_auditdb_parameters]
**
**         -uusername:      : specify userid for fastload and auditdb commands
**         fload_table      : fastload table.
**         fload_dbname     : fastload database name
**         audited_table    : auditdb table name
**         audited_database : auditdb database name
**         [other_auditdb_parameters] : other auditdb parameters, if any
**
** History:
** 22-Jul-2004 (noifr01)
**    created through simplification of previous cpp code
*****************************************************************************/


#include "compat.h"
#include "st.h"
#include "lo.h"
#include "nm.h"
#include "er.h"

static STATUS ExecCmd(char * lpszCmd);

int main(int argc, char* argv[])
{
	char *tmpFileName;
	char *dba;
	char *FloadTable;
	char *FloadDbname;
	char *AuditedTable;
	char *AuditedDatabase;
	char otherparams[1000];
	char auditdbcmd[1000];
	char fastloadcmd[1000];

	i4 iBegin, iresult, i;
	LOCATION LocTempFile;

	int nRetCode = 2;

	if (argc < 5 || (strcmp(argv[1], "-?") == 0) ||	(strcmp(argv[1], "?") == 0) )
	{
		printf(ERx("usage : adbtofst [-uusername] fload_table fload_dbname db_table audited_database [other_auditdb_parameters]"));
		return 2;
	}

	// Get II_TEMPORARY path
	if ( NMloc(TEMP, 0, (char *)NULL, &LocTempFile) == OK)
	{
		// Generate temporary file name
		if ( LOuniq( ERx( "adb" ), ERx( "trl" ), &LocTempFile ) == OK)
		{
			LOtos( &LocTempFile, &tmpFileName);
			if (tmpFileName[0]!=EOS)
				nRetCode=0;
		}
	}
	if (nRetCode!=0)
	{
		printf(ERx("Error Generating Temporary File Name\n"));
		return (nRetCode);
	}

	if (STbcompare(argv[1],0,"-u",2,0)==0)
	{
		dba             = argv[1];
		FloadTable      = argv[2];
		FloadDbname     = argv[3];
		AuditedTable    = argv[4];
		AuditedDatabase = argv[5];
		iBegin=6;
	}
	else
	{
		dba             = "";
		FloadTable      = argv[1];
		FloadDbname     = argv[2];
		AuditedTable    = argv[3];
		AuditedDatabase = argv[4];
		iBegin=5;
	}

	otherparams[0]=EOS;
	for(i=iBegin;i < argc; i++)
	{
		STcat(otherparams,argv[i]);
		STcat(otherparams,ERx(" "));
	}

	STprintf(auditdbcmd, "auditdb %s %s -table=%s -file=\"\\\"%s\\\"\" %s",dba,
	                                                                AuditedDatabase,
	                                                                AuditedTable,
	                                                                tmpFileName,
	                                                                otherparams);
	STprintf(fastloadcmd,"fastload %s %s -table=%s -file=\"\\\"%s\\\"\"",dba,
	                                                                FloadDbname,
	                                                                FloadTable,
	                                                                tmpFileName);

	iresult = ExecCmd(auditdbcmd);
	if (iresult!=OK)
		nRetCode=3; /* no error display here, since ExecCmd is supposed to have displayed one */
	else
		ExecCmd(fastloadcmd);

	if (LOdelete(&LocTempFile) != OK)
		printf("temporary file %s could not be deleted.\n",tmpFileName);

	return nRetCode;
}


static STATUS ExecCmd(char * lpszCmd)
{
	BOOL bResult;
	HANDLE hRead, hWrite;
	STARTUPINFO si;
	PROCESS_INFORMATION ProcInfo;

	#define NSIZE 256
	char tchszBuffer [NSIZE];
	DWORD dwlenRead;

	SECURITY_ATTRIBUTES sa;
	memset (&sa, 0, sizeof (sa));
	sa.nLength				= sizeof(sa);
	sa.bInheritHandle		= TRUE;
	sa.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&hRead, &hWrite, &sa, 0)) 
	{
		printf(ERx("Failed to Create Pipe\n"));
		return FAIL;
	}
	
	si.cb          = sizeof(STARTUPINFO);
	si.lpReserved  = NULL;
	si.lpReserved2 = NULL;
	si.cbReserved2 = 0;
	si.lpDesktop   = NULL;
	si.lpTitle     = NULL;
	si.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdInput   = GetStdHandle (STD_INPUT_HANDLE);
	si.hStdOutput  = hWrite;
	si.hStdError   = hWrite;

	bResult = CreateProcess(
		NULL,
		(LPTSTR)lpszCmd,
		NULL,
		NULL,
		TRUE ,
		CREATE_NEW_CONSOLE ,
		NULL,
		NULL,
		&si,
		&ProcInfo
	);
	CloseHandle(hWrite);

	if (!bResult)
	{
		printf(ERx("Failed to Create Process\n"));
		return FAIL;
	}

	while (ReadFile(hRead, tchszBuffer, (NSIZE-2), &dwlenRead, NULL))
	{
		if (dwlenRead > 0)
		{
			tchszBuffer[dwlenRead] = EOS;
			printf(tchszBuffer);
		}
		else
		{
			DWORD dw = 0;
			if (!GetExitCodeProcess(ProcInfo.hProcess, &dw)) 
			{
				printf(ERx("process execution failed.\n"));
				return FALSE;
			}
			if (dw != STILL_ACTIVE)
				break;
			else
				Sleep (1000);
		}
	}
	return (OK);
}
