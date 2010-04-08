/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdpdbse.h
**    Project  : Ingres VisualDBA
**
**
** 02-Apr-2003 (schph01)
**    SIR #107523 created.
**/

#if !defined(AFX_DGDPDBSE_H__436ACD56_6358_11D7_B704_00C04F1790C3__INCLUDED_)
#define AFX_DGDPDBSE_H__436ACD56_6358_11D7_B704_00C04F1790C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// dgdpdbse.h : header file
//

#include "calsctrl.h"  // CuListCtrl
#include "domseri2.h"
#include "domilist.h"  // CuDomImageList

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropDbSeq dialog

class CuDlgDomPropDbSeq : public CDialog
{
// Construction
public:
	CuDlgDomPropDbSeq(CWnd* pParent = NULL);   // standard constructor
	void AddProc (CuNameWithOwner *pDbProc);
	void OnCancel() {return;}
	void OnOK()     {return;}


// Dialog Data
	//{{AFX_DATA(CuDlgDomPropDbSeq)
	enum { IDD = IDD_DOMPROP_DB_SEQS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CuListCtrl m_cListCtrl;
	CuDomImageList m_ImageList;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropDbSeq)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

  CuDomPropDataDbSeq m_Data;

// Implementation
protected:
	void ResetDisplay();

	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropDbSeq)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGDPDBSE_H__436ACD56_6358_11D7_B704_00C04F1790C3__INCLUDED_)
