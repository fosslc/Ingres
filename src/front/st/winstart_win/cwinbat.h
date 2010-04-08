/*
**  Copyright (C) 1997, 2000 Ingres Corporation
*/

/*
**
**  Name: cwinbat.h
**
**  Description:
**  This file implements the cwinbat class header
**
**  History:
**
**  10-jan-00 (cucjo01)
**      Removed extraneous comma at end of enum statement to compile on UNIX
*/

#include "resource.h"

#ifndef __CWINBAT_H__
#define __CWINBAT_H__

enum WINBATRC { 
	WINBAT_OK, WINBAT_MOREDATA, WINBAT_NOCMDLINE = IDS_NOCMDLINE, 
	WINBAT_CREATEPIPEERROR = IDS_CREATEPIPEERROR, 
	WINBAT_CREATEPROCESSERROR = IDS_CREATEPROCESSERROR, 
	WINBAT_READFILEERROR = IDS_READFILEERROR, 
	WINBAT_WRITEFILEERROR = IDS_WRITEFILEERROR,
	WINBAT_CREATETHREADERROR = IDS_CREATETHREADERROR,
	WINBAT_DUPLICATEHANDLEEERROR = IDS_DUPLICATEHANDLEEERROR,
	WINBAT_GETEXITERROR = IDS_GETEXITERROR, 
	WINBAT_CLOSEHANDLEERROR = IDS_CLOSEHANDLEERROR,
	WINBAT_TERMINATETHREADERROR = IDS_TERMINATETHREADERROR,
	WINBAT_TERMINATINGEXEC = IDS_TERMINATINGEXEC,
	WINBAT_AREYOUSURE = IDS_AREYOUSURE,
	WINBAT_TERMINATEPROCESSERROR = IDS_TERMINATEPROCESSERROR
};

class CWinBat
{

public:

		  		CWinBat(char* lpCmdLine = NULL);
	WINBATRC	WinBatStart(void);
	WINBATRC 	WinBatRead(char *lpBuffer, DWORD dwBufferSize, DWORD *lpdwBytesRead);
	WINBATRC 	WinBatWrite(char *lpszBuffer, DWORD *lpdwWritten);
	DWORD		WinBatGetError(void) { return m_dwLastError; };
	void		WinBatSetError(DWORD dwError) { m_dwLastError = dwError; };
	BOOL		WinBatRunning(void);
	DWORD 		WinBatTerminate(void);
	void		WinBatSetCmdLine(char *);
	void		WinBatCloseAll(void); 
	void		WinBatSetThread(CWinThread *lpcThread) { m_lpcThread = lpcThread; };
	CWinThread*	WinBatGetThread(void) { return m_lpcThread; };
	BOOL 		GetThreadTerminated(void) { return m_lpcThread ? m_bThreadTerminated: TRUE;};	
	void 		SetThreadTerminated(BOOL bThreadTerminated) 
								{ m_bThreadTerminated = bThreadTerminated; };	

// Implementation

protected:
//	char	   			   *m_lpCmdLine;	
	char					m_chCmdLine[256];
	STARTUPINFO             m_siStartInfo;
 	PROCESS_INFORMATION     m_piProcInfo;
	DWORD					m_dwLastError, m_dwRead;
	SECURITY_ATTRIBUTES		m_saAttr;
	HANDLE					m_hChildStdinRd, m_hChildStdinWr,
							m_hChildStdoutRd, m_hChildStdoutWr;
	CWinThread 			   *m_lpcThread;
	BOOL					m_bThreadTerminated;
};

#endif
