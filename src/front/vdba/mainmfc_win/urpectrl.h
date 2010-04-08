/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : urpectrl.h , Header File                                              //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / VDBA                                                      //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Dialog box for Create/Alter User (List control of privileges)         //
****************************************************************************************/
/* History:
* --------
* uk$so01: (26-Oct-1999 created)
*          Rewrite the dialog code, in C++/MFC.
*
*/
#if !defined(AFX_URPECTRL_H__CCC5A423_8B7B_11D3_A316_00609725DDAF__INCLUDED_)
#define AFX_URPECTRL_H__CCC5A423_8B7B_11D3_A316_00609725DDAF__INCLUDED_
#include "editlsct.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CaURPPrivilegesItemData: public CObject
{
public:
	CaURPPrivilegesItemData(int nPriv, int nNoPriv, BOOL bValue)
		: m_nUse(0), m_nPrivilege(nPriv), m_nNoPrivilege(nNoPriv), m_bValue(bValue), m_bGranted(FALSE) {}
	virtual ~CaURPPrivilegesItemData(){}

	CaURPPrivilegesItemData(LPCTSTR lpszPrivilege, BOOL bUser, BOOL bDefault)
		:m_strPrivilege(lpszPrivilege), m_bUser(bUser), m_bDefault(bDefault){}


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

	CString m_strPrivilege;
	BOOL    m_bUser;
	BOOL    m_bDefault;


	int  m_nUse;        // 0: Ignore. 1: Privilege. -1: NoPrivilege
	int  m_nPrivilege;
	int  m_nNoPrivilege;
	CString m_strLimit; // Limit value.
	BOOL m_bValue;      // TRUE if Privilege needs value.('m_strLimit') 
	BOOL m_bGranted;
};


class CuEditableListCtrlURPPrivileges : public CuListCtrl
{
public:
	CuEditableListCtrlURPPrivileges();
	void DisplayPrivileges(int nItem = -1);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlURPPrivileges)
	//}}AFX_VIRTUAL

	// Implementation
public:
	BOOL m_bReadOnly;
	virtual ~CuEditableListCtrlURPPrivileges();

	// Generated message map functions
protected:
	void DisplayItem(int nItem);

	//{{AFX_MSG(CuEditableListCtrlURPPrivileges)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_URPECTRL_H__CCC5A423_8B7B_11D3_A316_00609725DDAF__INCLUDED_)
