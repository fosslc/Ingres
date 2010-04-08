/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : fileerr.cpp : implementation file.
**    Project  : INGRES II/ SQL /Test.
**    Author   : Schalk Philippe (schph01)
**    Purpose  : implementation of the CaFileError and CaSqlErrorInfo class.
**
** History:
**
** 01-Sept-2004 (schph01)
**    Added
** 11-Oct-2004 (schph01)
**    #106648 Removed un-needed inclusion of cr.h and pm.h, that were
**    preventing the build to work if these files weren't in the header path
*/

#include "stdafx.h"
#include "resource.h"
#include "fileerr.h"
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
extern "C"
{
#include <compat.h>
#include <gl.h>
#include <ercl.h>
# include <iicommon.h>
#include <nm.h>
}
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
// CaSqlErrorInfo

CaSqlErrorInfo::~CaSqlErrorInfo()
{
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
BOOL CaFileError::SetLocalFileName()
{
	USES_CONVERSION;
	TCHAR szFileId  [_MAX_PATH];
	CString csTempPath;
	char* penv;
# ifndef VMS
	NMgtAt("II_TEMPORARY", &penv );
	if( penv == NULL || *penv == EOS )
		csTempPath = _T(".");
	else
		csTempPath = A2T(penv);
# else
	csTempPath = TEMP_DIR;
# endif

# ifdef NT_GENERIC
	csTempPath += _T("\\");
#endif
	if (GetTempFileName(csTempPath, _T("dba"), 0, szFileId) == 0)
		return FALSE;

	m_strLocalSqlErrorFileName = szFileId;
	return TRUE;
}

BOOL CaFileError::SetGlobalFileName()
{
	TCHAR* penv = _tgetenv(_T("II_SYSTEM"));
	if (!penv || *penv == _T('\0'))
		return FALSE;

	// build full file name with path
	m_strGlobalSqlErrorFileName = penv;
#if defined (MAINWIN)
	m_strGlobalSqlErrorFileName += _T("/");
	m_strGlobalSqlErrorFileName += _T("ingres");
	m_strGlobalSqlErrorFileName += _T("/files/errvdba.log");
#else
	m_strGlobalSqlErrorFileName += _T("\\");
	m_strGlobalSqlErrorFileName += _T("ingres");
	m_strGlobalSqlErrorFileName += _T("\\files\\errvdba.log");
#endif

	return TRUE;
}

void CaFileError::InitializeFilesNames()
{
	if ( SetLocalFileName() && SetGlobalFileName())
		bStoreInFileActif = TRUE;
	else
	{
		//"Error creating temporary file for managing the SQL Error History.\n"
		//"This functionality will be disabled."
		AfxMessageBox(IDS_E_CREAT_TEMP_FILE);
	}
}

CaFileError::CaFileError()
{
	bStoreInFileActif = FALSE;
	m_strLocalSqlErrorFileName.Empty();
	m_strGlobalSqlErrorFileName.Empty();
}

CaFileError::~CaFileError()
{
	if (!bStoreInFileActif)
		return;
	DeleteFile(m_strLocalSqlErrorFileName);
}


BOOL WriteInFile ( LPCTSTR lpErrorText,LPCTSTR lpStatementText, LPCTSTR lpErrorNum,LPCTSTR lpFileName)
{
	HANDLE h;
	CString csErrStmt,csErrNum,csErrTxt,csEndLine,csTempo;
	csEndLine = _T("\r\n");
	h = CreateFile(  lpFileName,             // pointer to name of the file
	                 GENERIC_WRITE,          // access (read-write) mode
	                 0,                      // share mode
	                 NULL,                   //LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	                                         // pointer to security attributes
	                 OPEN_ALWAYS,            // how to create
	                 FILE_ATTRIBUTE_NORMAL,  // file attributes
	                 NULL                    // handle to file with attributes to 
	                                         // copy
	);
	if (h != INVALID_HANDLE_VALUE)
	{
		DWORD dwRes,dwNbBytes;
		dwRes = SetFilePointer(h,       // handle of file
		                       0,       // number of bytes to move file pointer
		                       NULL,    // pointer to high-order DWORD of 
		                                // distance to move
		                       FILE_END // how to move
		);
		if (dwRes != 0xFFFFFFFF )
		{
			// statement that produced the error
			csErrStmt.LoadString(IDS_F_SQLERR_STMT);
			csTempo = lpStatementText;
			WriteFile(h, csErrStmt, csErrStmt.GetLength(),&dwNbBytes,NULL);
			WriteFile(h, csEndLine, csEndLine.GetLength(),&dwNbBytes,NULL);
			WriteFile(h, csTempo, csTempo.GetLength(),&dwNbBytes,NULL);
			WriteFile(h, csEndLine, csEndLine.GetLength(),&dwNbBytes,NULL);
			// Sql error code
			csTempo.LoadString(IDS_FORMAT_SQLERR_ERRNUM);
			csErrNum.Format(csTempo,lpErrorNum);
			WriteFile(h, csErrNum, csErrNum.GetLength(),&dwNbBytes,NULL);
			WriteFile(h, csEndLine, csEndLine.GetLength(),&dwNbBytes,NULL);
			// Sql error text line after line and time after time
			csErrTxt.LoadString(IDS_F_SQLERR_ERRTXT);
			WriteFile(h, csErrTxt, csErrTxt.GetLength(),&dwNbBytes,NULL);
			WriteFile(h, csEndLine, csEndLine.GetLength(),&dwNbBytes,NULL);

			TCHAR *p, *p2;
			p = (LPTSTR)(LPCTSTR)lpErrorText;
			while (p && *p != _T('\0')) {
				p2 = _tcschr(p, _T('\n'));
				if (p2) {
					WriteFile(h,p, p2-p,&dwNbBytes,NULL);
					WriteFile(h, csEndLine, csEndLine.GetLength(),&dwNbBytes,NULL);
					p = ++p2;
				}
				else {
					WriteFile(h, p, _tcslen(p),&dwNbBytes,NULL);
					WriteFile(h, csEndLine, csEndLine.GetLength(),&dwNbBytes,NULL);
					break;
				}
			}

			// separator for next statement
			TCHAR tcSeparate[70];
			memset(tcSeparate, '-', 70);
			tcSeparate[70] = _T('\0');
			WriteFile(h, tcSeparate, _tcslen(tcSeparate),&dwNbBytes,NULL);
			WriteFile(h, csEndLine, csEndLine.GetLength(),&dwNbBytes,NULL);
		}

		CloseHandle(h);
		return TRUE;
	}
	return FALSE;
}


void CaFileError::WriteSqlErrorInFiles(LPCTSTR lpErrorText,LPCTSTR lpStatementText, LPCTSTR lpErrorNum)
{
	if (!bStoreInFileActif)
		return;
	WriteInFile(lpErrorText,lpStatementText,lpErrorNum,m_strLocalSqlErrorFileName);
	WriteInFile(lpErrorText,lpStatementText,lpErrorNum,m_strGlobalSqlErrorFileName);

};

#define BUFSIZE 256
#define SQLERRORTXTLEN    (1024+1)
/*
**
**  Method : ReadSqlErrorFromFile
**
**  Summary:  Read from file the sql errors and filled the CTypedPtrList.
**  Args:     CTypedPtrList to be filled
**
**  Returns:  int
**      0 = everything OK or Empty File.
**      1 = m_strLocalSqlErrorFileName not initialize.
**      2 = Get Size failed
**      3 = Open File failed
**      4 = read file failed
**      5 = bad file format (key word not found)
*/
int CaFileError::ReadSqlErrorFromFile(CTypedPtrList<CPtrList, CaSqlErrorInfo*>* lstSqlError)
{
	TCHAR *lpCurrentError = NULL;
	TCHAR *tcFileContain  = NULL;
	int nCount,result;
	TCHAR *lpErr, *lpTxt, *lpNum, *lpSep,*lpTemp;
	int iNbChar,iRetlineLength,iSeperatelineLength,iErrorStatementLength,iErrorTextLength;

	if (m_strLocalSqlErrorFileName.IsEmpty())
		return 1;

	// Get size file.
	struct _stat StatusInfo;
	result = _tstat( m_strLocalSqlErrorFileName, &StatusInfo );
	if( result != 0 )
		return 2;
	if (!StatusInfo.st_size)
		return 0; // this file is empty
	int hFile = _topen( m_strLocalSqlErrorFileName, _O_RDONLY | _O_BINARY);
	if( hFile == -1 )
		return 3;


	tcFileContain = new TCHAR [StatusInfo.st_size+1];
	nCount =_read(hFile, tcFileContain,StatusInfo.st_size);
	_close(hFile);
	if (nCount<=0)
	{
		delete (tcFileContain);
		return 4;
	}
	tcFileContain[nCount] = _T('\0');

	CaSqlErrorInfo* pItem;

	TCHAR bufErrorStatement[BUFSIZE];
	TCHAR bufErrorNum[BUFSIZE];
	TCHAR bufErrorNum2[BUFSIZE];
	TCHAR bufErrorText[BUFSIZE];
	TCHAR bufSeparate[BUFSIZE];
	TCHAR bufRetLine[] = _T("\r\n");
	TCHAR tcSQLQueryTxt[SQLERRORTXTLEN];
	TCHAR tcSQLErrorTxt[SQLERRORTXTLEN];
	TCHAR tcErrNum[20];

	iRetlineLength = _tcslen(bufRetLine);
	// statement that produced the error
	CString csTempo;
	csTempo.LoadString(IDS_F_SQLERR_STMT);
	_tcscpy(bufErrorStatement, csTempo);
	iErrorStatementLength = _tcslen(bufErrorStatement);
	// Sql error code
	csTempo.LoadString(IDS_F_SQLERR_ERRNUM);
	_tcscpy(bufErrorNum, csTempo);
	csTempo.LoadString(IDS_FORMAT_SQLERR_ERRNUM);
	_tcscpy(bufErrorNum2, csTempo);
	// Sql error text line after line and time after time
	csTempo.LoadString(IDS_F_SQLERR_ERRTXT);
	_tcscpy(bufErrorText, csTempo);
	iErrorTextLength = _tcslen(bufErrorText);
	// separator for next statement
	memset(bufSeparate, '-', 70);
	bufSeparate[70] = '\0';
	iSeperatelineLength = _tcslen(bufSeparate);

	lpCurrentError = tcFileContain;

	while (*lpCurrentError != _T('\0'))
	{
		lpErr = _tcsstr(lpCurrentError,bufErrorStatement);
		lpNum = _tcsstr(lpCurrentError,bufErrorNum);
		lpTxt = _tcsstr(lpCurrentError,bufErrorText);
		lpSep = _tcsstr(lpCurrentError,bufSeparate);
		if (!lpErr || lpErr!= lpCurrentError || !lpNum || !lpTxt || !lpSep) // structure error in file
		{
			delete (tcFileContain);
			return 5;
		}
		lpTemp = lpCurrentError + iErrorStatementLength + iRetlineLength;
		iNbChar = lpNum - lpTemp - iRetlineLength+1;
		lstrcpyn(tcSQLQueryTxt,lpTemp,iNbChar);

		_stscanf(lpNum, bufErrorNum2, tcErrNum);

		lpTemp = lpTxt + iErrorTextLength + iRetlineLength;
		iNbChar = lpSep - lpTemp - iRetlineLength+1;
		lstrcpyn(tcSQLErrorTxt,lpTemp,iNbChar);
		pItem = new CaSqlErrorInfo (tcSQLErrorTxt,tcSQLQueryTxt,tcErrNum);
		lstSqlError->AddTail(pItem);
		lpCurrentError = lpSep + iSeperatelineLength + iRetlineLength;
	}

	delete (tcFileContain);
	return 0;
}
