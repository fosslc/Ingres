/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : msgnosol.h : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modal dialog box that acts like MessageBox 
**
** History:
**
** 14-Mar-2001 (uk$so01)
**    Created
**/


#if !defined(AFX_MSGNOSOL_H__DCBEEC05_1702_11D5_8742_00C04F1F754A__INCLUDED_)
#define AFX_MSGNOSOL_H__DCBEEC05_1702_11D5_8742_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "formdata.h"

class CxDlgMessageNoSolution : public CDialog
{
public:
	CxDlgMessageNoSolution(CWnd* pParent = NULL);
	void SetData(CaIIAInfo* pData){m_pData = pData;}

	// Dialog Data
	//{{AFX_DATA(CxDlgMessageNoSolution)
	enum { IDD = IDD_XNOSOLUTION_INFO };
	CStatic	m_cStaticImage1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgMessageNoSolution)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	HICON m_hIcon;
	CaIIAInfo* m_pData;

	// Generated message map functions
	//{{AFX_MSG(CxDlgMessageNoSolution)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonSeeRejectedCombinaition();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSGNOSOL_H__DCBEEC05_1702_11D5_8742_00C04F1F754A__INCLUDED_)
