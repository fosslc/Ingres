/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : fileioex.cpp , Implementation File 
**  Project  : Ingres II / Visual Manager
**  Author   : Sotheavut UK (uk$so01)
**  Purpose  : Data Manipulation Langage of IVM (exec process with redirect
**	       input:output)
**
**  History:
**	07-Jul-2000 (uk$so01)
**	    Created for BUG #102010
**	04-Jun-2002 (hanje04)
**	    Define tchszReturn to be 0x0a 0x00 for MAINWIN builds to stop
**	    unwanted ^M's in generated text files.
**	02-feb-2004 (somsa01)
**	    Updated to build with .NET 2003 compiler.
*/

#include "stdafx.h"
#include "fileioex.h"
#ifdef NT_GENERIC
#include <fstream>
using namespace std;
#else
#include <fstream.h>
#endif
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static BOOL stream_isopen (ifstream& f)
{
#if defined (MAINWIN)
	return (f)? TRUE: FALSE;
#else
	return f.is_open();
#endif
}

//
// Excute the process using files or buffer to 
// redirect input/output.
BOOL FILEIO_ExecCommand (CaFileIOExecParam* pParam)
{
	BOOL bSuccess = TRUE;
	HANDLE hChildStdoutRd = NULL, hChildStdoutWr = NULL;
	HANDLE hChildStdinRd = NULL, hChildStdinWr = NULL;
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcInfo;
	DWORD dwLastError = 0;
	CString strFail  = _T("Execute process failed !");
	CString strResult = _T("");
	DWORD dwWrite = 0;
	ASSERT (pParam);
	if (!pParam)
		return FALSE;

	LPCTSTR lpszCmd = pParam->m_strCommand;
	LPCTSTR lpszFileIn = pParam->m_strInputArg;
	LPCTSTR lpszFileOut = pParam->m_strOutputArg;

	try
	{
#if defined (SIMULATION)
		ASSERT (FALSE);
		return TRUE;
#endif
		//
		// Init security attributes
		SECURITY_ATTRIBUTES sa;
		memset (&sa, 0, sizeof (sa));
		sa.nLength              = sizeof(sa);
		sa.bInheritHandle       = TRUE; // make pipe handles inheritable
		sa.lpSecurityDescriptor = NULL;

		//
		// Create a pipe for the child's STDOUT
		if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &sa, 0)) 
		{
			dwLastError = GetLastError();
			LPVOID lpMsgBuf;
			FormatMessage (
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dwLastError,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR)&lpMsgBuf,
				0,
				NULL);
			strFail = (LPCTSTR)lpMsgBuf;
			LocalFree(lpMsgBuf);
			bSuccess = FALSE;
			pParam->m_nErrorCode = 4; // Fail to create pipe
			pParam->m_strError   = strFail;
			throw (LPCTSTR)strFail;
		}
		//
		// Create a pipe for the child's STDIN
		if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &sa, 0)) 
		{
			dwLastError = GetLastError();
			LPVOID lpMsgBuf;
			FormatMessage (
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dwLastError,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR)&lpMsgBuf,
				0,
				NULL);
			strFail = (LPCTSTR)lpMsgBuf;
			LocalFree(lpMsgBuf);
			bSuccess = FALSE;
			pParam->m_nErrorCode = 4; // Fail to create pipe
			pParam->m_strError   = strFail;
			throw (LPCTSTR)strFail;
		}

		//
		// Check if we can access the file as input:
		if ((pParam->m_nFlag & INPUT_ASFILE) && _taccess(lpszFileIn, 0) == -1)
		{
			bSuccess = FALSE;
			pParam->m_nErrorCode = 1; // Cannot acces input file name
		}

		//
		// Write data to the child process input:
		if (bSuccess)
		{
			if (pParam->m_nFlag & INPUT_ASFILE)
			{
				TCHAR tchszText [512];
#ifdef MAINWIN
				TCHAR tchszReturn [] = {0x0D, 0x0A, 0x00};
#else
				TCHAR tchszReturn [] = {0x0A, 0x00};
#endif
				CString strText = _T("");
				ifstream in (lpszFileIn, ios::binary);
				if (!stream_isopen(in))
				{
					pParam->m_nErrorCode = 2; // Cannot open input file name
					bSuccess = FALSE;
				}

				while (bSuccess && in.peek() != _TEOF)
				{
					in.read (tchszText, 1024);
					bSuccess = WriteFile (hChildStdinWr, tchszText, 1024, &dwWrite, NULL);
				}
			}
		}

		if (bSuccess)
		{
			//
			// Create the child process
			StartupInfo.cb = sizeof(STARTUPINFO);
			StartupInfo.lpReserved = NULL;
			StartupInfo.lpReserved2 = NULL;
			StartupInfo.cbReserved2 = 0;
			StartupInfo.lpDesktop = NULL;
			StartupInfo.lpTitle = NULL;
			StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			StartupInfo.wShowWindow = SW_HIDE;
			StartupInfo.hStdInput = hChildStdinRd;
			StartupInfo.hStdOutput = hChildStdoutWr;
			StartupInfo.hStdError = hChildStdoutWr;

			bSuccess = CreateProcess(
				NULL,                    // pointer to name of executable module
				(LPTSTR)lpszCmd,         // pointer to command line string
				NULL,                    // pointer to process security attributes
				NULL,                    // pointer to thread security attributes
				TRUE ,                   // handle inheritance flag
				CREATE_NEW_CONSOLE,      // creation flags
				NULL,                    // pointer to new environment block
				NULL,                    // pointer to current directory name
				&StartupInfo,            // pointer to STARTUPINFO
				&ProcInfo                // pointer to PROCESS_INFORMATION
			);

			if (!bSuccess)
			{
				dwLastError = GetLastError();
				LPVOID lpMsgBuf;
				FormatMessage (
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					dwLastError,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR)&lpMsgBuf,
					0,
					NULL);
				strFail = (LPCTSTR)lpMsgBuf;
				LocalFree(lpMsgBuf);
				bSuccess = FALSE;
				pParam->m_nErrorCode = 5; // Fail to create process
				pParam->m_strError   = strFail;
				throw (LPCTSTR)strFail;
			}
		}
		CloseHandle(hChildStdinRd);
		CloseHandle(hChildStdoutWr);
		hChildStdinRd = INVALID_HANDLE_VALUE ;
		hChildStdoutWr = INVALID_HANDLE_VALUE;

		if (bSuccess && !(pParam->m_nFlag & INPUT_ASFILE) && !pParam->m_strInputArg.IsEmpty())
		{
			bSuccess = WriteFile (hChildStdinWr, pParam->m_strInputArg, pParam->m_strInputArg.GetLength(), &dwWrite, NULL);
			if (!bSuccess)
				pParam->m_nErrorCode = 6; // Cannot write to pipe
		}

		WaitForSingleObject (ProcInfo.hProcess, 1500);

		if (bSuccess)
		{
			//
			// Reading data from the pipe:
			CloseHandle (hChildStdinWr);
			hChildStdinWr = NULL;
			int nTried = 0;
			const int nSize = 1024;
			TCHAR tchszBuffer [nSize];
			DWORD dwBytesRead;

			while (bSuccess && ReadFile(hChildStdoutRd, tchszBuffer, nSize-2, &dwBytesRead, NULL))
			{
				if (dwBytesRead > 0)
				{
					tchszBuffer [(int)dwBytesRead] = _T('\0');
					strResult+= tchszBuffer;
					tchszBuffer[0] = _T('\0');
				}

				DWORD dwExitCode = 0;
				if (!GetExitCodeProcess(ProcInfo.hProcess, &dwExitCode)) 
				{
					bSuccess = FALSE;
					pParam->m_nErrorCode = 7; // Cannot read from pipe
					break;
				}

				if (dwExitCode != STILL_ACTIVE && dwBytesRead < (nSize-2))
					break;
				else
				if (dwBytesRead ==  (nSize-2))
				{
					Sleep (500);
					nTried++;
					if (nTried > 2*60)
					{
						bSuccess = FALSE;
						pParam->m_nErrorCode = -1; // Generic error
						if (dwExitCode == STILL_ACTIVE)
							TerminateProcess (ProcInfo.hProcess, 0);
					}
				}
				
			}
		}

		if (bSuccess && !(pParam->m_nFlag & OUTPUT_ASFILE))
		{
			pParam->m_strOutputArg = strResult;
		}

		//
		// Write the result to the output file:
		if (bSuccess && (pParam->m_nFlag & OUTPUT_ASFILE))
		{
			ofstream out (lpszFileOut, ios::trunc);
			if (!out)
			{
				pParam->m_nErrorCode = 3; // Cannot create output file
				bSuccess = FALSE;
			}
			if (bSuccess)
			{
				out << (LPCTSTR)strResult;
			}
		}
	}
	catch (LPCTSTR lpszStr)
	{
		TRACE1 ("FILEIO_ExecCommand: \n", lpszStr);
		lpszStr = NULL;
		bSuccess = FALSE;
	}
	catch (...)
	{
		TRACE0 ("FILEIO_ExecCommand failed \n");
		pParam->m_nErrorCode = -1; // Generic error
		bSuccess = FALSE;
	}

	if (ProcInfo.hProcess != INVALID_HANDLE_VALUE )
		CloseHandle (ProcInfo.hProcess);
	if (ProcInfo.hThread != INVALID_HANDLE_VALUE )
		CloseHandle (ProcInfo.hThread);
	if (hChildStdoutRd != INVALID_HANDLE_VALUE )
		CloseHandle(hChildStdoutRd);
	if (hChildStdoutWr != INVALID_HANDLE_VALUE )
		CloseHandle(hChildStdoutWr);
	if (hChildStdinRd != INVALID_HANDLE_VALUE )
		CloseHandle(hChildStdinRd);
	if (hChildStdinWr != INVALID_HANDLE_VALUE )
		CloseHandle(hChildStdinWr);
	return bSuccess;
}
