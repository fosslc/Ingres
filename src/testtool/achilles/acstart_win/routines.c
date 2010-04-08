/*
** routines.c
**
** Support routines for NT version of ACSTART
**
** History
**	06-Feb-96 (bocpa01)
**		Created.
**	05-Jun-97 (cohmi01)
**		Generalized to run with Ingres or Jasmine.
**		New ACHILLES_SUT env var may be set to select
**		the system-under-test.
**	07-Aug-2009 (drivi01)
**		Disable deprecated POSIX functions warning 4996 
**		by adding pragma.
**/
/* Turn off POSIX warning for this file until Microsoft fixes this bug */
#pragma warning(disable: 4996)


#include "acstart.h"


/* Begin - items for specific products being tested */
#define PROD_INGRES	0  
#define PROD_JASMINE	1  
static  int	Prod = PROD_INGRES;	/* assume testing ingres */

char *ProductNames[] = 	 {"INGRES",	"JASMINE",	NULL};
char *SystemvarNames[] = {"II_SYSTEM",	"JAS_SYSTEM" };
char *NameutilNames[] =	 {"IINAMU",	"JASNAMU" };

#define PRODUCT_NAME	(ProductNames[Prod])
#define SYSTEMVAR_NAME	(SystemvarNames[Prod])  
#define NAMEUTIL_NAME	(NameutilNames[Prod])


/* End - items for specific products being tested */

#define VARLENGTH		128
#define NUMBEROFARGS	 10

HANDLE	hInstance = 0;
DWORD	pid = 0,	ret_val = 0;
BOOL	debug_sw			= FALSE,	interactive_sw		= FALSE,		
		repeat_sw			= FALSE,	output_sw			= FALSE,	
		user_sw				= TRUE,		get_out				= FALSE, 
		need_repeat			= FALSE,	need_output			= FALSE,		
		need_user			= FALSE, 	need_cfg			= TRUE,		
		need_db				= TRUE,		need_time			= FALSE, 
		time_sw				= FALSE,	ach_result_exists	= FALSE,	
		ach_config_exists	= FALSE,	need_vnd			= TRUE,
		need_dbn			= TRUE,		need_svr			= TRUE,
		have_vnd			= FALSE,	have_dbn			= FALSE,
		have_svr			= FALSE,	skip_vn				= TRUE,
        need_prot			= TRUE,		need_remnode		= TRUE,
		need_listen			= TRUE,		check_db			= FALSE;
int		i,	repeat_val=9999,	time_val=9999;

char 	output_val[STRINGLENGTH]		= "",	user_val[STRINGLENGTH]			= "", 
		test_case[STRINGLENGTH]			= "",	cfg_file[STRINGLENGTH]			= "", 
		achilles_config[STRINGLENGTH]	= "",	achilles_result[STRINGLENGTH]	= "", 
		dbspec[STRINGLENGTH]			= "",	host[STRINGLENGTH]				= "",
		fname[STRINGLENGTH]				= "",	fn_root[STRINGLENGTH]			= "",
		childlog[STRINGLENGTH]			= "",	log_file[STRINGLENGTH]			= "",
		rpt_file[STRINGLENGTH]			= "",	vnode[STRINGLENGTH]				= "",
		dname[STRINGLENGTH]				= "",	server[STRINGLENGTH]			= "",
		entry_type[STRINGLENGTH]		= "",	net_protocol[STRINGLENGTH]		= "",
		rem_node[STRINGLENGTH]			= "",	list_addr[STRINGLENGTH]			= "";

int String2Integer(char *pszValue) {

	int length = strlen(pszValue), i;

	for (i = 0; i < length && isdigit(*(pszValue + i)); i++);
	if (i < length)
		return -1;
	return atoi(pszValue);
}

PCHAR Substitute(PCHAR psz, PCHAR pszFrom, PCHAR pszTo, CHAR flag) {

	static	char  chNewValue[STRINGLENGTH];
			char *pszNewValue = chNewValue;
	int		cbFromLength = strlen(pszFrom), cbToLength = strlen(pszTo);

	memset(chNewValue, '\0', sizeof(chNewValue));
	while (*psz) {
		if (strncmp(psz, pszFrom, cbFromLength))
			*pszNewValue++ = *psz++;
		else {
			strcat(pszNewValue, pszTo);
			psz += cbFromLength;
			pszNewValue += cbToLength;
			if (flag != 'g') {
				strcat(pszNewValue, psz);
				pszNewValue += strlen(psz);
				break;
			}
		}
	}
	*pszNewValue = '\0';
	return chNewValue;
}

