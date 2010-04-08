#if !defined(AFX_ICEDCONN_H__516E09E4_5CEF_11D2_9727_00609725DBF9__INCLUDED_)
#define AFX_ICEDCONN_H__516E09E4_5CEF_11D2_9727_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// IceDConn.h : header file
//
extern "C"
{
#include "main.h"
#include "compat.h"
#ifndef CL_PROTOTYPED
#define CL_PROTOTYPED
#endif
#include "gc.h"
#include "monitor.h"
#include "domdata.h"
#include "msghandl.h"
#include "getdata.h"
#include "ice.h"
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceDbConnection dialog

class CxDlgIceDbConnection : public CDialog
{
// Construction
public:
	int m_nHnode;
	CString m_csStaticInfo;
	CString m_csDbName;
	CString m_csVnodeName;
	CString m_csCurrentVnodeName;
	CxDlgIceDbConnection(CWnd* pParent = NULL);   // standard constructor
	LPDBCONNECTIONDATA m_pStructDbConnect;

// Dialog Data
	//{{AFX_DATA(CxDlgIceDbConnection)
	enum { IDD = IDD_ICE_DATABASE_CONNECT };
	CButton	m_ctrlOK;
	CEdit	m_ctrledName;
	CEdit	m_ctrledComment;
	CComboBox	m_ctrlcbDbUser;
	CComboBox	m_ctrlcbNode;
	CComboBox	m_ctrlcbDatabase;
	CString	m_csDatabase;
	CString	m_csUser;
	CString	m_csComment;
	CString	m_csConnectName;
	CString	m_csNode;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceDbConnection)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void FillVnodeName ();
	void FillDatabasesOfVnode (BOOL bInitWithFirstItem);
	void FillDatabasesUsers ();
	void EnableDisableOK ();
	BOOL FillstructureFromCtrl( LPDBCONNECTIONDATA pDbConnNew );

	// Generated message map functions
	//{{AFX_MSG(CxDlgIceDbConnection)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChangeIceName();
	afx_msg void OnSelchangeNode();
	afx_msg void OnSelchangeIceDbUser();
	afx_msg void OnSelchangeDatabase();
	afx_msg void OnDestroy();
	afx_msg void OnKillfocusDatabase();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICEDCONN_H__516E09E4_5CEF_11D2_9727_00609725DBF9__INCLUDED_)
