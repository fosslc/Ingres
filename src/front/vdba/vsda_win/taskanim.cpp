/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : taskanim.cpp, Implementation file 
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Dialog that run thread with animation, allow interupt task 
**               (Parameters and execute task)
**
** History:
**
** 26-Nov-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    created
** 18-Aug-2004 (uk$so01)
**    BUG #112841 / ISSUE #13623161, Allow user to cancel the Comparison
**    operation.
*/


#include "stdafx.h"
#include "vsda.h"
#include "taskanim.h"
#include "snaparam.h"
#include "loadsave.h"
#include "vsdtree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define _USE_SETLOCKMODE_TIMEOUT


//
// RUN THE STORE SCHEMA OR INSTALLATION ....
// ************************************************************************************************
CaExecParamInstallation::CaExecParamInstallation(CaVsdStoreSchema* pStoreSchema):CaExecParam(INTERRUPT_NOT_ALLOWED)
{
	m_pStoreSchema = pStoreSchema;
	m_strMsgBoxTitle.LoadString(AFX_IDS_APP_TITLE);
	m_strMsgFailed.LoadString (IDS_MSG_FAIL_2_STOREDATA);
	//m_strInterruptOperation.LoadString(IDS_INTERRUPT_OPERATION);

	m_strException = _T("");
	m_bOutOfMemory = FALSE;
	m_bInterrupted = FALSE;
}

CaExecParamInstallation::~CaExecParamInstallation()
{
	if (m_pStoreSchema)
		delete m_pStoreSchema;
	m_pStoreSchema = NULL;
}

int CaExecParamInstallation::Run(HWND hWndTaskDlg)
{
	ASSERT (m_pStoreSchema);
	if (!m_pStoreSchema)
		return 1;

	try
	{
		m_pStoreSchema->SetHwndAnimate(hWndTaskDlg);
		if (m_pStoreSchema->GetDatabase().IsEmpty())
			VSD_StoreInstallation (*m_pStoreSchema);
		else
			VSD_StoreSchema (*m_pStoreSchema);

		return 0;
	}
	catch(CMemoryException* e)
	{
		e->Delete();
		m_bOutOfMemory = TRUE;;
	}
	catch(CeSqlException e)
	{
		m_strException = e.GetReason();
	}
	catch (CFileException* e)
	{
		CString strErr;
		e->GetErrorMessage(strErr.GetBuffer(_MAX_PATH), _MAX_PATH);
		strErr.ReleaseBuffer();
		m_strException = strErr;
		e->Delete();
	}
	catch(...)
	{
		//
		// Failed to construct file format identification.
		m_strException = m_strMsgFailed;
	}
	return 1;
}


void CaExecParamInstallation::OnTerminate(HWND hWndTaskDlg)
{
}

BOOL CaExecParamInstallation::OnCancelled(HWND hWndTaskDlg)
{
	ASSERT (m_pStoreSchema);
	if (!m_pStoreSchema)
		return FALSE;
	/*
	theApp.m_synchronizeInterrupt.BlockUserInterface(TRUE);    // Block itself;
	theApp.m_synchronizeInterrupt.SetRequestCancel(TRUE);      // Request the cancel.
	theApp.m_synchronizeInterrupt.BlockWorkerThread(TRUE);     // Block worker thread (Run() method);
	theApp.m_synchronizeInterrupt.WaitUserInterface();         // Wait itself until worker thread allows to do something;

	int nRes = ::MessageBox(hWndTaskDlg, m_strInterruptDetection, m_strMsgBoxTitle, MB_ICONQUESTION | MB_OKCANCEL);
	theApp.m_synchronizeInterrupt.BlockWorkerThread(FALSE);    // Release worker thread (Run() method);
	theApp.m_synchronizeInterrupt.SetRequestCancel(FALSE);     // Un-Request the cancel.
	if (nRes == IDOK)
	{
		m_pIIAInfo->SetInterrupted(TRUE);
		m_bInterrupted = TRUE;
		return TRUE;
	}
	else
		return FALSE;
	*/
	return FALSE;
}