VOID GetString(UINT uID, LPTSTR lpBuffer, int nBufferMax) {

	if (hInstance == 0)
		hInstance = GetModuleHandle(NULL);
	LoadString(hInstance, uID, lpBuffer, nBufferMax);
}

BOOL RunUtility(PCHAR pszUtility, PCHAR pszCommand,
				PCHAR lpBuffer, DWORD dwBufferSize) {

	HANDLE					htmpChildStdinWr,	hChildStdinRd,	hChildStdinWr,
							hChildStdoutRd,		hLog;
	SECURITY_ATTRIBUTES		saAttr;
	PROCESS_INFORMATION     piProcInfo;
	DWORD					dwBytesWritten, dwBytesRead = 0;
	STARTUPINFO             siStartInfo;

	if (!pszUtility || !*pszUtility)
		return FALSE;

	// init security attributes

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;	 		// make pipe handles inheritable
	saAttr.lpSecurityDescriptor = NULL;
	
	// create a pipe for the child's STDOUT

	if (!CreatePipe(&hChildStdoutRd, &hLog, &saAttr, 0)) {
		ret_val = GetLastError();
		return FALSE;
	}

	// create a pipe for the child's STDIN

	if (!CreatePipe(&hChildStdinRd, &htmpChildStdinWr, &saAttr, 0)) {
		ret_val = GetLastError();
		return FALSE;
	}

	// duplicate the write handle to the STDIN pipe so it's not inherited

	if (!DuplicateHandle(GetCurrentProcess(), htmpChildStdinWr, 
						 GetCurrentProcess(), &hChildStdinWr, 
						 0, FALSE, DUPLICATE_SAME_ACCESS)) {
		ret_val = GetLastError();
		return FALSE;
	}
	CloseHandle(htmpChildStdinWr);					// don't need it any more

	// create the child process				

	siStartInfo.cb			= sizeof(STARTUPINFO);
   	siStartInfo.lpReserved	= NULL;
   	siStartInfo.lpReserved2 = NULL;
   	siStartInfo.cbReserved2 = 0;
   	siStartInfo.lpDesktop	= NULL;
	siStartInfo.lpTitle		= NULL;
   	siStartInfo.dwFlags		= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
   	siStartInfo.wShowWindow = SW_HIDE;
	siStartInfo.hStdInput	= hChildStdinRd;
	siStartInfo.hStdOutput	= hLog;
	siStartInfo.hStdError	= hLog;
   	if (!CreateProcess (NULL, pszUtility, NULL, NULL, TRUE, 
    					   NORMAL_PRIORITY_CLASS, NULL, NULL, 
    					   &siStartInfo, &piProcInfo)) {
		ret_val = GetLastError();
		return FALSE;
	}
	CloseHandle(hChildStdinRd);
	CloseHandle(hLog);

	if (!WriteFile(hChildStdinWr, pszCommand, strlen(pszCommand), 
						&dwBytesWritten, NULL)) {
		ret_val = GetLastError();		
		return FALSE;
	}
	CloseHandle(hChildStdinWr);
	while (ReadFile(hChildStdoutRd, lpBuffer, dwBufferSize, &dwBytesRead, NULL) &&
			dwBytesRead) {
		lpBuffer += dwBytesRead;
		dwBufferSize -= dwBytesRead;
	}
	*lpBuffer = '\0';
	CloseHandle(hChildStdoutRd);
	WaitForSingleObject(piProcInfo.hProcess, INFINITE);
	CloseHandle(piProcInfo.hProcess);
	GetExitCodeProcess(piProcInfo.hProcess, &ret_val);
	return TRUE;
}

