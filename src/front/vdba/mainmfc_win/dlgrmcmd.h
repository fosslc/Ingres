/*************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dlgrmcmd.h, Header File
**
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut
**
**    Purpose  : Popup Dialog Box for the RMCMD Execution
**
**  History after 01-01-2000:
**
**  04-Apr-2000 (noifr01)
**     (bug 101430) added boolean array member variable for the
**     "no carriage return" information attached to the returned 
**     lines
**  10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
************************************************************************/
#if !defined(AFX_DLGRMCMD_H__6DD3BF62_7B13_11D2_A2AF_00609725DDAF__INCLUDED_)
#define AFX_DLGRMCMD_H__6DD3BF62_7B13_11D2_A2AF_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#define FEEDBACK_MAXLINES 100
extern "C"
{
#include "dba.h"
}

class CxDlgRemoteCommand : public CDialog
{
public:
	CxDlgRemoteCommand(CWnd* pParent = NULL);
	void SetParams (LPVOID lpParam){m_pParam = lpParam;}
	// Dialog Data
	//{{AFX_DATA(CxDlgRemoteCommand)
	enum { IDD = IDD_XRMCMD };
	CStatic	m_cStatic1;
	CEdit	m_cEdit1;
	CButton	m_cButtonOK;
	CButton	m_cButtonSendAnswer;
	CAnimateCtrl	m_cAnimate1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgRemoteCommand)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CSize   m_sizeInitial;
	CSize   m_sizePadding;
	CSize   m_sizeButton[3];

	int     m_nCxEditMargin; // Right of Edit and the Button
	LPVOID  m_pParam;
	UINT    m_nTimer1;
	UINT    m_nEllapse;
	int     m_iTimerSubCount;
	int     m_iTimerCount;
	BOOL    m_bServerActive;
	LPUCHAR m_feedbackLines[FEEDBACK_MAXLINES];
	BOOL    m_bNoCR[FEEDBACK_MAXLINES];

	// Generated message map functions
	//{{AFX_MSG(CxDlgRemoteCommand)
	afx_msg void OnSendAnswer();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGRMCMD_H__6DD3BF62_7B13_11D2_A2AF_00609725DDAF__INCLUDED_)
