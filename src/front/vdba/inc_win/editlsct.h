/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : editlsct.h,  header file
**    Project  : 
**    Author   : UK Sotheavut 
**    Purpose  : Owner draw LISTCTRL to provide an editable fields (edit box, combobox 
**               and spin control
**
** History:
**
** xx-Jan-1996 (uk$so01)
**    Created
** 08-Nov-2000 (uk$so01)
**    Create library ctrltool.lib for more resuable of this control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vcbf uses libwctrl.lib).
** 23-Mar-2005 (komve01)
**    Issue# 13919436, Bug#113774. 
**    Changed the return type of HideProperty function to return false,
**    incase of any errors.
** 17-May-2005 (komve01)
**    Issue# 13864404, Bug#114371. 
**    Added a parameter for making the list control a simple list control.
**    The default behavior is the same as earlier, but for a special case
**    like VCDA (Main Parameters Tab), we do not want the default behavior
**    rather we would just like to have it as a simple list control.
**/

//
// CuEditableListCtrl window
#ifndef LYOUTCTL_HEADER
#define LYOUTCTL_HEADER
#include "calsctrl.h"

class CuLayoutEditDlg;
class CuLayoutComboDlg;
class CuLayoutSpinDlg;


class CuEditableListCtrl : public CuListCtrl
{
public:
	CuEditableListCtrl();
	BOOL Create( DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID );

	
	BOOL HideProperty  (BOOL bValidate = TRUE);
	void SetEditText   (int iIndex, int iSubItem, CRect rCell, LPCTSTR lpszText = NULL, BOOL bReadOnly = FALSE);
	void SetEditNumber (int iIndex, int iSubItem, CRect rCell, BOOL bReadOnly = FALSE);
	void SetEditSpin   (int iIndex, int iSubItem, CRect rCell, int Min = 0, int Max = UD_MAXVAL);
	void SetComboBox   (int iIndex, int iSubItem, CRect rCell, LPCTSTR lpszText = NULL);
	void SetComboBox   (int iIndex, int iSubItem, CRect rCell, int iSelect);

	void SetEditInteger     (int iIndex, int iSubItem, CRect rCell, BOOL bReadOnly = FALSE);
	void SetEditIntegerSpin (int iIndex, int iSubItem, CRect rCell, int Min = 0, int Max = UD_MAXVAL, BOOL bReadOnly = FALSE);

	CComboBox* GetComboBox ();
	CComboBox* COMBO_GetComboBox();
	CString COMBO_GetText ();
	void COMBO_GetText    (CString& strText);
	void COMBO_GetEditItem(int& iItem, int& iSubItem);
	void COMBO_SetEditableMode(BOOL bEditable);
	BOOL COMBO_IsEditableMode();

	CString EDIT_GetText();
	CEdit*  EDIT_GetEditCtrl();
	void EDIT_GetText (CString& strText);
	void EDIT_GetEditItem(int& iItem, int& iSubItem);
	void EDIT_SetExpressionDialog(BOOL bExpressionDld = TRUE);

	CString EDITNUMBER_GetText ();
	CEdit*  EDITNUMBER_GetEditCtrl();
	void EDITNUMBER_GetText (CString& strText);
	void EDITNUMBER_GetEditItem(int& iItem, int& iSubItem);
	void EDITNUMBER_SetSpecialParse(BOOL bSet);

	CString EDITSPIN_GetText ();
	CEdit*  EDITSPIN_GetEditCtrl();
	void EDITSPIN_GetText (CString& strText);
	void EDITSPIN_GetEditItem(int& iItem, int& iSubItem);
	void EDITSPIN_SetRange (int  Min, int  Max);
	void EDITSPIN_GetRange (int& Min, int& Max);

	CString EDITINTEGER_GetText ();
	CEdit*  EDITINTEGER_GetEditCtrl();
	void EDITINTEGER_GetText (CString& strText);
	void EDITINTEGER_GetEditItem(int& iItem, int& iSubItem);

