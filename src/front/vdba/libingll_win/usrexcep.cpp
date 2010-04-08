/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : usrexcep.cpp, Implementation File
**    Project  : Virtual Node Manager
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : User defined exception
**
** History:
**
** 06-Jan-2000 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 16-Mar-2009 (drivi01)
**    Replace CArchiveException::generic with CArchiveException::genericException,
**    and CFileException::generic with CFileException::genericException
**    to clean up warnings in efforts to port to 2008 compiler.
**/

#include "stdafx.h"
#include "usrexcep.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CeException::CeException()
{
	m_lErrorCode = 0;
}

CeException::CeException(LONG lErrorCode)
{
	m_lErrorCode = lErrorCode;
}


CeSqlException::CeSqlException()
{
	m_tchszReason[0] = _T('\0');
}

CeSqlException::CeSqlException(LPCTSTR lpszReason, LONG lErrorCode):CeException(lErrorCode)
{
	SetReason(lpszReason);
}

void CeSqlException::SetReason(LPCTSTR lpszReason)
{
	if (!lpszReason)
	{
		m_tchszReason[0] = _T('\0');
		return;
	}
	if (lstrlen (lpszReason) < MAX_SQL_ERROR_TEXT)
		lstrcpy (m_tchszReason, lpszReason);
	else
		lstrcpyn(m_tchszReason, lpszReason, MAX_SQL_ERROR_TEXT);
}

static CString ArchiveGetMessage(CArchiveException* e)
{
	CString strMsg = _T("");
	switch (e->m_cause)
	{
	case CArchiveException::none:
		break;
	case CArchiveException::genericException:
		//"Serialization error.\nCause: Unknown."
		strMsg.Format (_T("Serialization error.%sCause: Unknown."), consttchszReturn);
		break;
	case CArchiveException::readOnly:
		//"Serialization error.\nCause: Tried to write into an archive opened for loading."
		strMsg.Format (_T("Serialization error.%sCause: Tried to write into an archive opened for loading."), consttchszReturn);
		break;
	case CArchiveException::endOfFile:
		//"Serialization error.\nCause: Reached end of file while reading an object."
		//lstrcpy (tchszMsg, _T("Serialization error.\nCause: Reached end of file while reading an object."));
		strMsg.Format (_T("Serialization error.%sCause: Reached end of file while reading an object."), consttchszReturn);
		break;
	case CArchiveException::writeOnly:
		//"Serialization error.\nCause: Tried to read from an archive opened for storing."
		strMsg.Format (_T("Serialization error.%sCause: Tried to read from an archive opened for storing."), consttchszReturn);
		break;
	case CArchiveException::badIndex:
		//"Serialization error.\nCause: Invalid file format."
		strMsg.Format (_T("Serialization error.%sCause: Invalid file format."), consttchszReturn);
		break;
	case CArchiveException::badClass:
		//"Serialization error.\nCause: Tried to read an object into an object of the wrong type."
		strMsg.Format (_T("Serialization error.%sCause: Tried to read an object into an object of the wrong type."), consttchszReturn);
		break;
	case CArchiveException::badSchema:
		//"Serialization error.\nCause: Tried to read an object with a different version of the class."
		strMsg.Format (_T("Serialization error.%sCause: Tried to read an object with a different version of the class."), consttchszReturn);
		break;
	default:
		strMsg.Format (_T("Serialization error.%sCause: Unknown."), consttchszReturn);
		break;
	}
	return strMsg;
}

static CString FileGetMessage(CFileException* e)
{
	CString strMsg = _T("");
	switch (e->m_cause)
	{
	case CFileException::genericException:
		//"Serialization error.\nCause: Unknown."
		strMsg.Format (_T("File error.%sCause: Unknown."), consttchszReturn);
		break;
	case CFileException::fileNotFound:
		//The file could not be located.
		strMsg.Format (_T("File error.%sCause: The file could not be located."), consttchszReturn);
		break;
	case CFileException::badPath:
		//All or part of the path is invalid.
		strMsg.Format (_T("File error.%sCause: All or part of the path is invalid."), consttchszReturn);
		break;
	case CFileException::tooManyOpenFiles:
		//The permitted number of open files was exceeded.
		strMsg.Format (_T("File error.%sCause: The permitted number of open files was exceeded."), consttchszReturn);
		break;
	case CFileException::accessDenied:
		//The file could not be accessed.
		strMsg.Format (_T("File error.%sCause: The file could not be accessed."), consttchszReturn);
		break;
	case CFileException::invalidFile:
		//There was an attempt to use an invalid file handle.
		strMsg.Format (_T("File error.%sCause: There was an attempt to use an invalid file handle."), consttchszReturn);
		break;
	case CFileException::removeCurrentDir:
		//The current working directory cannot be removed.
		strMsg.Format (_T("File error.%sCause: The current working directory cannot be removed."), consttchszReturn);
		break;
	case CFileException::directoryFull:
		//There are no more directory entries.
		strMsg.Format (_T("File error.%sCause: There are no more directory entries."), consttchszReturn);
		break;
	case CFileException::badSeek:
		//There was an error trying to set the file pointer.
		strMsg.Format (_T("File error.%sCause: There was an error trying to set the file pointer."), consttchszReturn);
		break;
	case CFileException::hardIO:
		//There was a hardware error.
		strMsg.Format (_T("File error.%sCause: There was a hardware error."), consttchszReturn);
		break;
	case CFileException::sharingViolation:
		//SHARE.EXE was not loaded, or a shared region was locked.
		strMsg.Format (_T("File error.%sCause: SHARE.EXE was not loaded, or a shared region was locked."), consttchszReturn);
		break;
	case CFileException::lockViolation:
		//There was an attempt to lock a region that was already locked.
		strMsg.Format (_T("File error.%sCause: There was an attempt to lock a region that was already locked."), consttchszReturn);
		break;
	case CFileException::diskFull:
		//The disk is full.
		strMsg.Format (_T("File error.%sCause: The disk is full."), consttchszReturn);
		break;
	case CFileException::endOfFile:
		//The end of file was reached. 
		strMsg.Format (_T("File error.%sCause: The end of file was reached."), consttchszReturn);
		break;
	default:
		strMsg.Format (_T("File error.%sCause:  Unknown."), consttchszReturn);
		break;
	}
	return strMsg;
}

void MSGBOX_ArchiveExceptionMessage (CArchiveException* e)
{
	CString strMsg = ArchiveGetMessage(e);
	if (!strMsg.IsEmpty())
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
}

void MSGBOX_FileExceptionMessage (CFileException* e)
{
	CString strMsg = FileGetMessage(e);
	if (!strMsg.IsEmpty())
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
}

void MSGBOX_ArchiveExceptionMessage (CArchiveException* e, CString& strMsg)
{
	strMsg = ArchiveGetMessage(e);
}

void MSGBOX_FileExceptionMessage (CFileException* e, CString& strMsg)
{
	strMsg = FileGetMessage(e);
}

