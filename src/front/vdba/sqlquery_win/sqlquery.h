/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlquery.h : main header file for SQLQUERY.DLL
**    Project  : sqlquery.ocx 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main application class
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
**/

#if !defined(AFX_SQLQUERY_H__634C3843_A069_11D5_8769_00C04F1F754A__INCLUDED_)
#define AFX_SQLQUERY_H__634C3843_A069_11D5_8769_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif
#include "resource.h" // main symbols
#include "sessimgr.h" // Session Manager (shared and global)

class CdSqlQueryRichEditDoc;
class CaDocTracker: public CObject
{
public:
	CaDocTracker(): m_pDoc(NULL), m_lControl(0){}
	CaDocTracker(CdSqlQueryRichEditDoc* pDoc, LPARAM pControl): m_pDoc(pDoc), m_lControl(pControl){};
	virtual ~CaDocTracker(){}

	LPARAM GetControl(){return m_lControl;}
	CdSqlQueryRichEditDoc* GetDoc(){return m_pDoc;}
protected:
	CdSqlQueryRichEditDoc* m_pDoc;
	LPARAM m_lControl;
};

class CSqlqueryApp : public COleControlModule
{
public:
	CSqlqueryApp();
	BOOL InitInstance();
	int ExitInstance();
	CTypedPtrList < CObList, CaDocTracker* >& GetDocTracker(){return m_listDoc;}
	CaSessionManager& GetSessionManager(){return m_sessionManager;}
	//
	// This function 'INGRESII_QueryObject' should call the query objects from the 
	// Static library ("INGRESII_llQueryObject") or from the COM Server ICAS.
	BOOL INGRESII_QueryObject(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);

	void OutOfMemoryMessage()
	{
		AfxMessageBox (m_tchszOutOfMemoryMessage, MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
	}
	CString& GetTraceBuffer(){return m_strTraceBuffer;}
	int GetCursorSequence()
	{
		m_nCursorSequence++;
		return m_nCursorSequence;
	}
	int GetSqlAssistantSessionStart(){return m_nSqlAssistantStart;}
	CString GetCharacterEncoding(){return m_strEncoding;}
	void SetCharacterEncoding(LPCTSTR lpszEncoding){m_strEncoding = lpszEncoding;}

	// Implementation
	CString m_strHelpFile;

protected:
	CaSessionManager m_sessionManager;
	TCHAR m_tchszOutOfMemoryMessage[128];
	CString m_strInstallationID;
	CString m_strTraceBuffer;
	int m_nCursorSequence;
	int m_nSqlAssistantStart;
	CString m_strEncoding;
	CTypedPtrList < CObList, CaDocTracker* > m_listDoc;

};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;
extern CSqlqueryApp theApp;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLQUERY_H__634C3843_A069_11D5_8769_00C04F1F754A__INCLUDED)