//
// RUN THE COMPARAISON OF SCHEMA OR INSTALLATION ....
// ************************************************************************************************
CaExecParamComparing::CaExecParamComparing(CaVsdRefreshTreeInfo* pPopulateParamp, CaVsdRoot* pTree, BOOL bInstallation)
    :CaExecParam(INTERRUPT_USERPRECIFY)
{
	m_pPopulateParamp = pPopulateParamp;
	m_pTreeData = pTree;
	m_bInstallation = bInstallation;

	m_strMsgBoxTitle.LoadString(AFX_IDS_APP_TITLE);
	m_strMsgFailed.LoadString (IDS_MSG_FAIL_2_COMPAREDATA);
//	m_strInterruptOperation.LoadString(IDS_INTERRUPT_COMPARING);
	m_bPopulated = FALSE;

	m_strException = _T("");
	m_bOutOfMemory = FALSE;
	m_bInterrupted = FALSE;
}

CaExecParamComparing::~CaExecParamComparing()
{
	m_pPopulateParamp = NULL;
}

int CaExecParamComparing::Run(HWND hWndTaskDlg)
{
	ASSERT (m_pPopulateParamp);
	if (!m_pPopulateParamp)
		return 1;
	try
	{
		m_pPopulateParamp->SetHwndAnimate(hWndTaskDlg);
		if (m_bInstallation)
		{
			m_bPopulated = m_pTreeData->Populate (m_pPopulateParamp);
		}
		else
		{
			if (m_pPopulateParamp->IsLoadSchema1())
			{
				CaVsdaLoadSchema* load = m_pPopulateParamp->GetLoadedSchema1();
				load->LoadSchema();
			}
			if (m_pPopulateParamp->IsLoadSchema2())
			{
				CaVsdaLoadSchema* load = m_pPopulateParamp->GetLoadedSchema2();
				load->LoadSchema();
			}
			m_pPopulateParamp->SetNode(m_pPopulateParamp->GetNode1(), m_pPopulateParamp->GetNode2());
			m_pPopulateParamp->SetDatabase(m_pPopulateParamp->GetDatabase1(), m_pPopulateParamp->GetDatabase2());
			CaVsdDatabase* pDatabase = m_pTreeData->SetSchemaDatabase(m_pPopulateParamp->GetDatabase1());
			if (pDatabase)
			{
				CaDatabase& database = pDatabase->GetObject();
				database.SetDBService(m_pPopulateParamp->GetFlag());
				pDatabase->Initialize();
				m_bPopulated = pDatabase->Populate(m_pPopulateParamp);
			}
		}

		return 0;
	}
	catch(CMemoryException* e)
	{
		e->Delete();
		m_bOutOfMemory = TRUE;;
	}
	catch(CeSqlException e)
	{
		m_strException = e.GetReason();
	}
	catch (CFileException* e)
	{
		CString strErr;
		e->GetErrorMessage(strErr.GetBuffer(_MAX_PATH), _MAX_PATH);
		strErr.ReleaseBuffer();
		m_strException = strErr;
		e->Delete();
	}
	catch(...)
	{
		//
		// Failed to construct file format identification.
		m_strException = m_strMsgFailed;
	}
	return 1;
}


void CaExecParamComparing::OnTerminate(HWND hWndTaskDlg)
{
}

BOOL CaExecParamComparing::OnCancelled(HWND hWndTaskDlg)
{
	ASSERT (m_pPopulateParamp);
	if (!m_pPopulateParamp)
		return FALSE;
	m_pPopulateParamp->Interrupt(TRUE);
	return TRUE;
	/*
	theApp.m_synchronizeInterrupt.BlockUserInterface(TRUE);    // Block itself;
	theApp.m_synchronizeInterrupt.SetRequestCancel(TRUE);      // Request the cancel.
	theApp.m_synchronizeInterrupt.BlockWorkerThread(TRUE);     // Block worker thread (Run() method);
	theApp.m_synchronizeInterrupt.WaitUserInterface();         // Wait itself until worker thread allows to do something;

	int nRes = ::MessageBox(hWndTaskDlg, m_strInterruptDetection, m_strMsgBoxTitle, MB_ICONQUESTION | MB_OKCANCEL);
	theApp.m_synchronizeInterrupt.BlockWorkerThread(FALSE);    // Release worker thread (Run() method);
	theApp.m_synchronizeInterrupt.SetRequestCancel(FALSE);     // Un-Request the cancel.
	if (nRes == IDOK)
	{
		m_pPopulateParamp->Interrup(TRUE);
		m_bInterrupted = TRUE;
		return TRUE;
	}
	else
		return FALSE;
	*/
	return FALSE;
}

