/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : edlsdbgr.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/Vdba.                                                       //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Editable List control to provide to Change Database Privileges        //
****************************************************************************************/
#if !defined(EDLSDBGR_HEADER)
#define EDLSDBGR_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "editlsct.h"

class CaDatabasePrivilegesItemData: public CObject
{
public:
	CaDatabasePrivilegesItemData(int nPriv, int nNoPriv, BOOL bValue)
		: m_nUse(0), m_nPrivilege(nPriv), m_nNoPrivilege(nNoPriv), m_bValue(bValue), m_bGranted(FALSE) {}
	virtual ~CaDatabasePrivilegesItemData(){}

	int  GetIDPrivilege(){return m_nPrivilege;}
	int  GetIDNoPrivilege(){return m_nNoPrivilege;}
	int  GetPrivilege(){return m_nUse;}
	void SetPrivilege(BOOL bCheck = TRUE){m_nUse = 1; m_bGranted = bCheck;}
	void SetNoPrivilege(BOOL bCheck = TRUE){m_nUse = -1;m_bGranted = bCheck;}
	void SetIgnore(){m_nUse = 0;m_bGranted = FALSE;}
	BOOL HasValue(){return m_bValue;}
	BOOL GetGrantPrivilege(){return (m_nUse == 1)? m_bGranted: FALSE;}
	BOOL GetGrantNoPrivilege(){return (m_nUse == -1)? m_bGranted: FALSE;}

	void SetValue (LPCTSTR lpszValue){m_strLimit = lpszValue;}
	CString GetValue(){ASSERT(m_bValue); return m_strLimit;}
	int GetPrivilegeType(){return m_nUse;}

	int  m_nUse;        // 0: Ignore. 1: Privilege. -1: NoPrivilege
	int  m_nPrivilege;
	int  m_nNoPrivilege;
	CString m_strLimit; // Limit value.
	BOOL m_bValue;      // TRUE if Privilege needs value.('m_strLimit') 
	BOOL m_bGranted;
};


/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlGrantDatabase window

class CuEditableListCtrlGrantDatabase : public CuEditableListCtrl
{
public:
	enum {C_PRIV, C_GRANT, C_LIMIT, C_LIMIT_VALUE, C_NOPRIV, C_GRANTNOPRIV};
	CuEditableListCtrlGrantDatabase();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlGrantDatabase)
	//}}AFX_VIRTUAL

	void EditValue (int iItem, int iSubItem, CRect rcCell);
	virtual LONG ManageEditNumberDlgOK (UINT wParam, LONG lParam) { return OnEditNumberDlgOK(wParam, lParam);}
	void DisplayPrivileges(int nItem = -1);

	//
	// Implementation
public:
	virtual ~CuEditableListCtrlGrantDatabase();

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuEditableListCtrlGrantDatabase)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LONG OnEditNumberDlgOK (UINT wParam, LONG lParam);
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif 
