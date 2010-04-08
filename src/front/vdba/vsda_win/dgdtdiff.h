/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdtdiff.h : header file
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Property page to show the detail of differences
**               for some kinds of objects.
**
** History:
**
** 10-Dec-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 07-May-2004 (schph01)
**    SIR #112281, add variable member m_pListCtrl.
**    The Detailed Differences properties windows does show differences for
**    any difference type.
*/

#if !defined(AFX_DGDTDIFF_H__6F33C6A1_0B84_11D7_87F9_00C04F1F754A__INCLUDED_)
#define AFX_DGDTDIFF_H__6F33C6A1_0B84_11D7_87F9_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "calsctrl.h"
class CuListCtrl;
class CfDifferenceDetail;
class CuDlgDifferenceDetail : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgDifferenceDetail)

public:
	CuDlgDifferenceDetail();
	~CuDlgDifferenceDetail();
	
	void SetListCtrl(CuListCtrl* p){m_pListCtrl = (CuListCtrl*)p;}
	(CuListCtrl*) GetCurrentListCtrl(){return m_pListCtrl;}

	// Dialog Data
	//{{AFX_DATA(CuDlgDifferenceDetail)
	enum { IDD = IDD_DIFFERENT_DETAIL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDifferenceDetail)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CfDifferenceDetail* m_pDetailFrame;
	CuListCtrl* m_pListCtrl;
	// Generated message map functions
	//{{AFX_MSG(CuDlgDifferenceDetail)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg long OnUpdateData(WPARAM w, LPARAM l);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGDTDIFF_H__6F33C6A1_0B84_11D7_87F9_00C04F1F754A__INCLUDED_)
