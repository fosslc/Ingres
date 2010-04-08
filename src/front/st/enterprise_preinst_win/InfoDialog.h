#pragma once
/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: InfoDialog.h
**
**  Description:
**	Header file for InfoDialog dialog.
**
**  History:
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
*/



class InfoDialog : public CDialog
{
	DECLARE_DYNAMIC(InfoDialog)

public:
	InfoDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~InfoDialog();

// Dialog Data
	enum { IDD = IDD_DIALOG_INFO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
};
