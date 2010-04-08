/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : msgcateg.cpp, Implementation file 
**    Project  : Visual Manager
**    Author   : UK Sotheavut 
**    Purpose  : Define Message Categories and Notification Levels 
**
**  History:
**	21-May-1999 (uk$so01)
**	    Created
**	03-Feb-2000 (uk$so01)
**	    (BUG #100327)
**	    Move the data files of ivm to the directory II_SYSTEM\ingres\files.
**	31-May-2000 (uk$so01)
**	    bug 99242 Handle DBCS
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "msgcateg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

static void PlacerPivot (CTypedPtrArray <CObArray, CaMessageEntry*>& T, int I, int J, int& K)
{
	int L;
	CaMessageEntry* TS;
	L = I;
	K = J;
	while  (L <= K) 
	{
		while (K >= L && T[K]->GetCategory() >  T[I]->GetCategory())
			K--;
		while (L <= K && T[L]->GetCategory() <= T[I]->GetCategory())
			L++;
		if (L < K ) 
		{
			TS   =T[L];
			T[L] =T[K];
			T[K] =TS  ;
			K--;
		}
	}

	TS  =T[I];
	T[I]=T[K];
	T[K]=TS  ;
}

static void TriQuickSort(CTypedPtrArray <CObArray, CaMessageEntry*>& T, int I, int J)
{
	int K;
	if (I < J) 
	{
		PlacerPivot ( T, I, J, K);
		TriQuickSort( T, I, K-1 );
		TriQuickSort( T, K+1, J );
	}
}

static int Find (long nCategory, CTypedPtrArray <CObArray, CaMessageEntry*>& T, int first, int last)
{
	if (first <= last)
	{
		int mid = (first + last) / 2;
		if (T [mid]->GetCategory() == nCategory)
			return mid;
		else
		if (T [mid]->GetCategory() > nCategory)
			return Find (nCategory, T, first, mid-1);
		else
			return Find (nCategory, T, mid+1, last);
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////
// CLASS: CaMessage
//////////////////////////////////////////////////////////////////////
IMPLEMENT_SERIAL (CaMessage, CObject, 1)
CaMessage::CaMessage()
{
	m_strTitle = _T("");
	m_lCode = -1;
	m_lCategory = -1;
	m_msgClass = IMSG_NOTIFY;
}

CaMessage::CaMessage(LPCTSTR lpszTitle, long lCode, long lCat, Imsgclass msgClass)
{
	m_strTitle = lpszTitle;
	m_lCode = lCode;
	m_lCategory = lCat;
	m_msgClass = msgClass;
}


CaMessage::~CaMessage()
{

}

void CaMessage::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strTitle;
		ar << m_lCode;
		ar << m_lCategory;
		ar << (int)m_msgClass;
	}
	else
	{
		int nClass;
		ar >> m_strTitle;
		ar >> m_lCode;
		ar >> m_lCategory;
		ar >> nClass; m_msgClass = Imsgclass(nClass);
	}
}


CaMessage* CaMessage::Clone()
{
	CaMessage* pNewItem = NULL;
	try
	{
		pNewItem = new CaMessage();
		pNewItem->SetTitle(m_strTitle);
		pNewItem->SetCode(m_lCode);
		pNewItem->SetCategory(m_lCategory);
		pNewItem->SetClass(m_msgClass);
	}
	catch (...)
	{
		return NULL;
	}
	return pNewItem;
}


//////////////////////////////////////////////////////////////////////
// CLASS: CaMessageEntry
//////////////////////////////////////////////////////////////////////
IMPLEMENT_SERIAL (CaMessageEntry, CaMessage, 1)
CaMessageEntry::CaMessageEntry():CaMessage()
{
	m_strTitleIngresCategory = _T("");
}

CaMessageEntry::CaMessageEntry(LPCTSTR lpszTitle, long lCode, long lCat, Imsgclass msgClass)
	:CaMessage(lpszTitle, lCode, lCat, msgClass)
{
	m_strTitleIngresCategory = _T("");
}

CaMessageEntry::~CaMessageEntry()
{
	while (!m_listMessage.IsEmpty())
		delete m_listMessage.RemoveHead();
}


void CaMessageEntry::Serialize (CArchive& ar)
{
	CaMessage::Serialize(ar);
	m_listMessage.Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_strTitleIngresCategory;
	}
	else
	{
		ar >> m_strTitleIngresCategory;
	}
}


