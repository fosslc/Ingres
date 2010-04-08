/****************************************************************************************
//                                                                                     //
v
//                                                                                     //
//    Source   : xdlgdrop.h, Header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/VDBA                                                        //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Dialog Box to allow to drop object with cascade | restrict            //
****************************************************************************************/
#if !defined(AFX_XDLGDROP_H__719D50D2_740B_11D2_A2AA_00609725DDAF__INCLUDED_)
#define AFX_XDLGDROP_H__719D50D2_740B_11D2_A2AA_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CxDlgDropObjectCascadeRestrict : public CDialog
{
public:
	CxDlgDropObjectCascadeRestrict(CWnd* pParent = NULL);
	void SetParam (LPVOID pParam){m_pParam = pParam;}
	// Dialog Data
	//{{AFX_DATA(CxDlgDropObjectCascadeRestrict)
	enum { IDD = IDD_XDROP_OBJECT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CString m_strTitle;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgDropObjectCascadeRestrict)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	LPVOID m_pParam;
	// Generated message map functions
	//{{AFX_MSG(CxDlgDropObjectCascadeRestrict)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGDROP_H__719D50D2_740B_11D2_A2AA_00609725DDAF__INCLUDED_)
