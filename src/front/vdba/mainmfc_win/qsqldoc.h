/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qsqldoc.h : header file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : interface of the CdSqlQuery class
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 15-MAR-2002 (schph01)
**    bug 107331 add variable m_IngresVersion in CdSqlQuery class,
**    and two methods to manage this.( SetIngresVersion() and
**    GetIngresVersion())
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
*/

#if !defined(AFX_XSQLDOC_H__1D04F61A_EFC9_11D5_8795_00C04F1F754A__INCLUDED_)
#define AFX_XSQLDOC_H__1D04F61A_EFC9_11D5_8795_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CuSqlQueryControl;
class CfSqlQueryFrame;
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

	void SetNode(LPCTSTR lpszItem);
	void SetServer(LPCTSTR lpszItem);
	void SetUser(LPCTSTR lpszItem);
	void SetDatabase(LPCTSTR lpszItem, UINT nFlag);
	void Connected(){UpdateAllViews(NULL, 1);}
	void SetDefaultConnectedUser(){m_strUser = GetDefaultConnectedUser();}
	BOOL IsConnected(){return m_bConnected;}

	CfSqlQueryFrame* GetFrameWindow();
	CToolBar* GetToolBar();
	CuSqlQueryControl* GetSqlQueryCtrl();
	int GetNodeHandle(){return m_nNodeHandle;}
	void SetNodeHandle(int nhNode){m_nNodeHandle = nhNode;}
	void SetIngresVersion(int iDBVers){m_IngresVersion = iDBVers;}
	int  GetIngresVersion(){return m_IngresVersion;}
	IUnknown* GetSqlUnknown();
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdSqlQuery)
	public:
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

	// Implementation
	void SetSeqNum(int val){m_SeqNum = val;}
	void SerializeSqlControl(CArchive& ar);
	BOOL IsLoadedDoc(){return m_bLoaded;}

public:
	virtual ~CdSqlQuery();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	BOOL IsToolbarVisible(){return m_bToolbarVisible;}
	CDockState& GetTollbarState(){return m_toolbarState;}
	WINDOWPLACEMENT& GetWindowPlacement() {return  m_wplj;}
	COleStreamFile& GetSqlStreamFile(){return m_sqlStream;}

protected:
	CString m_strNode;
	CString m_strServer;
	CString m_strUser;
	CString m_strDatabase;
	UINT    m_nDbFlag;
	BOOL    m_bConnected;
	int     m_SeqNum;
	int     m_nNodeHandle;
	BOOL    m_bLoaded;
	int     m_IngresVersion;
	BOOL    m_bToolbarVisible; 
	CDockState      m_toolbarState;
	WINDOWPLACEMENT m_wplj;
	COleStreamFile m_sqlStream;

	// Generated message map functions
protected:
	//{{AFX_MSG(CdSqlQuery)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XSQLDOC_H__1D04F61A_EFC9_11D5_8795_00C04F1F754A__INCLUDED_)