	CString EDITINTEGERSPIN_GetText ();
	CEdit*  EDITINTEGERSPIN_GetEditCtrl();
	void EDITINTEGERSPIN_GetText (CString& strText);
	void EDITINTEGERSPIN_GetEditItem(int& iItem, int& iSubItem);
	void EDITINTEGERSPIN_SetRange (int  Min, int  Max);
	void EDITINTEGERSPIN_GetRange (int& Min, int& Max);

	// virtual interface to afx_msg OnXyzDlgOK methods
	virtual LONG ManageEditDlgOK (UINT wParam, LONG lParam)           { ASSERT (FALSE); return 0L; }
	virtual LONG ManageEditNumberDlgOK (UINT wParam, LONG lParam)     { ASSERT (FALSE); return 0L; }
	virtual LONG ManageEditSpinDlgOK (UINT wParam, LONG lParam)       { ASSERT (FALSE); return 0L; }
	virtual LONG ManageComboDlgOK (UINT wParam, LONG lParam)          { ASSERT (FALSE); return 0L; }
	virtual LONG ManageEditIntegerDlgOK (UINT wParam, LONG lParam)    { ASSERT (FALSE); return 0L; }
	virtual LONG ManageEditIntegerSpinDlgOK (UINT wParam, LONG lParam){ ASSERT (FALSE); return 0L; }

	virtual void SetSimpleList(BOOL bSimpleList);

	// Overrides
	//
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	virtual ~CuEditableListCtrl();
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrl)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	CuLayoutSpinDlg*  GetEditNumberDlg();
	CuLayoutSpinDlg*  GetEditSpinDlg();
	CuLayoutEditDlg*  GetEditDlg();
	CuLayoutComboDlg* GetComboDlg();

	CuLayoutSpinDlg*  GetEditIntegerDlg();
	CuLayoutSpinDlg*  GetEditIntegerSpinDlg();

	void CreateChildren();
	//
	// Will be opaque to users:
	CDialog*  m_EditNumberDlgPtr;
	CDialog*  m_EditSpinDlgPtr;
	CDialog*  m_EditDlgPtr;
	CDialog*  m_ComboDlgPtr;
	CDialog*  m_EditIntegerDlgPtr;
	CDialog*  m_EditIntegerSpinDlgPtr;


	// text when entering edition mode
	// (used for optimization in OnEditXyzDlgCancel() )
	CString m_ComboDlgOriginalText;
	CString m_EditDlgOriginalText;
	CString m_EditNumberDlgOriginalText;
	CString m_EditSpinDlgOriginalText;
	CString m_EditIntegerDlgOriginalText;
	CString m_EditIntegerSpinDlgOriginalText;

	BOOL m_bCallingCreate; // TRUE if user calls the Create member
	BOOL m_bSimpleList;
	//{{AFX_MSG(CuEditableListCtrl)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnNcRButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LONG OnEditDlgOK               (UINT wParam, LONG lParam);
	afx_msg LONG OnEditDlgCancel           (UINT wParam, LONG lParam);
	afx_msg LONG OnEditNumberDlgOK         (UINT wParam, LONG lParam);
	afx_msg LONG OnEditNumberDlgCancel     (UINT wParam, LONG lParam);
	afx_msg LONG OnEditSpinDlgOK           (UINT wParam, LONG lParam);
	afx_msg LONG OnEditSpinDlgCancel       (UINT wParam, LONG lParam);
	afx_msg LONG OnComboDlgOK              (UINT wParam, LONG lParam);
	afx_msg LONG OnComboDlgCancel          (UINT wParam, LONG lParam);
	//
	// Negative integer management
	afx_msg LONG OnEditIntegerDlgOK        (UINT wParam, LONG lParam);
	afx_msg LONG OnEditIntegerDlgCancel    (UINT wParam, LONG lParam);
	afx_msg LONG OnEditIntegerSpinDlgOK    (UINT wParam, LONG lParam);
	afx_msg LONG OnEditIntegerSpinDlgCancel(UINT wParam, LONG lParam);

	DECLARE_MESSAGE_MAP()
};
#endif


