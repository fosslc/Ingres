/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdiffpg.h : header file
**    Project  : INGRESII/VDDA (vsda.ocx)
**    Author   : UK Sotheavut
**    Purpose  : Use the modeless dialog that has
**               - staic control (Header of the difference)
**               - edit multiline(The text of the difference)
**               This dialog is a child of CvDifferenceDetail and used 
**               for the pop-ud detail.
**
** History:
**
** 17-May-2004 (uk$so01)
**    SIR #109220, ISSUE 13407277: additional change to put the header
**    in the pop-up for an item's detail of difference.

*/

#pragma once
#include "afxwin.h"


// CuDlgDifferenceDetailPage dialog

class CuDlgDifferenceDetailPage : public CDialog
{
	DECLARE_DYNAMIC(CuDlgDifferenceDetailPage)

public:
	CuDlgDifferenceDetailPage(CWnd* pParent = NULL);   // standard constructor
	virtual ~CuDlgDifferenceDetailPage();
	void OnCancel() {return;}
	void OnOK()     {return;}

// Dialog Data
	enum { IDD = IDD_DIFFERENCE_DETAIL_PAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_strEdit1;
	CString m_strStatic1;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	CStatic m_cStatic1;
	CEdit m_cEdit1;
protected:
	virtual void PostNcDestroy();
};
