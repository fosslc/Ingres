/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : xdgevset.h , Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Dialog box for defining the preferences of events
**
** History:
**
** 21-May-1999 (uk$so01)
**    Created
** 27-Jan-2000 (uk$so01)
**    (Bug #100157): Activate Help Topic
** 08-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
** 21-Nov-2003 (uk$so01)
**    SIR #99596, Remove the class CuStaticResizeMark and the bitmap resizema.bmp
**/

#if !defined(AFX_XDGEVSET_H__0A2EF7D2_129C_11D3_A2EE_00609725DDAF__INCLUDED_)
#define AFX_XDGEVSET_H__0A2EF7D2_129C_11D3_A2EE_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CxDlgEventSetting : public CDialog
{
public:
	CxDlgEventSetting(CWnd* pParent = NULL);
	UINT GetHelpID() {return IDD_XEVENT_SETTING;}

	// Dialog Data
	//{{AFX_DATA(CxDlgEventSetting)
	enum { IDD = IDD_XEVENT_SETTING };
	CStatic	m_cStaticProgress;
	CButton	m_cButtonCancel;
	CButton	m_cButtonOK;
	CStatic	m_cStatic1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgEventSetting)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CWnd* m_pFrame;
	HICON m_hIcon;

	void ResizeControls();
	// Generated message map functions
	//{{AFX_MSG(CxDlgEventSetting)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDGEVSET_H__0A2EF7D2_129C_11D3_A2EE_00609725DDAF__INCLUDED_)
