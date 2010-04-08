// cwinbat.cpp : implementation file
//

#include "stdafx.h"
#include "cwinbat.h"

 CWinBat::CWinBat(char* lpCmdLine /*= NULL*/) //: m_csCmdLine('\0', 256) //m_lpCmdLine(lpCmdLine)
{
	WinBatSetCmdLine(lpCmdLine);
}	
							 
void CWinBat::WinBatSetCmdLine(char *lpCmdLine) 
{ 
	if (lpCmdLine)
		strcpy(m_chCmdLine, lpCmdLine); 
	else
		m_chCmdLine[0] = '\0';
}

WINBATRC CWinBat::WinBatStart(void)
{

	HANDLE hChildStdinWr;

	if (!m_chCmdLine[0])
		return WINBAT_NOCMDLINE;

	// init security attributes

	m_saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	m_saAttr.bInheritHandle = TRUE;	 		// make pipe handles inheritable
	m_saAttr.lpSecurityDescriptor = NULL;
	
	// create a pipe for the child's STDOUT

	if (!CreatePipe(&m_hChildStdoutRd, &m_hChildStdoutWr, &m_saAttr, 0)) {
		m_dwLastError = GetLastError();
		return WINBAT_CREATEPIPEERROR;
	}

	// create a pipe for the child's STDIN

	if (!CreatePipe(&m_hChildStdinRd, &hChildStdinWr, &m_saAttr, 0)) {
		m_dwLastError = GetLastError();
		return WINBAT_CREATEPIPEERROR;
	}

	// duplicate the write handle to the STDIN pipe so it's not inherited

	if (!DuplicateHandle(GetCurrentProcess(), hChildStdinWr, 
						 GetCurrentProcess(), &m_hChildStdinWr, 
						 0, FALSE, DUPLICATE_SAME_ACCESS)) {
		m_dwLastError = GetLastError();
		return WINBAT_DUPLICATEHANDLEEERROR;
	}
	CloseHandle(hChildStdinWr);					// don't need it any more

	// create the child process				

	m_siStartInfo.cb = sizeof(STARTUPINFO);
   	m_siStartInfo.lpReserved = NULL;
   	m_siStartInfo.lpReserved2 = NULL;
   	m_siStartInfo.cbReserved2 = 0;
   	m_siStartInfo.lpDesktop = NULL;
	m_siStartInfo.lpTitle = NULL;
   	m_siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
   	m_siStartInfo.wShowWindow = SW_HIDE;
	m_siStartInfo.hStdInput = m_hChildStdinRd;
	m_siStartInfo.hStdOutput = m_hChildStdoutWr;
	m_siStartInfo.hStdError = m_hChildStdoutWr;
   	if (!CreateProcess (NULL, m_chCmdLine, NULL, NULL, TRUE, 
    					   NORMAL_PRIORITY_CLASS, NULL, NULL, 
    					   &m_siStartInfo, &m_piProcInfo)) {
		m_dwLastError = GetLastError();
		return WINBAT_CREATEPROCESSERROR;
	}
	CloseHandle(m_hChildStdinRd);
	CloseHandle(m_hChildStdoutWr);
	return WINBAT_OK;

}
WINBATRC CWinBat::WinBatRead(char *lpBuffer, DWORD dwBufferSize, DWORD *lpdwBytesRead)
{

	// Check if it's still running
	
//	if (!WinBatRunning ())					// doesn't work on SMP machine
//		return WINBAT_OK;

	// read output from the child

	if (!ReadFile(m_hChildStdoutRd, lpBuffer, dwBufferSize, lpdwBytesRead, NULL)) {
		m_dwLastError = GetLastError();		
		return WINBAT_READFILEERROR;
	}
	*(lpBuffer + *lpdwBytesRead) = '\0';
//	m_dwLastError = GetLastError();		
//	sprintf(lpBuffer + strlen(lpBuffer), "\r\nrc=%d\r\n", m_dwLastError);
//	if (dwBufferSize == *lpdwBytesRead)
		return WINBAT_MOREDATA;
//	return WINBAT_OK;
}
WINBATRC CWinBat::WinBatWrite(char *lpszBuffer, DWORD *lpdwWritten)
{
	if (!WriteFile(m_hChildStdinWr, lpszBuffer, (DWORD)strlen(lpszBuffer), 
						lpdwWritten, NULL)) {
		m_dwLastError = GetLastError();		
		return WINBAT_WRITEFILEERROR;
	}
	return WINBAT_OK;
}

BOOL CWinBat::WinBatRunning(void) 
{
	DWORD dwExitCode;

	if (GetExitCodeProcess(m_piProcInfo.hProcess, &dwExitCode))
		return (dwExitCode == STILL_ACTIVE);
	return FALSE;
}

DWORD CWinBat::WinBatTerminate(void)
{
	if (!TerminateProcess(m_piProcInfo.hProcess, 0)) {
		m_dwLastError = GetLastError();
		return WINBAT_TERMINATEPROCESSERROR;
	}
	CloseHandle(m_piProcInfo.hProcess);
	return WINBAT_OK;
}

void CWinBat::WinBatCloseAll(void) 
{
	CloseHandle(m_hChildStdinWr);
	CloseHandle(m_hChildStdoutRd);
}
