/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : editlnet.h, Header File 
**    Project  : OpenIngres Configuration Manager 
**    Author   : Blatte Emmanuel 
**    Purpose  : Special Owner draw List Control for the Net Protocol Page 
**               See the CONCEPT.DOC for more detail 
**
** History:
** xx-Sep-1997 (Blatte Emmanuel)
**    Created.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/


/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlNet window
#ifndef EDITLISTCONTROL_NET_HEADER
#define EDITLISTCONTROL_NET_HEADER
#include "editlsct.h"


class CuEditableListCtrlNet : public CuEditableListCtrl
{
public:
  int GetCurSel();
	CuEditableListCtrlNet();

	// Overrides
	//
	virtual ~CuEditableListCtrlNet();
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlNet)
	protected:
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CuEditableListCtrlNet)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LONG OnEditDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditDlgCancel (UINT wParam, LONG lParam);
	afx_msg LONG OnEditNumberDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditNumberDlgCancel (UINT wParam, LONG lParam);
	afx_msg LONG OnEditSpinDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditSpinDlgCancel (UINT wParam, LONG lParam);

  // special derivations
  virtual LONG ManageEditDlgOK (UINT wParam, LONG lParam);
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	DECLARE_MESSAGE_MAP()
};
#endif

