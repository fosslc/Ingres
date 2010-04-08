#pragma once
/*
** Copyright (c) 2006 Ingres Corporation
*/

/*
**
** Name: InfoDialog.h - header to InforDialog.cpp
**
** History:
**	16-Nov-2006 (drivi01)
**	    Created.
*/


class InfoDialog : public CDialog
{
	DECLARE_DYNAMIC(InfoDialog)

public:
	InfoDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~InfoDialog();

// Dialog Data
	enum { IDD = IDD_DIALOG_INFO };
	C256bmp m_image;
	CStatic m_launchpad;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
};
