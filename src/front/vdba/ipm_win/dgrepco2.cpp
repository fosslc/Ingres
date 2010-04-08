/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgrepco2.cpp, Implementation file
**    Project  : INGRES II/ Replication Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless Dialog to display the special page for Transaction Collision
**
** History:
**
** xx-Sep-1998 (uk$so01)
**    Created
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgrepco2.h"
#include "vrepcolf.h"
#include "frepcoll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CuDlgReplicationPageCollisionRight2::CuDlgReplicationPageCollisionRight2(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationPageCollisionRight2::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationPageCollisionRight2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgReplicationPageCollisionRight2::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationPageCollisionRight2)
	DDX_Control(pDX, IDC_BUTTON1, m_cButton1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgReplicationPageCollisionRight2, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationPageCollisionRight2)
	ON_BN_CLICKED(IDC_BUTTON1, OnApplyResolution)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationPageCollisionRight2 message handlers

void CuDlgReplicationPageCollisionRight2::OnApplyResolution() 
{
	CfReplicationPageCollision* pFrame = (CfReplicationPageCollision*)GetParentFrame();
	ASSERT (pFrame);
	if (pFrame)
	{
		CvReplicationPageCollisionViewLeft* pViewL = (CvReplicationPageCollisionViewLeft*)pFrame->GetLeftPane();
		if (!pViewL->CheckResolveCurrentTransaction())
		{
			//_T("Not all sequences are marked as \nSource prevails or Target prevails.");
			AfxMessageBox (IDS_E_ALL_SEQUENCES);
			return;
		}
		pViewL->ResolveCurrentTransaction();
	}
}

void CuDlgReplicationPageCollisionRight2::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here
	
}

void CuDlgReplicationPageCollisionRight2::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgReplicationPageCollisionRight2::DisplayData (CaCollisionTransaction* pData)
{
	if (!pData)
	{
		GetDlgItem (IDC_BUTTON1)->ShowWindow (SW_HIDE);
		return;
	}
	else
		GetDlgItem (IDC_BUTTON1)->ShowWindow (SW_SHOW);

	if (pData->IsResolved())
		GetDlgItem (IDC_BUTTON1)->EnableWindow (FALSE);
	else
		GetDlgItem (IDC_BUTTON1)->EnableWindow (TRUE);
}
