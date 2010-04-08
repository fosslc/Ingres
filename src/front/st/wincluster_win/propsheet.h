/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
**  Name: PropSheet.h : header file
**
**  History:
**
**	29-apr-2004 (wonst02)
**	    Created; cloned from similar enterprise_preinst directory.
*/

#if !defined(AFX_PROPSHEET_H__0E578486_9B8A_4863_91B1_582307DE0B59__INCLUDED_)
#define AFX_PROPSHEET_H__0E578486_9B8A_4863_91B1_582307DE0B59__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// WcPropSheet

class WcPropSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(WcPropSheet)

// Construction
public:
	WcPropSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
	// Some error has occurred and the dialog must be terminated
	DWORD m_GlobalError;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
public:
	virtual BOOL OnInitDialog();
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

// Implementation
public:
	virtual ~WcPropSheet();

	// Generated message map functions
protected:
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnClose();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	UINT MyMessageBox(UINT uiStrID,UINT uiFlags,LPCSTR param=0);
	// Display a system error message in a message box
	void SysError(UINT stringId, LPCSTR insertChr, DWORD lastError);
};

#endif // !defined(AFX_PROPSHEET_H__0E578486_9B8A_4863_91B1_582307DE0B59__INCLUDED_)
