/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdpseq.h : header file
**    Project  : Ingres VisualDBA
**
**
** 02-Apr-2003 (schph01)
**    SIR #107523 created.
**/
#if !defined(AFX_DGDPSEQ_H__0565D063_6135_11D7_B702_00C04F1790C3__INCLUDED_)
#define AFX_DGDPSEQ_H__0565D063_6135_11D7_B702_00C04F1790C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "domseri.h"
/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropSeq dialog

class CuDlgDomPropSeq : public CDialog
{
// Construction
public:
	CuDlgDomPropSeq(CWnd* pParent = NULL);   // standard constructor
    void OnCancel() {return;}
    void OnOK()     {return;}

// Dialog Data
	//{{AFX_DATA(CuDlgDomPropSeq)
	enum { IDD = IDD_DOMPROP_SEQ };
	CString	m_csEdCache;
	CString	m_csEdIncrement;
	CString	m_csEdMaxVal;
	CString	m_EdMinVal;
	CString	m_csEdPrecision;
	CString	m_csEdStartWith;
	BOOL	m_bCache;
	BOOL	m_bCycle;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropSeq)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropSeq)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

	void ResetDisplay();

	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
private :
	void RefreshDisplay();

	CuDomPropDataSequence m_Data;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGDPSEQ_H__0565D063_6135_11D7_B702_00C04F1790C3__INCLUDED_)