CaMessage* CaMessageEntry::Add (CaMessage* pNewMessage)
{
	CaMessage* pMessage = NULL;
	POSITION pos = m_listMessage.GetHeadPosition();
	while (pos != NULL)
	{
		pMessage = m_listMessage.GetNext (pos);
		if (pMessage->GetCategory() == pNewMessage->GetCategory() &&
			pMessage->GetCode() == pNewMessage->GetCode())
		{
			delete pNewMessage;
			return pMessage;
		}
	}
	m_listMessage.AddTail (pNewMessage);
	return pNewMessage;
}

void CaMessageEntry::Remove (long nID)
{
	CaMessage* pMsg = NULL;
	POSITION p, pos = m_listMessage.GetHeadPosition();

	while (pos != NULL)
	{
		p = pos;
		pMsg = m_listMessage.GetNext (pos);

		if (pMsg->GetCode() == nID)
		{
			m_listMessage.RemoveAt (p);
			delete pMsg;
			break;
		}
	}
}


void CaMessageEntry::Remove (CaMessage* pMessage)
{
	CaMessage* pMsg = NULL;
	POSITION p, pos = m_listMessage.GetHeadPosition();

	while (pos != NULL)
	{
		p = pos;
		pMsg = m_listMessage.GetNext (pos);

		if (pMsg == pMessage && pMsg->GetCode() == pMessage->GetCode())
		{
			m_listMessage.RemoveAt (p);
			delete pMsg;
			break;
		}
	}
}

CaMessage* CaMessageEntry::Search (long nID)
{
	CaMessage* pMsg = NULL;
	POSITION pos = m_listMessage.GetHeadPosition();

	while (pos != NULL)
	{
		pMsg = m_listMessage.GetNext (pos);

		if (pMsg->GetCode() == nID)
		{
			return pMsg;
		}
	}
	return NULL;
}

CaMessageEntry* CaMessageEntry::Clone()
{
	CaMessageEntry* pNewItem = NULL;
	try
	{
		CaMessage* pMessage = NULL;
		CaMessage* pNewMessage = NULL;
		//
		// From CaMessage:
		pNewItem = new CaMessageEntry();
		pNewItem->SetCategory(m_lCategory);
		pNewItem->SetCode(m_lCode);
		pNewItem->SetClass(m_msgClass);
		pNewItem->SetTitle(m_strTitle);
		pNewItem->SetIngresCategoryTitle(m_strTitleIngresCategory);

		//
		// From the list of user specialized messages:
		CTypedPtrList<CObList, CaMessage*>& lMessage = pNewItem->GetListMessage();
		POSITION pos = m_listMessage.GetHeadPosition();
		while (pos != NULL)
		{
			pMessage = m_listMessage.GetNext (pos);
			pNewMessage = pMessage->Clone();
			if (pNewMessage)
				lMessage.AddTail (pNewMessage);
		}
	}
	catch (...)
	{
		return NULL;
	}
	return pNewItem;
}




//////////////////////////////////////////////////////////////////////
// CLASS: CaMessageManager
//////////////////////////////////////////////////////////////////////
IMPLEMENT_SERIAL (CaMessageManager, CObject, 1)
CaMessageManager::CaMessageManager()
{

}


CaMessageManager::~CaMessageManager()
{
	INT_PTR nSize = m_arrayMsgEntry.GetSize();
	for (int i=0; i<nSize; i++)
	{
		CaMessageEntry* pObj = m_arrayMsgEntry.GetAt(i);
		delete pObj;
	}
	m_arrayMsgEntry.RemoveAll();
}


void CaMessageManager::Serialize (CArchive& ar)
{
	m_arrayMsgEntry.Serialize (ar);
}

