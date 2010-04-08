/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : taskanim.cpp, implementation File 
** Project  : Ingres II/ (Vdba monitor as ActiveX control)
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Data manipulation: access the lowlevel data through 
**            Library or COM module with animation
** History:
**
** 07-Dec-2001 (uk$so01)
**    Created.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "ipmdml.h"
#include "ipmdoc.h"
#include "taskanim.h"


CaExecParamQueryData::CaExecParamQueryData(CaRunObject* pRunObject)
	:CaExecParam(INTERRUPT_NOT_ALLOWED), m_pRunObject(pRunObject)
{
	SetExecuteImmediately (FALSE);
	SetDelayExecution (2000);
	m_bInterrupted = FALSE;
	m_bSucceeded = TRUE;
}

int CaExecParamQueryData::Run(HWND hWndTaskDlg)
{
	BOOL bOK = FALSE;
	try
	{
#if defined (_ANIMATION_)
		BOOL bEnableFirst = TRUE;
		CaSwitchSessionThread sw (bEnableFirst);
#endif
		bOK = m_pRunObject->Run();
		if (!bOK)
			m_bSucceeded = FALSE;
#if defined (_ANIMATION_)
		sw.Out();
#endif
		return bOK? 0: 1;
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		TRACE0("Exception in: CaExecParamQueryData::Run(HWND hWndTaskDlg)\n");
	}
	m_bSucceeded = FALSE;
	return 1;
}

void CaExecParamQueryData::OnTerminate(HWND hWndTaskDlg)
{

}

BOOL CaExecParamQueryData::OnCancelled(HWND hWndTaskDlg)
{


	return FALSE;
}



//
// Animation Query Objects
// ************************************************************************************************
class CaRunQueryObject: public CaRunObject
{
public:
	CaRunQueryObject(CaIpmQueryInfo* pQueryInfo, CPtrList* pListInfoStruct):CaRunObject(NULL) 
	{
		m_pQueryInfo = pQueryInfo;
		m_pListInfoStruct = pListInfoStruct;
	}
	~CaRunQueryObject(){}

	virtual BOOL Run();

protected:
	CaIpmQueryInfo* m_pQueryInfo;
	CPtrList* m_pListInfoStruct;
};

BOOL CaRunQueryObject::Run()
{
	BOOL bOK = FALSE;
	CPtrList& listObject = *m_pListInfoStruct;
	bOK = IPM_llQueryInfo (m_pQueryInfo, listObject);
	return bOK;
}

BOOL IPM_QueryInfo (CaIpmQueryInfo* pQueryInfo, CPtrList& listInfoStruct)
{
	BOOL bOK = TRUE;
	CaRunQueryObject* pRunObject = new CaRunQueryObject(pQueryInfo, &listInfoStruct);
	CaExecParamQueryData* pExecParam = new CaExecParamQueryData (pRunObject);
#if defined (_ANIMATION_)
	BOOL bEnableFirst = FALSE;
	CaSwitchSessionThread sw (bEnableFirst);

	CString strMsgAnimateTitle;
	strMsgAnimateTitle.LoadString (IDS_ANIMATE_TITLE_QUERYOBJECT);
	CxDlgWait dlg (strMsgAnimateTitle);
	dlg.SetUseAnimateAvi(AVI_FETCHF);
	dlg.SetExecParam (pExecParam);
	dlg.SetDeleteParam(FALSE);
	dlg.SetShowProgressBar(FALSE);
	dlg.SetHideCancelBottonAlways(TRUE);
	dlg.DoModal();

	sw.Out();
#else
	pExecParam->Run();
#endif

	if (!pExecParam->IsSucceeded())
	{
		while (!listInfoStruct.IsEmpty())
			delete listInfoStruct.RemoveHead();

		bOK = FALSE;
		//
		// Or throw exception:
		if (pQueryInfo->IsReportError())
		{
			if (pQueryInfo->GetException().GetErrorCode() ==  RES_NOGRANT)
			{
				// Failed to guery objects because you have no grants.
				AfxMessageBox (IDS_FAIL_2_QUERY_OBJECTxNOGRANT);
			}
			else
			{
				// Failed to guery objects.
				AfxMessageBox (IDS_FAIL_2_QUERY_OBJECT);
			}
		}
	}

	delete pExecParam;
	delete pRunObject;
	return bOK;
}




//
// Animation Query Detail Objects
// ************************************************************************************************
class CaRunQueryObjectDetail: public CaRunObject
{
public:
	CaRunQueryObjectDetail(CaIpmQueryInfo* pQueryInfo, LPVOID lpInfoStruct):CaRunObject(NULL) 
	{
		m_pQueryInfo = pQueryInfo;
		m_pInfoStruct = lpInfoStruct;
	}
	~CaRunQueryObjectDetail(){}

	virtual BOOL Run();

protected:
	CaIpmQueryInfo* m_pQueryInfo;
	LPVOID m_pInfoStruct;
};

BOOL CaRunQueryObjectDetail::Run()
{
	BOOL bOK = FALSE;
	bOK = IPM_llQueryDetailInfo (m_pQueryInfo, m_pInfoStruct);
	return bOK;
}

BOOL IPM_QueryDetailInfo  (CaIpmQueryInfo* pQueryInfo, LPVOID lpInfoStruct)
{
	BOOL bOK = TRUE;
	CaRunQueryObjectDetail* pRunObject = new CaRunQueryObjectDetail (pQueryInfo, lpInfoStruct);
	CaExecParamQueryData*  pExecParam = new CaExecParamQueryData(pRunObject);

#if defined (_ANIMATION_)
	BOOL bEnableFirst = FALSE;
	CaSwitchSessionThread sw (bEnableFirst);

	CString strMsgAnimateTitle;
	strMsgAnimateTitle.LoadString (IDS_ANIMATE_TITLE_QUERYOBJECT_DETAIL);
	CxDlgWait dlg (strMsgAnimateTitle);
	dlg.SetUseAnimateAvi(AVI_FETCHF);
	dlg.SetExecParam (pExecParam);
	dlg.SetDeleteParam(FALSE);
	dlg.SetShowProgressBar(FALSE);
	dlg.SetHideCancelBottonAlways(TRUE);
	dlg.DoModal();

	sw.Out();
#else
	pExecParam->Run();
#endif

	if (!pExecParam->IsSucceeded())
	{
		bOK = FALSE;
		//
		// Or throw exception:
		if (pQueryInfo->IsReportError())
			AfxMessageBox (_T("Query Detail Object Failed"));
	}

	delete pExecParam;
	delete pRunObject;

	return bOK;
}
