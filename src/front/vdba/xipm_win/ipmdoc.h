/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ipmdoc.h: Header File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : interface of the CdIpm class (MFC frame/view/doc).
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
*/



#if !defined(AFX_IPMDOC_H__AE712EFA_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
#define AFX_IPMDOC_H__AE712EFA_E8C7_11D5_8792_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "ipmctrl.h"

class CdIpm : public CDocument
{
protected: // create from serialization only
	CdIpm();
	DECLARE_DYNCREATE(CdIpm)


	// Operations
public:
	CString GetNode(){return m_strNode;}
	CString GetServer(){return m_strServer;}
	CString GetUser(){return m_strUser;}
	CString GetDefaultConnectedUser(){return _T("");}

	void SetNode(LPCTSTR lpszItem);
	void SetServer(LPCTSTR lpszItem);
	void SetUser(LPCTSTR lpszItem);
	void Connected(){UpdateAllViews(NULL, 1);}
	void SetConnectState(BOOL bOK);
	void SetDefaultConnectedUser(){m_strUser = GetDefaultConnectedUser();}
	BOOL IsConnected(){return m_bConnected;}
	int  GetExpressRefreshFrequency();
	void SetExpressRefreshFrequency(int nFrequency);

	CuIpm* GetIpmCtrl();
	void SetWorkingFile(LPCTSTR lpszFileName){m_strWorkingFile = lpszFileName;}
	CString GetWorkingFile(){return m_strWorkingFile;}
	CaFileError* GetClassFilesErrors(){return &m_FilesErrors;}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdIpm)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CdIpm();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
protected:
	CString m_strNode;
	CString m_strServer;
	CString m_strUser;
	BOOL    m_bConnected;
	int     m_nExpressRefreshFrequency;
	HANDLE  m_hMutexAccessnExpressRefreshFrequency;

	CString m_strWorkingFile; // Environment file (when open environment)

	CaFileError m_FilesErrors;

	// Generated message map functions
protected:
	//{{AFX_MSG(CdIpm)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IPMDOC_H__AE712EFA_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
