/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : staautco.h , Header File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01) 
**    Purpose  : Status Page for Component that starts automatically by others
**
** History:
**
** 04-Mar-1999 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
**
**/

#if !defined(AFX_STAAUTCO_H__28F5FDF9_D144_11D2_A2D6_00609725DDAF__INCLUDED_)
#define AFX_STAAUTCO_H__28F5FDF9_D144_11D2_A2D6_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
class CuDlgStatusAutoStartComponent : public CDialog
{
public:
	CuDlgStatusAutoStartComponent(CWnd* pParent = NULL);
	void OnOK(){return;}
	void OnCancel(){return;}


	// Dialog Data
	//{{AFX_DATA(CuDlgStatusAutoStartComponent)
	enum { IDD = IDD_STATUS_AUTOSTART_COMPONENT };
	BOOL	m_bUnreadCriticalEvent;
	BOOL	m_bStarted;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgStatusAutoStartComponent)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgStatusAutoStartComponent)
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnComponentChanged (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFindAccepted (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFind (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STAAUTCO_H__28F5FDF9_D144_11D2_A2D6_00609725DDAF__INCLUDED_)
