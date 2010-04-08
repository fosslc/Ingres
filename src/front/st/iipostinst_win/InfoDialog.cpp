/*
** Copyright (c) 2006 Ingres Corporation
*/

/*
**
** Name: InfoDialog.cpp - Information Dialog, displays information
**						  in context.
**						  Triggered by "i" button in the dialgos.
**
** History:
**	16-Nov-2006 (drivi01)
**	    Created.
*/

#include "stdafx.h"
#include "iipostinst.h"
#include "InfoDialog.h"
#include ".\infodialog.h"


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
	DDX_Control(pDX, IDC_LAUNCH_PAD, m_launchpad);
}


BOOL
InfoDialog::OnInitDialog()
{
	BOOL ret;
	ret = CDialog::OnInitDialog();
	
	
	HBITMAP hbitmap = ::LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_BITMAP_LAUNCHPAD));
	m_launchpad.SetBitmap(hbitmap);
	this->Invalidate(FALSE);
	return ret;
}

BEGIN_MESSAGE_MAP(InfoDialog, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()


// InfoDialog message handlers

void InfoDialog::OnBnClickedButton1()
{
	this->EndDialog(TRUE);
}

