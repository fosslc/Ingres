/******************************************************************************
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/
/*
**
**
**    Source   : ckplst.h, header file
**
**    Project  : Ingres Journal Analyser
**    Author   : Schalk Philippe
**
**    Purpose  : Manage dialog box for display list of checkpoints.
**
**
** History:
**
** 02-Dec-2003 (schph01)
**    SIR #111389 add in project for displayed the checkpoint list.
**/
#if !defined(AFX_CKPLST_H__001085F3_D388_11D5_B657_00C04F1790C3__INCLUDED_)
#define AFX_CKPLST_H__001085F3_D388_11D5_B657_00C04F1790C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "calsctrl.h"
#include "resource.h"
#include "chkpoint.h"



/////////////////////////////////////////////////////////////////////////////
// CxDlgCheckPointLst dialog

class CxDlgCheckPointLst : public CDialog
{
// Construction
public:
	CxDlgCheckPointLst(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CxDlgCheckPointLst)
	enum { IDD = IDD_CHECKPOINT_LIST };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CString m_csCurDBName;
	CString m_csCurDBOwner;
	CString m_csCurVnodeName;

	CString GetSelectedCheckPoint(){return m_csCurrentSelectedCheckPoint;}
	void SetSelectedCheckPoint(CString& csValue){m_csCurrentSelectedCheckPoint = csValue;}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgCheckPointLst)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	// columns list control
	CuListCtrl                   m_cListCtrl;
	CImageList                   m_ImageList;
	CImageList                   m_ImageCheck;

	CaCheckPointList m_CaCheckPointList;
	CString m_csCurrentSelectedCheckPoint;
	void InitializeListCtrl();
	void fillListCtrl();

protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgCheckPointLst)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CKPLST_H__001085F3_D388_11D5_B657_00C04F1790C3__INCLUDED_)