UINT InitEnvironment() {

	CHAR	chBuffer[128] = "";
	CHAR	**ppch;

	ach_result_exists = 
		GetEnvironmentVariable("ACHILLES_RESULT", achilles_result, 
								sizeof(achilles_result));
	ach_config_exists = 
		GetEnvironmentVariable("ACHILLES_CONFIG", achilles_config, 
								sizeof(achilles_config));

	/* Determine the 'system-under-test'. If no ACHILLES_SUT 	*/
	/* var set, assume it is Ingres					*/
	if (GetEnvironmentVariable("ACHILLES_SUT", chBuffer, sizeof(chBuffer))) 
	{
	    for (ppch = ProductNames; *ppch; ppch++)
		if (strcmp(*ppch, chBuffer) == 0)
		    break;
	    if (! *ppch)
		return IDS_BAD_SUT;  /* didnt find a legal product name in var */
	    else
		Prod = (char *) (ppch - ProductNames);  /* index of product name in array */
	}
	else
	    Prod = PROD_INGRES;
	printf("\nACSTART: System Under Test is: %s\n", PRODUCT_NAME);
	    

	if ((GetEnvironmentVariable(SYSTEMVAR_NAME, chBuffer, sizeof(chBuffer)) == 0)) 
		return IDS_NOIISYSTEM;
	return 0;
}

BOOL ProcessParameters() {

	CHAR	chBuffer[STRINGLENGTH] = "", tmp[STRINGLENGTH], *pszNextToken;
	DWORD	dwFileAttributes = 0, dw;

	if (need_repeat) {
		repeat_val = 9999;
		need_repeat = FALSE;
		repeat_sw = TRUE;
	}
	if (need_time) {
		time_val = 9999;
		need_time = FALSE;
		time_sw = TRUE;
	}

	if ((dwFileAttributes  = GetFileAttributes(test_case)) == 0xFFFFFFFF) 
		if (ach_config_exists) {
			sprintf(chBuffer, "%s\\%s", achilles_config, test_case);
			strcpy(test_case, chBuffer);
			if ((dwFileAttributes  = GetFileAttributes(test_case)) == 0xFFFFFFFF) {
				Message(MSGTYPE_ERROR, IDS_CANNOTFIND, test_case);
				return FALSE;
			}
		} else {
			Message(MSGTYPE_ERROR, IDS_CANNOTFIND, test_case);
			return FALSE;
		}

	if (dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY ||
		dwFileAttributes == FILE_ATTRIBUTE_SYSTEM) {
		Message(MSGTYPE_ERROR, IDS_CANNOTREAD, test_case);
		return FALSE;
	}

	if (!output_sw && ach_result_exists) {
		strcpy(output_val, achilles_result);
		output_sw = TRUE;		
	}
	if (output_sw) {
		if ((dwFileAttributes  = GetFileAttributes(output_val)) == 0xFFFFFFFF ||
			dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
			Message(MSGTYPE_WARNING, IDS_NODIRECTORY, output_val);
			if (!CreateDirectory(output_val, NULL)) {
				sprintf(chBuffer, "%s\nError code = %d", output_val, GetLastError());
				Message(MSGTYPE_ERROR, IDS_CANNOTCREATEDIRECTORY, chBuffer);
				return FALSE;
			}
		}
	} else {
		Message(MSGTYPE_ERROR, IDS_NOOUTPUTPATH, NULL);
		return FALSE;
	}
	if (strcmp(output_val, ".\\") == 0 || strcmp(output_val, ".") == 0) {
		GetCurrentDirectory(sizeof(chBuffer), chBuffer);
		strcpy(output_val, chBuffer);
	}

	pid = GetCurrentProcessId();
	dw = sizeof(host);
	GetComputerName(host, &dw);
	sprintf(fname, "%s%d", host, pid);
	sprintf(fn_root, "%s\\%s", Substitute(output_val, "\\\\", "\\", '\0'), fname);
	sprintf(cfg_file, "%s.config", fn_root);
	sprintf(log_file, "%s.log", fn_root);
	sprintf(rpt_file, "%s.rpt", fn_root);
	strcpy(childlog, fname);
  
	need_vnd = TRUE;  
	need_dbn = TRUE;  
	need_svr = TRUE;
	have_vnd = FALSE; 
	have_dbn = FALSE; 
	have_svr = FALSE;		
	strcpy(tmp, Substitute(dbspec, " ", "", '\0'));
	strcpy(tmp, Substitute(dbspec, "::", " ", '\0'));
	pszNextToken = strtok(tmp, " ");
	while (pszNextToken) {
		if (need_vnd) {
			strcpy(vnode, pszNextToken);
			need_vnd = FALSE;
			have_vnd = TRUE;
		} else if (need_dbn) {
			strcpy(dname, pszNextToken);
			need_dbn = FALSE; 
			have_dbn = TRUE;
		}
		pszNextToken = strtok(NULL, " ");
	}
	if (need_dbn) {
		strcpy(dname, vnode);
		have_dbn = TRUE;
		need_dbn = FALSE;
		have_vnd = FALSE;
		need_vnd = TRUE;
		strcpy(vnode, "");
	}						
	strcpy(tmp, Substitute(dname, "\\", " ", '\0'));
	need_dbn = TRUE; 
	have_dbn = FALSE;
	pszNextToken = strtok(tmp, " "); 
	while (pszNextToken) {
		if (need_dbn) {
			strcpy(dname, pszNextToken);
			need_dbn = FALSE; 
			have_dbn = TRUE;
        } else if (need_svr) {
			strcpy(server, pszNextToken);
			need_svr = FALSE; 
			have_svr = TRUE;
		}
		pszNextToken = strtok(NULL, " ");
	}						
	if (need_dbn) {
		Message(MSGTYPE_ERROR, IDS_WRONGDBSPEC, dbspec);
		return FALSE;
	}						
	if (have_svr && strcmp(server, "ingres") == 0) {
		need_svr = TRUE;
		have_svr = FALSE;
		strcpy(server, "");
	}
	if (have_vnd) 
		sprintf(tmp, "%s::%s", vnode, dname);
	else 
		strcpy(tmp, dname);
	if (have_svr)
		sprintf(tmp + strlen(tmp), "/%s", server);
	strcpy(dbspec, tmp);  
	return TRUE;
}