//
// Update this from m:
void CaMessageManager::Update (CaMessageManager& m)
{
	//
	// Update the message Entry:
	CaMessageEntry* pObjThis = NULL;
	CaMessageEntry* pObjFrom = NULL;
	CTypedPtrArray <CObArray, CaMessageEntry*>& ml = m.GetListEntry();
	int i;
	INT_PTR nSize = ml.GetSize();
	if (nSize == 0)
		return;

	for (i=0; i<nSize; i++)
	{
		pObjFrom = ml[i];
		if (pObjFrom)
		{
			pObjThis = FindEntry (pObjFrom->GetCategory());
			if (!pObjThis)
				continue;
			//
			// Update the state of message entry:
			pObjThis->SetClass (pObjFrom->GetClass());

			//
			// Remove those user specialized messages if they are not in 'm':
			CaMessage* pMsgThis = NULL;
			CTypedPtrList<CObList, CaMessage*>& lmsgThis = pObjThis->GetListMessage();
			POSITION p, pos = lmsgThis.GetHeadPosition();
			while (pos != NULL)
			{
				p = pos;
				pMsgThis = lmsgThis.GetNext (pos);
				if (!pObjFrom->Search(pMsgThis->GetCode()))
				{
					lmsgThis.RemoveAt (p);
					delete pMsgThis;
				}
			}

			//
			// Update the state of user specialized messages:
			CTypedPtrList<CObList, CaMessage*>& lmsgFrom = pObjFrom->GetListMessage();
			CaMessage* pMsgFrom = NULL;
			pos = lmsgFrom.GetHeadPosition();
			while (pos != NULL)
			{
				pMsgFrom = lmsgFrom.GetNext (pos);
				pMsgThis = pObjThis->Search (pMsgFrom->GetCode());
				if (pMsgThis)
					pMsgThis->SetClass (pMsgFrom->GetClass());
				else
				{
					CaMessage* pNewClone = pMsgFrom->Clone();
					if (pNewClone)
						lmsgThis.AddTail (pNewClone);
				}
			}
		}
	}
}


CaMessageEntry* CaMessageManager::FindEntry (long nCategory)
{
	INT_PTR nSize = m_arrayMsgEntry.GetSize();
	if (nSize == 0)
		return NULL;
	int idx = Find (nCategory, m_arrayMsgEntry, 0, (int)(m_arrayMsgEntry.GetSize()-1));
	if (idx != -1)
		return m_arrayMsgEntry[idx];
	return NULL;
}


CaMessage* CaMessageManager::Add (long nCategory, CaMessage* pNewMessage)
{
	//
	// Find if the entry of 'nCategory' exist:
	CaMessageEntry* pEntry = FindEntry (nCategory);
	CString strMsgDisplay;

	if (!pEntry)
	{
		//
		// The message becomes a Message Entry:
		CString strMsg;
		if (!strMsg.LoadString(IDS_MSG_UNCLASSIFY_XXX))
			strMsg = _T("Unclassified %s Messages");
		strMsgDisplay.Format (strMsg, (LPCTSTR)pNewMessage->GetTitle());
		pEntry = new CaMessageEntry();
		pEntry->SetCategory (nCategory);
		pEntry->SetCode (pNewMessage->GetCode());
		pEntry->SetClass (pNewMessage->GetClass());
		pEntry->SetTitle (strMsgDisplay);
		pEntry->SetIngresCategoryTitle (pNewMessage->GetTitle());

		delete pNewMessage;
		m_arrayMsgEntry.Add (pEntry);
		TriQuickSort(m_arrayMsgEntry, 0, (int)(m_arrayMsgEntry.GetSize()-1));
		return (CaMessage*)pEntry;
	}
	//
	// The message is a user specialized message:
	return pEntry->Add (pNewMessage);
}

void CaMessageManager::Remove (long nCategory, long nID)
{
	CaMessageEntry* pEntry = FindEntry (nCategory);
	ASSERT (pEntry);
	if (pEntry)
	{
		pEntry->Remove (nID);
	}
}

void CaMessageManager::Remove (CaMessage* pMessage)
{
	long nCategory = pMessage->GetCategory();
	CaMessageEntry* pEntry = FindEntry (nCategory);
	ASSERT (pEntry);
	if (pEntry)
	{
		pEntry->Remove (pMessage->GetCode());
	}
}


CaMessage* CaMessageManager::Search (long nCategory, long nID)
{
	CaMessageEntry* pEntry = FindEntry (nCategory);
	if (!pEntry)
		return NULL;
	return pEntry->Search(nID);
}


CaMessageManager* CaMessageManager::Clone()
{
	CaMessageManager* pNewItem = NULL;
	try
	{
		CaMessageEntry* pEntry = NULL;
		CaMessageEntry* pNewEntry = NULL;
		pNewItem = new CaMessageManager();
		CTypedPtrArray <CObArray, CaMessageEntry*>& lEntry = pNewItem->GetListEntry();
		
		int i;
		INT_PTR nSize = m_arrayMsgEntry.GetSize();
		for (i=0; i<nSize; i++)
		{
			pEntry = m_arrayMsgEntry[i];
			pNewEntry = pEntry->Clone();
			if (pNewEntry)
				lEntry.Add(pNewEntry);
		}
	}
	catch (...)
	{
		return NULL;
	}
	return pNewItem;
}


