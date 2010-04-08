/****************************************************************************************
//                                                                                     //
//  Copyright (C) March, 1999 Computer Associates International, Inc.                  //
//                                                                                     //
//    Source   : dgdpihtm.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : INGRES II / VDBA.                                                     //
//    Author   : UK Sotheavut (uk$so01)                                                //
//                                                                                     //
//    Purpose  : The Right pane (for B-Unit page and facet)                            //
//               It contains the HTML source or and image .GIF                         //
****************************************************************************************/
#if !defined(AFX_DGDPIHTM_H__C568EB73_D6C4_11D2_A2D8_00609725DDAF__INCLUDED_)
#define AFX_DGDPIHTM_H__C568EB73_D6C4_11D2_A2D8_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "webbrows.h"

//#define DOMPROP_HTML_SIMULATE
#if !defined (DOMPROP_HTML_SIMULATE)
#include "domseri3.h"
#endif

class CuDlgDomPropIceHtml : public CDialog
{
public:
	//
	// nMode = 0: -> HTML source (IDD_DOMPROP_ICE_HTML_SOURCE)
	// nMode = 1: -> Image Gif   (IDD_DOMPROP_ICE_HTML_IMAGE)
	CuDlgDomPropIceHtml(int nMode = 0, CWnd* pParent = NULL);
	void OnCancel() {return;}
	void OnOK()     {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgDomPropIceHtml)
	enum { IDD = IDD_DOMPROP_ICE_HTML_SOURCE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	UINT m_nIDD;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropIceHtml)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	int m_nMode;
	#if !defined (DOMPROP_HTML_SIMULATE)
	CuDomPropDataIceFacetAndPage m_Data;
	#endif
	void RefreshDisplay();
	void ResetDisplay();

	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropIceHtml)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGDPIHTM_H__C568EB73_D6C4_11D2_A2D8_00609725DDAF__INCLUDED_)
