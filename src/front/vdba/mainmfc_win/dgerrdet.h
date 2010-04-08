#if !defined(AFX_DGERRDET_H__41A96C62_B279_11D1_A35B_00609725DBFA__INCLUDED_)
#define AFX_DGERRDET_H__41A96C62_B279_11D1_A35B_00609725DBFA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgErrDet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuDlgErrorWitnDetailBtn dialog

class CuDlgErrorWitnDetailBtn : public CDialog
{
// Construction
public:
	CuDlgErrorWitnDetailBtn(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CuDlgErrorWitnDetailBtn)
	enum { IDD = IDD_ERROR_WITH_HISTORY };
	CButton	m_btnOk;
	CStatic	m_edErrTxt;
	CButton	m_btnDetail;
	CStatic	m_stIcon;
	CString	m_errText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgErrorWitnDetailBtn)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
    BOOL CalculateTxtRect(CRect *prcNewTxt);

protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgErrorWitnDetailBtn)
	virtual BOOL OnInitDialog();
	afx_msg void OnDetail();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGERRDET_H__41A96C62_B279_11D1_A35B_00609725DBFA__INCLUDED_)
