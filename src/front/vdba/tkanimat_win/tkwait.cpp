/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : tkwait.cpp: implementation of the CxDlgWait class.
**    Project  : Extension DLL (Task Animation).
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Dialog to show the progression of Task (Interruptible)
**
** History:
** 07-Feb-1999 (uk$so01)
**    created
** 16-Mar-2001 (uk$so01)
**    Changed to be an Extension DLL
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
*/


#include "stdafx.h"
#include "tkwait.h"
#include "xdlgwait.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//
// CLASS CaExecParam:
// ************************************************************************************************
void CaExecParam::Init(UINT nInteruptType)
{
	m_nInterruptType = nInteruptType;
	m_bExecuteImmediately = TRUE;
	m_nDelayExecution = 0;
	m_bInterrupted = FALSE;
}


CaExecParam::CaExecParam()
{
	Init(INTERRUPT_NOT_ALLOWED);
}

CaExecParam::~CaExecParam()
{
}


CaExecParam::CaExecParam(UINT nInteruptType)
{
	Init(nInteruptType);
}

CaExecParam::CaExecParam(const CaExecParam& c)
{
	Copy(c);
}

void CaExecParam::Copy(const CaExecParam& c)
{
	m_bExecuteImmediately = c.m_bExecuteImmediately;
	m_bInterrupted = c.m_bInterrupted;
	m_nInterruptType = c.m_nInterruptType;
}




//
// CLASS CxDlgWait:
// ************************************************************************************************
CxDlgWait::CxDlgWait(LPCTSTR lpszCaption, CWnd* pParent)
{
	m_pDlg = (void*)(CDialog*)new CxDlgWaitTask (lpszCaption, pParent);
}

CxDlgWait::~CxDlgWait()
{
	if (m_pDlg)
		delete (CxDlgWaitTask*)m_pDlg;
	m_pDlg = NULL;
}


int  CxDlgWait::DoModal(){return ((CxDlgWaitTask*)m_pDlg)->DoModal();}
void CxDlgWait::SetUseExtraInfo(){((CxDlgWaitTask*)m_pDlg)->SetUseExtraInfo();}
void CxDlgWait::CenterDialog(CxDlgWait::CenterType nType){((CxDlgWaitTask*)m_pDlg)->CenterDialog(nType);}
void CxDlgWait::SetExecParam (CaExecParam* pExecParam){((CxDlgWaitTask*)m_pDlg)->SetExecParam(pExecParam);}
CaExecParam* CxDlgWait::GetExecParam (){return ((CxDlgWaitTask*)m_pDlg)->GetExecParam();}
void CxDlgWait::SetDeleteParam(BOOL bDelete){((CxDlgWaitTask*)m_pDlg)->SetDeleteParam(bDelete);}
void CxDlgWait::SetUseAnimateAvi (int nAviType)
{
	int nR = -1;
	switch (nAviType)
	{
	case AVI_CLOCK:
		nR = IDR_ANCLOCK;
		break;
	case AVI_CONNECT:
		nR = IDR_ANCONNECT;
		break;
	case AVI_DELETE:
		nR = IDR_ANDELETE;
		break;
	case AVI_DISCONNECT:
		nR = IDR_ANDISCONNECT;
		break;
	case AVI_DRAGDROP:
		nR = IDR_ANDRAGDC;
		break;
	case AVI_EXAMINE:
		nR = IDR_ANEXAMINE;
		break;
	case AVI_FETCHF:
		nR = IDR_ANFETCHF;
		break;
	case AVI_FETCHR:
		nR = IDR_ANFETCHR;
		break;
	case AVI_REFRESH:
		nR = IDR_ANREFRESH;
		break;
	default:
		nR = IDR_ANCLOCK;
		break;
	}
	((CxDlgWaitTask*)m_pDlg)->SetUseAnimateAvi(nR);
}

void CxDlgWait::SetShowProgressBar(BOOL bShow){((CxDlgWaitTask*)m_pDlg)->SetShowProgressBar(bShow);}
void CxDlgWait::SetHideDialogWhenTerminate(BOOL bHide){((CxDlgWaitTask*)m_pDlg)->SetHideDialogWhenTerminate(bHide);}
void CxDlgWait::SetHideCancelBottonAlways(BOOL bHide){((CxDlgWaitTask*)m_pDlg)->SetHideCancelBottonAlways(bHide);}

