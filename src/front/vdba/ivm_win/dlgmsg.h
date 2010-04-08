/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dlgmsg.h , Header File
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : dialog modeless for message
**
** History:
**
** 16-Sep-1999 (uk$so01)
**    created
** 17-Jun-2002 (uk$so01)
**    BUG #107930, fix bug of displaying the message box when the number of
**    events reached the limit of setting.
*/

#if !defined(AFX_DLGMSG_H__FFC4DB82_69B1_11D3_A309_00609725DDAF__INCLUDED_)
#define AFX_DLGMSG_H__FFC4DB82_69B1_11D3_A309_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CaMessageBoxParam
{
public:
	CaMessageBoxParam(LPCTSTR lpszMessage, BOOL bShowCheckbox = TRUE, BOOL bChecked = TRUE)
	{
		m_strMessage = lpszMessage;
		m_bShowCheckbox = bShowCheckbox;
		m_bChecked = bChecked;
	}
	virtual ~CaMessageBoxParam(){}
	virtual void EndMessage(){}

	CString GetMessage(){return m_strMessage;}
	BOOL IsShowCheckbox(){return m_bShowCheckbox;}
	BOOL IsChecked(){return m_bChecked;}
	void SetChecked(BOOL bSet){m_bChecked = bSet;}

protected:
	CString m_strMessage;
	BOOL m_bShowCheckbox;
	BOOL m_bChecked;
};

class CuDlgShowMessage : public CDialog
{
public:
	CuDlgShowMessage(CWnd* pParent = NULL);
	void SetParam(CaMessageBoxParam* pParam);
	BOOL m_bShowCheckBox;

	//{{AFX_DATA(CuDlgShowMessage)
	enum { IDD = IDD_MESSAGE };
	CButton	m_cCheck1;
	CStatic	m_cStatic1;
	BOOL	m_bShowLater;
	CString	m_strStatic2;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgShowMessage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CaMessageBoxParam* m_pParam;

	// Generated message map functions
	//{{AFX_MSG(CuDlgShowMessage)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGMSG_H__FFC4DB82_69B1_11D3_A309_00609725DDAF__INCLUDED_)
