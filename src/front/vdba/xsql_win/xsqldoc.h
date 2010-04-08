/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xsqldoc.h : header file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : interface of the CdSqlQuery class
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 17-Oct-2003 (uk$so01)
**    SIR #106648, Additional change to change #464605 (role/password)
** 01-Sept-2004 (schph01)
**    SIR #106648 add variable member and method in CdSqlQuery class  for
**    managing the 'History of SQL Statements Errors' dialog.
*/

#if !defined(AFX_XSQLDOC_H__1D04F61A_EFC9_11D5_8795_00C04F1F754A__INCLUDED_)
#define AFX_XSQLDOC_H__1D04F61A_EFC9_11D5_8795_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "fileerr.h"

class CuSqlQueryControl;
class CdSqlQuery : public CDocument
{
protected: // create from serialization only
	CdSqlQuery();
	DECLARE_DYNCREATE(CdSqlQuery)


	// Operations
public:
	CString GetNode(){return m_strNode;}
	CString GetServer(){return m_strServer;}
	CString GetUser(){return m_strUser;}
	CString GetDatabase(){return m_strDatabase;}
	CString GetDefaultConnectedUser(){return _T("");}
	UINT    GetDbFlag(){return m_nDbFlag;}
	void SetConnectInfo(LPCTSTR lpszConnectString){m_strConnectString = lpszConnectString;}

	void SetNode(LPCTSTR lpszItem);
	void SetServer(LPCTSTR lpszItem);
	void SetUser(LPCTSTR lpszItem);
	void SetDatabase(LPCTSTR lpszItem, UINT nFlag);
	void Connected(){UpdateAllViews(NULL, 1);}
	void SetDefaultConnectedUser(){m_strUser = GetDefaultConnectedUser();}
	BOOL IsConnected(){return m_bConnected;}
	CString GetConnectInfo(){return m_strConnectString;}

	CuSqlQueryControl* GetSqlQueryCtrl();
	void SetWorkingFile(LPCTSTR lpszFileName){m_strWorkingFile = lpszFileName;}
	CString GetWorkingFile(){return m_strWorkingFile;}
	CaFileError* GetClassFilesErrors(){return &m_FilesErrors;}

#if defined (_OPTION_GROUPxROLE)
	void RolePassword(BOOL bPassword){m_bRolePassword = bPassword;}
	void RolePassword(LPCTSTR lpszPsw){m_strRolePassword = lpszPsw;}
	CString GetRolePassword(){return m_strRolePassword;}
	CString GetGroup(){return m_strGroup;}
	CString GetRole(){return m_strRole;}
	void SetGroup(LPCTSTR lpszItem);
	void SetRole(LPCTSTR lpszItem);
	CString GetDefaultConnectedGroup(){return _T("");}
	CString GetDefaultConnectedRole(){return _T("");}
	void SetDefaultConnectedGroup(){m_strGroup = GetDefaultConnectedGroup();}
	void SetDefaultConnectedRole(){m_strRole = GetDefaultConnectedRole();}
	BOOL RolePassword(){ return m_bRolePassword;}
#endif

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdSqlQuery)
	public:
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CdSqlQuery();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CString m_strNode;
	CString m_strServer;
	CString m_strUser;
	CString m_strDatabase;
	BOOL    m_bConnected;
	UINT    m_nDbFlag;
	CString m_strConnectString;

	CString m_strWorkingFile; // Environment file (when open environment)
	CaFileError m_FilesErrors;
#if defined (_OPTION_GROUPxROLE)
	CString m_strGroup;
	CString m_strRole;
	CString m_strRolePassword;
	BOOL    m_bRolePassword;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CdSqlQuery)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XSQLDOC_H__1D04F61A_EFC9_11D5_8795_00C04F1F754A__INCLUDED_)
