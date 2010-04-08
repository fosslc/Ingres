/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : dgnewmsg.h , Header File                                              //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Modeless Dialog to Inform user of new message arriving                //
****************************************************************************************/
/* History:
* --------
* uk$so01: (06-May-1999 created)
*
*
*/

#if !defined(AFX_DGNEWMSG_H__2443B362_02ED_11D3_A2EA_00609725DDAF__INCLUDED_)
#define AFX_DGNEWMSG_H__2443B362_02ED_11D3_A2EA_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CuDlgYouHaveNewMessage : public CDialog
{
public:
	CuDlgYouHaveNewMessage(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CuDlgYouHaveNewMessage)
	enum { IDD = IDD_YOUHAVEMESSAGE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgYouHaveNewMessage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgYouHaveNewMessage)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGNEWMSG_H__2443B362_02ED_11D3_A2EA_00609725DDAF__INCLUDED_)
