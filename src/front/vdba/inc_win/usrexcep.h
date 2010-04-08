/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : usrexcep.h, Header File
**    Project  : Virtual Node Manager
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : User defined exception
**
** History:
**
** 06-Jan-2000 (uk$so01)
**    Created
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
**/

#if !defined(USEREXCEPTION_HEADER)
#define USEREXCEPTION_HEADER
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "constdef.h"

#define MAX_SQL_ERROR_TEXT 1024+1

class CeException
{
public:
	CeException();
	CeException(LONG lErrorCode);
	LONG GetErrorCode(){return m_lErrorCode;}
	void SetErrorCode(LONG lErrorCode){m_lErrorCode=lErrorCode;}
protected:
	LONG  m_lErrorCode;

};

class CeSqlException: public CeException
{
public:
	CeSqlException();
	CeSqlException(LPCTSTR lpszReason, LONG lErrorCode=0);
	~CeSqlException(){};

	void SetReason(LPCTSTR lpszReason);
	LPCTSTR GetReason(){return m_tchszReason;}

protected:
	TCHAR m_tchszReason [MAX_SQL_ERROR_TEXT];

};

class CeNodeException: public CeSqlException
{
public:
	CeNodeException(): CeSqlException() {}
	CeNodeException(LPCTSTR lpszReason): CeSqlException(lpszReason){}
	~CeNodeException(){};

};


void MSGBOX_ArchiveExceptionMessage (CArchiveException* e);
void MSGBOX_FileExceptionMessage (CFileException* e);
void MSGBOX_ArchiveExceptionMessage (CArchiveException* e, CString& strMsg);
void MSGBOX_FileExceptionMessage (CFileException* e, CString& strMsg);


#endif // !defined(USEREXCEPTION_HEADER)
