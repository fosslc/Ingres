/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: InfoDialog.cpp : implementation file
**
**  Description:
**	This module is designed to display text information for the user.
**
**  History:
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
*/


#include "stdafx.h"
#include "enterprise.h"
#include "InfoDialog.h"


// InfoDialog dialog

IMPLEMENT_DYNAMIC(InfoDialog, CDialog)
InfoDialog::InfoDialog(CWnd* pParent /*=NULL*/)
	: CDialog(InfoDialog::IDD, pParent)
{
}

InfoDialog::~InfoDialog()
{
}

void InfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL InfoDialog::OnInitDialog()
{	
	
	this->SetWindowText("Instance Name");
	return CDialog::OnInitDialog();

}

BEGIN_MESSAGE_MAP(InfoDialog, CDialog)
END_MESSAGE_MAP()


// InfoDialog message handlers