CHAR chUtilityOutputBuffer[1024 * 4];

PCHAR IsRunning(PCHAR pszServer) {

	CHAR	*pszLine = NULL;

	sprintf(chUtilityOutputBuffer, "show %s", pszServer);
	if (!RunUtility(NAMEUTIL_NAME, chUtilityOutputBuffer, chUtilityOutputBuffer, 
					sizeof(chUtilityOutputBuffer))) 
			return FALSE;
	pszLine = strtok(chUtilityOutputBuffer, "\n");
	while (pszLine && (strstr(pszLine, NAMEUTIL_NAME) == 0 || 
					   strstr(pszLine, pszServer) == 0)) 
		pszLine = strtok(NULL, "\n");
	return pszLine;
}

BOOL PrepareRun() {

	PCHAR	pszServer = NULL, pszCurrentLine = NULL, pchBuffer = NULL, 
			pszCurrentToken = NULL, pSemi, pszArg[NUMBEROFARGS], 
			pszNextLine;
	CHAR	chLine[STRINGLENGTH] = "";
	HANDLE	hConfig = 0, hTestCase = 0;	
	DWORD	cbTestCaseSize, cbBytesRead, dwBytesWritten;
	int		i;

	if (have_vnd) {
		if ((pszServer = IsRunning("COMSVR")) == NULL) {
			Message(MSGTYPE_ERROR, IDS_NOCOMSVRS, NULL);
			return FALSE;
		}
	} else if (need_svr) {
		if ((pszServer = IsRunning(PRODUCT_NAME)) == NULL) {
			Message(MSGTYPE_ERROR, IDS_NODBMSSERVER, NULL);
			return FALSE;
		}
	} else if (stricmp(server, "star") == 0) {
		if ((pszServer = IsRunning("STAR")) == NULL) {
			Message(MSGTYPE_ERROR, IDS_NOSTARSERVER, NULL);
			return FALSE;
		}
	}	
	pszCurrentToken = strtok(pszServer, " ");
	while (pszCurrentToken) {
		if (skip_vn)
			skip_vn = FALSE;
		else if (need_prot) {
			strcpy(net_protocol, pszCurrentToken);
			need_prot = FALSE;
		} else if (need_remnode) {
			strcpy(rem_node, pszCurrentToken);
			need_remnode = FALSE;
		} else if (need_listen) {
			strcpy(list_addr, pszCurrentToken);
			need_listen = FALSE;
		}
		pszCurrentToken = strtok(NULL, " ");
	}
	
	/*#############################################################################
	# Check that remote db is available
	#############################################################################*/

	if (check_db) {

		CHAR chCmdLine[STRINGLENGTH];

		sprintf(chCmdLine, "sql.bat %s", dbspec);
		if (!RunUtility(chCmdLine, "\\q", chUtilityOutputBuffer, 
					sizeof(chUtilityOutputBuffer))) 
			return FALSE;
		if (ret_val != 0) {
			Message(MSGTYPE_ERROR, IDS_CANNOTACCESSDB, dbspec);
			return FALSE;
		}
	}

	/*#############################################################################
	#
	#	Translate template file into config file. This is dependent on input 
	#	switches, repeat_sw and user_sw, that override certain defaults..
	#
	#############################################################################*/


	if ((hTestCase = CreateFile(test_case, GENERIC_READ, 0, NULL, 
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) == 0) {

		CHAR	chBuffer[STRINGLENGTH];
		DWORD	dwErrorCode = GetLastError();
		
		sprintf(chBuffer, "%s. Eror code = %d", test_case, dwErrorCode);
		Message(MSGTYPE_ERROR, IDS_CANNOTOPENCONFIG, chBuffer);
		return FALSE;
	}

	if ((hConfig = CreateFile(cfg_file, GENERIC_WRITE, 0, NULL, 
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) == 0) {

		CHAR	chBuffer[STRINGLENGTH];
		DWORD	dwErrorCode = GetLastError();
		
		sprintf(chBuffer, "%s. Eror code = %d", cfg_file, dwErrorCode);
		Message(MSGTYPE_ERROR, IDS_CANNOTOPENCONFIG, chBuffer);
		return FALSE;
	}

	if ((cbTestCaseSize = GetFileSize(hTestCase, NULL)) == 0xFFFFFFFF) {

		CHAR	chBuffer[STRINGLENGTH];
		DWORD	dwErrorCode = GetLastError();
		
		sprintf(chBuffer, "%s. Eror code = %d", test_case, dwErrorCode);
		Message(MSGTYPE_ERROR, IDS_CANNOTGETCONFIGSIZE, chBuffer);
		CloseHandle(hConfig);
		return FALSE;
	}

	if ((pchBuffer = LocalAlloc(LPTR, cbTestCaseSize + 16)) == NULL) {

		CHAR	chBuffer[128];

		Message(MSGTYPE_ERROR, IDS_CANNOTALLOC, itoa(cbTestCaseSize, chBuffer, 10));
		CloseHandle(hConfig);
		return FALSE;
	}

	if (!ReadFile(hTestCase, pchBuffer, cbTestCaseSize, &cbBytesRead, NULL)) {

		CHAR	chBuffer[STRINGLENGTH];
		DWORD	dwErrorCode = GetLastError();
		
		sprintf(chBuffer, "%s. Eror code = %d", test_case, dwErrorCode);
		Message(MSGTYPE_ERROR, IDS_CANNOTREADCONFIG, chBuffer);
		CloseHandle(hConfig);
		LocalFree(pchBuffer);
		return FALSE;
	}
	CloseHandle(hTestCase);

	pszCurrentLine = pchBuffer;	
	if ((pszNextLine = strchr(pszCurrentLine, '\n')) == NULL)
		pszNextLine = pszCurrentLine + strlen(pszCurrentLine);
	else {
		*pszNextLine = '\0';
		pszNextLine++;
	}
	while (*pszCurrentLine) {
		if (*pszCurrentLine != '#' && *pszCurrentLine != '$' && *pszCurrentLine != 0x0d) {
			chLine[0] = '\0';
			pSemi = strchr(pszCurrentLine, ';'); 
			i = 0;
			for (pszCurrentToken = pszCurrentLine; *pszCurrentToken; pszCurrentToken++) {
				if (*pszCurrentToken == '\t' || *pszCurrentToken == 0x0d)
					*pszCurrentToken = ' ';
			}
			if (strstr(test_case, ".config")) {
			} else {
				pszArg[i++] = pszCurrentLine;
				if (pSemi) {
					*(pszCurrentLine + (pSemi - pszCurrentLine)) = '\0';
					pszCurrentToken = strtok(pSemi + 1, " ");
				} else {
					pszCurrentToken = strtok(pszCurrentLine, " ");
					pszCurrentToken = strtok(NULL, " ");
				}
				while (pszCurrentToken && i < NUMBEROFARGS) {
					pszArg[i++] = pszCurrentToken;
					pszCurrentToken = strtok(NULL, " ");
				}
				sprintf(chLine, "%s %s %s", pszArg[0], "<DBNAME>", pszArg[1]);
				sprintf(chLine + strlen(chLine), " %s %s %s %s %s", 
					pszArg[2], "<CHILDNUM>", pszArg[3], pszArg[4], ";");
			    sprintf(chLine + strlen(chLine), " %s %d %s %s %s", 
					pszArg[5], repeat_val, pszArg[6], pszArg[7], pszArg[8]);
				sprintf(chLine + strlen(chLine), " %s %s %s %s\n", 
					pszArg[9], "<NONE>", childlog, user_val);
			}
			if (!WriteFile(hConfig, chLine, strlen(chLine), &dwBytesWritten, NULL)) {
			
				CHAR	chBuffer[STRINGLENGTH];
				DWORD	dwErrorCode = GetLastError();
		
				sprintf(chBuffer, "%s. Eror code = %d", test_case, dwErrorCode);
				Message(MSGTYPE_ERROR, IDS_CANNOTWRITECONFIG, chBuffer);
				CloseHandle(hConfig);
				LocalFree(pchBuffer);
				return FALSE;
			}
		}
 		pszCurrentLine = pszNextLine;
		if ((pszNextLine = strchr(pszCurrentLine, '\n')) == NULL)
			pszNextLine = pszCurrentLine + strlen(pszCurrentLine);
		else {
			*pszNextLine = '\0';
			pszNextLine++;
		}
	}	
	CloseHandle(hConfig);
 	LocalFree(pchBuffer);
 	return TRUE;
}

DWORD StartAchilles(BOOL bInteractive) {

	/* set default time to 9999 - (i.e infinite) */

	CHAR				chCmdLine[STRINGLENGTH];
	SECURITY_ATTRIBUTES	saAttr;
	PROCESS_INFORMATION	piProcInfo;
	STARTUPINFO			siStartInfo;
	HANDLE				hLog = 0, hLogDup = 0;

	// init security attributes

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;	 		// make log handle inheritable
	saAttr.lpSecurityDescriptor = NULL;

	// prepare command line

	sprintf(chCmdLine, "achilles -l -s -d %s -f %s -o %s -a %d",
			dbspec, cfg_file, output_val, time_val);

	if (!bInteractive) {

		// open log file

		if ((hLog = CreateFile(log_file, GENERIC_WRITE, FILE_SHARE_READ, &saAttr,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {

			CHAR	chBuffer[STRINGLENGTH];
			DWORD	dwErrorCode = GetLastError();
		
			sprintf(chBuffer, "%s. Eror code = %d", log_file, dwErrorCode);
			Message(MSGTYPE_ERROR, IDS_CANNOTOPENLOG, chBuffer);
			CloseHandle(hLog);
			return 0;
		}
		if (!DuplicateHandle(GetCurrentProcess(), hLog, GetCurrentProcess(), &hLogDup,
				0, FALSE, DUPLICATE_SAME_ACCESS)) {

			CHAR	chBuffer[STRINGLENGTH];
			DWORD	dwErrorCode = GetLastError();
		
			sprintf(chBuffer, ". Eror code = %d", dwErrorCode);
			Message(MSGTYPE_ERROR, IDS_CANNOTDUPLICATELOG, chBuffer);
			CloseHandle(hLog);
			return 0;
		}
		CloseHandle(hLog);
	}
	siStartInfo.cb			= sizeof(STARTUPINFO);
	siStartInfo.lpReserved	= NULL;
	siStartInfo.lpReserved2 = NULL;
	siStartInfo.cbReserved2	= 0;
	siStartInfo.lpDesktop	= NULL;
	siStartInfo.lpTitle		= NULL;
	siStartInfo.dwFlags		= bInteractive ? 
								STARTF_USESHOWWINDOW : 
								STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	siStartInfo.wShowWindow = bInteractive ? SW_SHOW: SW_HIDE;
	siStartInfo.hStdInput	= 0;
	siStartInfo.hStdOutput	= hLogDup;
	siStartInfo.hStdError	= hLogDup;
	if (!CreateProcess (NULL, chCmdLine, NULL, NULL, TRUE, 
					    NORMAL_PRIORITY_CLASS, NULL, NULL, 
    					&siStartInfo, &piProcInfo)) {

		CHAR	chBuffer[STRINGLENGTH];
		DWORD	dwErrorCode = GetLastError();
		
		sprintf(chBuffer, "%s. Eror code = %d", chCmdLine, dwErrorCode);
		Message(MSGTYPE_ERROR, IDS_CANNOTSTARTACHILLES, chBuffer);
		return 0;
	}
	if (bInteractive)
		WaitForSingleObject(piProcInfo.hProcess, INFINITE);
	return piProcInfo.dwProcessId;
}
