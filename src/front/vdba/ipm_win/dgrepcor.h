/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgrepcor.cpp, Implementation file
**    Project  : INGRES II/ Replication Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless Dialog to display the conflict rows
**
** History:
**
** xx-May-1998 (uk$so01)
**    Created
**/


#if !defined(AFX_DGREPCOR_H__09BFF511_EB0A_11D1_A27C_00609725DDAF__INCLUDED_)
#define AFX_DGREPCOR_H__09BFF511_EB0A_11D1_A27C_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "lsrgbitm.h"
#include "collidta.h"


class CuDlgReplicationPageCollisionRight : public CDialog
{
public:
	CuDlgReplicationPageCollisionRight(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}
	void DisplayData (CaCollisionSequence* pData);
	void SetListCtrlColumnHeader (int iColumn, LPCTSTR lpszText);

	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationPageCollisionRight)
	enum { IDD = IDD_REPCOLLISION_PAGE_RIGHT };
	CString	m_strCollisionType;
	CString	m_strCDDS;
	CString	m_strTable;
	CString	m_strTransaction;
	CString	m_strSourceTransTime;
	CString	m_strTargetTransTime;
	CString	m_strPrevTransID;
	CString	m_strSourceDbName;
	//}}AFX_DATA
	CuListCtrlColorItem m_cListCtrl;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationPageCollisionRight)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CImageList m_ImageList;

	CaCollisionItem* ViewLeftGetCurrentSelectedItem();
	void ViewLeftUpdateImageCurrentSelectedItem();
	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationPageCollisionRight)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnRadioUndefined();
	afx_msg void OnRadioSourcePrevail();
	afx_msg void OnRadioTargetPreval();
	afx_msg void OnApplyResolution();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGREPCOR_H__09BFF511_EB0A_11D1_A27C_00609725DDAF__INCLUDED_)
