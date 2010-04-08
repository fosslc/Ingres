/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : sqlerr.cpp: implementation
**    Project  : Meta data library 
**    Author   : Schalk Philippe(schph01)
**    Purpose  : Store SQL error in temporary file name.
**
** History:
**
** 16-Jul-2004 (schph01)
**    Created
**/

#include "stdafx.h"
#include "mgterr.h"
#include "ingobdml.h"

static TCHAR tcErrorFileName[_MAX_PATH]= _T("");
static TCHAR tcGlobalErrorFileName[_MAX_PATH]= _T("");

BOOL INGRESII_llInitGlobalErrorLogFile()
{
	TCHAR* penv = _tgetenv(_T("II_SYSTEM"));
	if (!penv || *penv == _T('\0'))
		return FALSE;

	// build full file name with path
	lstrcpy(tcGlobalErrorFileName, penv);
#if defined (MAINWIN)
	lstrcat(tcGlobalErrorFileName, _T("/"));
	lstrcat(tcGlobalErrorFileName, _T("ingres"));
	lstrcat(tcGlobalErrorFileName, _T("/files/errvdba.log"));
#else
	lstrcat(tcGlobalErrorFileName, _T("\\"));
	lstrcat(tcGlobalErrorFileName, _T("ingres"));
	lstrcat(tcGlobalErrorFileName, _T("\\files\\errvdba.log"));
#endif

	return TRUE;
}

void INGRESII_llSetErrorLogFile(LPTSTR pFileName)
{
	if ( pFileName && *pFileName != _T('\0'))
		lstrcpy(tcErrorFileName,pFileName);
	else
		tcErrorFileName[0]=_T('\0');

	if ( tcGlobalErrorFileName && *tcGlobalErrorFileName == _T('\0'))
	{
		if (!INGRESII_llInitGlobalErrorLogFile())
			tcGlobalErrorFileName[0]=_T('\0');
	}
}
LPTSTR INGRESII_llGetErrorLogFile()
{
	return tcErrorFileName;
}



void INGRESII_llWriteInFile(LPTSTR FileName, LPTSTR SqlTextErr,LPTSTR SqlStatement,int iErr)
{
	char      buf[256];
	char      buf2[256];
	HFILE     hFile;
	OFSTRUCT  openBuff;
	char      *p, *p2;
	if (FileName[0])
	{
	// open for write with share denied,
	// preceeded by create if does not exist yet
	hFile = OpenFile(FileName, &openBuff, OF_EXIST);
	if (hFile != HFILE_ERROR)
		hFile = OpenFile(FileName, &openBuff, OF_SHARE_DENY_NONE | OF_WRITE);
	else {
		hFile = OpenFile(FileName, &openBuff, OF_CREATE);
		if (hFile != HFILE_ERROR) {
		_lclose(hFile);
		hFile = OpenFile(FileName, &openBuff, OF_SHARE_DENY_NONE | OF_WRITE);
		}
	}

	if (hFile != HFILE_ERROR) {
		if (_llseek(hFile, 0, 2) != HFILE_ERROR) {
		// statement that produced the error
		lstrcpy(buf, "Erroneous statement:");
		_lwrite(hFile, buf, lstrlen(buf));
		_lwrite(hFile, "\r\n", 2);
		_lwrite(hFile, SqlStatement,
			lstrlen(SqlStatement));
		_lwrite(hFile, "\r\n", 2);

		// Sql error code
		lstrcpy(buf, "Sql error code: %ld");
		wsprintf(buf2, buf, iErr);
		_lwrite(hFile, buf2, lstrlen(buf2));
		_lwrite(hFile, "\r\n", 2);

		// Sql error text line after line and time after time
		lstrcpy(buf, "Sql error text:");
		_lwrite(hFile, buf, lstrlen(buf));
		_lwrite(hFile, "\r\n", 2);
		p = SqlTextErr;
		while (p) {
			p2 = strchr(p, '\n');
			if (p2) {
			_lwrite(hFile, p, p2-p);
			_lwrite(hFile, "\r\n", 2);
			p = ++p2;
			}
			else {
			_lwrite(hFile, p, lstrlen(p));
			_lwrite(hFile, "\r\n", 2);
			break;
			}
		}

		// separator for next statement
		memset(buf, '-', 70);
		buf[70] = '\0';
		_lwrite(hFile, buf, lstrlen(buf));
		_lwrite(hFile, "\r\n", 2);

		}
		_lclose(hFile);
	}
	}
}

void INGRESII_ManageErrorInLogFiles( LPTSTR SqlTextErr,LPTSTR SqlStatement,int iErr)
{
	INGRESII_llWriteInFile( tcErrorFileName,SqlTextErr,SqlStatement,iErr);
	INGRESII_llWriteInFile( tcGlobalErrorFileName,SqlTextErr,SqlStatement,iErr);
}
