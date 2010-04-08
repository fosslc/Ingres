/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : ivmsgdml.cpp , Implementation File
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data Manipulation of Message Setting of IVM
**
**  History:
**	21-June-1999 (uk$so01)
**	    Created
**	03-Feb-2000 (uk$so01)
**	    (BUG #100327)
**	    Move the data files of ivm to the directory II_SYSTEM\ingres\files.
**	31-May-2000 (uk$so01)
**	    bug 99242 Handle DBCS
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
**  02-Feb-2004 (noifr01)
**    removed any unneeded reference to iostream libraries, including now
**    unused internal test code 
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "ivmsgdml.h"
#include "evtcats.h"
#include "ivmdml.h"
#include "maindoc.h"
#include "wmusrmsg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// Initialize the Message Entry:
BOOL IVM_InitializeUnclassifyCategory (CaMessageManager& messageManager)
{
	//
	// Real mode:
	long lCat, lCode;
	CString strMsgText;

	BOOL bOk = GetFirstIngClass4Interface (lCat, strMsgText);
	while (bOk)
	{
		lCode = lCat; // It's the same
		CaMessage* pMessage = new CaMessage(strMsgText, lCode, lCat, IMSG_NOTIFY);
		messageManager.Add (lCat, pMessage);
		bOk = GetNextIngClass4Interface (lCat, strMsgText);
	}
	return TRUE;
}

//
// For each message entry, initialize its states (Alert, Notify, Discard).
// For each message entry, initialize the user's specialized messages.
BOOL IVM_SerializeUserSpecializedMessage (CaMessageManager& messageManager, LPCTSTR lpszFile, BOOL bLoad)
{
	try
	{
		CFile f;
		if (bLoad)
		{
			//
			// Load the setting of user category
			if(f.Open (lpszFile, CFile::modeRead))
			{
				CaMessageManager m;
				CArchive ar(&f, CArchive::load);

				m.Serialize (ar);
				//
				// Update the messageManager from 'm':
				messageManager.Update (m);
				return TRUE;
			}
			return FALSE;
		}
		else
		{
			//
			// Store the setting of user category
			if(f.Open (lpszFile, CFile::modeCreate|CFile::modeWrite))
			{
				CArchive ar(&f, CArchive::store);
				messageManager.Serialize (ar);
				return TRUE;
			}
			return FALSE;
		}
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (IVM_SerializeUserSpecializedMessage): \nCannot serialize the settings of messages."));
		return FALSE;
	}
	return TRUE;
}



//////////////////////////////******************

BOOL IVM_ReadUserCategory (CaMessageManager& messageManager)
{
	return TRUE;
}



BOOL IVM_LoadUserCategory (CTypedPtrList<CObList, CaCategoryDataUser*>& listUserFolder, LPCTSTR lpszFile)
{
	return TRUE;
}

BOOL IVM_SaveUserCategory (CTypedPtrList<CObList, CaCategoryDataUser*>& listUserFolder, LPCTSTR lpszFile)
{
	return TRUE;
}



CaMessage* IVM_LookupMessage (long lCategory, long lCode)
{
	CaMessageManager& m = theApp.m_setting.m_messageManager;
	CaMessage* pMsg = m.Search (lCategory, lCode);

	return pMsg;
}

CaMessage* IVM_LookupMessage (long lCategory, long lCode, CaMessageManager* pMsgManager)
{
	CaMessage* pMsg = pMsgManager->Search (lCategory, lCode);

	return pMsg;
}





BOOL IVM_QueryIngresMessage (CaMessageEntry* pEntry, CTypedPtrList<CObList, CaLoggedEvent*>& lsMsg)
{
	//
	// Real mode:
	CaMessage* pMsgClass = NULL;
	long lCode;
	CString strMsgText;
	BOOL bOK = GetFirstIngCatMessage(pEntry->GetCategory(), lCode, strMsgText);
	while (bOK)
	{
		CaLoggedEvent* pMessage = new CaLoggedEvent();
		pMessage->m_strText = strMsgText;
		pMessage->SetCatType (pEntry->GetCategory());
		pMessage->SetCode (lCode);
		
		pMsgClass = IVM_LookupMessage (pEntry->GetCategory(), lCode);
		if (pMsgClass)
			pMessage->SetClass (pMsgClass->GetClass());
		else
			pMessage->SetClass (pEntry->GetClass());
		lsMsg.AddTail (pMessage);

		bOK = GetNextIngCatMessage(lCode, strMsgText);
	}
	return TRUE;
}


//
// Serialize the message entries classes {A|N|D}
// Serialize the user defined folders:
BOOL IVM_SerializeMessageSetting(CaMessageManager* pMessageMgr, CaCategoryDataUserManager* pUserFolder, BOOL bStoring)
{
	CString strFile = theApp.m_strMessageSettingsFile;
	//
	// Store the settings:
	if (!pMessageMgr || !pUserFolder)
		return FALSE;

	CFile f;
	if (bStoring)
	{
		if(f.Open (strFile, CFile::modeCreate|CFile::modeWrite))
		{
			CArchive ar(&f, CArchive::store);
			ar << theApp.m_strMSCATUSRSerializeVersionNumber;
			//
			// Message and Category Class (Alert, Notify, Discard):
			pMessageMgr->Serialize (ar);
			//
			// User specialized folders:
			pUserFolder->Serialize (ar);
			return TRUE;
		}
	}
	else
	{
		CFileStatus status;
		//
		// File does not exist:
		if (CFile::GetStatus(strFile, status) == 0)
			return TRUE;

		if(f.Open (strFile, CFile::modeRead))
		{
			CaMessageManager m;
			CArchive ar(&f, CArchive::load);
			CString strCheck;
			ar >> strCheck;
			if (theApp.m_strMSCATUSRSerializeVersionNumber.CompareNoCase (strCheck) != 0)
			{
				TRACE0 ("IVM_SerializeMessageSetting: nothing is loaded because of wrong version number !!! \n");
				CdMainDoc* pMainDoc = theApp.GetMainDocument();
				ASSERT (pMainDoc && IsWindow (pMainDoc->m_hwndMainFrame));
				if (pMainDoc && IsWindow (pMainDoc->m_hwndMainFrame))
					IVM_PostModelessMessageBox(IDS_W_MARKERCHANGED_CATEGORIES,pMainDoc->m_hwndMainFrame);
				return FALSE;
			}

			m.Serialize (ar);
#if defined (_DEBUG) && defined (SIMULATION)
			TRACE0 ("After Loading the message entry:\n");
			m.Output();
#endif

			//
			// Update the pMessageMgr from 'm':
			pMessageMgr->Update (m);
#if defined (_DEBUG) && defined (SIMULATION)
			TRACE0 ("After Updating the message entry:\n");
			pMessageMgr->Output();
#endif
			//
			// User specialized folders:
			pUserFolder->Serialize (ar);
			return TRUE;
		}
	}

	return FALSE;
}



void IVM_DichotomySort(CObArray& T, PFNEVENTCOMPARE compare, LPARAM lParamSort, CProgressCtrl* pCtrl)
{
	int nCount = (int)T.GetSize();
	if (T.GetSize() < 2)
		return;
	BOOL bStep = (pCtrl && IsWindow(pCtrl->m_hWnd))? TRUE: FALSE;
	CObject* e = NULL;
	int nStart = 1;
		
	if (bStep)
	{	pCtrl->SetRange(0, nCount);
		pCtrl->SetStep(1);
	}

	while (nStart < nCount)
	{
		e = T[nStart];
		T.RemoveAt(nStart);

		IVM_DichoInsertSort (T, 0, nStart-1, e, lParamSort, compare);
		nStart++;
		if (bStep)
			pCtrl->StepIt();
	}
}

int IVM_DichoInsertSort (CObArray& T, int nMin, int nMax, CObject* e, LPARAM sr, PFNEVENTCOMPARE compare)
{
	int nPos = IVM_GetDichoRange (T, nMin, nMax, e, sr, compare);
	int i = nMax;
	//
	// Use the advantage of programing langage to gain the performance, eg
	// to shift the element to the right, we use InsertAt instead of moving
	// element one by one:
	T.InsertAt (nPos, e);
	return nPos;
}

int IVM_GetDichoRange (CObArray& T, int nMin, int nMax, CObject* e, LPARAM sr, PFNEVENTCOMPARE compare)
{
	int nCmp, m;
	int left = nMin;
	int right= nMax;
	while (left <= right)
	{
		m = (left + right)/2;
		nCmp = compare ((LPARAM)e, (LPARAM)T[m], (LPARAM)sr);

		if (nCmp < 0)
		{
			right = m-1;
		}
		else
		{
			left = m+1;
		}
	}
	return left;
}



