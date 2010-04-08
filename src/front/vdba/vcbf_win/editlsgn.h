/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : editlsgn.h, Header File  
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut
**    Purpose  : Special Owner draw List Control for the Generic Page
**               See the CONCEPT.DOC for more detail
** History:
** xx-Sep-1997 (uk$so01)
**    Created.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#ifndef EDITLISTCONTROLGENERIC_HEADER
#define EDITLISTCONTROLGENERIC_HEADER
#include "editlsct.h"

class CuEditableListCtrlGeneric : public CuEditableListCtrl
{
public:
	CuEditableListCtrlGeneric();
	void EditValue (int iItem, int iSubItem, CRect rcCell);
	// Overrides
	//
	virtual ~CuEditableListCtrlGeneric();

	virtual LONG ManageEditDlgOK (UINT wParam, LONG lParam)        {return OnEditDlgOK(wParam, lParam);}      
	virtual LONG ManageEditNumberDlgOK (UINT wParam, LONG lParam)  {return OnEditNumberDlgOK(wParam, lParam);}
	virtual LONG ManageEditSpinDlgOK (UINT wParam, LONG lParam)    {return OnEditSpinDlgOK(wParam, lParam);}  
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	void SetGenericForCache (BOOL bCache = TRUE){m_bCache = bCache;};
	BOOL GetGenericForCache (){return m_bCache;};
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlGeneric)
	protected:
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	BOOL m_bCache; // When this control is used for CACHE, the low-level call is slightly
                   // different. If this member is TRUE, then the control is used for CACHE.

	//{{AFX_MSG(CuEditableListCtrlGeneric)
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
	DECLARE_MESSAGE_MAP()
};

#endif

