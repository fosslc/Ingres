/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : tcomment.h, Header File 
**    Project  : Ingres II/Vdba.
**    Author   : Schalk Philippe
**
**    Purpose  : Editable List control to Change Comment on column
**
** History:
** xx-Oct-2000 (schph01)
**    Created
** 28-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#if !defined(AFX_TCOMMENT_H__A1690293_A383_11D4_B5B5_00C04F1790C3__INCLUDED_)
#define AFX_TCOMMENT_H__A1690293_A383_11D4_B5B5_00C04F1790C3__INCLUDED_

#if !defined (BUFSIZE)
#define BUFSIZE 256
#endif

extern "C"
{
#include "dbaset.h"
#include "dbadlg1.h"
#include "domdata.h"
#include "dba.h"
#include "dbaginfo.h"
#include "msghandl.h"
}

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "editlsct.h"

class CuEditableListCtrlCommentInsertValue : public CuEditableListCtrl
{
public:
	CuEditableListCtrlCommentInsertValue();
	virtual ~CuEditableListCtrlCommentInsertValue();

	// Limit the edit lenght for a comment with MAX_LENGTH_COMMENT
	void SetLimitEditText();


protected:
	virtual LONG ManageEditDlgOK       (UINT wParam, LONG lParam) {return OnEditDlgOK(wParam, lParam);}
	//{{AFX_MSG(CuEditableListCtrlSqlWizardInsertValue)
	//}}AFX_MSG
	afx_msg LONG OnEditDlgOK (UINT wParam, LONG lParam);

	DECLARE_MESSAGE_MAP()

};

////////////////////////////////////////////////////////////////////////////
// CxDlgObjectComment dialog

class CxDlgObjectComment : public CDialog
{
// Construction
public:
	CxDlgObjectComment(CWnd* pParent = NULL);   // standard constructor

	//comment for the table
	void    SetCommentObject(LPTSTR lpstring) {m_csCommentObject = lpstring;}
	CString GetCommentObject()                {return m_csCommentObject;}

	BOOL InitializeObjectColumns();
	void ObjectColumn_Done (); // Free Allocated memory
	LPCOMMENTCOLUMN FindCurrentColumn(CString &);

	BOOL FillListColumnWithComment();   // SQL request to obtain the comment for this object
	                                    // and the comment for each columns
	void GenerateSyntaxComment4Object (CString &csText);

	int m_iCurObjType; // current Object OT_TABLE,OT_VIEW ...
	// node Handle for DOMGetFirstObject() function
	int m_nNodeHandle;
	CString m_csDBName;
	CString m_csObjectName;
	CString m_csObjectOwner;
	TCHAR   *m_tcObjComment; // existing comment on this object
	
	//List of comment for each columns
	LPOBJECTLIST m_ListColumn;

// Dialog Data
	//{{AFX_DATA(CxDlgObjectComment)
	enum { IDD = IDD_XOBJECT_COMMENT };
	CuEditableListCtrlCommentInsertValue	m_cListCtrl;
	CEdit	m_cEditTableComment;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgObjectComment)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
// Implementation
protected:
	CImageList m_ImageList;

	// Generated message map functions
	//{{AFX_MSG(CxDlgObjectComment)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDestroy();
	afx_msg void OnSetfocusTableComment();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	BOOL m_bEditUnselected;
	CString m_csCommentObject; // new general comment on this object (Table, View)

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TCOMMENT_H__A1690293_A383_11D4_B5B5_00C04F1790C3__INCLUDED_)
