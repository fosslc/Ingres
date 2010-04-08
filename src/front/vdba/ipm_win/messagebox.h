/*
**  Copyright (C) 2008 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : messagebox.h, header file
**    Project  : CA-OpenIngres/Replicator.
**    Author   : Viktoriya Driker
**    Purpose  : Message dialog to request information or notify of
**				 special action. One of the buttons requires elevation 
**				 icon painted.
**
** History:
**
** 20-Oct-2008 (drivi01)
**		Created.
*/


#pragma once


// CMessageBox dialog

class CMessageBox : public CDialog
{
	DECLARE_DYNAMIC(CMessageBox)

public:
	CMessageBox(CWnd* pParent = NULL, CString msg = "");   // standard constructor
	virtual ~CMessageBox();
	BOOL OnInitDialog();
	HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);
	CString m_msg;
	CBitmap m_shield;
	CBitmapButton m_yes;


// Dialog Data
	enum { IDD = IDD_MESSAGEBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CBrush m_brush;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedYes();
	afx_msg void OnBnClickedNo();

};
