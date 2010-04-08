/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdpseqg.h : header file
**    Project  : Ingres VisualDBA
**
**
** 02-Apr-2003 (schph01)
**    SIR #107523 created.
**/
#if !defined(AFX_DGDPSEQG_H__436ACD55_6358_11D7_B704_00C04F1790C3__INCLUDED_)
#define AFX_DGDPSEQG_H__436ACD55_6358_11D7_B704_00C04F1790C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "domseri.h"
#include "domilist.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropSeqGrantees dialog

class CuDlgDomPropSeqGrantees : public CDialog
{
// Construction
public:
	CuDlgDomPropSeqGrantees(CWnd* pParent = NULL);   // standard constructor
	void OnCancel() {return;}
	void OnOK()     {return;}

// Dialog Data
	//{{AFX_DATA(CuDlgDomPropSeqGrantees)
	enum { IDD = IDD_DOMPROP_SEQ_GRANTEES };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CuListCtrlCheckmarks  m_cListCtrl;
	CImageList            m_ImageCheck;
	CuDomImageList        m_ImageList;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropSeqGrantees)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

private:
	void RefreshDisplay();
	void AddSeqGrantee (CuSeqGrantee* pSeqGrantees);

	CuDomPropDataSeqGrantees m_Data;

// Implementation
protected:
	void ResetDisplay();

	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropSeqGrantees)
	afx_msg void OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGDPSEQG_H__436ACD55_6358_11D7_B704_00C04F1790C3__INCLUDED_)