void CaMessageManager::Output()
{
#if defined (_DEBUG)
	CaMessageEntry* pEntry = NULL;
	CaMessage* pMsg = NULL;
	int i, nCount = m_arrayMsgEntry.GetSize();
	for (i=0; i<nCount; i++)
	{
		pEntry = m_arrayMsgEntry.GetAt (i);
		TRACE2 ("Entry [%06d] = %06d\n", i, pEntry->GetCategory());
		CTypedPtrList<CObList, CaMessage*>& l = pEntry->GetListMessage();
		if (!l.IsEmpty())
		{
			POSITION pos = l.GetHeadPosition();
			while (pos != NULL)
			{
				pMsg = l.GetNext (pos);
				TRACE2 ("\tMessage<%05d>: %s\n", pMsg->GetCode(), pMsg->GetTitle());
			}
		}
	}
#endif
}



// *****************************************

//
// Serializeable user defined categories:
IMPLEMENT_SERIAL (CaCategoryDataUser, CObject, 1)
CaCategoryDataUser::CaCategoryDataUser()
{
	m_bFolder = FALSE;
}

CaCategoryDataUser::CaCategoryDataUser(long nCat, long nCode, BOOL bFolder, LPCTSTR lpszPath)
{
	m_bFolder = bFolder;
	m_strPath = lpszPath;
	m_nCategory = nCat;
	m_nMsgCode = nCode;
}

CaCategoryDataUser::~CaCategoryDataUser()
{

}

void CaCategoryDataUser::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strPath;
		ar << m_bFolder;
		ar << m_nCategory;
		ar << m_nMsgCode;
	}
	else
	{
		ar >> m_strPath;
		ar >> m_bFolder;
		ar >> m_nCategory;
		ar >> m_nMsgCode;
	}
}


IMPLEMENT_SERIAL (CaCategoryDataUserManager, CObject, 1)
CaCategoryDataUserManager::CaCategoryDataUserManager()
{
#if defined (_DEBUG) && defined (SIMULATION)
	m_userSpecializedFile = _T("usrcateg.dat");
	m_bSimulate = TRUE;
#else
	m_userSpecializedFile = theApp.m_strArchiveEventFile;
	m_bSimulate = FALSE;
#endif
}

CaCategoryDataUserManager::~CaCategoryDataUserManager()
{
	while (!m_listUserFolder.IsEmpty())
		delete m_listUserFolder.RemoveHead();
}

void CaCategoryDataUserManager::Serialize (CArchive& ar)
{
	m_listUserFolder.Serialize (ar);
}


/*
BOOL CaCategoryDataUserManager::LoadCategory()
{
	if (m_bSimulate)
		IVM_LoadUserCategory (m_listUserFolder, m_userSpecializedFile);
	else
	{
		CFile f;
		if(f.Open (m_userSpecializedFile, CFile::modeRead))
		{
			CArchive ar(&f, CArchive::load);
			Serialize (ar);
		}
	}
	return TRUE;
}
*/

/*
BOOL CaCategoryDataUserManager::SaveCategory()
{
	if (m_bSimulate)
		IVM_SaveUserCategory (m_listUserFolder, m_userSpecializedFile);
	else
	{
		CFile f;
		if(f.Open (m_userSpecializedFile, CFile::modeCreate|CFile::modeWrite))
		{
			CArchive ar(&f, CArchive::store);
			Serialize (ar);
		}
	}
	return TRUE;
}
*/

void CaCategoryDataUserManager::Output()
{
#if defined (_DEBUG)
	CaCategoryDataUser* pObj = NULL;
	POSITION pos = m_listUserFolder.GetHeadPosition();
	CString strItem;

	while (pos != NULL)
	{
		pObj = m_listUserFolder.GetNext (pos);
		strItem.Format (
			_T("C=%d N=%d F=%d %s\n"), 
			pObj->GetCategory(),
			pObj->GetCode(),
			pObj->IsFolder(),
			pObj->GetPath());
		TRACE1 ("%s", (LPCTSTR)strItem);
	}
#endif
}

