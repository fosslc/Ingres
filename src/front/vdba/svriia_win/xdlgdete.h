/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlgdete.h : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modal dialog box to show the process of detection
**
** History:
**
** 13-Mar-2001 (uk$so01)
**    Created
**/


#if !defined(AFX_XDLGDETE_H__DCBEEC04_1702_11D5_8742_00C04F1F754A__INCLUDED_)
#define AFX_XDLGDETE_H__DCBEEC04_1702_11D5_8742_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "calsctrl.h"
#include "formdata.h"

class CxDlgDetectionInfo : public CDialog
{
public:
	CxDlgDetectionInfo(CWnd* pParent = NULL);
	void SetData(CaIIAInfo* pData){m_pData = pData;}
	// Dialog Data
	//{{AFX_DATA(CxDlgDetectionInfo)
	enum { IDD = IDD_XDETECTION_INFO };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgDetectionInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuListCtrl m_cListCtrl;
	CaIIAInfo* m_pData;
	CImageList m_ImageCheck;

	// Generated message map functions
	//{{AFX_MSG(CxDlgDetectionInfo)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGDETE_H__DCBEEC04_1702_11D5_8742_00C04F1F754A__INCLUDED_)
